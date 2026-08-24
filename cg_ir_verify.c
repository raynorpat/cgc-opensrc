/****************************************************************************\
Copyright (c) 2002, NVIDIA Corporation.

NVIDIA Corporation("NVIDIA") supplies this software to you in
consideration of your agreement to the following terms, and your use,
installation, modification or redistribution of this NVIDIA software
constitutes acceptance of these terms.  If you do not agree with these
terms, please do not use, install, modify or redistribute this NVIDIA
software.

In consideration of your agreement to abide by the following terms, and
subject to these terms, NVIDIA grants you a personal, non-exclusive
license, under NVIDIA's copyrights in this original NVIDIA software (the
"NVIDIA Software"), to use, reproduce, modify and redistribute the
NVIDIA Software, with or without modifications, in source and/or binary
forms; provided that if you redistribute the NVIDIA Software, you must
retain the copyright notice of NVIDIA, this notice and the following
text and disclaimers in all such redistributions of the NVIDIA Software.
Neither the name, trademarks, service marks nor logos of NVIDIA
Corporation may be used to endorse or promote products derived from the
NVIDIA Software without specific prior written permission from NVIDIA.
Except as expressly stated in this notice, no other rights or licenses
express or implied, are granted by NVIDIA herein, including but not
limited to any patent rights that may be infringed by your derivative
works or by other works in which the NVIDIA Software may be
incorporated. No hardware is licensed hereunder.

THE NVIDIA SOFTWARE IS BEING PROVIDED ON AN "AS IS" BASIS, WITHOUT
WARRANTIES OR CONDITIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED,
INCLUDING WITHOUT LIMITATION, WARRANTIES OR CONDITIONS OF TITLE,
NON-INFRINGEMENT, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL NVIDIA BE LIABLE FOR ANY SPECIAL,
INDIRECT, INCIDENTAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
\****************************************************************************/

//
// cg_ir_verify.c - Standalone Cg IR invariant checker.  Runs once per
//        module immediately after construction and before any HAL
//        callback, walking declarations and functions in source order
//        while tracking the current function, loop depth, and declared
//        symbol identities.  Verification stops at the first invariant
//        failure so release builds produce one controlled internal
//        diagnostic; assertion-enabled builds abort at the failure site
//        instead.
//
// Visibility authority is the set of declared identities: module
// globals, function parameters, and DECL statements.  A function's
// "locals" list mirrors emission order and is checked for shape only.
// Scopes are tracked in a fixed-size binding stack; exhausting it is
// itself an ownership failure rather than silent truncation.
//

#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "slglobals.h"
#include "cg_ir.h"

/////////////////////////// Verification context ///////////////////////////

/* Visible-symbol capacity: well over any lowered program's live scopes. */

#define CG_IR_MAX_BINDINGS 1024

typedef struct CgIRBinding_Rec {
    Symbol *symbol;
    int depth;
} CgIRBinding;

typedef struct CgIRVerifyContext_Rec {
    const CgIRModule *module;
    CgIRVerifyDiagnostic *diagnostic;
    const CgIRFunction *function;
    int loopDepth;
    int scopeDepth;
    int bindingCount;
    CgIRBinding bindings[CG_IR_MAX_BINDINGS];
} CgIRVerifyContext;

/*
 * CgIRFail() - Record the one controlled internal diagnostic and stop.
 *          Assertion-enabled builds never reach the diagnostic: the
 *          invariant violation aborts right here.
 */

static int CgIRFail(CgIRVerifyContext *ctx, CgIRVerifyReason reason,
                   SourceLoc loc, const void *node)
{
    assert(!"Cg IR invariant violated");
    if (ctx->diagnostic != NULL) {
        ctx->diagnostic->reason = reason;
        ctx->diagnostic->loc = loc;
        ctx->diagnostic->node = node;
    }
    return 0;
} // CgIRFail

const char *CgIRVerifyReasonName(CgIRVerifyReason reason)
{
    switch (reason) {
    case CGIR_VERIFY_OK:
        return "ok";
    case CGIR_VERIFY_TYPE:
        return "type";
    case CGIR_VERIFY_OWNER:
        return "owner";
    case CGIR_VERIFY_OPERAND:
        return "operand";
    case CGIR_VERIFY_LVALUE:
        return "lvalue";
    case CGIR_VERIFY_CALL:
        return "call";
    case CGIR_VERIFY_INTRINSIC:
        return "intrinsic";
    case CGIR_VERIFY_CONTROL:
        return "control";
    case CGIR_VERIFY_INTERFACE:
        return "interface";
    case CGIR_VERIFY_LOCATION:
        return "location";
    default:
        return "<invalid>";
    }
} // CgIRVerifyReasonName

///////////////////////////// Type classification ///////////////////////////

/*
 * Scalar-kind gates.  Conversion-valid kinds exclude only the NONE and
 * UNDEFINED placeholders; arithmetic additionally excludes Boolean;
 * integral delegates to the canonical traits.
 */

static int lKnownKind(CgScalarKind kind)
{
    return kind > CG_SCALAR_UNDEFINED && kind < CG_SCALAR_COUNT;
} // lKnownKind

static int lArithmeticKind(CgScalarKind kind)
{
    return lKnownKind(kind) && kind != CG_SCALAR_BOOL;
} // lArithmeticKind

static int lIntegralKind(CgScalarKind kind)
{
    return lKnownKind(kind) && CgScalarIsIntegral(kind);
} // lIntegralKind

/*
 * lIsBooleanType() - Any Boolean scalar or vector, matching the
 *          language's Boolean-expression rules (discards accept
 *         vectors; conditions stay scalar).
 */

static int lIsBooleanType(const Type *type)
{
    if (!IsBoolean(type))
        return 0;
    return IsScalar(type) || IsVector(type, NULL);
} // lIsBooleanType

/*
 * lShapeOf() - Component geometry: scalars are 1x1, vectors len x 1,
 *          matrices rows x cols.  Everything else has no shape.
 */

typedef struct CgIRShape_Rec {
    int rows;
    int cols;
} CgIRShape;

static int lShapeOf(const Type *type, CgIRShape *shape)
{
    int first, second;

    if (IsScalar(type)) {
        shape->rows = 1;
        shape->cols = 1;
        return 1;
    }
    if (IsVector(type, &first)) {
        shape->rows = first;
        shape->cols = 1;
        return 1;
    }
    if (IsMatrix(type, &first, &second)) {
        shape->cols = first;
        shape->rows = second;
        return 1;
    }
    return 0;
} // lShapeOf

static int lComponentCount(const Type *type)
{
    CgIRShape shape;

    if (!lShapeOf(type, &shape))
        return 0;
    return shape.rows * shape.cols;
} // lComponentCount

/*
 * lShapesCompatible() - Componentwise binary compatibility: scalars
 *          mix with anything, everything else needs equal geometry.
 */

static int lShapesCompatible(const CgIRShape *left, const CgIRShape *right)
{
    if ((left->rows == 1 && left->cols == 1) ||
        (right->rows == 1 && right->cols == 1))
    {
        return 1;
    }
    return left->rows == right->rows && left->cols == right->cols;
} // lShapesCompatible

/*
 * lPromotedShape() - Componentwise result geometry.
 */

static void lPromotedShape(const CgIRShape *left, const CgIRShape *right,
                           CgIRShape *result)
{
    result->rows = left->rows > right->rows ? left->rows : right->rows;
    result->cols = left->cols > right->cols ? left->cols : right->cols;
} // lPromotedShape

/*
 * lStandardShapeType() - The interned standard type for a scalar kind
 *          plus component geometry, or UndefinedType outside the
 *          registry.
 */

static Type *lStandardShapeType(CgScalarKind kind, const CgIRShape *shape)
{
    if (shape->rows == 1 && shape->cols == 1)
        return GetStandardTypeKind(kind, 0, 0);
    if (shape->cols == 1)
        return GetStandardTypeKind(kind, shape->rows, 0);
    return GetStandardTypeKind(kind, shape->rows, shape->cols);
} // lStandardShapeType

/*
 * lCanonicalType() - Resolved, canonical types only.  Scalars and
 *          packed shapes must agree with their interned registry entry
 *          modulo qualifiers; unpacked arrays carry a positive or
 *          unsized element count; remaining categories are their own
 *          identity.
 */

static int lCanonicalType(const Type *type)
{
    Type *interned;
    int len, rows, cols;

    if (type == NULL || type == UndefinedType)
        return 0;
    if (IsVoid(type))
        return 1;
    switch (GetCategory(type)) {
    case TYPE_CATEGORY_SCALAR:
        interned = GetStandardTypeKind(GetScalarKind(type), 0, 0);
        return interned != UndefinedType &&
               IsSameUnqualifiedType(type, interned);
    case TYPE_CATEGORY_ARRAY:
        if (IsVector(type, &len)) {
            interned = GetStandardTypeKind(GetScalarKind(type), len, 0);
            return len >= 1 && len <= 4 && interned != UndefinedType &&
                   IsSameUnqualifiedType(type, interned);
        }
        if (IsMatrix(type, &cols, &rows)) {
            interned = GetStandardTypeKind(GetScalarKind(type), rows, cols);
            return rows >= 1 && rows <= 4 && cols >= 1 && cols <= 4 &&
                   interned != UndefinedType &&
                   IsSameUnqualifiedType(type, interned);
        }
        return type->arr.numels > 0 || type->arr.numels == CG_ARRAY_UNSIZED;
    case TYPE_CATEGORY_FUNCTION:
    case TYPE_CATEGORY_STRUCT:
    case TYPE_CATEGORY_CONNECTOR:
    case TYPE_CATEGORY_SAMPLER:
    case TYPE_CATEGORY_INTERFACE:
        return 1;
    default:
        return 0;
    }
} // lCanonicalType

///////////////////////////////// Walk state //////////////////////////////////

static int lScopeMark(const CgIRVerifyContext *ctx)
{
    return ctx->bindingCount;
} // lScopeMark

static void lScopeRelease(CgIRVerifyContext *ctx, int mark)
{
    ctx->bindingCount = mark;
} // lScopeRelease

/*
 * lResolveSymbol() - Membership in the visible-identity chain.
 */

static int lResolveSymbol(CgIRVerifyContext *ctx, Symbol *symbol,
                          SourceLoc loc)
{
    int index;

    if (symbol != NULL) {
        for (index = ctx->bindingCount - 1; index >= 0; index--) {
            if (ctx->bindings[index].symbol == symbol)
                return 1;
        }
    }
    return CgIRFail(ctx, CGIR_VERIFY_OWNER, loc, symbol);
} // lResolveSymbol

/*
 * lDeclareSymbol() - Register one declaration at the current scope
 *          depth.  Redeclaring one identity in a single scope is an
 *          ownership violation; distinct identities shadow freely.
 */

static int lDeclareSymbol(CgIRVerifyContext *ctx, Symbol *symbol,
                          SourceLoc loc)
{
    int index;

    if (symbol == NULL)
        return 1;
    for (index = ctx->bindingCount - 1; index >= 0; index--) {
        if (ctx->bindings[index].depth < ctx->scopeDepth)
            break;
        if (ctx->bindings[index].symbol == symbol)
            return CgIRFail(ctx, CGIR_VERIFY_OWNER, loc, symbol);
    }
    if (ctx->bindingCount >= CG_IR_MAX_BINDINGS)
        return CgIRFail(ctx, CGIR_VERIFY_OWNER, loc, symbol);
    ctx->bindings[ctx->bindingCount].symbol = symbol;
    ctx->bindings[ctx->bindingCount].depth = ctx->scopeDepth;
    ctx->bindingCount++;
    return 1;
} // lDeclareSymbol

////////////////////////////// Expression rules ///////////////////////////////

static int lVerifyExpr(CgIRVerifyContext *ctx, const CgIRExpr *expr);

/*
 * lRequire() - Shared per-node gates: canonical type and coherent
 *          location provenance (a set synthesized flag demands a real
 *          location; the all-zero location with a clear flag is the
 *          documented conventional-synthesis encoding).
 */

static int lRequireNodeBasics(CgIRVerifyContext *ctx, const CgIRExpr *expr)
{
    if (!lCanonicalType(expr->type))
        return CgIRFail(ctx, CGIR_VERIFY_TYPE, expr->loc, expr);
    if (expr->synthesized && expr->loc.file == 0 && expr->loc.line == 0)
        return CgIRFail(ctx, CGIR_VERIFY_LOCATION, expr->loc, expr);
    return 1;
} // lRequireNodeBasics

/*
 * lSwizzleComponentsAgree() - The mask must address existing components
 *          of the object; read swizzles may repeat components freely.
 */

static int lSwizzleComponentsAgree(CgIRVerifyContext *ctx,
                                  const CgIRExpr *expr, int objectLen)
{
    int index, component;

    if (expr->u.swizzle.componentCount < 1 ||
        expr->u.swizzle.componentCount > 4)
    {
        return CgIRFail(ctx, CGIR_VERIFY_OPERAND, expr->loc, expr);
    }
    if (expr->u.swizzle.mask < 0 || expr->u.swizzle.mask > 0xFF)
        return CgIRFail(ctx, CGIR_VERIFY_OPERAND, expr->loc, expr);
    for (index = 0; index < expr->u.swizzle.componentCount; index++) {
        component = (expr->u.swizzle.mask >> (2 * index)) & 0x3;
        if (component >= objectLen)
            return CgIRFail(ctx, CGIR_VERIFY_OPERAND, expr->loc, expr);
    }
    return 1;
} // lSwizzleComponentsAgree

/*
 * lWriteMaskUnique() - A swizzle used as an assignment target is a
 *          write mask: repeated components would race one store.
 */

static int lWriteMaskUnique(CgIRVerifyContext *ctx, const CgIRExpr *target)
{
    int seen[4];
    int index, component;

    seen[0] = seen[1] = seen[2] = seen[3] = 0;
    for (index = 0; index < target->u.swizzle.componentCount; index++) {
        component = (target->u.swizzle.mask >> (2 * index)) & 0x3;
        if (seen[component])
            return CgIRFail(ctx, CGIR_VERIFY_LVALUE, target->loc, target);
        seen[component] = 1;
    }
    return 1;
} // lWriteMaskUnique

static int lVerifyBinary(CgIRVerifyContext *ctx, const CgIRExpr *expr)
{
    const CgIRExpr *left = expr->u.binary.left;
    const CgIRExpr *right = expr->u.binary.right;
    Type *usual, *expected;
    CgScalarKind kind;
    CgIRShape leftShape, rightShape, resultShape;
    int op = expr->u.binary.op;
    int multiply = op == CGIR_OP_MULTIPLY;
    int comparison = op >= CGIR_OP_LESS && op <= CGIR_OP_NOT_EQUAL;
    int logical = op == CGIR_OP_LOGICAL_AND || op == CGIR_OP_LOGICAL_OR;
    int bitwise = (op >= CGIR_OP_SHIFT_LEFT && op <= CGIR_OP_SHIFT_RIGHT) ||
                  (op >= CGIR_OP_BITWISE_AND && op <= CGIR_OP_BITWISE_OR);

    if (left == NULL || right == NULL)
        return CgIRFail(ctx, CGIR_VERIFY_OPERAND, expr->loc, expr);
    if (!lVerifyExpr(ctx, left) || !lVerifyExpr(ctx, right))
        return 0;
    if (!lShapeOf(left->type, &leftShape) || !lShapeOf(right->type, &rightShape))
        return CgIRFail(ctx, CGIR_VERIFY_OPERAND, expr->loc, expr);

    if (comparison) {
        if (!lKnownKind(GetScalarKind(left->type)) ||
            !lKnownKind(GetScalarKind(right->type)))
        {
            return CgIRFail(ctx, CGIR_VERIFY_OPERAND, expr->loc, expr);
        }
        if (!lShapesCompatible(&leftShape, &rightShape))
            return CgIRFail(ctx, CGIR_VERIFY_OPERAND, expr->loc, expr);
        lPromotedShape(&leftShape, &rightShape, &resultShape);
        expected = lStandardShapeType(CG_SCALAR_BOOL, &resultShape);
    } else if (logical) {
        if (!lIsBooleanType(left->type) || !lIsBooleanType(right->type))
            return CgIRFail(ctx, CGIR_VERIFY_OPERAND, expr->loc, expr);
        if (!lShapesCompatible(&leftShape, &rightShape))
            return CgIRFail(ctx, CGIR_VERIFY_OPERAND, expr->loc, expr);
        lPromotedShape(&leftShape, &rightShape, &resultShape);
        expected = lStandardShapeType(CG_SCALAR_BOOL, &resultShape);
    } else if (bitwise) {
        if (!lIntegralKind(GetScalarKind(left->type)) ||
            !lIntegralKind(GetScalarKind(right->type)))
        {
            return CgIRFail(ctx, CGIR_VERIFY_OPERAND, expr->loc, expr);
        }
        if (!lShapesCompatible(&leftShape, &rightShape))
            return CgIRFail(ctx, CGIR_VERIFY_OPERAND, expr->loc, expr);
        usual = CgUsualArithmeticType(left->type, right->type);
        if (usual == NULL)
            return CgIRFail(ctx, CGIR_VERIFY_OPERAND, expr->loc, expr);
        lPromotedShape(&leftShape, &rightShape, &resultShape);
        expected = lStandardShapeType(GetScalarKind(usual), &resultShape);
    } else {
        /* Arithmetic family: MULTIPLY carries matrix/vector products,
         * everything else is componentwise. */
        if (!lArithmeticKind(GetScalarKind(left->type)) ||
            !lArithmeticKind(GetScalarKind(right->type)))
        {
            return CgIRFail(ctx, CGIR_VERIFY_OPERAND, expr->loc, expr);
        }
        if (multiply && leftShape.cols != 1 && rightShape.rows == 1 &&
            rightShape.cols == 1 && leftShape.cols == rightShape.rows)
        {
            /* M * V: columns of the matrix meet the vector length. */
            resultShape.rows = leftShape.rows;
            resultShape.cols = 1;
        } else if (multiply && leftShape.cols == 1 && leftShape.rows != 1 &&
                   rightShape.cols != 1 &&
                   leftShape.rows == rightShape.rows)
        {
            /* V * M: the vector length meets the matrix rows. */
            resultShape.rows = 1;
            resultShape.cols = rightShape.cols;
        } else if (multiply && leftShape.cols != 1 &&
                   rightShape.cols != 1 &&
                   leftShape.cols == rightShape.rows)
        {
            /* M * M: inner dimensions contract. */
            resultShape.rows = leftShape.rows;
            resultShape.cols = rightShape.cols;
        } else {
            if (op == CGIR_OP_MODULO &&
                (!lIntegralKind(GetScalarKind(left->type)) ||
                 !lIntegralKind(GetScalarKind(right->type))))
            {
                return CgIRFail(ctx, CGIR_VERIFY_OPERAND, expr->loc, expr);
            }
            if (!lShapesCompatible(&leftShape, &rightShape))
                return CgIRFail(ctx, CGIR_VERIFY_OPERAND, expr->loc, expr);
            lPromotedShape(&leftShape, &rightShape, &resultShape);
        }
        usual = CgUsualArithmeticType(left->type, right->type);
        if (usual == NULL)
            return CgIRFail(ctx, CGIR_VERIFY_OPERAND, expr->loc, expr);
        kind = GetScalarKind(usual);
        if (!lKnownKind(kind))
            return CgIRFail(ctx, CGIR_VERIFY_OPERAND, expr->loc, expr);
        expected = lStandardShapeType(kind, &resultShape);
    }

    if (expected == NULL || expected == UndefinedType ||
        !IsSameUnqualifiedType(expr->type, expected))
    {
        return CgIRFail(ctx, CGIR_VERIFY_TYPE, expr->loc, expr);
    }
    return 1;
} // lVerifyBinary

/*
 * lVerifyCallArguments() - Shared arity/direction/type agreement for
 *          ordinary calls and interface dispatch.  Out and inout
 *          formals need an lvalue actual of the same unqualified type;
 *          in formals accept implicit conversions.
 */

static int lVerifyCallArguments(CgIRVerifyContext *ctx,
                               const CgIRExpr *call, CgIRVerifyReason reason,
                               const CgIRDecl *formals,
                               const Symbol *formalSymbols,
                               CgIRExpr *arguments)
{
    CgIRExpr *actual;
    const CgIRDecl *formalDecl;
    const Symbol *formalSymb;
    CgConversionRank rank;
    int actuals, formalsCount;

    actuals = 0;
    for (actual = arguments; actual != NULL; actual = actual->next) {
        if (!lVerifyExpr(ctx, actual))
            return 0;
        actuals++;
    }
    formalsCount = 0;
    formalDecl = formals;
    formalSymb = formalSymbols;
    while (formalDecl != NULL || formalSymb != NULL) {
        Type *formalType;
        int direction;

        if (formalDecl != NULL) {
            formalType = formalDecl->type;
            formalDecl = formalDecl->next;
        } else {
            formalType = formalSymb->type;
            formalSymb = formalSymb->next;
        }
        formalsCount++;
        if (formalType == NULL)
            return CgIRFail(ctx, reason, call->loc, call);
        actual = arguments;
        {
            int index;

            for (index = 1; index < formalsCount && actual != NULL; index++)
                actual = actual->next;
        }
        if (actual == NULL)
            return CgIRFail(ctx, reason, call->loc, call);
        direction = formalType->properties & TYPE_QUALIFIER_OUT;
        if (direction) {
            if (!actual->isLvalue)
                return CgIRFail(ctx, reason, call->loc, call);
            if (!IsSameUnqualifiedType(actual->type, formalType))
                return CgIRFail(ctx, reason, call->loc, call);
        } else {
            rank = CgClassifyConversion(actual->type, formalType, 0);
            if (rank < CG_CONVERSION_IMPLICIT_WARN)
                return CgIRFail(ctx, reason, call->loc, call);
        }
    }
    if (actuals != formalsCount)
        return CgIRFail(ctx, reason, call->loc, call);
    return 1;
} // lVerifyCallArguments

static int lVerifyExpr(CgIRVerifyContext *ctx, const CgIRExpr *expr)
{
    Symbol *memberCursor;
    const CgIRExpr *cursor;
    const CgIRFunction *callee;
    const CgIntrinsicSignature *signature;
    TypeList *parameter;
    Type *objectType, *expected;
    Symbol *member, *found;
    const Symbol *methodFormals;
    CgConversionRank rank;
    int len, components, argumentCount, parameterCount, singleScalar;

    assert(expr != NULL);
    switch (expr->kind) {
    case CGIR_EXPR_CONSTANT:
        if (GetCategory(expr->type) != TYPE_CATEGORY_SCALAR ||
            !lKnownKind(expr->u.constant.kind))
        {
            return CgIRFail(ctx, CGIR_VERIFY_TYPE, expr->loc, expr);
        }
        if (!(expr->u.constant.kind == GetScalarKind(expr->type) ||
              expr->u.constant.kind == CG_SCALAR_CFLOAT ||
              expr->u.constant.kind == CG_SCALAR_CINT))
        {
            return CgIRFail(ctx, CGIR_VERIFY_TYPE, expr->loc, expr);
        }
        break;
    case CGIR_EXPR_SYMBOL:
        if (!lResolveSymbol(ctx, expr->u.symbol, expr->loc))
            return 0;
        break;
    case CGIR_EXPR_MEMBER:
        if (expr->u.member.object == NULL || expr->u.member.member == NULL)
            return CgIRFail(ctx, CGIR_VERIFY_OPERAND, expr->loc, expr);
        if (!lVerifyExpr(ctx, expr->u.member.object))
            return 0;
        objectType = expr->u.member.object->type;
        member = expr->u.member.member;
        if (GetCategory(objectType) != TYPE_CATEGORY_STRUCT ||
            objectType->str.members == NULL)
        {
            return CgIRFail(ctx, CGIR_VERIFY_OPERAND, expr->loc, expr);
        }
        found = NULL;
        for (memberCursor = objectType->str.members->params;
             memberCursor != NULL; memberCursor = memberCursor->next)
        {
            if (memberCursor == member ||
                (memberCursor->name == member->name &&
                 !IsFunction(memberCursor)))
            {
                found = memberCursor;
                break;
            }
        }
        if (found == NULL)
            return CgIRFail(ctx, CGIR_VERIFY_OPERAND, expr->loc, expr);
        if (!IsSameUnqualifiedType(expr->type, found->type))
            return CgIRFail(ctx, CGIR_VERIFY_TYPE, expr->loc, expr);
        break;
    case CGIR_EXPR_INDEX:
        if (expr->u.index.object == NULL || expr->u.index.index == NULL)
            return CgIRFail(ctx, CGIR_VERIFY_OPERAND, expr->loc, expr);
        if (!lVerifyExpr(ctx, expr->u.index.object) ||
            !lVerifyExpr(ctx, expr->u.index.index))
        {
            return 0;
        }
        objectType = expr->u.index.object->type;
        if (!IsArray(objectType) && !IsVector(objectType, NULL) &&
            !IsMatrix(objectType, NULL, NULL))
        {
            return CgIRFail(ctx, CGIR_VERIFY_OPERAND, expr->loc, expr);
        }
        if (!IsScalar(expr->u.index.index->type) ||
            !lIntegralKind(GetScalarKind(expr->u.index.index->type)))
        {
            return CgIRFail(ctx, CGIR_VERIFY_OPERAND, expr->loc, expr);
        }
        if (IsArray(objectType)) {
            expected = objectType->arr.eltype;
        } else if (IsVector(objectType, NULL)) {
            expected = GetStandardTypeKind(GetScalarKind(objectType), 0, 0);
        } else {
            IsMatrix(objectType, &len, NULL);
            expected = GetStandardTypeKind(GetScalarKind(objectType), len, 0);
        }
        if (expected == NULL || expected == UndefinedType ||
            !IsSameUnqualifiedType(expr->type, expected))
        {
            return CgIRFail(ctx, CGIR_VERIFY_TYPE, expr->loc, expr);
        }
        break;
    case CGIR_EXPR_LENGTH:
        if (expr->u.length.object == NULL)
            return CgIRFail(ctx, CGIR_VERIFY_OPERAND, expr->loc, expr);
        if (!lVerifyExpr(ctx, expr->u.length.object))
            return 0;
        objectType = expr->u.length.object->type;
        if (!IsArray(objectType) && !IsVector(objectType, NULL) &&
            !IsMatrix(objectType, NULL, NULL))
        {
            return CgIRFail(ctx, CGIR_VERIFY_OPERAND, expr->loc, expr);
        }
        expected = GetStandardTypeKind(CG_SCALAR_INT, 0, 0);
        if (expected == UndefinedType ||
            !IsSameUnqualifiedType(expr->type, expected))
        {
            return CgIRFail(ctx, CGIR_VERIFY_TYPE, expr->loc, expr);
        }
        break;
    case CGIR_EXPR_SWIZZLE:
        if (expr->u.swizzle.object == NULL)
            return CgIRFail(ctx, CGIR_VERIFY_OPERAND, expr->loc, expr);
        if (!lVerifyExpr(ctx, expr->u.swizzle.object))
            return 0;
        objectType = expr->u.swizzle.object->type;
        if (IsScalar(objectType)) {
            len = 1;
        } else if (IsVector(objectType, &len)) {
            /* len set by IsVector */
        } else {
            return CgIRFail(ctx, CGIR_VERIFY_OPERAND, expr->loc, expr);
        }
        if (!lSwizzleComponentsAgree(ctx, expr, len))
            return 0;
        if (expr->u.swizzle.componentCount == 1)
            expected = GetStandardTypeKind(GetScalarKind(objectType), 0, 0);
        else
            expected = GetStandardTypeKind(GetScalarKind(objectType),
                                           expr->u.swizzle.componentCount, 0);
        if (expected == UndefinedType ||
            !IsSameUnqualifiedType(expr->type, expected))
        {
            return CgIRFail(ctx, CGIR_VERIFY_TYPE, expr->loc, expr);
        }
        break;
    case CGIR_EXPR_CONSTRUCT:
        components = lComponentCount(expr->type);
        if (components <= 0 || !lKnownKind(GetScalarKind(expr->type)))
            return CgIRFail(ctx, CGIR_VERIFY_TYPE, expr->loc, expr);
        argumentCount = 0;
        singleScalar = 0;
        for (cursor = expr->u.construct.arguments; cursor != NULL;
             cursor = cursor->next)
        {
            if (!lVerifyExpr(ctx, cursor))
                return 0;
            if (!lKnownKind(GetScalarKind(cursor->type)))
                return CgIRFail(ctx, CGIR_VERIFY_OPERAND, expr->loc, expr);
            len = lComponentCount(cursor->type);
            if (len <= 0)
                return CgIRFail(ctx, CGIR_VERIFY_OPERAND, expr->loc, expr);
            if (IsScalar(cursor->type))
                singleScalar = 1;
            components -= len;
            argumentCount++;
        }
        if (!(components == 0 || (argumentCount == 1 && singleScalar)))
            return CgIRFail(ctx, CGIR_VERIFY_OPERAND, expr->loc, expr);
        break;
    case CGIR_EXPR_CAST:
        if (expr->u.cast.operand == NULL)
            return CgIRFail(ctx, CGIR_VERIFY_OPERAND, expr->loc, expr);
        if (!lVerifyExpr(ctx, expr->u.cast.operand))
            return 0;
        if (IsSampler(expr->type, NULL) ||
            IsSampler(expr->u.cast.operand->type, NULL))
        {
            return CgIRFail(ctx, CGIR_VERIFY_TYPE, expr->loc, expr);
        }
        if (CgClassifyConversion(expr->u.cast.operand->type, expr->type,
                                 1) == CG_CONVERSION_NONE)
        {
            return CgIRFail(ctx, CGIR_VERIFY_TYPE, expr->loc, expr);
        }
        break;
    case CGIR_EXPR_UNARY:
        if (expr->u.unary.operand == NULL)
            return CgIRFail(ctx, CGIR_VERIFY_OPERAND, expr->loc, expr);
        if (!lVerifyExpr(ctx, expr->u.unary.operand))
            return 0;
        objectType = expr->u.unary.operand->type;
        switch (expr->u.unary.op) {
        case CGIR_OP_PRE_INCREMENT:
        case CGIR_OP_PRE_DECREMENT:
        case CGIR_OP_POST_INCREMENT:
        case CGIR_OP_POST_DECREMENT:
            if (!expr->u.unary.operand->isLvalue)
                return CgIRFail(ctx, CGIR_VERIFY_LVALUE, expr->loc, expr);
            if (!lArithmeticKind(GetScalarKind(objectType)))
                return CgIRFail(ctx, CGIR_VERIFY_OPERAND, expr->loc, expr);
            break;
        case CGIR_OP_NEGATE:
        case CGIR_OP_POSITIVE:
            if (!lArithmeticKind(GetScalarKind(objectType)))
                return CgIRFail(ctx, CGIR_VERIFY_OPERAND, expr->loc, expr);
            break;
        case CGIR_OP_LOGICAL_NOT:
            if (!lIsBooleanType(objectType))
                return CgIRFail(ctx, CGIR_VERIFY_OPERAND, expr->loc, expr);
            break;
        case CGIR_OP_BITWISE_NOT:
            if (!lIntegralKind(GetScalarKind(objectType)))
                return CgIRFail(ctx, CGIR_VERIFY_OPERAND, expr->loc, expr);
            break;
        default:
            return CgIRFail(ctx, CGIR_VERIFY_OPERAND, expr->loc, expr);
        }
        if (!IsSameUnqualifiedType(expr->type, objectType))
            return CgIRFail(ctx, CGIR_VERIFY_TYPE, expr->loc, expr);
        break;
    case CGIR_EXPR_BINARY:
        if (!lVerifyBinary(ctx, expr))
            return 0;
        break;
    case CGIR_EXPR_ASSIGN:
        if (expr->u.assign.target == NULL || expr->u.assign.value == NULL)
            return CgIRFail(ctx, CGIR_VERIFY_OPERAND, expr->loc, expr);
        if (!lVerifyExpr(ctx, expr->u.assign.target) ||
            !lVerifyExpr(ctx, expr->u.assign.value))
        {
            return 0;
        }
        if (!expr->u.assign.target->isLvalue)
            return CgIRFail(ctx, CGIR_VERIFY_LVALUE, expr->loc, expr);
        if (expr->u.assign.target->kind == CGIR_EXPR_SWIZZLE) {
            if (!lWriteMaskUnique(ctx, expr->u.assign.target))
                return 0;
        }
        objectType = expr->u.assign.target->type;
        if (!IsSameUnqualifiedType(expr->type, objectType))
            return CgIRFail(ctx, CGIR_VERIFY_TYPE, expr->loc, expr);
        rank = CgClassifyConversion(expr->u.assign.value->type, objectType, 0);
        if (rank < CG_CONVERSION_IMPLICIT_WARN)
            return CgIRFail(ctx, CGIR_VERIFY_TYPE, expr->loc, expr);
        break;
    case CGIR_EXPR_CONDITIONAL:
        if (expr->u.conditional.condition == NULL ||
            expr->u.conditional.trueExpr == NULL ||
            expr->u.conditional.falseExpr == NULL)
        {
            return CgIRFail(ctx, CGIR_VERIFY_OPERAND, expr->loc, expr);
        }
        if (!lVerifyExpr(ctx, expr->u.conditional.condition) ||
            !lVerifyExpr(ctx, expr->u.conditional.trueExpr) ||
            !lVerifyExpr(ctx, expr->u.conditional.falseExpr))
        {
            return 0;
        }
        if (!IsScalar(expr->u.conditional.condition->type) ||
            !lIsBooleanType(expr->u.conditional.condition->type))
        {
            return CgIRFail(ctx, CGIR_VERIFY_OPERAND, expr->loc, expr);
        }
        rank = CgClassifyConversion(expr->u.conditional.trueExpr->type,
                                    expr->u.conditional.falseExpr->type, 0);
        if (rank == CG_CONVERSION_NONE) {
            rank = CgClassifyConversion(expr->u.conditional.falseExpr->type,
                                        expr->u.conditional.trueExpr->type, 0);
            if (rank == CG_CONVERSION_NONE)
                return CgIRFail(ctx, CGIR_VERIFY_TYPE, expr->loc, expr);
        }
        rank = CgClassifyConversion(expr->u.conditional.trueExpr->type,
                                    expr->type, 0);
        if (rank < CG_CONVERSION_IMPLICIT_WARN)
            return CgIRFail(ctx, CGIR_VERIFY_TYPE, expr->loc, expr);
        rank = CgClassifyConversion(expr->u.conditional.falseExpr->type,
                                    expr->type, 0);
        if (rank < CG_CONVERSION_IMPLICIT_WARN)
            return CgIRFail(ctx, CGIR_VERIFY_TYPE, expr->loc, expr);
        break;
    case CGIR_EXPR_CALL:
        if (expr->u.call.callee == NULL)
            return CgIRFail(ctx, CGIR_VERIFY_CALL, expr->loc, expr);
        callee = ctx->module->functions;
        while (callee != NULL && callee->symbol != expr->u.call.callee)
            callee = callee->next;
        if (callee == NULL)
            return CgIRFail(ctx, CGIR_VERIFY_CALL, expr->loc, expr);
        if (!IsSameUnqualifiedType(expr->type, callee->resultType))
            return CgIRFail(ctx, CGIR_VERIFY_CALL, expr->loc, expr);
        if (!lVerifyCallArguments(ctx, expr, CGIR_VERIFY_CALL,
                                  callee->parameters, NULL,
                                  expr->u.call.arguments))
        {
            return 0;
        }
        break;
    case CGIR_EXPR_INTERFACE_CALL:
        if (expr->u.interfaceCall.method == NULL ||
            expr->u.interfaceCall.receiver == NULL)
        {
            return CgIRFail(ctx, CGIR_VERIFY_INTERFACE, expr->loc, expr);
        }
        if (!lVerifyExpr(ctx, expr->u.interfaceCall.receiver))
            return 0;
        objectType = expr->u.interfaceCall.receiver->type;
        if (GetCategory(objectType) != TYPE_CATEGORY_INTERFACE ||
            expr->u.interfaceCall.method->kind != FUNCTION_S ||
            !expr->u.interfaceCall.method->details.fun.isMethod ||
            expr->u.interfaceCall.method->details.fun.ownerType != objectType)
        {
            return CgIRFail(ctx, CGIR_VERIFY_INTERFACE, expr->loc, expr);
        }
        /*
         * Real function/method symbols carry a FUNCTION type whose
         * result lives in ->fun.rettype; hand-built symbols may carry
         * the bare result type directly.
         */
        expected = IsCategory(expr->u.interfaceCall.method->type,
                              TYPE_CATEGORY_FUNCTION)
                       ? expr->u.interfaceCall.method->type->fun.rettype
                       : expr->u.interfaceCall.method->type;
        if (!IsSameUnqualifiedType(expr->type, expected))
        {
            return CgIRFail(ctx, CGIR_VERIFY_INTERFACE, expr->loc, expr);
        }
        /*
         * Receiver-formal policy: production methods carry the implicit
         * receiver as a prepended leading formal sharing the owner type
         * (lSynthesizeMethodReceiver), while this node keeps the
         * receiver in its own field and binds "arguments" to the
         * declared formals alone.  Skip that one leading formal -- its
         * compatibility is already enforced by the ownerType identity
         * above; synthetic symbols without a receiver formal verify
         * unchanged.
         */
        methodFormals = expr->u.interfaceCall.method->details.fun.params;
        if (methodFormals != NULL && methodFormals->type == objectType)
            methodFormals = methodFormals->next;
        if (!lVerifyCallArguments(ctx, expr, CGIR_VERIFY_INTERFACE, NULL,
                                  methodFormals,
                                  expr->u.interfaceCall.arguments))
        {
            return 0;
        }
        break;
    case CGIR_EXPR_INTRINSIC:
        signature = expr->u.intrinsicCall.signature;
        if (signature == NULL ||
            signature->intrinsic != expr->u.intrinsicCall.intrinsic)
        {
            return CgIRFail(ctx, CGIR_VERIFY_INTRINSIC, expr->loc, expr);
        }
        if (!IsSameUnqualifiedType(expr->type, signature->result))
            return CgIRFail(ctx, CGIR_VERIFY_INTRINSIC, expr->loc, expr);
        parameterCount = 0;
        for (parameter = signature->parameters; parameter != NULL;
             parameter = parameter->next)
        {
            parameterCount++;
        }
        argumentCount = 0;
        for (cursor = expr->u.intrinsicCall.arguments; cursor != NULL;
             cursor = cursor->next)
        {
            argumentCount++;
        }
        if (argumentCount != parameterCount)
            return CgIRFail(ctx, CGIR_VERIFY_INTRINSIC, expr->loc, expr);
        if (!(signature->flags & CG_INTRINSIC_OUT_PARAMS)) {
            cursor = expr->u.intrinsicCall.arguments;
            parameter = signature->parameters;
            while (cursor != NULL && parameter != NULL) {
                if (CgClassifyConversion(cursor->type, parameter->type, 0) <
                    CG_CONVERSION_IMPLICIT_WARN)
                {
                    return CgIRFail(ctx, CGIR_VERIFY_INTRINSIC, expr->loc,
                                    expr);
                }
                cursor = cursor->next;
                parameter = parameter->next;
            }
        }
        cursor = expr->u.intrinsicCall.arguments;
        while (cursor != NULL) {
            if (!lVerifyExpr(ctx, cursor))
                return 0;
            cursor = cursor->next;
        }
        break;
    default:
        return CgIRFail(ctx, CGIR_VERIFY_OPERAND, expr->loc, expr);
    }
    return lRequireNodeBasics(ctx, expr);
} // lVerifyExpr

////////////////////////////// Statement rules ////////////////////////////////

static int lVerifyStmtList(CgIRVerifyContext *ctx, const CgIRStmt *stmts);
static int lVerifyDecl(CgIRVerifyContext *ctx, const CgIRDecl *decl,
                       int declare);

/*
 * lRequireScalarBoolean() - If/loop conditions take a scalar Boolean,
 *          matching the language's Boolean-condition rule.
 */

static int lRequireScalarBoolean(CgIRVerifyContext *ctx,
                                const CgIRExpr *condition)
{
    if (!IsScalar(condition->type) ||
        GetScalarKind(condition->type) != CG_SCALAR_BOOL)
    {
        return CgIRFail(ctx, CGIR_VERIFY_OPERAND, condition->loc, condition);
    }
    return 1;
} // lRequireScalarBoolean

static int lVerifyStmt(CgIRVerifyContext *ctx, const CgIRStmt *stmt)
{
    const CgIRExpr *predicate;
    int mark;

    assert(stmt != NULL);
    if (stmt->synthesized && stmt->loc.file == 0 && stmt->loc.line == 0)
        return CgIRFail(ctx, CGIR_VERIFY_LOCATION, stmt->loc, stmt);
    switch (stmt->kind) {
    case CGIR_STMT_BLOCK:
        mark = lScopeMark(ctx);
        ctx->scopeDepth++;
        if (!lVerifyStmtList(ctx, stmt->u.block))
            return 0;
        ctx->scopeDepth--;
        lScopeRelease(ctx, mark);
        break;
    case CGIR_STMT_DECL:
        if (stmt->u.decl == NULL)
            return CgIRFail(ctx, CGIR_VERIFY_OPERAND, stmt->loc, stmt);
        if (!lVerifyDecl(ctx, stmt->u.decl, 1))
            return 0;
        break;
    case CGIR_STMT_EXPR:
        if (stmt->u.expression == NULL)
            return CgIRFail(ctx, CGIR_VERIFY_OPERAND, stmt->loc, stmt);
        if (!lVerifyExpr(ctx, stmt->u.expression))
            return 0;
        break;
    case CGIR_STMT_IF:
        if (stmt->u.ifStmt.condition == NULL ||
            stmt->u.ifStmt.trueBranch == NULL)
        {
            return CgIRFail(ctx, CGIR_VERIFY_OPERAND, stmt->loc, stmt);
        }
        if (!lVerifyExpr(ctx, stmt->u.ifStmt.condition))
            return 0;
        if (!lRequireScalarBoolean(ctx, stmt->u.ifStmt.condition))
            return 0;
        if (!lVerifyStmt(ctx, stmt->u.ifStmt.trueBranch))
            return 0;
        if (stmt->u.ifStmt.falseBranch != NULL &&
            !lVerifyStmt(ctx, stmt->u.ifStmt.falseBranch))
        {
            return 0;
        }
        break;
    case CGIR_STMT_WHILE:
    case CGIR_STMT_DO:
        if (stmt->u.loop.condition == NULL || stmt->u.loop.body == NULL)
            return CgIRFail(ctx, CGIR_VERIFY_OPERAND, stmt->loc, stmt);
        if (!lVerifyExpr(ctx, stmt->u.loop.condition))
            return 0;
        if (!lRequireScalarBoolean(ctx, stmt->u.loop.condition))
            return 0;
        ctx->loopDepth++;
        if (!lVerifyStmt(ctx, stmt->u.loop.body))
            return 0;
        ctx->loopDepth--;
        break;
    case CGIR_STMT_FOR:
        mark = lScopeMark(ctx);
        ctx->scopeDepth++;
        if (stmt->u.forStmt.init != NULL &&
            !lVerifyStmt(ctx, stmt->u.forStmt.init))
        {
            return 0;
        }
        if (stmt->u.forStmt.condition != NULL) {
            if (!lVerifyExpr(ctx, stmt->u.forStmt.condition))
                return 0;
            if (!lRequireScalarBoolean(ctx, stmt->u.forStmt.condition))
                return 0;
        }
        if (stmt->u.forStmt.step != NULL &&
            !lVerifyExpr(ctx, stmt->u.forStmt.step))
        {
            return 0;
        }
        if (stmt->u.forStmt.body == NULL)
            return CgIRFail(ctx, CGIR_VERIFY_OPERAND, stmt->loc, stmt);
        ctx->loopDepth++;
        if (!lVerifyStmt(ctx, stmt->u.forStmt.body))
            return 0;
        ctx->loopDepth--;
        lScopeRelease(ctx, mark);
        ctx->scopeDepth--;
        break;
    case CGIR_STMT_RETURN:
        assert(ctx->function != NULL);
        if (IsVoid(ctx->function->resultType)) {
            if (stmt->u.returnExpr != NULL)
                return CgIRFail(ctx, CGIR_VERIFY_TYPE, stmt->loc, stmt);
        } else {
            if (stmt->u.returnExpr == NULL)
                return CgIRFail(ctx, CGIR_VERIFY_TYPE, stmt->loc, stmt);
            if (!lVerifyExpr(ctx, stmt->u.returnExpr))
                return 0;
            if (CgClassifyConversion(stmt->u.returnExpr->type,
                                     ctx->function->resultType,
                                     0) < CG_CONVERSION_IMPLICIT_WARN)
            {
                return CgIRFail(ctx, CGIR_VERIFY_TYPE, stmt->loc, stmt);
            }
        }
        break;
    case CGIR_STMT_BREAK:
    case CGIR_STMT_CONTINUE:
        if (ctx->loopDepth <= 0)
            return CgIRFail(ctx, CGIR_VERIFY_CONTROL, stmt->loc, stmt);
        break;
    case CGIR_STMT_DISCARD:
        predicate = stmt->u.discard.condition;
        if (predicate != NULL) {
            if (!lVerifyExpr(ctx, predicate))
                return 0;
            if (!lIsBooleanType(predicate->type))
                return CgIRFail(ctx, CGIR_VERIFY_OPERAND, stmt->loc, stmt);
        }
        break;
    default:
        return CgIRFail(ctx, CGIR_VERIFY_OPERAND, stmt->loc, stmt);
    }
    return 1;
} // lVerifyStmt

static int lVerifyStmtList(CgIRVerifyContext *ctx, const CgIRStmt *stmts)
{
    const CgIRStmt *cursor;

    for (cursor = stmts; cursor != NULL; cursor = cursor->next) {
        if (!lVerifyStmt(ctx, cursor))
            return 0;
    }
    return 1;
} // lVerifyStmtList

///////////////////////////// Declaration rules ///////////////////////////////

#define CGIR_STORAGE_LIMIT   ((int) CGIR_STORAGE_VARYING)
#define CGIR_DOMAIN_LIMIT    ((int) CGIR_DOMAIN_VARYING)

/*
 * lVerifyDecl() - One declared object: canonical type, sane storage and
 *          domain enums, verified initializer.  "declare" registers the
 *          identity afterwards so an initializer can never resolve the
 *          symbol being defined.
 */

static int lVerifyDecl(CgIRVerifyContext *ctx, const CgIRDecl *decl,
                       int declare)
{
    assert(decl != NULL);
    if (!lCanonicalType(decl->type))
        return CgIRFail(ctx, CGIR_VERIFY_TYPE, decl->loc, decl);
    if ((int) decl->storage < (int) CGIR_STORAGE_NONE ||
        (int) decl->storage > CGIR_STORAGE_LIMIT ||
        (int) decl->domain < (int) CGIR_DOMAIN_NONE ||
        (int) decl->domain > CGIR_DOMAIN_LIMIT)
    {
        return CgIRFail(ctx, CGIR_VERIFY_OWNER, decl->loc, decl);
    }
    if (decl->initializer != NULL && !lVerifyExpr(ctx, decl->initializer))
        return 0;
    if (declare && !lDeclareSymbol(ctx, decl->symbol, decl->loc))
        return 0;
    return 1;
} // lVerifyDecl

////////////////////////////// Function rules /////////////////////////////////

/*
 * lVerifyFunction() - Current function, loop depth, and parameter
 *          scope are established here; the locals list is emission
 *          metadata checked for shape only.
 */

static int lVerifyFunction(CgIRVerifyContext *ctx, const CgIRFunction *fn)
{
    const CgIRDecl *decl;
    int mark;

    assert(fn != NULL);
    if (!lCanonicalType(fn->resultType))
        return CgIRFail(ctx, CGIR_VERIFY_TYPE, fn->loc, fn);
    if (fn->body == NULL)
        return CgIRFail(ctx, CGIR_VERIFY_OWNER, fn->loc, fn);
    ctx->function = fn;
    ctx->loopDepth = 0;
    /* Parameters live exactly for this function's body walk; nothing
     * they bind may stay visible to later functions. */
    mark = lScopeMark(ctx);
    ctx->scopeDepth++;
    for (decl = fn->parameters; decl != NULL; decl = decl->next) {
        if (!lVerifyDecl(ctx, decl, 1))
            return 0;
    }
    for (decl = fn->locals; decl != NULL; decl = decl->next) {
        if (!lVerifyDecl(ctx, decl, 0))
            return 0;
    }
    if (!lVerifyStmt(ctx, fn->body))
        return 0;
    lScopeRelease(ctx, mark);
    ctx->scopeDepth--;
    ctx->function = NULL;
    return 1;
} // lVerifyFunction

/////////////////////////////// Module entry //////////////////////////////////

int CgIRVerifyModule(const CgIRModule *module,
                     CgIRVerifyDiagnostic *diagnostic)
{
    CgIRVerifyContext ctx;
    const CgIRDecl *decl;
    const CgIRFunction *fn, *prior, *secondEntry;
    SourceLoc emptyLoc;

    assert(module != NULL);
    memset(&ctx, 0, sizeof(ctx));
    memset(&emptyLoc, 0, sizeof(emptyLoc));
    ctx.module = module;
    ctx.diagnostic = diagnostic;
    /* Scope depth 1 is the module scope holding globals. */
    ctx.scopeDepth = 1;
    if (diagnostic != NULL)
        memset(diagnostic, 0, sizeof(*diagnostic));

    /* A sticky allocation failure means the graph is incomplete by
     * construction; report ownership about the module itself. */
    if (CgIRModuleFailed(module))
        return CgIRFail(&ctx, CGIR_VERIFY_OWNER, emptyLoc, module);

    for (decl = module->globals; decl != NULL; decl = decl->next) {
        if (!lVerifyDecl(&ctx, decl, 1))
            return 0;
    }

    secondEntry = NULL;
    for (fn = module->functions; fn != NULL; fn = fn->next) {
        for (prior = module->functions; prior != fn; prior = prior->next) {
            if (prior->symbol != NULL && prior->symbol == fn->symbol)
                return CgIRFail(&ctx, CGIR_VERIFY_OWNER, fn->loc, fn);
        }
        if (fn->isEntry) {
            if (module->entry == NULL || secondEntry != NULL)
                return CgIRFail(&ctx, CGIR_VERIFY_OWNER, fn->loc, fn);
            secondEntry = fn;
        }
        if (!lVerifyFunction(&ctx, fn))
            return 0;
    }
    if (module->entry != NULL) {
        for (fn = module->functions; fn != NULL; fn = fn->next) {
            if (fn == module->entry)
                break;
        }
        if (fn == NULL || !module->entry->isEntry)
            return CgIRFail(&ctx, CGIR_VERIFY_OWNER, module->entry->loc,
                            module);
    }
    return 1;
} // CgIRVerifyModule

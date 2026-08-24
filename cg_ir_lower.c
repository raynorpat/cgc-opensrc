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
INCLUDING WITHOUT LIMITATION, WARRANTIES OF TITLE, NON-INFRINGEMENT,
MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL NVIDIA BE LIABLE FOR ANY SPECIAL, INDIRECT,
INCIDENTAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
\****************************************************************************/

//
// cg_ir_lower.c - One-for-one lowering of the typed frontend tree into
//        Cg IR for exactly the symbols the reach graph admits.
//
// Provenance: frontend expressions carry no location of their own, so
// every node takes the location of the statement being lowered; a
// synthesized empty block (for `if` branches like "if (c) ;") uses the
// documented zero-location encoding instead.  Lvalue and side-effect
// flags copy straight off the source node.
//
// Matrix `_m` selectors: the frontend packs row<<2|col per component in
// four bits, which the IR swizzle payload cannot hold.  Lowering keeps
// them explicit anyway: one component becomes nested indexing
// m[row][col], several components become a constructor of those
// selections, and only multi-component matrix WRITE masks -- which
// would need fan-out stores the IR deliberately lacks -- are rejected
// with an internal diagnostic at the construct.
//

#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#include "slglobals.h"
#include "cg_ir.h"
#include "cg_reach.h"
#include "cg_ir_lower.h"

/////////////////////////////// Lowering state //////////////////////////////

typedef struct CgIRLowerRec_ {
    CgIRModule *module;
    Scope *globalScope;
    Symbol *entry;
    SourceLoc loc;    // provenance: enclosing statement or function
    Symbol *function;      // function being lowered
    Scope *funScope;       // its formal/local scope
    CgIRFunction *irFunction;
    int tempCounter;       // synthesized temporaries
} CgIRLower;

/*
 * lIRPoolAlloc() - Module storage comes from the global scope's memory
 *          pool, so releasing the compilation releases the IR graph.
 */

static void *lIRPoolAlloc(void *arg, size_t size)
{
    return mem_Calloc((MemoryPool *) arg, size, 1);
} // lIRPoolAlloc

/*
 * lUnlowerable() - One controlled internal diagnostic for a tree shape
 *          the backend-neutral IR deliberately cannot represent, then a
 *          sticky module failure so nothing downstream consumes a lossy
 *          graph.
 */

static void lUnlowerable(CgIRLower *L, const char *what)
{
    InternalError(&L->loc, ERROR_S_CG_IR_UNLOWERABLE, what);
    L->module->failed = 1;
} // lUnlowerable

//////////////////////////////// Declarations ////////////////////////////////

/*
 * lDefaultDomain() - Program-interface role: entry formals join the
 *          varying interface, globals the uniform interface, and
 *          everything else has no interface role.
 */

static CgIRDomain lDefaultDomain(int isEntryParam, int isGlobal)
{
    if (isEntryParam)
        return CGIR_DOMAIN_VARYING;
    if (isGlobal)
        return CGIR_DOMAIN_UNIFORM;
    return CGIR_DOMAIN_NONE;
} // lDefaultDomain

/*
 * lMapDomain() - Explicit uniform/varying declarations win over any
 *          default role.
 */

static CgIRDomain lMapDomain(Type *fType, CgIRDomain fallback)
{
    int domain;

    domain = GetDomain(fType);
    if (domain == TYPE_DOMAIN_UNIFORM)
        return CGIR_DOMAIN_UNIFORM;
    if (domain == TYPE_DOMAIN_VARYING)
        return CGIR_DOMAIN_VARYING;
    return fallback;
} // lMapDomain

static CgIRStorage lMapStorage(Type *fType)
{
    int qualifiers, domain;

    qualifiers = GetQualifiers(fType);
    domain = GetDomain(fType);
    if (qualifiers & TYPE_QUALIFIER_CONST)
        return CGIR_STORAGE_CONST;
    if (domain == TYPE_DOMAIN_UNIFORM)
        return CGIR_STORAGE_UNIFORM;
    if (domain == TYPE_DOMAIN_VARYING)
        return CGIR_STORAGE_VARYING;
    return CGIR_STORAGE_NONE;
} // lMapStorage

/*
 * lNewDecl() - One IR declaration for a frontend symbol.  A poison
 *          recovery type can never be represented in the IR; the
 *          frontend already reported its one diagnostic, so lowering
 *          fails the module quietly (all-or-nothing) instead of
 *          reporting a second diagnostic.
 */

static CgIRDecl *lNewDecl(CgIRLower *L, Symbol *symbol, Type *fType,
                          CgIRDomain defaultDomain, CgIRExpr *initializer)
{
    if (CgTypeIsPoison(fType)) {
        L->module->failed = 1;
        return NULL;
    }
    return CgIRNewDecl(L->module, symbol, symbol ? symbol->name : 0,
                       fType, lMapStorage(fType),
                       lMapDomain(fType, defaultDomain),
                       symbol ? symbol->details.var.semantics : 0,
                       initializer, &L->loc);
} // lNewDecl

///////////////////////////// Expression lowering ////////////////////////////

static CgIRExpr *lLowerExpr(CgIRLower *L, expr *fExpr);

/*
 * lAppendExpr() - Ordered list append for argument/constructor chains.
 */

static void lAppendExpr(CgIRExpr **list, CgIRExpr *expr)
{
    CgIRAppendExpr(list, expr);
} // lAppendExpr

/*
 * lScalarConst() - One typed constant component.
 */

static CgIRExpr *lScalarConst(CgIRLower *L, const scalar_constant *value)
{
    return CgIRNewConstant(L->module,
                           GetStandardTypeKind(value->kind, 0, 0),
                           &L->loc, value);
} // lScalarConst

/*
 * lAppendListElement() - One flattened EXPR_LIST element lowered and
 *          appended in order.
 */

static int lAppendListElement(CgIRLower *L, expr *fExpr, CgIRExpr **list)
{
    CgIRExpr *lowered;

    lowered = lLowerExpr(L, fExpr);
    if (lowered == NULL)
        return 0;
    lAppendExpr(list, lowered);
    return 1;
} // lAppendListElement

/*
 * lFlattenExprList() - EXPR_LIST_OP chains flatten into ordered IR
 *          argument lists so aggregate initializer nesting keeps
 *          element order.
 */

static int lFlattenExprList(CgIRLower *L, expr *chain, CgIRExpr **list)
{
    while (chain != NULL) {
        if (chain->common.kind != BINARY_N ||
            chain->bin.op != EXPR_LIST_OP)
        {
            lUnlowerable(L, "initializer argument list");
            return 0;
        }
        if (!lAppendListElement(L, chain->bin.left, list))
            return 0;
        chain = chain->bin.right;
    }
    return 1;
} // lFlattenExprList

/*
 * lLowerConstant() - Scalar constants lower directly; folded vector
 *          constants become explicit constructors of scalar components
 *          because IR constants carry one datum each.
 */

static CgIRExpr *lLowerConstant(CgIRLower *L, expr *fExpr)
{
    CgIRExpr *result, *component;
    int len, ii;

    len = SUBOP_GET_S(fExpr->co.subop);
    if (len <= 1)
        return CgIRNewConstant(L->module, fExpr->common.type, &L->loc,
                               &fExpr->co.val[0]);
    result = CgIRNewConstruct(L->module, fExpr->common.type, &L->loc, NULL);
    if (result == NULL)
        return NULL;
    for (ii = 0; ii < len; ii++) {
        component = lScalarConst(L, &fExpr->co.val[ii]);
        if (component == NULL)
            return NULL;
        lAppendExpr(&result->u.construct.arguments, component);
    }
    return result;
} // lLowerConstant

/*
 * lIndexNode() - m[row] selection typed the way the verifier derives
 *          row types, then [col] on top when asked for one component.
 */

static CgIRExpr *lMatrixRow(CgIRLower *L, CgIRExpr *object, int row)
{
    Type *matrixType;
    Type *rowType;
    CgNumericValue index;
    CgIRExpr *indexExpr;
    int len, len2, kind;

    matrixType = object->type;
    kind = GetScalarKind(matrixType);
    IsMatrix(matrixType, &len, &len2);
    rowType = GetStandardTypeKind(kind, len, 0);
    memset(&index, 0, sizeof(index));
    index.kind = CG_SCALAR_INT;
    index.value.i = row;
    indexExpr = CgIRNewConstant(L->module, IntType, &L->loc, &index);
    if (indexExpr == NULL)
        return NULL;
    return CgIRNewIndex(L->module, rowType, &L->loc, object, indexExpr);
} // lMatrixRow

/*
 * lLowerMatrixSelector() - Explicit component selection for `_m`
 *          swizzles.  Single components nest two indexes; read groups
 *          build a constructor of selections; write groups fail loudly.
 */

static CgIRExpr *lLowerMatrixSelector(CgIRLower *L, expr *fExpr)
{
    CgIRExpr *object, *result, *rowVec, *component;
    CgNumericValue colIndex;
    int count, mask16, selector, ii, row, column;

    object = lLowerExpr(L, fExpr->un.arg);
    if (object == NULL)
        return NULL;
    count = SUBOP_GET_T2(fExpr->un.subop);
    if (count == 0)
        count = 1;
    mask16 = SUBOP_GET_MASK16(fExpr->un.subop);
    if (count == 1) {
        selector = mask16 & 15;
        row = (selector >> 2) & 3;
        column = selector & 3;
        rowVec = lMatrixRow(L, object, row);
        if (rowVec == NULL)
            return NULL;
        if (fExpr->common.IsLValue)
            rowVec->isLvalue = 1;
        memset(&colIndex, 0, sizeof(colIndex));
        colIndex.kind = CG_SCALAR_INT;
        colIndex.value.i = column;
        component = CgIRNewConstant(L->module, IntType, &L->loc, &colIndex);
        if (component == NULL)
            return NULL;
        result = CgIRNewIndex(L->module, fExpr->common.type, &L->loc,
                              rowVec, component);
        if (result != NULL && fExpr->common.IsLValue)
            result->isLvalue = 1;
        return result;
    }
    /* Component groups read explicitly even when the frontend marked
     * them potential lvalues; only their use as an assignment target
     * (checked by the assignment lowering) has no IR encoding. */
    result = CgIRNewConstruct(L->module, fExpr->common.type, &L->loc, NULL);
    if (result == NULL)
        return NULL;
    for (ii = 0; ii < count; ii++) {
        selector = (mask16 >> (ii * 4)) & 15;
        row = (selector >> 2) & 3;
        column = selector & 3;
        rowVec = lMatrixRow(L, object, row);
        if (rowVec == NULL)
            return NULL;
        memset(&colIndex, 0, sizeof(colIndex));
        colIndex.kind = CG_SCALAR_INT;
        colIndex.value.i = column;
        component = CgIRNewConstant(L->module, IntType, &L->loc, &colIndex);
        if (component == NULL)
            return NULL;
        component = CgIRNewIndex(L->module,
                                 GetStandardTypeKind(
                                     GetScalarKind(object->type), 0, 0),
                                 &L->loc, rowVec, component);
        if (component == NULL)
            return NULL;
        lAppendExpr(&result->u.construct.arguments, component);
    }
    return result;
} // lLowerMatrixSelector

/*
 * lShapeOfIR() - Component geometry of an already-lowered type,
 *         mirroring the verifier's derivation.
 */

typedef struct LowerShape_Rec {
    int rows;
    int cols;
} LowerShape;

static int lShapeOfIR(const Type *type, LowerShape *shape)
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
} // lShapeOfIR

/*
 * lCompoundAssignType() - The IR verifies compound assignments as
 *          arithmetic on their operands, so the result type must be
 *          exactly the usual-arithmetic promotion of both sides with
 *          the target's geometry -- matching what the verifier derives.
 */

static Type *lCompoundAssignType(const CgIRExpr *target,
                                 const CgIRExpr *value)
{
    Type *usual;
    LowerShape targetShape, valueShape, promoted;
    CgScalarKind kind;
    int len;

    usual = CgUsualArithmeticType(target->type, value->type);
    kind = usual ? GetScalarKind(usual) : GetScalarKind(target->type);
    if (!IsVector(target->type, &len) && !IsMatrix(target->type, &len,
        &len))
    {
        return GetStandardTypeKind(kind, 0, 0);
    }
    if (!lShapeOfIR(target->type, &targetShape) ||
        !lShapeOfIR(value->type, &valueShape))
    {
        return GetStandardTypeKind(kind, 0, 0);
    }
    promoted.rows = targetShape.rows > valueShape.rows
                        ? targetShape.rows : valueShape.rows;
    promoted.cols = targetShape.cols > valueShape.cols
                        ? targetShape.cols : valueShape.cols;
    if (promoted.rows == 1 && promoted.cols == 1)
        return GetStandardTypeKind(kind, 0, 0);
    if (promoted.cols == 1)
        return GetStandardTypeKind(kind, promoted.rows, 0);
    return GetStandardTypeKind(kind, promoted.rows, promoted.cols);
} // lCompoundAssignType

/*
 * lIROpForUnary() - Shape-independent identity of a unary operator;
 *          swizzles, casts, constructors, length queries, and kill are
 *          handled by their callers.
 */

static int lIROpForUnary(opcode op)
{
    switch (op) {
    case NEG_OP: case NEG_V_OP:     return CGIR_OP_NEGATE;
    case POS_OP: case POS_V_OP:     return CGIR_OP_POSITIVE;
    case BNOT_OP: case BNOT_V_OP:   return CGIR_OP_LOGICAL_NOT;
    case NOT_OP: case NOT_V_OP:     return CGIR_OP_BITWISE_NOT;
    case PREDEC_OP:                 return CGIR_OP_PRE_DECREMENT;
    case PREINC_OP:                 return CGIR_OP_PRE_INCREMENT;
    case POSTDEC_OP:                return CGIR_OP_POST_DECREMENT;
    case POSTINC_OP:                return CGIR_OP_POST_INCREMENT;
    default:                        return CGIR_OP_NONE;
    }
} // lIROpForUnary

/*
 * lIROpForBinary() - Shape suffixes collapse onto one identity; compound
 *          assignments keep their own opcodes.
 */

static int lIROpForBinary(opcode op)
{
    switch (op) {    case MUL_OP: case MUL_V_OP: case MUL_SV_OP: case MUL_VS_OP:
        return CGIR_OP_MULTIPLY;
    case DIV_OP: case DIV_V_OP: case DIV_SV_OP: case DIV_VS_OP:
        return CGIR_OP_DIVIDE;
    case MOD_OP: case MOD_V_OP: case MOD_SV_OP: case MOD_VS_OP:
        return CGIR_OP_MODULO;
    case ADD_OP: case ADD_V_OP: case ADD_SV_OP: case ADD_VS_OP:
        return CGIR_OP_ADD;
    case SUB_OP: case SUB_V_OP: case SUB_SV_OP: case SUB_VS_OP:
        return CGIR_OP_SUBTRACT;
    case SHL_OP: case SHL_V_OP:
        return CGIR_OP_SHIFT_LEFT;
    case SHR_OP: case SHR_V_OP:
        return CGIR_OP_SHIFT_RIGHT;
    case LT_OP: case LT_V_OP: case LT_SV_OP: case LT_VS_OP:
        return CGIR_OP_LESS;
    case GT_OP: case GT_V_OP: case GT_SV_OP: case GT_VS_OP:
        return CGIR_OP_GREATER;
    case LE_OP: case LE_V_OP: case LE_SV_OP: case LE_VS_OP:
        return CGIR_OP_LESS_EQUAL;
    case GE_OP: case GE_V_OP: case GE_SV_OP: case GE_VS_OP:
        return CGIR_OP_GREATER_EQUAL;
    case EQ_OP: case EQ_V_OP: case EQ_SV_OP: case EQ_VS_OP:
        return CGIR_OP_EQUAL;
    case NE_OP: case NE_V_OP: case NE_SV_OP: case NE_VS_OP:
        return CGIR_OP_NOT_EQUAL;
    /* Frontend spelling trap: AND/OR are bitwise ('&','|') while
     * BAND/BOR are the Boolean operators ('&&','||'). */
    case AND_OP: case AND_V_OP: case AND_SV_OP: case AND_VS_OP:
        return CGIR_OP_BITWISE_AND;
    case XOR_OP: case XOR_V_OP: case XOR_SV_OP: case XOR_VS_OP:
        return CGIR_OP_BITWISE_XOR;
    case OR_OP: case OR_V_OP: case OR_SV_OP: case OR_VS_OP:
        return CGIR_OP_BITWISE_OR;
    case BAND_OP: case BAND_V_OP: case BAND_SV_OP: case BAND_VS_OP:
        return CGIR_OP_LOGICAL_AND;
    case BOR_OP: case BOR_V_OP: case BOR_SV_OP: case BOR_VS_OP:
        return CGIR_OP_LOGICAL_OR;
    case ASSIGN_OP:
    case ASSIGN_V_OP:
    case ASSIGN_GEN_OP:
    case ASSIGN_DYN_OP:   return CGIR_OP_ASSIGN;
    case ASSIGNMINUS_OP:  return CGIR_OP_SUBTRACT_ASSIGN;
    case ASSIGNMOD_OP:    return CGIR_OP_MODULO_ASSIGN;
    case ASSIGNPLUS_OP:   return CGIR_OP_ADD_ASSIGN;
    case ASSIGNSLASH_OP:  return CGIR_OP_DIVIDE_ASSIGN;
    case ASSIGNSTAR_OP:   return CGIR_OP_MULTIPLY_ASSIGN;
    default:              return CGIR_OP_NONE;
    }
} // lIROpForBinary

/*
 * lLowerCallArguments() - FUN_ARG_OP chains become ordered IR lists.
 */

static CgIRExpr *lLowerCallArguments(CgIRLower *L, expr *chain,
                                     int skipFirst)
{
    CgIRExpr *list;
    int skipped;

    list = NULL;
    skipped = 0;
    while (chain != NULL && !L->module->failed) {
        if (chain->common.kind != BINARY_N ||
            chain->bin.op != FUN_ARG_OP)
        {
            lUnlowerable(L, "call argument list");
            return NULL;
        }
        if (!skipFirst || skipped > 0) {
            CgIRExpr *argument;

            argument = lLowerExpr(L, chain->bin.left);
            if (argument == NULL)
                return NULL;
            lAppendExpr(&list, argument);
        }
        skipped++;
        chain = chain->bin.right;
    }
    if (skipFirst && skipped == 0) {
        lUnlowerable(L, "interface dispatch without receiver");
        return NULL;
    }
    return list;
} // lLowerCallArguments

/*
 * lNormalizeAssignValue() - Constant folding can replace a converted
 *          initializer with a same-shape value whose type no longer
 *          carries an implicit classification.  When the language still
 *          admits the conversion explicitly, make the cast node say so;
 *          anything else stays untouched for the verifier to judge.
 */

static CgIRExpr *lNormalizeAssignValue(CgIRLower *L, CgIRExpr *value,
                                       Type *targetType)
{
    if (value == NULL || targetType == NULL)
        return value;
    if (CgClassifyConversion(value->type, targetType, 0) !=
        CG_CONVERSION_NONE)
    {
        return value;
    }
    if (CgClassifyConversion(value->type, targetType, 1) ==
        CG_CONVERSION_NONE)
    {
        return value;
    }
    return CgIRNewCast(L->module, targetType, &L->loc, value);
} // lNormalizeAssignValue

/*
 * lLowerInterfaceCall() - Receiver-skip contract: the receiver rides its
 *          own field, the first actual is dropped, and arguments bind
 *          the declared formals alone.  The prepended receiver formal is
 *          never cloned.
 */

static CgIRExpr *lLowerInterfaceCall(CgIRLower *L, expr *fExpr)
{
    expr *selection;
    Symbol *method;
    CgIRExpr *receiver, *arguments;

    selection = fExpr->bin.left;
    if (selection == NULL || selection->common.kind != BINARY_N ||
        selection->bin.op != MEMBER_SELECTOR_OP ||
        selection->bin.right == NULL ||
        selection->bin.right->common.kind != SYMB_N ||
        selection->bin.right->sym.symbol == NULL)
    {
        lUnlowerable(L, "interface dispatch shape");
        return NULL;
    }
    method = selection->bin.right->sym.symbol;
    receiver = lLowerExpr(L, selection->bin.left);
    if (receiver == NULL)
        return NULL;
    arguments = lLowerCallArguments(L, fExpr->bin.right, 1);
    if (arguments == NULL && L->module->failed)
        return NULL;
    return CgIRNewInterfaceCall(L->module, fExpr->common.type, &L->loc,
                                method, receiver, arguments);
} // lLowerInterfaceCall

static CgIRExpr *lLowerExpr(CgIRLower *L, expr *fExpr)
{
    CgIRExpr *result, *left, *right;
    int irOp;

    if (fExpr == NULL)
        return NULL;
    switch (fExpr->common.kind) {
    case SYMB_N:
        if (fExpr->sym.op != VARIABLE_OP) {
            lUnlowerable(L, "symbol reference opcode");
            return NULL;
        }
        if (fExpr->sym.symbol != NULL &&
            fExpr->sym.symbol->kind == CONSTANT_S)
        {
            /* Named Boolean constants (true/false) are super-global
             * CONSTANT symbols, not declarations the module owns;
             * lower to the datum itself. */
            CgNumericValue named;

            memset(&named, 0, sizeof(named));
            named.kind = CG_SCALAR_BOOL;
            named.value.i =
                fExpr->sym.symbol->details.con.value ? 1 : 0;
            return CgIRNewConstant(L->module, fExpr->common.type, &L->loc,
                                   &named);
        }
        result = CgIRNewSymbol(L->module, fExpr->common.type, &L->loc,
                               fExpr->sym.symbol);
        break;
    case CONST_N:
        return lLowerConstant(L, fExpr);
    case UNARY_N:
        switch (fExpr->un.op) {
        case SWIZZLE_Z_OP:
            left = lLowerExpr(L, fExpr->un.arg);
            if (left == NULL)
                return NULL;
            {
                /* Same legacy-base caveat as indexing: derive the
                 * canonical result kind from the object. */
                int count = SUBOP_GET_S2(fExpr->un.subop);
                Type *swizType;

                if (count <= 0)
                    count = 1;
                swizType = count == 1
                    ? GetStandardTypeKind(GetScalarKind(left->type), 0, 0)
                    : GetStandardTypeKind(GetScalarKind(left->type), count,
                                          0);
                result = CgIRNewSwizzle(L->module, swizType, &L->loc,
                                        left,
                                        SUBOP_GET_MASK(fExpr->un.subop),
                                        count);
            }
            break;
        case SWIZMAT_Z_OP:
            return lLowerMatrixSelector(L, fExpr);
        case CAST_CS_OP:
        case CAST_CV_OP:
        case CAST_CM_OP:
        case CAST_SHAPE_OP:
        case CAST_STRUCT_OP:
            left = lLowerExpr(L, fExpr->un.arg);
            if (left == NULL)
                return NULL;
            result = CgIRNewCast(L->module, fExpr->common.type, &L->loc,
                                 left);
            break;
        case ARRAY_LENGTH_OP:
            left = lLowerExpr(L, fExpr->un.arg);
            if (left == NULL)
                return NULL;
            result = CgIRNewLength(L->module, fExpr->common.type, &L->loc,
                                   left);
            break;
        case VECTOR_V_OP:
            result = CgIRNewConstruct(L->module, fExpr->common.type,
                                      &L->loc, NULL);
            if (result != NULL &&
                !lFlattenExprList(L, fExpr->un.arg,
                                  &result->u.construct.arguments))
            {
                return NULL;
            }
            break;
        case KILL_OP:
            /* Discard predicates unwrap at the statement; a bare kill
             * expression has no IR identity. */
            lUnlowerable(L, "kill outside discard");
            return NULL;
        default:
            irOp = lIROpForUnary(fExpr->un.op);
            if (irOp == CGIR_OP_NONE) {
                lUnlowerable(L, "unary operator");
                return NULL;
            }
            left = lLowerExpr(L, fExpr->un.arg);
            if (left == NULL)
                return NULL;
            if (fExpr->un.op == PREINC_OP ||
                fExpr->un.op == PREDEC_OP ||
                fExpr->un.op == POSTINC_OP ||
                fExpr->un.op == POSTDEC_OP)
            {
                /* The parser builds increment/decrement without a
                 * result type (legacy expands them later); the IR
                 * requires identity with the operand's type. */
                result = CgIRNewUnary(L->module, left->type, &L->loc,
                                      (CgIROp) irOp, left);
            } else {
                result = CgIRNewUnary(L->module, fExpr->common.type,
                                      &L->loc, (CgIROp) irOp, left);
            }
            break;
        }
        break;
    case BINARY_N:
        switch (fExpr->bin.op) {
        case MEMBER_SELECTOR_OP:
            left = lLowerExpr(L, fExpr->bin.left);
            if (left == NULL)
                return NULL;
            if (fExpr->bin.right == NULL ||
                fExpr->bin.right->common.kind != SYMB_N ||
                fExpr->bin.right->sym.symbol == NULL)
            {
                lUnlowerable(L, "member selection");
                return NULL;
            }
            result = CgIRNewMember(L->module, fExpr->common.type, &L->loc,
                                   left, fExpr->bin.right->sym.symbol);
            break;
        case ARRAY_INDEX_OP:
            left = lLowerExpr(L, fExpr->bin.left);
            right = left ? lLowerExpr(L, fExpr->bin.right) : NULL;
            if (right == NULL)
                return NULL;
            {
                /* Frontend index nodes stamp legacy four-bit bases
                 * that cannot hold half/fixed; rederive the canonical
                 * element type the way the verifier will. */
                Type *objectType = left->type;
                Type *elementType = objectType;
                int ilen;

                if (IsArray(objectType) && !IsVector(objectType, &ilen))
                {
                    elementType = objectType->arr.eltype;
                } else if (IsVector(objectType, &ilen)) {
                    elementType = GetStandardTypeKind(
                        GetScalarKind(objectType), 0, 0);
                }
                result = CgIRNewIndex(L->module, elementType, &L->loc,
                                      left, right);
            }
            break;
        case FUN_CALL_OP:
            if (fExpr->bin.left == NULL ||
                fExpr->bin.left->common.kind != SYMB_N ||
                fExpr->bin.left->sym.symbol == NULL)
            {
                lUnlowerable(L, "direct call callee");
                return NULL;
            }
            left = NULL;
            right = lLowerCallArguments(L, fExpr->bin.right, 0);
            if (right == NULL && L->module->failed)
                return NULL;
            result = CgIRNewCall(L->module, fExpr->common.type, &L->loc,
                                 fExpr->bin.left->sym.symbol, right);
            break;
        case FUN_INTRINSIC_OP:
            if (fExpr->bin.left == NULL ||
                fExpr->bin.left->common.kind != SYMB_N ||
                fExpr->bin.left->sym.symbol == NULL ||
                fExpr->bin.left->sym.symbol->details.fun.intrinsic == NULL)
            {
                lUnlowerable(L, "intrinsic call identity");
                return NULL;
            }
            right = lLowerCallArguments(L, fExpr->bin.right, 0);
            if (right == NULL && L->module->failed)
                return NULL;
            result = CgIRNewIntrinsicCall(
                L->module, fExpr->common.type, &L->loc,
                fExpr->bin.left->sym.symbol->details.fun.intrinsic->intrinsic,
                fExpr->bin.left->sym.symbol->details.fun.intrinsic, right);
            break;
        case INTERFACE_CALL_OP:
            return lLowerInterfaceCall(L, fExpr);
        case COMMA_OP:
        case EXPR_LIST_OP:
        case FUN_ARG_OP:
            lUnlowerable(L, "sequencing inside an expression");
            return NULL;
        default:
            irOp = lIROpForBinary(fExpr->bin.op);
            if (irOp == CGIR_OP_NONE) {
                lUnlowerable(L, "binary operator");
                return NULL;
            }
            if ((CgIROp) irOp == CGIR_OP_ASSIGN &&
                fExpr->bin.left != NULL &&
                fExpr->bin.left->common.kind == UNARY_N &&
                fExpr->bin.left->un.op == SWIZMAT_Z_OP &&
                SUBOP_GET_T2(fExpr->bin.left->un.subop) > 1)
            {
                /* Fan-out stores need temporaries the IR deliberately
                 * leaves to later tasks; reject loudly instead. */
                lUnlowerable(L, "multi-component matrix write mask");
                return NULL;
            }
            left = lLowerExpr(L, fExpr->bin.left);
            right = left ? lLowerExpr(L, fExpr->bin.right) : NULL;
            if (right == NULL)
                return NULL;
            switch ((CgIROp) irOp) {
            case CGIR_OP_ASSIGN:
                {
                    Type *assignType = fExpr->common.type;

                    if (fExpr->bin.op == ASSIGN_DYN_OP &&
                        fExpr->bin.left != NULL)
                    {
                        /* Dynamic assignment keeps the destination's
                         * unsized canonical type on the IR node; the
                         * runtime shape rides the value subtree. */
                        assignType = fExpr->bin.left->common.type;
                    }
                    right = lNormalizeAssignValue(
                        L, right, fExpr->bin.left->common.type);
                    if (right == NULL)
                        return NULL;
                    result = CgIRNewAssign(L->module, assignType, &L->loc,
                                           (CgIROp) irOp, left, right);
                }
                break;
            case CGIR_OP_ADD_ASSIGN:
            case CGIR_OP_SUBTRACT_ASSIGN:
            case CGIR_OP_MULTIPLY_ASSIGN:
            case CGIR_OP_DIVIDE_ASSIGN:
            case CGIR_OP_MODULO_ASSIGN:
                result = CgIRNewAssign(L->module,
                                       lCompoundAssignType(left, right),
                                       &L->loc, (CgIROp) irOp, left,
                                       right);
                break;
            default:
                result = CgIRNewBinary(L->module, fExpr->common.type,
                                       &L->loc, (CgIROp) irOp, left, right);
                break;
            }
            break;
        }
        break;
    case TRINARY_N:
        switch (fExpr->tri.op) {
        case COND_OP:
        case COND_V_OP:
        case COND_SV_OP:
        case COND_GEN_OP:
            left = lLowerExpr(L, fExpr->tri.arg1);
            right = left ? lLowerExpr(L, fExpr->tri.arg2) : NULL;
            result = right ? lLowerExpr(L, fExpr->tri.arg3) : NULL;
            if (result == NULL)
                return NULL;
            result = CgIRNewConditional(L->module, fExpr->common.type,
                                        &L->loc, left, right, result);
            break;
        default:
            lUnlowerable(L, "conditional assignment operator");
            return NULL;
        }
        break;
    default:
        lUnlowerable(L, "expression node");
        return NULL;
    }
    if (result == NULL)
        return NULL;
    result->isLvalue = fExpr->common.IsLValue;
    result->sideEffects = fExpr->common.HasSideEffects;
    return result;
} // lLowerExpr

///////////////////////////// Statement lowering /////////////////////////////

static int lLowerStmtList(CgIRLower *L, stmt *fStmt, CgIRStmt **list);

/*
 * lEmptyBlock() - A synthesized empty block under the documented
 *          zero-location encoding (clear synthesized flag).
 */

static CgIRStmt *lEmptyBlock(CgIRLower *L)
{
    return CgIRNewBlockStmt(L->module, NULL);
} // lEmptyBlock

/*
 * lBranchBody() - Frontend branches are statement lists; an IR branch
 *          is one statement, so lists wrap in a block and "if (c) ;"
 *          keeps an explicit empty block.
 */

static CgIRStmt *lBranchBody(CgIRLower *L, stmt *fStmt)
{
    CgIRStmt *list, *block;

    list = NULL;
    if (!lLowerStmtList(L, fStmt, &list))
        return NULL;
    if (list == NULL)
        return lEmptyBlock(L);
    if (list->next == NULL)
        return list;
    block = CgIRNewBlockStmt(L->module,
                             fStmt ? &fStmt->commonst.loc : NULL);
    if (block == NULL)
        return NULL;
    block->u.block = list;
    return block;
} // lBranchBody

/*
 * lNewTemp() - A compiler-synthesized temporary.  It exists only in the
 *          IR -- visibility comes from its DECL statement -- so the
 *          frontend symbol table and generated output stay untouched.
 */

static Symbol *lNewTemp(CgIRLower *L, Type *fType)
{
    char name[64];
    Symbol *temp;

    sprintf(name, "$cglmpr%d", L->tempCounter++);
    temp = (Symbol *) (calloc)(1, sizeof(Symbol));
    if (temp == NULL)
        return NULL;
    temp->name = LookUpAddString(atable, name);
    temp->type = fType;
    temp->loc = L->loc;
    return temp;
} // lNewTemp

/*
 * lTryMatrixGroupWrite() - `m._mI0_J0_mI1_J1 = value` fans out into one
 *          explicit scalar store per selected component.  The object and
 *          the value lower exactly once; pure IR subtrees are shared by
 *          the stores, while a side-effecting object or value first
 *          moves into its own synthesized temporary so every store reads
 *          an effect-free node instead of sharing one effecting subtree.
 *          Returns 1 when the statement was handled, 0 to fall through
 *          (including every non-group-write shape), -1 on lowering
 *          failure.
 */

static int lTryMatrixGroupWrite(CgIRLower *L, expr *fExpr, CgIRStmt **list)
{
    expr *selector;
    CgIRExpr *object, *value, *rowVec, *component, *target, *assign;
    CgIRStmt *stmt;
    CgNumericValue indexValue;
    int count, mask16, ii, row, column;

    if (fExpr == NULL || fExpr->common.kind != BINARY_N ||
        lIROpForBinary(fExpr->bin.op) != CGIR_OP_ASSIGN ||
        fExpr->bin.op == ASSIGN_DYN_OP ||
        fExpr->bin.left == NULL ||
        fExpr->bin.left->common.kind != UNARY_N ||
        fExpr->bin.left->un.op != SWIZMAT_Z_OP)
    {
        return 0;
    }
    selector = fExpr->bin.left;
    count = SUBOP_GET_T2(selector->un.subop);
    if (count <= 1)
        return 0;
    if (selector->un.arg == NULL) {
        lUnlowerable(L, "matrix selector without object");
        return -1;
    }
    object = lLowerExpr(L, selector->un.arg);
    if (object == NULL)
        return -1;
    if (selector->un.arg->common.HasSideEffects &&
        L->funScope != NULL)
    {
        /* Evaluate the effecting object exactly once into a synthesized
         * temporary; every store then reads the temporary. */
        Symbol *temp;

        temp = lNewTemp(L, selector->un.arg->common.type);
        if (temp == NULL)
            return -1;
        {
            CgIRDecl *tempDecl;
            CgIRExpr *tempRef;

            tempDecl = lNewDecl(L, temp, temp->type, CGIR_DOMAIN_NONE,
                                NULL);
            if (tempDecl != NULL) {
                CgIRAppendDecl(&L->irFunction->locals, tempDecl);
                stmt = CgIRNewDeclStmt(L->module, &L->loc, tempDecl);
                if (stmt != NULL)
                    CgIRAppendStmt(list, stmt);
            }
            tempRef = CgIRNewSymbol(L->module, object->type, &L->loc,
                                    temp);
            if (tempRef == NULL)
                return -1;
            tempRef->isLvalue = 1;
            assign = CgIRNewAssign(L->module, object->type, &L->loc,
                                   CGIR_OP_ASSIGN, tempRef, object);
            if (assign == NULL)
                return -1;
            stmt = CgIRNewExprStmt(L->module, &L->loc, assign);
            if (stmt == NULL)
                return -1;
            CgIRAppendStmt(list, stmt);
            object = tempRef;
        }
    }
    object->isLvalue = 1;
    value = lLowerExpr(L, fExpr->bin.right);
    if (value == NULL)
        return -1;
    if (fExpr->bin.right->common.HasSideEffects &&
        L->funScope != NULL)
    {
        /* Evaluate the effecting value exactly once into a synthesized
         * temporary; every store then reads the temporary instead of
         * sharing one effecting subtree. */
        Symbol *temp;

        temp = lNewTemp(L, fExpr->bin.right->common.type);
        if (temp == NULL)
            return -1;
        {
            CgIRDecl *tempDecl;
            CgIRExpr *tempRef;

            tempDecl = lNewDecl(L, temp, temp->type, CGIR_DOMAIN_NONE,
                                NULL);
            if (tempDecl != NULL) {
                CgIRAppendDecl(&L->irFunction->locals, tempDecl);
                stmt = CgIRNewDeclStmt(L->module, &L->loc, tempDecl);
                if (stmt != NULL)
                    CgIRAppendStmt(list, stmt);
            }
            tempRef = CgIRNewSymbol(L->module, value->type, &L->loc,
                                    temp);
            if (tempRef == NULL)
                return -1;
            tempRef->isLvalue = 1;
            assign = CgIRNewAssign(L->module, value->type, &L->loc,
                                   CGIR_OP_ASSIGN, tempRef, value);
            if (assign == NULL)
                return -1;
            stmt = CgIRNewExprStmt(L->module, &L->loc, assign);
            if (stmt == NULL)
                return -1;
            CgIRAppendStmt(list, stmt);
            value = tempRef;
        }
    }
    mask16 = SUBOP_GET_MASK16(selector->un.subop);
    for (ii = 0; ii < count; ii++) {
        row = ((mask16 >> (ii * 4)) >> 2) & 3;
        column = (mask16 >> (ii * 4)) & 3;
        rowVec = lMatrixRow(L, object, row);
        if (rowVec == NULL)
            return -1;
        rowVec->isLvalue = 1;
        memset(&indexValue, 0, sizeof(indexValue));
        indexValue.kind = CG_SCALAR_INT;
        indexValue.value.i = column;
        component = CgIRNewConstant(L->module, IntType, &L->loc,
                                    &indexValue);
        if (component == NULL)
            return -1;
        target = CgIRNewIndex(L->module,
                              GetStandardTypeKind(
                                  GetScalarKind(object->type), 0, 0),
                              &L->loc, rowVec, component);
        if (target == NULL)
            return -1;
        target->isLvalue = 1;
        assign = CgIRNewAssign(L->module, target->type, &L->loc,
                               CGIR_OP_ASSIGN, target, value);
        if (assign == NULL)
            return -1;
        stmt = CgIRNewExprStmt(L->module, &L->loc, assign);
        if (stmt == NULL)
            return -1;
        CgIRAppendStmt(list, stmt);
    }
    return 1;
} // lTryMatrixGroupWrite

/*
 * lCommaStatement() - Top-level comma sequences become ordered
 *          expression statements; commas nested deeper have no
 *          sequencing identity in the IR and are rejected where they
 *          cannot occur.
 */

static int lCommaStatement(CgIRLower *L, expr *fExpr, CgIRStmt **list)
{
    CgIRExpr *lowered;
    CgIRStmt *stmt;

    if (fExpr == NULL)
        return 1;
    {
        int groupWrite;

        groupWrite = lTryMatrixGroupWrite(L, fExpr, list);
        if (groupWrite != 0)
            return groupWrite > 0;
        if (L->module->failed)
            return 0;
    }
    if (fExpr->common.kind == BINARY_N && fExpr->bin.op == COMMA_OP) {
        return lCommaStatement(L, fExpr->bin.left, list) &&
               lCommaStatement(L, fExpr->bin.right, list);
    }
    lowered = lLowerExpr(L, fExpr);
    if (lowered == NULL)
        return 0;
    stmt = CgIRNewExprStmt(L->module, &L->loc, lowered);
    if (stmt == NULL)
        return 0;
    CgIRAppendStmt(list, stmt);
    return 1;
} // lCommaStatement

/*
 * lLowerStmt() - Statement-for-statement mapping.  DISCARD unwraps the
 *          frontend's kill wrapper; COMMENT statements carry no IR
 *          identity and drop.
 */

static int lLowerStmt(CgIRLower *L, stmt *fStmt, CgIRStmt **list)
{
    CgIRExpr *condition;
    CgIRStmt *stmt, *body;
    expr *sourceCond;

    if (fStmt == NULL)
        return 1;
    L->loc = fStmt->commonst.loc;
    switch (fStmt->commonst.kind) {
    case EXPR_STMT:
        if (fStmt->exprst.exp == NULL)
            return 1;
        return lCommaStatement(L, fStmt->exprst.exp, list);
    case IF_STMT:
        condition = lLowerExpr(L, fStmt->ifst.cond);
        if (condition == NULL)
            return 0;
        body = lBranchBody(L, fStmt->ifst.thenstmt);
        if (body == NULL)
            return 0;
        stmt = NULL;
        {
            CgIRStmt *elseBody;

            elseBody = fStmt->ifst.elsestmt
                           ? lBranchBody(L, fStmt->ifst.elsestmt)
                           : NULL;
            if (fStmt->ifst.elsestmt != NULL && elseBody == NULL)
                return 0;
            stmt = CgIRNewIfStmt(L->module, &L->loc, condition, body,
                                 elseBody);
        }
        break;
    case WHILE_STMT:
        condition = lLowerExpr(L, fStmt->whilest.cond);
        if (condition == NULL)
            return 0;
        body = lBranchBody(L, fStmt->whilest.body);
        stmt = body ? CgIRNewWhileStmt(L->module, &L->loc, condition, body)
                    : NULL;
        break;
    case DO_STMT:
        condition = lLowerExpr(L, fStmt->whilest.cond);
        if (condition == NULL)
            return 0;
        body = lBranchBody(L, fStmt->whilest.body);
        stmt = body ? CgIRNewDoStmt(L->module, &L->loc, condition, body)
                    : NULL;
        break;
    case FOR_STMT:
        return lLowerForStmt(L, fStmt, list);
    case BLOCK_STMT:
        stmt = CgIRNewBlockStmt(L->module, &L->loc);
        if (stmt == NULL)
            return 0;
        if (!lLowerStmtList(L, fStmt->blockst.body, &stmt->u.block))
            return 0;
        break;
    case RETURN_STMT:
        condition = fStmt->returnst.exp ? lLowerExpr(L, fStmt->returnst.exp)
                                        : NULL;
        if (fStmt->returnst.exp != NULL && condition == NULL)
            return 0;
        stmt = CgIRNewReturnStmt(L->module, &L->loc, condition);
        break;
    case BREAK_STMT:
        stmt = CgIRNewBreakStmt(L->module, &L->loc);
        break;
    case CONTINUE_STMT:
        stmt = CgIRNewContinueStmt(L->module, &L->loc);
        break;
    case DISCARD_STMT:
        /* Bare `discard;` arrives as KILL_OP wrapping a NULL predicate
         * under some profiles; unwrap first, then test for empty. */
        sourceCond = fStmt->discardst.cond;
        if (sourceCond != NULL && sourceCond->common.kind == UNARY_N &&
            sourceCond->un.op == KILL_OP)
        {
            sourceCond = sourceCond->un.arg;
        }
        condition = NULL;
        if (sourceCond != NULL) {
            condition = lLowerExpr(L, sourceCond);
            if (condition == NULL)
                return 0;
        }
        stmt = CgIRNewDiscardStmt(L->module, &L->loc, condition);
        break;
    case COMMENT_STMT:
        return 1;
    default:
        lUnlowerable(L, "statement kind");
        return 0;
    }
    if (stmt == NULL)
        return 0;
    CgIRAppendStmt(list, stmt);
    return 1;
} // lLowerStmt

/*
 * lSingleListExpr() - FOR init/step positions hold statement lists in
 *          the frontend but one slot in the IR.  An empty list lowers
 *          to nothing; exactly one expression lowers to it; anything
 *          else has no faithful encoding here.
 */

static CgIRExpr *lSingleListExpr(CgIRLower *L, stmt *fStmt, const char *what)
{
    expr *found;

    found = NULL;
    for (; fStmt != NULL; fStmt = fStmt->commonst.next) {
        if (fStmt->commonst.kind != EXPR_STMT ||
            fStmt->exprst.exp == NULL)
        {
            continue;
        }
        if (found != NULL) {
            lUnlowerable(L, what);
            return NULL;
        }
        found = fStmt->exprst.exp;
    }
    return found ? lLowerExpr(L, found) : NULL;
} // lSingleListExpr

/*
 * lLowerForStmt() - Init keeps statement shape (wrapped when comma-
 *          widened); step must collapse to one expression.
 */

static int lLowerForStmt(CgIRLower *L, stmt *fStmt, CgIRStmt **list)
{
    CgIRStmt *stmt, *init, *body;
    CgIRExpr *condition, *step;

    init = NULL;
    if (fStmt->forst.init != NULL) {
        CgIRStmt *initList;

        initList = NULL;
        if (!lLowerStmtList(L, fStmt->forst.init, &initList))
            return 0;
        if (initList != NULL && initList->next != NULL) {
            stmt = CgIRNewBlockStmt(L->module,
                                    &fStmt->forst.init->commonst.loc);
            if (stmt == NULL)
                return 0;
            stmt->u.block = initList;
            initList = stmt;
        }
        init = initList;
    }
    condition = fStmt->forst.cond ? lLowerExpr(L, fStmt->forst.cond) : NULL;
    if (fStmt->forst.cond != NULL && condition == NULL)
        return 0;
    step = lSingleListExpr(L, fStmt->forst.step, "for step sequence");
    if (step == NULL && L->module->failed)
        return 0;
    body = lBranchBody(L, fStmt->forst.body);
    if (body == NULL)
        return 0;
    stmt = CgIRNewForStmt(L->module, &L->loc, init, condition, step, body);
    if (stmt == NULL)
        return 0;
    CgIRAppendStmt(list, stmt);
    return 1;
} // lLowerForStmt

static int lLowerStmtList(CgIRLower *L, stmt *fStmt, CgIRStmt **list)
{
    for (; fStmt != NULL; fStmt = fStmt->commonst.next) {
        if (!lLowerStmt(L, fStmt, list))
            return 0;
        if (L->module->failed)
            return 0;
    }
    return 1;
} // lLowerStmtList

///////////////////////////// Function lowering //////////////////////////////

/*
 * Locals collection: every variable declared in one of the function's
 * own body scopes (the formal scope excepted), ordered by stable source
 * declaration order so DECL statements read like the source.
 */

typedef struct LocalListRec {
    Symbol **symbols;
    int count;
    int capacity;
} LocalList;

static int lLocalsAdd(LocalList *list, Symbol *symbol)
{
    if (list->count >= list->capacity) {
        int capacity;
        Symbol **grown;

        capacity = list->capacity > 0 ? list->capacity * 2 : 16;
        grown = (Symbol **) (realloc)(list->symbols,
                                      capacity * sizeof(Symbol *));
        if (grown == NULL)
            return 0;
        list->symbols = grown;
        list->capacity = capacity;
    }
    list->symbols[list->count++] = symbol;
    return 1;
} // lLocalsAdd

static void lLocalsSort(LocalList *list)
{
    int ii, kk;

    for (ii = 1; ii < list->count; ii++) {
        Symbol *symbol;

        symbol = list->symbols[ii];
        kk = ii - 1;
        while (kk >= 0 &&
               list->symbols[kk]->sourceOrdinal > symbol->sourceOrdinal)
        {
            list->symbols[kk + 1] = list->symbols[kk];
            kk--;
        }
        list->symbols[kk + 1] = symbol;
    }
} // lLocalsSort

/*
 * lCollectTreeSymbols() - Scope symbol tables are binary trees ordered
 *          by bit-reversed atom; the ->next chain only links overload
 *          sets, so every node must be visited through left/right.
 */

static int lCollectTreeSymbols(Symbol *root, LocalList *out)
{
    if (root == NULL)
        return 1;
    if (root->kind == VARIABLE_S && !lLocalsAdd(out, root))
        return 0;
    return lCollectTreeSymbols(root->left, out) &&
           lCollectTreeSymbols(root->right, out);
} // lCollectTreeSymbols

/*
 * lIsDeclaredFormal() - The function body parses as a block_item_list
 *          inside the formal scope, so locals share that scope with the
 *          formals; only the declared parameter chain distinguishes
 *          them.
 */

static int lIsDeclaredFormal(Symbol *symbol, Symbol *formals)
{
    for (; formals != NULL; formals = formals->next) {
        if (formals == symbol)
            return 1;
    }
    return 0;
} // lIsDeclaredFormal

static int lCollectLocals(Symbol *function, LocalList *out)
{
    Scope *funScope, *scope, *inner;

    out->symbols = NULL;
    out->count = 0;
    out->capacity = 0;
    funScope = function->details.fun.locals;
    if (funScope == NULL || funScope->funindex == 0)
        return 1;
    for (scope = ScopeList; scope != NULL; scope = scope->next) {
        if (scope->funindex != funScope->funindex)
            continue;
        for (inner = scope; inner != NULL && inner != funScope;
             inner = inner->parent)
        {
        }
        if (inner != funScope)
            continue;
        {
            LocalList scoped;
            int ii;

            scoped.symbols = NULL;
            scoped.count = 0;
            scoped.capacity = 0;
            if (!lCollectTreeSymbols(scope->symbols, &scoped)) {
                (free)(scoped.symbols);
                (free)(out->symbols);
                out->symbols = NULL;
                return 0;
            }
            for (ii = 0; ii < scoped.count; ii++) {
                if (!lIsDeclaredFormal(scoped.symbols[ii],
                                       function->details.fun.params) &&
                    !lLocalsAdd(out, scoped.symbols[ii]))
                {
                    (free)(scoped.symbols);
                    (free)(out->symbols);
                    out->symbols = NULL;
                    return 0;
                }
            }
            (free)(scoped.symbols);
        }
    }
    lLocalsSort(out);
    return 1;
} // lCollectLocals

/*
 * lLowerFunction() - Parameters, source-ordered DECL statements, then
 *          the body.  The entry lowers with a void result: program
 *          outputs already flow through explicit $vout connector
 *          assignments that BuildSemanticStructs and the return
 *          rewrite placed in the statement list.
 */

static CgIRFunction *lLowerFunction(CgIRLower *L, Symbol *symbol)
{
    CgIRFunction *function;
    CgIRDecl *decl;
    CgIRStmt *body, *declStmts, *block;
    LocalList locals;
    Type *resultType;
    Symbol *formal;
    int ii, isEntry;

    isEntry = (symbol == L->entry);
    resultType = VoidType;
    if (!isEntry) {
        resultType = symbol->type->fun.rettype;
        if (resultType == NULL) {
            lUnlowerable(L, "function without result type");
            return NULL;
        }
    }
    function = CgIRNewFunction(L->module, symbol, resultType, &symbol->loc);
    if (function == NULL)
        return NULL;
    L->function = symbol;
    L->funScope = symbol->details.fun.locals;
    L->irFunction = function;

    /* Formals lower in declaration order; methods keep their prepended
     * receiver as an ordinary first parameter. */
    for (formal = symbol->details.fun.params; formal != NULL;
         formal = formal->next)
    {
        L->loc = formal->loc;
        decl = lNewDecl(L, formal, formal->type,
                        isEntry ? CGIR_DOMAIN_VARYING : CGIR_DOMAIN_NONE,
                        NULL);
        if (decl == NULL)
            return NULL;
        CgIRAppendDecl(&function->parameters, decl);
    }

    /* Visibility authority: every local declares before the body runs. */
    body = NULL;
    declStmts = NULL;
    if (!lCollectLocals(symbol, &locals)) {
        /* Collection itself ran out of memory; mirror the sticky
         * module-failure convention so nothing downstream proceeds. */
        L->module->failed = 1;
        return NULL;
    }
    for (ii = 0; ii < locals.count; ii++) {
        L->loc = locals.symbols[ii]->loc;
        decl = lNewDecl(L, locals.symbols[ii], locals.symbols[ii]->type,
                        CGIR_DOMAIN_NONE, NULL);
        if (decl == NULL)
            break;
        CgIRAppendDecl(&function->locals, decl);
        {
            CgIRStmt *stmt;

            stmt = CgIRNewDeclStmt(L->module, &L->loc, decl);
            if (stmt == NULL)
                break;
            CgIRAppendStmt(&declStmts, stmt);
        }
    }
    (free)(locals.symbols);
    if (L->module->failed)
        return NULL;

    /* The entry prologue runs the global initializers first, matching
     * the legacy ConcatStmts(fScope->initStmts, ...) placement. */
    if (isEntry && !lLowerStmtList(L, L->globalScope->initStmts, &body))
        return NULL;
    if (!lLowerStmtList(L, symbol->details.fun.statements, &body))
        return NULL;

    block = CgIRNewBlockStmt(L->module, &symbol->loc);
    if (block == NULL)
        return NULL;
    block->u.block = declStmts;
    if (declStmts != NULL) {
        while (declStmts->next != NULL)
            declStmts = declStmts->next;
        declStmts->next = body;
    } else {
        block->u.block = body;
    }
    function->body = block;

    if (isEntry) {
        function->isEntry = 1;
        L->module->entry = function;
    }
    CgIRAppendFunction(&L->module->functions, function);
    return function;
} // lLowerFunction

/*
 * lLowerGlobal() - One module-scope variable.  Non-static initializer
 *          data rides the declaration; static globals initialize
 *          through the entry-prologue statements instead.
 */

static int lLowerGlobal(CgIRLower *L, Symbol *symbol)
{
    CgIRDecl *decl;
    expr *initExpr;
    CgIRExpr *initializer;

    initializer = NULL;
    initExpr = symbol->details.var.init;
    if (initExpr != NULL) {
        if (initExpr->common.kind == BINARY_N &&
            initExpr->bin.op == EXPR_LIST_OP)
        {
            initExpr = initExpr->bin.left;
        }
        L->loc = symbol->loc;
        initializer = lLowerExpr(L, initExpr);
        if (initializer == NULL)
            return 0;
    }
    L->loc = symbol->loc;
    decl = lNewDecl(L, symbol, symbol->type, CGIR_DOMAIN_UNIFORM,
                    initializer);
    if (decl == NULL)
        return 0;
    CgIRAppendDecl(&L->module->globals, decl);
    return 1;
} // lLowerGlobal

/////////////////////////////// Program entry ////////////////////////////////

int CgIRLowerProgram(CgIRLowerContext *context, Scope *globalScope,
                     Symbol *entry)
{
    CgIRLower L;
    int index;

    if (context == NULL || context->module == NULL ||
        context->reach == NULL || globalScope == NULL || entry == NULL)
    {
        return 0;
    }
    /* Borrowed profile identity: it lives in the HAL, which outlives
     * every module built during one compilation. */
    context->module->profile = &Cg->theHAL->profileIdentity;

    L.module = context->module;
    L.globalScope = globalScope;
    L.entry = entry;
    L.function = NULL;
    L.funScope = NULL;
    L.irFunction = NULL;
    L.tempCounter = 0;

    for (index = 0; index < context->reach->nodeCount; index++) {
        Symbol *symbol;

        if (CgIRModuleFailed(L.module))
            return 0;
        symbol = context->reach->nodes[index].symbol;
        switch (symbol->kind) {
        case FUNCTION_S:
            if (lLowerFunction(&L, symbol) == NULL)
                return 0;
            break;
        case VARIABLE_S:
            if (!lLowerGlobal(&L, symbol))
                return 0;
            break;
        default:
            break;
        }
    }
    return !CgIRModuleFailed(context->module);
} // CgIRLowerProgram

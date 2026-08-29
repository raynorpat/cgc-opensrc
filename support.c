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
NON-INFRINGEMENT, MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, OR
ITS USE AND OPERATION EITHER ALONE OR IN COMBINATION WITH OTHER
PRODUCTS.

IN NO EVENT SHALL NVIDIA BE LIABLE FOR ANY SPECIAL, INDIRECT,
INCIDENTAL, EXEMPLARY, CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
TO, LOST PROFITS; PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) OR ARISING IN ANY WAY
OUT OF THE USE, REPRODUCTION, MODIFICATION AND/OR DISTRIBUTION OF THE
NVIDIA SOFTWARE, HOWEVER CAUSED AND WHETHER UNDER THEORY OF CONTRACT,
TORT (INCLUDING NEGLIGENCE), STRICT LIABILITY OR OTHERWISE, EVEN IF
NVIDIA HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
\****************************************************************************/

//
// support.c
//

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "slglobals.h"
#include "glsl_hal.h"
#include "language.h"
#include "cg_stdlib.h"

dtype CurrentDeclTypeSpecs = { 0, };

#undef PICK
#define PICK(a, b, c, d, e) d

const nodekind NodeKind[] = {
    OPCODE_TABLE
};

#undef PICK

#define PICK(a, b, c, d, e) c

int const opcode_atom[] = {
    OPCODE_TABLE
};

#undef PICK

/*
 * NewDeclNode() - Create a new declaration node.
 *
 */
 
decl *NewDeclNode(SourceLoc *loc, int atom, dtype *type)
{
    decl *pdecl;

    pdecl = (decl *) malloc(sizeof(decl));
    pdecl->kind = DECL_N;
    pdecl->loc = *loc;
    pdecl->name = atom;
    pdecl->semantics = 0;
    pdecl->type = *type;
    pdecl->next = NULL;
    pdecl->symb = NULL;
    pdecl->params = NULL;
    pdecl->initexpr = NULL;
    return pdecl;
} // NewDeclNode

/*
 * NewSymbNode() - Create a new symbol node.
 *
 */
 
symb *NewSymbNode(opcode op, Symbol *fSymb)
{
    symb *psymb;

    assert(NodeKind[op] == SYMB_N);
    psymb = (symb *) malloc(sizeof(symb));
    psymb->kind = SYMB_N;
    psymb->type = fSymb->type;
    psymb->IsLValue = 1;
    if (GetQualifiers(psymb->type) & TYPE_QUALIFIER_CONST) {
        psymb->IsConst = 1;
    } else {
        psymb->IsConst = 0;
    }
    psymb->HasSideEffects = 0;
    psymb->op = op;
    psymb->symbol = fSymb;
    return psymb;
} // NewSymbNode

/*
 * lLegacyBaseForKind() - Legacy four-bit base caching a canonical scalar
 *                        kind in "constant.subop".  Kinds without a base of
 *                        their own share the int or float legacy base.
 *
 */

static int lLegacyBaseForKind(CgScalarKind kind)
{
    switch (kind) {
    case CG_SCALAR_CFLOAT:
        return TYPE_BASE_CFLOAT;
    case CG_SCALAR_CINT:
        return TYPE_BASE_CINT;
    case CG_SCALAR_BOOL:
        return TYPE_BASE_BOOLEAN;
    case CG_SCALAR_CHAR:
    case CG_SCALAR_UCHAR:
    case CG_SCALAR_SHORT:
    case CG_SCALAR_USHORT:
    case CG_SCALAR_INT:
    case CG_SCALAR_UINT:
    case CG_SCALAR_LONG:
    case CG_SCALAR_ULONG:
        return TYPE_BASE_INT;
    case CG_SCALAR_FIXED:
    case CG_SCALAR_HALF:
    case CG_SCALAR_FLOAT:
    case CG_SCALAR_DOUBLE:
        return TYPE_BASE_FLOAT;
    default:
        return TYPE_BASE_UNDEFINED_TYPE;
    }
} // lLegacyBaseForKind

/*
 * lKindForLegacyBase() - Canonical kind a legacy constructor's base stands
 *                        for; unknown bases keep the compile-time kinds.
 *
 */

static CgScalarKind lKindForLegacyBase(int base)
{
    switch (base) {
    case TYPE_BASE_CFLOAT:
        return CG_SCALAR_CFLOAT;
    case TYPE_BASE_BOOLEAN:
        return CG_SCALAR_BOOL;
    case TYPE_BASE_INT:
        return CG_SCALAR_INT;
    case TYPE_BASE_FLOAT:
        return CG_SCALAR_FLOAT;
    default:
        return CG_SCALAR_CINT;
    }
} // lKindForLegacyBase

/*
 * NewNumericConstNode() - Create a constant node holding one typed value.
 *
 * The node type comes from the interned standard types for the value's
 * canonical kind, and the legacy subop cache records the old four-bit base
 * when the kind has one.
 *
 */

constant *NewNumericConstNode(opcode op, const CgNumericValue *value)
{
    constant *pconst;

    assert(NodeKind[op] == CONST_N);
    assert(value);
    pconst = (constant *) malloc(sizeof(constant));
    pconst->kind = CONST_N;
    pconst->type = GetStandardTypeKind(value->kind, 0, 0);
    pconst->IsLValue = 0;
    pconst->IsConst = 0;
    pconst->HasSideEffects = 0;
    pconst->op = op;
    pconst->subop = SUBOP__(lLegacyBaseForKind(value->kind));
    pconst->val[0] = *value;
    pconst->tempptr[0] = 0;
    return pconst;
} // NewNumericConstNode

/*
 * NewIConstNode() - Create a new integer constant node.
 *
 */

constant *NewIConstNode(opcode op, int fval, int base)
{
    CgNumericValue value;

    assert(NodeKind[op] == CONST_N);
    CgNumericSetSigned(&value, lKindForLegacyBase(base), fval);
    return NewNumericConstNode(op, &value);
} // NewIConstNode

/*
 * NewBConstNode() - Create a new Boolean constant node.
 *
 */

constant *NewBConstNode(opcode op, int fval, int base)
{
    CgNumericValue value;

    assert(NodeKind[op] == CONST_N);
    CgNumericSetSigned(&value, lKindForLegacyBase(base), (fval != 0));
    return NewNumericConstNode(op, &value);
} // NewBConstNode

/*
 * NewFConstNode() - Create a new floating point constant node.
 *
 */

constant *NewFConstNode(opcode op, float fval, int base)
{
    CgNumericValue value;

    assert(NodeKind[op] == CONST_N);
    CgNumericSetFloat(&value, lKindForLegacyBase(base), fval);
    return NewNumericConstNode(op, &value);
} // NewFConstNode

/*
 * NewFConstNodeV() - Create a new floating point constant vector node.
 *
 */
 
constant *NewFConstNodeV(opcode op, float *fval, int len, int base)
{
    constant *pconst;
    CgScalarKind kind;
    int ii;

    assert(NodeKind[op] == CONST_N);
    pconst = (constant *) malloc(sizeof(constant));
    pconst->kind = CONST_N;
    pconst->type = GetStandardType(base, len, 0);
    pconst->IsLValue = 0;
    pconst->IsConst = 0;
    pconst->HasSideEffects = 0;
    pconst->op = op;
    pconst->subop = SUBOP_V(len, base);
    kind = lKindForLegacyBase(base);
    for (ii = 0; ii < len; ii++) {
        pconst->val[ii].kind = kind;
        pconst->val[ii].value.f = fval[ii];
    }
    pconst->tempptr[0] = 0;
    return pconst;
} // NewFConstNodeV

/*
 * NewUnopNode() - Create a unary op node.
 *
 */
 
unary *NewUnopNode(opcode op, expr *arg)
{
    unary *pun;

    assert(NodeKind[op] == UNARY_N);
    pun = (unary *) malloc(sizeof(unary));
    pun->kind = UNARY_N;
    pun->type = UndefinedType;
    pun->IsLValue = 0;
    pun->IsConst = 0;
    pun->HasSideEffects = 0;
    if (arg)
        pun->HasSideEffects = arg->common.HasSideEffects;
    pun->op = op;
    pun->subop = 0;
    pun->arg = arg;
    pun->targetType = NULL;
    pun->tempptr[0] = 0;
    return pun;
} // NewUnopNode

/*
 * NewUnopSubNode() - Create a unary op node.
 *
 */
 
unary *NewUnopSubNode(opcode op, int subop, expr *arg)
{
    unary *pun;

    assert(NodeKind[op] == UNARY_N);
    pun = (unary *) malloc(sizeof(unary));
    pun->kind = UNARY_N;
    pun->type = UndefinedType;
    pun->IsLValue = 0;
    pun->IsConst = 0;
    pun->HasSideEffects = 0;
    if (arg)
        pun->HasSideEffects = arg->common.HasSideEffects;
    pun->op = op;
    pun->subop = subop;
    pun->arg = arg;
    pun->targetType = NULL;
    pun->tempptr[0] = 0;
    return pun;
} // NewUnopSubNode

/*
 * NewBinopNode() - Create a binary op node.
 *
 */
 
binary *NewBinopNode(opcode op, expr *left, expr *right)
{
    binary *pbin;

    assert(NodeKind[op] == BINARY_N);
    pbin = (binary *) malloc(sizeof(binary));
    pbin->kind = BINARY_N;
    pbin->type = UndefinedType;
    pbin->IsLValue = 0;
    pbin->IsConst = 0;
    pbin->HasSideEffects = 0;
    if (left)
        pbin->HasSideEffects = left->common.HasSideEffects;
    if (right)
        pbin->HasSideEffects |= right->common.HasSideEffects;
    pbin->op = op;
    pbin->subop = 0;
    pbin->left = left;
    pbin->right = right;
    pbin->tempptr[0] = 0;
    pbin->tempptr[1] = 0;
    return pbin;
} // NewBinopNode

/*
 * NewBinopSubNode() - Create a binary op node.
 *
 */
 
binary *NewBinopSubNode(opcode op, int subop, expr *left, expr *right)
{
    binary *pbin;

    assert(NodeKind[op] == BINARY_N);
    pbin = (binary *) malloc(sizeof(binary));
    pbin->kind = BINARY_N;
    pbin->type = UndefinedType;
    pbin->IsLValue = 0;
    pbin->IsConst = 0;
    pbin->HasSideEffects = 0;
    if (left)
        pbin->HasSideEffects = left->common.HasSideEffects;
    if (right)
        pbin->HasSideEffects |= right->common.HasSideEffects;
    pbin->op = op;
    pbin->subop = subop;
    pbin->left = left;
    pbin->right = right;
    pbin->tempptr[0] = 0;
    pbin->tempptr[1] = 0;
    return pbin;
} // NewBinopSubNode

/*
 * NewTriopNode() - Create a trinary op node.
 *
 */
 
trinary *NewTriopNode(opcode op, expr *arg1, expr *arg2, expr *arg3)
{
    trinary *ptri;

    assert(NodeKind[op] == TRINARY_N);
    ptri = (trinary *) malloc(sizeof(trinary));
    ptri->kind = TRINARY_N;
    ptri->type = UndefinedType;
    ptri->IsLValue = 0;
    ptri->IsConst = 0;
    ptri->HasSideEffects = 0;
    if (arg1)
        ptri->HasSideEffects = arg1->common.HasSideEffects;
    if (arg2)
        ptri->HasSideEffects |= arg2->common.HasSideEffects;
    if (arg3)
        ptri->HasSideEffects |= arg3->common.HasSideEffects;
    ptri->op = op;
    ptri->subop = 0;
    ptri->arg1 = arg1;
    ptri->arg2 = arg2;
    ptri->arg3 = arg3;
    ptri->tempptr[0] = 0;
    ptri->tempptr[1] = 0;
    ptri->tempptr[2] = 0;
    return ptri;
} // NewTriopNode

/*
 * NewTriopSubNode() - Create a trinary op node.
 *
 */
 
trinary *NewTriopSubNode(opcode op, int subop, expr *arg1, expr *arg2, expr *arg3)
{
    trinary *ptri;

    assert(NodeKind[op] == TRINARY_N);
    ptri = (trinary *) malloc(sizeof(trinary));
    ptri->kind = TRINARY_N;
    ptri->type = UndefinedType;
    ptri->IsLValue = 0;
    ptri->IsConst = 0;
    ptri->HasSideEffects = 0;
    if (arg1)
        ptri->HasSideEffects = arg1->common.HasSideEffects;
    if (arg2)
        ptri->HasSideEffects |= arg2->common.HasSideEffects;
    if (arg3)
        ptri->HasSideEffects |= arg3->common.HasSideEffects;
    ptri->op = op;
    ptri->subop = subop;
    ptri->arg1 = arg1;
    ptri->arg2 = arg2;
    ptri->arg3 = arg3;
    ptri->tempptr[0] = 0;
    ptri->tempptr[1] = 0;
    ptri->tempptr[2] = 0;
    return ptri;
} // NewTriopSubNode

/*
 * DupSymbNode() - Duplicate a symb op node.
 *
 */
 
symb *DupSymbNode(const symb *fsymb)
{
    symb *lsymb;

    lsymb = (symb *) malloc(sizeof(symb));
    *lsymb = *fsymb;
    return lsymb;
} // DupSymbNode

/*
 * DupConstNode() - Duplicate a constant op node.
 *
 */
 
constant *DupConstNode(const constant *fconst)
{
    constant *lconst;

    lconst = (constant *) malloc(sizeof(constant));
    *lconst = *fconst;
    return lconst;
} // DupConstNode

/*
 * DupUnaryNode() - Duplicate a unary op node.
 *
 */
 
unary *DupUnaryNode(const unary *fun)
{
    unary *lun;

    lun = (unary *) malloc(sizeof(unary));
    *lun = *fun;
    return lun;
} // DupUnaryNode

/*
 * DupBinaryNode() - Duplicate a binary op node.
 *
 */
 
binary *DupBinaryNode(const binary *fbin)
{
    binary *lbin;

    lbin = (binary *) malloc(sizeof(binary));
    *lbin = *fbin;
    return lbin;
} // DupBinaryNode

/*
 * DupTrinaryNode() - Duplicate a trinary op node.
 *
 */
 
trinary *DupTrinaryNode(const trinary *ftri)
{
    trinary *ltri;

    ltri = (trinary *) malloc(sizeof(trinary));
    *ltri = *ftri;
    return ltri;
} // DupTrinaryNode


/*
 * DupNode() - Duplicate a expression node.
 *
 */
 
expr *DupNode(const expr *fExpr)
{
    switch (fExpr->common.kind) {
    case SYMB_N: return (expr *) DupSymbNode(&fExpr->sym);
    case CONST_N: return (expr *) DupConstNode(&fExpr->co);
    case UNARY_N: return (expr *) DupUnaryNode(&fExpr->un);
    case BINARY_N: return (expr *) DupBinaryNode(&fExpr->bin);
    case TRINARY_N: return (expr *) DupTrinaryNode(&fExpr->tri);
    }

    FatalError("unsupported node type in DupNode");
    return NULL;
} // DupNode


/*
 * NewExprStmt() - Create an expression statement.
 *
 */
 
expr_stmt *NewExprStmt(SourceLoc *loc, expr *fExpr)
{
    expr_stmt *lStmt;

    lStmt = (expr_stmt *) malloc(sizeof(expr_stmt));
    lStmt->kind = EXPR_STMT;
    lStmt->next = NULL;
    lStmt->loc = *loc;
    lStmt->exp = fExpr;
    return lStmt;
} // NewExprStmt

/*
 * NewSimpleStmt() - Create a leaf statement with no expression or children.
 *
 */

common_stmt *NewSimpleStmt(SourceLoc *loc, stmtkind kind)
{
    common_stmt *lStmt;

    lStmt = (common_stmt *) malloc(sizeof(common_stmt));
    lStmt->kind = kind;
    lStmt->next = NULL;
    lStmt->loc = *loc;
    return lStmt;
} // NewSimpleStmt

/*
 * NewIfStmt() - Create an expression statement.
 *
 */
 
if_stmt *NewIfStmt(SourceLoc *loc, expr *fExpr, stmt *thenstmt, stmt *elsestmt)
{
    if_stmt *lStmt;

    lStmt = (if_stmt *) malloc(sizeof(if_stmt));
    lStmt->kind = IF_STMT;
    lStmt->next = NULL;
    lStmt->loc = *loc;
    lStmt->cond = fExpr;
    lStmt->thenstmt = thenstmt;
    lStmt->elsestmt = elsestmt;
    return lStmt;
} // NewIfStmt

/*
 * NewIfStmt() - Create an expression statement.
 *
 */
 
if_stmt *SetThenElseStmts(SourceLoc *loc, stmt *ifstmt, stmt *thenstmt, stmt *elsestmt)
{
    if_stmt *lStmt;

    lStmt = (if_stmt *) ifstmt;
    assert(lStmt->kind == IF_STMT);
    lStmt->thenstmt = thenstmt;
    lStmt->elsestmt = elsestmt;
    return lStmt;
} // NewIfStmt

/*
 * NewWhileStmt() - Create a while statement.
 *
 */
 
while_stmt *NewWhileStmt(SourceLoc *loc, stmtkind kind, expr *fExpr, stmt *body)
{
    while_stmt *lStmt;

    lStmt = (while_stmt *) malloc(sizeof(while_stmt));
    lStmt->kind = kind;
    lStmt->next = NULL;
    lStmt->loc = *loc;
    lStmt->cond = fExpr;
    lStmt->body = body;
    return lStmt;
} // NewWhileStmt

/*
 * NewForStmt() - Create a for statement.
 *
 */
 
for_stmt *NewForStmt(SourceLoc *loc, stmt *fexpr1, expr *fexpr2, stmt *fexpr3, stmt *body)
{
    for_stmt *lStmt;

    lStmt = (for_stmt *) malloc(sizeof(for_stmt));
    lStmt->kind = FOR_STMT;
    lStmt->next = NULL;
    lStmt->loc = *loc;
    lStmt->init = fexpr1;
    lStmt->cond = fexpr2;
    lStmt->step = fexpr3;
    lStmt->body = body;
    return lStmt;
} // NewForStmt

/*
 * NewBlockStmt() - Create a block statement.
 *
 */
 
block_stmt *NewBlockStmt(SourceLoc *loc, stmt *fStmt)
{
    block_stmt *lStmt;

    lStmt = (block_stmt *) malloc(sizeof(block_stmt));
    lStmt->kind = BLOCK_STMT;
    lStmt->next = NULL;
    lStmt->loc = *loc;
    lStmt->body = fStmt;
    return lStmt;
} // NewBlockStmt

/*
 * NewReturnStmt() - Create an expression statement.
 *
 */
 
return_stmt *NewReturnStmt(SourceLoc *loc, Scope *fScope, expr *fExpr)
{
    return_stmt *lStmt;
    expr *lExpr;

    if (fScope) {
        while (fScope->level > 2)
            fScope = fScope->next;
        fScope->HasReturnStmt = 1;
        if ((fExpr && IsSampler(fExpr->common.type, NULL)) ||
            IsSampler(fScope->returnType, NULL))
        {
            SemanticError(loc, ERROR___SAMPLER_RETURN);
        }
        if (fScope->returnType) {
            if (fScope->returnType == VoidType) {
                if (fExpr) {
                    SemanticError(loc, ERROR___VOID_FUN_RETURNS_VALUE);
                }
            } else if (!CgTypeIsPoison(fScope->returnType) &&
                       !(fExpr && CgTypeIsPoison(fExpr->common.type))) {
                if (ConvertType(loc, fExpr, fScope->returnType, fExpr->common.type, &lExpr, 0, 0, 1)) {
                    fExpr = lExpr;
                } else {
                    SemanticError(loc, ERROR___RETURN_EXPR_INCOMPAT);
                }
            }
        }
    }
    lStmt = (return_stmt *) malloc(sizeof(return_stmt));
    lStmt->kind = RETURN_STMT;
    lStmt->next = NULL;
    lStmt->loc = *loc;
    lStmt->exp = fExpr;
    return lStmt;
} // NewReturnStmt

/*
 * NewDiscardStmt() - Create a discard statement.
 *
 */
 
discard_stmt *NewDiscardStmt(SourceLoc *loc, expr *fExpr)
{
    discard_stmt *lStmt;
    int len;

    lStmt = (discard_stmt *) malloc(sizeof(discard_stmt));
    lStmt->kind = DISCARD_STMT;
    lStmt->next = NULL;
    lStmt->loc = *loc;
    if (fExpr && IsVector(fExpr->common.type, &len)) {
        /* empty */ ;
    } else {
        len = 0;
    }
    if (fExpr == NULL || fExpr->common.kind != UNARY_N ||
        fExpr->un.op != KILL_OP)
    {
        fExpr = (expr *) NewUnopSubNode(KILL_OP,
                                        SUBOP_V(len, TYPE_BASE_BOOLEAN),
                                        fExpr);
    }
    lStmt->cond = fExpr;
    return lStmt;
} // NewDiscardStmt

/*
 * NewCommentStmt() - Create a comment statement.
 *
 */
 
comment_stmt *NewCommentStmt(SourceLoc *loc, const char *str)
{
    comment_stmt *lStmt;

    lStmt = (comment_stmt *) malloc(sizeof(comment_stmt));
    lStmt->kind = COMMENT_STMT;
    lStmt->next = NULL;
    lStmt->loc = *loc;
    lStmt->str = AddAtom(atable, str);
    return lStmt;
} // NewCommentStmt

/************************************* dtype functions: *************************************/

///////////////////////////////////////////////////////////////////////////////////////////////
//////////////////// Geometry topology modifier recording: ////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////

/*
 * Modifier tokens reduce at the head of a declaration-specifier run,
 * before the base type keyword has initialised CurrentDeclTypeSpecs.
 * Each request is validated and parked here; SetDType() initialises
 * the pending record into the dtype it is setting up, which both
 * applies it at the right time and keeps one declaration's modifiers
 * out of the next one.  ClearPendingGeometryModifiers() drops an
 * unconsumed record when parser recovery abandons a declaration.
 */

static CgGeometryModifiers lPendingGeometryModifiers;

void ClearPendingGeometryModifiers(void)
{
    CgGeometryInitModifiers(&lPendingGeometryModifiers);
} // ClearPendingGeometryModifiers

/*
 * SetGeometryInputModifier() - Parser-facing application of one input
 *         topology token.  The version gate reports the stable Cg 2.0
 *         language diagnostic at the token and returns zero so parsing
 *         recovers at the declaration boundary.  The pure helper
 *         validates against the pending record, whose structured reason
 *         maps to the stable repeated (C6303) or conflicting (C6304)
 *         diagnostics naming the first location.  "specifiers" receives
 *         the record when SetDType runs, not here, so the scratch dtype
 *         never sees a half-initialised state.
 *
 * Returns: TRUE if O.K.
 *
 */

int SetGeometryInputModifier(SourceLoc *loc, dtype *specifiers, CgGeometryInput input)
{
    CgGeometryDiagnostic diagnostic;

    if (!CgLanguageAllowsGeometry(Cg->options.languageVersion)) {
        SemanticError(loc, ERROR___REQUIRES_CG_20_LANGUAGE);
        return 0;
    }
    if (!CgGeometryApplyInputModifier(&lPendingGeometryModifiers, input,
                                      loc, &diagnostic)) {
        if (diagnostic.reason == CG_GEOMETRY_DIAGNOSTIC_REPEATED_INPUT) {
            SemanticError(loc, ERROR_S_GEOMETRY_REPEATED_MODIFIER,
                          CgGeometryInputName(input));
        } else {
            SemanticError(loc, ERROR_SS_GEOMETRY_MODIFIER_CONFLICT,
                          CgGeometryInputName(input),
                          CgGeometryInputName(lPendingGeometryModifiers.input));
        }
        return 0;
    }
    return 1;
} // SetGeometryInputModifier

/*
 * SetGeometryOutputModifier() - Output-token twin of
 *         SetGeometryInputModifier over the independent output slot.
 *
 * Returns: TRUE if O.K.
 *
 */

int SetGeometryOutputModifier(SourceLoc *loc, dtype *specifiers, CgGeometryOutput output)
{
    CgGeometryDiagnostic diagnostic;

    if (!CgLanguageAllowsGeometry(Cg->options.languageVersion)) {
        SemanticError(loc, ERROR___REQUIRES_CG_20_LANGUAGE);
        return 0;
    }
    if (!CgGeometryApplyOutputModifier(&lPendingGeometryModifiers, output,
                                       loc, &diagnostic)) {
        if (diagnostic.reason == CG_GEOMETRY_DIAGNOSTIC_REPEATED_OUTPUT) {
            SemanticError(loc, ERROR_S_GEOMETRY_REPEATED_MODIFIER,
                          CgGeometryOutputName(output));
        } else {
            SemanticError(loc, ERROR_SS_GEOMETRY_MODIFIER_CONFLICT,
                          CgGeometryOutputName(output),
                          CgGeometryOutputName(lPendingGeometryModifiers.output));
        }
        return 0;
    }
    return 1;
} // SetGeometryOutputModifier

/*
 * SetAttribArrayType() - Parser-facing construction of one unresolved
 *         AttribArray source type.  Like the topology-modifier
 *         wrappers, the version gate reports the stable Cg 2.0
 *         language diagnostic at the token; the pure interner in
 *         cg_types.c then builds the canonical identity at extent
 *         zero.  Element legality is checked later by declaration
 *         validation, not here.
 *
 * Returns: The interned unresolved attribute-array type.
 *
 */

Type *SetAttribArrayType(SourceLoc *loc, Type *element)
{
    if (!CgLanguageAllowsGeometry(Cg->options.languageVersion)) {
        SemanticError(loc, ERROR___REQUIRES_CG_20_LANGUAGE);
        return UndefinedType;
    }
    return CgGetAttribArrayType(element, 0);
} // SetAttribArrayType

/*
 * lMergeGeometryModifiers() - Carry a declaration's source modifiers
 *         onto a function symbol.  Fields the newer record leaves
 *         unknown keep their previous values, so a definition may
 *         restate or extend its prototype's modifiers without losing
 *         any.
 */
static void lMergeGeometryModifiers(CgGeometryModifiers *dst,
                                    const CgGeometryModifiers *src)
{
    if (src->input != CG_GEOMETRY_INPUT_UNKNOWN) {
        dst->input = src->input;
        dst->inputLoc = src->inputLoc;
    }
    if (src->output != CG_GEOMETRY_OUTPUT_UNKNOWN) {
        dst->output = src->output;
        dst->outputLoc = src->outputLoc;
    }
} // lMergeGeometryModifiers

/*
 * lRejectGeometryModifiersOnNonFunction() - Topology modifiers qualify
 *         functions only; an ordinary object carrying a nonempty
 *         modifier record is rejected here so no later stage ever sees
 *         modifiers off a function.
 */

static void lRejectGeometryModifiersOnNonFunction(SourceLoc *loc,
                                                  const dtype *fDtype)
{
    if (fDtype->geometry.input == CG_GEOMETRY_INPUT_UNKNOWN &&
        fDtype->geometry.output == CG_GEOMETRY_OUTPUT_UNKNOWN) {
        return;
    }
    SemanticError(loc, ERROR___GEOMETRY_MODIFIER_FUNCTION);
} // lRejectGeometryModifiersOnNonFunction

/*
 * GetTypePointer() - Strange function that returns a pointer to the type defined by it's
 *         argument.  There are 2 cases:
 *
 * A) IsDerived is TRUE:  This type is a stack-frame resident copy of another type.
 *          It has been modified by a qualifier, etc., and does not have a copy in the heap.
 *          Copy the contents into a freshly malloc'ed type and return it's address.
 * B) IsDerived is FALSE: This type is the same as that pointed to by "base".  Return "base".
 */

Type *GetTypePointer(SourceLoc *loc, const dtype *fDtype)
{
    Type *pType;

    if (fDtype) {
        if (fDtype->IsDerived) {
            if (Cg->theHAL->CheckDeclarators(loc, fDtype))
                ; /* empty statement */
            pType = NewType(0, 0);
            *pType = fDtype->type;
            pType->properties &= ~(TYPE_MISC_TYPEDEF | TYPE_MISC_PACKED_KW);
            pType->co.size = Cg->theHAL->GetSizeof(pType);
        } else {
            pType = fDtype->basetype;
        }
    } else {
        pType = UndefinedType;
    }
    return pType;
} // GetTypePointer

/*
 * SetDType() - Set the fields of a dtype to match a type.
 *
 */

dtype *SetDType(dtype *fDtype, Type *fType)
{
    fDtype->basetype = fType;
    fDtype->IsDerived = 0;
    fDtype->numNewDims = 0;
    fDtype->storageClass = SC_UNKNOWN;
    /* Topology modifier tokens reduce before the base type keyword, so
     * their record is parked in the pending store and initialised into
     * the dtype here, exactly when the rest of the dtype is set up.
     * Consuming the pending record also keeps one declaration's
     * modifiers out of the next one. */
    fDtype->geometry = lPendingGeometryModifiers;
    CgGeometryInitModifiers(&lPendingGeometryModifiers);
    fDtype->type = *fType;
    return fDtype;
} // SetDType

/*
 * NewDType() - Initialize the fields of a dtype.
 *
 */

dtype *NewDType(dtype *fDtype, Type *baseType, int category)
{
    fDtype->basetype = baseType;
    fDtype->IsDerived = 1;
    fDtype->numNewDims = 0;
    fDtype->storageClass = SC_UNKNOWN;
    InitType(&fDtype->type);
    fDtype->type.properties = category;
    return fDtype;
} // NewDType

/*
 * SetTypeCategory() - Set the category of a type.  Issue an error if it's already set to a
 *         conflicting category.
 *
 * Returns: TRUE if O.K.
 *
 */

int SetTypeCategory(SourceLoc *loc, int atom, dtype *fType, int category, int Force)
{
    int lcategory;

    lcategory = fType->type.properties & TYPE_CATEGORY_MASK;
    if (Force || lcategory == TYPE_CATEGORY_NONE) {
        fType->type.properties &= ~TYPE_CATEGORY_MASK;
        fType->type.properties |= category;
        fType->IsDerived = 1;
    } else {
        if (lcategory != category) {
            SemanticError(loc, ERROR_S_CONFLICTING_DECLARATION, GetAtomString(atable, atom));
            return 0;
        }
    }
    return 1;
} // SetTypeCategory

/*
 * SetTypeQualifiers() - Set a type's qualifier bits.  Issue an error if any bit is already set.
 *
 * Returns: TRUE if O.K.
 *
 */

int SetTypeQualifiers(SourceLoc *loc, dtype *fType, int qualifiers)
{
    int lqualifiers;

    qualifiers &= TYPE_QUALIFIER_MASK;
    lqualifiers = fType->type.properties & TYPE_QUALIFIER_MASK;
    if (lqualifiers & qualifiers) {
        SemanticWarning(loc, WARNING___QUALIFIER_SPECIFIED_TWICE);
    }
    if (lqualifiers != qualifiers) {
        fType->type.properties |= qualifiers & TYPE_QUALIFIER_MASK;
        fType->IsDerived = 1;
        if ((fType->type.properties & (TYPE_QUALIFIER_CONST | TYPE_QUALIFIER_OUT)) ==
            (TYPE_QUALIFIER_CONST | TYPE_QUALIFIER_OUT))
        {
            SemanticError(loc, ERROR___CONST_OUT_INVALID);
        }
    }
    return 1;
} // SetTypeCategory

/*
 * SetTypeDomain() - Set the domain of a type.  Issue an error if it's already set to a
 *         conflicting domain.
 *
 * Returns: TRUE if O.K.
 *
 */

int SetTypeDomain(SourceLoc *loc, dtype *fType, int domain)
{
    int ldomain;

    ldomain = fType->type.properties & TYPE_DOMAIN_MASK;
    if (ldomain == TYPE_DOMAIN_UNKNOWN) {
        fType->type.properties &= ~TYPE_DOMAIN_MASK;
        fType->type.properties |= domain;
        fType->IsDerived = 1;
    } else {
        if (ldomain == domain) {
            SemanticWarning(loc, WARNING___DOMAIN_SPECIFIED_TWICE);
        } else {
            SemanticError(loc, ERROR___CONFLICTING_DOMAIN);
            return 0;
        }
    }
    return 1;
} // SetTypeDomain

/*
 * SetTypeMisc() - Set a bit in the misc field a type.  Issue an error if it's already set.
 *
 * Returns: TRUE if O.K.
 *
 */

int SetTypeMisc(SourceLoc *loc, dtype *fType, int misc)
{
    if (fType) {
        if (fType->type.properties & misc) {
            SemanticError(loc, ERROR___REPEATED_TYPE_ATTRIB);
            return 0;
        }
        if (misc & ~TYPE_MISC_TYPEDEF)
            fType->IsDerived = 1;
        fType->type.properties |= misc;
        return 1;
    }
    return 0;
} // SetTypeMisc

/*
 * SetTypePacked() - Add the PAKED attribute to a type specifier.  Issue an error if it's already set.
 *
 * Returns: TRUE if O.K.
 *
 */

int SetTypePacked(SourceLoc *loc, dtype *fType)
{
    if (fType) {
		if (fType->type.properties & TYPE_MISC_PACKED_KW) {
			SemanticError(loc, ERROR___REPEATED_TYPE_ATTRIB);
			return 0;
        }
        fType->type.properties |= TYPE_MISC_PACKED | TYPE_MISC_PACKED_KW;
        return 1;
    }
    return 0;
} // SetTypePacked

/*
 * SetStorageClass() - Set the storage class of a type.  Issue an error if it's already set to
 *         a conflicting value.
 *
 * Returns: TRUE if O.K.
 *
 */

int SetStorageClass(SourceLoc *loc, dtype *fType, int storage)
{
    fType->type.properties;
    if (fType->storageClass == SC_UNKNOWN) {
        fType->storageClass = (StorageClass) storage;
    } else {
        if (fType->storageClass == (StorageClass) storage) {
            SemanticError(loc, ERROR___STORAGE_SPECIFIED_TWICE);
            return 0;
        } else {
            SemanticError(loc, ERROR___CONFLICTING_STORAGE);
            return 0;
        }
    }
    return 1;
} // SetStorageClass

/*
 * ResolveScalarTypeSpecifier() - Map a scalar type-specifier token to its
 *         canonical standard type.  "isUnsigned" upgrades integral kinds to
 *         their unsigned counterparts; token combinations that have no
 *         unsigned form report a language diagnostic and yield
 *         UndefinedType.
 */

Type *ResolveScalarTypeSpecifier(SourceLoc *loc, int token, int isUnsigned)
{
    CgScalarKind kind;
    Type *fType;

    // A lone "unsigned" means unsigned int:

    if (token == UNSIGNED_SY && !isUnsigned)
        return GetStandardTypeKind(CG_SCALAR_UINT, 0, 0);

    switch (token) {
    case CHAR_SY:    kind = CG_SCALAR_CHAR;    break;
    case SHORT_SY:   kind = CG_SCALAR_SHORT;   break;
    case INT_SY:     kind = CG_SCALAR_INT;     break;
    case LONG_SY:    kind = CG_SCALAR_LONG;    break;
    case HALF_SY:    kind = CG_SCALAR_HALF;    break;
    case FIXED_SY:   kind = CG_SCALAR_FIXED;   break;
    case FLOAT_SY:   kind = CG_SCALAR_FLOAT;   break;
    case DOUBLE_SY:  kind = CG_SCALAR_DOUBLE;  break;
    default:
        kind = CG_SCALAR_UNDEFINED;
        break;
    }

    if (isUnsigned) {
        switch (kind) {
        case CG_SCALAR_CHAR:  kind = CG_SCALAR_UCHAR;  break;
        case CG_SCALAR_SHORT: kind = CG_SCALAR_USHORT; break;
        case CG_SCALAR_INT:   kind = CG_SCALAR_UINT;   break;
        case CG_SCALAR_LONG:  kind = CG_SCALAR_ULONG;  break;
        default:
            SemanticError(loc, ERROR_S_TYPE_NAME_EXPECTED,
                          GetAtomString(atable, token));
            return UndefinedType;
        }
    }

    fType = GetStandardTypeKind(kind, 0, 0);
    if (!fType || kind == CG_SCALAR_UNDEFINED) {
        SemanticError(loc, ERROR_S_TYPE_NAME_EXPECTED,
                      GetAtomString(atable, token));
        return UndefinedType;
    }
    return fType;
} // ResolveScalarTypeSpecifier

/********************************** Parser Semantic Rules: ***********************************/

/*
 * Initializer() - Create an EXPR_LIST_OP node with an expression argument.
 *
 */
 
expr *Initializer(SourceLoc *loc, expr *fExpr)
{
    expr *lExpr;

    lExpr = (expr *) NewBinopNode(EXPR_LIST_OP, fExpr, NULL);
    return lExpr;
} // Initilaizer

/*
 * InitializerList() - Add an expression to a list of expressions.  Either can be NULL.
 *
 * Assumes that the nodes on list are EXPR_LIST_OP binary nodes.
 *
 */
 
expr *InitializerList(SourceLoc *loc, expr *list, expr *last)
{
    expr *lExpr;

    if (list) {
        if (last) {
            lExpr = list;
            while (lExpr->bin.right)
                lExpr = lExpr->bin.right;
            lExpr->bin.right = last;
        }
        return list;
    } else {
        return last;
    }
} // InitializerList

/*
 * ArgumentList() - Add an actual argument to a list of parameters.
 *
 */

expr *ArgumentList(SourceLoc *loc, expr *flist, expr *fExpr)
{
    expr *lExpr, *nExpr;

    nExpr = (expr *) NewBinopNode(FUN_ARG_OP, fExpr, NULL);
    nExpr->common.type = fExpr->common.type;
    nExpr->common.IsLValue = IsLValue(fExpr);
    if (GetQualifiers(nExpr->common.type) & TYPE_QUALIFIER_CONST) {
        nExpr->common.IsConst = 1;
    } else {
        nExpr->common.IsConst = 0;
    }
    if (flist) {
        lExpr = flist;
        while (lExpr->bin.right)
            lExpr = lExpr->bin.right;
        lExpr->bin.right = nExpr;
        return flist;
    } else {
        return nExpr;
    }
} // ArgumentList

/*
 * ExpressionList() - Add an expression to the end of a list of expressions..
 *
 */

expr *ExpressionList(SourceLoc *loc, expr *fList, expr *fExpr)
{
    expr *lExpr, *nExpr;

    nExpr = (expr *) NewBinopNode(EXPR_LIST_OP, fExpr, NULL);
    nExpr->common.type = fExpr->common.type;
    if (fList) {
        lExpr = fList;
        while (lExpr->bin.right)
            lExpr = lExpr->bin.right;
        lExpr->bin.right = nExpr;
        return fList;
    } else {
        return nExpr;
    }
} // ExpressionList

/*
 * NewGeometryArgument() - Wrap one typed actual argument with its
 *          optional inline binding-semantic atom (0 when unannotated)
 *          and the argument's source location.  The wrapper mirrors
 *          the binary layout so generic walkers pass through it; only
 *          call resolution and the geometry special-call path look
 *          inside.
 */

expr *NewGeometryArgument(SourceLoc *loc, expr *value, int semantic)
{
    struct geometry_arg_rec *parg;

    assert(NodeKind[GEOMETRY_ARGUMENT_OP] == BINARY_N);
    parg = (struct geometry_arg_rec *)
        malloc(sizeof(struct geometry_arg_rec));
    parg->kind = BINARY_N;
    parg->type = value ? value->common.type : UndefinedType;
    parg->IsLValue = 0;
    parg->IsConst = 0;
    parg->HasSideEffects = 0;
    if (value) {
        parg->IsLValue = IsLValue(value);
        parg->IsConst = value->common.IsConst;
        parg->HasSideEffects = value->common.HasSideEffects;
    }
    parg->op = GEOMETRY_ARGUMENT_OP;
    parg->subop = 0;
    parg->left = value;
    parg->right = NULL;
    parg->unused = NULL;
    parg->semantic = semantic;
    if (loc) {
        parg->loc = *loc;
    } else {
        parg->loc.file = 0;
        parg->loc.line = 0;
    }
    parg->tempptr[0] = 0;
    parg->tempptr[1] = 0;
    return (expr *) parg;
} // NewGeometryArgument

int IsGeometryArgument(const expr *value)
{
    return value != NULL &&
           value->common.kind == BINARY_N &&
           value->bin.op == GEOMETRY_ARGUMENT_OP;
} // IsGeometryArgument

expr *GetGeometryArgumentValue(expr *value)
{
    assert(IsGeometryArgument(value));
    return value->bin.left;
} // GetGeometryArgumentValue

int GetGeometryArgumentSemantic(const expr *value)
{
    assert(IsGeometryArgument(value));
    return ((const struct geometry_arg_rec *) value)->semantic;
} // GetGeometryArgumentSemantic

/*
 * AddDecl() - Add a declaration to a list of declarations.  Either can be NULL.
 *
 */
 
decl *AddDecl(decl *first, decl *last)
{
    decl *lDecl;

    if (first) {
        if (last) {
            lDecl = first;
            while (lDecl->next)
                lDecl = lDecl->next;
            lDecl->next = last;
        }
        return first;
    } else {
        return last;
    }
} // AddDecl

/*
 * AddStmt() - Add a list of statements to then end of another list.  Either can be NULL.
 *
 */
 
stmt *AddStmt(stmt *first, stmt *last)
{
    stmt *lStmt;

    if (first) {
        if (last) {
            lStmt = first;
            while (lStmt->exprst.next)
                lStmt = lStmt->exprst.next;
            lStmt->exprst.next = last;
        }
        return first;
    } else {
        return last;
    }
} // AddStmt

/*
 * CheckStatement() - See if this statement is supported by the target profile.
 *
 */

stmt *CheckStmt(stmt *fStmt)
{
    // Can't do it here.  Must wait until we know which functions are being used.
    //if (fStmt)
    //    theHAL->CheckStatement(&fStmt->commonst.loc, fStmt);
    return fStmt;
} // CheckStmt

/*
 * Struct scopes are suspended while a method body parses so that method
 * definitions sit at ordinary function depth: their locals scopes,
 * return-statement bookkeeping, and inlining metadata then behave exactly
 * like free functions.  SuspendStructScopeForMethodBody() runs from
 * Function_Definition_Header() when the header appears inside a struct;
 * ResumeStructScopeAfterMethodBody() runs from the function-definition
 * grammar action after the body scope is popped.  The pair is a no-op for
 * definitions outside struct bodies.
 */

typedef struct SuspendedScopeRec {
    Scope *scope;
    struct SuspendedScopeRec *next;
} SuspendedScope;

static SuspendedScope *lSuspendedStructScopes;

void SuspendStructScopeForMethodBody(void)
{
    SuspendedScope *lSuspend;

    if (CurrentScope && CurrentScope->IsStructScope) {
        lSuspend = (SuspendedScope *) malloc(sizeof(SuspendedScope));
        lSuspend->scope = PopScope();
        lSuspend->next = lSuspendedStructScopes;
        lSuspendedStructScopes = lSuspend;
    }
} // SuspendStructScopeForMethodBody

void ResumeStructScopeAfterMethodBody(void)
{
    SuspendedScope *lSuspend;

    if (lSuspendedStructScopes) {
        lSuspend = lSuspendedStructScopes;
        lSuspendedStructScopes = lSuspend->next;
        PushScope(lSuspend->scope);
        free(lSuspend);
    }
} // ResumeStructScopeAfterMethodBody

/*
 * Function_Definition_Header() - Combine function <declaration_specifiers> and <declarator>.
 *
 */

decl *Function_Definition_Header(SourceLoc *loc, decl *fDecl)
{
    Symbol *lSymb = fDecl->symb;
    Symbol *formals;
    int ccount;
    int InProgram;
    int category, domain, qualifiers;
    Type *retType;

    SuspendStructScopeForMethodBody();

    if (IsFunction(lSymb)) {
        if (fDecl->type.type.properties & TYPE_MISC_ABSTRACT_PARAMS) {
            SemanticError(loc, ERROR_S_ABSTRACT_NOT_ALLOWED,
                          GetAtomString(atable, fDecl->name));
        }
        if (lSymb->details.fun.statements) {
            SemanticError(loc, ERROR_S_FUN_ALREADY_DEFINED,
                          GetAtomString(atable, fDecl->name));
        }
        if (Cg->theHAL->entryName == fDecl->name) {
            InProgram = 1;
            fDecl->type.type.properties |= TYPE_MISC_PROGRAM;
            lSymb->details.fun.locals->pid = Cg->theHAL->pid;
        } else {
            InProgram = 0;
        }
        retType = lSymb->type->fun.rettype;
        ccount = 0;
        formals = lSymb->details.fun.params;
        while (formals) {
            category = GetCategory(formals->type);
            domain = GetDomain(formals->type);
            qualifiers = GetQualifiers(formals->type);
            if (qualifiers & TYPE_QUALIFIER_OUT)
                lSymb->details.fun.HasOutParams = 1;
            formals = formals->next;
        }
#if 000 // Can't warn anymore -- could be writing to a global variable
        if (!lSymb->details.fun.HasOutParams && IsVoid(retType)) {
            SemanticWarning(loc, WARNING_S_VOID_FUN_HAS_NO_OUT_ARGS,
                          GetAtomString(atable, fDecl->name));
        }
#endif
        PushScope(lSymb->details.fun.locals); 
    } else {
        SemanticError(loc, ERROR_S_NOT_A_FUN,
                      GetAtomString(atable, fDecl->name));
        PushScope(NewScope());
    }
    CurrentScope->funindex = ++NextFunctionIndex;
    return fDecl;
} // Function_Definition_Header

/*
 * lCheckInitializationData() - Check data in an init_declarator for compatibility
 *         with variable.
 */

static int lCheckInitializationData(SourceLoc *loc, Type *vType, expr *dExpr, int IsGlobal)
{
    int category, base, ii, vlen, subop;
    expr *lExpr, *tExpr;

    if (!dExpr || !vType)
        return 0;
    base = GetBase(vType);
    category = GetCategory(vType);
    switch (category) {
    default:
    case TYPE_CATEGORY_NONE:
        return 0;
    case TYPE_CATEGORY_SCALAR:
    case TYPE_CATEGORY_SAMPLER:
        if (dExpr->common.kind == BINARY_N && dExpr->bin.op == EXPR_LIST_OP) {
            if (!dExpr->bin.left) {
                /* "{}" parses as an empty initializer list. */
                SemanticError(loc, ERROR___TOO_LITTLE_DATA);
                return 0;
            }
            lExpr = FoldConstants(dExpr->bin.left);
            if (lExpr->common.kind == CONST_N) {
                if (ConvertType(loc, lExpr, vType, lExpr->co.type, &tExpr, 1, 0, 1)) {
                    dExpr->bin.left = tExpr;
                    return 1;
                } else {
                    if (!CgTypeIsPoison(vType) &&
                        !CgTypeIsPoison(lExpr->common.type))
                        SemanticError(loc, ERROR___INVALID_INITIALIZATION);
                    return 0;
                }
            } else {
#if 000 // RSG
                if (IsGlobal) {
                    SemanticError(loc, ERROR___NON_CONST_INITIALIZATION);
                    return 0;
                } else {
#endif // RSG
                    return 1;
#if 000 // RSG
                }
#endif // RSG
            }
        } else {
            SemanticError(loc, ERROR___INVALID_INITIALIZATION);
            return 0;
        }
    case TYPE_CATEGORY_ARRAY:
        vlen = vType->arr.numels;
        if (dExpr->common.kind == BINARY_N && dExpr->bin.op == EXPR_LIST_OP) {
            lExpr = dExpr->bin.left;
            if (!lExpr) {
                SemanticError(loc, ERROR___TOO_LITTLE_DATA);
                return 0;
            }
            if (lExpr->common.kind == BINARY_N && lExpr->bin.op == EXPR_LIST_OP) {
                for (ii = 0; ii < vlen; ii++) {
                    if (lExpr) {
                        if (lExpr->common.kind == BINARY_N && lExpr->bin.op == EXPR_LIST_OP) {
                            if (lCheckInitializationData(loc, vType->arr.eltype, lExpr, IsGlobal)) {
                                /* O.K. */
                            } else {
                                return 0;
                            }
                        } else {
                            SemanticError(loc, ERROR___INVALID_INITIALIZATION);
                            return 0;
                        }
                    } else {
                        SemanticError(loc, ERROR___TOO_LITTLE_DATA);
                        return 0;
                    }
                    lExpr = lExpr->bin.right;
                }
                if (lExpr) {
                    SemanticError(loc, ERROR___TOO_MUCH_DATA);
                } else {
                    subop = SUBOP_V(vlen, GetBase(vType));
                    dExpr->bin.left = (expr *) NewUnopSubNode(VECTOR_V_OP, subop, dExpr->bin.left);
                    if (!IsVector(vType, NULL) && !IsMatrix(vType, NULL, NULL)) {
                        /* First-class arrays copy as their declared shape;
                         * repacking them would change packedness and break
                         * the whole-array rvalue copy. */
                        dExpr->bin.left->un.type = vType;
                    } else {
                        dExpr->bin.left->un.type =
                            GetStandardType(GetBase(vType), vlen, 0);
                    }
                    return 1;
                }
            } else {
                if (ConvertType(loc, lExpr, vType, lExpr->common.type, &tExpr, 0, 0, 1)) {
                    dExpr->bin.left = tExpr;
                    return 1;
                } else {
                    if (!CgTypeIsPoison(vType) &&
                        !CgTypeIsPoison(lExpr->common.type))
                        SemanticError(loc, ERROR___INCOMPAT_TYPE_INIT);
                    return 0;
                }
            }
        } else {
            SemanticError(loc, ERROR___INVALID_INITIALIZATION);
        }
        return 0;
    case TYPE_CATEGORY_FUNCTION:
    case TYPE_CATEGORY_STRUCT:
    case TYPE_CATEGORY_CONNECTOR:
        SemanticError(loc, ERROR___INVALID_INITIALIZATION);
        return 0;
    }
} // lCheckInitializationData

/*
 * Param_Init_Declarator() - Process a parameter initialization declarator.
 *
 */

decl *Param_Init_Declarator(SourceLoc *loc, Scope *fScope, decl *fDecl, expr *fExpr)
{
    Type *lType;

    if (fDecl) {
        lType = &fDecl->type.type;
        if (IsVoid(lType)) {
            SemanticError(loc, ERROR_S_VOID_TYPE_INVALID,
                          GetAtomString(atable, fDecl->name));
        }
        if (GetCategory(lType) == TYPE_CATEGORY_FUNCTION) {
            SemanticError(loc, ERROR_S_FUN_TYPE_INVALID,
                          GetAtomString(atable, fDecl->name));
        }
        if (fExpr) {
            /* Domain/qualifier/constant rules for parameter defaults
             * are enforced with full function context by
             * lValidateParameterDefaults() once the formal list is
             * complete; here the initializer is only shaped and
             * recorded on the declarator. */
            /* An empty-bracket array parameter sizes its top-level
             * dimension from the initializer list exactly like a local
             * declaration; the count lives in the declaration-local
             * dtype copy and survives GetTypePointer(). */
            if (IsUnsizedArray(lType)) {
                int numels = lCountInitializerElements(fExpr);
                if (numels <= 0) {
                    SemanticError(loc, ERROR_S_CANNOT_INFER_ARRAY_SIZE);
                    return fDecl;
                }
                lType->arr.numels = numels;
            }
            if (lCheckInitializationData(loc, lType, fExpr, 0)) {
                fDecl->initexpr = fExpr;
            }
        }
    }
    return fDecl;
} // Param_Init_Declarator

/*
 * lCountInitializerElements() - Count the top-level elements of an
 *         initializer list.  The grammar wraps every initializer in an
 *         EXPR_LIST_OP node and chains the members of a brace list off the
 *         wrapper's left node, so counting starts one level down.  An
 *         empty or malformed list counts as zero so callers can report
 *         that the size cannot be inferred.
 *
 */

static int lCountInitializerElements(expr *fExpr)
{
    int numels = 0;

    if (!fExpr || fExpr->common.kind != BINARY_N ||
        fExpr->bin.op != EXPR_LIST_OP)
    {
        return 0;
    }
    fExpr = fExpr->bin.left;
    while (fExpr &&
           fExpr->common.kind == BINARY_N &&
           fExpr->bin.op == EXPR_LIST_OP)
    {
        numels++;
        fExpr = fExpr->bin.right;
    }
    return numels;
} // lCountInitializerElements

/*
 * lSizeUnsizedArrayFromInitializer() - Replace the declared unsized array
 *         type of "lSymb" with a concrete array whose top-level length is
 *         the number of top-level elements in the initializer list.  The
 *         declaration context owns this Type object (unsized types are not
 *         interned), so replacing it cannot alias any other declaration.
 *
 * Returns: TRUE if the type was sized, FALSE after emitting the language
 *          diagnostic for lists from which no size can be inferred.
 *
 */

static int lSizeUnsizedArrayFromInitializer(SourceLoc *loc, Symbol *lSymb,
                                            expr *fExpr)
{
    Type *nType;
    int numels;

    numels = lCountInitializerElements(fExpr);
    if (numels <= 0) {
        SemanticError(loc, ERROR_S_CANNOT_INFER_ARRAY_SIZE);
        return 0;
    }
    nType = DupType(lSymb->type);
    nType->arr.numels = numels;
    nType->arr.size = Cg->theHAL->GetSizeof(nType);
    lSymb->type = nType;
    return 1;
} // lSizeUnsizedArrayFromInitializer

/*
 * Init_Declarator() - Set initial value and/or semantics for this declarator.
 *
 */

stmt *Init_Declarator(SourceLoc *loc, Scope *fScope, decl *fDecl, expr *fExpr)
{
    int category, base;
    stmt *lStmt = NULL;
    int IsGlobal, IsStatic, IsUniform, IsParam, DontAssign;
    Symbol *lSymb;
    expr *lExpr;
    Type *lType;

    if (fDecl) {
        lSymb = fDecl->symb;
        if (fExpr) {
            if (lSymb->kind != VARIABLE_S) {
                SemanticError(loc, ERROR_S_INIT_NON_VARIABLE,
                            GetAtomString(atable, lSymb->name));
            } else if (fScope->IsStructScope) {
                SemanticError(loc, ERROR_S_INIT_STRUCT_MEMBER,
                            GetAtomString(atable, lSymb->name));
            } else if (lSymb->storageClass == SC_EXTERN) {
                SemanticError(loc, ERROR_S_INIT_EXTERN,
                            GetAtomString(atable, lSymb->name));
            } else {
                lType = lSymb->type;
                IsGlobal = fScope->level <= 1;
                IsStatic = lSymb->storageClass == SC_STATIC;
                IsUniform = GetDomain(lType) == TYPE_DOMAIN_UNIFORM;
                IsParam = lSymb->properties & SYMB_IS_PARAMETER;
                if (IsUnsizedArray(lType)) {
                    /* An empty-bracket declarator with an initializer list
                     * takes its top-level length from that list; without
                     * one it stays dynamically sized and is only usable
                     * through whole-array assignment. */
                    if (fExpr) {
                        if (!lSizeUnsizedArrayFromInitializer(loc, lSymb, fExpr))
                            return lStmt;
                        lType = lSymb->type;
                    }
                }
                if (IsGlobal && !IsStatic) {
                    DontAssign = 1;
                } else if (IsParam) {
                    DontAssign = 1;
                } else {
                    DontAssign = 0;
                }
                if (lCheckInitializationData(loc, lType, fExpr, IsGlobal)) {
                    category = GetCategory(lType);
                    base = GetBase(lType);
                    switch (category) {
                    default:
                    case TYPE_CATEGORY_NONE:
                        SemanticError(loc, ERROR___INVALID_INITIALIZATION);
                        break;
                    case TYPE_CATEGORY_SCALAR:
                    case TYPE_CATEGORY_SAMPLER:
                        assert(fExpr->common.kind == BINARY_N && fExpr->bin.op == EXPR_LIST_OP);
                        if (DontAssign) {
                            lSymb->details.var.init = fExpr;
                        } else {
                            lExpr = (expr *) NewSymbNode(VARIABLE_OP, lSymb);
                            lStmt = NewSimpleAssignmentStmt(loc, lExpr, fExpr->bin.left, 1);
                        }
                        break;
                    case TYPE_CATEGORY_ARRAY:
                        /* Vectors, matrices, and first-class arrays all
                         * take whole-aggregate initializers; the rvalue
                         * copy is a full-array assignment. */
                        assert(fExpr->common.kind == BINARY_N && fExpr->bin.op == EXPR_LIST_OP);
                        if (DontAssign) {
                            lSymb->details.var.init = fExpr;
                        } else {
                            lExpr = (expr *) NewSymbNode(VARIABLE_OP, lSymb);
                            lStmt = NewSimpleAssignmentStmt(loc, lExpr, fExpr->bin.left, 1);
                        }
                        break;
                    }
                }
            }
        }
        if (lSymb->kind == FUNCTION_S) {
            lSymb = lSymb->details.fun.params;
            while (lSymb) {
                if (lSymb->kind == VARIABLE_S) {
                    if (lSymb->details.var.semantics)
                        SemanticWarning(loc, WARNING_S_FORWARD_SEMANTICS_IGNORED,
                                        GetAtomString(atable, lSymb->name));
                }
                lSymb = lSymb->next;
            }
        }
    }
    return lStmt;
} // Init_Declarator

/*
 * lCheckSamplerDeclaration() - Enforce the Cg 2.0 sampler placement rules
 * where a variable declarator introduces a name: samplers live only as
 * formal parameters (handled by the caller) and as global uniform
 * variables.  Function locals, static globals, and structure members are
 * rejected, as are arrays and other aggregates with sampler elements.
 * Global declarations must carry uniform domain; varying or unqualified
 * globals would otherwise be silently bound as uniforms.  Since the
 * GLSL-only sampler pid-gate was retired these rules apply uniformly to
 * every profile, GLSL profiles included.
 */

static void lCheckSamplerDeclaration(SourceLoc *loc, Scope *fScope,
                                     int name, Type *fType, int IsStatic)
{
    Type *element;

    if (!IsSampler(fType, NULL)) {
        element = fType;
        while (element && IsArray(element))
            element = element->arr.eltype;
        if (!element || !IsSampler(element, NULL))
            return;
    }
    if (fScope->IsStructScope || fScope->level > 1 || IsStatic)
    {
        SemanticError(loc, ERROR_S_SAMPLER_DECLARATION,
                      GetAtomString(atable, name));
    }
    else if (GetDomain(fType) != TYPE_DOMAIN_UNIFORM)
    {
        /* The remaining case is a file-scope declaration; it is only a
         * legal sampler home when declared "uniform". */
        SemanticError(loc, ERROR_S_SAMPLER_DECLARATION,
                      GetAtomString(atable, name));
    }
} // lCheckSamplerDeclaration

/*
 * lAttribArrayUseNoun() - The placement phrase one rejected use names
 *         in its diagnostic.
 */

static const char *lAttribArrayUseNoun(CgGeometryDeclarationUse use)
{
    switch (use) {
    case CG_GEOMETRY_DECL_GLOBAL:
        return "a global variable";
    case CG_GEOMETRY_DECL_UNIFORM:
        return "a uniform variable";
    case CG_GEOMETRY_DECL_OUTPUT:
        return "an output parameter";
    case CG_GEOMETRY_DECL_RETURN:
        return "a function return value";
    case CG_GEOMETRY_DECL_MEMBER:
        return "a struct member";
    case CG_GEOMETRY_DECL_LOCAL:
        return "a local variable";
    case CG_GEOMETRY_DECL_ENTRY_INPUT:
    case CG_GEOMETRY_DECL_HELPER_INPUT:
    default:
        return "this declaration";
    }
} // lAttribArrayUseNoun

/*
 * lReportAttribArrayFailure() - Map one structured attribute-array
 *         validation reason onto its stable reserved diagnostic:
 *         illegal elements name the element type, unresolved helper
 *         inputs report the stage rule, and every other failure is a
 *         prohibited placement naming its use.
 */

static void lReportAttribArrayFailure(SourceLoc *loc, const Symbol *symbol,
                                      CgGeometryDeclarationUse use,
                                      const CgGeometryDiagnostic *diagnostic)
{
    char tname[128], uname[128], ename[256];

    if (diagnostic->reason == CG_GEOMETRY_DIAGNOSTIC_ATTRIB_ELEMENT) {
        FormatTypeString(tname, sizeof tname, uname, sizeof uname,
                         CgAttribArrayElement(symbol->type));
        strncpy(ename, tname, sizeof ename - 1);
        ename[sizeof ename - 1] = '\0';
        strncat(ename, uname, sizeof ename - strlen(ename) - 1);
        SemanticError(loc, ERROR_S_GEOMETRY_ATTRIB_ELEMENT, ename);
    } else if (diagnostic->reason ==
               CG_GEOMETRY_DIAGNOSTIC_ATTRIB_STAGE) {
        SemanticError(loc, ERROR___GEOMETRY_ATTRIB_STAGE);
    } else {
        SemanticError(loc, ERROR_S_GEOMETRY_ATTRIB_PLACEMENT,
                      lAttribArrayUseNoun(use));
    }
} // lReportAttribArrayFailure

/*
 * lValidateAttribArraySymbol() - Validate one declaration through the
 *         shared geometry rule and report exactly one reserved
 *         diagnostic on failure.  Callers pass program NULL while
 *         only source-level facts exist.
 */

static void lValidateAttribArraySymbol(SourceLoc *loc, const Symbol *symbol,
                                       CgGeometryDeclarationUse use)
{
    CgGeometryDiagnostic diagnostic;

    if (CgGeometryValidateAttribArrayDeclaration(NULL, symbol, use,
                                                 &diagnostic)) {
        return;
    }
    lReportAttribArrayFailure(loc, symbol, use, &diagnostic);
} // lValidateAttribArraySymbol

/*
 * lCheckAttribArrayVariable() - Placement rules for one variable
 *         declarator: struct members, uniforms, globals, and locals
 *         are all prohibited homes for an AttribArray.  Runs beside
 *         the sampler declaration rules before the symbol is defined.
 */

static void lCheckAttribArrayVariable(SourceLoc *loc, Scope *fScope,
                                      int name, Type *fType)
{
    Symbol stack;

    if (!CgIsAttribArray(fType)) {
        return;
    }
    memset(&stack, 0, sizeof(stack));
    stack.kind = VARIABLE_S;
    stack.name = name;
    stack.loc = *loc;
    stack.type = fType;
    if (fScope->IsStructScope) {
        lValidateAttribArraySymbol(loc, &stack, CG_GEOMETRY_DECL_MEMBER);
    } else if (GetDomain(fType) == TYPE_DOMAIN_UNIFORM) {
        lValidateAttribArraySymbol(loc, &stack, CG_GEOMETRY_DECL_UNIFORM);
    } else if (fScope->level <= 1) {
        lValidateAttribArraySymbol(loc, &stack, CG_GEOMETRY_DECL_GLOBAL);
    } else {
        lValidateAttribArraySymbol(loc, &stack, CG_GEOMETRY_DECL_LOCAL);
    }
} // lCheckAttribArrayVariable

/*
 * lCheckAttribArrayFunction() - Attribute-array rules for one function
 *         declarator: the return type is always a prohibited home, and
 *         formals qualify only as entry inputs when their own
 *         declaration carries topology modifiers -- any other function
 *         is a helper whose reachability is proven only by
 *         selected-program analysis, so its array formals fail the
 *         stage rule for now.  Out and uniform formals are prohibited
 *         regardless of the enclosing function.
 */

static void lCheckAttribArrayFunction(SourceLoc *loc, decl *fDecl,
                                      Symbol *fSymb)
{
    CgGeometryDeclarationUse use;
    CgGeometryModifiers *geometry;
    Symbol stack;
    Symbol *formal;
    int isGeometryEntry;

    if (CgIsAttribArray(fSymb->type->fun.rettype)) {
        memset(&stack, 0, sizeof(stack));
        stack.kind = VARIABLE_S;
        stack.name = fDecl->name;
        stack.loc = *loc;
        stack.type = fSymb->type->fun.rettype;
        lValidateAttribArraySymbol(loc, &stack, CG_GEOMETRY_DECL_RETURN);
    }
    geometry = &fDecl->type.geometry;
    isGeometryEntry = geometry->input != CG_GEOMETRY_INPUT_UNKNOWN ||
                      geometry->output != CG_GEOMETRY_OUTPUT_UNKNOWN;
    for (formal = fSymb->details.fun.params; formal;
         formal = formal->next)
    {
        if (!CgIsAttribArray(formal->type)) {
            continue;
        }
        if (GetQualifiers(formal->type) & TYPE_QUALIFIER_OUT) {
            use = CG_GEOMETRY_DECL_OUTPUT;
        } else if (GetDomain(formal->type) == TYPE_DOMAIN_UNIFORM) {
            use = CG_GEOMETRY_DECL_UNIFORM;
        } else if (isGeometryEntry) {
            use = CG_GEOMETRY_DECL_ENTRY_INPUT;
        } else {
            use = CG_GEOMETRY_DECL_HELPER_INPUT;
        }
        lValidateAttribArraySymbol(&formal->loc, formal, use);
    }
} // lCheckAttribArrayFunction

///////////////////////////////////////////////////////////////////////////////////////////////
//////////////////// Profile specifiers and parameter defaults: ///////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////

/*
 * A <profile_specifier> is parsed before a function's return type.  The
 * grammar action records it here and the next top-level function
 * declarator consumes it; a specifier followed by anything other than a
 * function declaration is rejected where the declarator is processed.
 */

static CgProfileSelector lPendingProfileSelector;
static int lHavePendingProfileSpecifier;

/*
 * SetPendingProfileSpecifier() - Validate an identifier appearing in
 *         profile-specifier position.  It must name either a registered
 *         profile (EnumerateProfiles) or a wildcard atom some profile
 *         registered through SetProfileIdentity.  Typedef shadowing
 *         needs no handling here: shadowed names reach the parser as
 *         TYPEIDENT_SY and never enter this rule.
 */

void SetPendingProfileSpecifier(SourceLoc *loc, int ident)
{
    slProfile *lProfile;
    int ii;

    lHavePendingProfileSpecifier = 0;
    for (ii = 0; (lProfile = EnumerateProfiles(ii)) != NULL; ii++) {
        if (!strcmp(lProfile->name, GetAtomString(atable, ident))) {
            /* Exact profile name: highest selector specificity. */
            lPendingProfileSelector.name = ident;
            lPendingProfileSelector.specificity =
                CG_PROFILE_EXACT_SPECIFICITY;
            lPendingProfileSelector.isOpen = 0;
            lHavePendingProfileSpecifier = 1;
            return;
        }
    }
    for (ii = 0; (lProfile = EnumerateProfiles(ii)) != NULL; ii++) {
        const CgProfileIdentity *identity = &lProfile->profileIdentity;
        int jj;

        for (jj = 0; jj < identity->wildcardCount; jj++) {
            if (identity->wildcards[jj] == ident) {
                lPendingProfileSelector.name = ident;
                lPendingProfileSelector.specificity =
                    identity->specificity ? identity->specificity[jj] : 0;
                lPendingProfileSelector.isOpen = 0;
                lHavePendingProfileSpecifier = 1;
                return;
            }
        }
    }
    SemanticError(loc, ERROR_S_UNKNOWN_PROFILE,
                  GetAtomString(atable, ident));
} // SetPendingProfileSpecifier

/*
 * ClearPendingProfileSpecifier() - Drop any unconsumed specifier so it
 *         cannot leak into a later declaration.
 */

void ClearPendingProfileSpecifier(void)
{
    lHavePendingProfileSpecifier = 0;
} // ClearPendingProfileSpecifier

/*
 * lAttachPendingProfileSpecifier() - Give a freshly declared function
 *         symbol whatever specifier is pending.
 */

static void lAttachPendingProfileSpecifier(Symbol *fSymb)
{
    if (lHavePendingProfileSpecifier && fSymb && IsFunction(fSymb)) {
        fSymb->details.fun.profileSelector = lPendingProfileSelector;
        lHavePendingProfileSpecifier = 0;
    }
} // lAttachPendingProfileSpecifier

/*
 * lRejectPendingProfileSpecifier() - A specifier was followed by a
 *         non-function declaration.
 */

static void lRejectPendingProfileSpecifier(SourceLoc *loc)
{
    if (lHavePendingProfileSpecifier) {
        SemanticError(loc, ERROR_S_PROFILE_SPECIFIER_MISPLACED,
                      GetAtomString(atable, lPendingProfileSelector.name));
        lHavePendingProfileSpecifier = 0;
    }
} // lRejectPendingProfileSpecifier

/*
 * lIsEntryFunctionName() - Top-level entry parameters become program
 *         interface values, so their defaults follow the uniform rules
 *         rather than the helper ones.
 */

static int lIsEntryFunctionName(int name)
{
    return name == Cg->theHAL->entryName;
} // lIsEntryFunctionName

/*
 * lIsConstantInit() - TRUE when every element of an initializer-list
 *         node folds to a constant.  Parameter initializers arrive
 *         wrapped by Initializer() in EXPR_LIST_OP nodes whose members
 *         chain through bin.right.
 */

static int lIsConstantInit(expr *fInit)
{
    expr *element;

    if (!fInit || fInit->common.kind != BINARY_N ||
        fInit->bin.op != EXPR_LIST_OP)
    {
        return 0;
    }
    for (element = fInit; element; element = element->bin.right) {
        expr *value;

        if (element->common.kind != BINARY_N ||
            element->bin.op != EXPR_LIST_OP)
        {
            break;
        }
        value = element->bin.left;
        if (!value) {
            return 0;
        }
        if (value->common.kind == BINARY_N &&
            value->bin.op == EXPR_LIST_OP)
        {
            /* Nested brace lists were validated elementwise when the
             * declarator recorded them. */
            continue;
        }
        if (FoldConstants(value)->common.kind != CONST_N) {
            return 0;
        }
    }
    return 1;
} // lIsConstantInit

/*
 * lValidateParameterDefaults() - Enforce the Cg 2.0 default-argument
 *         rules over one function's ordered formals:
 *
 *           - helper calls fill unsupplied trailing arguments from
 *             defaults, so a default may only appear after parameters
 *             that always receive values;
 *           - a top-level (entry) default requires a uniform parameter,
 *             and entry uniforms are program-interface values rather
 *             than call arguments, so positionality does not apply;
 *           - a helper default requires a plain in parameter;
 *           - every default is a compile-time constant convertible to
 *             the parameter type, stored converted.
 *
 *         Invalid defaults are dropped from the formal so later passes
 *         see only well-formed calls.
 */

static void lValidateParameterDefaults(SourceLoc *loc, Symbol *funSymb)
{
    Symbol *formal;
    int IsEntry;

    if (!funSymb || !IsFunction(funSymb)) {
        return;
    }
    IsEntry = lIsEntryFunctionName(funSymb->name);
    for (formal = funSymb->details.fun.params; formal;
         formal = formal->next)
    {
        expr *init = formal->details.var.init;
        Type *formalType = formal->type;

        if (IsEntry) {
            /* Entry formals become program inputs: a default is just
             * the uniform's binding value and stands on its own. */
            if (init && GetDomain(formalType) != TYPE_DOMAIN_UNIFORM) {
                SemanticError(&formal->loc, ERROR_S_NON_UNIFORM_PARAM_INIT,
                              GetAtomString(atable, formal->name));
                formal->details.var.init = NULL;
            }
            continue;
        }
        if (!init) {
            continue;
        }
        /* Defaults are legal only as a trailing run: every parameter
         * after a defaulted one must carry a default too. */
        {
            Symbol *scan;

            for (scan = formal->next; scan; scan = scan->next) {
                if (!scan->details.var.init) {
                    SemanticError(&formal->loc,
                                  ERROR_S_DEFAULT_AFTER_OPTIONAL,
                                  GetAtomString(atable, formal->name));
                    formal->details.var.init = NULL;
                    break;
                }
            }
            if (!formal->details.var.init) {
                continue;
            }
        }
        {
            int quals = GetQualifiers(formalType);

            if (quals & TYPE_QUALIFIER_OUT) {
                SemanticError(&formal->loc, ERROR_S_DEFAULT_PARAM_QUALIFIER,
                              GetAtomString(atable, formal->name));
                formal->details.var.init = NULL;
                continue;
            }
            if (!lIsConstantInit(init)) {
                SemanticError(&formal->loc, ERROR___DEFAULT_NOT_CONSTANT,
                              GetAtomString(atable, formal->name));
                formal->details.var.init = NULL;
                continue;
            }
            /* Single-value defaults are re-expressed at the parameter's
             * own type inside the same EXPR_LIST wrapper; multi-element
             * brace lists were converted elementwise when recorded. */
            if (init->bin.left && !init->bin.right &&
                init->bin.left->common.type &&
                !IsSameUnqualifiedType(formalType,
                                       init->bin.left->common.type))
            {
                expr *converted = NULL;
                expr *value = FoldConstants(init->bin.left);

                /* Poison defaults keep their recorded shape: the value's
                 * type already reported its one diagnostic. */
                if (!CgTypeIsPoison(value->common.type)) {
                    if (!ConvertType(loc, value, formalType,
                                     value->common.type, &converted, 0, 0, 0))
                    {
                        SemanticError(&formal->loc,
                                      ERROR___DEFAULT_NOT_CONVERTIBLE,
                                      GetAtomString(atable, formal->name));
                        formal->details.var.init = NULL;
                        continue;
                    }
                    init->bin.left = converted;
                }
            }
        }
    }
} // lValidateParameterDefaults

/*
 * lPreserveParameterDefaults() - A redeclaration that matches an older
 *         one by signature cannot change an existing default: carry the
 *         original default forward when the redeclaration omits it and
 *         reject any attempt to re-specify one.
 */

static void lPreserveParameterDefaults(SourceLoc *loc, Symbol *fOldFun,
                                       Symbol *fNewParams, int atom)
{
    Symbol *oldFormal, *newFormal;

    for (oldFormal = fOldFun->details.fun.params, newFormal = fNewParams;
         oldFormal && newFormal;
         oldFormal = oldFormal->next, newFormal = newFormal->next)
    {
        if (oldFormal->details.var.init && newFormal->details.var.init) {
            SemanticError(loc, ERROR_S_DEFAULT_REDECLARATION,
                          GetAtomString(atable, atom));
            return;
        }
        if (oldFormal->details.var.init) {
            newFormal->details.var.init = oldFormal->details.var.init;
        }
    }
} // lPreserveParameterDefaults

/*
 * Declarator() - Process a declarator.
 *
 */

decl *Declarator(SourceLoc *loc, decl *fDecl, int semantics)
{
    Symbol *lSymb, *params;
    Scope *lScope;
    Type *lType;

    if (CurrentScope->InFormalParameters) {
        /*
         * Don't add formal parameters to the symbol table until we're
         * sure that we're in a function declaration.
         *
         */
        if (fDecl->type.storageClass != SC_UNKNOWN)
            SemanticError(&fDecl->loc, ERROR_S_STORAGE_NOT_ALLOWED,
                          GetAtomString(atable, fDecl->name));
        /* Topology modifiers never qualify an individual formal. */
        lRejectGeometryModifiersOnNonFunction(&fDecl->loc, &fDecl->type);
        fDecl->semantics = semantics;
    } else {
        lSymb = LookUpLocalSymbol(CurrentScope, fDecl->name);
        if (!lSymb) {
            lType = GetTypePointer(&fDecl->loc, &fDecl->type);
            if (IsVoid(lType)) {
                SemanticError(&fDecl->loc, ERROR_S_VOID_TYPE_INVALID,
                              GetAtomString(atable, fDecl->name));
            }
            if (GetCategory(&fDecl->type.type) != TYPE_CATEGORY_FUNCTION) {
                lRejectPendingProfileSpecifier(&fDecl->loc);
                lRejectGeometryModifiersOnNonFunction(&fDecl->loc, &fDecl->type);
            }
            if (fDecl->type.type.properties & TYPE_MISC_TYPEDEF) {
                lSymb = DefineTypedef(loc, CurrentScope, fDecl->name, lType);
                if (semantics)
                    SemanticError(loc, ERROR_S_SEMANTICS_NON_VARIABLE,
                                GetAtomString(atable, fDecl->name));
                if (fDecl->type.storageClass != SC_UNKNOWN)
                    SemanticError(&fDecl->loc, ERROR_S_STORAGE_NOT_ALLOWED_TYPEDEF,
                         GetAtomString(atable, fDecl->name));
            } else {
                if (GetQualifiers(&fDecl->type.type) & TYPE_QUALIFIER_INOUT) {
                    SemanticError(&fDecl->loc, ERROR_S_IN_OUT_PARAMS_ONLY,
                                  GetAtomString(atable, fDecl->name));
                }
                if (GetCategory(&fDecl->type.type) == TYPE_CATEGORY_FUNCTION) {
                    lScope = NewScope();
                    params = AddFormalParamDecls(lScope, fDecl->params);
                    lSymb = DeclareFunc(&fDecl->loc, CurrentScope, NULL, fDecl->name, lType, lScope, params);
                    lAttachPendingProfileSpecifier(lSymb);
                    lMergeGeometryModifiers(&lSymb->details.fun.geometry,
                                            &fDecl->type.geometry);
                    lCheckAttribArrayFunction(&fDecl->loc, fDecl, lSymb);
                    lValidateParameterDefaults(&fDecl->loc, lSymb);
                    // Programs may carry a return-value semantic; it is bound
                    // later by BuildSemanticStructs() for the selected entry.
                    if (semantics)
                        lSymb->details.fun.semantics = semantics;
                } else {
                    if (fDecl->type.type.properties & TYPE_MISC_INTERNAL) {
                        SemanticError(&fDecl->loc, ERROR_S_INTERNAL_FOR_FUN,
                                      GetAtomString(atable, fDecl->name));
                    }
                    if (fDecl->type.type.properties & TYPE_MISC_INLINE) {
                        SemanticError(&fDecl->loc, ERROR_S_INLINE_FOR_FUN,
                                      GetAtomString(atable, fDecl->name));
                    }
                    /* Unsized arrays are permitted where the language
                     * gives them a dynamic shape: formal parameters are
                     * handled above, and function locals can be assigned
                     * from a sized array.  Globals and struct members
                     * have no such context and stay invalid. */
                    if (IsUnsizedArray(lType) &&
                        (CurrentScope->level <= 1 || CurrentScope->IsStructScope))
                    {
                        SemanticError(&fDecl->loc, ERROR_S_UNSIZED_ARRAY,
                                      GetAtomString(atable, fDecl->name));
                    }
                    lCheckSamplerDeclaration(&fDecl->loc, CurrentScope,
                                             fDecl->name, lType,
                                             fDecl->type.storageClass == SC_STATIC);
                    lCheckAttribArrayVariable(&fDecl->loc, CurrentScope,
                                              fDecl->name, lType);
                    if (IsCategory(lType, TYPE_CATEGORY_ARRAY) && !IsPacked(lType)) {
                        if (!Cg->theHAL->GetCapsBit(CAPS_INDEXED_ARRAYS)) {
                            // XYZZY - This test needs to be moved to later to support multiple profiles
                            SemanticError(&fDecl->loc, ERROR_S_UNPACKED_ARRAY,
                                          GetAtomString(atable, fDecl->name));
                        }
                    }
                    lSymb = DefineVar(loc, CurrentScope, fDecl->name, lType);
                    lSymb->storageClass = fDecl->type.storageClass;
                    if (semantics) {
                        if (CurrentScope->IsStructScope) {
                            CurrentScope->HasSemantics = 1;
                        } else {
                            if (CurrentScope->level > 1) {
                                SemanticError(&fDecl->loc, ERROR_S_NO_LOCAL_SEMANTICS,
                                              GetAtomString(atable, fDecl->name));
                            } else if (fDecl->type.storageClass == SC_STATIC) {
                                SemanticError(&fDecl->loc, ERROR_S_STATIC_CANT_HAVE_SEMANTICS,
                                              GetAtomString(atable, fDecl->name));
#if 000 // RSG -- Not sure if this is true.  Do non-static global variables with semantics have to be declared "uniform"?
                            } else if (GetDomain(&fDecl->type.type) != TYPE_DOMAIN_UNIFORM) {
                                SemanticError(&fDecl->loc, ERROR_S_NON_STATIC_SEM_NOT_UNIFORM,
                                              GetAtomString(atable, fDecl->name));
#endif // RSG
                            }
                        }
                        lSymb->details.var.semantics = semantics;
                    }
                    if (CurrentScope->level == 1) {
                        if (fDecl->type.storageClass != SC_STATIC &&
                            GetDomain(&fDecl->type.type) != TYPE_DOMAIN_VARYING)
                        {
                            lSymb->properties |= SYMB_NEEDS_BINDING;
                        }
                    }
                    if (CurrentScope->IsStructScope)
                        AddParameter(CurrentScope, lSymb);
                }
            }
        } else {
            if (GetCategory(&fDecl->type.type) == TYPE_CATEGORY_FUNCTION) {
                lType = GetTypePointer(&fDecl->loc, &fDecl->type);
                lScope = NewScope();
                params = AddFormalParamDecls(lScope, fDecl->params);
                lSymb = DeclareFunc(&fDecl->loc, CurrentScope, lSymb, fDecl->name, lType, lScope, params);
                lAttachPendingProfileSpecifier(lSymb);
                /* A matching redeclaration merges its modifiers into
                 * the surviving symbol; fields the redeclaration omits
                 * keep the original values. */
                lMergeGeometryModifiers(&lSymb->details.fun.geometry,
                                        &fDecl->type.geometry);
                lCheckAttribArrayFunction(&fDecl->loc, fDecl, lSymb);
                lValidateParameterDefaults(&fDecl->loc, lSymb);
                lSymb->storageClass = fDecl->type.storageClass;
                // See the matching new-declaration path above.
                if (semantics)
                    lSymb->details.fun.semantics = semantics;
            } else {
                lRejectPendingProfileSpecifier(&fDecl->loc);
                lRejectGeometryModifiersOnNonFunction(&fDecl->loc, &fDecl->type);
                if (!IsTypeBase(&fDecl->type.type, TYPE_BASE_UNDEFINED_TYPE)) {
                    SemanticError(&fDecl->loc, ERROR_S_NAME_ALREADY_DEFINED,
                                  GetAtomString(atable, fDecl->name));
                }
            }
        }
        fDecl->symb = lSymb;
    }
    return fDecl;
} // Declarator

/*
 * lInsertDimension() - Insert a dimension below dims levels.
 *
 */

 static int lInsertDimension(SourceLoc *loc, dtype *fDtype, int dims, int fnumels, int Packed)
 {
    int lnumels, lproperties;
    Type *lType, *elType;

    if (dims == 0) {
        fDtype->IsDerived = 0;
        lnumels = fnumels;
    } else {
        lType = &fDtype->type;
        if (IsArray(lType)) {
            lnumels = lType->arr.numels;
            lproperties = fDtype->type.arr.properties & TYPE_MISC_MASK;
            //lsize = lType->arr.size;
            elType = lType->arr.eltype;
            fDtype->type = *elType;
            if (!lInsertDimension(loc, fDtype, dims - 1, fnumels, Packed))
                return 0;  // error encountered below
            fDtype->type.arr.properties |= lproperties;
        } else {
            return 0;
        }
    }
    lType = GetTypePointer(loc, fDtype);
    SetTypeCategory(loc, 0, fDtype, TYPE_CATEGORY_ARRAY, 1);
    fDtype->type.arr.eltype = lType;
    fDtype->type.arr.numels = lnumels;
    fDtype->numNewDims = dims + 1;
	if (Packed) {
		fDtype->type.properties |= TYPE_MISC_PACKED;
	} else {
		fDtype->type.properties &= ~TYPE_MISC_PACKED;
	}
    fDtype->IsDerived = 1;
    return 1;
 } // lInsertDimension

/*
 * Array_Declarator() - Declare an array of this type.
 *
 */

decl *Array_Declarator(SourceLoc *loc, decl *fDecl, int size, int Empty)
{
    dtype *lDtype;
    Type *lType;
    int dims;

    lDtype = &fDecl->type;
    if (Empty) {
        /* An empty bracket pair declares a dynamically sized array; the
         * explicit sentinel keeps 0 reserved for invalid/recovery types. */
        size = CG_ARRAY_UNSIZED;
    } else if (size <= 0) {
        SemanticError(loc, ERROR___DIMENSION_LT_1);
        size = 1;
    }
    if (IsVoid(&lDtype->type))
        SemanticError(loc, ERROR___ARRAY_OF_VOID);
    switch (GetCategory(&lDtype->type)) {
    case TYPE_CATEGORY_SCALAR:
    case TYPE_CATEGORY_SAMPLER:
        lType = lDtype->basetype;
        SetTypeCategory(loc, 0, lDtype, TYPE_CATEGORY_ARRAY, 1);
        lDtype->type.arr.eltype = lType;
        lDtype->type.arr.numels = size;
        lDtype->numNewDims = 1;
        break;
    case TYPE_CATEGORY_ARRAY:
        dims = lDtype->numNewDims;
        lInsertDimension(loc, lDtype, dims, size, lDtype->type.properties & TYPE_MISC_PACKED_KW);
        // if (TotalNumberDimensions > MAX_ARRAY_DIMENSIONS)
        //    SemanticError(loc, ERROR_D_EXCEEDS_MAX_DIMS, MAX_ARRAY_DIMENSIONS);
        break;
    case TYPE_CATEGORY_FUNCTION:
        SemanticError(loc, ERROR___ARRAY_OF_FUNS);
        break;
    case TYPE_CATEGORY_ATTRIB_ARRAY:
        /* Attribute arrays are canonical read-only inputs; they are
         * never arrayed further. */
        SemanticError(loc, ERROR_S_GEOMETRY_ATTRIB_PLACEMENT,
                      "a derived attribute-array shape");
        break;
    case TYPE_CATEGORY_STRUCT:
        lType = GetTypePointer(loc, lDtype);
        NewDType(lDtype, lType, TYPE_CATEGORY_ARRAY);
        lDtype->type.co.properties |= lType->co.properties & (TYPE_DOMAIN_MASK | TYPE_QUALIFIER_MASK);
        lDtype->type.arr.eltype = lType;
        lDtype->type.arr.numels = size;
        lDtype->numNewDims = 1;
        break;
    default:
        InternalError(loc, 999, "ArrayDeclarator(): unknown category");
        break;
    }
    lDtype->IsDerived = 1;
    return fDecl;
} // Array_Declarator

/*
 * AddFormalParamDecls() - Add a list of formal parameter declarations to a function
 *         definition's scope.
 */

Symbol *AddFormalParamDecls(Scope *fScope, decl *params)
{
    Symbol *lSymb, *first = NULL, *last;
    Type *lType;

    while (params) {
        lSymb = LookUpLocalSymbol(fScope, params->name);
        if (lSymb) {
            SemanticError(&params->loc, ERROR_S_PARAM_NAME_TWICE,
                          GetAtomString(atable, params->name));
        } else {
            lSymb = AddSymbol(&params->loc, fScope, params->name,
                              GetTypePointer(&params->loc, &params->type), VARIABLE_S);
            lSymb->properties |= SYMB_IS_PARAMETER;
            lSymb->details.var.semantics = params->semantics;
            lSymb->details.var.init = params->initexpr;
            if (first) {
                last->next = lSymb;
            } else {
                first = lSymb;
            }
            last = lSymb;
            lType = lSymb->type;
            if (IsCategory(lType, TYPE_CATEGORY_ARRAY) && !IsPacked(lType)) {
                if (!Cg->theHAL->GetCapsBit(CAPS_INDEXED_ARRAYS)) {
                    SemanticError(&params->loc, ERROR_S_UNPACKED_ARRAY,
                                  GetAtomString(atable, params->name));
                }
            }
            /* Sampler formals must be plain samplers: aggregates of
             * samplers have no language meaning in any profile. */
            {
                Type *element = lType;
                while (element && IsArray(element))
                    element = element->arr.eltype;
                if (IsArray(lType) && element && IsSampler(element, NULL))
                {
                    SemanticError(&params->loc, ERROR_S_SAMPLER_DECLARATION,
                                  GetAtomString(atable, params->name));
                }
            }
        }
        params = params->next;
    }
    return first;
} // AddFormalParamDecls

/*
 * SetFunTypeParams() - Build a list of types and set this function type's abstract parameter types.
 *
 */

decl *SetFunTypeParams(Scope *fScope, decl *func, decl *params, decl *actuals)
{
    TypeList *formals, *prev, *lType;

    fScope->InFormalParameters--;
    formals = prev = NULL;
    while (params) {
        lType = (TypeList *) malloc(sizeof(TypeList));
        lType->next = NULL;
        lType->type = GetTypePointer(&params->loc, &params->type);
        if (formals) {
            prev->next = lType;
        } else {
            formals = lType;
        }
        prev = lType;
        params = params->next;
    }
    if (func && IsCategory(&func->type.type, TYPE_CATEGORY_FUNCTION)) {
        func->type.type.fun.paramtypes = formals;
        func->type.IsDerived = 1;
    }
    if (actuals) {
        func->params = actuals;
    } else {
        if (!CurrentScope->HasVoidParameter)
            func->type.type.properties |= TYPE_MISC_ABSTRACT_PARAMS;
    }
    return func;
} // SetFunTypeParams


/*
 * FunctionDeclHeader()
 *
 */

decl *FunctionDeclHeader(SourceLoc *loc, Scope *fScope, decl *func)
{
    Type *rtnType = GetTypePointer(Cg->tokenLoc, &func->type);

    if (IsUnsizedArray(rtnType))
        SemanticError(loc, ERROR_S_UNSIZED_ARRAY, GetAtomString(atable, func->name));
    NewDType(&func->type, NULL, TYPE_CATEGORY_FUNCTION);
    CurrentScope->InFormalParameters++;
    func->type.type.properties |= rtnType->properties & (TYPE_MISC_INLINE | TYPE_MISC_INTERNAL);
    rtnType->properties &= ~(TYPE_MISC_INLINE | TYPE_MISC_INTERNAL);
    func->type.type.fun.paramtypes = NULL;
    func->type.type.fun.rettype = rtnType;
    func->type.IsDerived = 1;
    return func;
} // FunctionDeclHeader

/*
 * StructHeader() - Process a struct header.
 *
 */

Type *StructHeader(SourceLoc *loc, Scope *fScope, int cType, int tag)
{
    Symbol *lSymb;
    Type *lType;

    if (tag) {
        lSymb = LookUpTag(fScope, tag);
        if (!lSymb) {
            lSymb = AddTag(loc, fScope, tag, TYPE_CATEGORY_STRUCT);
            lSymb->type->str.tag = tag;
            lSymb->type->str.semantics = cType;
            lSymb->type->str.variety = CID_NONE_ID;
        }
        lType = lSymb->type;
        if (!IsCategory(lType, TYPE_CATEGORY_STRUCT)) {
            SemanticError(loc, ERROR_S_TAG_IS_NOT_A_STRUCT, GetAtomString(atable, tag));
            lType = UndefinedType;
        }
    } else {
        lType = NewType(TYPE_CATEGORY_STRUCT, 0);
    }
    return lType;
} // StructOrConnectorHeader

/*
 * InterfaceHeader() - Process an interface header.  The interface analog
 *         of StructHeader(): register or reuse the tag, whose type carries
 *         TYPE_CATEGORY_INTERFACE and an initially empty member scope.
 */

Type *InterfaceHeader(SourceLoc *loc, Scope *fScope, int tag)
{
    Symbol *lSymb;
    Type *lType;

    if (tag) {
        lSymb = LookUpTag(fScope, tag);
        if (!lSymb) {
            lSymb = AddTag(loc, fScope, tag, TYPE_CATEGORY_INTERFACE);
            /* AddTag seeds str.unqualifiedtype, which aliases
             * iface.members in the type union; interfaces never use
             * unqualifiedtype, so clear the overlap before the member
             * scope is attached at completion. */
            lSymb->type->iface.members = NULL;
            lSymb->type->iface.tag = tag;
        }
        lType = lSymb->type;
        if (!IsCategory(lType, TYPE_CATEGORY_INTERFACE)) {
            SemanticError(loc, ERROR_S_NAME_ALREADY_DEFINED,
                          GetAtomString(atable, tag));
            lType = UndefinedType;
        }
    } else {
        lType = NewType(TYPE_CATEGORY_INTERFACE, 0);
    }
    return lType;
} // InterfaceHeader

/*
 * SetStructInterface() - Interpret "struct Name : Type" where the
 *         right-hand identifier resolved to a declared type name.  Only
 *         interfaces may appear there; the connector-semantic form is
 *         handled by a separate production before a type name is ever
 *         consulted.  The implemented interface is recorded on the struct
 *         type so completion can check conformance.
 */

Type *SetStructInterface(SourceLoc *loc, Scope *fScope, int tag, int interfaceAtom)
{
    Type *interfaceType;
    Type *structType;

    interfaceType = LookUpTypeSymbol(fScope, interfaceAtom);
    if (!IsCategory(interfaceType, TYPE_CATEGORY_INTERFACE)) {
        SemanticError(loc, ERROR_S_TAG_IS_NOT_AN_INTERFACE,
                      GetAtomString(atable, interfaceAtom));
        interfaceType = NULL;
    }
    structType = StructHeader(loc, fScope, 0, tag);
    if (IsCategory(structType, TYPE_CATEGORY_STRUCT))
        structType->str.implementedInterface = interfaceType;
    return structType;
} // SetStructInterface

/*
 * lMarkMethod() - Record that this member function belongs to "fOwner".
 */

static void lMarkMethod(Symbol *fSymb, Type *fOwner)
{
    fSymb->details.fun.ownerType = fOwner;
    fSymb->details.fun.isMethod = 1;
} // lMarkMethod

/*
 * lSynthesizeMethodReceiver() - Give a method an implicit leading formal
 *         holding its receiver.  The receiver exists only inside the
 *         compiler: call sites prepend the receiver expression as an
 *         ordinary argument while source-level signatures keep the
 *         declared formals alone.  Both the formal symbol and the front
 *         of the function type's parameter list grow the owner type so
 *         argument binding, inlining, and conformance matching stay
 *         positionally consistent.
 */

static void lSynthesizeMethodReceiver(SourceLoc *loc, Symbol *fSymb, Type *fOwner)
{
    static int receiverAtom = 0;
    Symbol *receiver;
    TypeList *param;

    if (receiverAtom == 0)
        receiverAtom = LookUpAddString(atable, "$this");
    /* The owner type is shared, not duplicated: the receiver slot must
     * compare identical to the owning type at call sites. */
    receiver = AddSymbol(loc, fSymb->details.fun.locals, receiverAtom,
                         fOwner, VARIABLE_S);
    receiver->properties |= SYMB_IS_PARAMETER;
    receiver->next = fSymb->details.fun.params;
    fSymb->details.fun.params = receiver;
    param = (TypeList *) malloc(sizeof(TypeList));
    param->type = fOwner;
    param->next = fSymb->type->fun.paramtypes;
    fSymb->type->fun.paramtypes = param;
} // lSynthesizeMethodReceiver

/*
 * SetInterfaceMembers() - Complete an interface declaration: attach the
 *         member scope, keep only method prototypes (data members are
 *         rejected; bodies cannot be parsed inside an interface), mark
 *         the methods, and publish the tag as a type name.
 */

Type *SetInterfaceMembers(SourceLoc *loc, Type *fType, Scope *members)
{
    Symbol *lSymb, *tSymb;

    if (!fType || IsCategory(fType, TYPE_CATEGORY_INTERFACE) == 0)
        return fType;
    if (fType->iface.members) {
        SemanticError(loc, ERROR_SSD_STRUCT_ALREADY_DEFINED,
                      GetAtomString(atable, fType->iface.tag),
                      GetAtomString(atable, fType->iface.loc.file),
                      fType->iface.loc.line);
        return fType;
    }
    lSymb = members->symbols;
    while (lSymb) {
        if (!IsFunction(lSymb)) {
            SemanticError(&lSymb->loc, ERROR_S_INTERFACE_DATA_MEMBER,
                          GetAtomString(atable, fType->iface.tag));
        } else if (!lSymb->details.fun.isMethod) {
            lMarkMethod(lSymb, fType);
            lSynthesizeMethodReceiver(&lSymb->loc, lSymb, fType);
        }
        lSymb = lSymb->next;
    }
    fType->iface.members = members;
    fType->iface.loc = *loc;
    if (fType->iface.tag) {
        tSymb = LookUpLocalSymbol(CurrentScope, fType->iface.tag);
        if (!tSymb) {
            DefineTypedef(loc, CurrentScope, fType->iface.tag, fType);
        } else if (!IsTypedef(tSymb)) {
            SemanticError(loc, ERROR_S_NAME_ALREADY_DEFINED,
                          GetAtomString(atable, fType->iface.tag));
        }
    }
    return fType;
} // SetInterfaceMembers

/*
 * lSignatureHasPoison() - TRUE when a method's signature involves the
 *         poison recovery type: its declaration already reported its
 *         one diagnostic, so conformance checking must not report a
 *         second one about the same broken type.
 */

static int lSignatureHasPoison(Symbol *fSymb)
{
    TypeList *param;

    if (!fSymb->type)
        return 0;
    if (CgTypeIsPoison(fSymb->type->fun.rettype))
        return 1;
    for (param = fSymb->type->fun.paramtypes; param; param = param->next) {
        if (CgTypeIsPoison(param->type))
            return 1;
    }
    return 0;
} // lSignatureHasPoison

/*
 * lSignatureMatches() - Compare an implementing method against an
 *         interface method: same name is checked by the caller; here the
 *         return type, parameter count, parameter directions, and
 *         unqualified parameter types must agree.  The implicit receiver
 *         parameter both sides carry is skipped.
 */

static int lSignatureMatches(Symbol *fImpl, Symbol *fDecl)
{
    TypeList *implParam, *declParam;

    if (!IsSameUnqualifiedType(fImpl->type->fun.rettype,
                               fDecl->type->fun.rettype))
    {
        return 0;
    }
    implParam = fImpl->type->fun.paramtypes;
    declParam = fDecl->type->fun.paramtypes;
    if (implParam)
        implParam = implParam->next;
    if (declParam)
        declParam = declParam->next;
    while (implParam && declParam) {
        if ((GetQualifiers(implParam->type) & (TYPE_QUALIFIER_IN |
             TYPE_QUALIFIER_OUT)) !=
            (GetQualifiers(declParam->type) & (TYPE_QUALIFIER_IN |
             TYPE_QUALIFIER_OUT)))
        {
            return 0;
        }
        if (!IsSameUnqualifiedType(implParam->type, declParam->type))
            return 0;
        implParam = implParam->next;
        declParam = declParam->next;
    }
    return implParam == NULL && declParam == NULL;
} // lSignatureMatches

/*
 * CheckInterfaceConformance() - Complete a struct definition and validate
 *         it against the interface it implements, if any.  Completion
 *     marks every member function as a method and synthesizes its
 *         implicit receiver, so later phases see one uniform calling
 *         convention regardless of inheritance.  Conformance requires
 *         exactly one implementing method per interface method with a
 *         matching signature; failures report at the struct with the
 *         interface method's declaration as the note.
 */

void CheckInterfaceConformance(SourceLoc *loc, Type *fType)
{
    Type *interfaceType;
    Symbol *member, *decl, *named, *impl;

    if (!fType || !IsCategory(fType, TYPE_CATEGORY_STRUCT))
        return;
    for (member = fType->str.members ? fType->str.members->symbols : NULL;
         member; member = member->next)
    {
        if (IsFunction(member) && !member->details.fun.isMethod) {
            lMarkMethod(member, fType);
            lSynthesizeMethodReceiver(&member->loc, member, fType);
        }
    }
    interfaceType = fType->str.implementedInterface;
    if (!interfaceType)
        return;
    for (decl = interfaceType->iface.members ?
             interfaceType->iface.members->symbols : NULL;
         decl; decl = decl->next)
    {
        if (!IsFunction(decl))
            continue;
        named = impl = NULL;
        for (member = fType->str.members->symbols; member;
             member = member->next)
        {
            if (!IsFunction(member) || member->name != decl->name)
                continue;
            named = member;
            if (lSignatureMatches(member, decl)) {
                impl = member;
                break;
            }
        }
        if (impl)
            continue;
        if (named && lSignatureHasPoison(named)) {
            /* The mismatch is the poisoned signature itself; its
             * declaration already reported, so stay silent here. */
            continue;
        }
        if (named) {
            SemanticError(loc, ERROR_SSSSD_INTERFACE_METHOD_SIGNATURE,
                          GetAtomString(atable, decl->name),
                          GetAtomString(atable, fType->str.tag),
                          GetAtomString(atable, decl->loc.file),
                          decl->loc.line);
        } else {
            SemanticError(loc, ERROR_SSSSD_INTERFACE_METHOD_MISSING,
                          GetAtomString(atable, fType->str.tag),
                          GetAtomString(atable, decl->name),
                          GetAtomString(atable, decl->loc.file),
                          decl->loc.line);
        }
    }
} // CheckInterfaceConformance

/*
 * DefineVar() - Define a new variable in the current scope.
 *
 */

Symbol *DefineVar(SourceLoc *loc, Scope *fScope, int atom, Type *fType)
{
    Symbol *lSymb;

    lSymb = AddSymbol(loc, fScope, atom, fType, VARIABLE_S);
    return lSymb;
} // DefineVar

/*
 * DefineTypedef() - Define a new type name in the current scope.
 *
 */

Symbol *DefineTypedef(SourceLoc *loc, Scope *fScope, int atom, Type *fType)
{
    return AddSymbol(loc, fScope, atom, fType, TYPEDEF_S);
} // DefineTypedef

/*
 * DeclareFunc() - Declare an identifier as a function in the scope fScope.  If it's already
 *         in the symbol table check the overloading rules to make sure that it either
 *         A) matches a previous declaration exactly, or B) is unambiguously resolvable.
 */

Symbol *DeclareFunc(SourceLoc *loc, Scope *fScope, Symbol *fSymb, int atom, Type *fType,
                    Scope *locals, Symbol *params)
{
    int DiffParamTypes, DiffParamQualifiers, DiffParamCount, DiffReturnType;
    int DiffProfileSelector;
    TypeList *oldArgType, *newArgType;
    Symbol *lSymb;
    int index, group, OK;

    if (fSymb) {
        if (GetCategory(fSymb->type) != TYPE_CATEGORY_FUNCTION) {
            SemanticError(loc, ERROR_S_NAME_ALREADY_DEFINED, GetAtomString(atable, atom));
            lSymb = fSymb;
        } else {
            OK = 1;
            lSymb = fSymb;
            while (lSymb) {
                if (GetCategory(fSymb->type) != TYPE_CATEGORY_FUNCTION) {
                    InternalError(loc, ERROR_S_SYMBOL_TYPE_NOT_FUNCTION,
                                  GetAtomString(atable, lSymb->name));
                    return fSymb;
                }
                DiffParamTypes = DiffParamQualifiers = DiffParamCount = DiffReturnType = 0;
                DiffProfileSelector = 0;
                if (!IsSameUnqualifiedType(lSymb->type->fun.rettype, fType->fun.rettype))
                    DiffReturnType = 1;
                oldArgType = lSymb->type->fun.paramtypes;
                newArgType = fType->fun.paramtypes;
                while (newArgType && oldArgType) {
                    if (!IsSameUnqualifiedType(oldArgType->type, newArgType->type)) {
                        DiffParamTypes = 1;
                    } else if (GetQualifiers(oldArgType->type) != GetQualifiers(newArgType->type)) {
                        DiffParamQualifiers = 1;
                    }
                    oldArgType = oldArgType->next;
                    newArgType = newArgType->next;
                }
                if (newArgType || oldArgType)
                    DiffParamCount = 1;
                if (!DiffParamCount && !DiffParamTypes) {
                    if (DiffParamQualifiers) {
                        SemanticError(loc, ERROR_S_OVERLOAD_DIFF_ONLY_QUALS,
                                      GetAtomString(atable, atom));
                        OK = 0;
                        break;
                    }
                    if (DiffReturnType) {
                        SemanticError(loc, ERROR_S_OVERLOAD_DIFF_ONLY_RETURN,
                                      GetAtomString(atable, atom));
                        OK = 0;
                        break;
                    }
                    /* Profile qualifications distinguish same-signature
                     * overloads: a declaration naming a different
                     * profile than an existing candidate declares a new
                     * one rather than redefining it. */
                    if (lHavePendingProfileSpecifier &&
                        (lSymb->details.fun.profileSelector.isOpen ||
                         lSymb->details.fun.profileSelector.name !=
                             lPendingProfileSelector.name))
                    {
                        DiffProfileSelector = 1;
                    }
                    if (!DiffProfileSelector)
                        break; // Found the matching function
                }
                lSymb = lSymb->details.fun.overload;
            }
            if (OK) {
                if (DiffParamCount || DiffParamTypes || DiffProfileSelector) {
                    lSymb = NewSymbol(loc, fScope, atom, fType, FUNCTION_S);
                    lSymb->details.fun.params = params;
                    lSymb->details.fun.locals = locals;
                    lSymb->details.fun.overload = fSymb->details.fun.overload;
                    fSymb->details.fun.overload = lSymb;
                    if (GetCategory(fType) == TYPE_CATEGORY_FUNCTION) {
                        locals->returnType = fType->fun.rettype;
                    } else {
                        locals->returnType = UndefinedType;
                    }
                } else {
                    if (!(lSymb->properties & SYMB_IS_DEFINED)) {
                        // Overwrite previous definitions if this function is not yet defined.
                        // Prototype parameter names are ignored.
                        /* A matching redeclaration may not change an
                         * existing default argument; omitted defaults
                         * carry forward unchanged. */
                        lPreserveParameterDefaults(loc, lSymb,
                                                   params, atom);
                        lSymb->details.fun.params = params;
                        lSymb->details.fun.locals = locals;
                    } else {
                        // Declarator for a function that's already been defined.  Not an error.
                    }
                }
            } else {
                // Found a function that differs only by qualifiers or return type.  Error arleady issued.
                // lSymb = fSymb;
            }
        }
    } else {
        lSymb = AddSymbol(loc, fScope, atom, fType, FUNCTION_S);
        lSymb->details.fun.params = params;
        lSymb->details.fun.locals = locals;
        if (GetCategory(fType) == TYPE_CATEGORY_FUNCTION) {
            locals->returnType = fType->fun.rettype;
        } else {
            locals->returnType = UndefinedType;
        }
    }
    if (lSymb->type->properties & TYPE_MISC_INTERNAL) {
        if (lSymb->details.fun.intrinsic != NULL) {
            /* Declarative catalog intrinsic: identity and validity are
             * pinned at installation; no profile name-mapping here. */
            lSymb->properties |= SYMB_IS_BUILTIN;
        } else {
            int errorsBefore = GetErrorCount();
            index = Cg->theHAL->CheckInternalFunction(lSymb, &group);
            if (index) {
                //
                // lSymb->InternalIndex = index; etc.
                //
                lSymb->properties |= SYMB_IS_DEFINED | SYMB_IS_BUILTIN;
                lSymb->details.fun.group = group;
                lSymb->details.fun.index = index;
            } else if (GetErrorCount() == errorsBefore) {
                SemanticError(loc, ERROR_S_INVALID_INTERNAL_FUNCTION,
                              GetAtomString(atable, atom));
            }
        }
    }

    return lSymb;
} // DeclareFunc

/*
 * DefineFunction() - Set the body of the function "func" to the statements in "body".
 *
 */

void DefineFunction(SourceLoc *loc, Scope *fScope, decl *func, stmt *body)
{
    Symbol *lSymb = func->symb;
    SymbolList *lSymbList;
    Scope *globals;
    Type *lType;

    if (IsFunction(lSymb)) {
        if (body) {
            /* The definition restates or extends whatever modifiers its
             * declarations recorded; nothing is ever dropped here. */
            lMergeGeometryModifiers(&lSymb->details.fun.geometry,
                                    &func->type.geometry);
            if (lSymb->properties & SYMB_IS_DEFINED) {
                SemanticError(loc, ERROR_S_FUN_ALREADY_DEFINED,
                              GetAtomString(atable, lSymb->name));
            } else {
                lSymb->properties |= SYMB_IS_DEFINED;
                lSymb->details.fun.statements = body;
                lType = lSymb->type;
                if ((lType->properties & TYPE_MISC_INLINE) ||
                    Cg->theHAL->GetCapsBit(CAPS_INLINE_ALL_FUNCTIONS))
                {
                    lSymb->properties |= SYMB_IS_INLINE_FUNCTION;
                }
            }
            if (!fScope->HasReturnStmt && !IsVoid(fScope->returnType)) {
                SemanticError(loc, ERROR_S_FUNCTION_HAS_NO_RETURN,
                              GetAtomString(atable, lSymb->name));
            }
            if (func->type.type.properties & TYPE_MISC_PROGRAM) {
                globals = fScope->parent;
                if (!globals->programs) {
                    lSymbList = (SymbolList *) malloc(sizeof(SymbolList));
                    lSymbList->next = globals->programs;
                    lSymbList->symb = lSymb;
                    globals->programs = lSymbList;
                } else {
                    SemanticError(loc, ERROR_S_ONE_PROGRAM,
                                  GetAtomString(atable, globals->programs->symb->name));
                }
            }
        } else {
            SemanticError(loc, ERROR_S_NO_STATEMENTS, GetAtomString(atable, func->name));
        }
        if (Cg->options.DumpParseTree) {
            PrintScopeDeclarations();
            PrintFunction(lSymb);
        }
    }
} // DefineFunction

/*
 * GlobalInitStatements()
 *
 */

int GlobalInitStatements(Scope *fScope, stmt *fStmt)
{
    stmt *lStmt;

    if (fStmt) {
        if (fScope->initStmts) {
            lStmt = fScope->initStmts;
            while (lStmt->commonst.next)
                lStmt = lStmt->commonst.next;
            lStmt->commonst.next = fStmt;
        } else {
            fScope->initStmts = fStmt;
        }
    }
    return 1;
} // GlobalInitStatements

/*
 * BasicVariable() - A variable identifier has been encountered.
 *
 */

expr *BasicVariable(SourceLoc *loc, int name)
{
    Symbol *lSymb;
    
    lSymb = LookUpSymbol(CurrentScope, name);
    if (!lSymb) {
        SemanticError(loc, ERROR_S_UNDEFINED_VAR, GetAtomString(atable, name));
        lSymb = DefineVar(loc, CurrentScope, name, UndefinedType);
    }
    return (expr *) NewSymbNode(VARIABLE_OP, lSymb);
} // BasicVariable

/*
 * IsLValue() - Is this expression an l-value?
 *
 */

int IsLValue(const expr *fExpr)
{
    if (fExpr) {
        return fExpr->common.IsLValue;
    } else {
        return 0;
    }
} // IsLValue

/*
 * IsConst() - Is this expression an l-value?
 *
 */

int IsConst(const expr *fExpr)
{
    if (fExpr) {
        return fExpr->common.IsConst;
    } else {
        return 0;
    }
} // IsConst


/*
 * IsArrayIndex() - Is this expression an array index expression?
 *
 */

int IsArrayIndex(const expr *fExpr)
{
    if (fExpr) {
        return fExpr->common.kind == BINARY_N && fExpr->bin.op == ARRAY_INDEX_OP;
    } else {
        return 0;
    }
} // IsArrayIndex

/*
 * lIsNumericKind() - TRUE if a canonical scalar kind participates in
 *                    arithmetic (integral or floating family, including the
 *                    compile-time kinds).
 *
 */

static int lIsNumericKind(CgScalarKind kind)
{
    return CgScalarIsIntegral(kind) || CgScalarIsFloating(kind);
} // lIsNumericKind

/*
 * lCastTargetType() - The canonical unqualified form of a cast target for
 *                     scalar, vector, and matrix results so expression nodes
 *                     carry interned types whose scalar kinds survive kinds
 *                     above the four-bit legacy bases; other categories use
 *                     the target type verbatim.
 *
 */

static Type *lCastTargetType(Type *toType)
{
    switch (GetCategory(toType)) {
    case TYPE_CATEGORY_SCALAR:
        return GetStandardTypeKind(GetScalarKind(toType), 0, 0);
    case TYPE_CATEGORY_ARRAY:
        if (IsVector(toType, NULL)) {
            return GetStandardTypeKind(GetScalarKind(toType),
                                       toType->arr.numels, 0);
        }
        if (IsMatrix(toType, NULL, NULL)) {
            return GetStandardTypeKind(GetScalarKind(toType),
                                       toType->arr.numels,
                                       toType->arr.eltype->arr.numels);
        }
        break;
    default:
        break;
    }
    return toType;
} // lCastTargetType

/*
 * lNewShapeCast() - Build a typed shape-conversion node.  The target type
 *                   rides in the node's targetType field; no four-bit subop
 *                   encoding is involved.
 *
 */

static expr *lNewShapeCast(opcode op, expr *fExpr, Type *toType)
{
    unary *unnode = NewUnopSubNode(op, 0, fExpr);

    unnode->type = toType;
    unnode->targetType = toType;
    return (expr *) unnode;
} // lNewShapeCast

/*
 * ConvertType() - Type cast fExpr from fromType to toType if needed.  Ignore qualifiers.
 *
 * If "result" is NULL just check validity of cast; don't allocate cast operator node.
 *
 * AllowShapeConversions gates the conversions that introduce or collapse
 * aggregate structure (scalar replication and first-element extraction):
 * typed contexts such as assignments, initializers, returns, and casts
 * enable them, while overload-resolution probing and argument binding pass
 * 0 so function selection keeps its exact-shape rules until ranked
 * conversion landing replaces it.
 *
 */

int ConvertType(SourceLoc *loc, expr *fExpr, Type *toType, Type *fromType,
                expr **result, int IgnorePacked, int Explicit,
                int AllowShapeConversions)
{
    CgConversionRank rank;
    unary *unnode;
    Type *targetType;
    int ToPacked, FromPacked;

    if (!toType || !fromType)
        return 0;
    if (CgTypeIsPoison(toType) || CgTypeIsPoison(fromType)) {
        /* A poisoned operand already reported its one diagnostic;
         * conversion layers return early instead of reporting again. */
        return 0;
    }
    ToPacked = (toType->properties & TYPE_MISC_PACKED) != 0;
    FromPacked = (fromType->properties & TYPE_MISC_PACKED) != 0;
    if (Explicit && IsSameUnqualifiedType(toType, fromType) &&
        IsSampler(toType, NULL))
    {
        /* Samplers are opaque: they cannot be cast, not even to their
         * own type. */
        return 0;
    }
    if (IsSameUnqualifiedType(toType, fromType) &&
        ((ToPacked == FromPacked) || IgnorePacked))
    {
        if (result)
            *result = fExpr;
        return 1;
    }
    if (GetCategory(toType) == TYPE_CATEGORY_ARRAY &&
        GetCategory(fromType) == TYPE_CATEGORY_ARRAY &&
        IsUnsizedArray(toType) &&
        IsPacked(toType) == IsPacked(fromType) &&
        IsSameUnqualifiedType(toType->arr.eltype, fromType->arr.eltype))
    {
        /* A concrete array binds identically to an unsized array with the
         * same element shape (parameter passing and whole-array copies):
         * the runtime length travels with the value, so no conversion
         * node exists and no shape information is lost.  This is exact-
         * shape compatibility, not a scalar<->aggregate shape conversion,
         * so it stays valid while probing overload resolution too. */
        if (result)
            *result = fExpr;
        return 1;
    }
    if (!AllowShapeConversions &&
        ((GetCategory(fromType) == TYPE_CATEGORY_SCALAR &&
          GetCategory(toType) == TYPE_CATEGORY_ARRAY) ||
         (GetCategory(fromType) == TYPE_CATEGORY_ARRAY &&
          GetCategory(toType) == TYPE_CATEGORY_SCALAR)))
    {
        return 0;
    }
    rank = CgClassifyConversion(fromType, toType, Explicit);
    if (rank == CG_CONVERSION_NONE)
        return 0;
    if (GetCategory(fromType) == TYPE_CATEGORY_ARRAY &&
        GetCategory(toType) == TYPE_CATEGORY_ARRAY &&
        !IgnorePacked && ToPacked != FromPacked)
    {
        /* Array-to-array conversions must agree on packedness. */
        return 0;
    }
    if (!result)
        return 1;
    if (rank == CG_CONVERSION_IMPLICIT_WARN)
        SemanticWarning(loc, WARNING___IMPLICIT_CONVERSION);
    targetType = lCastTargetType(toType);
    switch (GetCategory(fromType)) {
    case TYPE_CATEGORY_SCALAR:
        switch (GetCategory(toType)) {
        case TYPE_CATEGORY_SCALAR:
            unnode = NewUnopSubNode(CAST_CS_OP,
                                    SUBOP_CS(CgScalarLegacyBase(GetScalarKind(toType)),
                                             CgScalarLegacyBase(GetScalarKind(fromType))),
                                    fExpr);
            unnode->type = targetType;
            unnode->targetType = targetType;
            unnode->HasSideEffects = fExpr->common.HasSideEffects;
            *result = (expr *) unnode;
            return 1;
        case TYPE_CATEGORY_ARRAY:
            /* Scalar replication fills every element of the target. */
            *result = lNewShapeCast(CAST_SHAPE_OP, fExpr, targetType);
            return 1;
        default:
            return 0;
        }
        break;
    case TYPE_CATEGORY_ARRAY:
        switch (GetCategory(toType)) {
        case TYPE_CATEGORY_SCALAR:
            /* First-element extraction. */
            *result = lNewShapeCast(CAST_SHAPE_OP, fExpr, targetType);
            return 1;
        case TYPE_CATEGORY_ARRAY:
            if (fromType->arr.numels == toType->arr.numels &&
                IsScalar(fromType->arr.eltype) && IsScalar(toType->arr.eltype))
            {
                /* Same-size element conversion keeps its legacy node. */
                unnode = NewUnopSubNode(CAST_CV_OP,
                                        SUBOP_CV(CgScalarLegacyBase(GetScalarKind(toType->arr.eltype)),
                                                 toType->arr.numels,
                                                 CgScalarLegacyBase(GetScalarKind(fromType->arr.eltype))),
                                        fExpr);
                unnode->type = targetType;
                unnode->targetType = targetType;
                unnode->HasSideEffects = fExpr->common.HasSideEffects;
                *result = (expr *) unnode;
            } else {
                *result = lNewShapeCast(CAST_SHAPE_OP, fExpr, targetType);
            }
            return 1;
        default:
            return 0;
        }
        break;
    case TYPE_CATEGORY_STRUCT:
        *result = lNewShapeCast(CAST_STRUCT_OP, fExpr, targetType);
        return 1;
    default:
        if (GetCategory(toType) == TYPE_CATEGORY_STRUCT) {
            *result = lNewShapeCast(CAST_STRUCT_OP, fExpr, targetType);
            return 1;
        }
        return 0;
    }
} // ConvertType

/*
 * CastScalarVectorMatrix() - Cast a scalar, vector, or matrix expression.
 *
 * Scalar: len = 0.
 * Vector: len >= 1 and len2 = 0
 * Matrix: len >= 1 and len2 >= 1
 *
 * len = 1 means "float f[1]" not "float f"
 *
 * The node type comes from the interned canonical registry so kinds above
 * the four-bit legacy bases survive on expression nodes; the subop keeps
 * the legacy bases purely as lowering information.
 *
 */

expr *CastScalarVectorMatrix(expr *fExpr, CgScalarKind fkind, CgScalarKind tkind,
                             int len, int len2)
{
    opcode op;
    int subop;
    unary *unnode;
    Type *tType;

    if (len == 0) {
        op = CAST_CS_OP;
        subop = SUBOP_CS(CgScalarLegacyBase(tkind), CgScalarLegacyBase(fkind));
        tType = GetStandardTypeKind(tkind, 0, 0);
    } else if (len2 == 0) {
        op = CAST_CV_OP;
        subop = SUBOP_CV(CgScalarLegacyBase(tkind), len, CgScalarLegacyBase(fkind));
        tType = GetStandardTypeKind(tkind, len, 0);
    } else {
        op = CAST_CM_OP;
        subop = SUBOP_CM(len2, CgScalarLegacyBase(tkind), len, CgScalarLegacyBase(fkind));
        /* Same layout as GetStandardType(tbase, len, len2): [len2] of [len]. */
        tType = GetStandardTypeKind(tkind, len2, len);
    }
    unnode = NewUnopSubNode(op, subop, fExpr);
    unnode->type = tType;
    unnode->targetType = tType;
    return (expr *) unnode;
} // CastScalarVectorMatrix

/*
 * ConvertNumericOperands() - Convert two scalar, vector, or matrix expressions to the same type
 *         for use in an expression.  Number of dimensions and lengths may differ.
 *
 * The common result kind is the language-level usual arithmetic conversion
 * of the two operand element kinds.
 *
 * Returns: canonical scalar type of the resulting values, NULL if the
 *          operands are not both numeric.
 *
 */

Type *ConvertNumericOperands(int baseop, expr **lexpr, expr **rexpr,
                             Type *lType, Type *rType,
                             int llen, int rlen, int llen2, int rlen2)
{
    Type *nType;
    CgScalarKind lkind, rkind, nkind;

    nType = CgUsualArithmeticType(lType, rType);
    if (!nType)
        return NULL;
    lkind = GetScalarKind(lType);
    rkind = GetScalarKind(rType);
    nkind = GetScalarKind(nType);
    if (nkind != lkind)
        *lexpr = CastScalarVectorMatrix(*lexpr, lkind, nkind, llen, llen2);
    if (nkind != rkind)
        *rexpr = CastScalarVectorMatrix(*rexpr, rkind, nkind, rlen, rlen2);
    return nType;
} // ConvertNumericOperands

/*
 * CheckBooleanExpr()
 *
 */

expr *CheckBooleanExpr(SourceLoc *loc, expr *fExpr, int AllowVector)
{
    int len = 0, HasError = 0;
    Type *lType, *leltype;

    lType = leltype = fExpr->common.type;
    if (IsScalar(lType)) {
        if (!IsBoolean(lType)) {
            SemanticError(loc, ERROR___BOOL_EXPR_EXPECTED);
            HasError = 1;
        }
    } else if (IsVector(lType, &len)) {
        leltype = lType->arr.eltype;
        if (AllowVector) {
            if (len > 4) {
                SemanticError(loc, ERROR___VECTOR_EXPR_LEN_GR_4);
                HasError = 1;
                len = 4;
            }
            if (!IsBoolean(lType)) {
                SemanticError(loc, ERROR___BOOL_EXPR_EXPECTED);
                HasError = 1;
            }
        } else {
            SemanticError(loc, ERROR___SCALAR_BOOL_EXPR_EXPECTED);
            HasError = 1;
        }
    } else {
        SemanticError(loc, ERROR___BOOL_EXPR_EXPECTED);
        HasError = 1;
    }
    if (HasError)
        fExpr->common.type = GetStandardType(TYPE_BASE_BOOLEAN, len, 0);
    return fExpr;
} // CheckBooleanExpr

/*
 * NewUnaryOperator() - See if this is a valid unary operation.  Return a new node with the
 *         proper operator description.
 *
 * Valid operators are:
 *
 *     op      arg1    arg2   result
 *   ------   ------  ------  ------
 *   NEG      scalar  scalar  scalar
 *   NEG_V    vector  vector  vector
 *
 */

expr *NewUnaryOperator(SourceLoc *loc, int fop, int name, expr *fExpr, int IntegralOnly)
{
    int lop, subop = 0, HasError = 0, len = 0;
    CgScalarKind lkind;
    Type *lType, *eltype;
    unary *result = NULL;
    int MustBeBoolean, OK = 0;

    lop = fop;
    MustBeBoolean = fop == BNOT_OP ? 1 : 0;
    lType = eltype = fExpr->common.type;
    if (IsScalar(lType) || IsSampler(lType, NULL)) {
        subop = 0;
    } else if (IsVector(lType, &len)) {
        eltype = lType->arr.eltype;
        lop = fop + OFFSET_V_OP;
        subop = SUBOP_V(len, 0);
    } else {
        SemanticError(loc, ERROR_S_INVALID_OPERANDS, GetAtomString(atable, name));
        HasError = 1;
    }
    if (!HasError) {
        if (len > 4) {
            SemanticError(loc, ERROR_S_VECTOR_OPERAND_GR_4, GetAtomString(atable, name));
        } else {
            lkind = GetScalarKind(eltype);
            SUBOP_SET_T(subop, CgScalarLegacyBase(lkind));
            if (MustBeBoolean) {
                if (lkind == CG_SCALAR_BOOL) {
                    OK = 1;
                } else {
                    SemanticError(loc, ERROR___BOOL_EXPR_EXPECTED);
                }
            } else {
                if (lIsNumericKind(lkind)) {
                    if (IntegralOnly) {
                        if (CgScalarIsIntegral(lkind)) {
                            OK = 1;
                        } else {
                            SemanticError(loc, ERROR_S_OPERANDS_NOT_INTEGRAL, GetAtomString(atable, name));
                        }
                    } else {
                        OK = 1;
                    }
                } else {
                    SemanticError(loc, ERROR_S_OPERANDS_NOT_NUMERIC, GetAtomString(atable, name));
                }
            }
            if (OK) {
                result = NewUnopSubNode(lop, subop, fExpr);
                result->type = GetStandardTypeKind(lkind, len, 0);
            }
        }
    }
    if (!result) {
        result = NewUnopSubNode(lop, 0, fExpr);
        result->type = UndefinedType;
    }
    return (expr *) result;
} // NewUnaryOperator

/*
 * NewBinaryOperator() - See if this is a valid binary operation.  Return a new node with the
 *         proper operator description.
 *
 * Valid operators are:
 *
 *     op      arg1    arg2   result
 *   ------   ------  ------  ------
 *   MUL      scalar  scalar  scalar
 *   MUL_V    vector  vector  vector
 *   MUL_SV*  scalar  vector  vector
 *   MUL_VS*  vector  scalar  vector
 *
 *    *only allowed for smearing operators MUL, DIV, ADD, SUB.
 */

expr *NewBinaryOperator(SourceLoc *loc, int fop, int name, expr *lExpr, expr *rexpr, int IntegralOnly)
{
    int lop, subop = 0, HasError = 0, llen = 0, rlen = 0, nlen;
    CgScalarKind nkind;
    Type *lType, *rtype, *leltype, *reltype, *nType;
    binary *result = NULL;
    int CanSmear;

    lop = fop;
    CanSmear = fop == MUL_OP || fop == DIV_OP || fop == ADD_OP || fop == SUB_OP ? 1 : 0;
    lType = leltype = lExpr->common.type;
    rtype = reltype = rexpr->common.type;
    if (IsScalar(lType) || IsSampler(lType, NULL)) {
        if (IsScalar(rtype) || IsSampler(rtype, NULL)) {
            subop = 0;
        } else if (IsVector(rtype, &rlen)) {
            if (CanSmear) {
                reltype = rtype->arr.eltype;
                lop = fop + OFFSET_SV_OP;
                subop = SUBOP_SV(rlen, 0);
            } else {
                SemanticError(loc, ERROR_S_SCALAR_OP_VECTOR_INVALID, GetAtomString(atable, name));
                HasError = 1;
            }
        } else {
            SemanticError(loc, ERROR_S_INVALID_OPERANDS, GetAtomString(atable, name));
            HasError = 1;
        }
    } else if (IsVector(lType, &llen)) {
        leltype = lType->arr.eltype;
        if (IsScalar(rtype) || IsSampler(rtype, NULL)) {
            if (CanSmear) {
                lop = fop + OFFSET_VS_OP;
                subop = SUBOP_VS(llen, 0);
            } else {
                SemanticError(loc, ERROR_S_VECTOR_OP_SCALAR_INVALID, GetAtomString(atable, name));
                HasError = 1;
            }
        } else {
            if (IsVector(rtype, &rlen)) {
                reltype = rtype->arr.eltype;
                lop = fop + OFFSET_V_OP;
                subop = SUBOP_VS(llen, 0);
                if (llen != rlen) {
                    SemanticError(loc, ERROR_S_VECTOR_OPERANDS_DIFF_LEN, GetAtomString(atable, name));
                    HasError = 1;
                }
            } else {
                SemanticError(loc, ERROR_S_INVALID_OPERANDS, GetAtomString(atable, name));
                HasError = 1;
            }
        }
    } else {
        SemanticError(loc, ERROR_S_INVALID_OPERANDS, GetAtomString(atable, name));
        HasError = 1;
    }
    if (!HasError) {
        if (llen > 4 || rlen > 4) {
            SemanticError(loc, ERROR_S_VECTOR_OPERAND_GR_4, GetAtomString(atable, name));
        } else {
            if (lIsNumericKind(GetScalarKind(leltype)) &&
                lIsNumericKind(GetScalarKind(reltype)))
            {
                nType = ConvertNumericOperands(fop, &lExpr, &rexpr, leltype, reltype,
                                               llen, rlen, 0, 0);
                nkind = GetScalarKind(nType);
                SUBOP_SET_T(subop, CgScalarLegacyBase(nkind));
                nlen = llen > rlen ? llen : rlen;
                result = NewBinopSubNode(lop, subop, lExpr, rexpr);
                result->type = GetStandardTypeKind(nkind, nlen, 0);
                if (IntegralOnly && !CgScalarIsIntegral(nkind)) {
                    SemanticError(loc, ERROR_S_OPERANDS_NOT_INTEGRAL, GetAtomString(atable, name));
                }
            } else {
                SemanticError(loc, ERROR_S_OPERANDS_NOT_NUMERIC, GetAtomString(atable, name));
            }
        }
    }
    if (!result) {
        result = NewBinopSubNode(lop, 0, lExpr, rexpr);
        result->type = UndefinedType;
    }
    return (expr *) result;
} // NewBinaryOperator

/*
 * NewBinaryBooleanOperator() - See if this is a valid binary Boolean operator.  Return a new
 *         node with the proper operator description.
 *
 * Valid operators are:
 *
 *     op      arg1    arg2   result
 *   ------   ------  ------  ------
 *   BAND     scalar  scalar  scalar
 *   BAND_V   vector  vector  vector
 *
 */

expr *NewBinaryBooleanOperator(SourceLoc *loc, int fop, int name, expr *lExpr, expr *rexpr)
{
    int lop, subop = 0, HasError = 0, llen = 0, rlen = 0;
    int lbase, rbase;
    Type *lType, *rtype, *leltype, *reltype;
    binary *result = NULL;

    lop = fop;
    lType = leltype = lExpr->common.type;
    rtype = reltype = rexpr->common.type;
    if (IsScalar(lType) || IsSampler(lType, NULL)) {
        if (IsScalar(rtype) || IsSampler(rtype, NULL)) {
            subop = SUBOP__(TYPE_BASE_BOOLEAN);
        } else {
            SemanticError(loc, ERROR_S_INVALID_OPERANDS, GetAtomString(atable, name));
            HasError = 1;
        }
    } else if (IsVector(lType, &llen)) {
        leltype = lType->arr.eltype;
        if (IsVector(rtype, &rlen)) {
            reltype = rtype->arr.eltype;
            lop = fop + OFFSET_V_OP;
            subop = SUBOP_V(llen, TYPE_BASE_BOOLEAN);
            if (llen != rlen) {
                SemanticError(loc, ERROR_S_VECTOR_OPERANDS_DIFF_LEN, GetAtomString(atable, name));
                HasError = 1;
            }
        } else {
            SemanticError(loc, ERROR_S_INVALID_OPERANDS, GetAtomString(atable, name));
            HasError = 1;
        }
    } else {
        SemanticError(loc, ERROR_S_INVALID_OPERANDS, GetAtomString(atable, name));
        HasError = 1;
    }
    if (!HasError) {
        if (llen > 4) {
            SemanticError(loc, ERROR_S_VECTOR_OPERAND_GR_4, GetAtomString(atable, name));
        } else {
            lbase = GetBase(lType);
            rbase = GetBase(rtype);
            if (lbase == TYPE_BASE_BOOLEAN && rbase == TYPE_BASE_BOOLEAN) {
                result = NewBinopSubNode(lop, subop, lExpr, rexpr);
                result->type = GetStandardType(TYPE_BASE_BOOLEAN, llen, 0);
            } else {
                SemanticError(loc, ERROR_S_OPERANDS_NOT_BOOLEAN, GetAtomString(atable, name));
            }
            if (lExpr->common.HasSideEffects || rexpr->common.HasSideEffects) {
                SemanticError(loc, ERROR_S_OPERANDS_HAVE_SIDE_EFFECTS, GetAtomString(atable, name));
            }
        }
    }
    if (!result) {
        result = NewBinopSubNode(lop, 0, lExpr, rexpr);
        result->type = UndefinedType;
    }
    return (expr *) result;
} // NewBinaryBooleanOperator

/*
 * NewBinaryComparisonOperator() - See if this is a valid binary comparison.  Return a new node
 *         with the proper operator description.
 *
 * Valid operators are:
 *
 *    op     arg1    arg2   result
 *   ----   ------  ------  ------
 *   LT     scalar  scalar  scalar
 *   LT_V   vector  vector  vector
 *
 */

expr *NewBinaryComparisonOperator(SourceLoc *loc, int fop, int name, expr *lExpr, expr *rexpr)
{
    int lop, subop = 0, HasError = 0, llen = 0, rlen = 0, nlen = 0;
    Type *lType, *rtype, *leltype, *reltype, *nType;
    binary *result = NULL;

    lop = fop;
    lType = leltype = lExpr->common.type;
    rtype = reltype = rexpr->common.type;
    if (IsScalar(lType) || IsSampler(lType, NULL)) {
        if (IsScalar(rtype) || IsSampler(rtype, NULL)) {
            subop = 0;
        } else if (IsVector(rtype, &rlen)) {
            reltype = rtype->arr.eltype;
            lop = fop + OFFSET_SV_OP;
            subop = SUBOP_SV(rlen, 0);
        } else {
            SemanticError(loc, ERROR_S_INVALID_OPERANDS, GetAtomString(atable, name));
            HasError = 1;
        }
    } else if (IsVector(lType, &llen)) {
        leltype = lType->arr.eltype;
        if (IsScalar(rtype)) {
            lop = fop + OFFSET_VS_OP;
            subop = SUBOP_VS(llen, 0);
        } else if (IsVector(rtype, &rlen)) {
            reltype = rtype->arr.eltype;
            lop = fop + OFFSET_V_OP;
            subop = SUBOP_V(llen, 0);
            if (llen != rlen) {
                SemanticError(loc, ERROR_S_VECTOR_OPERANDS_DIFF_LEN, GetAtomString(atable, name));
                HasError = 1;
            }
            nlen = llen;
        } else {
            SemanticError(loc, ERROR_S_INVALID_OPERANDS, GetAtomString(atable, name));
            HasError = 1;
        }
    } else {
        SemanticError(loc, ERROR_S_INVALID_OPERANDS, GetAtomString(atable, name));
        HasError = 1;
    }
    if (!HasError) {
        if (nlen > 4) {
            SemanticError(loc, ERROR_S_VECTOR_OPERAND_GR_4, GetAtomString(atable, name));
        } else {
            if (lIsNumericKind(GetScalarKind(lType)) &&
                lIsNumericKind(GetScalarKind(rtype)))
            {
                nType = ConvertNumericOperands(fop, &lExpr, &rexpr, lType, rtype,
                                               llen, rlen, 0, 0);
                SUBOP_SET_T(subop, CgScalarLegacyBase(GetScalarKind(nType)));
                nlen = llen > rlen ? llen : rlen;
                result = NewBinopSubNode(lop, subop, lExpr, rexpr);
                result->type = GetStandardType(TYPE_BASE_BOOLEAN, nlen, 0);
            } else if (GetScalarKind(lType) == CG_SCALAR_BOOL &&
                       GetScalarKind(rtype) == CG_SCALAR_BOOL) {
                subop = SUBOP_V(nlen, TYPE_BASE_BOOLEAN);
                result = NewBinopSubNode(lop, subop, lExpr, rexpr);
                result->type = GetStandardType(TYPE_BASE_BOOLEAN, nlen, 0);
            } else {
                SemanticError(loc, ERROR_S_OPERANDS_NOT_NUMERIC, GetAtomString(atable, name));
            }
        }
    }
    if (!result) {
        result = NewBinopSubNode(lop, 0, lExpr, rexpr);
        result->type = UndefinedType;
    }
    return (expr *) result;
} // NewBinaryComparisonOperator

/*
 * NewConditionalOperator() - Check the types of the components of a conditional expression.
 *         Return a new node with the proper operator description.
 *
 * Valid forma are:
 *
 *     op       cond    exp1    exp2   result
 *   -------   ------  ------  ------  ------
 *   COND      scalar  scalar  scalar  scalar
 *   COND_SV   scalar  vector  vector  vector
 *   COND_V    vector  vector  vector  vector
 *   COND_GEN  scalar   type    type    type
 */

expr *NewConditionalOperator(SourceLoc *loc, expr *bexpr, expr *lExpr, expr *rexpr)
{
    int lop, subop, blen = 0, llen = 0, rlen = 0, nlen = 0;
    int HasError = 0, LIsNumeric, LIsBoolean, LIsSimple;
    int category;
    CgScalarKind lkind, rkind, nkind;
    Type *btype, *lType, *rtype, *beltype, *leltype, *reltype, *nType;
    Type *resulttype = UndefinedType;
    trinary *result = NULL;

    // Type of conditional expression is checked elsewhere.

    lop = COND_OP;
    subop = 0;
    btype = beltype = bexpr->common.type;
    lType = leltype = lExpr->common.type;
    rtype = reltype = rexpr->common.type;
    lkind = GetScalarKind(leltype);
    rkind = GetScalarKind(reltype);
    LIsNumeric = lIsNumericKind(lkind) & lIsNumericKind(rkind);
    LIsBoolean = (lkind == CG_SCALAR_BOOL) & (rkind == CG_SCALAR_BOOL);
    LIsSimple = LIsNumeric | LIsBoolean;
    if (LIsSimple) {

        // 1) Numeric

        if (IsScalar(btype)) {

            // 1A) Scalar ? Scalar : Scalar

            if (IsScalar(lType)) {
                if (IsScalar(rtype)) {
                    // O.K.
                } else {
                    SemanticError(loc, ERROR___QSTN_SCALAR_3RD_OPND_EXPECTED);
                    HasError = 1;
                }

            // 1B) Scalar ? Vector : Vector

            } else if (IsVector(lType, &llen)) {
                leltype = lType->arr.eltype;
                if (IsVector(rtype, &rlen)) {
                    reltype = rtype->arr.eltype;
                    lkind = GetScalarKind(leltype);
                    rkind = GetScalarKind(reltype);
                    lop = COND_SV_OP;
                    subop = SUBOP_SV(llen, 0);
                } else {
                    SemanticError(loc, ERROR___QSTN_VECTOR_3RD_OPND_EXPECTED);
                    HasError = 1;
                }

            // 1C) Scalar ? Array : Array >>--->> Treat as non-numeric case

            } else {
                LIsSimple = 0; // Check type compatibility later
            }
        } else if (IsVector(btype, &blen)) {

            // 1D) Vector ? Vector : Vector

            if (IsVector(lType, &llen) && IsVector(rtype, &rlen)) {
                lop = COND_V_OP;
                subop = SUBOP_SV(llen, 0);
                leltype = lType->arr.eltype;
                reltype = rtype->arr.eltype;
                lkind = GetScalarKind(leltype);
                rkind = GetScalarKind(reltype);
            } else {
                SemanticError(loc, ERROR___QSTN_VECTOR_23_OPNDS_EXPECTED);
                HasError = 1;
            }
        } else {
            SemanticError(loc, ERROR___QSTN_INVALID_1ST_OPERAND);
            HasError = 1;
        }
    }
    if (!LIsSimple) {

        // 2) Not numeric - must be same type.  Requires scalar condition.

        if (IsScalar(btype)) {
            if (IsSameUnqualifiedType(lType, rtype)) {
                lop = COND_GEN_OP;
                resulttype = lType;
            } else {
                SemanticError(loc, ERROR___QSTN_23_OPNDS_INCOMPAT);
                HasError = 1;
            }
        } else {
            SemanticError(loc, ERROR___QSTN_1ST_OPERAND_NOT_SCALAR);
            HasError = 1;
        }
    }
    if (!HasError) {
        if ((lExpr->common.HasSideEffects ||
             rexpr->common.HasSideEffects) &&
            !Cg->theHAL->GetCapsBit(CAPS_CONDITIONAL_SIDE_EFFECTS))
        {
            SemanticError(loc, ERROR_S_OPERANDS_HAVE_SIDE_EFFECTS, "?:");
        }
        if (LIsSimple) {
            if (LIsBoolean) {
                nType = GetStandardTypeKind(CG_SCALAR_BOOL, 0, 0);
                if (llen == rlen && (blen == 0 || blen == llen)) {
                    SUBOP_SET_T(subop, TYPE_BASE_BOOLEAN);
                    result = NewTriopSubNode(lop, subop, bexpr, lExpr, rexpr);
                    result->type = GetStandardType(TYPE_BASE_BOOLEAN, llen, 0);
                } else {
                    SemanticError(loc, ERROR_S_VECTOR_OPERANDS_DIFF_LEN, "\"? :\"");
                    HasError = 1;
                }
            } else {
                nType = ConvertNumericOperands(COND_OP, &lExpr, &rexpr, leltype, reltype,
                                               llen, rlen, 0, 0);
                if (!nType) {
                    SemanticError(loc, ERROR___QSTN_23_OPNDS_INVALID);
                    HasError = 1;
                } else {
                    nkind = GetScalarKind(nType);
                    if (llen == rlen && (blen == 0 || blen == llen)) {
                        SUBOP_SET_T(subop, CgScalarLegacyBase(nkind));
                        result = NewTriopSubNode(lop, subop, bexpr, lExpr, rexpr);
                        result->type = GetStandardTypeKind(nkind, llen, 0);
                    } else {
                        SemanticError(loc, ERROR_S_VECTOR_OPERANDS_DIFF_LEN, "\"? :\"");
                        HasError = 1;
                    }
                }
            }
        } else {
            category = GetCategory(lType);
            if ((category == TYPE_CATEGORY_SCALAR ||
                 category == TYPE_CATEGORY_ARRAY ||
                 category == TYPE_CATEGORY_STRUCT ||
                 category == TYPE_CATEGORY_SAMPLER) &&
                !IsVoid(lType))
            {
                if (category == TYPE_CATEGORY_SAMPLER)
                {
                    /* Conditional selection would copy a sampler value. */
                    SemanticError(loc, ERROR___SAMPLER_CONDITIONAL);
                    HasError = 1;
                }
                result = NewTriopSubNode(lop, 0, bexpr, lExpr, rexpr);
                result->type = lType;
            } else {
                SemanticError(loc, ERROR___QSTN_23_OPNDS_INVALID);
                HasError = 1;
            }
        }
    }
    if (!result) {
        result = NewTriopSubNode(lop, 0, bexpr, lExpr, rexpr);
        result->type = UndefinedType;
    }
    return (expr *) result;
} // NewConditionalOperator

/*
 * NewSwizzleOperator() - See if this is a valid swizzle operation.  Return a new node with the
 *         proper operator description.
 *
 */

expr *NewSwizzleOperator(SourceLoc *loc, expr *fExpr, int ident)
{
    int HasError = 0, len = 0, ii, maxi, base, tmask, mask = 0, mlen = 0, LIsLValue;
    Type *ftype, *feltype;
    unary *result = NULL;

    mask = GetSwizzleOrWriteMask(loc, ident, &LIsLValue, &mlen);
    ftype = fExpr->common.type;
    if (IsScalar(ftype)) {
        feltype = ftype;
        maxi = 0;
    } else if (IsVector(ftype, &len)) {
        feltype = ftype->arr.eltype;
        maxi = len - 1;
        if (len > 4) {
            SemanticError(loc, ERROR_S_VECTOR_OPERAND_GR_4, ".");
            HasError = 1;
        }
    } else {
        SemanticError(loc, ERROR_S_OPERANDS_NOT_SCALAR_VECTOR, ".");
        HasError = 1;
    }
    if (!HasError) {
        base = GetBase(feltype);
        tmask = mask;
        for (ii = 0; ii < mlen; ii++) {
            if ((tmask & 0x3) > maxi) {
                SemanticError(loc, ERROR_S_SWIZZLE_MASK_EL_MISSING,
                              GetAtomString(atable, ident));
                HasError = 1;
                break;
            }
            tmask >>= 2;
        }
        if (!HasError) {
            if (mlen == 1)
                mlen = 0; // I.e. scalar, not array[1]
            result = NewUnopSubNode(SWIZZLE_Z_OP, SUBOP_Z(mask, mlen, len, base), fExpr);
            result->type = GetStandardType(base, mlen, 0);
            result->IsLValue = LIsLValue & fExpr->common.IsLValue;
            result->IsConst = result->IsLValue & fExpr->common.IsConst;
        }
    }
    if (!result) {
        result = NewUnopSubNode(SWIZZLE_Z_OP, 0, fExpr);
        result->type = UndefinedType;
    }
    return (expr *) result;
} // NewSwizzleOperator

/*
 * NewMatrixSwizzleOperator() - See if this is a valid matrix swizzle operation.  Return a new
 *         node with the proper operator description.
 */

expr *NewMatrixSwizzleOperator(SourceLoc *loc, expr *fExpr, int ident)
{
    int HasError = 0, len = 0, len2 = 0, ii, maxi, base, tmask, mask = 0, mlen = 0, LIsLValue;
    Type *ftype, *feltype;
    unary *result = NULL;

    mask = GetMatrixSwizzleOrWriteMask(loc, ident, &LIsLValue, &mlen);
    ftype = fExpr->common.type;
    if (IsMatrix(ftype, &len, &len2)) {
        feltype = ftype->arr.eltype;
        maxi = len - 1;
        if (len > 4 || len2 > 4) {
            SemanticError(loc, ERROR_S_MATRIX_OPERAND_GR_4, ".");
            HasError = 1;
        }
    } else {
        SemanticError(loc, ERROR_S_OPERANDS_NOT_MATRIX, ".");
        HasError = 1;
    }
    if (!HasError) {
        base = GetBase(feltype);
        tmask = mask;
        for (ii = 0; ii < mlen; ii++) {
            if ((tmask & 0x3) >= len || ((tmask >> 2) & 0x3) >= len2) {
                SemanticError(loc, ERROR_S_SWIZZLE_MASK_EL_MISSING,
                              GetAtomString(atable, ident));
                HasError = 1;
                break;
            }
            tmask >>= 4;
        }
        if (!HasError) {
            if (mlen == 1)
                mlen = 0; // I.e. scalar, not array[1]
            result = NewUnopSubNode(SWIZMAT_Z_OP, SUBOP_ZM(mask, mlen, len2, len, base), fExpr);
            result->type = GetStandardType(base, mlen, 0);
            result->IsLValue = LIsLValue & fExpr->common.IsLValue;
            result->IsConst = result->IsLValue & fExpr->common.IsConst;
        }
    }
    if (!result) {
        result = NewUnopSubNode(SWIZMAT_Z_OP, 0, fExpr);
        result->type = UndefinedType;
    }
    return (expr *) result;
} // NewMatrixSwizzleOperator

/*
 * NewVectorConstructor() - Construct a vector of length 1 to 4 from the expressions in fExpr.
 *
 */

expr *NewVectorConstructor(SourceLoc *loc, Type *fType, expr *fExpr)
{
    int len = 0, HasError = 0, size = 0, lNumeric, nNumeric, vlen, vlen2;
    int IsMatrixConstructor = 0;
    int MatrixRowSize = 0;
    CgScalarKind lkind, nkind = CG_SCALAR_NONE;
    unary *result = NULL;
    expr *lExpr;
    Type *lType, *rType;

    if (fType) {
        rType = fType;
        if (IsScalar(fType) || IsSampler(fType, NULL)) {
            /* A scalar type applied to one scalar argument is a cast,
             * not a length-one vector construction: */
            if (fExpr && fExpr->common.kind == BINARY_N &&
                fExpr->bin.op == EXPR_LIST_OP &&
                fExpr->bin.right == NULL)
            {
                lkind = GetScalarKind(fExpr->bin.left->common.type);
                if ((lIsNumericKind(lkind) || lkind == CG_SCALAR_BOOL) &&
                    IsScalar(fExpr->bin.left->common.type))
                {
                    return NewCastOperator(loc, fExpr->bin.left, rType);
                }
            }
            size = 1;
        } else if (IsVector(fType, &vlen)) {
            size = vlen;
        } else if (IsMatrix(fType, &vlen, &vlen2)) {
            size = vlen*vlen2;
            if (Cg->theHAL->GetCapsBit(CAPS_MATRIX_CONSTRUCTOR_AST)) {
                IsMatrixConstructor = 1;
                MatrixRowSize = vlen;
            }
        } else {
            SemanticError(loc, ERROR___INVALID_TYPE_FUNCTION);
            rType = UndefinedType;
        }
    } else {
        rType = UndefinedType;
    }
    lExpr = fExpr;
    while (lExpr) {
        vlen = 0;
        lType = lExpr->common.type;
        lkind = GetScalarKind(lType);
        lNumeric = lIsNumericKind(lkind);
        if (!lNumeric && lkind != CG_SCALAR_BOOL) {
            SemanticError(loc, ERROR___VECTOR_CONSTR_NOT_NUM_BOOL);
            HasError = 1;
            break;
        }
        if (IsScalar(lType)) {
            vlen = 1;
        } else if (IsMatrixConstructor && IsVector(lType, &vlen)) {
            /* Matrix rows may combine scalars and vectors. */
        } else {
            SemanticError(loc, ERROR___VECTOR_CONSTR_NOT_SCALAR);
            HasError = 1;
            break;
        }
        if (IsMatrixConstructor) {
            if (vlen > size - len ||
                vlen > MatrixRowSize - len % MatrixRowSize)
            {
                SemanticError(loc, ERROR___TOO_MUCH_DATA_TYPE_FUN);
                HasError = 1;
                break;
            }
        }
        if (len == 0) {
            nkind = lkind;
            nNumeric = lNumeric;
        } else if (IsMatrixConstructor || len + vlen <= 4) {
            if (lNumeric == nNumeric) {
                if (nNumeric) {
                    nkind = GetScalarKind(CgUsualArithmeticType(
                        GetStandardTypeKind(nkind, 0, 0),
                        GetStandardTypeKind(lkind, 0, 0)));
                }
            } else {
                SemanticError(loc, ERROR___MIXED_NUM_NONNUM_VECT_CNSTR);
                HasError = 1;
                break;
            }
        } else {
            SemanticError(loc, ERROR___CONSTRUCTER_VECTOR_LEN_GR_4);
            HasError = 1;
            break;
        }
        len += vlen;
        lExpr = lExpr->bin.right;
    }
    if (size && !HasError) {
        if (size > len) {
            SemanticError(loc, ERROR___TOO_LITTLE_DATA_TYPE_FUN);
            HasError = 1;
        } else if (size < len) {
            SemanticError(loc, ERROR___TOO_MUCH_DATA_TYPE_FUN);
            HasError = 1;
        }
    }
    if (!HasError) {
        lExpr = fExpr;
        while (lExpr) {
            lType = lExpr->common.type;
            lkind = GetScalarKind(lType);
            if (lkind != nkind) {
                vlen = 0;
                IsVector(lType, &vlen);
                lExpr->bin.left = CastScalarVectorMatrix(
                    lExpr->bin.left, lkind, nkind, vlen, 0);
            }
            lExpr = lExpr->bin.right;
        }
        /* VECTOR_V_OP has no room for a 16-component matrix length. */
        result = NewUnopSubNode(VECTOR_V_OP,
            SUBOP_V(IsMatrixConstructor ? 0 : len, CgScalarLegacyBase(nkind)), fExpr);
        result->type = IsMatrixConstructor ? rType :
            GetStandardTypeKind(nkind, len, 0);
    }
    if (!result) {
        result = NewUnopSubNode(VECTOR_V_OP, 0, fExpr);
        result->type = rType;
    }
    return (expr *) result;
} // NewVectorConstructor

/*
 * NewCastOperator() - Type cast "fExpr" to "ftype" if possible.
 *
 */

expr *NewCastOperator(SourceLoc *loc, expr *fExpr, Type *toType)
{
    expr *lExpr;

    if (CgTypeIsPoison(fExpr->common.type) || CgTypeIsPoison(toType))
        return fExpr;
    if (ConvertType(loc, fExpr, toType, fExpr->common.type, &lExpr, 0, 1, 1)) {
        lExpr->common.type = toType;
        return lExpr;
    } else {
        SemanticError(loc, ERROR___INVALID_CAST);
        return fExpr;
    }
} // NewCastOperator

/*
 * lNewArrayLengthOperator() - Build the ".length" query on an array.
 *
 * Sized arrays fold to a compile-time int constant; unsized arrays keep
 * an explicit ARRAY_LENGTH_OP node of canonical int type so the length
 * stays a runtime value.  Attribute arrays never demand a source
 * extent: their canonical source type is always unresolved (extent 0),
 * so they always build the explicit node and CgGeometryResolveLengths
 * folds it to the selected topology's vertex count after resolution.
 *
 */

static expr *lNewArrayLengthOperator(SourceLoc *loc, expr *fExpr)
{
    Type *ftype = fExpr->common.type;
    constant *cexpr;
    unary *result;
    CgNumericValue value;

    if (ftype && GetCategory(ftype) == TYPE_CATEGORY_ARRAY &&
        ftype->arr.numels != CG_ARRAY_UNSIZED)
    {
        CgNumericSetSigned(&value, CG_SCALAR_CINT, ftype->arr.numels);
        cexpr = NewNumericConstNode(ICONST_OP, &value);
        return (expr *) cexpr;
    }
    result = NewUnopSubNode(ARRAY_LENGTH_OP, 0, fExpr);
    result->type = GetStandardTypeKind(CG_SCALAR_INT, 0, 0);
    result->targetType = NULL;
    result->IsLValue = 0;
    result->IsConst = 0;
    result->HasSideEffects = fExpr->common.HasSideEffects;
    return (expr *) result;
} // lNewArrayLengthOperator

/*
 * NewMemberSelectorOrSwizzleOrWriteMaskOperator() - Construct either a struct member
 *         operator,or a swizzle operator, or a writemask operator, depending upon the
 *         type of the expression "fExpr".   I think I'm gonna barf.
 */

expr *NewMemberSelectorOrSwizzleOrWriteMaskOperator(SourceLoc *loc, expr *fExpr, int ident)
{
    Type *lType = fExpr->common.type;
    int len, len2;
    expr *lExpr, *mExpr;
    Symbol *lSymb;
    static int lengthAtom = 0;

    /* ".length" on an array is a typed length query, detected before any
     * structure lookup; arrays are never structs, so member access on
     * structures is unaffected.  Attribute arrays take the same typed
     * query without demanding a source extent. */
    if (!lengthAtom)
        lengthAtom = LookUpAddString(atable, "length");
    if (ident == lengthAtom && (IsArray(lType) || CgIsAttribArray(lType))) {
        return lNewArrayLengthOperator(loc, fExpr);
    }
    if (IsCategory(lType, TYPE_CATEGORY_STRUCT)) {
        lSymb = LookUpLocalSymbol(lType->str.members, ident);
        if (lSymb) {
            mExpr = (expr *) NewSymbNode(MEMBER_OP, lSymb);
            lExpr = (expr *) NewBinopNode(MEMBER_SELECTOR_OP, fExpr, mExpr);
            if (IsFunction(lSymb)) {
                /* Method selection keeps both the receiver expression and
                 * the selected method symbol for the call operator; a
                 * method by itself is not an l-value. */
                lExpr->common.IsLValue = 0;
            } else {
                lExpr->common.IsLValue = fExpr->common.IsLValue;
                lExpr->common.IsConst = fExpr->common.IsConst;
            }
            lExpr->common.type = lSymb->type;
        } else {
            SemanticError(loc, ERROR_SS_NOT_A_MEMBER,
                          GetAtomString(atable, ident), GetAtomString(atable, lType->str.tag));
            lExpr = fExpr;
        }
    } else if (IsCategory(lType, TYPE_CATEGORY_INTERFACE)) {
        /* Interfaces carry methods only: data members are rejected when
         * the interface body is completed. */
        lSymb = LookUpLocalSymbol(lType->iface.members, ident);
        if (lSymb && IsFunction(lSymb)) {
            mExpr = (expr *) NewSymbNode(MEMBER_OP, lSymb);
            lExpr = (expr *) NewBinopNode(MEMBER_SELECTOR_OP, fExpr, mExpr);
            lExpr->common.IsLValue = 0;
            lExpr->common.type = lSymb->type;
        } else {
            SemanticError(loc, ERROR_SS_NOT_A_MEMBER,
                          GetAtomString(atable, ident), GetAtomString(atable, lType->iface.tag));
            lExpr = fExpr;
        }
    } else if (IsScalar(lType) || IsVector(lType, &len)) {
        lExpr = NewSwizzleOperator(loc, fExpr, ident);
    } else if (IsMatrix(lType, &len, &len2)) {
        lExpr = NewMatrixSwizzleOperator(loc, fExpr, ident);
    } else {
        SemanticError(loc, ERROR_S_LEFT_EXPR_NOT_STRUCT_ARRAY, GetAtomString(atable, ident));
        lExpr = fExpr;
    }
    return lExpr;
} // NewMemberSelectorOrSwizzleOrWriteMaskOperator

/*
 * NewIndexOperator() - Construct an array index operator.
 *
 */

expr *NewIndexOperator(SourceLoc *loc, expr *fExpr, expr *ixexpr)
{
    expr *lExpr;

    if (IsCategory(fExpr->common.type, TYPE_CATEGORY_ARRAY)) {
        lExpr = (expr *) NewBinopNode(ARRAY_INDEX_OP, fExpr, ixexpr);
        lExpr->common.IsLValue = fExpr->common.IsLValue;
        lExpr->common.IsConst = fExpr->common.IsConst;
        lExpr->common.type = GetElementType(fExpr->common.type);
    } else if (CgIsAttribArray(fExpr->common.type)) {
        /* Indexing an attribute array reads one element of the
         * read-only input: the element type results, but the result
         * is never an l-value, so element writes fail below. */
        lExpr = (expr *) NewBinopNode(ARRAY_INDEX_OP, fExpr, ixexpr);
        lExpr->common.IsLValue = 0;
        lExpr->common.IsConst = fExpr->common.IsConst;
        lExpr->common.type = CgAttribArrayElement(fExpr->common.type);
    } else {
        SemanticError(loc, ERROR___INDEX_OF_NON_ARRAY);
        lExpr = fExpr;
    }
    return lExpr;
} // NewIndexOperator

/*
 * lChainHasDefaults() - TRUE when any overload in the chain declares a
 *         parameter default, so even a lone candidate must go through
 *         resolution to have its defaults appended.
 */

static int lChainHasDefaults(Symbol *fSymb)
{
    Symbol *formal;

    for (; fSymb; fSymb = fSymb->details.fun.overload) {
        for (formal = fSymb->details.fun.params; formal;
             formal = formal->next)
        {
            if (formal->details.var.init) {
                return 1;
            }
        }
    }
    return 0;
} // lChainHasDefaults

/*
 * lAppendDefaultArguments() - Clone the converted defaults of the last
 *         "count" formals onto the call's actual list so every later
 *         pass sees an ordinary, complete call.
 */

static expr *lAppendDefaultArguments(SourceLoc *loc, Symbol *fSymb,
                                     expr *actuals, int count)
{
    Symbol *formal;
    int total = 0;
    int skip;

    if (count <= 0) {
        return actuals;
    }
    for (formal = fSymb->details.fun.params; formal; formal = formal->next) {
        total++;
    }
    skip = total - count;
    for (formal = fSymb->details.fun.params; formal && skip < total;
         formal = formal->next)
    {
        if (skip > 0) {
            skip--;
            continue;
        }
        if (formal->details.var.init) {
            expr *stored = formal->details.var.init;
            expr *value = stored;

            /* Stored defaults keep the initializer EXPR_LIST wrapper;
             * argument lists carry the bare value expression. */
            if (value->common.kind == BINARY_N &&
                value->bin.op == EXPR_LIST_OP && !value->bin.right)
            {
                value = value->bin.left;
            }
            actuals = ArgumentList(loc, actuals, DupExpr(value));
        }
    }
    return actuals;
} // lAppendDefaultArguments

/*
 * lFormatOverloadCandidate() - Render one candidate signature into
 *         "out" as "rettype name(param, ...)", mirroring the type
 *         formatting the tree dumps use.
 */

static void lFormatOverloadCandidate(Symbol *fSymb, char *out, int size)
{
    char tname[128], uname[128];
    Symbol *param;

    FormatTypeString(tname, sizeof tname, uname, sizeof uname,
                     fSymb->type->fun.rettype);
    strncpy(out, tname, size - 1);
    out[size - 1] = '\0';
    strncat(out, uname, size - strlen(out) - 1);
    strncat(out, " ", size - strlen(out) - 1);
    strncat(out, GetAtomString(atable, fSymb->name),
            size - strlen(out) - 1);
    strncat(out, "(", size - strlen(out) - 1);
    for (param = fSymb->details.fun.params; param; param = param->next) {
        FormatTypeString(tname, sizeof tname, uname, sizeof uname,
                         param->type);
        strncat(out, tname, size - strlen(out) - 1);
        strncat(out, uname, size - strlen(out) - 1);
        if (param->next)
            strncat(out, ", ", size - strlen(out) - 1);
    }
    strncat(out, ")", size - strlen(out) - 1);
} // lFormatOverloadCandidate

/*
 * lResolveOverloadedFunction() - Resolve an overloaded function call
 *         through CgResolveOverload.  All candidate state and ranking
 *         live inside the non-mutating resolver; this wrapper only
 *         reports failures like the legacy path did and clones stored
 *         default arguments into complete calls (*fActuals is updated
 *         when defaults are appended).
 */

static Symbol *lResolveOverloadedFunction(SourceLoc *loc, Symbol *fSymb,
                                          expr **fActuals)
{
    CgOverloadResult lResult;
    Symbol *lSymb;
    int numvalid = 0;
    char candidate[512];

    if (!CgResolveOverload(&Cg->theHAL->profileIdentity, fSymb, *fActuals,
                           &lResult))
    {
        if (lResult.ambiguous) {
            SemanticError(loc, ERROR_S_AMBIGUOUS_FUN_REFERENCE,
                          GetAtomString(atable, fSymb->name));
            /* Layered notes carry each candidate through the scanner
             * diagnostic channel instead of raw stdout writes. */
            lSymb = fSymb;
            while (lSymb) {
                lFormatOverloadCandidate(lSymb, candidate,
                                         sizeof candidate);
                SemanticNote(loc, NOTICE_S_OVERLOAD_CANDIDATE,
                             ++numvalid, candidate);
                lSymb = lSymb->details.fun.overload;
            }
            return fSymb;
        }
        /* No viable overload: fall back to ordinary argument binding
         * against the first declaration so the call reports its one
         * precise diagnostic (arity, parameter type, ...) exactly like
         * a lone prototype would. */
        return fSymb;
    }
    lSymb = lResult.symbol;
    if (lResult.usedDefaults > 0) {
        *fActuals = lAppendDefaultArguments(loc, lSymb, *fActuals,
                                            lResult.usedDefaults);
    }
    return lSymb;
} // lResolveOverloadedFunction

/*
 * lNewMethodCallActuals() - Prepend the implicit receiver to a method
 *         call's declared actuals.  The receiver is an internal argument
 *         only; it is never exposed as a source-level formal.
 */

static expr *lNewMethodCallActuals(SourceLoc *loc, expr *fReceiver, expr *fActuals)
{
    expr *head, *tail;

    head = ArgumentList(loc, NULL, fReceiver);
    tail = head;
    while (tail->bin.right)
        tail = tail->bin.right;
    tail->bin.right = fActuals;
    return head;
} // lNewMethodCallActuals

/*
 * lIsMethodSelection() - TRUE when a function-call callee is a member
 *         selection naming a method symbol rather than an ordinary data
 *         member or plain function reference.
 */

static int lIsMethodSelection(const expr *fExpr)
{
    return fExpr != NULL &&
           fExpr->common.kind == BINARY_N &&
           fExpr->bin.op == MEMBER_SELECTOR_OP &&
           fExpr->bin.right != NULL &&
           fExpr->bin.right->common.kind == SYMB_N &&
           fExpr->bin.right->sym.op == MEMBER_OP &&
           fExpr->bin.right->sym.symbol != NULL &&
           IsFunction(fExpr->bin.right->sym.symbol) &&
           fExpr->bin.right->sym.symbol->details.fun.isMethod;
} // lIsMethodSelection

/*
 * lNewMethodCallOperator() - Build a method call.  The implicit receiver
 *         becomes an internal first argument and ordinary argument
 *         conversion applies to every slot including the receiver.  A
 *         call through an interface receiver becomes INTERFACE_CALL_OP:
 *         no implementing function is known at compile time, so dispatch
 *         stays symbolic while the node preserves the interface's
 *         declared result type.  A struct receiver produces an ordinary
 *         direct call to the implementing method.
 */

static expr *lNewMethodCallOperator(SourceLoc *loc, expr *selection, expr *actuals)
{
    Symbol *method;
    expr *receiver, *funExpr, *result;

    method = selection->bin.right->sym.symbol;
    receiver = selection->bin.left;
    funExpr = (expr *) NewSymbNode(VARIABLE_OP, method);
    result = NewFunctionCallOperator(loc, funExpr,
                                     lNewMethodCallActuals(loc, receiver,
                                                           actuals));
    if (result->common.kind == BINARY_N &&
        result->bin.op == FUN_CALL_OP &&
        IsCategory(receiver->common.type, TYPE_CATEGORY_INTERFACE))
    {
        /* Keep the full selection expression so dumps show both the
         * receiver and the selected interface method. */
        result->bin.op = INTERFACE_CALL_OP;
        result->bin.left = selection;
    }
    return result;
} // lNewMethodCallOperator

/*
 * lUnwrapPlainGeometryArguments() - Replace every unannotated geometry
 *          argument wrapper in a FUN_ARG_OP chain with its wrapped
 *          value, in place.  Ordinary call resolution and every later
 *          pass keep seeing exactly the argument trees they always
 *          saw; only annotated arguments stay wrapped until the call's
 *         selected function decides whether annotations are legal.
 */

static void lUnwrapPlainGeometryArguments(expr *fActuals)
{
    expr *link;

    for (link = fActuals; link != NULL && link->common.kind == BINARY_N &&
                          link->bin.op == FUN_ARG_OP;
         link = link->bin.right)
    {
        if (IsGeometryArgument(link->bin.left) &&
            GetGeometryArgumentSemantic(link->bin.left) == 0)
        {
            link->bin.left = GetGeometryArgumentValue(link->bin.left);
        }
    }
} // lUnwrapPlainGeometryArguments

/*
 * lRejectStrayGeometryArguments() - Diagnose and unwrap annotated
 *          geometry arguments bound to any callee without the geometry
 *          special flag: inline binding semantics exist only for the
 *          geometry operations.
 */

static void lRejectStrayGeometryArguments(SourceLoc *loc, expr *fActuals,
                                          const char *callee)
{
    expr *link;

    for (link = fActuals; link != NULL && link->common.kind == BINARY_N &&
                          link->bin.op == FUN_ARG_OP;
         link = link->bin.right)
    {
        if (IsGeometryArgument(link->bin.left)) {
            SemanticError(loc,
                          ERROR_SS_GEOMETRY_ARGUMENT_ANNOTATION,
                          GetAtomString(atable,
                              GetGeometryArgumentSemantic(
                                  link->bin.left)),
                          callee);
            link->bin.left = GetGeometryArgumentValue(link->bin.left);
        }
    }
} // lRejectStrayGeometryArguments

/*
 * NewFunctionCallOperator() - Construct a function call node.  Check types of parameters,
 *         resolve overloaded function, etc.
 *
 */

expr *NewFunctionCallOperator(SourceLoc *loc, expr *funExpr, expr *actuals)
{
    binary *result = NULL;
    Type *funType, *formalType, *actualType;
    TypeList *lFormals;
    expr *lExpr, *lActuals;
    Symbol *lSymb;
    CgSamplerKind formalKind, actualKind;
    int paramno, inout;
    int lop, lsubop = FUN_CALL_OP;

    if (lIsMethodSelection(funExpr))
        return lNewMethodCallOperator(loc, funExpr, actuals);

    lUnwrapPlainGeometryArguments(actuals);

    funType = funExpr->common.type;
    if (IsCategory(funType, TYPE_CATEGORY_FUNCTION)) {
        lop = FUN_CALL_OP;
        lsubop = 0;
        if (funExpr->common.kind == SYMB_N) {
            lSymb = funExpr->sym.symbol;
            if (lSymb->kind == FUNCTION_S) {
                /* Resolve whenever there is a real overload set or any
                 * candidate declares parameter defaults; lone
                 * default-free functions keep the direct binding path
                 * and its diagnostics. */
                if (lSymb->details.fun.overload ||
                    lChainHasDefaults(lSymb))
                {
                    lSymb = lResolveOverloadedFunction(loc, lSymb,
                                                       &actuals);
                    funExpr->sym.symbol = lSymb;
                    funType = funExpr->common.type = lSymb->type;
                }
                if (funType->properties & TYPE_MISC_INTERNAL) {
                    /* An intrinsic call carries its identity through
                     * the selected symbol's immutable catalog
                     * signature; a body attached by a portable
                     * stdlib definition turns the call back into an
                     * ordinary inlined call.  No packed group/index
                     * encoding exists anymore. */
                    if (lSymb->details.fun.statements == NULL)
                        lop = FUN_INTRINSIC_OP;
                }
            } else {
                InternalError(loc, ERROR_S_SYMBOL_NOT_FUNCTION, GetAtomString(atable, lSymb->name));
            }
        }
        if (lop == FUN_INTRINSIC_OP &&
            lSymb != NULL &&
            lSymb->details.fun.intrinsic != NULL &&
            CgIntrinsicIsGeometrySpecial(
                lSymb->details.fun.intrinsic->intrinsic))
        {
            /* A selected geometry operation owns its argument syntax:
             * annotated arguments stay wrapped for the statement
             * classifier, and arity, types, and placement are checked
             * there -- never against the placeholder empty parameter
             * list. */
            result = NewBinopSubNode(FUN_CALL_OP, 0, funExpr, actuals);
            result->IsLValue = 0;
            result->IsConst = 0;
            result->HasSideEffects = 1;
            result->type = VoidType;
            return (expr *) result;
        }
        {
            const char *lCallee = "";

            if (funExpr->common.kind == SYMB_N &&
                lSymb != NULL && lSymb->kind == FUNCTION_S)
            {
                lCallee = GetAtomString(atable, lSymb->name);
            }
            lRejectStrayGeometryArguments(loc, actuals, lCallee);
        }
        lFormals = funType->fun.paramtypes;
        lActuals = actuals;
        paramno = 0;
        while (lFormals && lActuals) {
            paramno++;
            formalType = lFormals->type;
            actualType = lActuals->common.type;
            inout = 0;
            if ((formalType->properties & TYPE_QUALIFIER_IN) ||
                !(formalType->properties & TYPE_QUALIFIER_INOUT))
                inout |= 1;
            if (formalType->properties & TYPE_QUALIFIER_OUT) {
                inout |= 2;
                if (IsSampler(formalType, NULL)) {
                    /* Samplers are read-only interface values: only in
                     * parameter passing copies them. */
                    lExpr = lActuals->bin.left;
                    SemanticError(loc, ERROR_S_SAMPLER_OUT_PARAM,
                        lExpr && lExpr->common.kind == SYMB_N &&
                        lExpr->sym.op == VARIABLE_OP && lExpr->sym.symbol ?
                        GetAtomString(atable, lExpr->sym.symbol->name) : "");
                }
                lExpr = lActuals->bin.left;
                if (lExpr) {
                    if (lExpr->common.IsLValue) {
                        if (!(GetQualifiers(actualType) & TYPE_QUALIFIER_CONST)) {
                            if (IsSameUnqualifiedType(formalType, actualType) &&
                                IsPacked(formalType) == IsPacked(actualType))
                            {
                                SUBOP_SET_MASK(lActuals->bin.subop, inout);
                            } else if (!CgTypeIsPoison(actualType) &&
                                       !CgTypeIsPoison(formalType)) {
                                SemanticError(loc, ERROR_D_OUT_PARAM_NOT_SAME_TYPE, paramno);
                            }
                        } else {
                            SemanticError(loc, ERROR_D_OUT_PARAM_IS_CONST, paramno);
                        }
                    } else {
                        SemanticError(loc, ERROR_D_OUT_PARAM_NOT_LVALUE, paramno);
                    }
                }
                /* Boundary ruling (Task 7, revisited by Task 12):
                 * argument binding keeps the exact-shape rules -- no
                 * scalar<->aggregate shape conversions here.  The
                 * ranked resolver in cg_overload.c applies the same
                 * exclusion to its candidates (lIsShapeConversion), so
                 * probing and binding agree; admitting replication
                 * candidates would surface new ambiguities across
                 * heavily overloaded names while single-candidate
                 * calls still reject them at binding time. */
            } else if (ConvertType(loc, lActuals->bin.left, formalType, actualType, &lExpr, 0, 0, 0)) {
                lActuals->bin.left = lExpr;
                SUBOP_SET_MASK(lActuals->bin.subop, inout);
            } else if (IsSampler(formalType, &formalKind) &&
                       IsSampler(actualType, &actualKind) &&
                       CgSamplerCompatible(formalKind, actualKind))
            {
                /* Any specific sampler binds directly to a deprecated
                 * base-sampler formal; incompatible specific kinds fall
                 * through to the error below. */
                SUBOP_SET_MASK(lActuals->bin.subop, inout);
            } else if (!CgTypeIsPoison(actualType) &&
                       !CgTypeIsPoison(formalType)) {
                SemanticError(loc, ERROR_D_INCOMPATIBLE_PARAMETER, paramno);
            }
            lFormals = lFormals->next;
            lActuals = lActuals->bin.right;
        }
        if (lFormals) {
            if (!IsVoid(lFormals->type))
                SemanticError(loc, ERROR___TOO_FEW_PARAMS);
        } else if (lActuals) {
            SemanticError(loc, ERROR___TOO_MANY_PARAMS);
        }
        result = NewBinopSubNode(lop, lsubop, funExpr, actuals);
        result->IsLValue = 0;
        result->IsConst = 0;
        result->HasSideEffects |= lSymb->details.fun.HasOutParams;
        result->type = funExpr->common.type->fun.rettype;
    } else {
        SemanticError(loc, ERROR___CALL_OF_NON_FUNCTION);
    }
    if (!result) {
        result = NewBinopNode(FUN_CALL_OP, funExpr, actuals);
        result->type = UndefinedType;
    }
    return (expr *) result;
} // NewFunctionCallOperator

/*
 * NewSimpleAssignment() - Build a new simple assignment expression.
 *
 */

/*
 * lTargetsAttribArray() - TRUE when the target expression reads
 *         through an attribute array: the array itself, an indexed
 *         element, or a member of either.  Whole-array and element
 *         writes are both rejected as read-only uses.
 */

static int lTargetsAttribArray(const expr *fExpr)
{
    if (!fExpr) {
        return 0;
    }
    if (CgIsAttribArray(fExpr->common.type)) {
        return 1;
    }
    if (fExpr->common.kind == BINARY_N &&
        (fExpr->bin.op == ARRAY_INDEX_OP ||
         fExpr->bin.op == MEMBER_SELECTOR_OP))
    {
        return lTargetsAttribArray(fExpr->bin.left);
    }
    return 0;
} // lTargetsAttribArray

/*
 * NewSimpleAssignment() - Build a new simple assignment expression.
 *
 */

expr *NewSimpleAssignment(SourceLoc *loc, expr *fVar, expr *fExpr, int InInit)
{
    int lop, subop, base, len, vqualifiers, vdomain, edomain;
    Type *vType, *eType;
    expr *lExpr;

    vType = fVar->common.type;
    eType = fExpr->common.type;
    vqualifiers = GetQualifiers(vType);
    vdomain = GetDomain(vType);
    edomain = GetDomain(eType);
    if (lTargetsAttribArray(fVar)) {
        /* Attribute arrays are read-only geometry inputs: whole-array
         * assignment and element stores both fail with the reserved
         * read-only diagnostic instead of the generic l-value rule. */
        SemanticError(loc, ERROR___GEOMETRY_ATTRIB_READ_ONLY);
    } else if (!fVar->common.IsLValue) {
        SemanticError(loc, ERROR___ASSIGN_TO_NON_LVALUE);
    }
    //if ((vqualifiers & TYPE_QUALIFIER_CONST) && !InInit)
    if (fVar->common.IsConst && !InInit)
        SemanticError(loc, ERROR___ASSIGN_TO_CONST_VALUE);
    if (IsSampler(vType, NULL) || IsSampler(eType, NULL))
    {
        /* Samplers are opaque language types: they may only be copied
         * through parameter passing, never assigned or initialized. */
        SemanticError(loc, ERROR___SAMPLER_ASSIGNMENT);
    }
    if (vdomain == TYPE_DOMAIN_UNIFORM && edomain == TYPE_DOMAIN_VARYING)
        SemanticError(loc, ERROR___ASSIGN_VARYING_TO_UNIFORM);
    if (IsArray(vType) && IsUnsizedArray(vType) && IsArray(eType)) {
        /* Dynamic-array assignment: the destination keeps its declared
         * unsized canonical type object; the node carries the runtime
         * shape of the source.  The element shapes must agree, and
         * packedness must agree at every nesting layer. */
        if (IsPacked(vType) == IsPacked(eType) &&
            IsSameUnqualifiedType(vType->arr.eltype, eType->arr.eltype))
        {
            lExpr = (expr *) NewBinopSubNode(ASSIGN_DYN_OP,
                                             SUBOP__(GetBase(vType)),
                                             fVar, fExpr);
            lExpr->common.type = IsUnsizedArray(eType) ? vType : eType;
            return lExpr;
        }
        SemanticError(loc, ERROR___ASSIGN_INCOMPATIBLE_TYPES);
    } else if (ConvertType(loc, fExpr, vType, eType, &lExpr, InInit, 0, 1)) {
        fExpr = lExpr;
    } else {
        if (!CgTypeIsPoison(vType) && !CgTypeIsPoison(eType))
            SemanticError(loc, ERROR___ASSIGN_INCOMPATIBLE_TYPES);
    }
    base = GetBase(vType);
    if (IsScalar(vType)) {
        lop = ASSIGN_OP;
        subop = SUBOP__(base);
    } else if (IsVector(vType, &len)) {
        lop = ASSIGN_V_OP;
        subop = SUBOP_V(len, base);
    } else {
        lop = ASSIGN_GEN_OP;
        subop = SUBOP__(base);
    }
    lExpr = (expr *) NewBinopSubNode(lop, subop, fVar, fExpr);
    lExpr->common.type = vType;
    return lExpr;
} // NewSimpleAssignment

/*
 * NewSimpleAssignmentStmt() - Build a new simple assignment statement.
 *
 */

stmt *NewSimpleAssignmentStmt(SourceLoc *loc, expr *fVar, expr *fExpr, int InInit)
{
    return (stmt *) NewExprStmt(loc, NewSimpleAssignment(loc, fVar, fExpr, InInit));
} // NewSimpleAssignmentStmt

/*
 * NewCompoundAssignment() - Build a new simple assignment statement.
 *
 */

stmt *NewCompoundAssignmentStmt(SourceLoc *loc, opcode op, expr *fVar, expr *fExpr)
{
    return (stmt *) NewExprStmt(loc, (expr *) NewBinopNode(op, fVar, fExpr));
} // NewCompoundAssignment

#if 000
/*
 * NewMaskedAssignment() - Add a new masked assignment to an assignment statement.
 *
 * "fStmt" is an assignment stmt of the form: "exp" or "var = exp" ...
 * Returns a stmt of the form: "var@@mask = exp" or "var@@mask = (var = exp)" ...
 *
 */

stmt *NewMaskedAssignment(SourceLoc *loc, int mask, expr *fExpr, stmt *fStmt)
{
    int lop, subop, base, len;
    expr *lExpr;
    Type *lType;
    char str[5];
    int ii, kk;

    if (!fExpr->common.IsLValue)
        SemanticError(loc, ERROR___ASSIGN_TO_NON_LVALUE);
    if (ConvertType(fStmt->exprst.exp, fExpr->common.type, fStmt->exprst.exp->common.type,
                    &lExpr, 0)) {
        fStmt->exprst.exp = lExpr;
    } else {
        SemanticError(loc, ERROR___ASSIGN_INCOMPATIBLE_TYPES);
    }
    lType = fExpr->common.type;
    base = GetBase(lType);
    if (IsScalar(lType)) {
        SemanticError(loc, ERROR___MASKED_ASSIGN_TO_VAR);
        lop = ASSIGN_OP;
        subop = SUBOP__(base);
    } else if (IsVector(lType, &len)) {
        if (len > 4) {
            SemanticError(loc, ERROR_S_VECTOR_OPERAND_GR_4, "@@");
            len = 4;
        }
        if ((~0 << len) & mask) {
            kk = 0;
            for (ii = len; ii < 4; ii++) {
                if (mask & (1 << ii))
                    str[kk++] = "xyzw"[ii];
            }
            str[kk] = '\0';
            SemanticError(loc, ERROR_S_MASKED_ASSIGN_NON_EXISTENT, str);
            mask &= (1 << len) - 1;
        }
        lop = ASSIGN_MASKED_KV_OP;
        subop = SUBOP_KV(mask, len, base);
    } else {
        SemanticError(loc, ERROR___MASKED_ASSIGN_TO_VAR);
        lop = ASSIGN_GEN_OP;
        subop = SUBOP__(base);
    }
    fStmt->exprst.exp = (expr *) NewBinopSubNode(lop, subop, fExpr, fStmt->exprst.exp);
    fStmt->exprst.exp->common.type = lType;
    return fStmt;
} // NewMaskedAssignment

/*
 * NewConditionalAssignment() - Add a new simple assignment to an assignment statement.
 *
 * "fStmt" is an assignment stmt of the form: "exp" or "var = exp" ...
 * Returns a stmt of the form: "var@@(cond) = exp" or "var@@(cond) = (var = exp)" ...
 *
 */

stmt *NewConditionalAssignment(SourceLoc *loc, expr *fcond, expr *fExpr, stmt *fStmt)
{
    int lop, subop, base, len, clen;
    expr *lExpr;
    Type *lType, *ctype;

    if (!fExpr->common.IsLValue)
        SemanticError(loc, ERROR___ASSIGN_TO_NON_LVALUE);
    if (ConvertType(fStmt->exprst.exp, fExpr->common.type, fStmt->exprst.exp->common.type,
                    &lExpr, 0)) {
        fStmt->exprst.exp = lExpr;
    } else {
        SemanticError(loc, ERROR___ASSIGN_INCOMPATIBLE_TYPES);
    }
    lType = fExpr->common.type;
    base = GetBase(lType);
    ctype = fcond->common.type;
    if (IsScalar(lType)) {
        if (!IsScalar(ctype))
            SemanticError(loc, ERROR___SCALAR_BOOL_EXPR_EXPECTED);
        lop = ASSIGN_COND_OP;
        subop = SUBOP__(base);
    } else if (IsVector(lType, &len)) {
        if (len > 4) {
            SemanticError(loc, ERROR_S_VECTOR_OPERAND_GR_4, "@@()");
            len = 4;
        }
        if (IsScalar(ctype)) {
            lop = ASSIGN_COND_SV_OP;
        } else {
            lop = ASSIGN_COND_V_OP;
            if (IsVector(ctype, &clen)) {
                if (clen != len)
                    SemanticError(loc, ERROR_S_VECTOR_OPERANDS_DIFF_LEN, "@@()");
            } else {
                SemanticError(loc, ERROR_S_INVALID_CONDITION_OPERAND);
            }
        }
        subop = SUBOP_V(len, base);
    } else {
        lop = ASSIGN_COND_GEN_OP;
        subop = SUBOP__(base);
    }
    fStmt->exprst.exp = (expr *) NewTriopSubNode(lop, subop, fExpr, fcond, fStmt->exprst.exp);
    fStmt->exprst.exp->common.type = lType;
    return fStmt;
} // NewConditionalAssignment
#endif

/*************************************** misc: ******************************************/

/* 
 * InitTempptr() -- initialize the tempptr fields of an expr node -- designed
 *   be passed as an argument to ApplyToXXXX.  Sets tempptr[0] to its arg1
 *   argument and clears the rest of tempptr to NULL
 */

void InitTempptr(expr *fExpr, void *arg1, int arg2)
{
    fExpr->common.tempptr[0] = arg1;
    memset(&fExpr->common.tempptr[1], 0,
           sizeof(fExpr->common.tempptr) - sizeof(fExpr->common.tempptr[0]));
} // InitTempptr

/********************************** Error checking: ******************************************/

ErrorLoc *ErrorLocsFirst = NULL;
ErrorLoc *ErrorLocsLast = NULL;
int ErrorPending = 0;

/*
 * RecordErrorPos() - Record the fact that an error token was seen at source location LOC.
 *
 * Error tokens should be encountered in source line order, so we just add them to the end of
 * the list of error locations.
 */

void RecordErrorPos(SourceLoc *loc)
{
    ErrorLoc *eloc = (ErrorLoc *) malloc(sizeof(ErrorLoc));

    eloc->loc = *loc;
    eloc->hit = 0;
    eloc->next = NULL;
    if (ErrorLocsFirst == NULL) {
        ErrorLocsFirst = eloc;
    } else {
        ErrorLocsLast->next = eloc;
    }
    ErrorLocsLast = eloc;

    // If an error has been generated during parsing before the error
    // token was seen, then we mark this error token as being hit and
    // clear the error pending flag.

    if (ErrorPending) {
        eloc->hit = 1;
        ErrorPending = 0;
    }
} // RecordErrorPos

/*
 * MarkErrorPosHit() - Upon seeing an error at LOC, mark the
 * corresponding error location as hit.
 */

void MarkErrorPosHit(SourceLoc *loc)
{
    ErrorLoc *eloc;
    ErrorLoc *match = NULL;

    for (eloc = ErrorLocsFirst; eloc != NULL; eloc = eloc->next) {
        if (loc->line <= eloc->loc.line) {
            match = eloc;
        } else {
            break;
        }
    }
    
    // If we found an error token location that comes after LOC, then
    // mark it as hit, otherwise, we haven't seen the error token yet,
    // so make a note that there's an error pending.

    if (match != NULL) {
        match->hit = 1;
    } else {
        ErrorPending = 1;
    }
} // MakeErrorPosHit

/*
 * CheckAllErrorsGenerated() - Verify that at least one error was
 * generated in the appropriate location for each error token that
 * appeared in the program.
 */

void CheckAllErrorsGenerated(void)
{
    ErrorLoc *eloc;

    // Turn off ErrorMode so we can generate real errors for the
    // absence of errors.
    Cg->options.ErrorMode = 0;

    for (eloc = ErrorLocsFirst; eloc != NULL; eloc = eloc->next) {
        if (eloc->hit == 0) {
            SemanticError (&(eloc->loc), ERROR___NO_ERROR);
        }
    }
} // CheckAllErrorsGenerated

///////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////// End of support.c //////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////


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
express or implied, are granted by NVIDIA herein including but not
limited to any patent rights that may be infringed by your derivative
works. No hardware is licensed hereunder.

THE NVIDIA SOFTWARE IS BEING PROVIDED ON AN "AS IS" BASIS, WITHOUT
WARRANTIES OR CONDITIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED,
INCLUDING WITHOUT LIMITATION, WARRANTIES OF TITLE,
NON-INFRINGEMENT, MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR
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
// arb_lower.c - Lower the normalized Cg AST into the private ARB vector IR.
//

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "slglobals.h"
#include "arb_ir.h"

typedef struct ArbLowerContext_Rec {
    ArbProgram *ir;
    const ArbProfileDesc *profile;
    Symbol *program;
    struct ArbStaticValue_Rec *staticValues;
    int dstSelValid;        // Target selection order recorded by LowerLValue
    int dstSelWidth;
    int dstSel[4];
} ArbLowerContext;

typedef struct ArbStaticValue_Rec {
    struct ArbStaticValue_Rec *next;
    Symbol *symbol;
    int value;
} ArbStaticValue;

static int LowerStatement(ArbLowerContext *ctx, stmt *statement);
static int LowerAssignment(ArbLowerContext *ctx, expr *left, expr *right,
                           int mask, const SourceLoc *loc);
static int LowerLValue(ArbLowerContext *ctx, expr *expression,
                       ArbOperand *operand, int *mask);
static int LowerExpression(ArbLowerContext *ctx, expr *expression,
                           ArbOperand *operand);
static int LowerConnectorMember(ArbLowerContext *ctx, expr *expression,
                                ArbOperand *operand);

/*
 * MaskFromType() - Return the destination mask covering the components of
 *         a scalar or vector type.
 */

static int MaskFromType(Type *fType)
{
    int len;

    if (IsVector(fType, &len)) {
        switch (len) {
        case 2: return ARB_MASK_X | ARB_MASK_Y;
        case 3: return ARB_MASK_X | ARB_MASK_Y | ARB_MASK_Z;
        default: return ARB_MASK_XYZW;
        }
    }
    return ARB_MASK_X;
} // MaskFromType

/*
 * GetSymbolTemp() - Map a plain local variable onto a virtual temporary,
 *         allocating it on first use.  The heap index cell is tracked in
 *         the context so backend-owned pointers never outlive lowering.
 */

static int GetSymbolTemp(ArbLowerContext *ctx, Symbol *symbol)
{
    int *index;
    if (symbol->tempptr)
        return *(int *) symbol->tempptr;
    index = (int *) malloc(sizeof(int));
    if (!index)
        return -1;
    *index = ArbNewTemp(ctx->ir);
    symbol->tempptr = index;
    return *index;
} // GetSymbolTemp

/*
 * ClearSymbolTemps() - Release every temp-index cell allocated through
 *         GetSymbolTemp and detach it from its symbol.  The compiler's
 *         ClearAllSymbolTempptr() also scrubs expression nodes that may
 *         have cached backend data.
 */

static void ClearSymbolTemps(ArbLowerContext *ctx)
{
    SymbolList *tracked = (SymbolList *) ctx->program->tempptr2;
    while (tracked) {
        SymbolList *next = tracked->next;
        if (tracked->symb && tracked->symb->tempptr) {
            free(tracked->symb->tempptr);
            tracked->symb->tempptr = NULL;
        }
        free(tracked);
        tracked = next;
    }
    ctx->program->tempptr2 = NULL;
    ClearAllSymbolTempptr();
} // ClearSymbolTemps

/*
 * TrackSymbolTemp() - Remember a symbol whose tempptr the backend owns.
 */

static void TrackSymbolTemp(ArbLowerContext *ctx, Symbol *symbol)
{
    SymbolList *node = (SymbolList *) malloc(sizeof(SymbolList));
    if (!node)
        return;
    node->symb = symbol;
    node->next = (SymbolList *) ctx->program->tempptr2;
    ctx->program->tempptr2 = node;
} // TrackSymbolTemp

/*
 * ResolveMemberBinding() - If a member-selector chain resolves to a bound
 *         connector member, fill in the operand and return nonzero.
 */

static int ResolveMemberBinding(Binding *fBind, ArbOperand *operand)
{
    if (!fBind || fBind->none.kind != BK_CONNECTOR ||
        !(fBind->none.properties & BIND_IS_BOUND))
    {
        return 0;
    }
    if (fBind->none.properties & BIND_INPUT) {
        *operand = ArbInputOperand(fBind->conn.regno);
    } else if (fBind->none.properties & BIND_OUTPUT) {
        *operand = ArbOutputOperand(fBind->conn.regno);
    } else {
        return 0;
    }
    operand->bindingName = fBind->conn.rname;
    return 1;
} // ResolveMemberBinding

/*
 * LowerConnectorMember() - Lower a MEMBER_SELECTOR_OP (or bare variable)
 *         that refers to a connector register, uniform, or local value.
 *         Returns 0 when the shape is unsupported.
 */

static int LowerConnectorMember(ArbLowerContext *ctx, expr *expression,
                                ArbOperand *operand)
{
    Symbol *member;
    Binding *fBind;
    Type *memberType;
    int len;

    if (expression->common.kind == SYMB_N &&
        expression->sym.op == VARIABLE_OP)
    {
        Symbol *symb = expression->sym.symbol;
        int temp;

        if (ResolveMemberBinding(symb->details.var.bind, operand))
            return 1;
        if (GetDomain(symb->type) & TYPE_DOMAIN_UNIFORM) {
            // Uniform parameters are packed with the binding support.
            SemanticError(Cg->pLastSourceLoc,
                          ERROR_S_ARB_UNSUPPORTED_OPERATION,
                          opcode_name[VARIABLE_OP]);
            return 0;
        }
        // Plain scalar or vector local variable:
        if (!IsScalar(symb->type) && !IsVector(symb->type, &len)) {
            SemanticError(Cg->pLastSourceLoc,
                          ERROR_S_ARB_UNSUPPORTED_OPERATION,
                          opcode_name[VARIABLE_OP]);
            return 0;
        }
        temp = GetSymbolTemp(ctx, symb);
        if (temp < 0)
            return 0;
        TrackSymbolTemp(ctx, symb);
        *operand = ArbTempOperand(temp);
        if (IsScalar(symb->type)) {
            operand->swizzle[0] = 0;
            operand->swizzle[1] = 0;
            operand->swizzle[2] = 0;
            operand->swizzle[3] = 0;
        }
        return 1;
    }

    if (expression->common.kind != BINARY_N ||
        expression->bin.op != MEMBER_SELECTOR_OP)
    {
        SemanticError(Cg->tokenLoc, ERROR_S_ARB_UNSUPPORTED_OPERATION,
                      opcode_name[expression->common.kind == BINARY_N ?
                                  expression->bin.op : VARIABLE_OP]);
        return 0;
    }

    member = expression->bin.right->sym.symbol;
    memberType = member->type;
    fBind = member->details.var.bind;
    if (ResolveMemberBinding(fBind, operand)) {
        // Scalar members read through their first component only.
        if (IsScalar(memberType)) {
            operand->swizzle[0] = 0;
            operand->swizzle[1] = 0;
            operand->swizzle[2] = 0;
            operand->swizzle[3] = 0;
        }
        return 1;
    }

    // Unbound local struct members are not representable yet.
    SemanticError(Cg->tokenLoc, ERROR_S_ARB_UNSUPPORTED_OPERATION,
                  opcode_name[MEMBER_SELECTOR_OP]);
    return 0;
} // LowerConnectorMember

/*
 * LowerLValue() - Resolve an assignment target.  Connector-register
 *         members map to their hardware registers; writes to input
 *         registers are elided by the caller as frontend bookkeeping.
 *         Swizzled targets such as value.yx contribute their written
 *         letters to the mask.
 */

static int LowerLValue(ArbLowerContext *ctx, expr *expression,
                       ArbOperand *operand, int *mask)
{
    Type *lType;

    if (expression->common.kind == UNARY_N &&
        expression->un.op == SWIZZLE_Z_OP)
    {
        unsigned int packed =
            (unsigned int) SUBOP_GET_MASK(expression->un.subop);
        int width = SUBOP_GET_S2(expression->un.subop);
    if (width <= 0)
        width = 1;
        int letterMask = 0;
        int ii;

        if (!LowerLValue(ctx, expression->un.arg, operand, mask))
            return 0;
        for (ii = 0; ii < width && ii < 4; ii++) {
            int letter = (int) ((packed >> (2 * ii)) & 3);
            letterMask |= (1 << letter);
            ctx->dstSel[ii] = letter;
        }
        for (; ii < 4; ii++)
            ctx->dstSel[ii] = ctx->dstSel[ii - 1];
        ctx->dstSelValid = 1;
        ctx->dstSelWidth = width;
        *mask &= letterMask;
        return 1;
    }

    if (expression->common.kind == SYMB_N &&
        expression->sym.op == VARIABLE_OP)
    {
        Symbol *symb = expression->sym.symbol;
        if (ResolveMemberBinding(symb->details.var.bind, operand)) {
            *mask = MaskFromType(symb->type);
            return 1;
        }
        if (GetDomain(symb->type) & TYPE_DOMAIN_UNIFORM) {
            SemanticError(Cg->pLastSourceLoc, ERROR_S_ARB_UNSUPPORTED_OPERATION,
                          opcode_name[VARIABLE_OP]);
            return 0;
        }
        {
            int temp = GetSymbolTemp(ctx, symb);
            if (temp < 0)
                return 0;
            TrackSymbolTemp(ctx, symb);
            *operand = ArbTempOperand(temp);
            if (IsScalar(symb->type))
                operand->swizzle[0] = operand->swizzle[1] =
                operand->swizzle[2] = operand->swizzle[3] = 0;
            *mask = MaskFromType(symb->type);
            return 1;
        }
    }

    if (expression->common.kind != BINARY_N ||
        expression->bin.op != MEMBER_SELECTOR_OP)
    {
        SemanticError(Cg->tokenLoc, ERROR_S_ARB_UNSUPPORTED_OPERATION,
                      opcode_name[ASSIGN_OP]);
        return 0;
    }

    lType = expression->common.type;
    if (!LowerConnectorMember(ctx, expression, operand))
        return 0;
    *mask = MaskFromType(lType);
    return 1;
} // LowerLValue

/*
 * SameOperand() - Whether two operands denote identical registers with
 *         identical modifiers, for identity-move elimination.
 */

static int SameOperand(const ArbOperand *a, const ArbOperand *b)
{
    int ii;

    if (a->file != b->file || a->index != b->index)
        return 0;
    if (a->negate != b->negate || a->absolute != b->absolute)
        return 0;
    for (ii = 0; ii < 4; ii++) {
        if (a->swizzle[ii] != b->swizzle[ii])
            return 0;
    }
    return 1;
} // SameOperand

/*
 * LowerAssignment() - Lower one fully-flattened assignment.  Returns 1 on
 *         success, including the cases where the statement carries no
 *         information (input-register copies and identity moves).
 */

static int LowerAssignment(ArbLowerContext *ctx, expr *left, expr *right,
                           int mask, const SourceLoc *loc)
{
    ArbOperand dst, src;
    ArbInstruction *inst;
    int dstMask;

    if (!LowerLValue(ctx, left, &dst, &dstMask))
        return 0;
    dstMask &= mask ? mask : ARB_MASK_XYZW;
    if (dst.file == ARB_REG_INPUT) {
        // Synthesized copy into a connector input: bookkeeping only.
        return 1;
    }
    if (!LowerExpression(ctx, right, &src))
        return 0;

    // Positional alignment: destination lane sel[p] (the p-th written
    // position) reads right-hand-side component p.  Without an explicit
    // selection, written lanes ascend and read components in order.

    if (dstMask != ARB_MASK_XYZW || ctx->dstSelValid) {
        signed char orig[4];
        signed char finalSwiz[4];
        int written = 0;
        int lane;

        for (lane = 0; lane < 4; lane++)
            orig[lane] = src.swizzle[lane];
        for (lane = 0; lane < 4; lane++)
            finalSwiz[lane] = orig[3];
        for (lane = 0; lane < 4; lane++) {
            int dstLane;
            if (!(dstMask & (1 << lane)))
                continue;
            dstLane = ctx->dstSelValid ? ctx->dstSel[written] : lane;
            finalSwiz[dstLane] = orig[written < 4 ? written : 3];
            written++;
        }
        for (lane = 0; lane < 4; lane++)
            src.swizzle[lane] = finalSwiz[lane];
    }

    if (SameOperand(&dst, &src)) {
        // Identity move (e.g. the $vout epilogue after direct mapping).
        return 1;
    }
    inst = ArbAppendInstruction(ctx->ir, ARB_OP_MOV, loc, dst);
    if (!inst)
        return 0;
    inst->mask = (unsigned char) dstMask;
    if (!ArbAddSource(inst, src))
        return 0;
    return 1;
} // LowerAssignment

/*
 * LowerExpressionStmt() - Lower one expression statement.  After pipeline
 *         normalization every remaining top-level expression with an
 *         effect is an assignment.
 */

static int LowerExpressionStmt(ArbLowerContext *ctx, expr *fExpr,
                               const SourceLoc *loc)
{
    if (!fExpr)
        return 1;
    ctx->dstSelValid = 0;
    ctx->dstSelWidth = 0;
    switch (fExpr->common.kind) {
    case BINARY_N:
        switch (fExpr->bin.op) {
        case ASSIGN_OP:
        case ASSIGN_V_OP:
        case ASSIGN_GEN_OP:
            return LowerAssignment(ctx, fExpr->bin.left, fExpr->bin.right,
                                   ARB_MASK_XYZW, loc);
        case ASSIGN_MASKED_KV_OP: {
            // KV masks carry one plain lane bit per written component.
            unsigned int packed =
                (unsigned int) SUBOP_GET_MASK(fExpr->bin.subop);
            int letterMask = (int) (packed & ARB_MASK_XYZW);
            return LowerAssignment(ctx, fExpr->bin.left, fExpr->bin.right,
                                   letterMask ? letterMask : ARB_MASK_XYZW,
                                   loc);
        }
        default:
            SemanticError(loc, ERROR_S_ARB_UNSUPPORTED_OPERATION,
                          opcode_name[fExpr->bin.op]);
            return 0;
        }
        break;
    default:
        SemanticError(loc, ERROR_S_ARB_UNSUPPORTED_OPERATION, "expression");
        return 0;
    }
} // LowerExpressionStmt

/*
 * LowerStatement() - Lower one statement in source order.
 */

static int LowerStatement(ArbLowerContext *ctx, stmt *statement)
{
    while (statement) {
        switch (statement->commonst.kind) {
        case EXPR_STMT:
            if (!LowerExpressionStmt(ctx, statement->exprst.exp,
                                     &statement->commonst.loc))
            {
                return 0;
            }
            break;
        case RETURN_STMT:
            // Program returns were rewritten to $vout assignments; any
            // surviving return carries no expression for this profile.
            break;
        case DISCARD_STMT:
            if (ctx->profile->stage == ARB_STAGE_VERTEX) {
                SemanticError(&statement->commonst.loc,
                              ERROR___ARB_VERTEX_DISCARD);
                return 0;
            }
            SemanticError(&statement->commonst.loc,
                          ERROR_S_ARB_UNSUPPORTED_OPERATION, "discard");
            return 0;
        case COMMENT_STMT:
            break;
        case BLOCK_STMT:
            if (!LowerStatement(ctx, statement->blockst.body))
                return 0;
            break;
        case IF_STMT:
        case WHILE_STMT:
        case DO_STMT:
        case FOR_STMT:
            SemanticError(&statement->commonst.loc,
                          ERROR_S_ARB_UNSUPPORTED_OPERATION,
                          opcode_name[VARIABLE_OP]);
            return 0;
        default:
            SemanticError(&statement->commonst.loc,
                          ERROR_S_ARB_UNSUPPORTED_OPERATION, "statement");
            return 0;
        }
        statement = statement->commonst.next;
    }
    return 1;
} // LowerStatement

/*
 * SmearOperand() - Return a copy of an operand reading its x component
 *         through all four lanes.
 */

static ArbOperand SmearOperand(ArbOperand operand)
{
    ArbOperand copy = operand;
    copy.swizzle[0] = 0;
    copy.swizzle[1] = 0;
    copy.swizzle[2] = 0;
    copy.swizzle[3] = 0;
    return copy;
} // SmearOperand

/*
 * EmitBinary() - Append one two-source instruction writing a fresh temp.
 */

static int EmitBinary(ArbLowerContext *ctx, ArbOpcode opcode, int mask,
                      const SourceLoc *loc, ArbOperand a, ArbOperand b,
                      ArbOperand *result)
{
    ArbInstruction *inst;
    int temp = ArbNewTemp(ctx->ir);

    if (temp < 0)
        return 0;
    inst = ArbAppendInstruction(ctx->ir, opcode, loc, ArbTempOperand(temp));
    if (!inst)
        return 0;
    inst->mask = (unsigned char) mask;
    if (!ArbAddSource(inst, a) || !ArbAddSource(inst, b))
        return 0;
    *result = ArbTempOperand(temp);
    return 1;
} // EmitBinary

/*
 * EmitUnary() - Append one one-source instruction writing a fresh temp.
 */

static int EmitUnary(ArbLowerContext *ctx, ArbOpcode opcode, int mask,
                     const SourceLoc *loc, ArbOperand a, ArbOperand *result)
{
    ArbInstruction *inst;
    int temp = ArbNewTemp(ctx->ir);

    if (temp < 0)
        return 0;
    inst = ArbAppendInstruction(ctx->ir, opcode, loc, ArbTempOperand(temp));
    if (!inst)
        return 0;
    inst->mask = (unsigned char) mask;
    if (!ArbAddSource(inst, a))
        return 0;
    *result = ArbTempOperand(temp);
    return 1;
} // EmitUnary

/*
 * LowerConstantNode() - Intern a scalar or vector constant node.
 */

static int LowerConstantNode(ArbLowerContext *ctx, expr *expression,
                             ArbOperand *operand)
{
    float values[4];
    int size;
    int index;
    int ii;

    switch (expression->co.op) {
    case FCONST_OP:
    case HCONST_OP:
    case XCONST_OP:
        for (ii = 0; ii < 4; ii++)
            values[ii] = expression->co.val[0].f;
        size = 1;
        break;
    case FCONST_V_OP:
    case HCONST_V_OP:
    case XCONST_V_OP:
        size = SUBOP_GET_S1(expression->co.subop);
        for (ii = 0; ii < 4; ii++)
            values[ii] = expression->co.val[ii < size ? ii : 0].f;
        break;
    case ICONST_OP:
    case BCONST_OP:
        for (ii = 0; ii < 4; ii++)
            values[ii] = (float) expression->co.val[0].i;
        size = 1;
        break;
    case ICONST_V_OP:
    case BCONST_V_OP:
        size = SUBOP_GET_S1(expression->co.subop);
        for (ii = 0; ii < 4; ii++)
            values[ii] = (float) expression->co.val[ii < size ? ii : 0].i;
        break;
    default:
        SemanticError(Cg->pLastSourceLoc,
                      ERROR_S_ARB_UNSUPPORTED_OPERATION, "constant");
        return 0;
    }
    index = ArbInternConstant(ctx->ir, values, size);
    if (index < 0)
        return 0;
    *operand = ArbConstOperand(index);
    return 1;
} // LowerConstantNode

/*
 * TypeWidth() - Component width of a scalar or vector type.
 */

static int TypeWidth(Type *fType)
{
    int len;

    if (fType && IsVector(fType, &len))
        return len;
    return 1;
} // TypeWidth

static int WidthMask(int width)
{
    switch (width) {
    case 2: return ARB_MASK_X | ARB_MASK_Y;
    case 3: return ARB_MASK_X | ARB_MASK_Y | ARB_MASK_Z;
    default: return ARB_MASK_XYZW;
    }
} // WidthMask

static int IsIdentitySwizzleLower(const signed char *swizzle)
{
    return swizzle[0] == 0 && swizzle[1] == 1 &&
           swizzle[2] == 2 && swizzle[3] == 3;
} // IsIdentitySwizzleLower

/*
 * ComposeSwizzledSource() - Apply a per-lane source-component selection
 *     over an existing operand's swizzle.
 */

static void ComposeSwizzledSource(ArbOperand *operand, const int *selection,
                                  int width)
{
    signed char old[4];
    int ii;

    for (ii = 0; ii < 4; ii++)
        old[ii] = operand->swizzle[ii];
    for (ii = 0; ii < 4; ii++) {
        int lane = ii < width ? selection[ii] : selection[width - 1];
        operand->swizzle[ii] = old[lane];
    }
} // ComposeSwizzledSource

/*
 * LowerSwizzleNode() - SWIZZLE_Z packs each destination lane's source
 *         component as a two-bit code in the subop mask field.
 */

static int LowerSwizzleNode(ArbLowerContext *ctx, expr *expression,
                            ArbOperand *operand)
{
    unsigned int packed = (unsigned int) SUBOP_GET_MASK(expression->un.subop);
    int width = SUBOP_GET_S2(expression->un.subop);
    if (width <= 0)
        width = 1;
    int selection[4];
    int ii;

    if (!LowerExpression(ctx, expression->un.arg, operand))
        return 0;
    for (ii = 0; ii < 4; ii++)
        selection[ii] = (int) ((packed >> (2 * ii)) & 3);
    for (ii = width; ii < 4; ii++)
        selection[ii] = selection[width - 1];
    for (ii = 0; ii < 4; ii++) {
        if (selection[ii] < 0 || selection[ii] > 3) {
            SemanticError(Cg->pLastSourceLoc,
                          ERROR_S_ARB_UNSUPPORTED_OPERATION, "swizzle");
            return 0;
        }
    }
    ComposeSwizzledSource(operand, selection, width);
    return 1;
} // LowerSwizzleNode

/*
 * LowerVectorConstructor() - Assemble components into a fresh temporary
 *         with one masked MOV per lane.
 */

static int LowerVectorConstructor(ArbLowerContext *ctx, expr *expression,
                                  const SourceLoc *loc, ArbOperand *operand)
{
    int width = SUBOP_GET_S1(expression->un.subop);
    expr *arg = expression->un.arg;
    int temp;
    int lane;

    temp = ArbNewTemp(ctx->ir);
    if (temp < 0)
        return 0;
    for (lane = 0; lane < width && arg; lane++) {
        ArbOperand component;
        ArbInstruction *inst;
        expr *item = arg;

        if (item->common.kind == BINARY_N && item->bin.op == EXPR_LIST_OP) {
            item = item->bin.left;
            arg = arg->bin.right;
        } else {
            arg = NULL;
        }
        if (!item || !LowerExpression(ctx, item, &component))
            return 0;
        inst = ArbAppendInstruction(ctx->ir, ARB_OP_MOV, loc,
                                    ArbTempOperand(temp));
        if (!inst)
            return 0;
        inst->mask = (unsigned char) (1 << lane);
        if (!ArbAddSource(inst, SmearOperand(component)))
            return 0;
    }
    *operand = ArbTempOperand(temp);
    return 1;
} // LowerVectorConstructor

/*
 * LowerBuiltinCall() - Map recognized internal functions to native ops.
 */

static int LowerBuiltinCall(ArbLowerContext *ctx, expr *expression,
                            const SourceLoc *loc, ArbOperand *operand)
{
    Symbol *fun = expression->bin.left->sym.symbol;
    expr *args = expression->bin.right;
    int index = fun->details.fun.index;

    if (fun->details.fun.group != ARB_BUILTIN_GROUP) {
        SemanticError(loc, ERROR_S_ARB_UNSUPPORTED_OPERATION,
                      GetAtomString(atable, fun->name));
        return 0;
    }
    switch (index) {
    case ARB_BUILTIN_RSQ: {
        // rsqrt(x) -> RSQ of the first argument's x component.
        expr *argExpr;
        ArbOperand arg;
        ArbOperand result;

        if (!args || args->bin.op != FUN_ARG_OP) {
            SemanticError(loc, ERROR_S_ARB_UNSUPPORTED_OPERATION, "rsqrt");
            return 0;
        }
        argExpr = args->bin.left;
        if (!LowerExpression(ctx, argExpr, &arg))
            return 0;
        arg = SmearOperand(arg);
        if (!EmitUnary(ctx, ARB_OP_RSQ, ARB_MASK_X, loc, arg, &result))
            return 0;
        *operand = result;
        return 1;
    }
    default:
        SemanticError(loc, ERROR_S_ARB_UNSUPPORTED_OPERATION,
                      GetAtomString(atable, fun->name));
        return 0;
    }
} // LowerBuiltinCall

/*
 * LowerBooleanOrComparison() - Comparisons produce 0.0/1.0 masks via SLT
 *     and SGE.  Boolean AND multiplies, OR adds then clamps with MIN, and
 *     NOT subtracts from one.
 */

static int LowerBooleanOrComparison(ArbLowerContext *ctx, expr *expression,
                                    const SourceLoc *loc, ArbOperand *operand)
{
    int op = expression->bin.op;
    int width = TypeWidth(expression->common.type);
    int mask = WidthMask(width);
    ArbOperand a, b;

    switch (op) {
    case LT_OP: case LT_V_OP: case LT_SV_OP: case LT_VS_OP:
    case GT_OP: case GT_V_OP: case GT_SV_OP: case GT_VS_OP:
    case LE_OP: case LE_V_OP: case LE_SV_OP: case LE_VS_OP:
    case GE_OP: case GE_V_OP: case GE_SV_OP: case GE_VS_OP:
    {
        ArbOpcode opcode;
        int swap = 0;

        if (!LowerExpression(ctx, expression->bin.left, &a) ||
            !LowerExpression(ctx, expression->bin.right, &b))
        {
            return 0;
        }
        switch (op) {
        case LT_OP: case LT_V_OP: case LT_SV_OP: case LT_VS_OP:
            opcode = ARB_OP_SLT;
            break;
        case GE_OP: case GE_V_OP: case GE_SV_OP: case GE_VS_OP:
            opcode = ARB_OP_SGE;
            break;
        case GT_OP: case GT_V_OP: case GT_SV_OP: case GT_VS_OP:
            opcode = ARB_OP_SLT;
            swap = 1;
            break;
        default: // LE family
            opcode = ARB_OP_SGE;
            swap = 1;
            break;
        }
        if (swap) {
            ArbOperand tmp = a;
            a = b;
            b = tmp;
        }
        if (TypeWidth(expression->bin.left->common.type) == 1 && width > 1)
            a = SmearOperand(a);
        if (TypeWidth(expression->bin.right->common.type) == 1 && width > 1)
            b = SmearOperand(b);
        return EmitBinary(ctx, opcode, mask, loc, a, b, operand);
    }
    case EQ_OP: case EQ_V_OP: case EQ_SV_OP: case EQ_VS_OP:
    case NE_OP: case NE_V_OP: case NE_SV_OP: case NE_VS_OP:
    {
        ArbOperand geAB, geBA, eq;
        int isNE = (op == NE_OP || op == NE_V_OP ||
                    op == NE_SV_OP || op == NE_VS_OP);

        if (!LowerExpression(ctx, expression->bin.left, &a) ||
            !LowerExpression(ctx, expression->bin.right, &b))
        {
            return 0;
        }
        if (TypeWidth(expression->bin.left->common.type) == 1 && width > 1)
            a = SmearOperand(a);
        if (TypeWidth(expression->bin.right->common.type) == 1 && width > 1)
            b = SmearOperand(b);
        if (!EmitBinary(ctx, ARB_OP_SGE, mask, loc, a, b, &geAB) ||
            !EmitBinary(ctx, ARB_OP_SGE, mask, loc, b, a, &geBA))
        {
            return 0;
        }
        if (!EmitBinary(ctx, ARB_OP_MUL, mask, loc, geAB, geBA, &eq))
            return 0;
        if (!isNE) {
            *operand = eq;
            return 1;
        } else {
            float one[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
            int cindex = ArbInternConstant(ctx->ir, one, 4);
            ArbOperand oneOp;
            if (cindex < 0)
                return 0;
            oneOp = ArbConstOperand(cindex);
            return EmitBinary(ctx, ARB_OP_SUB, mask, loc, oneOp, eq, operand);
        }
    }
    case AND_OP: case AND_V_OP: case AND_SV_OP: case AND_VS_OP:
    case BAND_OP: case BAND_V_OP: case BAND_SV_OP: case BAND_VS_OP:
    {
        if (!LowerExpression(ctx, expression->bin.left, &a) ||
            !LowerExpression(ctx, expression->bin.right, &b))
        {
            return 0;
        }
        return EmitBinary(ctx, ARB_OP_MUL, mask, loc, a, b, operand);
    }
    case OR_OP: case OR_V_OP: case OR_SV_OP: case OR_VS_OP:
    case BOR_OP: case BOR_V_OP: case BOR_SV_OP: case BOR_VS_OP:
    {
        ArbOperand sum;
        float one[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
        int cindex;
        ArbOperand oneOp;

        if (!LowerExpression(ctx, expression->bin.left, &a) ||
            !LowerExpression(ctx, expression->bin.right, &b))
        {
            return 0;
        }
        if (!EmitBinary(ctx, ARB_OP_ADD, mask, loc, a, b, &sum))
            return 0;
        cindex = ArbInternConstant(ctx->ir, one, 4);
        if (cindex < 0)
            return 0;
        oneOp = ArbConstOperand(cindex);
        return EmitBinary(ctx, ARB_OP_MIN, mask, loc, sum, oneOp, operand);
    }
    default:
        return 0;
    }
} // LowerBooleanOrComparison

/*
 * LowerArithmeticBinary() - Additive, multiplicative, and division forms
 *     with scalar smearing and reciprocal-based division.
 */

static int LowerArithmeticBinary(ArbLowerContext *ctx, expr *expression,
                                 const SourceLoc *loc, ArbOperand *operand)
{
    int op = expression->bin.op;
    int width = TypeWidth(expression->common.type);
    int mask = WidthMask(width);
    int lwidth = TypeWidth(expression->bin.left->common.type);
    int rwidth = TypeWidth(expression->bin.right->common.type);
    ArbOpcode opcode;
    ArbOperand a, b;
    int isDiv = 0;

    switch (op) {
    case ADD_OP: case ADD_V_OP: case ADD_SV_OP: case ADD_VS_OP:
        opcode = ARB_OP_ADD;
        break;
    case SUB_OP: case SUB_V_OP: case SUB_SV_OP: case SUB_VS_OP:
        opcode = ARB_OP_SUB;
        break;
    case MUL_OP: case MUL_V_OP: case MUL_SV_OP: case MUL_VS_OP:
        opcode = ARB_OP_MUL;
        break;
    case DIV_OP: case DIV_V_OP: case DIV_SV_OP: case DIV_VS_OP:
        isDiv = 1;
        break;
    default:
        return 0;
    }

    if (!LowerExpression(ctx, expression->bin.left, &a) ||
        !LowerExpression(ctx, expression->bin.right, &b))
    {
        return 0;
    }

    if (isDiv) {
        // Divide through a reciprocal of the right-hand side.
        ArbOperand rcp;
        int rcpMask = (rwidth == 1) ? ARB_MASK_X : mask;
        ArbOperand divisor = b;
        ArbOperand dividend = a;

        if (lwidth == 1)
            dividend = SmearOperand(dividend);
        if (rwidth == 1)
            divisor = SmearOperand(divisor);
        if (!EmitUnary(ctx, ARB_OP_RCP, rcpMask, loc, divisor, &rcp))
            return 0;
        if (rwidth == 1)
            rcp = SmearOperand(rcp);
        else if (rwidth < 4 && width > rwidth) {
            int sel[4];
            int ii;
            for (ii = 0; ii < 4; ii++)
                sel[ii] = ii < rwidth ? ii : rwidth - 1;
            ComposeSwizzledSource(&rcp, sel, width);
        }
        return EmitBinary(ctx, ARB_OP_MUL, mask, loc, dividend, rcp,
                          operand);
    }

    if ((op == ADD_SV_OP || op == SUB_SV_OP || op == MUL_SV_OP) &&
        lwidth == 1 && width > 1)
    {
        a = SmearOperand(a);
    }
    if ((op == ADD_VS_OP || op == SUB_VS_OP || op == MUL_VS_OP) &&
        rwidth == 1 && width > 1)
    {
        b = SmearOperand(b);
    }
    if (width == 1) {
        a = SmearOperand(a);
        b = SmearOperand(b);
        mask = ARB_MASK_X;
    } else if (lwidth == 1 && rwidth == 1) {
        a = SmearOperand(a);
        b = SmearOperand(b);
    } else if (lwidth < width && lwidth > 1) {
        int sel[4];
        int ii;
        for (ii = 0; ii < 4; ii++)
            sel[ii] = ii < lwidth ? ii : lwidth - 1;
        ComposeSwizzledSource(&a, sel, width);
    } else if (rwidth < width && rwidth > 1) {
        int sel[4];
        int ii;
        for (ii = 0; ii < 4; ii++)
            sel[ii] = ii < rwidth ? ii : rwidth - 1;
        ComposeSwizzledSource(&b, sel, width);
    }
    return EmitBinary(ctx, opcode, mask, loc, a, b, operand);
} // LowerArithmeticBinary

/*
 * SethiUllmanCost() - Simple register-need estimate used to order the
 *     evaluation of commutative subexpressions.
 */

static int SethiUllmanCost(expr *expression)
{
    int l, r;

    if (!expression)
        return 0;
    switch (expression->common.kind) {
    case SYMB_N:
    case CONST_N:
        return 1;
    case UNARY_N:
        l = SethiUllmanCost(expression->un.arg);
        return l > 1 ? l : 1;
    case BINARY_N:
        l = SethiUllmanCost(expression->bin.left);
        r = SethiUllmanCost(expression->bin.right);
        if (l == r)
            return l + 1;
        return (l > r ? l : r) + 1;
    default:
        return 1;
    }
} // SethiUllmanCost

static int IsCommutativeOpcode(int op)
{
    switch (op) {
    case ADD_OP: case ADD_V_OP: case ADD_SV_OP: case ADD_VS_OP:
    case MUL_OP: case MUL_V_OP: case MUL_SV_OP: case MUL_VS_OP:
    case AND_OP: case AND_V_OP: case BAND_OP: case BAND_V_OP:
    case OR_OP: case OR_V_OP: case BOR_OP: case BOR_V_OP:
    case EQ_OP: case EQ_V_OP:
        return 1;
    default:
        return 0;
    }
} // IsCommutativeOpcode

static int RejectUnsupportedBinary(expr *expression)
{
    switch (expression->bin.op) {
    case SHL_OP: case SHL_V_OP: case SHR_OP: case SHR_V_OP:
    case MOD_OP: case MOD_V_OP: case MOD_SV_OP: case MOD_VS_OP:
    case XOR_OP: case XOR_V_OP: case XOR_SV_OP: case XOR_VS_OP:
    case BNOT_OP: case BNOT_V_OP:
    case PREDEC_OP: case PREINC_OP: case POSTDEC_OP: case POSTINC_OP:
        SemanticError(Cg->pLastSourceLoc,
                      ERROR_S_ARB_UNSUPPORTED_OPERATION,
                      opcode_name[expression->bin.op]);
        return 1;
    default:
        return 0;
    }
} // RejectUnsupportedBinary

/*
 * LowerExpression() - Lower a readable expression into an operand.  For
 *     side-effect-free commutative operations the higher-cost subtree is
 *     lowered first so it occupies the first source slot while the emitted
 *     operands keep their mathematical order.
 */

static int LowerExpression(ArbLowerContext *ctx, expr *expression,
                           ArbOperand *operand)
{
    const SourceLoc *loc = Cg->pLastSourceLoc;

    if (!expression)
        return 0;
    switch (expression->common.kind) {
    case CONST_N:
        return LowerConstantNode(ctx, expression, operand);

    case SYMB_N:
        if (expression->sym.op != VARIABLE_OP) {
            SemanticError(loc, ERROR_S_ARB_UNSUPPORTED_OPERATION,
                          opcode_name[expression->sym.op]);
            return 0;
        }
        return LowerConnectorMember(ctx, expression, operand);

    case UNARY_N:
        switch (expression->un.op) {
        case NEG_OP:
        case NEG_V_OP:
            if (!LowerExpression(ctx, expression->un.arg, operand))
                return 0;
            operand->negate = !operand->negate;
            return 1;
        case POS_OP:
        case POS_V_OP:
            return LowerExpression(ctx, expression->un.arg, operand);
        case CAST_CS_OP:
        case CAST_CV_OP:
        case CAST_CM_OP:
            // Numeric casts are value-preserving at this level.
            return LowerExpression(ctx, expression->un.arg, operand);
        case SWIZZLE_Z_OP:
            return LowerSwizzleNode(ctx, expression, operand);
        case VECTOR_V_OP:
            return LowerVectorConstructor(ctx, expression, loc, operand);
        case KILL_OP:
            SemanticError(loc, ERROR_S_ARB_UNSUPPORTED_OPERATION, "discard");
            return 0;
        default:
            SemanticError(loc, ERROR_S_ARB_UNSUPPORTED_OPERATION,
                          opcode_name[expression->un.op]);
            return 0;
        }
        break;

    case BINARY_N: {
        int lcost, rcost;

        switch (expression->bin.op) {
        case MEMBER_SELECTOR_OP:
            return LowerConnectorMember(ctx, expression, operand);
        case FUN_ARG_OP:
        case EXPR_LIST_OP:
        case COMMA_OP:
            SemanticError(loc, ERROR_S_ARB_UNSUPPORTED_OPERATION, "argument");
            return 0;
        case FUN_CALL_OP:
            SemanticError(loc, ERROR_S_ARB_UNSUPPORTED_OPERATION, "call");
            return 0;
        case FUN_BUILTIN_OP:
            return LowerBuiltinCall(ctx, expression, loc, operand);
        case ASSIGN_OP: case ASSIGN_V_OP: case ASSIGN_GEN_OP:
        case ASSIGN_MASKED_KV_OP:
            SemanticError(loc, ERROR_S_ARB_UNSUPPORTED_OPERATION, "assign");
            return 0;
        case ARRAY_INDEX_OP:
            SemanticError(loc, ERROR_S_ARB_UNSUPPORTED_OPERATION, "index");
            return 0;
        default:
            break;
        }

        if (RejectUnsupportedBinary(expression))
            return 0;

        lcost = SethiUllmanCost(expression->bin.left);
        rcost = SethiUllmanCost(expression->bin.right);
        if (IsCommutativeOpcode(expression->bin.op) &&
            strcmp(opcode_name[expression->bin.op], "mul") == 0)
        {
            // Keep mul operand order stable except when reordering helps
            // register pressure on genuinely symmetric trees.
        }
        if (IsCommutativeOpcode(expression->bin.op) && rcost > lcost) {
            expr *tmp = expression->bin.left;
            expression->bin.left = expression->bin.right;
            expression->bin.right = tmp;
        }

        if (LowerArithmeticBinary(ctx, expression, loc, operand))
            return 1;
        if (LowerBooleanOrComparison(ctx, expression, loc, operand))
            return 1;
        SemanticError(loc, ERROR_S_ARB_UNSUPPORTED_OPERATION,
                      opcode_name[expression->bin.op]);
        return 0;
    }

    default:
        SemanticError(loc, ERROR_S_ARB_UNSUPPORTED_OPERATION, "expression");
        return 0;
    }
    return 0;
} // LowerExpression

/*
 * ArbLowerProgram() - Lower the whole program body.  Returns 0 after any
 *         failed statement or expression lowering.
 */

int ArbLowerProgram(ArbProgram *ir, const ArbProfileDesc *profile,
                    Symbol *program)
{
    ArbLowerContext ctx;

    ctx.ir = ir;
    ctx.profile = profile;
    ctx.program = program;
    ctx.staticValues = NULL;

    if (!LowerStatement(&ctx, program->details.fun.statements)) {
        ClearSymbolTemps(&ctx);
        return 0;
    }
    ClearSymbolTemps(&ctx);
    return 1;
} // ArbLowerProgram

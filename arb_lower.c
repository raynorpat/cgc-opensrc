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

#include "arb_lower_internal.h"

static int LowerAssignment(ArbLowerContext *ctx, expr *left, expr *right,
                           int mask, const SourceLoc *loc);
static int EvalConstInt(ArbLowerContext *ctx, expr *e, int *out);
static int ExtractStepDelta(ArbLowerContext *ctx, expr *e, Symbol *target,
                            int *delta, int *found);

/*
 * StaticPush/StaticPop/StaticFind() - Simulated induction-variable values.
 *     A symbol on the stack contributes its compile-time value instead of
 *     a register; nested loops restore the prior stack on exit.
 */

static void StaticPush(ArbLowerContext *ctx, Symbol *symbol, int value)
{
    ArbStaticValue *sv = (ArbStaticValue *) malloc(sizeof(ArbStaticValue));
    if (!sv)
        return;
    sv->symbol = symbol;
    sv->value = value;
    sv->next = ctx->staticValues;
    ctx->staticValues = sv;
} // StaticPush

static void StaticPop(ArbLowerContext *ctx)
{
    ArbStaticValue *sv = ctx->staticValues;
    if (sv) {
        ctx->staticValues = sv->next;
        free(sv);
    }
} // StaticPop

int ArbLowerStaticFind(const ArbLowerContext *ctx, Symbol *symbol, int *value)
{
    ArbStaticValue *sv = ctx->staticValues;
    while (sv) {
        if (sv->symbol == symbol) {
            *value = sv->value;
            return 1;
        }
        sv = sv->next;
    }
    return 0;
} // StaticFind

/*
 * EvalConstInt() - Integer constant evaluation supporting literals, the
 *     active induction symbols, unary +/-/!, and the arithmetic,
 *     comparison, and boolean operators with C semantics.  Division or
 *     modulus by zero fails.
 */

static int EvalConstInt(ArbLowerContext *ctx, expr *e, int *out)
{
    int a, b;

    if (!e)
        return 0;
    switch (e->common.kind) {
    case CONST_N:
        switch (e->co.op) {
        case ICONST_OP:
            *out = e->co.val[0].value.i;
            return 1;
        case BCONST_OP:
            *out = e->co.val[0].value.i ? 1 : 0;
            return 1;
        default:
            return 0;
        }
    case SYMB_N:
        if (e->sym.op == VARIABLE_OP &&
            GetBase(e->common.type) == TYPE_BASE_INT)
        {
            return ArbLowerStaticFind(ctx, e->sym.symbol, out);
        }
        return 0;
    case UNARY_N:
        if (!EvalConstInt(ctx, e->un.arg, &a))
            return 0;
        switch (e->un.op) {
        case NEG_OP: *out = -a; return 1;
        case POS_OP: *out = a; return 1;
        case BNOT_OP: *out = !a; return 1;
        default: return 0;
        }
    case TRINARY_N:
        if (e->tri.op != COND_OP && e->tri.op != COND_GEN_OP)
            return 0;
        if (!EvalConstInt(ctx, e->tri.arg1, &a))
            return 0;
        return EvalConstInt(ctx, a ? e->tri.arg2 : e->tri.arg3, out);
    case BINARY_N:
        switch (e->bin.op) {
        case BAND_OP:
        case AND_OP:
            if (!EvalConstInt(ctx, e->bin.left, &a))
                return 0;
            if (!a) { *out = 0; return 1; }
            if (!EvalConstInt(ctx, e->bin.right, &b))
                return 0;
            *out = b ? 1 : 0;
            return 1;
        case BOR_OP:
        case OR_OP:
            if (!EvalConstInt(ctx, e->bin.left, &a))
                return 0;
            if (a) { *out = 1; return 1; }
            if (!EvalConstInt(ctx, e->bin.right, &b))
                return 0;
            *out = b ? 1 : 0;
            return 1;
        default:
            break;
        }
        switch (e->bin.op) {
        case ADD_OP:
            if ((b > 0 && a > INT_MAX - b) || (b < 0 && a < INT_MIN - b))
                return 0;
            *out = a + b;
            return 1;
        case SUB_OP:
            if ((b < 0 && a > INT_MAX + b) || (b > 0 && a < INT_MIN + b))
                return 0;
            *out = a - b;
            return 1;
        case MUL_OP:
            if (a != 0 && (a > INT_MAX / b || a < INT_MIN / b))
                return 0;
            *out = a * b;
            return 1;
        case DIV_OP:
            if (b == 0 || (a == INT_MIN && b == -1))
                return 0;
            *out = a / b;
            return 1;
        case MOD_OP:
            if (b == 0 || (a == INT_MIN && b == -1))
                return 0;
            *out = a % b;
            return 1;
        case LT_OP: *out = a < b; return 1;
        case LE_OP: *out = a <= b; return 1;
        case GT_OP: *out = a > b; return 1;
        case GE_OP: *out = a >= b; return 1;
        case EQ_OP: *out = a == b; return 1;
        case NE_OP: *out = a != b; return 1;
        default:
            return 0;
        }
    default:
        return 0;
    }
} // EvalConstInt

/*
 * CollectAssignTargets() - Walk a comma/EXPR_LIST chain (either nesting
 *     orientation) and report whether the only assignment to "target"
 *     has an evaluable delta; other nodes are postinc temporaries.
 */

/*
 * TrailingUpdateScan() - Walk backwards over the trailing statements of a
 *     loop body, skipping value-discarded temporary reads, and identify
 *     the induction step assignment.  Fills consumed[] with every
 *     statement from the update through the end of the body.
 */

static int TrailingUpdateScan(ArbLowerContext *ctx, stmt *body,
                              Symbol *induction, int *delta,
                              stmt **consumed, int *consumedCount)
{
    stmt *tail[16];
    int tailCount = 0;
    stmt *s = body;
    int ii;

    // Unwrap any brace blocks to reach the statement sequence.
    while (s && s->commonst.kind == BLOCK_STMT)
        s = s->blockst.body;
    while (s) {
        if (tailCount < 16)
            tail[tailCount] = s;
        tailCount++;
        s = s->commonst.next;
    }
    if (tailCount > 16)
        return 0;

    // Skip trailing bare-symbol statements (postinc temp reads).
    ii = tailCount - 1;
    while (ii >= 0 &&
           tail[ii]->commonst.kind == EXPR_STMT &&
           tail[ii]->exprst.exp->common.kind == SYMB_N)
    {
        ii--;
    }
    if (ii < 0 || tail[ii]->commonst.kind != EXPR_STMT)
        return 0;
    if (!ExtractSimpleDelta(ctx, tail[ii]->exprst.exp, induction, delta))
        return 0;
    *consumedCount = 0;
    for (; ii < tailCount && *consumedCount < 16; ii++)
        consumed[(*consumedCount)++] = tail[ii];
    return 1;
} // TrailingUpdateScan

static int ExtractStepDelta(ArbLowerContext *ctx, expr *e, Symbol *target,
                            int *delta, int *found);

static int ExtractSimpleDelta(ArbLowerContext *ctx, expr *e, Symbol *target,
                               int *delta)
{
    int a, b;

    // Shapes: i = i + c ; i = i - c ; i = c + i
    if (e->common.kind != BINARY_N ||
        (e->bin.op != ASSIGN_OP && e->bin.op != ASSIGN_V_OP &&
         e->bin.op != ASSIGN_GEN_OP))
    {
        // Compound assignments were expanded by the pipeline.
        return 0;
    }
    if (e->bin.left->common.kind != SYMB_N ||
        e->bin.left->sym.symbol != target)
    {
        return 0;
    }
    {
        expr *rhs = e->bin.right;
        if (rhs->common.kind == BINARY_N && rhs->bin.op == ADD_OP) {
            if (rhs->bin.left->common.kind == SYMB_N &&
                rhs->bin.left->sym.symbol == target &&
                EvalConstInt(ctx, rhs->bin.right, &b))
            {
                *delta = b;
                return 1;
            }
            if (rhs->bin.right->common.kind == SYMB_N &&
                rhs->bin.right->sym.symbol == target &&
                EvalConstInt(ctx, rhs->bin.left, &b))
            {
                *delta = b;
                return 1;
            }
            return 0;
        }
        if (rhs->common.kind == BINARY_N && rhs->bin.op == SUB_OP &&
            rhs->bin.left->common.kind == SYMB_N &&
            rhs->bin.left->sym.symbol == target &&
            EvalConstInt(ctx, rhs->bin.right, &b))
        {
            *delta = -b;
            return 1;
        }
        if (EvalConstInt(ctx, rhs, &a)) {
            // i = <const>: only valid as a loop initializer, not a step.
            return 0;
        }
    }
    return 0;
} // ExtractSimpleDelta

static int ExtractStepDelta(ArbLowerContext *ctx, expr *e, Symbol *target,
                            int *delta, int *found)
{
    if (!e)
        return 1;
    if (e->common.kind == BINARY_N &&
        (e->bin.op == COMMA_OP || e->bin.op == EXPR_LIST_OP))
    {
        if (!ExtractStepDelta(ctx, e->bin.left, target, delta, found))
            return 0;
        if (!ExtractStepDelta(ctx, e->bin.right, target, delta, found))
            return 0;
        return 1;
    }
    if (e->common.kind == BINARY_N &&
        (e->bin.op == ASSIGN_OP || e->bin.op == ASSIGN_V_OP ||
         e->bin.op == ASSIGN_GEN_OP))
    {
        if (e->bin.left->common.kind == SYMB_N &&
            e->bin.left->sym.symbol == target)
        {
            if (*found)
                return 0; // Two writes to the induction symbol: ambiguous.
            if (!ExtractSimpleDelta(ctx, e, target, delta))
                return 0;
            *found = 1;
            return 1;
        }
        return 1; // Unrelated assignment (postinc temp shuffle).
    }
    // Any other expression is fine as long as it has no side effects on
    // the induction symbol.
    return 1;
} // ExtractStepDelta

/*
 * LoopInductionSymbol() - The local int symbol written by an expression,
 *     or NULL when the expression does not assign exactly one local int.
 */

static Symbol *AssignedLocalInt(expr *e)
{
    if (e && e->common.kind == BINARY_N &&
        (e->bin.op == ASSIGN_OP || e->bin.op == ASSIGN_V_OP ||
         e->bin.op == ASSIGN_GEN_OP) &&
        e->bin.left->common.kind == SYMB_N &&
        GetBase(e->bin.left->common.type) == TYPE_BASE_INT &&
        IsScalar(e->bin.left->common.type))
    {
        return e->bin.left->sym.symbol;
    }
    return NULL;
} // AssignedLocalInt

/*
 * CanonicalLoopCondition() - Decode "i <op> <const>" or "<const> <op> i".
 */

static int CanonicalLoopCondition(ArbLowerContext *ctx, expr *cond,
                                  Symbol **symb, int *bound, int *op)
{
    expr *lhs = cond ? cond->bin.left : NULL;
    expr *rhs = cond ? cond->bin.right : NULL;
    int b;

    if (!cond || cond->common.kind != BINARY_N)
        return 0;
    if (lhs->common.kind == SYMB_N &&
        GetBase(lhs->common.type) == TYPE_BASE_INT &&
        EvalConstInt(ctx, rhs, &b))
    {
        *symb = lhs->sym.symbol;
        *bound = b;
        // Normalize comparison variants (ltv/ltsv/ltvs) to scalars.
        switch (cond->bin.op) {
        case LT_OP: case LT_V_OP: case LT_SV_OP: case LT_VS_OP:
            *op = LT_OP; break;
        case LE_OP: case LE_V_OP: case LE_SV_OP: case LE_VS_OP:
            *op = LE_OP; break;
        case GT_OP: case GT_V_OP: case GT_SV_OP: case GT_VS_OP:
            *op = GT_OP; break;
        case GE_OP: case GE_V_OP: case GE_SV_OP: case GE_VS_OP:
            *op = GE_OP; break;
        case EQ_OP: case EQ_V_OP: case EQ_SV_OP: case EQ_VS_OP:
            *op = EQ_OP; break;
        case NE_OP: case NE_V_OP: case NE_SV_OP: case NE_VS_OP:
            *op = NE_OP; break;
        default:
            return 0;
        }
        return 1;
    }
    if (rhs->common.kind == SYMB_N &&
        GetBase(rhs->common.type) == TYPE_BASE_INT &&
        EvalConstInt(ctx, lhs, &b))
    {
        *symb = rhs->sym.symbol;
        *bound = b;
        // Mirror the comparison.
        switch (cond->bin.op) {
        case LT_OP: case LT_V_OP: case LT_SV_OP: case LT_VS_OP:
            *op = GT_OP; break;
        case LE_OP: case LE_V_OP: case LE_SV_OP: case LE_VS_OP:
            *op = GE_OP; break;
        case GT_OP: case GT_V_OP: case GT_SV_OP: case GT_VS_OP:
            *op = LT_OP; break;
        case GE_OP: case GE_V_OP: case GE_SV_OP: case GE_VS_OP:
            *op = LE_OP; break;
        case EQ_OP: case EQ_V_OP: case EQ_SV_OP: case EQ_VS_OP:
            *op = EQ_OP; break;
        case NE_OP: case NE_V_OP: case NE_SV_OP: case NE_VS_OP:
            *op = NE_OP; break;
        default:
            return 0;
        }
        return 1;
    }
    return 0;
} // CanonicalLoopCondition

/*
 * ConditionHolds() - Evaluate the canonical comparison for one value.
 */

static int ConditionHolds(int op, int value, int bound)
{
    switch (op) {
    case LT_OP: return value < bound;
    case LE_OP: return value <= bound;
    case GT_OP: return value > bound;
    case GE_OP: return value >= bound;
    case NE_OP: return value != bound;
    case EQ_OP: return value == bound;
    default: return 0;
    }
} // ConditionHolds

/*
 * StepProvesTermination() - Whether stepping by "delta" from any value
 *     satisfying the condition must eventually violate it.
 */

static int StepProvesTermination(int op, int delta)
{
    switch (op) {
    case LT_OP:
    case LE_OP:
        return delta > 0;
    case GT_OP:
    case GE_OP:
        return delta < 0;
    case NE_OP:
        return 1; // Exact reach verified during simulation bounds.
    case EQ_OP:
        return 1;
    default:
        return 0;
    }
} // StepProvesTermination

/*
 * SimulateLoopIterations() - Walk values start, start+delta, ... while
 *     the condition holds (test-first when testFirst).  Returns the list
 *     of body values via outValues and the count, or -1 when the loop is
 *     not provably finite within the unroll budget.
 */

static int SimulateLoopIterations(int op, int bound, int start, int delta,
                                  int testFirst, int *outValues, int maxCount)
{
    int count = 0;
    int v = start;

    for (;;) {
        if (testFirst && !ConditionHolds(op, v, bound))
            break;
        if (count >= maxCount)
            return -1;
        outValues[count++] = v;
        if (delta > 0 && v > INT_MAX - delta)
            return -1; // Signed overflow on the way to termination.
        if (delta < 0 && v < INT_MIN - delta)
            return -1;
        v += delta;
        if (!testFirst && !ConditionHolds(op, v, bound))
            break;
    }
    return count;
} // SimulateLoopIterations

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

    if (!ArbLowerLValue(ctx, left, &dst, &dstMask))
        return 0;
    dstMask &= mask ? mask : ARB_MASK_XYZW;
    if (dst.file == ARB_REG_INPUT) {
        // Synthesized copy into a connector input: bookkeeping only.
        return 1;
    }
    if (!ArbLowerExpression(ctx, right, &src))
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

    // Aggregates (packed matrices and arrays) copy one quad at a time.

    if (left->common.type && GetQuadRegSize(left->common.type) > 1 &&
        dst.file == ARB_REG_TEMP)
    {
        int quads = GetQuadRegSize(left->common.type);
        int k;

        for (k = 0; k < quads; k++) {
            ArbOperand srcQuad = src;
            ArbInstruction *qinst;

            if (src.file == ARB_REG_TEMP || src.file == ARB_REG_PARAM) {
                srcQuad.index = src.index + k;
                srcQuad.swizzle[0] = 0;
                srcQuad.swizzle[1] = 1;
                srcQuad.swizzle[2] = 2;
                srcQuad.swizzle[3] = 3;
            } else if (k > 0) {
                // Only the first quad can come from a scalar source.
                SemanticError(loc, ERROR_S_ARB_UNSUPPORTED_OPERATION,
                              "aggregate");
                return 0;
            }
            qinst = ArbAppendInstruction(ctx->ir, ARB_OP_MOV, loc,
                                         ArbTempOperand(dst.index + k));
            if (!qinst)
                return 0;
            qinst->mask = ARB_MASK_XYZW;
            if (!ArbAddSource(qinst, srcQuad))
                return 0;
        }
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
        case ASSIGN_COND_OP:
        case ASSIGN_COND_V_OP:
        case ASSIGN_COND_SV_OP:
        case ASSIGN_COND_GEN_OP: {
            // Flattened "dst = cond ? value : dst": read the old
            // destination as the false alternative.
            expr *dstExpr = fExpr->tri.arg1;
            expr *condExpr = fExpr->tri.arg2;
            expr *valExpr = fExpr->tri.arg3;
            ArbOperand dst, cond, tv, fv, sel;
            int dstMask;
            Type *dstType = dstExpr->common.type;

            ctx->dstSelValid = 0;
            ctx->dstSelWidth = 0;
            if (!ArbLowerLValue(ctx, dstExpr, &dst, &dstMask))
                return 0;
            if (!ArbLowerExpression(ctx, condExpr, &cond))
                return 0;
            if (!ArbLowerExpression(ctx, valExpr, &tv))
                return 0;
            fv = dst; // Old destination contents.
            if (fv.file == ARB_REG_TEMP)
                fv.swizzle[0] = fv.swizzle[1] = fv.swizzle[2] =
                    fv.swizzle[3] = IsScalar(dstType) ? 0 : fv.swizzle[3];
            if (!ArbLowerBuildSelect(ctx, loc, cond, tv, fv,
                             ArbLowerTypeWidth(dstType),
                             ArbLowerMaskFromType(dstType), &sel))
            {
                return 0;
            }
            {
                ArbInstruction *mov =
                    ArbAppendInstruction(ctx, ARB_OP_MOV, loc, dst);
                if (!mov)
                    return 0;
                mov->mask = (unsigned char) ArbLowerMaskFromType(dstType);
                if (!ArbAddSource(mov, sel))
                    return 0;
            }
            return 1;
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

///////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////// Canonical Loop Lowering //////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////

/*
 * PairWhileInitializers() - Pre-pass over one statement list: while and
 *     do loops whose induction symbol receives a constant initializer in
 *     the immediately preceding statement claim that statement (and their
 *     body's final step expression) as compile-time loop control.
 */

void ArbLowerPairWhileInitializers(ArbLowerContext *ctx, stmt *list)
{
    stmt *prev = NULL;
    stmt *s;

    for (s = list; s; s = s->commonst.next) {
        if (s->commonst.kind == WHILE_STMT || s->commonst.kind == DO_STMT) {
            if (prev && prev->commonst.kind == EXPR_STMT) {
                Symbol *target = AssignedLocalInt(prev->exprst.exp);
                int value;
                if (target &&
                    EvalConstInt(ctx, prev->exprst.exp->bin.right, &value))
                {
                    int delta = 0;
                    stmt *consumed[16];
                    int consumedCount = 0;

                    if (!TrailingUpdateScan(ctx, s->whilest.body, target,
                                            &delta, consumed,
                                            &consumedCount))
                    {
                    }
                    if (TrailingUpdateScan(ctx, s->whilest.body, target,
                                           &delta, consumed,
                                           &consumedCount))
                    {
                        LoopInit *rec = (LoopInit *)
                            malloc(sizeof(LoopInit));
                        int ii;


                        if (rec) {
                            rec->loop = s;
                            rec->symbol = target;
                            rec->value = value;
                            rec->next = ctx->loopInitHead;
                            ctx->loopInitHead = rec;
                        }
                        ArbLowerMarkConsumedStmt(ctx, prev);
                        for (ii = 0; ii < consumedCount; ii++)
                            ArbLowerMarkConsumedStmt(ctx, consumed[ii]);
                    }
                }
            }
        }
        prev = s;
    }
} // PairWhileInitializers

static LoopInit *FindLoopInit(ArbLowerContext *ctx, stmt *loop)
{
    LoopInit *rec = ctx->loopInitHead;
    while (rec) {
        if (rec->loop == loop)
            return rec;
        rec = rec->next;
    }
    return NULL;
} // FindLoopInit

/*
 * LowerLoopBody() - Simulate the induction values and lower the body once
 *     per iteration with the symbol bound to its static value.
 */

static int LowerLoopBody(ArbLowerContext *ctx, Symbol *induction,
                         const int *values, int count, stmt *body,
                         stmt *claimedStep)
{
    int ii;

    for (ii = 0; ii < count; ii++) {
        StaticPush(ctx, induction, values[ii]);
        if (!ArbLowerStatement(ctx, body)) {
            StaticPop(ctx);
            return 0;
        }
        StaticPop(ctx);
    }
    return 1;
} // LowerLoopBody

/*
 * LowerCanonicalFor() - Recognize and unroll "for (i = c; i <op> n; step)".
 */

int ArbLowerCanonicalFor(ArbLowerContext *ctx, stmt *fStmt)
{
    Symbol *induction = NULL;
    int start, bound, op, delta, found = 0;
    int values[ARB_MAX_UNROLL];
    int count;

    if (fStmt->forst.init && fStmt->forst.init->commonst.kind == EXPR_STMT)
        induction = AssignedLocalInt(fStmt->forst.init->exprst.exp);
    if (!induction)
        goto fail;
    if (!EvalConstInt(ctx, fStmt->forst.init->exprst.exp->bin.right, &start))
        goto fail;
    if (!CanonicalLoopCondition(ctx, fStmt->forst.cond, &induction, &bound, &op))
        goto fail;
    {
        expr *stepExpr = NULL;
        int okStep;
        if (fStmt->forst.step &&
            fStmt->forst.step->commonst.kind == EXPR_STMT)
        {
            stepExpr = fStmt->forst.step->exprst.exp;
        }
        okStep = stepExpr &&
                 ExtractStepDelta(ctx, stepExpr, induction, &delta, found);
        {
            // The step may be a linearized statement list; scan every
            // expression statement for the induction update.
            stmt *ss;
            for (ss = fStmt->forst.step; ss && !found; ss = ss->commonst.next)
            {
                if (ss->commonst.kind != EXPR_STMT || !ss->exprst.exp)
                    continue;
                if (ss->exprst.exp->common.kind == BINARY_N &&
                    (ss->exprst.exp->bin.op == ASSIGN_OP ||
                     ss->exprst.exp->bin.op == ASSIGN_V_OP ||
                     ss->exprst.exp->bin.op == ASSIGN_GEN_OP) &&
                    ss->exprst.exp->bin.left->common.kind == SYMB_N &&
                    ss->exprst.exp->bin.left->sym.symbol == induction)
                {
                    okStep = ExtractSimpleDelta(ctx, ss->exprst.exp,
                                                induction, &delta);
                    found = okStep ? 1 : 0;
                }
            }
        }
        if (!okStep || !found || delta == 0)
            goto fail;
    }
    if (!StepProvesTermination(op, delta))
        goto fail;
    count = SimulateLoopIterations(op, bound, start, delta, 1, values,
                                   ARB_MAX_UNROLL);
    if (count < 0)
        goto fail;
    return LowerLoopBody(ctx, induction, values, count, fStmt->forst.body,
                         NULL);

fail:
    SemanticError(&fStmt->commonst.loc, ERROR___ARB_LOOP_NOT_UNROLLABLE);
    return 0;
} // LowerCanonicalFor

/*
 * LowerCanonicalWhileDo() - Unroll paired while/do loops.  testFirst
 *     distinguishes while from do.  The induction start value comes from
 *     the initializer recorded by PairWhileInitializers.
 */

int ArbLowerCanonicalWhileDo(ArbLowerContext *ctx, stmt *fStmt,
                                 int testFirst)
{
    LoopInit *init = FindLoopInit(ctx, fStmt);
    Symbol *induction;
    int bound, op, delta, found = 0;
    int values[ARB_MAX_UNROLL];
    int count;

    if (!init)
        goto fail;
    induction = init->symbol;
    if (!CanonicalLoopCondition(ctx, fStmt->whilest.cond, &induction, &bound, &op))
        goto fail;
    {
        stmt *body = fStmt->whilest.body;
        stmt *tail[16];
        int tailCount = 0;
        stmt *s2 = body;
        int ii;

        while (s2 && s2->commonst.kind == BLOCK_STMT)
            s2 = s2->blockst.body;
        while (s2) {
            if (tailCount < 16)
                tail[tailCount] = s2;
            tailCount++;
            s2 = s2->commonst.next;
        }
        if (tailCount > 16)
            goto fail;
        ii = tailCount - 1;
        while (ii >= 0 &&
               tail[ii]->commonst.kind == EXPR_STMT &&
               tail[ii]->exprst.exp->common.kind == SYMB_N)
        {
            ii--;
        }
        if (ii < 0 || tail[ii]->commonst.kind != EXPR_STMT)
            goto fail;
        if (!ExtractStepDelta(ctx, tail[ii]->exprst.exp, induction, &delta,
                              &found) || !found || delta == 0)
        {
            goto fail;
        }
    }
    if (!StepProvesTermination(op, delta))
        goto fail;
    count = SimulateLoopIterations(op, bound, init->value, delta, testFirst,
                                   values, ARB_MAX_UNROLL);
    if (count < 0)
        goto fail;
    return LowerLoopBody(ctx, induction, values, count, fStmt->whilest.body,
                         NULL);

fail:
    SemanticError(&fStmt->commonst.loc, ERROR___ARB_LOOP_NOT_UNROLLABLE);
    return 0;
} // LowerCanonicalWhileDo

/*
 * LowerStatement() - Lower one statement list in source order.  While/do
 *     initializers paired during analysis are skipped as compile-time
 *     loop control; canonical for/while/do loops unroll with their
 *     induction symbol bound to simulated constants.
 */

int ArbLowerStatement(ArbLowerContext *ctx, stmt *statement)
{
    ArbLowerPairWhileInitializers(ctx, statement);
    while (statement) {
        if (ArbLowerIsConsumedStmt(ctx, statement)) {
            // Claimed initializer or step: no code.
            statement = statement->commonst.next;
            continue;
        }
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
            // The frontend wraps the condition in a KILL unary, which
            // LowerExpression lowers to KIL for fragment programs.
            {
                ArbOperand sink;
                if (ctx->profile->stage == ARB_STAGE_VERTEX) {
                    SemanticError(&statement->commonst.loc,
                                  ERROR___ARB_VERTEX_DISCARD);
                    return 0;
                }
                if (!ArbLowerExpression(ctx, statement->discardst.cond, &sink))
                    return 0;
            }
            break;        case COMMENT_STMT:
            break;
        case BLOCK_STMT:
            if (!ArbLowerStatement(ctx, statement->blockst.body))
                return 0;
            break;
        case IF_STMT:
            SemanticError(&statement->commonst.loc,
                          ERROR_S_ARB_UNSUPPORTED_OPERATION, "if");
            return 0;
        case FOR_STMT:
            if (!ArbLowerCanonicalFor(ctx, statement))
                return 0;
            break;
        case WHILE_STMT:
            if (!ArbLowerCanonicalWhileDo(ctx, statement, 1))
                return 0;
            break;
        case DO_STMT:
            if (!ArbLowerCanonicalWhileDo(ctx, statement, 0))
                return 0;
            break;
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
 * ArbLowerProgram() - Lower the whole program body.  Returns 0 after any
 *         failed statement or expression lowering.
 */

int ArbLowerProgram(ArbProgram *ir, const ArbProfileDesc *profile,
                    Symbol *program)
{
    ArbLowerContext ctx;

    memset(&ctx, 0, sizeof(ctx));
    ctx.ir = ir;
    ctx.profile = profile;
    ctx.program = program;
    ctx.staticValues = NULL;

    if (!ArbLowerStatement(&ctx, program->details.fun.statements)) {
        ArbLowerClearSymbolTemps(&ctx);
        return 0;
    }
    ArbLowerClearSymbolTemps(&ctx);

    if (profile->stage == ARB_STAGE_VERTEX) {
        // Generic ATTRn and conventional spellings must not mix on one
        // attribute register: compare resolved registers and spellings.
        int regnoName[16];
        int ii;
        ArbInstruction *walk;

        for (ii = 0; ii < 16; ii++)
            regnoName[ii] = 0;
        for (walk = ir->first; walk; walk = walk->next) {
            for (ii = 0; ii < walk->srcCount; ii++) {
                if (walk->src[ii].file == ARB_REG_INPUT &&
                    walk->src[ii].index >= 0 && walk->src[ii].index < 16)
                {
                    int name = walk->src[ii].bindingName;
                    if (regnoName[walk->src[ii].index] == 0)
                        regnoName[walk->src[ii].index] = name;
                    else if (regnoName[walk->src[ii].index] != name)
                    {
                        SemanticError(&program->loc,
                                      ERROR___ARB_ATTRIBUTE_ALIAS);
                        return 0;
                    }
                }
            }
        }

        // Required output: POSITION (register 0) must be written.
        for (walk = ir->first; walk; walk = walk->next) {
            if ((walk->dst.file == ARB_REG_OUTPUT ||
                 (walk->dst.file == ARB_REG_TEMP &&
                  walk->dst.index >= 0)) &&
                walk->dst.file == ARB_REG_OUTPUT &&
                walk->dst.index == 0)
            {
                break;
            }
        }
        if (!walk) {
            SemanticError(&program->loc,
                          ERROR___ARB_REQUIRED_POSITION);
            return 0;
        }
    }
    return 1;
} // ArbLowerProgram

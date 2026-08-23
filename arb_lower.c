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

typedef struct ConsumedStmt_Rec {
    struct ConsumedStmt_Rec *next;
    stmt *stmt;
} ConsumedStmt;

typedef struct LoopInit_Rec {
    struct LoopInit_Rec *next;
    stmt *loop;
    Symbol *symbol;
    int value;
} LoopInit;

typedef struct ArbLowerContext_Rec {
    ArbProgram *ir;
    const ArbProfileDesc *profile;
    Symbol *program;
    struct ArbStaticValue_Rec *staticValues;
    int dstSelValid;        // Target selection order recorded by LowerLValue
    int dstSelWidth;
    int dstSel[4];
    ConsumedStmt *consumedHead; // Statements claimed by loop analysis
    int attrRegno[16];          // Seen vertex attribute registers
    int attrName[16];           // Binding spelling atom per register
    int attrCount;
    LoopInit *loopInitHead;     // Paired while/do initializers
} ArbLowerContext;

#define ARB_MAX_UNROLL 256

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
static void TrackSymbolTemp(ArbLowerContext *ctx, Symbol *symbol);
static int EvalConstInt(ArbLowerContext *ctx, expr *e, int *out);
static ArbOperand SmearOperand(ArbOperand operand);
static int ExtractStepDelta(ArbLowerContext *ctx, expr *e, Symbol *target,
                            int *delta, int *found);

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
 * GetSymbolTemp() - Map a plain scalar or vector local onto a virtual
 *         temporary, allocating it on first use.  Aggregates get a
 *         contiguous block of one temp per quad via GetSymbolTempBlock.
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

static int GetSymbolTempBlock(ArbLowerContext *ctx, Symbol *symbol, int *count)
{
    int base;
    int ii;

    *count = GetQuadRegSize(symbol->type);
    if (*count <= 1) {
        *count = 1;
        return GetSymbolTemp(ctx, symbol);
    }
    if (symbol->tempptr)
        return *(int *) symbol->tempptr;
    base = ctx->ir->numVirtualTemps;
    for (ii = 0; ii < *count; ii++) {
        if (ArbNewTemp(ctx->ir) < 0)
            return -1;
    }
    {
        int *index = (int *) malloc(sizeof(int));
        if (!index)
            return -1;
        *index = base;
        symbol->tempptr = index;
    }
    TrackSymbolTemp(ctx, symbol);
    return base;
} // GetSymbolTempBlock

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

///////////////////////////////////////////////////////////////////////////////////////////////
//////////////////// Statically Evaluable Induction Values and Loops //////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////

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

static int StaticFind(const ArbLowerContext *ctx, Symbol *symbol, int *value)
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
            *out = e->co.val[0].i;
            return 1;
        case BCONST_OP:
            *out = e->co.val[0].i ? 1 : 0;
            return 1;
        default:
            return 0;
        }
    case SYMB_N:
        if (e->sym.op == VARIABLE_OP &&
            GetBase(e->common.type) == TYPE_BASE_INT)
        {
            return StaticFind(ctx, e->sym.symbol, out);
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
 * Consumed statements are compile-time loop control (initializers and
 * step expressions) and must not lower as ordinary assignments.
 */

static void MarkConsumedStmt(ArbLowerContext *ctx, stmt *fStmt)
{
    ConsumedStmt *node = (ConsumedStmt *) malloc(sizeof(ConsumedStmt));
    if (!node)
        return;
    node->stmt = fStmt;
    node->next = ctx->consumedHead;
    ctx->consumedHead = node;
} // MarkConsumedStmt

static int IsConsumedStmt(ArbLowerContext *ctx, stmt *fStmt)
{
    ConsumedStmt *node = ctx->consumedHead;
    while (node) {
        if (node->stmt == fStmt)
            return 1;
        node = node->next;
    }
    return 0;
} // IsConsumedStmt

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
 * UniformQuadBase() - Return the PARAM quad index of a uniform variable
 *         (or -1 when the symbol is not a bound uniform).
 */

static int UniformQuadBase(Symbol *symb)
{
    Binding *fBind = symb->details.var.bind;

    if (!symb->details.var.bind ||
        !(GetDomain(symb->type) & TYPE_DOMAIN_UNIFORM))
    {
        return -1;
    }
    if (fBind->none.kind == BK_REGARRAY &&
        (fBind->none.properties & BIND_IS_BOUND))
    {
        return fBind->reg.regno + (symb->details.var.addr >> 2);
    }
    return -1;
} // UniformQuadBase

/*
 * ExprSame() - Structural equality for expression subtrees, used to
 *     recognize the multiply/add chains that inlined dot products leave.
 */

static int ExprSame(expr *a, expr *b)
{
    if (!a || !b)
        return a == b;
    if (a->common.kind != b->common.kind)
        return 0;
    switch (a->common.kind) {
    case CONST_N:
        return a->co.op == b->co.op && a->co.subop == b->co.subop &&
               memcmp(a->co.val, b->co.val, sizeof(a->co.val)) == 0;
    case SYMB_N:
        return a->sym.op == b->sym.op && a->sym.symbol == b->sym.symbol;
    case UNARY_N:
        return a->un.op == b->un.op && a->un.subop == b->un.subop &&
               ExprSame(a->un.arg, b->un.arg);
    case BINARY_N:
        return a->bin.op == b->bin.op && a->bin.subop == b->bin.subop &&
               ExprSame(a->bin.left, b->bin.left) &&
               ExprSame(a->bin.right, b->bin.right);
    default:
        return 0;
    }
} // ExprSame

/*
 * SwizzleComponentLeaf() - Recognize "base.component" scalar extracts and
 *     report the base subtree and selected component.
 */

static int SwizzleComponentLeaf(expr *e, expr **base, int *comp)
{
    unsigned int packed;

    if (!e || e->common.kind != UNARY_N || e->un.op != SWIZZLE_Z_OP)
        return 0;
    packed = (unsigned int) SUBOP_GET_MASK(e->un.subop);
    if (SUBOP_GET_S2(e->un.subop) > 1 && SUBOP_GET_S2(e->un.subop) != 0)
        return 0;
    *comp = (int) (packed & 3);
    *base = e->un.arg;
    return 1;
} // SwizzleComponentLeaf

typedef struct DotTerm_Rec {
    expr *a;
    expr *b;
    int comp;
} DotTerm;

static int IsMulFamily(int op)
{
    switch (op) {
    case MUL_OP: case MUL_V_OP: case MUL_SV_OP: case MUL_VS_OP:
        return 1;
    default:
        return 0;
    }
} // IsMulFamily

static int IsAddFamily(int op)
{
    switch (op) {
    case ADD_OP: case ADD_V_OP: case ADD_SV_OP: case ADD_VS_OP:
        return 1;
    default:
        return 0;
    }
} // IsAddFamily

/*
 * DetectDotChain() - Match "(a.x*b.x + a.y*b.y + ...)" shapes produced by
 *     inlined dot() calls.  Every term must pair matching components of
 *     two structurally identical source vectors.  On success, *ua and
 *     *ub point at the two source subtrees and *width holds the term
 *     count (3 or 4).
 */

static int DetectDotTermList(expr *e, DotTerm *terms, int *count);

static int DetectDotTerm(expr *e, DotTerm *term)
{
    expr *la, *lb, *ba, *bb;
    int ca, cb;

    if (!e || e->common.kind != BINARY_N || !IsMulFamily(e->bin.op))
        return 0;
    la = e->bin.left;
    lb = e->bin.right;
    if (!SwizzleComponentLeaf(la, &ba, &ca) ||
        !SwizzleComponentLeaf(lb, &bb, &cb))
    {
        return 0;
    }
    if (ca != cb)
        return 0;
    term->a = ba;
    term->b = bb;
    term->comp = ca;
    return 1;
} // DetectDotTerm

static int DetectDotTermList(expr *e, DotTerm *terms, int *count)
{
    if (!e || e->common.kind != BINARY_N)
        return 0;
    if (IsAddFamily(e->bin.op)) {
        int n = *count;
        if (n >= 3)
            return 0;
        if (!DetectDotTermList(e->bin.left, terms, count))
            return 0;
        if (*count >= 4)
            return 0;
        if (!DetectDotTerm(e->bin.right, &terms[*count]))
            return 0;
        (*count)++;
        return 1;
    }
    if (*count != 0)
        return 0;
    if (!DetectDotTerm(e, &terms[0]))
        return 0;
    *count = 1;
    return 1;
} // DetectDotTermList

static int DetectDotChain(expr *e, expr **ua, expr **ub, int *width)
{
    DotTerm terms[4];
    int count = 0;
    int ii;

    if (!DetectDotTermList(e, terms, &count))
        return 0;
    if (count < 1)
        return 0;
    // Every term pairs component i of one source with component i of the
    // other; sources must match across all terms.
    for (ii = 1; ii < count; ii++) {
        if (terms[ii].comp != ii)
            return 0;
        if (!(ExprSame(terms[ii].a, terms[0].a) &&
              ExprSame(terms[ii].b, terms[0].b)) &&
            !(ExprSame(terms[ii].a, terms[0].b) &&
              ExprSame(terms[ii].b, terms[0].a)))
        {
            return 0;
        }
    }
    if (count != 3 && count != 4)
        return 0;
    *ua = terms[0].a;
    *ub = terms[0].b;
    *width = count;
    return 1;
} // DetectDotChain

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
        int base;

        if (ResolveMemberBinding(symb->details.var.bind, operand))
            return 1;
        if (GetDomain(symb->type) & TYPE_DOMAIN_UNIFORM) {
            base = UniformQuadBase(symb);
            if (base < 0) {
                SemanticError(Cg->pLastSourceLoc,
                              ERROR_S_ARB_UNSUPPORTED_OPERATION,
                              opcode_name[VARIABLE_OP]);
                return 0;
            }
            *operand = ArbParamOperand(base);
            if (IsScalar(symb->type)) {
                operand->swizzle[0] = 0;
                operand->swizzle[1] = 0;
                operand->swizzle[2] = 0;
                operand->swizzle[3] = 0;
            }
            return 1;
        }
        // Plain scalar or vector local variable; aggregates resolve to
        // the base of their contiguous temp block.
        if (!IsScalar(symb->type) && !IsVector(symb->type, &len)) {
            int count;
            int base = GetSymbolTempBlock(ctx, symb, &count);
            if (base < 0)
                return 0;
            *operand = ArbTempOperand(base);
            return 1;
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

    // A member of a uniform struct or matrix: add the member's quad
    // offset to the uniform's base register.

    if (expression->bin.left->common.kind == SYMB_N) {
        Symbol *baseSymb = expression->bin.left->sym.symbol;
        int base = UniformQuadBase(baseSymb);
        if (base >= 0) {
            *operand = ArbParamOperand(base + (member->details.var.addr >> 2));
            if (IsScalar(memberType)) {
                operand->swizzle[0] = 0;
                operand->swizzle[1] = 0;
                operand->swizzle[2] = 0;
                operand->swizzle[3] = 0;
            }
            return 1;
        }
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
            if (!LowerLValue(ctx, dstExpr, &dst, &dstMask))
                return 0;
            if (!LowerExpression(ctx, condExpr, &cond))
                return 0;
            if (!LowerExpression(ctx, valExpr, &tv))
                return 0;
            fv = dst; // Old destination contents.
            if (fv.file == ARB_REG_TEMP)
                fv.swizzle[0] = fv.swizzle[1] = fv.swizzle[2] =
                    fv.swizzle[3] = IsScalar(dstType) ? 0 : fv.swizzle[3];
            if (!BuildSelect(ctx, loc, cond, tv, fv,
                             TypeWidth(dstType),
                             MaskFromType(dstType), &sel))
            {
                return 0;
            }
            {
                ArbInstruction *mov =
                    ArbAppendInstruction(ctx, ARB_OP_MOV, loc, dst);
                if (!mov)
                    return 0;
                mov->mask = (unsigned char) MaskFromType(dstType);
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

static void PairWhileInitializers(ArbLowerContext *ctx, stmt *list)
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
                        MarkConsumedStmt(ctx, prev);
                        for (ii = 0; ii < consumedCount; ii++)
                            MarkConsumedStmt(ctx, consumed[ii]);
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
        if (!LowerStatement(ctx, body)) {
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

static int LowerCanonicalFor(ArbLowerContext *ctx, stmt *fStmt)
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

static int LowerCanonicalWhileDo(ArbLowerContext *ctx, stmt *fStmt,
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

static int LowerStatement(ArbLowerContext *ctx, stmt *statement)
{
    PairWhileInitializers(ctx, statement);
    while (statement) {
        if (IsConsumedStmt(ctx, statement)) {
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
                if (!LowerExpression(ctx, statement->discardst.cond, &sink))
                    return 0;
            }
            break;        case COMMENT_STMT:
            break;
        case BLOCK_STMT:
            if (!LowerStatement(ctx, statement->blockst.body))
                return 0;
            break;
        case IF_STMT:
            SemanticError(&statement->commonst.loc,
                          ERROR_S_ARB_UNSUPPORTED_OPERATION, "if");
            return 0;
        case FOR_STMT:
            if (!LowerCanonicalFor(ctx, statement))
                return 0;
            break;
        case WHILE_STMT:
            if (!LowerCanonicalWhileDo(ctx, statement, 1))
                return 0;
            break;
        case DO_STMT:
            if (!LowerCanonicalWhileDo(ctx, statement, 0))
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
 * BuildSelect() - Conditional selection.  ARBVP1 expands the compare-
 *     and-select sequence; ARBFP1 emits a native CMP with the condition
 *     normalized so true values are nonnegative and false negative
 *     (SUB(c, 1)).
 */

static int BuildSelect(ArbLowerContext *ctx, const SourceLoc *loc,
                       ArbOperand cond, ArbOperand tv, ArbOperand fv,
                       int width, int mask, ArbOperand *result)
{
    float one[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
    int cindex;
    ArbOperand oneOp, m, inv, t, f;

    if (width == 1) {
        cond = SmearOperand(cond);
        tv = SmearOperand(tv);
        fv = SmearOperand(fv);
        mask = ARB_MASK_X;
    } else {
        // Conditions are scalar by language rule; smear across lanes.
        cond = SmearOperand(cond);
    }

    if (ctx->profile->stage == ARB_STAGE_FRAGMENT) {
        // CMP dst, c', tv, fv where c' = c - 1:
        // true -> 0 (nonnegative) selects tv; false -> -1 selects fv.
        int ci;
        ArbOperand signedCond;
        ArbInstruction *inst;
        int temp;

        ci = ArbInternConstant(ctx->ir, one, 4);
        if (ci < 0)
            return 0;
        oneOp = ArbConstOperand(ci);
        if (!EmitBinary(ctx, ARB_OP_SUB, mask, loc, cond, oneOp,
                        &signedCond))
        {
            return 0;
        }
        temp = ArbNewTemp(ctx->ir);
        if (temp < 0)
            return 0;
        inst = ArbAppendInstruction(ctx->ir, ARB_OP_CMP, loc,
                                    ArbTempOperand(temp));
        if (!inst)
            return 0;
        inst->mask = (unsigned char) mask;
        if (!ArbAddSource(inst, signedCond) || !ArbAddSource(inst, tv) ||
            !ArbAddSource(inst, fv))
        {
            return 0;
        }
        *result = ArbTempOperand(temp);
        return 1;
    }

    cindex = ArbInternConstant(ctx->ir, one, 4);
    if (cindex < 0)
        return 0;
    oneOp = ArbConstOperand(cindex);
    {
        float zero[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
        int zindex = ArbInternConstant(ctx->ir, zero, 4);
        ArbOperand zeroOp;
        if (zindex < 0)
            return 0;
        zeroOp = ArbConstOperand(zindex);
        if (!EmitBinary(ctx, ARB_OP_SGE, mask, loc, cond, zeroOp, &m))
            return 0;
    }
    if (!EmitBinary(ctx, ARB_OP_SUB, mask, loc, oneOp, m, &inv))
        return 0;
    if (!EmitBinary(ctx, ARB_OP_MUL, mask, loc, tv, m, &t))
        return 0;
    if (!EmitBinary(ctx, ARB_OP_MUL, mask, loc, fv, inv, &f))
        return 0;
    return EmitBinary(ctx, ARB_OP_ADD, mask, loc, t, f, result);
} // BuildSelect

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
    case ARB_BUILTIN_TEX1D: case ARB_BUILTIN_TEX1DPROJ: case ARB_BUILTIN_TEX1DBIAS:
    case ARB_BUILTIN_TEX2D: case ARB_BUILTIN_TEX2DPROJ: case ARB_BUILTIN_TEX2DBIAS:
    case ARB_BUILTIN_TEX3D: case ARB_BUILTIN_TEX3DPROJ: case ARB_BUILTIN_TEX3DBIAS:
    case ARB_BUILTIN_TEXCUBE: case ARB_BUILTIN_TEXCUBEPROJ: case ARB_BUILTIN_TEXCUBEBIAS:
    case ARB_BUILTIN_TEXRECT: case ARB_BUILTIN_TEXRECTPROJ: {
        // tex*(sampler, coord): the first argument must be a sampler
        // parameter with a texture-unit binding.
        ArbOpcode opcode;
        ArbTextureTarget target;
        int minCoord;
        Symbol *sampler;
        Binding *fBind;
        ArbOperand coord;
        ArbInstruction *inst;
        int temp;
        expr *samplerExpr, *coordExpr;

        if (index == ARB_BUILTIN_TEX1D || index == ARB_BUILTIN_TEX1DPROJ ||
            index == ARB_BUILTIN_TEX1DBIAS)
        {
            target = ARB_TEX_1D;
            minCoord = 1;
        } else if (index == ARB_BUILTIN_TEX2D ||
                   index == ARB_BUILTIN_TEX2DPROJ ||
                   index == ARB_BUILTIN_TEX2DBIAS)
        {
            target = ARB_TEX_2D;
            minCoord = 2;
        } else if (index == ARB_BUILTIN_TEX3D ||
                   index == ARB_BUILTIN_TEX3DPROJ ||
                   index == ARB_BUILTIN_TEX3DBIAS)
        {
            target = ARB_TEX_3D;
            minCoord = 3;
        } else if (index == ARB_BUILTIN_TEXCUBE ||
                   index == ARB_BUILTIN_TEXCUBEPROJ ||
                   index == ARB_BUILTIN_TEXCUBEBIAS)
        {
            target = ARB_TEX_CUBE;
            minCoord = 3;
        } else {
            target = ARB_TEX_RECT;
            minCoord = 2;
        }
        if (index == ARB_BUILTIN_TEX1DPROJ || index == ARB_BUILTIN_TEX2DPROJ ||
            index == ARB_BUILTIN_TEX3DPROJ || index == ARB_BUILTIN_TEXCUBEPROJ ||
            index == ARB_BUILTIN_TEXRECTPROJ)
        {
            opcode = ARB_OP_TXP;
        } else if (index == ARB_BUILTIN_TEX1DBIAS ||
                   index == ARB_BUILTIN_TEX2DBIAS ||
                   index == ARB_BUILTIN_TEX3DBIAS ||
                   index == ARB_BUILTIN_TEXCUBEBIAS)
        {
            opcode = ARB_OP_TXB;
        } else {
            opcode = ARB_OP_TEX;
        }

        if (!args || args->bin.op != FUN_ARG_OP) {
            SemanticError(loc, ERROR_S_ARB_UNSUPPORTED_OPERATION,
                          GetAtomString(atable, fun->name));
            return 0;
        }
        samplerExpr = args->bin.left;
        coordExpr = NULL;
        if (args->bin.right && args->bin.right->bin.op == FUN_ARG_OP)
            coordExpr = args->bin.right->bin.left;
        if (samplerExpr->common.kind != SYMB_N ||
            samplerExpr->sym.op != VARIABLE_OP || !coordExpr)
        {
            SemanticError(loc, ERROR_S_ARB_UNSUPPORTED_OPERATION,
                          GetAtomString(atable, fun->name));
            return 0;
        }
        sampler = samplerExpr->sym.symbol;
        fBind = sampler->details.var.bind;
        if (!fBind || fBind->none.kind != BK_TEXUNIT ||
            !(fBind->none.properties & BIND_IS_BOUND))
        {
            SemanticError(loc, ERROR_S_ARB_UNSUPPORTED_OPERATION,
                          GetAtomString(atable, fun->name));
            return 0;
        }
        {
            // Coordinate width must satisfy the target minimum.
            Type *coordType = coordExpr->common.type;
            int w = 0;
            if (!coordType)
                w = 0;
            else if (IsVector(coordType, &w))
                ;
            else if (IsScalar(coordType))
                w = 1;
            else
                w = 0;
            if (!coordType || w < minCoord)
            {
                SemanticError(loc, ERROR_S_ARB_UNSUPPORTED_OPERATION,
                              GetAtomString(atable, fun->name));
                return 0;
            }
        }
        if (!LowerExpression(ctx, coordExpr, &coord))
            return 0;

        temp = ArbNewTemp(ctx->ir);
        if (temp < 0)
            return 0;
        inst = ArbAppendInstruction(ctx->ir, opcode, loc,
                                    ArbTempOperand(temp));
        if (!inst)
            return 0;
        inst->mask = ARB_MASK_XYZW;
        inst->textureUnit = (signed char) fBind->texunit.unitno;
        inst->textureTarget = target;
        if (!ArbAddSource(inst, coord))
            return 0;
        *operand = ArbTempOperand(temp);
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
        {
            // An induction symbol on the simulation stack contributes
            // its compile-time value instead of a register.
            int sval;
            if (StaticFind(ctx, expression->sym.symbol, &sval)) {
                float cv[4];
                int ci;
                cv[0] = (float) sval;
                ci = ArbInternConstant(ctx->ir, cv, 1);
                if (ci < 0)
                    return 0;
                *operand = ArbConstOperand(ci);
                return 1;
            }
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
        case SWIZMAT_Z_OP: {
            // Matrix element selects: each 4-bit code packs (row<<2)|col.
            // Same-row groups read that row vector with a column swizzle.
            unsigned int packed16 =
                (unsigned int) SUBOP_GET_MASK16(expression->un.subop);
            int width = SUBOP_GET_T2(expression->un.subop);
            int codes[4];
            int row0;
            int sameRow = 1;
            int ii;

            if (width <= 0)
                width = 1;
            if (width > 4)
                width = 4;
            for (ii = 0; ii < width; ii++)
                codes[ii] = (int) ((packed16 >> (4 * ii)) & 15);
            row0 = codes[0] >> 2;
            for (ii = 1; ii < width; ii++) {
                if ((codes[ii] >> 2) != row0) {
                    sameRow = 0;
                    break;
                }
            }
            if (!sameRow) {
                SemanticError(loc, ERROR_S_ARB_UNSUPPORTED_OPERATION,
                              "swizmat");
                return 0;
            }
            {
                Symbol *matSymb;
                int base;

                if (expression->un.arg->common.kind == SYMB_N &&
                    expression->un.arg->sym.op == VARIABLE_OP)
                {
                    matSymb = expression->un.arg->sym.symbol;
                } else if (expression->un.arg->common.kind == BINARY_N &&
                           expression->un.arg->bin.op == MEMBER_SELECTOR_OP &&
                           expression->un.arg->bin.left->common.kind == SYMB_N)
                {
                    matSymb = expression->un.arg->bin.left->sym.symbol;
                } else {
                    SemanticError(loc, ERROR_S_ARB_UNSUPPORTED_OPERATION,
                                  "swizmat");
                    return 0;
                }
                base = UniformQuadBase(matSymb);

                if (base >= 0) {
                    *operand = ArbParamOperand(base + row0);
                } else if (matSymb->tempptr) {
                    // Local matrix rows live in consecutive temps.
                    *operand = ArbTempOperand(*(int *) matSymb->tempptr +
                                              row0);
                } else {
                    SemanticError(loc, ERROR_S_ARB_UNSUPPORTED_OPERATION,
                                  "swizmat");
                    return 0;
                }
                {
                    int sel[4];
                    for (ii = 0; ii < 4; ii++) {
                        int cc = codes[ii < width ? ii : width - 1];
                        sel[ii] = cc & 3;
                    }
                    ComposeSwizzledSource(operand, sel, width);
                }
            }
            return 1;
        }
        case VECTOR_V_OP:
            return LowerVectorConstructor(ctx, expression, loc, operand);
        case KILL_OP:
            // Normalized fragment discard: the argument is a 0/1 boolean;
            // convert to signed-negative and emit destination-free KIL.
            if (ctx->profile->stage == ARB_STAGE_VERTEX) {
                SemanticError(loc, ERROR___ARB_VERTEX_DISCARD);
                return 0;
            }
            {
                ArbOperand cond, signedCond;
                float one[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
                int cindex;
                ArbOperand oneOp;
                ArbInstruction *kil;

                if (!LowerExpression(ctx, expression->un.arg, &cond))
                    return 0;
                cond = SmearOperand(cond);
                cindex = ArbInternConstant(ctx->ir, one, 4);
                if (cindex < 0)
                    return 0;
                oneOp = ArbConstOperand(cindex);
                if (!EmitBinary(ctx, ARB_OP_SUB, ARB_MASK_XYZW, loc,
                                cond, oneOp, &signedCond))
                {
                    return 0;
                }
                kil = ArbAppendInstruction(ctx->ir, ARB_OP_KIL, loc, cond);
                if (!kil)
                    return 0;
                kil->mask = ARB_MASK_XYZW;
                if (!ArbAddSource(kil, signedCond))
                    return 0;
                return 1;
            }
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
        case ARRAY_INDEX_OP: {
            // Constant-index reads of uniform arrays (and packed matrix
            // rows) resolve to a PARAM register plus element stride.
            Symbol *baseSymb = NULL;
            int extra = 0;
            expr *baseExpr = expression->bin.left;
            int index;

            for (;;) {
                if (baseExpr->common.kind == SYMB_N) {
                    baseSymb = baseExpr->sym.symbol;
                    break;
                } else if (baseExpr->common.kind == BINARY_N &&
                           baseExpr->bin.op == ARRAY_INDEX_OP)
                {
                    // Multidimensional arrays fold left; accumulate.
                    if (!IsConst(baseExpr->bin.right))
                        break;
                    index = GetConstIndex(baseExpr->bin.right);
                    extra += index *
                             GetQuadRegSize(baseExpr->bin.left->common.type ?
                                            baseExpr->bin.left->common.type :
                                            baseExpr->common.type);
                    baseExpr = baseExpr->bin.left;
                    continue;
                } else {
                    break;
                }
            }
            if (!baseSymb)
            {
                SemanticError(loc, ERROR_S_ARB_UNSUPPORTED_OPERATION,
                              "index");
                return 0;
            }
            {
                int base = UniformQuadBase(baseSymb);
                Type *elType = baseSymb->type;
                ArbOperand idxOp;
                int isConstIndex = 0;
                int constIndex = 0;

                if (base < 0 ||
                    GetCategory(elType) != TYPE_CATEGORY_ARRAY)
                {
                    SemanticError(loc,
                                  ERROR_S_ARB_UNSUPPORTED_OPERATION,
                                  "index");
                    return 0;
                }

                int savedConsts = ctx->ir->numConstants;
                // Lower the index once; a constant result selects the
                // static PARAM register, anything else becomes relative.
                if (LowerExpression(ctx, expression->bin.right, &idxOp))
                {
                    float cv[4];
                    if (idxOp.file == ARB_REG_CONST &&
                        !idxOp.negate && !idxOp.absolute &&
                        idxOp.relative == 0 &&
                        ArbGetConstant(ctx->ir, idxOp.index, cv))
                    {
                        constIndex = (int) cv[0];
                        isConstIndex = 1;
                        ArbTruncateConstants(ctx->ir, savedConsts);
                    }
                }
                else
                {
                    return 0;
                }

                if (isConstIndex)
                {
                    *operand = ArbParamOperand(
                        base + (baseSymb->details.var.addr >> 2) + extra +
                        constIndex *
                            GetQuadRegSize(elType->arr.eltype));
                    return 1;
                }

                if (ctx->profile->stage == ARB_STAGE_FRAGMENT)
                {
                    SemanticError(loc,
                                  ERROR___ARB_FRAGMENT_DYNAMIC_INDEX);
                    return 0;
                }
                {
                    ArbOperand arlDst, sum;
                    int relBase = base + extra;

                    if (relBase < -64 || relBase > 63)
                    {
                        float bv[4];
                        int ci;
                        ArbOperand bOp;
                        bv[0] = (float) relBase;
                        ci = ArbInternConstant(ctx->ir, bv, 1);
                        if (ci < 0)
                            return 0;
                        bOp = SmearOperand(ArbConstOperand(ci));
                        if (!EmitBinary(ctx, ARB_OP_ADD, ARB_MASK_X, loc,
                                        idxOp, bOp, &sum))
                        {
                            return 0;
                        }
                        idxOp = sum;
                        relBase = 0;
                    }
                    arlDst.file = ARB_REG_ADDRESS;
                    arlDst.index = 0;
                    arlDst.bindingName = 0;
                    arlDst.swizzle[0] = 0;
                    arlDst.swizzle[1] = 1;
                    arlDst.swizzle[2] = 2;
                    arlDst.swizzle[3] = 3;
                    arlDst.negate = 0;
                    arlDst.absolute = 0;
                    arlDst.relative = 0;
                    arlDst.relativeOffset = 0;
                    {
                        ArbInstruction *arl =
                            ArbAppendInstruction(ctx->ir, ARB_OP_ARL, loc,
                                                 arlDst);
                        if (!arl)
                            return 0;
                        arl->mask = ARB_MASK_X;
                        if (!ArbAddSource(arl, SmearOperand(idxOp)))
                            return 0;
                    }
                    *operand = ArbParamOperand(relBase);
                    operand->relative = 1;
                    return 1;
                }
            }            SemanticError(loc, ERROR_S_ARB_UNSUPPORTED_OPERATION, "index");
            return 0;
        }
        default:
            break;
        }

        if (RejectUnsupportedBinary(expression))
            return 0;

        // Inlined dot() calls leave multiply/add chains over matching
        // components of two source vectors; emit native DP3/DP4.

        {
            expr *ua, *ub;
            int dwidth;
            ArbOperand u, v;

            if (IsAddFamily(expression->bin.op) &&
                DetectDotChain(expression, &ua, &ub, &dwidth))
            {
                ArbOperand dot;
                if (!LowerExpression(ctx, ua, &u) ||
                    !LowerExpression(ctx, ub, &v))
                {
                    return 0;
                }
                if (!EmitBinary(ctx, dwidth == 4 ? ARB_OP_DP4 : ARB_OP_DP3,
                                ARB_MASK_X, loc, u, v, &dot))
                {
                    return 0;
                }
                *operand = dot;
                return 1;
            }
        }

        lcost = SethiUllmanCost(expression->bin.left);
        rcost = SethiUllmanCost(expression->bin.right);
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

    case TRINARY_N:
        switch (expression->tri.op) {
        case COND_OP:
        case COND_V_OP:
        case COND_SV_OP:
        case COND_GEN_OP: {
            // Both alternatives evaluate unconditionally, then select.
            ArbOperand cond, tv, fv, sel;
            int width = TypeWidth(expression->common.type);
            if (!LowerExpression(ctx, expression->tri.arg1, &cond) ||
                !LowerExpression(ctx, expression->tri.arg2, &tv) ||
                !LowerExpression(ctx, expression->tri.arg3, &fv))
            {
                return 0;
            }
            if (!BuildSelect(ctx, loc, cond, tv, fv, width,
                             WidthMask(width), &sel))
            {
                return 0;
            }
            *operand = sel;
            return 1;
        }
        case ASSIGN_COND_OP:
        case ASSIGN_COND_V_OP:
        case ASSIGN_COND_SV_OP:
        case ASSIGN_COND_GEN_OP:
            // Flattened conditional assignments lower at statement level.
            SemanticError(loc, ERROR_S_ARB_UNSUPPORTED_OPERATION, "assc");
            return 0;
        default:
            SemanticError(loc, ERROR_S_ARB_UNSUPPORTED_OPERATION, "?:");
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

    memset(&ctx, 0, sizeof(ctx));
    ctx.ir = ir;
    ctx.profile = profile;
    ctx.program = program;
    ctx.staticValues = NULL;

    if (!LowerStatement(&ctx, program->details.fun.statements)) {
        ClearSymbolTemps(&ctx);
        return 0;
    }
    ClearSymbolTemps(&ctx);

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

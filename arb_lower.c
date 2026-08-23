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
 *         registers are frontend bookkeeping and are reported through
 *         *IsInputWrite so the caller can elide them.  Everything else
 *         must be a plain local variable.
 */

static int LowerLValue(ArbLowerContext *ctx, expr *expression,
                       ArbOperand *operand, int *mask)
{
    Type *lType;

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
    switch (fExpr->common.kind) {
    case BINARY_N:
        switch (fExpr->bin.op) {
        case ASSIGN_OP:
        case ASSIGN_V_OP:
        case ASSIGN_GEN_OP:
            return LowerAssignment(ctx, fExpr->bin.left, fExpr->bin.right,
                                   ARB_MASK_XYZW, loc);
        case ASSIGN_MASKED_KV_OP:
            return LowerAssignment(ctx, fExpr->bin.left, fExpr->bin.right,
                                   SUBOP_GET_MASK(fExpr->bin.subop), loc);
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
 * LowerExpression() - Lower a readable expression into an operand.
 */

static int LowerExpression(ArbLowerContext *ctx, expr *expression,
                           ArbOperand *operand)
{
    if (!expression)
        return 0;
    switch (expression->common.kind) {
    case SYMB_N:
    case BINARY_N:
        if (expression->common.kind == BINARY_N &&
            expression->bin.op != MEMBER_SELECTOR_OP)
        {
            SemanticError(Cg->tokenLoc, ERROR_S_ARB_UNSUPPORTED_OPERATION,
                          opcode_name[expression->bin.op]);
            return 0;
        }
        return LowerConnectorMember(ctx, expression, operand);
    case CONST_N:
        SemanticError(Cg->tokenLoc, ERROR_S_ARB_UNSUPPORTED_OPERATION,
                      "constant");
        return 0;
    default:
        SemanticError(Cg->tokenLoc, ERROR_S_ARB_UNSUPPORTED_OPERATION,
                      "expression");
        return 0;
    }
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

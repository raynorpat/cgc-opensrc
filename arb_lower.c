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

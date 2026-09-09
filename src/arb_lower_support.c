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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "arb_lower_internal.h"

/*
 * MaskFromType() - Return the destination mask covering the components of
 *         a scalar or vector type.
 */

int ArbLowerMaskFromType(Type *fType)
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

int ArbLowerGetSymbolTemp(ArbLowerContext *ctx, Symbol *symbol)
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

int ArbLowerGetSymbolTempBlock(ArbLowerContext *ctx, Symbol *symbol, int *count)
{
    int base;
    int ii;

    *count = GetQuadRegSize(symbol->type);
    if (*count <= 1) {
        *count = 1;
        return ArbLowerGetSymbolTemp(ctx, symbol);
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
    ArbLowerTrackSymbolTemp(ctx, symbol);
    return base;
} // GetSymbolTempBlock

/*
 * ClearSymbolTemps() - Release every temp-index cell allocated through
 *         GetSymbolTemp and detach it from its symbol.  The compiler's
 *         ClearAllSymbolTempptr() also scrubs expression nodes that may
 *         have cached backend data.
 */

void ArbLowerClearSymbolTemps(ArbLowerContext *ctx)
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

void ArbLowerTrackSymbolTemp(ArbLowerContext *ctx, Symbol *symbol)
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
 * Consumed statements are compile-time loop control (initializers and
 * step expressions) and must not lower as ordinary assignments.
 */

void ArbLowerMarkConsumedStmt(ArbLowerContext *ctx, stmt *fStmt)
{
    ConsumedStmt *node = (ConsumedStmt *) malloc(sizeof(ConsumedStmt));
    if (!node)
        return;
    node->stmt = fStmt;
    node->next = ctx->consumedHead;
    ctx->consumedHead = node;
} // MarkConsumedStmt

int ArbLowerIsConsumedStmt(ArbLowerContext *ctx, stmt *fStmt)
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
 * SmearOperand() - Return a copy of an operand reading its x component
 *         through all four lanes.
 */

ArbOperand ArbLowerSmearOperand(ArbOperand operand)
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

int ArbLowerEmitBinary(ArbLowerContext *ctx, ArbOpcode opcode, int mask,
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

int ArbLowerEmitUnary(ArbLowerContext *ctx, ArbOpcode opcode, int mask,
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

int ArbLowerBuildSelect(ArbLowerContext *ctx, const SourceLoc *loc,
                       ArbOperand cond, ArbOperand tv, ArbOperand fv,
                       int width, int mask, ArbOperand *result)
{
    float one[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
    int cindex;
    ArbOperand oneOp, m, inv, t, f;

    if (width == 1) {
        cond = ArbLowerSmearOperand(cond);
        tv = ArbLowerSmearOperand(tv);
        fv = ArbLowerSmearOperand(fv);
        mask = ARB_MASK_X;
    } else {
        // Conditions are scalar by language rule; smear across lanes.
        cond = ArbLowerSmearOperand(cond);
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
        if (!ArbLowerEmitBinary(ctx, ARB_OP_SUB, mask, loc, cond, oneOp,
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
        if (!ArbLowerEmitBinary(ctx, ARB_OP_SGE, mask, loc, cond, zeroOp, &m))
            return 0;
    }
    if (!ArbLowerEmitBinary(ctx, ARB_OP_SUB, mask, loc, oneOp, m, &inv))
        return 0;
    if (!ArbLowerEmitBinary(ctx, ARB_OP_MUL, mask, loc, tv, m, &t))
        return 0;
    if (!ArbLowerEmitBinary(ctx, ARB_OP_MUL, mask, loc, fv, inv, &f))
        return 0;
    return ArbLowerEmitBinary(ctx, ARB_OP_ADD, mask, loc, t, f, result);
} // BuildSelect

/*
 * TypeWidth() - Component width of a scalar or vector type.
 */

int ArbLowerTypeWidth(Type *fType)
{
    int len;

    if (fType && IsVector(fType, &len))
        return len;
    return 1;
} // TypeWidth

int ArbLowerWidthMask(int width)
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

void ArbLowerComposeSwizzledSource(ArbOperand *operand, const int *selection,
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

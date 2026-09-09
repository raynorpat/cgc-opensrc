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

int ArbLowerUniformQuadBase(Symbol *symb)
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
 * LowerConnectorMember() - Lower a MEMBER_SELECTOR_OP (or bare variable)
 *         that refers to a connector register, uniform, or local value.
 *         Returns 0 when the shape is unsupported.
 */

int ArbLowerConnectorMember(ArbLowerContext *ctx, expr *expression,
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
            base = ArbLowerUniformQuadBase(symb);
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
            int base = ArbLowerGetSymbolTempBlock(ctx, symb, &count);
            if (base < 0)
                return 0;
            *operand = ArbTempOperand(base);
            return 1;
        }
        temp = ArbLowerGetSymbolTemp(ctx, symb);
        if (temp < 0)
            return 0;
        ArbLowerTrackSymbolTemp(ctx, symb);
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
        int base = ArbLowerUniformQuadBase(baseSymb);
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

int ArbLowerLValue(ArbLowerContext *ctx, expr *expression,
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

        if (!ArbLowerLValue(ctx, expression->un.arg, operand, mask))
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
            *mask = ArbLowerMaskFromType(symb->type);
            return 1;
        }
        if (GetDomain(symb->type) & TYPE_DOMAIN_UNIFORM) {
            SemanticError(Cg->pLastSourceLoc, ERROR_S_ARB_UNSUPPORTED_OPERATION,
                          opcode_name[VARIABLE_OP]);
            return 0;
        }
        {
            int temp = ArbLowerGetSymbolTemp(ctx, symb);
            if (temp < 0)
                return 0;
            ArbLowerTrackSymbolTemp(ctx, symb);
            *operand = ArbTempOperand(temp);
            if (IsScalar(symb->type))
                operand->swizzle[0] = operand->swizzle[1] =
                operand->swizzle[2] = operand->swizzle[3] = 0;
            *mask = ArbLowerMaskFromType(symb->type);
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
    if (!ArbLowerConnectorMember(ctx, expression, operand))
        return 0;
    *mask = ArbLowerMaskFromType(lType);
    return 1;
} // LowerLValue

/*
 * LowerSwizzleNode() - SWIZZLE_Z packs each destination lane's source
 *         component as a two-bit code in the subop mask field.
 */

int ArbLowerSwizzleNode(ArbLowerContext *ctx, expr *expression,
                            ArbOperand *operand)
{
    unsigned int packed = (unsigned int) SUBOP_GET_MASK(expression->un.subop);
    int width = SUBOP_GET_S2(expression->un.subop);
    if (width <= 0)
        width = 1;
    int selection[4];
    int ii;

    if (!ArbLowerExpression(ctx, expression->un.arg, operand))
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
    ArbLowerComposeSwizzledSource(operand, selection, width);
    return 1;
} // LowerSwizzleNode

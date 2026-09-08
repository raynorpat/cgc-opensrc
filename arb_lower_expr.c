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
            values[ii] = expression->co.val[0].value.f;
        size = 1;
        break;
    case FCONST_V_OP:
    case HCONST_V_OP:
    case XCONST_V_OP:
        size = SUBOP_GET_S1(expression->co.subop);
        for (ii = 0; ii < 4; ii++)
            values[ii] = expression->co.val[ii < size ? ii : 0].value.f;
        break;
    case ICONST_OP:
    case BCONST_OP:
        for (ii = 0; ii < 4; ii++)
            values[ii] = (float) expression->co.val[0].value.i;
        size = 1;
        break;
    case ICONST_V_OP:
    case BCONST_V_OP:
        size = SUBOP_GET_S1(expression->co.subop);
        for (ii = 0; ii < 4; ii++)
            values[ii] = (float) expression->co.val[ii < size ? ii : 0].value.i;
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
        if (!item || !ArbLowerExpression(ctx, item, &component))
            return 0;
        inst = ArbAppendInstruction(ctx->ir, ARB_OP_MOV, loc,
                                    ArbTempOperand(temp));
        if (!inst)
            return 0;
        inst->mask = (unsigned char) (1 << lane);
        if (!ArbAddSource(inst, ArbLowerSmearOperand(component)))
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

    if (fun->details.fun.intrinsic != NULL) {
        /* Declarative-catalog call: translate the stable intrinsic
         * identity to the equivalent ARB builtin index.  Catalog
         * signatures supersede the profile __internal prototypes for
         * every shared name; bias forms remain catalog-free and keep
         * their legacy group/index classification. */
        switch (fun->details.fun.intrinsic->intrinsic) {
        case CG_INTRINSIC_RSQRT:
            index = ARB_BUILTIN_RSQ;
            break;
        case CG_INTRINSIC_TEX1D:
            index = ARB_BUILTIN_TEX1D;
            break;
        case CG_INTRINSIC_TEX1DPROJ:
            index = ARB_BUILTIN_TEX1DPROJ;
            break;
        case CG_INTRINSIC_TEX2D:
            index = ARB_BUILTIN_TEX2D;
            break;
        case CG_INTRINSIC_TEX2DPROJ:
            index = ARB_BUILTIN_TEX2DPROJ;
            break;
        case CG_INTRINSIC_TEX3D:
            index = ARB_BUILTIN_TEX3D;
            break;
        case CG_INTRINSIC_TEX3DPROJ:
            index = ARB_BUILTIN_TEX3DPROJ;
            break;
        case CG_INTRINSIC_TEXCUBE:
            index = ARB_BUILTIN_TEXCUBE;
            break;
        case CG_INTRINSIC_TEXCUBEPROJ:
            index = ARB_BUILTIN_TEXCUBEPROJ;
            break;
        case CG_INTRINSIC_TEXRECT:
            index = ARB_BUILTIN_TEXRECT;
            break;
        case CG_INTRINSIC_TEXRECTPROJ:
            index = ARB_BUILTIN_TEXRECTPROJ;
            break;
        default:
            SemanticError(loc, ERROR_S_ARB_UNSUPPORTED_OPERATION,
                          GetAtomString(atable, fun->name));
            return 0;
        }
    } else if (fun->details.fun.group != ARB_BUILTIN_GROUP) {
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
        if (!ArbLowerExpression(ctx, argExpr, &arg))
            return 0;
        arg = ArbLowerSmearOperand(arg);
        if (!ArbLowerEmitUnary(ctx, ARB_OP_RSQ, ARB_MASK_X, loc, arg, &result))
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
        if (!ArbLowerExpression(ctx, coordExpr, &coord))
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
    int width = ArbLowerTypeWidth(expression->common.type);
    int mask = ArbLowerWidthMask(width);
    ArbOperand a, b;

    switch (op) {
    case LT_OP: case LT_V_OP: case LT_SV_OP: case LT_VS_OP:
    case GT_OP: case GT_V_OP: case GT_SV_OP: case GT_VS_OP:
    case LE_OP: case LE_V_OP: case LE_SV_OP: case LE_VS_OP:
    case GE_OP: case GE_V_OP: case GE_SV_OP: case GE_VS_OP:
    {
        ArbOpcode opcode;
        int swap = 0;

        if (!ArbLowerExpression(ctx, expression->bin.left, &a) ||
            !ArbLowerExpression(ctx, expression->bin.right, &b))
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
        if (ArbLowerTypeWidth(expression->bin.left->common.type) == 1 && width > 1)
            a = ArbLowerSmearOperand(a);
        if (ArbLowerTypeWidth(expression->bin.right->common.type) == 1 && width > 1)
            b = ArbLowerSmearOperand(b);
        return ArbLowerEmitBinary(ctx, opcode, mask, loc, a, b, operand);
    }
    case EQ_OP: case EQ_V_OP: case EQ_SV_OP: case EQ_VS_OP:
    case NE_OP: case NE_V_OP: case NE_SV_OP: case NE_VS_OP:
    {
        ArbOperand geAB, geBA, eq;
        int isNE = (op == NE_OP || op == NE_V_OP ||
                    op == NE_SV_OP || op == NE_VS_OP);

        if (!ArbLowerExpression(ctx, expression->bin.left, &a) ||
            !ArbLowerExpression(ctx, expression->bin.right, &b))
        {
            return 0;
        }
        if (ArbLowerTypeWidth(expression->bin.left->common.type) == 1 && width > 1)
            a = ArbLowerSmearOperand(a);
        if (ArbLowerTypeWidth(expression->bin.right->common.type) == 1 && width > 1)
            b = ArbLowerSmearOperand(b);
        if (!ArbLowerEmitBinary(ctx, ARB_OP_SGE, mask, loc, a, b, &geAB) ||
            !ArbLowerEmitBinary(ctx, ARB_OP_SGE, mask, loc, b, a, &geBA))
        {
            return 0;
        }
        if (!ArbLowerEmitBinary(ctx, ARB_OP_MUL, mask, loc, geAB, geBA, &eq))
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
            return ArbLowerEmitBinary(ctx, ARB_OP_SUB, mask, loc, oneOp, eq, operand);
        }
    }
    case AND_OP: case AND_V_OP: case AND_SV_OP: case AND_VS_OP:
    case BAND_OP: case BAND_V_OP: case BAND_SV_OP: case BAND_VS_OP:
    {
        if (!ArbLowerExpression(ctx, expression->bin.left, &a) ||
            !ArbLowerExpression(ctx, expression->bin.right, &b))
        {
            return 0;
        }
        return ArbLowerEmitBinary(ctx, ARB_OP_MUL, mask, loc, a, b, operand);
    }
    case OR_OP: case OR_V_OP: case OR_SV_OP: case OR_VS_OP:
    case BOR_OP: case BOR_V_OP: case BOR_SV_OP: case BOR_VS_OP:
    {
        ArbOperand sum;
        float one[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
        int cindex;
        ArbOperand oneOp;

        if (!ArbLowerExpression(ctx, expression->bin.left, &a) ||
            !ArbLowerExpression(ctx, expression->bin.right, &b))
        {
            return 0;
        }
        if (!ArbLowerEmitBinary(ctx, ARB_OP_ADD, mask, loc, a, b, &sum))
            return 0;
        cindex = ArbInternConstant(ctx->ir, one, 4);
        if (cindex < 0)
            return 0;
        oneOp = ArbConstOperand(cindex);
        return ArbLowerEmitBinary(ctx, ARB_OP_MIN, mask, loc, sum, oneOp, operand);
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
    int width = ArbLowerTypeWidth(expression->common.type);
    int mask = ArbLowerWidthMask(width);
    int lwidth = ArbLowerTypeWidth(expression->bin.left->common.type);
    int rwidth = ArbLowerTypeWidth(expression->bin.right->common.type);
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

    if (!ArbLowerExpression(ctx, expression->bin.left, &a) ||
        !ArbLowerExpression(ctx, expression->bin.right, &b))
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
            dividend = ArbLowerSmearOperand(dividend);
        if (rwidth == 1)
            divisor = ArbLowerSmearOperand(divisor);
        if (!ArbLowerEmitUnary(ctx, ARB_OP_RCP, rcpMask, loc, divisor, &rcp))
            return 0;
        if (rwidth == 1)
            rcp = ArbLowerSmearOperand(rcp);
        else if (rwidth < 4 && width > rwidth) {
            int sel[4];
            int ii;
            for (ii = 0; ii < 4; ii++)
                sel[ii] = ii < rwidth ? ii : rwidth - 1;
            ArbLowerComposeSwizzledSource(&rcp, sel, width);
        }
        return ArbLowerEmitBinary(ctx, ARB_OP_MUL, mask, loc, dividend, rcp,
                          operand);
    }

    if ((op == ADD_SV_OP || op == SUB_SV_OP || op == MUL_SV_OP) &&
        lwidth == 1 && width > 1)
    {
        a = ArbLowerSmearOperand(a);
    }
    if ((op == ADD_VS_OP || op == SUB_VS_OP || op == MUL_VS_OP) &&
        rwidth == 1 && width > 1)
    {
        b = ArbLowerSmearOperand(b);
    }
    if (width == 1) {
        a = ArbLowerSmearOperand(a);
        b = ArbLowerSmearOperand(b);
        mask = ARB_MASK_X;
    } else if (lwidth == 1 && rwidth == 1) {
        a = ArbLowerSmearOperand(a);
        b = ArbLowerSmearOperand(b);
    } else if (lwidth < width && lwidth > 1) {
        int sel[4];
        int ii;
        for (ii = 0; ii < 4; ii++)
            sel[ii] = ii < lwidth ? ii : lwidth - 1;
        ArbLowerComposeSwizzledSource(&a, sel, width);
    } else if (rwidth < width && rwidth > 1) {
        int sel[4];
        int ii;
        for (ii = 0; ii < 4; ii++)
            sel[ii] = ii < rwidth ? ii : rwidth - 1;
        ArbLowerComposeSwizzledSource(&b, sel, width);
    }
    return ArbLowerEmitBinary(ctx, opcode, mask, loc, a, b, operand);
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

int ArbLowerExpression(ArbLowerContext *ctx, expr *expression,
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
            if (ArbLowerStaticFind(ctx, expression->sym.symbol, &sval)) {
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
        return ArbLowerConnectorMember(ctx, expression, operand);

    case UNARY_N:
        switch (expression->un.op) {
        case NEG_OP:
        case NEG_V_OP:
            if (!ArbLowerExpression(ctx, expression->un.arg, operand))
                return 0;
            operand->negate = !operand->negate;
            return 1;
        case POS_OP:
        case POS_V_OP:
            return ArbLowerExpression(ctx, expression->un.arg, operand);
        case CAST_CS_OP:
        case CAST_CV_OP:
        case CAST_CM_OP:
            // Numeric casts are value-preserving at this level.
            return ArbLowerExpression(ctx, expression->un.arg, operand);
        case SWIZZLE_Z_OP:
            return ArbLowerSwizzleNode(ctx, expression, operand);
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
                base = ArbLowerUniformQuadBase(matSymb);

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
                    ArbLowerComposeSwizzledSource(operand, sel, width);
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

                if (!ArbLowerExpression(ctx, expression->un.arg, &cond))
                    return 0;
                cond = ArbLowerSmearOperand(cond);
                cindex = ArbInternConstant(ctx->ir, one, 4);
                if (cindex < 0)
                    return 0;
                oneOp = ArbConstOperand(cindex);
                if (!ArbLowerEmitBinary(ctx, ARB_OP_SUB, ARB_MASK_XYZW, loc,
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
            return ArbLowerConnectorMember(ctx, expression, operand);
        case FUN_ARG_OP:
        case EXPR_LIST_OP:
        case COMMA_OP:
            SemanticError(loc, ERROR_S_ARB_UNSUPPORTED_OPERATION, "argument");
            return 0;
        case FUN_CALL_OP:
            SemanticError(loc, ERROR_S_ARB_UNSUPPORTED_OPERATION, "call");
            return 0;
        case FUN_INTRINSIC_OP:
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
                int base = ArbLowerUniformQuadBase(baseSymb);
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
                if (ArbLowerExpression(ctx, expression->bin.right, &idxOp))
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
                        bOp = ArbLowerSmearOperand(ArbConstOperand(ci));
                        if (!ArbLowerEmitBinary(ctx, ARB_OP_ADD, ARB_MASK_X, loc,
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
                        if (!ArbAddSource(arl, ArbLowerSmearOperand(idxOp)))
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
                if (!ArbLowerExpression(ctx, ua, &u) ||
                    !ArbLowerExpression(ctx, ub, &v))
                {
                    return 0;
                }
                if (!ArbLowerEmitBinary(ctx, dwidth == 4 ? ARB_OP_DP4 : ARB_OP_DP3,
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
            int width = ArbLowerTypeWidth(expression->common.type);
            if (!ArbLowerExpression(ctx, expression->tri.arg1, &cond) ||
                !ArbLowerExpression(ctx, expression->tri.arg2, &tv) ||
                !ArbLowerExpression(ctx, expression->tri.arg3, &fv))
            {
                return 0;
            }
            if (!ArbLowerBuildSelect(ctx, loc, cond, tv, fv, width,
                             ArbLowerWidthMask(width), &sel))
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

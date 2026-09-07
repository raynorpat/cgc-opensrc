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
INCIDENTAL, EXEMPLARY, CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, LOST PROFITS; PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) OR ARISING IN
ANY WAY OUT OF THE USE, REPRODUCTION, MODIFICATION AND/OR DISTRIBUTION
OF THE NVIDIA SOFTWARE, HOWEVER CAUSED AND WHETHER UNDER THEORY OF
CONTRACT, TORT (INCLUDING NEGLIGENCE), STRICT LIABILITY OR OTHERWISE,
EVEN IF NVIDIA HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
\****************************************************************************/
// glsl_lower_legacy_expr.c
//

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <float.h>
#include <limits.h>

#include "glsl_lower_internal.h"



GlslExpr *GlslLowerExprChain(GlslLowerContext *context, expr *source,
                                    opcode listOp)
{
    GlslExpr *list;
    GlslExpr *item;

    list = NULL;
    for (; source != NULL; source = source->bin.right) {
        if (source->common.kind != BINARY_N || source->bin.op != listOp) {
            GlslRecordFailure(context, "GLSL expression list");
            return NULL;
        }
        item = GlslLowerExpr(context, source->bin.left);
        if (item == NULL)
            return NULL;
        GlslAppendExpr(&list, item);
    }
    return list;
}

static GlslExpr *GlslLowerTextureArguments(GlslLowerContext *context,
                                           expr *source)
{
    GlslExpr *sampler;
    GlslExpr *coord;
    GlslDecl *decl;
    expr *samplerSource;

    if (source == NULL || source->common.kind != BINARY_N ||
        source->bin.op != FUN_ARG_OP || source->bin.right == NULL ||
        source->bin.right->common.kind != BINARY_N ||
        source->bin.right->bin.op != FUN_ARG_OP ||
        source->bin.right->bin.right != NULL)
    {
        GlslRecordFailureKind(context, GLSL_ERROR_INTRINSIC,
                              "texture intrinsic");
        return NULL;
    }
    samplerSource = source->bin.left;
    if (samplerSource == NULL || samplerSource->common.kind != SYMB_N ||
        samplerSource->sym.op != VARIABLE_OP ||
        samplerSource->sym.symbol == NULL)
    {
        GlslRecordFailureKind(context, GLSL_ERROR_SAMPLER,
            "texture sampler argument must be a direct bound uniform");
        return NULL;
    }
    decl = GlslFindDecl(context, samplerSource->sym.symbol);
    if (decl == NULL || decl->storage != GLSL_STORAGE_SAMPLER ||
        !GlslIsSamplerType(&decl->type))
    {
        GlslRecordFailureKind(context, GLSL_ERROR_SAMPLER,
            "texture sampler argument must be a direct bound uniform");
        return NULL;
    }
    sampler = GlslNewExpr(context->module, GLSL_EXPR_SYMBOL, decl->type);
    if (sampler == NULL)
        return NULL;
    sampler->u.symbol = decl;
    coord = GlslLowerExpr(context, source->bin.right->bin.left);
    if (coord == NULL)
        return NULL;
    sampler->next = coord;
    return sampler;
}

GlslExpr *GlslNewLiteral(GlslLowerContext *context, GlslBase base,
    int intValue, float floatValue)
{
    GlslExprKind kind;
    GlslExpr *target;
    GlslType type;

    type = GlslNumericType(base, 1);
    if (base == GLSL_BASE_FLOAT)
        kind = GLSL_EXPR_FLOAT;
    else if (base == GLSL_BASE_BOOL)
        kind = GLSL_EXPR_BOOL;
    else
        kind = GLSL_EXPR_INT;
    target = GlslNewExpr(context->module, kind, type);
    if (target == NULL)
        return NULL;
    if (kind == GLSL_EXPR_FLOAT)
        target->u.literalFloat = floatValue;
    else if (kind == GLSL_EXPR_BOOL)
        target->u.literalBool = intValue != 0;
    else
        target->u.literalInt = intValue;
    return target;
}

static GlslExpr *GlslLowerConstant(GlslLowerContext *context, expr *source,
                                   const GlslType *type)
{
    GlslExpr *target;
    GlslExpr *item;
    int i;

    if (type->base == GLSL_BASE_FLOAT) {
        for (i = 0; i < type->len; i++) {
            if (source->co.val[i].value.f != source->co.val[i].value.f ||
                source->co.val[i].value.f > FLT_MAX ||
                source->co.val[i].value.f < -FLT_MAX)
            {
                GlslRecordFailure(context,
                                  "non-finite floating-point constant");
                return NULL;
            }
        }
    }

    if (type->len == 1) {
        if (type->base == GLSL_BASE_FLOAT)
            return GlslNewLiteral(context, type->base, 0, source->co.val[0].value.f);
        return GlslNewLiteral(context, type->base, (int) source->co.val[0].value.i, 0.0f);
    }
    target = GlslNewExpr(context->module, GLSL_EXPR_CONSTRUCT, *type);
    if (target == NULL)
        return NULL;
    for (i = 0; i < type->len; i++) {
        if (type->base == GLSL_BASE_FLOAT)
            item = GlslNewLiteral(context, type->base, 0, source->co.val[i].value.f);
        else
            item = GlslNewLiteral(context, type->base, (int) source->co.val[i].value.i, 0.0f);
        if (item == NULL)
            return NULL;
        GlslAppendExpr(&target->u.construct.arguments, item);
    }
    return target;
}

GlslExpr *GlslNewSwizzle(GlslLowerContext *context, GlslExpr *object,
    const GlslType *type, const char *mask)
{
    GlslExpr *target;

    target = GlslNewExpr(context->module, GLSL_EXPR_SWIZZLE, *type);
    if (target != NULL) {
        target->u.swizzle.object = object;
        target->u.swizzle.mask = GlslCopyText(context->module, mask);
        if (target->u.swizzle.mask == NULL)
            return NULL;
    }
    return target;
}

static GlslExpr *GlslLowerSwizzle(GlslLowerContext *context, expr *source,
                                  const GlslType *type)
{
    GlslExpr *object;
    GlslExpr *target;
    char maskText[5];
    int count;
    int i;
    int mask;

    object = GlslLowerExpr(context, source->un.arg);
    if (object == NULL)
        return NULL;
    count = SUBOP_GET_S2(source->un.subop);
    if (count == 0)
        count = 1;
    if (count < 1 || count > 4)
        return NULL;
    mask = SUBOP_GET_MASK(source->un.subop);
    for (i = count - 1; i >= 0; i--)
        maskText[i] = "xyzw"[(mask >> (i * 2)) & 3];
    maskText[count] = '\0';
    if (object->type.len == 1) {
        if (type->len == 1)
            return object;
        target = GlslNewExpr(context->module, GLSL_EXPR_CONSTRUCT, *type);
        if (target != NULL)
            target->u.construct.arguments = object;
        return target;
    }
    return GlslNewSwizzle(context, object, type, maskText);
}

static GlslOperator GlslUnaryOperator(opcode op)
{
    switch (op) {
    case NEG_OP:
    case NEG_V_OP: return GLSL_OP_NEGATE;
    case POS_OP:
    case POS_V_OP: return GLSL_OP_POSITIVE;
    case BNOT_OP:
    case BNOT_V_OP: return GLSL_OP_LOGICAL_NOT;
    default: return GLSL_OP_NONE;
    }
}

static GlslOperator GlslBinaryOperator(opcode op)
{
    switch (op) {
    case ASSIGN_OP:
    case ASSIGN_V_OP:
    case ASSIGN_GEN_OP:
    case ASSIGN_DYN_OP: return GLSL_OP_ASSIGN;
    case MUL_OP: case MUL_V_OP: case MUL_SV_OP: case MUL_VS_OP:
        return GLSL_OP_MULTIPLY;
    case DIV_OP: case DIV_V_OP: case DIV_SV_OP: case DIV_VS_OP:
        return GLSL_OP_DIVIDE;
    case ADD_OP: case ADD_V_OP: case ADD_SV_OP: case ADD_VS_OP:
        return GLSL_OP_ADD;
    case SUB_OP: case SUB_V_OP: case SUB_SV_OP: case SUB_VS_OP:
        return GLSL_OP_SUBTRACT;
    case LT_OP: return GLSL_OP_LESS;
    case GT_OP: return GLSL_OP_GREATER;
    case LE_OP: return GLSL_OP_LESS_EQUAL;
    case GE_OP: return GLSL_OP_GREATER_EQUAL;
    case EQ_OP: return GLSL_OP_EQUAL;
    case NE_OP: return GLSL_OP_NOT_EQUAL;
    case BAND_OP: return GLSL_OP_LOGICAL_AND;
    case BOR_OP: return GLSL_OP_LOGICAL_OR;
    default: return GLSL_OP_NONE;
    }
}

static const char *GlslVectorComparisonName(opcode op)
{
    switch (op) {
    case LT_V_OP: case LT_SV_OP: case LT_VS_OP: return "lessThan";
    case GT_V_OP: case GT_SV_OP: case GT_VS_OP: return "greaterThan";
    case LE_V_OP: case LE_SV_OP: case LE_VS_OP: return "lessThanEqual";
    case GE_V_OP: case GE_SV_OP: case GE_VS_OP: return "greaterThanEqual";
    case EQ_V_OP: case EQ_SV_OP: case EQ_VS_OP: return "equal";
    case NE_V_OP: case NE_SV_OP: case NE_VS_OP: return "notEqual";
    default: return NULL;
    }
}

int GlslValidateTextureCall(GlslLowerContext *context,
    GlslBuiltin builtin, const GlslType *result, GlslExpr *arguments)
{
    GlslType samplerType;
    GlslType coordType;
    GlslType resultType;
    GlslBase samplerBase;
    GlslBinding *binding;
    GlslDecl *decl;
    Symbol *symbol;
    int coordLen;
    int sourceBase;

    if (builtin < GLSL_BUILTIN_TEX1D ||
        builtin > GLSL_BUILTIN_TEXCUBE_PROJ) return 1;
    if (context->profile->stage == GLSL_STAGE_VERTEX) {
        GlslRecordFailureKind(context, GLSL_ERROR_STAGE_OPERATION,
                              "texture sampling");
        return 0;
    }
    switch (builtin) {
    case GLSL_BUILTIN_TEX1D:
        samplerBase = GLSL_BASE_SAMPLER1D;
        sourceBase = TYPE_BASE_GLSL_SAMPLER1D;
        coordLen = 1;
        break;
    case GLSL_BUILTIN_TEX2D:
        samplerBase = GLSL_BASE_SAMPLER2D;
        sourceBase = TYPE_BASE_GLSL_SAMPLER2D;
        coordLen = 2;
        break;
    case GLSL_BUILTIN_TEX3D:
        samplerBase = GLSL_BASE_SAMPLER3D;
        sourceBase = TYPE_BASE_GLSL_SAMPLER3D;
        coordLen = 3;
        break;
    case GLSL_BUILTIN_TEXCUBE:
        samplerBase = GLSL_BASE_SAMPLERCUBE;
        sourceBase = TYPE_BASE_GLSL_SAMPLERCUBE;
        coordLen = 3;
        break;
    case GLSL_BUILTIN_TEX1D_PROJ:
        samplerBase = GLSL_BASE_SAMPLER1D;
        sourceBase = TYPE_BASE_GLSL_SAMPLER1D;
        coordLen = 4;
        break;
    case GLSL_BUILTIN_TEX2D_PROJ:
        samplerBase = GLSL_BASE_SAMPLER2D;
        sourceBase = TYPE_BASE_GLSL_SAMPLER2D;
        coordLen = 4;
        break;
    case GLSL_BUILTIN_TEX3D_PROJ:
        samplerBase = GLSL_BASE_SAMPLER3D;
        sourceBase = TYPE_BASE_GLSL_SAMPLER3D;
        coordLen = 4;
        break;
    case GLSL_BUILTIN_TEXCUBE_PROJ:
        samplerBase = GLSL_BASE_SAMPLERCUBE;
        sourceBase = TYPE_BASE_GLSL_SAMPLERCUBE;
        coordLen = 4;
        break;
    default:
        return 0;
    }
    samplerType = GlslNumericType(samplerBase, 1);
    coordType = GlslNumericType(GLSL_BASE_FLOAT, coordLen);
    resultType = GlslNumericType(GLSL_BASE_FLOAT, 4);
    if (arguments == NULL || arguments->next == NULL ||
        arguments->next->next != NULL ||
        !GlslTypesEqual(&arguments->type, &samplerType) ||
        !GlslTypesEqual(&arguments->next->type, &coordType) ||
        !GlslTypesEqual(result, &resultType))
    {
        GlslRecordFailureKind(context, GLSL_ERROR_INTRINSIC,
                              GlslBuiltinSpelling(builtin));
        return 0;
    }
    if (arguments->kind != GLSL_EXPR_SYMBOL ||
        arguments->u.symbol == NULL)
    {
        GlslRecordFailureKind(context, GLSL_ERROR_SAMPLER,
            "texture sampler argument must be a direct bound uniform");
        return 0;
    }
    decl = arguments->u.symbol;
    symbol = (Symbol *) decl->identity;
    binding = symbol != NULL ?
              GlslFindUniformBinding(context->module, symbol) : NULL;
    if (decl->storage != GLSL_STORAGE_SAMPLER ||
        !GlslTypesEqual(&decl->type, &samplerType) ||
        symbol == NULL || symbol->kind != VARIABLE_S ||
        symbol->type == NULL ||
        GetDomain(symbol->type) != TYPE_DOMAIN_UNIFORM ||
        (GetCategory(symbol->type) != TYPE_CATEGORY_SCALAR &&
         GetCategory(symbol->type) != TYPE_CATEGORY_SAMPLER) ||
        GetBase(symbol->type) != sourceBase ||
        binding == NULL || binding->storage != GLSL_STORAGE_SAMPLER ||
        binding->declaration != decl || binding->name == NULL ||
        decl->name == NULL || strcmp(binding->name, decl->name))
    {
        GlslRecordFailureKind(context, GLSL_ERROR_SAMPLER,
            "texture sampler argument must be a direct bound uniform");
        return 0;
    }
    return 1;
}

/*
 * GlslIntrinsicBuiltin() - Map a stable catalog intrinsic identity to
 *        its exact core GLSL 1.50 builtin.  GLSL_BUILTIN_NONE means the
 *        cataloged intrinsic has no exact GLSL 1.50 lowering and must
 *        reach profile validation.  The Cg spellings map once here by
 *        intrinsic identity, never by source name: lerp to mix, frac to
 *        fract, rsqrt to inversesqrt, saturate to clamp, base tex* to
 *        texture, projected tex*proj to textureProj, and explicit-LOD
 *        identities to textureLod.
 */

GlslBuiltin GlslIntrinsicBuiltin(CgIntrinsic intrinsic)
{
    switch (intrinsic) {
    case CG_INTRINSIC_MUL:       return GLSL_BUILTIN_MUL;
    case CG_INTRINSIC_DOT:       return GLSL_BUILTIN_DOT;
    case CG_INTRINSIC_CROSS:     return GLSL_BUILTIN_CROSS;
    case CG_INTRINSIC_NORMALIZE: return GLSL_BUILTIN_NORMALIZE;
    case CG_INTRINSIC_REFLECT:   return GLSL_BUILTIN_REFLECT;
    case CG_INTRINSIC_REFRACT:   return GLSL_BUILTIN_REFRACT;
    case CG_INTRINSIC_LENGTH:    return GLSL_BUILTIN_LENGTH;
    case CG_INTRINSIC_DISTANCE:  return GLSL_BUILTIN_DISTANCE;
    case CG_INTRINSIC_MIN:       return GLSL_BUILTIN_MIN;
    case CG_INTRINSIC_MAX:       return GLSL_BUILTIN_MAX;
    case CG_INTRINSIC_CLAMP:     return GLSL_BUILTIN_CLAMP;
    case CG_INTRINSIC_ABS:       return GLSL_BUILTIN_ABS;
    case CG_INTRINSIC_SIGN:      return GLSL_BUILTIN_SIGN;
    case CG_INTRINSIC_FLOOR:     return GLSL_BUILTIN_FLOOR;
    case CG_INTRINSIC_CEIL:      return GLSL_BUILTIN_CEIL;
    case CG_INTRINSIC_SQRT:      return GLSL_BUILTIN_SQRT;
    case CG_INTRINSIC_EXP:       return GLSL_BUILTIN_EXP;
    case CG_INTRINSIC_EXP2:      return GLSL_BUILTIN_EXP2;
    case CG_INTRINSIC_LOG:       return GLSL_BUILTIN_LOG;
    case CG_INTRINSIC_LOG2:      return GLSL_BUILTIN_LOG2;
    case CG_INTRINSIC_SIN:       return GLSL_BUILTIN_SIN;
    case CG_INTRINSIC_COS:       return GLSL_BUILTIN_COS;
    case CG_INTRINSIC_TAN:       return GLSL_BUILTIN_TAN;
    case CG_INTRINSIC_ASIN:      return GLSL_BUILTIN_ASIN;
    case CG_INTRINSIC_ACOS:      return GLSL_BUILTIN_ACOS;
    case CG_INTRINSIC_ATAN:      return GLSL_BUILTIN_ATAN;
    case CG_INTRINSIC_RSQRT:     return GLSL_BUILTIN_RSQRT;
    case CG_INTRINSIC_LERP:      return GLSL_BUILTIN_LERP;
    case CG_INTRINSIC_FRAC:      return GLSL_BUILTIN_FRAC;
    case CG_INTRINSIC_SATURATE:  return GLSL_BUILTIN_SATURATE;
    case CG_INTRINSIC_TEX1D:     return GLSL_BUILTIN_TEX1D;
    case CG_INTRINSIC_TEX2D:     return GLSL_BUILTIN_TEX2D;
    case CG_INTRINSIC_TEX3D:     return GLSL_BUILTIN_TEX3D;
    case CG_INTRINSIC_TEXCUBE:   return GLSL_BUILTIN_TEXCUBE;
    case CG_INTRINSIC_TEX1DPROJ: return GLSL_BUILTIN_TEX1D_PROJ;
    case CG_INTRINSIC_TEX2DPROJ: return GLSL_BUILTIN_TEX2D_PROJ;
    case CG_INTRINSIC_TEX3DPROJ: return GLSL_BUILTIN_TEX3D_PROJ;
    case CG_INTRINSIC_TEXCUBEPROJ: return GLSL_BUILTIN_TEXCUBE_PROJ;
    default:
        return GLSL_BUILTIN_NONE;
    }
} // GlslIntrinsicBuiltin

static GlslExpr *GlslLowerCall(GlslLowerContext *context, expr *source,
                               const GlslType *type)
{
    GlslExpr *target;
    GlslExpr *arguments;
    GlslExpr *left;
    GlslExpr *right;
    GlslExpr *zero;
    GlslExpr *one;
    GlslFunction *function;
    Symbol *symbol;
    const char *name;
    GlslBuiltin builtin;
    GlslType scalarType;

    if (source->bin.left == NULL ||
        source->bin.left->common.kind != SYMB_N) return NULL;
    symbol = source->bin.left->sym.symbol;
    function = GlslFindFunction(context->module, symbol);
    if (function != NULL) {
        name = function->name;
    } else if (source->bin.op == FUN_INTRINSIC_OP && symbol != NULL &&
               symbol->kind == FUNCTION_S &&
               (symbol->properties & SYMB_IS_BUILTIN))
    {
        const CgIntrinsicSignature *signature =
            CgIntrinsicSignatureForSymbol(symbol);

        /* Lowering is keyed on the stable intrinsic identity carried by
         * the selected symbol, never on a name lookup.  A cataloged
         * intrinsic without an exact GLSL 1.50 lowering fails profile
         * validation with the existing intrinsic diagnostic. */
        builtin = signature != NULL ?
                  GlslIntrinsicBuiltin(signature->intrinsic) :
                  GLSL_BUILTIN_NONE;
        if (builtin == GLSL_BUILTIN_NONE) {
            GlslRecordFailureKind(context, GLSL_ERROR_INTRINSIC,
                signature != NULL ? signature->name :
                GetAtomString(atable, symbol->name));
            return NULL;
        }
        name = GlslBuiltinSpelling(builtin);
        if (name == NULL)
            return NULL;
    } else {
        return NULL;
    }
    arguments = NULL;
    if (source->bin.right != NULL) {
        if (function == NULL && builtin >= GLSL_BUILTIN_TEX1D &&
            builtin <= GLSL_BUILTIN_TEXCUBE_PROJ)
        {
            arguments = GlslLowerTextureArguments(context,
                                                   source->bin.right);
        } else {
            arguments = GlslLowerExprChain(context, source->bin.right,
                                            FUN_ARG_OP);
        }
        if (arguments == NULL)
            return NULL;
    }
    if (function == NULL) {
        if (!GlslValidateTextureCall(context, builtin, type, arguments))
            return NULL;
        if (builtin == GLSL_BUILTIN_MUL ||
            (builtin == GLSL_BUILTIN_DOT &&
             arguments != NULL && arguments->type.len == 1))
        {
            left = arguments;
            right = left != NULL ? left->next : NULL;
            if (left == NULL || right == NULL || right->next != NULL)
                return NULL;
            left->next = NULL;
            right->next = NULL;
            target = GlslNewExpr(context->module, GLSL_EXPR_BINARY, *type);
            if (target == NULL)
                return NULL;
            target->u.binary.op = GLSL_OP_MULTIPLY;
            target->u.binary.left = left;
            target->u.binary.right = right;
            return target;
        }
        if (builtin == GLSL_BUILTIN_SATURATE) {
            if (arguments == NULL || arguments->next != NULL)
                return NULL;
            scalarType = GlslNumericType(GLSL_BASE_FLOAT, 1);
            zero = GlslNewLiteral(context, GLSL_BASE_FLOAT, 0, 0.0f);
            one = GlslNewLiteral(context, GLSL_BASE_FLOAT, 0, 1.0f);
            if (zero == NULL || one == NULL)
                return NULL;
            if (type->len > 1) {
                target = GlslNewExpr(context->module,
                                     GLSL_EXPR_CONSTRUCT, *type);
                if (target == NULL)
                    return NULL;
                target->u.construct.arguments = zero;
                zero = target;
                target = GlslNewExpr(context->module,
                                     GLSL_EXPR_CONSTRUCT, *type);
                if (target == NULL)
                    return NULL;
                target->u.construct.arguments = one;
                one = target;
            } else if (!GlslTypesEqual(type, &scalarType)) {
                return NULL;
            }
            arguments->next = zero;
            zero->next = one;
        }
    }
    target = GlslNewExpr(context->module, GLSL_EXPR_CALL, *type);
    if (target == NULL)
        return NULL;
    target->u.call.name = name;
    target->u.call.arguments = arguments;
    target->u.call.builtin = function == NULL ? builtin : GLSL_BUILTIN_NONE;
    return target;
}

static GlslExpr *GlslLowerVectorComparison(GlslLowerContext *context,
    expr *source, const GlslType *type, const char *name)
{
    GlslExpr *target;
    GlslExpr *constructor;
    GlslExpr *left;
    GlslExpr *right;
    GlslType vectorType;

    left = GlslLowerExpr(context, source->bin.left);
    right = GlslLowerExpr(context, source->bin.right);
    if (left == NULL || right == NULL)
        return NULL;
    if (left->type.len == 1 && type->len > 1) {
        vectorType = GlslNumericType(left->type.base, type->len);
        constructor = GlslNewExpr(context->module, GLSL_EXPR_CONSTRUCT,
                                  vectorType);
        if (constructor == NULL)
            return NULL;
        constructor->u.construct.arguments = left;
        left = constructor;
    }
    if (right->type.len == 1 && type->len > 1) {
        vectorType = GlslNumericType(right->type.base, type->len);
        constructor = GlslNewExpr(context->module, GLSL_EXPR_CONSTRUCT,
                                  vectorType);
        if (constructor == NULL)
            return NULL;
        constructor->u.construct.arguments = right;
        right = constructor;
    }
    target = GlslNewExpr(context->module, GLSL_EXPR_CALL, *type);
    if (target == NULL)
        return NULL;
    target->u.call.name = name;
    target->u.call.arguments = left;
    left->next = right;
    return target;
}

static GlslExpr *GlslLowerMaskedAssignment(GlslLowerContext *context,
    expr *source, const GlslType *type)
{
    GlslExpr *target;
    GlslExpr *left;
    GlslExpr *right;
    GlslType maskedType;
    char maskText[5];
    int mask;
    int i;
    int count;

    left = GlslLowerExpr(context, source->bin.left);
    right = GlslLowerExpr(context, source->bin.right);
    if (left == NULL || right == NULL)
        return NULL;
    mask = SUBOP_GET_MASK(source->bin.subop);
    count = 0;
    for (i = 0; i < 4; i++) {
        if (mask & (1 << i))
            maskText[count++] = "xyzw"[i];
    }
    maskText[count] = '\0';
    if (count == 0)
        return NULL;
    maskedType = GlslNumericType(left->type.base, count);
    left = GlslNewSwizzle(context, left, &maskedType, maskText);
    if (left == NULL)
        return NULL;
    target = GlslNewExpr(context->module, GLSL_EXPR_BINARY, *type);
    if (target != NULL) {
        target->u.binary.op = GLSL_OP_ASSIGN;
        target->u.binary.left = left;
        target->u.binary.right = right;
    }
    return target;
}

static GlslExpr *GlslLowerComponent(GlslLowerContext *context, expr *source,
    int component, GlslBase base)
{
    GlslExpr *target;
    GlslType type;
    int len;
    char mask[2];

    target = GlslLowerExpr(context, source);
    if (target == NULL)
        return NULL;
    if (!IsVector(source->common.type, &len) || len <= 1)
        return target;
    type = GlslNumericType(base, 1);
    mask[0] = "xyzw"[component];
    mask[1] = '\0';
    return GlslNewSwizzle(context, target, &type, mask);
}

static GlslExpr *GlslLowerConditional(GlslLowerContext *context,
    expr *source, const GlslType *type)
{
    GlslExpr *target;
    GlslExpr *componentExpr;
    GlslExpr *condition;
    GlslExpr *trueExpr;
    GlslExpr *falseExpr;
    GlslType componentType;
    int conditionLen;
    int i;

    conditionLen = 0;
    IsVector(source->tri.arg1->common.type, &conditionLen);
    if (conditionLen <= 1) {
        target = GlslNewExpr(context->module, GLSL_EXPR_CONDITIONAL, *type);
        if (target == NULL)
            return NULL;
        target->u.conditional.condition = GlslLowerExpr(context,
                                                        source->tri.arg1);
        target->u.conditional.trueExpr = GlslLowerExpr(context,
                                                       source->tri.arg2);
        target->u.conditional.falseExpr = GlslLowerExpr(context,
                                                        source->tri.arg3);
        if (target->u.conditional.condition == NULL ||
            target->u.conditional.trueExpr == NULL ||
            target->u.conditional.falseExpr == NULL) return NULL;
        return target;
    }
    if (type->len < 2 || type->len > 4 || conditionLen != type->len)
        return NULL;
    target = GlslNewExpr(context->module, GLSL_EXPR_CONSTRUCT, *type);
    if (target == NULL)
        return NULL;
    componentType = GlslNumericType(type->base, 1);
    for (i = 0; i < type->len; i++) {
        condition = GlslLowerComponent(context, source->tri.arg1, i,
                                       GLSL_BASE_BOOL);
        trueExpr = GlslLowerComponent(context, source->tri.arg2, i,
                                      type->base);
        falseExpr = GlslLowerComponent(context, source->tri.arg3, i,
                                       type->base);
        if (condition == NULL || trueExpr == NULL || falseExpr == NULL)
            return NULL;
        componentExpr = GlslNewExpr(context->module,
            GLSL_EXPR_CONDITIONAL, componentType);
        if (componentExpr == NULL)
            return NULL;
        componentExpr->u.conditional.condition = condition;
        componentExpr->u.conditional.trueExpr = trueExpr;
        componentExpr->u.conditional.falseExpr = falseExpr;
        GlslAppendExpr(&target->u.construct.arguments, componentExpr);
    }
    return target;
}

GlslExpr *GlslLowerExpr(GlslLowerContext *context, expr *source)
{
    GlslExpr *target;
    GlslExpr *operand;
    GlslDecl *decl;
    GlslType type;
    GlslOperator op;
    Symbol *member;
    const char *comparison;

    if (source == NULL ||
        !GlslLowerType(context, source->common.type, &type, NULL))
    {
        GlslRecordFailureKind(context, GLSL_ERROR_UNSUPPORTED_TYPE,
                              "GLSL profile expression type");
        return NULL;
    }
    if (GlslIsSamplerType(&type)) {
        GlslRecordFailureKind(context, GLSL_ERROR_SAMPLER,
                              "opaque sampler expression");
        return NULL;
    }
    if (source->common.kind == SYMB_N && source->sym.op == VARIABLE_OP) {
        decl = GlslFindDecl(context, source->sym.symbol);
        if (decl == NULL)
            return NULL;
        target = GlslNewExpr(context->module, GLSL_EXPR_SYMBOL, type);
        if (target != NULL)
            target->u.symbol = decl;
        return target;
    }
    if (source->common.kind == CONST_N)
        return GlslLowerConstant(context, source, &type);
    if (source->common.kind == UNARY_N) {
        if (source->un.op == SWIZZLE_Z_OP)
            return GlslLowerSwizzle(context, source, &type);
        if (source->un.op == SWIZMAT_Z_OP)
            return GlslLowerMatrixSwizzle(context, source, &type);
        if (source->un.op == VECTOR_V_OP && type.rows != 0)
            return GlslLowerMatrixConstructor(context, source, &type);
        if (source->un.op == VECTOR_V_OP) {
            target = GlslNewExpr(context->module, GLSL_EXPR_CONSTRUCT, type);
            if (target == NULL)
                return NULL;
            target->u.construct.arguments = GlslLowerExprChain(context,
                source->un.arg, EXPR_LIST_OP);
            if (target->u.construct.arguments == NULL)
                return NULL;
            return target;
        }
        if (source->un.op == CAST_CS_OP || source->un.op == CAST_CV_OP ||
            source->un.op == CAST_CM_OP)
        {
            operand = GlslLowerExpr(context, source->un.arg);
            if (operand == NULL)
                return NULL;
            if (GlslTypesEqual(&operand->type, &type))
                return operand;
            target = GlslNewExpr(context->module, GLSL_EXPR_CONSTRUCT, type);
            if (target != NULL)
                target->u.construct.arguments = operand;
            return target;
        }
        if (source->un.op == BNOT_V_OP) {
            operand = GlslLowerExpr(context, source->un.arg);
            if (operand == NULL)
                return NULL;
            target = GlslNewExpr(context->module, GLSL_EXPR_CALL, type);
            if (target != NULL) {
                target->u.call.name = "not";
                target->u.call.arguments = operand;
            }
            return target;
        }
        op = GlslUnaryOperator(source->un.op);
        if (op != GLSL_OP_NONE) {
            target = GlslNewExpr(context->module, GLSL_EXPR_UNARY, type);
            if (target == NULL)
                return NULL;
            target->u.unary.op = op;
            target->u.unary.operand = GlslLowerExpr(context, source->un.arg);
            if (target->u.unary.operand == NULL)
                return NULL;
            return target;
        }
    }
    if (source->common.kind == BINARY_N) {
        if (source->bin.op == MEMBER_SELECTOR_OP) {
            if (source->bin.right == NULL ||
                source->bin.right->common.kind != SYMB_N ||
                source->bin.right->sym.op != MEMBER_OP) return NULL;
            member = source->bin.right->sym.symbol;
            if (source->bin.left != NULL &&
                source->bin.left->common.kind == SYMB_N &&
                source->bin.left->sym.op == VARIABLE_OP &&
                (source->bin.left->sym.symbol == Cg->theHAL->varyingIn ||
                 source->bin.left->sym.symbol == Cg->theHAL->varyingOut))
            {
                decl = GlslLowerInterface(context, member);
                if (decl == NULL)
                    return NULL;
                target = GlslNewExpr(context->module, GLSL_EXPR_SYMBOL, type);
                if (target != NULL)
                    target->u.symbol = decl;
                return target;
            }
            decl = GlslFindDecl(context, member);
            if (decl == NULL)
                return NULL;
            target = GlslNewExpr(context->module, GLSL_EXPR_MEMBER, type);
            if (target == NULL)
                return NULL;
            target->u.member.object = GlslLowerExpr(context, source->bin.left);
            if (target->u.member.object == NULL)
                return NULL;
            target->u.member.decl = decl;
            target->u.member.name = decl->name;
            return target;
        }
        if (source->bin.op == ARRAY_INDEX_OP) {
            target = GlslNewExpr(context->module, GLSL_EXPR_INDEX, type);
            if (target == NULL)
                return NULL;
            target->u.index.object = GlslLowerExpr(context, source->bin.left);
            target->u.index.index = GlslLowerExpr(context, source->bin.right);
            if (target->u.index.object == NULL || target->u.index.index == NULL)
                return NULL;
            return target;
        }
        if (source->bin.op == FUN_CALL_OP ||
            source->bin.op == FUN_INTRINSIC_OP)
            return GlslLowerCall(context, source, &type);
        if ((source->bin.op == ASSIGN_OP ||
             source->bin.op == ASSIGN_V_OP ||
             source->bin.op == ASSIGN_GEN_OP ||
             source->bin.op == ASSIGN_DYN_OP ||
             source->bin.op == ASSIGN_MASKED_KV_OP) &&
            GlslMatrixSelectorCount(source->bin.left) > 1)
        {
            GlslRecordFailure(context,
                              "matrix selector assignment context");
            return NULL;
        }
        if (source->bin.op == ASSIGN_MASKED_KV_OP)
            return GlslLowerMaskedAssignment(context, source, &type);
        comparison = GlslVectorComparisonName(source->bin.op);
        if (comparison != NULL)
            return GlslLowerVectorComparison(context, source, &type,
                                              comparison);
        op = GlslBinaryOperator(source->bin.op);
        if (op != GLSL_OP_NONE) {
            target = GlslNewExpr(context->module, GLSL_EXPR_BINARY, type);
            if (target == NULL)
                return NULL;
            target->u.binary.op = op;
            target->u.binary.left = GlslLowerExpr(context, source->bin.left);
            target->u.binary.right = GlslLowerExpr(context, source->bin.right);
            if (target->u.binary.left == NULL ||
                target->u.binary.right == NULL) return NULL;
            return target;
        }
    }
    if (source->common.kind == TRINARY_N &&
        (source->tri.op == COND_OP || source->tri.op == COND_V_OP ||
         source->tri.op == COND_SV_OP || source->tri.op == COND_GEN_OP))
    {
        return GlslLowerConditional(context, source, &type);
    }
    GlslRecordFailure(context, GlslUnsupportedExprReason(source));
    return NULL;
}

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
// glsl_lower_ir_expr.c
//

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <float.h>
#include <limits.h>

#include "glsl_lower_internal.h"



///////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////// Cg IR lowering (Cg 2.0 sources) //////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////

/*
 * Everything below consumes a verified Cg IR module (cg_ir.h) instead
 * of the frontend tree: declarations come from the module's ordered
 * globals/functions/DECL statements, expressions switch over
 * CgIRExprKind, and statements over CgIRStmtKind.  Uniform bindings,
 * default values, and resource limits keep reading the HAL binding
 * metadata exactly as the legacy path did: the reach-filtered IR
 * intentionally omits unreferenced declarations, while the GLSL
 * interface must stay silent about them and still emit every bound
 * uniform (documented unused-uniform silence).
 *
 * Three synthesized IR shapes are recognized and re-collapsed into the
 * historical output forms here:
 *   - matrix group reads whose shared object has side effects lower to
 *     the cg_get_matN helper call;
 *   - matrix group writes (optionally preceded by the Task 16 object or
 *     value temporaries) lower back to one cg_set_matN call or to the
 *     per-component scalar fan-out between two plain variables;
 *   - aggregate assignments flatten member-wise with cg_index /
 *     cg_aggregate temporaries, mirroring the legacy transform pass
 *     that used to run ahead of tree lowering.
 */

static GlslExpr *GlslIRLowerExprList(GlslLowerContext *context,
                                     const CgIRExpr *args);

/*
 * GlslIRUnaryOperator() / GlslIRBinaryOperator() - Cg IR op identity to
 *          the GLSL writer's operator enum.
 */

static GlslOperator GlslIRUnaryOperator(CgIROp op)
{
    switch (op) {
    case CGIR_OP_NEGATE: return GLSL_OP_NEGATE;
    case CGIR_OP_POSITIVE: return GLSL_OP_POSITIVE;
    case CGIR_OP_LOGICAL_NOT: return GLSL_OP_LOGICAL_NOT;
    default: return GLSL_OP_NONE;
    }
} // GlslIRUnaryOperator

static GlslOperator GlslIRBinaryOperator(CgIROp op)
{
    switch (op) {
    case CGIR_OP_MULTIPLY: return GLSL_OP_MULTIPLY;
    case CGIR_OP_DIVIDE: return GLSL_OP_DIVIDE;
    case CGIR_OP_ADD: return GLSL_OP_ADD;
    case CGIR_OP_SUBTRACT: return GLSL_OP_SUBTRACT;
    case CGIR_OP_LESS: return GLSL_OP_LESS;
    case CGIR_OP_GREATER: return GLSL_OP_GREATER;
    case CGIR_OP_LESS_EQUAL: return GLSL_OP_LESS_EQUAL;
    case CGIR_OP_GREATER_EQUAL: return GLSL_OP_GREATER_EQUAL;
    case CGIR_OP_EQUAL: return GLSL_OP_EQUAL;
    case CGIR_OP_NOT_EQUAL: return GLSL_OP_NOT_EQUAL;
    case CGIR_OP_LOGICAL_AND: return GLSL_OP_LOGICAL_AND;
    case CGIR_OP_LOGICAL_OR: return GLSL_OP_LOGICAL_OR;
    default: return GLSL_OP_NONE;
    }
} // GlslIRBinaryOperator

static const char *GlslIRComparisonName(CgIROp op)
{
    switch (op) {
    case CGIR_OP_LESS: return "lessThan";
    case CGIR_OP_GREATER: return "greaterThan";
    case CGIR_OP_LESS_EQUAL: return "lessThanEqual";
    case CGIR_OP_GREATER_EQUAL: return "greaterThanEqual";
    case CGIR_OP_EQUAL: return "equal";
    case CGIR_OP_NOT_EQUAL: return "notEqual";
    default: return NULL;
    }
} // GlslIRComparisonName

/*
 * GlslIRUnsupportedReason() - Historical unsupported-operation text for
 *          the operator families outside the focused GLSL profile.
 */

static const char *GlslIRUnsupportedReason(CgIROp op)
{
    switch (op) {
    case CGIR_OP_MODULO:
    case CGIR_OP_MODULO_ASSIGN:
        return "remainder (%)";
    case CGIR_OP_SHIFT_LEFT:
    case CGIR_OP_SHIFT_RIGHT:
        return "shift operator";
    case CGIR_OP_BITWISE_AND:
    case CGIR_OP_BITWISE_XOR:
    case CGIR_OP_BITWISE_OR:
    case CGIR_OP_BITWISE_NOT:
        return "bitwise operator";
    default:
        return NULL;
    }
} // GlslIRUnsupportedReason

static GlslOperator GlslIRCompoundOperator(CgIROp op)
{
    switch (op) {
    case CGIR_OP_ADD_ASSIGN: return GLSL_OP_ADD;
    case CGIR_OP_SUBTRACT_ASSIGN: return GLSL_OP_SUBTRACT;
    case CGIR_OP_MULTIPLY_ASSIGN: return GLSL_OP_MULTIPLY;
    case CGIR_OP_DIVIDE_ASSIGN: return GLSL_OP_DIVIDE;
    default: return GLSL_OP_NONE;
    }
} // GlslIRCompoundOperator

/*
 * GlslIRSharedSelection() - Recognize the Task 16 group-read encoding:
 *          a constructor whose components are nested constant selections
 *      over ONE shared object node.  Node sharing identifies synthesized
 *          selector groups; user-written duplicates never share nodes.
 */

int GlslIRSharedSelection(const CgIRExpr *expr,
                                 const CgIRExpr **objectOut,
                                 int *countOut, int *maskOut)
{
    const CgIRExpr *argument;
    const CgIRExpr *object;
    int count;
    int mask;
    int i;

    if (expr == NULL || expr->kind != CGIR_EXPR_CONSTRUCT)
        return 0;
    count = 0;
    for (argument = expr->u.construct.arguments; argument != NULL;
         argument = argument->next)
        count++;
    if (count < 2 || count > 4)
        return 0;
    object = NULL;
    mask = 0;
    i = 0;
    for (argument = expr->u.construct.arguments; argument != NULL;
         argument = argument->next, i++)
    {
        const CgIRExpr *rowSel;
        const CgIRExpr *columnSel;
        int row;
        int column;

        if (argument->kind != CGIR_EXPR_INDEX)
            return 0;
        columnSel = argument->u.index.index;
        if (columnSel == NULL || columnSel->kind != CGIR_EXPR_CONSTANT ||
            columnSel->u.constant.kind != CG_SCALAR_INT)
            return 0;
        rowSel = argument->u.index.object;
        if (rowSel == NULL || rowSel->kind != CGIR_EXPR_INDEX ||
            rowSel->u.index.index == NULL ||
            rowSel->u.index.index->kind != CGIR_EXPR_CONSTANT ||
            rowSel->u.index.index->u.constant.kind != CG_SCALAR_INT)
        {
            return 0;
        }
        if (object == NULL) {
            object = rowSel->u.index.object;
        } else if (object != rowSel->u.index.object) {
            return 0;
        }
        row = (int) rowSel->u.index.index->u.constant.value.i;
        column = (int) columnSel->u.constant.value.i;
        if (row < 0 || row > 3 || column < 0 || column > 3) return 0;
        mask |= ((row << 2) | column) << (i * 4);
    }
    if (object == NULL)
        return 0;
    *objectOut = object;
    *countOut = count;
    *maskOut = mask;
    return 1;
} // GlslIRSharedSelection

static GlslExpr *GlslIRConstantComponent(GlslLowerContext *context,
                                         const CgNumericValue *value,
                                         GlslBase base)
{
    float component;

    if (base == GLSL_BASE_FLOAT) {
        component = (float) value->value.f;

        if (!GlslFiniteDefaultFloat(component)) {
            GlslRecordFailure(context,
                              "non-finite floating-point constant");
            return NULL;
        }
        return GlslNewLiteral(context, base, 0, component);
    }
    return GlslNewLiteral(context, base, (int) value->value.i, 0.0f);
} // GlslIRConstantComponent

static GlslExpr *GlslIRLowerConstant(GlslLowerContext *context,
                                     const CgIRExpr *expr)
{
    GlslType type;

    if (!GlslIRType(context, expr->type, &type, &expr->loc))
        return NULL;
    return GlslIRConstantComponent(context, &expr->u.constant, type.base);
} // GlslIRLowerConstant

static GlslExpr *GlslIRLowerSwizzle(GlslLowerContext *context,
                                    const CgIRExpr *expr)
{
    GlslExpr *object;
    GlslType type;
    char maskText[5];
    int count;
    int i;

    object = GlslIRLowerExpr(context, expr->u.swizzle.object);
    if (object == NULL)
        return NULL;
    if (!GlslIRType(context, expr->type, &type, &expr->loc))
        return NULL;
    count = expr->u.swizzle.componentCount;
    if (count < 1 || count > 4)
        return NULL;
    for (i = 0; i < count; i++)
        maskText[i] = "xyzw"[(expr->u.swizzle.mask >> (i * 2)) & 3];
    maskText[count] = '\0';
    if (object->type.len == 1) {
        GlslExpr *target;

        if (type.len == 1)
            return object;
        target = GlslNewExpr(context->module, GLSL_EXPR_CONSTRUCT, type);
        if (target != NULL)
            target->u.construct.arguments = object;
        return target;
    }
    return GlslNewSwizzle(context, object, &type, maskText);
} // GlslIRLowerSwizzle

/*
 * GlslIRLowerMatrixConstructor() - Row-major source arguments into a
 *          column-major GLSL constructor, or the impure-helper call.
 */

static GlslExpr *GlslIRLowerMatrixConstructor(GlslLowerContext *context,
                                              const CgIRExpr *expr,
                                              const GlslType *type)
{
    GlslExpr *arguments[GLSL_MATRIX_MAX_ARGUMENTS];
    GlslExpr *argument;
    GlslExpr *next;
    GlslExpr *target;
    GlslExpr *component;
    GlslType scalarType;
    const CgIRExpr *sourceArgument;
    char mask[2];
    int size;
    int count;
    int componentIndex;
    int componentCount;
    int row;
    int column;
    int hasSideEffects;

    size = type->rows;
    if (size < 2 || size > 4 || type->cols != size)
        return NULL;
    hasSideEffects = 0;
    for (sourceArgument = expr->u.construct.arguments;
         sourceArgument != NULL;
         sourceArgument = sourceArgument->next)
    {
        if (sourceArgument->sideEffects)
            hasSideEffects = 1;
    }
    argument = GlslIRLowerExprList(context, expr->u.construct.arguments);
    if (argument == NULL && expr->u.construct.arguments != NULL)
        return NULL;
    /* Keep impure expressions at the constructor call site and use each
       exactly once; argument evaluation order remains language-defined. */
    if (hasSideEffects)
        return GlslLowerImpureMatrixConstructor(context, argument, type);
    count = 0;
    while (argument != NULL) {
        next = argument->next;
        argument->next = NULL;
        componentCount = argument->type.len;
        if (!GlslMatrixNumericParameterType(&argument->type) ||
            count > size * size - componentCount)
        {
            if (!GlslMatrixNumericParameterType(&argument->type)) {
                GlslRecordFailureKind(context,
                                      GLSL_ERROR_UNSUPPORTED_TYPE,
                                      "matrix constructor argument type");
            }
            return NULL;
        }
        if (componentCount == 1) {
            arguments[count++] = argument;
        } else {
            for (componentIndex = 0;
                 componentIndex < componentCount;
                 componentIndex++)
            {
                scalarType = GlslNumericType(argument->type.base, 1);
                mask[0] = "xyzw"[componentIndex];
                mask[1] = '\0';
                component = GlslNewSwizzle(context, argument,
                                           &scalarType, mask);
                if (component == NULL)
                    return NULL;
                arguments[count++] = component;
            }
        }
        argument = next;
    }
    if (count != size * size)
        return NULL;
    target = GlslNewExpr(context->module, GLSL_EXPR_CONSTRUCT, *type);
    if (target == NULL)
        return NULL;
    for (column = 0; column < size; column++) {
        for (row = 0; row < size; row++) {
            GlslAppendExpr(&target->u.construct.arguments,
                           arguments[row * size + column]);
        }
    }
    return target;
} // GlslIRLowerMatrixConstructor

static GlslExpr *GlslIRLowerExprList(GlslLowerContext *context,
                                     const CgIRExpr *args)
{
    GlslExpr *list;
    GlslExpr *item;

    list = NULL;
    for (; args != NULL; args = args->next) {
        item = GlslIRLowerExpr(context, args);
        if (item == NULL)
            return NULL;
        GlslAppendExpr(&list, item);
    }
    return list;
} // GlslIRLowerExprList

/*
 * GlslIRLowerTextureArguments() - tex* intrinsics keep their historical
 *          shape: a direct bound sampler uniform followed by the
 *          coordinate.
 */

static GlslExpr *GlslIRLowerTextureArguments(GlslLowerContext *context,
                                             const CgIRExpr *args)
{
    GlslExpr *sampler;
    GlslExpr *coord;
    GlslDecl *decl;

    if (args == NULL || args->next == NULL || args->next->next != NULL) {
        GlslRecordFailureKind(context, GLSL_ERROR_INTRINSIC,
                              "texture intrinsic");
        return NULL;
    }
    if (args->kind != CGIR_EXPR_SYMBOL ||
        args->u.symbol == NULL)
    {
        GlslRecordFailureKind(context, GLSL_ERROR_SAMPLER,
            "texture sampler argument must be a direct bound uniform");
        return NULL;
    }
    decl = GlslFindDecl(context, args->u.symbol);
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
    coord = GlslIRLowerExpr(context, args->next);
    if (coord == NULL)
        return NULL;
    sampler->next = coord;
    return sampler;
} // GlslIRLowerTextureArguments

/*
 * GlslIRLowerCallCore() - User helper calls and catalog intrinsics.
 *      The mul/dot-scalar and saturate special shapes keep their
 *      legacy expansions; texture families keep their direct-uniform
 *          sampler rule through GlslValidateTextureCall above.
 */

static GlslExpr *GlslIRLowerCallCore(GlslLowerContext *context,
                                     Symbol *symbol,
                                     const CgIntrinsicSignature *signature,
                                     const CgIRExpr *argumentSource,
                                     const GlslType *type)
{
    GlslExpr *target;
    GlslExpr *arguments;
    GlslExpr *left;
    GlslExpr *right;
    GlslExpr *zero;
    GlslExpr *one;
    GlslFunction *function;
    const char *name;
    GlslBuiltin builtin;
    GlslType scalarType;

    function = symbol != NULL ?
               GlslFindFunction(context->module, symbol) : NULL;
    builtin = GLSL_BUILTIN_NONE;
    if (function != NULL) {
        name = function->name;
    } else {
        /* Lowering is keyed on the stable intrinsic identity carried by
         * the selected signature, never on a name lookup.  A cataloged
         * intrinsic without an exact GLSL 1.50 lowering fails profile
         * validation with the existing intrinsic diagnostic. */
        builtin = signature != NULL ?
                  GlslIntrinsicBuiltin(signature->intrinsic) :
                  GLSL_BUILTIN_NONE;
        if (builtin == GLSL_BUILTIN_NONE) {
            GlslRecordFailureKind(context, GLSL_ERROR_INTRINSIC,
                signature != NULL ? signature->name :
                symbol != NULL ? GetAtomString(atable, symbol->name) :
                                 NULL);
            return NULL;
        }
        name = GlslBuiltinSpelling(builtin);
        if (name == NULL)
            return NULL;
    }
    arguments = NULL;
    if (argumentSource != NULL) {
        if (function == NULL && builtin >= GLSL_BUILTIN_TEX1D &&
            builtin <= GLSL_BUILTIN_TEXCUBE_PROJ)
        {
            arguments = GlslIRLowerTextureArguments(context,
                                                    argumentSource);
        } else {
            arguments = GlslIRLowerExprList(context, argumentSource);
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
} // GlslIRLowerCallCore

static GlslExpr *GlslIRLowerVectorComparison(GlslLowerContext *context,
                                             const CgIRExpr *expr,
                                             const GlslType *type,
                                             const char *name)
{
    GlslExpr *target;
    GlslExpr *constructor;
    GlslExpr *left;
    GlslExpr *right;
    GlslType vectorType;

    left = GlslIRLowerExpr(context, expr->u.binary.left);
    right = left ? GlslIRLowerExpr(context, expr->u.binary.right) : NULL;
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
} // GlslIRLowerVectorComparison

static GlslExpr *GlslIRLowerComponent(GlslLowerContext *context,
                                      const CgIRExpr *source, int component,
                                      GlslBase base)
{
    GlslExpr *target;
    GlslType type;
    int len;
    char mask[2];

    target = GlslIRLowerExpr(context, source);
    if (target == NULL)
        return NULL;
    len = 0;
    if (source->type == NULL || !IsVector(source->type, &len) || len <= 1)
        return target;
    type = GlslNumericType(base, 1);
    mask[0] = "xyzw"[component];
    mask[1] = '\0';
    return GlslNewSwizzle(context, target, &type, mask);
} // GlslIRLowerComponent

static GlslExpr *GlslIRLowerConditional(GlslLowerContext *context,
                                        const CgIRExpr *expr,
                                        const GlslType *type)
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
    IsVector(expr->u.conditional.condition->type, &conditionLen);
    if (conditionLen <= 1) {
        target = GlslNewExpr(context->module, GLSL_EXPR_CONDITIONAL, *type);
        if (target == NULL)
            return NULL;
        target->u.conditional.condition = GlslIRLowerExpr(
            context, expr->u.conditional.condition);
        target->u.conditional.trueExpr = GlslIRLowerExpr(
            context, expr->u.conditional.trueExpr);
        target->u.conditional.falseExpr = GlslIRLowerExpr(
            context, expr->u.conditional.falseExpr);
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
        condition = GlslIRLowerComponent(context,
                                         expr->u.conditional.condition, i,
                                         GLSL_BASE_BOOL);
        trueExpr = GlslIRLowerComponent(context,
                                        expr->u.conditional.trueExpr, i,
                                        type->base);
        falseExpr = GlslIRLowerComponent(context,
                                         expr->u.conditional.falseExpr, i,
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
} // GlslIRLowerConditional

/*
 * GlslIRLowerIncrement() - Legacy expansion order: ++A became A = A + 1
 *          before tree lowering saw it; the same shape is rebuilt here.
 */

static GlslExpr *GlslIRLowerIncrement(GlslLowerContext *context,
                                      const CgIRExpr *expr,
                                      const GlslType *type)
{
    GlslExpr *target;
    GlslExpr *operand;
    GlslExpr *one;
    GlslExpr *arithmetic;
    GlslOperator op;

    op = expr->u.unary.op == CGIR_OP_PRE_INCREMENT ||
         expr->u.unary.op == CGIR_OP_POST_INCREMENT ?
         GLSL_OP_ADD : GLSL_OP_SUBTRACT;
    operand = GlslIRLowerExpr(context, expr->u.unary.operand);
    if (operand == NULL)
        return NULL;
    one = GlslNewLiteral(context, operand->type.base, 1, 0.0f);
    if (one == NULL)
        return NULL;
    arithmetic = GlslNewExpr(context->module, GLSL_EXPR_BINARY,
                             operand->type);
    if (arithmetic == NULL)
        return NULL;
    arithmetic->u.binary.op = op;
    arithmetic->u.binary.left = operand;
    arithmetic->u.binary.right = one;
    target = GlslNewExpr(context->module, GLSL_EXPR_BINARY, *type);
    if (target == NULL)
        return NULL;
    target->u.binary.op = GLSL_OP_ASSIGN;
    target->u.binary.left = GlslCloneExpr(context->module, operand);
    if (target->u.binary.left == NULL)
        return NULL;
    target->u.binary.right = arithmetic;
    return target;
} // GlslIRLowerIncrement

/*
 * GlslIRLowerExpr() - One Cg IR expression to the GLSL expression tree.
 *      Every decision reads the canonical type and the stable node
 *          kind; no four-bit subop is ever decoded here.
 */

GlslExpr *GlslIRLowerExpr(GlslLowerContext *context,
                                 const CgIRExpr *expr)
{
    GlslExpr *target;
    GlslExpr *operand;
    GlslExpr *inner;
    GlslDecl *decl;
    GlslType type;
    GlslOperator op;
    const CgIRExpr *sharedObject;
    int sharedCount;
    int sharedMask;
    int rows;
    int cols;

    if (expr == NULL)
        return NULL;
    if (!GlslIRType(context, expr->type, &type, &expr->loc)) {
        GlslRecordFailureKind(context, GLSL_ERROR_UNSUPPORTED_TYPE,
                              "GLSL profile expression type");
        return NULL;
    }
    if (GlslIsSamplerType(&type)) {
        GlslRecordFailureKind(context, GLSL_ERROR_SAMPLER,
                              "opaque sampler expression");
        return NULL;
    }
    switch (expr->kind) {
    case CGIR_EXPR_SYMBOL:
        target = GlslIRGeometryBuiltinArray(context, expr, &type);
        if (target != NULL)
            return target;
        decl = GlslFindDecl(context, expr->u.symbol);
        if (decl == NULL)
            return NULL;
        target = GlslNewExpr(context->module, GLSL_EXPR_SYMBOL, type);
        if (target != NULL)
            target->u.symbol = decl;
        return target;
    case CGIR_EXPR_CONSTANT:
        return GlslIRLowerConstant(context, expr);
    case CGIR_EXPR_MEMBER:
        if (expr->u.member.object != NULL &&
            expr->u.member.object->kind == CGIR_EXPR_SYMBOL &&
            expr->u.member.object->u.symbol != NULL &&
            (expr->u.member.object->u.symbol == Cg->theHAL->varyingIn ||
             expr->u.member.object->u.symbol ==
                 Cg->theHAL->varyingOut))
        {
            decl = GlslLowerInterface(context, expr->u.member.member);
            if (decl == NULL)
                return NULL;
            target = GlslNewExpr(context->module, GLSL_EXPR_SYMBOL, type);
            if (target != NULL)
                target->u.symbol = decl;
            return target;
        }
        decl = GlslFindDecl(context, expr->u.member.member);
        if (decl == NULL)
            return NULL;
        target = GlslNewExpr(context->module, GLSL_EXPR_MEMBER, type);
        if (target == NULL)
            return NULL;
        target->u.member.object = GlslIRLowerExpr(
            context, expr->u.member.object);
        if (target->u.member.object == NULL)
            return NULL;
        target->u.member.decl = decl;
        target->u.member.name = decl->name;
        return target;
    case CGIR_EXPR_INDEX:
        if (expr->u.index.object != NULL &&
            expr->u.index.object->kind == CGIR_EXPR_SYMBOL)
        {
            const CgIRDecl *param;

            param = GlslIRGeometryEntryParameter(
                context, expr->u.index.object->u.symbol);
            if (param != NULL && CgIsAttribArray(param->type) &&
                GlslGeometryInputMemberName(context->profile,
                                            param->semantic) != NULL)
            {
                GlslExpr *indexExpr;

                indexExpr = GlslIRLowerExpr(context,
                                            expr->u.index.index);
                return GlslIRGeometryBuiltinElement(context, param,
                    indexExpr, &type, &expr->loc);
            }
        }
        /* A two-level constant-index chain over a square matrix that
         * the Task 16 producer marked as a selector component (_mRC)
         * prints transposed (GLSL m[col][row]).  Genuinely explicit
         * source indexing builds the identical shape without the mark;
         * it takes the generic path below and prints as written, so
         * silent transposition of an explicit chain is impossible (the
         * marker is the single-producer invariant -- every chain the
         * `_m` lowering synthesizes carries it, nothing else may). */
        if (expr->selectorRead &&
            expr->u.index.object != NULL &&
            expr->u.index.object->kind == CGIR_EXPR_INDEX &&
            expr->u.index.object->u.index.object != NULL &&
            expr->u.index.index != NULL &&
            expr->u.index.index->kind == CGIR_EXPR_CONSTANT &&
            expr->u.index.index->u.constant.kind == CG_SCALAR_INT &&
            expr->u.index.object->u.index.index != NULL &&
            expr->u.index.object->u.index.index->kind ==
                CGIR_EXPR_CONSTANT &&
            expr->u.index.object->u.index.index->u.constant.kind ==
                CG_SCALAR_INT &&
            expr->u.index.object->u.index.object->type != NULL &&
            IsMatrix(expr->u.index.object->u.index.object->type,
                     &cols, &rows) && cols == rows)
        {
            int selRow = (int) expr->u.index.object->u.index.index->
                         u.constant.value.i;
            int selColumn = (int) expr->u.index.index->u.constant.value.i;
            GlslExpr *base;

            if (selRow < 0 || selRow > 3 || selColumn < 0 ||
                selColumn > 3 || rows < 2 || rows > 4)
                return NULL;
            base = GlslIRLowerExpr(context,
                                   expr->u.index.object->u.index.object);
            if (base == NULL)
                return NULL;
            return GlslIRMatrixElement(context, base, selRow, selColumn);
        }
        target = GlslNewExpr(context->module, GLSL_EXPR_INDEX, type);
        if (target == NULL)
            return NULL;
        target->u.index.object = GlslIRLowerExpr(context,
                                                 expr->u.index.object);
        target->u.index.index = target->u.index.object ?
            GlslIRLowerExpr(context, expr->u.index.index) : NULL;
        if (target->u.index.object == NULL ||
            target->u.index.index == NULL) return NULL;
        return target;
    case CGIR_EXPR_LENGTH:
        /* The legacy path had no array-length lowering either; the
         * generic unsupported-expression reason keeps that behavior. */
        GlslRecordFailure(context, "GLSL profile expression");
        return NULL;
    case CGIR_EXPR_SWIZZLE:
        return GlslIRLowerSwizzle(context, expr);
    case CGIR_EXPR_CONSTRUCT:
        if (type.rows != 0)
            return GlslIRLowerMatrixConstructor(context, expr, &type);
        if (GlslIRSharedSelection(expr, &sharedObject, &sharedCount,
                                  &sharedMask))
        {
            /* Task 16 selector group read: rebuild the historical form.
             * A side-effecting object evaluates once through the
             * cg_get_matN helper; a pure object prints the transposed
             * component constructor. */
            GlslMatrixSelectorHelper *helper;
            GlslType matrixType;
            GlslExpr *baseGlsl;
            int baseEffects;

            if (!GlslIRType(context, sharedObject->type, &matrixType,
                            &expr->loc))
                return NULL;
            baseEffects = GlslIRNeedsMaterialization(sharedObject);
            baseGlsl = GlslIRLowerExpr(context, sharedObject);
            if (baseGlsl == NULL)
                return NULL;
            if (sharedObject->sideEffects || baseEffects) {
                helper = GlslGetMatrixSelectorHelper(context,
                    GLSL_MATRIX_SELECTOR_GET, &matrixType, &type,
                    sharedCount, sharedMask);
                if (helper == NULL)
                    return NULL;
                target = GlslNewExpr(context->module, GLSL_EXPR_CALL,
                                     type);
                if (target == NULL)
                    return NULL;
                target->u.call.name = helper->function->name;
                target->u.call.arguments = baseGlsl;
                return target;
            }
            /* Pure group read: one lowered base, transposed selections
             * (Cg m[row][col] prints as GLSL m[col][row]), each on its
             * own clone exactly like the legacy constructor path. */
            target = GlslNewExpr(context->module, GLSL_EXPR_CONSTRUCT,
                                 type);
            if (target == NULL)
                return NULL;
            {
                int k;

                for (k = 0; k < sharedCount; k++) {
                    /* Frontend nibble packing: row<<2 | column. */
                    int column = (sharedMask >> (k * 4)) & 3;
                    int row = ((sharedMask >> (k * 4)) >> 2) & 3;
                    GlslExpr *leaf;

                    leaf = GlslCloneExpr(context->module, baseGlsl);
                    leaf = leaf ? GlslIRMatrixElement(context, leaf,
                                                       row, column)
                                : NULL;
                    if (leaf == NULL)
                        return NULL;
                    GlslAppendExpr(&target->u.construct.arguments, leaf);
                }
            }
            return target;
        }
        target = GlslNewExpr(context->module, GLSL_EXPR_CONSTRUCT, type);
        if (target == NULL)
            return NULL;
        target->u.construct.arguments = GlslIRLowerExprList(
            context, expr->u.construct.arguments);
        if (target->u.construct.arguments == NULL &&
            expr->u.construct.arguments != NULL)
            return NULL;
        return target;
    case CGIR_EXPR_CAST:
        operand = GlslIRLowerExpr(context, expr->u.cast.operand);
        if (operand == NULL)
            return NULL;
        if (GlslTypesEqual(&operand->type, &type))
            return operand;
        /* Legacy ConstantFoldNode folded casts of scalar constants into
         * the converted literal — in MAIN only (helpers kept their raw
         * casts); reproduce that scope.  Fold-coverage asymmetry: this
         * is the ONLY cast fold.  Vector/matrix constant construction
         * and constant comparisons stay unfolded on the IR path even
         * though legacy folded them ahead of printing -- no pinned
         * golden observes those forms, so they are left raw until one
         * does (see the arithmetic-fold note in CGIR_EXPR_BINARY). */
        if (context->function != NULL && context->function->isEntry &&
            expr->u.cast.operand != NULL &&
            expr->u.cast.operand->kind == CGIR_EXPR_CONSTANT)
        {
            CgNumericValue folded = expr->u.cast.operand->u.constant;
            CgScalarKind targetKind = GetScalarKind(expr->type);

            if (IsScalar(expr->type) && targetKind != CG_SCALAR_NONE &&
                CgNumericConvert(&folded, targetKind,
                                 &expr->u.cast.operand->u.constant) &&
                GlslIRScalarBase(context, targetKind, &type.base,
                                 &expr->loc))
            {
                return GlslIRConstantComponent(context, &folded,
                                               type.base);
            }
        }
        target = GlslNewExpr(context->module, GLSL_EXPR_CONSTRUCT, type);
        if (target != NULL)
            target->u.construct.arguments = operand;
        return target;
    case CGIR_EXPR_UNARY:
        switch (expr->u.unary.op) {
        case CGIR_OP_NEGATE:
        case CGIR_OP_POSITIVE:
            op = GlslIRUnaryOperator(expr->u.unary.op);
            target = GlslNewExpr(context->module, GLSL_EXPR_UNARY, type);
            if (target == NULL)
                return NULL;
            target->u.unary.op = op;
            target->u.unary.operand = GlslIRLowerExpr(
                context, expr->u.unary.operand);
            if (target->u.unary.operand == NULL)
                return NULL;
            return target;
        case CGIR_OP_LOGICAL_NOT:
            operand = GlslIRLowerExpr(context, expr->u.unary.operand);
            if (operand == NULL)
                return NULL;
            if (type.len > 1) {
                /* Vector '!' lowers to the not() builtin call exactly
                 * as the tree path emitted it. */
                target = GlslNewExpr(context->module, GLSL_EXPR_CALL,
                                     type);
                if (target != NULL) {
                    target->u.call.name = "not";
                    target->u.call.arguments = operand;
                }
                return target;
            }
            target = GlslNewExpr(context->module, GLSL_EXPR_UNARY, type);
            if (target == NULL)
                return NULL;
            target->u.unary.op = GLSL_OP_LOGICAL_NOT;
            target->u.unary.operand = operand;
            return target;
        case CGIR_OP_PRE_INCREMENT:
        case CGIR_OP_POST_INCREMENT:
        case CGIR_OP_PRE_DECREMENT:
        case CGIR_OP_POST_DECREMENT:
            return GlslIRLowerIncrement(context, expr, &type);
        default:
            GlslRecordFailure(context,
                              GlslIRUnsupportedReason(expr->u.unary.op));
            return NULL;
        }
    case CGIR_EXPR_ASSIGN:
        switch (expr->u.assign.op) {
        case CGIR_OP_ASSIGN:
            /* A vector value can never print into a scalar component
             * target ("m[0][0] = v"): well-typed user code cannot
             * build that assign, so reaching it means a producer
             * group-write run fell back elementwise -- fail loudly
             * instead of emitting invalid GLSL. */
            if (expr->u.assign.value != NULL &&
                expr->u.assign.value->type != NULL &&
                IsScalar(expr->type) &&
                IsVector(expr->u.assign.value->type, NULL))
            {
                GlslRecordFailure(context, "GLSL profile expression");
                return NULL;
            }
            target = GlslNewExpr(context->module, GLSL_EXPR_BINARY, type);
            if (target == NULL)
                return NULL;
            target->u.binary.op = GLSL_OP_ASSIGN;
            target->u.binary.left = GlslIRLowerExpr(
                context, expr->u.assign.target);
            target->u.binary.right = target->u.binary.left ?
                GlslIRLowerExpr(context, expr->u.assign.value) : NULL;
            if (target->u.binary.left == NULL ||
                target->u.binary.right == NULL) return NULL;
            return target;
        case CGIR_OP_ADD_ASSIGN:
        case CGIR_OP_SUBTRACT_ASSIGN:
        case CGIR_OP_MULTIPLY_ASSIGN:
        case CGIR_OP_DIVIDE_ASSIGN:
            /* Legacy order: A op= B expanded to A = A op B before the
             * tree path ever saw it. */
            op = GlslIRCompoundOperator(expr->u.assign.op);
            operand = GlslIRLowerExpr(context, expr->u.assign.target);
            inner = NULL;
            target = NULL;
            if (operand != NULL) {
                inner = GlslNewExpr(context->module, GLSL_EXPR_BINARY,
                                    operand->type);
                if (inner != NULL) {
                    inner->u.binary.op = op;
                    inner->u.binary.left = GlslCloneExpr(context->module,
                                                         operand);
                    inner->u.binary.right = inner->u.binary.left ?
                        GlslIRLowerExpr(context,
                                        expr->u.assign.value) : NULL;
                }
            }
            if (inner != NULL && inner->u.binary.left != NULL &&
                inner->u.binary.right != NULL)
            {
                target = GlslNewExpr(context->module, GLSL_EXPR_BINARY,
                                     type);
                if (target != NULL) {
                    target->u.binary.op = GLSL_OP_ASSIGN;
                    target->u.binary.left = operand;
                    target->u.binary.right = inner;
                }
            }
            return target;
        default:
            GlslRecordFailure(context,
                              GlslIRUnsupportedReason(expr->u.assign.op));
            return NULL;
        }
    case CGIR_EXPR_BINARY:
        if (GlslIRUnsupportedReason(expr->u.binary.op) != NULL) {
            GlslRecordFailure(context,
                              GlslIRUnsupportedReason(expr->u.binary.op));
            return NULL;
        }
        op = GlslIRBinaryOperator(expr->u.binary.op);
        if (op == GLSL_OP_NONE) {
            GlslRecordFailure(context, "GLSL profile expression");
            return NULL;
        }
        if ((op == GLSL_OP_LESS || op == GLSL_OP_GREATER ||
             op == GLSL_OP_LESS_EQUAL || op == GLSL_OP_GREATER_EQUAL ||
             op == GLSL_OP_EQUAL || op == GLSL_OP_NOT_EQUAL) &&
            type.len > 1)
        {
            return GlslIRLowerVectorComparison(context, expr, &type,
                GlslIRComparisonName(expr->u.binary.op));
        }
        if ((op == GLSL_OP_ADD || op == GLSL_OP_SUBTRACT ||
             op == GLSL_OP_MULTIPLY || op == GLSL_OP_DIVIDE) &&
            context->function != NULL && context->function->isEntry &&
            expr->u.binary.left != NULL &&
            expr->u.binary.left->kind == CGIR_EXPR_CONSTANT &&
            expr->u.binary.right != NULL &&
            expr->u.binary.right->kind == CGIR_EXPR_CONSTANT &&
            IsScalar(expr->type))
        {
            /* Fold-coverage asymmetry vs legacy ConstantFoldNode (it
             * ran on the frontend tree ahead of printing): this is the
             * ONLY arithmetic fold reproduced.  What folds: scalar
             * (+,-,*,/) over two constants, entry function only
             * (division by zero yields the infinity that non-finite
             * rejection pins).  What intentionally stays UNFOLDED:
             * vector/matrix constant arithmetic (emitted as literal
             * operator expressions) and every constant comparison --
             * scalar or the lessThan family above -- because no pinned
             * golden output observes those forms; folding them is left
             * until a golden demands it.  Helpers never fold, matching
             * legacy's MAIN-only scope. */
            CgNumericValue folded;
            CgNumericOp numop = op == GLSL_OP_ADD ? CG_NUMERIC_ADD :
                                op == GLSL_OP_SUBTRACT ? CG_NUMERIC_SUB :
                                op == GLSL_OP_MULTIPLY ?
                                                       CG_NUMERIC_MUL :
                                                       CG_NUMERIC_DIV;

            if (CgNumericBinary(&folded, numop,
                                &expr->u.binary.left->u.constant,
                                &expr->u.binary.right->u.constant))
            {
                return GlslIRConstantComponent(context, &folded,
                                               type.base);
            }
        }
        target = GlslNewExpr(context->module, GLSL_EXPR_BINARY, type);
        if (target == NULL)
            return NULL;
        target->u.binary.op = op;
        target->u.binary.left = GlslIRLowerExpr(context,
                                                expr->u.binary.left);
        target->u.binary.right = target->u.binary.left ?
            GlslIRLowerExpr(context, expr->u.binary.right) : NULL;
        if (target->u.binary.left == NULL ||
            target->u.binary.right == NULL) return NULL;
        return target;
    case CGIR_EXPR_CONDITIONAL:
        return GlslIRLowerConditional(context, expr, &type);
    case CGIR_EXPR_CALL:
        return GlslIRLowerCallCore(context, expr->u.call.callee, NULL,
                                   expr->u.call.arguments, &type);
    case CGIR_EXPR_INTRINSIC:
        return GlslIRLowerCallCore(context, NULL,
                                   expr->u.intrinsicCall.signature,
                                   expr->u.intrinsicCall.arguments, &type);
    case CGIR_EXPR_INTERFACE_CALL:
        /* Interface dispatch has no focused GLSL profile meaning; profile
         * validation rejects it with the historical reason. */
        GlslRecordFailure(context, "interface dispatch");
        return NULL;
    default:
        GlslRecordFailure(context, "GLSL profile expression");
        return NULL;
    }
} // GlslIRLowerExpr

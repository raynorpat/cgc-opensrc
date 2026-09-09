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
// glsl_lower_aggregate.c
//

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <float.h>
#include <limits.h>

#include "glsl_lower_internal.h"



static GlslExpr *GlslNewIndexLiteral(GlslLowerContext *context,
                                     GlslExpr *object,
                                     const GlslType *type, int index)
{
    GlslExpr *target;
    GlslExpr *literal;
    GlslType intType;

    intType = GlslNumericType(GLSL_BASE_INT, 1);
    literal = GlslNewExpr(context->module, GLSL_EXPR_INT, intType);
    target = GlslNewExpr(context->module, GLSL_EXPR_INDEX, *type);
    if (literal == NULL || target == NULL)
        return NULL;
    literal->u.literalInt = index;
    target->u.index.object = object;
    target->u.index.index = literal;
    return target;
}

static GlslExpr *GlslMatrixComponent(GlslLowerContext *context,
    GlslExpr *matrix, int row, int column)
{
    GlslExpr *columnExpr;
    GlslType columnType;
    GlslType scalarType;

    if (matrix == NULL || matrix->type.rows < 2 ||
        matrix->type.rows != matrix->type.cols || row < 0 || column < 0 ||
        row >= matrix->type.rows || column >= matrix->type.cols)
    {
        return NULL;
    }
    columnType = GlslNumericType(GLSL_BASE_FLOAT, matrix->type.rows);
    scalarType = GlslNumericType(GLSL_BASE_FLOAT, 1);
    columnExpr = GlslNewIndexLiteral(context, matrix, &columnType, column);
    if (columnExpr == NULL)
        return NULL;
    return GlslNewIndexLiteral(context, columnExpr, &scalarType, row);
}

int GlslMatrixSelectorCount(const expr *source)
{
    int count;

    if (source == NULL || source->common.kind != UNARY_N ||
        source->un.op != SWIZMAT_Z_OP) return 0;
    count = SUBOP_GET_T2(source->un.subop);
    return count == 0 ? 1 : count;
}

static GlslExpr *GlslMatrixMaskComponent(GlslLowerContext *context,
    GlslExpr *matrix, int mask, int component)
{
    int selector;
    int row;
    int column;

    selector = (mask >> (component * 4)) & 15;
    row = (selector >> 2) & 3;
    column = selector & 3;
    return GlslMatrixComponent(context, matrix, row, column);
}

GlslExpr *GlslMatrixSelectorComponent(GlslLowerContext *context,
    GlslExpr *matrix, const expr *selectorSource, int component)
{
    if (selectorSource == NULL)
        return NULL;
    return GlslMatrixMaskComponent(context, matrix,
        SUBOP_GET_MASK16(selectorSource->un.subop), component);
}


GlslExpr *GlslLowerMatrixSwizzle(GlslLowerContext *context,
    expr *source, const GlslType *type)
{
    GlslExpr *object;
    GlslExpr *target;
    GlslExpr *component;
    GlslMatrixSelectorHelper *helper;
    int count;
    int mask;
    int selector;
    int row;
    int column;
    int i;

    object = GlslLowerExpr(context, source->un.arg);
    if (object == NULL)
        return NULL;
    count = SUBOP_GET_T2(source->un.subop);
    if (count == 0)
        count = 1;
    if (count < 1 || count > 4)
        return NULL;
    mask = SUBOP_GET_MASK16(source->un.subop);
    if (count == 1) {
        selector = mask & 15;
        row = (selector >> 2) & 3;
        column = selector & 3;
        return GlslMatrixComponent(context, object, row, column);
    }
    if (source->un.arg->common.HasSideEffects) {
        helper = GlslGetMatrixSelectorHelper(context,
            GLSL_MATRIX_SELECTOR_GET, &object->type, type, count, mask);
        if (helper == NULL)
            return NULL;
        target = GlslNewExpr(context->module, GLSL_EXPR_CALL, *type);
        if (target == NULL)
            return NULL;
        target->u.call.name = helper->function->name;
        target->u.call.arguments = object;
        return target;
    }
    target = GlslNewExpr(context->module, GLSL_EXPR_CONSTRUCT, *type);
    if (target == NULL)
        return NULL;
    for (i = 0; i < count; i++) {
        selector = (mask >> (i * 4)) & 15;
        row = (selector >> 2) & 3;
        column = selector & 3;
        component = GlslMatrixComponent(context, object, row, column);
        if (component == NULL)
            return NULL;
        GlslAppendExpr(&target->u.construct.arguments, component);
    }
    return target;
}

int GlslMatrixNumericParameterType(const GlslType *type)
{
    return type != NULL &&
           (type->base == GLSL_BASE_FLOAT ||
            type->base == GLSL_BASE_INT) &&
           type->len >= 1 && type->len <= 4 && type->rows == 0 &&
           type->cols == 0 && type->arraySize == 0 &&
           type->structName == NULL && type->elementType == NULL &&
           type->members == NULL;
}

static GlslMatrixHelper *GlslFindMatrixHelper(GlslLowerContext *context,
    const GlslType *result, const GlslType *parameters, int parameterCount)
{
    GlslMatrixHelper *helper;
    int i;

    for (helper = context->matrixHelpers; helper != NULL;
         helper = helper->next)
    {
        if (helper->parameterCount != parameterCount ||
            !GlslTypesEqual(&helper->result, result)) continue;
        for (i = 0; i < parameterCount; i++) {
            if (!GlslTypesEqual(&helper->parameters[i], &parameters[i]))
                break;
        }
        if (i == parameterCount)
            return helper;
    }
    return NULL;
}

static const char *GlslMatrixHelperName(GlslLowerContext *context,
    const GlslType *result, const GlslType *parameters, int parameterCount)
{
    char candidate[256];
    char *end;
    int i;

    if (result == NULL || result->rows < 2 || result->rows > 4 ||
        result->cols != result->rows || parameterCount < 1 ||
        parameterCount > GLSL_MATRIX_MAX_ARGUMENTS) return NULL;
    sprintf(candidate, "cg_construct_mat%d", result->rows);
    end = candidate + strlen(candidate);
    for (i = 0; i < parameterCount; i++) {
        if (!GlslMatrixNumericParameterType(&parameters[i]))
            return NULL;
        if (parameters[i].base == GLSL_BASE_FLOAT) {
            if (parameters[i].len == 1)
                sprintf(end, "_f");
            else
                sprintf(end, "_v%d", parameters[i].len);
        } else {
            if (parameters[i].len == 1)
                sprintf(end, "_i");
            else
                sprintf(end, "_iv%d", parameters[i].len);
        }
        end += strlen(end);
    }
    return GlslAllocateDistinctNameForSource(context, candidate,
                                              &context->statementLoc);
}

static GlslExpr *GlslNewParameterComponent(GlslLowerContext *context,
    GlslDecl *parameter, int componentIndex)
{
    GlslExpr *symbol;
    GlslType scalarType;
    char mask[2];

    if (parameter == NULL || componentIndex < 0 ||
        componentIndex >= parameter->type.len) return NULL;
    symbol = GlslNewExpr(context->module, GLSL_EXPR_SYMBOL,
                         parameter->type);
    if (symbol == NULL)
        return NULL;
    symbol->u.symbol = parameter;
    if (parameter->type.len == 1)
        return symbol;
    scalarType = GlslNumericType(parameter->type.base, 1);
    mask[0] = "xyzw"[componentIndex];
    mask[1] = '\0';
    return GlslNewSwizzle(context, symbol, &scalarType, mask);
}

static GlslMatrixSelectorHelper *GlslFindMatrixSelectorHelper(
    GlslLowerContext *context, GlslMatrixSelectorHelperKind kind,
    const GlslType *matrixType, const GlslType *valueType, int count,
    int mask)
{
    GlslMatrixSelectorHelper *helper;

    for (helper = context->selectorHelpers; helper != NULL;
         helper = helper->next)
    {
        if (helper->kind == kind && helper->count == count &&
            helper->mask == mask &&
            GlslTypesEqual(&helper->matrixType, matrixType) &&
            GlslTypesEqual(&helper->valueType, valueType)) return helper;
    }
    return NULL;
}

static const char *GlslMatrixSelectorHelperName(
    GlslLowerContext *context, GlslMatrixSelectorHelperKind kind,
    const GlslType *matrixType, int count, int mask)
{
    char candidate[128];
    char *end;
    int column;
    int i;
    int row;
    int selector;

    if (matrixType == NULL || matrixType->rows < 2 ||
        matrixType->rows > 4 || matrixType->cols != matrixType->rows ||
        count < 2 || count > 4) return NULL;
    sprintf(candidate, "cg_%s_mat%d",
            kind == GLSL_MATRIX_SELECTOR_GET ? "get" : "set",
            matrixType->rows);
    end = candidate + strlen(candidate);
    for (i = 0; i < count; i++) {
        selector = (mask >> (i * 4)) & 15;
        row = (selector >> 2) & 3;
        column = selector & 3;
        if (row >= matrixType->rows || column >= matrixType->cols)
            return NULL;
        sprintf(end, "_m%d%d", row, column);
        end += strlen(end);
    }
    return GlslAllocateDistinctNameForSource(context, candidate,
                                              &context->statementLoc);
}

static GlslMatrixSelectorHelper *GlslCreateMatrixSelectorHelper(
    GlslLowerContext *context, GlslMatrixSelectorHelperKind kind,
    const GlslType *matrixType, const GlslType *valueType, int count,
    int mask)
{
    GlslMatrixSelectorHelper *helper;
    GlslFunction *function;
    GlslDecl *matrixParameter;
    GlslDecl *valueParameter;
    GlslExpr *matrixSymbol;
    GlslExpr *value;
    GlslExpr *component;
    GlslExpr *assignment;
    GlslExpr *constructor;
    GlslStmt *statement;
    GlslType resultType;
    GlslType scalarType;
    const char *functionName;
    const char *matrixName;
    const char *valueName;
    int i;

    if (valueType == NULL || valueType->base != GLSL_BASE_FLOAT ||
        valueType->rows != 0 || valueType->cols != 0 ||
        valueType->arraySize != 0 || valueType->elementType != NULL ||
        valueType->structName != NULL || valueType->members != NULL ||
        (valueType->len != 1 && valueType->len != count)) return NULL;
    functionName = GlslMatrixSelectorHelperName(context, kind, matrixType,
                                                 count, mask);
    if (functionName == NULL)
        return NULL;
    helper = (GlslMatrixSelectorHelper *) context->module->alloc(
        context->module->allocArg, sizeof(GlslMatrixSelectorHelper));
    if (helper == NULL)
        return NULL;
    memset(helper, 0, sizeof(GlslMatrixSelectorHelper));
    if (kind == GLSL_MATRIX_SELECTOR_GET)
        resultType = *valueType;
    else
        resultType = GlslNumericType(GLSL_BASE_VOID, 0);
    function = GlslNewFunction(context->module, resultType, functionName);
    if (function == NULL)
        return NULL;
    matrixName = GlslAllocateScopedSymbolNameForSource(context, function,
        NULL, "matrix", &context->statementLoc);
    matrixParameter = GlslNewDecl(context->module, GLSL_STORAGE_NONE,
                                  *matrixType, matrixName);
    if (matrixName == NULL || matrixParameter == NULL)
        return NULL;
    if (kind == GLSL_MATRIX_SELECTOR_SET)
        matrixParameter->parameterQualifier = GLSL_PARAMETER_INOUT;
    GlslAppendDecl(&function->parameters, matrixParameter);
    scalarType = GlslNumericType(GLSL_BASE_FLOAT, 1);
    if (kind == GLSL_MATRIX_SELECTOR_GET) {
        constructor = GlslNewExpr(context->module, GLSL_EXPR_CONSTRUCT,
                                  *valueType);
        statement = GlslNewStmt(context->module, GLSL_STMT_RETURN);
        if (constructor == NULL || statement == NULL)
            return NULL;
        for (i = 0; i < count; i++) {
            matrixSymbol = GlslNewExpr(context->module,
                                       GLSL_EXPR_SYMBOL, *matrixType);
            if (matrixSymbol == NULL)
                return NULL;
            matrixSymbol->u.symbol = matrixParameter;
            component = GlslMatrixMaskComponent(context, matrixSymbol,
                                                 mask, i);
            if (component == NULL)
                return NULL;
            GlslAppendExpr(&constructor->u.construct.arguments, component);
        }
        statement->u.returnExpr = constructor;
        function->body = statement;
    } else {
        valueName = GlslAllocateScopedSymbolNameForSource(context, function,
            NULL, "value", &context->statementLoc);
        valueParameter = GlslNewDecl(context->module, GLSL_STORAGE_NONE,
                                     *valueType, valueName);
        if (valueName == NULL || valueParameter == NULL)
            return NULL;
        GlslAppendDecl(&function->parameters, valueParameter);
        for (i = 0; i < count; i++) {
            matrixSymbol = GlslNewExpr(context->module,
                                       GLSL_EXPR_SYMBOL, *matrixType);
            if (matrixSymbol == NULL)
                return NULL;
            matrixSymbol->u.symbol = matrixParameter;
            component = GlslMatrixMaskComponent(context, matrixSymbol,
                                                 mask, i);
            value = GlslNewParameterComponent(context, valueParameter,
                valueType->len == 1 ? 0 : i);
            assignment = GlslNewExpr(context->module, GLSL_EXPR_BINARY,
                                     scalarType);
            statement = GlslNewStmt(context->module,
                                    GLSL_STMT_EXPRESSION);
            if (component == NULL || value == NULL || assignment == NULL ||
                statement == NULL) return NULL;
            assignment->u.binary.op = GLSL_OP_ASSIGN;
            assignment->u.binary.left = component;
            assignment->u.binary.right = value;
            statement->u.expression = assignment;
            GlslAppendStmt(&function->body, statement);
        }
    }
    helper->function = function;
    helper->kind = kind;
    helper->matrixType = *matrixType;
    helper->valueType = *valueType;
    helper->count = count;
    helper->mask = mask;
    if (context->selectorHelpers == NULL)
        context->selectorHelpers = helper;
    else
        context->lastSelectorHelper->next = helper;
    context->lastSelectorHelper = helper;
    return helper;
}

GlslMatrixSelectorHelper *GlslGetMatrixSelectorHelper(
    GlslLowerContext *context, GlslMatrixSelectorHelperKind kind,
    const GlslType *matrixType, const GlslType *valueType, int count,
    int mask)
{
    GlslMatrixSelectorHelper *helper;
    int normalizedMask;

    normalizedMask = mask & ((1 << (count * 4)) - 1);
    helper = GlslFindMatrixSelectorHelper(context, kind, matrixType,
                                          valueType, count,
                                          normalizedMask);
    if (helper == NULL) {
        helper = GlslCreateMatrixSelectorHelper(context, kind, matrixType,
                                                 valueType, count,
                                                 normalizedMask);
    }
    return helper;
}

static GlslMatrixHelper *GlslCreateMatrixHelper(GlslLowerContext *context,
    const GlslType *result, const GlslType *parameters, int parameterCount)
{
    GlslExpr *components[GLSL_MATRIX_MAX_ARGUMENTS];
    GlslExpr *component;
    GlslExpr *constructor;
    GlslStmt *returnStatement;
    GlslMatrixHelper *helper;
    GlslDecl *parameter;
    const char *functionName;
    const char *parameterName;
    char candidate[32];
    int componentCount;
    int parameterIndex;
    int componentIndex;
    int column;
    int row;

    functionName = GlslMatrixHelperName(context, result, parameters,
                                        parameterCount);
    if (functionName == NULL)
        return NULL;
    helper = (GlslMatrixHelper *) context->module->alloc(
        context->module->allocArg, sizeof(GlslMatrixHelper));
    if (helper == NULL)
        return NULL;
    memset(helper, 0, sizeof(GlslMatrixHelper));
    helper->function = GlslNewFunction(context->module, *result,
                                       functionName);
    if (helper->function == NULL)
        return NULL;
    helper->result = *result;
    helper->parameterCount = parameterCount;
    componentCount = 0;
    for (parameterIndex = 0; parameterIndex < parameterCount;
         parameterIndex++)
    {
        helper->parameters[parameterIndex] = parameters[parameterIndex];
        sprintf(candidate, "arg%d", parameterIndex);
        parameterName = GlslAllocateScopedSymbolNameForSource(context,
            helper->function, NULL, candidate, &context->statementLoc);
        if (parameterName == NULL)
            return NULL;
        parameter = GlslNewDecl(context->module, GLSL_STORAGE_NONE,
            parameters[parameterIndex], parameterName);
        if (parameter == NULL)
            return NULL;
        GlslAppendDecl(&helper->function->parameters, parameter);
        for (componentIndex = 0;
             componentIndex < parameters[parameterIndex].len;
             componentIndex++)
        {
            if (componentCount >= (int) (sizeof(components) /
                                         sizeof(components[0]))) return NULL;
            component = GlslNewParameterComponent(context, parameter,
                                                   componentIndex);
            if (component == NULL)
                return NULL;
            components[componentCount++] = component;
        }
    }
    if (componentCount != result->rows * result->cols)
        return NULL;
    constructor = GlslNewExpr(context->module, GLSL_EXPR_CONSTRUCT,
                              *result);
    returnStatement = GlslNewStmt(context->module, GLSL_STMT_RETURN);
    if (constructor == NULL || returnStatement == NULL)
        return NULL;
    for (column = 0; column < result->cols; column++) {
        for (row = 0; row < result->rows; row++) {
            GlslAppendExpr(&constructor->u.construct.arguments,
                           components[row * result->cols + column]);
        }
    }
    returnStatement->u.returnExpr = constructor;
    helper->function->body = returnStatement;
    if (context->matrixHelpers == NULL)
        context->matrixHelpers = helper;
    else
        context->lastMatrixHelper->next = helper;
    context->lastMatrixHelper = helper;
    return helper;
}

GlslExpr *GlslLowerImpureMatrixConstructor(
    GlslLowerContext *context, GlslExpr *arguments, const GlslType *type)
{
    GlslType parameters[GLSL_MATRIX_MAX_ARGUMENTS];
    GlslMatrixHelper *helper;
    GlslExpr *argument;
    GlslExpr *target;
    int componentCount;
    int parameterCount;

    componentCount = 0;
    parameterCount = 0;
    for (argument = arguments; argument != NULL; argument = argument->next) {
        if (parameterCount >= GLSL_MATRIX_MAX_ARGUMENTS ||
            !GlslMatrixNumericParameterType(&argument->type) ||
            componentCount > type->rows * type->cols - argument->type.len)
        {
            if (!GlslMatrixNumericParameterType(&argument->type)) {
                GlslRecordFailureKind(context,
                                      GLSL_ERROR_UNSUPPORTED_TYPE,
                                      "matrix constructor argument type");
            }
            return NULL;
        }
        parameters[parameterCount++] = argument->type;
        componentCount += argument->type.len;
    }
    if (parameterCount == 0 || componentCount != type->rows * type->cols)
        return NULL;
    helper = GlslFindMatrixHelper(context, type, parameters,
                                  parameterCount);
    if (helper == NULL) {
        helper = GlslCreateMatrixHelper(context, type, parameters,
                                        parameterCount);
        if (helper == NULL)
            return NULL;
    }
    target = GlslNewExpr(context->module, GLSL_EXPR_CALL, *type);
    if (target == NULL)
        return NULL;
    target->u.call.name = helper->function->name;
    target->u.call.arguments = arguments;
    return target;
}

GlslExpr *GlslLowerMatrixConstructor(GlslLowerContext *context,
    expr *source, const GlslType *type)
{
    GlslExpr *arguments[16];
    GlslExpr *argument;
    GlslExpr *next;
    GlslExpr *target;
    GlslExpr *component;
    GlslType scalarType;
    expr *sourceArgument;
    char mask[2];
    int count;
    int componentIndex;
    int componentCount;
    int row;
    int column;
    int size;
    int hasSideEffects;

    size = type->rows;
    if (size < 2 || size > 4 || type->cols != size)
        return NULL;
    hasSideEffects = 0;
    for (sourceArgument = source->un.arg; sourceArgument != NULL;
         sourceArgument = sourceArgument->bin.right)
    {
        if (sourceArgument->common.kind != BINARY_N ||
            sourceArgument->bin.op != EXPR_LIST_OP ||
            sourceArgument->bin.left == NULL) return NULL;
        if (sourceArgument->bin.left->common.HasSideEffects)
            hasSideEffects = 1;
    }
    argument = GlslLowerExprChain(context, source->un.arg, EXPR_LIST_OP);
    if (argument == NULL)
        return NULL;
    /* Keep impure expressions at the constructor call site and use each
       exactly once; argument evaluation order remains language-defined. */
    if (hasSideEffects)
        return GlslLowerImpureMatrixConstructor(context, argument, type);
    sourceArgument = source->un.arg;
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
            if (sourceArgument == NULL ||
                sourceArgument->common.kind != BINARY_N ||
                sourceArgument->bin.op != EXPR_LIST_OP)
            {
                return NULL;
            }
            for (componentIndex = 0; componentIndex < componentCount;
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
        sourceArgument = sourceArgument->bin.right;
    }
    if (sourceArgument != NULL || count != size * size)
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
}

void GlslPrependMatrixHelpers(GlslLowerContext *context)
{
    GlslMatrixHelper *helper;
    GlslFunction *first;
    GlslFunction *last;

    first = NULL;
    last = NULL;
    for (helper = context->matrixHelpers; helper != NULL;
         helper = helper->next)
    {
        if (first == NULL)
            first = helper->function;
        else
            last->next = helper->function;
        last = helper->function;
    }
    if (last != NULL) {
        last->next = context->module->functions;
        context->module->functions = first;
    }
}

void GlslPrependMatrixSelectorHelpers(GlslLowerContext *context)
{
    GlslMatrixSelectorHelper *helper;
    GlslFunction *first;
    GlslFunction *last;

    first = NULL;
    last = NULL;
    for (helper = context->selectorHelpers; helper != NULL;
         helper = helper->next)
    {
        if (first == NULL)
            first = helper->function;
        else
            last->next = helper->function;
        last = helper->function;
    }
    if (last != NULL) {
        last->next = context->module->functions;
        context->module->functions = first;
    }
}

/*
 * GlslIRNeedsMaterialization() - IR twin of the legacy aggregate
 *          operand test: side effects, calls, and anything unusual must
 *          evaluate through a temporary before leaf fan-out shares it.
 */

int GlslIRNeedsMaterialization(const CgIRExpr *expr)
{
    const CgIRExpr *argument;

    if (expr == NULL)
        return 0;
    if (expr->sideEffects)
        return 1;
    switch (expr->kind) {
    case CGIR_EXPR_SYMBOL:
    case CGIR_EXPR_CONSTANT:
        return 0;
    case CGIR_EXPR_MEMBER:
        return GlslIRNeedsMaterialization(expr->u.member.object);
    case CGIR_EXPR_INDEX:
        return GlslIRNeedsMaterialization(expr->u.index.object) ||
               GlslIRNeedsMaterialization(expr->u.index.index);
    case CGIR_EXPR_LENGTH:
        return GlslIRNeedsMaterialization(expr->u.length.object);
    case CGIR_EXPR_SWIZZLE:
        return GlslIRNeedsMaterialization(expr->u.swizzle.object);
    case CGIR_EXPR_CAST:
        return GlslIRNeedsMaterialization(expr->u.cast.operand);
    case CGIR_EXPR_UNARY:
        return GlslIRNeedsMaterialization(expr->u.unary.operand);
    case CGIR_EXPR_BINARY:
        return GlslIRNeedsMaterialization(expr->u.binary.left) ||
               GlslIRNeedsMaterialization(expr->u.binary.right);
    case CGIR_EXPR_CONDITIONAL:
        return GlslIRNeedsMaterialization(
                   expr->u.conditional.condition) ||
               GlslIRNeedsMaterialization(
                   expr->u.conditional.trueExpr) ||
               GlslIRNeedsMaterialization(
                   expr->u.conditional.falseExpr);
    case CGIR_EXPR_CONSTRUCT:
        for (argument = expr->u.construct.arguments; argument != NULL;
             argument = argument->next)
        {
            if (GlslIRNeedsMaterialization(argument)) return 1;
        }
        return 0;
    default:
        return 1;
    }
} // GlslIRNeedsMaterialization

/*
 * GlslIRContainsArray() - Legacy AggregateContainsArray over canonical
 *          Cg types: any nested array makes side-effecting operand
 *          materialization unsound.
 */

static int GlslIRContainsArray(const Type *type)
{
    const Symbol *member;
    int len;
    int len2;

    len = len2 = 0;
    if (IsScalar(type) || IsVector(type, &len) ||
        IsMatrix(type, &len, &len2))
    {
        return 0;
    }
    if (GetCategory(type) == TYPE_CATEGORY_ARRAY)
        return 1;
    if (GetCategory(type) == TYPE_CATEGORY_STRUCT &&
        type->str.members != NULL)
    {
        for (member = type->str.members->symbols; member != NULL;
             member = member->next)
        {
            if (GlslIRContainsArray(member->type)) return 1;
        }
    }
    return 0;
} // GlslIRContainsArray

/*
 * GlslIRPostNormalizeNeedsMaterialization() - Operand test evaluated
 *          after index hoisting: every dynamic index already moved into
 *          its own temporary, so only remaining effects count.
 */

static int GlslIRPostNormalizeNeedsMaterialization(const CgIRExpr *expr)
{
    const CgIRExpr *argument;

    if (expr == NULL)
        return 0;
    if (expr->sideEffects)
        return 1;
    switch (expr->kind) {
    case CGIR_EXPR_SYMBOL:
    case CGIR_EXPR_CONSTANT:
    case CGIR_EXPR_INDEX:
        return 0;
    case CGIR_EXPR_MEMBER:
        return GlslIRPostNormalizeNeedsMaterialization(
            expr->u.member.object);
    case CGIR_EXPR_LENGTH:
        return GlslIRPostNormalizeNeedsMaterialization(
            expr->u.length.object);
    case CGIR_EXPR_SWIZZLE:
        return GlslIRPostNormalizeNeedsMaterialization(
            expr->u.swizzle.object);
    case CGIR_EXPR_CAST:
        return GlslIRPostNormalizeNeedsMaterialization(
            expr->u.cast.operand);
    case CGIR_EXPR_UNARY:
        return GlslIRPostNormalizeNeedsMaterialization(
            expr->u.unary.operand);
    case CGIR_EXPR_BINARY:
        return GlslIRPostNormalizeNeedsMaterialization(
                   expr->u.binary.left) ||
               GlslIRPostNormalizeNeedsMaterialization(
                   expr->u.binary.right);
    case CGIR_EXPR_CONDITIONAL:
        return GlslIRPostNormalizeNeedsMaterialization(
                   expr->u.conditional.condition) ||
               GlslIRPostNormalizeNeedsMaterialization(
                   expr->u.conditional.trueExpr) ||
               GlslIRPostNormalizeNeedsMaterialization(
                   expr->u.conditional.falseExpr);
    case CGIR_EXPR_CONSTRUCT:
        for (argument = expr->u.construct.arguments; argument != NULL;
             argument = argument->next)
        {
            if (GlslIRPostNormalizeNeedsMaterialization(argument))
                return 1;
        }
        return 0;
    default:
        return 1;
    }
} // GlslIRPostNormalizeNeedsMaterialization

/*
 * GlslCloneExpr() - Deep copy of a lowered GLSL expression.  Aggregate
 *          fan-out duplicates the base object per leaf exactly as the
 *          legacy AssignAggregate duplicated its operands.
 */

GlslExpr *GlslCloneExpr(GlslModule *module, const GlslExpr *expr)
{
    GlslExpr *clone;
    GlslExpr *last;
    GlslExpr *argument;

    if (expr == NULL)
        return NULL;
    clone = (GlslExpr *) module->alloc(module->allocArg, sizeof(GlslExpr));
    if (clone == NULL)
        return NULL;
    *clone = *expr;
    clone->next = NULL;
    switch (clone->kind) {
    case GLSL_EXPR_UNARY:
        clone->u.unary.operand = GlslCloneExpr(module,
                                               expr->u.unary.operand);
        if (clone->u.unary.operand == NULL) return NULL;
        break;
    case GLSL_EXPR_BINARY:
        clone->u.binary.left = GlslCloneExpr(module,
                                             expr->u.binary.left);
        clone->u.binary.right = GlslCloneExpr(module,
                                              expr->u.binary.right);
        if (clone->u.binary.left == NULL || clone->u.binary.right == NULL)
            return NULL;
        break;
    case GLSL_EXPR_CONDITIONAL:
        clone->u.conditional.condition = GlslCloneExpr(
            module, expr->u.conditional.condition);
        clone->u.conditional.trueExpr = GlslCloneExpr(
            module, expr->u.conditional.trueExpr);
        clone->u.conditional.falseExpr = GlslCloneExpr(
            module, expr->u.conditional.falseExpr);
        if (clone->u.conditional.condition == NULL ||
            clone->u.conditional.trueExpr == NULL ||
            clone->u.conditional.falseExpr == NULL) return NULL;
        break;
    case GLSL_EXPR_CALL:
        last = NULL;
        for (argument = expr->u.call.arguments; argument != NULL;
             argument = argument->next)
        {
            GlslExpr *copy = GlslCloneExpr(module, argument);

            if (copy == NULL)
                return NULL;
            if (last == NULL)
                clone->u.call.arguments = copy;
            else
                last->next = copy;
            last = copy;
        }
        break;
    case GLSL_EXPR_CONSTRUCT:
        last = NULL;
        for (argument = expr->u.construct.arguments; argument != NULL;
             argument = argument->next)
        {
            GlslExpr *copy = GlslCloneExpr(module, argument);

            if (copy == NULL)
                return NULL;
            if (last == NULL)
                clone->u.construct.arguments = copy;
            else
                last->next = copy;
            last = copy;
        }
        break;
    case GLSL_EXPR_MEMBER:
        clone->u.member.object = GlslCloneExpr(module,
                                               expr->u.member.object);
        if (clone->u.member.object == NULL) return NULL;
        break;
    case GLSL_EXPR_INDEX:
        clone->u.index.object = GlslCloneExpr(module,
                                              expr->u.index.object);
        clone->u.index.index = GlslCloneExpr(module,
                                             expr->u.index.index);
        if (clone->u.index.object == NULL || clone->u.index.index == NULL)
            return NULL;
        break;
    case GLSL_EXPR_SWIZZLE:
        clone->u.swizzle.object = GlslCloneExpr(module,
                                                expr->u.swizzle.object);
        if (clone->u.swizzle.object == NULL) return NULL;
        break;
    default:
        break;
    }
    return clone;
} // GlslCloneExpr

static GlslExpr *GlslIRDeclRef(GlslLowerContext *context,
                               const GlslDecl *decl)
{
    GlslExpr *ref;

    ref = GlslNewExpr(context->module, GLSL_EXPR_SYMBOL, decl->type);
    if (ref != NULL)
        ref->u.symbol = (GlslDecl *) decl;
    return ref;
} // GlslIRDeclRef

static GlslExpr *GlslIRAppendMemberStep(GlslLowerContext *context,
                                        GlslExpr *base,
                                        const GlslDecl *member,
                                        const SourceLoc *loc)
{
    GlslExpr *step;

    step = GlslNewExpr(context->module, GLSL_EXPR_MEMBER,
                       member->type);
    if (step == NULL)
        return NULL;
    GlslSetLoc(&step->loc, loc);
    step->u.member.object = base;
    step->u.member.decl = (GlslDecl *) member;
    step->u.member.name = member->name;
    return step;
} // GlslIRAppendMemberStep

static GlslExpr *GlslIRAppendIndexStep(GlslLowerContext *context,
                                       GlslExpr *base, int index,
                                       GlslType type,
                                       const SourceLoc *loc)
{
    GlslExpr *literal;
    GlslExpr *step;
    GlslType intType;

    intType = GlslNumericType(GLSL_BASE_INT, 1);
    literal = GlslNewExpr(context->module, GLSL_EXPR_INT, intType);
    step = GlslNewExpr(context->module, GLSL_EXPR_INDEX, type);
    if (literal == NULL || step == NULL)
        return NULL;
    literal->u.literalInt = index;
    GlslSetLoc(&step->loc, loc);
    step->u.index.object = base;
    step->u.index.index = literal;
    return step;
} // GlslIRAppendIndexStep

static GlslExpr *GlslIRAppendDynamicIndex(GlslLowerContext *context,
                                          GlslExpr *object,
                                          GlslExpr *index,
                                          GlslType type,
                                          const SourceLoc *loc)
{
    GlslExpr *step;

    step = GlslNewExpr(context->module, GLSL_EXPR_INDEX, type);
    if (step == NULL)
        return NULL;
    GlslSetLoc(&step->loc, loc);
    step->u.index.object = object;
    step->u.index.index = index;
    return step;
} // GlslIRAppendDynamicIndex

/*
 * GlslIRChooseFlattenName() - First free cg_index/cg_aggregate-style
 *          name within the current function's declarations, mirroring
 *          the legacy flatten-temp naming search against the function
 *          scope.
 */

static void GlslIRChooseFlattenName(GlslLowerContext *context,
                                    const char *prefix, char *candidate,
                                    size_t size)
{
    const GlslDecl *decl;
    int index;
    int taken;

    for (index = 0;; index++) {
        if (index == 0)
            strcpy(candidate, prefix);
        else
            sprintf(candidate, "%s%d", prefix, index);
        taken = 0;
        for (decl = context->function->parameters; decl != NULL && !taken;
             decl = decl->next)
        {
            taken = decl->name != NULL &&
                    !strcmp(decl->name, candidate);
        }
        for (decl = context->function->locals; decl != NULL && !taken;
             decl = decl->next)
        {
            taken = decl->name != NULL &&
                    !strcmp(decl->name, candidate);
        }
        if (!taken)
            break;
    }
} // GlslIRChooseFlattenName

static Symbol *GlslIRNewTempSymbol(GlslLowerContext *context,
                                   Type *type, const char *name,
                                   const SourceLoc *loc)
{
    Symbol *temp;

    temp = (Symbol *) (calloc)(1, sizeof(Symbol));
    if (temp == NULL)
        return NULL;
    temp->name = LookUpAddString(atable, name);
    temp->type = type;
    if (loc != NULL)
        temp->loc = *loc;
    return temp;
} // GlslIRNewTempSymbol

/*
 * GlslIRAddLocal() - Register one local declaration (including
 *          synthesized temporaries) into the current function.  Sorted
 *          insertion reproduces the legacy locals ordering.
 */

GlslDecl *GlslIRAddLocal(GlslLowerContext *context, Symbol *symbol,
                                Type *type, const SourceLoc *loc)
{
    GlslDecl *decl;
    GlslType glslType;
    const char *sourceName;
    const char *name;
    const void *identity;
    const void *nameSpace;

    if (!GlslIRType(context, type, &glslType, loc))
        return NULL;
    sourceName = GetAtomString(atable, symbol->name);
    if (sourceName == NULL)
        return NULL;
    identity = symbol != NULL ? (const void *) symbol
                              : (const void *) type;
    nameSpace = context->function->isEntry ? NULL : context->function;
    if (nameSpace != NULL) {
        name = GlslAllocateScopedSymbolNameForSource(context, nameSpace,
                                                     identity, sourceName,
                                                     loc);
    } else {
        name = GlslAllocateSymbolNameForSource(context, identity,
                                               sourceName, loc);
    }
    if (name == NULL)
        return NULL;
    decl = GlslNewDecl(context->module, GLSL_STORAGE_NONE, glslType, name);
    if (decl == NULL)
        return NULL;
    decl->identity = identity;
    GlslSetLoc(&decl->loc, loc);
    decl->sourceOrdinal = symbol != NULL ? symbol->sourceOrdinal : 0;
    GlslInsertDecl(&context->function->locals, decl);
    return decl;
} // GlslIRAddLocal

/*
 * GlslIRNewIndexTemp() - Hoist one dynamic aggregate index into its own
 *          cg_index temporary exactly once.
 */

static GlslDecl *GlslIRNewIndexTemp(GlslLowerContext *context, Type *type,
                                    const CgIRExpr *index,
                                    GlslStmt **list)
{
    Symbol *temp;
    GlslDecl *decl;
    GlslExpr *init;
    GlslExpr *ref;
    GlslExpr *assign;
    GlslStmt *stmt;
    char candidate[64];

    GlslIRChooseFlattenName(context, "cg_index", candidate,
                            sizeof(candidate));
    temp = GlslIRNewTempSymbol(context, type, candidate, &index->loc);
    if (temp == NULL)
        return NULL;
    decl = GlslIRAddLocal(context, temp, type, &index->loc);
    if (decl == NULL)
        return NULL;
    ref = GlslIRDeclRef(context, decl);
    init = GlslIRLowerExpr(context, index);
    assign = NULL;
    stmt = NULL;
    if (ref != NULL && init != NULL) {
        assign = GlslNewExpr(context->module, GLSL_EXPR_BINARY,
                             decl->type);
        if (assign != NULL) {
            assign->u.binary.op = GLSL_OP_ASSIGN;
            assign->u.binary.left = ref;
            assign->u.binary.right = init;
            stmt = GlslNewStmt(context->module, GLSL_STMT_EXPRESSION);
            if (stmt != NULL) {
                GlslSetLoc(&stmt->loc, &index->loc);
                stmt->u.expression = assign;
                GlslAppendStmt(list, stmt);
            }
        }
    }
    if (ref == NULL || init == NULL || assign == NULL || stmt == NULL)
        return NULL;
    return decl;
} // GlslIRNewIndexTemp

/*
 * GlslIREmitAggregateLeaves() - Recursive leaf fan-out of one aggregate
 *          assignment; scalar, vector, and matrix leaves become one
 *          plain assignment each.
 */

static int GlslIREmitAggregateLeaves(GlslLowerContext *context,
                                     const GlslType *type,
                                     GlslExpr *target, GlslExpr *value,
                                     const SourceLoc *loc,
                                     GlslStmt **list)
{
    const GlslDecl *member;
    GlslExpr *targetLeaf;
    GlslExpr *valueLeaf;
    GlslExpr *assign;
    GlslStmt *stmt;
    int i;

    if (type->elementType != NULL) {
        for (i = 0; i < type->arraySize; i++) {
            targetLeaf = GlslIRAppendIndexStep(context,
                GlslCloneExpr(context->module, target), i,
                *type->elementType, loc);
            valueLeaf = GlslIRAppendIndexStep(context,
                GlslCloneExpr(context->module, value), i,
                *type->elementType, loc);
            if (targetLeaf == NULL || valueLeaf == NULL) return 0;
            if (!GlslIREmitAggregateLeaves(context, type->elementType,
                                           targetLeaf, valueLeaf, loc,
                                           list)) return 0;
        }
        return 1;
    }
    if (type->base == GLSL_BASE_STRUCT) {
        for (member = type->members; member != NULL;
             member = member->next)
        {
            targetLeaf = GlslIRAppendMemberStep(context,
                GlslCloneExpr(context->module, target), member, loc);
            valueLeaf = GlslIRAppendMemberStep(context,
                GlslCloneExpr(context->module, value), member, loc);
            if (targetLeaf == NULL || valueLeaf == NULL) return 0;
            if (!GlslIREmitAggregateLeaves(context, &member->type,
                                           targetLeaf, valueLeaf, loc,
                                           list)) return 0;
        }
        return 1;
    }
    assign = GlslNewExpr(context->module, GLSL_EXPR_BINARY, *type);
    stmt = GlslNewStmt(context->module, GLSL_STMT_EXPRESSION);
    if (assign == NULL || stmt == NULL)
        return 0;
    assign->u.binary.op = GLSL_OP_ASSIGN;
    assign->u.binary.left = target;
    assign->u.binary.right = value;
    GlslSetLoc(&stmt->loc, loc);
    stmt->u.expression = assign;
    GlslAppendStmt(list, stmt);
    return 1;
} // GlslIREmitAggregateLeaves

/*
 * GlslIRLowerMaterialized() - Lower an aggregate operand while hoisting
 *          every dynamic index into a cg_index temporary (children
 *          first, matching the legacy traversal order).
 */

static GlslExpr *GlslIRLowerMaterialized(GlslLowerContext *context,
                                         const CgIRExpr *expr,
                                         GlslStmt **list)
{
    GlslExpr *object;
    GlslExpr *index;
    GlslType type;

    if (expr == NULL)
        return NULL;
    switch (expr->kind) {
    case CGIR_EXPR_INDEX:
        object = GlslIRLowerMaterialized(context, expr->u.index.object,
                                         list);
        if (object == NULL)
            return NULL;
        if (!GlslIRType(context, expr->type, &type, &expr->loc))
            return NULL;
        if (GlslIRNeedsMaterialization(expr->u.index.index)) {
            GlslDecl *temp;

            temp = GlslIRNewIndexTemp(context, expr->u.index.index->type,
                                      expr->u.index.index, list);
            if (temp == NULL)
                return NULL;
            index = GlslIRDeclRef(context, temp);
            if (index == NULL)
                return NULL;
        } else {
            index = GlslIRLowerExpr(context, expr->u.index.index);
            if (index == NULL)
                return NULL;
        }
        return GlslIRAppendDynamicIndex(context, object, index, type,
                                        &expr->loc);
    case CGIR_EXPR_MEMBER: {
        GlslDecl *memberDecl;

        object = GlslIRLowerMaterialized(context, expr->u.member.object,
                                         list);
        if (object == NULL)
            return NULL;
        if (!GlslIRType(context, expr->type, &type, &expr->loc))
            return NULL;
        memberDecl = GlslFindDecl(context, expr->u.member.member);
        if (memberDecl == NULL)
            return NULL;
        return GlslIRAppendMemberStep(context, object, memberDecl,
                                      &expr->loc);
    }
    case CGIR_EXPR_SWIZZLE: {
        char maskText[5];
        int i;

        object = GlslIRLowerMaterialized(context, expr->u.swizzle.object,
                                         list);
        if (object == NULL)
            return NULL;
        if (!GlslIRType(context, expr->type, &type, &expr->loc))
            return NULL;
        for (i = 0; i < expr->u.swizzle.componentCount; i++)
            maskText[i] = "xyzw"[(expr->u.swizzle.mask >> (i * 2)) & 3];
        maskText[expr->u.swizzle.componentCount] = '\0';
        return GlslNewSwizzle(context, object, &type, maskText);
    }
    default:
        return GlslIRLowerExpr(context, expr);
    }
} // GlslIRLowerMaterialized

/*
 * GlslIRNewAggregateTemp() - The cg_aggregate materialization temporary
 *          holding one aggregate right-hand side.
 */

static GlslDecl *GlslIRNewAggregateTemp(GlslLowerContext *context,
                                        Type *type,
                                        const CgIRExpr *assign,
                                        GlslExpr *materializedValue,
                                        GlslStmt **list)
{
    Symbol *temp;
    GlslDecl *decl;
    GlslExpr *ref;
    GlslExpr *assignExpr;
    GlslStmt *stmt;
    GlslType declType;
    char candidate[64];

    GlslIRChooseFlattenName(context, "cg_aggregate", candidate,
                            sizeof(candidate));
    temp = GlslIRNewTempSymbol(context, type, candidate, &assign->loc);
    if (temp == NULL)
        return NULL;
    if (!GlslIRType(context, type, &declType, &assign->loc))
        return NULL;
    decl = GlslIRAddLocal(context, temp, type, &assign->loc);
    if (decl == NULL)
        return NULL;
    ref = GlslIRDeclRef(context, decl);
    assignExpr = NULL;
    stmt = NULL;
    if (ref != NULL && materializedValue != NULL) {
        assignExpr = GlslNewExpr(context->module, GLSL_EXPR_BINARY,
                                 declType);
        if (assignExpr != NULL) {
            assignExpr->u.binary.op = GLSL_OP_ASSIGN;
            assignExpr->u.binary.left = ref;
            assignExpr->u.binary.right = materializedValue;
            stmt = GlslNewStmt(context->module, GLSL_STMT_EXPRESSION);
            if (stmt != NULL) {
                GlslSetLoc(&stmt->loc, &assign->loc);
                stmt->u.expression = assignExpr;
                GlslAppendStmt(list, stmt);
            }
        }
    }
    if (ref == NULL || materializedValue == NULL || assignExpr == NULL ||
        stmt == NULL)
    {
        return NULL;
    }
    return decl;
} // GlslIRNewAggregateTemp

/*
 * GlslIRLowerAggregateAssign() - Struct/array assignment fan-out: index
 *          temporaries, the side-effecting-aggregate-with-arrays
 *          rejection, optional cg_aggregate materialization, then one
 *          assignment per scalar/vector/matrix leaf.
 */

int GlslIRLowerAggregateAssign(GlslLowerContext *context,
                                      const CgIRExpr *assign,
                                      GlslStmt **list)
{
    Type *aggregateType;
    GlslType leafRoot;
    GlslExpr *target;
    GlslExpr *value;
    GlslDecl *temp;
    GlslExpr *ref;

    aggregateType = assign->type;
    context->statementLoc = assign->loc;
    target = GlslIRLowerMaterialized(context, assign->u.assign.target,
                                     list);
    if (target == NULL)
        return 0;
    value = GlslIRLowerMaterialized(context, assign->u.assign.value, list);
    if (value == NULL)
        return 0;
    if (GlslIRPostNormalizeNeedsMaterialization(assign->u.assign.value))
    {
        if (GlslIRContainsArray(aggregateType)) {
            GlslRecordFailureKindAt(context,
                GLSL_ERROR_UNSUPPORTED_OPERATION,
                "side-effecting aggregate with arrays", &assign->loc);
            return 0;
        }
        temp = GlslIRNewAggregateTemp(context, aggregateType, assign,
                                      value, list);
        if (temp == NULL)
            return 0;
        ref = GlslIRDeclRef(context, temp);
        if (ref == NULL)
            return 0;
        value = ref;
    }
    if (!GlslIRType(context, aggregateType, &leafRoot, &assign->loc))
        return 0;
    return GlslIREmitAggregateLeaves(context, &leafRoot, target, value,
                                     &assign->loc, list);
} // GlslIRLowerAggregateAssign


const char *GlslIRDeclNameText(const CgIRDecl *decl)
{
    return GetAtomString(atable, decl->name);
} // GlslIRDeclNameText

/*
 * GlslIRMatrixStoreShape() - Match one scalar matrix-element store:
 *          ASSIGN(INDEX(INDEX(base,row),column), value) with constant
 *          element coordinates.
 */

static int GlslIRMatrixStoreShape(const CgIRExpr *expr,
                                  const CgIRExpr **base,
                                  const CgIRExpr **value,
                                  int *row, int *column)
{
    const CgIRExpr *rowSel;
    const CgIRExpr *rowIdx;
    const CgIRExpr *colIdx;

    if (expr == NULL || expr->kind != CGIR_EXPR_ASSIGN ||
        expr->u.assign.op != CGIR_OP_ASSIGN)
        return 0;
    rowSel = expr->u.assign.target;
    if (rowSel == NULL || rowSel->kind != CGIR_EXPR_INDEX)
        return 0;
    colIdx = rowSel->u.index.index;
    if (colIdx == NULL || colIdx->kind != CGIR_EXPR_CONSTANT ||
        colIdx->u.constant.kind != CG_SCALAR_INT)
        return 0;
    rowIdx = rowSel->u.index.object;
    if (rowIdx == NULL || rowIdx->kind != CGIR_EXPR_INDEX)
        return 0;
    {
        const CgIRExpr *rowConst = rowIdx->u.index.index;

        if (rowConst == NULL || rowConst->kind != CGIR_EXPR_CONSTANT ||
            rowConst->u.constant.kind != CG_SCALAR_INT)
            return 0;
        *row = (int) rowConst->u.constant.value.i;
    }
    *column = (int) colIdx->u.constant.value.i;
    if (*row < 0 || *row > 3 || *column < 0 || *column > 3)
        return 0;
    *base = rowIdx->u.index.object;
    *value = expr->u.assign.value;
    return 1;
} // GlslIRMatrixStoreShape

/*
 * GlslIRStoreTargetMarked() - True when the store's element chain is a
 *          Task 16 producer selector target (selectorRead): every
 *          store the `_m` group-write lowering synthesizes carries
 *          the mark and nothing else may set it.
 */

static int GlslIRStoreTargetMarked(const CgIRExpr *expr)
{
    return expr != NULL && expr->kind == CGIR_EXPR_ASSIGN &&
           expr->u.assign.target != NULL &&
           expr->u.assign.target->selectorRead;
} // GlslIRStoreTargetMarked

/*
 * GlslIRIsTempMove() - A synthesized temporary assignment: temp = expr.
 */

static int GlslIRIsTempMove(const CgIRExpr *expr, const void *identity,
                            const CgIRExpr **value)
{
    if (expr == NULL || expr->kind != CGIR_EXPR_ASSIGN ||
        expr->u.assign.op != CGIR_OP_ASSIGN)
        return 0;
    if (expr->u.assign.target == NULL ||
        expr->u.assign.target->kind != CGIR_EXPR_SYMBOL ||
        expr->u.assign.target->u.symbol != identity)
        return 0;
    *value = expr->u.assign.value;
    return 1;
} // GlslIRIsTempMove

GlslExpr *GlslIRMatrixElement(GlslLowerContext *context,
                                     GlslExpr *matrix, int row,
                                     int column)
{
    GlslExpr *columnExpr;
    GlslType columnType;
    GlslType scalarType;

    if (matrix == NULL || matrix->type.rows < 2 ||
        matrix->type.rows != matrix->type.cols || row < 0 || column < 0 ||
        row >= matrix->type.rows || column >= matrix->type.cols)
    {
        return NULL;
    }
    columnType = GlslNumericType(GLSL_BASE_FLOAT, matrix->type.rows);
    scalarType = GlslNumericType(GLSL_BASE_FLOAT, 1);
    columnExpr = GlslIRAppendIndexStep(context, matrix, column,
                                       columnType, &context->statementLoc);
    if (columnExpr == NULL)
        return NULL;
    return GlslIRAppendIndexStep(context, columnExpr, row, scalarType,
                                 &context->statementLoc);
} // GlslIRMatrixElement

/*
 * GlslIRTryGroupWrite() - Rebuild the historical output form of a Task
 *          16 matrix group write: either the cg_set_matN helper call
 *          (with any synthesized object/value temporaries folded back
 *          into their single-evaluation arguments) or the plain
 *          per-component fan-out between two simple variables.
 *          Returns 1 with *nextOut past the consumed run, 0 when the
 *          head is not a group write (including plain user store runs
 *          that only resemble one -- over-long fills or mixed
 *          coordinates -- which lower as independent statements), and
 *          -1 on lowering failure.
 */

int GlslIRTryGroupWrite(GlslLowerContext *context,
                               const CgIRStmt *head,
                               const CgIRStmt **nextOut, GlslStmt **list)
{
    const CgIRStmt *cursor;
    const void *objectTemp = NULL;
    const void *valueTemp = NULL;
    const CgIRExpr *objectSource = NULL;
    const CgIRExpr *valueSource = NULL;
    const CgIRStmt *storeLoop;
    const CgIRExpr *storeBase = NULL;
    const CgIRExpr *sharedValue = NULL;
    const CgIRExpr *rightBase = NULL;
    int storeCount = 0;
    int mask = 0;
    int rightMask = 0;
    int rows[4];
    int columns[4];
    int scalarFanout = 0;

    if (head == NULL)
        return 0;
    /* Optional leading temporaries: the object, then the value, each
     * declared and moved exactly once ahead of the stores. */
    cursor = head;
    if (head->kind == CGIR_STMT_DECL) {
        const char *declName = GlslIRDeclNameText(head->u.decl);

        if (declName == NULL || declName[0] != '$')
            return 0;
        if (head->next != NULL && head->next->kind == CGIR_STMT_EXPR &&
            GlslIRIsTempMove(head->next->u.expression,
                             head->u.decl->symbol, &objectSource))
        {
            const CgIRStmt *afterObject = head->next->next;

            objectTemp = head->u.decl->symbol;
            cursor = afterObject;
            if (afterObject != NULL &&
                afterObject->kind == CGIR_STMT_DECL &&
                afterObject->next != NULL &&
                afterObject->next->kind == CGIR_STMT_EXPR)
            {
                const char *secondName = GlslIRDeclNameText(
                    afterObject->u.decl);

                if (secondName != NULL && secondName[0] == '$' &&
                    GlslIRIsTempMove(afterObject->next->u.expression,
                                     afterObject->u.decl->symbol,
                                     &valueSource))
                {
                    valueTemp = afterObject->u.decl->symbol;
                    cursor = afterObject->next->next;
                }
            }
        }
    } else if (head->kind != CGIR_STMT_EXPR ||
               !GlslIRMatrixStoreShape(head->u.expression, &storeBase,
                                        &sharedValue, &rows[0],
                                        &columns[0]))
    {
        return 0;
    }
    /* Count consecutive constant-coordinate stores against one base. */
    storeLoop = cursor;
    for (; storeLoop != NULL; storeLoop = storeLoop->next) {
        const CgIRExpr *base;
        const CgIRExpr *value;
        int row;
        int column;

        if (storeLoop->kind != CGIR_STMT_EXPR ||
            !GlslIRMatrixStoreShape(storeLoop->u.expression, &base,
                                     &value, &row, &column))
            break;
        if (storeCount >= 4) {
            /* Frontend `_m` selector groups pack at most four
             * components, so a fifth consecutive store cannot extend a
             * producer run.  A producer run marks its store targets
             * (selectorRead): re-lowering it elementwise would print
             * each whole-vector value into one scalar component
             * target, so any marked run stays a loud failure.  Only
             * an over-long run of unmarked plain user stores is an
             * elementwise fill that falls back to independent
             * statements for the whole head; runs behind consumed "$"
             * temporaries remain genuine producer violations and stay
             * loud as well. */
            if ((cursor->kind == CGIR_STMT_EXPR &&
                 GlslIRStoreTargetMarked(cursor->u.expression)) ||
                objectTemp != NULL || valueTemp != NULL)
            {
                return -1;
            }
            return 0;
        }
        if (storeCount == 0)
            storeBase = base;
        else if (base != storeBase)
            break;
        if (objectTemp != NULL &&
            (base->kind != CGIR_EXPR_SYMBOL ||
             base->u.symbol != objectTemp))
            return -1;
        rows[storeCount] = row;
        columns[storeCount] = column;
        mask |= ((row << 2) | column) << (storeCount * 4);
        storeCount++;
    }
    if (storeCount < 2)
        return 0;
    if (objectTemp == NULL && storeBase->sideEffects)
        return -1;
    /* Classify the store values: one shared node, the value temporary,
     * or distinct selections over one right variable (scalar fan-out). */
    {
        const CgIRStmt *walk = cursor;
        const CgIRExpr *candidate = NULL;
        int allSame = 1;

        for (; walk != storeLoop; walk = walk->next) {
            const CgIRExpr *base;
            const CgIRExpr *value;
            int row;
            int column;

            (void) GlslIRMatrixStoreShape(walk->u.expression, &base,
                                           &value, &row, &column);
            if (candidate == NULL)
                candidate = value;
            else if (value != candidate)
                allSame = 0;
        }
        sharedValue = allSame ? candidate : NULL;
        if (sharedValue != NULL && objectTemp == NULL && valueTemp == NULL)
        {
            /* A shared group-read over a plain right variable paired
             * with a plain left variable is the legacy componentwise
             * fan-out, not the set-helper call. */
            const CgIRExpr *rbase = NULL;
            int rcount = 0;
            int rmask = 0;

            if (GlslIRSharedSelection(sharedValue, &rbase, &rcount,
                                      &rmask) &&
                rcount == storeCount &&
                rbase->kind == CGIR_EXPR_SYMBOL &&
                storeBase->kind == CGIR_EXPR_SYMBOL &&
                storeBase->u.symbol != rbase->u.symbol)
            {
                scalarFanout = 1;
                rightBase = rbase;
                rightMask = rmask;
            }
        }
        if (sharedValue == NULL && valueTemp != NULL) {
            /* Values must be the value temporary itself. */
            walk = cursor;
            for (; walk != storeLoop; walk = walk->next) {
                const CgIRExpr *base;
                const CgIRExpr *value;
                int row;
                int column;

                (void) GlslIRMatrixStoreShape(walk->u.expression, &base,
                                               &value, &row, &column);
                if (value->kind != CGIR_EXPR_SYMBOL ||
                    value->u.symbol != valueTemp)
                    return -1;
            }
        } else if (sharedValue == NULL && objectTemp == NULL &&
                   valueTemp == NULL)
        {
            /* Distinct per-store values: either the legacy scalar
             * fan-out between two plain variables, or two unrelated
             * scalar element stores.  Validate the fan-out shape;
             * anything else falls back to independent statements. */
            const CgIRStmt *fanWalk = cursor;

            scalarFanout = 1;
            for (; fanWalk != storeLoop; fanWalk = fanWalk->next) {
                const CgIRExpr *base;
                const CgIRExpr *value;
                const CgIRExpr *rightRowSel;
                int row;
                int column;

                if (!GlslIRMatrixStoreShape(fanWalk->u.expression, &base,
                                             &value, &row, &column))
                    return -1;
                if (value->kind != CGIR_EXPR_INDEX ||
                    value->u.index.index == NULL ||
                    value->u.index.index->kind != CGIR_EXPR_CONSTANT ||
                    value->u.index.object == NULL ||
                    value->u.index.object->kind != CGIR_EXPR_INDEX ||
                    value->u.index.object->u.index.index == NULL ||
                    value->u.index.object->u.index.index->kind !=
                        CGIR_EXPR_CONSTANT ||
                    value->u.index.object->u.index.object == NULL ||
                    value->u.index.object->u.index.object->kind !=
                        CGIR_EXPR_SYMBOL)
                {
                    return 0;
                }
                rightRowSel = value->u.index.object;
                /* The value's own coordinates must match the store's:
                 * emission would otherwise mirror the left coordinates
                 * onto the right side and silently move the wrong
                 * element.  Mixed-coordinate runs are plain user
                 * stores, not a producer shape -- fall back so every
                 * statement lowers independently. */
                if ((int) rightRowSel->u.index.index->u.constant.value.i !=
                        row ||
                    (int) value->u.index.index->u.constant.value.i !=
                        column)
                {
                    return 0;
                }
                if (rightBase == NULL)
                    rightBase = rightRowSel->u.index.object;
                else if (rightBase != rightRowSel->u.index.object)
                    return 0;
                if (storeBase->kind != CGIR_EXPR_SYMBOL ||
                    storeBase->u.symbol ==
                        rightRowSel->u.index.object->u.symbol)
                {
                    return 0;
                }
            }
            if (rightBase == NULL)
                return 0;
        } else if (sharedValue == NULL) {
            return -1;
        }
    }
    {
        SourceLoc emitLoc = cursor->loc;
        GlslExpr *leftGlsl;
        GlslExpr *valueGlsl;

        if (scalarFanout) {
            GlslExpr *rightGlsl;
            GlslType scalarType;
            int i;

            leftGlsl = GlslIRLowerExpr(context, storeBase);
            rightGlsl = leftGlsl ?
                        GlslIRLowerExpr(context, rightBase) : NULL;
            if (leftGlsl == NULL || rightGlsl == NULL)
                return -1;
            scalarType = GlslNumericType(GLSL_BASE_FLOAT, 1);
            for (i = 0; i < storeCount; i++) {
                GlslExpr *leftLeaf;
                GlslExpr *rightLeaf;
                GlslExpr *assignment;
                GlslStmt *statement;
                int rightRow;
                int rightColumn;

                leftLeaf = GlslCloneExpr(context->module, leftGlsl);
                leftLeaf = leftLeaf ?
                    GlslIRMatrixElement(context, leftLeaf, rows[i],
                                         columns[i]) : NULL;
                if (rightMask != 0) {
                    /* Right selections come from the shared group-read
                     * mask (same row<<2|column nibble encoding). */
                    rightRow = ((rightMask >> (i * 4)) >> 2) & 3;
                    rightColumn = (rightMask >> (i * 4)) & 3;
                } else {
                    /* Distinct-value runs validated every value's own
                     * coordinates against the store's above, so these
                     * are the value's real coordinates, not a mirror. */
                    rightRow = rows[i];
                    rightColumn = columns[i];
                }
                rightLeaf = GlslCloneExpr(context->module, rightGlsl);
                rightLeaf = rightLeaf ?
                    GlslIRMatrixElement(context, rightLeaf, rightRow,
                                         rightColumn) : NULL;
                assignment = leftLeaf != NULL && rightLeaf != NULL ?
                    GlslNewExpr(context->module, GLSL_EXPR_BINARY,
                                scalarType) : NULL;
                statement = assignment != NULL ?
                    GlslNewStmt(context->module,
                                GLSL_STMT_EXPRESSION) : NULL;
                if (statement == NULL)
                    return -1;
                assignment->u.binary.op = GLSL_OP_ASSIGN;
                assignment->u.binary.left = leftLeaf;
                assignment->u.binary.right = rightLeaf;
                GlslSetLoc(&statement->loc, &emitLoc);
                statement->u.expression = assignment;
                GlslAppendStmt(list, statement);
            }
        } else {
            GlslMatrixSelectorHelper *helper;
            GlslExpr *call;
            GlslStmt *statement;

            leftGlsl = GlslIRLowerExpr(context,
                objectTemp != NULL ? objectSource : storeBase);
            if (leftGlsl == NULL)
                return -1;
            valueGlsl = GlslIRLowerExpr(context,
                valueTemp != NULL ? valueSource :
                                    (sharedValue != NULL ? sharedValue :
                                                           valueSource));
            if (valueGlsl == NULL)
                return -1;
            helper = GlslGetMatrixSelectorHelper(context,
                GLSL_MATRIX_SELECTOR_SET, &leftGlsl->type,
                &valueGlsl->type, storeCount, mask);
            if (helper == NULL)
                return -1;
            call = GlslNewExpr(context->module, GLSL_EXPR_CALL,
                               helper->function->result);
            statement = call != NULL ?
                        GlslNewStmt(context->module,
                                    GLSL_STMT_EXPRESSION) : NULL;
            if (statement == NULL)
                return -1;
            call->u.call.name = helper->function->name;
            call->u.call.arguments = leftGlsl;
            leftGlsl->next = valueGlsl;
            GlslSetLoc(&statement->loc, &emitLoc);
            statement->u.expression = call;
            GlslAppendStmt(list, statement);
        }
    }
    *nextOut = storeLoop;
    return 1;
} // GlslIRTryGroupWrite

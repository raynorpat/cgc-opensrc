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
license, under NVIDIA's copyrights in this original NVIDIA software,
to use, reproduce, modify and redistribute the NVIDIA Software, with or
without modifications, in source and/or binary forms; provided that if
you redistribute the NVIDIA Software, you must retain the copyright
notice of NVIDIA, this notice and the following text and disclaimers in
all such redistributions of the NVIDIA Software.  Neither the name,
trademarks, service marks nor logos of NVIDIA Corporation may be used
to endorse or promote products derived from the NVIDIA Software without
specific prior written permission from NVIDIA.  Except as expressly
stated in this notice, no other rights or licenses express or implied,
are granted by NVIDIA herein, including but not limited to any patent
rights that may be infringed by your derivative works or by other works
in which the NVIDIA Software may be incorporated. No hardware is
licensed hereunder.

THE NVIDIA SOFTWARE IS BEING PROVIDED ON AN "AS IS" BASIS, WITHOUT
WARRANTIES OR CONDITIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED,
INCLUDING WITHOUT LIMITATION, WARRANTIES OR CONDITIONS OF TITLE,
NON-INFRINGEMENT, MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE,
OR ITS USE AND OPERATION EITHER ALONE OR IN COMBINATION WITH OTHER
PRODUCTS.

IN NO EVENT SHALL NVIDIA BE LIABLE FOR ANY SPECIAL, INDIRECT,
INCIDENTAL, EXEMPLARY, CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, LOST PROFITS; PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) ARISING IN
ANY WAY OUT OF THE USE, REPRODUCTION, MODIFICATION AND/OR DISTRIBUTION
OF THE NVIDIA SOFTWARE, HOWEVER CAUSED AND WHETHER UNDER THEORY OF
CONTRACT, TORT (INCLUDING NEGLIGENCE), STRICT LIABILITY OR OTHERWISE,
EVEN IF NVIDIA HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
\****************************************************************************/

#include <float.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>

#include "hlsl_lower_internal.h"

/*
 * HlslResolveBuiltinSymbol() - Resolve only immutable catalog identities or
 *         declarations already accepted by the HLSL HAL as group 4.  Merely
 *         sharing a source spelling never turns a user helper into a builtin.
 *         Return one for an exact row, zero for an ordinary helper, and minus
 *         one after recording an intrinsic or stage failure.
 */

int HlslResolveBuiltinSymbol(HlslLowerContext *context,
                                    Symbol *symbol,
                                    const SourceLoc *callLoc,
                                    HlslBuiltin *builtin,
                                    HlslType *result,
                                    HlslType *params, int *paramCount)
{
    const CgIntrinsicSignature *signature;
    TypeList *parameter;
    Type *sourceResult;
    const char *name;
    HlslSourceType sourceResultType;
    HlslSourceType sourceParams[HLSL_MAX_BUILTIN_ARGS];
    HlslBuiltin resolved;
    HlslBuiltin otherStage;
    int count;

    if (context == NULL || symbol == NULL || symbol->kind != FUNCTION_S ||
        symbol->type == NULL || builtin == NULL || result == NULL ||
        params == NULL || paramCount == NULL)
    {
        return 0;
    }
    signature = CgIntrinsicSignatureForSymbol(symbol);
    if (signature != NULL) {
        name = signature->name;
        sourceResult = signature->result;
        parameter = signature->parameters;
    } else if ((symbol->properties & SYMB_IS_BUILTIN) != 0 &&
               symbol->details.fun.group == HLSL_BUILTIN_GROUP &&
               symbol->details.fun.index > HLSL_BUILTIN_NONE &&
               symbol->details.fun.index < HLSL_BUILTIN_COUNT)
    {
        name = GetAtomString(atable, symbol->name);
        sourceResult = symbol->type->fun.rettype;
        parameter = symbol->type->fun.paramtypes;
    } else {
        return 0;
    }
    if (!HlslIsBuiltinName(name)) {
        HlslLowerFailure(context, HLSL_ERROR_INTRINSIC, name, callLoc);
        return -1;
    }
    if (!HlslDescribeSourceType(sourceResult, &sourceResultType)) {
        HlslLowerFailure(context,
            HlslIsTextureName(name) ? HLSL_ERROR_SAMPLER :
                                      HLSL_ERROR_INTRINSIC,
            name, callLoc);
        return -1;
    }
    count = 0;
    for (; parameter != NULL; parameter = parameter->next) {
        if (count >= HLSL_MAX_BUILTIN_ARGS ||
            !HlslDescribeSourceType(parameter->type, &sourceParams[count]))
        {
            HlslLowerFailure(context,
                HlslIsTextureName(name) ? HLSL_ERROR_SAMPLER :
                                          HLSL_ERROR_INTRINSIC,
                             name, callLoc);
            return -1;
        }
        count++;
    }
    resolved = HlslLookupSourceBuiltin(context->profile->stage, name,
        &sourceResultType, sourceParams, count);
    if (!HlslProfileAllowsBuiltin(context->profile, resolved))
        resolved = HLSL_BUILTIN_NONE;
    if (resolved == HLSL_BUILTIN_NONE) {
        otherStage = HLSL_BUILTIN_NONE;
        if (context->profile->stage != HLSL_STAGE_PIXEL)
            otherStage = HlslLookupSourceBuiltin(HLSL_STAGE_PIXEL, name,
                &sourceResultType, sourceParams, count);
        if (otherStage == HLSL_BUILTIN_NONE &&
            context->profile->stage != HLSL_STAGE_VERTEX)
        {
            otherStage = HlslLookupSourceBuiltin(HLSL_STAGE_VERTEX, name,
                &sourceResultType, sourceParams, count);
        }
        if (otherStage == HLSL_BUILTIN_NONE &&
            context->profile->stage != HLSL_STAGE_GEOMETRY)
        {
            otherStage = HlslLookupSourceBuiltin(HLSL_STAGE_GEOMETRY, name,
                &sourceResultType, sourceParams, count);
        }
        if (otherStage != HLSL_BUILTIN_NONE) {
            HlslLowerFailure(context,
                context->profile->syntax == HLSL_SYNTAX_MODERN &&
                HlslBuiltinIsTexture(otherStage) ?
                    HLSL_ERROR_TEXTURE_STAGE :
                HlslBuiltinIsTexture(otherStage) &&
                !HlslProfileHasCapability(context->profile,
                                           HLSL_CAP_TEXTURE_METHODS) ?
                    HLSL_ERROR_SAMPLER : HLSL_ERROR_STAGE_OPERATION,
                             name, callLoc);
        } else {
            HlslLowerFailure(context,
                HlslIsTextureName(name) ? HLSL_ERROR_SAMPLER :
                                          HLSL_ERROR_INTRINSIC,
                             name, callLoc);
        }
        return -1;
    }
    if (signature == NULL &&
        resolved != (HlslBuiltin) symbol->details.fun.index)
    {
        HlslLowerFailure(context, HLSL_ERROR_INTRINSIC, name, callLoc);
        return -1;
    }
    if (!HlslLowerType(context, sourceResult, result, callLoc))
        return -1;
    parameter = signature != NULL ? signature->parameters :
                symbol->type->fun.paramtypes;
    count = 0;
    for (; parameter != NULL; parameter = parameter->next) {
        if (count >= HLSL_MAX_BUILTIN_ARGS ||
            !HlslLowerType(context, parameter->type, &params[count],
                           callLoc))
        {
            if (context->module->errors == 0)
                HlslLowerFailure(context,
                    HlslIsTextureName(name) ? HLSL_ERROR_SAMPLER :
                                              HLSL_ERROR_INTRINSIC,
                                 name, callLoc);
            return -1;
        }
        count++;
    }
    if (!HlslBuiltinAccepts(context->profile->stage, resolved,
                            result, params, count))
    {
        HlslLowerFailure(context,
            HlslIsTextureName(name) ? HLSL_ERROR_SAMPLER :
                                      HLSL_ERROR_INTRINSIC,
            name, callLoc);
        return -1;
    }
    *builtin = resolved;
    *paramCount = count;
    return 1;
} // HlslResolveBuiltinSymbol

/*
 * HlslResolveBuiltinSignature() - Resolve an intrinsic directly from the
 *         immutable signature carried by verified Cg IR.  Geometry lowering
 *         must not recover this identity by returning to the source AST.
 */

static int HlslResolveBuiltinSignature(HlslLowerContext *context,
                                       const CgIntrinsicSignature *signature,
                                       const SourceLoc *callLoc,
                                       HlslBuiltin *builtin,
                                       HlslType *result,
                                       HlslType *params, int *paramCount)
{
    TypeList *parameter;
    HlslSourceType sourceResultType;
    HlslSourceType sourceParams[HLSL_MAX_BUILTIN_ARGS];
    HlslBuiltin resolved;
    HlslBuiltin otherStage;
    int count;

    if (context == NULL || signature == NULL || signature->name == NULL ||
        signature->result == NULL || builtin == NULL || result == NULL ||
        params == NULL || paramCount == NULL)
    {
        return 0;
    }
    if (!HlslIsBuiltinName(signature->name)) {
        HlslLowerFailure(context, HLSL_ERROR_INTRINSIC,
                         signature->name, callLoc);
        return -1;
    }
    if (!HlslDescribeSourceType(signature->result, &sourceResultType)) {
        HlslLowerFailure(context,
            HlslIsTextureName(signature->name) ? HLSL_ERROR_SAMPLER :
                                                 HLSL_ERROR_INTRINSIC,
            signature->name, callLoc);
        return -1;
    }
    count = 0;
    for (parameter = signature->parameters; parameter != NULL;
         parameter = parameter->next)
    {
        if (count >= HLSL_MAX_BUILTIN_ARGS ||
            !HlslDescribeSourceType(parameter->type, &sourceParams[count]))
        {
            HlslLowerFailure(context,
                HlslIsTextureName(signature->name) ? HLSL_ERROR_SAMPLER :
                                                     HLSL_ERROR_INTRINSIC,
                signature->name, callLoc);
            return -1;
        }
        count++;
    }
    resolved = HlslLookupSourceBuiltin(context->profile->stage,
        signature->name, &sourceResultType, sourceParams, count);
    if (!HlslProfileAllowsBuiltin(context->profile, resolved))
        resolved = HLSL_BUILTIN_NONE;
    if (resolved == HLSL_BUILTIN_NONE) {
        otherStage = HLSL_BUILTIN_NONE;
        if (context->profile->stage != HLSL_STAGE_PIXEL)
            otherStage = HlslLookupSourceBuiltin(HLSL_STAGE_PIXEL,
                signature->name, &sourceResultType, sourceParams, count);
        if (otherStage == HLSL_BUILTIN_NONE &&
            context->profile->stage != HLSL_STAGE_VERTEX)
        {
            otherStage = HlslLookupSourceBuiltin(HLSL_STAGE_VERTEX,
                signature->name, &sourceResultType, sourceParams, count);
        }
        if (otherStage == HLSL_BUILTIN_NONE &&
            context->profile->stage != HLSL_STAGE_GEOMETRY)
        {
            otherStage = HlslLookupSourceBuiltin(HLSL_STAGE_GEOMETRY,
                signature->name, &sourceResultType, sourceParams, count);
        }
        if (otherStage != HLSL_BUILTIN_NONE) {
            HlslLowerFailure(context,
                context->profile->syntax == HLSL_SYNTAX_MODERN &&
                HlslBuiltinIsTexture(otherStage) ?
                    HLSL_ERROR_TEXTURE_STAGE :
                HlslBuiltinIsTexture(otherStage) &&
                !HlslProfileHasCapability(context->profile,
                                           HLSL_CAP_TEXTURE_METHODS) ?
                    HLSL_ERROR_SAMPLER : HLSL_ERROR_STAGE_OPERATION,
                signature->name, callLoc);
        } else {
            HlslLowerFailure(context,
                HlslIsTextureName(signature->name) ? HLSL_ERROR_SAMPLER :
                                                     HLSL_ERROR_INTRINSIC,
                signature->name, callLoc);
        }
        return -1;
    }
    if (!HlslLowerType(context, signature->result, result, callLoc))
        return -1;
    count = 0;
    for (parameter = signature->parameters; parameter != NULL;
         parameter = parameter->next)
    {
        if (count >= HLSL_MAX_BUILTIN_ARGS ||
            !HlslLowerType(context, parameter->type, &params[count], callLoc))
        {
            if (context->module->errors == 0)
                HlslLowerFailure(context,
                    HlslIsTextureName(signature->name) ? HLSL_ERROR_SAMPLER :
                                                         HLSL_ERROR_INTRINSIC,
                    signature->name, callLoc);
            return -1;
        }
        count++;
    }
    if (!HlslBuiltinAccepts(context->profile->stage, resolved,
                            result, params, count))
    {
        HlslLowerFailure(context, HLSL_ERROR_INTRINSIC,
                         signature->name, callLoc);
        return -1;
    }
    *builtin = resolved;
    *paramCount = count;
    return 1;
} // HlslResolveBuiltinSignature

HlslExpr *HlslNewLiteral(HlslLowerContext *context, HlslBase base,
    int intValue, float floatValue)
{
    HlslExprKind kind;
    HlslExpr *target;
    HlslType type;

    type = HlslNumericType(base, 1);
    kind = base == HLSL_BASE_FLOAT ? HLSL_EXPR_FLOAT :
           base == HLSL_BASE_BOOL ? HLSL_EXPR_BOOL : HLSL_EXPR_INT;
    target = HlslNewSourceExpr(context, kind, type);
    if (target != NULL) {
        if (kind == HLSL_EXPR_FLOAT)
            target->u.literalFloat = floatValue;
        else if (kind == HLSL_EXPR_BOOL)
            target->u.literalBool = intValue != 0;
        else
            target->u.literalInt = intValue;
    }
    return target;
} // HlslNewLiteral

static HlslExpr *HlslLowerConstant(HlslLowerContext *context, expr *source,
                                   const HlslType *type)
{
    HlslExpr *target;
    HlslExpr *item;
    int i;

    if (type->base == HLSL_BASE_FLOAT) {
        for (i = 0; i < type->len; i++) {
            if (source->co.val[i].value.f != source->co.val[i].value.f ||
                source->co.val[i].value.f > FLT_MAX ||
                source->co.val[i].value.f < -FLT_MAX)
            {
                HlslLowerFailure(context, HLSL_ERROR_UNSUPPORTED_OPERATION,
                                 "non-finite constant", NULL);
                return NULL;
            }
        }
    }
    if (type->len == 1) {
        if (type->base == HLSL_BASE_FLOAT)
            return HlslNewLiteral(context, type->base, 0,
                                  source->co.val[0].value.f);
        return HlslNewLiteral(context, type->base,
                              (int) source->co.val[0].value.i, 0.0f);
    }
    target = HlslNewSourceExpr(context, HLSL_EXPR_CONSTRUCT, *type);
    if (target == NULL)
        return NULL;
    for (i = 0; i < type->len; i++) {
        if (type->base == HLSL_BASE_FLOAT)
            item = HlslNewLiteral(context, type->base, 0,
                                  source->co.val[i].value.f);
        else
            item = HlslNewLiteral(context, type->base,
                                  (int) source->co.val[i].value.i, 0.0f);
        if (item == NULL)
            return NULL;
        HlslAppendExpr(&target->u.construct.arguments, item);
    }
    return target;
} // HlslLowerConstant

static HlslOperator HlslBinaryOperator(opcode op)
{
    switch (op) {
    case ASSIGN_OP:
    case ASSIGN_V_OP:
    case ASSIGN_GEN_OP:
    case ASSIGN_DYN_OP: return HLSL_OP_ASSIGN;
    case ASSIGNPLUS_OP: return HLSL_OP_ADD_ASSIGN;
    case ASSIGNMINUS_OP: return HLSL_OP_SUBTRACT_ASSIGN;
    case ASSIGNSTAR_OP: return HLSL_OP_MULTIPLY_ASSIGN;
    case ASSIGNSLASH_OP: return HLSL_OP_DIVIDE_ASSIGN;
    case ASSIGNMOD_OP: return HLSL_OP_REMAINDER_ASSIGN;
    case BOR_OP: case BOR_V_OP: case BOR_SV_OP: case BOR_VS_OP:
        return HLSL_OP_LOGICAL_OR;
    case BAND_OP: case BAND_V_OP: case BAND_SV_OP: case BAND_VS_OP:
        return HLSL_OP_LOGICAL_AND;
    case OR_OP: case OR_V_OP: case OR_SV_OP: case OR_VS_OP:
        return HLSL_OP_BITWISE_OR;
    case XOR_OP: case XOR_V_OP: case XOR_SV_OP: case XOR_VS_OP:
        return HLSL_OP_BITWISE_XOR;
    case AND_OP: case AND_V_OP: case AND_SV_OP: case AND_VS_OP:
        return HLSL_OP_BITWISE_AND;
    case EQ_OP: case EQ_V_OP: case EQ_SV_OP: case EQ_VS_OP:
        return HLSL_OP_EQUAL;
    case NE_OP: case NE_V_OP: case NE_SV_OP: case NE_VS_OP:
        return HLSL_OP_NOT_EQUAL;
    case LT_OP: case LT_V_OP: case LT_SV_OP: case LT_VS_OP:
        return HLSL_OP_LESS;
    case GT_OP: case GT_V_OP: case GT_SV_OP: case GT_VS_OP:
        return HLSL_OP_GREATER;
    case LE_OP: case LE_V_OP: case LE_SV_OP: case LE_VS_OP:
        return HLSL_OP_LESS_EQUAL;
    case GE_OP: case GE_V_OP: case GE_SV_OP: case GE_VS_OP:
        return HLSL_OP_GREATER_EQUAL;
    case SHL_OP: case SHL_V_OP: return HLSL_OP_SHIFT_LEFT;
    case SHR_OP: case SHR_V_OP: return HLSL_OP_SHIFT_RIGHT;
    case ADD_OP: case ADD_V_OP: case ADD_SV_OP: case ADD_VS_OP:
        return HLSL_OP_ADD;
    case SUB_OP: case SUB_V_OP: case SUB_SV_OP: case SUB_VS_OP:
        return HLSL_OP_SUBTRACT;
    case MUL_OP: case MUL_V_OP: case MUL_SV_OP: case MUL_VS_OP:
        return HLSL_OP_MULTIPLY;
    case DIV_OP: case DIV_V_OP: case DIV_SV_OP: case DIV_VS_OP:
        return HLSL_OP_DIVIDE;
    case MOD_OP: case MOD_V_OP: case MOD_SV_OP: case MOD_VS_OP:
        return HLSL_OP_REMAINDER;
    default: return HLSL_OP_NONE;
    }
} // HlslBinaryOperator

static int HlslIsAssignmentOperator(HlslOperator op)
{
    return op == HLSL_OP_ASSIGN ||
           op == HLSL_OP_ADD_ASSIGN ||
           op == HLSL_OP_SUBTRACT_ASSIGN ||
           op == HLSL_OP_MULTIPLY_ASSIGN ||
           op == HLSL_OP_DIVIDE_ASSIGN ||
           op == HLSL_OP_REMAINDER_ASSIGN ||
           op == HLSL_OP_BITWISE_OR_ASSIGN ||
           op == HLSL_OP_BITWISE_XOR_ASSIGN ||
           op == HLSL_OP_BITWISE_AND_ASSIGN ||
           op == HLSL_OP_SHIFT_LEFT_ASSIGN ||
           op == HLSL_OP_SHIFT_RIGHT_ASSIGN;
} // HlslIsAssignmentOperator

static HlslOperator HlslUnaryOperator(opcode op)
{
    switch (op) {
    case NEG_OP: case NEG_V_OP: return HLSL_OP_NEGATE;
    case POS_OP: case POS_V_OP: return HLSL_OP_POSITIVE;
    case BNOT_OP: case BNOT_V_OP: return HLSL_OP_LOGICAL_NOT;
    case NOT_OP: case NOT_V_OP: return HLSL_OP_BITWISE_NOT;
    case PREINC_OP: return HLSL_OP_PRE_INCREMENT;
    case PREDEC_OP: return HLSL_OP_PRE_DECREMENT;
    case POSTINC_OP: return HLSL_OP_POST_INCREMENT;
    case POSTDEC_OP: return HLSL_OP_POST_DECREMENT;
    default: return HLSL_OP_NONE;
    }
} // HlslUnaryOperator

static int HlslIsComparison(HlslOperator op)
{
    return op == HLSL_OP_EQUAL || op == HLSL_OP_NOT_EQUAL ||
           op == HLSL_OP_LESS || op == HLSL_OP_GREATER ||
           op == HLSL_OP_LESS_EQUAL || op == HLSL_OP_GREATER_EQUAL;
} // HlslIsComparison

HlslExpr *HlslNewSymbolExpr(HlslLowerContext *context,
                                   HlslDecl *decl)
{
    HlslExpr *expression;

    expression = HlslNewSourceExpr(context, HLSL_EXPR_SYMBOL, decl->type);
    if (expression != NULL)
        expression->u.symbol = decl;
    return expression;
} // HlslNewSymbolExpr

HlslDecl *HlslNewTemporary(HlslLowerContext *context,
                                  const HlslType *type)
{
    HlslDecl *decl;
    const char *name;
    char source[32];

    if (context == NULL || context->function == NULL || type == NULL)
        return NULL;
    context->module->temporaryCount++;
    sprintf(source, "temp%d", context->module->temporaryCount);
    name = HlslAllocateDistinctName(context->module, source);
    if (name == NULL)
        return NULL;
    decl = HlslNewDecl(context->module, HLSL_STORAGE_NONE, *type, name);
    if (decl == NULL)
        return NULL;
    decl->publicName = name;
    HlslSetLoc(&decl->loc, &context->statementLoc);
    HlslAppendDecl(&context->function->locals, decl);
    return decl;
} // HlslNewTemporary

HlslStmt *HlslNewExpressionStmt(HlslLowerContext *context,
                                       HlslExpr *expression)
{
    HlslStmt *statement;

    if (expression == NULL)
        return NULL;
    statement = HlslNewStmt(context->module, HLSL_STMT_EXPRESSION);
    if (statement != NULL) {
        statement->u.expression = expression;
        HlslSetLoc(&statement->loc, &context->statementLoc);
    }
    return statement;
} // HlslNewExpressionStmt

HlslExpr *HlslNewAssignment(HlslLowerContext *context,
                                   HlslExpr *left, HlslExpr *right)
{
    HlslExpr *assignment;

    if (left == NULL || right == NULL)
        return NULL;
    assignment = HlslNewSourceExpr(context, HLSL_EXPR_BINARY, left->type);
    if (assignment != NULL) {
        assignment->u.binary.op = HLSL_OP_ASSIGN;
        assignment->u.binary.left = left;
        assignment->u.binary.right = right;
        assignment->hasSideEffects = 1;
    }
    return assignment;
} // HlslNewAssignment

static int HlslBuiltinHelperTypeEqual(const HlslType *left,
                                      const HlslType *right)
{
    return left != NULL && right != NULL &&
           left->base == right->base && left->len == right->len &&
           left->rows == right->rows && left->cols == right->cols &&
           left->arraySize == 0 && right->arraySize == 0;
} // HlslBuiltinHelperTypeEqual

static HlslFunction *HlslFindBuiltinHelper(HlslModule *module,
                                           HlslBuiltin builtin,
                                           const HlslType *type)
{
    HlslFunction *function;

    for (function = module->functions; function != NULL;
         function = function->next)
    {
        if (function->builtin == builtin &&
            HlslBuiltinHelperTypeEqual(&function->result, type))
        {
            return function;
        }
    }
    return NULL;
} // HlslFindBuiltinHelper

static HlslExpr *HlslNewBuiltinConstant(HlslLowerContext *context,
                                        const HlslType *type, float value)
{
    HlslExpr *literal;
    HlslExpr *construct;
    int i;

    if (type == NULL)
        return NULL;
    if (type->len == 1)
        return HlslNewLiteral(context, HLSL_BASE_FLOAT, 0, value);
    construct = HlslNewSourceExpr(context, HLSL_EXPR_CONSTRUCT, *type);
    if (construct == NULL)
        return NULL;
    for (i = 0; i < type->len; i++) {
        literal = HlslNewLiteral(context, HLSL_BASE_FLOAT, 0, value);
        if (literal == NULL)
            return NULL;
        HlslAppendExpr(&construct->u.construct.arguments, literal);
    }
    return construct;
} // HlslNewBuiltinConstant

static HlslFunction *HlslCreateBuiltinHelper(HlslLowerContext *context,
                                             HlslBuiltin builtin,
                                             const HlslType *type)
{
    HlslFunction *function;
    HlslDecl *parameter;
    HlslStmt *statement;
    HlslExpr *argument;
    HlslExpr *call;
    HlslExpr *inner;
    HlslExpr *zero;
    HlslExpr *one;
    const char *typeName;
    const char *name;
    const char *parameterName;
    char source[96];

    function = HlslFindBuiltinHelper(context->module, builtin, type);
    if (function != NULL)
        return function;
    typeName = HlslTypeName(type);
    if (typeName == NULL ||
        (builtin != HLSL_BUILTIN_RSQRT &&
         builtin != HLSL_BUILTIN_SATURATE))
    {
        return NULL;
    }
    sprintf(source, "cg_%s_%s", HlslBuiltinSpelling(builtin), typeName);
    function = HlslNewFunction(context->module, *type, NULL);
    if (function == NULL)
        return NULL;
    name = HlslAllocateGeneratedName(context->module, function, source);
    if (name == NULL)
        return NULL;
    function->name = name;
    function->builtin = builtin;
    function->needsPrototype = 1;
    parameterName = HlslAllocateScopedSymbolName(context->module,
        function, function, "value");
    parameter = HlslNewDecl(context->module, HLSL_STORAGE_NONE,
                            *type, parameterName);
    statement = HlslNewStmt(context->module, HLSL_STMT_RETURN);
    if (parameterName == NULL || parameter == NULL || statement == NULL)
        return NULL;
    parameter->publicName = parameterName;
    HlslAppendDecl(&function->parameters, parameter);

    argument = HlslNewSymbolExpr(context, parameter);
    if (argument == NULL)
        return NULL;
    if (builtin == HLSL_BUILTIN_RSQRT) {
        call = HlslNewSourceExpr(context, HLSL_EXPR_CALL, *type);
        if (call == NULL)
            return NULL;
        call->u.call.name = HlslBuiltinSpelling(builtin);
        call->u.call.builtin = builtin;
        call->u.call.arguments = argument;
    } else {
        zero = HlslNewBuiltinConstant(context, type, 0.0f);
        one = HlslNewBuiltinConstant(context, type, 1.0f);
        inner = HlslNewSourceExpr(context, HLSL_EXPR_CALL, *type);
        call = HlslNewSourceExpr(context, HLSL_EXPR_CALL, *type);
        if (zero == NULL || one == NULL || inner == NULL || call == NULL)
            return NULL;
        inner->u.call.name = HlslBuiltinSpelling(HLSL_BUILTIN_MAX);
        inner->u.call.builtin = HLSL_BUILTIN_MAX;
        inner->u.call.arguments = argument;
        HlslAppendExpr(&inner->u.call.arguments, zero);
        call->u.call.name = HlslBuiltinSpelling(HLSL_BUILTIN_MIN);
        call->u.call.builtin = HLSL_BUILTIN_MIN;
        call->u.call.arguments = inner;
        HlslAppendExpr(&call->u.call.arguments, one);
    }
    statement->u.returnExpr = call;
    HlslAppendStmt(&function->body, statement);
    HlslAppendFunction(&context->module->functions, function);
    return function;
} // HlslCreateBuiltinHelper

int HlslAppendExpression(HlslLowerContext *context, HlslStmt **list,
                                HlslExpr *expression)
{
    HlslStmt *statement;

    statement = HlslNewExpressionStmt(context, expression);
    if (statement == NULL)
        return 0;
    HlslAppendStmt(list, statement);
    return 1;
} // HlslAppendExpression

HlslExpr *HlslCaptureValue(HlslLowerContext *context,
                                  HlslStmt **list, HlslExpr *value)
{
    HlslDecl *temporary;
    HlslExpr *left;
    HlslExpr *result;
    HlslExpr *assignment;

    temporary = HlslNewTemporary(context, &value->type);
    left = temporary != NULL ? HlslNewSymbolExpr(context, temporary) : NULL;
    result = temporary != NULL ? HlslNewSymbolExpr(context, temporary) : NULL;
    assignment = HlslNewAssignment(context, left, value);
    if (assignment == NULL || result == NULL ||
        !HlslAppendExpression(context, list, assignment))
    {
        return NULL;
    }
    return result;
} // HlslCaptureValue

static int HlslStabilizeLvalueAddress(HlslLowerContext *context,
                                      HlslStmt **list, HlslExpr *value)
{
    if (value == NULL)
        return 0;
    switch (value->kind) {
    case HLSL_EXPR_SYMBOL:
        return 1;
    case HLSL_EXPR_MEMBER:
        return HlslStabilizeLvalueAddress(context, list,
                                           value->u.member.object);
    case HLSL_EXPR_SWIZZLE:
        return HlslStabilizeLvalueAddress(context, list,
                                           value->u.swizzle.object);
    case HLSL_EXPR_INDEX:
        if (!HlslStabilizeLvalueAddress(context, list,
                                        value->u.index.object))
        {
            return 0;
        }
        value->u.index.index = HlslCaptureValue(context, list,
                                                value->u.index.index);
        return value->u.index.index != NULL;
    default:
        return 0;
    }
} // HlslStabilizeLvalueAddress

int HlslTypeNeedsRecursiveCopy(const HlslType *type)
{
    const HlslDecl *member;

    if (type == NULL)
        return 0;
    if (type->arraySize > 0)
        return 1;
    if (type->base != HLSL_BASE_STRUCT)
        return 0;
    for (member = type->members; member != NULL; member = member->next) {
        if (HlslTypeNeedsRecursiveCopy(&member->type))
            return 1;
    }
    return 0;
} // HlslTypeNeedsRecursiveCopy

int HlslIsStableAggregateSource(const HlslExpr *expression)
{
    if (expression == NULL)
        return 0;
    if (expression->kind == HLSL_EXPR_SYMBOL)
        return 1;
    if (expression->kind == HLSL_EXPR_MEMBER)
        return HlslIsStableAggregateSource(expression->u.member.object);
    return 0;
} // HlslIsStableAggregateSource

static int HlslIsNativeAggregateTempAssignment(const expr *source)
{
    const expr *left;

    if (source == NULL || source->common.kind != BINARY_N ||
        HlslBinaryOperator(source->bin.op) != HLSL_OP_ASSIGN)
    {
        return 0;
    }
    left = source->bin.left;
    return left != NULL && left->common.kind == SYMB_N &&
           left->sym.symbol != NULL &&
           (left->sym.symbol->properties &
            SYMB_IS_NATIVE_AGGREGATE_TEMP) != 0;
} // HlslIsNativeAggregateTempAssignment

HlslExpr *HlslCopyMember(HlslLowerContext *context,
                                HlslExpr *object, HlslDecl *member)
{
    HlslExpr *expression;

    expression = object != NULL && member != NULL ?
        HlslNewSourceExpr(context, HLSL_EXPR_MEMBER, member->type) : NULL;
    if (expression != NULL) {
        expression->u.member.object = object;
        expression->u.member.decl = member;
        expression->u.member.name = member->name;
    }
    return expression;
} // HlslCopyMember

HlslExpr *HlslCopyIndex(HlslLowerContext *context,
                               HlslExpr *object,
                               const HlslType *elementType, int index)
{
    HlslExpr *expression;
    HlslExpr *subscript;

    expression = object != NULL && elementType != NULL ?
        HlslNewSourceExpr(context, HLSL_EXPR_INDEX, *elementType) : NULL;
    subscript = HlslNewLiteral(context, HLSL_BASE_INT, index, 0.0f);
    if (expression == NULL || subscript == NULL)
        return NULL;
    expression->u.index.object = object;
    expression->u.index.index = subscript;
    return expression;
} // HlslCopyIndex

int HlslAppendRecursiveCopy(HlslLowerContext *context,
                                   const HlslType *type,
                                   HlslExpr *target, HlslExpr *source,
                                   HlslStmt **statements)
{
    HlslDecl *member;
    HlslExpr *assignment;
    int i;

    if (type == NULL || target == NULL || source == NULL)
        return 0;
    if (type->arraySize > 0) {
        for (i = 0; i < type->arraySize; i++) {
            if (!HlslAppendRecursiveCopy(context, type->elementType,
                    HlslCopyIndex(context, target, type->elementType, i),
                    HlslCopyIndex(context, source, type->elementType, i),
                    statements))
            {
                return 0;
            }
        }
        return 1;
    }
    if (type->base == HLSL_BASE_STRUCT) {
        for (member = type->members; member != NULL; member = member->next) {
            if (!HlslAppendRecursiveCopy(context, &member->type,
                    HlslCopyMember(context, target, member),
                    HlslCopyMember(context, source, member), statements))
            {
                return 0;
            }
        }
        return 1;
    }
    assignment = HlslNewAssignment(context, target, source);
    return assignment != NULL &&
           HlslAppendExpression(context, statements, assignment);
} // HlslAppendRecursiveCopy

static HlslExpr *HlslDetachLastExpression(HlslStmt **statements)
{
    HlslStmt **place;
    HlslStmt *statement;
    HlslExpr *expression;

    if (statements == NULL || *statements == NULL)
        return NULL;
    place = statements;
    while ((*place)->next != NULL)
        place = &(*place)->next;
    statement = *place;
    if (statement->kind != HLSL_STMT_EXPRESSION)
        return NULL;
    *place = NULL;
    expression = statement->u.expression;
    statement->u.expression = NULL;
    return expression;
} // HlslDetachLastExpression

static HlslExpr *HlslLowerOrderedValue(HlslLowerContext *context,
                                       expr *source, HlslStmt **prefix,
                                       int captureSideEffects)
{
    HlslExpr *value;
    HlslStmt *childPrefix;

    childPrefix = NULL;
    value = HlslLowerExpr(context, source, &childPrefix, 1);
    if (value == NULL)
        return NULL;
    HlslAppendStmt(prefix, childPrefix);
    if (captureSideEffects && source->common.HasSideEffects)
        value = HlslCaptureValue(context, prefix, value);
    return value;
} // HlslLowerOrderedValue

static HlslExpr *HlslNewSwizzle(HlslLowerContext *context,
                                HlslExpr *object, const HlslType *type,
                                const char *mask)
{
    HlslExpr *target;

    target = HlslNewSourceExpr(context, HLSL_EXPR_SWIZZLE, *type);
    if (target != NULL) {
        target->u.swizzle.object = object;
        target->u.swizzle.mask = HlslCopyText(context, mask);
        if (target->u.swizzle.mask == NULL)
            return NULL;
    }
    return target;
} // HlslNewSwizzle

static HlslExpr *HlslComponent(HlslLowerContext *context,
                               HlslExpr *object, int component,
                               HlslBase base)
{
    HlslType type;
    char mask[2];

    if (object->type.len <= 1)
        return object;
    type = HlslNumericType(base, 1);
    mask[0] = "xyzw"[component];
    mask[1] = '\0';
    return HlslNewSwizzle(context, object, &type, mask);
} // HlslComponent

HlslExpr *HlslScalarizeVectorCondition(
    HlslLowerContext *context, HlslStmt **prefix, HlslExpr *condition)
{
    HlslExpr *result;
    HlslExpr *component;
    HlslExpr *combined;
    HlslType boolType;
    int i;

    if (condition == NULL || condition->type.base != HLSL_BASE_BOOL ||
        condition->type.len < 1 || condition->type.len > 4)
    {
        return NULL;
    }
    if (condition->type.len == 1)
        return condition;
    condition = HlslCaptureValue(context, prefix, condition);
    if (condition == NULL)
        return NULL;
    result = HlslComponent(context, condition, 0, HLSL_BASE_BOOL);
    boolType = HlslNumericType(HLSL_BASE_BOOL, 1);
    for (i = 1; i < condition->type.len; i++) {
        component = HlslComponent(context, condition, i,
                                  HLSL_BASE_BOOL);
        combined = HlslNewSourceExpr(context, HLSL_EXPR_BINARY,
                                     boolType);
        if (component == NULL || combined == NULL)
            return NULL;
        combined->u.binary.op = HLSL_OP_LOGICAL_OR;
        combined->u.binary.left = result;
        combined->u.binary.right = component;
        result = combined;
    }
    return result;
} // HlslScalarizeVectorCondition

static HlslExpr *HlslLowerSwizzle(HlslLowerContext *context, expr *source,
                                  const HlslType *type, HlslStmt **prefix,
                                  HlslValueMode valueMode)
{
    HlslExpr *object;
    HlslExpr *target;
    char maskText[5];
    int count;
    int mask;
    int i;

    object = HlslLowerExpr(context, source->un.arg, prefix,
        valueMode == HLSL_VALUE_LVALUE ? HLSL_VALUE_LVALUE :
                                        HLSL_VALUE_RVALUE);
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
        target = HlslNewSourceExpr(context, HLSL_EXPR_CONSTRUCT, *type);
        if (target != NULL)
            target->u.construct.arguments = object;
        return target;
    }
    return HlslNewSwizzle(context, object, type, maskText);
} // HlslLowerSwizzle

static HlslExpr *HlslLowerMatrixSwizzle(HlslLowerContext *context,
                                        expr *source,
                                        const HlslType *type,
                                        HlslStmt **prefix,
                                        HlslValueMode valueMode)
{
    HlslExpr *object;
    HlslExpr *rowExpression;
    HlslExpr *index;
    HlslType rowType;
    int count;
    int mask;
    int selector;
    int row;
    int column;
    int i;
    char columns[5];

    object = HlslLowerExpr(context, source->un.arg, prefix,
        valueMode == HLSL_VALUE_LVALUE ? HLSL_VALUE_LVALUE :
                                        HLSL_VALUE_RVALUE);
    if (object == NULL || object->type.rows <= 0 ||
        object->type.cols <= 0)
    {
        return NULL;
    }
    count = SUBOP_GET_T2(source->un.subop);
    if (count == 0)
        count = 1;
    if (count < 1 || count > 4)
        return NULL;
    mask = SUBOP_GET_MASK16(source->un.subop);
    selector = mask & 15;
    row = (selector >> 2) & 3;
    if (row >= object->type.rows)
        return NULL;
    for (i = 0; i < count; i++) {
        selector = (mask >> (i * 4)) & 15;
        if (((selector >> 2) & 3) != row)
            return NULL;
        column = selector & 3;
        if (column >= object->type.cols)
            return NULL;
        columns[i] = "xyzw"[column];
    }
    columns[count] = '\0';
    rowType = HlslNumericType(HLSL_BASE_FLOAT, object->type.cols);
    rowExpression = HlslNewSourceExpr(context, HLSL_EXPR_INDEX, rowType);
    index = HlslNewLiteral(context, HLSL_BASE_INT, row, 0.0f);
    if (rowExpression == NULL || index == NULL)
        return NULL;
    rowExpression->u.index.object = object;
    rowExpression->u.index.index = index;
    return HlslNewSwizzle(context, rowExpression, type, columns);
} // HlslLowerMatrixSwizzle

static HlslExpr *HlslLowerExprList(HlslLowerContext *context, expr *source,
                                   opcode listOp, HlslStmt **prefix,
                                   Symbol *formal)
{
    HlslExpr *list;
    HlslExpr *item;
    HlslExpr *rest;
    HlslStmt *itemPrefix;
    HlslStmt *restPrefix;
    int preserveLvalue;

    if (source == NULL || source->common.kind != BINARY_N ||
        source->bin.op != listOp)
    {
        return NULL;
    }
    itemPrefix = NULL;
    restPrefix = NULL;
    preserveLvalue = formal != NULL &&
        (GetQualifiers(formal->type) & TYPE_QUALIFIER_OUT);
    item = HlslLowerExpr(context, source->bin.left, &itemPrefix,
        preserveLvalue ? HLSL_VALUE_LVALUE : HLSL_VALUE_RVALUE);
    if (item == NULL)
        return NULL;
    rest = NULL;
    if (source->bin.right != NULL) {
        rest = HlslLowerExprList(context, source->bin.right, listOp,
                                 &restPrefix,
                                 formal != NULL ? formal->next : NULL);
        if (rest == NULL)
            return NULL;
    }
    HlslAppendStmt(prefix, itemPrefix);
    if (source->bin.right != NULL) {
        if (preserveLvalue &&
            (restPrefix != NULL ||
             source->bin.right->common.HasSideEffects))
        {
            if (!HlslStabilizeLvalueAddress(context, prefix, item))
                return NULL;
        } else if (!preserveLvalue &&
                   (source->bin.left->common.HasSideEffects ||
                    restPrefix != NULL ||
                    source->bin.right->common.HasSideEffects))
        {
            item = HlslCaptureValue(context, prefix, item);
            if (item == NULL)
                return NULL;
        }
    }
    HlslAppendStmt(prefix, restPrefix);
    list = item;
    HlslAppendExpr(&list, rest);
    return list;
} // HlslLowerExprList

static HlslExpr *HlslLowerCall(HlslLowerContext *context, expr *source,
                               const HlslType *type, HlslStmt **prefix)
{
    typedef struct HlslCopyOut_Rec {
        struct HlslCopyOut_Rec *next;
        HlslExpr *target;
        HlslDecl *temporary;
        HlslType type;
    } HlslCopyOut;

    HlslExpr *target;
    HlslExpr *argument;
    HlslExpr *nextArgument;
    HlslExpr **argumentPlace;
    HlslExpr *replacement;
    HlslExpr *result;
    HlslDecl *temporary;
    HlslFunction *function;
    HlslCopyOut *copyOut;
    HlslCopyOut **copyOutTail;
    HlslStmt *copies;
    Symbol *symbol;
    Symbol *formal;
    HlslBuiltin builtin;
    HlslBuiltinLowering lowering;
    HlslType builtinResult;
    HlslType builtinParams[HLSL_MAX_BUILTIN_ARGS];
    int builtinParamCount;
    int builtinStatus;

    if (source->bin.left == NULL ||
        source->bin.left->common.kind != SYMB_N)
    {
        return NULL;
    }
    symbol = source->bin.left->sym.symbol;
    builtinStatus = HlslResolveBuiltinSymbol(context, symbol,
        GetExprCallSite(source), &builtin, &builtinResult,
        builtinParams, &builtinParamCount);
    if (builtinStatus < 0)
        return NULL;
    if (builtinStatus > 0) {
        HlslTextureForm textureForm;
        HlslExpr *coordinate;
        HlslExpr *coordinateNext;

        target = HlslNewSourceExpr(context, HLSL_EXPR_CALL, *type);
        if (target == NULL)
            return NULL;
        target->u.call.arguments = HlslLowerExprList(context,
            source->bin.right, FUN_ARG_OP, prefix, NULL);
        if (source->bin.right != NULL &&
            target->u.call.arguments == NULL)
        {
            return NULL;
        }
        textureForm = HlslBuiltinTextureForm(builtin);
        if (context->profile->resourcePolicy ==
                HLSL_RESOURCE_POLICY_MODERN &&
            (textureForm == HLSL_TEXTURE_PROJECTED ||
             textureForm == HLSL_TEXTURE_BIAS ||
             textureForm == HLSL_TEXTURE_LOD))
        {
            coordinate = target->u.call.arguments != NULL ?
                target->u.call.arguments->next : NULL;
            if (coordinate == NULL)
                return NULL;
            coordinateNext = coordinate->next;
            coordinate->next = NULL;
            coordinate = HlslCaptureValue(context, prefix, coordinate);
            if (coordinate == NULL)
                return NULL;
            coordinate->next = coordinateNext;
            target->u.call.arguments->next = coordinate;
        }
        lowering = HlslBuiltinLoweringKind(builtin);
        if (lowering == HLSL_BUILTIN_LOWER_NATIVE) {
            target->u.call.name = HlslBuiltinSpelling(builtin);
            target->u.call.builtin = builtin;
        } else {
            function = HlslCreateBuiltinHelper(context, builtin,
                                               &builtinResult);
            if (function == NULL) {
                HlslLowerFailure(context, HLSL_ERROR_INTRINSIC,
                                 HlslBuiltinSpelling(builtin),
                                 GetExprCallSite(source));
                return NULL;
            }
            target->u.call.function = function;
            target->u.call.name = function->name;
        }
        target->hasSideEffects = source->common.HasSideEffects;
        return target;
    }
    function = HlslFindFunction(context->module, symbol);
    if (function == NULL)
        return NULL;
    target = HlslNewSourceExpr(context, HLSL_EXPR_CALL, *type);
    if (target == NULL)
        return NULL;
    target->u.call.function = function;
    target->u.call.name = function->name;
    target->u.call.arguments = HlslLowerExprList(context,
        source->bin.right, FUN_ARG_OP, prefix,
        symbol->details.fun.params);
    if (source->bin.right != NULL && target->u.call.arguments == NULL)
        return NULL;
    target->hasSideEffects = source->common.HasSideEffects;

    copyOut = NULL;
    copyOutTail = &copyOut;
    argumentPlace = &target->u.call.arguments;
    formal = symbol->details.fun.params;
    while (*argumentPlace != NULL && formal != NULL) {
        argument = *argumentPlace;
        nextArgument = argument->next;
        if ((GetQualifiers(formal->type) & TYPE_QUALIFIER_OUT) &&
            HlslTypeNeedsRecursiveCopy(&argument->type))
        {
            argument->next = NULL;
            temporary = HlslNewTemporary(context, &argument->type);
            replacement = temporary != NULL ?
                HlslNewSymbolExpr(context, temporary) : NULL;
            *copyOutTail = (HlslCopyOut *) HlslLowerAlloc(
                context, sizeof(HlslCopyOut));
            if (replacement == NULL || *copyOutTail == NULL)
                return NULL;
            (*copyOutTail)->target = argument;
            (*copyOutTail)->temporary = temporary;
            (*copyOutTail)->type = argument->type;
            copyOutTail = &(*copyOutTail)->next;
            if ((GetQualifiers(formal->type) & TYPE_QUALIFIER_INOUT) ==
                    TYPE_QUALIFIER_INOUT &&
                !HlslAppendRecursiveCopy(context, &argument->type,
                    HlslNewSymbolExpr(context, temporary), argument,
                    prefix))
            {
                return NULL;
            }
            replacement->next = nextArgument;
            *argumentPlace = replacement;
        }
        argumentPlace = &(*argumentPlace)->next;
        formal = formal->next;
    }
    if (*argumentPlace != NULL || formal != NULL)
        return NULL;
    if (copyOut == NULL)
        return target;

    if (type->base == HLSL_BASE_VOID) {
        if (!HlslAppendExpression(context, prefix, target))
            return NULL;
        result = NULL;
    } else {
        temporary = HlslNewTemporary(context, type);
        result = temporary != NULL ?
            HlslNewSymbolExpr(context, temporary) : NULL;
        if (temporary == NULL || result == NULL ||
            !HlslAppendExpression(context, prefix,
                HlslNewAssignment(context,
                    HlslNewSymbolExpr(context, temporary), target)))
        {
            return NULL;
        }
    }
    copies = NULL;
    for (; copyOut != NULL; copyOut = copyOut->next) {
        if (!HlslAppendRecursiveCopy(context, &copyOut->type,
                copyOut->target,
                HlslNewSymbolExpr(context, copyOut->temporary), &copies))
        {
            return NULL;
        }
    }
    if (type->base != HLSL_BASE_VOID) {
        HlslAppendStmt(prefix, copies);
        return result;
    }
    result = HlslDetachLastExpression(&copies);
    HlslAppendStmt(prefix, copies);
    return result;
} // HlslLowerCall

static HlslExpr *HlslLowerConditional(HlslLowerContext *context,
                                      expr *source, const HlslType *type,
                                      HlslStmt **prefix)
{
    HlslExpr *target;
    HlslExpr *condition;
    HlslExpr *trueExpr;
    HlslExpr *falseExpr;
    HlslExpr *componentExpr;
    HlslType componentType;
    int conditionLen;
    int i;

    conditionLen = 0;
    IsVector(source->tri.arg1->common.type, &conditionLen);
    condition = HlslLowerExpr(context, source->tri.arg1, prefix, 1);
    if (condition == NULL)
        return NULL;
    if (conditionLen <= 1 &&
        !source->tri.arg2->common.HasSideEffects &&
        !source->tri.arg3->common.HasSideEffects)
    {
        trueExpr = HlslLowerExpr(context, source->tri.arg2, prefix, 1);
        falseExpr = HlslLowerExpr(context, source->tri.arg3, prefix, 1);
        target = HlslNewSourceExpr(context, HLSL_EXPR_CONDITIONAL, *type);
        if (target == NULL || trueExpr == NULL || falseExpr == NULL)
            return NULL;
        target->u.conditional.condition = condition;
        target->u.conditional.trueExpr = trueExpr;
        target->u.conditional.falseExpr = falseExpr;
        return target;
    }
    if (conditionLen <= 1) {
        condition = HlslCaptureValue(context, prefix, condition);
        trueExpr = HlslLowerExpr(context, source->tri.arg2, prefix, 1);
        if (trueExpr != NULL)
            trueExpr = HlslCaptureValue(context, prefix, trueExpr);
        falseExpr = HlslLowerExpr(context, source->tri.arg3, prefix, 1);
        if (falseExpr != NULL)
            falseExpr = HlslCaptureValue(context, prefix, falseExpr);
        target = HlslNewSourceExpr(context, HLSL_EXPR_CONDITIONAL, *type);
        if (target == NULL || condition == NULL ||
            trueExpr == NULL || falseExpr == NULL)
        {
            return NULL;
        }
        target->u.conditional.condition = condition;
        target->u.conditional.trueExpr = trueExpr;
        target->u.conditional.falseExpr = falseExpr;
        return target;
    }
    if (conditionLen != type->len || type->len < 2 || type->len > 4)
        return NULL;
    condition = HlslCaptureValue(context, prefix, condition);
    trueExpr = HlslLowerExpr(context, source->tri.arg2, prefix, 1);
    if (trueExpr != NULL)
        trueExpr = HlslCaptureValue(context, prefix, trueExpr);
    falseExpr = HlslLowerExpr(context, source->tri.arg3, prefix, 1);
    if (falseExpr != NULL)
        falseExpr = HlslCaptureValue(context, prefix, falseExpr);
    if (condition == NULL || trueExpr == NULL || falseExpr == NULL)
        return NULL;
    target = HlslNewSourceExpr(context, HLSL_EXPR_CONSTRUCT, *type);
    if (target == NULL)
        return NULL;
    componentType = HlslNumericType(type->base, 1);
    for (i = 0; i < type->len; i++) {
        componentExpr = HlslNewSourceExpr(context,
            HLSL_EXPR_CONDITIONAL, componentType);
        if (componentExpr == NULL)
            return NULL;
        componentExpr->u.conditional.condition = HlslComponent(
            context, condition, i, HLSL_BASE_BOOL);
        componentExpr->u.conditional.trueExpr = HlslComponent(
            context, trueExpr, i, type->base);
        componentExpr->u.conditional.falseExpr = HlslComponent(
            context, falseExpr, i, type->base);
        if (componentExpr->u.conditional.condition == NULL ||
            componentExpr->u.conditional.trueExpr == NULL ||
            componentExpr->u.conditional.falseExpr == NULL)
        {
            return NULL;
        }
        HlslAppendExpr(&target->u.construct.arguments, componentExpr);
    }
    return target;
} // HlslLowerConditional

static HlslExpr *HlslLowerVectorComparison(HlslLowerContext *context,
    expr *source, const HlslType *type, HlslOperator op, HlslStmt **prefix)
{
    HlslExpr *target;
    HlslExpr *left;
    HlslExpr *right;
    HlslExpr *component;
    HlslType componentType;
    int i;

    left = HlslLowerOrderedValue(context, source->bin.left, prefix, 0);
    if (left != NULL)
        left = HlslCaptureValue(context, prefix, left);
    right = HlslLowerOrderedValue(context, source->bin.right, prefix, 0);
    if (right != NULL)
        right = HlslCaptureValue(context, prefix, right);
    if (left == NULL || right == NULL)
        return NULL;
    target = HlslNewSourceExpr(context, HLSL_EXPR_CONSTRUCT, *type);
    if (target == NULL)
        return NULL;
    componentType = HlslNumericType(HLSL_BASE_BOOL, 1);
    for (i = 0; i < type->len; i++) {
        component = HlslNewSourceExpr(context, HLSL_EXPR_BINARY,
                                      componentType);
        if (component == NULL)
            return NULL;
        component->u.binary.op = op;
        component->u.binary.left = HlslComponent(context, left, i,
                                                  left->type.base);
        component->u.binary.right = HlslComponent(context, right, i,
                                                   right->type.base);
        if (component->u.binary.left == NULL ||
            component->u.binary.right == NULL)
        {
            return NULL;
        }
        HlslAppendExpr(&target->u.construct.arguments, component);
    }
    return target;
} // HlslLowerVectorComparison

HlslExpr *HlslLowerExpr(HlslLowerContext *context, expr *source,
                               HlslStmt **prefix, HlslValueMode valueMode)
{
    HlslExpr *target;
    HlslDecl *decl;
    HlslType type;
    HlslOperator op;
    Symbol *member;
    HlslExpr *left;
    HlslExpr *right;
    HlslStmt *leftStatement;
    HlslStmt *leftPrefix;
    HlslStmt *rightPrefix;

    if (source != NULL && source->common.kind == BINARY_N &&
        source->bin.op == COMMA_OP)
    {
        left = HlslLowerExpr(context, source->bin.left, prefix,
                             HLSL_VALUE_DISCARD);
        if (left == NULL)
            return NULL;
        leftStatement = HlslNewExpressionStmt(context, left);
        if (leftStatement == NULL)
            return NULL;
        HlslAppendStmt(prefix, leftStatement);
        return HlslLowerExpr(context, source->bin.right, prefix,
                             valueMode);
    }
    if (source == NULL ||
        !HlslEnsureType(context, source->common.type) ||
        !HlslLowerType(context, source->common.type, &type, NULL))
    {
        return NULL;
    }
    if (source->common.kind == SYMB_N && source->sym.op == VARIABLE_OP) {
        decl = HlslFindDecl(context, source->sym.symbol);
        if (decl == NULL && context->function != NULL &&
            source->sym.symbol != NULL)
        {
            const char *sourceName;

            sourceName = GetAtomString(atable, source->sym.symbol->name);
            if (sourceName != NULL &&
                (sourceName[0] == '$' || sourceName[0] == '@'))
            {
                decl = HlslNewSourceDecl(context, source->sym.symbol,
                                         context->function->identity);
                if (decl != NULL)
                    HlslInsertDecl(&context->function->locals, decl);
            }
        }
        if (decl == NULL)
            return NULL;
        target = HlslNewSourceExpr(context, HLSL_EXPR_SYMBOL, type);
        if (target != NULL) {
            target->u.symbol = decl;
            target->hasSideEffects = source->common.HasSideEffects;
        }
        return target;
    }
    if (source->common.kind == CONST_N)
        return HlslLowerConstant(context, source, &type);
    if (source->common.kind == UNARY_N) {
        if (source->un.op == CAST_STRUCT_OP ||
            ((source->un.op == CAST_CS_OP ||
              source->un.op == CAST_CV_OP ||
              source->un.op == CAST_CM_OP ||
              source->un.op == CAST_SHAPE_OP) &&
             (type.arraySize > 0 || type.rows > 0 || type.cols > 0)))
        {
            HlslLowerFailure(context, HLSL_ERROR_UNSUPPORTED_OPERATION,
                             "aggregate cast", &context->statementLoc);
            return NULL;
        }
        if (source->un.op == SWIZZLE_Z_OP)
            return HlslLowerSwizzle(context, source, &type, prefix,
                                    valueMode);
        if (source->un.op == SWIZMAT_Z_OP)
            return HlslLowerMatrixSwizzle(context, source, &type, prefix,
                                          valueMode);
        if (source->un.op == VECTOR_V_OP) {
            target = HlslNewSourceExpr(context, HLSL_EXPR_CONSTRUCT, type);
            if (target == NULL)
                return NULL;
            target->u.construct.arguments = HlslLowerExprList(context,
                source->un.arg, EXPR_LIST_OP, prefix, NULL);
            return target->u.construct.arguments != NULL ? target : NULL;
        }
        if (source->un.op == CAST_CS_OP ||
            source->un.op == CAST_CV_OP ||
            source->un.op == CAST_CM_OP ||
            source->un.op == CAST_SHAPE_OP)
        {
            target = HlslNewSourceExpr(context, HLSL_EXPR_CAST, type);
            if (target == NULL)
                return NULL;
            target->u.cast.expression = HlslLowerExpr(
                context, source->un.arg, prefix, 1);
            return target->u.cast.expression != NULL ? target : NULL;
        }
        op = HlslUnaryOperator(source->un.op);
        if (op != HLSL_OP_NONE) {
            target = HlslNewSourceExpr(context, HLSL_EXPR_UNARY, type);
            if (target == NULL)
                return NULL;
            target->u.unary.op = op;
            target->u.unary.operand = HlslLowerExpr(
                context, source->un.arg, prefix, 1);
            target->hasSideEffects = source->common.HasSideEffects;
            return target->u.unary.operand != NULL ? target : NULL;
        }
    }
    if (source->common.kind == BINARY_N) {
        if (source->bin.op == MEMBER_SELECTOR_OP) {
            if (source->bin.right == NULL ||
                source->bin.right->common.kind != SYMB_N ||
                source->bin.right->sym.op != MEMBER_OP)
            {
                return NULL;
            }
            member = source->bin.right->sym.symbol;
            decl = HlslFindDecl(context, member);
            if (decl == NULL)
                return NULL;
            target = HlslNewSourceExpr(context, HLSL_EXPR_MEMBER, type);
            if (target == NULL)
                return NULL;
            target->u.member.object = HlslLowerExpr(
                context, source->bin.left, prefix,
                valueMode == HLSL_VALUE_LVALUE ? HLSL_VALUE_LVALUE :
                                                HLSL_VALUE_RVALUE);
            target->u.member.decl = decl;
            target->u.member.name = decl->name;
            return target->u.member.object != NULL ? target : NULL;
        }
        if (source->bin.op == ARRAY_INDEX_OP) {
            target = HlslNewSourceExpr(context, HLSL_EXPR_INDEX, type);
            if (target == NULL)
                return NULL;
            leftPrefix = NULL;
            rightPrefix = NULL;
            target->u.index.object = HlslLowerExpr(
                context, source->bin.left, &leftPrefix,
                valueMode == HLSL_VALUE_LVALUE ? HLSL_VALUE_LVALUE :
                                                HLSL_VALUE_RVALUE);
            target->u.index.index = HlslLowerOrderedValue(
                context, source->bin.right, &rightPrefix, 1);
            if (target->u.index.object == NULL ||
                target->u.index.index == NULL)
            {
                return NULL;
            }
            HlslAppendStmt(prefix, leftPrefix);
            if (valueMode != HLSL_VALUE_LVALUE &&
                (source->bin.left->common.HasSideEffects ||
                 rightPrefix != NULL ||
                 source->bin.right->common.HasSideEffects))
            {
                target->u.index.object = HlslCaptureValue(
                    context, prefix, target->u.index.object);
                if (target->u.index.object == NULL)
                    return NULL;
            }
            HlslAppendStmt(prefix, rightPrefix);
            return target;
        }
        if (source->bin.op == FUN_CALL_OP ||
            source->bin.op == FUN_INTRINSIC_OP)
            return HlslLowerCall(context, source, &type, prefix);
        op = HlslBinaryOperator(source->bin.op);
        if (op != HLSL_OP_NONE) {
            if (HlslIsComparison(op) && type.base == HLSL_BASE_BOOL &&
                type.len > 1)
            {
                return HlslLowerVectorComparison(context, source, &type,
                                                  op, prefix);
            }
            leftPrefix = NULL;
            rightPrefix = NULL;
            left = HlslLowerExpr(context, source->bin.left,
                &leftPrefix, HlslIsAssignmentOperator(op) ?
                             HLSL_VALUE_LVALUE : HLSL_VALUE_RVALUE);
            right = HlslLowerExpr(context, source->bin.right,
                                  &rightPrefix, 1);
            if (left == NULL || right == NULL)
                return NULL;
            HlslAppendStmt(prefix, leftPrefix);
            if (!HlslIsAssignmentOperator(op) &&
                (source->bin.left->common.HasSideEffects ||
                 rightPrefix != NULL ||
                 source->bin.right->common.HasSideEffects))
            {
                left = HlslCaptureValue(context, prefix, left);
                if (left == NULL)
                    return NULL;
            }
            if (HlslIsAssignmentOperator(op) &&
                (rightPrefix != NULL ||
                 source->bin.right->common.HasSideEffects) &&
                !HlslStabilizeLvalueAddress(context, prefix, left))
            {
                return NULL;
            }
            HlslAppendStmt(prefix, rightPrefix);
            if (op == HLSL_OP_ASSIGN &&
                HlslTypeNeedsRecursiveCopy(&type) &&
                HlslIsNativeAggregateTempAssignment(source))
            {
                target = HlslNewSourceExpr(context, HLSL_EXPR_BINARY, type);
                if (target == NULL)
                    return NULL;
                target->u.binary.op = op;
                target->u.binary.left = left;
                target->u.binary.right = right;
                target->hasSideEffects = source->common.HasSideEffects;
                return target;
            }
            if (op == HLSL_OP_ASSIGN &&
                HlslTypeNeedsRecursiveCopy(&type))
            {
                HlslStmt *copies;

                if (!HlslStabilizeLvalueAddress(context, prefix, left))
                    return NULL;
                copies = NULL;
                if (valueMode == HLSL_VALUE_RVALUE) {
                    HlslDecl *temporary;

                    temporary = HlslNewTemporary(context, &type);
                    if (temporary == NULL ||
                        !HlslAppendRecursiveCopy(context, &type,
                            HlslNewSymbolExpr(context, temporary), right,
                            &copies) ||
                        !HlslAppendRecursiveCopy(context, &type, left,
                            HlslNewSymbolExpr(context, temporary), &copies))
                    {
                        return NULL;
                    }
                    HlslAppendStmt(prefix, copies);
                    return HlslNewSymbolExpr(context, temporary);
                }
                if (!HlslAppendRecursiveCopy(context, &type, left, right,
                                             &copies))
                {
                    return NULL;
                }
                target = HlslDetachLastExpression(&copies);
                HlslAppendStmt(prefix, copies);
                return target;
            }
            if (op == HLSL_OP_ASSIGN &&
                valueMode == HLSL_VALUE_RVALUE)
            {
                right = HlslCaptureValue(context, prefix, right);
                target = HlslNewAssignment(context, left, right);
                if (target == NULL ||
                    !HlslAppendExpression(context, prefix, target))
                {
                    return NULL;
                }
                return HlslNewSymbolExpr(context, right->u.symbol);
            }
            target = HlslNewSourceExpr(context, HLSL_EXPR_BINARY, type);
            if (target == NULL)
                return NULL;
            target->u.binary.op = op;
            target->u.binary.left = left;
            target->u.binary.right = right;
            target->hasSideEffects = source->common.HasSideEffects;
            return target;
        }
    }
    if (source->common.kind == TRINARY_N &&
        (source->tri.op == COND_OP || source->tri.op == COND_V_OP ||
         source->tri.op == COND_SV_OP || source->tri.op == COND_GEN_OP))
    {
        return HlslLowerConditional(context, source, &type, prefix);
    }
    HlslLowerFailure(context, HLSL_ERROR_UNSUPPORTED_OPERATION,
                     "HLSL expression", NULL);
    return NULL;
} // HlslLowerExpr

static HlslOperator HlslIROperator(CgIROp op)
{
    switch (op) {
    case CGIR_OP_ASSIGN: return HLSL_OP_ASSIGN;
    case CGIR_OP_ADD_ASSIGN: return HLSL_OP_ADD_ASSIGN;
    case CGIR_OP_SUBTRACT_ASSIGN: return HLSL_OP_SUBTRACT_ASSIGN;
    case CGIR_OP_MULTIPLY_ASSIGN: return HLSL_OP_MULTIPLY_ASSIGN;
    case CGIR_OP_DIVIDE_ASSIGN: return HLSL_OP_DIVIDE_ASSIGN;
    case CGIR_OP_MODULO_ASSIGN: return HLSL_OP_REMAINDER_ASSIGN;
    case CGIR_OP_LOGICAL_OR: return HLSL_OP_LOGICAL_OR;
    case CGIR_OP_LOGICAL_AND: return HLSL_OP_LOGICAL_AND;
    case CGIR_OP_BITWISE_OR: return HLSL_OP_BITWISE_OR;
    case CGIR_OP_BITWISE_XOR: return HLSL_OP_BITWISE_XOR;
    case CGIR_OP_BITWISE_AND: return HLSL_OP_BITWISE_AND;
    case CGIR_OP_EQUAL: return HLSL_OP_EQUAL;
    case CGIR_OP_NOT_EQUAL: return HLSL_OP_NOT_EQUAL;
    case CGIR_OP_LESS: return HLSL_OP_LESS;
    case CGIR_OP_GREATER: return HLSL_OP_GREATER;
    case CGIR_OP_LESS_EQUAL: return HLSL_OP_LESS_EQUAL;
    case CGIR_OP_GREATER_EQUAL: return HLSL_OP_GREATER_EQUAL;
    case CGIR_OP_SHIFT_LEFT: return HLSL_OP_SHIFT_LEFT;
    case CGIR_OP_SHIFT_RIGHT: return HLSL_OP_SHIFT_RIGHT;
    case CGIR_OP_ADD: return HLSL_OP_ADD;
    case CGIR_OP_SUBTRACT: return HLSL_OP_SUBTRACT;
    case CGIR_OP_MULTIPLY: return HLSL_OP_MULTIPLY;
    case CGIR_OP_DIVIDE: return HLSL_OP_DIVIDE;
    case CGIR_OP_MODULO: return HLSL_OP_REMAINDER;
    case CGIR_OP_NEGATE: return HLSL_OP_NEGATE;
    case CGIR_OP_POSITIVE: return HLSL_OP_POSITIVE;
    case CGIR_OP_LOGICAL_NOT: return HLSL_OP_LOGICAL_NOT;
    case CGIR_OP_BITWISE_NOT: return HLSL_OP_BITWISE_NOT;
    case CGIR_OP_PRE_INCREMENT: return HLSL_OP_PRE_INCREMENT;
    case CGIR_OP_PRE_DECREMENT: return HLSL_OP_PRE_DECREMENT;
    case CGIR_OP_POST_INCREMENT: return HLSL_OP_POST_INCREMENT;
    case CGIR_OP_POST_DECREMENT: return HLSL_OP_POST_DECREMENT;
    case CGIR_OP_NONE: break;
    }
    return HLSL_OP_NONE;
} // HlslIROperator

/*
 * HlslLowerIRExprList() - Lower a CgIR argument/constructor list while
 *         making its left-to-right evaluation explicit.  A value is captured
 *         before any later prefix or side effect can observe changed state;
 *         writable actuals keep their lvalue and only stabilize its address.
 */

static HlslExpr *HlslLowerIRExprList(HlslLowerContext *context,
                                      const CgIRExpr *source,
                                      HlslStmt **prefix,
                                      const Symbol *formal,
                                      const TypeList *formalType)
{
    HlslExpr *head;
    HlslExpr *item;
    HlslExpr *rest;
    HlslStmt *itemPrefix;
    HlslStmt *restPrefix;
    const CgIRExpr *cursor;
    int laterSideEffects;
    int preserveLvalue;

    if (source == NULL)
        return NULL;
    itemPrefix = NULL;
    restPrefix = NULL;
    preserveLvalue =
        (formal != NULL &&
         (GetQualifiers(formal->type) & TYPE_QUALIFIER_OUT)) ||
        (formalType != NULL &&
         (GetQualifiers(formalType->type) & TYPE_QUALIFIER_OUT));
    item = HlslLowerIRExpr(context, source, &itemPrefix,
        preserveLvalue ? HLSL_VALUE_LVALUE : HLSL_VALUE_RVALUE);
    if (item == NULL)
        return NULL;
    rest = NULL;
    if (source->next != NULL) {
        rest = HlslLowerIRExprList(context, source->next, &restPrefix,
            formal != NULL ? formal->next : NULL,
            formalType != NULL ? formalType->next : NULL);
        if (rest == NULL)
            return NULL;
    }
    HlslAppendStmt(prefix, itemPrefix);
    if (source->next != NULL) {
        laterSideEffects = 0;
        for (cursor = source->next; cursor != NULL; cursor = cursor->next) {
            if (cursor->sideEffects) {
                laterSideEffects = 1;
                break;
            }
        }
        if (preserveLvalue &&
            (restPrefix != NULL || laterSideEffects))
        {
            if (!HlslStabilizeLvalueAddress(context, prefix, item))
                return NULL;
        } else if (!preserveLvalue &&
                   (source->sideEffects || restPrefix != NULL ||
                    laterSideEffects))
        {
            item = HlslCaptureValue(context, prefix, item);
            if (item == NULL)
                return NULL;
        }
    }
    HlslAppendStmt(prefix, restPrefix);
    head = item;
    HlslAppendExpr(&head, rest);
    return head;
} // HlslLowerIRExprList

static HlslExpr *HlslLowerIRConstant(HlslLowerContext *context,
                                      const CgIRExpr *source,
                                      const HlslType *type)
{
    CgScalarKind kind;

    kind = source->u.constant.kind;
    if (type->base == HLSL_BASE_FLOAT) {
        if (kind == CG_SCALAR_FLOAT || kind == CG_SCALAR_HALF ||
            kind == CG_SCALAR_FIXED || kind == CG_SCALAR_CFLOAT ||
            kind == CG_SCALAR_DOUBLE)
        {
            return HlslNewLiteral(context, HLSL_BASE_FLOAT, 0,
                                  source->u.constant.value.f);
        }
        return HlslNewLiteral(context, HLSL_BASE_FLOAT, 0,
                              (float) source->u.constant.value.i);
    }
    return HlslNewLiteral(context, type->base,
                          (int) source->u.constant.value.i, 0.0f);
} // HlslLowerIRConstant

int HlslIRTypeIsIdentical(const HlslType *left,
                                 const HlslType *right)
{
    return left != NULL && right != NULL &&
           left->base == right->base && left->len == right->len &&
           left->rows == right->rows && left->cols == right->cols &&
           left->arraySize == right->arraySize &&
           left->elementType == right->elementType &&
           left->structName == right->structName &&
           left->members == right->members;
} // HlslIRTypeIsIdentical

static HlslExpr *HlslLowerIRCall(HlslLowerContext *context,
                                  const CgIRExpr *source,
                                  const HlslType *type,
                                  HlslStmt **prefix)
{
    HlslExpr *target;
    HlslFunction *function;
    HlslBuiltin builtin;
    HlslBuiltinLowering lowering;
    HlslType builtinResult;
    HlslType builtinParams[HLSL_MAX_BUILTIN_ARGS];
    Symbol *symbol;
    int builtinParamCount;
    int builtinStatus;

    symbol = source->u.call.callee;
    builtinStatus = HlslResolveBuiltinSymbol(context, symbol,
        &source->loc, &builtin, &builtinResult, builtinParams,
        &builtinParamCount);
    if (builtinStatus < 0)
        return NULL;
    target = HlslNewSourceExpr(context, HLSL_EXPR_CALL, *type);
    if (target == NULL)
        return NULL;
    target->u.call.arguments = HlslLowerIRExprList(
        context, source->u.call.arguments, prefix,
        symbol != NULL ? symbol->details.fun.params : NULL, NULL);
    if (source->u.call.arguments != NULL &&
        target->u.call.arguments == NULL)
    {
        return NULL;
    }
    if (builtinStatus > 0) {
        HlslTextureForm textureForm;
        HlslExpr *coordinate;
        HlslExpr *coordinateNext;

        textureForm = HlslBuiltinTextureForm(builtin);
        if (context->profile->resourcePolicy ==
                HLSL_RESOURCE_POLICY_MODERN &&
            (textureForm == HLSL_TEXTURE_PROJECTED ||
             textureForm == HLSL_TEXTURE_BIAS ||
             textureForm == HLSL_TEXTURE_LOD))
        {
            coordinate = target->u.call.arguments != NULL ?
                target->u.call.arguments->next : NULL;
            if (coordinate == NULL)
                return NULL;
            coordinateNext = coordinate->next;
            coordinate->next = NULL;
            coordinate = HlslCaptureValue(context, prefix, coordinate);
            if (coordinate == NULL)
                return NULL;
            coordinate->next = coordinateNext;
            target->u.call.arguments->next = coordinate;
        }
        lowering = HlslBuiltinLoweringKind(builtin);
        if (lowering == HLSL_BUILTIN_LOWER_NATIVE) {
            target->u.call.name = HlslBuiltinSpelling(builtin);
            target->u.call.builtin = builtin;
        } else {
            function = HlslCreateBuiltinHelper(context, builtin,
                                               &builtinResult);
            if (function == NULL)
                return NULL;
            target->u.call.function = function;
            target->u.call.name = function->name;
        }
    } else {
        function = HlslFindFunction(context->module, symbol);
        if (function == NULL)
            return NULL;
        target->u.call.function = function;
        target->u.call.name = function->name;
    }
    target->hasSideEffects = source->sideEffects;
    return target;
} // HlslLowerIRCall

static HlslExpr *HlslLowerIRIntrinsic(HlslLowerContext *context,
                                      const CgIRExpr *source,
                                      const HlslType *type,
                                      HlslStmt **prefix)
{
    HlslExpr *target;
    HlslFunction *function;
    HlslBuiltin builtin;
    HlslBuiltinLowering lowering;
    HlslType builtinResult;
    HlslType builtinParams[HLSL_MAX_BUILTIN_ARGS];
    int builtinParamCount;
    HlslTextureForm textureForm;
    HlslExpr *coordinate;
    HlslExpr *coordinateNext;

    if (source->u.intrinsicCall.signature == NULL ||
        HlslResolveBuiltinSignature(context,
            source->u.intrinsicCall.signature, &source->loc, &builtin,
            &builtinResult, builtinParams, &builtinParamCount) <= 0)
    {
        return NULL;
    }
    target = HlslNewSourceExpr(context, HLSL_EXPR_CALL, *type);
    if (target == NULL)
        return NULL;
    target->u.call.arguments = HlslLowerIRExprList(context,
        source->u.intrinsicCall.arguments, prefix, NULL,
        source->u.intrinsicCall.signature->parameters);
    if (source->u.intrinsicCall.arguments != NULL &&
        target->u.call.arguments == NULL)
    {
        return NULL;
    }
    textureForm = HlslBuiltinTextureForm(builtin);
    if (context->profile->resourcePolicy == HLSL_RESOURCE_POLICY_MODERN &&
        (textureForm == HLSL_TEXTURE_PROJECTED ||
         textureForm == HLSL_TEXTURE_BIAS ||
         textureForm == HLSL_TEXTURE_LOD))
    {
        coordinate = target->u.call.arguments != NULL ?
            target->u.call.arguments->next : NULL;
        if (coordinate == NULL)
            return NULL;
        coordinateNext = coordinate->next;
        coordinate->next = NULL;
        coordinate = HlslCaptureValue(context, prefix, coordinate);
        if (coordinate == NULL)
            return NULL;
        coordinate->next = coordinateNext;
        target->u.call.arguments->next = coordinate;
    }
    lowering = HlslBuiltinLoweringKind(builtin);
    if (lowering == HLSL_BUILTIN_LOWER_NATIVE) {
        target->u.call.name = HlslBuiltinSpelling(builtin);
        target->u.call.builtin = builtin;
    } else {
        function = HlslCreateBuiltinHelper(context, builtin, &builtinResult);
        if (function == NULL) {
            HlslLowerFailure(context, HLSL_ERROR_INTRINSIC,
                             HlslBuiltinSpelling(builtin), &source->loc);
            return NULL;
        }
        target->u.call.function = function;
        target->u.call.name = function->name;
    }
    target->hasSideEffects = source->sideEffects;
    return target;
} // HlslLowerIRIntrinsic

HlslExpr *HlslLowerIRExpr(HlslLowerContext *context,
                                 const CgIRExpr *source,
                                 HlslStmt **prefix,
                                 HlslValueMode valueMode)
{
    HlslExpr *target;
    HlslExpr *left;
    HlslExpr *right;
    HlslExpr *component;
    HlslDecl *decl;
    HlslDecl *temporary;
    HlslOperator op;
    HlslType type;
    HlslType componentType;
    HlslStmt *leftPrefix;
    HlslStmt *rightPrefix;
    HlslStmt *truePrefix;
    HlslStmt *falsePrefix;
    HlslStmt *copies;
    char mask[5];
    int index;

    if (context == NULL || source == NULL ||
        !HlslLowerType(context, source->type, &type, &source->loc))
    {
        return NULL;
    }
    context->statementLoc = source->loc;
    switch (source->kind) {
    case CGIR_EXPR_CONSTANT:
        return HlslLowerIRConstant(context, source, &type);
    case CGIR_EXPR_SYMBOL:
        decl = HlslFindDecl(context, source->u.symbol);
        return decl != NULL ? HlslNewSymbolExpr(context, decl) : NULL;
    case CGIR_EXPR_MEMBER:
        left = HlslLowerIRExpr(context, source->u.member.object, prefix,
            valueMode == HLSL_VALUE_LVALUE ? HLSL_VALUE_LVALUE :
                                             HLSL_VALUE_RVALUE);
        decl = HlslFindDecl(context, source->u.member.member);
        target = left != NULL && decl != NULL ?
            HlslNewSourceExpr(context, HLSL_EXPR_MEMBER, type) : NULL;
        if (target != NULL) {
            target->u.member.object = left;
            target->u.member.decl = decl;
            target->u.member.name = decl->name;
        }
        break;
    case CGIR_EXPR_INDEX:
        leftPrefix = NULL;
        rightPrefix = NULL;
        left = HlslLowerIRExpr(context, source->u.index.object, &leftPrefix,
            valueMode == HLSL_VALUE_LVALUE ? HLSL_VALUE_LVALUE :
                                             HLSL_VALUE_RVALUE);
        right = HlslLowerIRExpr(context, source->u.index.index, &rightPrefix,
                                HLSL_VALUE_RVALUE);
        target = left != NULL && right != NULL ?
            HlslNewSourceExpr(context, HLSL_EXPR_INDEX, type) : NULL;
        if (target != NULL) {
            HlslAppendStmt(prefix, leftPrefix);
            if (valueMode != HLSL_VALUE_LVALUE &&
                (source->u.index.object->sideEffects ||
                 rightPrefix != NULL ||
                 source->u.index.index->sideEffects))
            {
                left = HlslCaptureValue(context, prefix, left);
                if (left == NULL)
                    return NULL;
            }
            if (source->u.index.index->sideEffects) {
                right = HlslCaptureValue(context, &rightPrefix, right);
                if (right == NULL)
                    return NULL;
            }
            HlslAppendStmt(prefix, rightPrefix);
            target->u.index.object = left;
            target->u.index.index = right;
        }
        break;
    case CGIR_EXPR_LENGTH:
        left = HlslLowerIRExpr(context, source->u.length.object, prefix,
                               HLSL_VALUE_RVALUE);
        if (left == NULL || left->type.arraySize <= 0)
            return NULL;
        return HlslNewLiteral(context, HLSL_BASE_INT,
                              left->type.arraySize, 0.0f);
    case CGIR_EXPR_SWIZZLE:
        left = HlslLowerIRExpr(context, source->u.swizzle.object, prefix,
            valueMode == HLSL_VALUE_LVALUE ? HLSL_VALUE_LVALUE :
                                             HLSL_VALUE_RVALUE);
        if (left == NULL || source->u.swizzle.componentCount < 1 ||
            source->u.swizzle.componentCount > 4)
        {
            return NULL;
        }
        for (index = 0; index < source->u.swizzle.componentCount; index++)
            mask[index] = "xyzw"[(source->u.swizzle.mask >>
                                  (2 * index)) & 3];
        mask[source->u.swizzle.componentCount] = '\0';
        target = HlslNewSourceExpr(context, HLSL_EXPR_SWIZZLE, type);
        if (target != NULL) {
            target->u.swizzle.object = left;
            target->u.swizzle.mask = HlslCopyText(context, mask);
            if (target->u.swizzle.mask == NULL)
                return NULL;
        }
        break;
    case CGIR_EXPR_CONSTRUCT:
        target = HlslNewSourceExpr(context, HLSL_EXPR_CONSTRUCT, type);
        if (target != NULL)
            target->u.construct.arguments = HlslLowerIRExprList(
                context, source->u.construct.arguments, prefix,
                NULL, NULL);
        if (target == NULL || (source->u.construct.arguments != NULL &&
                               target->u.construct.arguments == NULL))
        {
            return NULL;
        }
        break;
    case CGIR_EXPR_CAST:
        left = HlslLowerIRExpr(context, source->u.cast.operand, prefix,
                               HLSL_VALUE_RVALUE);
        /* The entry's verified IR contains implicit Cg scalar casts that
         * the historical direct lowering elided.  Keep that stable output;
         * helper casts remain explicit because they are part of the helper
         * ABI conversion surface. */
        if (left != NULL && context->function != NULL &&
            context->function->isEntry &&
            HlslIRTypeIsIdentical(&left->type, &type))
            return left;
        target = left != NULL ?
            HlslNewSourceExpr(context, HLSL_EXPR_CAST, type) : NULL;
        if (target != NULL)
            target->u.cast.expression = left;
        break;
    case CGIR_EXPR_UNARY:
        op = HlslIROperator(source->u.unary.op);
        left = HlslLowerIRExpr(context, source->u.unary.operand, prefix,
            op == HLSL_OP_PRE_INCREMENT ||
            op == HLSL_OP_PRE_DECREMENT ||
            op == HLSL_OP_POST_INCREMENT ||
            op == HLSL_OP_POST_DECREMENT ? HLSL_VALUE_LVALUE :
                                           HLSL_VALUE_RVALUE);
        target = left != NULL && op != HLSL_OP_NONE ?
            HlslNewSourceExpr(context, HLSL_EXPR_UNARY, type) : NULL;
        if (target != NULL) {
            target->u.unary.op = op;
            target->u.unary.operand = left;
        }
        break;
    case CGIR_EXPR_BINARY:
        op = HlslIROperator(source->u.binary.op);
        leftPrefix = NULL;
        rightPrefix = NULL;
        left = HlslLowerIRExpr(context, source->u.binary.left, &leftPrefix,
                               HLSL_VALUE_RVALUE);
        right = HlslLowerIRExpr(context, source->u.binary.right, &rightPrefix,
                                HLSL_VALUE_RVALUE);
        target = left != NULL && right != NULL && op != HLSL_OP_NONE ?
            HlslNewSourceExpr(context, HLSL_EXPR_BINARY, type) : NULL;
        if (target != NULL) {
            HlslAppendStmt(prefix, leftPrefix);
            if ((op == HLSL_OP_LOGICAL_AND ||
                 op == HLSL_OP_LOGICAL_OR) && rightPrefix != NULL)
            {
                HlslStmt *guard;
                HlslExpr *condition;

                temporary = HlslNewTemporary(context, &type);
                target = HlslNewAssignment(context,
                    temporary != NULL ?
                    HlslNewSymbolExpr(context, temporary) : NULL, left);
                if (temporary == NULL || target == NULL ||
                    !HlslAppendExpression(context, prefix, target))
                {
                    return NULL;
                }
                guard = HlslNewStmt(context->module, HLSL_STMT_IF);
                condition = HlslNewSymbolExpr(context, temporary);
                if (guard == NULL || condition == NULL)
                    return NULL;
                if (op == HLSL_OP_LOGICAL_OR) {
                    target = HlslNewSourceExpr(context, HLSL_EXPR_UNARY,
                                               type);
                    if (target == NULL)
                        return NULL;
                    target->u.unary.op = HLSL_OP_LOGICAL_NOT;
                    target->u.unary.operand = condition;
                    condition = target;
                }
                guard->u.ifStmt.condition = condition;
                guard->u.ifStmt.trueBranch = rightPrefix;
                target = HlslNewAssignment(context,
                    HlslNewSymbolExpr(context, temporary), right);
                if (target == NULL || !HlslAppendExpression(context,
                    &guard->u.ifStmt.trueBranch, target))
                {
                    return NULL;
                }
                HlslSetLoc(&guard->loc, &source->loc);
                HlslAppendStmt(prefix, guard);
                target = HlslNewSymbolExpr(context, temporary);
                if (target == NULL)
                    return NULL;
                break;
            }
            if (source->u.binary.left->sideEffects ||
                rightPrefix != NULL || source->u.binary.right->sideEffects)
            {
                left = HlslCaptureValue(context, prefix, left);
                if (left == NULL)
                    return NULL;
            }
            HlslAppendStmt(prefix, rightPrefix);
            target->u.binary.op = op;
            target->u.binary.left = left;
            target->u.binary.right = right;
        }
        break;
    case CGIR_EXPR_ASSIGN:
        op = HlslIROperator(source->u.assign.op);
        leftPrefix = NULL;
        rightPrefix = NULL;
        left = HlslLowerIRExpr(context, source->u.assign.target, &leftPrefix,
                               HLSL_VALUE_LVALUE);
        right = HlslLowerIRExpr(context, source->u.assign.value, &rightPrefix,
                                HLSL_VALUE_RVALUE);
        if (left == NULL || right == NULL || op == HLSL_OP_NONE)
            return NULL;
        HlslAppendStmt(prefix, leftPrefix);
        if ((rightPrefix != NULL || source->u.assign.value->sideEffects) &&
            !HlslStabilizeLvalueAddress(context, prefix, left))
        {
            return NULL;
        }
        HlslAppendStmt(prefix, rightPrefix);
        if (op == HLSL_OP_ASSIGN && HlslTypeNeedsRecursiveCopy(&type)) {
            if (!HlslStabilizeLvalueAddress(context, prefix, left))
                return NULL;
            if (!HlslIsStableAggregateSource(right)) {
                right = HlslCaptureValue(context, prefix, right);
                if (right == NULL)
                    return NULL;
            }
            copies = NULL;
            if (valueMode == HLSL_VALUE_RVALUE) {
                temporary = HlslNewTemporary(context, &type);
                if (temporary == NULL ||
                    !HlslAppendRecursiveCopy(context, &type,
                        HlslNewSymbolExpr(context, temporary), right,
                        &copies) ||
                    !HlslAppendRecursiveCopy(context, &type, left,
                        HlslNewSymbolExpr(context, temporary), &copies))
                {
                    return NULL;
                }
                HlslAppendStmt(prefix, copies);
                return HlslNewSymbolExpr(context, temporary);
            }
            if (!HlslAppendRecursiveCopy(context, &type, left, right,
                                         &copies))
            {
                return NULL;
            }
            target = HlslDetachLastExpression(&copies);
            HlslAppendStmt(prefix, copies);
            return target;
        }
        if (op == HLSL_OP_ASSIGN && valueMode == HLSL_VALUE_RVALUE) {
            right = HlslCaptureValue(context, prefix, right);
            target = HlslNewAssignment(context, left, right);
            if (target == NULL ||
                !HlslAppendExpression(context, prefix, target))
            {
                return NULL;
            }
            return right != NULL && right->kind == HLSL_EXPR_SYMBOL ?
                   HlslNewSymbolExpr(context, right->u.symbol) : NULL;
        }
        target = HlslNewSourceExpr(context, HLSL_EXPR_BINARY, type);
        if (target == NULL)
            return NULL;
        target->u.binary.op = op;
        target->u.binary.left = left;
        target->u.binary.right = right;
        break;
    case CGIR_EXPR_CONDITIONAL:
        leftPrefix = NULL;
        truePrefix = NULL;
        falsePrefix = NULL;
        target = HlslNewSourceExpr(context, HLSL_EXPR_CONDITIONAL, type);
        if (target != NULL) {
            target->u.conditional.condition = HlslLowerIRExpr(context,
                source->u.conditional.condition, &leftPrefix,
                HLSL_VALUE_RVALUE);
            target->u.conditional.trueExpr = HlslLowerIRExpr(context,
                source->u.conditional.trueExpr, &truePrefix,
                HLSL_VALUE_RVALUE);
            target->u.conditional.falseExpr = HlslLowerIRExpr(context,
                source->u.conditional.falseExpr, &falsePrefix,
                HLSL_VALUE_RVALUE);
            if (target->u.conditional.condition == NULL ||
                target->u.conditional.trueExpr == NULL ||
                target->u.conditional.falseExpr == NULL)
            {
                return NULL;
            }
            HlslAppendStmt(prefix, leftPrefix);
            if (target->u.conditional.condition->type.len > 1) {
                if (target->u.conditional.condition->type.len != type.len ||
                    type.len < 2 || type.len > 4)
                {
                    return NULL;
                }
                target->u.conditional.condition = HlslCaptureValue(
                    context, prefix, target->u.conditional.condition);
                HlslAppendStmt(prefix, truePrefix);
                target->u.conditional.trueExpr = HlslCaptureValue(
                    context, prefix, target->u.conditional.trueExpr);
                HlslAppendStmt(prefix, falsePrefix);
                target->u.conditional.falseExpr = HlslCaptureValue(
                    context, prefix, target->u.conditional.falseExpr);
                if (target->u.conditional.condition == NULL ||
                    target->u.conditional.trueExpr == NULL ||
                    target->u.conditional.falseExpr == NULL)
                {
                    return NULL;
                }
                left = target->u.conditional.condition;
                right = target->u.conditional.trueExpr;
                component = target->u.conditional.falseExpr;
                target = HlslNewSourceExpr(context,
                    HLSL_EXPR_CONSTRUCT, type);
                if (target == NULL)
                    return NULL;
                componentType = HlslNumericType(type.base, 1);
                for (index = 0; index < type.len; index++) {
                    HlslExpr *selection;

                    selection = HlslNewSourceExpr(context,
                        HLSL_EXPR_CONDITIONAL, componentType);
                    if (selection == NULL)
                        return NULL;
                    selection->u.conditional.condition = HlslComponent(
                        context, left, index, HLSL_BASE_BOOL);
                    selection->u.conditional.trueExpr = HlslComponent(
                        context, right, index, type.base);
                    selection->u.conditional.falseExpr = HlslComponent(
                        context, component, index, type.base);
                    if (selection->u.conditional.condition == NULL ||
                        selection->u.conditional.trueExpr == NULL ||
                        selection->u.conditional.falseExpr == NULL)
                    {
                        return NULL;
                    }
                    HlslAppendExpr(&target->u.construct.arguments,
                                   selection);
                }
            } else if (truePrefix != NULL || falsePrefix != NULL) {
                HlslStmt *guard;
                HlslExpr *branchValue;

                temporary = HlslNewTemporary(context, &type);
                guard = HlslNewStmt(context->module, HLSL_STMT_IF);
                if (temporary == NULL || guard == NULL)
                    return NULL;
                guard->u.ifStmt.condition = target->u.conditional.condition;
                guard->u.ifStmt.trueBranch = truePrefix;
                branchValue = target->u.conditional.trueExpr;
                if (HlslTypeNeedsRecursiveCopy(&type)) {
                    branchValue = HlslCaptureValue(context,
                        &guard->u.ifStmt.trueBranch, branchValue);
                    if (branchValue == NULL ||
                        !HlslAppendRecursiveCopy(context, &type,
                            HlslNewSymbolExpr(context, temporary),
                            branchValue, &guard->u.ifStmt.trueBranch))
                    {
                        return NULL;
                    }
                } else {
                    branchValue = HlslNewAssignment(context,
                        HlslNewSymbolExpr(context, temporary), branchValue);
                    if (branchValue == NULL ||
                        !HlslAppendExpression(context,
                            &guard->u.ifStmt.trueBranch, branchValue))
                    {
                        return NULL;
                    }
                }
                guard->u.ifStmt.falseBranch = falsePrefix;
                branchValue = target->u.conditional.falseExpr;
                if (HlslTypeNeedsRecursiveCopy(&type)) {
                    branchValue = HlslCaptureValue(context,
                        &guard->u.ifStmt.falseBranch, branchValue);
                    if (branchValue == NULL ||
                        !HlslAppendRecursiveCopy(context, &type,
                            HlslNewSymbolExpr(context, temporary),
                            branchValue, &guard->u.ifStmt.falseBranch))
                    {
                        return NULL;
                    }
                } else {
                    branchValue = HlslNewAssignment(context,
                        HlslNewSymbolExpr(context, temporary), branchValue);
                    if (branchValue == NULL ||
                        !HlslAppendExpression(context,
                            &guard->u.ifStmt.falseBranch, branchValue))
                    {
                        return NULL;
                    }
                }
                HlslSetLoc(&guard->loc, &source->loc);
                HlslAppendStmt(prefix, guard);
                target = HlslNewSymbolExpr(context, temporary);
                if (target == NULL)
                    return NULL;
            }
        }
        break;
    case CGIR_EXPR_CALL:
        return HlslLowerIRCall(context, source, &type, prefix);
    case CGIR_EXPR_INTRINSIC:
        return HlslLowerIRIntrinsic(context, source, &type, prefix);
    case CGIR_EXPR_INTERFACE_CALL:
        HlslLowerFailure(context, HLSL_ERROR_UNSUPPORTED_OPERATION,
                         "geometry Cg IR expression", &source->loc);
        return NULL;
    default:
        return NULL;
    }
    if (target != NULL)
        target->hasSideEffects = source->sideEffects;
    return target;
} // HlslLowerIRExpr

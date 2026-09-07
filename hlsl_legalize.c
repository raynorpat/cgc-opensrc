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
// hlsl_legalize.c
//

#include <stdio.h>
#include <string.h>

#include "slglobals.h"
#include "hlsl_hal.h"

static int HlslLegalizeFailure(HlslModule *module, HlslErrorKind kind,
                               const HlslLoc *loc, const char *reason)
{
    return HlslFail(module, kind, loc, reason);
} // HlslLegalizeFailure

static int HlslTypesEqualInner(const HlslType *left,
                               const HlslType *right, int depth)
{
    if (left == right)
        return left != NULL;
    if (left == NULL || right == NULL || depth >= 32 ||
        left->base != right->base || left->len != right->len ||
        left->rows != right->rows || left->cols != right->cols ||
        left->arraySize != right->arraySize)
    {
        return 0;
    }
    if (left->arraySize > 0) {
        return HlslTypesEqualInner(left->elementType, right->elementType,
                                   depth + 1);
    }
    if (left->base == HLSL_BASE_STRUCT) {
        if (left->structName == NULL || right->structName == NULL ||
            strcmp(left->structName, right->structName))
        {
            return 0;
        }
        return left->members == right->members;
    }
    return left->structName == NULL && right->structName == NULL &&
           left->members == NULL && right->members == NULL &&
           left->elementType == NULL && right->elementType == NULL;
} // HlslTypesEqualInner

static int HlslTypesEqual(const HlslType *left, const HlslType *right)
{
    return HlslTypesEqualInner(left, right, 0);
} // HlslTypesEqual

static int HlslTypeIsSampler(const HlslType *type)
{
    if (type == NULL)
        return 0;
    if (type->arraySize > 0)
        return HlslTypeIsSampler(type->elementType);
    return type->base == HLSL_BASE_SAMPLER1D ||
           type->base == HLSL_BASE_SAMPLER2D ||
           type->base == HLSL_BASE_SAMPLER3D ||
           type->base == HLSL_BASE_SAMPLERCUBE;
} // HlslTypeIsSampler

static int HlslTypeContainsUint(const HlslType *type, int depth)
{
    const HlslDecl *member;

    if (type == NULL)
        return 0;
    if (depth >= 32)
        return 1;
    if (type->base == HLSL_BASE_UINT)
        return 1;
    if (type->arraySize > 0)
        return HlslTypeContainsUint(type->elementType, depth + 1);
    for (member = type->members; member != NULL; member = member->next) {
        if (HlslTypeContainsUint(&member->type, depth + 1))
            return 1;
    }
    return 0;
} // HlslTypeContainsUint

static int HlslIsScalar(const HlslType *type, HlslBase base)
{
    return type != NULL && type->arraySize == 0 && type->base == base &&
           type->len == 1 && type->rows == 0 && type->cols == 0;
} // HlslIsScalar

static int HlslIsNumericScalarOrVector(const HlslType *type)
{
    return type != NULL && type->arraySize == 0 &&
           (type->base == HLSL_BASE_FLOAT || type->base == HLSL_BASE_INT ||
            type->base == HLSL_BASE_UINT) &&
           type->len >= 1 && type->len <= 4 &&
           type->rows == 0 && type->cols == 0;
} // HlslIsNumericScalarOrVector

static int HlslIsBooleanScalarOrVector(const HlslType *type)
{
    return type != NULL && type->arraySize == 0 &&
           type->base == HLSL_BASE_BOOL &&
           type->len >= 1 && type->len <= 4 &&
           type->rows == 0 && type->cols == 0;
} // HlslIsBooleanScalarOrVector

static int HlslIsIntegerScalarOrVector(const HlslType *type)
{
    return type != NULL && type->arraySize == 0 &&
           (type->base == HLSL_BASE_INT || type->base == HLSL_BASE_UINT) &&
           type->len >= 1 && type->len <= 4 &&
           type->rows == 0 && type->cols == 0;
} // HlslIsIntegerScalarOrVector

static int HlslScalarVectorLength(const HlslType *left,
                                  const HlslType *right)
{
    if (left->len > 1 && right->len > 1 && left->len != right->len)
        return 0;
    return left->len > right->len ? left->len : right->len;
} // HlslScalarVectorLength

static int HlslMultiplyResult(const HlslType *left,
                              const HlslType *right, HlslType *result)
{
    int length;

    if (left != NULL && right != NULL &&
        left->base == HLSL_BASE_FLOAT &&
        right->base == HLSL_BASE_FLOAT)
    {
        if (left->rows > 0 && left->cols > 0 &&
            right->rows == 0 && right->cols == 0 &&
            right->len == left->cols)
        {
            *result = HlslNumericType(HLSL_BASE_FLOAT, left->rows);
            return 1;
        }
        if (left->rows == 0 && left->cols == 0 &&
            right->rows > 0 && right->cols > 0 &&
            left->len == right->rows)
        {
            *result = HlslNumericType(HLSL_BASE_FLOAT, right->cols);
            return 1;
        }
        if (left->rows > 0 && left->cols > 0 &&
            right->rows > 0 && right->cols > 0 &&
            left->cols == right->rows)
        {
            *result = HlslMatrixType(left->rows, right->cols);
            return 1;
        }
    }
    if (!HlslIsNumericScalarOrVector(left) ||
        !HlslIsNumericScalarOrVector(right) ||
        left->base != right->base ||
        (left->len > 1 && right->len > 1 && left->len != right->len))
    {
        return 0;
    }
    length = left->len > right->len ? left->len : right->len;
    *result = HlslNumericType(left->base, length);
    return 1;
} // HlslMultiplyResult

static int HlslIsFloatMatrix(const HlslType *type)
{
    return type != NULL && type->arraySize == 0 &&
           type->base == HLSL_BASE_FLOAT &&
           type->rows >= 1 && type->rows <= 4 &&
           type->cols >= 1 && type->cols <= 4;
} // HlslIsFloatMatrix

static int HlslArithmeticResult(const HlslType *left,
                                const HlslType *right, HlslType *result)
{
    int leftMatrix;
    int rightMatrix;
    int length;

    if (left == NULL || right == NULL || result == NULL)
        return 0;
    leftMatrix = HlslIsFloatMatrix(left);
    rightMatrix = HlslIsFloatMatrix(right);
    if (leftMatrix || rightMatrix) {
        if (leftMatrix && rightMatrix &&
            left->rows == right->rows && left->cols == right->cols)
        {
            *result = *left;
            return 1;
        }
        if (leftMatrix && HlslIsScalar(right, HLSL_BASE_FLOAT)) {
            *result = *left;
            return 1;
        }
        if (rightMatrix && HlslIsScalar(left, HLSL_BASE_FLOAT)) {
            *result = *right;
            return 1;
        }
        return 0;
    }
    if (!HlslIsNumericScalarOrVector(left) ||
        !HlslIsNumericScalarOrVector(right) ||
        left->base != right->base ||
        (left->len > 1 && right->len > 1 && left->len != right->len))
    {
        return 0;
    }
    length = left->len > right->len ? left->len : right->len;
    *result = HlslNumericType(left->base, length);
    return 1;
} // HlslArithmeticResult

static int HlslMemberInList(const HlslDecl *members,
                            const HlslDecl *target)
{
    const HlslDecl *slow;
    const HlslDecl *fast;

    slow = members;
    fast = members;
    while (members != NULL) {
        if (members == target)
            return 1;
        members = members->next;
        if (fast != NULL && fast->next != NULL) {
            slow = slow->next;
            fast = fast->next->next;
            if (slow == fast)
                return 0;
        } else {
            fast = NULL;
        }
    }
    return 0;
} // HlslMemberInList

static int HlslLegalizeExpr(HlslModule *module, HlslExpr *expression,
                            int allowUint)
{
    HlslExpr *argument;
    HlslDecl *parameter;
    HlslType resultType;
    HlslType builtinParams[HLSL_MAX_BUILTIN_ARGS];
    int builtinParamCount;
    int length;
    int maskLength;

    if (expression == NULL || HlslTypeName(&expression->type) == NULL ||
        (!allowUint && HlslTypeContainsUint(&expression->type, 0)))
        return HlslLegalizeFailure(module, HLSL_ERROR_INVALID_IR,
            expression != NULL ? &expression->loc : NULL,
            "HLSL expression type");
    switch (expression->kind) {
    case HLSL_EXPR_SYMBOL:
        if (expression->u.symbol == NULL ||
            !HlslTypesEqual(&expression->type,
                            &expression->u.symbol->type))
        {
            return HlslLegalizeFailure(module, HLSL_ERROR_INVALID_IR,
                                       &expression->loc,
                                       "HLSL symbol declaration");
        }
        return 1;
    case HLSL_EXPR_INT:
        if (HlslIsScalar(&expression->type, HLSL_BASE_INT))
            return 1;
        break;
    case HLSL_EXPR_FLOAT:
        if (HlslIsScalar(&expression->type, HLSL_BASE_FLOAT))
            return 1;
        break;
    case HLSL_EXPR_BOOL:
        if (HlslIsScalar(&expression->type, HLSL_BASE_BOOL))
            return 1;
        break;
    case HLSL_EXPR_UNARY:
        if (expression->u.unary.operand == NULL ||
            !HlslLegalizeExpr(module, expression->u.unary.operand,
                              allowUint) ||
            !HlslTypesEqual(&expression->type,
                            &expression->u.unary.operand->type))
        {
            return HlslLegalizeFailure(module, HLSL_ERROR_INVALID_IR,
                                       &expression->loc,
                                       "HLSL unary operand");
        }
        switch (expression->u.unary.op) {
        case HLSL_OP_NEGATE:
        case HLSL_OP_POSITIVE:
        case HLSL_OP_PRE_INCREMENT:
        case HLSL_OP_PRE_DECREMENT:
        case HLSL_OP_POST_INCREMENT:
        case HLSL_OP_POST_DECREMENT:
            if (HlslIsNumericScalarOrVector(&expression->type))
                return 1;
            break;
        case HLSL_OP_LOGICAL_NOT:
            if (HlslIsBooleanScalarOrVector(&expression->type))
                return 1;
            break;
        case HLSL_OP_BITWISE_NOT:
            if (HlslIsIntegerScalarOrVector(&expression->type))
                return 1;
            break;
        default:
            break;
        }
        return HlslLegalizeFailure(module,
                                   HLSL_ERROR_UNSUPPORTED_OPERATION,
                                   &expression->loc,
                                   "HLSL unary operation");
    case HLSL_EXPR_BINARY:
        if (!HlslLegalizeExpr(module, expression->u.binary.left,
                              allowUint) ||
            !HlslLegalizeExpr(module, expression->u.binary.right,
                              allowUint))
        {
            return 0;
        }
        if (expression->u.binary.op >= HLSL_OP_ASSIGN &&
            expression->u.binary.op <= HLSL_OP_SHIFT_RIGHT_ASSIGN)
        {
            if (HlslTypeIsSampler(&expression->type) ||
                HlslTypeIsSampler(&expression->u.binary.left->type) ||
                HlslTypeIsSampler(&expression->u.binary.right->type))
            {
                return HlslLegalizeFailure(module, HLSL_ERROR_SAMPLER,
                                           &expression->loc,
                                           "sampler assignment");
            }
            if (HlslTypesEqual(&expression->type,
                               &expression->u.binary.left->type) &&
                HlslTypesEqual(&expression->type,
                               &expression->u.binary.right->type))
            {
                return 1;
            }
        } else if (expression->u.binary.op == HLSL_OP_ADD ||
                   expression->u.binary.op == HLSL_OP_SUBTRACT ||
                   expression->u.binary.op == HLSL_OP_MULTIPLY ||
                   expression->u.binary.op == HLSL_OP_DIVIDE)
        {
            if (HlslArithmeticResult(&expression->u.binary.left->type,
                                     &expression->u.binary.right->type,
                                     &resultType) &&
                HlslTypesEqual(&expression->type, &resultType))
            {
                return 1;
            }
        } else if (expression->u.binary.op == HLSL_OP_REMAINDER) {
            if (HlslArithmeticResult(&expression->u.binary.left->type,
                                     &expression->u.binary.right->type,
                                     &resultType) &&
                resultType.base == HLSL_BASE_INT &&
                HlslTypesEqual(&expression->type, &resultType))
            {
                return 1;
            }
        } else if (expression->u.binary.op == HLSL_OP_EQUAL ||
                   expression->u.binary.op == HLSL_OP_NOT_EQUAL ||
                   expression->u.binary.op == HLSL_OP_LESS ||
                   expression->u.binary.op == HLSL_OP_GREATER ||
                   expression->u.binary.op == HLSL_OP_LESS_EQUAL ||
                   expression->u.binary.op == HLSL_OP_GREATER_EQUAL)
        {
            length = HlslScalarVectorLength(
                &expression->u.binary.left->type,
                &expression->u.binary.right->type);
            if (length > 0 &&
                expression->u.binary.left->type.base ==
                    expression->u.binary.right->type.base &&
                HlslIsBooleanScalarOrVector(&expression->type) &&
                expression->type.len == length)
            {
                return 1;
            }
        } else if (expression->u.binary.op == HLSL_OP_LOGICAL_OR ||
                   expression->u.binary.op == HLSL_OP_LOGICAL_AND)
        {
            length = HlslScalarVectorLength(
                &expression->u.binary.left->type,
                &expression->u.binary.right->type);
            if (length > 0 &&
                HlslIsBooleanScalarOrVector(
                    &expression->u.binary.left->type) &&
                HlslIsBooleanScalarOrVector(
                    &expression->u.binary.right->type) &&
                HlslIsBooleanScalarOrVector(&expression->type) &&
                expression->type.len == length)
            {
                return 1;
            }
        } else if (expression->u.binary.op == HLSL_OP_BITWISE_OR ||
                   expression->u.binary.op == HLSL_OP_BITWISE_XOR ||
                   expression->u.binary.op == HLSL_OP_BITWISE_AND ||
                   expression->u.binary.op == HLSL_OP_SHIFT_LEFT ||
                   expression->u.binary.op == HLSL_OP_SHIFT_RIGHT)
        {
            length = HlslScalarVectorLength(
                &expression->u.binary.left->type,
                &expression->u.binary.right->type);
            if (length > 0 &&
                HlslIsIntegerScalarOrVector(
                    &expression->u.binary.left->type) &&
                HlslIsIntegerScalarOrVector(
                    &expression->u.binary.right->type) &&
                HlslIsIntegerScalarOrVector(&expression->type) &&
                expression->type.len == length)
            {
                return 1;
            }
        }
        return HlslLegalizeFailure(module, HLSL_ERROR_INVALID_IR,
                                   &expression->loc,
                                   "HLSL binary types");
    case HLSL_EXPR_CONDITIONAL:
        if (expression->u.conditional.condition == NULL ||
            expression->u.conditional.trueExpr == NULL ||
            expression->u.conditional.falseExpr == NULL ||
            !HlslLegalizeExpr(module,
                expression->u.conditional.condition, allowUint) ||
            !HlslLegalizeExpr(module,
                expression->u.conditional.trueExpr, allowUint) ||
            !HlslLegalizeExpr(module,
                expression->u.conditional.falseExpr, allowUint))
        {
            return 0;
        }
        if (HlslIsScalar(&expression->u.conditional.condition->type,
                         HLSL_BASE_BOOL) &&
            HlslExprIsPure(expression->u.conditional.trueExpr) &&
            HlslExprIsPure(expression->u.conditional.falseExpr) &&
            HlslTypesEqual(&expression->type,
                &expression->u.conditional.trueExpr->type) &&
            HlslTypesEqual(&expression->type,
                &expression->u.conditional.falseExpr->type))
        {
            return 1;
        }
        return HlslLegalizeFailure(module, HLSL_ERROR_INVALID_IR,
                                   &expression->loc,
                                   "HLSL conditional types");
    case HLSL_EXPR_CALL:
        if (expression->u.call.function == NULL &&
            expression->u.call.name != NULL)
        {
            if (expression->u.call.builtin == HLSL_BUILTIN_NONE ||
                HlslBuiltinSpelling(expression->u.call.builtin) == NULL ||
                strcmp(expression->u.call.name,
                       HlslBuiltinSpelling(expression->u.call.builtin)))
            {
                return HlslLegalizeFailure(module, HLSL_ERROR_INVALID_IR,
                    &expression->loc, "HLSL intrinsic identity");
            }
            builtinParamCount = 0;
            for (argument = expression->u.call.arguments;
                 argument != NULL; argument = argument->next)
            {
                if (builtinParamCount >= HLSL_MAX_BUILTIN_ARGS ||
                    !HlslLegalizeExpr(module, argument, allowUint))
                {
                    return HlslLegalizeFailure(module,
                        HLSL_ERROR_INVALID_IR, &expression->loc,
                        "HLSL intrinsic arguments");
                }
                builtinParams[builtinParamCount++] = argument->type;
            }
            if (HlslBuiltinAccepts(module->stage,
                    expression->u.call.builtin, &expression->type,
                    builtinParams, builtinParamCount))
            {
                return 1;
            }
            return HlslLegalizeFailure(module,
                HlslBuiltinIsTexture(expression->u.call.builtin) ?
                    HLSL_ERROR_SAMPLER : HLSL_ERROR_INVALID_IR,
                &expression->loc,
                HlslBuiltinIsTexture(expression->u.call.builtin) ?
                    expression->u.call.name : "HLSL intrinsic overload");
        }
        if (expression->u.call.function == NULL ||
            expression->u.call.builtin != HLSL_BUILTIN_NONE ||
            expression->u.call.name == NULL ||
            expression->u.call.function->name == NULL ||
            strcmp(expression->u.call.name,
                   expression->u.call.function->name) ||
            !HlslTypesEqual(&expression->type,
                            &expression->u.call.function->result))
        {
            return HlslLegalizeFailure(module, HLSL_ERROR_INVALID_IR,
                                       &expression->loc,
                                       "HLSL call");
        }
        argument = expression->u.call.arguments;
        parameter = expression->u.call.function->parameters;
        while (argument != NULL && parameter != NULL)
        {
            if (!HlslLegalizeExpr(module, argument, allowUint))
                return 0;
            if (!HlslTypesEqual(&argument->type, &parameter->type))
                return HlslLegalizeFailure(module, HLSL_ERROR_INVALID_IR,
                                           &argument->loc,
                                           "HLSL call argument type");
            argument = argument->next;
            parameter = parameter->next;
        }
        if (argument != NULL || parameter != NULL)
            return HlslLegalizeFailure(module, HLSL_ERROR_INVALID_IR,
                                       &expression->loc,
                                       "HLSL call arguments");
        return 1;
    case HLSL_EXPR_CONSTRUCT:
        for (argument = expression->u.construct.arguments;
             argument != NULL; argument = argument->next)
        {
            if (!HlslLegalizeExpr(module, argument, allowUint))
                return 0;
        }
        return 1;
    case HLSL_EXPR_CAST:
        if (expression->u.cast.expression == NULL ||
            !HlslLegalizeExpr(module, expression->u.cast.expression,
                              allowUint))
        {
            return 0;
        }
        if ((expression->type.base == HLSL_BASE_STRUCT &&
             expression->u.cast.expression->kind == HLSL_EXPR_INT &&
             expression->u.cast.expression->u.literalInt == 0) ||
            ((HlslIsNumericScalarOrVector(&expression->type) ||
              HlslIsBooleanScalarOrVector(&expression->type)) &&
             (HlslIsNumericScalarOrVector(
                  &expression->u.cast.expression->type) ||
              HlslIsBooleanScalarOrVector(
                  &expression->u.cast.expression->type)) &&
             expression->type.len ==
                 expression->u.cast.expression->type.len))
        {
            return 1;
        }
        return HlslLegalizeFailure(module, HLSL_ERROR_INVALID_IR,
                                   &expression->loc,
                                   "HLSL cast types");
    case HLSL_EXPR_MEMBER:
        if (expression->u.member.decl == NULL ||
            expression->u.member.name == NULL ||
            expression->u.member.object == NULL)
        {
            return HlslLegalizeFailure(module, HLSL_ERROR_INVALID_IR,
                                       &expression->loc,
                                       "HLSL member declaration");
        }
        if (!HlslLegalizeExpr(module, expression->u.member.object,
                              allowUint))
            return 0;
        if (expression->u.member.object->type.base != HLSL_BASE_STRUCT ||
            expression->u.member.decl->name == NULL ||
            strcmp(expression->u.member.name,
                   expression->u.member.decl->name) ||
            !HlslMemberInList(expression->u.member.object->type.members,
                              expression->u.member.decl) ||
            !HlslTypesEqual(&expression->type,
                            &expression->u.member.decl->type))
        {
            return HlslLegalizeFailure(module, HLSL_ERROR_INVALID_IR,
                                       &expression->loc,
                                       "HLSL member type");
        }
        return 1;
    case HLSL_EXPR_INDEX:
        if (!HlslLegalizeExpr(module, expression->u.index.object,
                              allowUint) ||
            !HlslLegalizeExpr(module, expression->u.index.index,
                              allowUint))
        {
            return 0;
        }
        if (!HlslIsScalar(&expression->u.index.index->type,
                          HLSL_BASE_INT))
        {
            return HlslLegalizeFailure(module, HLSL_ERROR_INVALID_IR,
                                       &expression->loc,
                                       "HLSL index type");
        }
        if (expression->u.index.object->type.arraySize > 0) {
            if (HlslTypesEqual(&expression->type,
                expression->u.index.object->type.elementType))
            {
                return 1;
            }
        } else if (expression->u.index.object->type.rows > 0 &&
                   expression->u.index.object->type.cols > 0 &&
                   expression->type.base == HLSL_BASE_FLOAT &&
                   expression->type.rows == 0 &&
                   expression->type.cols == 0 &&
                   expression->type.len ==
                       expression->u.index.object->type.cols)
        {
            return 1;
        } else if (HlslIsNumericScalarOrVector(
                       &expression->u.index.object->type) &&
                   expression->u.index.object->type.len > 1 &&
                   HlslIsScalar(&expression->type,
                        expression->u.index.object->type.base))
        {
            return 1;
        }
        return HlslLegalizeFailure(module, HLSL_ERROR_INVALID_IR,
                                   &expression->loc,
                                   "HLSL indexed object");
    case HLSL_EXPR_SWIZZLE:
        if (expression->u.swizzle.object == NULL ||
            expression->u.swizzle.mask == NULL ||
            !HlslLegalizeExpr(module, expression->u.swizzle.object,
                              allowUint))
        {
            return 0;
        }
        maskLength = (int) strlen(expression->u.swizzle.mask);
        if (maskLength == expression->type.len &&
            maskLength >= 1 && maskLength <= 4 &&
            expression->u.swizzle.object->type.len >= 1 &&
            expression->u.swizzle.object->type.len <= 4 &&
            expression->type.base ==
                expression->u.swizzle.object->type.base)
        {
            return 1;
        }
        return HlslLegalizeFailure(module, HLSL_ERROR_INVALID_IR,
                                   &expression->loc,
                                   "HLSL swizzle type");
    default:
        return HlslLegalizeFailure(module,
                                   HLSL_ERROR_UNSUPPORTED_OPERATION,
                                   &expression->loc,
                                   "HLSL expression");
    }
    return HlslLegalizeFailure(module, HLSL_ERROR_INVALID_IR,
                               &expression->loc,
                               "HLSL literal type");
} // HlslLegalizeExpr

static int HlslLegalizeDeclarations(HlslModule *module, HlslDecl *decl,
                                    int allowSampler, int allowUint)
{
    for (; decl != NULL; decl = decl->next) {
        if (decl->name == NULL || HlslTypeName(&decl->type) == NULL ||
            (!allowUint && HlslTypeContainsUint(&decl->type, 0)))
            return HlslLegalizeFailure(module, HLSL_ERROR_INVALID_IR,
                                       &decl->loc,
                                       "HLSL declaration");
        if (HlslTypeIsSampler(&decl->type) &&
            (!allowSampler || decl->type.arraySize != 0 ||
             decl->storage != HLSL_STORAGE_SAMPLER))
        {
            return HlslLegalizeFailure(module, HLSL_ERROR_SAMPLER,
                &decl->loc, decl->type.arraySize != 0 ?
                "sampler array" : "local sampler");
        }
        if (decl->initializer != NULL &&
            !HlslLegalizeExpr(module, decl->initializer, allowUint))
        {
            return 0;
        }
        if (decl->initializer != NULL &&
            !HlslTypesEqual(&decl->type, &decl->initializer->type))
        {
            return HlslLegalizeFailure(module, HLSL_ERROR_INVALID_IR,
                                       &decl->initializer->loc,
                                       "HLSL initializer type");
        }
        if (decl->members != NULL &&
            !HlslLegalizeDeclarations(module, decl->members, 0,
                                      allowUint))
        {
            return 0;
        }
    }
    return 1;
} // HlslLegalizeDeclarations

static int HlslLegalizeStatements(HlslModule *module, HlslStmt *statement,
                                  const HlslType *result, int loopDepth,
                                  int allowUint)
{
    for (; statement != NULL; statement = statement->next) {
        switch (statement->kind) {
        case HLSL_STMT_DECLARATION:
            if (statement->u.declaration == NULL ||
                !HlslLegalizeDeclarations(module,
                    statement->u.declaration, 0, allowUint))
            {
                return 0;
            }
            break;
        case HLSL_STMT_EXPRESSION:
            if (!HlslLegalizeExpr(module, statement->u.expression,
                                  allowUint))
                return 0;
            break;
        case HLSL_STMT_IF:
            if (!HlslLegalizeExpr(module,
                    statement->u.ifStmt.condition, allowUint) ||
                !HlslIsScalar(&statement->u.ifStmt.condition->type,
                              HLSL_BASE_BOOL) ||
                !HlslLegalizeStatements(module,
                    statement->u.ifStmt.trueBranch, result, loopDepth,
                    allowUint) ||
                !HlslLegalizeStatements(module,
                    statement->u.ifStmt.falseBranch, result, loopDepth,
                    allowUint))
            {
                return HlslLegalizeFailure(module, HLSL_ERROR_INVALID_IR,
                                           &statement->loc,
                                           "HLSL if statement");
            }
            break;
        case HLSL_STMT_WHILE:
        case HLSL_STMT_DO:
            if (!HlslLegalizeExpr(module, statement->u.loop.condition,
                                  allowUint) ||
                !HlslIsScalar(&statement->u.loop.condition->type,
                              HLSL_BASE_BOOL) ||
                !HlslLegalizeStatements(module, statement->u.loop.body,
                                         result, loopDepth + 1,
                                         allowUint))
            {
                return HlslLegalizeFailure(module, HLSL_ERROR_INVALID_IR,
                                           &statement->loc,
                                           "HLSL loop statement");
            }
            break;
        case HLSL_STMT_FOR:
            if ((statement->u.forStmt.init != NULL &&
                 !HlslLegalizeStatements(module,
                    statement->u.forStmt.init, result, loopDepth,
                    allowUint)) ||
                (statement->u.forStmt.condition != NULL &&
                 (!HlslLegalizeExpr(module,
                    statement->u.forStmt.condition, allowUint) ||
                  !HlslIsScalar(
                    &statement->u.forStmt.condition->type,
                    HLSL_BASE_BOOL))) ||
                (statement->u.forStmt.step != NULL &&
                 !HlslLegalizeStatements(module,
                    statement->u.forStmt.step, result, loopDepth,
                    allowUint)) ||
                !HlslLegalizeStatements(module,
                    statement->u.forStmt.body, result, loopDepth + 1,
                    allowUint))
            {
                return HlslLegalizeFailure(module, HLSL_ERROR_INVALID_IR,
                                           &statement->loc,
                                           "HLSL for statement");
            }
            break;
        case HLSL_STMT_BLOCK:
            if (!HlslLegalizeStatements(module, statement->u.block,
                                        result, loopDepth, allowUint))
                return 0;
            break;
        case HLSL_STMT_RETURN:
            if (statement->u.returnExpr != NULL) {
                if (!HlslLegalizeExpr(module, statement->u.returnExpr,
                                      allowUint))
                    return 0;
                if (result->base == HLSL_BASE_VOID ||
                    !HlslTypesEqual(result,
                                    &statement->u.returnExpr->type))
                {
                    return HlslLegalizeFailure(module,
                        HLSL_ERROR_INVALID_IR, &statement->loc,
                        "HLSL return type");
                }
            } else if (result->base != HLSL_BASE_VOID) {
                return HlslLegalizeFailure(module, HLSL_ERROR_INVALID_IR,
                                           &statement->loc,
                                           "HLSL return value");
            }
            if (result->base == HLSL_BASE_VOID &&
                (result->len != 0 || result->arraySize != 0 ||
                 result->rows != 0 || result->cols != 0))
            {
                return HlslLegalizeFailure(module, HLSL_ERROR_INVALID_IR,
                                           &statement->loc,
                                           "HLSL void return type");
            }
            break;
        case HLSL_STMT_DISCARD:
            if (module->stage != HLSL_STAGE_PIXEL)
                return HlslLegalizeFailure(module,
                    HLSL_ERROR_STAGE_OPERATION, &statement->loc,
                    "discard");
            break;
        case HLSL_STMT_BREAK:
        case HLSL_STMT_CONTINUE:
            if (loopDepth <= 0)
                return HlslLegalizeFailure(module,
                    HLSL_ERROR_INVALID_IR, &statement->loc,
                    "HLSL loop jump");
            break;
        case HLSL_STMT_APPEND:
            if (module->stage != HLSL_STAGE_GEOMETRY ||
                statement->u.append.record == NULL ||
                !HlslLegalizeExpr(module, statement->u.append.record,
                                  allowUint))
            {
                return HlslLegalizeFailure(module,
                    HLSL_ERROR_GEOMETRY_LAYOUT, &statement->loc,
                    "geometry append");
            }
            break;
        case HLSL_STMT_RESTART_STRIP:
            if (module->stage != HLSL_STAGE_GEOMETRY)
                return HlslLegalizeFailure(module,
                    HLSL_ERROR_GEOMETRY_LAYOUT, &statement->loc,
                    "geometry restart");
            break;
        default:
            return HlslLegalizeFailure(module,
                                       HLSL_ERROR_UNSUPPORTED_OPERATION,
                                       &statement->loc,
                                       "HLSL statement");
        }
    }
    return 1;
} // HlslLegalizeStatements

static HlslExpr *HlslGeometrySymbol(HlslModule *module, HlslDecl *decl)
{
    HlslExpr *expression;

    if (decl == NULL)
        return NULL;
    expression = HlslNewExpr(module, HLSL_EXPR_SYMBOL, decl->type);
    if (expression != NULL)
        expression->u.symbol = decl;
    return expression;
} // HlslGeometrySymbol

static HlslFlatReplay *HlslGeometryStateForTarget(HlslFunction *function,
                                                  HlslDecl *target)
{
    HlslFlatReplay *state;

    for (state = function != NULL ? function->geometryFlatState : NULL;
         state != NULL; state = state->next)
    {
        if (state->target == target)
            return state;
    }
    return NULL;
} // HlslGeometryStateForTarget

static int HlslAppendGeometryCallState(HlslModule *module,
                                       HlslFunction *caller,
                                       HlslExpr *call)
{
    HlslFunction *callee;
    HlslFlatReplay *calleeState;
    HlslFlatReplay *callerState;
    HlslExpr *argument;

    callee = call->u.call.function;
    if (callee == NULL || !callee->geometryEffect)
        return 1;
    if (caller == NULL || !caller->geometryEffect ||
        caller->geometryStream == NULL ||
        caller->geometryOutputRecord == NULL)
    {
        return 0;
    }
    argument = HlslGeometrySymbol(module, caller->geometryStream);
    if (argument == NULL)
        return 0;
    HlslAppendExpr(&call->u.call.arguments, argument);
    argument = HlslGeometrySymbol(module, caller->geometryOutputRecord);
    if (argument == NULL)
        return 0;
    HlslAppendExpr(&call->u.call.arguments, argument);
    for (calleeState = callee->geometryFlatState; calleeState != NULL;
         calleeState = calleeState->next)
    {
        callerState = HlslGeometryStateForTarget(caller,
                                                 calleeState->target);
        if (callerState == NULL)
            return 0;
        argument = HlslGeometrySymbol(module, callerState->shadow);
        if (argument == NULL)
            return 0;
        HlslAppendExpr(&call->u.call.arguments, argument);
        argument = HlslGeometrySymbol(module, callerState->defined);
        if (argument == NULL)
            return 0;
        HlslAppendExpr(&call->u.call.arguments, argument);
    }
    return 1;
} // HlslAppendGeometryCallState

static int HlslRewriteGeometryCallsExpr(HlslModule *module,
    HlslFunction *caller, HlslExpr *expression)
{
    HlslExpr *argument;

    if (expression == NULL)
        return 1;
    switch (expression->kind) {
    case HLSL_EXPR_UNARY:
        return HlslRewriteGeometryCallsExpr(module, caller,
                                            expression->u.unary.operand);
    case HLSL_EXPR_BINARY:
        return HlslRewriteGeometryCallsExpr(module, caller,
                                            expression->u.binary.left) &&
               HlslRewriteGeometryCallsExpr(module, caller,
                                            expression->u.binary.right);
    case HLSL_EXPR_CONDITIONAL:
        return HlslRewriteGeometryCallsExpr(module, caller,
                expression->u.conditional.condition) &&
               HlslRewriteGeometryCallsExpr(module, caller,
                expression->u.conditional.trueExpr) &&
               HlslRewriteGeometryCallsExpr(module, caller,
                expression->u.conditional.falseExpr);
    case HLSL_EXPR_CALL:
        for (argument = expression->u.call.arguments; argument != NULL;
             argument = argument->next)
        {
            if (!HlslRewriteGeometryCallsExpr(module, caller, argument))
                return 0;
        }
        return HlslAppendGeometryCallState(module, caller, expression);
    case HLSL_EXPR_CONSTRUCT:
        for (argument = expression->u.construct.arguments;
             argument != NULL; argument = argument->next)
        {
            if (!HlslRewriteGeometryCallsExpr(module, caller, argument))
                return 0;
        }
        return 1;
    case HLSL_EXPR_CAST:
        return HlslRewriteGeometryCallsExpr(module, caller,
                                            expression->u.cast.expression);
    case HLSL_EXPR_MEMBER:
        return HlslRewriteGeometryCallsExpr(module, caller,
                                            expression->u.member.object);
    case HLSL_EXPR_INDEX:
        return HlslRewriteGeometryCallsExpr(module, caller,
                    expression->u.index.object) &&
               HlslRewriteGeometryCallsExpr(module, caller,
                    expression->u.index.index);
    case HLSL_EXPR_SWIZZLE:
        return HlslRewriteGeometryCallsExpr(module, caller,
                                            expression->u.swizzle.object);
    case HLSL_EXPR_TEXTURE_METHOD:
        return HlslRewriteGeometryCallsExpr(module, caller,
                    expression->u.textureMethod.texture) &&
               HlslRewriteGeometryCallsExpr(module, caller,
                    expression->u.textureMethod.sampler) &&
               HlslRewriteGeometryCallsExpr(module, caller,
                    expression->u.textureMethod.coordinates) &&
               HlslRewriteGeometryCallsExpr(module, caller,
                    expression->u.textureMethod.argument1) &&
               HlslRewriteGeometryCallsExpr(module, caller,
                    expression->u.textureMethod.argument2);
    default:
        return 1;
    }
} // HlslRewriteGeometryCallsExpr

static int HlslRewriteGeometryCalls(HlslModule *module,
    HlslFunction *caller, HlslStmt *statement)
{
    for (; statement != NULL; statement = statement->next) {
        switch (statement->kind) {
        case HLSL_STMT_EXPRESSION:
            if (!HlslRewriteGeometryCallsExpr(module, caller,
                                              statement->u.expression))
                return 0;
            break;
        case HLSL_STMT_IF:
            if (!HlslRewriteGeometryCallsExpr(module, caller,
                    statement->u.ifStmt.condition) ||
                !HlslRewriteGeometryCalls(module, caller,
                    statement->u.ifStmt.trueBranch) ||
                !HlslRewriteGeometryCalls(module, caller,
                    statement->u.ifStmt.falseBranch)) return 0;
            break;
        case HLSL_STMT_WHILE:
        case HLSL_STMT_DO:
            if (!HlslRewriteGeometryCallsExpr(module, caller,
                    statement->u.loop.condition) ||
                !HlslRewriteGeometryCalls(module, caller,
                    statement->u.loop.body)) return 0;
            break;
        case HLSL_STMT_FOR:
            if (!HlslRewriteGeometryCalls(module, caller,
                    statement->u.forStmt.init) ||
                !HlslRewriteGeometryCallsExpr(module, caller,
                    statement->u.forStmt.condition) ||
                !HlslRewriteGeometryCalls(module, caller,
                    statement->u.forStmt.step) ||
                !HlslRewriteGeometryCalls(module, caller,
                    statement->u.forStmt.body)) return 0;
            break;
        case HLSL_STMT_BLOCK:
            if (!HlslRewriteGeometryCalls(module, caller,
                                          statement->u.block)) return 0;
            break;
        case HLSL_STMT_RETURN:
            if (!HlslRewriteGeometryCallsExpr(module, caller,
                                              statement->u.returnExpr))
                return 0;
            break;
        case HLSL_STMT_APPEND:
            if (!HlslRewriteGeometryCallsExpr(module, caller,
                                              statement->u.append.record))
                return 0;
            break;
        default:
            break;
        }
    }
    return 1;
} // HlslRewriteGeometryCalls

static HlslExpr *HlslGeometryInitializer(HlslModule *module,
                                         HlslType type, int boolean)
{
    HlslExpr *value;
    HlslExpr *cast;

    value = HlslNewExpr(module,
        boolean ? HLSL_EXPR_BOOL : HLSL_EXPR_INT,
        HlslNumericType(boolean ? HLSL_BASE_BOOL : HLSL_BASE_INT, 1));
    if (value == NULL)
        return NULL;
    if (boolean) {
        value->u.literalBool = 0;
        return value;
    }
    value->u.literalInt = 0;
    cast = HlslNewExpr(module, HLSL_EXPR_CAST, type);
    if (cast != NULL)
        cast->u.cast.expression = value;
    return cast;
} // HlslGeometryInitializer

static int HlslPrepareGeometryWrapper(HlslModule *module)
{
    HlslFunction *wrapper;
    HlslFunction *entry;
    HlslFlatReplay *entryState;
    HlslFlatReplay *wrapperState;
    HlslFlatReplay **tail;
    HlslDecl *shadow;
    HlslDecl *defined;
    HlslType streamType;
    HlslType boolType;
    const char *name;
    char generated[192];

    entry = module->entry;
    wrapper = module->wrapper;
    if (entry == NULL || wrapper == NULL)
        return 1;
    if (module->geometryOutputStruct == NULL)
        return 0;
    wrapper->geometryEffect = 1;
    streamType = HlslNumericType(HLSL_BASE_GEOMETRY_STREAM,
                                 (int) module->geometryStream + 1);
    name = HlslAllocateScopedSymbolName(module, wrapper, wrapper,
                                        "cgc_stream");
    wrapper->geometryStream = name != NULL ? HlslNewDecl(module,
        HLSL_STORAGE_NONE, streamType, name) : NULL;
    name = HlslAllocateScopedSymbolName(module, wrapper,
        module->geometryOutputStruct, "cgc_output");
    wrapper->geometryOutputRecord = name != NULL ? HlslNewDecl(module,
        HLSL_STORAGE_NONE, module->geometryOutputStruct->type, name) : NULL;
    if (wrapper->geometryStream == NULL ||
        wrapper->geometryOutputRecord == NULL)
        return 0;
    wrapper->geometryStream->parameterQualifier = HLSL_PARAMETER_INOUT;
    wrapper->geometryStream->geometryRole = HLSL_GEOMETRY_DECL_STREAM;
    wrapper->geometryStream->publicName = module->geometryOutputStruct->name;
    wrapper->geometryOutputRecord->geometryRole =
        HLSL_GEOMETRY_DECL_OUTPUT_RECORD;
    wrapper->geometryOutputRecord->initializer =
        HlslGeometryInitializer(module,
            wrapper->geometryOutputRecord->type, 0);
    if (wrapper->geometryOutputRecord->initializer == NULL)
        return 0;
    HlslAppendDecl(&wrapper->parameters, wrapper->geometryStream);
    HlslAppendDecl(&wrapper->locals, wrapper->geometryOutputRecord);

    boolType = HlslNumericType(HLSL_BASE_BOOL, 1);
    tail = &wrapper->geometryFlatState;
    for (entryState = entry->geometryFlatState; entryState != NULL;
         entryState = entryState->next)
    {
        if (strlen(entryState->target->name) + 18 > sizeof(generated))
            return 0;
        sprintf(generated, "cgc_flat_%s", entryState->target->name);
        name = HlslAllocateScopedSymbolName(module, wrapper,
                                            entryState->target, generated);
        shadow = name != NULL ? HlslNewDecl(module, HLSL_STORAGE_NONE,
            entryState->shadow->type, name) : NULL;
        sprintf(generated, "cgc_flat_%s_defined",
                entryState->target->name);
        name = HlslAllocateScopedSymbolName(module, wrapper, shadow,
                                            generated);
        defined = name != NULL ? HlslNewDecl(module, HLSL_STORAGE_NONE,
            boolType, name) : NULL;
        wrapperState = shadow != NULL && defined != NULL ?
            HlslNewFlatReplay(module, entryState->target,
                              shadow, defined) : NULL;
        if (wrapperState == NULL)
            return 0;
        shadow->geometryRole = HLSL_GEOMETRY_DECL_FLAT_SHADOW;
        defined->geometryRole = HLSL_GEOMETRY_DECL_FLAT_DEFINED;
        defined->initializer = HlslGeometryInitializer(module,
                                                       boolType, 1);
        if (defined->initializer == NULL)
            return 0;
        HlslAppendDecl(&wrapper->locals, shadow);
        HlslAppendDecl(&wrapper->locals, defined);
        *tail = wrapperState;
        tail = &wrapperState->next;
    }
    return 1;
} // HlslPrepareGeometryWrapper

static int HlslLegalizeGeometryState(HlslModule *module)
{
    HlslFunction *function;
    HlslFlatReplay *state;

    if (module->stage != HLSL_STAGE_GEOMETRY)
        return 1;
    if (!HlslPrepareGeometryWrapper(module))
        return 0;
    for (function = module->functions; function != NULL;
         function = function->next)
    {
        if (!function->geometryEffect || function == module->wrapper)
            continue;
        HlslAppendDecl(&function->parameters, function->geometryStream);
        HlslAppendDecl(&function->parameters,
                       function->geometryOutputRecord);
        for (state = function->geometryFlatState; state != NULL;
             state = state->next)
        {
            HlslAppendDecl(&function->parameters, state->shadow);
            HlslAppendDecl(&function->parameters, state->defined);
        }
    }
    for (function = module->functions; function != NULL;
         function = function->next)
    {
        if (!HlslRewriteGeometryCalls(module, function, function->body))
            return 0;
    }
    return 1;
} // HlslLegalizeGeometryState

int HlslLegalizeModule(HlslModule *module,
                       const HlslProfileDesc *profile)
{
    HlslFunction *function;
    int allowUint;

    if (module == NULL || profile == NULL ||
        module->stage != profile->stage || module->entry == NULL)
    {
        return HlslLegalizeFailure(module, HLSL_ERROR_INVALID_IR, NULL,
                                   "invalid HLSL legalization module");
    }
    allowUint = profile->semanticPolicy == HLSL_SEMANTIC_POLICY_MODERN;
    if (!HlslLegalizeGeometryState(module) ||
        !HlslLegalizeDeclarations(module, module->globals, 1,
                                  allowUint) ||
        !HlslLegalizeDeclarations(module, module->structs, 0,
                                  allowUint))
    {
        return 0;
    }
    for (function = module->functions; function != NULL;
         function = function->next)
    {
        if (function->name == NULL ||
            HlslTypeName(&function->result) == NULL ||
            (!allowUint &&
             HlslTypeContainsUint(&function->result, 0)) ||
            HlslTypeIsSampler(&function->result) ||
            !HlslLegalizeDeclarations(module, function->parameters, 1,
                                      allowUint) ||
            !HlslLegalizeDeclarations(module, function->locals, 0,
                                      allowUint))
        {
            return HlslLegalizeFailure(module,
                HlslTypeIsSampler(&function->result) ? HLSL_ERROR_SAMPLER :
                                                       HLSL_ERROR_INVALID_IR,
                &function->loc,
                HlslTypeIsSampler(&function->result) ? "sampler return" :
                                                       "HLSL function");
        }
        if (!HlslLegalizeStatements(module, function->body,
                                    &function->result, 0, allowUint))
        {
            return 0;
        }
    }
    return module->errors == 0;
} // HlslLegalizeModule

static HlslBase HlslModernTextureObjectBase(HlslBase samplerBase)
{
    switch (samplerBase) {
    case HLSL_BASE_SAMPLER1D: return HLSL_BASE_TEXTURE1D;
    case HLSL_BASE_SAMPLER2D: return HLSL_BASE_TEXTURE2D;
    case HLSL_BASE_SAMPLER3D: return HLSL_BASE_TEXTURE3D;
    case HLSL_BASE_SAMPLERCUBE: return HLSL_BASE_TEXTURECUBE;
    default: return HLSL_BASE_VOID;
    }
} // HlslModernTextureObjectBase

static HlslTextureDimension HlslModernTextureDimension(HlslBase base)
{
    switch (base) {
    case HLSL_BASE_TEXTURE1D: return HLSL_TEXTURE_1D;
    case HLSL_BASE_TEXTURE2D: return HLSL_TEXTURE_2D;
    case HLSL_BASE_TEXTURE3D: return HLSL_TEXTURE_3D;
    default: return HLSL_TEXTURE_CUBE;
    }
} // HlslModernTextureDimension

static const char *HlslModernAbiName(HlslModule *module,
    const HlslFunction *function, const HlslDecl *decl,
    const char *prefix)
{
    const char *publicName;
    char *source;
    size_t prefixLength;
    size_t nameLength;

    publicName = decl->publicName != NULL ? decl->publicName : decl->name;
    if (publicName == NULL)
        return NULL;
    prefixLength = strlen(prefix);
    nameLength = strlen(publicName);
    source = (char *) module->alloc(module->allocArg,
        prefixLength + nameLength + 1);
    if (source == NULL)
        return NULL;
    memcpy(source, prefix, prefixLength);
    memcpy(source + prefixLength, publicName, nameLength + 1);
    return HlslAllocateScopedSymbolName(module,
        function->identity != NULL ? function->identity : function,
        decl, source);
} // HlslModernAbiName

static int HlslModernSplitParameters(HlslModule *module,
                                     HlslFunction *function)
{
    HlslDecl *decl;
    HlslDecl *pair;
    HlslDecl *next;
    HlslBase textureBase;
    const char *textureName;
    const char *samplerName;

    for (decl = function->parameters; decl != NULL; decl = next) {
        next = decl->next;
        textureBase = HlslModernTextureObjectBase(decl->type.base);
        if (textureBase == HLSL_BASE_VOID)
            continue;
        textureName = HlslModernAbiName(module, function, decl,
                                        "cgc_texture_");
        pair = HlslNewDecl(module, HLSL_STORAGE_SAMPLER,
            HlslNumericType(HLSL_BASE_SAMPLER_STATE, 1), "pending");
        if (pair != NULL)
            pair->publicName = decl->publicName != NULL ?
                               decl->publicName : decl->name;
        samplerName = pair != NULL ? HlslModernAbiName(module, function,
            pair, "cgc_sampler_") : NULL;
        if (textureName == NULL || pair == NULL || samplerName == NULL)
            return HlslLegalizeFailure(module, HLSL_ERROR_INVALID_IR,
                &decl->loc, "modern HLSL sampler parameter");
        decl->type = HlslNumericType(textureBase, 1);
        decl->name = textureName;
        decl->resourcePair = pair;
        decl->resourcePairId = -1;
        pair->name = samplerName;
        pair->publicName = decl->publicName;
        pair->loc = decl->loc;
        pair->sourceOrdinal = decl->sourceOrdinal;
        pair->identity = pair;
        pair->parameterQualifier = decl->parameterQualifier;
        pair->resourcePair = decl;
        pair->resourcePairId = -1;
        pair->next = next;
        decl->next = pair;
    }
    return 1;
} // HlslModernSplitParameters

static HlslExpr *HlslModernSymbol(HlslModule *module, HlslDecl *decl,
                                  const HlslLoc *loc)
{
    HlslExpr *expression;

    expression = HlslNewLocatedExpr(module, HLSL_EXPR_SYMBOL,
                                    decl->type, loc);
    if (expression != NULL)
        expression->u.symbol = decl;
    return expression;
} // HlslModernSymbol

static HlslExpr *HlslModernSwizzle(HlslModule *module, HlslExpr *object,
                                   const char *mask, int width,
                                   const HlslLoc *loc)
{
    HlslExpr *expression;

    expression = HlslNewLocatedExpr(module, HLSL_EXPR_SWIZZLE,
        HlslNumericType(HLSL_BASE_FLOAT, width), loc);
    if (expression != NULL) {
        expression->u.swizzle.object = object;
        expression->u.swizzle.mask = mask;
    }
    return expression;
} // HlslModernSwizzle

static HlslDecl *HlslModernExpressionDecl(HlslExpr *expression)
{
    return expression != NULL && expression->kind == HLSL_EXPR_SYMBOL ?
           expression->u.symbol : NULL;
} // HlslModernExpressionDecl

static int HlslModernRewriteExpression(HlslModule *module,
    const HlslProfileDesc *profile, HlslExpr *expression);

static int HlslModernRewriteExpressionList(HlslModule *module,
    const HlslProfileDesc *profile, HlslExpr *expression)
{
    for (; expression != NULL; expression = expression->next) {
        if (!HlslModernRewriteExpression(module, profile, expression))
            return 0;
    }
    return 1;
} // HlslModernRewriteExpressionList

static int HlslModernRewriteTextureCall(HlslModule *module,
    const HlslProfileDesc *profile, HlslExpr *expression)
{
    HlslExpr *texture;
    HlslExpr *sampler;
    HlslExpr *coordinates;
    HlslExpr *argument1;
    HlslExpr *argument2;
    HlslExpr *spatial;
    HlslExpr *projection;
    HlslExpr *division;
    HlslExpr *built;
    HlslExpr *savedNext;
    HlslDecl *textureDecl;
    HlslTextureDimension dimension;
    HlslTextureMethod method;
    HlslTextureSelectReason reason;
    HlslTextureForm form;
    HlslType coordinateType;
    int spatialWidth;
    int coordinateWidth;
    const char *spatialMask;

    texture = expression->u.call.arguments;
    if (!HlslModernRewriteExpressionList(module, profile, texture))
        return 0;
    coordinates = texture != NULL ? texture->next : NULL;
    textureDecl = HlslModernExpressionDecl(texture);
    if (textureDecl == NULL || textureDecl->resourcePair == NULL ||
        coordinates == NULL)
    {
        return HlslLegalizeFailure(module, HLSL_ERROR_RESOURCE_PAIR,
                                   &expression->loc,
                                   expression->u.call.name);
    }
    texture->type = textureDecl->type;
    dimension = HlslModernTextureDimension(textureDecl->type.base);
    coordinateWidth = coordinates->type.len;
    if (!HlslModernSelectTextureMethod(profile->stage,
            expression->u.call.builtin, dimension, coordinateWidth,
            &expression->type, &method, &reason))
    {
        return HlslLegalizeFailure(module,
            reason == HLSL_TEXTURE_SELECT_STAGE ?
                HLSL_ERROR_STAGE_OPERATION : HLSL_ERROR_SAMPLER,
            &expression->loc, expression->u.call.name);
    }
    sampler = HlslModernSymbol(module, textureDecl->resourcePair,
                               &expression->loc);
    if (sampler == NULL)
        return 0;
    texture->next = NULL;
    argument1 = NULL;
    argument2 = NULL;
    form = HlslBuiltinTextureForm(expression->u.call.builtin);
    if (dimension == HLSL_TEXTURE_1D) {
        spatialWidth = 1;
        spatialMask = "x";
    } else if (dimension == HLSL_TEXTURE_2D) {
        spatialWidth = 2;
        spatialMask = "xy";
    } else {
        spatialWidth = 3;
        spatialMask = "xyz";
    }
    if (form == HLSL_TEXTURE_LOD || form == HLSL_TEXTURE_BIAS ||
        form == HLSL_TEXTURE_PROJECTED)
    {
        coordinates->next = NULL;
        spatial = HlslModernSwizzle(module, coordinates, spatialMask,
                                    spatialWidth, &expression->loc);
        projection = HlslModernSwizzle(module, coordinates, "w", 1,
                                       &expression->loc);
        if (spatial == NULL || projection == NULL)
            return 0;
        if (form == HLSL_TEXTURE_PROJECTED) {
            coordinateType = HlslNumericType(HLSL_BASE_FLOAT,
                                             spatialWidth);
            division = HlslNewLocatedExpr(module, HLSL_EXPR_BINARY,
                                          coordinateType,
                                          &expression->loc);
            if (division == NULL)
                return 0;
            division->u.binary.op = HLSL_OP_DIVIDE;
            division->u.binary.left = spatial;
            division->u.binary.right = projection;
            coordinates = division;
        } else {
            coordinates = spatial;
            argument1 = projection;
        }
    } else if (form == HLSL_TEXTURE_GRADIENT) {
        argument1 = coordinates->next;
        argument2 = argument1 != NULL ? argument1->next : NULL;
        coordinates->next = NULL;
        if (argument1 != NULL)
            argument1->next = NULL;
        if (argument2 != NULL)
            argument2->next = NULL;
    } else {
        coordinates->next = NULL;
    }
    built = HlslNewTextureMethod(module, method, texture, sampler,
        coordinates, argument1, argument2, expression->type,
        expression->loc);
    if (built == NULL)
        return 0;
    savedNext = expression->next;
    *expression = *built;
    expression->next = savedNext;
    return 1;
} // HlslModernRewriteTextureCall

static int HlslModernRewriteUserCall(HlslModule *module,
    const HlslProfileDesc *profile, HlslExpr *expression)
{
    HlslExpr **argumentPlace;
    HlslExpr *argument;
    HlslExpr *pairArgument;
    HlslDecl *argumentDecl;
    HlslDecl *parameter;

    argumentPlace = &expression->u.call.arguments;
    parameter = expression->u.call.function->parameters;
    while (*argumentPlace != NULL && parameter != NULL) {
        argument = *argumentPlace;
        if (!HlslModernRewriteExpression(module, profile, argument))
            return 0;
        if (parameter->resourcePair != NULL) {
            argumentDecl = HlslModernExpressionDecl(argument);
            if (argumentDecl == NULL || argumentDecl->resourcePair == NULL ||
                parameter->next != parameter->resourcePair)
            {
                return HlslLegalizeFailure(module, HLSL_ERROR_RESOURCE_PAIR,
                    &argument->loc, expression->u.call.name);
            }
            argument->type = argumentDecl->type;
            pairArgument = HlslModernSymbol(module,
                argumentDecl->resourcePair, &argument->loc);
            if (pairArgument == NULL)
                return 0;
            pairArgument->next = argument->next;
            argument->next = pairArgument;
            argumentPlace = &pairArgument->next;
            parameter = parameter->resourcePair->next;
        } else {
            argumentPlace = &argument->next;
            parameter = parameter->next;
        }
    }
    if (*argumentPlace != NULL || parameter != NULL)
        return HlslLegalizeFailure(module, HLSL_ERROR_INVALID_IR,
                                   &expression->loc,
                                   "modern HLSL call arguments");
    return 1;
} // HlslModernRewriteUserCall

static int HlslModernRewriteExpression(HlslModule *module,
    const HlslProfileDesc *profile, HlslExpr *expression)
{
    if (expression == NULL)
        return 1;
    switch (expression->kind) {
    case HLSL_EXPR_SYMBOL:
        if (expression->u.symbol != NULL &&
            expression->u.symbol->resourcePair != NULL)
        {
            expression->type = expression->u.symbol->type;
        }
        return 1;
    case HLSL_EXPR_UNARY:
        return HlslModernRewriteExpression(module, profile,
                                            expression->u.unary.operand);
    case HLSL_EXPR_BINARY:
        return HlslModernRewriteExpression(module, profile,
                    expression->u.binary.left) &&
               HlslModernRewriteExpression(module, profile,
                    expression->u.binary.right);
    case HLSL_EXPR_CONDITIONAL:
        return HlslModernRewriteExpression(module, profile,
                    expression->u.conditional.condition) &&
               HlslModernRewriteExpression(module, profile,
                    expression->u.conditional.trueExpr) &&
               HlslModernRewriteExpression(module, profile,
                    expression->u.conditional.falseExpr);
    case HLSL_EXPR_CALL:
        if (HlslBuiltinIsTexture(expression->u.call.builtin))
            return HlslModernRewriteTextureCall(module, profile, expression);
        if (expression->u.call.function != NULL)
            return HlslModernRewriteUserCall(module, profile, expression);
        return HlslModernRewriteExpressionList(module, profile,
                                                expression->u.call.arguments);
    case HLSL_EXPR_CONSTRUCT:
        return HlslModernRewriteExpressionList(module, profile,
            expression->u.construct.arguments);
    case HLSL_EXPR_CAST:
        return HlslModernRewriteExpression(module, profile,
                                            expression->u.cast.expression);
    case HLSL_EXPR_MEMBER:
        return HlslModernRewriteExpression(module, profile,
                                            expression->u.member.object);
    case HLSL_EXPR_INDEX:
        return HlslModernRewriteExpression(module, profile,
                    expression->u.index.object) &&
               HlslModernRewriteExpression(module, profile,
                    expression->u.index.index);
    case HLSL_EXPR_SWIZZLE:
        return HlslModernRewriteExpression(module, profile,
                                            expression->u.swizzle.object);
    case HLSL_EXPR_TEXTURE_METHOD:
    case HLSL_EXPR_INT:
    case HLSL_EXPR_FLOAT:
    case HLSL_EXPR_BOOL:
        return 1;
    }
    return 0;
} // HlslModernRewriteExpression

static int HlslModernRewriteStatements(HlslModule *module,
    const HlslProfileDesc *profile, HlslStmt *statement)
{
    for (; statement != NULL; statement = statement->next) {
        switch (statement->kind) {
        case HLSL_STMT_DECLARATION:
            if (!HlslModernRewriteExpression(module, profile,
                    statement->u.declaration != NULL ?
                    statement->u.declaration->initializer : NULL))
                return 0;
            break;
        case HLSL_STMT_EXPRESSION:
            if (!HlslModernRewriteExpression(module, profile,
                                              statement->u.expression))
                return 0;
            break;
        case HLSL_STMT_IF:
            if (!HlslModernRewriteExpression(module, profile,
                    statement->u.ifStmt.condition) ||
                !HlslModernRewriteStatements(module, profile,
                    statement->u.ifStmt.trueBranch) ||
                !HlslModernRewriteStatements(module, profile,
                    statement->u.ifStmt.falseBranch))
                return 0;
            break;
        case HLSL_STMT_WHILE:
        case HLSL_STMT_DO:
            if (!HlslModernRewriteExpression(module, profile,
                    statement->u.loop.condition) ||
                !HlslModernRewriteStatements(module, profile,
                    statement->u.loop.body))
                return 0;
            break;
        case HLSL_STMT_FOR:
            if (!HlslModernRewriteStatements(module, profile,
                    statement->u.forStmt.init) ||
                !HlslModernRewriteExpression(module, profile,
                    statement->u.forStmt.condition) ||
                !HlslModernRewriteStatements(module, profile,
                    statement->u.forStmt.step) ||
                !HlslModernRewriteStatements(module, profile,
                    statement->u.forStmt.body))
                return 0;
            break;
        case HLSL_STMT_BLOCK:
            if (!HlslModernRewriteStatements(module, profile,
                                              statement->u.block))
                return 0;
            break;
        case HLSL_STMT_RETURN:
            if (!HlslModernRewriteExpression(module, profile,
                                              statement->u.returnExpr))
                return 0;
            break;
        case HLSL_STMT_APPEND:
            if (!HlslModernRewriteExpression(module, profile,
                                              statement->u.append.record))
                return 0;
            break;
        default:
            break;
        }
    }
    return 1;
} // HlslModernRewriteStatements

int HlslLegalizeModernTextureAbi(HlslModule *module,
                                 const HlslProfileDesc *profile)
{
    HlslFunction *function;

    if (module == NULL || profile == NULL ||
        profile->resourcePolicy != HLSL_RESOURCE_POLICY_MODERN)
    {
        return 1;
    }
    for (function = module->functions; function != NULL;
         function = function->next)
    {
        if (!HlslModernSplitParameters(module, function))
            return 0;
    }
    for (function = module->functions; function != NULL;
         function = function->next)
    {
        if (!HlslModernRewriteStatements(module, profile, function->body))
            return 0;
    }
    return 1;
} // HlslLegalizeModernTextureAbi

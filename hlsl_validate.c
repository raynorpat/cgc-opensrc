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
OF THE NVIDIA SOFTWARE, HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
\****************************************************************************/
// hlsl_validate.c
//

#include <limits.h>
#include <stdio.h>
#include <string.h>

#include "slglobals.h"
#include "hlsl_hal.h"

typedef struct HlslPointerFrame_Rec {
    const struct HlslPointerFrame_Rec *parent;
    const void *pointer;
} HlslPointerFrame;

static int HlslFrameContains(const HlslPointerFrame *frame,
                             const void *pointer)
{
    for (; frame != NULL; frame = frame->parent) {
        if (frame->pointer == pointer)
            return 1;
    }
    return 0;
} // HlslFrameContains

static int HlslDeclListHasCycle(const HlslDecl *list)
{
    const HlslDecl *slow;
    const HlslDecl *fast;

    slow = list;
    fast = list;
    while (fast != NULL && fast->next != NULL) {
        slow = slow->next;
        fast = fast->next->next;
        if (slow == fast)
            return 1;
    }
    return 0;
} // HlslDeclListHasCycle

static int HlslNameListHasCycle(const HlslName *list)
{
    const HlslName *slow;
    const HlslName *fast;

    slow = list;
    fast = list;
    while (fast != NULL && fast->next != NULL) {
        slow = slow->next;
        fast = fast->next->next;
        if (slow == fast)
            return 1;
    }
    return 0;
} // HlslNameListHasCycle

static int HlslExprListHasCycle(const HlslExpr *list)
{
    const HlslExpr *slow;
    const HlslExpr *fast;

    slow = list;
    fast = list;
    while (fast != NULL && fast->next != NULL) {
        slow = slow->next;
        fast = fast->next->next;
        if (slow == fast)
            return 1;
    }
    return 0;
} // HlslExprListHasCycle

static int HlslStmtListHasCycle(const HlslStmt *list)
{
    const HlslStmt *slow;
    const HlslStmt *fast;

    slow = list;
    fast = list;
    while (fast != NULL && fast->next != NULL) {
        slow = slow->next;
        fast = fast->next->next;
        if (slow == fast)
            return 1;
    }
    return 0;
} // HlslStmtListHasCycle

static int HlslFunctionListHasCycle(const HlslFunction *list)
{
    const HlslFunction *slow;
    const HlslFunction *fast;

    slow = list;
    fast = list;
    while (fast != NULL && fast->next != NULL) {
        slow = slow->next;
        fast = fast->next->next;
        if (slow == fast)
            return 1;
    }
    return 0;
} // HlslFunctionListHasCycle

static int HlslBindingListHasCycle(const HlslBinding *list)
{
    const HlslBinding *slow;
    const HlslBinding *fast;

    slow = list;
    fast = list;
    while (fast != NULL && fast->next != NULL) {
        slow = slow->next;
        fast = fast->next->next;
        if (slow == fast)
            return 1;
    }
    return 0;
} // HlslBindingListHasCycle

static int HlslAllocationListHasCycle(const HlslBinding *list)
{
    const HlslBinding *slow;
    const HlslBinding *fast;

    slow = list;
    fast = list;
    while (fast != NULL && fast->allocationNext != NULL) {
        slow = slow->allocationNext;
        fast = fast->allocationNext->allocationNext;
        if (slow == fast)
            return 1;
    }
    return 0;
} // HlslAllocationListHasCycle

static int HlslTypeIsValidInner(const HlslType *type, int allowVoid,
                                const HlslPointerFrame *parent, int depth)
{
    HlslPointerFrame frame;
    const HlslDecl *member;

    if (type == NULL || depth > 128 || type->arraySize < 0 ||
        HlslFrameContains(parent, type))
    {
        return 0;
    }
    frame.parent = parent;
    frame.pointer = type;
    if (type->arraySize > 0) {
        return type->base == HLSL_BASE_VOID && type->len == 0 &&
               type->rows == 0 && type->cols == 0 &&
               type->structName == NULL && type->members == NULL &&
               type->elementType != NULL &&
               HlslTypeIsValidInner(type->elementType, 0, &frame,
                                    depth + 1);
    }
    if (type->elementType != NULL)
        return 0;
    if (type->base == HLSL_BASE_STRUCT) {
        if (type->len != 0 || type->rows != 0 || type->cols != 0 ||
            type->structName == NULL || type->structName[0] == '\0' ||
            HlslDeclListHasCycle(type->members))
        {
            return 0;
        }
        for (member = type->members; member != NULL; member = member->next) {
            if (member->name == NULL || member->name[0] == '\0' ||
                !HlslTypeIsValidInner(&member->type, 0, &frame, depth + 1))
            {
                return 0;
            }
        }
        return 1;
    }
    if (type->structName != NULL || type->members != NULL)
        return 0;
    if (type->rows != 0 || type->cols != 0) {
        return type->base == HLSL_BASE_FLOAT && type->len == 0 &&
               type->rows >= 1 && type->rows <= 4 &&
               type->cols >= 1 && type->cols <= 4;
    }
    switch (type->base) {
    case HLSL_BASE_VOID:
        return allowVoid && type->len == 0;
    case HLSL_BASE_FLOAT:
    case HLSL_BASE_INT:
    case HLSL_BASE_BOOL:
        return type->len >= 1 && type->len <= 4;
    case HLSL_BASE_SAMPLER1D:
    case HLSL_BASE_SAMPLER2D:
    case HLSL_BASE_SAMPLER3D:
    case HLSL_BASE_SAMPLERCUBE:
        return type->len == 1;
    case HLSL_BASE_STRUCT:
        break;
    }
    return 0;
} // HlslTypeIsValidInner

static int HlslTypeIsValid(const HlslType *type, int allowVoid)
{
    return HlslTypeIsValidInner(type, allowVoid, NULL, 0);
} // HlslTypeIsValid

static int HlslTypesEqualInner(const HlslType *left, const HlslType *right,
                               const HlslPointerFrame *leftParent,
                               const HlslPointerFrame *rightParent,
                               int depth)
{
    HlslPointerFrame leftFrame;
    HlslPointerFrame rightFrame;
    const HlslDecl *leftMember;
    const HlslDecl *rightMember;

    if (left == NULL || right == NULL || depth > 128 ||
        HlslFrameContains(leftParent, left) ||
        HlslFrameContains(rightParent, right) ||
        left->base != right->base || left->len != right->len ||
        left->rows != right->rows || left->cols != right->cols ||
        left->arraySize != right->arraySize)
    {
        return 0;
    }
    if ((left->structName == NULL) != (right->structName == NULL) ||
        (left->structName != NULL &&
         strcmp(left->structName, right->structName)))
    {
        return 0;
    }
    leftFrame.parent = leftParent;
    leftFrame.pointer = left;
    rightFrame.parent = rightParent;
    rightFrame.pointer = right;
    if (left->arraySize > 0) {
        return HlslTypesEqualInner(left->elementType, right->elementType,
                                   &leftFrame, &rightFrame, depth + 1);
    }
    leftMember = left->members;
    rightMember = right->members;
    while (leftMember != NULL && rightMember != NULL) {
        if (!HlslTypesEqualInner(&leftMember->type, &rightMember->type,
                                 &leftFrame, &rightFrame, depth + 1))
        {
            return 0;
        }
        leftMember = leftMember->next;
        rightMember = rightMember->next;
    }
    return leftMember == NULL && rightMember == NULL;
} // HlslTypesEqualInner

static int HlslTypesEqual(const HlslType *left, const HlslType *right)
{
    if (left == right)
        return left != NULL && HlslTypeIsValid(left, 1);
    return HlslTypesEqualInner(left, right, NULL, NULL, 0);
} // HlslTypesEqual

static int HlslIsScalar(const HlslType *type, HlslBase base)
{
    return type != NULL && type->arraySize == 0 && type->base == base &&
           type->len == 1 && type->rows == 0 && type->cols == 0;
} // HlslIsScalar

static int HlslIsNumericScalarOrVector(const HlslType *type)
{
    return type != NULL && type->arraySize == 0 &&
           (type->base == HLSL_BASE_FLOAT || type->base == HLSL_BASE_INT) &&
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
           type->base == HLSL_BASE_INT &&
           type->len >= 1 && type->len <= 4 &&
           type->rows == 0 && type->cols == 0;
} // HlslIsIntegerScalarOrVector

static int HlslIsFloatMatrix(const HlslType *type)
{
    return type != NULL && type->arraySize == 0 &&
           type->base == HLSL_BASE_FLOAT &&
           type->rows >= 1 && type->rows <= 4 &&
           type->cols >= 1 && type->cols <= 4;
} // HlslIsFloatMatrix

static int HlslScalarVectorLength(const HlslType *left,
                                  const HlslType *right)
{
    if (left->len > 1 && right->len > 1 && left->len != right->len)
        return 0;
    return left->len > right->len ? left->len : right->len;
} // HlslScalarVectorLength

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

static int HlslUnaryTypesAgree(const HlslExpr *expression)
{
    const HlslType *type;

    type = &expression->type;
    if (!HlslTypesEqual(type, &expression->u.unary.operand->type))
        return 0;
    switch (expression->u.unary.op) {
    case HLSL_OP_NEGATE:
    case HLSL_OP_POSITIVE:
    case HLSL_OP_PRE_INCREMENT:
    case HLSL_OP_PRE_DECREMENT:
    case HLSL_OP_POST_INCREMENT:
    case HLSL_OP_POST_DECREMENT:
        return HlslIsNumericScalarOrVector(type);
    case HLSL_OP_LOGICAL_NOT:
        return HlslIsBooleanScalarOrVector(type);
    case HLSL_OP_BITWISE_NOT:
        return HlslIsIntegerScalarOrVector(type);
    default:
        return 0;
    }
} // HlslUnaryTypesAgree

static int HlslBinaryTypesAgree(const HlslExpr *expression)
{
    const HlslType *left;
    const HlslType *right;
    HlslType result;
    int length;

    left = &expression->u.binary.left->type;
    right = &expression->u.binary.right->type;
    if (expression->u.binary.op >= HLSL_OP_ASSIGN &&
        expression->u.binary.op <= HLSL_OP_SHIFT_RIGHT_ASSIGN)
    {
        return HlslTypesEqual(&expression->type, left) &&
               HlslTypesEqual(&expression->type, right);
    }
    if (expression->u.binary.op == HLSL_OP_ADD ||
        expression->u.binary.op == HLSL_OP_SUBTRACT ||
        expression->u.binary.op == HLSL_OP_MULTIPLY ||
        expression->u.binary.op == HLSL_OP_DIVIDE)
    {
        return HlslArithmeticResult(left, right, &result) &&
               HlslTypesEqual(&expression->type, &result);
    }
    if (expression->u.binary.op == HLSL_OP_REMAINDER) {
        return HlslArithmeticResult(left, right, &result) &&
               result.base == HLSL_BASE_INT &&
               HlslTypesEqual(&expression->type, &result);
    }
    length = HlslScalarVectorLength(left, right);
    if (expression->u.binary.op >= HLSL_OP_EQUAL &&
        expression->u.binary.op <= HLSL_OP_GREATER_EQUAL)
    {
        return length > 0 && left->base == right->base &&
               HlslIsBooleanScalarOrVector(&expression->type) &&
               expression->type.len == length;
    }
    if (expression->u.binary.op == HLSL_OP_LOGICAL_OR ||
        expression->u.binary.op == HLSL_OP_LOGICAL_AND)
    {
        return length > 0 && HlslIsBooleanScalarOrVector(left) &&
               HlslIsBooleanScalarOrVector(right) &&
               HlslIsBooleanScalarOrVector(&expression->type) &&
               expression->type.len == length;
    }
    if (expression->u.binary.op == HLSL_OP_BITWISE_OR ||
        expression->u.binary.op == HLSL_OP_BITWISE_XOR ||
        expression->u.binary.op == HLSL_OP_BITWISE_AND ||
        expression->u.binary.op == HLSL_OP_SHIFT_LEFT ||
        expression->u.binary.op == HLSL_OP_SHIFT_RIGHT)
    {
        return length > 0 && HlslIsIntegerScalarOrVector(left) &&
               HlslIsIntegerScalarOrVector(right) &&
               HlslIsIntegerScalarOrVector(&expression->type) &&
               expression->type.len == length;
    }
    return 0;
} // HlslBinaryTypesAgree

static int HlslTypeComponentCount(const HlslType *type)
{
    if (type->rows > 0 && type->cols > 0)
        return type->rows * type->cols;
    if (type->arraySize == 0 &&
        (HlslIsNumericScalarOrVector(type) ||
         HlslIsBooleanScalarOrVector(type)))
    {
        return type->len;
    }
    return 0;
} // HlslTypeComponentCount

static int HlslConstructTypesAgree(const HlslExpr *expression)
{
    const HlslExpr *argument;
    const HlslDecl *member;
    int components;
    int count;

    argument = expression->u.construct.arguments;
    if (argument == NULL)
        return 0;
    if (expression->type.arraySize > 0) {
        count = 0;
        for (; argument != NULL; argument = argument->next) {
            if (!HlslTypesEqual(&argument->type,
                                expression->type.elementType))
            {
                return 0;
            }
            count++;
        }
        return count == expression->type.arraySize;
    }
    if (expression->type.base == HLSL_BASE_STRUCT) {
        member = expression->type.members;
        for (; argument != NULL && member != NULL;
             argument = argument->next, member = member->next)
        {
            if (!HlslTypesEqual(&argument->type, &member->type))
                return 0;
        }
        return argument == NULL && member == NULL;
    }
    count = HlslTypeComponentCount(&expression->type);
    if (count == 0)
        return 0;
    components = 0;
    for (; argument != NULL; argument = argument->next) {
        count = HlslTypeComponentCount(&argument->type);
        if (count == 0 || components > INT_MAX - count)
            return 0;
        components += count;
    }
    return components == HlslTypeComponentCount(&expression->type);
} // HlslConstructTypesAgree

static int HlslDeclTreeContains(const HlslDecl *list,
                                const HlslDecl *target, int depth)
{
    const HlslDecl *decl;

    if (target == NULL || depth > 128 || HlslDeclListHasCycle(list))
        return 0;
    for (decl = list; decl != NULL; decl = decl->next) {
        if (decl == target ||
            HlslDeclTreeContains(decl->members, target, depth + 1))
        {
            return 1;
        }
    }
    return 0;
} // HlslDeclTreeContains

static int HlslOwnsDecl(const HlslModule *module, const HlslDecl *target)
{
    const HlslFunction *function;

    if (module == NULL || target == NULL)
        return 0;
    if (HlslDeclTreeContains(module->globals, target, 0) ||
        HlslDeclTreeContains(module->structs, target, 0))
    {
        return 1;
    }
    if (HlslFunctionListHasCycle(module->functions))
        return 0;
    for (function = module->functions; function != NULL;
         function = function->next)
    {
        if (HlslDeclTreeContains(function->parameters, target, 0) ||
            HlslDeclTreeContains(function->locals, target, 0))
        {
            return 1;
        }
    }
    return 0;
} // HlslOwnsDecl

static int HlslTypeDeclsAreOwnedInner(const HlslModule *module,
    const HlslType *type, const HlslPointerFrame *parent, int depth)
{
    HlslPointerFrame frame;
    const HlslDecl *member;

    if (type == NULL || depth > 128 || HlslFrameContains(parent, type))
        return 0;
    frame.parent = parent;
    frame.pointer = type;
    if (type->arraySize > 0) {
        return HlslTypeDeclsAreOwnedInner(module, type->elementType,
                                          &frame, depth + 1);
    }
    if (type->base != HLSL_BASE_STRUCT)
        return 1;
    for (member = type->members; member != NULL; member = member->next) {
        if (!HlslOwnsDecl(module, member) ||
            !HlslTypeDeclsAreOwnedInner(module, &member->type,
                                        &frame, depth + 1))
        {
            return 0;
        }
    }
    return 1;
} // HlslTypeDeclsAreOwnedInner

static int HlslTypeDeclsAreOwned(const HlslModule *module,
                                 const HlslType *type)
{
    return HlslTypeDeclsAreOwnedInner(module, type, NULL, 0);
} // HlslTypeDeclsAreOwned

static int HlslDeclTypesAreOwned(const HlslModule *module,
                                 const HlslDecl *decl, int depth)
{
    if (depth > 128 || HlslDeclListHasCycle(decl))
        return 0;
    for (; decl != NULL; decl = decl->next) {
        if (!HlslTypeDeclsAreOwned(module, &decl->type) ||
            !HlslDeclTypesAreOwned(module, decl->members, depth + 1))
        {
            return 0;
        }
    }
    return 1;
} // HlslDeclTypesAreOwned

static int HlslOwnsFunction(const HlslModule *module,
                            const HlslFunction *target)
{
    const HlslFunction *function;

    if (module == NULL || target == NULL ||
        HlslFunctionListHasCycle(module->functions))
    {
        return 0;
    }
    for (function = module->functions; function != NULL;
         function = function->next)
    {
        if (function == target)
            return 1;
    }
    return 0;
} // HlslOwnsFunction

static int HlslMemberBelongsToType(const HlslType *type,
                                   const HlslDecl *target)
{
    const HlslDecl *member;

    if (type == NULL || type->base != HLSL_BASE_STRUCT ||
        HlslDeclListHasCycle(type->members))
    {
        return 0;
    }
    for (member = type->members; member != NULL; member = member->next) {
        if (member == target)
            return 1;
    }
    return 0;
} // HlslMemberBelongsToType

static int HlslDeclShapeIsValid(const HlslDecl *decl)
{
    if (decl == NULL || decl->name == NULL || decl->name[0] == '\0' ||
        decl->storage < HLSL_STORAGE_NONE ||
        decl->storage > HLSL_STORAGE_BUILTIN ||
        decl->storageClass < HLSL_STORAGE_CLASS_AUTO ||
        decl->storageClass > HLSL_STORAGE_CLASS_EXTERN ||
        decl->typeQualifier < HLSL_TYPE_QUALIFIER_NONE ||
        decl->typeQualifier > HLSL_TYPE_QUALIFIER_CONST ||
        decl->parameterQualifier < HLSL_PARAMETER_IN ||
        decl->parameterQualifier > HLSL_PARAMETER_INOUT ||
        !HlslTypeIsValid(&decl->type, 0))
    {
        return 0;
    }
    if (decl->members != NULL &&
        (decl->type.base != HLSL_BASE_STRUCT ||
         decl->type.members != decl->members))
    {
        return 0;
    }
    return 1;
} // HlslDeclShapeIsValid

static int HlslDeclListShapeIsValid(const HlslDecl *list, int depth)
{
    const HlslDecl *decl;

    if (depth > 128 || HlslDeclListHasCycle(list))
        return 0;
    for (decl = list; decl != NULL; decl = decl->next) {
        if (!HlslDeclShapeIsValid(decl) ||
            !HlslDeclListShapeIsValid(decl->members, depth + 1))
        {
            return 0;
        }
    }
    return 1;
} // HlslDeclListShapeIsValid

typedef struct HlslValidationContext_Rec {
    HlslModule *module;
    const HlslProfileDesc *profile;
    const HlslFunction *function;
    int loopDepth;
    int entryCalls;
} HlslValidationContext;

static int HlslValidateExpression(HlslValidationContext *context,
                                  const HlslExpr *expression,
                                  const HlslPointerFrame *parent,
                                  int depth);

static int HlslValidateExpressionList(HlslValidationContext *context,
                                      const HlslExpr *expression,
                                      const HlslPointerFrame *parent,
                                      int depth)
{
    if (depth > 512 || HlslExprListHasCycle(expression))
        return HlslFail(context->module, HLSL_ERROR_INVALID_IR,
                        expression != NULL ? &expression->loc : NULL,
                        "cyclic HLSL expression list");
    for (; expression != NULL; expression = expression->next) {
        if (!HlslValidateExpression(context, expression, parent, depth))
            return 0;
    }
    return 1;
} // HlslValidateExpressionList

static int HlslValidateUserCall(HlslValidationContext *context,
                                const HlslExpr *expression,
                                const HlslPointerFrame *frame, int depth)
{
    const HlslDecl *parameter;
    const HlslExpr *argument;

    if (expression->u.call.builtin != HLSL_BUILTIN_NONE ||
        !HlslOwnsFunction(context->module,
                          expression->u.call.function) ||
        expression->u.call.name == NULL ||
        expression->u.call.name[0] == '\0' ||
        !HlslTypesEqual(&expression->type,
                        &expression->u.call.function->result) ||
        HlslDeclListHasCycle(expression->u.call.function->parameters))
    {
        return HlslFail(context->module, HLSL_ERROR_INVALID_IR,
                        &expression->loc, "invalid HLSL function call");
    }
    parameter = expression->u.call.function->parameters;
    argument = expression->u.call.arguments;
    while (parameter != NULL && argument != NULL) {
        if (!HlslTypesEqual(&parameter->type, &argument->type)) {
            return HlslFail(context->module, HLSL_ERROR_INVALID_IR,
                            &expression->loc,
                            "HLSL call parameter type mismatch");
        }
        parameter = parameter->next;
        argument = argument->next;
    }
    if (parameter != NULL || argument != NULL) {
        return HlslFail(context->module, HLSL_ERROR_INVALID_IR,
                        &expression->loc, "HLSL call arity mismatch");
    }
    if (!HlslValidateExpressionList(context,
            expression->u.call.arguments, frame, depth + 1))
    {
        return 0;
    }
    if (expression->u.call.function == context->module->entry)
        context->entryCalls++;
    return 1;
} // HlslValidateUserCall

static int HlslValidateBuiltinCall(HlslValidationContext *context,
                                   const HlslExpr *expression,
                                   const HlslPointerFrame *frame, int depth)
{
    const HlslExpr *argument;
    HlslType parameters[HLSL_MAX_BUILTIN_ARGS];
    const char *spelling;
    int count;

    spelling = HlslBuiltinSpelling(expression->u.call.builtin);
    if (expression->u.call.function != NULL || spelling == NULL ||
        expression->u.call.name == NULL ||
        strcmp(expression->u.call.name, spelling) ||
        HlslExprListHasCycle(expression->u.call.arguments))
    {
        return HlslFail(context->module, HLSL_ERROR_INVALID_IR,
                        &expression->loc, "invalid HLSL builtin call");
    }
    count = 0;
    for (argument = expression->u.call.arguments; argument != NULL;
         argument = argument->next)
    {
        if (count >= HLSL_MAX_BUILTIN_ARGS) {
            return HlslFail(context->module, HLSL_ERROR_INVALID_IR,
                            &expression->loc, "HLSL builtin arity");
        }
        parameters[count++] = argument->type;
    }
    if (!HlslBuiltinAccepts(HLSL_STAGE_VERTEX,
            expression->u.call.builtin, &expression->type,
            parameters, count) &&
        !HlslBuiltinAccepts(HLSL_STAGE_PIXEL,
            expression->u.call.builtin, &expression->type,
            parameters, count))
    {
        return HlslFail(context->module, HLSL_ERROR_INVALID_IR,
                        &expression->loc,
                        "HLSL builtin signature mismatch");
    }
    return HlslValidateExpressionList(context,
        expression->u.call.arguments, frame, depth + 1);
} // HlslValidateBuiltinCall

static int HlslValidateIndexType(const HlslExpr *expression)
{
    HlslType expected;
    const HlslType *objectType;

    objectType = &expression->u.index.object->type;
    if (objectType->arraySize > 0)
        return HlslTypesEqual(&expression->type, objectType->elementType);
    if (objectType->rows != 0) {
        expected = HlslNumericType(objectType->base, objectType->cols);
        return HlslTypesEqual(&expression->type, &expected);
    }
    if (objectType->len > 1) {
        expected = HlslNumericType(objectType->base, 1);
        return HlslTypesEqual(&expression->type, &expected);
    }
    return 0;
} // HlslValidateIndexType

static int HlslSwizzleIsValid(const HlslExpr *expression)
{
    static const char *families[] = { "xyzw", "rgba", "stpq" };
    const char *mask;
    const char *component;
    HlslType expected;
    int family;
    int currentFamily;
    int length;

    mask = expression->u.swizzle.mask;
    if (mask == NULL || mask[0] == '\0' ||
        (!HlslIsNumericScalarOrVector(
             &expression->u.swizzle.object->type) &&
         !HlslIsBooleanScalarOrVector(
             &expression->u.swizzle.object->type)))
    {
        return 0;
    }
    family = -1;
    length = 0;
    for (; *mask != '\0'; mask++) {
        component = NULL;
        for (currentFamily = 0; currentFamily < 3; currentFamily++) {
            component = strchr(families[currentFamily], *mask);
            if (component != NULL)
                break;
        }
        if (component == NULL ||
            (family >= 0 && family != currentFamily) ||
            component - families[currentFamily] >=
                expression->u.swizzle.object->type.len ||
            ++length > 4)
        {
            return 0;
        }
        family = currentFamily;
    }
    expected = HlslNumericType(expression->u.swizzle.object->type.base,
                               length);
    return HlslTypesEqual(&expression->type, &expected);
} // HlslSwizzleIsValid

static int HlslValidateExpression(HlslValidationContext *context,
                                  const HlslExpr *expression,
                                  const HlslPointerFrame *parent,
                                  int depth)
{
    HlslPointerFrame frame;

    if (expression == NULL || depth > 512 ||
        HlslFrameContains(parent, expression) ||
        !HlslTypeIsValid(&expression->type, 1) ||
        !HlslTypeDeclsAreOwned(context->module, &expression->type))
    {
        return HlslFail(context->module, HLSL_ERROR_INVALID_IR,
                        expression != NULL ? &expression->loc : NULL,
                        "invalid HLSL expression graph");
    }
    frame.parent = parent;
    frame.pointer = expression;
    switch (expression->kind) {
    case HLSL_EXPR_SYMBOL:
        if (!HlslOwnsDecl(context->module, expression->u.symbol) ||
            !HlslTypesEqual(&expression->type,
                            &expression->u.symbol->type))
        {
            return HlslFail(context->module, HLSL_ERROR_INVALID_IR,
                            &expression->loc,
                            "invalid HLSL symbol expression");
        }
        return 1;
    case HLSL_EXPR_INT:
        return expression->type.base == HLSL_BASE_INT &&
               expression->type.len == 1 ? 1 :
               HlslFail(context->module, HLSL_ERROR_INVALID_IR,
                        &expression->loc, "invalid HLSL integer literal");
    case HLSL_EXPR_FLOAT:
        return expression->type.base == HLSL_BASE_FLOAT &&
               expression->type.len == 1 ? 1 :
               HlslFail(context->module, HLSL_ERROR_INVALID_IR,
                        &expression->loc, "invalid HLSL float literal");
    case HLSL_EXPR_BOOL:
        return expression->type.base == HLSL_BASE_BOOL &&
               expression->type.len == 1 ? 1 :
               HlslFail(context->module, HLSL_ERROR_INVALID_IR,
                        &expression->loc, "invalid HLSL bool literal");
    case HLSL_EXPR_UNARY:
        if (expression->u.unary.operand == NULL ||
            !HlslValidateExpression(context,
                expression->u.unary.operand, &frame, depth + 1) ||
            !HlslUnaryTypesAgree(expression))
        {
            return context->module->errors != 0 ? 0 :
                HlslFail(context->module, HLSL_ERROR_INVALID_IR,
                         &expression->loc,
                         "invalid HLSL unary expression");
        }
        return 1;
    case HLSL_EXPR_BINARY:
        if (expression->u.binary.left == NULL ||
            expression->u.binary.right == NULL ||
            !HlslValidateExpression(context,
                expression->u.binary.left, &frame, depth + 1) ||
            !HlslValidateExpression(context,
                expression->u.binary.right, &frame, depth + 1) ||
            !HlslBinaryTypesAgree(expression))
        {
            return context->module->errors != 0 ? 0 :
                HlslFail(context->module, HLSL_ERROR_INVALID_IR,
                         &expression->loc,
                         "invalid HLSL binary expression");
        }
        return 1;
    case HLSL_EXPR_CONDITIONAL:
        if (expression->u.conditional.condition == NULL ||
            expression->u.conditional.trueExpr == NULL ||
            expression->u.conditional.falseExpr == NULL ||
            !HlslValidateExpression(context,
                expression->u.conditional.condition, &frame, depth + 1) ||
            !HlslValidateExpression(context,
                expression->u.conditional.trueExpr, &frame, depth + 1) ||
            !HlslValidateExpression(context,
                expression->u.conditional.falseExpr, &frame, depth + 1) ||
            !HlslIsScalar(&expression->u.conditional.condition->type,
                          HLSL_BASE_BOOL) ||
            !HlslTypesEqual(&expression->type,
                &expression->u.conditional.trueExpr->type) ||
            !HlslTypesEqual(&expression->type,
                &expression->u.conditional.falseExpr->type))
        {
            return context->module->errors != 0 ? 0 :
                HlslFail(context->module, HLSL_ERROR_INVALID_IR,
                         &expression->loc,
                         "invalid HLSL conditional expression");
        }
        return 1;
    case HLSL_EXPR_CALL:
        if (expression->u.call.function != NULL)
            return HlslValidateUserCall(context, expression, &frame, depth);
        if (expression->u.call.builtin != HLSL_BUILTIN_NONE)
            return HlslValidateBuiltinCall(context, expression,
                                           &frame, depth);
        return HlslFail(context->module, HLSL_ERROR_INVALID_IR,
                        &expression->loc, "unresolved HLSL call");
    case HLSL_EXPR_CONSTRUCT:
        if (!HlslValidateExpressionList(context,
                expression->u.construct.arguments, &frame, depth + 1) ||
            !HlslConstructTypesAgree(expression))
        {
            return context->module->errors != 0 ? 0 :
                HlslFail(context->module, HLSL_ERROR_INVALID_IR,
                         &expression->loc,
                         "invalid HLSL constructor expression");
        }
        return 1;
    case HLSL_EXPR_CAST:
        if (expression->u.cast.expression == NULL ||
            !HlslValidateExpression(context,
                expression->u.cast.expression, &frame, depth + 1) ||
            (!(HlslIsNumericScalarOrVector(&expression->type) ||
               HlslIsBooleanScalarOrVector(&expression->type)) ||
             !(HlslIsNumericScalarOrVector(
                   &expression->u.cast.expression->type) ||
               HlslIsBooleanScalarOrVector(
                   &expression->u.cast.expression->type)) ||
             expression->type.len !=
                 expression->u.cast.expression->type.len))
        {
            return context->module->errors != 0 ? 0 :
                HlslFail(context->module, HLSL_ERROR_INVALID_IR,
                         &expression->loc,
                         "invalid HLSL cast expression");
        }
        return 1;
    case HLSL_EXPR_MEMBER:
        if (expression->u.member.object == NULL ||
            expression->u.member.decl == NULL ||
            expression->u.member.name == NULL ||
            expression->u.member.decl->name == NULL ||
            strcmp(expression->u.member.name,
                   expression->u.member.decl->name) ||
            !HlslValidateExpression(context,
                expression->u.member.object, &frame, depth + 1) ||
            !HlslOwnsDecl(context->module, expression->u.member.decl) ||
            !HlslMemberBelongsToType(
                &expression->u.member.object->type,
                expression->u.member.decl) ||
            !HlslTypesEqual(&expression->type,
                            &expression->u.member.decl->type))
        {
            return context->module->errors != 0 ? 0 :
                HlslFail(context->module, HLSL_ERROR_INVALID_IR,
                         &expression->loc,
                         "invalid HLSL member expression");
        }
        return 1;
    case HLSL_EXPR_INDEX:
        if (expression->u.index.object == NULL ||
            expression->u.index.index == NULL ||
            !HlslValidateExpression(context,
                expression->u.index.object, &frame, depth + 1) ||
            !HlslValidateExpression(context,
                expression->u.index.index, &frame, depth + 1) ||
            expression->u.index.index->type.base != HLSL_BASE_INT ||
            expression->u.index.index->type.len != 1 ||
            !HlslValidateIndexType(expression))
        {
            return context->module->errors != 0 ? 0 :
                HlslFail(context->module, HLSL_ERROR_INVALID_IR,
                         &expression->loc,
                         "invalid HLSL index expression");
        }
        return 1;
    case HLSL_EXPR_SWIZZLE:
        if (expression->u.swizzle.object == NULL ||
            !HlslValidateExpression(context,
                expression->u.swizzle.object, &frame, depth + 1) ||
            !HlslSwizzleIsValid(expression))
        {
            return context->module->errors != 0 ? 0 :
                HlslFail(context->module, HLSL_ERROR_INVALID_IR,
                         &expression->loc,
                         "invalid HLSL swizzle expression");
        }
        return 1;
    }
    return HlslFail(context->module, HLSL_ERROR_INVALID_IR,
                    &expression->loc, "invalid HLSL expression kind");
} // HlslValidateExpression

static int HlslValidateDeclInitializers(HlslValidationContext *context,
                                        const HlslDecl *decl, int depth)
{
    if (depth > 128 || HlslDeclListHasCycle(decl))
        return HlslFail(context->module, HLSL_ERROR_INVALID_IR,
                        decl != NULL ? &decl->loc : NULL,
                        "cyclic HLSL declaration list");
    for (; decl != NULL; decl = decl->next) {
        if ((decl->initializer != NULL &&
             (!HlslValidateExpression(context, decl->initializer,
                                      NULL, 0) ||
              !HlslTypesEqual(&decl->type, &decl->initializer->type))) ||
            !HlslValidateDeclInitializers(context, decl->members,
                                          depth + 1))
        {
            return context->module->errors != 0 ? 0 :
                HlslFail(context->module, HLSL_ERROR_INVALID_IR,
                         &decl->loc,
                         "invalid HLSL declaration initializer");
        }
    }
    return 1;
} // HlslValidateDeclInitializers

static int HlslValidateStatements(HlslValidationContext *context,
                                  const HlslStmt *statement, int depth)
{
    if (depth > 512 || HlslStmtListHasCycle(statement))
        return HlslFail(context->module, HLSL_ERROR_INVALID_IR,
                        statement != NULL ? &statement->loc : NULL,
                        "cyclic HLSL statement list");
    for (; statement != NULL; statement = statement->next) {
        switch (statement->kind) {
        case HLSL_STMT_DECLARATION:
            if (!HlslOwnsDecl(context->module,
                              statement->u.declaration) ||
                !HlslValidateDeclInitializers(context,
                    statement->u.declaration, 0))
            {
                return context->module->errors != 0 ? 0 :
                    HlslFail(context->module, HLSL_ERROR_INVALID_IR,
                             &statement->loc,
                             "invalid HLSL declaration statement");
            }
            break;
        case HLSL_STMT_EXPRESSION:
            if (statement->u.expression == NULL ||
                !HlslValidateExpression(context,
                    statement->u.expression, NULL, 0))
            {
                return context->module->errors != 0 ? 0 :
                    HlslFail(context->module, HLSL_ERROR_INVALID_IR,
                             &statement->loc,
                             "invalid HLSL expression statement");
            }
            break;
        case HLSL_STMT_IF:
            if (statement->u.ifStmt.condition == NULL ||
                !HlslValidateExpression(context,
                    statement->u.ifStmt.condition, NULL, 0) ||
                !HlslIsScalar(&statement->u.ifStmt.condition->type,
                              HLSL_BASE_BOOL) ||
                !HlslValidateStatements(context,
                    statement->u.ifStmt.trueBranch, depth + 1) ||
                !HlslValidateStatements(context,
                    statement->u.ifStmt.falseBranch, depth + 1))
            {
                return context->module->errors != 0 ? 0 :
                    HlslFail(context->module, HLSL_ERROR_INVALID_IR,
                             &statement->loc,
                             "invalid HLSL if statement");
            }
            break;
        case HLSL_STMT_WHILE:
        case HLSL_STMT_DO:
            if (statement->u.loop.condition == NULL ||
                !HlslValidateExpression(context,
                    statement->u.loop.condition, NULL, 0) ||
                !HlslIsScalar(&statement->u.loop.condition->type,
                              HLSL_BASE_BOOL))
            {
                return context->module->errors != 0 ? 0 :
                    HlslFail(context->module, HLSL_ERROR_INVALID_IR,
                             &statement->loc,
                             "invalid HLSL loop condition");
            }
            context->loopDepth++;
            if (!HlslValidateStatements(context, statement->u.loop.body,
                                        depth + 1))
            {
                context->loopDepth--;
                return context->module->errors != 0 ? 0 :
                    HlslFail(context->module, HLSL_ERROR_INVALID_IR,
                             &statement->loc,
                             "invalid HLSL loop body");
            }
            context->loopDepth--;
            break;
        case HLSL_STMT_FOR:
            context->loopDepth++;
            if (!HlslValidateStatements(context,
                    statement->u.forStmt.init, depth + 1) ||
                (statement->u.forStmt.condition != NULL &&
                 (!HlslValidateExpression(context,
                    statement->u.forStmt.condition, NULL, 0) ||
                  !HlslIsScalar(&statement->u.forStmt.condition->type,
                                HLSL_BASE_BOOL))) ||
                !HlslValidateStatements(context,
                    statement->u.forStmt.step, depth + 1) ||
                !HlslValidateStatements(context,
                    statement->u.forStmt.body, depth + 1))
            {
                context->loopDepth--;
                return context->module->errors != 0 ? 0 :
                    HlslFail(context->module, HLSL_ERROR_INVALID_IR,
                             &statement->loc,
                             "invalid HLSL for statement");
            }
            context->loopDepth--;
            break;
        case HLSL_STMT_BLOCK:
            if (!HlslValidateStatements(context, statement->u.block,
                                        depth + 1))
            {
                return 0;
            }
            break;
        case HLSL_STMT_RETURN:
            if ((context->function->result.base == HLSL_BASE_VOID &&
                 statement->u.returnExpr != NULL) ||
                (context->function->result.base != HLSL_BASE_VOID &&
                 (statement->u.returnExpr == NULL ||
                  !HlslValidateExpression(context,
                    statement->u.returnExpr, NULL, 0) ||
                  !HlslTypesEqual(&context->function->result,
                    &statement->u.returnExpr->type))))
            {
                return context->module->errors != 0 ? 0 :
                    HlslFail(context->module, HLSL_ERROR_INVALID_IR,
                             &statement->loc,
                             "HLSL return type mismatch");
            }
            break;
        case HLSL_STMT_DISCARD:
            break;
        case HLSL_STMT_BREAK:
        case HLSL_STMT_CONTINUE:
            if (context->loopDepth == 0) {
                return HlslFail(context->module, HLSL_ERROR_INVALID_IR,
                                &statement->loc,
                                "HLSL jump outside loop");
            }
            break;
        default:
            return HlslFail(context->module, HLSL_ERROR_INVALID_IR,
                            &statement->loc,
                            "invalid HLSL statement kind");
        }
    }
    return 1;
} // HlslValidateStatements

static int HlslValidateStructuralModule(HlslModule *module,
                                        const HlslProfileDesc *profile)
{
    HlslValidationContext context;
    HlslFunction *function;
    int wrapperCalls;
    int otherCalls;
    int entryFlags;

    if (HlslNameListHasCycle(module->names) ||
        HlslDeclListHasCycle(module->globals) ||
        HlslDeclListHasCycle(module->structs) ||
        HlslFunctionListHasCycle(module->functions) ||
        HlslBindingListHasCycle(module->bindings) ||
        HlslAllocationListHasCycle(module->allocatedBindings) ||
        !HlslDeclListShapeIsValid(module->globals, 0) ||
        !HlslDeclListShapeIsValid(module->structs, 0) ||
        !HlslDeclTypesAreOwned(module, module->globals, 0) ||
        !HlslDeclTypesAreOwned(module, module->structs, 0))
    {
        return HlslFail(module, HLSL_ERROR_INVALID_IR, NULL,
                        "malformed HLSL module lists");
    }
    if (!HlslOwnsFunction(module, module->entry) ||
        !HlslOwnsFunction(module, module->wrapper) ||
        module->entry == module->wrapper || module->wrapper->name == NULL ||
        strcmp(module->wrapper->name, "main"))
    {
        return HlslFail(module, HLSL_ERROR_INVALID_IR, NULL,
                        "unowned HLSL entry function");
    }
    memset(&context, 0, sizeof(context));
    context.module = module;
    context.profile = profile;
    if (!HlslValidateDeclInitializers(&context, module->globals, 0) ||
        !HlslValidateDeclInitializers(&context, module->structs, 0))
    {
        return 0;
    }
    otherCalls = context.entryCalls;
    wrapperCalls = 0;
    entryFlags = 0;
    /* Validate every callable signature before any body can compare an
       argument against it.  Function lists are not guaranteed to place a
       callee before its caller, and malformed recursive member lists must
       never reach type equality. */
    for (function = module->functions; function != NULL;
         function = function->next)
    {
        if (function->name == NULL || function->name[0] == '\0' ||
            !HlslTypeIsValid(&function->result, 1) ||
            !HlslTypeDeclsAreOwned(module, &function->result) ||
            !HlslDeclListShapeIsValid(function->parameters, 0) ||
            !HlslDeclListShapeIsValid(function->locals, 0) ||
            !HlslDeclTypesAreOwned(module, function->parameters, 0) ||
            !HlslDeclTypesAreOwned(module, function->locals, 0))
        {
            return HlslFail(module, HLSL_ERROR_INVALID_IR,
                            &function->loc,
                            "invalid HLSL function structure");
        }
        if (function->isEntry)
            entryFlags++;
    }
    for (function = module->functions; function != NULL;
         function = function->next)
    {
        context.function = function;
        context.loopDepth = 0;
        context.entryCalls = 0;
        if (!HlslValidateDeclInitializers(&context,
                function->parameters, 0) ||
            !HlslValidateDeclInitializers(&context,
                function->locals, 0) ||
            !HlslValidateStatements(&context, function->body, 0))
        {
            return 0;
        }
        if (function == module->wrapper)
            wrapperCalls += context.entryCalls;
        else
            otherCalls += context.entryCalls;
    }
    if (entryFlags != 1 || !module->entry->isEntry)
        return HlslFail(module, HLSL_ERROR_INVALID_IR,
                        &module->entry->loc,
                        "invalid HLSL entry ownership");
    if (wrapperCalls != 1 || otherCalls != 0) {
        return HlslFail(module, HLSL_ERROR_ENTRY_ABI,
                        &module->wrapper->loc,
                        "entry wrapper call count");
    }
    return 1;
} // HlslValidateStructuralModule

static HlslBase HlslSamplerTypeBaseInner(const HlslType *type, int *isArray,
    int *malformed, const HlslPointerFrame *parent)
{
    HlslPointerFrame frame;

    if (type == NULL || type->arraySize < 0 ||
        HlslFrameContains(parent, type))
    {
        *malformed = 1;
        return HLSL_BASE_VOID;
    }
    frame.parent = parent;
    frame.pointer = type;
    if (type->arraySize > 0) {
        if (type->elementType == NULL) {
            *malformed = 1;
            return HLSL_BASE_VOID;
        }
        *isArray = 1;
        return HlslSamplerTypeBaseInner(type->elementType, isArray,
                                        malformed, &frame);
    }
    if (type->elementType != NULL)
        return HLSL_BASE_VOID;
    switch (type->base) {
    case HLSL_BASE_SAMPLER1D:
    case HLSL_BASE_SAMPLER2D:
    case HLSL_BASE_SAMPLER3D:
    case HLSL_BASE_SAMPLERCUBE:
        return type->len == 1 && type->rows == 0 && type->cols == 0 ?
               type->base : HLSL_BASE_VOID;
    default:
        return HLSL_BASE_VOID;
    }
} // HlslSamplerTypeBaseInner

static const char *HlslBindingReason(const HlslBinding *binding,
                                     const char *fallback)
{
    if (binding != NULL) {
        if (binding->semantic != NULL && binding->semantic[0] != '\0')
            return binding->semantic;
        if (binding->publicName != NULL && binding->publicName[0] != '\0')
            return binding->publicName;
        if (binding->name != NULL && binding->name[0] != '\0')
            return binding->name;
    }
    return fallback;
} // HlslBindingReason

int HlslValidateSamplerUsage(HlslModule *module,
                             const HlslProfileDesc *profile)
{
    HlslBinding *binding;
    HlslBase units[HLSL_MAX_SAMPLERS];
    HlslBase base;
    int isArray;
    int malformed;
    int regno;

    if (module == NULL || profile == NULL ||
        module->stage != profile->stage || module->errors != 0)
    {
        return HlslFail(module, HLSL_ERROR_INVALID_IR, NULL,
                        "invalid HLSL sampler module");
    }
    if (HlslBindingListHasCycle(module->bindings))
        return HlslFail(module, HLSL_ERROR_INVALID_IR, NULL,
                        "cyclic HLSL binding list");
    memset(units, 0, sizeof(units));
    for (binding = module->bindings; binding != NULL;
         binding = binding->next)
    {
        isArray = 0;
        malformed = 0;
        base = HlslSamplerTypeBaseInner(&binding->type, &isArray,
                                        &malformed, NULL);
        if (malformed)
            return HlslFail(module, HLSL_ERROR_INVALID_IR, &binding->loc,
                            "malformed HLSL sampler type");
        if (base == HLSL_BASE_VOID) {
            if (binding->storage == HLSL_STORAGE_SAMPLER)
                return HlslFail(module, HLSL_ERROR_SAMPLER, &binding->loc,
                    HlslBindingReason(binding, "sampler"));
            continue;
        }
        if (isArray)
            return HlslFail(module, HLSL_ERROR_SAMPLER, &binding->loc,
                            "sampler array");
        if (binding->storage != HLSL_STORAGE_SAMPLER)
            return HlslFail(module, HLSL_ERROR_SAMPLER, &binding->loc,
                HlslBindingReason(binding, "sampler"));
        if (profile->limits == NULL)
            return HlslFail(module, HLSL_ERROR_INVALID_IR, &binding->loc,
                            "missing HLSL sampler limits");
        if (!binding->hasExplicitRegister && !binding->isAllocated)
            continue;
        if (binding->physical.bank != HLSL_REGISTER_S ||
            binding->physical.regno < 0 ||
            binding->physical.regno >= profile->limits->samplers ||
            binding->physical.regno >= HLSL_MAX_SAMPLERS)
        {
            return HlslFail(module, HLSL_ERROR_SAMPLER, &binding->loc,
                HlslBindingReason(binding, "sampler register"));
        }
        regno = binding->physical.regno;
        if (units[regno] != HLSL_BASE_VOID && units[regno] != base)
            return HlslFail(module, HLSL_ERROR_SAMPLER, &binding->loc,
                HlslBindingReason(binding, "sampler unit"));
        units[regno] = base;
    }
    return 1;
} // HlslValidateSamplerUsage

static int HlslSetResourceFailure(HlslModule *module, const char *resource,
                                  int used, int available,
                                  const HlslLoc *loc)
{
    if (module->errors == 0) {
        module->resourceName = resource;
        module->resourceUsed = used;
        module->resourceAvailable = available;
    }
    return HlslFail(module, HLSL_ERROR_RESOURCE_LIMIT, loc, resource);
} // HlslSetResourceFailure

static int HlslCountDeclarations(const HlslDecl *members)
{
    int count;

    count = 0;
    for (; members != NULL; members = members->next) {
        if (count == INT_MAX)
            return INT_MAX;
        count++;
    }
    return count;
} // HlslCountDeclarations

static const HlslLoc *HlslLastDeclLoc(const HlslDecl *members)
{
    if (members == NULL)
        return NULL;
    while (members->next != NULL)
        members = members->next;
    return &members->loc;
} // HlslLastDeclLoc

static const HlslSemanticDesc *HlslFindSemanticDescriptor(
    const HlslProfileDesc *profile, const char *canonical, int isOutput)
{
    const HlslSemanticDesc *semantics;
    char root[64];
    int count;
    int index;
    int i;

    if (profile == NULL || canonical == NULL ||
        !HlslParseSemantic(canonical, root, sizeof(root), &index))
    {
        return NULL;
    }
    if (isOutput) {
        semantics = profile->outputSemantics;
        count = profile->numOutputSemantics;
    } else {
        semantics = profile->inputSemantics;
        count = profile->numInputSemantics;
    }
    for (i = 0; i < count; i++) {
        if (!strcmp(root, semantics[i].root) &&
            index >= semantics[i].firstIndex &&
            index < semantics[i].firstIndex + semantics[i].count)
        {
            return &semantics[i];
        }
    }
    return NULL;
} // HlslFindSemanticDescriptor

static int HlslInterfaceTypeIsValid(const HlslType *type,
                                    const HlslSemanticDesc *descriptor,
                                    int isOutput)
{
    int expectedDirection;
    int oppositeDirection;

    expectedDirection = isOutput ? SEM_OUT : SEM_IN;
    oppositeDirection = isOutput ? SEM_IN : SEM_OUT;
    if (type == NULL || descriptor == NULL || type->arraySize != 0 ||
        type->base != HLSL_BASE_FLOAT || type->rows != 0 ||
        type->cols != 0 || type->len < 1 || type->len > 4 ||
        !(descriptor->properties & expectedDirection) ||
        (descriptor->properties & oppositeDirection))
    {
        return 0;
    }
    switch (descriptor->interfaceKind) {
    case HLSL_INTERFACE_POSITION:
    case HLSL_INTERFACE_PIXEL_POSITION:
    case HLSL_INTERFACE_COLOR:
        return type->len == descriptor->width && type->len > 1;
    case HLSL_INTERFACE_POINT_SIZE:
    case HLSL_INTERFACE_FACE:
    case HLSL_INTERFACE_DEPTH:
        return type->len == 1 && descriptor->width == 1;
    case HLSL_INTERFACE_VARYING:
        return type->len <= descriptor->width;
    }
    return 0;
} // HlslInterfaceTypeIsValid

static int HlslValidateInterfaceSemantics(HlslModule *module,
    const HlslProfileDesc *profile, const HlslDecl *members, int isOutput)
{
    const HlslDecl *left;
    const HlslDecl *right;
    const HlslSemanticDesc *descriptor;
    const char *leftCanonical;
    const char *rightCanonical;

    for (left = members; left != NULL; left = left->next) {
        leftCanonical = HlslCanonicalSemantic(profile, left->semantic,
                                              isOutput);
        descriptor = HlslFindSemanticDescriptor(profile, leftCanonical,
                                                isOutput);
        if (leftCanonical == NULL || descriptor == NULL ||
            !HlslInterfaceTypeIsValid(&left->type, descriptor, isOutput))
        {
            return HlslFail(module, HLSL_ERROR_SEMANTIC, &left->loc,
                            HlslBindingReason(NULL, left->semantic));
        }
        for (right = left->next; right != NULL; right = right->next) {
            rightCanonical = HlslCanonicalSemantic(profile,
                                                    right->semantic,
                                                    isOutput);
            if ((left->name != NULL && right->name != NULL &&
                 !strcmp(left->name, right->name)) ||
                (rightCanonical != NULL &&
                 !strcmp(leftCanonical, rightCanonical)))
            {
                return HlslFail(module, HLSL_ERROR_INTERFACE_CONFLICT,
                                &right->loc, leftCanonical);
            }
        }
    }
    return 1;
} // HlslValidateInterfaceSemantics

static int HlslColorOutputUsage(const HlslProfileDesc *profile,
                                const HlslDecl *members,
                                const HlslLoc **failureLoc)
{
    const char *canonical;
    char root[64];
    int index;
    int used;

    used = 0;
    for (; members != NULL; members = members->next) {
        canonical = HlslCanonicalSemantic(profile, members->semantic, 1);
        if (canonical != NULL &&
            HlslParseSemantic(canonical, root, sizeof(root), &index) &&
            !strcmp(root, "COLOR") && index < INT_MAX && index + 1 > used)
        {
            used = index + 1;
            if (failureLoc != NULL)
                *failureLoc = &members->loc;
        }
    }
    return used;
} // HlslColorOutputUsage

static int HlslHasPosition(const HlslProfileDesc *profile,
                           const HlslDecl *members)
{
    const char *canonical;

    for (; members != NULL; members = members->next) {
        canonical = HlslCanonicalSemantic(profile, members->semantic, 1);
        if (canonical != NULL && !strcmp(canonical, "POSITION0"))
            return 1;
    }
    return 0;
} // HlslHasPosition

static int HlslValidateInterfaces(HlslModule *module,
                                  const HlslProfileDesc *profile)
{
    HlslDecl *structure;
    HlslDecl *input;
    HlslDecl *output;
    const HlslLoc *colorLoc;
    int used;
    int colors;

    input = NULL;
    output = NULL;
    for (structure = module->structs; structure != NULL;
         structure = structure->next)
    {
        if (structure->storage == HLSL_STORAGE_INPUT) {
            if (input != NULL)
                return HlslFail(module, HLSL_ERROR_INTERFACE_CONFLICT,
                                &structure->loc,
                                "HLSL input interface");
            input = structure;
        } else if (structure->storage == HLSL_STORAGE_OUTPUT) {
            if (output != NULL)
                return HlslFail(module, HLSL_ERROR_INTERFACE_CONFLICT,
                                &structure->loc,
                                "HLSL output interface");
            output = structure;
        }
    }
    if (input == NULL || output == NULL || input->members == NULL ||
        output->members == NULL)
    {
        return HlslFail(module, HLSL_ERROR_ENTRY_ABI, NULL,
                        "HLSL interface structures");
    }
    if (!HlslValidateInterfaceSemantics(module, profile,
                                        input->members, 0) ||
        !HlslValidateInterfaceSemantics(module, profile,
                                        output->members, 1))
    {
        return 0;
    }
    used = HlslCountDeclarations(input->members);
    if (used > profile->limits->inputs)
        return HlslSetResourceFailure(module, "inputs", used,
            profile->limits->inputs, HlslLastDeclLoc(input->members));
    used = HlslCountDeclarations(output->members);
    if (used > profile->limits->outputs)
        return HlslSetResourceFailure(module, "outputs", used,
            profile->limits->outputs, HlslLastDeclLoc(output->members));
    if (profile->stage == HLSL_STAGE_PIXEL) {
        colorLoc = NULL;
        colors = HlslColorOutputUsage(profile, output->members, &colorLoc);
        if (colors > profile->limits->colorOutputs)
            return HlslSetResourceFailure(module, "color outputs", colors,
                profile->limits->colorOutputs, colorLoc);
    }
    if (module->stage == HLSL_STAGE_VERTEX &&
        !HlslHasPosition(profile, output->members))
    {
        return HlslFail(module, HLSL_ERROR_REQUIRED_POSITION,
                        &output->loc, "POSITION0");
    }
    return 1;
} // HlslValidateInterfaces

static HlslRegisterBank HlslTypeBankInner(const HlslType *type,
                                          const HlslPointerFrame *parent)
{
    HlslPointerFrame frame;
    const HlslDecl *member;
    HlslRegisterBank bank;
    HlslRegisterBank memberBank;

    if (type == NULL || HlslFrameContains(parent, type))
        return HLSL_REGISTER_NONE;
    frame.parent = parent;
    frame.pointer = type;
    if (type->arraySize > 0)
        return HlslTypeBankInner(type->elementType, &frame);
    if (type->base == HLSL_BASE_STRUCT) {
        bank = HLSL_REGISTER_NONE;
        for (member = type->members; member != NULL; member = member->next) {
            memberBank = HlslTypeBankInner(&member->type, &frame);
            if (memberBank == HLSL_REGISTER_NONE)
                return HLSL_REGISTER_NONE;
            if (bank == HLSL_REGISTER_NONE)
                bank = memberBank;
            else if (bank != memberBank)
                return HLSL_REGISTER_NONE;
        }
        return bank;
    }
    switch (type->base) {
    case HLSL_BASE_FLOAT: return HLSL_REGISTER_C;
    case HLSL_BASE_INT: return HLSL_REGISTER_I;
    case HLSL_BASE_BOOL: return HLSL_REGISTER_B;
    case HLSL_BASE_SAMPLER1D:
    case HLSL_BASE_SAMPLER2D:
    case HLSL_BASE_SAMPLER3D:
    case HLSL_BASE_SAMPLERCUBE:
        return HLSL_REGISTER_S;
    default:
        return HLSL_REGISTER_NONE;
    }
} // HlslTypeBankInner

static int HlslBankLimit(const HlslProfileDesc *profile,
                         HlslRegisterBank bank)
{
    switch (bank) {
    case HLSL_REGISTER_C: return profile->limits->floatConstants;
    case HLSL_REGISTER_I: return profile->limits->intConstants;
    case HLSL_REGISTER_B: return profile->limits->boolConstants;
    case HLSL_REGISTER_S: return profile->limits->samplers;
    case HLSL_REGISTER_NONE: break;
    }
    return 0;
} // HlslBankLimit

static int HlslBankMaximum(HlslRegisterBank bank)
{
    switch (bank) {
    case HLSL_REGISTER_C: return HLSL_MAX_FLOAT_CONSTANTS;
    case HLSL_REGISTER_I: return HLSL_MAX_INT_CONSTANTS;
    case HLSL_REGISTER_B: return HLSL_MAX_BOOL_CONSTANTS;
    case HLSL_REGISTER_S: return HLSL_MAX_SAMPLERS;
    case HLSL_REGISTER_NONE: break;
    }
    return 0;
} // HlslBankMaximum

static unsigned char *HlslBankUsage(HlslRegisterBank bank,
                                    unsigned char *c,
                                    unsigned char *i,
                                    unsigned char *b,
                                    unsigned char *s)
{
    switch (bank) {
    case HLSL_REGISTER_C: return c;
    case HLSL_REGISTER_I: return i;
    case HLSL_REGISTER_B: return b;
    case HLSL_REGISTER_S: return s;
    case HLSL_REGISTER_NONE: break;
    }
    return NULL;
} // HlslBankUsage

static const char *HlslBankName(HlslRegisterBank bank)
{
    switch (bank) {
    case HLSL_REGISTER_C: return "c";
    case HLSL_REGISTER_I: return "i";
    case HLSL_REGISTER_B: return "b";
    case HLSL_REGISTER_S: return "s";
    case HLSL_REGISTER_NONE: break;
    }
    return "register";
} // HlslBankName

static int HlslStringsEqual(const char *left, const char *right)
{
    if (left == NULL || left[0] == '\0')
        return right == NULL || right[0] == '\0';
    return right != NULL && !strcmp(left, right);
} // HlslStringsEqual

static int HlslPhysicalBindingsEqual(const HlslPhysicalBinding *left,
                                     const HlslPhysicalBinding *right)
{
    return left->bank == right->bank && left->regno == right->regno &&
           left->span == right->span &&
           left->component == right->component;
} // HlslPhysicalBindingsEqual

static int HlslLocationsEqual(const HlslLoc *left, const HlslLoc *right)
{
    return left->file == right->file && left->line == right->line;
} // HlslLocationsEqual

static const char *HlslPublicBindingName(const HlslBinding *binding)
{
    if (binding == NULL)
        return NULL;
    if (binding->publicName != NULL && binding->publicName[0] != '\0')
        return binding->publicName;
    return binding->name;
} // HlslPublicBindingName

static int HlslPublicLeafBelongsToRoot(const HlslBinding *root,
                                       const HlslBinding *leaf)
{
    const char *rootName;
    const char *leafName;
    size_t length;

    rootName = HlslPublicBindingName(root);
    leafName = HlslPublicBindingName(leaf);
    if (rootName == NULL || rootName[0] == '\0' ||
        leafName == NULL || leafName[0] == '\0')
    {
        return 0;
    }
    length = strlen(rootName);
    return !strncmp(rootName, leafName, length) &&
           (leafName[length] == '\0' || leafName[length] == '.' ||
            leafName[length] == '[');
} // HlslPublicLeafBelongsToRoot

static int HlslNameOwnsEmission(const HlslModule *module,
                                const HlslBinding *leaf,
                                const char *emitted)
{
    const HlslName *name;

    if (emitted == NULL || emitted[0] == '\0')
        return 0;
    for (name = module->names; name != NULL; name = name->next) {
        if (name->identity == leaf && name->emitted != NULL &&
            !strcmp(name->emitted, emitted))
        {
            return 1;
        }
    }
    return 0;
} // HlslNameOwnsEmission

static int HlslAllocationOccurrences(const HlslModule *module,
                                     const HlslBinding *target)
{
    const HlslBinding *binding;
    int count;

    count = 0;
    for (binding = module->allocatedBindings; binding != NULL;
         binding = binding->allocationNext)
    {
        if (binding == target)
            count++;
    }
    return count;
} // HlslAllocationOccurrences

static int HlslRootLeafOccurrences(const HlslModule *module,
                                   const HlslBinding *target)
{
    const HlslBinding *root;
    const HlslBinding *leaf;
    int count;

    count = 0;
    for (root = module->bindings; root != NULL; root = root->next) {
        for (leaf = root->leafBindings; leaf != NULL; leaf = leaf->next) {
            if (leaf == target)
                count++;
        }
    }
    return count;
} // HlslRootLeafOccurrences

static int HlslDefaultsEqual(const HlslBinding *root,
                             const HlslBinding *leaf, int offset)
{
    int index;

    if (leaf->defaultCount < 0 || offset < 0 ||
        offset > root->defaultCount ||
        leaf->defaultCount > root->defaultCount - offset)
    {
        return 0;
    }
    if (leaf->defaultCount == 0)
        return leaf->defaultValues == NULL &&
               leaf->defaultLiterals == NULL;
    if ((root->defaultValues == NULL) !=
        (leaf->defaultValues == NULL) ||
        (root->defaultLiterals == NULL) !=
        (leaf->defaultLiterals == NULL))
    {
        return 0;
    }
    for (index = 0; index < leaf->defaultCount; index++) {
        if (root->defaultValues != NULL &&
            root->defaultValues[offset + index] !=
                leaf->defaultValues[index])
        {
            return 0;
        }
        if (root->defaultLiterals != NULL &&
            (root->defaultLiterals[offset + index].base !=
                 leaf->defaultLiterals[index].base ||
             memcmp(&root->defaultLiterals[offset + index].value,
                    &leaf->defaultLiterals[index].value,
                    sizeof(leaf->defaultLiterals[index].value))))
        {
            return 0;
        }
    }
    return 1;
} // HlslDefaultsEqual

static int HlslValidateBindingLeaf(HlslModule *module,
    const HlslBinding *root, const HlslBinding *leaf,
    int expectedOffset, int defaultOffset)
{
    const HlslDecl *declaration;

    declaration = leaf->declaration;
    if (leaf->leafBindings != NULL || !leaf->isAllocated ||
        HlslAllocationOccurrences(module, leaf) != 1 ||
        HlslRootLeafOccurrences(module, leaf) != 1 ||
        declaration == NULL ||
        !HlslDeclTreeContains(module->globals, declaration, 0) ||
        leaf->storage != root->storage ||
        declaration->storage != leaf->storage ||
        !HlslTypesEqual(&leaf->type, &declaration->type) ||
        !HlslStringsEqual(leaf->semantic, root->semantic) ||
        !HlslStringsEqual(declaration->semantic, leaf->semantic) ||
        !HlslLocationsEqual(&leaf->loc, &root->loc) ||
        !HlslLocationsEqual(&declaration->loc, &leaf->loc) ||
        leaf->sourceOrdinal != root->sourceOrdinal ||
        declaration->sourceOrdinal != leaf->sourceOrdinal ||
        leaf->recursiveOffset != expectedOffset ||
        leaf->hasExplicitRegister != root->hasExplicitRegister ||
        leaf->isOutput != root->isOutput ||
        leaf->sourceBase != root->sourceBase ||
        !HlslPublicLeafBelongsToRoot(root, leaf) ||
        leaf->name == NULL ||
        strcmp(leaf->name, HlslPublicBindingName(leaf)) ||
        !HlslNameOwnsEmission(module, leaf, declaration->name) ||
        !HlslDefaultsEqual(root, leaf, defaultOffset) ||
        (leaf->defaultCount == 0) != (declaration->initializer == NULL) ||
        !HlslPhysicalBindingsEqual(&leaf->physical,
                                   &declaration->physical))
    {
        return 0;
    }
    return 1;
} // HlslValidateBindingLeaf

static int HlslValidateBindings(HlslModule *module,
                                const HlslProfileDesc *profile)
{
    HlslBinding *binding;
    HlslBinding *leaf;
    unsigned char c[HLSL_MAX_FLOAT_CONSTANTS];
    unsigned char i[HLSL_MAX_INT_CONSTANTS];
    unsigned char b[HLSL_MAX_BOOL_CONSTANTS];
    unsigned char s[HLSL_MAX_SAMPLERS];
    unsigned char *usage;
    HlslRegisterBank bank;
    int expectedSpan;
    int limit;
    int maximum;
    int offset;
    int defaultOffset;
    int leafOffset;
    int leafSpan;

    memset(c, 0, sizeof(c));
    memset(i, 0, sizeof(i));
    memset(b, 0, sizeof(b));
    memset(s, 0, sizeof(s));
    for (binding = module->bindings; binding != NULL;
         binding = binding->next)
    {
        if (!binding->isAllocated || binding->leafBindings == NULL ||
            binding->name == NULL || binding->name[0] == '\0' ||
            HlslPublicBindingName(binding) == NULL ||
            strcmp(binding->name, HlslPublicBindingName(binding)) ||
            HlslBindingListHasCycle(binding->leafBindings))
        {
            return HlslFail(module, HLSL_ERROR_INVALID_IR, &binding->loc,
                            "unallocated HLSL binding");
        }
        leafOffset = 0;
        defaultOffset = 0;
        for (leaf = binding->leafBindings; leaf != NULL;
             leaf = leaf->next)
        {
            leafSpan = HlslTypeRegisterSpan(&leaf->type);
            if (leafSpan <= 0 ||
                !HlslValidateBindingLeaf(module, binding, leaf,
                                         leafOffset, defaultOffset) ||
                leafOffset > INT_MAX - leafSpan ||
                defaultOffset > INT_MAX - leaf->defaultCount)
            {
                return HlslFail(module, HLSL_ERROR_INVALID_IR,
                                &binding->loc,
                                "invalid HLSL binding graph");
            }
            leafOffset += leafSpan;
            defaultOffset += leaf->defaultCount;
        }
        if (leafOffset != HlslTypeRegisterSpan(&binding->type) ||
            defaultOffset != binding->defaultCount ||
            (binding->leafBindings->next == NULL &&
             (binding->declaration != binding->leafBindings->declaration ||
              !HlslTypesEqual(&binding->type,
                              &binding->declaration->type) ||
              binding->storage != binding->declaration->storage ||
              !HlslPhysicalBindingsEqual(&binding->physical,
                  &binding->leafBindings->physical))) ||
            (binding->leafBindings->next != NULL &&
             (binding->declaration != NULL ||
              binding->physical.bank != HLSL_REGISTER_NONE ||
              binding->physical.regno != 0 ||
              binding->physical.span != 0 ||
              binding->physical.component != 0)))
        {
            return HlslFail(module, HLSL_ERROR_INVALID_IR, &binding->loc,
                            "invalid HLSL binding root");
        }
    }
    for (binding = module->allocatedBindings; binding != NULL;
         binding = binding->allocationNext)
    {
        if (HlslRootLeafOccurrences(module, binding) != 1)
            return HlslFail(module, HLSL_ERROR_INVALID_IR, &binding->loc,
                            "orphaned HLSL binding leaf");
        bank = HlslTypeBankInner(&binding->type, NULL);
        expectedSpan = HlslTypeRegisterSpan(&binding->type);
        if (bank == HLSL_REGISTER_NONE || expectedSpan <= 0 ||
            binding->physical.bank != bank ||
            binding->physical.span != expectedSpan ||
            binding->physical.component != 0 ||
            binding->physical.regno < 0 || binding->declaration == NULL ||
            binding->declaration->physical.bank != bank ||
            binding->declaration->physical.regno !=
                binding->physical.regno ||
            binding->declaration->physical.span != expectedSpan)
        {
            return HlslFail(module, HLSL_ERROR_INVALID_IR, &binding->loc,
                            "invalid HLSL physical binding");
        }
        limit = HlslBankLimit(profile, bank);
        maximum = HlslBankMaximum(bank);
        if (limit < 0 || limit > maximum)
            return HlslFail(module, HLSL_ERROR_INVALID_IR, &binding->loc,
                            "invalid HLSL register limit");
        if (expectedSpan > limit ||
            binding->physical.regno > limit - expectedSpan)
        {
            return HlslSetResourceFailure(module, HlslBankName(bank),
                binding->physical.regno > INT_MAX - expectedSpan ?
                    INT_MAX : binding->physical.regno + expectedSpan,
                limit, &binding->loc);
        }
        usage = HlslBankUsage(bank, c, i, b, s);
        for (offset = 0; offset < expectedSpan; offset++) {
            if (usage[binding->physical.regno + offset]) {
                return HlslFail(module, HLSL_ERROR_REGISTER_COLLISION,
                                &binding->loc,
                                HlslBindingReason(binding, "register"));
            }
            usage[binding->physical.regno + offset] = 1;
        }
    }
    return 1;
} // HlslValidateBindings

static int HlslValidateTargetExpression(HlslModule *module,
    const HlslProfileDesc *profile, const HlslExpr *expression);

static int HlslValidateTargetDeclarations(HlslModule *module,
    const HlslProfileDesc *profile, const HlslDecl *declaration)
{
    for (; declaration != NULL; declaration = declaration->next) {
        if (!HlslValidateTargetExpression(module, profile,
                declaration->initializer) ||
            !HlslValidateTargetDeclarations(module, profile,
                declaration->members))
        {
            return 0;
        }
    }
    return 1;
} // HlslValidateTargetDeclarations

static int HlslValidateTargetExpressionList(HlslModule *module,
    const HlslProfileDesc *profile, const HlslExpr *expression)
{
    for (; expression != NULL; expression = expression->next) {
        if (!HlslValidateTargetExpression(module, profile, expression))
            return 0;
    }
    return 1;
} // HlslValidateTargetExpressionList

static int HlslValidateTargetExpression(HlslModule *module,
    const HlslProfileDesc *profile, const HlslExpr *expression)
{
    const HlslExpr *argument;
    HlslType parameters[HLSL_MAX_BUILTIN_ARGS];
    int count;

    if (expression == NULL)
        return 1;
    switch (expression->kind) {
    case HLSL_EXPR_UNARY:
        return HlslValidateTargetExpression(module, profile,
                                             expression->u.unary.operand);
    case HLSL_EXPR_BINARY:
        return HlslValidateTargetExpression(module, profile,
                    expression->u.binary.left) &&
               HlslValidateTargetExpression(module, profile,
                    expression->u.binary.right);
    case HLSL_EXPR_CONDITIONAL:
        return HlslValidateTargetExpression(module, profile,
                    expression->u.conditional.condition) &&
               HlslValidateTargetExpression(module, profile,
                    expression->u.conditional.trueExpr) &&
               HlslValidateTargetExpression(module, profile,
                    expression->u.conditional.falseExpr);
    case HLSL_EXPR_CALL:
        if (!HlslValidateTargetExpressionList(module, profile,
                                              expression->u.call.arguments))
        {
            return 0;
        }
        if (expression->u.call.builtin == HLSL_BUILTIN_NONE)
            return 1;
        count = 0;
        for (argument = expression->u.call.arguments; argument != NULL;
             argument = argument->next)
        {
            parameters[count++] = argument->type;
        }
        if (HlslBuiltinAccepts(profile->stage,
                expression->u.call.builtin, &expression->type,
                parameters, count))
        {
            return 1;
        }
        if (HlslBuiltinIsTexture(expression->u.call.builtin))
            return HlslFail(module, HLSL_ERROR_SAMPLER, &expression->loc,
                            expression->u.call.name);
        return HlslFail(module, HLSL_ERROR_STAGE_OPERATION,
                        &expression->loc, expression->u.call.name);
    case HLSL_EXPR_CONSTRUCT:
        return HlslValidateTargetExpressionList(module, profile,
            expression->u.construct.arguments);
    case HLSL_EXPR_CAST:
        return HlslValidateTargetExpression(module, profile,
                                             expression->u.cast.expression);
    case HLSL_EXPR_MEMBER:
        return HlslValidateTargetExpression(module, profile,
                                             expression->u.member.object);
    case HLSL_EXPR_INDEX:
        return HlslValidateTargetExpression(module, profile,
                    expression->u.index.object) &&
               HlslValidateTargetExpression(module, profile,
                    expression->u.index.index);
    case HLSL_EXPR_SWIZZLE:
        return HlslValidateTargetExpression(module, profile,
                                             expression->u.swizzle.object);
    default:
        return 1;
    }
} // HlslValidateTargetExpression

static int HlslValidateTargetStatements(HlslModule *module,
    const HlslProfileDesc *profile, const HlslStmt *statement)
{
    for (; statement != NULL; statement = statement->next) {
        switch (statement->kind) {
        case HLSL_STMT_DECLARATION:
            if (!HlslValidateTargetExpression(module, profile,
                    statement->u.declaration->initializer))
            {
                return 0;
            }
            break;
        case HLSL_STMT_EXPRESSION:
            if (!HlslValidateTargetExpression(module, profile,
                                               statement->u.expression))
                return 0;
            break;
        case HLSL_STMT_IF:
            if (!HlslValidateTargetExpression(module, profile,
                    statement->u.ifStmt.condition) ||
                !HlslValidateTargetStatements(module, profile,
                    statement->u.ifStmt.trueBranch) ||
                !HlslValidateTargetStatements(module, profile,
                    statement->u.ifStmt.falseBranch))
            {
                return 0;
            }
            break;
        case HLSL_STMT_WHILE:
        case HLSL_STMT_DO:
            if (!HlslValidateTargetExpression(module, profile,
                    statement->u.loop.condition) ||
                !HlslValidateTargetStatements(module, profile,
                    statement->u.loop.body))
            {
                return 0;
            }
            break;
        case HLSL_STMT_FOR:
            if (!HlslValidateTargetStatements(module, profile,
                    statement->u.forStmt.init) ||
                !HlslValidateTargetExpression(module, profile,
                    statement->u.forStmt.condition) ||
                !HlslValidateTargetStatements(module, profile,
                    statement->u.forStmt.step) ||
                !HlslValidateTargetStatements(module, profile,
                    statement->u.forStmt.body))
            {
                return 0;
            }
            break;
        case HLSL_STMT_BLOCK:
            if (!HlslValidateTargetStatements(module, profile,
                                               statement->u.block))
                return 0;
            break;
        case HLSL_STMT_RETURN:
            if (!HlslValidateTargetExpression(module, profile,
                                               statement->u.returnExpr))
                return 0;
            break;
        case HLSL_STMT_DISCARD:
            if (profile->stage != HLSL_STAGE_PIXEL)
                return HlslFail(module, HLSL_ERROR_STAGE_OPERATION,
                                &statement->loc, "discard");
            break;
        default:
            break;
        }
    }
    return 1;
} // HlslValidateTargetStatements

static int HlslValidateTargetModule(HlslModule *module,
                                    const HlslProfileDesc *profile)
{
    HlslFunction *function;

    if (profile->limits == NULL || profile->limits->inputs < 0 ||
        profile->limits->outputs < 0 ||
        profile->limits->colorOutputs < 0 ||
        profile->limits->floatConstants < 0 ||
        profile->limits->intConstants < 0 ||
        profile->limits->boolConstants < 0 ||
        profile->limits->samplers < 0)
    {
        return HlslFail(module, HLSL_ERROR_INVALID_IR, NULL,
                        "missing HLSL profile limits");
    }
    if (!HlslValidateTargetDeclarations(module, profile,
            module->globals) ||
        !HlslValidateTargetDeclarations(module, profile,
            module->structs) ||
        !HlslValidateInterfaces(module, profile) ||
        !HlslValidateBindings(module, profile) ||
        !HlslValidateSamplerUsage(module, profile))
    {
        return 0;
    }
    for (function = module->functions; function != NULL;
         function = function->next)
    {
        if (!HlslValidateTargetDeclarations(module, profile,
                function->parameters) ||
            !HlslValidateTargetDeclarations(module, profile,
                function->locals))
        {
            return 0;
        }
        if (!HlslValidateTargetStatements(module, profile, function->body))
            return 0;
    }
    return 1;
} // HlslValidateTargetModule

static int HlslIsEmptyModule(const HlslModule *module)
{
    const HlslFunction *entry;

    entry = module->entry;
    return entry != NULL && module->wrapper == NULL &&
           module->functions == entry && entry->next == NULL &&
           entry->name != NULL && !strcmp(entry->name, "main") &&
           entry->result.base == HLSL_BASE_VOID &&
           entry->parameters == NULL && entry->body == NULL &&
           module->structs == NULL && module->globals == NULL &&
           module->bindings == NULL;
} // HlslIsEmptyModule

int HlslValidateModule(HlslModule *module,
                       const HlslProfileDesc *profile)
{
    if (module == NULL || profile == NULL ||
        (module->stage != HLSL_STAGE_VERTEX &&
         module->stage != HLSL_STAGE_PIXEL) ||
        module->stage != profile->stage || module->entry == NULL ||
        module->errors != 0)
    {
        return HlslFail(module, HLSL_ERROR_INVALID_IR, NULL,
                        "invalid HLSL module");
    }
    if (HlslIsEmptyModule(module))
        return 1;
    if (module->wrapper == NULL || module->wrapper->name == NULL ||
        strcmp(module->wrapper->name, "main"))
    {
        return HlslFail(module, HLSL_ERROR_ENTRY_ABI, NULL,
                        "missing HLSL entry wrapper");
    }
    return HlslValidateStructuralModule(module, profile) &&
           HlslValidateTargetModule(module, profile);
} // HlslValidateModule

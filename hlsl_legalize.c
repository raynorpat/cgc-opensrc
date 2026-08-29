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
    if (module != NULL && module->errors == 0) {
        module->errorKind = kind;
        module->errorReason = reason;
        if (loc != NULL)
            module->errorLoc = *loc;
    }
    if (module != NULL)
        module->errors++;
    return 0;
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

static int HlslMultiplyResult(const HlslType *left,
                              const HlslType *right, HlslType *result)
{
    int length;

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

static int HlslLegalizeExpr(HlslModule *module, HlslExpr *expression)
{
    HlslExpr *argument;
    HlslDecl *parameter;
    HlslType resultType;

    if (expression == NULL || HlslTypeName(&expression->type) == NULL)
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
    case HLSL_EXPR_BINARY:
        if (expression->u.binary.op != HLSL_OP_ASSIGN &&
            expression->u.binary.op != HLSL_OP_MULTIPLY)
        {
            return HlslLegalizeFailure(module,
                                       HLSL_ERROR_UNSUPPORTED_OPERATION,
                                       &expression->loc,
                                       "HLSL binary operation");
        }
        if (!HlslLegalizeExpr(module, expression->u.binary.left) ||
            !HlslLegalizeExpr(module, expression->u.binary.right))
        {
            return 0;
        }
        if (expression->u.binary.op == HLSL_OP_ASSIGN) {
            if (HlslTypesEqual(&expression->type,
                               &expression->u.binary.left->type) &&
                HlslTypesEqual(&expression->type,
                               &expression->u.binary.right->type))
            {
                return 1;
            }
        } else if (HlslMultiplyResult(&expression->u.binary.left->type,
                                      &expression->u.binary.right->type,
                                      &resultType) &&
                   HlslTypesEqual(&expression->type, &resultType))
        {
            return 1;
        }
        return HlslLegalizeFailure(module, HLSL_ERROR_INVALID_IR,
                                   &expression->loc,
                                   "HLSL binary types");
    case HLSL_EXPR_CALL:
        if (expression->u.call.function == NULL ||
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
            if (!HlslLegalizeExpr(module, argument))
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
            if (!HlslLegalizeExpr(module, argument))
                return 0;
        }
        return 1;
    case HLSL_EXPR_MEMBER:
        if (expression->u.member.decl == NULL ||
            expression->u.member.name == NULL ||
            expression->u.member.object == NULL)
        {
            return HlslLegalizeFailure(module, HLSL_ERROR_INVALID_IR,
                                       &expression->loc,
                                       "HLSL member declaration");
        }
        if (!HlslLegalizeExpr(module, expression->u.member.object))
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
        if (!HlslLegalizeExpr(module, expression->u.index.object) ||
            !HlslLegalizeExpr(module, expression->u.index.index))
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

static int HlslLegalizeDeclarations(HlslModule *module, HlslDecl *decl)
{
    for (; decl != NULL; decl = decl->next) {
        if (decl->name == NULL || HlslTypeName(&decl->type) == NULL)
            return HlslLegalizeFailure(module, HLSL_ERROR_INVALID_IR,
                                       &decl->loc,
                                       "HLSL declaration");
        if (decl->initializer != NULL &&
            !HlslLegalizeExpr(module, decl->initializer))
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
            !HlslLegalizeDeclarations(module, decl->members))
        {
            return 0;
        }
    }
    return 1;
} // HlslLegalizeDeclarations

static int HlslLegalizeStatements(HlslModule *module, HlslStmt *statement,
                                  const HlslType *result)
{
    for (; statement != NULL; statement = statement->next) {
        switch (statement->kind) {
        case HLSL_STMT_EXPRESSION:
            if (!HlslLegalizeExpr(module, statement->u.expression))
                return 0;
            break;
        case HLSL_STMT_BLOCK:
            if (!HlslLegalizeStatements(module, statement->u.block,
                                        result))
                return 0;
            break;
        case HLSL_STMT_RETURN:
            if (statement->u.returnExpr != NULL) {
                if (!HlslLegalizeExpr(module, statement->u.returnExpr))
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
        default:
            return HlslLegalizeFailure(module,
                                       HLSL_ERROR_UNSUPPORTED_OPERATION,
                                       &statement->loc,
                                       "HLSL statement");
        }
    }
    return 1;
} // HlslLegalizeStatements

int HlslLegalizeModule(HlslModule *module,
                       const HlslProfileDesc *profile)
{
    HlslFunction *function;

    if (module == NULL || profile == NULL ||
        module->stage != profile->stage || module->entry == NULL)
    {
        return HlslLegalizeFailure(module, HLSL_ERROR_INVALID_IR, NULL,
                                   "invalid HLSL legalization module");
    }
    if (!HlslLegalizeDeclarations(module, module->globals) ||
        !HlslLegalizeDeclarations(module, module->structs))
    {
        return 0;
    }
    for (function = module->functions; function != NULL;
         function = function->next)
    {
        if (function->name == NULL || HlslTypeName(&function->result) == NULL ||
            !HlslLegalizeDeclarations(module, function->parameters) ||
            !HlslLegalizeDeclarations(module, function->locals))
        {
            return HlslLegalizeFailure(module, HLSL_ERROR_INVALID_IR,
                                       &function->loc,
                                       "HLSL function");
        }
        if (!HlslLegalizeStatements(module, function->body,
                                    &function->result))
        {
            return 0;
        }
    }
    return module->errors == 0;
} // HlslLegalizeModule

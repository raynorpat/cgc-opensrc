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
// hlsl_validate.c
//

#include <stdio.h>
#include <string.h>

#include "slglobals.h"
#include "hlsl_hal.h"

static int HlslValidateFailure(HlslModule *module, HlslErrorKind kind,
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
} // HlslValidateFailure

static int HlslDeclInList(const HlslDecl *list, const HlslDecl *target)
{
    for (; list != NULL; list = list->next) {
        if (list == target)
            return 1;
    }
    return 0;
} // HlslDeclInList

static int HlslOwnsDecl(const HlslModule *module, const HlslDecl *target)
{
    const HlslDecl *structure;
    const HlslFunction *function;

    if (target == NULL)
        return 0;
    if (HlslDeclInList(module->globals, target))
        return 1;
    for (structure = module->structs; structure != NULL;
         structure = structure->next)
    {
        if (structure == target ||
            HlslDeclInList(structure->members, target))
        {
            return 1;
        }
    }
    for (function = module->functions; function != NULL;
         function = function->next)
    {
        if (HlslDeclInList(function->parameters, target) ||
            HlslDeclInList(function->locals, target))
        {
            return 1;
        }
    }
    return 0;
} // HlslOwnsDecl

static int HlslOwnsFunction(const HlslModule *module,
                            const HlslFunction *target)
{
    const HlslFunction *function;

    for (function = module->functions; function != NULL;
         function = function->next)
    {
        if (function == target)
            return 1;
    }
    return 0;
} // HlslOwnsFunction

static int HlslCountEntryCalls(HlslModule *module, const HlslExpr *expression,
                               int *count)
{
    const HlslExpr *argument;
    HlslType params[3];
    int paramCount;

    if (expression == NULL)
        return 1;
    switch (expression->kind) {
    case HLSL_EXPR_SYMBOL:
        if (!HlslOwnsDecl(module, expression->u.symbol))
            return HlslValidateFailure(module, HLSL_ERROR_INVALID_IR,
                                       &expression->loc,
                                       "unowned HLSL declaration");
        return 1;
    case HLSL_EXPR_INT:
    case HLSL_EXPR_FLOAT:
    case HLSL_EXPR_BOOL:
        return 1;
    case HLSL_EXPR_UNARY:
        return HlslCountEntryCalls(module, expression->u.unary.operand,
                                   count);
    case HLSL_EXPR_BINARY:
        return HlslCountEntryCalls(module, expression->u.binary.left,
                                   count) &&
               HlslCountEntryCalls(module, expression->u.binary.right,
                                   count);
    case HLSL_EXPR_CONDITIONAL:
        return HlslCountEntryCalls(module,
                    expression->u.conditional.condition, count) &&
               HlslCountEntryCalls(module,
                    expression->u.conditional.trueExpr, count) &&
               HlslCountEntryCalls(module,
                    expression->u.conditional.falseExpr, count);
    case HLSL_EXPR_CALL:
        if (expression->u.call.function == NULL &&
            expression->u.call.name != NULL)
        {
            paramCount = 0;
            for (argument = expression->u.call.arguments;
                 argument != NULL; argument = argument->next)
            {
                if (paramCount >= 3)
                    break;
                params[paramCount++] = argument->type;
                if (!HlslCountEntryCalls(module, argument, count))
                    return 0;
            }
            if (argument == NULL &&
                HlslLookupBuiltin(module->stage,
                    expression->u.call.name, &expression->type,
                    params, paramCount) != HLSL_BUILTIN_NONE)
            {
                return 1;
            }
        }
        if (!HlslOwnsFunction(module, expression->u.call.function))
            return HlslValidateFailure(module, HLSL_ERROR_INVALID_IR,
                                       &expression->loc,
                                       "unowned HLSL function");
        if (expression->u.call.function == module->entry)
            (*count)++;
        for (argument = expression->u.call.arguments; argument != NULL;
             argument = argument->next)
        {
            if (!HlslCountEntryCalls(module, argument, count))
                return 0;
        }
        return 1;
    case HLSL_EXPR_CONSTRUCT:
        for (argument = expression->u.construct.arguments;
             argument != NULL; argument = argument->next)
        {
            if (!HlslCountEntryCalls(module, argument, count))
                return 0;
        }
        return 1;
    case HLSL_EXPR_CAST:
        return HlslCountEntryCalls(module, expression->u.cast.expression,
                                   count);
    case HLSL_EXPR_MEMBER:
        if (!HlslOwnsDecl(module, expression->u.member.decl))
            return HlslValidateFailure(module, HLSL_ERROR_INVALID_IR,
                                       &expression->loc,
                                       "unowned HLSL member");
        return HlslCountEntryCalls(module, expression->u.member.object,
                                   count);
    case HLSL_EXPR_INDEX:
        return HlslCountEntryCalls(module, expression->u.index.object,
                                   count) &&
               HlslCountEntryCalls(module, expression->u.index.index,
                                   count);
    case HLSL_EXPR_SWIZZLE:
        return HlslCountEntryCalls(module, expression->u.swizzle.object,
                                   count);
    default:
        return HlslValidateFailure(module, HLSL_ERROR_INVALID_IR,
                                   &expression->loc,
                                   "invalid HLSL expression");
    }
} // HlslCountEntryCalls

static int HlslCountStatementCalls(HlslModule *module,
                                   const HlslStmt *statement,
                                   int *count)
{
    for (; statement != NULL; statement = statement->next) {
        switch (statement->kind) {
        case HLSL_STMT_DECLARATION:
            if (!HlslOwnsDecl(module, statement->u.declaration) ||
                !HlslCountEntryCalls(module,
                    statement->u.declaration->initializer, count))
            {
                return 0;
            }
            break;
        case HLSL_STMT_EXPRESSION:
            if (!HlslCountEntryCalls(module, statement->u.expression,
                                     count))
            {
                return 0;
            }
            break;
        case HLSL_STMT_IF:
            if (!HlslCountEntryCalls(module,
                    statement->u.ifStmt.condition, count) ||
                !HlslCountStatementCalls(module,
                    statement->u.ifStmt.trueBranch, count) ||
                !HlslCountStatementCalls(module,
                    statement->u.ifStmt.falseBranch, count))
            {
                return 0;
            }
            break;
        case HLSL_STMT_WHILE:
        case HLSL_STMT_DO:
            if (!HlslCountEntryCalls(module,
                    statement->u.loop.condition, count) ||
                !HlslCountStatementCalls(module,
                    statement->u.loop.body, count))
            {
                return 0;
            }
            break;
        case HLSL_STMT_FOR:
            if (!HlslCountStatementCalls(module,
                    statement->u.forStmt.init, count) ||
                !HlslCountEntryCalls(module,
                    statement->u.forStmt.condition, count) ||
                !HlslCountStatementCalls(module,
                    statement->u.forStmt.step, count) ||
                !HlslCountStatementCalls(module,
                    statement->u.forStmt.body, count))
            {
                return 0;
            }
            break;
        case HLSL_STMT_BLOCK:
            if (!HlslCountStatementCalls(module, statement->u.block,
                                         count))
            {
                return 0;
            }
            break;
        case HLSL_STMT_RETURN:
            if (!HlslCountEntryCalls(module, statement->u.returnExpr,
                                     count))
            {
                return 0;
            }
            break;
        case HLSL_STMT_DISCARD:
        case HLSL_STMT_BREAK:
        case HLSL_STMT_CONTINUE:
            break;
        default:
            return HlslValidateFailure(module, HLSL_ERROR_INVALID_IR,
                                       &statement->loc,
                                       "invalid HLSL statement");
        }
    }
    return 1;
} // HlslCountStatementCalls

static int HlslValidateDeclarationExpressions(HlslModule *module,
                                              const HlslDecl *decl,
                                              int *entryCallCount)
{
    for (; decl != NULL; decl = decl->next) {
        if ((decl->initializer != NULL &&
             !HlslCountEntryCalls(module, decl->initializer,
                                  entryCallCount)) ||
            (decl->members != NULL &&
             !HlslValidateDeclarationExpressions(module, decl->members,
                                                  entryCallCount)))
        {
            return 0;
        }
    }
    return 1;
} // HlslValidateDeclarationExpressions

static int HlslSemanticIsUnique(const HlslDecl *members)
{
    const HlslDecl *left;
    const HlslDecl *right;

    for (left = members; left != NULL; left = left->next) {
        if (left->semantic == NULL || left->semantic[0] == '\0')
            return 0;
        for (right = left->next; right != NULL; right = right->next) {
            if ((right->name != NULL && left->name != NULL &&
                 !strcmp(left->name, right->name)) ||
                (right->semantic != NULL &&
                 !strcmp(left->semantic, right->semantic)))
            {
                return 0;
            }
        }
    }
    return 1;
} // HlslSemanticIsUnique

static int HlslHasPosition(const HlslDecl *members)
{
    for (; members != NULL; members = members->next) {
        if (members->semantic != NULL &&
            !strcmp(members->semantic, "POSITION0"))
        {
            return 1;
        }
    }
    return 0;
} // HlslHasPosition

static int HlslIsEmptyModule(const HlslModule *module)
{
    const HlslFunction *entry;

    entry = module->entry;
    return entry != NULL && module->wrapper == NULL &&
           module->functions == entry && entry->next == NULL &&
           !strcmp(entry->name, "main") &&
           entry->result.base == HLSL_BASE_VOID &&
           entry->parameters == NULL && entry->body == NULL &&
           module->structs == NULL && module->globals == NULL &&
           module->bindings == NULL;
} // HlslIsEmptyModule

int HlslValidateModule(HlslModule *module,
                       const HlslProfileDesc *profile)
{
    HlslBinding *binding;
    HlslDecl *structure;
    HlslFunction *function;
    int functionCallCount;
    int entryCallCount;
    int nonWrapperCallCount;
    int sawInput;
    int sawOutput;

    if (module == NULL || profile == NULL ||
        (module->stage != HLSL_STAGE_VERTEX &&
         module->stage != HLSL_STAGE_PIXEL) ||
        module->stage != profile->stage || module->entry == NULL ||
        module->errors != 0)
    {
        return HlslValidateFailure(module, HLSL_ERROR_INVALID_IR, NULL,
                                   "invalid HLSL module");
    }
    if (HlslIsEmptyModule(module))
        return 1;
    if (module->wrapper == NULL || strcmp(module->wrapper->name, "main"))
        return HlslValidateFailure(module, HLSL_ERROR_ENTRY_ABI, NULL,
                                   "missing HLSL entry wrapper");
    if (module->entry == module->wrapper ||
        !HlslOwnsFunction(module, module->entry) ||
        !HlslOwnsFunction(module, module->wrapper))
    {
        return HlslValidateFailure(module, HLSL_ERROR_INVALID_IR, NULL,
                                   "unowned HLSL entry function");
    }
    nonWrapperCallCount = 0;
    if (!HlslValidateDeclarationExpressions(module, module->globals,
                                             &nonWrapperCallCount) ||
        !HlslValidateDeclarationExpressions(module, module->structs,
                                             &nonWrapperCallCount))
    {
        return 0;
    }
    entryCallCount = -1;
    for (function = module->functions; function != NULL;
         function = function->next)
    {
        functionCallCount = 0;
        if (!HlslValidateDeclarationExpressions(module,
                                                function->parameters,
                                                &functionCallCount) ||
            !HlslValidateDeclarationExpressions(module, function->locals,
                                                 &functionCallCount) ||
            !HlslCountStatementCalls(module, function->body,
                                     &functionCallCount))
        {
            return 0;
        }
        if (function == module->wrapper)
            entryCallCount = functionCallCount;
    }
    if (entryCallCount != 1)
        return HlslValidateFailure(module, HLSL_ERROR_ENTRY_ABI,
                                   &module->wrapper->loc,
                                   "entry wrapper call count");
    sawInput = 0;
    sawOutput = 0;
    for (structure = module->structs; structure != NULL;
         structure = structure->next)
    {
        if (structure->storage == HLSL_STORAGE_INPUT) {
            if (sawInput || !HlslSemanticIsUnique(structure->members))
                return HlslValidateFailure(module,
                    HLSL_ERROR_INTERFACE_CONFLICT, &structure->loc,
                    "HLSL input interface");
            sawInput = 1;
        } else if (structure->storage == HLSL_STORAGE_OUTPUT) {
            if (sawOutput || !HlslSemanticIsUnique(structure->members))
                return HlslValidateFailure(module,
                    HLSL_ERROR_INTERFACE_CONFLICT, &structure->loc,
                    "HLSL output interface");
            if (module->stage == HLSL_STAGE_VERTEX &&
                !HlslHasPosition(structure->members))
            {
                return HlslValidateFailure(module,
                    HLSL_ERROR_REQUIRED_POSITION, &structure->loc,
                    "POSITION0");
            }
            sawOutput = 1;
        }
    }
    if (!sawInput || !sawOutput)
        return HlslValidateFailure(module, HLSL_ERROR_ENTRY_ABI, NULL,
                                   "HLSL interface structures");
    for (binding = module->bindings; binding != NULL;
         binding = binding->next)
    {
        if (!binding->isAllocated || binding->leafBindings == NULL ||
            binding->declaration == NULL)
        {
            return HlslValidateFailure(module, HLSL_ERROR_INVALID_IR,
                                       &binding->loc,
                                       "unallocated HLSL binding");
        }
    }
    return 1;
} // HlslValidateModule

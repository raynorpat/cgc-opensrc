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

static int HlslLegalizeExpr(HlslModule *module, HlslExpr *expression)
{
    HlslExpr *argument;

    if (expression == NULL || HlslTypeName(&expression->type) == NULL)
        return HlslLegalizeFailure(module, HLSL_ERROR_INVALID_IR,
            expression != NULL ? &expression->loc : NULL,
            "HLSL expression type");
    switch (expression->kind) {
    case HLSL_EXPR_SYMBOL:
        if (expression->u.symbol == NULL)
            return HlslLegalizeFailure(module, HLSL_ERROR_INVALID_IR,
                                       &expression->loc,
                                       "HLSL symbol declaration");
        return 1;
    case HLSL_EXPR_INT:
    case HLSL_EXPR_FLOAT:
    case HLSL_EXPR_BOOL:
        return 1;
    case HLSL_EXPR_BINARY:
        if (expression->u.binary.op != HLSL_OP_ASSIGN &&
            expression->u.binary.op != HLSL_OP_MULTIPLY)
        {
            return HlslLegalizeFailure(module,
                                       HLSL_ERROR_UNSUPPORTED_OPERATION,
                                       &expression->loc,
                                       "HLSL binary operation");
        }
        return HlslLegalizeExpr(module, expression->u.binary.left) &&
               HlslLegalizeExpr(module, expression->u.binary.right);
    case HLSL_EXPR_CALL:
        if (expression->u.call.function == NULL ||
            expression->u.call.name == NULL)
        {
            return HlslLegalizeFailure(module, HLSL_ERROR_INVALID_IR,
                                       &expression->loc,
                                       "HLSL call");
        }
        for (argument = expression->u.call.arguments; argument != NULL;
             argument = argument->next)
        {
            if (!HlslLegalizeExpr(module, argument))
                return 0;
        }
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
            expression->u.member.name == NULL)
        {
            return HlslLegalizeFailure(module, HLSL_ERROR_INVALID_IR,
                                       &expression->loc,
                                       "HLSL member declaration");
        }
        return HlslLegalizeExpr(module, expression->u.member.object);
    case HLSL_EXPR_INDEX:
        return HlslLegalizeExpr(module, expression->u.index.object) &&
               HlslLegalizeExpr(module, expression->u.index.index);
    default:
        return HlslLegalizeFailure(module,
                                   HLSL_ERROR_UNSUPPORTED_OPERATION,
                                   &expression->loc,
                                   "HLSL expression");
    }
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
        if (decl->members != NULL &&
            !HlslLegalizeDeclarations(module, decl->members))
        {
            return 0;
        }
    }
    return 1;
} // HlslLegalizeDeclarations

static int HlslLegalizeStatements(HlslModule *module, HlslStmt *statement)
{
    for (; statement != NULL; statement = statement->next) {
        switch (statement->kind) {
        case HLSL_STMT_EXPRESSION:
            if (!HlslLegalizeExpr(module, statement->u.expression))
                return 0;
            break;
        case HLSL_STMT_BLOCK:
            if (!HlslLegalizeStatements(module, statement->u.block))
                return 0;
            break;
        case HLSL_STMT_RETURN:
            if (statement->u.returnExpr != NULL &&
                !HlslLegalizeExpr(module, statement->u.returnExpr))
            {
                return 0;
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
            !HlslLegalizeDeclarations(module, function->locals) ||
            !HlslLegalizeStatements(module, function->body))
        {
            return HlslLegalizeFailure(module, HLSL_ERROR_INVALID_IR,
                                       &function->loc,
                                       "HLSL function");
        }
    }
    return module->errors == 0;
} // HlslLegalizeModule

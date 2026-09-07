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

static int HlslCollectCallsInExpr(HlslLowerContext *context, expr *source);
int HlslCollectIRCallsInStatements(HlslLowerContext *context,
                                           const CgIRStmt *source);

const CgIRFunction *HlslFindSourceIRFunction(
    const HlslLowerContext *context, const Symbol *symbol)
{
    const CgIRFunction *function;

    if (context == NULL || context->sourceIR == NULL || symbol == NULL)
        return NULL;
    for (function = context->sourceIR->functions; function != NULL;
         function = function->next)
    {
        if (function->symbol == symbol)
            return function;
    }
    return NULL;
} // HlslFindSourceIRFunction

int HlslCollectCallsInStatements(HlslLowerContext *context,
                                        stmt *source)
{
    for (; source != NULL; source = source->commonst.next) {
        context->statementLoc = source->commonst.loc;
        switch (source->commonst.kind) {
        case EXPR_STMT:
            if (!HlslCollectCallsInExpr(context, source->exprst.exp))
                return 0;
            break;
        case BLOCK_STMT:
            if (!HlslCollectCallsInStatements(context,
                                               source->blockst.body))
            {
                return 0;
            }
            break;
        case IF_STMT:
            if (!HlslCollectCallsInExpr(context, source->ifst.cond) ||
                !HlslCollectCallsInStatements(context,
                                               source->ifst.thenstmt) ||
                !HlslCollectCallsInStatements(context,
                                               source->ifst.elsestmt))
            {
                return 0;
            }
            break;
        case WHILE_STMT:
        case DO_STMT:
            if (!HlslCollectCallsInExpr(context, source->whilest.cond) ||
                !HlslCollectCallsInStatements(context,
                                               source->whilest.body))
            {
                return 0;
            }
            break;
        case FOR_STMT:
            if (!HlslCollectCallsInStatements(context,
                                               source->forst.init) ||
                !HlslCollectCallsInExpr(context, source->forst.cond) ||
                !HlslCollectCallsInStatements(context,
                                               source->forst.step) ||
                !HlslCollectCallsInStatements(context,
                                               source->forst.body))
            {
                return 0;
            }
            break;
        case RETURN_STMT:
            if (!HlslCollectCallsInExpr(context, source->returnst.exp))
                return 0;
            break;
        case COMMENT_STMT:
        case DISCARD_STMT:
        case BREAK_STMT:
        case CONTINUE_STMT:
            if (source->commonst.kind == DISCARD_STMT &&
                !HlslCollectCallsInExpr(context,
                                        source->discardst.cond))
            {
                return 0;
            }
            break;
        default:
            return HlslLowerFailure(context,
                                    HLSL_ERROR_UNSUPPORTED_OPERATION,
                                    "HLSL helper statement", NULL);
        }
    }
    return 1;
} // HlslCollectCallsInStatements

static int HlslCollectHelper(HlslLowerContext *context, Symbol *symbol,
                             const SourceLoc *callLoc)
{
    const CgIRFunction *sourceFunction;
    HlslFunction *function;
    HlslType result;
    char *generated;
    const char *name;
    Symbol *caller;
    SourceLoc callerLoc;

    if (symbol == NULL || symbol->kind != FUNCTION_S)
        return 0;
    if (symbol->properties & SYMB_IS_BUILTIN)
        return 1;
    if (!HlslRejectStorage(context, symbol))
        return 0;
    function = HlslFindFunction(context->module, symbol);
    if (function != NULL) {
        if (function->visitState == 1)
            return HlslLowerFailure(context,
                                    HLSL_ERROR_UNSUPPORTED_OPERATION,
                                    "recursive HLSL helper",
                                    callLoc);
        return 1;
    }
    if (!HlslEnsureType(context, symbol->type->fun.rettype) ||
        !HlslLowerType(context, symbol->type->fun.rettype,
                       &result, &symbol->loc))
    {
        return 0;
    }
    generated = HlslGeneratedSource(context,
                                    GetAtomString(atable, symbol->name));
    name = generated != NULL ?
           HlslAllocateGeneratedName(context->module, symbol,
                                     generated) : NULL;
    function = HlslNewFunction(context->module, result, name);
    if (function == NULL)
        return 0;
    function->identity = symbol;
    function->visitState = 1;
    caller = context->collectingHelper;
    callerLoc = context->statementLoc;
    if (caller != NULL)
        function->needsPrototype = 1;
    HlslSetLoc(&function->loc, &symbol->loc);
    HlslAppendFunction(&context->module->functions, function);
    context->collectingHelper = symbol;
    sourceFunction = HlslFindSourceIRFunction(context, symbol);
    if ((context->sourceIR != NULL && sourceFunction == NULL) ||
        (sourceFunction != NULL &&
         !HlslCollectIRCallsInStatements(context, sourceFunction->body)) ||
        (sourceFunction == NULL &&
         !HlslCollectCallsInStatements(context,
                                       symbol->details.fun.statements)))
    {
        context->collectingHelper = caller;
        context->statementLoc = callerLoc;
        return 0;
    }
    context->collectingHelper = caller;
    context->statementLoc = callerLoc;
    function->visitState = 2;
    return 1;
} // HlslCollectHelper

static int HlslCollectCallsInExpr(HlslLowerContext *context, expr *source)
{
    const SourceLoc *callLoc;
    HlslBuiltin builtin;
    HlslType result;
    HlslType params[HLSL_MAX_BUILTIN_ARGS];
    int paramCount;
    int builtinStatus;

    if (source == NULL)
        return 1;
    switch (source->common.kind) {
    case DECL_N:
    case SYMB_N:
    case CONST_N:
        return 1;
    case UNARY_N:
        return HlslCollectCallsInExpr(context, source->un.arg);
    case BINARY_N:
        callLoc = source->bin.op == FUN_CALL_OP ?
                  GetExprCallSite(source) : NULL;
        if (source->bin.op == FUN_CALL_OP && source->bin.left != NULL &&
            source->bin.left->common.kind == SYMB_N)
        {
            builtinStatus = HlslResolveBuiltinSymbol(context,
                source->bin.left->sym.symbol, callLoc, &builtin,
                &result, params, &paramCount);
            if (builtinStatus < 0)
                return 0;
            if (builtinStatus == 0 &&
                !HlslCollectHelper(context,
                    source->bin.left->sym.symbol, callLoc))
            {
                return 0;
            }
        }
        return HlslCollectCallsInExpr(context, source->bin.left) &&
               HlslCollectCallsInExpr(context, source->bin.right);
    case TRINARY_N:
        return HlslCollectCallsInExpr(context, source->tri.arg1) &&
               HlslCollectCallsInExpr(context, source->tri.arg2) &&
               HlslCollectCallsInExpr(context, source->tri.arg3);
    default:
        return 0;
    }
} // HlslCollectCallsInExpr

static int HlslCollectIRCallsInExpr(HlslLowerContext *context,
                                    const CgIRExpr *source)
{
    const CgIRExpr *argument;
    HlslBuiltin builtin;
    HlslType result;
    HlslType params[HLSL_MAX_BUILTIN_ARGS];
    int paramCount;
    int builtinStatus;

    if (source == NULL)
        return 1;
    switch (source->kind) {
    case CGIR_EXPR_CONSTANT:
    case CGIR_EXPR_SYMBOL:
        return 1;
    case CGIR_EXPR_MEMBER:
        return HlslCollectIRCallsInExpr(context, source->u.member.object);
    case CGIR_EXPR_INDEX:
        return HlslCollectIRCallsInExpr(context, source->u.index.object) &&
               HlslCollectIRCallsInExpr(context, source->u.index.index);
    case CGIR_EXPR_LENGTH:
        return HlslCollectIRCallsInExpr(context, source->u.length.object);
    case CGIR_EXPR_SWIZZLE:
        return HlslCollectIRCallsInExpr(context, source->u.swizzle.object);
    case CGIR_EXPR_CONSTRUCT:
        argument = source->u.construct.arguments;
        break;
    case CGIR_EXPR_CAST:
        return HlslCollectIRCallsInExpr(context, source->u.cast.operand);
    case CGIR_EXPR_UNARY:
        return HlslCollectIRCallsInExpr(context, source->u.unary.operand);
    case CGIR_EXPR_BINARY:
        return HlslCollectIRCallsInExpr(context, source->u.binary.left) &&
               HlslCollectIRCallsInExpr(context, source->u.binary.right);
    case CGIR_EXPR_ASSIGN:
        return HlslCollectIRCallsInExpr(context, source->u.assign.target) &&
               HlslCollectIRCallsInExpr(context, source->u.assign.value);
    case CGIR_EXPR_CONDITIONAL:
        return HlslCollectIRCallsInExpr(context,
                   source->u.conditional.condition) &&
               HlslCollectIRCallsInExpr(context,
                   source->u.conditional.trueExpr) &&
               HlslCollectIRCallsInExpr(context,
                   source->u.conditional.falseExpr);
    case CGIR_EXPR_CALL:
        builtinStatus = HlslResolveBuiltinSymbol(context,
            source->u.call.callee, &source->loc, &builtin, &result,
            params, &paramCount);
        if (builtinStatus < 0 ||
            (builtinStatus == 0 && !HlslCollectHelper(context,
                source->u.call.callee, &source->loc)))
        {
            return 0;
        }
        argument = source->u.call.arguments;
        break;
    case CGIR_EXPR_INTERFACE_CALL:
        if (!HlslCollectIRCallsInExpr(context,
                                      source->u.interfaceCall.receiver))
        {
            return 0;
        }
        argument = source->u.interfaceCall.arguments;
        break;
    case CGIR_EXPR_INTRINSIC:
        argument = source->u.intrinsicCall.arguments;
        break;
    default:
        return 0;
    }
    for (; argument != NULL; argument = argument->next) {
        if (!HlslCollectIRCallsInExpr(context, argument))
            return 0;
    }
    return 1;
} // HlslCollectIRCallsInExpr

int HlslCollectIRCallsInStatements(HlslLowerContext *context,
                                           const CgIRStmt *source)
{
    const CgIRGeometryValue *value;

    for (; source != NULL; source = source->next) {
        context->statementLoc = source->loc;
        switch (source->kind) {
        case CGIR_STMT_BLOCK:
            if (!HlslCollectIRCallsInStatements(context, source->u.block))
                return 0;
            break;
        case CGIR_STMT_DECL:
            if (source->u.decl != NULL &&
                !HlslCollectIRCallsInExpr(context,
                                          source->u.decl->initializer))
            {
                return 0;
            }
            break;
        case CGIR_STMT_EXPR:
            if (!HlslCollectIRCallsInExpr(context, source->u.expression))
                return 0;
            break;
        case CGIR_STMT_IF:
            if (!HlslCollectIRCallsInExpr(context,
                    source->u.ifStmt.condition) ||
                !HlslCollectIRCallsInStatements(context,
                    source->u.ifStmt.trueBranch) ||
                !HlslCollectIRCallsInStatements(context,
                    source->u.ifStmt.falseBranch))
            {
                return 0;
            }
            break;
        case CGIR_STMT_WHILE:
        case CGIR_STMT_DO:
            if (!HlslCollectIRCallsInExpr(context,
                    source->u.loop.condition) ||
                !HlslCollectIRCallsInStatements(context,
                    source->u.loop.body))
            {
                return 0;
            }
            break;
        case CGIR_STMT_FOR:
            if (!HlslCollectIRCallsInStatements(context,
                    source->u.forStmt.init) ||
                !HlslCollectIRCallsInExpr(context,
                    source->u.forStmt.condition) ||
                !HlslCollectIRCallsInExpr(context,
                    source->u.forStmt.step) ||
                !HlslCollectIRCallsInStatements(context,
                    source->u.forStmt.body))
            {
                return 0;
            }
            break;
        case CGIR_STMT_RETURN:
            if (!HlslCollectIRCallsInExpr(context, source->u.returnExpr))
                return 0;
            break;
        case CGIR_STMT_DISCARD:
            if (!HlslCollectIRCallsInExpr(context,
                                          source->u.discard.condition))
            {
                return 0;
            }
            break;
        case CGIR_STMT_GEOMETRY_EMIT:
        case CGIR_STMT_GEOMETRY_FLAT:
            for (value = source->u.geometry.values; value != NULL;
                 value = value->next)
            {
                if (!HlslCollectIRCallsInExpr(context, value->value))
                    return 0;
            }
            break;
        case CGIR_STMT_GEOMETRY_RESTART:
        case CGIR_STMT_BREAK:
        case CGIR_STMT_CONTINUE:
            break;
        default:
            return 0;
        }
    }
    return 1;
} // HlslCollectIRCallsInStatements


static int HlslDeclListContains(const HlslDecl *declarations,
                                const HlslDecl *target)
{
    for (; declarations != NULL; declarations = declarations->next) {
        if (declarations == target)
            return 1;
    }
    return 0;
} // HlslDeclListContains

int HlslInitializeReturnedStructs(HlslModule *module,
                                         HlslFunction *function,
                                         HlslStmt *statements)
{
    HlslDecl *declaration;
    HlslExpr *zero;
    HlslExpr *initializer;

    for (; statements != NULL; statements = statements->next) {
        switch (statements->kind) {
        case HLSL_STMT_IF:
            if (!HlslInitializeReturnedStructs(module, function,
                    statements->u.ifStmt.trueBranch) ||
                !HlslInitializeReturnedStructs(module, function,
                    statements->u.ifStmt.falseBranch))
            {
                return 0;
            }
            break;
        case HLSL_STMT_WHILE:
        case HLSL_STMT_DO:
            if (!HlslInitializeReturnedStructs(module, function,
                                                statements->u.loop.body))
            {
                return 0;
            }
            break;
        case HLSL_STMT_FOR:
            if (!HlslInitializeReturnedStructs(module, function,
                    statements->u.forStmt.init) ||
                !HlslInitializeReturnedStructs(module, function,
                    statements->u.forStmt.body))
            {
                return 0;
            }
            break;
        case HLSL_STMT_BLOCK:
            if (!HlslInitializeReturnedStructs(module, function,
                                                statements->u.block))
            {
                return 0;
            }
            break;
        case HLSL_STMT_RETURN:
            if (statements->u.returnExpr == NULL ||
                statements->u.returnExpr->kind != HLSL_EXPR_SYMBOL)
            {
                break;
            }
            declaration = statements->u.returnExpr->u.symbol;
            if (declaration == NULL || declaration->initializer != NULL ||
                declaration->type.base != HLSL_BASE_STRUCT ||
                !HlslDeclListContains(function->locals, declaration))
            {
                break;
            }
            zero = HlslNewExpr(module, HLSL_EXPR_INT,
                               HlslNumericType(HLSL_BASE_INT, 1));
            initializer = HlslNewExpr(module, HLSL_EXPR_CAST,
                                      declaration->type);
            if (zero == NULL || initializer == NULL)
                return 0;
            zero->u.literalInt = 0;
            initializer->u.cast.expression = zero;
            declaration->initializer = initializer;
            break;
        default:
            break;
        }
    }
    return 1;
} // HlslInitializeReturnedStructs

int HlslLowerFunction(HlslLowerContext *context,
                             HlslFunction *function)
{
    const CgIRFunction *sourceFunction;
    Symbol *symbol;
    const char *name;

    symbol = (Symbol *) function->identity;
    name = symbol != NULL ? GetAtomString(atable, symbol->name) : NULL;
    context->function = function;
    context->statementLoc = symbol->loc;
    if (!HlslCollectParameters(context, symbol->details.fun.params, 0))
        return context->module->errors != 0 ? 0 :
            HlslLowerFailure(context, HLSL_ERROR_UNSUPPORTED_OPERATION,
                             name, &symbol->loc);
    if (symbol->details.fun.locals == NULL)
        return HlslLowerFailure(context, HLSL_ERROR_UNSUPPORTED_OPERATION,
                                name, &symbol->loc);
    if (!HlslCollectLocals(context, symbol->details.fun.locals->symbols))
        return context->module->errors != 0 ? 0 :
            HlslLowerFailure(context, HLSL_ERROR_UNSUPPORTED_OPERATION,
                             name, &symbol->loc);
    sourceFunction = HlslFindSourceIRFunction(context, symbol);
    if ((sourceFunction != NULL &&
         !HlslLowerIRStatements(context,
             sourceFunction->body != NULL &&
             sourceFunction->body->kind == CGIR_STMT_BLOCK &&
             sourceFunction->body->next == NULL ?
                 sourceFunction->body->u.block : sourceFunction->body,
                                &function->body)) ||
        (sourceFunction == NULL &&
         !HlslLowerStatements(context, symbol->details.fun.statements,
                              &function->body)))
    {
        return context->module->errors != 0 ? 0 :
            HlslLowerFailure(context, HLSL_ERROR_UNSUPPORTED_OPERATION,
                             name, &symbol->loc);
    }
    return HlslInitializeReturnedStructs(context->module, function,
                                         function->body);
} // HlslLowerFunction

Type *HlslOriginalEntryResult(Symbol *program)
{
    Type *result;
    const char *tag;
    stmt *statement;

    result = program->type->fun.rettype;
    if (GetCategory(result) != TYPE_CATEGORY_STRUCT)
        return result;
    tag = GetAtomString(atable, result->str.tag);
    if (tag == NULL || strcmp(tag, "$progret"))
        return result;
    statement = program->details.fun.statements;
    while (statement != NULL && statement->commonst.next != NULL)
        statement = statement->commonst.next;
    if (statement != NULL && statement->commonst.kind == RETURN_STMT &&
        statement->returnst.exp != NULL)
    {
        return statement->returnst.exp->common.type;
    }
    return result;
} // HlslOriginalEntryResult

int HlslIsEmptyEntry(Symbol *program)
{
    stmt *statement;

    if (program->details.fun.params != NULL)
        return 0;
    statement = program->details.fun.statements;
    while (statement != NULL && statement->commonst.kind == COMMENT_STMT)
        statement = statement->commonst.next;
    return IsVoid(program->type->fun.rettype) &&
           (statement == NULL ||
            (statement->commonst.kind == RETURN_STMT &&
             statement->returnst.exp == NULL &&
             statement->commonst.next == NULL));
} // HlslIsEmptyEntry

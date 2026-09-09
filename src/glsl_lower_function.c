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
// glsl_lower_function.c
//

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <float.h>
#include <limits.h>

#include "glsl_lower_internal.h"



static int GlslCollectCallsInExpr(GlslLowerContext *context, expr *source);
static int GlslIRCollectHelper(GlslLowerContext *context,
                               const Symbol *symbol);

static int GlslFunctionComesBefore(const GlslFunction *left,
                                   const GlslFunction *right)
{
    const Symbol *leftSymbol;
    const Symbol *rightSymbol;
    const char *leftName;
    const char *rightName;

    if (left->loc.file != right->loc.file)
        return left->loc.file < right->loc.file;
    if (left->loc.line != right->loc.line)
        return left->loc.line < right->loc.line;
    leftSymbol = (const Symbol *) left->identity;
    rightSymbol = (const Symbol *) right->identity;
    leftName = GetAtomString(atable, leftSymbol->name);
    rightName = GetAtomString(atable, rightSymbol->name);
    return strcmp(leftName, rightName) < 0;
}

static void GlslInsertFunction(GlslFunction **list, GlslFunction *function)
{
    GlslFunction **place;

    place = list;
    while (*place != NULL && !GlslFunctionComesBefore(function, *place))
        place = &(*place)->next;
    function->next = *place;
    *place = function;
}

int GlslCollectCallsInStatements(GlslLowerContext *context,
                                        stmt *source)
{
    for (; source != NULL; source = source->commonst.next) {
        context->statementLoc = source->commonst.loc;
        switch (source->commonst.kind) {
        case EXPR_STMT:
            if (!GlslCollectCallsInExpr(context, source->exprst.exp)) return 0;
            break;
        case IF_STMT:
            if (!GlslCollectCallsInExpr(context, source->ifst.cond) ||
                !GlslCollectCallsInStatements(context, source->ifst.thenstmt) ||
                !GlslCollectCallsInStatements(context, source->ifst.elsestmt)) return 0;
            break;
        case WHILE_STMT:
        case DO_STMT:
            if (!GlslCollectCallsInExpr(context, source->whilest.cond) ||
                !GlslCollectCallsInStatements(context, source->whilest.body)) return 0;
            break;
        case FOR_STMT:
            if (!GlslCollectCallsInStatements(context, source->forst.init) ||
                !GlslCollectCallsInExpr(context, source->forst.cond) ||
                !GlslCollectCallsInStatements(context, source->forst.step) ||
                !GlslCollectCallsInStatements(context, source->forst.body)) return 0;
            break;
        case BLOCK_STMT:
            if (!GlslCollectCallsInStatements(context, source->blockst.body)) return 0;
            break;
        case RETURN_STMT:
            if (!GlslCollectCallsInExpr(context, source->returnst.exp)) return 0;
            break;
        case DISCARD_STMT:
            if (!GlslCollectCallsInExpr(context, source->discardst.cond)) return 0;
            break;
        case COMMENT_STMT:
        case BREAK_STMT:
        case CONTINUE_STMT:
            break;
        default:
            return 0;
        }
    }
    return 1;
}

static int GlslCollectHelper(GlslLowerContext *context, Symbol *symbol)
{
    GlslFunction *function;
    GlslType result;
    const char *fileName;

    if (symbol == NULL || symbol->kind != FUNCTION_S)
        return 0;
    if (symbol->properties & SYMB_IS_BUILTIN)
        return 1;
    fileName = GetAtomString(atable, symbol->loc.file);
    if (fileName != NULL && !strcmp(fileName, "<stdlib>")) {
        GlslRecordFailure(context, "GLSL standard-library helper");
        return 0;
    }
    if (Cg->theHAL->IsTexobjBase(GetBase(symbol->type->fun.rettype))) {
        context->statementLoc = symbol->loc;
        GlslRecordFailureKind(context, GLSL_ERROR_SAMPLER,
                              "sampler helper result");
        return 0;
    }
    function = GlslFindFunction(context->module, symbol);
    if (function != NULL) {
        if (function->visitState == 1) {
            GlslRecordFailure(context, "recursive GLSL helper");
            return 0;
        }
        return 1;
    }
    if (!GlslEnsureTypeAt(context, symbol->type->fun.rettype,
                          &symbol->loc) ||
        !GlslEnsureParameterTypes(context, symbol->details.fun.params) ||
        symbol->details.fun.locals == NULL ||
        !GlslEnsureSymbolTypes(context,
                               symbol->details.fun.locals->symbols) ||
        !GlslLowerType(context, symbol->type->fun.rettype, &result,
                       &symbol->loc))
    {
        GlslRecordFailureKind(context, GLSL_ERROR_UNSUPPORTED_TYPE,
                              "GLSL helper type");
        return 0;
    }
    function = GlslNewFunction(context->module, result, NULL);
    if (function == NULL)
        return 0;
    function->identity = symbol;
    function->visitState = 1;
    if (symbol->details.fun.statements != NULL) {
        GlslSetLoc(&function->loc,
                   &symbol->details.fun.statements->commonst.loc);
    } else {
        GlslSetLoc(&function->loc, &symbol->loc);
    }
    GlslInsertFunction(&context->module->functions, function);
    if (!GlslCollectCallsInStatements(context,
                                      symbol->details.fun.statements))
    {
        return 0;
    }
    function->visitState = 2;
    return 1;
}

static int GlslCollectCallsInExpr(GlslLowerContext *context, expr *source)
{
    Symbol *symbol;

    if (source == NULL)
        return 1;
    switch (source->common.kind) {
    case DECL_N:
    case SYMB_N:
    case CONST_N:
        return 1;
    case UNARY_N:
        return GlslCollectCallsInExpr(context, source->un.arg);
    case BINARY_N:
        if (source->bin.op == FUN_CALL_OP && source->bin.left != NULL &&
            source->bin.left->common.kind == SYMB_N)
        {
            symbol = source->bin.left->sym.symbol;
            if (!GlslCollectHelper(context, symbol))
                return 0;
        }
        return GlslCollectCallsInExpr(context, source->bin.left) &&
               GlslCollectCallsInExpr(context, source->bin.right);
    case TRINARY_N:
        return GlslCollectCallsInExpr(context, source->tri.arg1) &&
               GlslCollectCallsInExpr(context, source->tri.arg2) &&
               GlslCollectCallsInExpr(context, source->tri.arg3);
    default:
        return 0;
    }
}

static int GlslMappedSignatureEqual(GlslLowerContext *context,
    const Symbol *left, const Symbol *right)
{
    Symbol *leftParam;
    Symbol *rightParam;
    GlslType leftType;
    GlslType rightType;

    leftParam = left->details.fun.params;
    rightParam = right->details.fun.params;
    while (leftParam != NULL && rightParam != NULL) {
        if (!GlslLowerTypeAuto(context, leftParam->type, &leftType,
                               &leftParam->loc) ||
            !GlslLowerTypeAuto(context, rightParam->type, &rightType,
                               &rightParam->loc) ||
            !GlslTypesEqual(&leftType, &rightType)) return 0;
        leftParam = leftParam->next;
        rightParam = rightParam->next;
    }
    return leftParam == NULL && rightParam == NULL;
}

static int GlslBuildSignature(GlslLowerContext *context,
    const Symbol *symbol, char *buffer, size_t size)
{
    Symbol *parameter;
    GlslType type;
    const char *typeName;
    size_t used;

    used = 0;
    buffer[0] = '\0';
    for (parameter = symbol->details.fun.params; parameter != NULL;
         parameter = parameter->next)
    {
        if (!GlslLowerTypeAuto(context, parameter->type, &type,
                               &parameter->loc)) return 0;
        typeName = GlslTypeName(&type);
        if (typeName == NULL || used + strlen(typeName) + 2 > size) return 0;
        if (used != 0)
            buffer[used++] = '_';
        strcpy(buffer + used, typeName);
        used += strlen(typeName);
    }
    if (used == 0) {
        if (size < 5) return 0;
        strcpy(buffer, "void");
    }
    return 1;
}

int GlslAssignHelperNames(GlslLowerContext *context)
{
    GlslFunction *function;
    GlslFunction *previous;
    GlslFunction *sameName;
    GlslFunction *sameSignature;
    Symbol *symbol;
    Symbol *previousSymbol;
    const char *sourceName;
    char signature[256];
    char candidate[512];
    int collapsedIndex;

    for (function = context->module->functions; function != NULL;
         function = function->next)
    {
        symbol = (Symbol *) function->identity;
        sourceName = GetAtomString(atable, symbol->name);
        sameName = NULL;
        sameSignature = NULL;
        collapsedIndex = 0;
        for (previous = context->module->functions; previous != function;
             previous = previous->next)
        {
            previousSymbol = (Symbol *) previous->identity;
            if (previousSymbol->name == symbol->name) {
                if (sameName == NULL)
                    sameName = previous;
                if (GlslMappedSignatureEqual(context, previousSymbol,
                                             symbol))
                {
                    sameSignature = previous;
                    collapsedIndex++;
                }
            }
        }
        if (sameName == NULL) {
            function->name = GlslAllocateSymbolNameForSource(context,
                symbol, sourceName, &symbol->loc);
        } else if (sameSignature == NULL) {
            function->name = sameName->name;
        } else {
            if (!GlslBuildSignature(context, symbol, signature,
                                    sizeof(signature)) ||
                strlen(sourceName) + strlen(signature) + 24 >
                    sizeof(candidate)) return 0;
            sprintf(candidate, "%s_%s_%d", sourceName, signature,
                    collapsedIndex);
            function->name = GlslAllocateSymbolNameForSource(context,
                symbol, candidate, &symbol->loc);
        }
        if (function->name == NULL)
            return 0;
    }
    return 1;
}

static int GlslFunctionIsAfter(const GlslModule *module,
    const GlslFunction *caller, const GlslFunction *callee)
{
    const GlslFunction *function;
    int sawCaller;

    sawCaller = 0;
    for (function = module->functions; function != NULL;
         function = function->next)
    {
        if (function == caller)
            sawCaller = 1;
        if (function == callee)
            return sawCaller;
    }
    return 0;
}

static void GlslMarkForwardCallsInExpr(GlslLowerContext *context,
    GlslFunction *caller, expr *source)
{
    GlslFunction *callee;

    if (source == NULL)
        return;
    switch (source->common.kind) {
    case UNARY_N:
        GlslMarkForwardCallsInExpr(context, caller, source->un.arg);
        break;
    case BINARY_N:
        if (source->bin.op == FUN_CALL_OP && source->bin.left != NULL &&
            source->bin.left->common.kind == SYMB_N)
        {
            callee = GlslFindFunction(context->module,
                                      source->bin.left->sym.symbol);
            if (callee != NULL &&
                GlslFunctionIsAfter(context->module, caller, callee))
            {
                callee->needsPrototype = 1;
            }
        }
        GlslMarkForwardCallsInExpr(context, caller, source->bin.left);
        GlslMarkForwardCallsInExpr(context, caller, source->bin.right);
        break;
    case TRINARY_N:
        GlslMarkForwardCallsInExpr(context, caller, source->tri.arg1);
        GlslMarkForwardCallsInExpr(context, caller, source->tri.arg2);
        GlslMarkForwardCallsInExpr(context, caller, source->tri.arg3);
        break;
    default:
        break;
    }
}

static void GlslMarkForwardCallsInStatements(GlslLowerContext *context,
    GlslFunction *caller, stmt *source)
{
    for (; source != NULL; source = source->commonst.next) {
        switch (source->commonst.kind) {
        case EXPR_STMT:
            GlslMarkForwardCallsInExpr(context, caller, source->exprst.exp);
            break;
        case IF_STMT:
            GlslMarkForwardCallsInExpr(context, caller, source->ifst.cond);
            GlslMarkForwardCallsInStatements(context, caller,
                                              source->ifst.thenstmt);
            GlslMarkForwardCallsInStatements(context, caller,
                                              source->ifst.elsestmt);
            break;
        case WHILE_STMT:
        case DO_STMT:
            GlslMarkForwardCallsInExpr(context, caller, source->whilest.cond);
            GlslMarkForwardCallsInStatements(context, caller,
                                              source->whilest.body);
            break;
        case FOR_STMT:
            GlslMarkForwardCallsInStatements(context, caller,
                                              source->forst.init);
            GlslMarkForwardCallsInExpr(context, caller, source->forst.cond);
            GlslMarkForwardCallsInStatements(context, caller,
                                              source->forst.step);
            GlslMarkForwardCallsInStatements(context, caller,
                                              source->forst.body);
            break;
        case BLOCK_STMT:
            GlslMarkForwardCallsInStatements(context, caller,
                                              source->blockst.body);
            break;
        case RETURN_STMT:
            GlslMarkForwardCallsInExpr(context, caller, source->returnst.exp);
            break;
        case DISCARD_STMT:
            GlslMarkForwardCallsInExpr(context, caller,
                                       source->discardst.cond);
            break;
        default:
            break;
        }
    }
}

void GlslMarkForwardCalls(GlslLowerContext *context)
{
    GlslFunction *function;
    Symbol *symbol;

    for (function = context->module->functions; function != NULL;
         function = function->next)
    {
        symbol = (Symbol *) function->identity;
        GlslMarkForwardCallsInStatements(context, function,
                                         symbol->details.fun.statements);
    }
}


int GlslLowerHelper(GlslLowerContext *context,
                           GlslFunction *function)
{
    Symbol *symbol;

    symbol = (Symbol *) function->identity;
    context->function = function;
    context->statementLoc = symbol->loc;
    if (!GlslCollectParameters(context, symbol->details.fun.params, 0) ||
        !GlslCollectLocals(context, symbol->details.fun.locals->symbols, 0) ||
        !GlslLowerStatementList(context, symbol->details.fun.statements,
                                &function->body)) return 0;
    return 1;
}

/*
 * GlslIRFindIRFunction() - The Cg IR definition matching a resolved
 *          callee identity.
 */

const CgIRFunction *GlslIRFindIRFunction(const CgIRModule *source,
                                                const Symbol *symbol)
{
    const CgIRFunction *function;

    for (function = source->functions; function != NULL;
         function = function->next)
    {
        if (function->symbol == symbol)
            return function;
    }
    return NULL;
} // GlslIRFindIRFunction

static int GlslIRCollectCallsInExpr(GlslLowerContext *context,
                                    const CgIRExpr *expr)
{
    const CgIRExpr *argument;

    if (expr == NULL)
        return 1;
    switch (expr->kind) {
    case CGIR_EXPR_CALL:
        if (!GlslIRCollectHelper(context, expr->u.call.callee)) return 0;
        for (argument = expr->u.call.arguments; argument != NULL;
             argument = argument->next)
        {
            if (!GlslIRCollectCallsInExpr(context, argument)) return 0;
        }
        return 1;
    case CGIR_EXPR_MEMBER:
        return GlslIRCollectCallsInExpr(context, expr->u.member.object);
    case CGIR_EXPR_INDEX:
        return GlslIRCollectCallsInExpr(context,
                                        expr->u.index.object) &&
               GlslIRCollectCallsInExpr(context, expr->u.index.index);
    case CGIR_EXPR_LENGTH:
        return GlslIRCollectCallsInExpr(context, expr->u.length.object);
    case CGIR_EXPR_SWIZZLE:
        return GlslIRCollectCallsInExpr(context, expr->u.swizzle.object);
    case CGIR_EXPR_CONSTRUCT:
        for (argument = expr->u.construct.arguments; argument != NULL;
             argument = argument->next)
        {
            if (!GlslIRCollectCallsInExpr(context, argument)) return 0;
        }
        return 1;
    case CGIR_EXPR_CAST:
        return GlslIRCollectCallsInExpr(context, expr->u.cast.operand);
    case CGIR_EXPR_UNARY:
        return GlslIRCollectCallsInExpr(context, expr->u.unary.operand);
    case CGIR_EXPR_BINARY:
        return GlslIRCollectCallsInExpr(context, expr->u.binary.left) &&
               GlslIRCollectCallsInExpr(context, expr->u.binary.right);
    case CGIR_EXPR_ASSIGN:
        return GlslIRCollectCallsInExpr(context,
                                        expr->u.assign.target) &&
               GlslIRCollectCallsInExpr(context, expr->u.assign.value);
    case CGIR_EXPR_CONDITIONAL:
        return GlslIRCollectCallsInExpr(context,
                                        expr->u.conditional.condition) &&
               GlslIRCollectCallsInExpr(context,
                                        expr->u.conditional.trueExpr) &&
               GlslIRCollectCallsInExpr(context,
                                        expr->u.conditional.falseExpr);
    case CGIR_EXPR_INTERFACE_CALL:
        if (!GlslIRCollectCallsInExpr(context,
                                      expr->u.interfaceCall.receiver))
            return 0;
        for (argument = expr->u.interfaceCall.arguments; argument != NULL;
             argument = argument->next)
        {
            if (!GlslIRCollectCallsInExpr(context, argument)) return 0;
        }
        return 1;
    case CGIR_EXPR_INTRINSIC:
        for (argument = expr->u.intrinsicCall.arguments; argument != NULL;
             argument = argument->next)
        {
            if (!GlslIRCollectCallsInExpr(context, argument)) return 0;
        }
        return 1;
    default:
        return 1;
    }
} // GlslIRCollectCallsInExpr

int GlslIRCollectCallsInStmt(GlslLowerContext *context,
                                    const CgIRStmt *stmt)
{
    if (stmt == NULL)
        return 1;
    switch (stmt->kind) {
    case CGIR_STMT_BLOCK:
        for (stmt = stmt->u.block; stmt != NULL; stmt = stmt->next) {
            if (!GlslIRCollectCallsInStmt(context, stmt)) return 0;
        }
        return 1;
    case CGIR_STMT_DECL:
        return stmt->u.decl->initializer == NULL ||
               GlslIRCollectCallsInExpr(context,
                                        stmt->u.decl->initializer);
    case CGIR_STMT_EXPR:
        return GlslIRCollectCallsInExpr(context, stmt->u.expression);
    case CGIR_STMT_IF:
        return GlslIRCollectCallsInExpr(context,
                                        stmt->u.ifStmt.condition) &&
               GlslIRCollectCallsInStmt(context,
                                        stmt->u.ifStmt.trueBranch) &&
               GlslIRCollectCallsInStmt(context,
                                        stmt->u.ifStmt.falseBranch);
    case CGIR_STMT_WHILE:
    case CGIR_STMT_DO:
        return GlslIRCollectCallsInExpr(context,
                                        stmt->u.loop.condition) &&
               GlslIRCollectCallsInStmt(context, stmt->u.loop.body);
    case CGIR_STMT_FOR:
        return GlslIRCollectCallsInStmt(context, stmt->u.forStmt.init) &&
               (stmt->u.forStmt.condition == NULL ||
                GlslIRCollectCallsInExpr(context,
                                         stmt->u.forStmt.condition)) &&
               (stmt->u.forStmt.step == NULL ||
                GlslIRCollectCallsInExpr(context, stmt->u.forStmt.step)) &&
               GlslIRCollectCallsInStmt(context, stmt->u.forStmt.body);
    case CGIR_STMT_RETURN:
        return stmt->u.returnExpr == NULL ||
               GlslIRCollectCallsInExpr(context, stmt->u.returnExpr);
    case CGIR_STMT_DISCARD:
        return stmt->u.discard.condition == NULL ||
               GlslIRCollectCallsInExpr(context,
                                        stmt->u.discard.condition);
    case CGIR_STMT_GEOMETRY_EMIT:
    case CGIR_STMT_GEOMETRY_FLAT:
        {
            const CgIRGeometryValue *value;

            for (value = stmt->u.geometry.values; value != NULL;
                 value = value->next)
            {
                if (!GlslIRCollectCallsInExpr(context, value->value))
                    return 0;
            }
        }
        return 1;
    default:
        return 1;
    }
} // GlslIRCollectCallsInStmt

/*
 * GlslIRFirstBodyLoc() - Helper functions sort by their first executable
 *          statement location, mirroring the legacy collection that read
 *      the raw statement list head (declarations are not statements).
 */

static const SourceLoc *GlslIRFirstBodyLoc(const CgIRFunction *function,
                                           const Symbol *symbol)
{
    const CgIRStmt *first;

    first = function->body;
    if (first != NULL && first->kind == CGIR_STMT_BLOCK)
        first = first->u.block;
    for (; first != NULL; first = first->next) {
        if (first->kind != CGIR_STMT_DECL)
            return &first->loc;
    }
    return &symbol->loc;
} // GlslIRFirstBodyLoc

/*
 * GlslIRCollectHelper() - IR-driven twin of the legacy helper walk:
 *          resolve identity, reject standard-library and sampler-result
 *          helpers, ensure every mentioned type, then recurse into the
 *          body depth-first with a cycle guard.
 */

static int GlslIRCollectHelper(GlslLowerContext *context,
                               const Symbol *symbol)
{
    const CgIRFunction *irFunction;
    const CgIRDecl *decl;
    GlslFunction *function;
    GlslType result;
    const char *fileName;

    if (symbol == NULL || symbol->kind != FUNCTION_S)
        return 0;
    if (symbol->properties & SYMB_IS_BUILTIN)
        return 1;
    fileName = GetAtomString(atable, symbol->loc.file);
    if (fileName != NULL && !strcmp(fileName, "<stdlib>")) {
        GlslRecordFailure(context, "GLSL standard-library helper");
        return 0;
    }
    if (symbol->type == NULL ||
        GlslIRIsSamplerValue(symbol->type->fun.rettype))
    {
        context->statementLoc = symbol->loc;
        GlslRecordFailureKind(context, GLSL_ERROR_SAMPLER,
                              "sampler helper result");
        return 0;
    }
    function = GlslFindFunction(context->module, symbol);
    if (function != NULL) {
        if (function->visitState == 1) {
            GlslRecordFailure(context, "recursive GLSL helper");
            return 0;
        }
        return 1;
    }
    irFunction = GlslIRFindIRFunction(context->source, symbol);
    if (irFunction == NULL)
        return 0;
    if (!GlslIREnsureTypeAt(context, symbol->type->fun.rettype,
                            &symbol->loc))
    {
        GlslRecordFailureKind(context, GLSL_ERROR_UNSUPPORTED_TYPE,
                              "GLSL helper type");
        return 0;
    }
    for (decl = irFunction->parameters; decl != NULL; decl = decl->next) {
        if (!GlslIRSamplerPlacementCheck(context, decl) ||
            !GlslIREnsureTypeAt(context, decl->type, &decl->loc))
        {
            GlslRecordFailureKind(context, GLSL_ERROR_UNSUPPORTED_TYPE,
                                  "GLSL helper type");
            return 0;
        }
    }
    for (decl = irFunction->locals; decl != NULL; decl = decl->next) {
        const char *localName;

        localName = GetAtomString(atable, decl->name);
        if (localName == NULL || localName[0] == '$')
            continue;
        if (!GlslIRSamplerPlacementCheck(context, decl) ||
            !GlslIREnsureTypeAt(context, decl->type, &decl->loc))
        {
            GlslRecordFailureKind(context, GLSL_ERROR_UNSUPPORTED_TYPE,
                                  "GLSL helper type");
            return 0;
        }
    }
    if (!GlslIRType(context, symbol->type->fun.rettype, &result,
                    &symbol->loc))
    {
        GlslRecordFailureKind(context, GLSL_ERROR_UNSUPPORTED_TYPE,
                              "GLSL helper type");
        return 0;
    }
    function = GlslNewFunction(context->module, result, NULL);
    if (function == NULL)
        return 0;
    function->identity = symbol;
    function->visitState = 1;
    GlslSetLoc(&function->loc, GlslIRFirstBodyLoc(irFunction, symbol));
    GlslInsertFunction(&context->module->functions, function);
    if (!GlslIRCollectCallsInStmt(context, irFunction->body)) return 0;
    function->visitState = 2;
    return 1;
} // GlslIRCollectHelper

/*
 * GlslIRMarkForwardCalls* - Calls whose target sorts after its caller
 *          need a prototype declaration.
 */

static void GlslIRMarkForwardCallsInExpr(GlslLowerContext *context,
                                         GlslFunction *caller,
                                         const CgIRExpr *expr)
{
    const CgIRExpr *argument;
    GlslFunction *callee;

    if (expr == NULL)
        return;
    switch (expr->kind) {
    case CGIR_EXPR_CALL:
        callee = GlslFindFunction(context->module, expr->u.call.callee);
        if (callee != NULL &&
            GlslFunctionIsAfter(context->module, caller, callee))
        {
            callee->needsPrototype = 1;
        }
        for (argument = expr->u.call.arguments; argument != NULL;
             argument = argument->next)
        {
            GlslIRMarkForwardCallsInExpr(context, caller, argument);
        }
        break;
    case CGIR_EXPR_MEMBER:
        GlslIRMarkForwardCallsInExpr(context, caller,
                                     expr->u.member.object);
        break;
    case CGIR_EXPR_INDEX:
        GlslIRMarkForwardCallsInExpr(context, caller,
                                     expr->u.index.object);
        GlslIRMarkForwardCallsInExpr(context, caller,
                                     expr->u.index.index);
        break;
    case CGIR_EXPR_LENGTH:
        GlslIRMarkForwardCallsInExpr(context, caller,
                                     expr->u.length.object);
        break;
    case CGIR_EXPR_SWIZZLE:
        GlslIRMarkForwardCallsInExpr(context, caller,
                                     expr->u.swizzle.object);
        break;
    case CGIR_EXPR_CONSTRUCT:
        for (argument = expr->u.construct.arguments; argument != NULL;
             argument = argument->next)
        {
            GlslIRMarkForwardCallsInExpr(context, caller, argument);
        }
        break;
    case CGIR_EXPR_CAST:
        GlslIRMarkForwardCallsInExpr(context, caller, expr->u.cast.operand);
        break;
    case CGIR_EXPR_UNARY:
        GlslIRMarkForwardCallsInExpr(context, caller,
                                     expr->u.unary.operand);
        break;
    case CGIR_EXPR_BINARY:
        GlslIRMarkForwardCallsInExpr(context, caller, expr->u.binary.left);
        GlslIRMarkForwardCallsInExpr(context, caller, expr->u.binary.right);
        break;
    case CGIR_EXPR_ASSIGN:
        GlslIRMarkForwardCallsInExpr(context, caller,
                                     expr->u.assign.target);
        GlslIRMarkForwardCallsInExpr(context, caller,
                                     expr->u.assign.value);
        break;
    case CGIR_EXPR_CONDITIONAL:
        GlslIRMarkForwardCallsInExpr(context, caller,
                                     expr->u.conditional.condition);
        GlslIRMarkForwardCallsInExpr(context, caller,
                                     expr->u.conditional.trueExpr);
        GlslIRMarkForwardCallsInExpr(context, caller,
                                     expr->u.conditional.falseExpr);
        break;
    case CGIR_EXPR_INTERFACE_CALL:
        GlslIRMarkForwardCallsInExpr(context, caller,
                                     expr->u.interfaceCall.receiver);
        for (argument = expr->u.interfaceCall.arguments; argument != NULL;
             argument = argument->next)
        {
            GlslIRMarkForwardCallsInExpr(context, caller, argument);
        }
        break;
    case CGIR_EXPR_INTRINSIC:
        for (argument = expr->u.intrinsicCall.arguments; argument != NULL;
             argument = argument->next)
        {
            GlslIRMarkForwardCallsInExpr(context, caller, argument);
        }
        break;
    default:
        break;
    }
} // GlslIRMarkForwardCallsInExpr

void GlslIRMarkForwardCallsInStmt(GlslLowerContext *context,
                                         GlslFunction *caller,
                                         const CgIRStmt *stmt)
{
    if (stmt == NULL)
        return;
    switch (stmt->kind) {
    case CGIR_STMT_BLOCK:
        for (stmt = stmt->u.block; stmt != NULL; stmt = stmt->next) {
            GlslIRMarkForwardCallsInStmt(context, caller, stmt);
        }
        break;
    case CGIR_STMT_DECL:
        if (stmt->u.decl->initializer != NULL) {
            GlslIRMarkForwardCallsInExpr(context, caller,
                                         stmt->u.decl->initializer);
        }
        break;
    case CGIR_STMT_EXPR:
        GlslIRMarkForwardCallsInExpr(context, caller, stmt->u.expression);
        break;
    case CGIR_STMT_IF:
        GlslIRMarkForwardCallsInExpr(context, caller,
                                     stmt->u.ifStmt.condition);
        GlslIRMarkForwardCallsInStmt(context, caller,
                                     stmt->u.ifStmt.trueBranch);
        GlslIRMarkForwardCallsInStmt(context, caller,
                                     stmt->u.ifStmt.falseBranch);
        break;
    case CGIR_STMT_WHILE:
    case CGIR_STMT_DO:
        GlslIRMarkForwardCallsInExpr(context, caller,
                                     stmt->u.loop.condition);
        GlslIRMarkForwardCallsInStmt(context, caller, stmt->u.loop.body);
        break;
    case CGIR_STMT_FOR:
        GlslIRMarkForwardCallsInStmt(context, caller, stmt->u.forStmt.init);
        GlslIRMarkForwardCallsInExpr(context, caller,
                                     stmt->u.forStmt.condition);
        GlslIRMarkForwardCallsInExpr(context, caller, stmt->u.forStmt.step);
        GlslIRMarkForwardCallsInStmt(context, caller, stmt->u.forStmt.body);
        break;
    case CGIR_STMT_RETURN:
        GlslIRMarkForwardCallsInExpr(context, caller, stmt->u.returnExpr);
        break;
    case CGIR_STMT_DISCARD:
        GlslIRMarkForwardCallsInExpr(context, caller,
                                     stmt->u.discard.condition);
        break;
    case CGIR_STMT_GEOMETRY_EMIT:
    case CGIR_STMT_GEOMETRY_FLAT:
        {
            const CgIRGeometryValue *value;

            for (value = stmt->u.geometry.values; value != NULL;
                 value = value->next)
            {
                GlslIRMarkForwardCallsInExpr(context, caller,
                                             value->value);
            }
        }
        break;
    default:
        break;
    }
} // GlslIRMarkForwardCallsInStmt

/*
 * GlslIRLowerFunctionBody() - Parameters, then the structured body.
 *          Entry uniform formals skip the local list: they surface
 *          through the HAL uniform scan exactly as in the legacy path.
 */

int GlslIRLowerFunctionBody(GlslLowerContext *context,
                                   const CgIRFunction *irFunction)
{
    GlslFunction *function;
    const CgIRDecl *param;
    int isEntry;

    function = context->function;
    isEntry = irFunction->isEntry;
    for (param = irFunction->parameters; param != NULL;
         param = param->next)
    {
        GlslDecl *decl;
        GlslType type;
        const char *sourceName;
        const char *name;
        int qualifiers;
        const void *identity;
        const void *nameSpace;

        if (isEntry && param->domain == CGIR_DOMAIN_UNIFORM)
            continue;
        if (isEntry && context->module->stage == GLSL_STAGE_GEOMETRY &&
            param->semantic != 0)
        {
            if (!GlslIRRegisterGeometryInput(context, param)) {
                if (context->module->errorReason == NULL)
                    GlslRecordFailure(context,
                                      "geometry input interface");
                return 0;
            }
            continue;
        }
        if (isEntry && CgIsAttribArray(param->type))
            continue;
        if (!GlslIRSamplerPlacementCheck(context, param))
            return 0;
        if (!GlslIREnsureTypeAt(context, param->type, &param->loc))
            return 0;
        if (!GlslIRType(context, param->type, &type, &param->loc))
            return 0;
        sourceName = GetAtomString(atable, param->name);
        if (sourceName == NULL)
            return 0;
        identity = param->symbol != NULL ? (const void *) param->symbol
                                         : (const void *) param->type;
        nameSpace = isEntry ? NULL : (const void *) function;
        if (nameSpace != NULL) {
            name = GlslAllocateScopedSymbolNameForSource(context,
                nameSpace, identity, sourceName, &param->loc);
        } else {
            name = GlslAllocateSymbolNameForSource(context, identity,
                                                   sourceName,
                                                   &param->loc);
        }
        if (name == NULL)
            return 0;
        decl = GlslNewDecl(context->module, GLSL_STORAGE_NONE, type, name);
        if (decl == NULL)
            return 0;
        decl->identity = identity;
        GlslSetLoc(&decl->loc, &param->loc);
        decl->sourceOrdinal = param->symbol != NULL ?
                              param->symbol->sourceOrdinal : 0;
        if (isEntry) {
            GlslAppendDecl(&function->locals, decl);
        } else {
            qualifiers = GetQualifiers(param->type);
            if ((qualifiers & TYPE_QUALIFIER_INOUT) ==
                TYPE_QUALIFIER_INOUT)
            {
                decl->parameterQualifier = GLSL_PARAMETER_INOUT;
            } else if (qualifiers & TYPE_QUALIFIER_OUT) {
                decl->parameterQualifier = GLSL_PARAMETER_OUT;
            }
            GlslAppendDecl(&function->parameters, decl);
        }
    }
    /* The body block's declarations register as locals while its
     * statements lower. */
    return irFunction->body == NULL ||
           irFunction->body->kind != CGIR_STMT_BLOCK ||
           GlslIRBlockBody(context, irFunction->body->u.block,
                           &function->body);
} // GlslIRLowerFunctionBody

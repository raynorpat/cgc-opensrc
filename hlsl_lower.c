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
// hlsl_lower.c
//

#include <float.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>

#include "slglobals.h"
#include "cg_stdlib.h"
#include "cg_ir.h"
#include "hlsl_hal.h"
#include "hlsl_lower_internal.h"

static HlslDecl *HlslGeometryOutputMember(HlslLowerContext *context,
                                           int semantic)
{
    HlslDecl *member;
    const char *canonical;

    if (context == NULL || context->module->geometryOutputStruct == NULL)
        return NULL;
    canonical = GetAtomString(atable, semantic);
    if (canonical == NULL)
        return NULL;
    for (member = context->module->geometryOutputStruct->members;
         member != NULL; member = member->next)
    {
        if (member->publicName != NULL &&
            !strcmp(member->publicName, canonical))
        {
            return member;
        }
    }
    return NULL;
} // HlslGeometryOutputMember

static HlslFlatReplay *HlslGeometryFlatState(HlslLowerContext *context,
                                              HlslDecl *target)
{
    HlslFlatReplay *state;

    if (context == NULL || context->function == NULL)
        return NULL;
    for (state = context->function->geometryFlatState; state != NULL;
         state = state->next)
    {
        if (state->target == target)
            return state;
    }
    return NULL;
} // HlslGeometryFlatState

static HlslExpr *HlslGeometryMemberExpr(HlslLowerContext *context,
                                         HlslDecl *object,
                                         HlslDecl *member)
{
    HlslExpr *expression;

    if (object == NULL || member == NULL)
        return NULL;
    expression = HlslNewSourceExpr(context, HLSL_EXPR_MEMBER,
                                   member->type);
    if (expression != NULL) {
        expression->u.member.object = HlslNewSymbolExpr(context, object);
        expression->u.member.decl = member;
        expression->u.member.name = member->name;
        if (expression->u.member.object == NULL)
            return NULL;
    }
    return expression;
} // HlslGeometryMemberExpr

static HlslExpr *HlslGeometryConvert(HlslLowerContext *context,
                                      HlslExpr *value,
                                      const HlslType *type)
{
    HlslExpr *conversion;

    if (value == NULL || type == NULL)
        return NULL;
    if (value->type.base == type->base && value->type.len == type->len &&
        value->type.rows == type->rows && value->type.cols == type->cols &&
        value->type.arraySize == type->arraySize)
    {
        return value;
    }
    conversion = HlslNewSourceExpr(context, HLSL_EXPR_CAST, *type);
    if (conversion != NULL)
        conversion->u.cast.expression = value;
    return conversion;
} // HlslGeometryConvert

static const CgIRExpr *HlslGeometryValueRoot(const CgIRExpr *value)
{
    while (value != NULL && value->kind == CGIR_EXPR_MEMBER)
        value = value->u.member.object;
    return value;
} // HlslGeometryValueRoot

static HlslExpr *HlslGeometryCapturedMember(
    HlslLowerContext *context, const CgIRExpr *value,
    const CgIRExpr *root,
    HlslDecl *capturedRoot)
{
    HlslExpr *target;
    HlslExpr *object;
    HlslDecl *member;
    HlslType type;

    if (context == NULL || value == NULL || root == NULL ||
        capturedRoot == NULL)
    {
        return NULL;
    }
    if (value == root)
        return HlslNewSymbolExpr(context, capturedRoot);
    if (value->kind != CGIR_EXPR_MEMBER ||
        value->u.member.object == NULL ||
        value->u.member.member == NULL ||
        !HlslLowerType(context, value->type, &type, &value->loc))
    {
        return NULL;
    }
    object = HlslGeometryCapturedMember(context, value->u.member.object,
                                        root, capturedRoot);
    member = HlslFindDecl(context, value->u.member.member);
    target = object != NULL && member != NULL ?
        HlslNewSourceExpr(context, HLSL_EXPR_MEMBER, type) : NULL;
    if (target != NULL) {
        target->u.member.object = object;
        target->u.member.decl = member;
        target->u.member.name = member->name;
    }
    return target;
} // HlslGeometryCapturedMember

HlslStmt *HlslLowerGeometryOperation(
    HlslLowerContext *context, const CgIRStmt *operation)
{
    const CgIRGeometryValue *value;
    HlslFlatReplay *state;
    HlslDecl *target;
    HlslExpr *lowered;
    HlslExpr *left;
    HlslExpr *assignment;
    HlslExpr *record;
    HlslExpr *zero;
    HlslStmt *statement;
    HlslStmt *body;
    HlslDecl **targets;
    HlslExpr **captured;
    HlslExpr **rootCaptures;
    const CgIRExpr **roots;
    HlslLoc loc;
    int valueCount;
    int valueIndex;
    int rootIndex;
    int rootUses;

    if (context == NULL || operation == NULL || context->function == NULL ||
        !context->function->geometryEffect ||
        context->function->geometryStream == NULL ||
        context->function->geometryOutputRecord == NULL)
    {
        return NULL;
    }
    HlslSetLoc(&loc, &operation->loc);
    if (operation->kind == CGIR_STMT_GEOMETRY_RESTART)
        return HlslNewRestartStrip(context->module, loc);

    valueCount = 0;
    for (value = operation->u.geometry.values; value != NULL;
         value = value->next)
    {
        valueCount++;
    }
    if (valueCount <= 0 ||
        (size_t) valueCount > (size_t) -1 / sizeof(HlslDecl *) ||
        (size_t) valueCount > (size_t) -1 / sizeof(HlslExpr *))
    {
        return NULL;
    }
    targets = (HlslDecl **) HlslLowerAlloc(context,
        (size_t) valueCount * sizeof(HlslDecl *));
    captured = (HlslExpr **) HlslLowerAlloc(context,
        (size_t) valueCount * sizeof(HlslExpr *));
    rootCaptures = (HlslExpr **) HlslLowerAlloc(context,
        (size_t) valueCount * sizeof(HlslExpr *));
    roots = (const CgIRExpr **) HlslLowerAlloc(context,
        (size_t) valueCount * sizeof(CgIRExpr *));
    if (targets == NULL || captured == NULL || rootCaptures == NULL ||
        roots == NULL)
    {
        return NULL;
    }

    valueIndex = 0;
    for (value = operation->u.geometry.values; value != NULL;
         value = value->next)
    {
        roots[valueIndex] = HlslGeometryValueRoot(value->value);
        valueIndex++;
    }

    body = NULL;
    valueIndex = 0;
    for (value = operation->u.geometry.values; value != NULL;
         value = value->next)
    {
        target = HlslGeometryOutputMember(context,
                                           value->canonicalSemantic);
        if (target == NULL)
            return NULL;
        context->statementLoc = value->loc;
        rootUses = 0;
        for (rootIndex = 0; rootIndex < valueCount; rootIndex++) {
            if (roots[rootIndex] == roots[valueIndex])
                rootUses++;
        }
        if (rootUses > 1) {
            rootIndex = 0;
            while (rootIndex < valueIndex &&
                   roots[rootIndex] != roots[valueIndex])
            {
                rootIndex++;
            }
            if (rootIndex == valueIndex) {
                lowered = HlslLowerIRExpr(context, roots[valueIndex],
                                          &body, HLSL_VALUE_RVALUE);
                rootCaptures[valueIndex] = lowered != NULL ?
                    HlslCaptureValue(context, &body, lowered) : NULL;
            } else {
                rootCaptures[valueIndex] = rootCaptures[rootIndex];
            }
            lowered = rootCaptures[valueIndex] != NULL &&
                      rootCaptures[valueIndex]->kind == HLSL_EXPR_SYMBOL ?
                HlslGeometryCapturedMember(context, value->value,
                    roots[valueIndex], rootCaptures[valueIndex]->u.symbol) :
                NULL;
        } else {
            lowered = HlslLowerIRExpr(context, value->value, &body,
                                      HLSL_VALUE_RVALUE);
        }
        if (lowered == NULL)
            return NULL;
        lowered = HlslGeometryConvert(context, lowered, &target->type);
        captured[valueIndex] = lowered != NULL ?
            HlslCaptureValue(context, &body, lowered) : NULL;
        targets[valueIndex] = target;
        if (captured[valueIndex] == NULL)
            return NULL;
        valueIndex++;
    }
    context->statementLoc = operation->loc;
    if (operation->kind == CGIR_STMT_GEOMETRY_EMIT) {
        left = HlslNewSymbolExpr(context,
            context->function->geometryOutputRecord);
        zero = HlslNewLiteral(context, HLSL_BASE_INT, 0, 0.0f);
        record = HlslNewSourceExpr(context, HLSL_EXPR_CAST,
            context->function->geometryOutputRecord->type);
        if (record != NULL)
            record->u.cast.expression = zero;
        assignment = HlslNewAssignment(context, left, record);
        statement = HlslNewExpressionStmt(context, assignment);
        if (statement == NULL)
            return NULL;
        HlslAppendStmt(&body, statement);
    }
    for (valueIndex = 0; valueIndex < valueCount; valueIndex++) {
        target = targets[valueIndex];
        if (operation->kind == CGIR_STMT_GEOMETRY_FLAT) {
            state = HlslGeometryFlatState(context, target);
            if (state == NULL)
                return NULL;
            left = HlslNewSymbolExpr(context, state->shadow);
        } else {
            left = HlslGeometryMemberExpr(context,
                context->function->geometryOutputRecord, target);
        }
        assignment = HlslNewAssignment(context, left,
                                        captured[valueIndex]);
        statement = HlslNewExpressionStmt(context, assignment);
        if (statement == NULL)
            return NULL;
        HlslAppendStmt(&body, statement);
        if (operation->kind == CGIR_STMT_GEOMETRY_FLAT) {
            statement = HlslNewBoolAssignment(context, state->defined, 1);
            if (statement == NULL)
                return NULL;
            HlslAppendStmt(&body, statement);
        }
    }
    if (operation->kind == CGIR_STMT_GEOMETRY_EMIT) {
        record = HlslNewSymbolExpr(context,
            context->function->geometryOutputRecord);
        statement = HlslNewAppend(context->module, record,
            context->function->geometryFlatState, loc);
        if (statement == NULL)
            return NULL;
        HlslAppendStmt(&body, statement);
    }
    statement = HlslNewStmt(context->module, HLSL_STMT_BLOCK);
    if (statement != NULL) {
        statement->u.block = body;
        statement->loc = loc;
    }
    return statement;
} // HlslLowerGeometryOperation

static int HlslCollectCallsInExpr(HlslLowerContext *context, expr *source);
static int HlslCollectIRCallsInStatements(HlslLowerContext *context,
                                           const CgIRStmt *source);

static const CgIRFunction *HlslFindSourceIRFunction(
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

static int HlslCollectCallsInStatements(HlslLowerContext *context,
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

static int HlslCollectIRCallsInStatements(HlslLowerContext *context,
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

static int HlslGeometryIRHasDirectEffect(const CgIRStmt *source)
{
    for (; source != NULL; source = source->next) {
        if (source->kind == CGIR_STMT_GEOMETRY_EMIT ||
            source->kind == CGIR_STMT_GEOMETRY_FLAT ||
            source->kind == CGIR_STMT_GEOMETRY_RESTART)
            return 1;
        switch (source->kind) {
        case CGIR_STMT_BLOCK:
            if (HlslGeometryIRHasDirectEffect(source->u.block))
                return 1;
            break;
        case CGIR_STMT_IF:
            if (HlslGeometryIRHasDirectEffect(source->u.ifStmt.trueBranch) ||
                HlslGeometryIRHasDirectEffect(source->u.ifStmt.falseBranch))
            {
                return 1;
            }
            break;
        case CGIR_STMT_WHILE:
        case CGIR_STMT_DO:
            if (HlslGeometryIRHasDirectEffect(source->u.loop.body))
                return 1;
            break;
        case CGIR_STMT_FOR:
            if (HlslGeometryIRHasDirectEffect(source->u.forStmt.init) ||
                HlslGeometryIRHasDirectEffect(source->u.forStmt.body))
            {
                return 1;
            }
            break;
        default:
            break;
        }
    }
    return 0;
} // HlslGeometryIRHasDirectEffect

static int HlslGeometryIRExprCallsEffect(HlslLowerContext *context,
                                         const CgIRExpr *source)
{
    const CgIRExpr *argument;
    HlslFunction *callee;

    if (source == NULL)
        return 0;
    if (source->kind == CGIR_EXPR_CALL) {
        callee = HlslFindFunction(context->module, source->u.call.callee);
        if (callee != NULL && callee->geometryEffect)
            return 1;
    }
    switch (source->kind) {
    case CGIR_EXPR_MEMBER:
        return HlslGeometryIRExprCallsEffect(context,
                                             source->u.member.object);
    case CGIR_EXPR_INDEX:
        return HlslGeometryIRExprCallsEffect(context,
                    source->u.index.object) ||
               HlslGeometryIRExprCallsEffect(context,
                    source->u.index.index);
    case CGIR_EXPR_LENGTH:
        return HlslGeometryIRExprCallsEffect(context,
                                             source->u.length.object);
    case CGIR_EXPR_SWIZZLE:
        return HlslGeometryIRExprCallsEffect(context,
                                             source->u.swizzle.object);
    case CGIR_EXPR_CAST:
        return HlslGeometryIRExprCallsEffect(context,
                                             source->u.cast.operand);
    case CGIR_EXPR_UNARY:
        return HlslGeometryIRExprCallsEffect(context,
                                             source->u.unary.operand);
    case CGIR_EXPR_BINARY:
        return HlslGeometryIRExprCallsEffect(context,
                    source->u.binary.left) ||
               HlslGeometryIRExprCallsEffect(context,
                    source->u.binary.right);
    case CGIR_EXPR_ASSIGN:
        return HlslGeometryIRExprCallsEffect(context,
                    source->u.assign.target) ||
               HlslGeometryIRExprCallsEffect(context,
                    source->u.assign.value);
    case CGIR_EXPR_CONDITIONAL:
        return HlslGeometryIRExprCallsEffect(context,
                    source->u.conditional.condition) ||
               HlslGeometryIRExprCallsEffect(context,
                    source->u.conditional.trueExpr) ||
               HlslGeometryIRExprCallsEffect(context,
                    source->u.conditional.falseExpr);
    case CGIR_EXPR_CONSTRUCT:
        argument = source->u.construct.arguments;
        break;
    case CGIR_EXPR_CALL:
        argument = source->u.call.arguments;
        break;
    case CGIR_EXPR_INTERFACE_CALL:
        if (HlslGeometryIRExprCallsEffect(context,
                                         source->u.interfaceCall.receiver))
        {
            return 1;
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
        if (HlslGeometryIRExprCallsEffect(context, argument))
            return 1;
    }
    return 0;
} // HlslGeometryIRExprCallsEffect

static int HlslGeometryIRCallsEffect(HlslLowerContext *context,
                                     const CgIRStmt *source)
{
    const CgIRGeometryValue *value;

    for (; source != NULL; source = source->next) {
        switch (source->kind) {
        case CGIR_STMT_BLOCK:
            if (HlslGeometryIRCallsEffect(context, source->u.block))
                return 1;
            break;
        case CGIR_STMT_DECL:
            if (source->u.decl != NULL &&
                HlslGeometryIRExprCallsEffect(context,
                                              source->u.decl->initializer))
            {
                return 1;
            }
            break;
        case CGIR_STMT_EXPR:
            if (HlslGeometryIRExprCallsEffect(context,
                                              source->u.expression))
                return 1;
            break;
        case CGIR_STMT_IF:
            if (HlslGeometryIRExprCallsEffect(context,
                    source->u.ifStmt.condition) ||
                HlslGeometryIRCallsEffect(context,
                    source->u.ifStmt.trueBranch) ||
                HlslGeometryIRCallsEffect(context,
                    source->u.ifStmt.falseBranch))
            {
                return 1;
            }
            break;
        case CGIR_STMT_WHILE:
        case CGIR_STMT_DO:
            if (HlslGeometryIRExprCallsEffect(context,
                    source->u.loop.condition) ||
                HlslGeometryIRCallsEffect(context,
                    source->u.loop.body))
            {
                return 1;
            }
            break;
        case CGIR_STMT_FOR:
            if (HlslGeometryIRCallsEffect(context,
                    source->u.forStmt.init) ||
                HlslGeometryIRExprCallsEffect(context,
                    source->u.forStmt.condition) ||
                HlslGeometryIRExprCallsEffect(context,
                    source->u.forStmt.step) ||
                HlslGeometryIRCallsEffect(context,
                    source->u.forStmt.body))
            {
                return 1;
            }
            break;
        case CGIR_STMT_RETURN:
            if (HlslGeometryIRExprCallsEffect(context,
                                              source->u.returnExpr))
                return 1;
            break;
        case CGIR_STMT_DISCARD:
            if (HlslGeometryIRExprCallsEffect(context,
                                              source->u.discard.condition))
                return 1;
            break;
        case CGIR_STMT_GEOMETRY_EMIT:
        case CGIR_STMT_GEOMETRY_FLAT:
            for (value = source->u.geometry.values; value != NULL;
                 value = value->next)
            {
                if (HlslGeometryIRExprCallsEffect(context, value->value))
                    return 1;
            }
            break;
        default:
            break;
        }
    }
    return 0;
} // HlslGeometryStatementsCallEffect

static int HlslMarkGeometryEffects(HlslLowerContext *context)
{
    const CgIRFunction *sourceFunction;
    HlslFunction *function;
    int changed;

    for (function = context->module->functions; function != NULL;
         function = function->next)
    {
        sourceFunction = HlslFindSourceIRFunction(context,
            (Symbol *) function->identity);
        function->geometryEffect = sourceFunction != NULL &&
            HlslGeometryIRHasDirectEffect(sourceFunction->body);
    }
    do {
        changed = 0;
        for (function = context->module->functions; function != NULL;
             function = function->next)
        {
            sourceFunction = HlslFindSourceIRFunction(context,
                (Symbol *) function->identity);
            if (!function->geometryEffect && sourceFunction != NULL &&
                HlslGeometryIRCallsEffect(context, sourceFunction->body))
            {
                function->geometryEffect = 1;
                changed = 1;
            }
        }
    } while (changed);
    return 1;
} // HlslMarkGeometryEffects

static int HlslGeometrySemanticInfo(HlslLowerContext *context,
    const CgIRGeometryValue *value, HlslType *type, const char **spelling,
    HlslSemanticKind *kindOut, int *indexOut)
{
    const char *canonical;
    char root[64];
    char emitted[96];
    HlslSemanticKind kind;
    HlslBase abiBase;
    int index;

    canonical = GetAtomString(atable, value->canonicalSemantic);
    if (canonical == NULL ||
        !HlslParseSemantic(canonical, root, sizeof(root), &index) ||
        !HlslLowerType(context, value->type, type, &value->loc))
    {
        return 0;
    }
    kind = HlslModernSemantic(HLSL_STAGE_GEOMETRY,
                              HLSL_DIRECTION_OUTPUT, root, index);
    if (kind == HLSL_SEMANTIC_UNSUPPORTED ||
        !HlslModernSemanticIsLegal(HLSL_STAGE_GEOMETRY,
                                   HLSL_DIRECTION_OUTPUT, kind))
    {
        return 0;
    }
    if (kind == HLSL_SEMANTIC_USER) {
        if (sprintf(emitted, "%s%d", root, index) < 0)
            return 0;
    } else if (!HlslModernSemanticSpelling(kind, index, emitted,
                                           sizeof(emitted))) {
        return 0;
    }
    abiBase = HlslModernAbiBase(kind);
    if (abiBase == HLSL_BASE_UINT || abiBase == HLSL_BASE_BOOL)
        type->base = abiBase;
    *spelling = HlslCopyText(context, emitted);
    *kindOut = kind;
    *indexOut = index;
    return *spelling != NULL;
} // HlslGeometrySemanticInfo

static const char *HlslGeometryMemberName(HlslLowerContext *context,
                                           const char *semantic)
{
    char name[128];
    size_t i;

    if (semantic == NULL || strlen(semantic) + 1 > sizeof(name))
        return NULL;
    for (i = 0; semantic[i] != '\0'; i++) {
        name[i] = semantic[i] >= 'A' && semantic[i] <= 'Z' ?
                  (char) (semantic[i] - 'A' + 'a') : semantic[i];
    }
    name[i] = '\0';
    return HlslAllocateScopedSymbolName(context->module,
        context->module->geometryOutputStruct, semantic, name);
} // HlslGeometryMemberName

static int HlslCollectGeometryOutputStatements(HlslLowerContext *context,
                                                const CgIRStmt *source,
                                                HlslDecl *structure)
{
    const CgIRGeometryValue *value;
    HlslDecl *member;
    HlslType memberType;
    HlslSemanticKind kind;
    const char *canonical;
    const char *sourceSemantic;
    const char *semantic;
    const char *name;
    int semanticIndex;

    for (; source != NULL; source = source->next) {
        switch (source->kind) {
        case CGIR_STMT_BLOCK:
            if (!HlslCollectGeometryOutputStatements(context,
                    source->u.block, structure))
                return 0;
            break;
        case CGIR_STMT_IF:
            if (!HlslCollectGeometryOutputStatements(context,
                    source->u.ifStmt.trueBranch, structure) ||
                !HlslCollectGeometryOutputStatements(context,
                    source->u.ifStmt.falseBranch, structure))
            {
                return 0;
            }
            break;
        case CGIR_STMT_WHILE:
        case CGIR_STMT_DO:
            if (!HlslCollectGeometryOutputStatements(context,
                    source->u.loop.body, structure))
                return 0;
            break;
        case CGIR_STMT_FOR:
            if (!HlslCollectGeometryOutputStatements(context,
                    source->u.forStmt.init, structure) ||
                !HlslCollectGeometryOutputStatements(context,
                    source->u.forStmt.body, structure))
            {
                return 0;
            }
            break;
        case CGIR_STMT_GEOMETRY_EMIT:
        case CGIR_STMT_GEOMETRY_FLAT:
            for (value = source->u.geometry.values; value != NULL;
                 value = value->next)
            {
                canonical = GetAtomString(atable,
                                          value->canonicalSemantic);
                sourceSemantic = GetAtomString(atable,
                                               value->sourceSemantic);
                if (canonical == NULL ||
                    !HlslGeometrySemanticInfo(context, value,
                        &memberType, &semantic, &kind, &semanticIndex))
                {
                    return 0;
                }
                member = HlslGeometryOutputMember(context,
                                                   value->canonicalSemantic);
                if (member == NULL) {
                    name = HlslGeometryMemberName(context, canonical);
                    member = name != NULL ? HlslNewDecl(context->module,
                        HLSL_STORAGE_NONE, memberType, name) : NULL;
                    if (member == NULL)
                        return 0;
                    member->identity = value;
                    member->publicName = canonical;
                    member->semantic = semantic;
                    member->canonicalSemantic = semantic;
                    member->inputSemantic = sourceSemantic;
                    member->semanticKind = kind;
                    member->semanticIndex = semanticIndex;
                    member->interpolation =
                        HlslModernRequiredTargetInterpolation(
                            memberType.base);
                    HlslSetLoc(&member->loc, &value->loc);
                    HlslAppendDecl(&structure->members, member);
                } else if (!HlslIRTypeIsIdentical(&member->type,
                                                   &memberType)) {
                    HlslLoc conflictLoc;

                    HlslSetLoc(&conflictLoc, &value->loc);
                    return HlslFailRelated(context->module,
                        HLSL_ERROR_INTERFACE_CONFLICT, &conflictLoc,
                        &member->loc, canonical);
                }
                if (source->kind == CGIR_STMT_GEOMETRY_FLAT) {
                    member->geometryRole =
                        HLSL_GEOMETRY_DECL_FLAT_TARGET;
                    member->interpolation =
                        HLSL_INTERPOLATION_NOINTERPOLATION;
                }
            }
            break;
        default:
            break;
        }
    }
    return 1;
} // HlslCollectGeometryOutputStatements

static int HlslCollectGeometryOutput(HlslLowerContext *context)
{
    static int outputIdentity;
    static int placeholderIdentity;
    const CgIRFunction *function;
    HlslDecl *structure;
    HlslDecl *member;
    HlslType structureType;
    const char *name;
    const char *structureName;

    if (context->module->stage != HLSL_STAGE_GEOMETRY ||
        context->sourceIR == NULL)
    {
        return 1;
    }
    structureName = HlslAllocateGeneratedName(context->module,
        &outputIdentity, "cg_GeometryOut");
    structureType = HlslNumericType(HLSL_BASE_STRUCT, 0);
    structureType.structName = structureName;
    structure = structureName != NULL ? HlslNewDecl(context->module,
        HLSL_STORAGE_OUTPUT, structureType, structureName) : NULL;
    if (structure == NULL)
        return 0;
    context->module->geometryOutputStruct = structure;

    for (function = context->sourceIR->functions; function != NULL;
         function = function->next)
    {
        if (!HlslCollectGeometryOutputStatements(context, function->body,
                                                  structure))
            return 0;
    }
    if (structure->members == NULL) {
        name = HlslAllocateScopedSymbolName(context->module, structure,
                                            &placeholderIdentity,
                                            "cgc_placeholder");
        member = name != NULL ? HlslNewDecl(context->module,
            HLSL_STORAGE_NONE, HlslNumericType(HLSL_BASE_FLOAT, 1),
            name) : NULL;
        if (member == NULL)
            return 0;
        member->identity = &placeholderIdentity;
        member->semantic = HlslCopyText(context, "TEXCOORD0");
        member->canonicalSemantic = member->semantic;
        member->semanticKind = HLSL_SEMANTIC_USER;
        member->semanticIndex = 0;
        member->geometryRole = HLSL_GEOMETRY_DECL_OUTPUT_PLACEHOLDER;
        HlslAppendDecl(&structure->members, member);
    }
    structure->type.members = structure->members;
    HlslAppendDecl(&context->module->structs, structure);
    return 1;
} // HlslCollectGeometryOutput

static int HlslPrepareGeometryFunctionState(HlslLowerContext *context,
                                             HlslFunction *function)
{
    HlslFlatReplay **tail;
    HlslFlatReplay *state;
    HlslDecl *member;
    HlslDecl *shadow;
    HlslDecl *defined;
    HlslType streamType;
    HlslType boolType;
    const char *name;
    char generated[192];

    if (!function->geometryEffect)
        return 1;
    if (context->module->geometryOutputStruct == NULL ||
        context->module->geometryOutputStruct->members == NULL)
    {
        return 0;
    }
    streamType = HlslNumericType(HLSL_BASE_GEOMETRY_STREAM,
        (int) context->module->geometryStream + 1);
    name = HlslAllocateScopedSymbolName(context->module, function,
                                        function, "cgc_stream");
    function->geometryStream = name != NULL ? HlslNewDecl(
        context->module, HLSL_STORAGE_NONE, streamType, name) : NULL;
    name = HlslAllocateScopedSymbolName(context->module, function,
        context->module->geometryOutputStruct, "cgc_output");
    function->geometryOutputRecord = name != NULL ? HlslNewDecl(
        context->module, HLSL_STORAGE_NONE,
        context->module->geometryOutputStruct->type, name) : NULL;
    if (function->geometryStream == NULL ||
        function->geometryOutputRecord == NULL)
    {
        return 0;
    }
    function->geometryStream->parameterQualifier = HLSL_PARAMETER_INOUT;
    function->geometryStream->geometryRole = HLSL_GEOMETRY_DECL_STREAM;
    function->geometryStream->publicName =
        context->module->geometryOutputStruct->name;
    function->geometryOutputRecord->parameterQualifier =
        HLSL_PARAMETER_INOUT;
    function->geometryOutputRecord->geometryRole =
        HLSL_GEOMETRY_DECL_OUTPUT_RECORD;

    tail = &function->geometryFlatState;
    boolType = HlslNumericType(HLSL_BASE_BOOL, 1);
    for (member = context->module->geometryOutputStruct->members;
         member != NULL; member = member->next)
    {
        if (member->geometryRole != HLSL_GEOMETRY_DECL_FLAT_TARGET)
            continue;
        if (strlen(member->name) + 18 > sizeof(generated))
            return 0;
        sprintf(generated, "cgc_flat_%s", member->name);
        name = HlslAllocateScopedSymbolName(context->module, function,
                                            member, generated);
        shadow = name != NULL ? HlslNewDecl(context->module,
            HLSL_STORAGE_NONE, member->type, name) : NULL;
        sprintf(generated, "cgc_flat_%s_defined", member->name);
        name = HlslAllocateScopedSymbolName(context->module, function,
                                            shadow, generated);
        defined = name != NULL ? HlslNewDecl(context->module,
            HLSL_STORAGE_NONE, boolType, name) : NULL;
        state = shadow != NULL && defined != NULL ?
            HlslNewFlatReplay(context->module, member, shadow, defined) :
            NULL;
        if (state == NULL)
            return 0;
        shadow->parameterQualifier = HLSL_PARAMETER_INOUT;
        shadow->geometryRole = HLSL_GEOMETRY_DECL_FLAT_SHADOW;
        defined->parameterQualifier = HLSL_PARAMETER_INOUT;
        defined->geometryRole = HLSL_GEOMETRY_DECL_FLAT_DEFINED;
        *tail = state;
        tail = &state->next;
    }
    return 1;
} // HlslPrepareGeometryFunctionState

static int HlslPrepareGeometryFunctions(HlslLowerContext *context)
{
    HlslFunction *function;

    HlslMarkGeometryEffects(context);
    for (function = context->module->functions; function != NULL;
         function = function->next)
    {
        if (!HlslPrepareGeometryFunctionState(context, function))
            return 0;
    }
    return 1;
} // HlslPrepareGeometryFunctions

static int HlslDeclListContains(const HlslDecl *declarations,
                                const HlslDecl *target)
{
    for (; declarations != NULL; declarations = declarations->next) {
        if (declarations == target)
            return 1;
    }
    return 0;
} // HlslDeclListContains

static int HlslInitializeReturnedStructs(HlslModule *module,
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

static int HlslLowerFunction(HlslLowerContext *context,
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

static Type *HlslOriginalEntryResult(Symbol *program)
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

static int HlslIsEmptyEntry(Symbol *program)
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

int HlslLowerProgramWithIR(HlslModule *module,
                     const HlslProfileDesc *profile, SourceLoc *loc,
                     Scope *scope, Symbol *program,
                     const CgIRModule *sourceIR)
{
    HlslLowerContext context;
    HlslFunction *function;
    HlslFunction *helper;
    HlslType result;
    Type *sourceResult;
    const char *name;
    int emptyEntry;
    HlslGeometryInput geometryInput;
    HlslGeometryStream geometryStream;
    int geometryExtent;

    if (module == NULL || profile == NULL || scope == NULL ||
        program == NULL || program->kind != FUNCTION_S ||
        module->stage != profile->stage)
    {
        memset(&context, 0, sizeof(context));
        context.module = module;
        return HlslLowerFailure(&context, HLSL_ERROR_ENTRY_ABI,
                                "HLSL entry program", loc);
    }
    memset(&context, 0, sizeof(context));
    context.module = module;
    context.profile = profile;
    context.scope = scope;
    context.sourceIR = sourceIR;
    context.statementLoc = program->loc;
    context.entryFile = program->loc.file;
    if (!HlslRejectStorage(&context, program))
        return 0;
    if (profile->stage == HLSL_STAGE_GEOMETRY) {
        if (sourceIR == NULL || sourceIR->stage != CGIR_STAGE_GEOMETRY ||
            sourceIR->entry == NULL || sourceIR->entry->symbol != program ||
            sourceIR->geometry == NULL)
        {
            return HlslLowerFailure(&context, HLSL_ERROR_GEOMETRY_LAYOUT,
                                    "verified geometry layout",
                                    &program->loc);
        }
        if (!sourceIR->geometry->hasMaxOutputVertices ||
            sourceIR->geometry->maxOutputVertices == 0)
        {
            return HlslLowerFailure(&context,
                                    HLSL_ERROR_GEOMETRY_MISSING_MAX,
                                    "Vertices=N", &program->loc);
        }
        if (sourceIR->geometry->maxOutputVertices > (unsigned int) INT_MAX ||
            !HlslModernGeometryInput(sourceIR->geometry->inputTopology,
                                     &geometryInput, &geometryExtent) ||
            !HlslModernGeometryStream(sourceIR->geometry->outputTopology,
                                      &geometryStream) ||
            geometryExtent !=
                (int) sourceIR->geometry->inputVertexCount ||
            !HlslSetGeometryLayout(module, geometryInput, geometryStream,
                geometryExtent,
                (int) sourceIR->geometry->maxOutputVertices))
        {
            return HlslLowerFailure(&context, HLSL_ERROR_GEOMETRY_LAYOUT,
                                    "verified geometry layout",
                                    &program->loc);
        }
        context.geometryInputExtent = geometryExtent;
        if (!HlslCollectGeometryOutput(&context))
            return HlslLowerFailure(&context, HLSL_ERROR_INVALID_IR,
                                    "geometry output interface",
                                    &program->loc);
    }
    emptyEntry = profile->stage == HLSL_STAGE_GEOMETRY ? 0 :
                 HlslIsEmptyEntry(program);
    sourceResult = HlslOriginalEntryResult(program);
    if (!HlslEnsureType(&context, sourceResult) ||
        !HlslLowerType(&context, sourceResult, &result, &program->loc) ||
        (profile->stage == HLSL_STAGE_GEOMETRY ?
         !HlslCollectIRCallsInStatements(&context, sourceIR->entry->body) :
         !HlslCollectCallsInStatements(&context,
                                       program->details.fun.statements)))
    {
        return 0;
    }
    if (profile->stage != HLSL_STAGE_GEOMETRY &&
        program->details.fun.geometry.output !=
            CG_GEOMETRY_OUTPUT_UNKNOWN)
    {
        return HlslLowerFailure(&context,
                                HLSL_ERROR_UNSUPPORTED_OPERATION,
                                "geometry output modifier",
                                &program->details.fun.geometry.outputLoc);
    }
    if (profile->stage != HLSL_STAGE_GEOMETRY &&
        program->details.fun.geometry.input !=
            CG_GEOMETRY_INPUT_UNKNOWN)
    {
        return HlslLowerFailure(&context,
                                HLSL_ERROR_UNSUPPORTED_OPERATION,
                                "geometry input modifier",
                                &program->details.fun.geometry.inputLoc);
    }
    if (!HlslCollectUniformList(&context, Cg->theHAL->uniformParam) ||
        !HlslCollectUniformList(&context, Cg->theHAL->uniformGlobal) ||
        !HlslCollectUniformTree(&context, scope->symbols) ||
        !HlslCollectDefaults(&context))
    {
        return 0;
    }
    if (profile->stage != HLSL_STAGE_GEOMETRY) {
        for (helper = module->functions; helper != NULL;
             helper = helper->next)
        {
            if (helper->identity != NULL &&
                !HlslLowerFunction(&context, helper))
            {
                return 0;
            }
        }
    }
    name = emptyEntry ? "main" :
           HlslAllocateGeneratedName(module, program, "cg_entry");
    if (name == NULL)
        return HlslLowerFailure(&context, HLSL_ERROR_INVALID_IR,
                                "HLSL entry name", &program->loc);
    function = HlslNewFunction(module, result, name);
    if (function == NULL)
        return HlslLowerFailure(&context, HLSL_ERROR_INVALID_IR,
                                "HLSL entry allocation", &program->loc);
    function->identity = program;
    function->isEntry = 1;
    function->semantic = HlslFunctionSemantic(&context, program);
    HlslSetLoc(&function->loc, loc != NULL ? loc : &program->loc);
    module->entry = function;
    HlslAppendFunction(&module->functions, function);
    if (profile->stage == HLSL_STAGE_GEOMETRY &&
        !HlslPrepareGeometryFunctions(&context))
    {
        return HlslLowerFailure(&context, HLSL_ERROR_INVALID_IR,
                                "geometry function state", &program->loc);
    }
    if (profile->stage == HLSL_STAGE_GEOMETRY) {
        for (helper = module->functions;
             helper != NULL && helper != function;
             helper = helper->next)
        {
            if (helper->identity != NULL &&
                !HlslLowerFunction(&context, helper))
            {
                return 0;
            }
        }
    }
    context.function = function;
    if (!HlslCollectParameters(&context, program->details.fun.params, 1) ||
        program->details.fun.locals == NULL ||
        !HlslCollectLocals(&context,
                           program->details.fun.locals->symbols) ||
        (!emptyEntry &&
         !(profile->stage == HLSL_STAGE_GEOMETRY ?
           HlslLowerIRStatements(&context,
               sourceIR->entry->body != NULL &&
               sourceIR->entry->body->kind == CGIR_STMT_BLOCK &&
               sourceIR->entry->body->next == NULL ?
                   sourceIR->entry->body->u.block : sourceIR->entry->body,
                                 &function->body) :
           HlslLowerStatements(&context, program->details.fun.statements,
                               &function->body))) ||
        (!emptyEntry &&
         !HlslInitializeReturnedStructs(module, function, function->body)))
    {
        return 0;
    }
    return HlslSortStructs(&context) && module->errors == 0;
} // HlslLowerProgramWithIR

int HlslLowerProgram(HlslModule *module, const HlslProfileDesc *profile,
                     SourceLoc *loc, Scope *scope, Symbol *program)
{
    return HlslLowerProgramWithIR(module, profile, loc, scope, program,
                                  NULL);
} // HlslLowerProgram

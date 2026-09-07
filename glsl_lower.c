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
// glsl_lower.c
//

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <float.h>
#include <limits.h>

#include "glsl_lower_internal.h"



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

static int GlslCollectCallsInExpr(GlslLowerContext *context, expr *source);

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
 * GlslLowerLegacyProgram() - Lower the post-transform frontend tree for
 *          explicit -version 1.1 compiles only.  Cg 2.0 sources lower
 *          from the verified Cg IR through GlslLowerCgIR; this tree
 *          consumer remains solely so the historical language path
 *          keeps its exact bytes until final parity retires it.
 */

int GlslLowerLegacyProgram(GlslModule *module, const GlslProfileDesc *profile,
                           SourceLoc *loc, Scope *scope, Symbol *program)
{
    GlslLowerContext context;
    GlslFunction *function;
    GlslFunction *helper;
    GlslFunction *next;
    GlslType voidType;
    Type *result;
    const char *functionName;

    if (module == NULL || profile == NULL || scope == NULL ||
        program == NULL || program->kind != FUNCTION_S ||
        (profile->stage != GLSL_STAGE_VERTEX &&
         profile->stage != GLSL_STAGE_FRAGMENT) ||
        module->stage != profile->stage)
    {
        if (module != NULL)
            module->errors++;
        return 0;
    }
    memset(&context, 0, sizeof(context));
    context.module = module;
    context.profile = profile;
    context.scope = scope;
    context.statementLoc = program->loc;
    result = program->type->fun.rettype;
    if (GetCategory(result) != TYPE_CATEGORY_STRUCT ||
        !GlslValidateEntryInterfaces(&context, program) ||
        !GlslEnsureTypeAt(&context, result, &program->loc) ||
        !GlslEnsureParameterTypes(&context,
                                  program->details.fun.params) ||
        program->details.fun.locals == NULL ||
        !GlslEnsureSymbolTypes(&context,
                               program->details.fun.locals->symbols) ||
        !GlslCollectCallsInStatements(&context,
                                      program->details.fun.statements) ||
        !GlslCollectUniforms(&context, program) ||
        !GlslSortStructs(&context) ||
        !GlslAssignHelperNames(&context)) return GlslLowerError(&context);
    if (!GlslCollectDefaults(&context))
        return GlslLowerError(&context);
    GlslMarkForwardCalls(&context);
    for (helper = module->functions; helper != NULL; helper = next) {
        next = helper->next;
        if (!GlslLowerHelper(&context, helper))
            return GlslLowerError(&context);
    }
    functionName = GlslAllocateSymbolNameForSource(&context, program,
        "main", &program->loc);
    if (functionName == NULL)
        return GlslLowerError(&context);
    voidType = GlslNumericType(GLSL_BASE_VOID, 0);
    function = GlslNewFunction(module, voidType, functionName);
    if (function == NULL)
        return GlslLowerError(&context);
    function->identity = program;
    function->isEntry = 1;
    GlslSetLoc(&function->loc, loc != NULL ? loc : &program->loc);
    context.function = function;
    module->entry = function;
    GlslAppendFunction(&module->functions, function);
    if (!GlslCollectParameters(&context, program->details.fun.params, 1) ||
        !GlslCollectLocals(&context, program->details.fun.locals->symbols, 1) ||
        !GlslLowerStatementList(&context, program->details.fun.statements,
                                &function->body))
    {
        return GlslLowerError(&context);
    }
    GlslPrependMatrixHelpers(&context);
    GlslPrependMatrixSelectorHelpers(&context);
    if (!GlslValidateUniformLimit(&context) ||
        !GlslAllocateTextureUnits(&context) ||
        !GlslValidateInterfaceLimits(&context))
    {
        return GlslLowerError(&context);
    }
    return module->errors == 0;
}
static int GlslIRLowerStatement(GlslLowerContext *context,
                                const CgIRStmt *stmt, GlslStmt **list);
static int GlslIRBranch(GlslLowerContext *context, const CgIRStmt *branch,
                        GlslStmt **out);
static int GlslIRForPart(GlslLowerContext *context, const CgIRStmt *init,
                         GlslStmt **out);

int GlslIRInstallGeometryInfo(GlslLowerContext *context)
{
    const CgIRGeometryInfo *source;
    GlslGeometryInfo *target;

    if (context->source->stage != CGIR_STAGE_GEOMETRY)
        return 1;
    source = context->source->geometry;
    if (source == NULL || !source->hasMaxOutputVertices ||
        source->maxOutputVertices == 0)
    {
        GlslRecordFailure(context, "geometry maximum output vertices");
        return 0;
    }
    target = (GlslGeometryInfo *) (*context->module->alloc)(
        context->module->allocArg, sizeof(GlslGeometryInfo));
    if (target == NULL)
        return 0;
    memset(target, 0, sizeof(*target));
    switch (source->inputTopology) {
    case CG_GEOMETRY_INPUT_POINT:
        target->inputTopology = GLSL_GEOMETRY_INPUT_POINTS;
        break;
    case CG_GEOMETRY_INPUT_LINE:
        target->inputTopology = GLSL_GEOMETRY_INPUT_LINES;
        break;
    case CG_GEOMETRY_INPUT_LINE_ADJACENCY:
        target->inputTopology = GLSL_GEOMETRY_INPUT_LINES_ADJACENCY;
        break;
    case CG_GEOMETRY_INPUT_TRIANGLE:
        target->inputTopology = GLSL_GEOMETRY_INPUT_TRIANGLES;
        break;
    case CG_GEOMETRY_INPUT_TRIANGLE_ADJACENCY:
        target->inputTopology = GLSL_GEOMETRY_INPUT_TRIANGLES_ADJACENCY;
        break;
    default:
        GlslRecordFailure(context, "geometry input topology");
        return 0;
    }
    switch (source->outputTopology) {
    case CG_GEOMETRY_OUTPUT_POINTS:
        target->outputTopology = GLSL_GEOMETRY_OUTPUT_POINTS;
        break;
    case CG_GEOMETRY_OUTPUT_LINE_STRIP:
        target->outputTopology = GLSL_GEOMETRY_OUTPUT_LINE_STRIP;
        break;
    case CG_GEOMETRY_OUTPUT_TRIANGLE_STRIP:
        target->outputTopology = GLSL_GEOMETRY_OUTPUT_TRIANGLE_STRIP;
        break;
    default:
        GlslRecordFailure(context, "geometry output topology");
        return 0;
    }
    target->inputVertexCount = (int) source->inputVertexCount;
    target->maxOutputVertices = (int) source->maxOutputVertices;
    GlslSetLoc(&target->inputLoc, &source->inputLoc);
    GlslSetLoc(&target->outputLoc, &source->outputLoc);
    GlslSetLoc(&target->maxVerticesLoc, &source->maxVerticesLoc);
    context->module->geometry = target;
    return 1;
}

GlslInterpolation GlslInterpolationForType(const GlslType *type)
{
    while (type != NULL && type->elementType != NULL)
        type = type->elementType;
    if (type != NULL &&
        (type->base == GLSL_BASE_INT || type->base == GLSL_BASE_BOOL))
    {
        return GLSL_INTERPOLATION_FLAT;
    }
    return GLSL_INTERPOLATION_DEFAULT;
}

int GlslIRRegisterGeometryInput(GlslLowerContext *context,
    const CgIRDecl *param)
{
    const char *interfaceName;
    const char *canonical;
    const char *name;
    GlslType type;
    GlslStorage storage;
    GlslDecl *decl;
    GlslBinding *binding;
    GlslGeometryInputBinding *record;
    GlslGeometryInputBinding *existing;
    char generatedName[256];

    if (param->semantic == 0 || param->symbol == NULL) {
        GlslRecordFailure(context, "geometry input identity");
        return 0;
    }
    interfaceName = GlslCanonicalInterfaceName(context->profile,
                                                param->semantic, 0);
    if (interfaceName == NULL) {
        GlslRecordFailure(context, "geometry input semantic");
        return 0;
    }
    if (!GlslIRType(context, param->type, &type, &param->loc)) {
        if (context->module->errorReason == NULL)
            GlslRecordFailure(context, "geometry input type");
        return 0;
    }
    canonical = GetAtomString(atable, param->semantic);
    if (canonical == NULL) {
        GlslRecordFailure(context, "geometry input semantic atom");
        return 0;
    }
    if (!strncmp(interfaceName, "gl_", 3)) {
        storage = GLSL_STORAGE_BUILTIN;
        name = interfaceName;
        binding = NULL;
    } else {
        if (strlen(interfaceName) + 4 > sizeof(generatedName)) {
            GlslRecordFailure(context, "geometry input name length");
            return 0;
        }
        sprintf(generatedName, "cg_%s", interfaceName);
        name = GlslAllocateNameForSource(context, generatedName,
                                         &param->loc);
        storage = GLSL_STORAGE_INPUT;
        binding = GlslNewBinding(context->module, storage,
                                 name, canonical);
        if (name == NULL || binding == NULL) {
            GlslRecordFailure(context, "geometry input binding");
            return 0;
        }
    }
    decl = NULL;
    for (existing = context->geometryInputs; existing != NULL;
         existing = existing->next)
    {
        if (existing->declaration != NULL &&
            existing->declaration->name != NULL &&
            !strcmp(existing->declaration->name, name))
        {
            decl = existing->declaration;
            break;
        }
    }
    if (decl == NULL)
        decl = GlslNewDecl(context->module, storage, type, name);
    record = (GlslGeometryInputBinding *) context->module->alloc(
        context->module->allocArg, sizeof(GlslGeometryInputBinding));
    if (decl == NULL || record == NULL) {
        GlslRecordFailure(context, "geometry input declaration");
        return 0;
    }
    memset(record, 0, sizeof(GlslGeometryInputBinding));
    if (existing == NULL) {
        decl->identity = param->symbol;
        decl->interpolation = GlslInterpolationForType(&type);
        GlslSetLoc(&decl->loc, &param->loc);
        decl->sourceOrdinal = param->symbol->sourceOrdinal;
    }
    record->source = param->symbol;
    record->declaration = decl;
    record->next = context->geometryInputs;
    context->geometryInputs = record;
    if (binding != NULL) {
        binding->declaration = decl;
        binding->interfaceKey = interfaceName;
        binding->interpolation = decl->interpolation;
        GlslSetLoc(&binding->loc, &param->loc);
        binding->sourceOrdinal = param->symbol->sourceOrdinal;
        GlslAppendDecl(&context->module->globals, decl);
        GlslInsertBinding(&context->module->bindings, binding);
    }
    return 1;
}

static GlslDecl *GlslIRGeometryOutputDecl(GlslLowerContext *context,
    int semantic, Type *sourceType, const SourceLoc *loc)
{
    const char *name;
    GlslType type;
    GlslDecl *decl;
    GlslBinding *binding;
    GlslGeometryOutputBinding *output;

    if (!GlslIRType(context, sourceType, &type, loc))
        return NULL;
    name = GlslCanonicalInterfaceName(context->profile, semantic, 1);
    if (name == NULL) {
        GlslRecordFailure(context, "geometry output interface");
        return NULL;
    }
    for (output = context->geometryOutputs; output != NULL;
         output = output->next)
    {
        if (output->interfaceKey != NULL &&
            !strcmp(output->interfaceKey, name))
            return output->declaration;
    }
    if (!strncmp(name, "gl_", 3)) {
        decl = GlslNewDecl(context->module, GLSL_STORAGE_BUILTIN,
                           type, name);
        if (decl == NULL)
            return NULL;
        GlslSetLoc(&decl->loc, loc);
    } else {
        char generatedName[256];
        const char *declName;
        const char *canonical;

        if (strlen(name) + 4 > sizeof(generatedName))
            return NULL;
        sprintf(generatedName, "cg_%s", name);
        declName = GlslAllocateNameForSource(context, generatedName, loc);
        canonical = GetAtomString(atable, semantic);
        if (declName == NULL || canonical == NULL)
            return NULL;
        decl = GlslNewDecl(context->module, GLSL_STORAGE_OUTPUT,
                           type, declName);
        binding = GlslNewBinding(context->module, GLSL_STORAGE_OUTPUT,
                                 declName, canonical);
        if (decl == NULL || binding == NULL)
            return NULL;
        decl->interpolation = GlslInterpolationForType(&type);
        GlslSetLoc(&decl->loc, loc);
        binding->declaration = decl;
        binding->interfaceKey = name;
        binding->isOutput = 1;
        binding->interpolation = decl->interpolation;
        GlslSetLoc(&binding->loc, loc);
        GlslAppendDecl(&context->module->globals, decl);
        GlslInsertBinding(&context->module->bindings, binding);
    }
    output = (GlslGeometryOutputBinding *) context->module->alloc(
        context->module->allocArg, sizeof(GlslGeometryOutputBinding));
    if (output == NULL)
        return NULL;
    output->interfaceKey = name;
    output->declaration = decl;
    output->next = context->geometryOutputs;
    context->geometryOutputs = output;
    return decl;
}

GlslStmt *GlslIRGeometryAssignments(
    GlslLowerContext *context, const CgIRGeometryValue *value)
{
    GlslStmt *list;

    list = NULL;
    for (; value != NULL; value = value->next) {
        GlslType type;
        GlslDecl *decl;
        GlslExpr *left;
        GlslExpr *right;
        GlslExpr *assign;
        GlslStmt *stmt;

        if (!GlslIRType(context, value->type, &type, &value->loc))
            return NULL;
        decl = GlslIRGeometryOutputDecl(context,
            value->canonicalSemantic, value->type, &value->loc);
        if (decl == NULL)
            return NULL;
        left = GlslNewExpr(context->module, GLSL_EXPR_SYMBOL, type);
        right = GlslIRLowerExpr(context, value->value);
        assign = GlslNewExpr(context->module, GLSL_EXPR_BINARY, type);
        stmt = GlslNewStmt(context->module, GLSL_STMT_EXPRESSION);
        if (decl == NULL || left == NULL || right == NULL ||
            assign == NULL || stmt == NULL)
        {
            return NULL;
        }
        left->u.symbol = decl;
        assign->u.binary.op = GLSL_OP_ASSIGN;
        assign->u.binary.left = left;
        assign->u.binary.right = right;
        stmt->u.expression = assign;
        GlslSetLoc(&stmt->loc, &value->loc);
        GlslAppendStmt(&list, stmt);
    }
    return list;
}

static GlslGeometryFlat *GlslIRFindGeometryFlat(
    GlslLowerContext *context, int semantic)
{
    GlslGeometryFlat *flat;

    for (flat = context->geometryFlat; flat != NULL; flat = flat->next) {
        if (flat->semantic == semantic)
            return flat;
    }
    return NULL;
}

static GlslStmt *GlslIRAssignDecl(GlslLowerContext *context,
    GlslDecl *decl, GlslExpr *right, const SourceLoc *loc)
{
    GlslExpr *left;
    GlslExpr *assign;
    GlslStmt *stmt;

    left = GlslNewExpr(context->module, GLSL_EXPR_SYMBOL, decl->type);
    assign = GlslNewExpr(context->module, GLSL_EXPR_BINARY, decl->type);
    stmt = GlslNewStmt(context->module, GLSL_STMT_EXPRESSION);
    if (left == NULL || right == NULL || assign == NULL || stmt == NULL)
        return NULL;
    left->u.symbol = decl;
    assign->u.binary.op = GLSL_OP_ASSIGN;
    assign->u.binary.left = left;
    assign->u.binary.right = right;
    stmt->u.expression = assign;
    GlslSetLoc(&stmt->loc, loc);
    return stmt;
}

GlslStmt *GlslIRGeometryFlatAssignments(
    GlslLowerContext *context, const CgIRGeometryValue *value)
{
    GlslStmt *list;

    list = NULL;
    for (; value != NULL; value = value->next) {
        GlslGeometryFlat *flat;
        GlslExpr *right;
        GlslExpr *defined;
        GlslStmt *stmt;

        flat = GlslIRFindGeometryFlat(context,
                                      value->canonicalSemantic);
        if (flat == NULL) {
            GlslRecordFailure(context, "geometry flat state");
            return NULL;
        }
        right = GlslIRLowerExpr(context, value->value);
        stmt = GlslIRAssignDecl(context, flat->shadow, right,
                                &value->loc);
        if (stmt == NULL)
            return NULL;
        GlslAppendStmt(&list, stmt);
        defined = GlslNewExpr(context->module, GLSL_EXPR_BOOL,
                              flat->defined->type);
        if (defined == NULL)
            return NULL;
        defined->u.literalBool = 1;
        stmt = GlslIRAssignDecl(context, flat->defined, defined,
                                &value->loc);
        if (stmt == NULL)
            return NULL;
        GlslAppendStmt(&list, stmt);
    }
    return list;
}

GlslFlatReplay *GlslIRGeometryFlatReplay(
    GlslLowerContext *context)
{
    GlslGeometryFlat *flat;
    GlslFlatReplay *list;
    GlslFlatReplay *last;

    list = NULL;
    last = NULL;
    for (flat = context->geometryFlat; flat != NULL; flat = flat->next) {
        GlslFlatReplay *replay;

        replay = GlslNewFlatReplay(context->module, flat->target,
                                   flat->shadow, flat->defined);
        if (replay == NULL)
            return NULL;
        if (list == NULL)
            list = replay;
        else
            last->next = replay;
        last = replay;
    }
    return list;
}

static int GlslIRCollectHelper(GlslLowerContext *context,
                               const Symbol *symbol);

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

static int GlslIRRegisterGeometryFlatValue(GlslLowerContext *context,
    const CgIRGeometryValue *value)
{
    GlslGeometryFlat *flat;
    GlslType type;
    GlslType boolType;
    const char *semantic;
    const char *shadowName;
    const char *definedName;
    char generatedName[256];

    if (GlslIRFindGeometryFlat(context, value->canonicalSemantic) != NULL)
        return 1;
    if (!GlslIRType(context, value->type, &type, &value->loc))
        return 0;
    semantic = GetAtomString(atable, value->canonicalSemantic);
    if (semantic == NULL || strlen(semantic) + 17 > sizeof(generatedName))
        return 0;
    flat = (GlslGeometryFlat *) context->module->alloc(
        context->module->allocArg, sizeof(GlslGeometryFlat));
    if (flat == NULL)
        return 0;
    memset(flat, 0, sizeof(GlslGeometryFlat));
    flat->semantic = value->canonicalSemantic;
    flat->target = GlslIRGeometryOutputDecl(context,
        value->canonicalSemantic, value->type, &value->loc);
    sprintf(generatedName, "cg_flat_%s", semantic);
    shadowName = GlslAllocateNameForSource(context, generatedName,
                                           &value->loc);
    sprintf(generatedName, "cg_flat_%s_defined", semantic);
    definedName = GlslAllocateNameForSource(context, generatedName,
                                            &value->loc);
    boolType = GlslNumericType(GLSL_BASE_BOOL, 1);
    flat->shadow = GlslNewDecl(context->module, GLSL_STORAGE_NONE,
                               type, shadowName);
    flat->defined = GlslNewDecl(context->module, GLSL_STORAGE_NONE,
                                boolType, definedName);
    if (flat->target == NULL || shadowName == NULL ||
        definedName == NULL || flat->shadow == NULL ||
        flat->defined == NULL)
    {
        return 0;
    }
    GlslSetLoc(&flat->shadow->loc, &value->loc);
    GlslSetLoc(&flat->defined->loc, &value->loc);
    GlslAppendDecl(&context->module->globals, flat->shadow);
    GlslAppendDecl(&context->module->globals, flat->defined);
    if (context->geometryFlat == NULL)
        context->geometryFlat = flat;
    else
        context->lastGeometryFlat->next = flat;
    context->lastGeometryFlat = flat;
    return 1;
}

static int GlslIRCollectGeometryFlatInStmt(GlslLowerContext *context,
    const CgIRStmt *stmt)
{
    const CgIRGeometryValue *value;

    if (stmt == NULL)
        return 1;
    switch (stmt->kind) {
    case CGIR_STMT_BLOCK:
        for (stmt = stmt->u.block; stmt != NULL; stmt = stmt->next) {
            if (!GlslIRCollectGeometryFlatInStmt(context, stmt))
                return 0;
        }
        return 1;
    case CGIR_STMT_IF:
        return GlslIRCollectGeometryFlatInStmt(context,
                                                stmt->u.ifStmt.trueBranch) &&
               GlslIRCollectGeometryFlatInStmt(context,
                                                stmt->u.ifStmt.falseBranch);
    case CGIR_STMT_WHILE:
    case CGIR_STMT_DO:
        return GlslIRCollectGeometryFlatInStmt(context,
                                                stmt->u.loop.body);
    case CGIR_STMT_FOR:
        return GlslIRCollectGeometryFlatInStmt(context,
                                                stmt->u.forStmt.init) &&
               GlslIRCollectGeometryFlatInStmt(context,
                                                stmt->u.forStmt.body);
    case CGIR_STMT_GEOMETRY_FLAT:
        for (value = stmt->u.geometry.values; value != NULL;
             value = value->next)
        {
            if (!GlslIRRegisterGeometryFlatValue(context, value))
                return 0;
        }
        return 1;
    default:
        return 1;
    }
}

int GlslIRCollectGeometryFlatState(GlslLowerContext *context)
{
    GlslFunction *function;

    if (context->module->stage != GLSL_STAGE_GEOMETRY)
        return 1;
    if (!GlslIRCollectGeometryFlatInStmt(context,
                                         context->source->entry->body))
    {
        return 0;
    }
    for (function = context->module->functions; function != NULL;
         function = function->next)
    {
        const CgIRFunction *source;

        source = GlslIRFindIRFunction(context->source,
                                      (const Symbol *) function->identity);
        if (source == NULL ||
            !GlslIRCollectGeometryFlatInStmt(context, source->body))
        {
            return 0;
        }
    }
    return 1;
}

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

const CgIRDecl *GlslIRGeometryEntryParameter(
    const GlslLowerContext *context, const Symbol *symbol)
{
    const CgIRDecl *param;

    if (context->module->stage != GLSL_STAGE_GEOMETRY ||
        context->entry == NULL || symbol == NULL)
    {
        return NULL;
    }
    for (param = context->entry->parameters; param != NULL;
         param = param->next)
    {
        if (param->symbol == symbol)
            return param;
    }
    return NULL;
}

GlslExpr *GlslIRGeometryBuiltinElement(
    GlslLowerContext *context, const CgIRDecl *param,
    GlslExpr *indexExpr, const GlslType *resultType,
    const SourceLoc *loc)
{
    const char *memberName;
    GlslType perVertexType;
    GlslType glInType;
    GlslType *element;
    GlslDecl *glInDecl;
    GlslExpr *glIn;
    GlslExpr *indexed;
    GlslExpr *memberExpr;

    memberName = GlslGeometryInputMemberName(context->profile,
                                              param->semantic);
    if (memberName == NULL)
        return NULL;
    perVertexType = GlslNumericType(GLSL_BASE_STRUCT, 0);
    perVertexType.structName = "gl_PerVertex";
    element = (GlslType *) context->module->alloc(
        context->module->allocArg, sizeof(GlslType));
    if (element == NULL)
        return NULL;
    *element = perVertexType;
    glInType = GlslNumericType(GLSL_BASE_VOID, 0);
    glInType.arraySize = context->module->geometry->inputVertexCount;
    glInType.elementType = element;
    glInDecl = GlslNewDecl(context->module, GLSL_STORAGE_BUILTIN,
                           glInType, "gl_in");
    glIn = GlslNewExpr(context->module, GLSL_EXPR_SYMBOL, glInType);
    indexed = GlslNewExpr(context->module, GLSL_EXPR_INDEX,
                          perVertexType);
    memberExpr = GlslNewExpr(context->module, GLSL_EXPR_MEMBER,
                             *resultType);
    if (glInDecl == NULL || glIn == NULL || indexExpr == NULL ||
        indexed == NULL || memberExpr == NULL)
    {
        return NULL;
    }
    glIn->u.symbol = glInDecl;
    indexed->u.index.object = glIn;
    indexed->u.index.index = indexExpr;
    memberExpr->u.member.object = indexed;
    memberExpr->u.member.name = memberName;
    GlslSetLoc(&memberExpr->loc, loc);
    return memberExpr;
}

GlslExpr *GlslIRGeometryBuiltinArray(
    GlslLowerContext *context, const CgIRExpr *expr,
    const GlslType *type)
{
    const CgIRDecl *param;
    GlslExpr *target;
    GlslType elementType;
    int i;

    param = GlslIRGeometryEntryParameter(context, expr->u.symbol);
    if (param == NULL || !CgIsAttribArray(param->type) ||
        GlslGeometryInputMemberName(context->profile,
                                    param->semantic) == NULL ||
        type->elementType == NULL)
    {
        return NULL;
    }
    elementType = *type->elementType;
    target = GlslNewExpr(context->module, GLSL_EXPR_CONSTRUCT, *type);
    if (target == NULL)
        return NULL;
    for (i = 0; i < context->module->geometry->inputVertexCount; i++) {
        GlslExpr *indexExpr;
        GlslExpr *item;

        indexExpr = GlslNewLiteral(context, GLSL_BASE_INT, i, 0.0f);
        item = GlslIRGeometryBuiltinElement(context, param, indexExpr,
                                            &elementType, &expr->loc);
        if (item == NULL)
            return NULL;
        GlslAppendExpr(&target->u.construct.arguments, item);
    }
    return target;
}

///////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////// Cg IR statement lowering (Cg 2.0) //////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////

static int GlslIRLowerStatement(GlslLowerContext *context,
                                const CgIRStmt *source, GlslStmt **list)
{
    GlslStmt *target;
    GlslExpr *condition;
    GlslType boolType;

    if (source == NULL)
        return 1;
    context->statementLoc = source->loc;
    switch (source->kind) {
    case CGIR_STMT_DECL:
        /* Declarations are consumed by the enclosing block walker;
         * reaching this point means an unexpected producer shape. */
        GlslRecordFailure(context, "GLSL profile declaration placement");
        return 0;
    case CGIR_STMT_EXPR:
        if (source->u.expression == NULL)
            return 1;
        /* Struct assignments flatten member-wise exactly as the legacy
         * FlattenStructAssignments pass arranged before tree lowering
         * saw them; targets that are themselves native aggregate
         * temporaries (the return rewrite's cg_return) stay whole. */
        if (source->u.expression->kind == CGIR_EXPR_ASSIGN &&
            source->u.expression->u.assign.op == CGIR_OP_ASSIGN &&
            source->u.expression->type != NULL &&
            IsStruct(source->u.expression->type) &&
            !(source->u.expression->u.assign.target != NULL &&
              source->u.expression->u.assign.target->kind ==
                  CGIR_EXPR_SYMBOL &&
              source->u.expression->u.assign.target->u.symbol != NULL &&
              (source->u.expression->u.assign.target->u.symbol->properties &
               SYMB_IS_NATIVE_AGGREGATE_TEMP)))
        {
            return GlslIRLowerAggregateAssign(context,
                                              source->u.expression, list);
        }
        target = GlslNewStmt(context->module, GLSL_STMT_EXPRESSION);
        if (target == NULL)
            return 0;
        target->u.expression = GlslIRLowerExpr(context,
                                               source->u.expression);
        if (target->u.expression == NULL)
            return 0;
        break;
    case CGIR_STMT_IF:
        target = GlslNewStmt(context->module, GLSL_STMT_IF);
        if (target == NULL)
            return 0;
        target->u.ifStmt.condition = GlslIRLowerExpr(
            context, source->u.ifStmt.condition);
        if (target->u.ifStmt.condition == NULL)
            return 0;
        if (!GlslIRBranch(context, source->u.ifStmt.trueBranch,
                          &target->u.ifStmt.trueBranch) ||
            !GlslIRBranch(context, source->u.ifStmt.falseBranch,
                          &target->u.ifStmt.falseBranch)) return 0;
        break;
    case CGIR_STMT_WHILE:
    case CGIR_STMT_DO:
        target = GlslNewStmt(context->module,
            source->kind == CGIR_STMT_WHILE ? GLSL_STMT_WHILE :
                                              GLSL_STMT_DO);
        if (target == NULL)
            return 0;
        target->u.loop.condition = GlslIRLowerExpr(
            context, source->u.loop.condition);
        if (target->u.loop.condition == NULL)
            return 0;
        context->loopDepth++;
        if (!GlslIRBranch(context, source->u.loop.body,
                          &target->u.loop.body))
        {
            context->loopDepth--;
            return 0;
        }
        context->loopDepth--;
        break;
    case CGIR_STMT_FOR:
        target = GlslNewStmt(context->module, GLSL_STMT_FOR);
        if (target == NULL)
            return 0;
        if (source->u.forStmt.init != NULL &&
            !GlslIRForPart(context, source->u.forStmt.init,
                           &target->u.forStmt.init))
            return 0;
        if (source->u.forStmt.condition != NULL) {
            target->u.forStmt.condition = GlslIRLowerExpr(
                context, source->u.forStmt.condition);
            if (target->u.forStmt.condition == NULL)
                return 0;
        }
        if (source->u.forStmt.step != NULL) {
            GlslStmt *stepStmt;

            stepStmt = GlslNewStmt(context->module,
                                   GLSL_STMT_EXPRESSION);
            if (stepStmt == NULL)
                return 0;
            stepStmt->u.expression = GlslIRLowerExpr(
                context, source->u.forStmt.step);
            if (stepStmt->u.expression == NULL)
                return 0;
            target->u.forStmt.step = stepStmt;
        }
        context->loopDepth++;
        if (!GlslIRBranch(context, source->u.forStmt.body,
                          &target->u.forStmt.body))
        {
            context->loopDepth--;
            return 0;
        }
        context->loopDepth--;
        break;
    case CGIR_STMT_BLOCK:
        target = GlslNewStmt(context->module, GLSL_STMT_BLOCK);
        if (target == NULL)
            return 0;
        if (!GlslIRBlockBody(context, source->u.block, &target->u.block))
            return 0;
        break;
    case CGIR_STMT_RETURN:
        target = GlslNewStmt(context->module, GLSL_STMT_RETURN);
        if (target == NULL)
            return 0;
        if (source->u.returnExpr != NULL) {
            target->u.returnExpr = GlslIRLowerExpr(
                context, source->u.returnExpr);
            if (target->u.returnExpr == NULL)
                return 0;
        }
        break;
    case CGIR_STMT_DISCARD:
        if (context->module->stage != GLSL_STAGE_FRAGMENT) {
            GlslRecordFailureKind(context, GLSL_ERROR_STAGE_OPERATION,
                                  "discard");
            return 0;
        }
        if (source->u.discard.condition == NULL) {
            target = GlslNewStmt(context->module, GLSL_STMT_DISCARD);
            if (target == NULL)
                return 0;
        } else {
            GlslStmt *discard;
            GlslExpr *reduction;

            target = GlslNewStmt(context->module, GLSL_STMT_IF);
            discard = GlslNewStmt(context->module, GLSL_STMT_DISCARD);
            if (target == NULL || discard == NULL)
                return 0;
            condition = GlslIRLowerExpr(context,
                                        source->u.discard.condition);
            if (condition == NULL)
                return 0;
            if (condition->type.base != GLSL_BASE_BOOL ||
                condition->type.rows != 0 || condition->type.cols != 0 ||
                condition->type.arraySize != 0 ||
                condition->type.structName != NULL ||
                condition->type.elementType != NULL ||
                condition->type.len < 1 || condition->type.len > 4)
            {
                GlslRecordFailureKind(context,
                                      GLSL_ERROR_UNSUPPORTED_TYPE,
                                      "discard condition type");
                return 0;
            }
            if (condition->type.len > 1) {
                boolType = GlslNumericType(GLSL_BASE_BOOL, 1);
                reduction = GlslNewExpr(context->module,
                                        GLSL_EXPR_CALL, boolType);
                if (reduction == NULL)
                    return 0;
                reduction->u.call.name = "any";
                reduction->u.call.arguments = condition;
                condition = reduction;
            }
            target->u.ifStmt.condition = condition;
            GlslSetLoc(&discard->loc, &source->loc);
            target->u.ifStmt.trueBranch = discard;
        }
        break;
    case CGIR_STMT_BREAK:
        if (context->loopDepth == 0) {
            GlslRecordFailure(context, "break outside loop");
            return 0;
        }
        target = GlslNewStmt(context->module, GLSL_STMT_BREAK);
        break;
    case CGIR_STMT_CONTINUE:
        if (context->loopDepth == 0) {
            GlslRecordFailure(context, "continue outside loop");
            return 0;
        }
        target = GlslNewStmt(context->module, GLSL_STMT_CONTINUE);
        break;
    case CGIR_STMT_GEOMETRY_EMIT:
        if (context->module->stage != GLSL_STAGE_GEOMETRY) {
            GlslRecordFailureKind(context, GLSL_ERROR_STAGE_OPERATION,
                                  "emitVertex");
            return 0;
        }
        target = GlslNewGeometryEmit(context->module,
            GlslIRGeometryAssignments(context, source->u.geometry.values),
            GlslIRGeometryFlatReplay(context));
        if (target == NULL || target->u.emit.assignments == NULL)
            return 0;
        break;
    case CGIR_STMT_GEOMETRY_RESTART:
        if (context->module->stage != GLSL_STAGE_GEOMETRY) {
            GlslRecordFailureKind(context, GLSL_ERROR_STAGE_OPERATION,
                                  "restartStrip");
            return 0;
        }
        target = GlslNewGeometryRestart(context->module);
        break;
    case CGIR_STMT_GEOMETRY_FLAT:
        if (context->module->stage != GLSL_STAGE_GEOMETRY) {
            GlslRecordFailureKind(context, GLSL_ERROR_STAGE_OPERATION,
                                  "flatAttrib");
            return 0;
        }
        target = GlslNewStmt(context->module, GLSL_STMT_BLOCK);
        if (target == NULL)
            return 0;
        target->u.block = GlslIRGeometryFlatAssignments(context,
            source->u.geometry.values);
        if (target->u.block == NULL)
            return 0;
        break;
    default:
        GlslRecordFailure(context, "GLSL profile statement");
        return 0;
    }
    if (target == NULL)
        return 0;
    GlslSetLoc(&target->loc, &source->loc);
    GlslAppendStmt(list, target);
    return 1;
} // GlslIRLowerStatement

/*
 * GlslIRBlockBody() - Walk one IR block: declarations become function
 *          locals (or feed the group-write reconstruction), everything
 *          else lowers statement by statement.
 */

int GlslIRBlockBody(GlslLowerContext *context,
                           const CgIRStmt *stmt, GlslStmt **out)
{
    while (stmt != NULL) {
        const CgIRStmt *next = stmt->next;
        int consumed;

        /* Group writes may start at a bare store run (pure case) or at
         * a synthesized "$" temporary declaration (hoisted case). */
        consumed = GlslIRTryGroupWrite(context, stmt, &next, out);
        if (consumed < 0)
            return 0;
        if (consumed > 0) {
            stmt = next;
            continue;
        }
        if (stmt->kind == CGIR_STMT_DECL) {
            if (!GlslIRLocalDeclaration(context, stmt, &next, out))
                return 0;
        } else {
            if (!GlslIRLowerStatement(context, stmt, out)) return 0;
        }
        stmt = next;
    }
    return 1;
} // GlslIRBlockBody

/*
 * GlslIRBranch() - An IR branch is one statement or a wrapped block;
 *          the GLSL branch slot takes the flat statement list.
 */

static int GlslIRBranch(GlslLowerContext *context, const CgIRStmt *branch,
                        GlslStmt **out)
{
    if (branch == NULL)
        return 1;
    if (branch->kind == CGIR_STMT_BLOCK)
        return GlslIRBlockBody(context, branch->u.block, out);
    return GlslIRLowerStatement(context, branch, out);
} // GlslIRBranch

/*
 * GlslIRForPart() - The for-header init keeps expression statements
 *      only, exactly as the legacy for-part walker demanded.
 */

static int GlslIRForPart(GlslLowerContext *context, const CgIRStmt *init,
                         GlslStmt **out)
{
    GlslStmt *target;

    if (init == NULL)
        return 1;
    if (init->kind == CGIR_STMT_BLOCK)
        init = init->u.block;
    for (; init != NULL; init = init->next) {
        if (init->kind != CGIR_STMT_EXPR ||
            init->u.expression == NULL)
        {
            GlslRecordFailure(context, "GLSL for expression");
            return 0;
        }
        target = GlslNewStmt(context->module, GLSL_STMT_EXPRESSION);
        if (target == NULL)
            return 0;
        target->u.expression = GlslIRLowerExpr(context,
                                               init->u.expression);
        if (target->u.expression == NULL)
            return 0;
        GlslSetLoc(&target->loc, &init->loc);
        GlslAppendStmt(out, target);
    }
    return 1;
} // GlslIRForPart

///////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////// Cg IR module assembly (public) ///////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////

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

/*
 * GlslLowerCgIR() - Lower one verified Cg IR module into the GLSL
 *      module.  Returns nonzero when lowering succeeded; failures
 *      record exactly one profile diagnostic through the module error
 *          fields for the HAL hooks to translate.
 */

int GlslLowerCgIR(GlslModule *module, const GlslProfileDesc *profile,
                  const CgIRModule *source)
{
    GlslLowerContext context;
    GlslFunction *function;
    GlslType voidType;
    const char *functionName;

    if (module == NULL || profile == NULL || source == NULL ||
        source->entry == NULL ||
        (profile->stage != GLSL_STAGE_VERTEX &&
         profile->stage != GLSL_STAGE_FRAGMENT &&
         profile->stage != GLSL_STAGE_GEOMETRY) ||
        module->stage != profile->stage)
    {
        if (module != NULL)
            module->errors++;
        return 0;
    }
    memset(&context, 0, sizeof(context));
    context.module = module;
    context.profile = profile;
    context.source = source;
    context.entry = source->entry;
    context.statementLoc = source->entry->loc;

    if (!GlslIRInstallGeometryInfo(&context))
        return GlslLowerError(&context);

    /* Phase order mirrors the legacy pipeline so the first failure in
     * any program surfaces from the same layer. */

    if (!GlslIRValidateEntryInterfaces(&context, source->entry)) {
        if (module->errorReason == NULL)
            GlslRecordFailure(&context, "entry interface reservation");
        return GlslLowerError(&context);
    }
    if (!GlslIRCollectCallsInStmt(&context, source->entry->body)) {
        if (module->errorReason == NULL)
            GlslRecordFailure(&context, "reachable helper collection");
        return GlslLowerError(&context);
    }
    if (!GlslIRCollectGeometryFlatState(&context)) {
        if (module->errorReason == NULL)
            GlslRecordFailure(&context, "geometry flat state");
        return GlslLowerError(&context);
    }
    if (!GlslIRCollectUniforms(&context, source->entry)) {
        if (module->errorReason == NULL)
            GlslRecordFailure(&context, "uniform collection");
        return GlslLowerError(&context);
    }
    if (!GlslIREnsureEntryLocals(&context, source->entry)) {
        if (module->errorReason == NULL)
            GlslRecordFailure(&context, "entry type collection");
        return GlslLowerError(&context);
    }
    if (!GlslSortStructs(&context))
        return GlslLowerError(&context);
    if (!GlslAssignHelperNames(&context))
        return GlslLowerError(&context);
    if (!GlslCollectDefaults(&context))
        return GlslLowerError(&context);

    /* Forward prototypes: a call to a helper that sorts after its
     * caller needs a declaration. */
    for (function = module->functions; function != NULL;
         function = function->next)
    {
        const CgIRFunction *irFunction;

        irFunction = GlslIRFindIRFunction(source, function->identity);
        if (irFunction != NULL)
            GlslIRMarkForwardCallsInStmt(&context, function,
                                         irFunction->body);
    }

    /* Helper bodies, in module order. */
    for (function = module->functions; function != NULL;
         function = function->next)
    {
        const CgIRFunction *irFunction;

        irFunction = GlslIRFindIRFunction(source, function->identity);
        context.function = function;
        if (irFunction == NULL)
            return GlslLowerError(&context);
        context.statementLoc = irFunction->loc;
        if (!GlslIRLowerFunctionBody(&context, irFunction))
            return GlslLowerError(&context);
    }

    functionName = GlslAllocateSymbolNameForSource(&context,
        source->entry->symbol, "main", &source->entry->loc);
    if (functionName == NULL)
        return GlslLowerError(&context);
    voidType = GlslNumericType(GLSL_BASE_VOID, 0);
    function = GlslNewFunction(module, voidType, functionName);
    if (function == NULL)
        return GlslLowerError(&context);
    function->identity = source->entry->symbol;
    function->isEntry = 1;
    GlslSetLoc(&function->loc, &source->entry->loc);
    context.function = function;
    module->entry = function;
    GlslAppendFunction(&module->functions, function);
    if (!GlslIRLowerFunctionBody(&context, source->entry))
        return GlslLowerError(&context);
    GlslPrependMatrixHelpers(&context);
    GlslPrependMatrixSelectorHelpers(&context);
    if (!GlslValidateUniformLimit(&context) ||
        !GlslAllocateTextureUnits(&context) ||
        !GlslValidateInterfaceLimits(&context))
    {
        return GlslLowerError(&context);
    }
    return module->errors == 0;
} // GlslLowerCgIR


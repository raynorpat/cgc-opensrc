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


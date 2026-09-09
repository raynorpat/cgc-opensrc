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

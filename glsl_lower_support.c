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
// glsl_lower_support.c
//

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <float.h>
#include <limits.h>

#include "glsl_lower_internal.h"


void GlslSetLoc(GlslLoc *target, const SourceLoc *source)
{
    if (source != NULL) {
        target->file = source->file;
        target->line = source->line;
    }
}

const char *GlslAllocateSymbolNameForSource(
    GlslLowerContext *context, const void *identity, const char *source,
    const SourceLoc *loc)
{
    GlslLoc glslLoc;

    memset(&glslLoc, 0, sizeof(glslLoc));
    GlslSetLoc(&glslLoc, loc);
    return GlslAllocateSymbolNameAt(context->module, identity, source,
                                    loc != NULL ? &glslLoc : NULL);
}

const char *GlslAllocateNameForSource(GlslLowerContext *context,
    const char *source, const SourceLoc *loc)
{
    GlslLoc glslLoc;

    memset(&glslLoc, 0, sizeof(glslLoc));
    GlslSetLoc(&glslLoc, loc);
    return GlslAllocateNameAt(context->module, source,
                              loc != NULL ? &glslLoc : NULL);
}

const char *GlslAllocateDistinctNameForSource(
    GlslLowerContext *context, const char *source, const SourceLoc *loc)
{
    GlslLoc glslLoc;

    memset(&glslLoc, 0, sizeof(glslLoc));
    GlslSetLoc(&glslLoc, loc);
    return GlslAllocateDistinctNameAt(context->module, source,
                                      loc != NULL ? &glslLoc : NULL);
}

const char *GlslAllocateScopedSymbolNameForSource(
    GlslLowerContext *context, const void *nameSpace, const void *identity,
    const char *source, const SourceLoc *loc)
{
    GlslLoc glslLoc;

    memset(&glslLoc, 0, sizeof(glslLoc));
    GlslSetLoc(&glslLoc, loc);
    return GlslAllocateScopedSymbolNameAt(context->module, nameSpace,
                                          identity, source,
                                          loc != NULL ? &glslLoc : NULL);
}

int GlslLowerError(GlslLowerContext *context)
{
    context->module->errors++;
    return 0;
}

void GlslRecordFailure(GlslLowerContext *context, const char *reason)
{
    if (context->module->errorReason != NULL)
        return;
    context->module->errorLoc.file = context->statementLoc.file;
    context->module->errorLoc.line = context->statementLoc.line;
    if (context->module->errorKind == GLSL_ERROR_NONE)
        context->module->errorKind = GLSL_ERROR_UNSUPPORTED_OPERATION;
    context->module->errorReason = reason;
    context->module->errorSymbol =
        context->function != NULL ? context->function->identity : NULL;
}

void GlslRecordFailureKind(GlslLowerContext *context,
                                  GlslErrorKind kind,
                                  const char *reason)
{
    if (context->module->errorReason != NULL)
        return;
    context->module->errorKind = kind;
    GlslRecordFailure(context, reason);
}

void GlslRecordFailureKindAt(GlslLowerContext *context,
                                    GlslErrorKind kind,
                                    const char *reason,
                                    const SourceLoc *loc)
{
    SourceLoc savedLoc;

    if (loc == NULL) {
        GlslRecordFailureKind(context, kind, reason);
        return;
    }
    savedLoc = context->statementLoc;
    context->statementLoc = *loc;
    GlslRecordFailureKind(context, kind, reason);
    context->statementLoc = savedLoc;
}

const char *GlslUnsupportedExprReason(const expr *source)
{
    if (source != NULL && source->common.kind == BINARY_N) {
        switch (source->bin.op) {
        case MOD_OP: case MOD_V_OP: case MOD_SV_OP: case MOD_VS_OP:
            return "remainder (%)";
        case SHL_OP: case SHL_V_OP: case SHR_OP: case SHR_V_OP:
            return "shift operator";
        case AND_OP: case AND_V_OP: case AND_SV_OP: case AND_VS_OP:
        case XOR_OP: case XOR_V_OP: case XOR_SV_OP: case XOR_VS_OP:
        case OR_OP: case OR_V_OP: case OR_SV_OP: case OR_VS_OP:
            return "bitwise operator";
        case INTERFACE_CALL_OP:
            return "interface dispatch";
        default:
            break;
        }
    }
    return "GLSL profile expression";
}

char *GlslCopyText(GlslModule *module, const char *text)
{
    char *copy;
    size_t size;

    size = strlen(text) + 1;
    copy = (char *) module->alloc(module->allocArg, size);
    if (copy != NULL)
        memcpy(copy, text, size);
    return copy;
}

int GlslTypesEqual(const GlslType *left, const GlslType *right)
{
    if (left == NULL || right == NULL || left->base != right->base ||
        left->len != right->len || left->rows != right->rows ||
        left->cols != right->cols || left->arraySize != right->arraySize)
    {
        return 0;
    }
    if ((left->structName == NULL) != (right->structName == NULL))
        return 0;
    if (left->structName != NULL &&
        strcmp(left->structName, right->structName)) return 0;
    if ((left->elementType == NULL) != (right->elementType == NULL))
        return 0;
    if (left->elementType != NULL)
        return GlslTypesEqual(left->elementType, right->elementType);
    return GlslTypeName(left) != NULL && GlslTypeName(right) != NULL;
}

int GlslIsSamplerType(const GlslType *type)
{
    if (type == NULL || type->len != 1 || type->rows != 0 ||
        type->cols != 0 || type->arraySize != 0 ||
        type->structName != NULL || type->elementType != NULL ||
        type->members != NULL)
    {
        return 0;
    }
    return type->base == GLSL_BASE_SAMPLER1D ||
           type->base == GLSL_BASE_SAMPLER2D ||
           type->base == GLSL_BASE_SAMPLER3D ||
           type->base == GLSL_BASE_SAMPLERCUBE;
}

static GlslDecl *GlslFindDeclList(GlslDecl *list, const void *identity)
{
    GlslDecl *decl;

    for (decl = list; decl != NULL; decl = decl->next) {
        if (decl->identity == identity)
            return decl;
    }
    return NULL;
}

GlslDecl *GlslFindDecl(GlslLowerContext *context,
                              const void *identity)
{
    GlslDecl *decl;
    GlslBinding *binding;
    GlslGeometryInputBinding *geometryInput;

    if (context->function != NULL) {
        decl = GlslFindDeclList(context->function->parameters, identity);
        if (decl == NULL)
            decl = GlslFindDeclList(context->function->locals, identity);
        if (decl != NULL)
            return decl;
    }
    decl = GlslFindDeclList(context->module->globals, identity);
    if (decl != NULL)
        return decl;
    for (geometryInput = context->geometryInputs; geometryInput != NULL;
         geometryInput = geometryInput->next)
    {
        if (geometryInput->source == identity)
            return geometryInput->declaration;
    }
    for (decl = context->module->structs; decl != NULL; decl = decl->next) {
        GlslDecl *member;

        member = GlslFindDeclList(decl->members, identity);
        if (member != NULL)
            return member;
    }
    for (binding = context->module->bindings; binding != NULL;
         binding = binding->next)
    {
        if (binding->declaration != NULL &&
            binding->declaration->identity == identity)
        {
            return binding->declaration;
        }
    }
    return NULL;
}

GlslFunction *GlslFindFunction(GlslModule *module,
                                      const void *identity)
{
    GlslFunction *function;

    for (function = module->functions; function != NULL;
         function = function->next)
    {
        if (function->identity == identity)
            return function;
    }
    return NULL;
}

void GlslAppendExpr(GlslExpr **list, GlslExpr *expression)
{
    GlslExpr *last;

    if (*list == NULL) {
        *list = expression;
        return;
    }
    for (last = *list; last->next != NULL; last = last->next)
        ;
    last->next = expression;
}

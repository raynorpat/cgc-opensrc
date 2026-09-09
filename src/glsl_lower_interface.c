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
// glsl_lower_interface.c
//

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <float.h>
#include <limits.h>

#include "glsl_lower_internal.h"



static int GlslSamplerDeclComesBefore(const GlslDecl *left,
                                      const GlslDecl *right)
{
    if (left->loc.file != right->loc.file)
        return left->loc.file < right->loc.file;
    if (left->loc.line != right->loc.line)
        return left->loc.line < right->loc.line;
    if (left->sourceOrdinal != right->sourceOrdinal)
        return left->sourceOrdinal < right->sourceOrdinal;
    return strcmp(left->name, right->name) < 0;
}

static void GlslInsertSamplerDecl(GlslDecl **list, GlslDecl *decl)
{
    GlslDecl **place;

    place = list;
    while (*place != NULL &&
           ((*place)->storage != GLSL_STORAGE_SAMPLER ||
            !GlslSamplerDeclComesBefore(decl, *place)))
    {
        place = &(*place)->next;
    }
    decl->next = *place;
    *place = decl;
}

GlslBinding *GlslFindUniformBinding(GlslModule *module,
                                           const Symbol *symbol)
{
    GlslBinding *binding;

    for (binding = module->bindings; binding != NULL;
         binding = binding->next)
    {
        if ((binding->storage == GLSL_STORAGE_UNIFORM ||
             binding->storage == GLSL_STORAGE_SAMPLER) &&
            binding->declaration != NULL &&
            binding->declaration->identity == symbol) return binding;
    }
    return NULL;
}

static int GlslCollectUniformSymbol(GlslLowerContext *context,
                                    Symbol *symbol)
{
    GlslBinding *binding;
    GlslDecl *decl;
    GlslType type;
    GlslStorage storage;
    const char *sourceName;
    const char *name;

    if (symbol == NULL || symbol->kind != VARIABLE_S ||
        GetDomain(symbol->type) != TYPE_DOMAIN_UNIFORM)
    {
        return 1;
    }
    if (GlslFindUniformBinding(context->module, symbol) != NULL)
        return 1;
    if (Cg->theHAL->IsTexobjBase(GetBase(symbol->type)) &&
        GetCategory(symbol->type) != TYPE_CATEGORY_SCALAR &&
        GetCategory(symbol->type) != TYPE_CATEGORY_SAMPLER)
    {
        context->statementLoc = symbol->loc;
        GlslRecordFailureKind(context, GLSL_ERROR_SAMPLER,
                              "sampler arrays or aggregates");
        return 0;
    }
    if (!GlslEnsureTypeAuto(context, symbol->type, &symbol->loc) ||
        !GlslLowerTypeAuto(context, symbol->type, &type,
                           &symbol->loc)) return 0;
    storage = Cg->theHAL->IsTexobjBase(GetBase(symbol->type)) ?
              GLSL_STORAGE_SAMPLER : GLSL_STORAGE_UNIFORM;
    sourceName = GetAtomString(atable, symbol->name);
    name = GlslAllocateSymbolNameForSource(context, symbol, sourceName,
                                           &symbol->loc);
    if (name == NULL)
        return 0;
    decl = GlslNewDecl(context->module, storage, type, name);
    binding = GlslNewBinding(context->module, storage,
                             name, "");
    if (decl == NULL || binding == NULL)
        return 0;
    decl->identity = symbol;
    GlslSetLoc(&decl->loc, &symbol->loc);
    decl->sourceOrdinal = symbol->sourceOrdinal;
    binding->declaration = decl;
    GlslSetLoc(&binding->loc, &symbol->loc);
    binding->sourceOrdinal = symbol->sourceOrdinal;
    if (storage == GLSL_STORAGE_SAMPLER)
        GlslInsertSamplerDecl(&context->module->globals, decl);
    else
        GlslAppendDecl(&context->module->globals, decl);
    GlslInsertBinding(&context->module->bindings, binding);
    return 1;
}

static int GlslCollectUniformList(GlslLowerContext *context,
                                  SymbolList *list)
{
    for (; list != NULL; list = list->next) {
        if (!GlslCollectUniformSymbol(context, list->symb))
            return 0;
    }
    return 1;
}

static int GlslCollectUniformsInExpr(GlslLowerContext *context,
                                     expr *source)
{
    if (source == NULL)
        return 1;
    switch (source->common.kind) {
    case SYMB_N:
        if (source->sym.op == VARIABLE_OP)
            return GlslCollectUniformSymbol(context, source->sym.symbol);
        return 1;
    case DECL_N:
    case CONST_N:
        return 1;
    case UNARY_N:
        return GlslCollectUniformsInExpr(context, source->un.arg);
    case BINARY_N:
        return GlslCollectUniformsInExpr(context, source->bin.left) &&
               GlslCollectUniformsInExpr(context, source->bin.right);
    case TRINARY_N:
        return GlslCollectUniformsInExpr(context, source->tri.arg1) &&
               GlslCollectUniformsInExpr(context, source->tri.arg2) &&
               GlslCollectUniformsInExpr(context, source->tri.arg3);
    default:
        return 0;
    }
}

static int GlslCollectUniformsInStatements(GlslLowerContext *context,
                                            stmt *source)
{
    for (; source != NULL; source = source->commonst.next) {
        switch (source->commonst.kind) {
        case EXPR_STMT:
            if (!GlslCollectUniformsInExpr(context, source->exprst.exp))
                return 0;
            break;
        case IF_STMT:
            if (!GlslCollectUniformsInExpr(context, source->ifst.cond) ||
                !GlslCollectUniformsInStatements(context,
                                                  source->ifst.thenstmt) ||
                !GlslCollectUniformsInStatements(context,
                                                  source->ifst.elsestmt))
                return 0;
            break;
        case WHILE_STMT:
        case DO_STMT:
            if (!GlslCollectUniformsInExpr(context, source->whilest.cond) ||
                !GlslCollectUniformsInStatements(context,
                                                  source->whilest.body))
                return 0;
            break;
        case FOR_STMT:
            if (!GlslCollectUniformsInStatements(context,
                                                  source->forst.init) ||
                !GlslCollectUniformsInExpr(context, source->forst.cond) ||
                !GlslCollectUniformsInStatements(context,
                                                  source->forst.step) ||
                !GlslCollectUniformsInStatements(context,
                                                  source->forst.body))
                return 0;
            break;
        case BLOCK_STMT:
            if (!GlslCollectUniformsInStatements(context,
                                                  source->blockst.body))
                return 0;
            break;
        case RETURN_STMT:
            if (!GlslCollectUniformsInExpr(context, source->returnst.exp))
                return 0;
            break;
        case DISCARD_STMT:
            if (!GlslCollectUniformsInExpr(context, source->discardst.cond))
                return 0;
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

int GlslCollectUniforms(GlslLowerContext *context, Symbol *program)
{
    GlslFunction *function;
    Symbol *symbol;

    if (!GlslCollectUniformList(context, Cg->theHAL->uniformParam) ||
        !GlslCollectUniformList(context, Cg->theHAL->uniformGlobal) ||
        !GlslCollectUniformsInStatements(context,
                                          program->details.fun.statements))
    {
        return 0;
    }
    for (function = context->module->functions; function != NULL;
         function = function->next)
    {
        symbol = (Symbol *) function->identity;
        if (!GlslCollectUniformsInStatements(context,
                                              symbol->details.fun.statements))
            return 0;
    }
    return 1;
}

int GlslValidateUniformLimit(GlslLowerContext *context)
{
    GlslBinding *binding;
    const char *resourceName;
    int componentCount;
    int limit;
    int used;

    limit = context->profile->limits.uniformComponents;
    if (limit < 0) {
        GlslRecordFailure(context, "uniform component limit");
        return 0;
    }
    resourceName = context->profile->stage == GLSL_STAGE_FRAGMENT ?
                   "fragment uniform components" :
                   context->profile->stage == GLSL_STAGE_GEOMETRY ?
                   "geometry uniform components" :
                   "vertex uniform components";
    used = 0;
    for (binding = context->module->bindings; binding != NULL;
         binding = binding->next)
    {
        if (binding->storage != GLSL_STORAGE_UNIFORM ||
            binding->declaration == NULL) continue;
        componentCount = GlslTypeComponentCount(
            &binding->declaration->type);
        if (componentCount <= 0) {
            context->statementLoc.file =
                (unsigned short) binding->loc.file;
            context->statementLoc.line =
                (unsigned short) binding->loc.line;
            GlslRecordFailureKind(context, GLSL_ERROR_UNSUPPORTED_TYPE,
                                  "uniform component count");
            return 0;
        }
        if (componentCount > limit - used) {
            context->module->errorLoc = binding->loc;
            context->module->resourceName = resourceName;
            context->module->resourceUsed =
                componentCount > INT_MAX - used ? INT_MAX :
                used + componentCount;
            context->module->resourceAvailable = limit;
            return 0;
        }
        used += componentCount;
    }
    return 1;
}

static int GlslRecordResourceLimit(GlslLowerContext *context,
                                   const GlslBinding *binding,
                                   const char *resourceName,
                                   int used, int available)
{
    context->module->errorKind = GLSL_ERROR_RESOURCE_LIMIT;
    context->module->errorLoc = binding->loc;
    context->module->resourceName = resourceName;
    context->module->resourceUsed = used;
    context->module->resourceAvailable = available;
    return 0;
}

static int GlslRecordResourceLimitAt(GlslLowerContext *context,
                                     const GlslLoc *loc,
                                     const char *resourceName,
                                     int used, int available)
{
    context->module->errorKind = GLSL_ERROR_RESOURCE_LIMIT;
    if (loc != NULL)
        context->module->errorLoc = *loc;
    context->module->resourceName = resourceName;
    context->module->resourceUsed = used;
    context->module->resourceAvailable = available;
    return 0;
}

static int GlslValidateGeometryInterfaceLimits(
    GlslLowerContext *context)
{
    GlslGeometryInputBinding *input;
    GlslGeometryInputBinding *prior;
    GlslGeometryOutputBinding *output;
    int inputComponents;
    int outputComponents;
    int components;
    int total;

    if (context->profile->limits.inputComponents <= 0 ||
        context->profile->limits.outputComponents <= 0 ||
        context->profile->limits.totalOutputComponents <= 0 ||
        context->module->geometry == NULL)
    {
        GlslRecordFailure(context, "geometry resource limits");
        return 0;
    }
    inputComponents = 0;
    for (input = context->geometryInputs; input != NULL;
         input = input->next)
    {
        for (prior = context->geometryInputs; prior != input;
             prior = prior->next)
        {
            if (prior->declaration == input->declaration)
                break;
        }
        if (prior != input)
            continue;
        if (input->declaration->type.elementType != NULL)
            components = GlslTypeComponentCount(
                input->declaration->type.elementType);
        else
            components = GlslTypeComponentCount(
                &input->declaration->type);
        if (components <= 0 || components > INT_MAX - inputComponents) {
            GlslRecordFailureKindAt(context, GLSL_ERROR_UNSUPPORTED_TYPE,
                "geometry input component count", NULL);
            return 0;
        }
        inputComponents += components;
        if (inputComponents > context->profile->limits.inputComponents) {
            return GlslRecordResourceLimitAt(context,
                &input->declaration->loc,
                "geometry input components", inputComponents,
                context->profile->limits.inputComponents);
        }
    }

    outputComponents = 0;
    for (output = context->geometryOutputs; output != NULL;
         output = output->next)
    {
        components = GlslTypeComponentCount(&output->declaration->type);
        if (components <= 0 || components > INT_MAX - outputComponents) {
            GlslRecordFailureKindAt(context, GLSL_ERROR_UNSUPPORTED_TYPE,
                "geometry output component count", NULL);
            return 0;
        }
        outputComponents += components;
        if (outputComponents >
            context->profile->limits.outputComponents)
        {
            return GlslRecordResourceLimitAt(context,
                &output->declaration->loc, "geometry output components",
                outputComponents,
                context->profile->limits.outputComponents);
        }
    }
    if (outputComponents > INT_MAX /
        context->module->geometry->maxOutputVertices)
    {
        total = INT_MAX;
    } else {
        total = outputComponents *
                context->module->geometry->maxOutputVertices;
    }
    if (total > context->profile->limits.totalOutputComponents) {
        return GlslRecordResourceLimitAt(context,
            &context->module->geometry->maxVerticesLoc,
            "geometry total output components", total,
            context->profile->limits.totalOutputComponents);
    }
    return 1;
}

int GlslValidateInterfaceLimits(GlslLowerContext *context)
{
    GlslBinding *binding;
    int attributes;
    int interfaceComponents;
    int fragmentColors;
    int components;
    const char *componentResource;

    if (context->profile->stage == GLSL_STAGE_GEOMETRY)
        return GlslValidateGeometryInterfaceLimits(context);
    attributes = 0;
    interfaceComponents = 0;
    fragmentColors = 0;
    componentResource =
        context->profile->stage == GLSL_STAGE_VERTEX ?
        "vertex output components" : "fragment input components";
    for (binding = context->module->bindings; binding != NULL;
         binding = binding->next)
    {
        if (binding->declaration == NULL)
            continue;
        if (context->profile->stage == GLSL_STAGE_VERTEX &&
            binding->storage == GLSL_STORAGE_INPUT)
        {
            attributes++;
            if (attributes > context->profile->limits.attributes) {
                return GlslRecordResourceLimit(context, binding,
                    "vertex attributes", attributes,
                    context->profile->limits.attributes);
            }
        } else if (context->profile->stage == GLSL_STAGE_VERTEX &&
                   binding->storage == GLSL_STORAGE_OUTPUT)
        {
            components = GlslTypeComponentCount(
                &binding->declaration->type);
            if (components <= 0 ||
                components > INT_MAX - interfaceComponents)
            {
                context->statementLoc.file =
                    (unsigned short) binding->loc.file;
                context->statementLoc.line =
                    (unsigned short) binding->loc.line;
                GlslRecordFailureKind(context,
                                      GLSL_ERROR_UNSUPPORTED_TYPE,
                                      "interface value");
                return 0;
            }
            interfaceComponents += components;
            if (context->profile->limits.outputComponents <= 0 ||
                interfaceComponents >
                context->profile->limits.outputComponents)
            {
                return GlslRecordResourceLimit(context, binding,
                    componentResource, interfaceComponents,
                    context->profile->limits.outputComponents);
            }
        } else if (context->profile->stage == GLSL_STAGE_FRAGMENT &&
                   binding->storage == GLSL_STORAGE_INPUT)
        {
            components = GlslTypeComponentCount(
                &binding->declaration->type);
            if (components <= 0 ||
                components > INT_MAX - interfaceComponents)
            {
                context->statementLoc.file =
                    (unsigned short) binding->loc.file;
                context->statementLoc.line =
                    (unsigned short) binding->loc.line;
                GlslRecordFailureKind(context,
                                      GLSL_ERROR_UNSUPPORTED_TYPE,
                                      "interface value");
                return 0;
            }
            interfaceComponents += components;
            if (context->profile->limits.inputComponents <= 0 ||
                interfaceComponents >
                context->profile->limits.inputComponents)
            {
                return GlslRecordResourceLimit(context, binding,
                    componentResource, interfaceComponents,
                    context->profile->limits.inputComponents);
            }
        } else if (context->profile->stage == GLSL_STAGE_FRAGMENT &&
                   binding->isOutput &&
                   binding->storage == GLSL_STORAGE_OUTPUT &&
                   binding->semantic != NULL &&
                   !strncmp(binding->semantic, "COLOR", 5))
        {
            /* Fragment COLOR outputs are ordinary user interfaces in
             * core 1.50; the focused one-color-output limit counts
             * them by their canonical COLOR semantic root. */
            fragmentColors++;
            if (fragmentColors >
                context->profile->limits.colorOutputs)
            {
                return GlslRecordResourceLimit(context, binding,
                    "fragment color outputs", fragmentColors,
                    context->profile->limits.colorOutputs);
            }
        }
    }
    return 1;
}

int GlslAllocateTextureUnits(GlslLowerContext *context)
{
    GlslBinding *binding;
    Binding *sourceBinding;
    Symbol *symbol;
    const char *resourceName;
    char unitText[32];
    int limit;
    int used;

    limit = context->profile->limits.textureUnits;
    resourceName = context->profile->stage == GLSL_STAGE_FRAGMENT ?
                   "fragment texture units" :
                   context->profile->stage == GLSL_STAGE_GEOMETRY ?
                   "geometry texture units" : "vertex texture units";
    used = 0;
    for (binding = context->module->bindings; binding != NULL;
         binding = binding->next)
    {
        if (binding->storage != GLSL_STORAGE_SAMPLER ||
            binding->declaration == NULL) continue;
        if (used >= limit) {
            context->module->errorLoc = binding->loc;
            context->module->resourceName = resourceName;
            context->module->resourceUsed = used + 1;
            context->module->resourceAvailable = limit;
            return 0;
        }
        sprintf(unitText, "%d", used);
        binding->semantic = GlslCopyText(context->module, unitText);
        if (binding->semantic == NULL)
            return 0;
        symbol = (Symbol *) binding->declaration->identity;
        sourceBinding = symbol != NULL ? symbol->details.var.bind : NULL;
        if (sourceBinding == NULL) {
            context->statementLoc.file =
                (unsigned short) binding->loc.file;
            context->statementLoc.line =
                (unsigned short) binding->loc.line;
            GlslRecordFailureKind(context, GLSL_ERROR_SAMPLER,
                                  "sampler binding");
            return 0;
        }
        sourceBinding->none.kind = BK_TEXUNIT;
        sourceBinding->none.properties =
            BIND_IS_BOUND | BIND_INPUT | BIND_UNIFORM;
        sourceBinding->none.base = GetBase(symbol->type);
        sourceBinding->none.size = symbol->type->co.size;
        sourceBinding->texunit.unitno = used;
        used++;
    }
    return 1;
}

static int GlslBindingComesBefore(const GlslBinding *left,
                                  const GlslBinding *right)
{
    if (left->storage != right->storage)
        return left->storage < right->storage;
    if (left->isOutput != right->isOutput)
        return left->isOutput < right->isOutput;
    if (left->loc.file != right->loc.file)
        return left->loc.file < right->loc.file;
    if (left->loc.line != right->loc.line)
        return left->loc.line < right->loc.line;
    if (left->sourceOrdinal != right->sourceOrdinal)
        return left->sourceOrdinal < right->sourceOrdinal;
    return strcmp(left->name, right->name) < 0;
}

void GlslInsertBinding(GlslBinding **list, GlslBinding *binding)
{
    GlslBinding **place;

    place = list;
    while (*place != NULL && !GlslBindingComesBefore(binding, *place))
        place = &(*place)->next;
    binding->next = *place;
    *place = binding;
}

static int GlslHasInterfaceBinding(GlslLowerContext *context,
                                   const char *interfaceKey,
                                   int isOutput)
{
    GlslBinding *binding;

    for (binding = context->module->bindings; binding != NULL;
         binding = binding->next)
    {
        if (binding->interfaceKey != NULL &&
            binding->isOutput == isOutput &&
            !strcmp(binding->interfaceKey, interfaceKey)) return 1;
    }
    return 0;
}

static const char *GlslReservedInterfaceName(GlslLowerContext *context,
                                             const char *interfaceKey,
                                             int isOutput)
{
    GlslInterfaceSource *source;

    for (source = context->interfaceSources; source != NULL;
         source = source->next)
    {
        if (source->isOutput == isOutput &&
            !strcmp(source->interfaceKey, interfaceKey))
        {
            return source->reservedName;
        }
    }
    return NULL;
}

GlslDecl *GlslLowerInterface(GlslLowerContext *context,
                                    Symbol *member)
{
    Binding *sourceBinding;
    GlslBinding *binding;
    GlslDecl *decl;
    GlslType type;
    GlslStorage storage;
    GlslInterpolation interpolation;
    const char *canonical;
    const char *interfaceName;
    const char *name;
    char generatedName[256];
    int isOutput;

    decl = GlslFindDecl(context, member);
    if (decl != NULL)
        return decl;
    sourceBinding = member->details.var.bind;
    if (sourceBinding == NULL || sourceBinding->none.kind != BK_CONNECTOR ||
        !(sourceBinding->none.properties & BIND_IS_BOUND) ||
        sourceBinding->conn.rname == 0)
    {
        return NULL;
    }
    isOutput = (sourceBinding->none.properties & BIND_OUTPUT) != 0;
    if (isOutput ==
        ((sourceBinding->none.properties & BIND_INPUT) != 0)) return NULL;
    canonical = GetAtomString(atable, sourceBinding->conn.rname);
    interfaceName = GlslCanonicalInterfaceName(context->profile,
        sourceBinding->conn.rname, isOutput);
    if (canonical == NULL || interfaceName == NULL ||
        !GlslLowerTypeAuto(context, member->type, &type,
                           &member->loc)) return NULL;
    /* Interstage interfaces are directional: an input of either focused
     * stage is "in" and an output of either stage is "out".  Integer
     * interfaces interpolate flat; matching producer/consumer
     * declarations derive the identical qualifier from the identical
     * semantic and type. */
    storage = isOutput ? GLSL_STORAGE_OUTPUT : GLSL_STORAGE_INPUT;
    interpolation = GlslInterpolationForType(&type);
    if (GlslHasInterfaceBinding(context, interfaceName, isOutput)) {
        context->statementLoc = member->loc;
        GlslRecordFailureKind(context, GLSL_ERROR_INTERFACE_CONFLICT,
                              interfaceName);
        return NULL;
    }
    if (!strncmp(interfaceName, "gl_", 3)) {
        storage = GLSL_STORAGE_BUILTIN;
        name = interfaceName;
        interpolation = GLSL_INTERPOLATION_DEFAULT;
    } else {
        if (strlen(interfaceName) + 4 > sizeof(generatedName))
            return NULL;
        sprintf(generatedName, "cg_%s", interfaceName);
        name = GlslReservedInterfaceName(context, interfaceName,
                                         isOutput);
        if (name == NULL) {
            name = GlslAllocateSymbolNameForSource(context, member,
                generatedName, &member->loc);
        }
        if (name == NULL)
            return NULL;
    }
    decl = GlslNewDecl(context->module, storage, type, name);
    binding = GlslNewBinding(context->module, storage, name, canonical);
    if (decl == NULL || binding == NULL)
        return NULL;
    decl->identity = member;
    decl->interpolation = interpolation;
    GlslSetLoc(&decl->loc, &member->loc);
    decl->sourceOrdinal = member->sourceOrdinal;
    binding->declaration = decl;
    binding->interfaceKey = interfaceName;
    binding->isOutput = isOutput;
    binding->interpolation = interpolation;
    GlslSetLoc(&binding->loc, &member->loc);
    binding->sourceOrdinal = member->sourceOrdinal;
    if (storage != GLSL_STORAGE_BUILTIN)
        GlslAppendDecl(&context->module->globals, decl);
    GlslInsertBinding(&context->module->bindings, binding);
    return decl;
}

static int GlslValidateInterfaceSource(GlslLowerContext *context,
                                       Symbol *source, int isOutput)
{
    GlslInterfaceSource *current;
    GlslInterfaceSource *record;
    Binding *binding;
    const char *interfaceKey;
    char generatedName[256];
    int reserveVarying;

    binding = source->details.var.bind;
    if (binding == NULL || binding->none.kind != BK_CONNECTOR ||
        !(binding->none.properties & BIND_IS_BOUND) ||
        binding->conn.rname == 0) return 1;
    interfaceKey = GlslCanonicalInterfaceName(
        context->profile, binding->conn.rname, isOutput);
    if (interfaceKey == NULL)
        return 1;
    for (current = context->interfaceSources; current != NULL;
         current = current->next)
    {
        if (current->isOutput != isOutput ||
            strcmp(current->interfaceKey, interfaceKey)) continue;
        if (current->source == source)
            return 1;
        context->statementLoc = source->loc;
        GlslRecordFailureKind(context, GLSL_ERROR_INTERFACE_CONFLICT,
                              interfaceKey);
        return 0;
    }
    record = (GlslInterfaceSource *) context->module->alloc(
        context->module->allocArg, sizeof(GlslInterfaceSource));
    if (record == NULL)
        return 0;
    record->source = source;
    record->interfaceKey = interfaceKey;
    record->reservedName = NULL;
    record->isOutput = isOutput;
    reserveVarying = (context->profile->stage == GLSL_STAGE_VERTEX &&
                      isOutput) ||
                     (context->profile->stage == GLSL_STAGE_FRAGMENT &&
                      !isOutput);
    if (reserveVarying && strncmp(interfaceKey, "gl_", 3)) {
        if (strlen(interfaceKey) + 4 > sizeof(generatedName))
            return 0;
        sprintf(generatedName, "cg_%s", interfaceKey);
        record->reservedName = GlslAllocateNameForSource(context,
            generatedName, &source->loc);
        if (record->reservedName == NULL)
            return 0;
    }
    record->next = context->interfaceSources;
    context->interfaceSources = record;
    return 1;
}

static int GlslValidateInterfaceType(GlslLowerContext *context,
                                     Type *type, Symbol *source,
                                     int isOutput)
{
    Symbol *member;

    if (GetCategory(type) != TYPE_CATEGORY_STRUCT)
        return GlslValidateInterfaceSource(context, source, isOutput);
    if (type->str.members == NULL)
        return 1;
    for (member = type->str.members->symbols; member != NULL;
         member = member->next)
    {
        if (!GlslValidateInterfaceSource(context, member, isOutput))
            return 0;
    }
    return 1;
}

int GlslValidateEntryInterfaces(GlslLowerContext *context,
                                       Symbol *program)
{
    Symbol *formal;
    Symbol *member;
    Type *result;
    int isOutput;

    for (formal = program->details.fun.params; formal != NULL;
         formal = formal->next)
    {
        if (GetDomain(formal->type) == TYPE_DOMAIN_UNIFORM)
            continue;
        isOutput = (GetQualifiers(formal->type) &
                    TYPE_QUALIFIER_OUT) != 0;
        if (!GlslValidateInterfaceType(context, formal->type,
                                       formal, isOutput)) return 0;
    }
    result = program->type->fun.rettype;
    if (GetCategory(result) != TYPE_CATEGORY_STRUCT ||
        result->str.members == NULL) return 1;
    for (member = result->str.members->symbols; member != NULL;
         member = member->next)
    {
        if (!GlslValidateInterfaceSource(context, member, 1))
            return 0;
    }
    return 1;
}

/*
 * GlslIRCollectUniformsInExpr() - Uniform references inside statement
 *          trees join the binding metadata scan so referenced uniforms
 *          are never missed.  GlslCollectUniformSymbol filters
 *          non-uniform symbols itself.
 */

static int GlslIRCollectUniformsInExpr(GlslLowerContext *context,
                                       const CgIRExpr *expr)
{
    const CgIRExpr *argument;

    if (expr == NULL)
        return 1;
    switch (expr->kind) {
    case CGIR_EXPR_SYMBOL:
        return GlslCollectUniformSymbol(context, expr->u.symbol);
    case CGIR_EXPR_CONSTANT:
        return 1;
    case CGIR_EXPR_MEMBER:
        return GlslIRCollectUniformsInExpr(context, expr->u.member.object);
    case CGIR_EXPR_INDEX:
        return GlslIRCollectUniformsInExpr(context,
                                           expr->u.index.object) &&
               GlslIRCollectUniformsInExpr(context, expr->u.index.index);
    case CGIR_EXPR_LENGTH:
        return GlslIRCollectUniformsInExpr(context,
                                           expr->u.length.object);
    case CGIR_EXPR_SWIZZLE:
        return GlslIRCollectUniformsInExpr(context,
                                           expr->u.swizzle.object);
    case CGIR_EXPR_CONSTRUCT:
        for (argument = expr->u.construct.arguments; argument != NULL;
             argument = argument->next)
        {
            if (!GlslIRCollectUniformsInExpr(context, argument)) return 0;
        }
        return 1;
    case CGIR_EXPR_CAST:
        return GlslIRCollectUniformsInExpr(context, expr->u.cast.operand);
    case CGIR_EXPR_UNARY:
        return GlslIRCollectUniformsInExpr(context,
                                           expr->u.unary.operand);
    case CGIR_EXPR_BINARY:
        return GlslIRCollectUniformsInExpr(context,
                                           expr->u.binary.left) &&
               GlslIRCollectUniformsInExpr(context,
                                           expr->u.binary.right);
    case CGIR_EXPR_ASSIGN:
        return GlslIRCollectUniformsInExpr(context,
                                           expr->u.assign.target) &&
               GlslIRCollectUniformsInExpr(context,
                                           expr->u.assign.value);
    case CGIR_EXPR_CONDITIONAL:
        return GlslIRCollectUniformsInExpr(context,
                                           expr->u.conditional.condition) &&
               GlslIRCollectUniformsInExpr(context,
                                           expr->u.conditional.trueExpr) &&
               GlslIRCollectUniformsInExpr(context,
                                           expr->u.conditional.falseExpr);
    case CGIR_EXPR_CALL:
        for (argument = expr->u.call.arguments; argument != NULL;
             argument = argument->next)
        {
            if (!GlslIRCollectUniformsInExpr(context, argument)) return 0;
        }
        return 1;
    case CGIR_EXPR_INTERFACE_CALL:
        if (!GlslIRCollectUniformsInExpr(context,
                                         expr->u.interfaceCall.receiver))
            return 0;
        for (argument = expr->u.interfaceCall.arguments; argument != NULL;
             argument = argument->next)
        {
            if (!GlslIRCollectUniformsInExpr(context, argument)) return 0;
        }
        return 1;
    case CGIR_EXPR_INTRINSIC:
        for (argument = expr->u.intrinsicCall.arguments; argument != NULL;
             argument = argument->next)
        {
            if (!GlslIRCollectUniformsInExpr(context, argument)) return 0;
        }
        return 1;
    default:
        return 1;
    }
} // GlslIRCollectUniformsInExpr

static int GlslIRCollectUniformsInStmt(GlslLowerContext *context,
                                       const CgIRStmt *stmt)
{
    if (stmt == NULL)
        return 1;
    switch (stmt->kind) {
    case CGIR_STMT_BLOCK:
        for (stmt = stmt->u.block; stmt != NULL; stmt = stmt->next) {
            if (!GlslIRCollectUniformsInStmt(context, stmt)) return 0;
        }
        return 1;
    case CGIR_STMT_DECL:
        return stmt->u.decl->initializer == NULL ||
               GlslIRCollectUniformsInExpr(context,
                                           stmt->u.decl->initializer);
    case CGIR_STMT_EXPR:
        return GlslIRCollectUniformsInExpr(context, stmt->u.expression);
    case CGIR_STMT_IF:
        return GlslIRCollectUniformsInExpr(context,
                                           stmt->u.ifStmt.condition) &&
               GlslIRCollectUniformsInStmt(context,
                                           stmt->u.ifStmt.trueBranch) &&
               GlslIRCollectUniformsInStmt(context,
                                           stmt->u.ifStmt.falseBranch);
    case CGIR_STMT_WHILE:
    case CGIR_STMT_DO:
        return GlslIRCollectUniformsInExpr(context,
                                           stmt->u.loop.condition) &&
               GlslIRCollectUniformsInStmt(context, stmt->u.loop.body);
    case CGIR_STMT_FOR:
        return GlslIRCollectUniformsInStmt(context,
                                           stmt->u.forStmt.init) &&
               (stmt->u.forStmt.condition == NULL ||
                GlslIRCollectUniformsInExpr(context,
                                            stmt->u.forStmt.condition)) &&
               (stmt->u.forStmt.step == NULL ||
                GlslIRCollectUniformsInExpr(context,
                                            stmt->u.forStmt.step)) &&
               GlslIRCollectUniformsInStmt(context, stmt->u.forStmt.body);
    case CGIR_STMT_RETURN:
        return stmt->u.returnExpr == NULL ||
               GlslIRCollectUniformsInExpr(context, stmt->u.returnExpr);
    case CGIR_STMT_DISCARD:
        return stmt->u.discard.condition == NULL ||
               GlslIRCollectUniformsInExpr(context,
                                           stmt->u.discard.condition);
    case CGIR_STMT_GEOMETRY_EMIT:
    case CGIR_STMT_GEOMETRY_FLAT:
        {
            const CgIRGeometryValue *value;

            for (value = stmt->u.geometry.values; value != NULL;
                 value = value->next)
            {
                if (!GlslIRCollectUniformsInExpr(context, value->value))
                    return 0;
            }
        }
        return 1;
    default:
        return 1;
    }
} // GlslIRCollectUniformsInStmt

/*
 * GlslIRValidateEntryInterfaces() - Reserve canonical interface names
 *          for every varying-domain entry formal (and its members)
 *          before bodies allocate names.  The void entry result needs
 *          no output-side reservation: output members surface through
 *          $vout writes and conflict-check live.
 */

int GlslIRValidateEntryInterfaces(GlslLowerContext *context,
                                         const CgIRFunction *entry)
{
    const CgIRDecl *param;
    Symbol *member;
    int isOutput;

    for (param = entry->parameters; param != NULL; param = param->next) {
        if (param->domain == CGIR_DOMAIN_UNIFORM)
            continue;
        if (param->symbol == NULL)
            continue;
        isOutput = (GetQualifiers(param->type) &
                    TYPE_QUALIFIER_OUT) != 0;
        if (GetCategory(param->type) != TYPE_CATEGORY_STRUCT) {
            if (!GlslValidateInterfaceSource(context, param->symbol,
                                             isOutput))
            {
                return 0;
            }
            continue;
        }
        if (param->type->str.members == NULL)
            continue;
        for (member = param->type->str.members->symbols; member != NULL;
             member = member->next)
        {
            if (!GlslValidateInterfaceSource(context, member, isOutput))
                return 0;
        }
    }
    /* The void entry result's output members surface only through $vout
     * writes; reserve their canonical names here exactly as the legacy
     * result-structure walk did.  Members whose key a formal already
     * claimed are skipped (they are the same interface value).  The
     * input connector covers global varyings read through $vin. */
    {
        const Symbol *connMember;

        if (Cg->theHAL->varyingOut != NULL &&
            Cg->theHAL->varyingOut->type->str.members != NULL)
        {
            for (connMember =
                     Cg->theHAL->varyingOut->type->str.members->symbols;
                 connMember != NULL; connMember = connMember->next)
            {
                Binding *bind = connMember->details.var.bind;
                const char *key;
                const GlslInterfaceSource *src;

                if (bind == NULL || bind->none.kind != BK_CONNECTOR ||
                    !(bind->none.properties & BIND_IS_BOUND) ||
                    bind->conn.rname == 0)
                    continue;
                key = GlslCanonicalInterfaceName(context->profile,
                                                 bind->conn.rname, 1);
                if (key == NULL)
                    continue;
                {
                    int seen = 0;

                    for (src = context->interfaceSources; src != NULL;
                         src = src->next)
                    {
                        if (src->isOutput == 1 &&
                            !strcmp(src->interfaceKey, key))
                        {
                            seen = 1;
                            break;
                        }
                    }
                    if (seen)
                        continue;
                }
                if (!GlslValidateInterfaceSource(context,
                    (Symbol *) connMember, 1)) return 0;
            }
        }
        if (Cg->theHAL->varyingIn != NULL &&
            Cg->theHAL->varyingIn->type->str.members != NULL)
        {
            for (connMember =
                     Cg->theHAL->varyingIn->type->str.members->symbols;
                 connMember != NULL; connMember = connMember->next)
            {
                Binding *bind = connMember->details.var.bind;
                const char *key;
                const GlslInterfaceSource *src;

                if (bind == NULL || bind->none.kind != BK_CONNECTOR ||
                    !(bind->none.properties & BIND_IS_BOUND) ||
                    bind->conn.rname == 0)
                    continue;
                key = GlslCanonicalInterfaceName(context->profile,
                                                 bind->conn.rname, 0);
                if (key == NULL)
                    continue;
                {
                    int seen = 0;

                    for (src = context->interfaceSources; src != NULL;
                         src = src->next)
                    {
                        if (src->isOutput == 0 &&
                            !strcmp(src->interfaceKey, key))
                        {
                            seen = 1;
                            break;
                        }
                    }
                    if (seen)
                        continue;
                }
                if (!GlslValidateInterfaceSource(context,
                    (Symbol *) connMember, 0)) return 0;
            }
        }
    }
    return 1;
} // GlslIRValidateEntryInterfaces

/*
 * GlslIRCollectUniforms() - Binding metadata first (every bound uniform
 *          emits even when unreferenced), then the entry body and each
 *          collected helper body, mirroring the legacy scan order.
 */

int GlslIRCollectUniforms(GlslLowerContext *context,
                                 const CgIRFunction *entry)
{
    GlslFunction *function;

    if (!GlslCollectUniformList(context, Cg->theHAL->uniformParam) ||
        !GlslCollectUniformList(context, Cg->theHAL->uniformGlobal))
    {
        return 0;
    }
    if (!GlslIRCollectUniformsInStmt(context, entry->body)) return 0;
    for (function = context->module->functions; function != NULL;
         function = function->next)
    {
        const CgIRFunction *irFunction;

        irFunction = GlslIRFindIRFunction(context->source,
                                          function->identity);
        if (irFunction == NULL)
            continue;
        if (!GlslIRCollectUniformsInStmt(context, irFunction->body))
            return 0;
    }
    return 1;
} // GlslIRCollectUniforms

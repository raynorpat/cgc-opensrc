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


GlslExpr *GlslLowerExprChain(GlslLowerContext *context, expr *source,
                                    opcode listOp)
{
    GlslExpr *list;
    GlslExpr *item;

    list = NULL;
    for (; source != NULL; source = source->bin.right) {
        if (source->common.kind != BINARY_N || source->bin.op != listOp) {
            GlslRecordFailure(context, "GLSL expression list");
            return NULL;
        }
        item = GlslLowerExpr(context, source->bin.left);
        if (item == NULL)
            return NULL;
        GlslAppendExpr(&list, item);
    }
    return list;
}

static GlslExpr *GlslLowerTextureArguments(GlslLowerContext *context,
                                           expr *source)
{
    GlslExpr *sampler;
    GlslExpr *coord;
    GlslDecl *decl;
    expr *samplerSource;

    if (source == NULL || source->common.kind != BINARY_N ||
        source->bin.op != FUN_ARG_OP || source->bin.right == NULL ||
        source->bin.right->common.kind != BINARY_N ||
        source->bin.right->bin.op != FUN_ARG_OP ||
        source->bin.right->bin.right != NULL)
    {
        GlslRecordFailureKind(context, GLSL_ERROR_INTRINSIC,
                              "texture intrinsic");
        return NULL;
    }
    samplerSource = source->bin.left;
    if (samplerSource == NULL || samplerSource->common.kind != SYMB_N ||
        samplerSource->sym.op != VARIABLE_OP ||
        samplerSource->sym.symbol == NULL)
    {
        GlslRecordFailureKind(context, GLSL_ERROR_SAMPLER,
            "texture sampler argument must be a direct bound uniform");
        return NULL;
    }
    decl = GlslFindDecl(context, samplerSource->sym.symbol);
    if (decl == NULL || decl->storage != GLSL_STORAGE_SAMPLER ||
        !GlslIsSamplerType(&decl->type))
    {
        GlslRecordFailureKind(context, GLSL_ERROR_SAMPLER,
            "texture sampler argument must be a direct bound uniform");
        return NULL;
    }
    sampler = GlslNewExpr(context->module, GLSL_EXPR_SYMBOL, decl->type);
    if (sampler == NULL)
        return NULL;
    sampler->u.symbol = decl;
    coord = GlslLowerExpr(context, source->bin.right->bin.left);
    if (coord == NULL)
        return NULL;
    sampler->next = coord;
    return sampler;
}

GlslExpr *GlslNewLiteral(GlslLowerContext *context, GlslBase base,
    int intValue, float floatValue)
{
    GlslExprKind kind;
    GlslExpr *target;
    GlslType type;

    type = GlslNumericType(base, 1);
    if (base == GLSL_BASE_FLOAT)
        kind = GLSL_EXPR_FLOAT;
    else if (base == GLSL_BASE_BOOL)
        kind = GLSL_EXPR_BOOL;
    else
        kind = GLSL_EXPR_INT;
    target = GlslNewExpr(context->module, kind, type);
    if (target == NULL)
        return NULL;
    if (kind == GLSL_EXPR_FLOAT)
        target->u.literalFloat = floatValue;
    else if (kind == GLSL_EXPR_BOOL)
        target->u.literalBool = intValue != 0;
    else
        target->u.literalInt = intValue;
    return target;
}

static GlslExpr *GlslLowerConstant(GlslLowerContext *context, expr *source,
                                   const GlslType *type)
{
    GlslExpr *target;
    GlslExpr *item;
    int i;

    if (type->base == GLSL_BASE_FLOAT) {
        for (i = 0; i < type->len; i++) {
            if (source->co.val[i].value.f != source->co.val[i].value.f ||
                source->co.val[i].value.f > FLT_MAX ||
                source->co.val[i].value.f < -FLT_MAX)
            {
                GlslRecordFailure(context,
                                  "non-finite floating-point constant");
                return NULL;
            }
        }
    }

    if (type->len == 1) {
        if (type->base == GLSL_BASE_FLOAT)
            return GlslNewLiteral(context, type->base, 0, source->co.val[0].value.f);
        return GlslNewLiteral(context, type->base, (int) source->co.val[0].value.i, 0.0f);
    }
    target = GlslNewExpr(context->module, GLSL_EXPR_CONSTRUCT, *type);
    if (target == NULL)
        return NULL;
    for (i = 0; i < type->len; i++) {
        if (type->base == GLSL_BASE_FLOAT)
            item = GlslNewLiteral(context, type->base, 0, source->co.val[i].value.f);
        else
            item = GlslNewLiteral(context, type->base, (int) source->co.val[i].value.i, 0.0f);
        if (item == NULL)
            return NULL;
        GlslAppendExpr(&target->u.construct.arguments, item);
    }
    return target;
}

GlslExpr *GlslNewSwizzle(GlslLowerContext *context, GlslExpr *object,
    const GlslType *type, const char *mask)
{
    GlslExpr *target;

    target = GlslNewExpr(context->module, GLSL_EXPR_SWIZZLE, *type);
    if (target != NULL) {
        target->u.swizzle.object = object;
        target->u.swizzle.mask = GlslCopyText(context->module, mask);
        if (target->u.swizzle.mask == NULL)
            return NULL;
    }
    return target;
}

static GlslExpr *GlslLowerSwizzle(GlslLowerContext *context, expr *source,
                                  const GlslType *type)
{
    GlslExpr *object;
    GlslExpr *target;
    char maskText[5];
    int count;
    int i;
    int mask;

    object = GlslLowerExpr(context, source->un.arg);
    if (object == NULL)
        return NULL;
    count = SUBOP_GET_S2(source->un.subop);
    if (count == 0)
        count = 1;
    if (count < 1 || count > 4)
        return NULL;
    mask = SUBOP_GET_MASK(source->un.subop);
    for (i = count - 1; i >= 0; i--)
        maskText[i] = "xyzw"[(mask >> (i * 2)) & 3];
    maskText[count] = '\0';
    if (object->type.len == 1) {
        if (type->len == 1)
            return object;
        target = GlslNewExpr(context->module, GLSL_EXPR_CONSTRUCT, *type);
        if (target != NULL)
            target->u.construct.arguments = object;
        return target;
    }
    return GlslNewSwizzle(context, object, type, maskText);
}

static GlslExpr *GlslNewIndexLiteral(GlslLowerContext *context,
                                     GlslExpr *object,
                                     const GlslType *type, int index)
{
    GlslExpr *target;
    GlslExpr *literal;
    GlslType intType;

    intType = GlslNumericType(GLSL_BASE_INT, 1);
    literal = GlslNewExpr(context->module, GLSL_EXPR_INT, intType);
    target = GlslNewExpr(context->module, GLSL_EXPR_INDEX, *type);
    if (literal == NULL || target == NULL)
        return NULL;
    literal->u.literalInt = index;
    target->u.index.object = object;
    target->u.index.index = literal;
    return target;
}

static GlslExpr *GlslMatrixComponent(GlslLowerContext *context,
    GlslExpr *matrix, int row, int column)
{
    GlslExpr *columnExpr;
    GlslType columnType;
    GlslType scalarType;

    if (matrix == NULL || matrix->type.rows < 2 ||
        matrix->type.rows != matrix->type.cols || row < 0 || column < 0 ||
        row >= matrix->type.rows || column >= matrix->type.cols)
    {
        return NULL;
    }
    columnType = GlslNumericType(GLSL_BASE_FLOAT, matrix->type.rows);
    scalarType = GlslNumericType(GLSL_BASE_FLOAT, 1);
    columnExpr = GlslNewIndexLiteral(context, matrix, &columnType, column);
    if (columnExpr == NULL)
        return NULL;
    return GlslNewIndexLiteral(context, columnExpr, &scalarType, row);
}

int GlslMatrixSelectorCount(const expr *source)
{
    int count;

    if (source == NULL || source->common.kind != UNARY_N ||
        source->un.op != SWIZMAT_Z_OP) return 0;
    count = SUBOP_GET_T2(source->un.subop);
    return count == 0 ? 1 : count;
}

static GlslExpr *GlslMatrixMaskComponent(GlslLowerContext *context,
    GlslExpr *matrix, int mask, int component)
{
    int selector;
    int row;
    int column;

    selector = (mask >> (component * 4)) & 15;
    row = (selector >> 2) & 3;
    column = selector & 3;
    return GlslMatrixComponent(context, matrix, row, column);
}

GlslExpr *GlslMatrixSelectorComponent(GlslLowerContext *context,
    GlslExpr *matrix, const expr *selectorSource, int component)
{
    if (selectorSource == NULL)
        return NULL;
    return GlslMatrixMaskComponent(context, matrix,
        SUBOP_GET_MASK16(selectorSource->un.subop), component);
}


GlslExpr *GlslLowerMatrixSwizzle(GlslLowerContext *context,
    expr *source, const GlslType *type)
{
    GlslExpr *object;
    GlslExpr *target;
    GlslExpr *component;
    GlslMatrixSelectorHelper *helper;
    int count;
    int mask;
    int selector;
    int row;
    int column;
    int i;

    object = GlslLowerExpr(context, source->un.arg);
    if (object == NULL)
        return NULL;
    count = SUBOP_GET_T2(source->un.subop);
    if (count == 0)
        count = 1;
    if (count < 1 || count > 4)
        return NULL;
    mask = SUBOP_GET_MASK16(source->un.subop);
    if (count == 1) {
        selector = mask & 15;
        row = (selector >> 2) & 3;
        column = selector & 3;
        return GlslMatrixComponent(context, object, row, column);
    }
    if (source->un.arg->common.HasSideEffects) {
        helper = GlslGetMatrixSelectorHelper(context,
            GLSL_MATRIX_SELECTOR_GET, &object->type, type, count, mask);
        if (helper == NULL)
            return NULL;
        target = GlslNewExpr(context->module, GLSL_EXPR_CALL, *type);
        if (target == NULL)
            return NULL;
        target->u.call.name = helper->function->name;
        target->u.call.arguments = object;
        return target;
    }
    target = GlslNewExpr(context->module, GLSL_EXPR_CONSTRUCT, *type);
    if (target == NULL)
        return NULL;
    for (i = 0; i < count; i++) {
        selector = (mask >> (i * 4)) & 15;
        row = (selector >> 2) & 3;
        column = selector & 3;
        component = GlslMatrixComponent(context, object, row, column);
        if (component == NULL)
            return NULL;
        GlslAppendExpr(&target->u.construct.arguments, component);
    }
    return target;
}

int GlslMatrixNumericParameterType(const GlslType *type)
{
    return type != NULL &&
           (type->base == GLSL_BASE_FLOAT ||
            type->base == GLSL_BASE_INT) &&
           type->len >= 1 && type->len <= 4 && type->rows == 0 &&
           type->cols == 0 && type->arraySize == 0 &&
           type->structName == NULL && type->elementType == NULL &&
           type->members == NULL;
}

static GlslMatrixHelper *GlslFindMatrixHelper(GlslLowerContext *context,
    const GlslType *result, const GlslType *parameters, int parameterCount)
{
    GlslMatrixHelper *helper;
    int i;

    for (helper = context->matrixHelpers; helper != NULL;
         helper = helper->next)
    {
        if (helper->parameterCount != parameterCount ||
            !GlslTypesEqual(&helper->result, result)) continue;
        for (i = 0; i < parameterCount; i++) {
            if (!GlslTypesEqual(&helper->parameters[i], &parameters[i]))
                break;
        }
        if (i == parameterCount)
            return helper;
    }
    return NULL;
}

static const char *GlslMatrixHelperName(GlslLowerContext *context,
    const GlslType *result, const GlslType *parameters, int parameterCount)
{
    char candidate[256];
    char *end;
    int i;

    if (result == NULL || result->rows < 2 || result->rows > 4 ||
        result->cols != result->rows || parameterCount < 1 ||
        parameterCount > GLSL_MATRIX_MAX_ARGUMENTS) return NULL;
    sprintf(candidate, "cg_construct_mat%d", result->rows);
    end = candidate + strlen(candidate);
    for (i = 0; i < parameterCount; i++) {
        if (!GlslMatrixNumericParameterType(&parameters[i]))
            return NULL;
        if (parameters[i].base == GLSL_BASE_FLOAT) {
            if (parameters[i].len == 1)
                sprintf(end, "_f");
            else
                sprintf(end, "_v%d", parameters[i].len);
        } else {
            if (parameters[i].len == 1)
                sprintf(end, "_i");
            else
                sprintf(end, "_iv%d", parameters[i].len);
        }
        end += strlen(end);
    }
    return GlslAllocateDistinctNameForSource(context, candidate,
                                              &context->statementLoc);
}

static GlslExpr *GlslNewParameterComponent(GlslLowerContext *context,
    GlslDecl *parameter, int componentIndex)
{
    GlslExpr *symbol;
    GlslType scalarType;
    char mask[2];

    if (parameter == NULL || componentIndex < 0 ||
        componentIndex >= parameter->type.len) return NULL;
    symbol = GlslNewExpr(context->module, GLSL_EXPR_SYMBOL,
                         parameter->type);
    if (symbol == NULL)
        return NULL;
    symbol->u.symbol = parameter;
    if (parameter->type.len == 1)
        return symbol;
    scalarType = GlslNumericType(parameter->type.base, 1);
    mask[0] = "xyzw"[componentIndex];
    mask[1] = '\0';
    return GlslNewSwizzle(context, symbol, &scalarType, mask);
}

static GlslMatrixSelectorHelper *GlslFindMatrixSelectorHelper(
    GlslLowerContext *context, GlslMatrixSelectorHelperKind kind,
    const GlslType *matrixType, const GlslType *valueType, int count,
    int mask)
{
    GlslMatrixSelectorHelper *helper;

    for (helper = context->selectorHelpers; helper != NULL;
         helper = helper->next)
    {
        if (helper->kind == kind && helper->count == count &&
            helper->mask == mask &&
            GlslTypesEqual(&helper->matrixType, matrixType) &&
            GlslTypesEqual(&helper->valueType, valueType)) return helper;
    }
    return NULL;
}

static const char *GlslMatrixSelectorHelperName(
    GlslLowerContext *context, GlslMatrixSelectorHelperKind kind,
    const GlslType *matrixType, int count, int mask)
{
    char candidate[128];
    char *end;
    int column;
    int i;
    int row;
    int selector;

    if (matrixType == NULL || matrixType->rows < 2 ||
        matrixType->rows > 4 || matrixType->cols != matrixType->rows ||
        count < 2 || count > 4) return NULL;
    sprintf(candidate, "cg_%s_mat%d",
            kind == GLSL_MATRIX_SELECTOR_GET ? "get" : "set",
            matrixType->rows);
    end = candidate + strlen(candidate);
    for (i = 0; i < count; i++) {
        selector = (mask >> (i * 4)) & 15;
        row = (selector >> 2) & 3;
        column = selector & 3;
        if (row >= matrixType->rows || column >= matrixType->cols)
            return NULL;
        sprintf(end, "_m%d%d", row, column);
        end += strlen(end);
    }
    return GlslAllocateDistinctNameForSource(context, candidate,
                                              &context->statementLoc);
}

static GlslMatrixSelectorHelper *GlslCreateMatrixSelectorHelper(
    GlslLowerContext *context, GlslMatrixSelectorHelperKind kind,
    const GlslType *matrixType, const GlslType *valueType, int count,
    int mask)
{
    GlslMatrixSelectorHelper *helper;
    GlslFunction *function;
    GlslDecl *matrixParameter;
    GlslDecl *valueParameter;
    GlslExpr *matrixSymbol;
    GlslExpr *value;
    GlslExpr *component;
    GlslExpr *assignment;
    GlslExpr *constructor;
    GlslStmt *statement;
    GlslType resultType;
    GlslType scalarType;
    const char *functionName;
    const char *matrixName;
    const char *valueName;
    int i;

    if (valueType == NULL || valueType->base != GLSL_BASE_FLOAT ||
        valueType->rows != 0 || valueType->cols != 0 ||
        valueType->arraySize != 0 || valueType->elementType != NULL ||
        valueType->structName != NULL || valueType->members != NULL ||
        (valueType->len != 1 && valueType->len != count)) return NULL;
    functionName = GlslMatrixSelectorHelperName(context, kind, matrixType,
                                                 count, mask);
    if (functionName == NULL)
        return NULL;
    helper = (GlslMatrixSelectorHelper *) context->module->alloc(
        context->module->allocArg, sizeof(GlslMatrixSelectorHelper));
    if (helper == NULL)
        return NULL;
    memset(helper, 0, sizeof(GlslMatrixSelectorHelper));
    if (kind == GLSL_MATRIX_SELECTOR_GET)
        resultType = *valueType;
    else
        resultType = GlslNumericType(GLSL_BASE_VOID, 0);
    function = GlslNewFunction(context->module, resultType, functionName);
    if (function == NULL)
        return NULL;
    matrixName = GlslAllocateScopedSymbolNameForSource(context, function,
        NULL, "matrix", &context->statementLoc);
    matrixParameter = GlslNewDecl(context->module, GLSL_STORAGE_NONE,
                                  *matrixType, matrixName);
    if (matrixName == NULL || matrixParameter == NULL)
        return NULL;
    if (kind == GLSL_MATRIX_SELECTOR_SET)
        matrixParameter->parameterQualifier = GLSL_PARAMETER_INOUT;
    GlslAppendDecl(&function->parameters, matrixParameter);
    scalarType = GlslNumericType(GLSL_BASE_FLOAT, 1);
    if (kind == GLSL_MATRIX_SELECTOR_GET) {
        constructor = GlslNewExpr(context->module, GLSL_EXPR_CONSTRUCT,
                                  *valueType);
        statement = GlslNewStmt(context->module, GLSL_STMT_RETURN);
        if (constructor == NULL || statement == NULL)
            return NULL;
        for (i = 0; i < count; i++) {
            matrixSymbol = GlslNewExpr(context->module,
                                       GLSL_EXPR_SYMBOL, *matrixType);
            if (matrixSymbol == NULL)
                return NULL;
            matrixSymbol->u.symbol = matrixParameter;
            component = GlslMatrixMaskComponent(context, matrixSymbol,
                                                 mask, i);
            if (component == NULL)
                return NULL;
            GlslAppendExpr(&constructor->u.construct.arguments, component);
        }
        statement->u.returnExpr = constructor;
        function->body = statement;
    } else {
        valueName = GlslAllocateScopedSymbolNameForSource(context, function,
            NULL, "value", &context->statementLoc);
        valueParameter = GlslNewDecl(context->module, GLSL_STORAGE_NONE,
                                     *valueType, valueName);
        if (valueName == NULL || valueParameter == NULL)
            return NULL;
        GlslAppendDecl(&function->parameters, valueParameter);
        for (i = 0; i < count; i++) {
            matrixSymbol = GlslNewExpr(context->module,
                                       GLSL_EXPR_SYMBOL, *matrixType);
            if (matrixSymbol == NULL)
                return NULL;
            matrixSymbol->u.symbol = matrixParameter;
            component = GlslMatrixMaskComponent(context, matrixSymbol,
                                                 mask, i);
            value = GlslNewParameterComponent(context, valueParameter,
                valueType->len == 1 ? 0 : i);
            assignment = GlslNewExpr(context->module, GLSL_EXPR_BINARY,
                                     scalarType);
            statement = GlslNewStmt(context->module,
                                    GLSL_STMT_EXPRESSION);
            if (component == NULL || value == NULL || assignment == NULL ||
                statement == NULL) return NULL;
            assignment->u.binary.op = GLSL_OP_ASSIGN;
            assignment->u.binary.left = component;
            assignment->u.binary.right = value;
            statement->u.expression = assignment;
            GlslAppendStmt(&function->body, statement);
        }
    }
    helper->function = function;
    helper->kind = kind;
    helper->matrixType = *matrixType;
    helper->valueType = *valueType;
    helper->count = count;
    helper->mask = mask;
    if (context->selectorHelpers == NULL)
        context->selectorHelpers = helper;
    else
        context->lastSelectorHelper->next = helper;
    context->lastSelectorHelper = helper;
    return helper;
}

GlslMatrixSelectorHelper *GlslGetMatrixSelectorHelper(
    GlslLowerContext *context, GlslMatrixSelectorHelperKind kind,
    const GlslType *matrixType, const GlslType *valueType, int count,
    int mask)
{
    GlslMatrixSelectorHelper *helper;
    int normalizedMask;

    normalizedMask = mask & ((1 << (count * 4)) - 1);
    helper = GlslFindMatrixSelectorHelper(context, kind, matrixType,
                                          valueType, count,
                                          normalizedMask);
    if (helper == NULL) {
        helper = GlslCreateMatrixSelectorHelper(context, kind, matrixType,
                                                 valueType, count,
                                                 normalizedMask);
    }
    return helper;
}

static GlslMatrixHelper *GlslCreateMatrixHelper(GlslLowerContext *context,
    const GlslType *result, const GlslType *parameters, int parameterCount)
{
    GlslExpr *components[GLSL_MATRIX_MAX_ARGUMENTS];
    GlslExpr *component;
    GlslExpr *constructor;
    GlslStmt *returnStatement;
    GlslMatrixHelper *helper;
    GlslDecl *parameter;
    const char *functionName;
    const char *parameterName;
    char candidate[32];
    int componentCount;
    int parameterIndex;
    int componentIndex;
    int column;
    int row;

    functionName = GlslMatrixHelperName(context, result, parameters,
                                        parameterCount);
    if (functionName == NULL)
        return NULL;
    helper = (GlslMatrixHelper *) context->module->alloc(
        context->module->allocArg, sizeof(GlslMatrixHelper));
    if (helper == NULL)
        return NULL;
    memset(helper, 0, sizeof(GlslMatrixHelper));
    helper->function = GlslNewFunction(context->module, *result,
                                       functionName);
    if (helper->function == NULL)
        return NULL;
    helper->result = *result;
    helper->parameterCount = parameterCount;
    componentCount = 0;
    for (parameterIndex = 0; parameterIndex < parameterCount;
         parameterIndex++)
    {
        helper->parameters[parameterIndex] = parameters[parameterIndex];
        sprintf(candidate, "arg%d", parameterIndex);
        parameterName = GlslAllocateScopedSymbolNameForSource(context,
            helper->function, NULL, candidate, &context->statementLoc);
        if (parameterName == NULL)
            return NULL;
        parameter = GlslNewDecl(context->module, GLSL_STORAGE_NONE,
            parameters[parameterIndex], parameterName);
        if (parameter == NULL)
            return NULL;
        GlslAppendDecl(&helper->function->parameters, parameter);
        for (componentIndex = 0;
             componentIndex < parameters[parameterIndex].len;
             componentIndex++)
        {
            if (componentCount >= (int) (sizeof(components) /
                                         sizeof(components[0]))) return NULL;
            component = GlslNewParameterComponent(context, parameter,
                                                   componentIndex);
            if (component == NULL)
                return NULL;
            components[componentCount++] = component;
        }
    }
    if (componentCount != result->rows * result->cols)
        return NULL;
    constructor = GlslNewExpr(context->module, GLSL_EXPR_CONSTRUCT,
                              *result);
    returnStatement = GlslNewStmt(context->module, GLSL_STMT_RETURN);
    if (constructor == NULL || returnStatement == NULL)
        return NULL;
    for (column = 0; column < result->cols; column++) {
        for (row = 0; row < result->rows; row++) {
            GlslAppendExpr(&constructor->u.construct.arguments,
                           components[row * result->cols + column]);
        }
    }
    returnStatement->u.returnExpr = constructor;
    helper->function->body = returnStatement;
    if (context->matrixHelpers == NULL)
        context->matrixHelpers = helper;
    else
        context->lastMatrixHelper->next = helper;
    context->lastMatrixHelper = helper;
    return helper;
}

GlslExpr *GlslLowerImpureMatrixConstructor(
    GlslLowerContext *context, GlslExpr *arguments, const GlslType *type)
{
    GlslType parameters[GLSL_MATRIX_MAX_ARGUMENTS];
    GlslMatrixHelper *helper;
    GlslExpr *argument;
    GlslExpr *target;
    int componentCount;
    int parameterCount;

    componentCount = 0;
    parameterCount = 0;
    for (argument = arguments; argument != NULL; argument = argument->next) {
        if (parameterCount >= GLSL_MATRIX_MAX_ARGUMENTS ||
            !GlslMatrixNumericParameterType(&argument->type) ||
            componentCount > type->rows * type->cols - argument->type.len)
        {
            if (!GlslMatrixNumericParameterType(&argument->type)) {
                GlslRecordFailureKind(context,
                                      GLSL_ERROR_UNSUPPORTED_TYPE,
                                      "matrix constructor argument type");
            }
            return NULL;
        }
        parameters[parameterCount++] = argument->type;
        componentCount += argument->type.len;
    }
    if (parameterCount == 0 || componentCount != type->rows * type->cols)
        return NULL;
    helper = GlslFindMatrixHelper(context, type, parameters,
                                  parameterCount);
    if (helper == NULL) {
        helper = GlslCreateMatrixHelper(context, type, parameters,
                                        parameterCount);
        if (helper == NULL)
            return NULL;
    }
    target = GlslNewExpr(context->module, GLSL_EXPR_CALL, *type);
    if (target == NULL)
        return NULL;
    target->u.call.name = helper->function->name;
    target->u.call.arguments = arguments;
    return target;
}

GlslExpr *GlslLowerMatrixConstructor(GlslLowerContext *context,
    expr *source, const GlslType *type)
{
    GlslExpr *arguments[16];
    GlslExpr *argument;
    GlslExpr *next;
    GlslExpr *target;
    GlslExpr *component;
    GlslType scalarType;
    expr *sourceArgument;
    char mask[2];
    int count;
    int componentIndex;
    int componentCount;
    int row;
    int column;
    int size;
    int hasSideEffects;

    size = type->rows;
    if (size < 2 || size > 4 || type->cols != size)
        return NULL;
    hasSideEffects = 0;
    for (sourceArgument = source->un.arg; sourceArgument != NULL;
         sourceArgument = sourceArgument->bin.right)
    {
        if (sourceArgument->common.kind != BINARY_N ||
            sourceArgument->bin.op != EXPR_LIST_OP ||
            sourceArgument->bin.left == NULL) return NULL;
        if (sourceArgument->bin.left->common.HasSideEffects)
            hasSideEffects = 1;
    }
    argument = GlslLowerExprChain(context, source->un.arg, EXPR_LIST_OP);
    if (argument == NULL)
        return NULL;
    /* Keep impure expressions at the constructor call site and use each
       exactly once; argument evaluation order remains language-defined. */
    if (hasSideEffects)
        return GlslLowerImpureMatrixConstructor(context, argument, type);
    sourceArgument = source->un.arg;
    count = 0;
    while (argument != NULL) {
        next = argument->next;
        argument->next = NULL;
        componentCount = argument->type.len;
        if (!GlslMatrixNumericParameterType(&argument->type) ||
            count > size * size - componentCount)
        {
            if (!GlslMatrixNumericParameterType(&argument->type)) {
                GlslRecordFailureKind(context,
                                      GLSL_ERROR_UNSUPPORTED_TYPE,
                                      "matrix constructor argument type");
            }
            return NULL;
        }
        if (componentCount == 1) {
            arguments[count++] = argument;
        } else {
            if (sourceArgument == NULL ||
                sourceArgument->common.kind != BINARY_N ||
                sourceArgument->bin.op != EXPR_LIST_OP)
            {
                return NULL;
            }
            for (componentIndex = 0; componentIndex < componentCount;
                 componentIndex++)
            {
                scalarType = GlslNumericType(argument->type.base, 1);
                mask[0] = "xyzw"[componentIndex];
                mask[1] = '\0';
                component = GlslNewSwizzle(context, argument,
                                           &scalarType, mask);
                if (component == NULL)
                    return NULL;
                arguments[count++] = component;
            }
        }
        argument = next;
        sourceArgument = sourceArgument->bin.right;
    }
    if (sourceArgument != NULL || count != size * size)
        return NULL;
    target = GlslNewExpr(context->module, GLSL_EXPR_CONSTRUCT, *type);
    if (target == NULL)
        return NULL;
    for (column = 0; column < size; column++) {
        for (row = 0; row < size; row++) {
            GlslAppendExpr(&target->u.construct.arguments,
                           arguments[row * size + column]);
        }
    }
    return target;
}

static GlslOperator GlslUnaryOperator(opcode op)
{
    switch (op) {
    case NEG_OP:
    case NEG_V_OP: return GLSL_OP_NEGATE;
    case POS_OP:
    case POS_V_OP: return GLSL_OP_POSITIVE;
    case BNOT_OP:
    case BNOT_V_OP: return GLSL_OP_LOGICAL_NOT;
    default: return GLSL_OP_NONE;
    }
}

static GlslOperator GlslBinaryOperator(opcode op)
{
    switch (op) {
    case ASSIGN_OP:
    case ASSIGN_V_OP:
    case ASSIGN_GEN_OP:
    case ASSIGN_DYN_OP: return GLSL_OP_ASSIGN;
    case MUL_OP: case MUL_V_OP: case MUL_SV_OP: case MUL_VS_OP:
        return GLSL_OP_MULTIPLY;
    case DIV_OP: case DIV_V_OP: case DIV_SV_OP: case DIV_VS_OP:
        return GLSL_OP_DIVIDE;
    case ADD_OP: case ADD_V_OP: case ADD_SV_OP: case ADD_VS_OP:
        return GLSL_OP_ADD;
    case SUB_OP: case SUB_V_OP: case SUB_SV_OP: case SUB_VS_OP:
        return GLSL_OP_SUBTRACT;
    case LT_OP: return GLSL_OP_LESS;
    case GT_OP: return GLSL_OP_GREATER;
    case LE_OP: return GLSL_OP_LESS_EQUAL;
    case GE_OP: return GLSL_OP_GREATER_EQUAL;
    case EQ_OP: return GLSL_OP_EQUAL;
    case NE_OP: return GLSL_OP_NOT_EQUAL;
    case BAND_OP: return GLSL_OP_LOGICAL_AND;
    case BOR_OP: return GLSL_OP_LOGICAL_OR;
    default: return GLSL_OP_NONE;
    }
}

static const char *GlslVectorComparisonName(opcode op)
{
    switch (op) {
    case LT_V_OP: case LT_SV_OP: case LT_VS_OP: return "lessThan";
    case GT_V_OP: case GT_SV_OP: case GT_VS_OP: return "greaterThan";
    case LE_V_OP: case LE_SV_OP: case LE_VS_OP: return "lessThanEqual";
    case GE_V_OP: case GE_SV_OP: case GE_VS_OP: return "greaterThanEqual";
    case EQ_V_OP: case EQ_SV_OP: case EQ_VS_OP: return "equal";
    case NE_V_OP: case NE_SV_OP: case NE_VS_OP: return "notEqual";
    default: return NULL;
    }
}

int GlslValidateTextureCall(GlslLowerContext *context,
    GlslBuiltin builtin, const GlslType *result, GlslExpr *arguments)
{
    GlslType samplerType;
    GlslType coordType;
    GlslType resultType;
    GlslBase samplerBase;
    GlslBinding *binding;
    GlslDecl *decl;
    Symbol *symbol;
    int coordLen;
    int sourceBase;

    if (builtin < GLSL_BUILTIN_TEX1D ||
        builtin > GLSL_BUILTIN_TEXCUBE_PROJ) return 1;
    if (context->profile->stage == GLSL_STAGE_VERTEX) {
        GlslRecordFailureKind(context, GLSL_ERROR_STAGE_OPERATION,
                              "texture sampling");
        return 0;
    }
    switch (builtin) {
    case GLSL_BUILTIN_TEX1D:
        samplerBase = GLSL_BASE_SAMPLER1D;
        sourceBase = TYPE_BASE_GLSL_SAMPLER1D;
        coordLen = 1;
        break;
    case GLSL_BUILTIN_TEX2D:
        samplerBase = GLSL_BASE_SAMPLER2D;
        sourceBase = TYPE_BASE_GLSL_SAMPLER2D;
        coordLen = 2;
        break;
    case GLSL_BUILTIN_TEX3D:
        samplerBase = GLSL_BASE_SAMPLER3D;
        sourceBase = TYPE_BASE_GLSL_SAMPLER3D;
        coordLen = 3;
        break;
    case GLSL_BUILTIN_TEXCUBE:
        samplerBase = GLSL_BASE_SAMPLERCUBE;
        sourceBase = TYPE_BASE_GLSL_SAMPLERCUBE;
        coordLen = 3;
        break;
    case GLSL_BUILTIN_TEX1D_PROJ:
        samplerBase = GLSL_BASE_SAMPLER1D;
        sourceBase = TYPE_BASE_GLSL_SAMPLER1D;
        coordLen = 4;
        break;
    case GLSL_BUILTIN_TEX2D_PROJ:
        samplerBase = GLSL_BASE_SAMPLER2D;
        sourceBase = TYPE_BASE_GLSL_SAMPLER2D;
        coordLen = 4;
        break;
    case GLSL_BUILTIN_TEX3D_PROJ:
        samplerBase = GLSL_BASE_SAMPLER3D;
        sourceBase = TYPE_BASE_GLSL_SAMPLER3D;
        coordLen = 4;
        break;
    case GLSL_BUILTIN_TEXCUBE_PROJ:
        samplerBase = GLSL_BASE_SAMPLERCUBE;
        sourceBase = TYPE_BASE_GLSL_SAMPLERCUBE;
        coordLen = 4;
        break;
    default:
        return 0;
    }
    samplerType = GlslNumericType(samplerBase, 1);
    coordType = GlslNumericType(GLSL_BASE_FLOAT, coordLen);
    resultType = GlslNumericType(GLSL_BASE_FLOAT, 4);
    if (arguments == NULL || arguments->next == NULL ||
        arguments->next->next != NULL ||
        !GlslTypesEqual(&arguments->type, &samplerType) ||
        !GlslTypesEqual(&arguments->next->type, &coordType) ||
        !GlslTypesEqual(result, &resultType))
    {
        GlslRecordFailureKind(context, GLSL_ERROR_INTRINSIC,
                              GlslBuiltinSpelling(builtin));
        return 0;
    }
    if (arguments->kind != GLSL_EXPR_SYMBOL ||
        arguments->u.symbol == NULL)
    {
        GlslRecordFailureKind(context, GLSL_ERROR_SAMPLER,
            "texture sampler argument must be a direct bound uniform");
        return 0;
    }
    decl = arguments->u.symbol;
    symbol = (Symbol *) decl->identity;
    binding = symbol != NULL ?
              GlslFindUniformBinding(context->module, symbol) : NULL;
    if (decl->storage != GLSL_STORAGE_SAMPLER ||
        !GlslTypesEqual(&decl->type, &samplerType) ||
        symbol == NULL || symbol->kind != VARIABLE_S ||
        symbol->type == NULL ||
        GetDomain(symbol->type) != TYPE_DOMAIN_UNIFORM ||
        (GetCategory(symbol->type) != TYPE_CATEGORY_SCALAR &&
         GetCategory(symbol->type) != TYPE_CATEGORY_SAMPLER) ||
        GetBase(symbol->type) != sourceBase ||
        binding == NULL || binding->storage != GLSL_STORAGE_SAMPLER ||
        binding->declaration != decl || binding->name == NULL ||
        decl->name == NULL || strcmp(binding->name, decl->name))
    {
        GlslRecordFailureKind(context, GLSL_ERROR_SAMPLER,
            "texture sampler argument must be a direct bound uniform");
        return 0;
    }
    return 1;
}

/*
 * GlslIntrinsicBuiltin() - Map a stable catalog intrinsic identity to
 *        its exact core GLSL 1.50 builtin.  GLSL_BUILTIN_NONE means the
 *        cataloged intrinsic has no exact GLSL 1.50 lowering and must
 *        reach profile validation.  The Cg spellings map once here by
 *        intrinsic identity, never by source name: lerp to mix, frac to
 *        fract, rsqrt to inversesqrt, saturate to clamp, base tex* to
 *        texture, projected tex*proj to textureProj, and explicit-LOD
 *        identities to textureLod.
 */

GlslBuiltin GlslIntrinsicBuiltin(CgIntrinsic intrinsic)
{
    switch (intrinsic) {
    case CG_INTRINSIC_MUL:       return GLSL_BUILTIN_MUL;
    case CG_INTRINSIC_DOT:       return GLSL_BUILTIN_DOT;
    case CG_INTRINSIC_CROSS:     return GLSL_BUILTIN_CROSS;
    case CG_INTRINSIC_NORMALIZE: return GLSL_BUILTIN_NORMALIZE;
    case CG_INTRINSIC_REFLECT:   return GLSL_BUILTIN_REFLECT;
    case CG_INTRINSIC_REFRACT:   return GLSL_BUILTIN_REFRACT;
    case CG_INTRINSIC_LENGTH:    return GLSL_BUILTIN_LENGTH;
    case CG_INTRINSIC_DISTANCE:  return GLSL_BUILTIN_DISTANCE;
    case CG_INTRINSIC_MIN:       return GLSL_BUILTIN_MIN;
    case CG_INTRINSIC_MAX:       return GLSL_BUILTIN_MAX;
    case CG_INTRINSIC_CLAMP:     return GLSL_BUILTIN_CLAMP;
    case CG_INTRINSIC_ABS:       return GLSL_BUILTIN_ABS;
    case CG_INTRINSIC_SIGN:      return GLSL_BUILTIN_SIGN;
    case CG_INTRINSIC_FLOOR:     return GLSL_BUILTIN_FLOOR;
    case CG_INTRINSIC_CEIL:      return GLSL_BUILTIN_CEIL;
    case CG_INTRINSIC_SQRT:      return GLSL_BUILTIN_SQRT;
    case CG_INTRINSIC_EXP:       return GLSL_BUILTIN_EXP;
    case CG_INTRINSIC_EXP2:      return GLSL_BUILTIN_EXP2;
    case CG_INTRINSIC_LOG:       return GLSL_BUILTIN_LOG;
    case CG_INTRINSIC_LOG2:      return GLSL_BUILTIN_LOG2;
    case CG_INTRINSIC_SIN:       return GLSL_BUILTIN_SIN;
    case CG_INTRINSIC_COS:       return GLSL_BUILTIN_COS;
    case CG_INTRINSIC_TAN:       return GLSL_BUILTIN_TAN;
    case CG_INTRINSIC_ASIN:      return GLSL_BUILTIN_ASIN;
    case CG_INTRINSIC_ACOS:      return GLSL_BUILTIN_ACOS;
    case CG_INTRINSIC_ATAN:      return GLSL_BUILTIN_ATAN;
    case CG_INTRINSIC_RSQRT:     return GLSL_BUILTIN_RSQRT;
    case CG_INTRINSIC_LERP:      return GLSL_BUILTIN_LERP;
    case CG_INTRINSIC_FRAC:      return GLSL_BUILTIN_FRAC;
    case CG_INTRINSIC_SATURATE:  return GLSL_BUILTIN_SATURATE;
    case CG_INTRINSIC_TEX1D:     return GLSL_BUILTIN_TEX1D;
    case CG_INTRINSIC_TEX2D:     return GLSL_BUILTIN_TEX2D;
    case CG_INTRINSIC_TEX3D:     return GLSL_BUILTIN_TEX3D;
    case CG_INTRINSIC_TEXCUBE:   return GLSL_BUILTIN_TEXCUBE;
    case CG_INTRINSIC_TEX1DPROJ: return GLSL_BUILTIN_TEX1D_PROJ;
    case CG_INTRINSIC_TEX2DPROJ: return GLSL_BUILTIN_TEX2D_PROJ;
    case CG_INTRINSIC_TEX3DPROJ: return GLSL_BUILTIN_TEX3D_PROJ;
    case CG_INTRINSIC_TEXCUBEPROJ: return GLSL_BUILTIN_TEXCUBE_PROJ;
    default:
        return GLSL_BUILTIN_NONE;
    }
} // GlslIntrinsicBuiltin

static GlslExpr *GlslLowerCall(GlslLowerContext *context, expr *source,
                               const GlslType *type)
{
    GlslExpr *target;
    GlslExpr *arguments;
    GlslExpr *left;
    GlslExpr *right;
    GlslExpr *zero;
    GlslExpr *one;
    GlslFunction *function;
    Symbol *symbol;
    const char *name;
    GlslBuiltin builtin;
    GlslType scalarType;

    if (source->bin.left == NULL ||
        source->bin.left->common.kind != SYMB_N) return NULL;
    symbol = source->bin.left->sym.symbol;
    function = GlslFindFunction(context->module, symbol);
    if (function != NULL) {
        name = function->name;
    } else if (source->bin.op == FUN_INTRINSIC_OP && symbol != NULL &&
               symbol->kind == FUNCTION_S &&
               (symbol->properties & SYMB_IS_BUILTIN))
    {
        const CgIntrinsicSignature *signature =
            CgIntrinsicSignatureForSymbol(symbol);

        /* Lowering is keyed on the stable intrinsic identity carried by
         * the selected symbol, never on a name lookup.  A cataloged
         * intrinsic without an exact GLSL 1.50 lowering fails profile
         * validation with the existing intrinsic diagnostic. */
        builtin = signature != NULL ?
                  GlslIntrinsicBuiltin(signature->intrinsic) :
                  GLSL_BUILTIN_NONE;
        if (builtin == GLSL_BUILTIN_NONE) {
            GlslRecordFailureKind(context, GLSL_ERROR_INTRINSIC,
                signature != NULL ? signature->name :
                GetAtomString(atable, symbol->name));
            return NULL;
        }
        name = GlslBuiltinSpelling(builtin);
        if (name == NULL)
            return NULL;
    } else {
        return NULL;
    }
    arguments = NULL;
    if (source->bin.right != NULL) {
        if (function == NULL && builtin >= GLSL_BUILTIN_TEX1D &&
            builtin <= GLSL_BUILTIN_TEXCUBE_PROJ)
        {
            arguments = GlslLowerTextureArguments(context,
                                                   source->bin.right);
        } else {
            arguments = GlslLowerExprChain(context, source->bin.right,
                                            FUN_ARG_OP);
        }
        if (arguments == NULL)
            return NULL;
    }
    if (function == NULL) {
        if (!GlslValidateTextureCall(context, builtin, type, arguments))
            return NULL;
        if (builtin == GLSL_BUILTIN_MUL ||
            (builtin == GLSL_BUILTIN_DOT &&
             arguments != NULL && arguments->type.len == 1))
        {
            left = arguments;
            right = left != NULL ? left->next : NULL;
            if (left == NULL || right == NULL || right->next != NULL)
                return NULL;
            left->next = NULL;
            right->next = NULL;
            target = GlslNewExpr(context->module, GLSL_EXPR_BINARY, *type);
            if (target == NULL)
                return NULL;
            target->u.binary.op = GLSL_OP_MULTIPLY;
            target->u.binary.left = left;
            target->u.binary.right = right;
            return target;
        }
        if (builtin == GLSL_BUILTIN_SATURATE) {
            if (arguments == NULL || arguments->next != NULL)
                return NULL;
            scalarType = GlslNumericType(GLSL_BASE_FLOAT, 1);
            zero = GlslNewLiteral(context, GLSL_BASE_FLOAT, 0, 0.0f);
            one = GlslNewLiteral(context, GLSL_BASE_FLOAT, 0, 1.0f);
            if (zero == NULL || one == NULL)
                return NULL;
            if (type->len > 1) {
                target = GlslNewExpr(context->module,
                                     GLSL_EXPR_CONSTRUCT, *type);
                if (target == NULL)
                    return NULL;
                target->u.construct.arguments = zero;
                zero = target;
                target = GlslNewExpr(context->module,
                                     GLSL_EXPR_CONSTRUCT, *type);
                if (target == NULL)
                    return NULL;
                target->u.construct.arguments = one;
                one = target;
            } else if (!GlslTypesEqual(type, &scalarType)) {
                return NULL;
            }
            arguments->next = zero;
            zero->next = one;
        }
    }
    target = GlslNewExpr(context->module, GLSL_EXPR_CALL, *type);
    if (target == NULL)
        return NULL;
    target->u.call.name = name;
    target->u.call.arguments = arguments;
    target->u.call.builtin = function == NULL ? builtin : GLSL_BUILTIN_NONE;
    return target;
}

static GlslExpr *GlslLowerVectorComparison(GlslLowerContext *context,
    expr *source, const GlslType *type, const char *name)
{
    GlslExpr *target;
    GlslExpr *constructor;
    GlslExpr *left;
    GlslExpr *right;
    GlslType vectorType;

    left = GlslLowerExpr(context, source->bin.left);
    right = GlslLowerExpr(context, source->bin.right);
    if (left == NULL || right == NULL)
        return NULL;
    if (left->type.len == 1 && type->len > 1) {
        vectorType = GlslNumericType(left->type.base, type->len);
        constructor = GlslNewExpr(context->module, GLSL_EXPR_CONSTRUCT,
                                  vectorType);
        if (constructor == NULL)
            return NULL;
        constructor->u.construct.arguments = left;
        left = constructor;
    }
    if (right->type.len == 1 && type->len > 1) {
        vectorType = GlslNumericType(right->type.base, type->len);
        constructor = GlslNewExpr(context->module, GLSL_EXPR_CONSTRUCT,
                                  vectorType);
        if (constructor == NULL)
            return NULL;
        constructor->u.construct.arguments = right;
        right = constructor;
    }
    target = GlslNewExpr(context->module, GLSL_EXPR_CALL, *type);
    if (target == NULL)
        return NULL;
    target->u.call.name = name;
    target->u.call.arguments = left;
    left->next = right;
    return target;
}

static GlslExpr *GlslLowerMaskedAssignment(GlslLowerContext *context,
    expr *source, const GlslType *type)
{
    GlslExpr *target;
    GlslExpr *left;
    GlslExpr *right;
    GlslType maskedType;
    char maskText[5];
    int mask;
    int i;
    int count;

    left = GlslLowerExpr(context, source->bin.left);
    right = GlslLowerExpr(context, source->bin.right);
    if (left == NULL || right == NULL)
        return NULL;
    mask = SUBOP_GET_MASK(source->bin.subop);
    count = 0;
    for (i = 0; i < 4; i++) {
        if (mask & (1 << i))
            maskText[count++] = "xyzw"[i];
    }
    maskText[count] = '\0';
    if (count == 0)
        return NULL;
    maskedType = GlslNumericType(left->type.base, count);
    left = GlslNewSwizzle(context, left, &maskedType, maskText);
    if (left == NULL)
        return NULL;
    target = GlslNewExpr(context->module, GLSL_EXPR_BINARY, *type);
    if (target != NULL) {
        target->u.binary.op = GLSL_OP_ASSIGN;
        target->u.binary.left = left;
        target->u.binary.right = right;
    }
    return target;
}

static GlslExpr *GlslLowerComponent(GlslLowerContext *context, expr *source,
    int component, GlslBase base)
{
    GlslExpr *target;
    GlslType type;
    int len;
    char mask[2];

    target = GlslLowerExpr(context, source);
    if (target == NULL)
        return NULL;
    if (!IsVector(source->common.type, &len) || len <= 1)
        return target;
    type = GlslNumericType(base, 1);
    mask[0] = "xyzw"[component];
    mask[1] = '\0';
    return GlslNewSwizzle(context, target, &type, mask);
}

static GlslExpr *GlslLowerConditional(GlslLowerContext *context,
    expr *source, const GlslType *type)
{
    GlslExpr *target;
    GlslExpr *componentExpr;
    GlslExpr *condition;
    GlslExpr *trueExpr;
    GlslExpr *falseExpr;
    GlslType componentType;
    int conditionLen;
    int i;

    conditionLen = 0;
    IsVector(source->tri.arg1->common.type, &conditionLen);
    if (conditionLen <= 1) {
        target = GlslNewExpr(context->module, GLSL_EXPR_CONDITIONAL, *type);
        if (target == NULL)
            return NULL;
        target->u.conditional.condition = GlslLowerExpr(context,
                                                        source->tri.arg1);
        target->u.conditional.trueExpr = GlslLowerExpr(context,
                                                       source->tri.arg2);
        target->u.conditional.falseExpr = GlslLowerExpr(context,
                                                        source->tri.arg3);
        if (target->u.conditional.condition == NULL ||
            target->u.conditional.trueExpr == NULL ||
            target->u.conditional.falseExpr == NULL) return NULL;
        return target;
    }
    if (type->len < 2 || type->len > 4 || conditionLen != type->len)
        return NULL;
    target = GlslNewExpr(context->module, GLSL_EXPR_CONSTRUCT, *type);
    if (target == NULL)
        return NULL;
    componentType = GlslNumericType(type->base, 1);
    for (i = 0; i < type->len; i++) {
        condition = GlslLowerComponent(context, source->tri.arg1, i,
                                       GLSL_BASE_BOOL);
        trueExpr = GlslLowerComponent(context, source->tri.arg2, i,
                                      type->base);
        falseExpr = GlslLowerComponent(context, source->tri.arg3, i,
                                       type->base);
        if (condition == NULL || trueExpr == NULL || falseExpr == NULL)
            return NULL;
        componentExpr = GlslNewExpr(context->module,
            GLSL_EXPR_CONDITIONAL, componentType);
        if (componentExpr == NULL)
            return NULL;
        componentExpr->u.conditional.condition = condition;
        componentExpr->u.conditional.trueExpr = trueExpr;
        componentExpr->u.conditional.falseExpr = falseExpr;
        GlslAppendExpr(&target->u.construct.arguments, componentExpr);
    }
    return target;
}

GlslExpr *GlslLowerExpr(GlslLowerContext *context, expr *source)
{
    GlslExpr *target;
    GlslExpr *operand;
    GlslDecl *decl;
    GlslType type;
    GlslOperator op;
    Symbol *member;
    const char *comparison;

    if (source == NULL ||
        !GlslLowerType(context, source->common.type, &type, NULL))
    {
        GlslRecordFailureKind(context, GLSL_ERROR_UNSUPPORTED_TYPE,
                              "GLSL profile expression type");
        return NULL;
    }
    if (GlslIsSamplerType(&type)) {
        GlslRecordFailureKind(context, GLSL_ERROR_SAMPLER,
                              "opaque sampler expression");
        return NULL;
    }
    if (source->common.kind == SYMB_N && source->sym.op == VARIABLE_OP) {
        decl = GlslFindDecl(context, source->sym.symbol);
        if (decl == NULL)
            return NULL;
        target = GlslNewExpr(context->module, GLSL_EXPR_SYMBOL, type);
        if (target != NULL)
            target->u.symbol = decl;
        return target;
    }
    if (source->common.kind == CONST_N)
        return GlslLowerConstant(context, source, &type);
    if (source->common.kind == UNARY_N) {
        if (source->un.op == SWIZZLE_Z_OP)
            return GlslLowerSwizzle(context, source, &type);
        if (source->un.op == SWIZMAT_Z_OP)
            return GlslLowerMatrixSwizzle(context, source, &type);
        if (source->un.op == VECTOR_V_OP && type.rows != 0)
            return GlslLowerMatrixConstructor(context, source, &type);
        if (source->un.op == VECTOR_V_OP) {
            target = GlslNewExpr(context->module, GLSL_EXPR_CONSTRUCT, type);
            if (target == NULL)
                return NULL;
            target->u.construct.arguments = GlslLowerExprChain(context,
                source->un.arg, EXPR_LIST_OP);
            if (target->u.construct.arguments == NULL)
                return NULL;
            return target;
        }
        if (source->un.op == CAST_CS_OP || source->un.op == CAST_CV_OP ||
            source->un.op == CAST_CM_OP)
        {
            operand = GlslLowerExpr(context, source->un.arg);
            if (operand == NULL)
                return NULL;
            if (GlslTypesEqual(&operand->type, &type))
                return operand;
            target = GlslNewExpr(context->module, GLSL_EXPR_CONSTRUCT, type);
            if (target != NULL)
                target->u.construct.arguments = operand;
            return target;
        }
        if (source->un.op == BNOT_V_OP) {
            operand = GlslLowerExpr(context, source->un.arg);
            if (operand == NULL)
                return NULL;
            target = GlslNewExpr(context->module, GLSL_EXPR_CALL, type);
            if (target != NULL) {
                target->u.call.name = "not";
                target->u.call.arguments = operand;
            }
            return target;
        }
        op = GlslUnaryOperator(source->un.op);
        if (op != GLSL_OP_NONE) {
            target = GlslNewExpr(context->module, GLSL_EXPR_UNARY, type);
            if (target == NULL)
                return NULL;
            target->u.unary.op = op;
            target->u.unary.operand = GlslLowerExpr(context, source->un.arg);
            if (target->u.unary.operand == NULL)
                return NULL;
            return target;
        }
    }
    if (source->common.kind == BINARY_N) {
        if (source->bin.op == MEMBER_SELECTOR_OP) {
            if (source->bin.right == NULL ||
                source->bin.right->common.kind != SYMB_N ||
                source->bin.right->sym.op != MEMBER_OP) return NULL;
            member = source->bin.right->sym.symbol;
            if (source->bin.left != NULL &&
                source->bin.left->common.kind == SYMB_N &&
                source->bin.left->sym.op == VARIABLE_OP &&
                (source->bin.left->sym.symbol == Cg->theHAL->varyingIn ||
                 source->bin.left->sym.symbol == Cg->theHAL->varyingOut))
            {
                decl = GlslLowerInterface(context, member);
                if (decl == NULL)
                    return NULL;
                target = GlslNewExpr(context->module, GLSL_EXPR_SYMBOL, type);
                if (target != NULL)
                    target->u.symbol = decl;
                return target;
            }
            decl = GlslFindDecl(context, member);
            if (decl == NULL)
                return NULL;
            target = GlslNewExpr(context->module, GLSL_EXPR_MEMBER, type);
            if (target == NULL)
                return NULL;
            target->u.member.object = GlslLowerExpr(context, source->bin.left);
            if (target->u.member.object == NULL)
                return NULL;
            target->u.member.decl = decl;
            target->u.member.name = decl->name;
            return target;
        }
        if (source->bin.op == ARRAY_INDEX_OP) {
            target = GlslNewExpr(context->module, GLSL_EXPR_INDEX, type);
            if (target == NULL)
                return NULL;
            target->u.index.object = GlslLowerExpr(context, source->bin.left);
            target->u.index.index = GlslLowerExpr(context, source->bin.right);
            if (target->u.index.object == NULL || target->u.index.index == NULL)
                return NULL;
            return target;
        }
        if (source->bin.op == FUN_CALL_OP ||
            source->bin.op == FUN_INTRINSIC_OP)
            return GlslLowerCall(context, source, &type);
        if ((source->bin.op == ASSIGN_OP ||
             source->bin.op == ASSIGN_V_OP ||
             source->bin.op == ASSIGN_GEN_OP ||
             source->bin.op == ASSIGN_DYN_OP ||
             source->bin.op == ASSIGN_MASKED_KV_OP) &&
            GlslMatrixSelectorCount(source->bin.left) > 1)
        {
            GlslRecordFailure(context,
                              "matrix selector assignment context");
            return NULL;
        }
        if (source->bin.op == ASSIGN_MASKED_KV_OP)
            return GlslLowerMaskedAssignment(context, source, &type);
        comparison = GlslVectorComparisonName(source->bin.op);
        if (comparison != NULL)
            return GlslLowerVectorComparison(context, source, &type,
                                              comparison);
        op = GlslBinaryOperator(source->bin.op);
        if (op != GLSL_OP_NONE) {
            target = GlslNewExpr(context->module, GLSL_EXPR_BINARY, type);
            if (target == NULL)
                return NULL;
            target->u.binary.op = op;
            target->u.binary.left = GlslLowerExpr(context, source->bin.left);
            target->u.binary.right = GlslLowerExpr(context, source->bin.right);
            if (target->u.binary.left == NULL ||
                target->u.binary.right == NULL) return NULL;
            return target;
        }
    }
    if (source->common.kind == TRINARY_N &&
        (source->tri.op == COND_OP || source->tri.op == COND_V_OP ||
         source->tri.op == COND_SV_OP || source->tri.op == COND_GEN_OP))
    {
        return GlslLowerConditional(context, source, &type);
    }
    GlslRecordFailure(context, GlslUnsupportedExprReason(source));
    return NULL;
}


static int GlslLowerMatrixAssignment(GlslLowerContext *context,
    expr *source, const SourceLoc *loc, GlslStmt **list)
{
    GlslMatrixSelectorHelper *helper;
    GlslExpr *leftMatrix;
    GlslExpr *rightMatrix;
    GlslExpr *rightValue;
    GlslExpr *left;
    GlslExpr *right;
    GlslExpr *assignment;
    GlslExpr *call;
    GlslStmt *statement;
    GlslType scalarType;
    int count;
    int rightCount;
    int mask;
    int i;

    count = GlslMatrixSelectorCount(source->bin.left);
    if (count <= 1)
        return 0;
    rightCount = GlslMatrixSelectorCount(source->bin.right);
    if (rightCount != 0 && rightCount != count)
        return 0;
    leftMatrix = GlslLowerExpr(context, source->bin.left->un.arg);
    if (leftMatrix == NULL)
        return 0;
    if (!source->bin.left->un.arg->common.HasSideEffects &&
        rightCount != 0 &&
        !source->bin.right->un.arg->common.HasSideEffects &&
        source->bin.left->un.arg->common.kind == SYMB_N &&
        source->bin.left->un.arg->sym.op == VARIABLE_OP &&
        source->bin.right->un.arg->common.kind == SYMB_N &&
        source->bin.right->un.arg->sym.op == VARIABLE_OP &&
        source->bin.left->un.arg->sym.symbol !=
            source->bin.right->un.arg->sym.symbol)
    {
        rightMatrix = GlslLowerExpr(context, source->bin.right->un.arg);
        if (rightMatrix == NULL)
            return 0;
        scalarType = GlslNumericType(GLSL_BASE_FLOAT, 1);
        for (i = 0; i < count; i++) {
            left = GlslMatrixSelectorComponent(context, leftMatrix,
                                                source->bin.left, i);
            right = GlslMatrixSelectorComponent(context, rightMatrix,
                                                 source->bin.right, i);
            assignment = GlslNewExpr(context->module, GLSL_EXPR_BINARY,
                                     scalarType);
            statement = GlslNewStmt(context->module,
                                    GLSL_STMT_EXPRESSION);
            if (left == NULL || right == NULL || assignment == NULL ||
                statement == NULL) return 0;
            assignment->u.binary.op = GLSL_OP_ASSIGN;
            assignment->u.binary.left = left;
            assignment->u.binary.right = right;
            statement->u.expression = assignment;
            GlslSetLoc(&statement->loc, loc);
            GlslAppendStmt(list, statement);
        }
        return 1;
    }
    rightValue = GlslLowerExpr(context, source->bin.right);
    if (rightValue == NULL)
        return 0;
    mask = SUBOP_GET_MASK16(source->bin.left->un.subop);
    helper = GlslGetMatrixSelectorHelper(context,
        GLSL_MATRIX_SELECTOR_SET, &leftMatrix->type, &rightValue->type,
        count, mask);
    if (helper == NULL)
        return 0;
    call = GlslNewExpr(context->module, GLSL_EXPR_CALL,
                       helper->function->result);
    statement = GlslNewStmt(context->module, GLSL_STMT_EXPRESSION);
    if (call == NULL || statement == NULL)
        return 0;
    call->u.call.name = helper->function->name;
    call->u.call.arguments = leftMatrix;
    leftMatrix->next = rightValue;
    statement->u.expression = call;
    GlslSetLoc(&statement->loc, loc);
    GlslAppendStmt(list, statement);
    return 1;
}

static int GlslLowerBranch(GlslLowerContext *context, stmt *source,
                           GlslStmt **list)
{
    if (source != NULL && source->commonst.next == NULL &&
        source->commonst.kind == BLOCK_STMT)
    {
        source = source->blockst.body;
    }
    return GlslLowerStatementList(context, source, list);
}

static int GlslLowerForPart(GlslLowerContext *context, stmt *source,
                            GlslStmt **list)
{
    GlslStmt *target;

    for (; source != NULL; source = source->commonst.next) {
        context->statementLoc = source->commonst.loc;
        if (source->commonst.kind != EXPR_STMT || source->exprst.exp == NULL) {
            GlslRecordFailure(context, "GLSL for expression");
            return 0;
        }
        target = GlslNewStmt(context->module, GLSL_STMT_EXPRESSION);
        if (target == NULL)
            return 0;
        GlslSetLoc(&target->loc, &source->commonst.loc);
        target->u.expression = GlslLowerExpr(context, source->exprst.exp);
        if (target->u.expression == NULL)
            return 0;
        GlslAppendStmt(list, target);
    }
    return 1;
}

int GlslLowerStatementList(GlslLowerContext *context, stmt *source,
                                  GlslStmt **list)
{
    GlslStmt *target;

    for (; source != NULL; source = source->commonst.next) {
        context->statementLoc = source->commonst.loc;
        if (source->commonst.kind == COMMENT_STMT)
            continue;
        switch (source->commonst.kind) {
        case EXPR_STMT:
            if (source->exprst.exp == NULL)
                continue;
            if (source->exprst.exp->common.kind == BINARY_N &&
                (source->exprst.exp->bin.op == ASSIGN_OP ||
                 source->exprst.exp->bin.op == ASSIGN_V_OP ||
                 source->exprst.exp->bin.op == ASSIGN_GEN_OP) &&
                GlslMatrixSelectorCount(
                    source->exprst.exp->bin.left) > 1)
            {
                if (!GlslLowerMatrixAssignment(context,
                                                source->exprst.exp,
                                                &source->commonst.loc,
                                                list)) return 0;
                continue;
            }
            target = GlslNewStmt(context->module, GLSL_STMT_EXPRESSION);
            if (target == NULL)
                return 0;
            target->u.expression = GlslLowerExpr(context, source->exprst.exp);
            if (target->u.expression == NULL)
                return 0;
            break;
        case IF_STMT:
            target = GlslNewStmt(context->module, GLSL_STMT_IF);
            if (target == NULL)
                return 0;
            target->u.ifStmt.condition = GlslLowerExpr(context,
                                                       source->ifst.cond);
            if (target->u.ifStmt.condition == NULL ||
                !GlslLowerBranch(context, source->ifst.thenstmt,
                                 &target->u.ifStmt.trueBranch) ||
                !GlslLowerBranch(context, source->ifst.elsestmt,
                                 &target->u.ifStmt.falseBranch)) return 0;
            break;
        case WHILE_STMT:
        case DO_STMT:
            target = GlslNewStmt(context->module,
                source->commonst.kind == WHILE_STMT ?
                GLSL_STMT_WHILE : GLSL_STMT_DO);
            if (target == NULL)
                return 0;
            target->u.loop.condition = GlslLowerExpr(context,
                                                     source->whilest.cond);
            if (target->u.loop.condition == NULL)
                return 0;
            context->loopDepth++;
            if (!GlslLowerBranch(context, source->whilest.body,
                                 &target->u.loop.body)) {
                context->loopDepth--;
                return 0;
            }
            context->loopDepth--;
            break;
        case FOR_STMT:
            target = GlslNewStmt(context->module, GLSL_STMT_FOR);
            if (target == NULL)
                return 0;
            if (!GlslLowerForPart(context, source->forst.init,
                                  &target->u.forStmt.init) ||
                (source->forst.cond != NULL &&
                 (target->u.forStmt.condition = GlslLowerExpr(
                    context, source->forst.cond)) == NULL) ||
                !GlslLowerForPart(context, source->forst.step,
                                  &target->u.forStmt.step)) return 0;
            context->loopDepth++;
            if (!GlslLowerBranch(context, source->forst.body,
                                 &target->u.forStmt.body)) {
                context->loopDepth--;
                return 0;
            }
            context->loopDepth--;
            break;
        case BLOCK_STMT:
            target = GlslNewStmt(context->module, GLSL_STMT_BLOCK);
            if (target == NULL)
                return 0;
            if (!GlslLowerStatementList(context, source->blockst.body,
                                        &target->u.block)) return 0;
            break;
        case RETURN_STMT:
            target = GlslNewStmt(context->module, GLSL_STMT_RETURN);
            if (target == NULL)
                return 0;
            if (source->returnst.exp != NULL) {
                target->u.returnExpr = GlslLowerExpr(context,
                                                     source->returnst.exp);
                if (target->u.returnExpr == NULL)
                    return 0;
            }
            break;
        case DISCARD_STMT:
            if (context->module->stage != GLSL_STAGE_FRAGMENT) {
                GlslRecordFailureKind(context, GLSL_ERROR_STAGE_OPERATION,
                                      "discard");
                return 0;
            }
            if (source->discardst.cond == NULL ||
                source->discardst.cond->common.kind != UNARY_N ||
                source->discardst.cond->un.op != KILL_OP)
            {
                GlslRecordFailure(context, "GLSL discard statement");
                return 0;
            }
            if (source->discardst.cond->un.arg == NULL) {
                target = GlslNewStmt(context->module, GLSL_STMT_DISCARD);
            } else {
                GlslExpr *condition;
                GlslExpr *reduction;
                GlslStmt *discard;
                GlslType boolType;

                target = GlslNewStmt(context->module, GLSL_STMT_IF);
                discard = GlslNewStmt(context->module, GLSL_STMT_DISCARD);
                if (target == NULL || discard == NULL)
                    return 0;
                condition = GlslLowerExpr(
                    context, source->discardst.cond->un.arg);
                if (condition == NULL ||
                    condition->type.base != GLSL_BASE_BOOL ||
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
                GlslSetLoc(&discard->loc, &source->commonst.loc);
                target->u.ifStmt.trueBranch = discard;
            }
            break;
        case BREAK_STMT:
            if (context->loopDepth == 0) {
                GlslRecordFailure(context, "break outside loop");
                return 0;
            }
            target = GlslNewStmt(context->module, GLSL_STMT_BREAK);
            break;
        case CONTINUE_STMT:
            if (context->loopDepth == 0) {
                GlslRecordFailure(context, "continue outside loop");
                return 0;
            }
            target = GlslNewStmt(context->module, GLSL_STMT_CONTINUE);
            break;
        default:
            GlslRecordFailure(context, "GLSL profile statement");
            return 0;
        }
        if (target == NULL)
            return 0;
        GlslSetLoc(&target->loc, &source->commonst.loc);
        GlslAppendStmt(list, target);
    }
    return 1;
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

void GlslPrependMatrixHelpers(GlslLowerContext *context)
{
    GlslMatrixHelper *helper;
    GlslFunction *first;
    GlslFunction *last;

    first = NULL;
    last = NULL;
    for (helper = context->matrixHelpers; helper != NULL;
         helper = helper->next)
    {
        if (first == NULL)
            first = helper->function;
        else
            last->next = helper->function;
        last = helper->function;
    }
    if (last != NULL) {
        last->next = context->module->functions;
        context->module->functions = first;
    }
}

void GlslPrependMatrixSelectorHelpers(GlslLowerContext *context)
{
    GlslMatrixSelectorHelper *helper;
    GlslFunction *first;
    GlslFunction *last;

    first = NULL;
    last = NULL;
    for (helper = context->selectorHelpers; helper != NULL;
         helper = helper->next)
    {
        if (first == NULL)
            first = helper->function;
        else
            last->next = helper->function;
        last = helper->function;
    }
    if (last != NULL) {
        last->next = context->module->functions;
        context->module->functions = first;
    }
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
///////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////// Cg IR lowering (Cg 2.0 sources) //////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////

/*
 * Everything below consumes a verified Cg IR module (cg_ir.h) instead
 * of the frontend tree: declarations come from the module's ordered
 * globals/functions/DECL statements, expressions switch over
 * CgIRExprKind, and statements over CgIRStmtKind.  Uniform bindings,
 * default values, and resource limits keep reading the HAL binding
 * metadata exactly as the legacy path did: the reach-filtered IR
 * intentionally omits unreferenced declarations, while the GLSL
 * interface must stay silent about them and still emit every bound
 * uniform (documented unused-uniform silence).
 *
 * Three synthesized IR shapes are recognized and re-collapsed into the
 * historical output forms here:
 *   - matrix group reads whose shared object has side effects lower to
 *     the cg_get_matN helper call;
 *   - matrix group writes (optionally preceded by the Task 16 object or
 *     value temporaries) lower back to one cg_set_matN call or to the
 *     per-component scalar fan-out between two plain variables;
 *   - aggregate assignments flatten member-wise with cg_index /
 *     cg_aggregate temporaries, mirroring the legacy transform pass
 *     that used to run ahead of tree lowering.
 */

static GlslExpr *GlslIRLowerExprList(GlslLowerContext *context,
                                     const CgIRExpr *args);
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
 * GlslIRNeedsMaterialization() - IR twin of the legacy aggregate
 *          operand test: side effects, calls, and anything unusual must
 *          evaluate through a temporary before leaf fan-out shares it.
 */

int GlslIRNeedsMaterialization(const CgIRExpr *expr)
{
    const CgIRExpr *argument;

    if (expr == NULL)
        return 0;
    if (expr->sideEffects)
        return 1;
    switch (expr->kind) {
    case CGIR_EXPR_SYMBOL:
    case CGIR_EXPR_CONSTANT:
        return 0;
    case CGIR_EXPR_MEMBER:
        return GlslIRNeedsMaterialization(expr->u.member.object);
    case CGIR_EXPR_INDEX:
        return GlslIRNeedsMaterialization(expr->u.index.object) ||
               GlslIRNeedsMaterialization(expr->u.index.index);
    case CGIR_EXPR_LENGTH:
        return GlslIRNeedsMaterialization(expr->u.length.object);
    case CGIR_EXPR_SWIZZLE:
        return GlslIRNeedsMaterialization(expr->u.swizzle.object);
    case CGIR_EXPR_CAST:
        return GlslIRNeedsMaterialization(expr->u.cast.operand);
    case CGIR_EXPR_UNARY:
        return GlslIRNeedsMaterialization(expr->u.unary.operand);
    case CGIR_EXPR_BINARY:
        return GlslIRNeedsMaterialization(expr->u.binary.left) ||
               GlslIRNeedsMaterialization(expr->u.binary.right);
    case CGIR_EXPR_CONDITIONAL:
        return GlslIRNeedsMaterialization(
                   expr->u.conditional.condition) ||
               GlslIRNeedsMaterialization(
                   expr->u.conditional.trueExpr) ||
               GlslIRNeedsMaterialization(
                   expr->u.conditional.falseExpr);
    case CGIR_EXPR_CONSTRUCT:
        for (argument = expr->u.construct.arguments; argument != NULL;
             argument = argument->next)
        {
            if (GlslIRNeedsMaterialization(argument)) return 1;
        }
        return 0;
    default:
        return 1;
    }
} // GlslIRNeedsMaterialization

/*
 * GlslIRContainsArray() - Legacy AggregateContainsArray over canonical
 *          Cg types: any nested array makes side-effecting operand
 *          materialization unsound.
 */

static int GlslIRContainsArray(const Type *type)
{
    const Symbol *member;
    int len;
    int len2;

    len = len2 = 0;
    if (IsScalar(type) || IsVector(type, &len) ||
        IsMatrix(type, &len, &len2))
    {
        return 0;
    }
    if (GetCategory(type) == TYPE_CATEGORY_ARRAY)
        return 1;
    if (GetCategory(type) == TYPE_CATEGORY_STRUCT &&
        type->str.members != NULL)
    {
        for (member = type->str.members->symbols; member != NULL;
             member = member->next)
        {
            if (GlslIRContainsArray(member->type)) return 1;
        }
    }
    return 0;
} // GlslIRContainsArray

/*
 * GlslIRPostNormalizeNeedsMaterialization() - Operand test evaluated
 *          after index hoisting: every dynamic index already moved into
 *          its own temporary, so only remaining effects count.
 */

static int GlslIRPostNormalizeNeedsMaterialization(const CgIRExpr *expr)
{
    const CgIRExpr *argument;

    if (expr == NULL)
        return 0;
    if (expr->sideEffects)
        return 1;
    switch (expr->kind) {
    case CGIR_EXPR_SYMBOL:
    case CGIR_EXPR_CONSTANT:
    case CGIR_EXPR_INDEX:
        return 0;
    case CGIR_EXPR_MEMBER:
        return GlslIRPostNormalizeNeedsMaterialization(
            expr->u.member.object);
    case CGIR_EXPR_LENGTH:
        return GlslIRPostNormalizeNeedsMaterialization(
            expr->u.length.object);
    case CGIR_EXPR_SWIZZLE:
        return GlslIRPostNormalizeNeedsMaterialization(
            expr->u.swizzle.object);
    case CGIR_EXPR_CAST:
        return GlslIRPostNormalizeNeedsMaterialization(
            expr->u.cast.operand);
    case CGIR_EXPR_UNARY:
        return GlslIRPostNormalizeNeedsMaterialization(
            expr->u.unary.operand);
    case CGIR_EXPR_BINARY:
        return GlslIRPostNormalizeNeedsMaterialization(
                   expr->u.binary.left) ||
               GlslIRPostNormalizeNeedsMaterialization(
                   expr->u.binary.right);
    case CGIR_EXPR_CONDITIONAL:
        return GlslIRPostNormalizeNeedsMaterialization(
                   expr->u.conditional.condition) ||
               GlslIRPostNormalizeNeedsMaterialization(
                   expr->u.conditional.trueExpr) ||
               GlslIRPostNormalizeNeedsMaterialization(
                   expr->u.conditional.falseExpr);
    case CGIR_EXPR_CONSTRUCT:
        for (argument = expr->u.construct.arguments; argument != NULL;
             argument = argument->next)
        {
            if (GlslIRPostNormalizeNeedsMaterialization(argument))
                return 1;
        }
        return 0;
    default:
        return 1;
    }
} // GlslIRPostNormalizeNeedsMaterialization

/*
 * GlslCloneExpr() - Deep copy of a lowered GLSL expression.  Aggregate
 *          fan-out duplicates the base object per leaf exactly as the
 *          legacy AssignAggregate duplicated its operands.
 */

GlslExpr *GlslCloneExpr(GlslModule *module, const GlslExpr *expr)
{
    GlslExpr *clone;
    GlslExpr *last;
    GlslExpr *argument;

    if (expr == NULL)
        return NULL;
    clone = (GlslExpr *) module->alloc(module->allocArg, sizeof(GlslExpr));
    if (clone == NULL)
        return NULL;
    *clone = *expr;
    clone->next = NULL;
    switch (clone->kind) {
    case GLSL_EXPR_UNARY:
        clone->u.unary.operand = GlslCloneExpr(module,
                                               expr->u.unary.operand);
        if (clone->u.unary.operand == NULL) return NULL;
        break;
    case GLSL_EXPR_BINARY:
        clone->u.binary.left = GlslCloneExpr(module,
                                             expr->u.binary.left);
        clone->u.binary.right = GlslCloneExpr(module,
                                              expr->u.binary.right);
        if (clone->u.binary.left == NULL || clone->u.binary.right == NULL)
            return NULL;
        break;
    case GLSL_EXPR_CONDITIONAL:
        clone->u.conditional.condition = GlslCloneExpr(
            module, expr->u.conditional.condition);
        clone->u.conditional.trueExpr = GlslCloneExpr(
            module, expr->u.conditional.trueExpr);
        clone->u.conditional.falseExpr = GlslCloneExpr(
            module, expr->u.conditional.falseExpr);
        if (clone->u.conditional.condition == NULL ||
            clone->u.conditional.trueExpr == NULL ||
            clone->u.conditional.falseExpr == NULL) return NULL;
        break;
    case GLSL_EXPR_CALL:
        last = NULL;
        for (argument = expr->u.call.arguments; argument != NULL;
             argument = argument->next)
        {
            GlslExpr *copy = GlslCloneExpr(module, argument);

            if (copy == NULL)
                return NULL;
            if (last == NULL)
                clone->u.call.arguments = copy;
            else
                last->next = copy;
            last = copy;
        }
        break;
    case GLSL_EXPR_CONSTRUCT:
        last = NULL;
        for (argument = expr->u.construct.arguments; argument != NULL;
             argument = argument->next)
        {
            GlslExpr *copy = GlslCloneExpr(module, argument);

            if (copy == NULL)
                return NULL;
            if (last == NULL)
                clone->u.construct.arguments = copy;
            else
                last->next = copy;
            last = copy;
        }
        break;
    case GLSL_EXPR_MEMBER:
        clone->u.member.object = GlslCloneExpr(module,
                                               expr->u.member.object);
        if (clone->u.member.object == NULL) return NULL;
        break;
    case GLSL_EXPR_INDEX:
        clone->u.index.object = GlslCloneExpr(module,
                                              expr->u.index.object);
        clone->u.index.index = GlslCloneExpr(module,
                                             expr->u.index.index);
        if (clone->u.index.object == NULL || clone->u.index.index == NULL)
            return NULL;
        break;
    case GLSL_EXPR_SWIZZLE:
        clone->u.swizzle.object = GlslCloneExpr(module,
                                                expr->u.swizzle.object);
        if (clone->u.swizzle.object == NULL) return NULL;
        break;
    default:
        break;
    }
    return clone;
} // GlslCloneExpr

static GlslExpr *GlslIRDeclRef(GlslLowerContext *context,
                               const GlslDecl *decl)
{
    GlslExpr *ref;

    ref = GlslNewExpr(context->module, GLSL_EXPR_SYMBOL, decl->type);
    if (ref != NULL)
        ref->u.symbol = (GlslDecl *) decl;
    return ref;
} // GlslIRDeclRef

static GlslExpr *GlslIRAppendMemberStep(GlslLowerContext *context,
                                        GlslExpr *base,
                                        const GlslDecl *member,
                                        const SourceLoc *loc)
{
    GlslExpr *step;

    step = GlslNewExpr(context->module, GLSL_EXPR_MEMBER,
                       member->type);
    if (step == NULL)
        return NULL;
    GlslSetLoc(&step->loc, loc);
    step->u.member.object = base;
    step->u.member.decl = (GlslDecl *) member;
    step->u.member.name = member->name;
    return step;
} // GlslIRAppendMemberStep

static GlslExpr *GlslIRAppendIndexStep(GlslLowerContext *context,
                                       GlslExpr *base, int index,
                                       GlslType type,
                                       const SourceLoc *loc)
{
    GlslExpr *literal;
    GlslExpr *step;
    GlslType intType;

    intType = GlslNumericType(GLSL_BASE_INT, 1);
    literal = GlslNewExpr(context->module, GLSL_EXPR_INT, intType);
    step = GlslNewExpr(context->module, GLSL_EXPR_INDEX, type);
    if (literal == NULL || step == NULL)
        return NULL;
    literal->u.literalInt = index;
    GlslSetLoc(&step->loc, loc);
    step->u.index.object = base;
    step->u.index.index = literal;
    return step;
} // GlslIRAppendIndexStep

static GlslExpr *GlslIRAppendDynamicIndex(GlslLowerContext *context,
                                          GlslExpr *object,
                                          GlslExpr *index,
                                          GlslType type,
                                          const SourceLoc *loc)
{
    GlslExpr *step;

    step = GlslNewExpr(context->module, GLSL_EXPR_INDEX, type);
    if (step == NULL)
        return NULL;
    GlslSetLoc(&step->loc, loc);
    step->u.index.object = object;
    step->u.index.index = index;
    return step;
} // GlslIRAppendDynamicIndex

/*
 * GlslIRChooseFlattenName() - First free cg_index/cg_aggregate-style
 *          name within the current function's declarations, mirroring
 *          the legacy flatten-temp naming search against the function
 *          scope.
 */

static void GlslIRChooseFlattenName(GlslLowerContext *context,
                                    const char *prefix, char *candidate,
                                    size_t size)
{
    const GlslDecl *decl;
    int index;
    int taken;

    for (index = 0;; index++) {
        if (index == 0)
            strcpy(candidate, prefix);
        else
            sprintf(candidate, "%s%d", prefix, index);
        taken = 0;
        for (decl = context->function->parameters; decl != NULL && !taken;
             decl = decl->next)
        {
            taken = decl->name != NULL &&
                    !strcmp(decl->name, candidate);
        }
        for (decl = context->function->locals; decl != NULL && !taken;
             decl = decl->next)
        {
            taken = decl->name != NULL &&
                    !strcmp(decl->name, candidate);
        }
        if (!taken)
            break;
    }
} // GlslIRChooseFlattenName

static Symbol *GlslIRNewTempSymbol(GlslLowerContext *context,
                                   Type *type, const char *name,
                                   const SourceLoc *loc)
{
    Symbol *temp;

    temp = (Symbol *) (calloc)(1, sizeof(Symbol));
    if (temp == NULL)
        return NULL;
    temp->name = LookUpAddString(atable, name);
    temp->type = type;
    if (loc != NULL)
        temp->loc = *loc;
    return temp;
} // GlslIRNewTempSymbol

/*
 * GlslIRAddLocal() - Register one local declaration (including
 *          synthesized temporaries) into the current function.  Sorted
 *          insertion reproduces the legacy locals ordering.
 */

GlslDecl *GlslIRAddLocal(GlslLowerContext *context, Symbol *symbol,
                                Type *type, const SourceLoc *loc)
{
    GlslDecl *decl;
    GlslType glslType;
    const char *sourceName;
    const char *name;
    const void *identity;
    const void *nameSpace;

    if (!GlslIRType(context, type, &glslType, loc))
        return NULL;
    sourceName = GetAtomString(atable, symbol->name);
    if (sourceName == NULL)
        return NULL;
    identity = symbol != NULL ? (const void *) symbol
                              : (const void *) type;
    nameSpace = context->function->isEntry ? NULL : context->function;
    if (nameSpace != NULL) {
        name = GlslAllocateScopedSymbolNameForSource(context, nameSpace,
                                                     identity, sourceName,
                                                     loc);
    } else {
        name = GlslAllocateSymbolNameForSource(context, identity,
                                               sourceName, loc);
    }
    if (name == NULL)
        return NULL;
    decl = GlslNewDecl(context->module, GLSL_STORAGE_NONE, glslType, name);
    if (decl == NULL)
        return NULL;
    decl->identity = identity;
    GlslSetLoc(&decl->loc, loc);
    decl->sourceOrdinal = symbol != NULL ? symbol->sourceOrdinal : 0;
    GlslInsertDecl(&context->function->locals, decl);
    return decl;
} // GlslIRAddLocal

/*
 * GlslIRNewIndexTemp() - Hoist one dynamic aggregate index into its own
 *          cg_index temporary exactly once.
 */

static GlslDecl *GlslIRNewIndexTemp(GlslLowerContext *context, Type *type,
                                    const CgIRExpr *index,
                                    GlslStmt **list)
{
    Symbol *temp;
    GlslDecl *decl;
    GlslExpr *init;
    GlslExpr *ref;
    GlslExpr *assign;
    GlslStmt *stmt;
    char candidate[64];

    GlslIRChooseFlattenName(context, "cg_index", candidate,
                            sizeof(candidate));
    temp = GlslIRNewTempSymbol(context, type, candidate, &index->loc);
    if (temp == NULL)
        return NULL;
    decl = GlslIRAddLocal(context, temp, type, &index->loc);
    if (decl == NULL)
        return NULL;
    ref = GlslIRDeclRef(context, decl);
    init = GlslIRLowerExpr(context, index);
    assign = NULL;
    stmt = NULL;
    if (ref != NULL && init != NULL) {
        assign = GlslNewExpr(context->module, GLSL_EXPR_BINARY,
                             decl->type);
        if (assign != NULL) {
            assign->u.binary.op = GLSL_OP_ASSIGN;
            assign->u.binary.left = ref;
            assign->u.binary.right = init;
            stmt = GlslNewStmt(context->module, GLSL_STMT_EXPRESSION);
            if (stmt != NULL) {
                GlslSetLoc(&stmt->loc, &index->loc);
                stmt->u.expression = assign;
                GlslAppendStmt(list, stmt);
            }
        }
    }
    if (ref == NULL || init == NULL || assign == NULL || stmt == NULL)
        return NULL;
    return decl;
} // GlslIRNewIndexTemp

/*
 * GlslIREmitAggregateLeaves() - Recursive leaf fan-out of one aggregate
 *          assignment; scalar, vector, and matrix leaves become one
 *          plain assignment each.
 */

static int GlslIREmitAggregateLeaves(GlslLowerContext *context,
                                     const GlslType *type,
                                     GlslExpr *target, GlslExpr *value,
                                     const SourceLoc *loc,
                                     GlslStmt **list)
{
    const GlslDecl *member;
    GlslExpr *targetLeaf;
    GlslExpr *valueLeaf;
    GlslExpr *assign;
    GlslStmt *stmt;
    int i;

    if (type->elementType != NULL) {
        for (i = 0; i < type->arraySize; i++) {
            targetLeaf = GlslIRAppendIndexStep(context,
                GlslCloneExpr(context->module, target), i,
                *type->elementType, loc);
            valueLeaf = GlslIRAppendIndexStep(context,
                GlslCloneExpr(context->module, value), i,
                *type->elementType, loc);
            if (targetLeaf == NULL || valueLeaf == NULL) return 0;
            if (!GlslIREmitAggregateLeaves(context, type->elementType,
                                           targetLeaf, valueLeaf, loc,
                                           list)) return 0;
        }
        return 1;
    }
    if (type->base == GLSL_BASE_STRUCT) {
        for (member = type->members; member != NULL;
             member = member->next)
        {
            targetLeaf = GlslIRAppendMemberStep(context,
                GlslCloneExpr(context->module, target), member, loc);
            valueLeaf = GlslIRAppendMemberStep(context,
                GlslCloneExpr(context->module, value), member, loc);
            if (targetLeaf == NULL || valueLeaf == NULL) return 0;
            if (!GlslIREmitAggregateLeaves(context, &member->type,
                                           targetLeaf, valueLeaf, loc,
                                           list)) return 0;
        }
        return 1;
    }
    assign = GlslNewExpr(context->module, GLSL_EXPR_BINARY, *type);
    stmt = GlslNewStmt(context->module, GLSL_STMT_EXPRESSION);
    if (assign == NULL || stmt == NULL)
        return 0;
    assign->u.binary.op = GLSL_OP_ASSIGN;
    assign->u.binary.left = target;
    assign->u.binary.right = value;
    GlslSetLoc(&stmt->loc, loc);
    stmt->u.expression = assign;
    GlslAppendStmt(list, stmt);
    return 1;
} // GlslIREmitAggregateLeaves

/*
 * GlslIRLowerMaterialized() - Lower an aggregate operand while hoisting
 *          every dynamic index into a cg_index temporary (children
 *          first, matching the legacy traversal order).
 */

static GlslExpr *GlslIRLowerMaterialized(GlslLowerContext *context,
                                         const CgIRExpr *expr,
                                         GlslStmt **list)
{
    GlslExpr *object;
    GlslExpr *index;
    GlslType type;

    if (expr == NULL)
        return NULL;
    switch (expr->kind) {
    case CGIR_EXPR_INDEX:
        object = GlslIRLowerMaterialized(context, expr->u.index.object,
                                         list);
        if (object == NULL)
            return NULL;
        if (!GlslIRType(context, expr->type, &type, &expr->loc))
            return NULL;
        if (GlslIRNeedsMaterialization(expr->u.index.index)) {
            GlslDecl *temp;

            temp = GlslIRNewIndexTemp(context, expr->u.index.index->type,
                                      expr->u.index.index, list);
            if (temp == NULL)
                return NULL;
            index = GlslIRDeclRef(context, temp);
            if (index == NULL)
                return NULL;
        } else {
            index = GlslIRLowerExpr(context, expr->u.index.index);
            if (index == NULL)
                return NULL;
        }
        return GlslIRAppendDynamicIndex(context, object, index, type,
                                        &expr->loc);
    case CGIR_EXPR_MEMBER: {
        GlslDecl *memberDecl;

        object = GlslIRLowerMaterialized(context, expr->u.member.object,
                                         list);
        if (object == NULL)
            return NULL;
        if (!GlslIRType(context, expr->type, &type, &expr->loc))
            return NULL;
        memberDecl = GlslFindDecl(context, expr->u.member.member);
        if (memberDecl == NULL)
            return NULL;
        return GlslIRAppendMemberStep(context, object, memberDecl,
                                      &expr->loc);
    }
    case CGIR_EXPR_SWIZZLE: {
        char maskText[5];
        int i;

        object = GlslIRLowerMaterialized(context, expr->u.swizzle.object,
                                         list);
        if (object == NULL)
            return NULL;
        if (!GlslIRType(context, expr->type, &type, &expr->loc))
            return NULL;
        for (i = 0; i < expr->u.swizzle.componentCount; i++)
            maskText[i] = "xyzw"[(expr->u.swizzle.mask >> (i * 2)) & 3];
        maskText[expr->u.swizzle.componentCount] = '\0';
        return GlslNewSwizzle(context, object, &type, maskText);
    }
    default:
        return GlslIRLowerExpr(context, expr);
    }
} // GlslIRLowerMaterialized

/*
 * GlslIRNewAggregateTemp() - The cg_aggregate materialization temporary
 *          holding one aggregate right-hand side.
 */

static GlslDecl *GlslIRNewAggregateTemp(GlslLowerContext *context,
                                        Type *type,
                                        const CgIRExpr *assign,
                                        GlslExpr *materializedValue,
                                        GlslStmt **list)
{
    Symbol *temp;
    GlslDecl *decl;
    GlslExpr *ref;
    GlslExpr *assignExpr;
    GlslStmt *stmt;
    GlslType declType;
    char candidate[64];

    GlslIRChooseFlattenName(context, "cg_aggregate", candidate,
                            sizeof(candidate));
    temp = GlslIRNewTempSymbol(context, type, candidate, &assign->loc);
    if (temp == NULL)
        return NULL;
    if (!GlslIRType(context, type, &declType, &assign->loc))
        return NULL;
    decl = GlslIRAddLocal(context, temp, type, &assign->loc);
    if (decl == NULL)
        return NULL;
    ref = GlslIRDeclRef(context, decl);
    assignExpr = NULL;
    stmt = NULL;
    if (ref != NULL && materializedValue != NULL) {
        assignExpr = GlslNewExpr(context->module, GLSL_EXPR_BINARY,
                                 declType);
        if (assignExpr != NULL) {
            assignExpr->u.binary.op = GLSL_OP_ASSIGN;
            assignExpr->u.binary.left = ref;
            assignExpr->u.binary.right = materializedValue;
            stmt = GlslNewStmt(context->module, GLSL_STMT_EXPRESSION);
            if (stmt != NULL) {
                GlslSetLoc(&stmt->loc, &assign->loc);
                stmt->u.expression = assignExpr;
                GlslAppendStmt(list, stmt);
            }
        }
    }
    if (ref == NULL || materializedValue == NULL || assignExpr == NULL ||
        stmt == NULL)
    {
        return NULL;
    }
    return decl;
} // GlslIRNewAggregateTemp

/*
 * GlslIRLowerAggregateAssign() - Struct/array assignment fan-out: index
 *          temporaries, the side-effecting-aggregate-with-arrays
 *          rejection, optional cg_aggregate materialization, then one
 *          assignment per scalar/vector/matrix leaf.
 */

int GlslIRLowerAggregateAssign(GlslLowerContext *context,
                                      const CgIRExpr *assign,
                                      GlslStmt **list)
{
    Type *aggregateType;
    GlslType leafRoot;
    GlslExpr *target;
    GlslExpr *value;
    GlslDecl *temp;
    GlslExpr *ref;

    aggregateType = assign->type;
    context->statementLoc = assign->loc;
    target = GlslIRLowerMaterialized(context, assign->u.assign.target,
                                     list);
    if (target == NULL)
        return 0;
    value = GlslIRLowerMaterialized(context, assign->u.assign.value, list);
    if (value == NULL)
        return 0;
    if (GlslIRPostNormalizeNeedsMaterialization(assign->u.assign.value))
    {
        if (GlslIRContainsArray(aggregateType)) {
            GlslRecordFailureKindAt(context,
                GLSL_ERROR_UNSUPPORTED_OPERATION,
                "side-effecting aggregate with arrays", &assign->loc);
            return 0;
        }
        temp = GlslIRNewAggregateTemp(context, aggregateType, assign,
                                      value, list);
        if (temp == NULL)
            return 0;
        ref = GlslIRDeclRef(context, temp);
        if (ref == NULL)
            return 0;
        value = ref;
    }
    if (!GlslIRType(context, aggregateType, &leafRoot, &assign->loc))
        return 0;
    return GlslIREmitAggregateLeaves(context, &leafRoot, target, value,
                                     &assign->loc, list);
} // GlslIRLowerAggregateAssign


/*
 * GlslIRUnaryOperator() / GlslIRBinaryOperator() - Cg IR op identity to
 *          the GLSL writer's operator enum.
 */

static GlslOperator GlslIRUnaryOperator(CgIROp op)
{
    switch (op) {
    case CGIR_OP_NEGATE: return GLSL_OP_NEGATE;
    case CGIR_OP_POSITIVE: return GLSL_OP_POSITIVE;
    case CGIR_OP_LOGICAL_NOT: return GLSL_OP_LOGICAL_NOT;
    default: return GLSL_OP_NONE;
    }
} // GlslIRUnaryOperator

static GlslOperator GlslIRBinaryOperator(CgIROp op)
{
    switch (op) {
    case CGIR_OP_MULTIPLY: return GLSL_OP_MULTIPLY;
    case CGIR_OP_DIVIDE: return GLSL_OP_DIVIDE;
    case CGIR_OP_ADD: return GLSL_OP_ADD;
    case CGIR_OP_SUBTRACT: return GLSL_OP_SUBTRACT;
    case CGIR_OP_LESS: return GLSL_OP_LESS;
    case CGIR_OP_GREATER: return GLSL_OP_GREATER;
    case CGIR_OP_LESS_EQUAL: return GLSL_OP_LESS_EQUAL;
    case CGIR_OP_GREATER_EQUAL: return GLSL_OP_GREATER_EQUAL;
    case CGIR_OP_EQUAL: return GLSL_OP_EQUAL;
    case CGIR_OP_NOT_EQUAL: return GLSL_OP_NOT_EQUAL;
    case CGIR_OP_LOGICAL_AND: return GLSL_OP_LOGICAL_AND;
    case CGIR_OP_LOGICAL_OR: return GLSL_OP_LOGICAL_OR;
    default: return GLSL_OP_NONE;
    }
} // GlslIRBinaryOperator

static const char *GlslIRComparisonName(CgIROp op)
{
    switch (op) {
    case CGIR_OP_LESS: return "lessThan";
    case CGIR_OP_GREATER: return "greaterThan";
    case CGIR_OP_LESS_EQUAL: return "lessThanEqual";
    case CGIR_OP_GREATER_EQUAL: return "greaterThanEqual";
    case CGIR_OP_EQUAL: return "equal";
    case CGIR_OP_NOT_EQUAL: return "notEqual";
    default: return NULL;
    }
} // GlslIRComparisonName

/*
 * GlslIRUnsupportedReason() - Historical unsupported-operation text for
 *          the operator families outside the focused GLSL profile.
 */

static const char *GlslIRUnsupportedReason(CgIROp op)
{
    switch (op) {
    case CGIR_OP_MODULO:
    case CGIR_OP_MODULO_ASSIGN:
        return "remainder (%)";
    case CGIR_OP_SHIFT_LEFT:
    case CGIR_OP_SHIFT_RIGHT:
        return "shift operator";
    case CGIR_OP_BITWISE_AND:
    case CGIR_OP_BITWISE_XOR:
    case CGIR_OP_BITWISE_OR:
    case CGIR_OP_BITWISE_NOT:
        return "bitwise operator";
    default:
        return NULL;
    }
} // GlslIRUnsupportedReason

static GlslOperator GlslIRCompoundOperator(CgIROp op)
{
    switch (op) {
    case CGIR_OP_ADD_ASSIGN: return GLSL_OP_ADD;
    case CGIR_OP_SUBTRACT_ASSIGN: return GLSL_OP_SUBTRACT;
    case CGIR_OP_MULTIPLY_ASSIGN: return GLSL_OP_MULTIPLY;
    case CGIR_OP_DIVIDE_ASSIGN: return GLSL_OP_DIVIDE;
    default: return GLSL_OP_NONE;
    }
} // GlslIRCompoundOperator

/*
 * GlslIRSharedSelection() - Recognize the Task 16 group-read encoding:
 *          a constructor whose components are nested constant selections
 *      over ONE shared object node.  Node sharing identifies synthesized
 *          selector groups; user-written duplicates never share nodes.
 */

int GlslIRSharedSelection(const CgIRExpr *expr,
                                 const CgIRExpr **objectOut,
                                 int *countOut, int *maskOut)
{
    const CgIRExpr *argument;
    const CgIRExpr *object;
    int count;
    int mask;
    int i;

    if (expr == NULL || expr->kind != CGIR_EXPR_CONSTRUCT)
        return 0;
    count = 0;
    for (argument = expr->u.construct.arguments; argument != NULL;
         argument = argument->next)
        count++;
    if (count < 2 || count > 4)
        return 0;
    object = NULL;
    mask = 0;
    i = 0;
    for (argument = expr->u.construct.arguments; argument != NULL;
         argument = argument->next, i++)
    {
        const CgIRExpr *rowSel;
        const CgIRExpr *columnSel;
        int row;
        int column;

        if (argument->kind != CGIR_EXPR_INDEX)
            return 0;
        columnSel = argument->u.index.index;
        if (columnSel == NULL || columnSel->kind != CGIR_EXPR_CONSTANT ||
            columnSel->u.constant.kind != CG_SCALAR_INT)
            return 0;
        rowSel = argument->u.index.object;
        if (rowSel == NULL || rowSel->kind != CGIR_EXPR_INDEX ||
            rowSel->u.index.index == NULL ||
            rowSel->u.index.index->kind != CGIR_EXPR_CONSTANT ||
            rowSel->u.index.index->u.constant.kind != CG_SCALAR_INT)
        {
            return 0;
        }
        if (object == NULL) {
            object = rowSel->u.index.object;
        } else if (object != rowSel->u.index.object) {
            return 0;
        }
        row = (int) rowSel->u.index.index->u.constant.value.i;
        column = (int) columnSel->u.constant.value.i;
        if (row < 0 || row > 3 || column < 0 || column > 3) return 0;
        mask |= ((row << 2) | column) << (i * 4);
    }
    if (object == NULL)
        return 0;
    *objectOut = object;
    *countOut = count;
    *maskOut = mask;
    return 1;
} // GlslIRSharedSelection

static GlslExpr *GlslIRConstantComponent(GlslLowerContext *context,
                                         const CgNumericValue *value,
                                         GlslBase base)
{
    float component;

    if (base == GLSL_BASE_FLOAT) {
        component = (float) value->value.f;

        if (!GlslFiniteDefaultFloat(component)) {
            GlslRecordFailure(context,
                              "non-finite floating-point constant");
            return NULL;
        }
        return GlslNewLiteral(context, base, 0, component);
    }
    return GlslNewLiteral(context, base, (int) value->value.i, 0.0f);
} // GlslIRConstantComponent

static GlslExpr *GlslIRLowerConstant(GlslLowerContext *context,
                                     const CgIRExpr *expr)
{
    GlslType type;

    if (!GlslIRType(context, expr->type, &type, &expr->loc))
        return NULL;
    return GlslIRConstantComponent(context, &expr->u.constant, type.base);
} // GlslIRLowerConstant

static GlslExpr *GlslIRLowerSwizzle(GlslLowerContext *context,
                                    const CgIRExpr *expr)
{
    GlslExpr *object;
    GlslType type;
    char maskText[5];
    int count;
    int i;

    object = GlslIRLowerExpr(context, expr->u.swizzle.object);
    if (object == NULL)
        return NULL;
    if (!GlslIRType(context, expr->type, &type, &expr->loc))
        return NULL;
    count = expr->u.swizzle.componentCount;
    if (count < 1 || count > 4)
        return NULL;
    for (i = 0; i < count; i++)
        maskText[i] = "xyzw"[(expr->u.swizzle.mask >> (i * 2)) & 3];
    maskText[count] = '\0';
    if (object->type.len == 1) {
        GlslExpr *target;

        if (type.len == 1)
            return object;
        target = GlslNewExpr(context->module, GLSL_EXPR_CONSTRUCT, type);
        if (target != NULL)
            target->u.construct.arguments = object;
        return target;
    }
    return GlslNewSwizzle(context, object, &type, maskText);
} // GlslIRLowerSwizzle

/*
 * GlslIRLowerMatrixConstructor() - Row-major source arguments into a
 *          column-major GLSL constructor, or the impure-helper call.
 */

static GlslExpr *GlslIRLowerMatrixConstructor(GlslLowerContext *context,
                                              const CgIRExpr *expr,
                                              const GlslType *type)
{
    GlslExpr *arguments[GLSL_MATRIX_MAX_ARGUMENTS];
    GlslExpr *argument;
    GlslExpr *next;
    GlslExpr *target;
    GlslExpr *component;
    GlslType scalarType;
    const CgIRExpr *sourceArgument;
    char mask[2];
    int size;
    int count;
    int componentIndex;
    int componentCount;
    int row;
    int column;
    int hasSideEffects;

    size = type->rows;
    if (size < 2 || size > 4 || type->cols != size)
        return NULL;
    hasSideEffects = 0;
    for (sourceArgument = expr->u.construct.arguments;
         sourceArgument != NULL;
         sourceArgument = sourceArgument->next)
    {
        if (sourceArgument->sideEffects)
            hasSideEffects = 1;
    }
    argument = GlslIRLowerExprList(context, expr->u.construct.arguments);
    if (argument == NULL && expr->u.construct.arguments != NULL)
        return NULL;
    /* Keep impure expressions at the constructor call site and use each
       exactly once; argument evaluation order remains language-defined. */
    if (hasSideEffects)
        return GlslLowerImpureMatrixConstructor(context, argument, type);
    count = 0;
    while (argument != NULL) {
        next = argument->next;
        argument->next = NULL;
        componentCount = argument->type.len;
        if (!GlslMatrixNumericParameterType(&argument->type) ||
            count > size * size - componentCount)
        {
            if (!GlslMatrixNumericParameterType(&argument->type)) {
                GlslRecordFailureKind(context,
                                      GLSL_ERROR_UNSUPPORTED_TYPE,
                                      "matrix constructor argument type");
            }
            return NULL;
        }
        if (componentCount == 1) {
            arguments[count++] = argument;
        } else {
            for (componentIndex = 0;
                 componentIndex < componentCount;
                 componentIndex++)
            {
                scalarType = GlslNumericType(argument->type.base, 1);
                mask[0] = "xyzw"[componentIndex];
                mask[1] = '\0';
                component = GlslNewSwizzle(context, argument,
                                           &scalarType, mask);
                if (component == NULL)
                    return NULL;
                arguments[count++] = component;
            }
        }
        argument = next;
    }
    if (count != size * size)
        return NULL;
    target = GlslNewExpr(context->module, GLSL_EXPR_CONSTRUCT, *type);
    if (target == NULL)
        return NULL;
    for (column = 0; column < size; column++) {
        for (row = 0; row < size; row++) {
            GlslAppendExpr(&target->u.construct.arguments,
                           arguments[row * size + column]);
        }
    }
    return target;
} // GlslIRLowerMatrixConstructor

static GlslExpr *GlslIRLowerExprList(GlslLowerContext *context,
                                     const CgIRExpr *args)
{
    GlslExpr *list;
    GlslExpr *item;

    list = NULL;
    for (; args != NULL; args = args->next) {
        item = GlslIRLowerExpr(context, args);
        if (item == NULL)
            return NULL;
        GlslAppendExpr(&list, item);
    }
    return list;
} // GlslIRLowerExprList

/*
 * GlslIRLowerTextureArguments() - tex* intrinsics keep their historical
 *          shape: a direct bound sampler uniform followed by the
 *          coordinate.
 */

static GlslExpr *GlslIRLowerTextureArguments(GlslLowerContext *context,
                                             const CgIRExpr *args)
{
    GlslExpr *sampler;
    GlslExpr *coord;
    GlslDecl *decl;

    if (args == NULL || args->next == NULL || args->next->next != NULL) {
        GlslRecordFailureKind(context, GLSL_ERROR_INTRINSIC,
                              "texture intrinsic");
        return NULL;
    }
    if (args->kind != CGIR_EXPR_SYMBOL ||
        args->u.symbol == NULL)
    {
        GlslRecordFailureKind(context, GLSL_ERROR_SAMPLER,
            "texture sampler argument must be a direct bound uniform");
        return NULL;
    }
    decl = GlslFindDecl(context, args->u.symbol);
    if (decl == NULL || decl->storage != GLSL_STORAGE_SAMPLER ||
        !GlslIsSamplerType(&decl->type))
    {
        GlslRecordFailureKind(context, GLSL_ERROR_SAMPLER,
            "texture sampler argument must be a direct bound uniform");
        return NULL;
    }
    sampler = GlslNewExpr(context->module, GLSL_EXPR_SYMBOL, decl->type);
    if (sampler == NULL)
        return NULL;
    sampler->u.symbol = decl;
    coord = GlslIRLowerExpr(context, args->next);
    if (coord == NULL)
        return NULL;
    sampler->next = coord;
    return sampler;
} // GlslIRLowerTextureArguments

/*
 * GlslIRLowerCallCore() - User helper calls and catalog intrinsics.
 *      The mul/dot-scalar and saturate special shapes keep their
 *      legacy expansions; texture families keep their direct-uniform
 *          sampler rule through GlslValidateTextureCall above.
 */

static GlslExpr *GlslIRLowerCallCore(GlslLowerContext *context,
                                     Symbol *symbol,
                                     const CgIntrinsicSignature *signature,
                                     const CgIRExpr *argumentSource,
                                     const GlslType *type)
{
    GlslExpr *target;
    GlslExpr *arguments;
    GlslExpr *left;
    GlslExpr *right;
    GlslExpr *zero;
    GlslExpr *one;
    GlslFunction *function;
    const char *name;
    GlslBuiltin builtin;
    GlslType scalarType;

    function = symbol != NULL ?
               GlslFindFunction(context->module, symbol) : NULL;
    builtin = GLSL_BUILTIN_NONE;
    if (function != NULL) {
        name = function->name;
    } else {
        /* Lowering is keyed on the stable intrinsic identity carried by
         * the selected signature, never on a name lookup.  A cataloged
         * intrinsic without an exact GLSL 1.50 lowering fails profile
         * validation with the existing intrinsic diagnostic. */
        builtin = signature != NULL ?
                  GlslIntrinsicBuiltin(signature->intrinsic) :
                  GLSL_BUILTIN_NONE;
        if (builtin == GLSL_BUILTIN_NONE) {
            GlslRecordFailureKind(context, GLSL_ERROR_INTRINSIC,
                signature != NULL ? signature->name :
                symbol != NULL ? GetAtomString(atable, symbol->name) :
                                 NULL);
            return NULL;
        }
        name = GlslBuiltinSpelling(builtin);
        if (name == NULL)
            return NULL;
    }
    arguments = NULL;
    if (argumentSource != NULL) {
        if (function == NULL && builtin >= GLSL_BUILTIN_TEX1D &&
            builtin <= GLSL_BUILTIN_TEXCUBE_PROJ)
        {
            arguments = GlslIRLowerTextureArguments(context,
                                                    argumentSource);
        } else {
            arguments = GlslIRLowerExprList(context, argumentSource);
        }
        if (arguments == NULL)
            return NULL;
    }
    if (function == NULL) {
        if (!GlslValidateTextureCall(context, builtin, type, arguments))
            return NULL;
        if (builtin == GLSL_BUILTIN_MUL ||
            (builtin == GLSL_BUILTIN_DOT &&
             arguments != NULL && arguments->type.len == 1))
        {
            left = arguments;
            right = left != NULL ? left->next : NULL;
            if (left == NULL || right == NULL || right->next != NULL)
                return NULL;
            left->next = NULL;
            right->next = NULL;
            target = GlslNewExpr(context->module, GLSL_EXPR_BINARY, *type);
            if (target == NULL)
                return NULL;
            target->u.binary.op = GLSL_OP_MULTIPLY;
            target->u.binary.left = left;
            target->u.binary.right = right;
            return target;
        }
        if (builtin == GLSL_BUILTIN_SATURATE) {
            if (arguments == NULL || arguments->next != NULL)
                return NULL;
            scalarType = GlslNumericType(GLSL_BASE_FLOAT, 1);
            zero = GlslNewLiteral(context, GLSL_BASE_FLOAT, 0, 0.0f);
            one = GlslNewLiteral(context, GLSL_BASE_FLOAT, 0, 1.0f);
            if (zero == NULL || one == NULL)
                return NULL;
            if (type->len > 1) {
                target = GlslNewExpr(context->module,
                                     GLSL_EXPR_CONSTRUCT, *type);
                if (target == NULL)
                    return NULL;
                target->u.construct.arguments = zero;
                zero = target;
                target = GlslNewExpr(context->module,
                                     GLSL_EXPR_CONSTRUCT, *type);
                if (target == NULL)
                    return NULL;
                target->u.construct.arguments = one;
                one = target;
            } else if (!GlslTypesEqual(type, &scalarType)) {
                return NULL;
            }
            arguments->next = zero;
            zero->next = one;
        }
    }
    target = GlslNewExpr(context->module, GLSL_EXPR_CALL, *type);
    if (target == NULL)
        return NULL;
    target->u.call.name = name;
    target->u.call.arguments = arguments;
    target->u.call.builtin = function == NULL ? builtin : GLSL_BUILTIN_NONE;
    return target;
} // GlslIRLowerCallCore

static GlslExpr *GlslIRLowerVectorComparison(GlslLowerContext *context,
                                             const CgIRExpr *expr,
                                             const GlslType *type,
                                             const char *name)
{
    GlslExpr *target;
    GlslExpr *constructor;
    GlslExpr *left;
    GlslExpr *right;
    GlslType vectorType;

    left = GlslIRLowerExpr(context, expr->u.binary.left);
    right = left ? GlslIRLowerExpr(context, expr->u.binary.right) : NULL;
    if (left == NULL || right == NULL)
        return NULL;
    if (left->type.len == 1 && type->len > 1) {
        vectorType = GlslNumericType(left->type.base, type->len);
        constructor = GlslNewExpr(context->module, GLSL_EXPR_CONSTRUCT,
                                  vectorType);
        if (constructor == NULL)
            return NULL;
        constructor->u.construct.arguments = left;
        left = constructor;
    }
    if (right->type.len == 1 && type->len > 1) {
        vectorType = GlslNumericType(right->type.base, type->len);
        constructor = GlslNewExpr(context->module, GLSL_EXPR_CONSTRUCT,
                                  vectorType);
        if (constructor == NULL)
            return NULL;
        constructor->u.construct.arguments = right;
        right = constructor;
    }
    target = GlslNewExpr(context->module, GLSL_EXPR_CALL, *type);
    if (target == NULL)
        return NULL;
    target->u.call.name = name;
    target->u.call.arguments = left;
    left->next = right;
    return target;
} // GlslIRLowerVectorComparison

static GlslExpr *GlslIRLowerComponent(GlslLowerContext *context,
                                      const CgIRExpr *source, int component,
                                      GlslBase base)
{
    GlslExpr *target;
    GlslType type;
    int len;
    char mask[2];

    target = GlslIRLowerExpr(context, source);
    if (target == NULL)
        return NULL;
    len = 0;
    if (source->type == NULL || !IsVector(source->type, &len) || len <= 1)
        return target;
    type = GlslNumericType(base, 1);
    mask[0] = "xyzw"[component];
    mask[1] = '\0';
    return GlslNewSwizzle(context, target, &type, mask);
} // GlslIRLowerComponent

static GlslExpr *GlslIRLowerConditional(GlslLowerContext *context,
                                        const CgIRExpr *expr,
                                        const GlslType *type)
{
    GlslExpr *target;
    GlslExpr *componentExpr;
    GlslExpr *condition;
    GlslExpr *trueExpr;
    GlslExpr *falseExpr;
    GlslType componentType;
    int conditionLen;
    int i;

    conditionLen = 0;
    IsVector(expr->u.conditional.condition->type, &conditionLen);
    if (conditionLen <= 1) {
        target = GlslNewExpr(context->module, GLSL_EXPR_CONDITIONAL, *type);
        if (target == NULL)
            return NULL;
        target->u.conditional.condition = GlslIRLowerExpr(
            context, expr->u.conditional.condition);
        target->u.conditional.trueExpr = GlslIRLowerExpr(
            context, expr->u.conditional.trueExpr);
        target->u.conditional.falseExpr = GlslIRLowerExpr(
            context, expr->u.conditional.falseExpr);
        if (target->u.conditional.condition == NULL ||
            target->u.conditional.trueExpr == NULL ||
            target->u.conditional.falseExpr == NULL) return NULL;
        return target;
    }
    if (type->len < 2 || type->len > 4 || conditionLen != type->len)
        return NULL;
    target = GlslNewExpr(context->module, GLSL_EXPR_CONSTRUCT, *type);
    if (target == NULL)
        return NULL;
    componentType = GlslNumericType(type->base, 1);
    for (i = 0; i < type->len; i++) {
        condition = GlslIRLowerComponent(context,
                                         expr->u.conditional.condition, i,
                                         GLSL_BASE_BOOL);
        trueExpr = GlslIRLowerComponent(context,
                                        expr->u.conditional.trueExpr, i,
                                        type->base);
        falseExpr = GlslIRLowerComponent(context,
                                         expr->u.conditional.falseExpr, i,
                                         type->base);
        if (condition == NULL || trueExpr == NULL || falseExpr == NULL)
            return NULL;
        componentExpr = GlslNewExpr(context->module,
            GLSL_EXPR_CONDITIONAL, componentType);
        if (componentExpr == NULL)
            return NULL;
        componentExpr->u.conditional.condition = condition;
        componentExpr->u.conditional.trueExpr = trueExpr;
        componentExpr->u.conditional.falseExpr = falseExpr;
        GlslAppendExpr(&target->u.construct.arguments, componentExpr);
    }
    return target;
} // GlslIRLowerConditional

/*
 * GlslIRLowerIncrement() - Legacy expansion order: ++A became A = A + 1
 *          before tree lowering saw it; the same shape is rebuilt here.
 */

static GlslExpr *GlslIRLowerIncrement(GlslLowerContext *context,
                                      const CgIRExpr *expr,
                                      const GlslType *type)
{
    GlslExpr *target;
    GlslExpr *operand;
    GlslExpr *one;
    GlslExpr *arithmetic;
    GlslOperator op;

    op = expr->u.unary.op == CGIR_OP_PRE_INCREMENT ||
         expr->u.unary.op == CGIR_OP_POST_INCREMENT ?
         GLSL_OP_ADD : GLSL_OP_SUBTRACT;
    operand = GlslIRLowerExpr(context, expr->u.unary.operand);
    if (operand == NULL)
        return NULL;
    one = GlslNewLiteral(context, operand->type.base, 1, 0.0f);
    if (one == NULL)
        return NULL;
    arithmetic = GlslNewExpr(context->module, GLSL_EXPR_BINARY,
                             operand->type);
    if (arithmetic == NULL)
        return NULL;
    arithmetic->u.binary.op = op;
    arithmetic->u.binary.left = operand;
    arithmetic->u.binary.right = one;
    target = GlslNewExpr(context->module, GLSL_EXPR_BINARY, *type);
    if (target == NULL)
        return NULL;
    target->u.binary.op = GLSL_OP_ASSIGN;
    target->u.binary.left = GlslCloneExpr(context->module, operand);
    if (target->u.binary.left == NULL)
        return NULL;
    target->u.binary.right = arithmetic;
    return target;
} // GlslIRLowerIncrement

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

/*
 * GlslIRLowerExpr() - One Cg IR expression to the GLSL expression tree.
 *      Every decision reads the canonical type and the stable node
 *          kind; no four-bit subop is ever decoded here.
 */

GlslExpr *GlslIRLowerExpr(GlslLowerContext *context,
                                 const CgIRExpr *expr)
{
    GlslExpr *target;
    GlslExpr *operand;
    GlslExpr *inner;
    GlslDecl *decl;
    GlslType type;
    GlslOperator op;
    const CgIRExpr *sharedObject;
    int sharedCount;
    int sharedMask;
    int rows;
    int cols;

    if (expr == NULL)
        return NULL;
    if (!GlslIRType(context, expr->type, &type, &expr->loc)) {
        GlslRecordFailureKind(context, GLSL_ERROR_UNSUPPORTED_TYPE,
                              "GLSL profile expression type");
        return NULL;
    }
    if (GlslIsSamplerType(&type)) {
        GlslRecordFailureKind(context, GLSL_ERROR_SAMPLER,
                              "opaque sampler expression");
        return NULL;
    }
    switch (expr->kind) {
    case CGIR_EXPR_SYMBOL:
        target = GlslIRGeometryBuiltinArray(context, expr, &type);
        if (target != NULL)
            return target;
        decl = GlslFindDecl(context, expr->u.symbol);
        if (decl == NULL)
            return NULL;
        target = GlslNewExpr(context->module, GLSL_EXPR_SYMBOL, type);
        if (target != NULL)
            target->u.symbol = decl;
        return target;
    case CGIR_EXPR_CONSTANT:
        return GlslIRLowerConstant(context, expr);
    case CGIR_EXPR_MEMBER:
        if (expr->u.member.object != NULL &&
            expr->u.member.object->kind == CGIR_EXPR_SYMBOL &&
            expr->u.member.object->u.symbol != NULL &&
            (expr->u.member.object->u.symbol == Cg->theHAL->varyingIn ||
             expr->u.member.object->u.symbol ==
                 Cg->theHAL->varyingOut))
        {
            decl = GlslLowerInterface(context, expr->u.member.member);
            if (decl == NULL)
                return NULL;
            target = GlslNewExpr(context->module, GLSL_EXPR_SYMBOL, type);
            if (target != NULL)
                target->u.symbol = decl;
            return target;
        }
        decl = GlslFindDecl(context, expr->u.member.member);
        if (decl == NULL)
            return NULL;
        target = GlslNewExpr(context->module, GLSL_EXPR_MEMBER, type);
        if (target == NULL)
            return NULL;
        target->u.member.object = GlslIRLowerExpr(
            context, expr->u.member.object);
        if (target->u.member.object == NULL)
            return NULL;
        target->u.member.decl = decl;
        target->u.member.name = decl->name;
        return target;
    case CGIR_EXPR_INDEX:
        if (expr->u.index.object != NULL &&
            expr->u.index.object->kind == CGIR_EXPR_SYMBOL)
        {
            const CgIRDecl *param;

            param = GlslIRGeometryEntryParameter(
                context, expr->u.index.object->u.symbol);
            if (param != NULL && CgIsAttribArray(param->type) &&
                GlslGeometryInputMemberName(context->profile,
                                            param->semantic) != NULL)
            {
                GlslExpr *indexExpr;

                indexExpr = GlslIRLowerExpr(context,
                                            expr->u.index.index);
                return GlslIRGeometryBuiltinElement(context, param,
                    indexExpr, &type, &expr->loc);
            }
        }
        /* A two-level constant-index chain over a square matrix that
         * the Task 16 producer marked as a selector component (_mRC)
         * prints transposed (GLSL m[col][row]).  Genuinely explicit
         * source indexing builds the identical shape without the mark;
         * it takes the generic path below and prints as written, so
         * silent transposition of an explicit chain is impossible (the
         * marker is the single-producer invariant -- every chain the
         * `_m` lowering synthesizes carries it, nothing else may). */
        if (expr->selectorRead &&
            expr->u.index.object != NULL &&
            expr->u.index.object->kind == CGIR_EXPR_INDEX &&
            expr->u.index.object->u.index.object != NULL &&
            expr->u.index.index != NULL &&
            expr->u.index.index->kind == CGIR_EXPR_CONSTANT &&
            expr->u.index.index->u.constant.kind == CG_SCALAR_INT &&
            expr->u.index.object->u.index.index != NULL &&
            expr->u.index.object->u.index.index->kind ==
                CGIR_EXPR_CONSTANT &&
            expr->u.index.object->u.index.index->u.constant.kind ==
                CG_SCALAR_INT &&
            expr->u.index.object->u.index.object->type != NULL &&
            IsMatrix(expr->u.index.object->u.index.object->type,
                     &cols, &rows) && cols == rows)
        {
            int selRow = (int) expr->u.index.object->u.index.index->
                         u.constant.value.i;
            int selColumn = (int) expr->u.index.index->u.constant.value.i;
            GlslExpr *base;

            if (selRow < 0 || selRow > 3 || selColumn < 0 ||
                selColumn > 3 || rows < 2 || rows > 4)
                return NULL;
            base = GlslIRLowerExpr(context,
                                   expr->u.index.object->u.index.object);
            if (base == NULL)
                return NULL;
            return GlslIRMatrixElement(context, base, selRow, selColumn);
        }
        target = GlslNewExpr(context->module, GLSL_EXPR_INDEX, type);
        if (target == NULL)
            return NULL;
        target->u.index.object = GlslIRLowerExpr(context,
                                                 expr->u.index.object);
        target->u.index.index = target->u.index.object ?
            GlslIRLowerExpr(context, expr->u.index.index) : NULL;
        if (target->u.index.object == NULL ||
            target->u.index.index == NULL) return NULL;
        return target;
    case CGIR_EXPR_LENGTH:
        /* The legacy path had no array-length lowering either; the
         * generic unsupported-expression reason keeps that behavior. */
        GlslRecordFailure(context, "GLSL profile expression");
        return NULL;
    case CGIR_EXPR_SWIZZLE:
        return GlslIRLowerSwizzle(context, expr);
    case CGIR_EXPR_CONSTRUCT:
        if (type.rows != 0)
            return GlslIRLowerMatrixConstructor(context, expr, &type);
        if (GlslIRSharedSelection(expr, &sharedObject, &sharedCount,
                                  &sharedMask))
        {
            /* Task 16 selector group read: rebuild the historical form.
             * A side-effecting object evaluates once through the
             * cg_get_matN helper; a pure object prints the transposed
             * component constructor. */
            GlslMatrixSelectorHelper *helper;
            GlslType matrixType;
            GlslExpr *baseGlsl;
            int baseEffects;

            if (!GlslIRType(context, sharedObject->type, &matrixType,
                            &expr->loc))
                return NULL;
            baseEffects = GlslIRNeedsMaterialization(sharedObject);
            baseGlsl = GlslIRLowerExpr(context, sharedObject);
            if (baseGlsl == NULL)
                return NULL;
            if (sharedObject->sideEffects || baseEffects) {
                helper = GlslGetMatrixSelectorHelper(context,
                    GLSL_MATRIX_SELECTOR_GET, &matrixType, &type,
                    sharedCount, sharedMask);
                if (helper == NULL)
                    return NULL;
                target = GlslNewExpr(context->module, GLSL_EXPR_CALL,
                                     type);
                if (target == NULL)
                    return NULL;
                target->u.call.name = helper->function->name;
                target->u.call.arguments = baseGlsl;
                return target;
            }
            /* Pure group read: one lowered base, transposed selections
             * (Cg m[row][col] prints as GLSL m[col][row]), each on its
             * own clone exactly like the legacy constructor path. */
            target = GlslNewExpr(context->module, GLSL_EXPR_CONSTRUCT,
                                 type);
            if (target == NULL)
                return NULL;
            {
                int k;

                for (k = 0; k < sharedCount; k++) {
                    /* Frontend nibble packing: row<<2 | column. */
                    int column = (sharedMask >> (k * 4)) & 3;
                    int row = ((sharedMask >> (k * 4)) >> 2) & 3;
                    GlslExpr *leaf;

                    leaf = GlslCloneExpr(context->module, baseGlsl);
                    leaf = leaf ? GlslIRMatrixElement(context, leaf,
                                                       row, column)
                                : NULL;
                    if (leaf == NULL)
                        return NULL;
                    GlslAppendExpr(&target->u.construct.arguments, leaf);
                }
            }
            return target;
        }
        target = GlslNewExpr(context->module, GLSL_EXPR_CONSTRUCT, type);
        if (target == NULL)
            return NULL;
        target->u.construct.arguments = GlslIRLowerExprList(
            context, expr->u.construct.arguments);
        if (target->u.construct.arguments == NULL &&
            expr->u.construct.arguments != NULL)
            return NULL;
        return target;
    case CGIR_EXPR_CAST:
        operand = GlslIRLowerExpr(context, expr->u.cast.operand);
        if (operand == NULL)
            return NULL;
        if (GlslTypesEqual(&operand->type, &type))
            return operand;
        /* Legacy ConstantFoldNode folded casts of scalar constants into
         * the converted literal — in MAIN only (helpers kept their raw
         * casts); reproduce that scope.  Fold-coverage asymmetry: this
         * is the ONLY cast fold.  Vector/matrix constant construction
         * and constant comparisons stay unfolded on the IR path even
         * though legacy folded them ahead of printing -- no pinned
         * golden observes those forms, so they are left raw until one
         * does (see the arithmetic-fold note in CGIR_EXPR_BINARY). */
        if (context->function != NULL && context->function->isEntry &&
            expr->u.cast.operand != NULL &&
            expr->u.cast.operand->kind == CGIR_EXPR_CONSTANT)
        {
            CgNumericValue folded = expr->u.cast.operand->u.constant;
            CgScalarKind targetKind = GetScalarKind(expr->type);

            if (IsScalar(expr->type) && targetKind != CG_SCALAR_NONE &&
                CgNumericConvert(&folded, targetKind,
                                 &expr->u.cast.operand->u.constant) &&
                GlslIRScalarBase(context, targetKind, &type.base,
                                 &expr->loc))
            {
                return GlslIRConstantComponent(context, &folded,
                                               type.base);
            }
        }
        target = GlslNewExpr(context->module, GLSL_EXPR_CONSTRUCT, type);
        if (target != NULL)
            target->u.construct.arguments = operand;
        return target;
    case CGIR_EXPR_UNARY:
        switch (expr->u.unary.op) {
        case CGIR_OP_NEGATE:
        case CGIR_OP_POSITIVE:
            op = GlslIRUnaryOperator(expr->u.unary.op);
            target = GlslNewExpr(context->module, GLSL_EXPR_UNARY, type);
            if (target == NULL)
                return NULL;
            target->u.unary.op = op;
            target->u.unary.operand = GlslIRLowerExpr(
                context, expr->u.unary.operand);
            if (target->u.unary.operand == NULL)
                return NULL;
            return target;
        case CGIR_OP_LOGICAL_NOT:
            operand = GlslIRLowerExpr(context, expr->u.unary.operand);
            if (operand == NULL)
                return NULL;
            if (type.len > 1) {
                /* Vector '!' lowers to the not() builtin call exactly
                 * as the tree path emitted it. */
                target = GlslNewExpr(context->module, GLSL_EXPR_CALL,
                                     type);
                if (target != NULL) {
                    target->u.call.name = "not";
                    target->u.call.arguments = operand;
                }
                return target;
            }
            target = GlslNewExpr(context->module, GLSL_EXPR_UNARY, type);
            if (target == NULL)
                return NULL;
            target->u.unary.op = GLSL_OP_LOGICAL_NOT;
            target->u.unary.operand = operand;
            return target;
        case CGIR_OP_PRE_INCREMENT:
        case CGIR_OP_POST_INCREMENT:
        case CGIR_OP_PRE_DECREMENT:
        case CGIR_OP_POST_DECREMENT:
            return GlslIRLowerIncrement(context, expr, &type);
        default:
            GlslRecordFailure(context,
                              GlslIRUnsupportedReason(expr->u.unary.op));
            return NULL;
        }
    case CGIR_EXPR_ASSIGN:
        switch (expr->u.assign.op) {
        case CGIR_OP_ASSIGN:
            /* A vector value can never print into a scalar component
             * target ("m[0][0] = v"): well-typed user code cannot
             * build that assign, so reaching it means a producer
             * group-write run fell back elementwise -- fail loudly
             * instead of emitting invalid GLSL. */
            if (expr->u.assign.value != NULL &&
                expr->u.assign.value->type != NULL &&
                IsScalar(expr->type) &&
                IsVector(expr->u.assign.value->type, NULL))
            {
                GlslRecordFailure(context, "GLSL profile expression");
                return NULL;
            }
            target = GlslNewExpr(context->module, GLSL_EXPR_BINARY, type);
            if (target == NULL)
                return NULL;
            target->u.binary.op = GLSL_OP_ASSIGN;
            target->u.binary.left = GlslIRLowerExpr(
                context, expr->u.assign.target);
            target->u.binary.right = target->u.binary.left ?
                GlslIRLowerExpr(context, expr->u.assign.value) : NULL;
            if (target->u.binary.left == NULL ||
                target->u.binary.right == NULL) return NULL;
            return target;
        case CGIR_OP_ADD_ASSIGN:
        case CGIR_OP_SUBTRACT_ASSIGN:
        case CGIR_OP_MULTIPLY_ASSIGN:
        case CGIR_OP_DIVIDE_ASSIGN:
            /* Legacy order: A op= B expanded to A = A op B before the
             * tree path ever saw it. */
            op = GlslIRCompoundOperator(expr->u.assign.op);
            operand = GlslIRLowerExpr(context, expr->u.assign.target);
            inner = NULL;
            target = NULL;
            if (operand != NULL) {
                inner = GlslNewExpr(context->module, GLSL_EXPR_BINARY,
                                    operand->type);
                if (inner != NULL) {
                    inner->u.binary.op = op;
                    inner->u.binary.left = GlslCloneExpr(context->module,
                                                         operand);
                    inner->u.binary.right = inner->u.binary.left ?
                        GlslIRLowerExpr(context,
                                        expr->u.assign.value) : NULL;
                }
            }
            if (inner != NULL && inner->u.binary.left != NULL &&
                inner->u.binary.right != NULL)
            {
                target = GlslNewExpr(context->module, GLSL_EXPR_BINARY,
                                     type);
                if (target != NULL) {
                    target->u.binary.op = GLSL_OP_ASSIGN;
                    target->u.binary.left = operand;
                    target->u.binary.right = inner;
                }
            }
            return target;
        default:
            GlslRecordFailure(context,
                              GlslIRUnsupportedReason(expr->u.assign.op));
            return NULL;
        }
    case CGIR_EXPR_BINARY:
        if (GlslIRUnsupportedReason(expr->u.binary.op) != NULL) {
            GlslRecordFailure(context,
                              GlslIRUnsupportedReason(expr->u.binary.op));
            return NULL;
        }
        op = GlslIRBinaryOperator(expr->u.binary.op);
        if (op == GLSL_OP_NONE) {
            GlslRecordFailure(context, "GLSL profile expression");
            return NULL;
        }
        if ((op == GLSL_OP_LESS || op == GLSL_OP_GREATER ||
             op == GLSL_OP_LESS_EQUAL || op == GLSL_OP_GREATER_EQUAL ||
             op == GLSL_OP_EQUAL || op == GLSL_OP_NOT_EQUAL) &&
            type.len > 1)
        {
            return GlslIRLowerVectorComparison(context, expr, &type,
                GlslIRComparisonName(expr->u.binary.op));
        }
        if ((op == GLSL_OP_ADD || op == GLSL_OP_SUBTRACT ||
             op == GLSL_OP_MULTIPLY || op == GLSL_OP_DIVIDE) &&
            context->function != NULL && context->function->isEntry &&
            expr->u.binary.left != NULL &&
            expr->u.binary.left->kind == CGIR_EXPR_CONSTANT &&
            expr->u.binary.right != NULL &&
            expr->u.binary.right->kind == CGIR_EXPR_CONSTANT &&
            IsScalar(expr->type))
        {
            /* Fold-coverage asymmetry vs legacy ConstantFoldNode (it
             * ran on the frontend tree ahead of printing): this is the
             * ONLY arithmetic fold reproduced.  What folds: scalar
             * (+,-,*,/) over two constants, entry function only
             * (division by zero yields the infinity that non-finite
             * rejection pins).  What intentionally stays UNFOLDED:
             * vector/matrix constant arithmetic (emitted as literal
             * operator expressions) and every constant comparison --
             * scalar or the lessThan family above -- because no pinned
             * golden output observes those forms; folding them is left
             * until a golden demands it.  Helpers never fold, matching
             * legacy's MAIN-only scope. */
            CgNumericValue folded;
            CgNumericOp numop = op == GLSL_OP_ADD ? CG_NUMERIC_ADD :
                                op == GLSL_OP_SUBTRACT ? CG_NUMERIC_SUB :
                                op == GLSL_OP_MULTIPLY ?
                                                       CG_NUMERIC_MUL :
                                                       CG_NUMERIC_DIV;

            if (CgNumericBinary(&folded, numop,
                                &expr->u.binary.left->u.constant,
                                &expr->u.binary.right->u.constant))
            {
                return GlslIRConstantComponent(context, &folded,
                                               type.base);
            }
        }
        target = GlslNewExpr(context->module, GLSL_EXPR_BINARY, type);
        if (target == NULL)
            return NULL;
        target->u.binary.op = op;
        target->u.binary.left = GlslIRLowerExpr(context,
                                                expr->u.binary.left);
        target->u.binary.right = target->u.binary.left ?
            GlslIRLowerExpr(context, expr->u.binary.right) : NULL;
        if (target->u.binary.left == NULL ||
            target->u.binary.right == NULL) return NULL;
        return target;
    case CGIR_EXPR_CONDITIONAL:
        return GlslIRLowerConditional(context, expr, &type);
    case CGIR_EXPR_CALL:
        return GlslIRLowerCallCore(context, expr->u.call.callee, NULL,
                                   expr->u.call.arguments, &type);
    case CGIR_EXPR_INTRINSIC:
        return GlslIRLowerCallCore(context, NULL,
                                   expr->u.intrinsicCall.signature,
                                   expr->u.intrinsicCall.arguments, &type);
    case CGIR_EXPR_INTERFACE_CALL:
        /* Interface dispatch has no focused GLSL profile meaning; profile
         * validation rejects it with the historical reason. */
        GlslRecordFailure(context, "interface dispatch");
        return NULL;
    default:
        GlslRecordFailure(context, "GLSL profile expression");
        return NULL;
    }
} // GlslIRLowerExpr


///////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////// Cg IR statement lowering (Cg 2.0) //////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////

const char *GlslIRDeclNameText(const CgIRDecl *decl)
{
    return GetAtomString(atable, decl->name);
} // GlslIRDeclNameText

/*
 * GlslIRMatrixStoreShape() - Match one scalar matrix-element store:
 *          ASSIGN(INDEX(INDEX(base,row),column), value) with constant
 *          element coordinates.
 */

static int GlslIRMatrixStoreShape(const CgIRExpr *expr,
                                  const CgIRExpr **base,
                                  const CgIRExpr **value,
                                  int *row, int *column)
{
    const CgIRExpr *rowSel;
    const CgIRExpr *rowIdx;
    const CgIRExpr *colIdx;

    if (expr == NULL || expr->kind != CGIR_EXPR_ASSIGN ||
        expr->u.assign.op != CGIR_OP_ASSIGN)
        return 0;
    rowSel = expr->u.assign.target;
    if (rowSel == NULL || rowSel->kind != CGIR_EXPR_INDEX)
        return 0;
    colIdx = rowSel->u.index.index;
    if (colIdx == NULL || colIdx->kind != CGIR_EXPR_CONSTANT ||
        colIdx->u.constant.kind != CG_SCALAR_INT)
        return 0;
    rowIdx = rowSel->u.index.object;
    if (rowIdx == NULL || rowIdx->kind != CGIR_EXPR_INDEX)
        return 0;
    {
        const CgIRExpr *rowConst = rowIdx->u.index.index;

        if (rowConst == NULL || rowConst->kind != CGIR_EXPR_CONSTANT ||
            rowConst->u.constant.kind != CG_SCALAR_INT)
            return 0;
        *row = (int) rowConst->u.constant.value.i;
    }
    *column = (int) colIdx->u.constant.value.i;
    if (*row < 0 || *row > 3 || *column < 0 || *column > 3)
        return 0;
    *base = rowIdx->u.index.object;
    *value = expr->u.assign.value;
    return 1;
} // GlslIRMatrixStoreShape

/*
 * GlslIRStoreTargetMarked() - True when the store's element chain is a
 *          Task 16 producer selector target (selectorRead): every
 *          store the `_m` group-write lowering synthesizes carries
 *          the mark and nothing else may set it.
 */

static int GlslIRStoreTargetMarked(const CgIRExpr *expr)
{
    return expr != NULL && expr->kind == CGIR_EXPR_ASSIGN &&
           expr->u.assign.target != NULL &&
           expr->u.assign.target->selectorRead;
} // GlslIRStoreTargetMarked

/*
 * GlslIRIsTempMove() - A synthesized temporary assignment: temp = expr.
 */

static int GlslIRIsTempMove(const CgIRExpr *expr, const void *identity,
                            const CgIRExpr **value)
{
    if (expr == NULL || expr->kind != CGIR_EXPR_ASSIGN ||
        expr->u.assign.op != CGIR_OP_ASSIGN)
        return 0;
    if (expr->u.assign.target == NULL ||
        expr->u.assign.target->kind != CGIR_EXPR_SYMBOL ||
        expr->u.assign.target->u.symbol != identity)
        return 0;
    *value = expr->u.assign.value;
    return 1;
} // GlslIRIsTempMove

GlslExpr *GlslIRMatrixElement(GlslLowerContext *context,
                                     GlslExpr *matrix, int row,
                                     int column)
{
    GlslExpr *columnExpr;
    GlslType columnType;
    GlslType scalarType;

    if (matrix == NULL || matrix->type.rows < 2 ||
        matrix->type.rows != matrix->type.cols || row < 0 || column < 0 ||
        row >= matrix->type.rows || column >= matrix->type.cols)
    {
        return NULL;
    }
    columnType = GlslNumericType(GLSL_BASE_FLOAT, matrix->type.rows);
    scalarType = GlslNumericType(GLSL_BASE_FLOAT, 1);
    columnExpr = GlslIRAppendIndexStep(context, matrix, column,
                                       columnType, &context->statementLoc);
    if (columnExpr == NULL)
        return NULL;
    return GlslIRAppendIndexStep(context, columnExpr, row, scalarType,
                                 &context->statementLoc);
} // GlslIRMatrixElement

/*
 * GlslIRTryGroupWrite() - Rebuild the historical output form of a Task
 *          16 matrix group write: either the cg_set_matN helper call
 *          (with any synthesized object/value temporaries folded back
 *          into their single-evaluation arguments) or the plain
 *          per-component fan-out between two simple variables.
 *          Returns 1 with *nextOut past the consumed run, 0 when the
 *          head is not a group write (including plain user store runs
 *          that only resemble one -- over-long fills or mixed
 *          coordinates -- which lower as independent statements), and
 *          -1 on lowering failure.
 */

int GlslIRTryGroupWrite(GlslLowerContext *context,
                               const CgIRStmt *head,
                               const CgIRStmt **nextOut, GlslStmt **list)
{
    const CgIRStmt *cursor;
    const void *objectTemp = NULL;
    const void *valueTemp = NULL;
    const CgIRExpr *objectSource = NULL;
    const CgIRExpr *valueSource = NULL;
    const CgIRStmt *storeLoop;
    const CgIRExpr *storeBase = NULL;
    const CgIRExpr *sharedValue = NULL;
    const CgIRExpr *rightBase = NULL;
    int storeCount = 0;
    int mask = 0;
    int rightMask = 0;
    int rows[4];
    int columns[4];
    int scalarFanout = 0;

    if (head == NULL)
        return 0;
    /* Optional leading temporaries: the object, then the value, each
     * declared and moved exactly once ahead of the stores. */
    cursor = head;
    if (head->kind == CGIR_STMT_DECL) {
        const char *declName = GlslIRDeclNameText(head->u.decl);

        if (declName == NULL || declName[0] != '$')
            return 0;
        if (head->next != NULL && head->next->kind == CGIR_STMT_EXPR &&
            GlslIRIsTempMove(head->next->u.expression,
                             head->u.decl->symbol, &objectSource))
        {
            const CgIRStmt *afterObject = head->next->next;

            objectTemp = head->u.decl->symbol;
            cursor = afterObject;
            if (afterObject != NULL &&
                afterObject->kind == CGIR_STMT_DECL &&
                afterObject->next != NULL &&
                afterObject->next->kind == CGIR_STMT_EXPR)
            {
                const char *secondName = GlslIRDeclNameText(
                    afterObject->u.decl);

                if (secondName != NULL && secondName[0] == '$' &&
                    GlslIRIsTempMove(afterObject->next->u.expression,
                                     afterObject->u.decl->symbol,
                                     &valueSource))
                {
                    valueTemp = afterObject->u.decl->symbol;
                    cursor = afterObject->next->next;
                }
            }
        }
    } else if (head->kind != CGIR_STMT_EXPR ||
               !GlslIRMatrixStoreShape(head->u.expression, &storeBase,
                                        &sharedValue, &rows[0],
                                        &columns[0]))
    {
        return 0;
    }
    /* Count consecutive constant-coordinate stores against one base. */
    storeLoop = cursor;
    for (; storeLoop != NULL; storeLoop = storeLoop->next) {
        const CgIRExpr *base;
        const CgIRExpr *value;
        int row;
        int column;

        if (storeLoop->kind != CGIR_STMT_EXPR ||
            !GlslIRMatrixStoreShape(storeLoop->u.expression, &base,
                                     &value, &row, &column))
            break;
        if (storeCount >= 4) {
            /* Frontend `_m` selector groups pack at most four
             * components, so a fifth consecutive store cannot extend a
             * producer run.  A producer run marks its store targets
             * (selectorRead): re-lowering it elementwise would print
             * each whole-vector value into one scalar component
             * target, so any marked run stays a loud failure.  Only
             * an over-long run of unmarked plain user stores is an
             * elementwise fill that falls back to independent
             * statements for the whole head; runs behind consumed "$"
             * temporaries remain genuine producer violations and stay
             * loud as well. */
            if ((cursor->kind == CGIR_STMT_EXPR &&
                 GlslIRStoreTargetMarked(cursor->u.expression)) ||
                objectTemp != NULL || valueTemp != NULL)
            {
                return -1;
            }
            return 0;
        }
        if (storeCount == 0)
            storeBase = base;
        else if (base != storeBase)
            break;
        if (objectTemp != NULL &&
            (base->kind != CGIR_EXPR_SYMBOL ||
             base->u.symbol != objectTemp))
            return -1;
        rows[storeCount] = row;
        columns[storeCount] = column;
        mask |= ((row << 2) | column) << (storeCount * 4);
        storeCount++;
    }
    if (storeCount < 2)
        return 0;
    if (objectTemp == NULL && storeBase->sideEffects)
        return -1;
    /* Classify the store values: one shared node, the value temporary,
     * or distinct selections over one right variable (scalar fan-out). */
    {
        const CgIRStmt *walk = cursor;
        const CgIRExpr *candidate = NULL;
        int allSame = 1;

        for (; walk != storeLoop; walk = walk->next) {
            const CgIRExpr *base;
            const CgIRExpr *value;
            int row;
            int column;

            (void) GlslIRMatrixStoreShape(walk->u.expression, &base,
                                           &value, &row, &column);
            if (candidate == NULL)
                candidate = value;
            else if (value != candidate)
                allSame = 0;
        }
        sharedValue = allSame ? candidate : NULL;
        if (sharedValue != NULL && objectTemp == NULL && valueTemp == NULL)
        {
            /* A shared group-read over a plain right variable paired
             * with a plain left variable is the legacy componentwise
             * fan-out, not the set-helper call. */
            const CgIRExpr *rbase = NULL;
            int rcount = 0;
            int rmask = 0;

            if (GlslIRSharedSelection(sharedValue, &rbase, &rcount,
                                      &rmask) &&
                rcount == storeCount &&
                rbase->kind == CGIR_EXPR_SYMBOL &&
                storeBase->kind == CGIR_EXPR_SYMBOL &&
                storeBase->u.symbol != rbase->u.symbol)
            {
                scalarFanout = 1;
                rightBase = rbase;
                rightMask = rmask;
            }
        }
        if (sharedValue == NULL && valueTemp != NULL) {
            /* Values must be the value temporary itself. */
            walk = cursor;
            for (; walk != storeLoop; walk = walk->next) {
                const CgIRExpr *base;
                const CgIRExpr *value;
                int row;
                int column;

                (void) GlslIRMatrixStoreShape(walk->u.expression, &base,
                                               &value, &row, &column);
                if (value->kind != CGIR_EXPR_SYMBOL ||
                    value->u.symbol != valueTemp)
                    return -1;
            }
        } else if (sharedValue == NULL && objectTemp == NULL &&
                   valueTemp == NULL)
        {
            /* Distinct per-store values: either the legacy scalar
             * fan-out between two plain variables, or two unrelated
             * scalar element stores.  Validate the fan-out shape;
             * anything else falls back to independent statements. */
            const CgIRStmt *fanWalk = cursor;

            scalarFanout = 1;
            for (; fanWalk != storeLoop; fanWalk = fanWalk->next) {
                const CgIRExpr *base;
                const CgIRExpr *value;
                const CgIRExpr *rightRowSel;
                int row;
                int column;

                if (!GlslIRMatrixStoreShape(fanWalk->u.expression, &base,
                                             &value, &row, &column))
                    return -1;
                if (value->kind != CGIR_EXPR_INDEX ||
                    value->u.index.index == NULL ||
                    value->u.index.index->kind != CGIR_EXPR_CONSTANT ||
                    value->u.index.object == NULL ||
                    value->u.index.object->kind != CGIR_EXPR_INDEX ||
                    value->u.index.object->u.index.index == NULL ||
                    value->u.index.object->u.index.index->kind !=
                        CGIR_EXPR_CONSTANT ||
                    value->u.index.object->u.index.object == NULL ||
                    value->u.index.object->u.index.object->kind !=
                        CGIR_EXPR_SYMBOL)
                {
                    return 0;
                }
                rightRowSel = value->u.index.object;
                /* The value's own coordinates must match the store's:
                 * emission would otherwise mirror the left coordinates
                 * onto the right side and silently move the wrong
                 * element.  Mixed-coordinate runs are plain user
                 * stores, not a producer shape -- fall back so every
                 * statement lowers independently. */
                if ((int) rightRowSel->u.index.index->u.constant.value.i !=
                        row ||
                    (int) value->u.index.index->u.constant.value.i !=
                        column)
                {
                    return 0;
                }
                if (rightBase == NULL)
                    rightBase = rightRowSel->u.index.object;
                else if (rightBase != rightRowSel->u.index.object)
                    return 0;
                if (storeBase->kind != CGIR_EXPR_SYMBOL ||
                    storeBase->u.symbol ==
                        rightRowSel->u.index.object->u.symbol)
                {
                    return 0;
                }
            }
            if (rightBase == NULL)
                return 0;
        } else if (sharedValue == NULL) {
            return -1;
        }
    }
    {
        SourceLoc emitLoc = cursor->loc;
        GlslExpr *leftGlsl;
        GlslExpr *valueGlsl;

        if (scalarFanout) {
            GlslExpr *rightGlsl;
            GlslType scalarType;
            int i;

            leftGlsl = GlslIRLowerExpr(context, storeBase);
            rightGlsl = leftGlsl ?
                        GlslIRLowerExpr(context, rightBase) : NULL;
            if (leftGlsl == NULL || rightGlsl == NULL)
                return -1;
            scalarType = GlslNumericType(GLSL_BASE_FLOAT, 1);
            for (i = 0; i < storeCount; i++) {
                GlslExpr *leftLeaf;
                GlslExpr *rightLeaf;
                GlslExpr *assignment;
                GlslStmt *statement;
                int rightRow;
                int rightColumn;

                leftLeaf = GlslCloneExpr(context->module, leftGlsl);
                leftLeaf = leftLeaf ?
                    GlslIRMatrixElement(context, leftLeaf, rows[i],
                                         columns[i]) : NULL;
                if (rightMask != 0) {
                    /* Right selections come from the shared group-read
                     * mask (same row<<2|column nibble encoding). */
                    rightRow = ((rightMask >> (i * 4)) >> 2) & 3;
                    rightColumn = (rightMask >> (i * 4)) & 3;
                } else {
                    /* Distinct-value runs validated every value's own
                     * coordinates against the store's above, so these
                     * are the value's real coordinates, not a mirror. */
                    rightRow = rows[i];
                    rightColumn = columns[i];
                }
                rightLeaf = GlslCloneExpr(context->module, rightGlsl);
                rightLeaf = rightLeaf ?
                    GlslIRMatrixElement(context, rightLeaf, rightRow,
                                         rightColumn) : NULL;
                assignment = leftLeaf != NULL && rightLeaf != NULL ?
                    GlslNewExpr(context->module, GLSL_EXPR_BINARY,
                                scalarType) : NULL;
                statement = assignment != NULL ?
                    GlslNewStmt(context->module,
                                GLSL_STMT_EXPRESSION) : NULL;
                if (statement == NULL)
                    return -1;
                assignment->u.binary.op = GLSL_OP_ASSIGN;
                assignment->u.binary.left = leftLeaf;
                assignment->u.binary.right = rightLeaf;
                GlslSetLoc(&statement->loc, &emitLoc);
                statement->u.expression = assignment;
                GlslAppendStmt(list, statement);
            }
        } else {
            GlslMatrixSelectorHelper *helper;
            GlslExpr *call;
            GlslStmt *statement;

            leftGlsl = GlslIRLowerExpr(context,
                objectTemp != NULL ? objectSource : storeBase);
            if (leftGlsl == NULL)
                return -1;
            valueGlsl = GlslIRLowerExpr(context,
                valueTemp != NULL ? valueSource :
                                    (sharedValue != NULL ? sharedValue :
                                                           valueSource));
            if (valueGlsl == NULL)
                return -1;
            helper = GlslGetMatrixSelectorHelper(context,
                GLSL_MATRIX_SELECTOR_SET, &leftGlsl->type,
                &valueGlsl->type, storeCount, mask);
            if (helper == NULL)
                return -1;
            call = GlslNewExpr(context->module, GLSL_EXPR_CALL,
                               helper->function->result);
            statement = call != NULL ?
                        GlslNewStmt(context->module,
                                    GLSL_STMT_EXPRESSION) : NULL;
            if (statement == NULL)
                return -1;
            call->u.call.name = helper->function->name;
            call->u.call.arguments = leftGlsl;
            leftGlsl->next = valueGlsl;
            GlslSetLoc(&statement->loc, &emitLoc);
            statement->u.expression = call;
            GlslAppendStmt(list, statement);
        }
    }
    *nextOut = storeLoop;
    return 1;
} // GlslIRTryGroupWrite

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


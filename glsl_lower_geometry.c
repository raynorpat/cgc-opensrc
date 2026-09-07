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
// glsl_lower_geometry.c
//

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <float.h>
#include <limits.h>

#include "glsl_lower_internal.h"




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

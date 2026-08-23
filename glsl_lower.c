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
#include <float.h>
#include <limits.h>

#include "slglobals.h"
#include "glsl_hal.h"

#define GLSL_MATRIX_MAX_ARGUMENTS 16

typedef struct GlslMatrixHelper_Rec {
    struct GlslMatrixHelper_Rec *next;
    GlslFunction *function;
    GlslType result;
    GlslType parameters[GLSL_MATRIX_MAX_ARGUMENTS];
    int parameterCount;
} GlslMatrixHelper;

typedef enum GlslMatrixSelectorHelperKind_Enum {
    GLSL_MATRIX_SELECTOR_GET,
    GLSL_MATRIX_SELECTOR_SET
} GlslMatrixSelectorHelperKind;

typedef struct GlslMatrixSelectorHelper_Rec {
    struct GlslMatrixSelectorHelper_Rec *next;
    GlslFunction *function;
    GlslMatrixSelectorHelperKind kind;
    GlslType matrixType;
    GlslType valueType;
    int count;
    int mask;
} GlslMatrixSelectorHelper;

typedef struct GlslInterfaceSource_Rec {
    struct GlslInterfaceSource_Rec *next;
    const Symbol *source;
    const char *interfaceKey;
    const char *reservedName;
    int isOutput;
} GlslInterfaceSource;

typedef struct GlslLowerContext_Rec {
    GlslModule *module;
    const GlslProfileDesc *profile;
    Scope *scope;
    GlslFunction *function;
    GlslMatrixHelper *matrixHelpers;
    GlslMatrixHelper *lastMatrixHelper;
    GlslMatrixSelectorHelper *selectorHelpers;
    GlslMatrixSelectorHelper *lastSelectorHelper;
    GlslInterfaceSource *interfaceSources;
    SourceLoc statementLoc;
    int loopDepth;
} GlslLowerContext;

static void GlslSetLoc(GlslLoc *target, const SourceLoc *source)
{
    if (source != NULL) {
        target->file = source->file;
        target->line = source->line;
    }
}

static int GlslLowerError(GlslLowerContext *context)
{
    context->module->errors++;
    return 0;
}

static void GlslRecordFailure(GlslLowerContext *context, const char *reason)
{
    if (context->module->errorReason != NULL)
        return;
    context->module->errorLoc.file = context->statementLoc.file;
    context->module->errorLoc.line = context->statementLoc.line;
    if (context->module->errorKind == GLSL_ERROR_NONE)
        context->module->errorKind = GLSL_ERROR_UNSUPPORTED_OPERATION;
    context->module->errorReason = reason;
}

static void GlslRecordFailureKind(GlslLowerContext *context,
                                  GlslErrorKind kind,
                                  const char *reason)
{
    if (context->module->errorReason != NULL)
        return;
    context->module->errorKind = kind;
    GlslRecordFailure(context, reason);
}

static void GlslRecordFailureKindAt(GlslLowerContext *context,
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

static const char *GlslUnsupportedExprReason(const expr *source)
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
        default:
            break;
        }
    }
    return "GLSL 1.10 expression";
}

static char *GlslCopyText(GlslModule *module, const char *text)
{
    char *copy;
    size_t size;

    size = strlen(text) + 1;
    copy = (char *) module->alloc(module->allocArg, size);
    if (copy != NULL)
        memcpy(copy, text, size);
    return copy;
}

static int GlslTypesEqual(const GlslType *left, const GlslType *right)
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

static int GlslIsSamplerType(const GlslType *type)
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

static GlslDecl *GlslFindDecl(GlslLowerContext *context,
                              const void *identity)
{
    GlslDecl *decl;
    GlslBinding *binding;

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

static GlslFunction *GlslFindFunction(GlslModule *module,
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

static Type *GlslCanonicalStructType(Type *type)
{
    Type *canonical;

    if (type == NULL || GetCategory(type) != TYPE_CATEGORY_STRUCT)
        return NULL;
    canonical = type->str.unqualifiedtype;
    if (canonical == NULL)
        canonical = type;
    if (GetCategory(canonical) != TYPE_CATEGORY_STRUCT ||
        (canonical->str.unqualifiedtype != NULL &&
         canonical->str.unqualifiedtype != canonical))
    {
        return NULL;
    }
    return canonical;
}

static Symbol *GlslFindTag(Scope *scope, Type *type)
{
    Type *canonical;
    Symbol *tag;

    canonical = GlslCanonicalStructType(type);
    if (canonical == NULL)
        return NULL;
    if (scope == NULL)
        scope = CurrentScope;
    for (; scope != NULL; scope = scope->parent) {
        tag = LookUpLocalTag(scope, canonical->str.tag);
        if (tag != NULL &&
            GlslCanonicalStructType(tag->type) == canonical) return tag;
    }
    return NULL;
}

static GlslDecl *GlslFindStruct(GlslLowerContext *context, Type *type)
{
    GlslDecl *decl;
    Symbol *tag;

    tag = GlslFindTag(context->scope, type);
    if (tag == NULL)
        return NULL;
    for (decl = context->module->structs; decl != NULL; decl = decl->next) {
        if (decl->identity == tag)
            return decl;
    }
    return NULL;
}

static int GlslLowerType(GlslLowerContext *context, Type *source,
                         GlslType *target, const SourceLoc *loc)
{
    GlslDecl *structDecl;
    GlslBase glslBase;
    GlslType elementType;
    GlslType *element;
    int base;
    int category;
    int len;
    int rows;
    int cols;
    char matrixName[32];

    if (source == NULL || target == NULL)
        return 0;
    base = GetBase(source);
    if (IsVoid(source)) {
        *target = GlslNumericType(GLSL_BASE_VOID, 0);
        return 1;
    }
    if (IsMatrix(source, &cols, &rows)) {
        if (base != TYPE_BASE_FLOAT && base != TYPE_BASE_CFLOAT) {
            GlslRecordFailureKindAt(context, GLSL_ERROR_UNSUPPORTED_TYPE,
                                    "matrix", loc);
            return 0;
        }
        if (rows != cols || rows < 2 || rows > 4) {
            sprintf(matrixName, "%s%dx%d", GetBaseTypeNameString(base),
                    rows, cols);
            GlslRecordFailureKindAt(context,
                                    GLSL_ERROR_NON_SQUARE_MATRIX,
                                    GlslCopyText(context->module,
                                                 matrixName), loc);
            return 0;
        }
        *target = GlslMatrixType(rows);
        return 1;
    }
    switch (base) {
    case TYPE_BASE_FLOAT:
    case TYPE_BASE_CFLOAT:
        glslBase = GLSL_BASE_FLOAT;
        break;
    case TYPE_BASE_INT:
    case TYPE_BASE_CINT:
        glslBase = GLSL_BASE_INT;
        break;
    case TYPE_BASE_BOOLEAN:
        glslBase = GLSL_BASE_BOOL;
        break;
    case TYPE_BASE_GLSL_SAMPLER1D:
        glslBase = GLSL_BASE_SAMPLER1D;
        break;
    case TYPE_BASE_GLSL_SAMPLER2D:
        glslBase = GLSL_BASE_SAMPLER2D;
        break;
    case TYPE_BASE_GLSL_SAMPLER3D:
        glslBase = GLSL_BASE_SAMPLER3D;
        break;
    case TYPE_BASE_GLSL_SAMPLERCUBE:
        glslBase = GLSL_BASE_SAMPLERCUBE;
        break;
    default:
        glslBase = GLSL_BASE_VOID;
        break;
    }
    category = GetCategory(source);
    if (category == TYPE_CATEGORY_SCALAR && glslBase != GLSL_BASE_VOID) {
        *target = GlslNumericType(glslBase, 1);
        return 1;
    }
    if (category == TYPE_CATEGORY_ARRAY && IsVector(source, &len) &&
        glslBase != GLSL_BASE_VOID && len >= 1 && len <= 4)
    {
        *target = GlslNumericType(glslBase, len);
        return 1;
    }
    if (category == TYPE_CATEGORY_ARRAY && source->arr.numels > 0 &&
        GlslLowerType(context, source->arr.eltype, &elementType, loc))
    {
        element = (GlslType *) context->module->alloc(
            context->module->allocArg, sizeof(GlslType));
        if (element == NULL)
            return 0;
        *element = elementType;
        *target = GlslNumericType(GLSL_BASE_VOID, 0);
        target->arraySize = source->arr.numels;
        target->elementType = element;
        return 1;
    }
    if (category == TYPE_CATEGORY_STRUCT) {
        structDecl = GlslFindStruct(context, source);
        if (structDecl == NULL)
            return 0;
        *target = GlslNumericType(GLSL_BASE_STRUCT, 0);
        target->structName = structDecl->name;
        target->members = structDecl->members;
        return 1;
    }
    return 0;
}

static int GlslDeclComesBefore(const GlslDecl *left, const GlslDecl *right)
{
    if (left->loc.file != right->loc.file)
        return left->loc.file < right->loc.file;
    if (left->loc.line != right->loc.line)
        return left->loc.line < right->loc.line;
    return strcmp(left->name, right->name) < 0;
}

static void GlslInsertDecl(GlslDecl **list, GlslDecl *decl)
{
    GlslDecl **place;

    place = list;
    while (*place != NULL && !GlslDeclComesBefore(decl, *place))
        place = &(*place)->next;
    decl->next = *place;
    *place = decl;
}

static GlslDecl *GlslNewSourceDecl(GlslLowerContext *context,
    Symbol *symbol, const void *nameSpace)
{
    GlslDecl *decl;
    GlslType type;
    const char *sourceName;
    const char *name;

    if (!GlslLowerType(context, symbol->type, &type, &symbol->loc))
        return NULL;
    sourceName = GetAtomString(atable, symbol->name);
    if (nameSpace != NULL) {
        name = GlslAllocateScopedSymbolName(context->module, nameSpace,
                                            symbol, sourceName);
    } else {
        name = GlslAllocateSymbolName(context->module, symbol, sourceName);
    }
    if (name == NULL)
        return NULL;
    decl = GlslNewDecl(context->module, GLSL_STORAGE_NONE, type, name);
    if (decl != NULL) {
        decl->identity = symbol;
        GlslSetLoc(&decl->loc, &symbol->loc);
        decl->sourceOrdinal = symbol->sourceOrdinal;
    }
    return decl;
}

static int GlslCollectMembers(GlslLowerContext *context, Scope *memberScope,
    Symbol *symbol, GlslDecl **members)
{
    GlslDecl *decl;

    if (symbol == NULL)
        return 1;
    if (!GlslCollectMembers(context, memberScope, symbol->left, members))
        return 0;
    if (symbol->kind == VARIABLE_S) {
        decl = GlslNewSourceDecl(context, symbol, memberScope);
        if (decl == NULL)
            return 0;
        GlslInsertDecl(members, decl);
    }
    return GlslCollectMembers(context, memberScope, symbol->right, members);
}

static int GlslEnsureType(GlslLowerContext *context, Type *type);

static int GlslEnsureTypeAt(GlslLowerContext *context, Type *type,
                            const SourceLoc *loc)
{
    SourceLoc savedLoc;
    int result;

    if (loc == NULL)
        return GlslEnsureType(context, type);
    savedLoc = context->statementLoc;
    context->statementLoc = *loc;
    result = GlslEnsureType(context, type);
    context->statementLoc = savedLoc;
    return result;
}

static int GlslEnsureSymbolTypes(GlslLowerContext *context, Symbol *symbol)
{
    const char *name;

    if (symbol == NULL)
        return 1;
    if (!GlslEnsureSymbolTypes(context, symbol->left))
        return 0;
    if (symbol->kind == VARIABLE_S) {
        if (Cg->theHAL->IsTexobjBase(GetBase(symbol->type)) &&
            GetDomain(symbol->type) != TYPE_DOMAIN_UNIFORM)
        {
            GlslRecordFailureKindAt(context, GLSL_ERROR_SAMPLER,
                                    "samplers must be uniforms",
                                    &symbol->loc);
            return 0;
        }
        name = GetAtomString(atable, symbol->name);
        if (name != NULL && name[0] != '$' &&
            !GlslEnsureTypeAt(context, symbol->type,
                              &symbol->loc)) return 0;
    }
    return GlslEnsureSymbolTypes(context, symbol->right);
}

static int GlslEnsureParameterTypes(GlslLowerContext *context,
                                    Symbol *formal)
{
    for (; formal != NULL; formal = formal->next) {
        if (Cg->theHAL->IsTexobjBase(GetBase(formal->type)) &&
            GetDomain(formal->type) != TYPE_DOMAIN_UNIFORM)
        {
            GlslRecordFailureKindAt(context, GLSL_ERROR_SAMPLER,
                                    "samplers must be uniforms",
                                    &formal->loc);
            return 0;
        }
        if (!GlslEnsureTypeAt(context, formal->type, &formal->loc))
            return 0;
    }
    return 1;
}

static int GlslEnsureType(GlslLowerContext *context, Type *type)
{
    Type *canonical;
    GlslDecl *decl;
    GlslType structType;
    Symbol *tag;
    const char *sourceName;
    const char *name;

    if (type == NULL)
        return 0;
    if (IsMatrix(type, NULL, NULL) || IsVector(type, NULL) ||
        GetCategory(type) == TYPE_CATEGORY_SCALAR)
        return 1;
    if (GetCategory(type) == TYPE_CATEGORY_ARRAY)
        return type->arr.numels > 0 &&
               GlslEnsureType(context, type->arr.eltype);
    if (GetCategory(type) != TYPE_CATEGORY_STRUCT)
        return 0;
    canonical = GlslCanonicalStructType(type);
    if (canonical == NULL)
        return 0;
    if (GlslFindStruct(context, canonical) != NULL)
        return 1;
    tag = GlslFindTag(context->scope, canonical);
    if (tag == NULL)
        return 0;
    sourceName = GetAtomString(atable, canonical->str.tag);
    name = GlslAllocateSymbolName(context->module, tag, sourceName);
    if (name == NULL)
        return 0;
    structType = GlslNumericType(GLSL_BASE_STRUCT, 0);
    structType.structName = name;
    decl = GlslNewDecl(context->module, GLSL_STORAGE_NONE,
                       structType, name);
    if (decl == NULL)
        return 0;
    decl->identity = tag;
    GlslSetLoc(&decl->loc, &canonical->str.loc);
    GlslAppendDecl(&context->module->structs, decl);
    if (canonical->str.members == NULL ||
        !GlslEnsureSymbolTypes(context, canonical->str.members->symbols))
    {
        return 0;
    }
    return GlslCollectMembers(context, canonical->str.members,
        canonical->str.members->symbols, &decl->members);
}

static int GlslTypeUsesStruct(const GlslType *type, const char *name)
{
    if (type->elementType != NULL)
        return GlslTypeUsesStruct(type->elementType, name);
    return type->base == GLSL_BASE_STRUCT && type->structName != NULL &&
           !strcmp(type->structName, name);
}

static int GlslStructReady(const GlslDecl *decl,
                           const GlslDecl *remaining)
{
    const GlslDecl *member;
    const GlslDecl *other;

    for (member = decl->members; member != NULL; member = member->next) {
        for (other = remaining; other != NULL; other = other->next) {
            if (other != decl &&
                GlslTypeUsesStruct(&member->type, other->name)) return 0;
        }
    }
    return 1;
}

static int GlslSortStructs(GlslLowerContext *context)
{
    GlslDecl *remaining;
    GlslDecl *ordered;
    GlslDecl **tail;
    GlslDecl **place;
    GlslDecl **best;
    GlslDecl *decl;

    remaining = context->module->structs;
    ordered = NULL;
    tail = &ordered;
    while (remaining != NULL) {
        best = NULL;
        for (place = &remaining; *place != NULL; place = &(*place)->next) {
            if (GlslStructReady(*place, remaining) &&
                (best == NULL || GlslDeclComesBefore(*place, *best)))
            {
                best = place;
            }
        }
        if (best == NULL) {
            GlslRecordFailureKind(context, GLSL_ERROR_UNSUPPORTED_TYPE,
                                  "recursive GLSL structure");
            return 0;
        }
        decl = *best;
        *best = decl->next;
        decl->next = NULL;
        *tail = decl;
        tail = &decl->next;
    }
    context->module->structs = ordered;
    return 1;
}

static void GlslInsertBinding(GlslBinding **list, GlslBinding *binding);

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

static int GlslCollectParameters(GlslLowerContext *context, Symbol *formal,
                                 int entry)
{
    GlslDecl *decl;
    int qualifiers;

    for (; formal != NULL; formal = formal->next) {
        if (GetDomain(formal->type) == TYPE_DOMAIN_UNIFORM) {
            if (entry)
                continue;
            return 0;
        }
        decl = GlslNewSourceDecl(context, formal,
                                 entry ? NULL : context->function->identity);
        if (decl == NULL)
            return 0;
        if (entry) {
            GlslAppendDecl(&context->function->locals, decl);
        } else {
            qualifiers = GetQualifiers(formal->type);
            if ((qualifiers & TYPE_QUALIFIER_INOUT) ==
                TYPE_QUALIFIER_INOUT)
            {
                decl->parameterQualifier = GLSL_PARAMETER_INOUT;
            } else if (qualifiers & TYPE_QUALIFIER_OUT) {
                decl->parameterQualifier = GLSL_PARAMETER_OUT;
            }
            GlslAppendDecl(&context->function->parameters, decl);
        }
    }
    return 1;
}

static GlslBinding *GlslFindUniformBinding(GlslModule *module,
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
        GetCategory(symbol->type) != TYPE_CATEGORY_SCALAR)
    {
        context->statementLoc = symbol->loc;
        GlslRecordFailureKind(context, GLSL_ERROR_SAMPLER,
                              "sampler arrays or aggregates");
        return 0;
    }
    if (!GlslEnsureTypeAt(context, symbol->type, &symbol->loc) ||
        !GlslLowerType(context, symbol->type, &type,
                       &symbol->loc)) return 0;
    storage = Cg->theHAL->IsTexobjBase(GetBase(symbol->type)) ?
              GLSL_STORAGE_SAMPLER : GLSL_STORAGE_UNIFORM;
    sourceName = GetAtomString(atable, symbol->name);
    name = GlslAllocateSymbolName(context->module, symbol, sourceName);
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

static int GlslCollectUniforms(GlslLowerContext *context, Symbol *program)
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

static int GlslValidateUniformLimit(GlslLowerContext *context)
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

static int GlslValidateInterfaceLimits(GlslLowerContext *context)
{
    GlslBinding *binding;
    int attributes;
    int varyingComponents;
    int fragmentColors;
    int components;

    attributes = 0;
    varyingComponents = 0;
    fragmentColors = 0;
    for (binding = context->module->bindings; binding != NULL;
         binding = binding->next)
    {
        if (binding->declaration == NULL)
            continue;
        if (binding->storage == GLSL_STORAGE_ATTRIBUTE) {
            attributes++;
            if (attributes > context->profile->limits.attributes) {
                return GlslRecordResourceLimit(context, binding,
                    "vertex attributes", attributes,
                    context->profile->limits.attributes);
            }
        } else if (binding->storage == GLSL_STORAGE_VARYING) {
            components = GlslTypeComponentCount(
                &binding->declaration->type);
            if (components <= 0 ||
                components > INT_MAX - varyingComponents)
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
            varyingComponents += components;
            if (varyingComponents >
                context->profile->limits.varyingComponents)
            {
                return GlslRecordResourceLimit(context, binding,
                    "varying components", varyingComponents,
                    context->profile->limits.varyingComponents);
            }
        } else if (context->profile->stage == GLSL_STAGE_FRAGMENT &&
                   binding->storage == GLSL_STORAGE_BUILTIN &&
                   binding->isOutput && binding->name != NULL &&
                   !strcmp(binding->name, "gl_FragColor"))
        {
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

static int GlslAllocateTextureUnits(GlslLowerContext *context)
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
                   "fragment texture units" : "vertex texture units";
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

static int GlslAppendDefaultValue(GlslLowerContext *context, float *values,
    int capacity, int *count, float value)
{
    if (values == NULL || count == NULL || *count < 0 ||
        *count >= capacity)
    {
        GlslRecordFailure(context, "uniform default component count");
        return 0;
    }
    if (value != value || value > FLT_MAX || value < -FLT_MAX) {
        GlslRecordFailure(context, "uniform default finite value");
        return 0;
    }
    values[(*count)++] = value;
    return 1;
}

typedef struct GlslDefaultValue_Rec {
    int base;
    scalar_constant value;
} GlslDefaultValue;

typedef enum GlslDefaultBaseClass_Enum {
    GLSL_DEFAULT_BASE_INVALID,
    GLSL_DEFAULT_BASE_FLOAT,
    GLSL_DEFAULT_BASE_INT,
    GLSL_DEFAULT_BASE_BOOL
} GlslDefaultBaseClass;

static GlslDefaultBaseClass GlslDefaultBaseClassOf(int base)
{
    switch (base) {
    case TYPE_BASE_CFLOAT:
    case TYPE_BASE_FLOAT:
        return GLSL_DEFAULT_BASE_FLOAT;
    case TYPE_BASE_CINT:
    case TYPE_BASE_INT:
        return GLSL_DEFAULT_BASE_INT;
    case TYPE_BASE_BOOLEAN:
        return GLSL_DEFAULT_BASE_BOOL;
    default:
        if (Cg->theHAL->IsNumericBase(base))
            return GLSL_DEFAULT_BASE_FLOAT;
        return GLSL_DEFAULT_BASE_INVALID;
    }
}

static int GlslFiniteDefaultFloat(float value)
{
    return value == value && value <= FLT_MAX && value >= -FLT_MAX;
}

static int GlslAppendTypedDefaultValue(GlslLowerContext *context,
    GlslDefaultValue *values, int capacity, int *count, int base,
    const scalar_constant *value)
{
    GlslDefaultBaseClass baseClass;

    if (values == NULL || count == NULL || value == NULL || *count < 0 ||
        *count >= capacity)
    {
        GlslRecordFailure(context, "uniform default component count");
        return 0;
    }
    baseClass = GlslDefaultBaseClassOf(base);
    if (baseClass == GLSL_DEFAULT_BASE_INVALID) {
        GlslRecordFailureKind(context, GLSL_ERROR_UNSUPPORTED_TYPE,
                              "uniform default conversion type");
        return 0;
    }
    if (baseClass == GLSL_DEFAULT_BASE_FLOAT &&
        !GlslFiniteDefaultFloat(value->f))
    {
        GlslRecordFailure(context, "uniform default finite value");
        return 0;
    }
    values[*count].base = base;
    values[*count].value = *value;
    (*count)++;
    return 1;
}

static int GlslConvertDefaultValue(GlslLowerContext *context,
    GlslDefaultValue *value, int targetBase)
{
    GlslDefaultBaseClass sourceClass;
    GlslDefaultBaseClass targetClass;
    scalar_constant converted;
    double floating;

    if (value == NULL)
        return 0;
    sourceClass = GlslDefaultBaseClassOf(value->base);
    targetClass = GlslDefaultBaseClassOf(targetBase);
    if (sourceClass == GLSL_DEFAULT_BASE_INVALID ||
        targetClass == GLSL_DEFAULT_BASE_INVALID)
    {
        GlslRecordFailureKind(context, GLSL_ERROR_UNSUPPORTED_TYPE,
                              "uniform default conversion type");
        return 0;
    }
    if (sourceClass == GLSL_DEFAULT_BASE_FLOAT &&
        !GlslFiniteDefaultFloat(value->value.f))
    {
        GlslRecordFailure(context, "uniform default finite value");
        return 0;
    }
    switch (targetClass) {
    case GLSL_DEFAULT_BASE_FLOAT:
        if (sourceClass == GLSL_DEFAULT_BASE_FLOAT)
            converted.f = value->value.f;
        else if (sourceClass == GLSL_DEFAULT_BASE_INT)
            converted.f = (float) value->value.i;
        else
            converted.f = value->value.i ? 1.0f : 0.0f;
        if (!GlslFiniteDefaultFloat(converted.f)) {
            GlslRecordFailure(context, "uniform default finite value");
            return 0;
        }
        break;
    case GLSL_DEFAULT_BASE_INT:
        if (sourceClass == GLSL_DEFAULT_BASE_FLOAT) {
            floating = (double) value->value.f;
            if (floating < (double) INT_MIN ||
                floating > (double) INT_MAX)
            {
                GlslRecordFailure(context,
                                  "uniform default conversion range");
                return 0;
            }
            converted.i = (int) floating;
        } else if (sourceClass == GLSL_DEFAULT_BASE_INT) {
            converted.i = value->value.i;
        } else {
            converted.i = value->value.i ? 1 : 0;
        }
        break;
    case GLSL_DEFAULT_BASE_BOOL:
        if (sourceClass == GLSL_DEFAULT_BASE_FLOAT)
            converted.i = value->value.f != 0.0f;
        else
            converted.i = value->value.i != 0;
        break;
    default:
        GlslRecordFailureKind(context, GLSL_ERROR_UNSUPPORTED_TYPE,
                              "uniform default conversion type");
        return 0;
    }
    value->base = targetBase;
    value->value = converted;
    return 1;
}

static int GlslFlattenDefaultExpr(GlslLowerContext *context,
    const expr *source, GlslDefaultValue *values, int capacity, int *count)
{
    const expr *item;
    int base;
    int componentCount;
    int i;
    int len;
    int start;
    int targetBase;

    if (source == NULL) {
        GlslRecordFailure(context, "uniform default initializer");
        return 0;
    }
    if (source->common.kind == BINARY_N &&
        source->bin.op == EXPR_LIST_OP)
    {
        item = source;
        while (item != NULL && item->common.kind == BINARY_N &&
               item->bin.op == EXPR_LIST_OP)
        {
            if (item->bin.left == NULL ||
                !GlslFlattenDefaultExpr(context, item->bin.left, values,
                                        capacity, count)) return 0;
            item = item->bin.right;
        }
        return item == NULL || GlslFlattenDefaultExpr(context, item, values,
                                                       capacity, count);
    }
    if (source->common.kind == UNARY_N) {
        if (source->un.op == VECTOR_V_OP)
            return GlslFlattenDefaultExpr(context, source->un.arg, values,
                                           capacity, count);
        if (source->un.op == CAST_CS_OP || source->un.op == CAST_CV_OP ||
            source->un.op == CAST_CM_OP)
        {
            start = *count;
            if (!GlslFlattenDefaultExpr(context, source->un.arg, values,
                                         capacity, count)) return 0;
            componentCount = *count - start;
            if (source->un.op == CAST_CS_OP) {
                len = 1;
            } else if (source->un.op == CAST_CV_OP) {
                len = SUBOP_GET_S1(source->un.subop);
            } else {
                len = SUBOP_GET_S1(source->un.subop);
                if (len <= 0 || SUBOP_GET_S2(source->un.subop) <= 0 ||
                    len > INT_MAX / SUBOP_GET_S2(source->un.subop))
                {
                    GlslRecordFailure(context,
                                      "uniform default cast shape");
                    return 0;
                }
                len *= SUBOP_GET_S2(source->un.subop);
            }
            if (len <= 0 || componentCount != len) {
                GlslRecordFailure(context, "uniform default cast shape");
                return 0;
            }
            targetBase = SUBOP_GET_T2(source->un.subop);
            for (i = start; i < *count; i++) {
                if (!GlslConvertDefaultValue(context, &values[i],
                                             targetBase)) return 0;
            }
            return 1;
        }
    }
    if (source->common.kind == CONST_N) {
        base = GetBase(source->common.type);
        len = SUBOP_GET_S1(source->co.subop);
        if (len == 0)
            len = 1;
        if (len < 1 || len > 4) {
            GlslRecordFailure(context, "uniform default initializer");
            return 0;
        }
        for (i = 0; i < len; i++) {
            switch (source->co.op) {
            case FCONST_OP:
            case FCONST_V_OP:
            case HCONST_OP:
            case HCONST_V_OP:
            case XCONST_OP:
            case XCONST_V_OP:
                if (!GlslAppendTypedDefaultValue(context, values, capacity,
                        count, base, &source->co.val[i]))
                    return 0;
                break;
            case ICONST_OP:
            case ICONST_V_OP:
            case BCONST_OP:
            case BCONST_V_OP:
                if (!GlslAppendTypedDefaultValue(context, values, capacity,
                        count, base, &source->co.val[i]))
                    return 0;
                break;
            default:
                GlslRecordFailure(context, "uniform default initializer");
                return 0;
            }
        }
        return 1;
    }
    GlslRecordFailure(context, "uniform default expression shape");
    return 0;
}

static int GlslDefaultTargetBase(GlslLowerContext *context, GlslBase base)
{
    switch (base) {
    case GLSL_BASE_FLOAT:
        return TYPE_BASE_FLOAT;
    case GLSL_BASE_INT:
        return TYPE_BASE_INT;
    case GLSL_BASE_BOOL:
        return TYPE_BASE_BOOLEAN;
    default:
        GlslRecordFailureKind(context, GLSL_ERROR_UNSUPPORTED_TYPE,
                              "uniform default destination type");
        return TYPE_BASE_NO_TYPE;
    }
}

static int GlslStoreDefaultLeaf(GlslLowerContext *context, GlslBase base,
    int len, GlslDefaultValue *typedValues, int capacity, int *index,
    float *values, int *count)
{
    GlslDefaultBaseClass targetClass;
    int i;
    int targetBase;

    targetBase = GlslDefaultTargetBase(context, base);
    if (targetBase == TYPE_BASE_NO_TYPE || len <= 0 ||
        *index < 0 || *index > capacity - len)
    {
        if (targetBase != TYPE_BASE_NO_TYPE)
            GlslRecordFailure(context, "uniform default component count");
        return 0;
    }
    targetClass = GlslDefaultBaseClassOf(targetBase);
    for (i = 0; i < len; i++) {
        if (!GlslConvertDefaultValue(context, &typedValues[*index],
                                     targetBase)) return 0;
        if (targetClass == GLSL_DEFAULT_BASE_FLOAT) {
            if (!GlslAppendDefaultValue(context, values, capacity, count,
                    typedValues[*index].value.f)) return 0;
        } else {
            if (!GlslAppendDefaultValue(context, values, capacity, count,
                    (float) typedValues[*index].value.i)) return 0;
        }
        (*index)++;
    }
    return 1;
}

static int GlslStoreDefaultType(GlslLowerContext *context,
    const GlslType *type, GlslDefaultValue *typedValues, int capacity,
    int *index, float *values, int *count)
{
    const GlslDecl *member;
    int i;
    int len;

    if (type == NULL)
        return 0;
    if (type->elementType != NULL) {
        if (type->arraySize <= 0)
            return 0;
        for (i = 0; i < type->arraySize; i++) {
            if (!GlslStoreDefaultType(context, type->elementType,
                    typedValues, capacity, index, values, count)) return 0;
        }
        return 1;
    }
    if (type->base == GLSL_BASE_STRUCT) {
        if (type->members == NULL)
            return 0;
        for (member = type->members; member != NULL; member = member->next) {
            if (!GlslStoreDefaultType(context, &member->type, typedValues,
                    capacity, index, values, count)) return 0;
        }
        return 1;
    }
    if (type->rows != 0 || type->cols != 0) {
        if (type->rows <= 0 || type->cols <= 0 ||
            type->rows > INT_MAX / type->cols)
        {
            GlslRecordFailureKind(context, GLSL_ERROR_UNSUPPORTED_TYPE,
                                  "uniform default destination type");
            return 0;
        }
        len = type->rows * type->cols;
    } else {
        len = type->len;
    }
    return GlslStoreDefaultLeaf(context, type->base, len, typedValues,
                                capacity, index, values, count);
}

static int GlslCollectDefaults(GlslLowerContext *context)
{
    BindingList *item;
    GlslBinding *binding;
    Symbol *symbol;
    GlslDefaultValue *typedValues;
    int componentCount;
    int typedCount;
    int typedIndex;
    int valueCount;

    for (item = Cg->theHAL->defaultBindings; item != NULL;
         item = item->next)
    {
        if (item->binding == NULL ||
            item->binding->none.kind != BK_DEFAULT) continue;
        for (binding = context->module->bindings; binding != NULL;
             binding = binding->next)
        {
            if (binding->storage != GLSL_STORAGE_UNIFORM ||
                binding->declaration == NULL || binding->defaultCount != 0)
                continue;
            symbol = (Symbol *) binding->declaration->identity;
            if (symbol == NULL || item->identity != symbol)
                continue;
            context->statementLoc = symbol->loc;
            componentCount = GlslTypeComponentCount(
                &binding->declaration->type);
            if (componentCount <= 0 || item->initializer == NULL ||
                item->type != symbol->type)
            {
                GlslRecordFailure(context, "uniform default initializer");
                return 0;
            }
            if ((size_t) componentCount >
                (size_t) -1 / sizeof(GlslDefaultValue) ||
                (size_t) componentCount > (size_t) -1 / sizeof(float))
            {
                GlslRecordFailure(context,
                                  "uniform default component count");
                return 0;
            }
            typedValues = (GlslDefaultValue *) context->module->alloc(
                context->module->allocArg,
                (size_t) componentCount * sizeof(GlslDefaultValue));
            binding->defaultValues = (float *) context->module->alloc(
                context->module->allocArg,
                (size_t) componentCount * sizeof(float));
            if (typedValues == NULL || binding->defaultValues == NULL)
                return 0;
            typedCount = 0;
            if (!GlslFlattenDefaultExpr(context,
                    (const expr *) item->initializer,
                    typedValues, componentCount, &typedCount) ||
                typedCount != componentCount)
            {
                if (typedCount != componentCount)
                    GlslRecordFailure(context,
                                      "uniform default component count");
                return 0;
            }
            typedIndex = 0;
            valueCount = 0;
            if (!GlslStoreDefaultType(context, &binding->declaration->type,
                    typedValues, componentCount, &typedIndex,
                    binding->defaultValues, &valueCount) ||
                typedIndex != componentCount || valueCount != componentCount)
            {
                if (typedIndex != componentCount ||
                    valueCount != componentCount)
                {
                    GlslRecordFailure(context,
                                      "uniform default component count");
                }
                return 0;
            }
            binding->defaultCount = componentCount;
            break;
        }
    }
    return 1;
}

static int GlslCollectLocals(GlslLowerContext *context, Symbol *symbol,
                             int entry)
{
    const char *name;
    const void *nameSpace;
    GlslDecl *decl;

    if (symbol == NULL)
        return 1;
    if (!GlslCollectLocals(context, symbol->left, entry))
        return 0;
    if (symbol->kind == VARIABLE_S && GlslFindDecl(context, symbol) == NULL) {
        name = GetAtomString(atable, symbol->name);
        if (name == NULL || name[0] == '$')
            return 0;
        nameSpace = entry ? NULL : context->function->identity;
        decl = GlslNewSourceDecl(context, symbol, nameSpace);
        if (decl == NULL)
            return 0;
        GlslInsertDecl(&context->function->locals, decl);
    }
    return GlslCollectLocals(context, symbol->right, entry);
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

static void GlslInsertBinding(GlslBinding **list, GlslBinding *binding)
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

static GlslDecl *GlslLowerInterface(GlslLowerContext *context,
                                    Symbol *member)
{
    Binding *sourceBinding;
    GlslBinding *binding;
    GlslDecl *decl;
    GlslType type;
    GlslStorage storage;
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
        !GlslLowerType(context, member->type, &type,
                       &member->loc)) return NULL;
    if (GlslHasInterfaceBinding(context, interfaceName, isOutput)) {
        context->statementLoc = member->loc;
        GlslRecordFailureKind(context, GLSL_ERROR_INTERFACE_CONFLICT,
                              interfaceName);
        return NULL;
    }
    if (!strncmp(interfaceName, "gl_", 3)) {
        storage = GLSL_STORAGE_BUILTIN;
        name = interfaceName;
    } else {
        if (strlen(interfaceName) + 4 > sizeof(generatedName))
            return NULL;
        sprintf(generatedName, "cg_%s", interfaceName);
        name = GlslReservedInterfaceName(context, interfaceName,
                                         isOutput);
        if (name == NULL) {
            name = GlslAllocateSymbolName(context->module, member,
                                          generatedName);
        }
        if (name == NULL)
            return NULL;
        if (!isOutput && context->profile->stage == GLSL_STAGE_VERTEX)
            storage = GLSL_STORAGE_ATTRIBUTE;
        else
            storage = GLSL_STORAGE_VARYING;
    }
    decl = GlslNewDecl(context->module, storage, type, name);
    binding = GlslNewBinding(context->module, storage, name, canonical);
    if (decl == NULL || binding == NULL)
        return NULL;
    decl->identity = member;
    GlslSetLoc(&decl->loc, &member->loc);
    decl->sourceOrdinal = member->sourceOrdinal;
    binding->declaration = decl;
    binding->interfaceKey = interfaceName;
    binding->isOutput = isOutput;
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

static int GlslCollectCallsInStatements(GlslLowerContext *context,
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
        if (!GlslLowerType(context, leftParam->type, &leftType,
                           &leftParam->loc) ||
            !GlslLowerType(context, rightParam->type, &rightType,
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
        if (!GlslLowerType(context, parameter->type, &type,
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

static int GlslAssignHelperNames(GlslLowerContext *context)
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
            function->name = GlslAllocateSymbolName(context->module,
                                                    symbol, sourceName);
        } else if (sameSignature == NULL) {
            function->name = sameName->name;
        } else {
            if (!GlslBuildSignature(context, symbol, signature,
                                    sizeof(signature)) ||
                strlen(sourceName) + strlen(signature) + 24 >
                    sizeof(candidate)) return 0;
            sprintf(candidate, "%s_%s_%d", sourceName, signature,
                    collapsedIndex);
            function->name = GlslAllocateSymbolName(context->module,
                                                    symbol, candidate);
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

static void GlslMarkForwardCalls(GlslLowerContext *context)
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

static void GlslAppendExpr(GlslExpr **list, GlslExpr *expression)
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

static GlslExpr *GlslLowerExpr(GlslLowerContext *context, expr *source);

static GlslExpr *GlslLowerExprChain(GlslLowerContext *context, expr *source,
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

static GlslExpr *GlslNewLiteral(GlslLowerContext *context, GlslBase base,
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
            if (source->co.val[i].f != source->co.val[i].f ||
                source->co.val[i].f > FLT_MAX ||
                source->co.val[i].f < -FLT_MAX)
            {
                GlslRecordFailure(context,
                                  "non-finite floating-point constant");
                return NULL;
            }
        }
    }

    if (type->len == 1) {
        if (type->base == GLSL_BASE_FLOAT)
            return GlslNewLiteral(context, type->base, 0, source->co.val[0].f);
        return GlslNewLiteral(context, type->base, source->co.val[0].i, 0.0f);
    }
    target = GlslNewExpr(context->module, GLSL_EXPR_CONSTRUCT, *type);
    if (target == NULL)
        return NULL;
    for (i = 0; i < type->len; i++) {
        if (type->base == GLSL_BASE_FLOAT)
            item = GlslNewLiteral(context, type->base, 0, source->co.val[i].f);
        else
            item = GlslNewLiteral(context, type->base, source->co.val[i].i, 0.0f);
        if (item == NULL)
            return NULL;
        GlslAppendExpr(&target->u.construct.arguments, item);
    }
    return target;
}

static GlslExpr *GlslNewSwizzle(GlslLowerContext *context, GlslExpr *object,
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

static int GlslMatrixSelectorCount(const expr *source)
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

static GlslExpr *GlslMatrixSelectorComponent(GlslLowerContext *context,
    GlslExpr *matrix, const expr *selectorSource, int component)
{
    if (selectorSource == NULL)
        return NULL;
    return GlslMatrixMaskComponent(context, matrix,
        SUBOP_GET_MASK16(selectorSource->un.subop), component);
}

static GlslMatrixSelectorHelper *GlslGetMatrixSelectorHelper(
    GlslLowerContext *context, GlslMatrixSelectorHelperKind kind,
    const GlslType *matrixType, const GlslType *valueType, int count,
    int mask);

static GlslExpr *GlslLowerMatrixSwizzle(GlslLowerContext *context,
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

static int GlslMatrixNumericParameterType(const GlslType *type)
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
    return GlslAllocateDistinctName(context->module, candidate);
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
    return GlslAllocateDistinctName(context->module, candidate);
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
    matrixName = GlslAllocateScopedSymbolName(context->module, function,
                                               NULL, "matrix");
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
        valueName = GlslAllocateScopedSymbolName(context->module, function,
                                                 NULL, "value");
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

static GlslMatrixSelectorHelper *GlslGetMatrixSelectorHelper(
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
        parameterName = GlslAllocateScopedSymbolName(context->module,
            helper->function, NULL, candidate);
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

static GlslExpr *GlslLowerImpureMatrixConstructor(
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

static GlslExpr *GlslLowerMatrixConstructor(GlslLowerContext *context,
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
    case ASSIGN_GEN_OP: return GLSL_OP_ASSIGN;
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

static int GlslValidateTextureCall(GlslLowerContext *context,
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
        builtin > GLSL_BUILTIN_TEXCUBE) return 1;
    if (context->profile->stage != GLSL_STAGE_FRAGMENT) {
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
        GetCategory(symbol->type) != TYPE_CATEGORY_SCALAR ||
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
    } else if (source->bin.op == FUN_BUILTIN_OP && symbol != NULL &&
               symbol->kind == FUNCTION_S &&
               (symbol->properties & SYMB_IS_BUILTIN) &&
               symbol->details.fun.group == GLSL_BUILTIN_GROUP &&
               (source->bin.subop >> 16) == GLSL_BUILTIN_GROUP &&
               (source->bin.subop & 0xffff) == symbol->details.fun.index)
    {
        builtin = (GlslBuiltin) symbol->details.fun.index;
        name = GlslBuiltinSpelling(builtin);
        if (name == NULL)
            return NULL;
    } else {
        return NULL;
    }
    arguments = NULL;
    if (source->bin.right != NULL) {
        if (function == NULL && builtin >= GLSL_BUILTIN_TEX1D &&
            builtin <= GLSL_BUILTIN_TEXCUBE)
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
        builtin = (GlslBuiltin) symbol->details.fun.index;
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

static GlslExpr *GlslLowerExpr(GlslLowerContext *context, expr *source)
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
                              "GLSL 1.10 expression type");
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
            source->bin.op == FUN_BUILTIN_OP)
            return GlslLowerCall(context, source, &type);
        if ((source->bin.op == ASSIGN_OP ||
             source->bin.op == ASSIGN_V_OP ||
             source->bin.op == ASSIGN_GEN_OP ||
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

static int GlslLowerStatementList(GlslLowerContext *context, stmt *source,
                                  GlslStmt **list);

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

static int GlslLowerStatementList(GlslLowerContext *context, stmt *source,
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
            GlslRecordFailure(context, "GLSL 1.10 statement");
            return 0;
        }
        if (target == NULL)
            return 0;
        GlslSetLoc(&target->loc, &source->commonst.loc);
        GlslAppendStmt(list, target);
    }
    return 1;
}

static int GlslLowerHelper(GlslLowerContext *context,
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

static void GlslPrependMatrixHelpers(GlslLowerContext *context)
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

static void GlslPrependMatrixSelectorHelpers(GlslLowerContext *context)
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
        record->reservedName = GlslAllocateName(context->module,
                                                generatedName);
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

static int GlslValidateEntryInterfaces(GlslLowerContext *context,
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

int GlslLowerProgram(GlslModule *module, const GlslProfileDesc *profile,
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
    functionName = GlslAllocateSymbolName(module, program, "main");
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

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

#include <float.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>

#include "hlsl_lower_internal.h"

static Type *HlslCanonicalStructType(Type *type);

static const char *HlslLogicalTypeName(HlslLowerContext *context,
                                       Type *source)
{
    Type *current;
    Type *seen[129];
    Type *canonical;
    HlslSourceType described;
    const char *base;
    char dimension[32];
    char *result;
    char *write;
    int dimensions[128];
    int dimensionCount;
    int cols;
    int rows;
    int vectorLength;
    int i;
    size_t length;
    size_t dimensionLength;

    if (context == NULL || source == NULL)
        return NULL;
    current = source;
    dimensionCount = 0;
    while (GetCategory(current) == TYPE_CATEGORY_ARRAY &&
           !IsMatrix(current, &cols, &rows) &&
           !IsVector(current, &vectorLength))
    {
        if (dimensionCount >= 128 || current->arr.numels <= 0 ||
            current->arr.eltype == NULL)
        {
            return NULL;
        }
        for (i = 0; i < dimensionCount; i++) {
            if (seen[i] == current)
                return NULL;
        }
        seen[dimensionCount] = current;
        dimensions[dimensionCount++] = current->arr.numels;
        current = current->arr.eltype;
    }
    if (GetCategory(current) == TYPE_CATEGORY_STRUCT) {
        canonical = HlslCanonicalStructType(current);
        base = canonical != NULL && canonical->str.tag != 0 ?
               GetAtomString(atable, canonical->str.tag) : NULL;
    } else if (HlslDescribeSourceType(current, &described)) {
        base = HlslSourceTypeName(&described);
    } else {
        base = NULL;
    }
    if (base == NULL || base[0] == '\0')
        return NULL;
    length = strlen(base);
    for (i = 0; i < dimensionCount; i++) {
        sprintf(dimension, "[%d]", dimensions[i]);
        dimensionLength = strlen(dimension);
        if (length > (size_t) -1 - dimensionLength)
            return NULL;
        length += dimensionLength;
    }
    if (length == (size_t) -1)
        return NULL;
    result = (char *) HlslLowerAlloc(context, length + 1);
    if (result == NULL)
        return NULL;
    memcpy(result, base, strlen(base));
    write = result + strlen(base);
    for (i = 0; i < dimensionCount; i++) {
        sprintf(dimension, "[%d]", dimensions[i]);
        dimensionLength = strlen(dimension);
        memcpy(write, dimension, dimensionLength);
        write += dimensionLength;
    }
    *write = '\0';
    return result;
} // HlslLogicalTypeName

static Type *HlslCanonicalStructType(Type *type)
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
} // HlslCanonicalStructType

static Symbol *HlslFindTag(HlslLowerContext *context, Type *type)
{
    Type *canonical;
    Scope *scope;
    Symbol *tag;

    canonical = HlslCanonicalStructType(type);
    if (canonical == NULL)
        return NULL;
    scope = context->scope != NULL ? context->scope : CurrentScope;
    for (; scope != NULL; scope = scope->parent) {
        tag = LookUpLocalTag(scope, canonical->str.tag);
        if (tag != NULL && HlslCanonicalStructType(tag->type) == canonical)
            return tag;
    }
    return NULL;
} // HlslFindTag

static HlslDecl *HlslFindStruct(HlslLowerContext *context, Type *type)
{
    HlslDecl *decl;
    Symbol *tag;

    tag = HlslFindTag(context, type);
    if (tag == NULL)
        return NULL;
    for (decl = context->module->structs; decl != NULL; decl = decl->next) {
        if (decl->identity == tag)
            return decl;
    }
    return NULL;
} // HlslFindStruct

int HlslLowerType(HlslLowerContext *context, Type *source,
                         HlslType *target, const SourceLoc *loc)
{
    HlslDecl *structDecl;
    HlslType elementType;
    HlslType *element;
    HlslBase base;
    int sourceBase;
    int category;
    int len;
    int rows;
    int cols;

    if (source == NULL || target == NULL)
        return 0;
    if (CgIsAttribArray(source)) {
        if (context->profile == NULL ||
            context->profile->stage != HLSL_STAGE_GEOMETRY ||
            context->geometryInputExtent <= 0 ||
            (CgAttribArrayExtent(source) != 0 &&
             CgAttribArrayExtent(source) !=
                (unsigned int) context->geometryInputExtent) ||
            !HlslEnsureType(context, CgAttribArrayElement(source)) ||
            !HlslLowerType(context, CgAttribArrayElement(source),
                           &elementType, loc))
        {
            return HlslLowerFailure(context, HLSL_ERROR_ENTRY_ABI,
                                    "geometry AttribArray extent", loc);
        }
        element = (HlslType *) HlslLowerAlloc(context, sizeof(HlslType));
        if (element == NULL)
            return 0;
        *element = elementType;
        *target = HlslNumericType(HLSL_BASE_VOID, 0);
        target->arraySize = context->geometryInputExtent;
        target->elementType = element;
        return 1;
    }
    if (IsVoid(source)) {
        *target = HlslNumericType(HLSL_BASE_VOID, 0);
        return 1;
    }
    category = GetCategory(source);
    if (category == TYPE_CATEGORY_SAMPLER) {
        switch (source->samp.samplerKind) {
        case CG_SAMPLER_1D: base = HLSL_BASE_SAMPLER1D; break;
        case CG_SAMPLER_2D: base = HLSL_BASE_SAMPLER2D; break;
        case CG_SAMPLER_3D: base = HLSL_BASE_SAMPLER3D; break;
        case CG_SAMPLER_CUBE: base = HLSL_BASE_SAMPLERCUBE; break;
        default:
            return HlslLowerFailure(context,
                source->samp.samplerKind == CG_SAMPLER_RECT ?
                HLSL_ERROR_SAMPLER : HLSL_ERROR_UNSUPPORTED_TYPE,
                source->samp.samplerKind == CG_SAMPLER_RECT ?
                "samplerRECT" : "sampler", loc);
        }
        *target = HlslNumericType(base, 1);
        return 1;
    }
    sourceBase = GetBase(source);
    if (IsMatrix(source, &cols, &rows)) {
        if ((sourceBase != TYPE_BASE_FLOAT &&
             sourceBase != TYPE_BASE_CFLOAT) ||
            rows < 1 || rows > 4 || cols < 1 || cols > 4)
        {
            return HlslLowerFailure(context, HLSL_ERROR_UNSUPPORTED_TYPE,
                                    "matrix", loc);
        }
        *target = HlslMatrixType(rows, cols);
        return 1;
    }
    switch (sourceBase) {
    case TYPE_BASE_FLOAT:
    case TYPE_BASE_CFLOAT: base = HLSL_BASE_FLOAT; break;
    case TYPE_BASE_INT:
    case TYPE_BASE_CINT: base = HLSL_BASE_INT; break;
    case TYPE_BASE_BOOLEAN: base = HLSL_BASE_BOOL; break;
    default: base = HLSL_BASE_VOID; break;
    }
    if (category == TYPE_CATEGORY_SCALAR && base != HLSL_BASE_VOID) {
        *target = HlslNumericType(base, 1);
        return 1;
    }
    if (category == TYPE_CATEGORY_ARRAY && IsVector(source, &len) &&
        base != HLSL_BASE_VOID && len >= 1 && len <= 4)
    {
        *target = HlslNumericType(base, len);
        return 1;
    }
    if (category == TYPE_CATEGORY_ARRAY && source->arr.numels > 0 &&
        HlslEnsureType(context, source->arr.eltype) &&
        HlslLowerType(context, source->arr.eltype, &elementType, loc))
    {
        element = (HlslType *) HlslLowerAlloc(context, sizeof(HlslType));
        if (element == NULL)
            return 0;
        *element = elementType;
        *target = HlslNumericType(HLSL_BASE_VOID, 0);
        target->arraySize = source->arr.numels;
        target->elementType = element;
        return 1;
    }
    if (category == TYPE_CATEGORY_STRUCT) {
        structDecl = HlslFindStruct(context, source);
        if (structDecl == NULL)
            return 0;
        *target = HlslNumericType(HLSL_BASE_STRUCT, 0);
        target->structName = structDecl->name;
        target->members = structDecl->members;
        return 1;
    }
    return HlslLowerFailure(context, HLSL_ERROR_UNSUPPORTED_TYPE,
                            "HLSL type", loc);
} // HlslLowerType

static int HlslDeclComesBefore(const HlslDecl *left,
                               const HlslDecl *right)
{
    if (left->sourceOrdinal != right->sourceOrdinal)
        return left->sourceOrdinal < right->sourceOrdinal;
    if (left->loc.file != right->loc.file)
        return left->loc.file < right->loc.file;
    if (left->loc.line != right->loc.line)
        return left->loc.line < right->loc.line;
    return strcmp(left->name, right->name) < 0;
} // HlslDeclComesBefore

void HlslInsertDecl(HlslDecl **list, HlslDecl *decl)
{
    HlslDecl **place;

    place = list;
    while (*place != NULL && !HlslDeclComesBefore(decl, *place))
        place = &(*place)->next;
    decl->next = *place;
    *place = decl;
} // HlslInsertDecl

static const char *HlslSourceSemantic(HlslLowerContext *context,
                                      Symbol *symbol, int isOutput)
{
    const char *source;
    Binding *binding;

    if (symbol == NULL)
        return NULL;
    source = symbol->details.var.semantics != 0 ?
             GetAtomString(atable, symbol->details.var.semantics) : NULL;
    binding = symbol->details.var.bind;
    if (source == NULL && binding != NULL &&
        binding->none.kind == BK_CONNECTOR && binding->conn.rname != 0)
    {
        source = GetAtomString(atable, binding->conn.rname);
    }
    return source != NULL ?
           HlslCanonicalSemantic(context->profile, source, isOutput) : NULL;
} // HlslSourceSemantic

const char *HlslFunctionSemantic(HlslLowerContext *context,
                                        Symbol *symbol)
{
    const char *source;

    if (symbol == NULL || symbol->kind != FUNCTION_S ||
        symbol->details.fun.semantics == 0)
    {
        return NULL;
    }
    source = GetAtomString(atable, symbol->details.fun.semantics);
    return source != NULL ?
           HlslCanonicalSemantic(context->profile, source, 1) : NULL;
} // HlslFunctionSemantic

int HlslRejectStorage(HlslLowerContext *context, Symbol *symbol)
{
    if (symbol->storageClass == SC_STATIC)
        return HlslLowerFailure(context, HLSL_ERROR_UNSUPPORTED_OPERATION,
                                "static storage", &symbol->loc);
    if (symbol->storageClass == SC_EXTERN)
        return HlslLowerFailure(context, HLSL_ERROR_UNSUPPORTED_OPERATION,
                                "extern storage", &symbol->loc);
    return 1;
} // HlslRejectStorage

HlslDecl *HlslNewSourceDecl(HlslLowerContext *context,
    Symbol *symbol, const void *nameSpace)
{
    HlslDecl *decl;
    HlslType type;
    const char *sourceName;
    const char *name;
    char *generatedName;

    if (!HlslRejectStorage(context, symbol))
        return NULL;

    if (!HlslEnsureType(context, symbol->type) ||
        !HlslLowerType(context, symbol->type, &type, &symbol->loc))
    {
        return NULL;
    }
    sourceName = GetAtomString(atable, symbol->name);
    if (sourceName == NULL)
        return NULL;
    if (sourceName[0] == '$' || sourceName[0] == '@') {
        generatedName = HlslGeneratedSource(context, "temp");
        name = generatedName != NULL ?
               HlslAllocateGeneratedName(context->module, symbol,
                                         generatedName) : NULL;
    } else if (nameSpace != NULL) {
        name = HlslAllocateScopedSymbolName(context->module, nameSpace,
                                            symbol, sourceName);
    } else {
        name = HlslAllocateSymbolName(context->module, symbol, sourceName);
    }
    if (name == NULL)
        return NULL;
    decl = HlslNewDecl(context->module, HLSL_STORAGE_NONE, type, name);
    if (decl != NULL) {
        decl->identity = symbol;
        decl->publicName = sourceName;
        decl->sourceOrdinal = symbol->sourceOrdinal;
        HlslSetLoc(&decl->loc, &symbol->loc);
    }
    return decl;
} // HlslNewSourceDecl

static int HlslCollectMembers(HlslLowerContext *context, Scope *memberScope,
    Symbol *symbol, HlslDecl **members)
{
    HlslDecl *decl;

    if (symbol == NULL)
        return 1;
    if (!HlslCollectMembers(context, memberScope, symbol->left, members))
        return 0;
    if (symbol->kind == VARIABLE_S) {
        decl = HlslNewSourceDecl(context, symbol, memberScope);
        if (decl == NULL)
            return 0;
        decl->semantic = HlslSourceSemantic(context, symbol, 1);
        decl->inputSemantic = HlslSourceSemantic(context, symbol, 0);
        HlslInsertDecl(members, decl);
    } else if (symbol->kind == FUNCTION_S) {
        return HlslLowerFailure(context,
                                HLSL_ERROR_UNSUPPORTED_OPERATION,
                                "structure method", &symbol->loc);
    }
    return HlslCollectMembers(context, memberScope, symbol->right, members);
} // HlslCollectMembers

static int HlslEnsureSymbolTypes(HlslLowerContext *context, Symbol *symbol)
{
    if (symbol == NULL)
        return 1;
    if (!HlslEnsureSymbolTypes(context, symbol->left))
        return 0;
    if (symbol->kind == VARIABLE_S &&
        !HlslEnsureType(context, symbol->type))
    {
        return 0;
    }
    return HlslEnsureSymbolTypes(context, symbol->right);
} // HlslEnsureSymbolTypes

int HlslEnsureType(HlslLowerContext *context, Type *type)
{
    int category;
    Type *canonical;
    HlslDecl *decl;
    HlslType structType;
    Symbol *tag;
    char *generatedName;
    const char *sourceName;
    const char *name;

    if (type == NULL)
        return 0;
    category = GetCategory(type);
    if (CgIsAttribArray(type)) {
        if (context->profile == NULL ||
            context->profile->stage != HLSL_STAGE_GEOMETRY ||
            context->geometryInputExtent <= 0 ||
            (CgAttribArrayExtent(type) != 0 &&
             CgAttribArrayExtent(type) !=
                (unsigned int) context->geometryInputExtent))
        {
            return HlslLowerFailure(context, HLSL_ERROR_ENTRY_ABI,
                                    "geometry AttribArray extent", NULL);
        }
        return HlslEnsureType(context, CgAttribArrayElement(type));
    }
    if (IsVoid(type) || IsMatrix(type, NULL, NULL) ||
        IsVector(type, NULL) ||
        category == TYPE_CATEGORY_SCALAR ||
        category == TYPE_CATEGORY_SAMPLER)
    {
        return 1;
    }
    if (category == TYPE_CATEGORY_ARRAY) {
        if (type->arr.numels <= 0)
            return HlslLowerFailure(context, HLSL_ERROR_UNSUPPORTED_TYPE,
                                    "unsized array", NULL);
        return HlslEnsureType(context, type->arr.eltype);
    }
    if (category == TYPE_CATEGORY_INTERFACE)
        return HlslLowerFailure(context, HLSL_ERROR_UNSUPPORTED_TYPE,
                                "interface", &type->iface.loc);
    canonical = HlslCanonicalStructType(type);
    if (canonical == NULL)
        return 0;
    if (HlslFindStruct(context, canonical) != NULL)
        return 1;
    tag = HlslFindTag(context, canonical);
    if (tag == NULL) {
        if (canonical->str.tag == 0)
            return HlslLowerFailure(context, HLSL_ERROR_UNSUPPORTED_TYPE,
                                    "anonymous structure",
                                    &canonical->str.loc);
        return 0;
    }
    sourceName = GetAtomString(atable, canonical->str.tag);
    if (sourceName == NULL)
        return HlslLowerFailure(context, HLSL_ERROR_UNSUPPORTED_TYPE,
                                "anonymous structure",
                                &canonical->str.loc);
    if (sourceName[0] == '$')
        return 1;
    generatedName = HlslGeneratedSource(context, sourceName);
    name = generatedName != NULL ?
           HlslAllocateGeneratedName(context->module, tag,
                                     generatedName) : NULL;
    if (name == NULL)
        return 0;
    structType = HlslNumericType(HLSL_BASE_STRUCT, 0);
    structType.structName = name;
    decl = HlslNewDecl(context->module, HLSL_STORAGE_NONE,
                       structType, name);
    if (decl == NULL)
        return 0;
    decl->identity = tag;
    HlslSetLoc(&decl->loc, &canonical->str.loc);
    HlslAppendDecl(&context->module->structs, decl);
    if (canonical->str.members == NULL ||
        !HlslEnsureSymbolTypes(context, canonical->str.members->symbols))
    {
        return 0;
    }
    if (!HlslCollectMembers(context, canonical->str.members,
                            canonical->str.members->symbols,
                            &decl->members))
    {
        return 0;
    }
    decl->type.members = decl->members;
    return 1;
} // HlslEnsureType

static int HlslTypeUsesStruct(const HlslType *type, const char *name)
{
    if (type->elementType != NULL)
        return HlslTypeUsesStruct(type->elementType, name);
    return type->base == HLSL_BASE_STRUCT && type->structName != NULL &&
           !strcmp(type->structName, name);
} // HlslTypeUsesStruct

static int HlslStructReady(const HlslDecl *decl,
                           const HlslDecl *remaining)
{
    const HlslDecl *member;
    const HlslDecl *other;

    for (member = decl->members; member != NULL; member = member->next) {
        for (other = remaining; other != NULL; other = other->next) {
            if (other != decl &&
                HlslTypeUsesStruct(&member->type, other->name))
            {
                return 0;
            }
        }
    }
    return 1;
} // HlslStructReady

int HlslSortStructs(HlslLowerContext *context)
{
    HlslDecl *remaining;
    HlslDecl *ordered;
    HlslDecl **tail;
    HlslDecl **place;
    HlslDecl **best;
    HlslDecl *decl;

    remaining = context->module->structs;
    ordered = NULL;
    tail = &ordered;
    while (remaining != NULL) {
        best = NULL;
        for (place = &remaining; *place != NULL; place = &(*place)->next) {
            if (HlslStructReady(*place, remaining) &&
                (best == NULL || HlslDeclComesBefore(*place, *best)))
            {
                best = place;
            }
        }
        if (best == NULL)
            return HlslLowerFailure(context, HLSL_ERROR_UNSUPPORTED_TYPE,
                                    "recursive HLSL structure", NULL);
        decl = *best;
        *best = decl->next;
        decl->next = NULL;
        *tail = decl;
        tail = &decl->next;
    }
    context->module->structs = ordered;
    return 1;
} // HlslSortStructs

static int HlslParseRegisterSemantic(const char *semantic,
    HlslRegisterBank *bank, int *regno)
{
    char root[32];
    int index;

    if (semantic == NULL || bank == NULL || regno == NULL ||
        !HlslParseSemantic(semantic, root, sizeof(root), &index))
    {
        return 0;
    }
    if (!strcmp(root, "C") || !strcmp(root, "c"))
        *bank = HLSL_REGISTER_C;
    else if (!strcmp(root, "I") || !strcmp(root, "i"))
        *bank = HLSL_REGISTER_I;
    else if (!strcmp(root, "B") || !strcmp(root, "b"))
        *bank = HLSL_REGISTER_B;
    else if (!strcmp(root, "S") || !strcmp(root, "s") ||
             !strcmp(root, "TEXUNIT") || !strcmp(root, "texunit"))
        *bank = HLSL_REGISTER_S;
    else
        return 0;
    *regno = index;
    return 1;
} // HlslParseRegisterSemantic

static HlslBinding *HlslFindUniformBinding(HlslModule *module,
                                           const Symbol *symbol)
{
    HlslBinding *binding;

    for (binding = module->bindings; binding != NULL;
         binding = binding->next)
    {
        if (binding->declaration != NULL &&
            binding->declaration->identity == symbol)
        {
            return binding;
        }
    }
    return NULL;
} // HlslFindUniformBinding

static int HlslSymbolListContains(const SymbolList *list,
                                  const Symbol *symbol)
{
    for (; list != NULL; list = list->next) {
        if (list->symb == symbol)
            return 1;
    }
    return 0;
} // HlslSymbolListContains

static int HlslCollectUniform(HlslLowerContext *context, Symbol *symbol)
{
    Binding *sourceBinding;
    HlslBinding *binding;
    HlslDecl *identityDecl;
    HlslType type;
    HlslStorage storage;
    HlslRegisterBank bank;
    char *generatedName;
    const char *logicalTypeName;
    const char *sourceName;
    const char *semantic;
    int regno;

    if (symbol == NULL || symbol->kind != VARIABLE_S)
    {
        return 1;
    }
    if (!HlslRejectStorage(context, symbol))
        return 0;
    if (GetDomain(symbol->type) != TYPE_DOMAIN_UNIFORM &&
        !HlslSymbolListContains(Cg->theHAL->uniformParam, symbol) &&
        !HlslSymbolListContains(Cg->theHAL->uniformGlobal, symbol))
    {
        return 1;
    }
    if (HlslFindUniformBinding(context->module, symbol) != NULL)
        return 1;
    if (!HlslEnsureType(context, symbol->type) ||
        !HlslLowerType(context, symbol->type, &type, &symbol->loc))
    {
        return 0;
    }
    storage = GetCategory(symbol->type) == TYPE_CATEGORY_SAMPLER ?
              HLSL_STORAGE_SAMPLER : HLSL_STORAGE_UNIFORM;
    sourceName = GetAtomString(atable, symbol->name);
    semantic = symbol->details.var.semantics != 0 ?
               GetAtomString(atable, symbol->details.var.semantics) : NULL;
    logicalTypeName = HlslLogicalTypeName(context, symbol->type);
    binding = HlslNewBinding(context->module, storage, type,
                             sourceName, semantic);
    generatedName = HlslGeneratedSource(context, sourceName);
    identityDecl = generatedName != NULL ?
        HlslNewDecl(context->module, storage, type, generatedName) : NULL;
    if (logicalTypeName == NULL || binding == NULL || identityDecl == NULL)
        return 0;
    identityDecl->identity = symbol;
    binding->declaration = identityDecl;
    binding->publicName = sourceName;
    binding->logicalTypeName = logicalTypeName;
    binding->sourceOrdinal = symbol->sourceOrdinal;
    HlslSetLoc(&binding->loc, &symbol->loc);
    sourceBinding = symbol->details.var.bind;
    if (sourceBinding != NULL &&
        sourceBinding->none.kind == BK_REGARRAY &&
        HlslParseRegisterSemantic(
            GetAtomString(atable, sourceBinding->reg.rname), &bank, &regno))
    {
        binding->hasExplicitRegister = 1;
        binding->physical.bank = bank;
        binding->physical.regno = sourceBinding->reg.regno;
    } else if (sourceBinding != NULL &&
               sourceBinding->none.kind == BK_TEXUNIT)
    {
        binding->hasExplicitRegister = 1;
        binding->physical.bank = HLSL_REGISTER_S;
        binding->physical.regno = sourceBinding->texunit.unitno;
    } else if (HlslParseRegisterSemantic(semantic, &bank, &regno)) {
        binding->hasExplicitRegister = 1;
        binding->physical.bank = bank;
        binding->physical.regno = regno;
    }
    HlslAppendBinding(&context->module->bindings, binding);
    return 1;
} // HlslCollectUniform

int HlslCollectUniformList(HlslLowerContext *context,
                                  SymbolList *list)
{
    for (; list != NULL; list = list->next) {
        if (!HlslCollectUniform(context, list->symb))
            return 0;
    }
    return 1;
} // HlslCollectUniformList

int HlslCollectUniformTree(HlslLowerContext *context, Symbol *symbol)
{
    if (symbol == NULL)
        return 1;
    if (!HlslCollectUniformTree(context, symbol->left) ||
        (symbol->loc.file == context->entryFile &&
         !HlslCollectUniform(context, symbol)))
    {
        return 0;
    }
    return HlslCollectUniformTree(context, symbol->right);
} // HlslCollectUniformTree

typedef struct HlslDefaultValue_Rec {
    int base;
    scalar_constant value;
} HlslDefaultValue;

typedef enum HlslDefaultClass_Enum {
    HLSL_DEFAULT_INVALID,
    HLSL_DEFAULT_FLOAT,
    HLSL_DEFAULT_INT,
    HLSL_DEFAULT_BOOL
} HlslDefaultClass;

static HlslDefaultClass HlslDefaultClassOf(int base)
{
    switch (base) {
    case TYPE_BASE_CFLOAT:
    case TYPE_BASE_FLOAT:
        return HLSL_DEFAULT_FLOAT;
    case TYPE_BASE_CINT:
    case TYPE_BASE_INT:
        return HLSL_DEFAULT_INT;
    case TYPE_BASE_BOOLEAN:
        return HLSL_DEFAULT_BOOL;
    default:
        return Cg->theHAL->IsNumericBase(base) ? HLSL_DEFAULT_FLOAT :
                                                HLSL_DEFAULT_INVALID;
    }
} // HlslDefaultClassOf

static int HlslFiniteDefaultFloat(float value)
{
    return value == value && value <= FLT_MAX && value >= -FLT_MAX;
} // HlslFiniteDefaultFloat

static int HlslAppendTypedDefault(HlslLowerContext *context,
                                  HlslDefaultValue *values, int capacity,
                                  int *count, int base,
                                  const scalar_constant *value)
{
    if (values == NULL || count == NULL || value == NULL ||
        *count < 0 || *count >= capacity ||
        HlslDefaultClassOf(base) == HLSL_DEFAULT_INVALID ||
        (HlslDefaultClassOf(base) == HLSL_DEFAULT_FLOAT &&
         !HlslFiniteDefaultFloat((float) value->value.f)))
    {
        return HlslLowerFailure(context, HLSL_ERROR_INVALID_IR,
                                "uniform default value", NULL);
    }
    values[*count].base = base;
    values[*count].value = *value;
    (*count)++;
    return 1;
} // HlslAppendTypedDefault

static int HlslConvertDefault(HlslLowerContext *context,
                              HlslDefaultValue *value, int targetBase)
{
    HlslDefaultClass sourceClass;
    HlslDefaultClass targetClass;
    scalar_constant converted;
    double floating;

    if (value == NULL)
        return 0;
    sourceClass = HlslDefaultClassOf(value->base);
    targetClass = HlslDefaultClassOf(targetBase);
    if (sourceClass == HLSL_DEFAULT_INVALID ||
        targetClass == HLSL_DEFAULT_INVALID)
    {
        return HlslLowerFailure(context, HLSL_ERROR_UNSUPPORTED_TYPE,
                                "uniform default conversion", NULL);
    }
    if (sourceClass == HLSL_DEFAULT_FLOAT &&
        !HlslFiniteDefaultFloat((float) value->value.value.f))
    {
        return HlslLowerFailure(context, HLSL_ERROR_INVALID_IR,
                                "uniform default finite value", NULL);
    }
    switch (targetClass) {
    case HLSL_DEFAULT_FLOAT:
        converted.value.f = sourceClass == HLSL_DEFAULT_FLOAT ?
            value->value.value.f :
            (sourceClass == HLSL_DEFAULT_INT ?
             (float) value->value.value.i :
             (value->value.value.i ? 1.0f : 0.0f));
        if (!HlslFiniteDefaultFloat((float) converted.value.f))
            return HlslLowerFailure(context, HLSL_ERROR_INVALID_IR,
                                    "uniform default finite value", NULL);
        break;
    case HLSL_DEFAULT_INT:
        if (sourceClass == HLSL_DEFAULT_FLOAT) {
            floating = (double) value->value.value.f;
            if (floating < (double) INT_MIN || floating > (double) INT_MAX)
                return HlslLowerFailure(context, HLSL_ERROR_INVALID_IR,
                                        "uniform default range", NULL);
            converted.value.i = (int) floating;
        } else if (sourceClass == HLSL_DEFAULT_INT) {
            converted.value.i = (int) value->value.value.i;
        } else {
            converted.value.i = value->value.value.i ? 1 : 0;
        }
        break;
    case HLSL_DEFAULT_BOOL:
        converted.value.i = sourceClass == HLSL_DEFAULT_FLOAT ?
            value->value.value.f != 0.0f : value->value.value.i != 0;
        break;
    default:
        return 0;
    }
    value->base = targetBase;
    value->value = converted;
    return 1;
} // HlslConvertDefault

static int HlslFlattenDefaultExpr(HlslLowerContext *context,
                                  const expr *source,
                                  HlslDefaultValue *values, int capacity,
                                  int *count)
{
    const expr *item;
    int base;
    int componentCount;
    int i;
    int len;
    int start;
    int targetBase;

    if (source == NULL)
        return HlslLowerFailure(context, HLSL_ERROR_INVALID_IR,
                                "uniform default initializer", NULL);
    if (source->common.kind == BINARY_N &&
        source->bin.op == EXPR_LIST_OP)
    {
        item = source;
        while (item != NULL && item->common.kind == BINARY_N &&
               item->bin.op == EXPR_LIST_OP)
        {
            if (item->bin.left == NULL ||
                !HlslFlattenDefaultExpr(context, item->bin.left, values,
                                        capacity, count))
            {
                return 0;
            }
            item = item->bin.right;
        }
        return item == NULL ||
               HlslFlattenDefaultExpr(context, item, values, capacity,
                                      count);
    }
    if (source->common.kind == UNARY_N) {
        if (source->un.op == VECTOR_V_OP)
            return HlslFlattenDefaultExpr(context, source->un.arg, values,
                                          capacity, count);
        if (source->un.op == CAST_CS_OP || source->un.op == CAST_CV_OP ||
            source->un.op == CAST_CM_OP)
        {
            start = *count;
            if (!HlslFlattenDefaultExpr(context, source->un.arg, values,
                                        capacity, count))
            {
                return 0;
            }
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
                    return HlslLowerFailure(context, HLSL_ERROR_INVALID_IR,
                                            "uniform default cast shape",
                                            NULL);
                }
                len *= SUBOP_GET_S2(source->un.subop);
            }
            if (len <= 0 || componentCount != len)
                return HlslLowerFailure(context, HLSL_ERROR_INVALID_IR,
                                        "uniform default cast shape", NULL);
            targetBase = SUBOP_GET_T2(source->un.subop);
            for (i = start; i < *count; i++) {
                if (!HlslConvertDefault(context, &values[i], targetBase))
                    return 0;
            }
            return 1;
        }
    }
    if (source->common.kind == CONST_N) {
        base = GetBase(source->common.type);
        len = SUBOP_GET_S1(source->co.subop);
        if (len == 0)
            len = 1;
        if (len < 1 || len > 4)
            return HlslLowerFailure(context, HLSL_ERROR_INVALID_IR,
                                    "uniform default initializer", NULL);
        for (i = 0; i < len; i++) {
            switch (source->co.op) {
            case FCONST_OP:
            case FCONST_V_OP:
            case HCONST_OP:
            case HCONST_V_OP:
            case XCONST_OP:
            case XCONST_V_OP:
            case ICONST_OP:
            case ICONST_V_OP:
            case BCONST_OP:
            case BCONST_V_OP:
                if (!HlslAppendTypedDefault(context, values, capacity,
                                            count, base,
                                            &source->co.val[i]))
                {
                    return 0;
                }
                break;
            default:
                return HlslLowerFailure(context, HLSL_ERROR_INVALID_IR,
                                        "uniform default initializer",
                                        NULL);
            }
        }
        return 1;
    }
    if (source->common.kind == SYMB_N && source->sym.symbol != NULL &&
        GetBase(source->common.type) == TYPE_BASE_BOOLEAN)
    {
        scalar_constant value;
        const char *name;

        name = GetAtomString(atable, source->sym.symbol->name);
        memset(&value, 0, sizeof(value));
        if (name != NULL && (!strcmp(name, "true") ||
                             !strcmp(name, "false")))
        {
            value.value.i = !strcmp(name, "true");
            return HlslAppendTypedDefault(context, values, capacity, count,
                                          TYPE_BASE_BOOLEAN, &value);
        }
    }
    return HlslLowerFailure(context, HLSL_ERROR_INVALID_IR,
                            "uniform default expression shape", NULL);
} // HlslFlattenDefaultExpr

static int HlslDefaultComponentCount(const HlslType *type)
{
    const HlslDecl *member;
    int count;
    int memberCount;

    if (type == NULL)
        return -1;
    if (type->arraySize > 0) {
        count = HlslDefaultComponentCount(type->elementType);
        return count > 0 && count <= INT_MAX / type->arraySize ?
               count * type->arraySize : -1;
    }
    if (type->base == HLSL_BASE_STRUCT) {
        count = 0;
        for (member = type->members; member != NULL; member = member->next) {
            memberCount = HlslDefaultComponentCount(&member->type);
            if (memberCount < 0 || count > INT_MAX - memberCount)
                return -1;
            count += memberCount;
        }
        return count;
    }
    if (type->rows > 0 && type->cols > 0)
        return type->rows <= INT_MAX / type->cols ?
               type->rows * type->cols : -1;
    return type->len;
} // HlslDefaultComponentCount

static int HlslDefaultTargetBase(HlslBase base)
{
    switch (base) {
    case HLSL_BASE_FLOAT: return TYPE_BASE_FLOAT;
    case HLSL_BASE_INT: return TYPE_BASE_INT;
    case HLSL_BASE_BOOL: return TYPE_BASE_BOOLEAN;
    default: return TYPE_BASE_NO_TYPE;
    }
} // HlslDefaultTargetBase

static int HlslStoreDefaultType(HlslLowerContext *context,
                                const HlslType *type,
                                HlslDefaultValue *typedValues,
                                int capacity, int *index,
                                HlslDefaultLiteral *values)
{
    const HlslDecl *member;
    HlslDefaultClass targetClass;
    int len;
    int targetBase;
    int i;

    if (type == NULL)
        return 0;
    if (type->arraySize > 0) {
        for (i = 0; i < type->arraySize; i++) {
            if (!HlslStoreDefaultType(context, type->elementType,
                                      typedValues, capacity, index, values))
            {
                return 0;
            }
        }
        return 1;
    }
    if (type->base == HLSL_BASE_STRUCT) {
        for (member = type->members; member != NULL; member = member->next) {
            if (!HlslStoreDefaultType(context, &member->type, typedValues,
                                      capacity, index, values))
            {
                return 0;
            }
        }
        return 1;
    }
    len = type->rows > 0 && type->cols > 0 ?
          type->rows * type->cols : type->len;
    targetBase = HlslDefaultTargetBase(type->base);
    targetClass = HlslDefaultClassOf(targetBase);
    if (targetBase == TYPE_BASE_NO_TYPE || len <= 0 || *index < 0 ||
        *index > capacity - len)
    {
        return HlslLowerFailure(context, HLSL_ERROR_UNSUPPORTED_TYPE,
                                "uniform default destination", NULL);
    }
    for (i = 0; i < len; i++) {
        if (!HlslConvertDefault(context, &typedValues[*index], targetBase))
            return 0;
        values[*index].base = type->base;
        if (targetClass == HLSL_DEFAULT_FLOAT) {
            values[*index].value.floating =
                (float) typedValues[*index].value.value.f;
        } else if (targetClass == HLSL_DEFAULT_INT) {
            values[*index].value.integer =
                (int) typedValues[*index].value.value.i;
        } else {
            values[*index].value.boolean =
                typedValues[*index].value.value.i != 0;
        }
        (*index)++;
    }
    return 1;
} // HlslStoreDefaultType

int HlslCollectDefaults(HlslLowerContext *context)
{
    BindingList *item;
    HlslBinding *binding;
    HlslDefaultValue *typedValues;
    Symbol *symbol;
    int componentCount;
    int typedCount;
    int typedIndex;
    scalar_constant pragmaValue;

    for (item = Cg->theHAL->defaultBindings; item != NULL;
         item = item->next)
    {
        if (item->binding == NULL ||
            item->binding->none.kind != BK_DEFAULT)
        {
            continue;
        }
        for (binding = context->module->bindings; binding != NULL;
             binding = binding->next)
        {
            if (binding->storage != HLSL_STORAGE_UNIFORM ||
                binding->declaration == NULL || binding->defaultCount != 0)
            {
                continue;
            }
            symbol = (Symbol *) binding->declaration->identity;
            if (symbol == NULL || item->identity != symbol)
                continue;
            context->statementLoc = symbol->loc;
            componentCount = HlslDefaultComponentCount(&binding->type);
            if (componentCount <= 0 || item->type != symbol->type ||
                (size_t) componentCount >
                    (size_t) -1 / sizeof(HlslDefaultValue) ||
                (size_t) componentCount > (size_t) -1 / sizeof(float))
            {
                return HlslLowerFailure(context, HLSL_ERROR_INVALID_IR,
                                        "uniform default initializer",
                                        &symbol->loc);
            }
            typedValues = (HlslDefaultValue *) HlslLowerAlloc(context,
                (size_t) componentCount * sizeof(HlslDefaultValue));
            binding->defaultLiterals =
                (HlslDefaultLiteral *) HlslLowerAlloc(context,
                    (size_t) componentCount * sizeof(HlslDefaultLiteral));
            if (typedValues == NULL || binding->defaultLiterals == NULL)
                return 0;
            typedCount = 0;
            if (item->initializer != NULL) {
                if (!HlslFlattenDefaultExpr(context,
                        (const expr *) item->initializer, typedValues,
                        componentCount, &typedCount))
                {
                    return 0;
                }
            } else {
                if (item->binding->constdef.size != componentCount)
                    return HlslLowerFailure(context, HLSL_ERROR_INVALID_IR,
                        "uniform default component count", &symbol->loc);
                for (typedIndex = 0; typedIndex < componentCount;
                     typedIndex++)
                {
                    pragmaValue.kind = CG_SCALAR_FLOAT;
                    pragmaValue.value.f =
                        item->binding->constdef.val[typedIndex];
                    if (!HlslAppendTypedDefault(context, typedValues,
                            componentCount, &typedCount, TYPE_BASE_FLOAT,
                            &pragmaValue))
                    {
                        return 0;
                    }
                }
            }
            if (typedCount != componentCount)
                return HlslLowerFailure(context, HLSL_ERROR_INVALID_IR,
                    "uniform default component count", &symbol->loc);
            typedIndex = 0;
            if (!HlslStoreDefaultType(context, &binding->type, typedValues,
                                      componentCount, &typedIndex,
                                      binding->defaultLiterals))
            {
                return 0;
            }
            if (typedIndex != componentCount)
                return HlslLowerFailure(context, HLSL_ERROR_INVALID_IR,
                    "uniform default component count", &symbol->loc);
            binding->defaultCount = componentCount;
            break;
        }
    }
    return 1;
} // HlslCollectDefaults

static int HlslSemanticAtomMatches(int atom, const char *expectedRoot,
                                   int expectedIndex)
{
    const char *source;
    char upper[64];
    char root[64];
    size_t i;
    int index;

    source = atom != 0 ? GetAtomString(atable, atom) : NULL;
    if (source == NULL)
        return 0;
    for (i = 0; source[i] != '\0'; i++) {
        if (i + 1 >= sizeof(upper))
            return 0;
        upper[i] = source[i] >= 'a' && source[i] <= 'z' ?
                   (char) (source[i] - 'a' + 'A') : source[i];
    }
    upper[i] = '\0';
    return HlslParseSemantic(upper, root, sizeof(root), &index) &&
           !strcmp(root, expectedRoot) && index == expectedIndex;
} // HlslSemanticAtomMatches

static int HlslSemanticAtomIsPrimitiveIdentity(int atom)
{
    return HlslSemanticAtomMatches(atom, "INSTANCEID", 0) ||
           HlslSemanticAtomMatches(atom, "PRIMITIVEID", 0);
} // HlslSemanticAtomIsPrimitiveIdentity

int HlslCollectParameters(HlslLowerContext *context,
                                 Symbol *formal, int isEntry)
{
    HlslDecl *decl;
    int qualifiers;
    int isOutput;

    for (; formal != NULL; formal = formal->next) {
        qualifiers = GetQualifiers(formal->type);
        if (isEntry && context->profile->stage == HLSL_STAGE_GEOMETRY &&
            !CgIsAttribArray(formal->type) &&
            GetDomain(formal->type) != TYPE_DOMAIN_UNIFORM &&
            !(qualifiers & TYPE_QUALIFIER_OUT) &&
            !HlslSemanticAtomIsPrimitiveIdentity(
                formal->details.var.semantics))
        {
            return HlslLowerFailure(context, HLSL_ERROR_ENTRY_ABI,
                "geometry scalar input", &formal->loc);
        }
        decl = HlslNewSourceDecl(context, formal,
                                 context->function->identity);
        if (decl == NULL)
            return 0;
        if ((qualifiers & TYPE_QUALIFIER_INOUT) == TYPE_QUALIFIER_INOUT)
            decl->parameterQualifier = HLSL_PARAMETER_INOUT;
        else if (qualifiers & TYPE_QUALIFIER_OUT)
            decl->parameterQualifier = HLSL_PARAMETER_OUT;
        if (!isEntry) {
            if (GetCategory(formal->type) == TYPE_CATEGORY_SAMPLER)
                decl->storage = HLSL_STORAGE_SAMPLER;
            HlslAppendDecl(&context->function->parameters, decl);
            continue;
        }
        if (GetDomain(formal->type) == TYPE_DOMAIN_UNIFORM) {
            decl->storage = GetCategory(formal->type) ==
                            TYPE_CATEGORY_SAMPLER ?
                            HLSL_STORAGE_SAMPLER : HLSL_STORAGE_UNIFORM;
            if (!HlslCollectUniform(context, formal))
                return 0;
        } else {
            isOutput = decl->parameterQualifier != HLSL_PARAMETER_IN;
            decl->storage = isOutput ? HLSL_STORAGE_OUTPUT :
                                       HLSL_STORAGE_INPUT;
            if (decl->type.base == HLSL_BASE_STRUCT) {
                decl->semantic = NULL;
            } else {
                decl->semantic = HlslSourceSemantic(context, formal,
                                                     isOutput);
            }
            if (decl->type.base != HLSL_BASE_STRUCT &&
                decl->semantic == NULL &&
                !(context->profile->stage == HLSL_STAGE_GEOMETRY &&
                  CgIsAttribArray(formal->type)))
                return HlslLowerFailure(context, HLSL_ERROR_SEMANTIC,
                    GetAtomString(atable, formal->name), &formal->loc);
            if (decl->parameterQualifier == HLSL_PARAMETER_INOUT) {
                decl->inputSemantic = HlslSourceSemantic(context, formal, 0);
                if (decl->inputSemantic == NULL)
                    return HlslLowerFailure(context, HLSL_ERROR_SEMANTIC,
                        GetAtomString(atable, formal->name), &formal->loc);
            }
        }
        HlslAppendDecl(&context->function->parameters, decl);
    }
    return 1;
} // HlslCollectParameters

int HlslCollectLocals(HlslLowerContext *context, Symbol *symbol)
{
    HlslDecl *decl;
    const char *name;

    if (symbol == NULL)
        return 1;
    if (!HlslCollectLocals(context, symbol->left))
        return 0;
    if (symbol->kind == VARIABLE_S &&
        HlslFindDecl(context, symbol) == NULL)
    {
        name = GetAtomString(atable, symbol->name);
        if (name == NULL)
            return 0;
        decl = HlslNewSourceDecl(context, symbol,
                                 context->function->identity);
        if (decl == NULL)
            return 0;
        HlslInsertDecl(&context->function->locals, decl);
    }
    return HlslCollectLocals(context, symbol->right);
} // HlslCollectLocals

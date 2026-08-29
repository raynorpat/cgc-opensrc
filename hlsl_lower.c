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
#include "hlsl_hal.h"

typedef struct HlslLowerContext_Rec {
    HlslModule *module;
    const HlslProfileDesc *profile;
    Scope *scope;
    HlslFunction *function;
    Symbol *collectingHelper;
    SourceLoc statementLoc;
    int entryFile;
    int loopDepth;
} HlslLowerContext;

static int HlslEnsureType(HlslLowerContext *context, Type *type);
static HlslExpr *HlslLowerExpr(HlslLowerContext *context, expr *source,
                               HlslStmt **prefix, int valueRequired);

static void HlslSetLoc(HlslLoc *target, const SourceLoc *source)
{
    if (target != NULL && source != NULL) {
        target->file = source->file;
        target->line = source->line;
    }
} // HlslSetLoc

static HlslExpr *HlslNewSourceExpr(HlslLowerContext *context,
                                   HlslExprKind kind, HlslType type)
{
    HlslLoc loc;

    loc.file = 0;
    loc.line = 0;
    if (context != NULL)
        HlslSetLoc(&loc, &context->statementLoc);
    return context != NULL ?
           HlslNewLocatedExpr(context->module, kind, type, &loc) : NULL;
} // HlslNewSourceExpr

static int HlslLowerFailure(HlslLowerContext *context, HlslErrorKind kind,
                            const char *reason, const SourceLoc *loc)
{
    HlslModule *module;

    module = context != NULL ? context->module : NULL;
    if (module != NULL && module->errors == 0) {
        module->errorKind = kind;
        module->errorReason = reason;
        HlslSetLoc(&module->errorLoc, loc != NULL ? loc :
                   &context->statementLoc);
    }
    if (module != NULL)
        module->errors++;
    return 0;
} // HlslLowerFailure

static void *HlslLowerAlloc(HlslLowerContext *context, size_t size)
{
    void *memory;

    if (context == NULL || context->module == NULL ||
        context->module->alloc == NULL)
    {
        return NULL;
    }
    memory = (*context->module->alloc)(context->module->allocArg, size);
    if (memory != NULL)
        memset(memory, 0, size);
    return memory;
} // HlslLowerAlloc

static char *HlslGeneratedSource(HlslLowerContext *context,
                                 const char *source)
{
    char *name;
    size_t length;

    if (source == NULL)
        return NULL;
    length = strlen(source);
    name = (char *) HlslLowerAlloc(context, length + 4);
    if (name == NULL)
        return NULL;
    memcpy(name, "cg_", 3);
    memcpy(name + 3, source, length + 1);
    return name;
} // HlslGeneratedSource

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

static HlslDecl *HlslFindDeclList(HlslDecl *list, const void *identity)
{
    HlslDecl *decl;

    for (decl = list; decl != NULL; decl = decl->next) {
        if (decl->identity == identity)
            return decl;
    }
    return NULL;
} // HlslFindDeclList

static HlslDecl *HlslFindDecl(HlslLowerContext *context,
                              const void *identity)
{
    HlslDecl *decl;
    HlslBinding *binding;

    if (context->function != NULL) {
        decl = HlslFindDeclList(context->function->parameters, identity);
        if (decl == NULL)
            decl = HlslFindDeclList(context->function->locals, identity);
        if (decl != NULL)
            return decl;
    }
    decl = HlslFindDeclList(context->module->globals, identity);
    if (decl != NULL)
        return decl;
    for (decl = context->module->structs; decl != NULL; decl = decl->next) {
        HlslDecl *member;

        member = HlslFindDeclList(decl->members, identity);
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
} // HlslFindDecl

static HlslFunction *HlslFindFunction(HlslModule *module,
                                      const void *identity)
{
    HlslFunction *function;

    for (function = module->functions; function != NULL;
         function = function->next)
    {
        if (function->identity == identity)
            return function;
    }
    return NULL;
} // HlslFindFunction

static int HlslLowerType(HlslLowerContext *context, Type *source,
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
            return HlslLowerFailure(context, HLSL_ERROR_UNSUPPORTED_TYPE,
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

static void HlslInsertDecl(HlslDecl **list, HlslDecl *decl)
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

static const char *HlslFunctionSemantic(HlslLowerContext *context,
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

static HlslDecl *HlslNewSourceDecl(HlslLowerContext *context,
    Symbol *symbol, const void *nameSpace)
{
    HlslDecl *decl;
    HlslType type;
    const char *sourceName;
    const char *name;
    char *generatedName;

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
        HlslInsertDecl(members, decl);
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

static int HlslEnsureType(HlslLowerContext *context, Type *type)
{
    Type *canonical;
    HlslDecl *decl;
    HlslType structType;
    Symbol *tag;
    char *generatedName;
    const char *sourceName;
    const char *name;

    if (type == NULL)
        return 0;
    if (IsVoid(type) || IsMatrix(type, NULL, NULL) ||
        IsVector(type, NULL) ||
        GetCategory(type) == TYPE_CATEGORY_SCALAR ||
        GetCategory(type) == TYPE_CATEGORY_SAMPLER)
    {
        return 1;
    }
    if (GetCategory(type) == TYPE_CATEGORY_ARRAY)
        return type->arr.numels > 0 &&
               HlslEnsureType(context, type->arr.eltype);
    canonical = HlslCanonicalStructType(type);
    if (canonical == NULL)
        return 0;
    if (HlslFindStruct(context, canonical) != NULL)
        return 1;
    tag = HlslFindTag(context, canonical);
    if (tag == NULL)
        return 0;
    sourceName = GetAtomString(atable, canonical->str.tag);
    if (sourceName == NULL || sourceName[0] == '$')
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

static int HlslSortStructs(HlslLowerContext *context)
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
    if (!strcmp(root, "C"))
        *bank = HLSL_REGISTER_C;
    else if (!strcmp(root, "I"))
        *bank = HLSL_REGISTER_I;
    else if (!strcmp(root, "B"))
        *bank = HLSL_REGISTER_B;
    else if (!strcmp(root, "S") || !strcmp(root, "TEXUNIT"))
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

static int HlslCollectUniform(HlslLowerContext *context, Symbol *symbol)
{
    HlslBinding *binding;
    HlslDecl *identityDecl;
    HlslType type;
    HlslStorage storage;
    HlslRegisterBank bank;
    char *generatedName;
    const char *sourceName;
    const char *semantic;
    int regno;

    if (symbol == NULL || symbol->kind != VARIABLE_S ||
        GetDomain(symbol->type) != TYPE_DOMAIN_UNIFORM)
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
    binding = HlslNewBinding(context->module, storage, type,
                             sourceName, semantic);
    generatedName = HlslGeneratedSource(context, sourceName);
    identityDecl = generatedName != NULL ?
        HlslNewDecl(context->module, storage, type, generatedName) : NULL;
    if (binding == NULL || identityDecl == NULL)
        return 0;
    identityDecl->identity = symbol;
    binding->declaration = identityDecl;
    binding->publicName = sourceName;
    binding->sourceOrdinal = symbol->sourceOrdinal;
    HlslSetLoc(&binding->loc, &symbol->loc);
    if (HlslParseRegisterSemantic(semantic, &bank, &regno)) {
        binding->hasExplicitRegister = 1;
        binding->physical.bank = bank;
        binding->physical.regno = regno;
    }
    HlslAppendBinding(&context->module->bindings, binding);
    return 1;
} // HlslCollectUniform

static int HlslCollectUniformList(HlslLowerContext *context,
                                  SymbolList *list)
{
    for (; list != NULL; list = list->next) {
        if (!HlslCollectUniform(context, list->symb))
            return 0;
    }
    return 1;
} // HlslCollectUniformList

static int HlslCollectUniformTree(HlslLowerContext *context, Symbol *symbol)
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

static int HlslCollectParameters(HlslLowerContext *context,
                                 Symbol *formal, int isEntry)
{
    HlslDecl *decl;
    int qualifiers;
    int isOutput;

    for (; formal != NULL; formal = formal->next) {
        decl = HlslNewSourceDecl(context, formal,
                                 context->function->identity);
        if (decl == NULL)
            return 0;
        qualifiers = GetQualifiers(formal->type);
        if ((qualifiers & TYPE_QUALIFIER_INOUT) == TYPE_QUALIFIER_INOUT)
            decl->parameterQualifier = HLSL_PARAMETER_INOUT;
        else if (qualifiers & TYPE_QUALIFIER_OUT)
            decl->parameterQualifier = HLSL_PARAMETER_OUT;
        if (!isEntry) {
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
            decl->semantic = HlslSourceSemantic(context, formal, isOutput);
            if (decl->semantic == NULL)
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

static int HlslCollectLocals(HlslLowerContext *context, Symbol *symbol)
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

static HlslExpr *HlslNewLiteral(HlslLowerContext *context, HlslBase base,
    int intValue, float floatValue)
{
    HlslExprKind kind;
    HlslExpr *target;
    HlslType type;

    type = HlslNumericType(base, 1);
    kind = base == HLSL_BASE_FLOAT ? HLSL_EXPR_FLOAT :
           base == HLSL_BASE_BOOL ? HLSL_EXPR_BOOL : HLSL_EXPR_INT;
    target = HlslNewSourceExpr(context, kind, type);
    if (target != NULL) {
        if (kind == HLSL_EXPR_FLOAT)
            target->u.literalFloat = floatValue;
        else if (kind == HLSL_EXPR_BOOL)
            target->u.literalBool = intValue != 0;
        else
            target->u.literalInt = intValue;
    }
    return target;
} // HlslNewLiteral

static HlslExpr *HlslLowerConstant(HlslLowerContext *context, expr *source,
                                   const HlslType *type)
{
    HlslExpr *target;
    HlslExpr *item;
    int i;

    if (type->base == HLSL_BASE_FLOAT) {
        for (i = 0; i < type->len; i++) {
            if (source->co.val[i].value.f != source->co.val[i].value.f ||
                source->co.val[i].value.f > FLT_MAX ||
                source->co.val[i].value.f < -FLT_MAX)
            {
                HlslLowerFailure(context, HLSL_ERROR_UNSUPPORTED_OPERATION,
                                 "non-finite constant", NULL);
                return NULL;
            }
        }
    }
    if (type->len == 1) {
        if (type->base == HLSL_BASE_FLOAT)
            return HlslNewLiteral(context, type->base, 0,
                                  source->co.val[0].value.f);
        return HlslNewLiteral(context, type->base,
                              (int) source->co.val[0].value.i, 0.0f);
    }
    target = HlslNewSourceExpr(context, HLSL_EXPR_CONSTRUCT, *type);
    if (target == NULL)
        return NULL;
    for (i = 0; i < type->len; i++) {
        if (type->base == HLSL_BASE_FLOAT)
            item = HlslNewLiteral(context, type->base, 0,
                                  source->co.val[i].value.f);
        else
            item = HlslNewLiteral(context, type->base,
                                  (int) source->co.val[i].value.i, 0.0f);
        if (item == NULL)
            return NULL;
        HlslAppendExpr(&target->u.construct.arguments, item);
    }
    return target;
} // HlslLowerConstant

static HlslOperator HlslBinaryOperator(opcode op)
{
    switch (op) {
    case ASSIGN_OP:
    case ASSIGN_V_OP:
    case ASSIGN_GEN_OP:
    case ASSIGN_DYN_OP: return HLSL_OP_ASSIGN;
    case ASSIGNPLUS_OP: return HLSL_OP_ADD_ASSIGN;
    case ASSIGNMINUS_OP: return HLSL_OP_SUBTRACT_ASSIGN;
    case ASSIGNSTAR_OP: return HLSL_OP_MULTIPLY_ASSIGN;
    case ASSIGNSLASH_OP: return HLSL_OP_DIVIDE_ASSIGN;
    case ASSIGNMOD_OP: return HLSL_OP_REMAINDER_ASSIGN;
    case BOR_OP: case BOR_V_OP: case BOR_SV_OP: case BOR_VS_OP:
        return HLSL_OP_LOGICAL_OR;
    case BAND_OP: case BAND_V_OP: case BAND_SV_OP: case BAND_VS_OP:
        return HLSL_OP_LOGICAL_AND;
    case OR_OP: case OR_V_OP: case OR_SV_OP: case OR_VS_OP:
        return HLSL_OP_BITWISE_OR;
    case XOR_OP: case XOR_V_OP: case XOR_SV_OP: case XOR_VS_OP:
        return HLSL_OP_BITWISE_XOR;
    case AND_OP: case AND_V_OP: case AND_SV_OP: case AND_VS_OP:
        return HLSL_OP_BITWISE_AND;
    case EQ_OP: case EQ_V_OP: case EQ_SV_OP: case EQ_VS_OP:
        return HLSL_OP_EQUAL;
    case NE_OP: case NE_V_OP: case NE_SV_OP: case NE_VS_OP:
        return HLSL_OP_NOT_EQUAL;
    case LT_OP: case LT_V_OP: case LT_SV_OP: case LT_VS_OP:
        return HLSL_OP_LESS;
    case GT_OP: case GT_V_OP: case GT_SV_OP: case GT_VS_OP:
        return HLSL_OP_GREATER;
    case LE_OP: case LE_V_OP: case LE_SV_OP: case LE_VS_OP:
        return HLSL_OP_LESS_EQUAL;
    case GE_OP: case GE_V_OP: case GE_SV_OP: case GE_VS_OP:
        return HLSL_OP_GREATER_EQUAL;
    case SHL_OP: case SHL_V_OP: return HLSL_OP_SHIFT_LEFT;
    case SHR_OP: case SHR_V_OP: return HLSL_OP_SHIFT_RIGHT;
    case ADD_OP: case ADD_V_OP: case ADD_SV_OP: case ADD_VS_OP:
        return HLSL_OP_ADD;
    case SUB_OP: case SUB_V_OP: case SUB_SV_OP: case SUB_VS_OP:
        return HLSL_OP_SUBTRACT;
    case MUL_OP: case MUL_V_OP: case MUL_SV_OP: case MUL_VS_OP:
        return HLSL_OP_MULTIPLY;
    case DIV_OP: case DIV_V_OP: case DIV_SV_OP: case DIV_VS_OP:
        return HLSL_OP_DIVIDE;
    case MOD_OP: case MOD_V_OP: case MOD_SV_OP: case MOD_VS_OP:
        return HLSL_OP_REMAINDER;
    default: return HLSL_OP_NONE;
    }
} // HlslBinaryOperator

static HlslOperator HlslUnaryOperator(opcode op)
{
    switch (op) {
    case NEG_OP: case NEG_V_OP: return HLSL_OP_NEGATE;
    case POS_OP: case POS_V_OP: return HLSL_OP_POSITIVE;
    case BNOT_OP: case BNOT_V_OP: return HLSL_OP_LOGICAL_NOT;
    case NOT_OP: case NOT_V_OP: return HLSL_OP_BITWISE_NOT;
    case PREINC_OP: return HLSL_OP_PRE_INCREMENT;
    case PREDEC_OP: return HLSL_OP_PRE_DECREMENT;
    case POSTINC_OP: return HLSL_OP_POST_INCREMENT;
    case POSTDEC_OP: return HLSL_OP_POST_DECREMENT;
    default: return HLSL_OP_NONE;
    }
} // HlslUnaryOperator

static int HlslIsComparison(HlslOperator op)
{
    return op == HLSL_OP_EQUAL || op == HLSL_OP_NOT_EQUAL ||
           op == HLSL_OP_LESS || op == HLSL_OP_GREATER ||
           op == HLSL_OP_LESS_EQUAL || op == HLSL_OP_GREATER_EQUAL;
} // HlslIsComparison

static HlslExpr *HlslNewSymbolExpr(HlslLowerContext *context,
                                   HlslDecl *decl)
{
    HlslExpr *expression;

    expression = HlslNewSourceExpr(context, HLSL_EXPR_SYMBOL, decl->type);
    if (expression != NULL)
        expression->u.symbol = decl;
    return expression;
} // HlslNewSymbolExpr

static HlslDecl *HlslNewTemporary(HlslLowerContext *context,
                                  const HlslType *type)
{
    HlslDecl *decl;
    const char *name;
    char source[32];

    if (context == NULL || context->function == NULL || type == NULL)
        return NULL;
    context->module->temporaryCount++;
    sprintf(source, "temp%d", context->module->temporaryCount);
    name = HlslAllocateDistinctName(context->module, source);
    if (name == NULL)
        return NULL;
    decl = HlslNewDecl(context->module, HLSL_STORAGE_NONE, *type, name);
    if (decl == NULL)
        return NULL;
    decl->publicName = name;
    HlslSetLoc(&decl->loc, &context->statementLoc);
    HlslAppendDecl(&context->function->locals, decl);
    return decl;
} // HlslNewTemporary

static HlslStmt *HlslNewExpressionStmt(HlslLowerContext *context,
                                       HlslExpr *expression)
{
    HlslStmt *statement;

    if (expression == NULL)
        return NULL;
    statement = HlslNewStmt(context->module, HLSL_STMT_EXPRESSION);
    if (statement != NULL) {
        statement->u.expression = expression;
        HlslSetLoc(&statement->loc, &context->statementLoc);
    }
    return statement;
} // HlslNewExpressionStmt

static HlslExpr *HlslNewAssignment(HlslLowerContext *context,
                                   HlslExpr *left, HlslExpr *right)
{
    HlslExpr *assignment;

    if (left == NULL || right == NULL)
        return NULL;
    assignment = HlslNewSourceExpr(context, HLSL_EXPR_BINARY, left->type);
    if (assignment != NULL) {
        assignment->u.binary.op = HLSL_OP_ASSIGN;
        assignment->u.binary.left = left;
        assignment->u.binary.right = right;
        assignment->hasSideEffects = 1;
    }
    return assignment;
} // HlslNewAssignment

static int HlslAppendExpression(HlslLowerContext *context, HlslStmt **list,
                                HlslExpr *expression)
{
    HlslStmt *statement;

    statement = HlslNewExpressionStmt(context, expression);
    if (statement == NULL)
        return 0;
    HlslAppendStmt(list, statement);
    return 1;
} // HlslAppendExpression

static HlslExpr *HlslCaptureValue(HlslLowerContext *context,
                                  HlslStmt **list, HlslExpr *value)
{
    HlslDecl *temporary;
    HlslExpr *left;
    HlslExpr *result;
    HlslExpr *assignment;

    temporary = HlslNewTemporary(context, &value->type);
    left = temporary != NULL ? HlslNewSymbolExpr(context, temporary) : NULL;
    result = temporary != NULL ? HlslNewSymbolExpr(context, temporary) : NULL;
    assignment = HlslNewAssignment(context, left, value);
    if (assignment == NULL || result == NULL ||
        !HlslAppendExpression(context, list, assignment))
    {
        return NULL;
    }
    return result;
} // HlslCaptureValue

static char *HlslCopyText(HlslLowerContext *context, const char *text)
{
    char *copy;
    size_t length;

    length = strlen(text) + 1;
    copy = (char *) HlslLowerAlloc(context, length);
    if (copy != NULL)
        memcpy(copy, text, length);
    return copy;
} // HlslCopyText

static HlslExpr *HlslNewSwizzle(HlslLowerContext *context,
                                HlslExpr *object, const HlslType *type,
                                const char *mask)
{
    HlslExpr *target;

    target = HlslNewSourceExpr(context, HLSL_EXPR_SWIZZLE, *type);
    if (target != NULL) {
        target->u.swizzle.object = object;
        target->u.swizzle.mask = HlslCopyText(context, mask);
        if (target->u.swizzle.mask == NULL)
            return NULL;
    }
    return target;
} // HlslNewSwizzle

static HlslExpr *HlslComponent(HlslLowerContext *context,
                               HlslExpr *object, int component,
                               HlslBase base)
{
    HlslType type;
    char mask[2];

    if (object->type.len <= 1)
        return object;
    type = HlslNumericType(base, 1);
    mask[0] = "xyzw"[component];
    mask[1] = '\0';
    return HlslNewSwizzle(context, object, &type, mask);
} // HlslComponent

static HlslExpr *HlslLowerSwizzle(HlslLowerContext *context, expr *source,
                                  const HlslType *type, HlslStmt **prefix)
{
    HlslExpr *object;
    HlslExpr *target;
    char maskText[5];
    int count;
    int mask;
    int i;

    object = HlslLowerExpr(context, source->un.arg, prefix, 1);
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
        target = HlslNewSourceExpr(context, HLSL_EXPR_CONSTRUCT, *type);
        if (target != NULL)
            target->u.construct.arguments = object;
        return target;
    }
    return HlslNewSwizzle(context, object, type, maskText);
} // HlslLowerSwizzle

static HlslExpr *HlslLowerExprList(HlslLowerContext *context, expr *source,
                                   opcode listOp, HlslStmt **prefix)
{
    HlslExpr *list;
    HlslExpr *item;

    list = NULL;
    for (; source != NULL; source = source->bin.right) {
        if (source->common.kind != BINARY_N || source->bin.op != listOp)
            return NULL;
        item = HlslLowerExpr(context, source->bin.left, prefix, 1);
        if (item == NULL)
            return NULL;
        HlslAppendExpr(&list, item);
    }
    return list;
} // HlslLowerExprList

static HlslExpr *HlslLowerCall(HlslLowerContext *context, expr *source,
                               const HlslType *type, HlslStmt **prefix)
{
    HlslExpr *target;
    HlslFunction *function;
    Symbol *symbol;

    if (source->bin.left == NULL ||
        source->bin.left->common.kind != SYMB_N)
    {
        return NULL;
    }
    symbol = source->bin.left->sym.symbol;
    function = HlslFindFunction(context->module, symbol);
    if (function == NULL)
        return NULL;
    target = HlslNewSourceExpr(context, HLSL_EXPR_CALL, *type);
    if (target == NULL)
        return NULL;
    target->u.call.function = function;
    target->u.call.name = function->name;
    target->u.call.arguments = HlslLowerExprList(context,
        source->bin.right, FUN_ARG_OP, prefix);
    if (source->bin.right != NULL && target->u.call.arguments == NULL)
        return NULL;
    target->hasSideEffects = source->common.HasSideEffects;
    return target;
} // HlslLowerCall

static HlslExpr *HlslLowerConditional(HlslLowerContext *context,
                                      expr *source, const HlslType *type,
                                      HlslStmt **prefix)
{
    HlslExpr *target;
    HlslExpr *condition;
    HlslExpr *trueExpr;
    HlslExpr *falseExpr;
    HlslExpr *componentExpr;
    HlslExpr *left;
    HlslExpr *right;
    HlslDecl *temporary;
    HlslStmt *truePrefix;
    HlslStmt *falsePrefix;
    HlslStmt *ifStatement;
    HlslType componentType;
    int conditionLen;
    int i;

    conditionLen = 0;
    IsVector(source->tri.arg1->common.type, &conditionLen);
    condition = HlslLowerExpr(context, source->tri.arg1, prefix, 1);
    if (condition == NULL)
        return NULL;
    if (conditionLen <= 1 &&
        !source->tri.arg2->common.HasSideEffects &&
        !source->tri.arg3->common.HasSideEffects)
    {
        trueExpr = HlslLowerExpr(context, source->tri.arg2, prefix, 1);
        falseExpr = HlslLowerExpr(context, source->tri.arg3, prefix, 1);
        target = HlslNewSourceExpr(context, HLSL_EXPR_CONDITIONAL, *type);
        if (target == NULL || trueExpr == NULL || falseExpr == NULL)
            return NULL;
        target->u.conditional.condition = condition;
        target->u.conditional.trueExpr = trueExpr;
        target->u.conditional.falseExpr = falseExpr;
        return target;
    }
    if (conditionLen <= 1) {
        temporary = HlslNewTemporary(context, type);
        if (temporary == NULL)
            return NULL;
        truePrefix = NULL;
        falsePrefix = NULL;
        trueExpr = HlslLowerExpr(context, source->tri.arg2,
                                 &truePrefix, 1);
        falseExpr = HlslLowerExpr(context, source->tri.arg3,
                                  &falsePrefix, 1);
        left = HlslNewSymbolExpr(context, temporary);
        right = HlslNewAssignment(context, left, trueExpr);
        if (right == NULL ||
            !HlslAppendExpression(context, &truePrefix, right))
        {
            return NULL;
        }
        left = HlslNewSymbolExpr(context, temporary);
        right = HlslNewAssignment(context, left, falseExpr);
        if (right == NULL ||
            !HlslAppendExpression(context, &falsePrefix, right))
        {
            return NULL;
        }
        ifStatement = HlslNewStmt(context->module, HLSL_STMT_IF);
        if (ifStatement == NULL)
            return NULL;
        ifStatement->u.ifStmt.condition = condition;
        ifStatement->u.ifStmt.trueBranch = truePrefix;
        ifStatement->u.ifStmt.falseBranch = falsePrefix;
        HlslSetLoc(&ifStatement->loc, &context->statementLoc);
        HlslAppendStmt(prefix, ifStatement);
        return HlslNewSymbolExpr(context, temporary);
    }
    if (conditionLen != type->len || type->len < 2 || type->len > 4)
        return NULL;
    condition = HlslCaptureValue(context, prefix, condition);
    trueExpr = HlslLowerExpr(context, source->tri.arg2, prefix, 1);
    if (trueExpr != NULL)
        trueExpr = HlslCaptureValue(context, prefix, trueExpr);
    falseExpr = HlslLowerExpr(context, source->tri.arg3, prefix, 1);
    if (falseExpr != NULL)
        falseExpr = HlslCaptureValue(context, prefix, falseExpr);
    if (condition == NULL || trueExpr == NULL || falseExpr == NULL)
        return NULL;
    target = HlslNewSourceExpr(context, HLSL_EXPR_CONSTRUCT, *type);
    if (target == NULL)
        return NULL;
    componentType = HlslNumericType(type->base, 1);
    for (i = 0; i < type->len; i++) {
        componentExpr = HlslNewSourceExpr(context,
            HLSL_EXPR_CONDITIONAL, componentType);
        if (componentExpr == NULL)
            return NULL;
        componentExpr->u.conditional.condition = HlslComponent(
            context, condition, i, HLSL_BASE_BOOL);
        componentExpr->u.conditional.trueExpr = HlslComponent(
            context, trueExpr, i, type->base);
        componentExpr->u.conditional.falseExpr = HlslComponent(
            context, falseExpr, i, type->base);
        if (componentExpr->u.conditional.condition == NULL ||
            componentExpr->u.conditional.trueExpr == NULL ||
            componentExpr->u.conditional.falseExpr == NULL)
        {
            return NULL;
        }
        HlslAppendExpr(&target->u.construct.arguments, componentExpr);
    }
    return target;
} // HlslLowerConditional

static HlslExpr *HlslLowerVectorComparison(HlslLowerContext *context,
    expr *source, const HlslType *type, HlslOperator op, HlslStmt **prefix)
{
    HlslExpr *target;
    HlslExpr *left;
    HlslExpr *right;
    HlslExpr *component;
    HlslType componentType;
    int i;

    left = HlslLowerExpr(context, source->bin.left, prefix, 1);
    right = HlslLowerExpr(context, source->bin.right, prefix, 1);
    if (left == NULL || right == NULL)
        return NULL;
    left = HlslCaptureValue(context, prefix, left);
    right = HlslCaptureValue(context, prefix, right);
    if (left == NULL || right == NULL)
        return NULL;
    target = HlslNewSourceExpr(context, HLSL_EXPR_CONSTRUCT, *type);
    if (target == NULL)
        return NULL;
    componentType = HlslNumericType(HLSL_BASE_BOOL, 1);
    for (i = 0; i < type->len; i++) {
        component = HlslNewSourceExpr(context, HLSL_EXPR_BINARY,
                                      componentType);
        if (component == NULL)
            return NULL;
        component->u.binary.op = op;
        component->u.binary.left = HlslComponent(context, left, i,
                                                  left->type.base);
        component->u.binary.right = HlslComponent(context, right, i,
                                                   right->type.base);
        if (component->u.binary.left == NULL ||
            component->u.binary.right == NULL)
        {
            return NULL;
        }
        HlslAppendExpr(&target->u.construct.arguments, component);
    }
    return target;
} // HlslLowerVectorComparison

static HlslExpr *HlslLowerExpr(HlslLowerContext *context, expr *source,
                               HlslStmt **prefix, int valueRequired)
{
    HlslExpr *target;
    HlslDecl *decl;
    HlslType type;
    HlslOperator op;
    Symbol *member;
    HlslExpr *left;
    HlslExpr *right;
    HlslStmt *leftStatement;

    if (source == NULL ||
        !HlslEnsureType(context, source->common.type) ||
        !HlslLowerType(context, source->common.type, &type, NULL))
    {
        return NULL;
    }
    if (source->common.kind == SYMB_N && source->sym.op == VARIABLE_OP) {
        decl = HlslFindDecl(context, source->sym.symbol);
        if (decl == NULL && context->function != NULL &&
            source->sym.symbol != NULL)
        {
            const char *sourceName;

            sourceName = GetAtomString(atable, source->sym.symbol->name);
            if (sourceName != NULL &&
                (sourceName[0] == '$' || sourceName[0] == '@'))
            {
                decl = HlslNewSourceDecl(context, source->sym.symbol,
                                         context->function->identity);
                if (decl != NULL)
                    HlslInsertDecl(&context->function->locals, decl);
            }
        }
        if (decl == NULL)
            return NULL;
        target = HlslNewSourceExpr(context, HLSL_EXPR_SYMBOL, type);
        if (target != NULL) {
            target->u.symbol = decl;
            target->hasSideEffects = source->common.HasSideEffects;
        }
        return target;
    }
    if (source->common.kind == CONST_N)
        return HlslLowerConstant(context, source, &type);
    if (source->common.kind == UNARY_N) {
        if (source->un.op == SWIZZLE_Z_OP)
            return HlslLowerSwizzle(context, source, &type, prefix);
        if (source->un.op == VECTOR_V_OP) {
            target = HlslNewSourceExpr(context, HLSL_EXPR_CONSTRUCT, type);
            if (target == NULL)
                return NULL;
            target->u.construct.arguments = HlslLowerExprList(context,
                source->un.arg, EXPR_LIST_OP, prefix);
            return target->u.construct.arguments != NULL ? target : NULL;
        }
        if (source->un.op == CAST_CS_OP ||
            source->un.op == CAST_CV_OP ||
            source->un.op == CAST_CM_OP ||
            source->un.op == CAST_SHAPE_OP)
        {
            target = HlslNewSourceExpr(context, HLSL_EXPR_CAST, type);
            if (target == NULL)
                return NULL;
            target->u.cast.expression = HlslLowerExpr(
                context, source->un.arg, prefix, 1);
            return target->u.cast.expression != NULL ? target : NULL;
        }
        op = HlslUnaryOperator(source->un.op);
        if (op != HLSL_OP_NONE) {
            target = HlslNewSourceExpr(context, HLSL_EXPR_UNARY, type);
            if (target == NULL)
                return NULL;
            target->u.unary.op = op;
            target->u.unary.operand = HlslLowerExpr(
                context, source->un.arg, prefix, 1);
            target->hasSideEffects = source->common.HasSideEffects;
            return target->u.unary.operand != NULL ? target : NULL;
        }
    }
    if (source->common.kind == BINARY_N) {
        if (source->bin.op == COMMA_OP) {
            left = HlslLowerExpr(context, source->bin.left, prefix, 0);
            if (left == NULL)
                return NULL;
            leftStatement = HlslNewExpressionStmt(context, left);
            if (leftStatement == NULL)
                return NULL;
            HlslAppendStmt(prefix, leftStatement);
            return HlslLowerExpr(context, source->bin.right, prefix,
                                 valueRequired);
        }
        if (source->bin.op == MEMBER_SELECTOR_OP) {
            if (source->bin.right == NULL ||
                source->bin.right->common.kind != SYMB_N ||
                source->bin.right->sym.op != MEMBER_OP)
            {
                return NULL;
            }
            member = source->bin.right->sym.symbol;
            decl = HlslFindDecl(context, member);
            if (decl == NULL)
                return NULL;
            target = HlslNewSourceExpr(context, HLSL_EXPR_MEMBER, type);
            if (target == NULL)
                return NULL;
            target->u.member.object = HlslLowerExpr(
                context, source->bin.left, prefix, 1);
            target->u.member.decl = decl;
            target->u.member.name = decl->name;
            return target->u.member.object != NULL ? target : NULL;
        }
        if (source->bin.op == ARRAY_INDEX_OP) {
            target = HlslNewSourceExpr(context, HLSL_EXPR_INDEX, type);
            if (target == NULL)
                return NULL;
            target->u.index.object = HlslLowerExpr(
                context, source->bin.left, prefix, 1);
            target->u.index.index = HlslLowerExpr(
                context, source->bin.right, prefix, 1);
            return target->u.index.object != NULL &&
                   target->u.index.index != NULL ? target : NULL;
        }
        if (source->bin.op == FUN_CALL_OP)
            return HlslLowerCall(context, source, &type, prefix);
        op = HlslBinaryOperator(source->bin.op);
        if (op != HLSL_OP_NONE) {
            if (HlslIsComparison(op) && type.base == HLSL_BASE_BOOL &&
                type.len > 1)
            {
                return HlslLowerVectorComparison(context, source, &type,
                                                  op, prefix);
            }
            left = HlslLowerExpr(context, source->bin.left, prefix, 1);
            right = HlslLowerExpr(context, source->bin.right, prefix, 1);
            if (left == NULL || right == NULL)
                return NULL;
            if (op == HLSL_OP_ASSIGN && valueRequired) {
                right = HlslCaptureValue(context, prefix, right);
                target = HlslNewAssignment(context, left, right);
                if (target == NULL ||
                    !HlslAppendExpression(context, prefix, target))
                {
                    return NULL;
                }
                return HlslNewSymbolExpr(context, right->u.symbol);
            }
            target = HlslNewSourceExpr(context, HLSL_EXPR_BINARY, type);
            if (target == NULL)
                return NULL;
            target->u.binary.op = op;
            target->u.binary.left = left;
            target->u.binary.right = right;
            target->hasSideEffects = source->common.HasSideEffects;
            return target;
        }
    }
    if (source->common.kind == TRINARY_N &&
        (source->tri.op == COND_OP || source->tri.op == COND_V_OP ||
         source->tri.op == COND_SV_OP || source->tri.op == COND_GEN_OP))
    {
        return HlslLowerConditional(context, source, &type, prefix);
    }
    HlslLowerFailure(context, HLSL_ERROR_UNSUPPORTED_OPERATION,
                     "HLSL expression", NULL);
    return NULL;
} // HlslLowerExpr

static int HlslIsSyntheticInputAssignment(expr *source)
{
    expr *right;

    if (source == NULL || source->common.kind != BINARY_N ||
        (source->bin.op != ASSIGN_OP && source->bin.op != ASSIGN_V_OP &&
         source->bin.op != ASSIGN_GEN_OP))
    {
        return 0;
    }
    right = source->bin.right;
    while (right != NULL && right->common.kind == UNARY_N &&
           (right->un.op == CAST_CS_OP || right->un.op == CAST_CV_OP ||
            right->un.op == CAST_SHAPE_OP))
    {
        right = right->un.arg;
    }
    return right != NULL && right->common.kind == BINARY_N &&
           right->bin.op == MEMBER_SELECTOR_OP &&
           right->bin.left != NULL &&
           right->bin.left->common.kind == SYMB_N &&
           right->bin.left->sym.symbol == Cg->theHAL->varyingIn;
} // HlslIsSyntheticInputAssignment

static int HlslLowerStatements(HlslLowerContext *context, stmt *source,
                               HlslStmt **list)
{
    HlslStmt *target;
    HlslStmt *discard;
    HlslExpr *condition;
    expr *discardCondition;

    for (; source != NULL; source = source->commonst.next) {
        context->statementLoc = source->commonst.loc;
        if (source->commonst.kind == COMMENT_STMT)
            continue;
        if (source->commonst.kind == EXPR_STMT &&
            HlslIsSyntheticInputAssignment(source->exprst.exp))
        {
            continue;
        }
        if (source->commonst.kind == EXPR_STMT) {
            if (source->exprst.exp == NULL)
                continue;
            target = HlslNewStmt(context->module,
                                 HLSL_STMT_EXPRESSION);
            if (target != NULL)
                target->u.expression = HlslLowerExpr(
                    context, source->exprst.exp, list, 0);
            if (target == NULL || target->u.expression == NULL)
                return 0;
        } else if (source->commonst.kind == IF_STMT) {
            target = HlslNewStmt(context->module, HLSL_STMT_IF);
            if (target == NULL)
                return 0;
            target->u.ifStmt.condition = HlslLowerExpr(
                context, source->ifst.cond, list, 1);
            if (target->u.ifStmt.condition == NULL ||
                !HlslLowerStatements(context, source->ifst.thenstmt,
                                     &target->u.ifStmt.trueBranch) ||
                !HlslLowerStatements(context, source->ifst.elsestmt,
                                     &target->u.ifStmt.falseBranch))
            {
                return 0;
            }
        } else if (source->commonst.kind == WHILE_STMT ||
                   source->commonst.kind == DO_STMT)
        {
            target = HlslNewStmt(context->module,
                source->commonst.kind == WHILE_STMT ?
                HLSL_STMT_WHILE : HLSL_STMT_DO);
            if (target == NULL)
                return 0;
            target->u.loop.condition = HlslLowerExpr(
                context, source->whilest.cond, list, 1);
            if (target->u.loop.condition == NULL)
                return 0;
            context->loopDepth++;
            if (!HlslLowerStatements(context, source->whilest.body,
                                     &target->u.loop.body))
            {
                context->loopDepth--;
                return 0;
            }
            context->loopDepth--;
        } else if (source->commonst.kind == FOR_STMT) {
            target = HlslNewStmt(context->module, HLSL_STMT_FOR);
            if (target == NULL ||
                !HlslLowerStatements(context, source->forst.init,
                                     &target->u.forStmt.init) ||
                (source->forst.cond != NULL &&
                 (target->u.forStmt.condition = HlslLowerExpr(
                    context, source->forst.cond, list, 1)) == NULL) ||
                !HlslLowerStatements(context, source->forst.step,
                                     &target->u.forStmt.step))
            {
                return 0;
            }
            context->loopDepth++;
            if (!HlslLowerStatements(context, source->forst.body,
                                     &target->u.forStmt.body))
            {
                context->loopDepth--;
                return 0;
            }
            context->loopDepth--;
        } else if (source->commonst.kind == RETURN_STMT) {
            target = HlslNewStmt(context->module, HLSL_STMT_RETURN);
            if (target != NULL && source->returnst.exp != NULL)
                target->u.returnExpr = HlslLowerExpr(
                    context, source->returnst.exp, list, 1);
            if (target == NULL || (source->returnst.exp != NULL &&
                                   target->u.returnExpr == NULL))
            {
                return 0;
            }
        } else if (source->commonst.kind == DISCARD_STMT) {
            if (context->module->stage != HLSL_STAGE_PIXEL)
                return HlslLowerFailure(context, HLSL_ERROR_STAGE_OPERATION,
                                        "discard", NULL);
            discardCondition = source->discardst.cond;
            if (discardCondition != NULL &&
                discardCondition->common.kind == UNARY_N &&
                discardCondition->un.op == KILL_OP)
            {
                discardCondition = discardCondition->un.arg;
            }
            if (discardCondition == NULL) {
                target = HlslNewStmt(context->module, HLSL_STMT_DISCARD);
            } else {
                condition = HlslLowerExpr(context, discardCondition,
                                          list, 1);
                discard = HlslNewStmt(context->module,
                                      HLSL_STMT_DISCARD);
                target = HlslNewStmt(context->module, HLSL_STMT_IF);
                if (condition == NULL || discard == NULL || target == NULL)
                    return 0;
                target->u.ifStmt.condition = condition;
                target->u.ifStmt.trueBranch = discard;
                HlslSetLoc(&discard->loc, &source->commonst.loc);
            }
        } else if (source->commonst.kind == BREAK_STMT) {
            if (context->loopDepth == 0)
                return HlslLowerFailure(context,
                    HLSL_ERROR_UNSUPPORTED_OPERATION,
                    "break outside loop", NULL);
            target = HlslNewStmt(context->module, HLSL_STMT_BREAK);
        } else if (source->commonst.kind == CONTINUE_STMT) {
            if (context->loopDepth == 0)
                return HlslLowerFailure(context,
                    HLSL_ERROR_UNSUPPORTED_OPERATION,
                    "continue outside loop", NULL);
            target = HlslNewStmt(context->module, HLSL_STMT_CONTINUE);
        } else if (source->commonst.kind == BLOCK_STMT) {
            target = HlslNewStmt(context->module, HLSL_STMT_BLOCK);
            if (target == NULL ||
                !HlslLowerStatements(context, source->blockst.body,
                                     &target->u.block))
            {
                return 0;
            }
        } else {
            return HlslLowerFailure(context,
                                    HLSL_ERROR_UNSUPPORTED_OPERATION,
                                    "HLSL statement", NULL);
        }
        HlslSetLoc(&target->loc, &source->commonst.loc);
        HlslAppendStmt(list, target);
    }
    return 1;
} // HlslLowerStatements

static int HlslCollectCallsInExpr(HlslLowerContext *context, expr *source);

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

static int HlslCollectHelper(HlslLowerContext *context, Symbol *symbol)
{
    HlslFunction *function;
    HlslType result;
    char *generated;
    const char *name;
    Symbol *caller;

    if (symbol == NULL || symbol->kind != FUNCTION_S)
        return 0;
    if (symbol->properties & SYMB_IS_BUILTIN)
        return 1;
    function = HlslFindFunction(context->module, symbol);
    if (function != NULL) {
        if (function->visitState == 1)
            return HlslLowerFailure(context,
                                    HLSL_ERROR_UNSUPPORTED_OPERATION,
                                    "recursive HLSL helper",
                                    &context->statementLoc);
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
    if (caller != NULL)
        function->needsPrototype = 1;
    HlslSetLoc(&function->loc, &symbol->loc);
    HlslAppendFunction(&context->module->functions, function);
    context->collectingHelper = symbol;
    if (!HlslCollectCallsInStatements(context,
                                      symbol->details.fun.statements))
    {
        context->collectingHelper = caller;
        return 0;
    }
    context->collectingHelper = caller;
    function->visitState = 2;
    return 1;
} // HlslCollectHelper

static int HlslCollectCallsInExpr(HlslLowerContext *context, expr *source)
{
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
        if (source->bin.op == FUN_CALL_OP && source->bin.left != NULL &&
            source->bin.left->common.kind == SYMB_N &&
            !HlslCollectHelper(context, source->bin.left->sym.symbol))
        {
            return 0;
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

static int HlslLowerFunction(HlslLowerContext *context,
                             HlslFunction *function)
{
    Symbol *symbol;

    symbol = (Symbol *) function->identity;
    context->function = function;
    context->statementLoc = symbol->loc;
    if (!HlslCollectParameters(context, symbol->details.fun.params, 0) ||
        symbol->details.fun.locals == NULL ||
        !HlslCollectLocals(context,
                           symbol->details.fun.locals->symbols) ||
        !HlslLowerStatements(context, symbol->details.fun.statements,
                             &function->body))
    {
        return 0;
    }
    return 1;
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

int HlslLowerProgram(HlslModule *module, const HlslProfileDesc *profile,
                     SourceLoc *loc, Scope *scope, Symbol *program)
{
    HlslLowerContext context;
    HlslFunction *function;
    HlslFunction *helper;
    HlslType result;
    Type *sourceResult;
    const char *name;
    int emptyEntry;

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
    context.statementLoc = program->loc;
    context.entryFile = program->loc.file;
    emptyEntry = HlslIsEmptyEntry(program);
    sourceResult = HlslOriginalEntryResult(program);
    if (!HlslEnsureType(&context, sourceResult) ||
        !HlslLowerType(&context, sourceResult, &result, &program->loc) ||
        !HlslCollectCallsInStatements(&context,
                                      program->details.fun.statements) ||
        !HlslCollectUniformList(&context, Cg->theHAL->uniformParam) ||
        !HlslCollectUniformList(&context, Cg->theHAL->uniformGlobal) ||
        !HlslCollectUniformTree(&context, scope->symbols) ||
        !HlslSortStructs(&context))
    {
        return 0;
    }
    for (helper = module->functions; helper != NULL; helper = helper->next) {
        if (!HlslLowerFunction(&context, helper))
            return 0;
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
    context.function = function;
    if (!HlslCollectParameters(&context, program->details.fun.params, 1) ||
        program->details.fun.locals == NULL ||
        !HlslCollectLocals(&context,
                           program->details.fun.locals->symbols) ||
        (!emptyEntry &&
         !HlslLowerStatements(&context, program->details.fun.statements,
                              &function->body)))
    {
        return 0;
    }
    return module->errors == 0;
} // HlslLowerProgram

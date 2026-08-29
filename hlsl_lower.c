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

typedef enum HlslValueMode_Enum {
    HLSL_VALUE_DISCARD,
    HLSL_VALUE_RVALUE,
    HLSL_VALUE_LVALUE
} HlslValueMode;

static int HlslEnsureType(HlslLowerContext *context, Type *type);
static int HlslLowerType(HlslLowerContext *context, Type *source,
                         HlslType *target, const SourceLoc *loc);
static HlslExpr *HlslLowerExpr(HlslLowerContext *context, expr *source,
                               HlslStmt **prefix, HlslValueMode valueMode);
static int HlslResolveBuiltinSymbol(HlslLowerContext *context,
                                    Symbol *symbol,
                                    const SourceLoc *callLoc,
                                    HlslBuiltin *builtin,
                                    HlslType *result,
                                    HlslType *params, int *paramCount);

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

/*
 * HlslResolveBuiltinSymbol() - Resolve only immutable catalog identities or
 *         declarations already accepted by the HLSL HAL as group 4.  Merely
 *         sharing a source spelling never turns a user helper into a builtin.
 *         Return one for an exact row, zero for an ordinary helper, and minus
 *         one after recording an intrinsic or stage failure.
 */

static int HlslResolveBuiltinSymbol(HlslLowerContext *context,
                                    Symbol *symbol,
                                    const SourceLoc *callLoc,
                                    HlslBuiltin *builtin,
                                    HlslType *result,
                                    HlslType *params, int *paramCount)
{
    const CgIntrinsicSignature *signature;
    TypeList *parameter;
    Type *sourceResult;
    const char *name;
    HlslSourceType sourceResultType;
    HlslSourceType sourceParams[HLSL_MAX_BUILTIN_ARGS];
    HlslBuiltin resolved;
    HlslBuiltin otherStage;
    int count;

    if (context == NULL || symbol == NULL || symbol->kind != FUNCTION_S ||
        symbol->type == NULL || builtin == NULL || result == NULL ||
        params == NULL || paramCount == NULL)
    {
        return 0;
    }
    signature = CgIntrinsicSignatureForSymbol(symbol);
    if (signature != NULL) {
        name = signature->name;
        sourceResult = signature->result;
        parameter = signature->parameters;
    } else if ((symbol->properties & SYMB_IS_BUILTIN) != 0 &&
               symbol->details.fun.group == HLSL_BUILTIN_GROUP &&
               symbol->details.fun.index > HLSL_BUILTIN_NONE &&
               symbol->details.fun.index < HLSL_BUILTIN_COUNT)
    {
        name = GetAtomString(atable, symbol->name);
        sourceResult = symbol->type->fun.rettype;
        parameter = symbol->type->fun.paramtypes;
    } else {
        return 0;
    }
    if (!HlslIsBuiltinName(name)) {
        HlslLowerFailure(context, HLSL_ERROR_INTRINSIC, name, callLoc);
        return -1;
    }
    if (!HlslDescribeSourceType(sourceResult, &sourceResultType)) {
        HlslLowerFailure(context,
            HlslIsTextureName(name) ? HLSL_ERROR_SAMPLER :
                                      HLSL_ERROR_INTRINSIC,
            name, callLoc);
        return -1;
    }
    count = 0;
    for (; parameter != NULL; parameter = parameter->next) {
        if (count >= HLSL_MAX_BUILTIN_ARGS ||
            !HlslDescribeSourceType(parameter->type, &sourceParams[count]))
        {
            HlslLowerFailure(context,
                HlslIsTextureName(name) ? HLSL_ERROR_SAMPLER :
                                          HLSL_ERROR_INTRINSIC,
                             name, callLoc);
            return -1;
        }
        count++;
    }
    resolved = HlslLookupSourceBuiltin(context->profile->stage, name,
        &sourceResultType, sourceParams, count);
    if (resolved == HLSL_BUILTIN_NONE) {
        otherStage = HlslLookupSourceBuiltin(
            context->profile->stage == HLSL_STAGE_VERTEX ?
            HLSL_STAGE_PIXEL : HLSL_STAGE_VERTEX,
            name, &sourceResultType, sourceParams, count);
        if (otherStage != HLSL_BUILTIN_NONE) {
            HlslLowerFailure(context,
                HlslBuiltinIsTexture(otherStage) ? HLSL_ERROR_SAMPLER :
                                                   HLSL_ERROR_STAGE_OPERATION,
                             name, callLoc);
        } else {
            HlslLowerFailure(context,
                HlslIsTextureName(name) ? HLSL_ERROR_SAMPLER :
                                          HLSL_ERROR_INTRINSIC,
                             name, callLoc);
        }
        return -1;
    }
    if (signature == NULL &&
        resolved != (HlslBuiltin) symbol->details.fun.index)
    {
        HlslLowerFailure(context, HLSL_ERROR_INTRINSIC, name, callLoc);
        return -1;
    }
    if (!HlslLowerType(context, sourceResult, result, callLoc))
        return -1;
    parameter = signature != NULL ? signature->parameters :
                symbol->type->fun.paramtypes;
    count = 0;
    for (; parameter != NULL; parameter = parameter->next) {
        if (count >= HLSL_MAX_BUILTIN_ARGS ||
            !HlslLowerType(context, parameter->type, &params[count],
                           callLoc))
        {
            if (context->module->errors == 0)
                HlslLowerFailure(context,
                    HlslIsTextureName(name) ? HLSL_ERROR_SAMPLER :
                                              HLSL_ERROR_INTRINSIC,
                                 name, callLoc);
            return -1;
        }
        count++;
    }
    if (!HlslBuiltinAccepts(context->profile->stage, resolved,
                            result, params, count))
    {
        HlslLowerFailure(context,
            HlslIsTextureName(name) ? HLSL_ERROR_SAMPLER :
                                      HLSL_ERROR_INTRINSIC,
            name, callLoc);
        return -1;
    }
    *builtin = resolved;
    *paramCount = count;
    return 1;
} // HlslResolveBuiltinSymbol

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
        decl->inputSemantic = HlslSourceSemantic(context, symbol, 0);
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

static int HlslCollectDefaults(HlslLowerContext *context)
{
    BindingList *item;
    HlslBinding *binding;
    HlslDefaultValue *typedValues;
    Symbol *symbol;
    int componentCount;
    int typedCount;
    int typedIndex;

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
            if (componentCount <= 0 || item->initializer == NULL ||
                item->type != symbol->type ||
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
            if (!HlslFlattenDefaultExpr(context,
                    (const expr *) item->initializer, typedValues,
                    componentCount, &typedCount))
            {
                return 0;
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
                decl->semantic == NULL)
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

static int HlslIsAssignmentOperator(HlslOperator op)
{
    return op == HLSL_OP_ASSIGN ||
           op == HLSL_OP_ADD_ASSIGN ||
           op == HLSL_OP_SUBTRACT_ASSIGN ||
           op == HLSL_OP_MULTIPLY_ASSIGN ||
           op == HLSL_OP_DIVIDE_ASSIGN ||
           op == HLSL_OP_REMAINDER_ASSIGN ||
           op == HLSL_OP_BITWISE_OR_ASSIGN ||
           op == HLSL_OP_BITWISE_XOR_ASSIGN ||
           op == HLSL_OP_BITWISE_AND_ASSIGN ||
           op == HLSL_OP_SHIFT_LEFT_ASSIGN ||
           op == HLSL_OP_SHIFT_RIGHT_ASSIGN;
} // HlslIsAssignmentOperator

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

static int HlslBuiltinHelperTypeEqual(const HlslType *left,
                                      const HlslType *right)
{
    return left != NULL && right != NULL &&
           left->base == right->base && left->len == right->len &&
           left->rows == right->rows && left->cols == right->cols &&
           left->arraySize == 0 && right->arraySize == 0;
} // HlslBuiltinHelperTypeEqual

static HlslFunction *HlslFindBuiltinHelper(HlslModule *module,
                                           HlslBuiltin builtin,
                                           const HlslType *type)
{
    HlslFunction *function;

    for (function = module->functions; function != NULL;
         function = function->next)
    {
        if (function->builtin == builtin &&
            HlslBuiltinHelperTypeEqual(&function->result, type))
        {
            return function;
        }
    }
    return NULL;
} // HlslFindBuiltinHelper

static HlslExpr *HlslNewBuiltinConstant(HlslLowerContext *context,
                                        const HlslType *type, float value)
{
    HlslExpr *literal;
    HlslExpr *construct;
    int i;

    if (type == NULL)
        return NULL;
    if (type->len == 1)
        return HlslNewLiteral(context, HLSL_BASE_FLOAT, 0, value);
    construct = HlslNewSourceExpr(context, HLSL_EXPR_CONSTRUCT, *type);
    if (construct == NULL)
        return NULL;
    for (i = 0; i < type->len; i++) {
        literal = HlslNewLiteral(context, HLSL_BASE_FLOAT, 0, value);
        if (literal == NULL)
            return NULL;
        HlslAppendExpr(&construct->u.construct.arguments, literal);
    }
    return construct;
} // HlslNewBuiltinConstant

static HlslFunction *HlslCreateBuiltinHelper(HlslLowerContext *context,
                                             HlslBuiltin builtin,
                                             const HlslType *type)
{
    HlslFunction *function;
    HlslDecl *parameter;
    HlslStmt *statement;
    HlslExpr *argument;
    HlslExpr *call;
    HlslExpr *inner;
    HlslExpr *zero;
    HlslExpr *one;
    const char *typeName;
    const char *name;
    const char *parameterName;
    char source[96];

    function = HlslFindBuiltinHelper(context->module, builtin, type);
    if (function != NULL)
        return function;
    typeName = HlslTypeName(type);
    if (typeName == NULL ||
        (builtin != HLSL_BUILTIN_RSQRT &&
         builtin != HLSL_BUILTIN_SATURATE))
    {
        return NULL;
    }
    sprintf(source, "cg_%s_%s", HlslBuiltinSpelling(builtin), typeName);
    function = HlslNewFunction(context->module, *type, NULL);
    if (function == NULL)
        return NULL;
    name = HlslAllocateGeneratedName(context->module, function, source);
    if (name == NULL)
        return NULL;
    function->name = name;
    function->builtin = builtin;
    function->needsPrototype = 1;
    parameterName = HlslAllocateScopedSymbolName(context->module,
        function, function, "value");
    parameter = HlslNewDecl(context->module, HLSL_STORAGE_NONE,
                            *type, parameterName);
    statement = HlslNewStmt(context->module, HLSL_STMT_RETURN);
    if (parameterName == NULL || parameter == NULL || statement == NULL)
        return NULL;
    parameter->publicName = parameterName;
    HlslAppendDecl(&function->parameters, parameter);

    argument = HlslNewSymbolExpr(context, parameter);
    if (argument == NULL)
        return NULL;
    if (builtin == HLSL_BUILTIN_RSQRT) {
        call = HlslNewSourceExpr(context, HLSL_EXPR_CALL, *type);
        if (call == NULL)
            return NULL;
        call->u.call.name = HlslBuiltinSpelling(builtin);
        call->u.call.builtin = builtin;
        call->u.call.arguments = argument;
    } else {
        zero = HlslNewBuiltinConstant(context, type, 0.0f);
        one = HlslNewBuiltinConstant(context, type, 1.0f);
        inner = HlslNewSourceExpr(context, HLSL_EXPR_CALL, *type);
        call = HlslNewSourceExpr(context, HLSL_EXPR_CALL, *type);
        if (zero == NULL || one == NULL || inner == NULL || call == NULL)
            return NULL;
        inner->u.call.name = HlslBuiltinSpelling(HLSL_BUILTIN_MAX);
        inner->u.call.builtin = HLSL_BUILTIN_MAX;
        inner->u.call.arguments = argument;
        HlslAppendExpr(&inner->u.call.arguments, zero);
        call->u.call.name = HlslBuiltinSpelling(HLSL_BUILTIN_MIN);
        call->u.call.builtin = HLSL_BUILTIN_MIN;
        call->u.call.arguments = inner;
        HlslAppendExpr(&call->u.call.arguments, one);
    }
    statement->u.returnExpr = call;
    HlslAppendStmt(&function->body, statement);
    HlslAppendFunction(&context->module->functions, function);
    return function;
} // HlslCreateBuiltinHelper

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

static int HlslStabilizeLvalueAddress(HlslLowerContext *context,
                                      HlslStmt **list, HlslExpr *value)
{
    if (value == NULL)
        return 0;
    switch (value->kind) {
    case HLSL_EXPR_SYMBOL:
        return 1;
    case HLSL_EXPR_MEMBER:
        return HlslStabilizeLvalueAddress(context, list,
                                           value->u.member.object);
    case HLSL_EXPR_SWIZZLE:
        return HlslStabilizeLvalueAddress(context, list,
                                           value->u.swizzle.object);
    case HLSL_EXPR_INDEX:
        if (!HlslStabilizeLvalueAddress(context, list,
                                        value->u.index.object))
        {
            return 0;
        }
        value->u.index.index = HlslCaptureValue(context, list,
                                                value->u.index.index);
        return value->u.index.index != NULL;
    default:
        return 0;
    }
} // HlslStabilizeLvalueAddress

static int HlslTypeNeedsRecursiveCopy(const HlslType *type)
{
    const HlslDecl *member;

    if (type == NULL)
        return 0;
    if (type->arraySize > 0)
        return 1;
    if (type->base != HLSL_BASE_STRUCT)
        return 0;
    for (member = type->members; member != NULL; member = member->next) {
        if (HlslTypeNeedsRecursiveCopy(&member->type))
            return 1;
    }
    return 0;
} // HlslTypeNeedsRecursiveCopy

static int HlslIsNativeAggregateTempAssignment(const expr *source)
{
    const expr *left;

    if (source == NULL || source->common.kind != BINARY_N ||
        HlslBinaryOperator(source->bin.op) != HLSL_OP_ASSIGN)
    {
        return 0;
    }
    left = source->bin.left;
    return left != NULL && left->common.kind == SYMB_N &&
           left->sym.symbol != NULL &&
           (left->sym.symbol->properties &
            SYMB_IS_NATIVE_AGGREGATE_TEMP) != 0;
} // HlslIsNativeAggregateTempAssignment

static HlslExpr *HlslCopyMember(HlslLowerContext *context,
                                HlslExpr *object, HlslDecl *member)
{
    HlslExpr *expression;

    expression = object != NULL && member != NULL ?
        HlslNewSourceExpr(context, HLSL_EXPR_MEMBER, member->type) : NULL;
    if (expression != NULL) {
        expression->u.member.object = object;
        expression->u.member.decl = member;
        expression->u.member.name = member->name;
    }
    return expression;
} // HlslCopyMember

static HlslExpr *HlslCopyIndex(HlslLowerContext *context,
                               HlslExpr *object,
                               const HlslType *elementType, int index)
{
    HlslExpr *expression;
    HlslExpr *subscript;

    expression = object != NULL && elementType != NULL ?
        HlslNewSourceExpr(context, HLSL_EXPR_INDEX, *elementType) : NULL;
    subscript = HlslNewLiteral(context, HLSL_BASE_INT, index, 0.0f);
    if (expression == NULL || subscript == NULL)
        return NULL;
    expression->u.index.object = object;
    expression->u.index.index = subscript;
    return expression;
} // HlslCopyIndex

static int HlslAppendRecursiveCopy(HlslLowerContext *context,
                                   const HlslType *type,
                                   HlslExpr *target, HlslExpr *source,
                                   HlslStmt **statements)
{
    HlslDecl *member;
    HlslExpr *assignment;
    int i;

    if (type == NULL || target == NULL || source == NULL)
        return 0;
    if (type->arraySize > 0) {
        for (i = 0; i < type->arraySize; i++) {
            if (!HlslAppendRecursiveCopy(context, type->elementType,
                    HlslCopyIndex(context, target, type->elementType, i),
                    HlslCopyIndex(context, source, type->elementType, i),
                    statements))
            {
                return 0;
            }
        }
        return 1;
    }
    if (type->base == HLSL_BASE_STRUCT) {
        for (member = type->members; member != NULL; member = member->next) {
            if (!HlslAppendRecursiveCopy(context, &member->type,
                    HlslCopyMember(context, target, member),
                    HlslCopyMember(context, source, member), statements))
            {
                return 0;
            }
        }
        return 1;
    }
    assignment = HlslNewAssignment(context, target, source);
    return assignment != NULL &&
           HlslAppendExpression(context, statements, assignment);
} // HlslAppendRecursiveCopy

static HlslExpr *HlslDetachLastExpression(HlslStmt **statements)
{
    HlslStmt **place;
    HlslStmt *statement;
    HlslExpr *expression;

    if (statements == NULL || *statements == NULL)
        return NULL;
    place = statements;
    while ((*place)->next != NULL)
        place = &(*place)->next;
    statement = *place;
    if (statement->kind != HLSL_STMT_EXPRESSION)
        return NULL;
    *place = NULL;
    expression = statement->u.expression;
    statement->u.expression = NULL;
    return expression;
} // HlslDetachLastExpression

static HlslExpr *HlslLowerOrderedValue(HlslLowerContext *context,
                                       expr *source, HlslStmt **prefix,
                                       int captureSideEffects)
{
    HlslExpr *value;
    HlslStmt *childPrefix;

    childPrefix = NULL;
    value = HlslLowerExpr(context, source, &childPrefix, 1);
    if (value == NULL)
        return NULL;
    HlslAppendStmt(prefix, childPrefix);
    if (captureSideEffects && source->common.HasSideEffects)
        value = HlslCaptureValue(context, prefix, value);
    return value;
} // HlslLowerOrderedValue

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

static HlslExpr *HlslScalarizeVectorCondition(
    HlslLowerContext *context, HlslStmt **prefix, HlslExpr *condition)
{
    HlslExpr *result;
    HlslExpr *component;
    HlslExpr *combined;
    HlslType boolType;
    int i;

    if (condition == NULL || condition->type.base != HLSL_BASE_BOOL ||
        condition->type.len < 1 || condition->type.len > 4)
    {
        return NULL;
    }
    if (condition->type.len == 1)
        return condition;
    condition = HlslCaptureValue(context, prefix, condition);
    if (condition == NULL)
        return NULL;
    result = HlslComponent(context, condition, 0, HLSL_BASE_BOOL);
    boolType = HlslNumericType(HLSL_BASE_BOOL, 1);
    for (i = 1; i < condition->type.len; i++) {
        component = HlslComponent(context, condition, i,
                                  HLSL_BASE_BOOL);
        combined = HlslNewSourceExpr(context, HLSL_EXPR_BINARY,
                                     boolType);
        if (component == NULL || combined == NULL)
            return NULL;
        combined->u.binary.op = HLSL_OP_LOGICAL_OR;
        combined->u.binary.left = result;
        combined->u.binary.right = component;
        result = combined;
    }
    return result;
} // HlslScalarizeVectorCondition

static HlslExpr *HlslLowerSwizzle(HlslLowerContext *context, expr *source,
                                  const HlslType *type, HlslStmt **prefix,
                                  HlslValueMode valueMode)
{
    HlslExpr *object;
    HlslExpr *target;
    char maskText[5];
    int count;
    int mask;
    int i;

    object = HlslLowerExpr(context, source->un.arg, prefix,
        valueMode == HLSL_VALUE_LVALUE ? HLSL_VALUE_LVALUE :
                                        HLSL_VALUE_RVALUE);
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

static HlslExpr *HlslLowerMatrixSwizzle(HlslLowerContext *context,
                                        expr *source,
                                        const HlslType *type,
                                        HlslStmt **prefix,
                                        HlslValueMode valueMode)
{
    HlslExpr *object;
    HlslExpr *rowExpression;
    HlslExpr *index;
    HlslType rowType;
    int count;
    int mask;
    int selector;
    int row;
    int column;
    int i;
    char columns[5];

    object = HlslLowerExpr(context, source->un.arg, prefix,
        valueMode == HLSL_VALUE_LVALUE ? HLSL_VALUE_LVALUE :
                                        HLSL_VALUE_RVALUE);
    if (object == NULL || object->type.rows <= 0 ||
        object->type.cols <= 0)
    {
        return NULL;
    }
    count = SUBOP_GET_T2(source->un.subop);
    if (count == 0)
        count = 1;
    if (count < 1 || count > 4)
        return NULL;
    mask = SUBOP_GET_MASK16(source->un.subop);
    selector = mask & 15;
    row = (selector >> 2) & 3;
    if (row >= object->type.rows)
        return NULL;
    for (i = 0; i < count; i++) {
        selector = (mask >> (i * 4)) & 15;
        if (((selector >> 2) & 3) != row)
            return NULL;
        column = selector & 3;
        if (column >= object->type.cols)
            return NULL;
        columns[i] = "xyzw"[column];
    }
    columns[count] = '\0';
    rowType = HlslNumericType(HLSL_BASE_FLOAT, object->type.cols);
    rowExpression = HlslNewSourceExpr(context, HLSL_EXPR_INDEX, rowType);
    index = HlslNewLiteral(context, HLSL_BASE_INT, row, 0.0f);
    if (rowExpression == NULL || index == NULL)
        return NULL;
    rowExpression->u.index.object = object;
    rowExpression->u.index.index = index;
    return HlslNewSwizzle(context, rowExpression, type, columns);
} // HlslLowerMatrixSwizzle

static HlslExpr *HlslLowerExprList(HlslLowerContext *context, expr *source,
                                   opcode listOp, HlslStmt **prefix,
                                   Symbol *formal)
{
    HlslExpr *list;
    HlslExpr *item;
    HlslExpr *rest;
    HlslStmt *itemPrefix;
    HlslStmt *restPrefix;
    int preserveLvalue;

    if (source == NULL || source->common.kind != BINARY_N ||
        source->bin.op != listOp)
    {
        return NULL;
    }
    itemPrefix = NULL;
    restPrefix = NULL;
    preserveLvalue = formal != NULL &&
        (GetQualifiers(formal->type) & TYPE_QUALIFIER_OUT);
    item = HlslLowerExpr(context, source->bin.left, &itemPrefix,
        preserveLvalue ? HLSL_VALUE_LVALUE : HLSL_VALUE_RVALUE);
    if (item == NULL)
        return NULL;
    rest = NULL;
    if (source->bin.right != NULL) {
        rest = HlslLowerExprList(context, source->bin.right, listOp,
                                 &restPrefix,
                                 formal != NULL ? formal->next : NULL);
        if (rest == NULL)
            return NULL;
    }
    HlslAppendStmt(prefix, itemPrefix);
    if (source->bin.right != NULL) {
        if (preserveLvalue &&
            (restPrefix != NULL ||
             source->bin.right->common.HasSideEffects))
        {
            if (!HlslStabilizeLvalueAddress(context, prefix, item))
                return NULL;
        } else if (!preserveLvalue &&
                   (source->bin.left->common.HasSideEffects ||
                    restPrefix != NULL ||
                    source->bin.right->common.HasSideEffects))
        {
            item = HlslCaptureValue(context, prefix, item);
            if (item == NULL)
                return NULL;
        }
    }
    HlslAppendStmt(prefix, restPrefix);
    list = item;
    HlslAppendExpr(&list, rest);
    return list;
} // HlslLowerExprList

static HlslExpr *HlslLowerCall(HlslLowerContext *context, expr *source,
                               const HlslType *type, HlslStmt **prefix)
{
    typedef struct HlslCopyOut_Rec {
        struct HlslCopyOut_Rec *next;
        HlslExpr *target;
        HlslDecl *temporary;
        HlslType type;
    } HlslCopyOut;

    HlslExpr *target;
    HlslExpr *argument;
    HlslExpr *nextArgument;
    HlslExpr **argumentPlace;
    HlslExpr *replacement;
    HlslExpr *result;
    HlslDecl *temporary;
    HlslFunction *function;
    HlslCopyOut *copyOut;
    HlslCopyOut **copyOutTail;
    HlslStmt *copies;
    Symbol *symbol;
    Symbol *formal;
    HlslBuiltin builtin;
    HlslBuiltinLowering lowering;
    HlslType builtinResult;
    HlslType builtinParams[HLSL_MAX_BUILTIN_ARGS];
    int builtinParamCount;
    int builtinStatus;

    if (source->bin.left == NULL ||
        source->bin.left->common.kind != SYMB_N)
    {
        return NULL;
    }
    symbol = source->bin.left->sym.symbol;
    builtinStatus = HlslResolveBuiltinSymbol(context, symbol,
        GetExprCallSite(source), &builtin, &builtinResult,
        builtinParams, &builtinParamCount);
    if (builtinStatus < 0)
        return NULL;
    if (builtinStatus > 0) {
        target = HlslNewSourceExpr(context, HLSL_EXPR_CALL, *type);
        if (target == NULL)
            return NULL;
        target->u.call.arguments = HlslLowerExprList(context,
            source->bin.right, FUN_ARG_OP, prefix, NULL);
        if (source->bin.right != NULL &&
            target->u.call.arguments == NULL)
        {
            return NULL;
        }
        lowering = HlslBuiltinLoweringKind(builtin);
        if (lowering == HLSL_BUILTIN_LOWER_NATIVE) {
            target->u.call.name = HlslBuiltinSpelling(builtin);
            target->u.call.builtin = builtin;
        } else {
            function = HlslCreateBuiltinHelper(context, builtin,
                                               &builtinResult);
            if (function == NULL) {
                HlslLowerFailure(context, HLSL_ERROR_INTRINSIC,
                                 HlslBuiltinSpelling(builtin),
                                 GetExprCallSite(source));
                return NULL;
            }
            target->u.call.function = function;
            target->u.call.name = function->name;
        }
        target->hasSideEffects = source->common.HasSideEffects;
        return target;
    }
    function = HlslFindFunction(context->module, symbol);
    if (function == NULL)
        return NULL;
    target = HlslNewSourceExpr(context, HLSL_EXPR_CALL, *type);
    if (target == NULL)
        return NULL;
    target->u.call.function = function;
    target->u.call.name = function->name;
    target->u.call.arguments = HlslLowerExprList(context,
        source->bin.right, FUN_ARG_OP, prefix,
        symbol->details.fun.params);
    if (source->bin.right != NULL && target->u.call.arguments == NULL)
        return NULL;
    target->hasSideEffects = source->common.HasSideEffects;

    copyOut = NULL;
    copyOutTail = &copyOut;
    argumentPlace = &target->u.call.arguments;
    formal = symbol->details.fun.params;
    while (*argumentPlace != NULL && formal != NULL) {
        argument = *argumentPlace;
        nextArgument = argument->next;
        if ((GetQualifiers(formal->type) & TYPE_QUALIFIER_OUT) &&
            HlslTypeNeedsRecursiveCopy(&argument->type))
        {
            argument->next = NULL;
            temporary = HlslNewTemporary(context, &argument->type);
            replacement = temporary != NULL ?
                HlslNewSymbolExpr(context, temporary) : NULL;
            *copyOutTail = (HlslCopyOut *) HlslLowerAlloc(
                context, sizeof(HlslCopyOut));
            if (replacement == NULL || *copyOutTail == NULL)
                return NULL;
            (*copyOutTail)->target = argument;
            (*copyOutTail)->temporary = temporary;
            (*copyOutTail)->type = argument->type;
            copyOutTail = &(*copyOutTail)->next;
            if ((GetQualifiers(formal->type) & TYPE_QUALIFIER_INOUT) ==
                    TYPE_QUALIFIER_INOUT &&
                !HlslAppendRecursiveCopy(context, &argument->type,
                    HlslNewSymbolExpr(context, temporary), argument,
                    prefix))
            {
                return NULL;
            }
            replacement->next = nextArgument;
            *argumentPlace = replacement;
        }
        argumentPlace = &(*argumentPlace)->next;
        formal = formal->next;
    }
    if (*argumentPlace != NULL || formal != NULL)
        return NULL;
    if (copyOut == NULL)
        return target;

    if (type->base == HLSL_BASE_VOID) {
        if (!HlslAppendExpression(context, prefix, target))
            return NULL;
        result = NULL;
    } else {
        temporary = HlslNewTemporary(context, type);
        result = temporary != NULL ?
            HlslNewSymbolExpr(context, temporary) : NULL;
        if (temporary == NULL || result == NULL ||
            !HlslAppendExpression(context, prefix,
                HlslNewAssignment(context,
                    HlslNewSymbolExpr(context, temporary), target)))
        {
            return NULL;
        }
    }
    copies = NULL;
    for (; copyOut != NULL; copyOut = copyOut->next) {
        if (!HlslAppendRecursiveCopy(context, &copyOut->type,
                copyOut->target,
                HlslNewSymbolExpr(context, copyOut->temporary), &copies))
        {
            return NULL;
        }
    }
    if (type->base != HLSL_BASE_VOID) {
        HlslAppendStmt(prefix, copies);
        return result;
    }
    result = HlslDetachLastExpression(&copies);
    HlslAppendStmt(prefix, copies);
    return result;
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
        condition = HlslCaptureValue(context, prefix, condition);
        trueExpr = HlslLowerExpr(context, source->tri.arg2, prefix, 1);
        if (trueExpr != NULL)
            trueExpr = HlslCaptureValue(context, prefix, trueExpr);
        falseExpr = HlslLowerExpr(context, source->tri.arg3, prefix, 1);
        if (falseExpr != NULL)
            falseExpr = HlslCaptureValue(context, prefix, falseExpr);
        target = HlslNewSourceExpr(context, HLSL_EXPR_CONDITIONAL, *type);
        if (target == NULL || condition == NULL ||
            trueExpr == NULL || falseExpr == NULL)
        {
            return NULL;
        }
        target->u.conditional.condition = condition;
        target->u.conditional.trueExpr = trueExpr;
        target->u.conditional.falseExpr = falseExpr;
        return target;
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

    left = HlslLowerOrderedValue(context, source->bin.left, prefix, 0);
    if (left != NULL)
        left = HlslCaptureValue(context, prefix, left);
    right = HlslLowerOrderedValue(context, source->bin.right, prefix, 0);
    if (right != NULL)
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
                               HlslStmt **prefix, HlslValueMode valueMode)
{
    HlslExpr *target;
    HlslDecl *decl;
    HlslType type;
    HlslOperator op;
    Symbol *member;
    HlslExpr *left;
    HlslExpr *right;
    HlslStmt *leftStatement;
    HlslStmt *leftPrefix;
    HlslStmt *rightPrefix;

    if (source != NULL && source->common.kind == BINARY_N &&
        source->bin.op == COMMA_OP)
    {
        left = HlslLowerExpr(context, source->bin.left, prefix,
                             HLSL_VALUE_DISCARD);
        if (left == NULL)
            return NULL;
        leftStatement = HlslNewExpressionStmt(context, left);
        if (leftStatement == NULL)
            return NULL;
        HlslAppendStmt(prefix, leftStatement);
        return HlslLowerExpr(context, source->bin.right, prefix,
                             valueMode);
    }
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
            return HlslLowerSwizzle(context, source, &type, prefix,
                                    valueMode);
        if (source->un.op == SWIZMAT_Z_OP)
            return HlslLowerMatrixSwizzle(context, source, &type, prefix,
                                          valueMode);
        if (source->un.op == VECTOR_V_OP) {
            target = HlslNewSourceExpr(context, HLSL_EXPR_CONSTRUCT, type);
            if (target == NULL)
                return NULL;
            target->u.construct.arguments = HlslLowerExprList(context,
                source->un.arg, EXPR_LIST_OP, prefix, NULL);
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
                context, source->bin.left, prefix,
                valueMode == HLSL_VALUE_LVALUE ? HLSL_VALUE_LVALUE :
                                                HLSL_VALUE_RVALUE);
            target->u.member.decl = decl;
            target->u.member.name = decl->name;
            return target->u.member.object != NULL ? target : NULL;
        }
        if (source->bin.op == ARRAY_INDEX_OP) {
            target = HlslNewSourceExpr(context, HLSL_EXPR_INDEX, type);
            if (target == NULL)
                return NULL;
            leftPrefix = NULL;
            rightPrefix = NULL;
            target->u.index.object = HlslLowerExpr(
                context, source->bin.left, &leftPrefix,
                valueMode == HLSL_VALUE_LVALUE ? HLSL_VALUE_LVALUE :
                                                HLSL_VALUE_RVALUE);
            target->u.index.index = HlslLowerOrderedValue(
                context, source->bin.right, &rightPrefix, 1);
            if (target->u.index.object == NULL ||
                target->u.index.index == NULL)
            {
                return NULL;
            }
            HlslAppendStmt(prefix, leftPrefix);
            if (valueMode != HLSL_VALUE_LVALUE &&
                (source->bin.left->common.HasSideEffects ||
                 rightPrefix != NULL ||
                 source->bin.right->common.HasSideEffects))
            {
                target->u.index.object = HlslCaptureValue(
                    context, prefix, target->u.index.object);
                if (target->u.index.object == NULL)
                    return NULL;
            }
            HlslAppendStmt(prefix, rightPrefix);
            return target;
        }
        if (source->bin.op == FUN_CALL_OP ||
            source->bin.op == FUN_INTRINSIC_OP)
            return HlslLowerCall(context, source, &type, prefix);
        op = HlslBinaryOperator(source->bin.op);
        if (op != HLSL_OP_NONE) {
            if (HlslIsComparison(op) && type.base == HLSL_BASE_BOOL &&
                type.len > 1)
            {
                return HlslLowerVectorComparison(context, source, &type,
                                                  op, prefix);
            }
            leftPrefix = NULL;
            rightPrefix = NULL;
            left = HlslLowerExpr(context, source->bin.left,
                &leftPrefix, HlslIsAssignmentOperator(op) ?
                             HLSL_VALUE_LVALUE : HLSL_VALUE_RVALUE);
            right = HlslLowerExpr(context, source->bin.right,
                                  &rightPrefix, 1);
            if (left == NULL || right == NULL)
                return NULL;
            HlslAppendStmt(prefix, leftPrefix);
            if (!HlslIsAssignmentOperator(op) &&
                (source->bin.left->common.HasSideEffects ||
                 rightPrefix != NULL ||
                 source->bin.right->common.HasSideEffects))
            {
                left = HlslCaptureValue(context, prefix, left);
                if (left == NULL)
                    return NULL;
            }
            if (HlslIsAssignmentOperator(op) &&
                (rightPrefix != NULL ||
                 source->bin.right->common.HasSideEffects) &&
                !HlslStabilizeLvalueAddress(context, prefix, left))
            {
                return NULL;
            }
            HlslAppendStmt(prefix, rightPrefix);
            if (op == HLSL_OP_ASSIGN &&
                HlslTypeNeedsRecursiveCopy(&type) &&
                HlslIsNativeAggregateTempAssignment(source))
            {
                target = HlslNewSourceExpr(context, HLSL_EXPR_BINARY, type);
                if (target == NULL)
                    return NULL;
                target->u.binary.op = op;
                target->u.binary.left = left;
                target->u.binary.right = right;
                target->hasSideEffects = source->common.HasSideEffects;
                return target;
            }
            if (op == HLSL_OP_ASSIGN &&
                HlslTypeNeedsRecursiveCopy(&type))
            {
                HlslStmt *copies;

                if (!HlslStabilizeLvalueAddress(context, prefix, left))
                    return NULL;
                copies = NULL;
                if (valueMode == HLSL_VALUE_RVALUE) {
                    HlslDecl *temporary;

                    temporary = HlslNewTemporary(context, &type);
                    if (temporary == NULL ||
                        !HlslAppendRecursiveCopy(context, &type,
                            HlslNewSymbolExpr(context, temporary), right,
                            &copies) ||
                        !HlslAppendRecursiveCopy(context, &type, left,
                            HlslNewSymbolExpr(context, temporary), &copies))
                    {
                        return NULL;
                    }
                    HlslAppendStmt(prefix, copies);
                    return HlslNewSymbolExpr(context, temporary);
                }
                if (!HlslAppendRecursiveCopy(context, &type, left, right,
                                             &copies))
                {
                    return NULL;
                }
                target = HlslDetachLastExpression(&copies);
                HlslAppendStmt(prefix, copies);
                return target;
            }
            if (op == HLSL_OP_ASSIGN &&
                valueMode == HLSL_VALUE_RVALUE)
            {
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

static HlslExpr *HlslNewUnaryExpr(HlslLowerContext *context,
                                  HlslOperator op, HlslExpr *operand)
{
    HlslExpr *expression;

    if (operand == NULL)
        return NULL;
    expression = HlslNewSourceExpr(context, HLSL_EXPR_UNARY,
                                   operand->type);
    if (expression != NULL) {
        expression->u.unary.op = op;
        expression->u.unary.operand = operand;
        expression->hasSideEffects = operand->hasSideEffects;
    }
    return expression;
} // HlslNewUnaryExpr

static HlslStmt *HlslNewBreakGuard(HlslLowerContext *context,
                                   HlslExpr *condition)
{
    HlslStmt *guard;
    HlslStmt *jump;

    guard = HlslNewStmt(context->module, HLSL_STMT_IF);
    jump = HlslNewStmt(context->module, HLSL_STMT_BREAK);
    if (guard == NULL || jump == NULL)
        return NULL;
    guard->u.ifStmt.condition = HlslNewUnaryExpr(
        context, HLSL_OP_LOGICAL_NOT, condition);
    if (guard->u.ifStmt.condition == NULL)
        return NULL;
    guard->u.ifStmt.trueBranch = jump;
    HlslSetLoc(&guard->loc, &context->statementLoc);
    HlslSetLoc(&jump->loc, &context->statementLoc);
    return guard;
} // HlslNewBreakGuard

static HlslStmt *HlslNewBoolAssignment(HlslLowerContext *context,
                                       HlslDecl *decl, int value)
{
    HlslExpr *left;
    HlslExpr *right;
    HlslExpr *assignment;

    if (decl == NULL)
        return NULL;
    left = HlslNewSymbolExpr(context, decl);
    right = HlslNewLiteral(context, HLSL_BASE_BOOL, value, 0.0f);
    assignment = HlslNewAssignment(context, left, right);
    return HlslNewExpressionStmt(context, assignment);
} // HlslNewBoolAssignment

static int HlslStatementsAreForParts(const HlslStmt *statement)
{
    for (; statement != NULL; statement = statement->next) {
        if (statement->kind != HLSL_STMT_EXPRESSION)
            return 0;
    }
    return 1;
} // HlslStatementsAreForParts

static int HlslRewriteLoopBreaks(HlslLowerContext *context,
                                 HlslStmt *statement, HlslDecl *flag,
                                 int nestedLoopDepth)
{
    HlslStmt *assignment;
    HlslStmt *jump;

    for (; statement != NULL; statement = statement->next) {
        if (statement->kind == HLSL_STMT_BREAK && nestedLoopDepth == 0) {
            assignment = HlslNewBoolAssignment(context, flag, 1);
            jump = HlslNewStmt(context->module, HLSL_STMT_BREAK);
            if (assignment == NULL || jump == NULL)
                return 0;
            HlslSetLoc(&jump->loc, &context->statementLoc);
            assignment->next = jump;
            statement->kind = HLSL_STMT_BLOCK;
            statement->u.block = assignment;
        } else if (statement->kind == HLSL_STMT_IF) {
            if (!HlslRewriteLoopBreaks(context,
                    statement->u.ifStmt.trueBranch, flag,
                    nestedLoopDepth) ||
                !HlslRewriteLoopBreaks(context,
                    statement->u.ifStmt.falseBranch, flag,
                    nestedLoopDepth))
            {
                return 0;
            }
        } else if (statement->kind == HLSL_STMT_BLOCK) {
            if (!HlslRewriteLoopBreaks(context, statement->u.block,
                                       flag, nestedLoopDepth))
            {
                return 0;
            }
        } else if (statement->kind == HLSL_STMT_WHILE ||
                   statement->kind == HLSL_STMT_DO)
        {
            if (!HlslRewriteLoopBreaks(context, statement->u.loop.body,
                                       flag, nestedLoopDepth + 1))
            {
                return 0;
            }
        } else if (statement->kind == HLSL_STMT_FOR) {
            if (!HlslRewriteLoopBreaks(context,
                    statement->u.forStmt.body, flag,
                    nestedLoopDepth + 1))
            {
                return 0;
            }
        }
    }
    return 1;
} // HlslRewriteLoopBreaks

static int HlslLowerStatements(HlslLowerContext *context, stmt *source,
                               HlslStmt **list)
{
    HlslStmt *target;
    HlslStmt *discard;
    HlslStmt *conditionPrefix;
    HlslStmt *stepStatements;
    HlslStmt *body;
    HlslStmt *guard;
    HlslStmt *firstTest;
    HlslStmt *firstAssignment;
    HlslStmt *breakAssignment;
    HlslStmt *wrapper;
    HlslStmt *breakTest;
    HlslStmt *jump;
    HlslExpr *condition;
    HlslExpr *loopCondition;
    HlslExpr *flagExpr;
    HlslDecl *flag;
    HlslType boolType;
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
            conditionPrefix = NULL;
            body = NULL;
            target = HlslNewStmt(context->module,
                source->commonst.kind == WHILE_STMT ?
                HLSL_STMT_WHILE : HLSL_STMT_DO);
            if (target == NULL)
                return 0;
            loopCondition = HlslLowerExpr(
                context, source->whilest.cond, &conditionPrefix, 1);
            if (loopCondition == NULL)
                return 0;
            context->loopDepth++;
            if (!HlslLowerStatements(context, source->whilest.body,
                                     &body))
            {
                context->loopDepth--;
                return 0;
            }
            context->loopDepth--;
            context->statementLoc = source->commonst.loc;
            if (conditionPrefix == NULL) {
                target->u.loop.condition = loopCondition;
                target->u.loop.body = body;
            } else if (source->commonst.kind == WHILE_STMT) {
                target->u.loop.condition = HlslNewLiteral(
                    context, HLSL_BASE_BOOL, 1, 0.0f);
                guard = HlslNewBreakGuard(context, loopCondition);
                if (target->u.loop.condition == NULL || guard == NULL)
                    return 0;
                target->u.loop.body = conditionPrefix;
                HlslAppendStmt(&target->u.loop.body, guard);
                HlslAppendStmt(&target->u.loop.body, body);
            } else {
                boolType = HlslNumericType(HLSL_BASE_BOOL, 1);
                flag = HlslNewTemporary(context, &boolType);
                firstAssignment = HlslNewBoolAssignment(context, flag, 1);
                firstTest = HlslNewStmt(context->module, HLSL_STMT_IF);
                target->kind = HLSL_STMT_WHILE;
                target->u.loop.condition = HlslNewLiteral(
                    context, HLSL_BASE_BOOL, 1, 0.0f);
                guard = HlslNewBreakGuard(context, loopCondition);
                flagExpr = flag != NULL ?
                    HlslNewSymbolExpr(context, flag) : NULL;
                if (firstAssignment == NULL || firstTest == NULL ||
                    target->u.loop.condition == NULL || guard == NULL ||
                    flagExpr == NULL)
                {
                    return 0;
                }
                firstTest->u.ifStmt.condition = HlslNewUnaryExpr(
                    context, HLSL_OP_LOGICAL_NOT, flagExpr);
                if (firstTest->u.ifStmt.condition == NULL)
                    return 0;
                firstTest->u.ifStmt.trueBranch = conditionPrefix;
                HlslAppendStmt(&firstTest->u.ifStmt.trueBranch, guard);
                target->u.loop.body = firstTest;
                breakAssignment = HlslNewBoolAssignment(context, flag, 0);
                if (breakAssignment == NULL)
                    return 0;
                HlslAppendStmt(&target->u.loop.body, breakAssignment);
                HlslAppendStmt(&target->u.loop.body, body);
                HlslSetLoc(&firstTest->loc, &source->commonst.loc);
                HlslSetLoc(&firstAssignment->loc, &source->commonst.loc);
                HlslAppendStmt(list, firstAssignment);
            }
        } else if (source->commonst.kind == FOR_STMT) {
            conditionPrefix = NULL;
            stepStatements = NULL;
            body = NULL;
            target = HlslNewStmt(context->module, HLSL_STMT_FOR);
            if (target == NULL ||
                !HlslLowerStatements(context, source->forst.init,
                                     &target->u.forStmt.init) ||
                (source->forst.cond != NULL &&
                 (loopCondition = HlslLowerExpr(
                    context, source->forst.cond,
                    &conditionPrefix, 1)) == NULL) ||
                !HlslLowerStatements(context, source->forst.step,
                                     &stepStatements))
            {
                return 0;
            }
            context->loopDepth++;
            if (!HlslLowerStatements(context, source->forst.body,
                                     &body))
            {
                context->loopDepth--;
                return 0;
            }
            context->loopDepth--;
            context->statementLoc = source->commonst.loc;
            if (!HlslStatementsAreForParts(target->u.forStmt.init)) {
                HlslAppendStmt(list, target->u.forStmt.init);
                target->u.forStmt.init = NULL;
            }
            if (conditionPrefix == NULL) {
                target->u.forStmt.condition = source->forst.cond != NULL ?
                    loopCondition : NULL;
            } else {
                guard = HlslNewBreakGuard(context, loopCondition);
                if (guard == NULL)
                    return 0;
                HlslAppendStmt(&conditionPrefix, guard);
            }
            if (HlslStatementsAreForParts(stepStatements)) {
                target->u.forStmt.step = stepStatements;
                target->u.forStmt.body = conditionPrefix;
                HlslAppendStmt(&target->u.forStmt.body, body);
            } else {
                boolType = HlslNumericType(HLSL_BASE_BOOL, 1);
                flag = HlslNewTemporary(context, &boolType);
                breakAssignment = HlslNewBoolAssignment(context, flag, 0);
                wrapper = HlslNewStmt(context->module, HLSL_STMT_DO);
                breakTest = HlslNewStmt(context->module, HLSL_STMT_IF);
                jump = HlslNewStmt(context->module, HLSL_STMT_BREAK);
                flagExpr = flag != NULL ?
                    HlslNewSymbolExpr(context, flag) : NULL;
                if (flag == NULL || breakAssignment == NULL ||
                    wrapper == NULL || breakTest == NULL || jump == NULL ||
                    flagExpr == NULL ||
                    !HlslRewriteLoopBreaks(context, body, flag, 0))
                {
                    return 0;
                }
                wrapper->u.loop.condition = HlslNewLiteral(
                    context, HLSL_BASE_BOOL, 0, 0.0f);
                if (wrapper->u.loop.condition == NULL)
                    return 0;
                wrapper->u.loop.body = body;
                breakTest->u.ifStmt.condition = flagExpr;
                breakTest->u.ifStmt.trueBranch = jump;
                target->u.forStmt.body = conditionPrefix;
                HlslAppendStmt(&target->u.forStmt.body, breakAssignment);
                HlslAppendStmt(&target->u.forStmt.body, wrapper);
                HlslAppendStmt(&target->u.forStmt.body, breakTest);
                HlslAppendStmt(&target->u.forStmt.body, stepStatements);
                HlslSetLoc(&wrapper->loc, &source->commonst.loc);
                HlslSetLoc(&breakTest->loc, &source->commonst.loc);
                HlslSetLoc(&jump->loc, &source->commonst.loc);
            }
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
                if (condition != NULL)
                    condition = HlslScalarizeVectorCondition(
                        context, list, condition);
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

static int HlslCollectHelper(HlslLowerContext *context, Symbol *symbol,
                             const SourceLoc *callLoc)
{
    HlslFunction *function;
    HlslType result;
    char *generated;
    const char *name;
    Symbol *caller;
    SourceLoc callerLoc;

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
                                    callLoc);
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
    callerLoc = context->statementLoc;
    if (caller != NULL)
        function->needsPrototype = 1;
    HlslSetLoc(&function->loc, &symbol->loc);
    HlslAppendFunction(&context->module->functions, function);
    context->collectingHelper = symbol;
    if (!HlslCollectCallsInStatements(context,
                                      symbol->details.fun.statements))
    {
        context->collectingHelper = caller;
        context->statementLoc = callerLoc;
        return 0;
    }
    context->collectingHelper = caller;
    context->statementLoc = callerLoc;
    function->visitState = 2;
    return 1;
} // HlslCollectHelper

static int HlslCollectCallsInExpr(HlslLowerContext *context, expr *source)
{
    const SourceLoc *callLoc;
    HlslBuiltin builtin;
    HlslType result;
    HlslType params[HLSL_MAX_BUILTIN_ARGS];
    int paramCount;
    int builtinStatus;

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
        callLoc = source->bin.op == FUN_CALL_OP ?
                  GetExprCallSite(source) : NULL;
        if (source->bin.op == FUN_CALL_OP && source->bin.left != NULL &&
            source->bin.left->common.kind == SYMB_N)
        {
            builtinStatus = HlslResolveBuiltinSymbol(context,
                source->bin.left->sym.symbol, callLoc, &builtin,
                &result, params, &paramCount);
            if (builtinStatus < 0)
                return 0;
            if (builtinStatus == 0 &&
                !HlslCollectHelper(context,
                    source->bin.left->sym.symbol, callLoc))
            {
                return 0;
            }
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

static int HlslDeclListContains(const HlslDecl *declarations,
                                const HlslDecl *target)
{
    for (; declarations != NULL; declarations = declarations->next) {
        if (declarations == target)
            return 1;
    }
    return 0;
} // HlslDeclListContains

static int HlslInitializeReturnedStructs(HlslModule *module,
                                         HlslFunction *function,
                                         HlslStmt *statements)
{
    HlslDecl *declaration;
    HlslExpr *zero;
    HlslExpr *initializer;

    for (; statements != NULL; statements = statements->next) {
        switch (statements->kind) {
        case HLSL_STMT_IF:
            if (!HlslInitializeReturnedStructs(module, function,
                    statements->u.ifStmt.trueBranch) ||
                !HlslInitializeReturnedStructs(module, function,
                    statements->u.ifStmt.falseBranch))
            {
                return 0;
            }
            break;
        case HLSL_STMT_WHILE:
        case HLSL_STMT_DO:
            if (!HlslInitializeReturnedStructs(module, function,
                                                statements->u.loop.body))
            {
                return 0;
            }
            break;
        case HLSL_STMT_FOR:
            if (!HlslInitializeReturnedStructs(module, function,
                    statements->u.forStmt.init) ||
                !HlslInitializeReturnedStructs(module, function,
                    statements->u.forStmt.body))
            {
                return 0;
            }
            break;
        case HLSL_STMT_BLOCK:
            if (!HlslInitializeReturnedStructs(module, function,
                                                statements->u.block))
            {
                return 0;
            }
            break;
        case HLSL_STMT_RETURN:
            if (statements->u.returnExpr == NULL ||
                statements->u.returnExpr->kind != HLSL_EXPR_SYMBOL)
            {
                break;
            }
            declaration = statements->u.returnExpr->u.symbol;
            if (declaration == NULL || declaration->initializer != NULL ||
                declaration->type.base != HLSL_BASE_STRUCT ||
                !HlslDeclListContains(function->locals, declaration))
            {
                break;
            }
            zero = HlslNewExpr(module, HLSL_EXPR_INT,
                               HlslNumericType(HLSL_BASE_INT, 1));
            initializer = HlslNewExpr(module, HLSL_EXPR_CAST,
                                      declaration->type);
            if (zero == NULL || initializer == NULL)
                return 0;
            zero->u.literalInt = 0;
            initializer->u.cast.expression = zero;
            declaration->initializer = initializer;
            break;
        default:
            break;
        }
    }
    return 1;
} // HlslInitializeReturnedStructs

static int HlslLowerFunction(HlslLowerContext *context,
                             HlslFunction *function)
{
    Symbol *symbol;
    const char *name;

    symbol = (Symbol *) function->identity;
    name = symbol != NULL ? GetAtomString(atable, symbol->name) : NULL;
    context->function = function;
    context->statementLoc = symbol->loc;
    if (!HlslCollectParameters(context, symbol->details.fun.params, 0))
        return context->module->errors != 0 ? 0 :
            HlslLowerFailure(context, HLSL_ERROR_UNSUPPORTED_OPERATION,
                             name, &symbol->loc);
    if (symbol->details.fun.locals == NULL)
        return HlslLowerFailure(context, HLSL_ERROR_UNSUPPORTED_OPERATION,
                                name, &symbol->loc);
    if (!HlslCollectLocals(context, symbol->details.fun.locals->symbols))
        return context->module->errors != 0 ? 0 :
            HlslLowerFailure(context, HLSL_ERROR_UNSUPPORTED_OPERATION,
                             name, &symbol->loc);
    if (!HlslLowerStatements(context, symbol->details.fun.statements,
                             &function->body))
    {
        return context->module->errors != 0 ? 0 :
            HlslLowerFailure(context, HLSL_ERROR_UNSUPPORTED_OPERATION,
                             name, &symbol->loc);
    }
    return HlslInitializeReturnedStructs(context->module, function,
                                         function->body);
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
        !HlslCollectDefaults(&context))
    {
        return 0;
    }
    for (helper = module->functions; helper != NULL; helper = helper->next) {
        if (helper->identity != NULL &&
            !HlslLowerFunction(&context, helper))
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
                              &function->body)) ||
        (!emptyEntry &&
         !HlslInitializeReturnedStructs(module, function, function->body)))
    {
        return 0;
    }
    return HlslSortStructs(&context) && module->errors == 0;
} // HlslLowerProgram

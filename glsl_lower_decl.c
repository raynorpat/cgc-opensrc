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
// glsl_lower_decl.c
//

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <float.h>
#include <limits.h>

#include "glsl_lower_internal.h"


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

int GlslLowerType(GlslLowerContext *context, Type *source,
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
    if (GetCategory(source) == TYPE_CATEGORY_SAMPLER) {
        /* Adapter: canonical language samplers keep the historical
         * texture-object bases, so the GLSL type mapping below is
         * unchanged; kinds outside the focused GLSL profile are rejected
         * through the normal unsupported-type path. */
        switch (source->samp.samplerKind) {
        case CG_SAMPLER_1D:
            *target = GlslNumericType(GLSL_BASE_SAMPLER1D, 1);
            return 1;
        case CG_SAMPLER_2D:
            *target = GlslNumericType(GLSL_BASE_SAMPLER2D, 1);
            return 1;
        case CG_SAMPLER_3D:
            *target = GlslNumericType(GLSL_BASE_SAMPLER3D, 1);
            return 1;
        case CG_SAMPLER_CUBE:
            *target = GlslNumericType(GLSL_BASE_SAMPLERCUBE, 1);
            return 1;
        default:
            GlslRecordFailureKindAt(context, GLSL_ERROR_UNSUPPORTED_TYPE,
                                    source->samp.samplerKind ==
                                        CG_SAMPLER_RECT ? "samplerRECT" :
                                                          "sampler",
                                    loc);
            return 0;
        }
    }
    if (CgIsAttribArray(source)) {
        if (context->module->stage != GLSL_STAGE_GEOMETRY ||
            CgAttribArrayExtent(source) == 0 ||
            !GlslIRType(context, CgAttribArrayElement(source),
                        &elementType, loc))
        {
            GlslRecordFailureKindAt(context, GLSL_ERROR_UNSUPPORTED_TYPE,
                                    "AttribArray", loc);
            return 0;
        }
        element = (GlslType *) context->module->alloc(
            context->module->allocArg, sizeof(GlslType));
        if (element == NULL)
            return 0;
        *element = elementType;
        *target = GlslNumericType(GLSL_BASE_VOID, 0);
        target->arraySize = (int) CgAttribArrayExtent(source);
        target->elementType = element;
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

void GlslInsertDecl(GlslDecl **list, GlslDecl *decl)
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

    if (!GlslLowerTypeAuto(context, symbol->type, &type, &symbol->loc))
        return NULL;
    sourceName = GetAtomString(atable, symbol->name);
    if (nameSpace != NULL) {
        name = GlslAllocateScopedSymbolNameForSource(context, nameSpace,
            symbol, sourceName, &symbol->loc);
    } else {
        name = GlslAllocateSymbolNameForSource(context, symbol, sourceName,
                                               &symbol->loc);
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

int GlslEnsureTypeAt(GlslLowerContext *context, Type *type,
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

int GlslEnsureSymbolTypes(GlslLowerContext *context, Symbol *symbol)
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

int GlslEnsureParameterTypes(GlslLowerContext *context,
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
    if (GetCategory(type) == TYPE_CATEGORY_SAMPLER) {
        /* Adapter: canonical language samplers validate through their
         * family kind; the focused GLSL profile has no spelling for samplerRECT or the
         * deprecated base sampler. */
        switch (type->samp.samplerKind) {
        case CG_SAMPLER_1D:
        case CG_SAMPLER_2D:
        case CG_SAMPLER_3D:
        case CG_SAMPLER_CUBE:
            return 1;
        default:
            GlslRecordFailureKind(context, GLSL_ERROR_UNSUPPORTED_TYPE,
                                  type->samp.samplerKind ==
                                      CG_SAMPLER_RECT ? "samplerRECT" :
                                                        "sampler");
            return 0;
        }
    }
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
    if (sourceName[0] == '$')
        return 1;
    name = GlslAllocateSymbolNameForSource(context, tag, sourceName,
                                           &tag->loc);
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

int GlslSortStructs(GlslLowerContext *context)
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


int GlslCollectParameters(GlslLowerContext *context, Symbol *formal,
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

int GlslFiniteDefaultFloat(float value)
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
        !GlslFiniteDefaultFloat((float) value->value.f))
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
        !GlslFiniteDefaultFloat((float) value->value.value.f))
    {
        GlslRecordFailure(context, "uniform default finite value");
        return 0;
    }
    switch (targetClass) {
    case GLSL_DEFAULT_BASE_FLOAT:
        if (sourceClass == GLSL_DEFAULT_BASE_FLOAT)
            converted.value.f = value->value.value.f;
        else if (sourceClass == GLSL_DEFAULT_BASE_INT)
            converted.value.f = (float) value->value.value.i;
        else
            converted.value.f = value->value.value.i ? 1.0f : 0.0f;
        if (!GlslFiniteDefaultFloat((float) converted.value.f)) {
            GlslRecordFailure(context, "uniform default finite value");
            return 0;
        }
        break;
    case GLSL_DEFAULT_BASE_INT:
        if (sourceClass == GLSL_DEFAULT_BASE_FLOAT) {
            floating = (double) value->value.value.f;
            if (floating < (double) INT_MIN ||
                floating > (double) INT_MAX)
            {
                GlslRecordFailure(context,
                                  "uniform default conversion range");
                return 0;
            }
            converted.value.i = (int) floating;
        } else if (sourceClass == GLSL_DEFAULT_BASE_INT) {
            converted.value.i = (int) value->value.value.i;
        } else {
            converted.value.i = value->value.value.i ? 1 : 0;
        }
        break;
    case GLSL_DEFAULT_BASE_BOOL:
        if (sourceClass == GLSL_DEFAULT_BASE_FLOAT)
            converted.value.i = value->value.value.f != 0.0f;
        else
            converted.value.i = value->value.value.i != 0;
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
                    (float) typedValues[*index].value.value.f)) return 0;
        } else {
            if (!GlslAppendDefaultValue(context, values, capacity, count,
                    (float) typedValues[*index].value.value.i)) return 0;
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

int GlslCollectDefaults(GlslLowerContext *context)
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

int GlslCollectLocals(GlslLowerContext *context, Symbol *symbol,
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

/*
 * GlslLowerTypeAuto() - Shared helpers run under both lowering paths;
 *          struct resolution needs the scope-tag lookup on the legacy
 *          tree path and the IR registration map on the Cg 2.0 path.
 */

int GlslLowerTypeAuto(GlslLowerContext *context, Type *source,
                             GlslType *target, const SourceLoc *loc)
{
    if (context->source != NULL)
        return GlslIRType(context, source, target, loc);
    return GlslLowerType(context, source, target, loc);
} // GlslLowerTypeAuto

int GlslEnsureTypeAuto(GlslLowerContext *context, Type *type,
                              const SourceLoc *loc)
{
    if (context->source != NULL)
        return GlslIREnsureType(context, type, loc);
    return GlslEnsureType(context, type);
} // GlslEnsureTypeAuto

/*
 * GlslIRScalarBase() - Canonical scalar kind to GLSL base.  Kinds with
 *          no focused GLSL profile spelling record the existing unsupported-type
 *          diagnostic and return zero.
 */

int GlslIRScalarBase(GlslLowerContext *context, CgScalarKind kind,
                            GlslBase *base, const SourceLoc *loc)
{
    switch (kind) {
    case CG_SCALAR_FLOAT:
    case CG_SCALAR_CFLOAT:
        *base = GLSL_BASE_FLOAT;
        return 1;
    case CG_SCALAR_INT:
    case CG_SCALAR_CINT:
        *base = GLSL_BASE_INT;
        return 1;
    case CG_SCALAR_BOOL:
        *base = GLSL_BASE_BOOL;
        return 1;
    default:
        /* half/fixed/double and every unsigned width carry no GLSL
         * focused GLSL profile spelling; profile validation rejects them here. */
        GlslRecordFailureKindAt(context, GLSL_ERROR_UNSUPPORTED_TYPE,
                                CgScalarKindName(kind), loc);
        return 0;
    }
} // GlslIRScalarBase

/*
 * GlslIRIsSamplerValue() - Canonical sampler values (the language
 *          sampler category with a bound texture family).  The legacy
 *          typedef bases never appear in Cg 2.0 IR.
 */

int GlslIRIsSamplerValue(const Type *type)
{
    if (type == NULL || GetCategory(type) != TYPE_CATEGORY_SAMPLER)
        return 0;
    switch (type->samp.samplerKind) {
    case CG_SAMPLER_1D:
    case CG_SAMPLER_2D:
    case CG_SAMPLER_3D:
    case CG_SAMPLER_CUBE:
        return 1;
    default:
        return 0;
    }
} // GlslIRIsSamplerValue

/*
 * GlslIRFindStruct() - Struct declarations register with the canonical
 *          Type pointer as identity; no scope-tag lookup exists on the
 *          IR path.  Duplicated copies of one source structure share
 *          their tag atom and declaration location, so those match too.
 */

static GlslDecl *GlslIRFindStruct(GlslLowerContext *context, Type *type)
{
    Type *canonical;
    GlslDecl *decl;

    canonical = GlslCanonicalStructType(type);
    if (canonical == NULL)
        return NULL;
    for (decl = context->module->structs; decl != NULL; decl = decl->next) {
        if (decl->identity == canonical)
            return decl;
        if (decl->sourceOrdinal == canonical->str.tag &&
            decl->loc.file == canonical->str.loc.file &&
            decl->loc.line == canonical->str.loc.line)
        {
            return decl;
        }
    }
    return NULL;
} // GlslIRFindStruct

/*
 * GlslIRRegisterStruct() - Translate one canonical struct type into the
 *          module's struct list.  Anonymous internal structures (the
 *          "$vin"/"$vout" connectors) never emit.  "sourceOrdinal"
 *          doubles as the tag atom for duplicate matching (structure
 *          sorting reads location and name only).
 */

static int GlslIRRegisterStruct(GlslLowerContext *context, Type *type,
                                const SourceLoc *loc)
{
    Type *canonical;
    GlslDecl *decl;
    GlslType structType;
    const char *sourceName;
    const char *name;

    canonical = GlslCanonicalStructType(type);
    if (canonical == NULL)
        return 0;
    if (GlslIRFindStruct(context, canonical) != NULL)
        return 1;
    sourceName = GetAtomString(atable, canonical->str.tag);
    if (sourceName == NULL)
        return 0;
    if (sourceName[0] == '$')
        return 1;
    name = GlslAllocateSymbolNameForSource(context, canonical, sourceName,
                                           loc);
    if (name == NULL)
        return 0;
    structType = GlslNumericType(GLSL_BASE_STRUCT, 0);
    structType.structName = name;
    decl = GlslNewDecl(context->module, GLSL_STORAGE_NONE,
                       structType, name);
    if (decl == NULL)
        return 0;
    decl->identity = canonical;
    decl->sourceOrdinal = canonical->str.tag;
    GlslSetLoc(&decl->loc, &canonical->str.loc);
    GlslAppendDecl(&context->module->structs, decl);
    if (canonical->str.members == NULL ||
        !GlslCollectMembers(context, canonical->str.members,
                            canonical->str.members->symbols, &decl->members))
    {
        return 0;
    }
    return 1;
} // GlslIRRegisterStruct

/*
 * GlslIRType() - Cg IR canonical type to GlslType.  Mirrors the legacy
 *          mapping but derives everything from the canonical type
 *          object; profile-only features reject here with the existing
 *          6200-family diagnostics.
 */

int GlslIRType(GlslLowerContext *context, Type *source,
                      GlslType *target, const SourceLoc *loc)
{
    GlslDecl *structDecl;
    GlslBase glslBase;
    GlslType elementType;
    GlslType *element;
    int len;
    int rows;
    int cols;

    if (source == NULL || target == NULL)
        return 0;
    if (IsVoid(source)) {
        *target = GlslNumericType(GLSL_BASE_VOID, 0);
        return 1;
    }
    if (GetCategory(source) == TYPE_CATEGORY_INTERFACE) {
        GlslRecordFailureKindAt(context, GLSL_ERROR_UNSUPPORTED_TYPE,
                                "interface", loc);
        return 0;
    }
    if (GetCategory(source) == TYPE_CATEGORY_SAMPLER) {
        /* Canonical language samplers keep the historical texture-object
         * bases; samplerRECT and the deprecated base sampler have no
         * focused GLSL profile spelling and fail profile validation. */
        switch (source->samp.samplerKind) {
        case CG_SAMPLER_1D:
            *target = GlslNumericType(GLSL_BASE_SAMPLER1D, 1);
            return 1;
        case CG_SAMPLER_2D:
            *target = GlslNumericType(GLSL_BASE_SAMPLER2D, 1);
            return 1;
        case CG_SAMPLER_3D:
            *target = GlslNumericType(GLSL_BASE_SAMPLER3D, 1);
            return 1;
        case CG_SAMPLER_CUBE:
            *target = GlslNumericType(GLSL_BASE_SAMPLERCUBE, 1);
            return 1;
        default:
            GlslRecordFailureKindAt(context, GLSL_ERROR_UNSUPPORTED_TYPE,
                                    source->samp.samplerKind ==
                                        CG_SAMPLER_RECT ? "samplerRECT" :
                                                          "sampler",
                                    loc);
            return 0;
        }
    }
    if (CgIsAttribArray(source)) {
        if (context->module->stage != GLSL_STAGE_GEOMETRY ||
            CgAttribArrayExtent(source) == 0 ||
            !GlslIRType(context, CgAttribArrayElement(source),
                        &elementType, loc))
        {
            GlslRecordFailureKindAt(context, GLSL_ERROR_UNSUPPORTED_TYPE,
                                    "AttribArray", loc);
            return 0;
        }
        element = (GlslType *) context->module->alloc(
            context->module->allocArg, sizeof(GlslType));
        if (element == NULL)
            return 0;
        *element = elementType;
        *target = GlslNumericType(GLSL_BASE_VOID, 0);
        target->arraySize = (int) CgAttribArrayExtent(source);
        target->elementType = element;
        return 1;
    }
    if (IsMatrix(source, &cols, &rows)) {
        if (!GlslIRScalarBase(context, GetScalarKind(source), &glslBase,
                              loc))
            return 0;
        if (glslBase != GLSL_BASE_FLOAT) {
            GlslRecordFailureKindAt(context,
                                    GLSL_ERROR_UNSUPPORTED_TYPE,
                                    "matrix", loc);
            return 0;
        }
        if (rows != cols || rows < 2 || rows > 4) {
            char matrixName[32];

            sprintf(matrixName, "float%dx%d", rows, cols);
            GlslRecordFailureKindAt(context,
                                    GLSL_ERROR_NON_SQUARE_MATRIX,
                                    GlslCopyText(context->module,
                                                 matrixName), loc);
            return 0;
        }
        *target = GlslMatrixType(rows);
        return 1;
    }
    if (GetCategory(source) == TYPE_CATEGORY_STRUCT) {
        structDecl = GlslIRFindStruct(context, source);
        if (structDecl == NULL) {
            /* Lazy registration: a struct first seen through another
             * type's element/member walk registers here, mirroring
             * the legacy ensure-before-lower discipline. */
            if (!GlslIRRegisterStruct(context, source, loc))
                return 0;
            structDecl = GlslIRFindStruct(context, source);
            if (structDecl == NULL)
                return 0;
        }
        *target = GlslNumericType(GLSL_BASE_STRUCT, 0);
        target->structName = structDecl->name;
        target->members = structDecl->members;
        return 1;
    }
    if (IsVector(source, &len)) {
        if (!GlslIRScalarBase(context, GetScalarKind(source), &glslBase,
                              loc))
            return 0;
        if (len >= 1 && len <= 4) {
            *target = GlslNumericType(glslBase, len);
            return 1;
        }
        GlslRecordFailureKindAt(context, GLSL_ERROR_UNSUPPORTED_TYPE,
                                "vector", loc);
        return 0;
    }
    if (IsScalar(source)) {
        if (!GlslIRScalarBase(context, GetScalarKind(source), &glslBase,
                              loc))
            return 0;
        *target = GlslNumericType(glslBase, 1);
        return 1;
    }
    if (GetCategory(source) == TYPE_CATEGORY_ARRAY &&
        source->arr.numels != CG_ARRAY_UNSIZED && source->arr.numels > 0 &&
        GlslIRType(context, source->arr.eltype, &elementType, loc))
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
    if (GetCategory(source) == TYPE_CATEGORY_ARRAY) {
        GlslRecordFailureKindAt(context, GLSL_ERROR_UNSUPPORTED_TYPE,
                                "dynamic unsized array", loc);
        return 0;
    }
    return 0;
} // GlslIRType

/*
 * GlslIREnsureType() - Validate one canonical type for GLSL and
 *          register every struct it mentions.  Mirrors the legacy
 *          ensure walk without any scope dependency.
 */

static int GlslIREnsureType(GlslLowerContext *context, Type *type,
                            const SourceLoc *loc)
{
    int len;
    int rows;
    int cols;

    if (type == NULL)
        return 0;
    if (IsVoid(type))
        return 1;
    if (IsMatrix(type, &cols, &rows) || IsVector(type, &len) ||
        GetCategory(type) == TYPE_CATEGORY_SCALAR)
    {
        CgScalarKind kind;

        kind = GetScalarKind(type);
        switch (kind) {
        case CG_SCALAR_FLOAT:
        case CG_SCALAR_CFLOAT:
        case CG_SCALAR_INT:
        case CG_SCALAR_CINT:
        case CG_SCALAR_BOOL:
            return 1;
        default:
            /* half/fixed/double and unsigned widths carry no focused GLSL
             * spelling; profile validation rejects them here. */
            GlslRecordFailureKindAt(context, GLSL_ERROR_UNSUPPORTED_TYPE,
                                    CgScalarKindName(kind), loc);
            return 0;
        }
    }
    if (GetCategory(type) == TYPE_CATEGORY_INTERFACE) {
        GlslRecordFailureKindAt(context, GLSL_ERROR_UNSUPPORTED_TYPE,
                                "interface", loc);
        return 0;
    }
    if (GetCategory(type) == TYPE_CATEGORY_SAMPLER) {
        switch (type->samp.samplerKind) {
        case CG_SAMPLER_1D:
        case CG_SAMPLER_2D:
        case CG_SAMPLER_3D:
        case CG_SAMPLER_CUBE:
            return 1;
        default:
            GlslRecordFailureKindAt(context, GLSL_ERROR_UNSUPPORTED_TYPE,
                                    type->samp.samplerKind ==
                                        CG_SAMPLER_RECT ? "samplerRECT" :
                                                          "sampler",
                                    loc);
            return 0;
        }
    }
    if (CgIsAttribArray(type)) {
        if (context->module->stage != GLSL_STAGE_GEOMETRY ||
            CgAttribArrayExtent(type) == 0)
        {
            GlslRecordFailureKindAt(context, GLSL_ERROR_UNSUPPORTED_TYPE,
                                    "AttribArray", loc);
            return 0;
        }
        return GlslIREnsureType(context,
                                CgAttribArrayElement(type), loc);
    }
    if (GetCategory(type) == TYPE_CATEGORY_ARRAY) {
        if (type->arr.numels == CG_ARRAY_UNSIZED || type->arr.numels <= 0)
        {
            /* Dynamic unsized arrays are outside the focused GLSL storage rules;
             * name the construct instead of a generic fallback. */
            GlslRecordFailureKindAt(context, GLSL_ERROR_UNSUPPORTED_TYPE,
                                    "dynamic unsized array", loc);
            return 0;
        }
        return GlslIREnsureType(context, type->arr.eltype, loc);
    }
    if (GetCategory(type) == TYPE_CATEGORY_STRUCT)
        return GlslIRRegisterStruct(context, type, loc);
    return 0;
} // GlslIREnsureType

int GlslIREnsureTypeAt(GlslLowerContext *context, Type *type,
                              const SourceLoc *loc)
{
    SourceLoc savedLoc;
    int result;

    if (loc == NULL)
        return GlslIREnsureType(context, type, NULL);
    savedLoc = context->statementLoc;
    context->statementLoc = *loc;
    result = GlslIREnsureType(context, type, loc);
    context->statementLoc = savedLoc;
    return result;
} // GlslIREnsureTypeAt

/*
 * GlslIRSamplerPlacementCheck() - Samplers reach a shader only as
 *          global uniforms or as in-qualified formals; a helper formal
 *          (or local) sampler outside the uniform domain keeps failing
 *          with the historical sampler diagnostic because the language
 *          rules deliberately allow plain sampler formals.
 */

int GlslIRSamplerPlacementCheck(GlslLowerContext *context,
                                       const CgIRDecl *decl)
{
    if (decl->symbol == NULL || decl->symbol->type == NULL)
        return 1;
    if (GlslIRIsSamplerValue(decl->symbol->type) &&
        GetDomain(decl->symbol->type) != TYPE_DOMAIN_UNIFORM)
    {
        GlslRecordFailureKindAt(context, GLSL_ERROR_SAMPLER,
                                "samplers must be uniforms",
                                &decl->loc);
        return 0;
    }
    return 1;
} // GlslIRSamplerPlacementCheck

/*
 * GlslIRNewSourceDecl() - One GlslDecl translated from an ordered Cg IR
 *          declaration.  Source symbol identity drives name allocation
 *      exactly as the tree path did; anonymous temporaries fall back to
 *          their canonical type as identity.
 */

static GlslDecl *GlslIRNewSourceDecl(GlslLowerContext *context,
                                     const CgIRDecl *source,
                                     const void *nameSpace)
{
    GlslDecl *decl;
    GlslType type;
    const char *sourceName;
    const char *name;
    const void *identity;

    if (!GlslIRType(context, source->type, &type, &source->loc))
        return NULL;
    sourceName = GetAtomString(atable, source->name);
    if (sourceName == NULL)
        return NULL;
    identity = source->symbol != NULL ? (const void *) source->symbol
                                      : (const void *) source->type;
    if (nameSpace != NULL) {
        name = GlslAllocateScopedSymbolNameForSource(context, nameSpace,
                                                     identity, sourceName,
                                                     &source->loc);
    } else {
        name = GlslAllocateSymbolNameForSource(context, identity,
                                               sourceName, &source->loc);
    }
    if (name == NULL)
        return NULL;
    decl = GlslNewDecl(context->module, GLSL_STORAGE_NONE, type, name);
    if (decl != NULL) {
        decl->identity = identity;
        GlslSetLoc(&decl->loc, &source->loc);
        decl->sourceOrdinal = source->symbol != NULL ?
                              source->symbol->sourceOrdinal : 0;
    }
    return decl;
} // GlslIRNewSourceDecl


/*
 * GlslIRLocalDeclaration() - One block-level declaration: user locals
 *          join the function's locals; synthesized "$" temporaries must
 *          open a group-write pattern that consumes their statements.
 */

int GlslIRLocalDeclaration(GlslLowerContext *context,
                                  const CgIRStmt *stmt,
                                  const CgIRStmt **nextOut, GlslStmt **out)
{
    const char *declName;

    declName = GlslIRDeclNameText(stmt->u.decl);
    if (declName != NULL && declName[0] == '$') {
        /* Reached only when the group-write pattern rejected this
         * synthesized temporary: nothing else may consume it. */
        GlslRecordFailureKindAt(context,
                                GLSL_ERROR_UNSUPPORTED_OPERATION,
                                "matrix selector assignment context",
                                &stmt->loc);
        return 0;
    }
    if (!GlslIRSamplerPlacementCheck(context, stmt->u.decl))
        return 0;
    if (!GlslIREnsureTypeAt(context, stmt->u.decl->type,
                            &stmt->u.decl->loc))
        return 0;
    if (GlslIRAddLocal(context, stmt->u.decl->symbol, stmt->u.decl->type,
                       &stmt->u.decl->loc) == NULL)
    {
        return 0;
    }
    *nextOut = stmt->next;
    return 1;
} // GlslIRLocalDeclaration

/*
 * GlslIREnsureEntryLocals() - Register every entry-local declaration
 *          type ahead of structure sorting; helper types were already
 *          ensured during call collection.
 */

int GlslIREnsureEntryLocals(GlslLowerContext *context,
                                   const CgIRFunction *entry)
{
    const CgIRStmt *stmt;
    const CgIRDecl *decl;

    /* Entry formals first: structure types used only through
     * parameters must exist before structure sorting. */
    for (decl = entry->parameters; decl != NULL; decl = decl->next) {
        if (!GlslIREnsureTypeAt(context, decl->type, &decl->loc))
            return 0;
    }
    if (entry->body == NULL || entry->body->kind != CGIR_STMT_BLOCK)
        return 1;
    for (stmt = entry->body->u.block; stmt != NULL; stmt = stmt->next) {
        if (stmt->kind != CGIR_STMT_DECL)
            break;
        decl = stmt->u.decl;
        {
            const char *localName = GetAtomString(atable, decl->name);

            if (localName != NULL && localName[0] == '$')
                continue;
        }
        if (!GlslIRSamplerPlacementCheck(context, decl))
            return 0;
        if (!GlslIREnsureTypeAt(context, decl->type, &decl->loc))
            return 0;
    }
    return 1;
} // GlslIREnsureEntryLocals

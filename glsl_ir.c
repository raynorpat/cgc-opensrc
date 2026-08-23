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
INCIDENTAL, EXEMPLARY, CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
TO, LOST PROFITS; PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) OR ARISING IN ANY WAY
OUT OF THE USE, REPRODUCTION, MODIFICATION AND/OR DISTRIBUTION OF THE
NVIDIA SOFTWARE, HOWEVER CAUSED AND WHETHER UNDER THEORY OF CONTRACT,
TORT (INCLUDING NEGLIGENCE), STRICT LIABILITY OR OTHERWISE, EVEN IF
NVIDIA HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
\****************************************************************************/
// glsl_ir.c
//

#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include "glsl_ir.h"

static const int defaultNameIdentity = 0;
static const int defaultNameNamespace = 0;

static const char *reservedNames[] = {
    "asm", "attribute", "bool", "break", "bvec2", "bvec3", "bvec4",
    "cast", "class", "const", "continue", "default", "discard", "do",
    "double", "dvec2", "dvec3", "dvec4", "else", "enum", "extern",
    "external", "false", "fixed", "float", "for", "fvec2", "fvec3",
    "fvec4", "goto", "half", "hvec2", "hvec3", "hvec4", "if", "in",
    "inline", "inout", "input", "int", "interface", "invariant", "ivec2",
    "ivec3", "ivec4", "long", "mat2", "mat3", "mat4", "namespace",
    "noinline", "out", "output", "packed", "public", "return",
    "sampler1D", "sampler1DShadow", "sampler2D", "sampler2DRect",
    "sampler2DRectShadow", "sampler2DShadow", "sampler3D",
    "sampler3DRect", "samplerCube", "short", "sizeof", "static",
    "struct", "switch", "template", "this", "true", "typedef", "uniform",
    "union", "unsigned", "using", "varying", "vec2", "vec3", "vec4",
    "void", "volatile", "while"
};

static void *GlslAlloc(GlslModule *module, size_t size)
{
    void *memory;

    if (module == NULL || module->alloc == NULL)
        return NULL;
    memory = (*module->alloc)(module->allocArg, size);
    if (memory != NULL)
        memset(memory, 0, size);
    return memory;
}

static const char *GlslDuplicate(GlslModule *module, const char *string)
{
    char *copy;
    size_t size;

    if (string == NULL)
        return NULL;
    size = strlen(string) + 1;
    copy = (char *) GlslAlloc(module, size);
    if (copy != NULL)
        memcpy(copy, string, size);
    return copy;
}

static GlslName *GlslFindName(const GlslModule *module,
    const void *nameSpace, const void *identity, const char *source)
{
    GlslName *current;

    for (current = module->names; current != NULL; current = current->next) {
        if (current->nameSpace == nameSpace &&
            current->identity == identity &&
            !strcmp(current->source, source))
        {
            return current;
        }
    }
    return NULL;
}

static int GlslDecimalLength(int value)
{
    int length;

    length = 1;
    while (value >= 10) {
        value /= 10;
        length++;
    }
    return length;
}

static void GlslWriteDecimal(char *string, int value)
{
    int length;

    length = GlslDecimalLength(value);
    string += length;
    *string = '\0';
    while (length > 0) {
        *--string = (char) ('0' + value % 10);
        value /= 10;
        length--;
    }
}

static const char *GlslLegalizeName(GlslModule *module, const char *source)
{
    const char *input;
    char *name;
    char *output;
    int previousUnderscore;
    int reserved;
    size_t size;

    if (source == NULL)
        return NULL;
    reserved = GlslIsReservedName(source);
    size = strlen(source) + 4;
    name = (char *) GlslAlloc(module, size);
    if (name == NULL)
        return NULL;
    input = source;
    output = name;
    if (reserved) {
        memcpy(output, "cg_", 3);
        output += 3;
        previousUnderscore = 1;
    } else {
        previousUnderscore = 0;
    }
    while (*input != '\0') {
        if (*input != '_' || !previousUnderscore)
            *output++ = *input;
        previousUnderscore = *input == '_';
        input++;
    }
    *output = '\0';
    if (name[0] == '\0')
        strcpy(name, "cg_");
    return name;
}

static int GlslSuffixValue(const char *name, const char *base, int *value)
{
    const char *current;
    int digit;
    int result;

    current = name;
    if (!strcmp(base, "gl")) {
        if (strncmp(current, "cg_gl_", 6))
            return 0;
        current += 6;
    } else {
        while (*base != '\0' && *current == *base) {
            base++;
            current++;
        }
        if (*base != '\0')
            return 0;
        if (current > name && current[-1] != '_') {
            if (*current != '_')
                return 0;
            current++;
        }
    }
    if (*current < '1' || *current > '9')
        return 0;
    result = 0;
    while (*current != '\0') {
        if (*current < '0' || *current > '9')
            return 0;
        digit = *current - '0';
        if (result > (INT_MAX - digit) / 10)
            return 0;
        result = result * 10 + digit;
        current++;
    }
    *value = result;
    return 1;
}

static const char *GlslBuildSuffixedName(GlslModule *module,
    const char *base, int suffix)
{
    const char *prefix;
    char *name;
    size_t length;
    int separator;

    if (!strcmp(base, "gl")) {
        prefix = "cg_gl_";
        separator = 0;
    } else {
        prefix = base;
        separator = base[strlen(base) - 1] != '_';
    }
    length = strlen(prefix);
    name = (char *) GlslAlloc(module,
        length + separator + GlslDecimalLength(suffix) + 1);
    if (name == NULL)
        return NULL;
    memcpy(name, prefix, length);
    if (separator) {
        name[length] = '_';
        GlslWriteDecimal(name + length + 1, suffix);
    } else {
        GlslWriteDecimal(name + length, suffix);
    }
    return name;
}

static const char *GlslAllocate(GlslModule *module, const void *nameSpace,
    const void *identity, const char *source)
{
    const char *base;
    const char *emitted;
    const char *sourceCopy;
    GlslName *name;
    GlslName *current;
    unsigned char *occupied;
    size_t nameCount;
    int inUse;
    int limit;
    int suffix;
    int suffixValue;

    if (module == NULL || source == NULL)
        return NULL;
    base = GlslLegalizeName(module, source);
    if (base == NULL)
        return NULL;
    inUse = 0;
    nameCount = 0;
    for (current = module->names; current != NULL; current = current->next) {
        if (current->nameSpace != nameSpace)
            continue;
        if (!strcmp(current->emitted, base))
            inUse = 1;
        if (nameCount == (size_t) -1)
            return NULL;
        nameCount++;
    }
    if (!inUse) {
        emitted = base;
    } else {
        if (nameCount > (size_t) INT_MAX - 1 ||
            nameCount > (size_t) -1 - 2) {
            module->errorKind = GLSL_ERROR_NAME_COLLISION;
            module->errorReason = source;
            return NULL;
        }
        limit = (int) nameCount + 1;
        occupied = (unsigned char *) malloc(nameCount + 2);
        if (occupied == NULL)
            return NULL;
        memset(occupied, 0, nameCount + 2);
        for (current = module->names; current != NULL; current = current->next) {
            if (current->nameSpace == nameSpace &&
                GlslSuffixValue(current->emitted, base, &suffixValue) &&
                suffixValue <= limit)
                occupied[suffixValue] = 1;
        }
        for (suffix = 1; suffix <= limit; suffix++) {
            if (!occupied[suffix])
                break;
        }
        if (suffix > limit) {
            free(occupied);
            module->errorKind = GLSL_ERROR_NAME_COLLISION;
            module->errorReason = source;
            return NULL;
        }
        free(occupied);
        emitted = GlslBuildSuffixedName(module, base, suffix);
        if (emitted == NULL)
            return NULL;
    }
    name = (GlslName *) GlslAlloc(module, sizeof(GlslName));
    if (name == NULL)
        return NULL;
    sourceCopy = GlslDuplicate(module, source);
    if (sourceCopy == NULL)
        return NULL;
    name->nameSpace = nameSpace;
    name->identity = identity;
    name->source = sourceCopy;
    name->emitted = emitted;
    name->next = module->names;
    module->names = name;
    return emitted;
}

void GlslInitModule(GlslModule *module, GlslStage stage, GlslAllocFn alloc,
    void *allocArg)
{
    if (module == NULL)
        return;
    memset(module, 0, sizeof(GlslModule));
    module->stage = stage;
    module->alloc = alloc;
    module->allocArg = allocArg;
}

const char *GlslAllocateName(GlslModule *module, const char *source)
{
    GlslName *name;

    if (module == NULL || source == NULL)
        return NULL;
    name = GlslFindName(module, &defaultNameNamespace,
                        &defaultNameIdentity, source);
    if (name != NULL)
        return name->emitted;
    return GlslAllocate(module, &defaultNameNamespace,
                        &defaultNameIdentity, source);
}

const char *GlslAllocateSymbolName(GlslModule *module, const void *identity,
    const char *source)
{
    return GlslAllocateScopedSymbolName(module, &defaultNameNamespace,
                                        identity, source);
}

const char *GlslAllocateScopedSymbolName(GlslModule *module,
    const void *nameSpace, const void *identity, const char *source)
{
    GlslName *name;

    if (module == NULL || nameSpace == NULL || source == NULL)
        return NULL;
    if (identity == NULL) {
        name = GlslFindName(module, nameSpace, &defaultNameIdentity,
                            source);
        if (name != NULL)
            return name->emitted;
        return GlslAllocate(module, nameSpace, &defaultNameIdentity,
                            source);
    }
    for (name = module->names; name != NULL; name = name->next) {
        if (name->nameSpace == nameSpace && name->identity == identity)
            return name->emitted;
    }
    return GlslAllocate(module, nameSpace, identity, source);
}

const char *GlslAllocateDistinctName(GlslModule *module, const char *source)
{
    return GlslAllocate(module, &defaultNameNamespace, NULL, source);
}

GlslType GlslNumericType(GlslBase base, int len)
{
    GlslType type;

    memset(&type, 0, sizeof(GlslType));
    type.base = base;
    type.len = len;
    return type;
}

GlslType GlslMatrixType(int size)
{
    GlslType type;

    memset(&type, 0, sizeof(GlslType));
    type.base = GLSL_BASE_FLOAT;
    type.rows = size;
    type.cols = size;
    return type;
}

const char *GlslTypeName(const GlslType *type)
{
    static const char *floatNames[] = { "float", "vec2", "vec3", "vec4" };
    static const char *intNames[] = { "int", "ivec2", "ivec3", "ivec4" };
    static const char *boolNames[] = { "bool", "bvec2", "bvec3", "bvec4" };
    static const char *matrixNames[] = { "mat2", "mat3", "mat4" };

    if (type == NULL || type->arraySize < 0)
        return NULL;
    if (type->elementType != NULL) {
        if (type->elementType == type || type->arraySize <= 0 ||
            type->base != GLSL_BASE_VOID || type->len != 0 ||
            type->rows != 0 || type->cols != 0 ||
            type->structName != NULL || type->members != NULL)
        {
            return NULL;
        }
        return GlslTypeName(type->elementType);
    }
    if (type->base != GLSL_BASE_STRUCT &&
        (type->structName != NULL || type->members != NULL))
        return NULL;
    switch (type->base) {
    case GLSL_BASE_VOID:
        if (type->len == 0 && type->rows == 0 && type->cols == 0 &&
            type->arraySize == 0)
            return "void";
        break;
    case GLSL_BASE_FLOAT:
        if (type->rows == 0 && type->cols == 0 &&
            type->len >= 1 && type->len <= 4)
            return floatNames[type->len - 1];
        if (type->len == 0 && type->rows == type->cols &&
            type->rows >= 2 && type->rows <= 4)
            return matrixNames[type->rows - 2];
        break;
    case GLSL_BASE_INT:
        if (type->rows == 0 && type->cols == 0 &&
            type->len >= 1 && type->len <= 4)
            return intNames[type->len - 1];
        break;
    case GLSL_BASE_BOOL:
        if (type->rows == 0 && type->cols == 0 &&
            type->len >= 1 && type->len <= 4)
            return boolNames[type->len - 1];
        break;
    case GLSL_BASE_SAMPLER1D:
        if (type->len == 1 && type->rows == 0 && type->cols == 0)
            return "sampler1D";
        break;
    case GLSL_BASE_SAMPLER2D:
        if (type->len == 1 && type->rows == 0 && type->cols == 0)
            return "sampler2D";
        break;
    case GLSL_BASE_SAMPLER3D:
        if (type->len == 1 && type->rows == 0 && type->cols == 0)
            return "sampler3D";
        break;
    case GLSL_BASE_SAMPLERCUBE:
        if (type->len == 1 && type->rows == 0 && type->cols == 0)
            return "samplerCube";
        break;
    case GLSL_BASE_STRUCT:
        if (type->len == 0 && type->rows == 0 && type->cols == 0 &&
            type->structName != NULL && type->structName[0] != '\0')
            return type->structName;
        break;
    }
    return NULL;
}

static int GlslBuiltinNumericType(const GlslType *type, int len)
{
    return type != NULL && type->base == GLSL_BASE_FLOAT &&
           type->len == len && type->rows == 0 && type->cols == 0 &&
           type->arraySize == 0 && type->structName == NULL &&
           type->elementType == NULL && type->members == NULL;
}

static int GlslBuiltinMatrixType(const GlslType *type, int size)
{
    return type != NULL && type->base == GLSL_BASE_FLOAT &&
           type->len == 0 && type->rows == size && type->cols == size &&
           type->arraySize == 0 && type->structName == NULL &&
           type->elementType == NULL && type->members == NULL;
}

static int GlslBuiltinSamplerType(const GlslType *type, GlslBase base)
{
    return type != NULL && type->base == base && type->len == 1 &&
           type->rows == 0 && type->cols == 0 && type->arraySize == 0 &&
           type->structName == NULL && type->elementType == NULL &&
           type->members == NULL;
}

static GlslBuiltin GlslBuiltinFromName(const char *name)
{
    static const struct {
        const char *name;
        GlslBuiltin builtin;
    } names[] = {
        { "mul", GLSL_BUILTIN_MUL },
        { "dot", GLSL_BUILTIN_DOT },
        { "cross", GLSL_BUILTIN_CROSS },
        { "normalize", GLSL_BUILTIN_NORMALIZE },
        { "reflect", GLSL_BUILTIN_REFLECT },
        { "refract", GLSL_BUILTIN_REFRACT },
        { "length", GLSL_BUILTIN_LENGTH },
        { "distance", GLSL_BUILTIN_DISTANCE },
        { "min", GLSL_BUILTIN_MIN },
        { "max", GLSL_BUILTIN_MAX },
        { "clamp", GLSL_BUILTIN_CLAMP },
        { "abs", GLSL_BUILTIN_ABS },
        { "sign", GLSL_BUILTIN_SIGN },
        { "floor", GLSL_BUILTIN_FLOOR },
        { "ceil", GLSL_BUILTIN_CEIL },
        { "sqrt", GLSL_BUILTIN_SQRT },
        { "exp", GLSL_BUILTIN_EXP },
        { "exp2", GLSL_BUILTIN_EXP2 },
        { "log", GLSL_BUILTIN_LOG },
        { "log2", GLSL_BUILTIN_LOG2 },
        { "sin", GLSL_BUILTIN_SIN },
        { "cos", GLSL_BUILTIN_COS },
        { "tan", GLSL_BUILTIN_TAN },
        { "asin", GLSL_BUILTIN_ASIN },
        { "acos", GLSL_BUILTIN_ACOS },
        { "atan", GLSL_BUILTIN_ATAN },
        { "rsqrt", GLSL_BUILTIN_RSQRT },
        { "lerp", GLSL_BUILTIN_LERP },
        { "frac", GLSL_BUILTIN_FRAC },
        { "saturate", GLSL_BUILTIN_SATURATE },
        { "tex1D", GLSL_BUILTIN_TEX1D },
        { "tex2D", GLSL_BUILTIN_TEX2D },
        { "tex3D", GLSL_BUILTIN_TEX3D },
        { "texCUBE", GLSL_BUILTIN_TEXCUBE }
    };
    int i;

    if (name == NULL)
        return GLSL_BUILTIN_NONE;
    for (i = 0; i < (int) (sizeof(names) / sizeof(names[0])); i++) {
        if (!strcmp(name, names[i].name))
            return names[i].builtin;
    }
    return GLSL_BUILTIN_NONE;
}

int GlslIsBuiltinName(const char *name)
{
    return GlslBuiltinFromName(name) != GLSL_BUILTIN_NONE;
}

static int GlslBuiltinUnary(GlslBuiltin builtin)
{
    switch (builtin) {
    case GLSL_BUILTIN_NORMALIZE:
    case GLSL_BUILTIN_ABS:
    case GLSL_BUILTIN_SIGN:
    case GLSL_BUILTIN_FLOOR:
    case GLSL_BUILTIN_CEIL:
    case GLSL_BUILTIN_SQRT:
    case GLSL_BUILTIN_EXP:
    case GLSL_BUILTIN_EXP2:
    case GLSL_BUILTIN_LOG:
    case GLSL_BUILTIN_LOG2:
    case GLSL_BUILTIN_SIN:
    case GLSL_BUILTIN_COS:
    case GLSL_BUILTIN_TAN:
    case GLSL_BUILTIN_ASIN:
    case GLSL_BUILTIN_ACOS:
    case GLSL_BUILTIN_ATAN:
    case GLSL_BUILTIN_RSQRT:
    case GLSL_BUILTIN_FRAC:
    case GLSL_BUILTIN_SATURATE:
        return 1;
    default:
        return 0;
    }
}

GlslBuiltin GlslLookupBuiltin(const char *name, const GlslType *result,
                              const GlslType *params, int paramCount)
{
    GlslBuiltin builtin;
    int len;

    builtin = GlslBuiltinFromName(name);
    if (builtin == GLSL_BUILTIN_NONE || result == NULL ||
        params == NULL || paramCount < 1)
    {
        return GLSL_BUILTIN_NONE;
    }
    for (len = 1; len <= 4; len++) {
        if (!GlslBuiltinNumericType(result, len))
            continue;
        if (GlslBuiltinUnary(builtin) && paramCount == 1 &&
            GlslBuiltinNumericType(&params[0], len))
        {
            return builtin;
        }
        if ((builtin == GLSL_BUILTIN_MIN ||
             builtin == GLSL_BUILTIN_MAX) && paramCount == 2 &&
            GlslBuiltinNumericType(&params[0], len) &&
            GlslBuiltinNumericType(&params[1], len))
        {
            return builtin;
        }
        if (builtin == GLSL_BUILTIN_CLAMP && paramCount == 3 &&
            GlslBuiltinNumericType(&params[0], len) &&
            ((GlslBuiltinNumericType(&params[1], len) &&
              GlslBuiltinNumericType(&params[2], len)) ||
             (len > 1 && GlslBuiltinNumericType(&params[1], 1) &&
              GlslBuiltinNumericType(&params[2], 1))))
        {
            return builtin;
        }
        if (builtin == GLSL_BUILTIN_LERP && paramCount == 3 &&
            GlslBuiltinNumericType(&params[0], len) &&
            GlslBuiltinNumericType(&params[1], len) &&
            (GlslBuiltinNumericType(&params[2], len) ||
             (len > 1 && GlslBuiltinNumericType(&params[2], 1))))
        {
            return builtin;
        }
        if (builtin == GLSL_BUILTIN_REFLECT && paramCount == 2 &&
            GlslBuiltinNumericType(&params[0], len) &&
            GlslBuiltinNumericType(&params[1], len))
        {
            return builtin;
        }
        if (builtin == GLSL_BUILTIN_REFRACT && paramCount == 3 &&
            GlslBuiltinNumericType(&params[0], len) &&
            GlslBuiltinNumericType(&params[1], len) &&
            GlslBuiltinNumericType(&params[2], 1))
        {
            return builtin;
        }
        if (builtin == GLSL_BUILTIN_MUL && len >= 2 &&
            paramCount == 2 && GlslBuiltinMatrixType(&params[0], len) &&
            GlslBuiltinNumericType(&params[1], len))
        {
            return builtin;
        }
        if (builtin == GLSL_BUILTIN_CROSS && len == 3 &&
            paramCount == 2 && GlslBuiltinNumericType(&params[0], 3) &&
            GlslBuiltinNumericType(&params[1], 3))
        {
            return builtin;
        }
    }
    if ((builtin == GLSL_BUILTIN_DOT ||
         builtin == GLSL_BUILTIN_DISTANCE) && paramCount == 2 &&
        GlslBuiltinNumericType(result, 1))
    {
        for (len = 1; len <= 4; len++) {
            if (GlslBuiltinNumericType(&params[0], len) &&
                GlslBuiltinNumericType(&params[1], len)) return builtin;
        }
    }
    if (builtin == GLSL_BUILTIN_LENGTH && paramCount == 1 &&
        GlslBuiltinNumericType(result, 1))
    {
        for (len = 1; len <= 4; len++) {
            if (GlslBuiltinNumericType(&params[0], len)) return builtin;
        }
    }
    if (paramCount == 2 && GlslBuiltinNumericType(result, 4)) {
        switch (builtin) {
        case GLSL_BUILTIN_TEX1D:
            if (GlslBuiltinSamplerType(&params[0], GLSL_BASE_SAMPLER1D) &&
                GlslBuiltinNumericType(&params[1], 1)) return builtin;
            break;
        case GLSL_BUILTIN_TEX2D:
            if (GlslBuiltinSamplerType(&params[0], GLSL_BASE_SAMPLER2D) &&
                GlslBuiltinNumericType(&params[1], 2)) return builtin;
            break;
        case GLSL_BUILTIN_TEX3D:
            if (GlslBuiltinSamplerType(&params[0], GLSL_BASE_SAMPLER3D) &&
                GlslBuiltinNumericType(&params[1], 3)) return builtin;
            break;
        case GLSL_BUILTIN_TEXCUBE:
            if (GlslBuiltinSamplerType(&params[0], GLSL_BASE_SAMPLERCUBE) &&
                GlslBuiltinNumericType(&params[1], 3)) return builtin;
            break;
        default:
            break;
        }
    }
    return GLSL_BUILTIN_NONE;
}

const char *GlslBuiltinSpelling(GlslBuiltin builtin)
{
    static const char *spellings[] = {
        NULL,
        "mul", "dot", "cross", "normalize", "reflect", "refract",
        "length", "distance", "min", "max", "clamp", "abs", "sign",
        "floor", "ceil", "sqrt", "exp", "exp2", "log", "log2",
        "sin", "cos", "tan", "asin", "acos", "atan", "inversesqrt",
        "mix", "fract", "clamp", "texture1D", "texture2D",
        "texture3D", "textureCube"
    };

    if (builtin <= GLSL_BUILTIN_NONE || builtin > GLSL_BUILTIN_TEXCUBE)
        return NULL;
    return spellings[builtin];
}

typedef struct GlslTypeCountFrame_Rec {
    const struct GlslTypeCountFrame_Rec *parent;
    const void *identity;
    int kind;
} GlslTypeCountFrame;

static int GlslTypeCountIsRecursive(const GlslTypeCountFrame *frame,
    const void *identity, int kind)
{
    for (; frame != NULL; frame = frame->parent) {
        if (frame->kind == kind && frame->identity == identity)
            return 1;
    }
    return 0;
}

static int GlslTypeComponentCountInternal(const GlslType *type,
    const GlslTypeCountFrame *parent)
{
    GlslTypeCountFrame frame;
    const GlslDecl *member;
    int elementCount;
    int memberCount;
    int total;

    if (type == NULL)
        return 0;
    if (type->elementType != NULL) {
        if (type->arraySize <= 0 ||
            GlslTypeCountIsRecursive(parent, type, 1))
        {
            return 0;
        }
        frame.parent = parent;
        frame.identity = type;
        frame.kind = 1;
        elementCount = GlslTypeComponentCountInternal(type->elementType,
                                                       &frame);
        if (elementCount <= 0 || elementCount > INT_MAX / type->arraySize)
            return 0;
        return elementCount * type->arraySize;
    }
    if (type->arraySize > 0 && type->structName == NULL &&
        type->members == NULL && type->rows == 0 && type->cols == 0 &&
        (type->base == GLSL_BASE_FLOAT || type->base == GLSL_BASE_INT ||
         type->base == GLSL_BASE_BOOL) &&
        type->len >= 1 && type->len <= 4)
    {
        if (type->len > INT_MAX / type->arraySize)
            return 0;
        return type->len * type->arraySize;
    }
    if (type->arraySize != 0 || type->base == GLSL_BASE_VOID ||
        type->base == GLSL_BASE_SAMPLER1D ||
        type->base == GLSL_BASE_SAMPLER2D ||
        type->base == GLSL_BASE_SAMPLER3D ||
        type->base == GLSL_BASE_SAMPLERCUBE)
    {
        return 0;
    }
    if (type->base == GLSL_BASE_STRUCT) {
        if (type->structName == NULL || type->members == NULL ||
            GlslTypeCountIsRecursive(parent, type->members, 2))
        {
            return 0;
        }
        frame.parent = parent;
        frame.identity = type->members;
        frame.kind = 2;
        total = 0;
        for (member = type->members; member != NULL; member = member->next) {
            memberCount = GlslTypeComponentCountInternal(&member->type,
                                                          &frame);
            if (memberCount <= 0 || total > INT_MAX - memberCount)
                return 0;
            total += memberCount;
        }
        return total;
    }
    if (type->structName != NULL || type->members != NULL)
        return 0;
    if (type->rows != 0 || type->cols != 0) {
        if (type->base != GLSL_BASE_FLOAT || type->rows < 2 ||
            type->rows != type->cols || type->rows > 4) return 0;
        return type->rows * type->cols;
    }
    if ((type->base == GLSL_BASE_FLOAT || type->base == GLSL_BASE_INT ||
         type->base == GLSL_BASE_BOOL) && type->len >= 1 && type->len <= 4)
    {
        return type->len;
    }
    return 0;
}

int GlslTypeComponentCount(const GlslType *type)
{
    return GlslTypeComponentCountInternal(type, NULL);
}

int GlslParseSamplerUnit(const char *text, int *unit)
{
    int digit;
    int value;

    if (text == NULL || unit == NULL || text[0] == '\0')
        return 0;
    if (text[0] == '0') {
        if (text[1] != '\0')
            return 0;
        *unit = 0;
        return 1;
    }
    if (text[0] < '1' || text[0] > '9')
        return 0;
    value = 0;
    for (; *text != '\0'; text++) {
        if (*text < '0' || *text > '9')
            return 0;
        digit = *text - '0';
        if (value > (INT_MAX - digit) / 10)
            return 0;
        value = value * 10 + digit;
    }
    *unit = value;
    return 1;
}

int GlslSamplerUnitMatches(const char *text, int unit)
{
    int parsedUnit;

    return GlslParseSamplerUnit(text, &parsedUnit) && parsedUnit == unit;
}

int GlslIsReservedName(const char *name)
{
    int high;
    int low;
    int middle;
    int comparison;

    if (name == NULL)
        return 1;
    if (name[0] == 'g' && name[1] == 'l' && name[2] == '_')
        return 1;
    if (strstr(name, "__") != NULL)
        return 1;
    low = 0;
    high = (int) (sizeof(reservedNames) / sizeof(reservedNames[0])) - 1;
    while (low <= high) {
        middle = low + (high - low) / 2;
        comparison = strcmp(name, reservedNames[middle]);
        if (comparison == 0)
            return 1;
        if (comparison < 0)
            high = middle - 1;
        else
            low = middle + 1;
    }
    return 0;
}

int GlslReservedNameCount(void)
{
    return (int) (sizeof(reservedNames) / sizeof(reservedNames[0]));
}

const char *GlslReservedNameAt(int index)
{
    if (index < 0 || index >= GlslReservedNameCount())
        return NULL;
    return reservedNames[index];
}

GlslDecl *GlslNewDecl(GlslModule *module, GlslStorage storage, GlslType type,
    const char *name)
{
    GlslDecl *decl;

    decl = (GlslDecl *) GlslAlloc(module, sizeof(GlslDecl));
    if (decl != NULL) {
        decl->storage = storage;
        decl->type = type;
        decl->name = name;
        decl->sourceOrdinal = 0;
    }
    return decl;
}

GlslExpr *GlslNewExpr(GlslModule *module, GlslExprKind kind, GlslType type)
{
    GlslExpr *expr;

    expr = (GlslExpr *) GlslAlloc(module, sizeof(GlslExpr));
    if (expr != NULL) {
        expr->kind = kind;
        expr->type = type;
    }
    return expr;
}

GlslStmt *GlslNewStmt(GlslModule *module, GlslStmtKind kind)
{
    GlslStmt *stmt;

    stmt = (GlslStmt *) GlslAlloc(module, sizeof(GlslStmt));
    if (stmt != NULL)
        stmt->kind = kind;
    return stmt;
}

GlslFunction *GlslNewFunction(GlslModule *module, GlslType result,
    const char *name)
{
    GlslFunction *function;

    function = (GlslFunction *) GlslAlloc(module, sizeof(GlslFunction));
    if (function != NULL) {
        function->result = result;
        function->name = name;
    }
    return function;
}

GlslBinding *GlslNewBinding(GlslModule *module, GlslStorage storage,
    const char *name, const char *semantic)
{
    GlslBinding *binding;

    binding = (GlslBinding *) GlslAlloc(module, sizeof(GlslBinding));
    if (binding != NULL) {
        binding->storage = storage;
        binding->name = name;
        binding->semantic = semantic;
        binding->sourceOrdinal = 0;
    }
    return binding;
}

void GlslAppendDecl(GlslDecl **list, GlslDecl *decl)
{
    GlslDecl *current;

    if (list == NULL || decl == NULL)
        return;
    if (*list == NULL) {
        *list = decl;
        return;
    }
    for (current = *list; current->next != NULL; current = current->next)
        ;
    current->next = decl;
}

void GlslAppendStmt(GlslStmt **list, GlslStmt *stmt)
{
    GlslStmt *current;

    if (list == NULL || stmt == NULL)
        return;
    if (*list == NULL) {
        *list = stmt;
        return;
    }
    for (current = *list; current->next != NULL; current = current->next)
        ;
    current->next = stmt;
}

void GlslAppendFunction(GlslFunction **list, GlslFunction *function)
{
    GlslFunction *current;

    if (list == NULL || function == NULL)
        return;
    if (*list == NULL) {
        *list = function;
        return;
    }
    for (current = *list; current->next != NULL; current = current->next)
        ;
    current->next = function;
}

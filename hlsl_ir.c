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
// hlsl_ir.c
//

#include <limits.h>
#include <stdlib.h>
#include <string.h>

#include "hlsl_ir.h"

static const int defaultNameIdentity = 0;
static const int defaultNameNamespace = 0;

/* Cg and DirectX 9 HLSL keywords, type names, effect-language words,
 * and names owned by the generated entry-point ABI. */
static const char *reservedNames[] = {
    "AttribArray", "LINE", "LINE_ADJ", "LINE_OUT", "POINT", "POINT_OUT",
    "PixelShader", "TRIANGLE", "TRIANGLE_ADJ", "TRIANGLE_OUT",
    "VertexShader", "__internal", "__packed",
    "asm", "asm_fragment", "attribute", "auto", "bool", "break", "case",
    "cast", "catch", "const_cast",
    "centroid", "char", "class", "column_major", "compile",
    "compile_fragment", "const", "continue", "default", "discard",
    "decl", "delete", "do", "double", "dword", "dynamic_cast", "else",
    "emit", "enum", "explicit", "export", "extern", "external", "false",
    "fixed", "float", "for", "foreach",
    "friend", "get", "goto", "half", "if", "in", "inline", "inout",
    "input", "int", "interface", "invariant", "is", "long", "main",
    "matrix", "mutable", "namespace", "new", "noinline", "null",
    "nointerpolation",
    "operator", "out", "output", "packoffset", "packed", "pass",
    "pixelfragment", "pixelshader", "precise", "private", "protected",
    "public", "ref",
    "register", "reinterpret_cast", "return", "row_major", "sampler",
    "sampler1D",
    "sampler2D", "sampler3D", "samplerCUBE", "samplerCube",
    "samplerRECT", "sampler_state", "set", "shared", "short", "signed",
    "sizeof", "snorm", "stateblock", "stateblock_state", "static",
    "static_cast", "string", "struct", "switch", "technique", "technique10",
    "technique11", "template", "texture", "texture1D", "texture2D",
    "texture3D", "textureCUBE", "textureCube", "textureRECT", "this",
    "throw", "true", "try", "typedef", "typename", "uchar", "uint",
    "ulong", "uniform", "union", "unorm", "unsigned", "ushort", "using",
    "varying", "vector", "vertexfragment", "vertexshader", "virtual",
    "void", "volatile", "while", "yield",
    "AppendStructuredBuffer", "BlendState", "Buffer",
    "ByteAddressBuffer", "cbuffer", "CompileShader", "ComputeShader",
    "ConsumeStructuredBuffer", "DepthStencilState", "DepthStencilView",
    "DomainShader", "fxgroup", "GeometryShader", "groupshared",
    "Hullshader", "HullShader", "InputPatch", "line", "lineadj", "linear",
    "LineStream", "min16float", "min10float", "min16int", "min12int",
    "min16uint", "noperspective", "NULL", "OutputPatch", "point",
    "PointStream", "RasterizerState", "RenderTargetView", "RWBuffer",
    "RWByteAddressBuffer", "RWStructuredBuffer", "RWTexture1D",
    "RWTexture1DArray", "RWTexture2D", "RWTexture2DArray", "RWTexture3D",
    "sample", "SamplerState", "SamplerComparisonState", "StructuredBuffer",
    "tbuffer", "Texture1D", "Texture1DArray", "Texture2D",
    "Texture2DArray", "Texture2DMS", "Texture2DMSArray", "Texture3D",
    "TextureCube", "TextureCubeArray", "triangle", "triangleadj",
    "TriangleStream", "DWORD", "FLOAT", "VECTOR", "MATRIX", "STRING",
    "TEXTURE", "PIXELSHADER", "VERTEXSHADER"
};

static const char *reservedTypeBases[] = {
    "bool", "cfloat", "char", "cint", "double", "dword", "fixed",
    "float", "half", "int", "long", "min10float", "min16float",
    "min12int", "min16int", "min16uint", "short", "uchar", "uint",
    "ulong", "ushort"
};

static void *HlslAlloc(HlslModule *module, size_t size)
{
    void *memory;

    if (module == NULL || module->alloc == NULL)
        return NULL;
    memory = (*module->alloc)(module->allocArg, size);
    if (memory != NULL)
        memset(memory, 0, size);
    return memory;
}

static const char *HlslDuplicate(HlslModule *module, const char *string)
{
    char *copy;
    size_t size;

    if (string == NULL)
        return NULL;
    size = strlen(string) + 1;
    copy = (char *) HlslAlloc(module, size);
    if (copy != NULL)
        memcpy(copy, string, size);
    return copy;
}

static int HlslDecimalLength(int value)
{
    int length;

    length = 1;
    while (value >= 10) {
        value /= 10;
        length++;
    }
    return length;
}

static void HlslWriteDecimal(char *string, int value)
{
    int length;

    length = HlslDecimalLength(value);
    string += length;
    *string = '\0';
    while (length > 0) {
        *--string = (char) ('0' + value % 10);
        value /= 10;
        length--;
    }
}

static HlslName *HlslFindName(const HlslModule *module,
    const void *nameSpace, const void *identity, const char *source)
{
    HlslName *name;

    for (name = module->names; name != NULL; name = name->next) {
        if (name->nameSpace == nameSpace && name->identity == identity &&
            !strcmp(name->source, source))
        {
            return name;
        }
    }
    return NULL;
}

static const char *HlslLegalizeName(HlslModule *module, const char *source)
{
    char *name;
    size_t length;

    if (source == NULL)
        return NULL;
    length = strlen(source);
    name = (char *) HlslAlloc(module, length + 4);
    if (name == NULL)
        return NULL;
    if (HlslIsReservedName(source) || !strncmp(source, "cg_", 3)) {
        memcpy(name, "cg_", 3);
        memcpy(name + 3, source, length + 1);
    } else if (length == 0) {
        memcpy(name, "cg_", 4);
    } else {
        memcpy(name, source, length + 1);
    }
    return name;
}

static int HlslSuffixValue(const char *name, const char *base, int *value)
{
    const char *current;
    int digit;
    int result;
    size_t length;

    length = strlen(base);
    if (strncmp(name, base, length) || name[length] != '_')
        return 0;
    current = name + length + 1;
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

static const char *HlslBuildSuffixedName(HlslModule *module,
    const char *base, int suffix)
{
    char *name;
    size_t length;

    length = strlen(base);
    name = (char *) HlslAlloc(module,
        length + 1 + HlslDecimalLength(suffix) + 1);
    if (name == NULL)
        return NULL;
    memcpy(name, base, length);
    name[length] = '_';
    HlslWriteDecimal(name + length + 1, suffix);
    return name;
}

static const char *HlslRaiseNameCollision(HlslModule *module,
    const char *source)
{
    HlslFail(module, HLSL_ERROR_NAME_COLLISION, NULL, source);
    return NULL;
}

static const char *HlslAllocate(HlslModule *module, const void *nameSpace,
    const void *identity, const char *source, int generated)
{
    const char *base;
    const char *emitted;
    const char *sourceCopy;
    HlslName *name;
    HlslName *current;
    unsigned char *occupied;
    size_t nameCount;
    int baseInUse;
    int limit;
    int suffix;
    int suffixValue;

    if (module == NULL || nameSpace == NULL || source == NULL)
        return NULL;
    base = generated ? HlslDuplicate(module, source) :
                       HlslLegalizeName(module, source);
    if (base == NULL)
        return NULL;
    baseInUse = 0;
    nameCount = 0;
    for (current = module->names; current != NULL; current = current->next) {
        if (current->nameSpace != nameSpace)
            continue;
        if (!strcmp(current->emitted, base))
            baseInUse = 1;
        if (nameCount == (size_t) -1)
            return HlslRaiseNameCollision(module, source);
        nameCount++;
    }
    if (!baseInUse) {
        emitted = base;
    } else {
        if (nameCount > (size_t) INT_MAX - 1 ||
            nameCount > (size_t) -1 - 2)
        {
            return HlslRaiseNameCollision(module, source);
        }
        limit = (int) nameCount + 1;
        occupied = (unsigned char *) malloc(nameCount + 2);
        if (occupied == NULL)
            return NULL;
        memset(occupied, 0, nameCount + 2);
        for (current = module->names; current != NULL;
             current = current->next)
        {
            if (current->nameSpace == nameSpace &&
                HlslSuffixValue(current->emitted, base, &suffixValue) &&
                suffixValue <= limit)
            {
                occupied[suffixValue] = 1;
            }
        }
        for (suffix = 1; suffix <= limit; suffix++) {
            if (!occupied[suffix])
                break;
        }
        free(occupied);
        if (suffix > limit)
            return HlslRaiseNameCollision(module, source);
        emitted = HlslBuildSuffixedName(module, base, suffix);
        if (emitted == NULL)
            return NULL;
    }
    name = (HlslName *) HlslAlloc(module, sizeof(HlslName));
    if (name == NULL)
        return NULL;
    sourceCopy = HlslDuplicate(module, source);
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

void HlslInitModule(HlslModule *module, HlslStage stage, HlslAllocFn alloc,
    void *allocArg)
{
    if (module == NULL)
        return;
    memset(module, 0, sizeof(HlslModule));
    module->stage = stage;
    module->alloc = alloc;
    module->allocArg = allocArg;
}

static const int hlslErrorCodes[] = {
    0,
    6400,
    6401,
    6402,
    6403,
    6404,
    6405,
    6406,
    6407,
    6408,
    6409,
    6410,
    6411,
    9013,
    6412,
    6413,
    6414,
    6415,
    6416,
    6417,
    6418,
    6419,
    6420,
    6421,
    6422,
    6423,
    6424
};

int HlslErrorCode(HlslErrorKind kind)
{
    if (kind < HLSL_ERROR_NONE ||
        kind > HLSL_ERROR_GEOMETRY_TOTAL_OUTPUT_LIMIT)
        return 0;
    return hlslErrorCodes[(int) kind];
} // HlslErrorCode

int HlslFail(HlslModule *module, HlslErrorKind kind,
             const HlslLoc *loc, const char *reason)
{
    if (module != NULL && module->errors == 0) {
        module->errorKind = kind;
        module->errorReason = reason;
        if (loc != NULL)
            module->errorLoc = *loc;
    }
    if (module != NULL)
        module->errors++;
    return 0;
} // HlslFail

int HlslFailRelated(HlslModule *module, HlslErrorKind kind,
                    const HlslLoc *loc, const HlslLoc *relatedLoc,
                    const char *reason)
{
    if (module != NULL && module->errors == 0 && relatedLoc != NULL)
        module->relatedErrorLoc = *relatedLoc;
    return HlslFail(module, kind, loc, reason);
} // HlslFailRelated

const char *HlslAllocateName(HlslModule *module, const char *source)
{
    HlslName *name;

    if (module == NULL || source == NULL)
        return NULL;
    name = HlslFindName(module, &defaultNameNamespace,
                        &defaultNameIdentity, source);
    if (name != NULL)
        return name->emitted;
    return HlslAllocate(module, &defaultNameNamespace,
                        &defaultNameIdentity, source, 0);
}

const char *HlslAllocateSymbolName(HlslModule *module,
    const void *identity, const char *source)
{
    return HlslAllocateScopedSymbolName(module, &defaultNameNamespace,
                                         identity, source);
}

const char *HlslAllocateScopedSymbolName(HlslModule *module,
    const void *nameSpace, const void *identity, const char *source)
{
    HlslName *name;

    if (module == NULL || nameSpace == NULL || source == NULL)
        return NULL;
    if (identity == NULL) {
        name = HlslFindName(module, nameSpace, &defaultNameIdentity,
                            source);
        if (name != NULL)
            return name->emitted;
        return HlslAllocate(module, nameSpace, &defaultNameIdentity,
                            source, 0);
    }
    for (name = module->names; name != NULL; name = name->next) {
        if (name->nameSpace == nameSpace && name->identity == identity)
            return name->emitted;
    }
    return HlslAllocate(module, nameSpace, identity, source, 0);
}

const char *HlslAllocateGeneratedName(HlslModule *module,
    const void *identity, const char *source)
{
    HlslName *name;

    if (module == NULL || identity == NULL || source == NULL)
        return NULL;
    for (name = module->names; name != NULL; name = name->next) {
        if (name->nameSpace == &defaultNameNamespace &&
            name->identity == identity)
        {
            return name->emitted;
        }
    }
    return HlslAllocate(module, &defaultNameNamespace, identity, source, 1);
}

const char *HlslAllocateDistinctName(HlslModule *module,
    const char *source)
{
    return HlslAllocate(module, &defaultNameNamespace, NULL, source, 0);
}

HlslType HlslNumericType(HlslBase base, int len)
{
    HlslType type;

    memset(&type, 0, sizeof(HlslType));
    type.base = base;
    type.len = len;
    return type;
}

HlslType HlslMatrixType(int rows, int cols)
{
    HlslType type;

    memset(&type, 0, sizeof(HlslType));
    type.base = HLSL_BASE_FLOAT;
    type.rows = rows;
    type.cols = cols;
    return type;
}

typedef struct HlslTypeFrame_Rec {
    const struct HlslTypeFrame_Rec *parent;
    const HlslType *type;
} HlslTypeFrame;

static int HlslTypeFrameContains(const HlslTypeFrame *frame,
    const HlslType *type)
{
    for (; frame != NULL; frame = frame->parent) {
        if (frame->type == type)
            return 1;
    }
    return 0;
} // HlslTypeFrameContains

static const char *HlslTypeNameInner(const HlslType *type,
    const HlslTypeFrame *parent)
{
    static const char *floatNames[] = {
        "float", "float2", "float3", "float4"
    };
    static const char *intNames[] = {
        "int", "int2", "int3", "int4"
    };
    static const char *boolNames[] = {
        "bool", "bool2", "bool3", "bool4"
    };
    static const char *matrixNames[4][4] = {
        { "row_major float1x1", "row_major float1x2",
          "row_major float1x3", "row_major float1x4" },
        { "row_major float2x1", "row_major float2x2",
          "row_major float2x3", "row_major float2x4" },
        { "row_major float3x1", "row_major float3x2",
          "row_major float3x3", "row_major float3x4" },
        { "row_major float4x1", "row_major float4x2",
          "row_major float4x3", "row_major float4x4" }
    };
    HlslTypeFrame frame;

    if (type == NULL || type->arraySize < 0 ||
        HlslTypeFrameContains(parent, type))
    {
        return NULL;
    }
    frame.parent = parent;
    frame.type = type;
    if (type->arraySize > 0) {
        if (type->elementType == NULL)
            return NULL;
        return HlslTypeNameInner(type->elementType, &frame);
    }
    if (type->base != HLSL_BASE_STRUCT &&
        (type->structName != NULL || type->members != NULL))
    {
        return NULL;
    }
    if (type->rows > 0 || type->cols > 0) {
        if (type->base == HLSL_BASE_FLOAT && type->len == 0 &&
            type->rows >= 1 && type->rows <= 4 &&
            type->cols >= 1 && type->cols <= 4)
        {
            return matrixNames[type->rows - 1][type->cols - 1];
        }
        return NULL;
    }
    switch (type->base) {
    case HLSL_BASE_VOID:
        return type->len == 0 ? "void" : NULL;
    case HLSL_BASE_FLOAT:
        return type->len >= 1 && type->len <= 4 ?
               floatNames[type->len - 1] : NULL;
    case HLSL_BASE_INT:
        return type->len >= 1 && type->len <= 4 ?
               intNames[type->len - 1] : NULL;
    case HLSL_BASE_UINT:
        return type->len == 1 ? "uint" :
               type->len == 2 ? "uint2" :
               type->len == 3 ? "uint3" :
               type->len == 4 ? "uint4" : NULL;
    case HLSL_BASE_BOOL:
        return type->len >= 1 && type->len <= 4 ?
               boolNames[type->len - 1] : NULL;
    case HLSL_BASE_SAMPLER1D:
        return type->len == 1 ? "sampler1D" : NULL;
    case HLSL_BASE_SAMPLER2D:
        return type->len == 1 ? "sampler2D" : NULL;
    case HLSL_BASE_SAMPLER3D:
        return type->len == 1 ? "sampler3D" : NULL;
    case HLSL_BASE_SAMPLERCUBE:
        return type->len == 1 ? "samplerCUBE" : NULL;
    case HLSL_BASE_TEXTURE1D:
        return type->len == 1 ? "Texture1D" : NULL;
    case HLSL_BASE_TEXTURE2D:
        return type->len == 1 ? "Texture2D" : NULL;
    case HLSL_BASE_TEXTURE3D:
        return type->len == 1 ? "Texture3D" : NULL;
    case HLSL_BASE_TEXTURECUBE:
        return type->len == 1 ? "TextureCube" : NULL;
    case HLSL_BASE_SAMPLER_STATE:
        return type->len == 1 ? "SamplerState" : NULL;
    case HLSL_BASE_GEOMETRY_STREAM:
        return type->len >= 1 && type->len <= 3 ? "geometry_stream" : NULL;
    case HLSL_BASE_STRUCT:
        return type->len == 0 && type->structName != NULL &&
               type->structName[0] != '\0' ? type->structName : NULL;
    }
    return NULL;
}

const char *HlslTypeName(const HlslType *type)
{
    return HlslTypeNameInner(type, NULL);
}

HlslSourceType HlslSourceScalarType(HlslSourceBase base)
{
    HlslSourceType type;

    memset(&type, 0, sizeof(type));
    type.base = base;
    type.shape = HLSL_SOURCE_SHAPE_SCALAR;
    return type;
}

HlslSourceType HlslSourceVectorType(HlslSourceBase base, int len)
{
    HlslSourceType type;

    memset(&type, 0, sizeof(type));
    type.base = base;
    type.shape = HLSL_SOURCE_SHAPE_VECTOR;
    type.cols = len;
    return type;
}

HlslSourceType HlslSourceMatrixType(HlslSourceBase base,
                                    int rows, int cols)
{
    HlslSourceType type;

    memset(&type, 0, sizeof(type));
    type.base = base;
    type.shape = HLSL_SOURCE_SHAPE_MATRIX;
    type.rows = rows;
    type.cols = cols;
    return type;
}

const char *HlslSourceTypeName(const HlslSourceType *type)
{
    static const char *baseNames[] = {
        NULL, "float", "int", "bool", "int", "fixed", "half", "float",
        "sampler1D", "sampler2D", "sampler3D", "samplerCUBE"
    };
    static char names[4][32];
    static int nextName;
    const char *base;
    char *name;

    if (type == NULL || type->base <= HLSL_SOURCE_BASE_NONE ||
        type->base > HLSL_SOURCE_BASE_SAMPLERCUBE)
    {
        return type != NULL && type->shape == HLSL_SOURCE_SHAPE_VOID ?
               "void" : NULL;
    }
    base = baseNames[type->base];
    if (type->base >= HLSL_SOURCE_BASE_SAMPLER1D &&
        (type->shape != HLSL_SOURCE_SHAPE_SCALAR ||
         type->rows != 0 || type->cols != 0))
    {
        return NULL;
    }
    if (type->shape == HLSL_SOURCE_SHAPE_SCALAR)
        return base;
    name = names[nextName++ % 4];
    if (type->shape == HLSL_SOURCE_SHAPE_VECTOR &&
        type->cols >= 1 && type->cols <= 4)
    {
        sprintf(name, "%s%d", base, type->cols);
        return name;
    }
    if (type->shape == HLSL_SOURCE_SHAPE_MATRIX &&
        type->rows >= 1 && type->rows <= 4 &&
        type->cols >= 1 && type->cols <= 4)
    {
        sprintf(name, "%s%dx%d", base, type->rows, type->cols);
        return name;
    }
    return NULL;
}

#define HLSL_BUILTIN_STAGE_VERTEX 0x01
#define HLSL_BUILTIN_STAGE_PIXEL  0x02
#define HLSL_BUILTIN_STAGE_GEOMETRY 0x04
#define HLSL_BUILTIN_STAGE_BOTH   \
    (HLSL_BUILTIN_STAGE_VERTEX | HLSL_BUILTIN_STAGE_PIXEL | \
     HLSL_BUILTIN_STAGE_GEOMETRY)

#define HLSL_BUILTIN_BASE_FLOAT 0x01
#define HLSL_BUILTIN_BASE_INT   0x02
#define HLSL_BUILTIN_BASE_BOOL  0x04

#define HLSL_BUILTIN_SOURCE_FIXED 0x01
#define HLSL_BUILTIN_SOURCE_HALF  0x02
#define HLSL_BUILTIN_SOURCE_FLOAT 0x04
#define HLSL_BUILTIN_SOURCE_INT   0x08
#define HLSL_BUILTIN_SOURCE_BOOL  0x10
#define HLSL_BUILTIN_SOURCE_FLOATING \
    (HLSL_BUILTIN_SOURCE_FIXED | HLSL_BUILTIN_SOURCE_HALF | \
     HLSL_BUILTIN_SOURCE_FLOAT)

typedef enum HlslBuiltinWidthPattern_Enum {
    HLSL_BUILTIN_WIDTH_MUL,
    HLSL_BUILTIN_WIDTH_MATCH,
    HLSL_BUILTIN_WIDTH_REPLICATED,
    HLSL_BUILTIN_WIDTH_CROSS,
    HLSL_BUILTIN_WIDTH_REFRACT
} HlslBuiltinWidthPattern;

typedef enum HlslBuiltinResultPattern_Enum {
    HLSL_BUILTIN_RESULT_MUL,
    HLSL_BUILTIN_RESULT_SAME,
    HLSL_BUILTIN_RESULT_WIDEST,
    HLSL_BUILTIN_RESULT_SCALAR
} HlslBuiltinResultPattern;

typedef struct HlslBuiltinDesc_Rec {
    const char *source;
    HlslBuiltin builtin;
    int arity;
    unsigned baseMask;
    HlslBuiltinWidthPattern widthPattern;
    HlslBuiltinResultPattern resultPattern;
    unsigned stageMask;
    const char *hlsl;
    HlslBuiltinLowering lowering;
    unsigned sourceBaseMask;
    HlslTextureForm textureForm;
    HlslBase samplerBase;
    HlslSourceBase sourceSamplerBase;
    int coordWidth;
    unsigned sourceResultMask;
} HlslBuiltinDesc;

#define HLSL_TEXTURE_ROW(src, id, count, sampler, sourceSampler, width, \
                         stages, target, form, resultMask) \
    { src, id, count, 0, HLSL_BUILTIN_WIDTH_MATCH, \
      HLSL_BUILTIN_RESULT_SAME, stages, target, \
      HLSL_BUILTIN_LOWER_NATIVE, 0, form, sampler, sourceSampler, width, \
      resultMask }

/*
 * The signature table is the sole spelling-to-operation map.  A row
 * describes a complete overload family: its arity, admitted bases and
 * width relationship, result relationship, stage availability, target
 * spelling, and lowering strategy all travel together.
 */

static const HlslBuiltinDesc hlslBuiltinTable[] = {
    { "mul", HLSL_BUILTIN_MUL, 2, HLSL_BUILTIN_BASE_FLOAT,
      HLSL_BUILTIN_WIDTH_MUL, HLSL_BUILTIN_RESULT_MUL,
      HLSL_BUILTIN_STAGE_BOTH, "mul", HLSL_BUILTIN_LOWER_NATIVE,
      HLSL_BUILTIN_SOURCE_FLOAT },
    { "dot", HLSL_BUILTIN_DOT, 2, HLSL_BUILTIN_BASE_FLOAT,
      HLSL_BUILTIN_WIDTH_MATCH, HLSL_BUILTIN_RESULT_SCALAR,
      HLSL_BUILTIN_STAGE_BOTH, "dot", HLSL_BUILTIN_LOWER_NATIVE },
    { "cross", HLSL_BUILTIN_CROSS, 2, HLSL_BUILTIN_BASE_FLOAT,
      HLSL_BUILTIN_WIDTH_CROSS, HLSL_BUILTIN_RESULT_SAME,
      HLSL_BUILTIN_STAGE_BOTH, "cross", HLSL_BUILTIN_LOWER_NATIVE,
      HLSL_BUILTIN_SOURCE_FLOAT },
    { "normalize", HLSL_BUILTIN_NORMALIZE, 1, HLSL_BUILTIN_BASE_FLOAT,
      HLSL_BUILTIN_WIDTH_MATCH, HLSL_BUILTIN_RESULT_SAME,
      HLSL_BUILTIN_STAGE_BOTH, "normalize", HLSL_BUILTIN_LOWER_NATIVE },
    { "reflect", HLSL_BUILTIN_REFLECT, 2, HLSL_BUILTIN_BASE_FLOAT,
      HLSL_BUILTIN_WIDTH_REPLICATED, HLSL_BUILTIN_RESULT_WIDEST,
      HLSL_BUILTIN_STAGE_BOTH, "reflect", HLSL_BUILTIN_LOWER_NATIVE },
    { "refract", HLSL_BUILTIN_REFRACT, 3, HLSL_BUILTIN_BASE_FLOAT,
      HLSL_BUILTIN_WIDTH_REFRACT, HLSL_BUILTIN_RESULT_SAME,
      HLSL_BUILTIN_STAGE_BOTH, "refract", HLSL_BUILTIN_LOWER_NATIVE },
    { "length", HLSL_BUILTIN_LENGTH, 1, HLSL_BUILTIN_BASE_FLOAT,
      HLSL_BUILTIN_WIDTH_MATCH, HLSL_BUILTIN_RESULT_SCALAR,
      HLSL_BUILTIN_STAGE_BOTH, "length", HLSL_BUILTIN_LOWER_NATIVE },
    { "distance", HLSL_BUILTIN_DISTANCE, 2, HLSL_BUILTIN_BASE_FLOAT,
      HLSL_BUILTIN_WIDTH_MATCH, HLSL_BUILTIN_RESULT_SCALAR,
      HLSL_BUILTIN_STAGE_BOTH, "distance", HLSL_BUILTIN_LOWER_NATIVE },
    { "min", HLSL_BUILTIN_MIN, 2, HLSL_BUILTIN_BASE_FLOAT,
      HLSL_BUILTIN_WIDTH_REPLICATED, HLSL_BUILTIN_RESULT_WIDEST,
      HLSL_BUILTIN_STAGE_BOTH, "min", HLSL_BUILTIN_LOWER_NATIVE },
    { "max", HLSL_BUILTIN_MAX, 2, HLSL_BUILTIN_BASE_FLOAT,
      HLSL_BUILTIN_WIDTH_REPLICATED, HLSL_BUILTIN_RESULT_WIDEST,
      HLSL_BUILTIN_STAGE_BOTH, "max", HLSL_BUILTIN_LOWER_NATIVE },
    { "clamp", HLSL_BUILTIN_CLAMP, 3, HLSL_BUILTIN_BASE_FLOAT,
      HLSL_BUILTIN_WIDTH_REPLICATED, HLSL_BUILTIN_RESULT_WIDEST,
      HLSL_BUILTIN_STAGE_BOTH, "clamp", HLSL_BUILTIN_LOWER_NATIVE },
    { "abs", HLSL_BUILTIN_ABS, 1,
      HLSL_BUILTIN_BASE_FLOAT | HLSL_BUILTIN_BASE_INT,
      HLSL_BUILTIN_WIDTH_MATCH, HLSL_BUILTIN_RESULT_SAME,
      HLSL_BUILTIN_STAGE_BOTH, "abs", HLSL_BUILTIN_LOWER_NATIVE },
    { "sign", HLSL_BUILTIN_SIGN, 1,
      HLSL_BUILTIN_BASE_FLOAT | HLSL_BUILTIN_BASE_INT,
      HLSL_BUILTIN_WIDTH_MATCH, HLSL_BUILTIN_RESULT_SAME,
      HLSL_BUILTIN_STAGE_BOTH, "sign", HLSL_BUILTIN_LOWER_NATIVE },
    { "floor", HLSL_BUILTIN_FLOOR, 1, HLSL_BUILTIN_BASE_FLOAT,
      HLSL_BUILTIN_WIDTH_MATCH, HLSL_BUILTIN_RESULT_SAME,
      HLSL_BUILTIN_STAGE_BOTH, "floor", HLSL_BUILTIN_LOWER_NATIVE },
    { "ceil", HLSL_BUILTIN_CEIL, 1, HLSL_BUILTIN_BASE_FLOAT,
      HLSL_BUILTIN_WIDTH_MATCH, HLSL_BUILTIN_RESULT_SAME,
      HLSL_BUILTIN_STAGE_BOTH, "ceil", HLSL_BUILTIN_LOWER_NATIVE },
    { "round", HLSL_BUILTIN_ROUND, 1, HLSL_BUILTIN_BASE_FLOAT,
      HLSL_BUILTIN_WIDTH_MATCH, HLSL_BUILTIN_RESULT_SAME,
      HLSL_BUILTIN_STAGE_BOTH, "round", HLSL_BUILTIN_LOWER_NATIVE },
    { "trunc", HLSL_BUILTIN_TRUNC, 1, HLSL_BUILTIN_BASE_FLOAT,
      HLSL_BUILTIN_WIDTH_MATCH, HLSL_BUILTIN_RESULT_SAME,
      HLSL_BUILTIN_STAGE_BOTH, "trunc", HLSL_BUILTIN_LOWER_NATIVE,
      HLSL_BUILTIN_SOURCE_FLOAT },
    { "sqrt", HLSL_BUILTIN_SQRT, 1, HLSL_BUILTIN_BASE_FLOAT,
      HLSL_BUILTIN_WIDTH_MATCH, HLSL_BUILTIN_RESULT_SAME,
      HLSL_BUILTIN_STAGE_BOTH, "sqrt", HLSL_BUILTIN_LOWER_NATIVE },
    { "rsqrt", HLSL_BUILTIN_RSQRT, 1, HLSL_BUILTIN_BASE_FLOAT,
      HLSL_BUILTIN_WIDTH_MATCH, HLSL_BUILTIN_RESULT_SAME,
      HLSL_BUILTIN_STAGE_BOTH, "rsqrt", HLSL_BUILTIN_LOWER_HELPER },
    { "pow", HLSL_BUILTIN_POW, 2, HLSL_BUILTIN_BASE_FLOAT,
      HLSL_BUILTIN_WIDTH_REPLICATED, HLSL_BUILTIN_RESULT_WIDEST,
      HLSL_BUILTIN_STAGE_BOTH, "pow", HLSL_BUILTIN_LOWER_NATIVE },
    { "exp", HLSL_BUILTIN_EXP, 1, HLSL_BUILTIN_BASE_FLOAT,
      HLSL_BUILTIN_WIDTH_MATCH, HLSL_BUILTIN_RESULT_SAME,
      HLSL_BUILTIN_STAGE_BOTH, "exp", HLSL_BUILTIN_LOWER_NATIVE },
    { "exp2", HLSL_BUILTIN_EXP2, 1, HLSL_BUILTIN_BASE_FLOAT,
      HLSL_BUILTIN_WIDTH_MATCH, HLSL_BUILTIN_RESULT_SAME,
      HLSL_BUILTIN_STAGE_BOTH, "exp2", HLSL_BUILTIN_LOWER_NATIVE },
    { "log", HLSL_BUILTIN_LOG, 1, HLSL_BUILTIN_BASE_FLOAT,
      HLSL_BUILTIN_WIDTH_MATCH, HLSL_BUILTIN_RESULT_SAME,
      HLSL_BUILTIN_STAGE_BOTH, "log", HLSL_BUILTIN_LOWER_NATIVE },
    { "log2", HLSL_BUILTIN_LOG2, 1, HLSL_BUILTIN_BASE_FLOAT,
      HLSL_BUILTIN_WIDTH_MATCH, HLSL_BUILTIN_RESULT_SAME,
      HLSL_BUILTIN_STAGE_BOTH, "log2", HLSL_BUILTIN_LOWER_NATIVE },
    { "sin", HLSL_BUILTIN_SIN, 1, HLSL_BUILTIN_BASE_FLOAT,
      HLSL_BUILTIN_WIDTH_MATCH, HLSL_BUILTIN_RESULT_SAME,
      HLSL_BUILTIN_STAGE_BOTH, "sin", HLSL_BUILTIN_LOWER_NATIVE },
    { "cos", HLSL_BUILTIN_COS, 1, HLSL_BUILTIN_BASE_FLOAT,
      HLSL_BUILTIN_WIDTH_MATCH, HLSL_BUILTIN_RESULT_SAME,
      HLSL_BUILTIN_STAGE_BOTH, "cos", HLSL_BUILTIN_LOWER_NATIVE },
    { "tan", HLSL_BUILTIN_TAN, 1, HLSL_BUILTIN_BASE_FLOAT,
      HLSL_BUILTIN_WIDTH_MATCH, HLSL_BUILTIN_RESULT_SAME,
      HLSL_BUILTIN_STAGE_BOTH, "tan", HLSL_BUILTIN_LOWER_NATIVE },
    { "asin", HLSL_BUILTIN_ASIN, 1, HLSL_BUILTIN_BASE_FLOAT,
      HLSL_BUILTIN_WIDTH_MATCH, HLSL_BUILTIN_RESULT_SAME,
      HLSL_BUILTIN_STAGE_BOTH, "asin", HLSL_BUILTIN_LOWER_NATIVE },
    { "acos", HLSL_BUILTIN_ACOS, 1, HLSL_BUILTIN_BASE_FLOAT,
      HLSL_BUILTIN_WIDTH_MATCH, HLSL_BUILTIN_RESULT_SAME,
      HLSL_BUILTIN_STAGE_BOTH, "acos", HLSL_BUILTIN_LOWER_NATIVE },
    { "atan", HLSL_BUILTIN_ATAN, 1, HLSL_BUILTIN_BASE_FLOAT,
      HLSL_BUILTIN_WIDTH_MATCH, HLSL_BUILTIN_RESULT_SAME,
      HLSL_BUILTIN_STAGE_BOTH, "atan", HLSL_BUILTIN_LOWER_NATIVE },
    { "atan2", HLSL_BUILTIN_ATAN2, 2, HLSL_BUILTIN_BASE_FLOAT,
      HLSL_BUILTIN_WIDTH_REPLICATED, HLSL_BUILTIN_RESULT_WIDEST,
      HLSL_BUILTIN_STAGE_BOTH, "atan2", HLSL_BUILTIN_LOWER_NATIVE },
    { "sinh", HLSL_BUILTIN_SINH, 1, HLSL_BUILTIN_BASE_FLOAT,
      HLSL_BUILTIN_WIDTH_MATCH, HLSL_BUILTIN_RESULT_SAME,
      HLSL_BUILTIN_STAGE_BOTH, "sinh", HLSL_BUILTIN_LOWER_NATIVE },
    { "cosh", HLSL_BUILTIN_COSH, 1, HLSL_BUILTIN_BASE_FLOAT,
      HLSL_BUILTIN_WIDTH_MATCH, HLSL_BUILTIN_RESULT_SAME,
      HLSL_BUILTIN_STAGE_BOTH, "cosh", HLSL_BUILTIN_LOWER_NATIVE },
    { "tanh", HLSL_BUILTIN_TANH, 1, HLSL_BUILTIN_BASE_FLOAT,
      HLSL_BUILTIN_WIDTH_MATCH, HLSL_BUILTIN_RESULT_SAME,
      HLSL_BUILTIN_STAGE_BOTH, "tanh", HLSL_BUILTIN_LOWER_NATIVE },
    { "lerp", HLSL_BUILTIN_LERP, 3, HLSL_BUILTIN_BASE_FLOAT,
      HLSL_BUILTIN_WIDTH_REPLICATED, HLSL_BUILTIN_RESULT_WIDEST,
      HLSL_BUILTIN_STAGE_BOTH, "lerp", HLSL_BUILTIN_LOWER_NATIVE },
    { "frac", HLSL_BUILTIN_FRAC, 1, HLSL_BUILTIN_BASE_FLOAT,
      HLSL_BUILTIN_WIDTH_MATCH, HLSL_BUILTIN_RESULT_SAME,
      HLSL_BUILTIN_STAGE_BOTH, "frac", HLSL_BUILTIN_LOWER_NATIVE },
    { "fmod", HLSL_BUILTIN_FMOD, 2, HLSL_BUILTIN_BASE_FLOAT,
      HLSL_BUILTIN_WIDTH_REPLICATED, HLSL_BUILTIN_RESULT_WIDEST,
      HLSL_BUILTIN_STAGE_BOTH, "fmod", HLSL_BUILTIN_LOWER_NATIVE },
    { "saturate", HLSL_BUILTIN_SATURATE, 1, HLSL_BUILTIN_BASE_FLOAT,
      HLSL_BUILTIN_WIDTH_MATCH, HLSL_BUILTIN_RESULT_SAME,
      HLSL_BUILTIN_STAGE_BOTH, "saturate", HLSL_BUILTIN_LOWER_EXPANSION },
    { "step", HLSL_BUILTIN_STEP, 2, HLSL_BUILTIN_BASE_FLOAT,
      HLSL_BUILTIN_WIDTH_REPLICATED, HLSL_BUILTIN_RESULT_WIDEST,
      HLSL_BUILTIN_STAGE_BOTH, "step", HLSL_BUILTIN_LOWER_NATIVE },
    { "smoothstep", HLSL_BUILTIN_SMOOTHSTEP, 3, HLSL_BUILTIN_BASE_FLOAT,
      HLSL_BUILTIN_WIDTH_REPLICATED, HLSL_BUILTIN_RESULT_WIDEST,
      HLSL_BUILTIN_STAGE_BOTH, "smoothstep", HLSL_BUILTIN_LOWER_NATIVE },
    { "any", HLSL_BUILTIN_ANY, 1, HLSL_BUILTIN_BASE_BOOL,
      HLSL_BUILTIN_WIDTH_MATCH, HLSL_BUILTIN_RESULT_SCALAR,
      HLSL_BUILTIN_STAGE_BOTH, "any", HLSL_BUILTIN_LOWER_NATIVE },
    { "all", HLSL_BUILTIN_ALL, 1, HLSL_BUILTIN_BASE_BOOL,
      HLSL_BUILTIN_WIDTH_MATCH, HLSL_BUILTIN_RESULT_SCALAR,
      HLSL_BUILTIN_STAGE_BOTH, "all", HLSL_BUILTIN_LOWER_NATIVE },
    { "ddx", HLSL_BUILTIN_DDX, 1, HLSL_BUILTIN_BASE_FLOAT,
      HLSL_BUILTIN_WIDTH_MATCH, HLSL_BUILTIN_RESULT_SAME,
      HLSL_BUILTIN_STAGE_PIXEL, "ddx", HLSL_BUILTIN_LOWER_NATIVE },
    { "ddy", HLSL_BUILTIN_DDY, 1, HLSL_BUILTIN_BASE_FLOAT,
      HLSL_BUILTIN_WIDTH_MATCH, HLSL_BUILTIN_RESULT_SAME,
      HLSL_BUILTIN_STAGE_PIXEL, "ddy", HLSL_BUILTIN_LOWER_NATIVE },

    HLSL_TEXTURE_ROW("tex1D", HLSL_BUILTIN_TEX1D, 2,
      HLSL_BASE_SAMPLER1D, HLSL_SOURCE_BASE_SAMPLER1D, 1,
      HLSL_BUILTIN_STAGE_PIXEL, "tex1D", HLSL_TEXTURE_IMPLICIT,
      HLSL_BUILTIN_SOURCE_FLOAT),
    HLSL_TEXTURE_ROW("h4tex1D", HLSL_BUILTIN_TEX1D, 2,
      HLSL_BASE_SAMPLER1D, HLSL_SOURCE_BASE_SAMPLER1D, 1,
      HLSL_BUILTIN_STAGE_PIXEL, "tex1D", HLSL_TEXTURE_IMPLICIT,
      HLSL_BUILTIN_SOURCE_HALF),
    HLSL_TEXTURE_ROW("x4tex1D", HLSL_BUILTIN_TEX1D, 2,
      HLSL_BASE_SAMPLER1D, HLSL_SOURCE_BASE_SAMPLER1D, 1,
      HLSL_BUILTIN_STAGE_PIXEL, "tex1D", HLSL_TEXTURE_IMPLICIT,
      HLSL_BUILTIN_SOURCE_FIXED),
    HLSL_TEXTURE_ROW("tex1Dproj", HLSL_BUILTIN_TEX1DPROJ, 2,
      HLSL_BASE_SAMPLER1D, HLSL_SOURCE_BASE_SAMPLER1D, 4,
      HLSL_BUILTIN_STAGE_PIXEL, "tex1Dproj", HLSL_TEXTURE_PROJECTED,
      HLSL_BUILTIN_SOURCE_FLOAT),
    HLSL_TEXTURE_ROW("h4tex1Dproj", HLSL_BUILTIN_TEX1DPROJ, 2,
      HLSL_BASE_SAMPLER1D, HLSL_SOURCE_BASE_SAMPLER1D, 4,
      HLSL_BUILTIN_STAGE_PIXEL, "tex1Dproj", HLSL_TEXTURE_PROJECTED,
      HLSL_BUILTIN_SOURCE_HALF),
    HLSL_TEXTURE_ROW("x4tex1Dproj", HLSL_BUILTIN_TEX1DPROJ, 2,
      HLSL_BASE_SAMPLER1D, HLSL_SOURCE_BASE_SAMPLER1D, 4,
      HLSL_BUILTIN_STAGE_PIXEL, "tex1Dproj", HLSL_TEXTURE_PROJECTED,
      HLSL_BUILTIN_SOURCE_FIXED),
    HLSL_TEXTURE_ROW("tex1Dbias", HLSL_BUILTIN_TEX1DBIAS, 2,
      HLSL_BASE_SAMPLER1D, HLSL_SOURCE_BASE_SAMPLER1D, 4,
      HLSL_BUILTIN_STAGE_PIXEL, "tex1Dbias", HLSL_TEXTURE_BIAS,
      HLSL_BUILTIN_SOURCE_FLOAT),
    HLSL_TEXTURE_ROW("tex1Dlod", HLSL_BUILTIN_TEX1DLOD, 2,
      HLSL_BASE_SAMPLER1D, HLSL_SOURCE_BASE_SAMPLER1D, 4,
      HLSL_BUILTIN_STAGE_BOTH, "tex1Dlod", HLSL_TEXTURE_LOD,
      HLSL_BUILTIN_SOURCE_FLOAT),
    HLSL_TEXTURE_ROW("tex1D", HLSL_BUILTIN_TEX1DGRAD, 4,
      HLSL_BASE_SAMPLER1D, HLSL_SOURCE_BASE_SAMPLER1D, 1,
      HLSL_BUILTIN_STAGE_BOTH, "tex1Dgrad", HLSL_TEXTURE_GRADIENT,
      HLSL_BUILTIN_SOURCE_FLOAT),

    HLSL_TEXTURE_ROW("tex2D", HLSL_BUILTIN_TEX2D, 2,
      HLSL_BASE_SAMPLER2D, HLSL_SOURCE_BASE_SAMPLER2D, 2,
      HLSL_BUILTIN_STAGE_PIXEL, "tex2D", HLSL_TEXTURE_IMPLICIT,
      HLSL_BUILTIN_SOURCE_FLOAT),
    HLSL_TEXTURE_ROW("h4tex2D", HLSL_BUILTIN_TEX2D, 2,
      HLSL_BASE_SAMPLER2D, HLSL_SOURCE_BASE_SAMPLER2D, 2,
      HLSL_BUILTIN_STAGE_PIXEL, "tex2D", HLSL_TEXTURE_IMPLICIT,
      HLSL_BUILTIN_SOURCE_HALF),
    HLSL_TEXTURE_ROW("x4tex2D", HLSL_BUILTIN_TEX2D, 2,
      HLSL_BASE_SAMPLER2D, HLSL_SOURCE_BASE_SAMPLER2D, 2,
      HLSL_BUILTIN_STAGE_PIXEL, "tex2D", HLSL_TEXTURE_IMPLICIT,
      HLSL_BUILTIN_SOURCE_FIXED),
    HLSL_TEXTURE_ROW("tex2Dproj", HLSL_BUILTIN_TEX2DPROJ, 2,
      HLSL_BASE_SAMPLER2D, HLSL_SOURCE_BASE_SAMPLER2D, 4,
      HLSL_BUILTIN_STAGE_PIXEL, "tex2Dproj", HLSL_TEXTURE_PROJECTED,
      HLSL_BUILTIN_SOURCE_FLOAT),
    HLSL_TEXTURE_ROW("h4tex2Dproj", HLSL_BUILTIN_TEX2DPROJ, 2,
      HLSL_BASE_SAMPLER2D, HLSL_SOURCE_BASE_SAMPLER2D, 4,
      HLSL_BUILTIN_STAGE_PIXEL, "tex2Dproj", HLSL_TEXTURE_PROJECTED,
      HLSL_BUILTIN_SOURCE_HALF),
    HLSL_TEXTURE_ROW("x4tex2Dproj", HLSL_BUILTIN_TEX2DPROJ, 2,
      HLSL_BASE_SAMPLER2D, HLSL_SOURCE_BASE_SAMPLER2D, 4,
      HLSL_BUILTIN_STAGE_PIXEL, "tex2Dproj", HLSL_TEXTURE_PROJECTED,
      HLSL_BUILTIN_SOURCE_FIXED),
    HLSL_TEXTURE_ROW("tex2Dbias", HLSL_BUILTIN_TEX2DBIAS, 2,
      HLSL_BASE_SAMPLER2D, HLSL_SOURCE_BASE_SAMPLER2D, 4,
      HLSL_BUILTIN_STAGE_PIXEL, "tex2Dbias", HLSL_TEXTURE_BIAS,
      HLSL_BUILTIN_SOURCE_FLOAT),
    HLSL_TEXTURE_ROW("tex2Dlod", HLSL_BUILTIN_TEX2DLOD, 2,
      HLSL_BASE_SAMPLER2D, HLSL_SOURCE_BASE_SAMPLER2D, 4,
      HLSL_BUILTIN_STAGE_BOTH, "tex2Dlod", HLSL_TEXTURE_LOD,
      HLSL_BUILTIN_SOURCE_FLOAT),
    HLSL_TEXTURE_ROW("tex2D", HLSL_BUILTIN_TEX2DGRAD, 4,
      HLSL_BASE_SAMPLER2D, HLSL_SOURCE_BASE_SAMPLER2D, 2,
      HLSL_BUILTIN_STAGE_BOTH, "tex2Dgrad", HLSL_TEXTURE_GRADIENT,
      HLSL_BUILTIN_SOURCE_FLOAT),

    HLSL_TEXTURE_ROW("tex3D", HLSL_BUILTIN_TEX3D, 2,
      HLSL_BASE_SAMPLER3D, HLSL_SOURCE_BASE_SAMPLER3D, 3,
      HLSL_BUILTIN_STAGE_PIXEL, "tex3D", HLSL_TEXTURE_IMPLICIT,
      HLSL_BUILTIN_SOURCE_FLOAT),
    HLSL_TEXTURE_ROW("h4tex3D", HLSL_BUILTIN_TEX3D, 2,
      HLSL_BASE_SAMPLER3D, HLSL_SOURCE_BASE_SAMPLER3D, 3,
      HLSL_BUILTIN_STAGE_PIXEL, "tex3D", HLSL_TEXTURE_IMPLICIT,
      HLSL_BUILTIN_SOURCE_HALF),
    HLSL_TEXTURE_ROW("x4tex3D", HLSL_BUILTIN_TEX3D, 2,
      HLSL_BASE_SAMPLER3D, HLSL_SOURCE_BASE_SAMPLER3D, 3,
      HLSL_BUILTIN_STAGE_PIXEL, "tex3D", HLSL_TEXTURE_IMPLICIT,
      HLSL_BUILTIN_SOURCE_FIXED),
    HLSL_TEXTURE_ROW("tex3Dproj", HLSL_BUILTIN_TEX3DPROJ, 2,
      HLSL_BASE_SAMPLER3D, HLSL_SOURCE_BASE_SAMPLER3D, 4,
      HLSL_BUILTIN_STAGE_PIXEL, "tex3Dproj", HLSL_TEXTURE_PROJECTED,
      HLSL_BUILTIN_SOURCE_FLOAT),
    HLSL_TEXTURE_ROW("h4tex3Dproj", HLSL_BUILTIN_TEX3DPROJ, 2,
      HLSL_BASE_SAMPLER3D, HLSL_SOURCE_BASE_SAMPLER3D, 4,
      HLSL_BUILTIN_STAGE_PIXEL, "tex3Dproj", HLSL_TEXTURE_PROJECTED,
      HLSL_BUILTIN_SOURCE_HALF),
    HLSL_TEXTURE_ROW("x4tex3Dproj", HLSL_BUILTIN_TEX3DPROJ, 2,
      HLSL_BASE_SAMPLER3D, HLSL_SOURCE_BASE_SAMPLER3D, 4,
      HLSL_BUILTIN_STAGE_PIXEL, "tex3Dproj", HLSL_TEXTURE_PROJECTED,
      HLSL_BUILTIN_SOURCE_FIXED),
    HLSL_TEXTURE_ROW("tex3Dbias", HLSL_BUILTIN_TEX3DBIAS, 2,
      HLSL_BASE_SAMPLER3D, HLSL_SOURCE_BASE_SAMPLER3D, 4,
      HLSL_BUILTIN_STAGE_PIXEL, "tex3Dbias", HLSL_TEXTURE_BIAS,
      HLSL_BUILTIN_SOURCE_FLOAT),
    HLSL_TEXTURE_ROW("tex3Dlod", HLSL_BUILTIN_TEX3DLOD, 2,
      HLSL_BASE_SAMPLER3D, HLSL_SOURCE_BASE_SAMPLER3D, 4,
      HLSL_BUILTIN_STAGE_BOTH, "tex3Dlod", HLSL_TEXTURE_LOD,
      HLSL_BUILTIN_SOURCE_FLOAT),
    HLSL_TEXTURE_ROW("tex3D", HLSL_BUILTIN_TEX3DGRAD, 4,
      HLSL_BASE_SAMPLER3D, HLSL_SOURCE_BASE_SAMPLER3D, 3,
      HLSL_BUILTIN_STAGE_BOTH, "tex3Dgrad", HLSL_TEXTURE_GRADIENT,
      HLSL_BUILTIN_SOURCE_FLOAT),

    HLSL_TEXTURE_ROW("texCUBE", HLSL_BUILTIN_TEXCUBE, 2,
      HLSL_BASE_SAMPLERCUBE, HLSL_SOURCE_BASE_SAMPLERCUBE, 3,
      HLSL_BUILTIN_STAGE_PIXEL, "texCUBE", HLSL_TEXTURE_IMPLICIT,
      HLSL_BUILTIN_SOURCE_FLOAT),
    HLSL_TEXTURE_ROW("h4texCUBE", HLSL_BUILTIN_TEXCUBE, 2,
      HLSL_BASE_SAMPLERCUBE, HLSL_SOURCE_BASE_SAMPLERCUBE, 3,
      HLSL_BUILTIN_STAGE_PIXEL, "texCUBE", HLSL_TEXTURE_IMPLICIT,
      HLSL_BUILTIN_SOURCE_HALF),
    HLSL_TEXTURE_ROW("x4texCUBE", HLSL_BUILTIN_TEXCUBE, 2,
      HLSL_BASE_SAMPLERCUBE, HLSL_SOURCE_BASE_SAMPLERCUBE, 3,
      HLSL_BUILTIN_STAGE_PIXEL, "texCUBE", HLSL_TEXTURE_IMPLICIT,
      HLSL_BUILTIN_SOURCE_FIXED),
    HLSL_TEXTURE_ROW("texCUBEproj", HLSL_BUILTIN_TEXCUBEPROJ, 2,
      HLSL_BASE_SAMPLERCUBE, HLSL_SOURCE_BASE_SAMPLERCUBE, 4,
      HLSL_BUILTIN_STAGE_PIXEL, "texCUBEproj", HLSL_TEXTURE_PROJECTED,
      HLSL_BUILTIN_SOURCE_FLOAT),
    HLSL_TEXTURE_ROW("h4texCUBEproj", HLSL_BUILTIN_TEXCUBEPROJ, 2,
      HLSL_BASE_SAMPLERCUBE, HLSL_SOURCE_BASE_SAMPLERCUBE, 4,
      HLSL_BUILTIN_STAGE_PIXEL, "texCUBEproj", HLSL_TEXTURE_PROJECTED,
      HLSL_BUILTIN_SOURCE_HALF),
    HLSL_TEXTURE_ROW("x4texCUBEproj", HLSL_BUILTIN_TEXCUBEPROJ, 2,
      HLSL_BASE_SAMPLERCUBE, HLSL_SOURCE_BASE_SAMPLERCUBE, 4,
      HLSL_BUILTIN_STAGE_PIXEL, "texCUBEproj", HLSL_TEXTURE_PROJECTED,
      HLSL_BUILTIN_SOURCE_FIXED),
    HLSL_TEXTURE_ROW("texCUBEbias", HLSL_BUILTIN_TEXCUBEBIAS, 2,
      HLSL_BASE_SAMPLERCUBE, HLSL_SOURCE_BASE_SAMPLERCUBE, 4,
      HLSL_BUILTIN_STAGE_PIXEL, "texCUBEbias", HLSL_TEXTURE_BIAS,
      HLSL_BUILTIN_SOURCE_FLOAT),
    HLSL_TEXTURE_ROW("texCUBElod", HLSL_BUILTIN_TEXCUBELOD, 2,
      HLSL_BASE_SAMPLERCUBE, HLSL_SOURCE_BASE_SAMPLERCUBE, 4,
      HLSL_BUILTIN_STAGE_BOTH, "texCUBElod", HLSL_TEXTURE_LOD,
      HLSL_BUILTIN_SOURCE_FLOAT),
    HLSL_TEXTURE_ROW("texCUBE", HLSL_BUILTIN_TEXCUBEGRAD, 4,
      HLSL_BASE_SAMPLERCUBE, HLSL_SOURCE_BASE_SAMPLERCUBE, 3,
      HLSL_BUILTIN_STAGE_BOTH, "texCUBEgrad", HLSL_TEXTURE_GRADIENT,
      HLSL_BUILTIN_SOURCE_FLOAT)
};

#undef HLSL_TEXTURE_ROW

static const char *hlslRejectedTextureNames[] = {
    "texRECT", "texRECTproj", "h4texRECT", "x4texRECT",
    "h4texRECTproj", "x4texRECTproj"
};

#define HLSL_BUILTIN_TABLE_COUNT \
    ((int) (sizeof(hlslBuiltinTable) / sizeof(hlslBuiltinTable[0])))

static unsigned HlslBuiltinBaseMask(HlslBase base)
{
    switch (base) {
    case HLSL_BASE_FLOAT: return HLSL_BUILTIN_BASE_FLOAT;
    case HLSL_BASE_INT: return HLSL_BUILTIN_BASE_INT;
    case HLSL_BASE_BOOL: return HLSL_BUILTIN_BASE_BOOL;
    default: return 0;
    }
} // HlslBuiltinBaseMask

static int HlslBuiltinVectorType(const HlslType *type, unsigned baseMask)
{
    return type != NULL && type->arraySize == 0 &&
           (HlslBuiltinBaseMask(type->base) & baseMask) != 0 &&
           type->len >= 1 && type->len <= 4 &&
           type->rows == 0 && type->cols == 0 &&
           type->structName == NULL && type->elementType == NULL &&
           type->members == NULL;
} // HlslBuiltinVectorType

static int HlslBuiltinMatrixType(const HlslType *type)
{
    return type != NULL && type->arraySize == 0 &&
           type->base == HLSL_BASE_FLOAT && type->len == 0 &&
           type->rows >= 1 && type->rows <= 4 &&
           type->cols >= 1 && type->cols <= 4 &&
           type->structName == NULL && type->elementType == NULL &&
           type->members == NULL;
} // HlslBuiltinMatrixType

static int HlslBuiltinTypeEqual(const HlslType *left,
                                const HlslType *right)
{
    return left != NULL && right != NULL &&
           left->base == right->base && left->len == right->len &&
           left->rows == right->rows && left->cols == right->cols &&
           left->arraySize == right->arraySize &&
           left->structName == right->structName &&
           left->elementType == right->elementType &&
           left->members == right->members;
} // HlslBuiltinTypeEqual

static int HlslBuiltinMatchesTexture(const HlslBuiltinDesc *desc,
                                     const HlslType *result,
                                     const HlslType *params, int count)
{
    HlslType expected;
    int i;

    if (desc == NULL || desc->textureForm == HLSL_TEXTURE_NONE ||
        result == NULL || params == NULL || count != desc->arity ||
        count < 2 || count > HLSL_MAX_BUILTIN_ARGS)
    {
        return 0;
    }
    expected = HlslNumericType(HLSL_BASE_FLOAT, 4);
    if (!HlslBuiltinTypeEqual(result, &expected))
        return 0;
    expected = HlslNumericType(desc->samplerBase, 1);
    if (!HlslBuiltinTypeEqual(&params[0], &expected))
        return 0;
    expected = HlslNumericType(HLSL_BASE_FLOAT, desc->coordWidth);
    for (i = 1; i < count; i++) {
        if (!HlslBuiltinTypeEqual(&params[i], &expected))
            return 0;
    }
    return desc->textureForm == HLSL_TEXTURE_GRADIENT ? count == 4 :
           count == 2;
} // HlslBuiltinMatchesTexture

static int HlslBuiltinMulResult(const HlslType *left,
                                const HlslType *right,
                                const HlslType *result)
{
    HlslType expected;

    if (HlslBuiltinVectorType(left, HLSL_BUILTIN_BASE_FLOAT) &&
        HlslBuiltinVectorType(right, HLSL_BUILTIN_BASE_FLOAT) &&
        left->len == 1 && right->len == 1)
    {
        expected = HlslNumericType(HLSL_BASE_FLOAT, 1);
    } else if (HlslBuiltinMatrixType(left) &&
               HlslBuiltinVectorType(right, HLSL_BUILTIN_BASE_FLOAT) &&
               left->cols == right->len)
    {
        expected = HlslNumericType(HLSL_BASE_FLOAT, left->rows);
    } else if (HlslBuiltinVectorType(left, HLSL_BUILTIN_BASE_FLOAT) &&
               HlslBuiltinMatrixType(right) &&
               left->len == right->rows)
    {
        expected = HlslNumericType(HLSL_BASE_FLOAT, right->cols);
    } else if (HlslBuiltinMatrixType(left) &&
               HlslBuiltinMatrixType(right) &&
               left->cols == right->rows)
    {
        expected = HlslMatrixType(left->rows, right->cols);
    } else {
        return 0;
    }
    return HlslBuiltinTypeEqual(result, &expected);
} // HlslBuiltinMulResult

static int HlslBuiltinReplicatedWidths(const HlslType *params,
                                       int count, unsigned baseMask,
                                       int *width, HlslBase *base)
{
    int i;
    int widest;

    widest = 1;
    *base = HLSL_BASE_VOID;
    for (i = 0; i < count; i++) {
        if (!HlslBuiltinVectorType(&params[i], baseMask) ||
            (*base != HLSL_BASE_VOID && params[i].base != *base) ||
            (params[i].len > 1 && widest > 1 && params[i].len != widest))
        {
            return 0;
        }
        *base = params[i].base;
        if (params[i].len > widest)
            widest = params[i].len;
    }
    *width = widest;
    return 1;
} // HlslBuiltinReplicatedWidths

static int HlslBuiltinMatchesNormalized(const HlslBuiltinDesc *desc,
                                        const HlslType *result,
                                        const HlslType *params, int count)
{
    HlslType expected;
    HlslBase base;
    int width;
    int i;

    if (desc == NULL || result == NULL || params == NULL ||
        count != desc->arity)
    {
        return 0;
    }
    if (desc->textureForm != HLSL_TEXTURE_NONE)
        return HlslBuiltinMatchesTexture(desc, result, params, count);
    if (desc->widthPattern == HLSL_BUILTIN_WIDTH_MUL)
        return HlslBuiltinMulResult(&params[0], &params[1], result);
    if (desc->widthPattern == HLSL_BUILTIN_WIDTH_CROSS) {
        expected = HlslNumericType(HLSL_BASE_FLOAT, 3);
        return HlslBuiltinTypeEqual(&params[0], &expected) &&
               HlslBuiltinTypeEqual(&params[1], &expected) &&
               HlslBuiltinTypeEqual(result, &expected);
    }
    if (!HlslBuiltinReplicatedWidths(params, count, desc->baseMask,
                                     &width, &base))
    {
        return 0;
    }
    if (desc->widthPattern == HLSL_BUILTIN_WIDTH_MATCH) {
        for (i = 1; i < count; i++) {
            if (!HlslBuiltinTypeEqual(&params[0], &params[i]))
                return 0;
        }
        width = params[0].len;
    } else if (desc->widthPattern == HLSL_BUILTIN_WIDTH_REFRACT) {
        if (!HlslBuiltinTypeEqual(&params[0], &params[1]) ||
            params[2].base != HLSL_BASE_FLOAT || params[2].len != 1)
        {
            return 0;
        }
        width = params[0].len;
        base = params[0].base;
    }
    switch (desc->resultPattern) {
    case HLSL_BUILTIN_RESULT_SAME:
        expected = params[0];
        break;
    case HLSL_BUILTIN_RESULT_WIDEST:
        expected = HlslNumericType(base, width);
        break;
    case HLSL_BUILTIN_RESULT_SCALAR:
        expected = HlslNumericType(base, 1);
        break;
    case HLSL_BUILTIN_RESULT_MUL:
    default:
        return 0;
    }
    return HlslBuiltinTypeEqual(result, &expected);
} // HlslBuiltinMatchesNormalized

static unsigned HlslBuiltinSourceBaseMask(HlslSourceBase base)
{
    switch (base) {
    case HLSL_SOURCE_BASE_FIXED: return HLSL_BUILTIN_SOURCE_FIXED;
    case HLSL_SOURCE_BASE_HALF: return HLSL_BUILTIN_SOURCE_HALF;
    case HLSL_SOURCE_BASE_FLOAT: return HLSL_BUILTIN_SOURCE_FLOAT;
    case HLSL_SOURCE_BASE_INT: return HLSL_BUILTIN_SOURCE_INT;
    case HLSL_SOURCE_BASE_BOOL: return HLSL_BUILTIN_SOURCE_BOOL;
    default: return 0;
    }
} // HlslBuiltinSourceBaseMask

static unsigned HlslBuiltinAllowedSourceBases(const HlslBuiltinDesc *desc)
{
    unsigned mask;

    if (desc->sourceBaseMask != 0)
        return desc->sourceBaseMask;
    mask = 0;
    if ((desc->baseMask & HLSL_BUILTIN_BASE_FLOAT) != 0)
        mask |= HLSL_BUILTIN_SOURCE_FLOATING;
    if ((desc->baseMask & HLSL_BUILTIN_BASE_INT) != 0)
        mask |= HLSL_BUILTIN_SOURCE_INT;
    if ((desc->baseMask & HLSL_BUILTIN_BASE_BOOL) != 0)
        mask |= HLSL_BUILTIN_SOURCE_BOOL;
    return mask;
} // HlslBuiltinAllowedSourceBases

static int HlslSourceTypeEqual(const HlslSourceType *left,
                               const HlslSourceType *right)
{
    return left != NULL && right != NULL &&
           left->base == right->base && left->shape == right->shape &&
           left->rows == right->rows && left->cols == right->cols;
} // HlslSourceTypeEqual

static int HlslBuiltinMatchesSourceTexture(const HlslBuiltinDesc *desc,
                                           const HlslSourceType *result,
                                           const HlslSourceType *params,
                                           int count)
{
    HlslSourceType expected;
    int i;

    if (desc == NULL || desc->textureForm == HLSL_TEXTURE_NONE ||
        result == NULL || params == NULL || count != desc->arity ||
        count < 2 || count > HLSL_MAX_BUILTIN_ARGS ||
        result->shape != HLSL_SOURCE_SHAPE_VECTOR ||
        result->rows != 0 || result->cols != 4 ||
        (HlslBuiltinSourceBaseMask(result->base) &
         desc->sourceResultMask) == 0)
    {
        return 0;
    }
    expected = HlslSourceScalarType(desc->sourceSamplerBase);
    if (!HlslSourceTypeEqual(&params[0], &expected))
        return 0;
    expected = desc->coordWidth == 1 ?
        HlslSourceScalarType(HLSL_SOURCE_BASE_FLOAT) :
        HlslSourceVectorType(HLSL_SOURCE_BASE_FLOAT, desc->coordWidth);
    for (i = 1; i < count; i++) {
        if (!HlslSourceTypeEqual(&params[i], &expected))
            return 0;
    }
    return desc->textureForm == HLSL_TEXTURE_GRADIENT ? count == 4 :
           count == 2;
} // HlslBuiltinMatchesSourceTexture

static int HlslSourceScalarOrVector(const HlslSourceType *type,
                                    unsigned baseMask)
{
    return type != NULL &&
           (HlslBuiltinSourceBaseMask(type->base) & baseMask) != 0 &&
           ((type->shape == HLSL_SOURCE_SHAPE_SCALAR &&
             type->rows == 0 && type->cols == 0) ||
            (type->shape == HLSL_SOURCE_SHAPE_VECTOR &&
             type->rows == 0 && type->cols >= 1 && type->cols <= 4));
} // HlslSourceScalarOrVector

static int HlslSourceMatrix(const HlslSourceType *type,
                            unsigned baseMask)
{
    return type != NULL && type->shape == HLSL_SOURCE_SHAPE_MATRIX &&
           (HlslBuiltinSourceBaseMask(type->base) & baseMask) != 0 &&
           type->rows >= 1 && type->rows <= 4 &&
           type->cols >= 1 && type->cols <= 4;
} // HlslSourceMatrix

static int HlslSourceMulResult(const HlslSourceType *left,
                               const HlslSourceType *right,
                               const HlslSourceType *result,
                               unsigned baseMask)
{
    HlslSourceType expected;

    if (left->base != right->base)
        return 0;
    if (HlslSourceScalarOrVector(left, baseMask) &&
        HlslSourceScalarOrVector(right, baseMask) &&
        left->shape == HLSL_SOURCE_SHAPE_SCALAR &&
        right->shape == HLSL_SOURCE_SHAPE_SCALAR)
    {
        expected = HlslSourceScalarType(left->base);
    } else if (HlslSourceMatrix(left, baseMask) &&
               HlslSourceScalarOrVector(right, baseMask) &&
               right->shape == HLSL_SOURCE_SHAPE_VECTOR &&
               left->cols == right->cols)
    {
        expected = HlslSourceVectorType(left->base, left->rows);
    } else if (HlslSourceScalarOrVector(left, baseMask) &&
               left->shape == HLSL_SOURCE_SHAPE_VECTOR &&
               HlslSourceMatrix(right, baseMask) &&
               left->cols == right->rows)
    {
        expected = HlslSourceVectorType(left->base, right->cols);
    } else if (HlslSourceMatrix(left, baseMask) &&
               HlslSourceMatrix(right, baseMask) &&
               left->cols == right->rows)
    {
        expected = HlslSourceMatrixType(left->base,
                                        left->rows, right->cols);
    } else {
        return 0;
    }
    return HlslSourceTypeEqual(result, &expected);
} // HlslSourceMulResult

static int HlslSourceReplicated(const HlslSourceType *params, int count,
                                unsigned baseMask, HlslSourceBase *base,
                                HlslSourceShape *shape, int *width)
{
    int i;

    *base = HLSL_SOURCE_BASE_NONE;
    *shape = HLSL_SOURCE_SHAPE_SCALAR;
    *width = 0;
    for (i = 0; i < count; i++) {
        if (!HlslSourceScalarOrVector(&params[i], baseMask) ||
            (*base != HLSL_SOURCE_BASE_NONE && params[i].base != *base) ||
            (params[i].shape == HLSL_SOURCE_SHAPE_VECTOR &&
             *shape == HLSL_SOURCE_SHAPE_VECTOR &&
             params[i].cols != *width))
        {
            return 0;
        }
        *base = params[i].base;
        if (params[i].shape == HLSL_SOURCE_SHAPE_VECTOR) {
            *shape = HLSL_SOURCE_SHAPE_VECTOR;
            *width = params[i].cols;
        }
    }
    return 1;
} // HlslSourceReplicated

static int HlslBuiltinMatchesSource(const HlslBuiltinDesc *desc,
                                    const HlslSourceType *result,
                                    const HlslSourceType *params, int count)
{
    HlslSourceType expected;
    HlslSourceBase base;
    HlslSourceShape shape;
    unsigned baseMask;
    int width;
    int i;

    if (desc == NULL || result == NULL || params == NULL ||
        count != desc->arity)
    {
        return 0;
    }
    if (desc->textureForm != HLSL_TEXTURE_NONE) {
        return HlslBuiltinMatchesSourceTexture(desc, result,
                                               params, count);
    }
    baseMask = HlslBuiltinAllowedSourceBases(desc);
    if (desc->widthPattern == HLSL_BUILTIN_WIDTH_MUL) {
        return HlslSourceMulResult(&params[0], &params[1], result,
                                   baseMask);
    }
    if (desc->widthPattern == HLSL_BUILTIN_WIDTH_CROSS) {
        expected = HlslSourceVectorType(HLSL_SOURCE_BASE_FLOAT, 3);
        return HlslSourceTypeEqual(&params[0], &expected) &&
               HlslSourceTypeEqual(&params[1], &expected) &&
               HlslSourceTypeEqual(result, &expected);
    }
    if (desc->widthPattern == HLSL_BUILTIN_WIDTH_REFRACT) {
        return HlslSourceScalarOrVector(&params[0], baseMask) &&
               HlslSourceTypeEqual(&params[0], &params[1]) &&
               params[2].base == HLSL_SOURCE_BASE_FLOAT &&
               params[2].shape == HLSL_SOURCE_SHAPE_SCALAR &&
               HlslSourceTypeEqual(result, &params[0]);
    }
    if (!HlslSourceReplicated(params, count, baseMask,
                              &base, &shape, &width))
    {
        return 0;
    }
    if (desc->widthPattern == HLSL_BUILTIN_WIDTH_MATCH) {
        for (i = 1; i < count; i++) {
            if (!HlslSourceTypeEqual(&params[0], &params[i]))
                return 0;
        }
        base = params[0].base;
        shape = params[0].shape;
        width = params[0].cols;
    }
    switch (desc->resultPattern) {
    case HLSL_BUILTIN_RESULT_SAME:
        expected = params[0];
        break;
    case HLSL_BUILTIN_RESULT_WIDEST:
        expected = shape == HLSL_SOURCE_SHAPE_VECTOR ?
                   HlslSourceVectorType(base, width) :
                   HlslSourceScalarType(base);
        break;
    case HLSL_BUILTIN_RESULT_SCALAR:
        expected = HlslSourceScalarType(base);
        break;
    case HLSL_BUILTIN_RESULT_MUL:
    default:
        return 0;
    }
    return HlslSourceTypeEqual(result, &expected);
} // HlslBuiltinMatchesSource

HlslBuiltin HlslLookupBuiltin(HlslStage stage, const char *name,
                              const HlslType *result,
                              const HlslType *params, int paramCount)
{
    unsigned stageMask;
    int i;

    if (stage == HLSL_STAGE_VERTEX)
        stageMask = HLSL_BUILTIN_STAGE_VERTEX;
    else if (stage == HLSL_STAGE_PIXEL)
        stageMask = HLSL_BUILTIN_STAGE_PIXEL;
    else if (stage == HLSL_STAGE_GEOMETRY)
        stageMask = HLSL_BUILTIN_STAGE_GEOMETRY;
    else
        return HLSL_BUILTIN_NONE;
    if (name == NULL)
        return HLSL_BUILTIN_NONE;
    for (i = 0; i < HLSL_BUILTIN_TABLE_COUNT; i++) {
        if ((hlslBuiltinTable[i].stageMask & stageMask) != 0 &&
            !strcmp(name, hlslBuiltinTable[i].source) &&
            HlslBuiltinMatchesNormalized(&hlslBuiltinTable[i], result,
                                         params, paramCount))
        {
            return hlslBuiltinTable[i].builtin;
        }
    }
    return HLSL_BUILTIN_NONE;
} // HlslLookupBuiltin

HlslBuiltin HlslLookupSourceBuiltin(HlslStage stage, const char *name,
                                    const HlslSourceType *result,
                                    const HlslSourceType *params,
                                    int paramCount)
{
    unsigned stageMask;
    int i;

    if (stage == HLSL_STAGE_VERTEX)
        stageMask = HLSL_BUILTIN_STAGE_VERTEX;
    else if (stage == HLSL_STAGE_PIXEL)
        stageMask = HLSL_BUILTIN_STAGE_PIXEL;
    else if (stage == HLSL_STAGE_GEOMETRY)
        stageMask = HLSL_BUILTIN_STAGE_GEOMETRY;
    else
        return HLSL_BUILTIN_NONE;
    if (name == NULL)
        return HLSL_BUILTIN_NONE;
    for (i = 0; i < HLSL_BUILTIN_TABLE_COUNT; i++) {
        if ((hlslBuiltinTable[i].stageMask & stageMask) != 0 &&
            !strcmp(name, hlslBuiltinTable[i].source) &&
            HlslBuiltinMatchesSource(&hlslBuiltinTable[i], result,
                                     params, paramCount))
        {
            return hlslBuiltinTable[i].builtin;
        }
    }
    return HLSL_BUILTIN_NONE;
} // HlslLookupSourceBuiltin

int HlslIsBuiltinName(const char *name)
{
    int i;

    if (name == NULL)
        return 0;
    for (i = 0; i < HLSL_BUILTIN_TABLE_COUNT; i++) {
        if (!strcmp(name, hlslBuiltinTable[i].source))
            return 1;
    }
    return HlslIsTextureName(name);
} // HlslIsBuiltinName

int HlslIsTextureName(const char *name)
{
    int i;

    if (name == NULL)
        return 0;
    for (i = 0; i < HLSL_BUILTIN_TABLE_COUNT; i++) {
        if (hlslBuiltinTable[i].textureForm != HLSL_TEXTURE_NONE &&
            !strcmp(name, hlslBuiltinTable[i].source))
        {
            return 1;
        }
    }
    for (i = 0; i < (int) (sizeof(hlslRejectedTextureNames) /
                            sizeof(hlslRejectedTextureNames[0])); i++)
    {
        if (!strcmp(name, hlslRejectedTextureNames[i]))
            return 1;
    }
    return 0;
} // HlslIsTextureName

static const HlslBuiltinDesc *HlslBuiltinDescription(HlslBuiltin builtin)
{
    int i;

    for (i = 0; i < HLSL_BUILTIN_TABLE_COUNT; i++) {
        if (hlslBuiltinTable[i].builtin == builtin)
            return &hlslBuiltinTable[i];
    }
    return NULL;
} // HlslBuiltinDescription

int HlslBuiltinAccepts(HlslStage stage, HlslBuiltin builtin,
                       const HlslType *result,
                       const HlslType *params, int paramCount)
{
    const HlslBuiltinDesc *desc;
    unsigned stageMask;

    desc = HlslBuiltinDescription(builtin);
    if (desc == NULL)
        return 0;
    if (stage == HLSL_STAGE_VERTEX)
        stageMask = HLSL_BUILTIN_STAGE_VERTEX;
    else if (stage == HLSL_STAGE_PIXEL)
        stageMask = HLSL_BUILTIN_STAGE_PIXEL;
    else if (stage == HLSL_STAGE_GEOMETRY)
        stageMask = HLSL_BUILTIN_STAGE_GEOMETRY;
    else
        return 0;
    return (desc->stageMask & stageMask) != 0 &&
           HlslBuiltinMatchesNormalized(desc, result, params, paramCount);
} // HlslBuiltinAccepts

const char *HlslBuiltinSpelling(HlslBuiltin builtin)
{
    const HlslBuiltinDesc *desc;

    desc = HlslBuiltinDescription(builtin);
    return desc != NULL ? desc->hlsl : NULL;
} // HlslBuiltinSpelling

HlslBuiltinLowering HlslBuiltinLoweringKind(HlslBuiltin builtin)
{
    const HlslBuiltinDesc *desc;

    desc = HlslBuiltinDescription(builtin);
    return desc != NULL ? desc->lowering : HLSL_BUILTIN_LOWER_NATIVE;
} // HlslBuiltinLoweringKind

int HlslBuiltinIsTexture(HlslBuiltin builtin)
{
    const HlslBuiltinDesc *desc;

    desc = HlslBuiltinDescription(builtin);
    return desc != NULL && desc->textureForm != HLSL_TEXTURE_NONE;
} // HlslBuiltinIsTexture

HlslTextureForm HlslBuiltinTextureForm(HlslBuiltin builtin)
{
    const HlslBuiltinDesc *desc;

    desc = HlslBuiltinDescription(builtin);
    return desc != NULL ? desc->textureForm : HLSL_TEXTURE_NONE;
} // HlslBuiltinTextureForm

HlslBase HlslBuiltinSamplerBase(HlslBuiltin builtin)
{
    const HlslBuiltinDesc *desc;

    desc = HlslBuiltinDescription(builtin);
    return desc != NULL && desc->textureForm != HLSL_TEXTURE_NONE ?
           desc->samplerBase : HLSL_BASE_VOID;
} // HlslBuiltinSamplerBase

int HlslBuiltinTextureCoordWidth(HlslBuiltin builtin)
{
    const HlslBuiltinDesc *desc;

    desc = HlslBuiltinDescription(builtin);
    return desc != NULL && desc->textureForm != HLSL_TEXTURE_NONE ?
           desc->coordWidth : 0;
} // HlslBuiltinTextureCoordWidth

static int HlslDeclListHasCycle(const HlslDecl *list)
{
    const HlslDecl *slow;
    const HlslDecl *fast;

    slow = list;
    fast = list;
    while (fast != NULL && fast->next != NULL) {
        slow = slow->next;
        fast = fast->next->next;
        if (slow == fast)
            return 1;
    }
    return 0;
} // HlslDeclListHasCycle

static int HlslTypeRegisterSpanInner(const HlslType *type,
    const HlslTypeFrame *parent)
{
    HlslTypeFrame frame;
    const HlslDecl *member;
    int memberSpan;
    int span;

    if (type == NULL || type->arraySize < 0 ||
        HlslTypeFrameContains(parent, type))
    {
        return 0;
    }
    frame.parent = parent;
    frame.type = type;
    if (type->arraySize > 0) {
        if (type->elementType == NULL)
            return 0;
        span = HlslTypeRegisterSpanInner(type->elementType, &frame);
        if (span <= 0 || type->arraySize > INT_MAX / span)
            return 0;
        return type->arraySize * span;
    }
    if (type->elementType != NULL)
        return 0;
    if (type->base == HLSL_BASE_STRUCT) {
        if (type->len != 0 || type->rows != 0 || type->cols != 0 ||
            type->structName == NULL || type->structName[0] == '\0' ||
            HlslDeclListHasCycle(type->members))
        {
            return 0;
        }
        span = 0;
        for (member = type->members; member != NULL; member = member->next) {
            memberSpan = HlslTypeRegisterSpanInner(&member->type, &frame);
            if (memberSpan <= 0 || span > INT_MAX - memberSpan)
                return 0;
            span += memberSpan;
        }
        return span;
    }
    if (type->structName != NULL || type->members != NULL)
        return 0;
    if (type->rows != 0 || type->cols != 0) {
        if (type->base != HLSL_BASE_FLOAT || type->len != 0 ||
            type->rows < 1 || type->rows > 4 ||
            type->cols < 1 || type->cols > 4)
        {
            return 0;
        }
        return type->rows;
    }
    switch (type->base) {
    case HLSL_BASE_FLOAT:
    case HLSL_BASE_INT:
    case HLSL_BASE_UINT:
        return type->len >= 1 && type->len <= 4 ? 1 : 0;
    case HLSL_BASE_BOOL:
        /* Shader Model 3 exposes b# as scalar boolean registers. */
        return type->len >= 1 && type->len <= 4 ? type->len : 0;
    case HLSL_BASE_SAMPLER1D:
    case HLSL_BASE_SAMPLER2D:
    case HLSL_BASE_SAMPLER3D:
    case HLSL_BASE_SAMPLERCUBE:
        return type->len == 1 ? 1 : 0;
    case HLSL_BASE_TEXTURE1D:
    case HLSL_BASE_TEXTURE2D:
    case HLSL_BASE_TEXTURE3D:
    case HLSL_BASE_TEXTURECUBE:
    case HLSL_BASE_SAMPLER_STATE:
    case HLSL_BASE_GEOMETRY_STREAM:
    case HLSL_BASE_VOID:
    case HLSL_BASE_STRUCT:
        return 0;
    }
    return 0;
}

int HlslTypeRegisterSpan(const HlslType *type)
{
    return HlslTypeRegisterSpanInner(type, NULL);
}

int HlslIsReservedName(const char *name)
{
    int i;
    const char *suffix;
    size_t length;

    if (name == NULL || !strncmp(name, "cg_", 3))
        return 1;
    for (i = 0; i < (int) (sizeof(reservedNames) /
                            sizeof(reservedNames[0])); i++)
    {
        if (!strcmp(name, reservedNames[i]))
            return 1;
    }
    for (i = 0; i < (int) (sizeof(reservedTypeBases) /
                            sizeof(reservedTypeBases[0])); i++)
    {
        length = strlen(reservedTypeBases[i]);
        if (strncmp(name, reservedTypeBases[i], length))
            continue;
        suffix = name + length;
        if (suffix[0] == '\0')
            return 1;
        if (suffix[0] >= '1' && suffix[0] <= '4' && suffix[1] == '\0')
            return 1;
        if (suffix[0] >= '1' && suffix[0] <= '4' && suffix[1] == 'x' &&
            suffix[2] >= '1' && suffix[2] <= '4' && suffix[3] == '\0')
        {
            return 1;
        }
    }
    return 0;
}

int HlslReservedNameCount(void)
{
    return (int) (sizeof(reservedNames) / sizeof(reservedNames[0]));
}

const char *HlslReservedNameAt(int index)
{
    if (index < 0 || index >= HlslReservedNameCount())
        return NULL;
    return reservedNames[index];
}

HlslDecl *HlslNewDecl(HlslModule *module, HlslStorage storage,
    HlslType type, const char *name)
{
    HlslDecl *decl;

    decl = (HlslDecl *) HlslAlloc(module, sizeof(HlslDecl));
    if (decl != NULL) {
        decl->storage = storage;
        decl->type = type;
        decl->name = name;
        decl->semanticKind = HLSL_SEMANTIC_USER;
        decl->semanticIndex = 0;
        decl->canonicalSemantic = NULL;
        decl->interpolation = HLSL_INTERPOLATION_DEFAULT;
    }
    return decl;
}

HlslExpr *HlslNewExpr(HlslModule *module, HlslExprKind kind,
    HlslType type)
{
    HlslExpr *expr;

    expr = (HlslExpr *) HlslAlloc(module, sizeof(HlslExpr));
    if (expr != NULL) {
        expr->kind = kind;
        expr->type = type;
    }
    return expr;
}

HlslExpr *HlslNewLocatedExpr(HlslModule *module, HlslExprKind kind,
    HlslType type, const HlslLoc *loc)
{
    HlslExpr *expr;

    expr = HlslNewExpr(module, kind, type);
    if (expr != NULL && loc != NULL)
        expr->loc = *loc;
    return expr;
}

int HlslExprIsPure(const HlslExpr *expr)
{
    const HlslExpr *argument;

    if (expr == NULL)
        return 1;
    if (expr->hasSideEffects)
        return 0;
    switch (expr->kind) {
    case HLSL_EXPR_SYMBOL:
    case HLSL_EXPR_INT:
    case HLSL_EXPR_FLOAT:
    case HLSL_EXPR_BOOL:
        return 1;
    case HLSL_EXPR_UNARY:
        if (expr->u.unary.op == HLSL_OP_PRE_INCREMENT ||
            expr->u.unary.op == HLSL_OP_POST_INCREMENT ||
            expr->u.unary.op == HLSL_OP_PRE_DECREMENT ||
            expr->u.unary.op == HLSL_OP_POST_DECREMENT)
        {
            return 0;
        }
        return HlslExprIsPure(expr->u.unary.operand);
    case HLSL_EXPR_BINARY:
        if (expr->u.binary.op >= HLSL_OP_ASSIGN &&
            expr->u.binary.op <= HLSL_OP_SHIFT_RIGHT_ASSIGN)
        {
            return 0;
        }
        return HlslExprIsPure(expr->u.binary.left) &&
               HlslExprIsPure(expr->u.binary.right);
    case HLSL_EXPR_CONDITIONAL:
        return HlslExprIsPure(expr->u.conditional.condition) &&
               HlslExprIsPure(expr->u.conditional.trueExpr) &&
               HlslExprIsPure(expr->u.conditional.falseExpr);
    case HLSL_EXPR_CALL:
        for (argument = expr->u.call.arguments; argument != NULL;
             argument = argument->next)
        {
            if (!HlslExprIsPure(argument))
                return 0;
        }
        return 1;
    case HLSL_EXPR_CONSTRUCT:
        for (argument = expr->u.construct.arguments; argument != NULL;
             argument = argument->next)
        {
            if (!HlslExprIsPure(argument))
                return 0;
        }
        return 1;
    case HLSL_EXPR_CAST:
        return HlslExprIsPure(expr->u.cast.expression);
    case HLSL_EXPR_MEMBER:
        return HlslExprIsPure(expr->u.member.object);
    case HLSL_EXPR_INDEX:
        return HlslExprIsPure(expr->u.index.object) &&
               HlslExprIsPure(expr->u.index.index);
    case HLSL_EXPR_SWIZZLE:
        return HlslExprIsPure(expr->u.swizzle.object);
    case HLSL_EXPR_TEXTURE_METHOD:
        return HlslExprIsPure(expr->u.textureMethod.texture) &&
               HlslExprIsPure(expr->u.textureMethod.sampler) &&
               HlslExprIsPure(expr->u.textureMethod.coordinates) &&
               HlslExprIsPure(expr->u.textureMethod.argument1) &&
               HlslExprIsPure(expr->u.textureMethod.argument2);
    }
    return 0;
} // HlslExprIsPure

HlslStmt *HlslNewStmt(HlslModule *module, HlslStmtKind kind)
{
    HlslStmt *stmt;

    stmt = (HlslStmt *) HlslAlloc(module, sizeof(HlslStmt));
    if (stmt != NULL)
        stmt->kind = kind;
    return stmt;
}

HlslFunction *HlslNewFunction(HlslModule *module, HlslType result,
    const char *name)
{
    HlslFunction *function;

    function = (HlslFunction *) HlslAlloc(module, sizeof(HlslFunction));
    if (function != NULL) {
        function->result = result;
        function->name = name;
    }
    return function;
}

HlslBinding *HlslNewBinding(HlslModule *module, HlslStorage storage,
    HlslType type, const char *name, const char *semantic)
{
    HlslBinding *binding;

    binding = (HlslBinding *) HlslAlloc(module, sizeof(HlslBinding));
    if (binding != NULL) {
        binding->storage = storage;
        binding->type = type;
        binding->name = name;
        binding->publicName = name;
        binding->semantic = semantic;
    }
    return binding;
}

HlslExpr *HlslNewTextureMethod(HlslModule *module, HlslTextureMethod method,
    HlslExpr *texture, HlslExpr *sampler, HlslExpr *coordinates,
    HlslExpr *argument1, HlslExpr *argument2, HlslType result,
    HlslLoc loc)
{
    HlslExpr *expression;

    if (module == NULL || texture == NULL || sampler == NULL ||
        coordinates == NULL || method < HLSL_TEXTURE_METHOD_SAMPLE ||
        method > HLSL_TEXTURE_METHOD_SAMPLE_GRAD)
    {
        return NULL;
    }
    expression = HlslNewLocatedExpr(module, HLSL_EXPR_TEXTURE_METHOD,
                                    result, &loc);
    if (expression != NULL) {
        expression->u.textureMethod.method = method;
        expression->u.textureMethod.texture = texture;
        expression->u.textureMethod.sampler = sampler;
        expression->u.textureMethod.coordinates = coordinates;
        expression->u.textureMethod.argument1 = argument1;
        expression->u.textureMethod.argument2 = argument2;
    }
    return expression;
} // HlslNewTextureMethod

HlslResource *HlslNewResource(HlslModule *module, HlslResourceKind kind,
    HlslType type, const char *name, HlslLoc loc)
{
    HlslResource *resource;
    HlslResource *last;

    if (module == NULL || kind < HLSL_RESOURCE_CBUFFER ||
        kind > HLSL_RESOURCE_SAMPLER)
    {
        return NULL;
    }
    resource = (HlslResource *) HlslAlloc(module, sizeof(HlslResource));
    if (resource == NULL)
        return NULL;
    resource->owner = module;
    resource->kind = kind;
    resource->type = type;
    resource->name = name;
    resource->loc = loc;
    resource->binding.kind = kind;
    resource->binding.slot = -1;
    resource->binding.pairId = -1;
    if (module->resources == NULL) {
        module->resources = resource;
    } else {
        last = module->resources;
        while (last->next != NULL)
            last = last->next;
        last->next = resource;
    }
    return resource;
} // HlslNewResource

static int HlslModuleOwnsResource(const HlslModule *module,
                                  const HlslResource *resource)
{
    const HlslResource *current;
    const HlslResource *slow;
    const HlslResource *fast;

    slow = module->resources;
    fast = module->resources;
    while (fast != NULL && fast->next != NULL) {
        slow = slow->next;
        fast = fast->next->next;
        if (slow == fast)
            return 0;
    }
    for (current = module->resources; current != NULL;
         current = current->next)
    {
        if (current == resource)
            return 1;
    }
    return 0;
} // HlslModuleOwnsResource

int HlslBindResource(HlslModule *module, HlslResource *resource,
    int slot, int pairId)
{
    if (module == NULL || resource == NULL || resource->owner != module ||
        !HlslModuleOwnsResource(module, resource) ||
        resource->kind < HLSL_RESOURCE_CBUFFER ||
        resource->kind > HLSL_RESOURCE_SAMPLER ||
        resource->binding.kind != resource->kind || slot < 0 ||
        (resource->kind == HLSL_RESOURCE_CBUFFER && pairId != -1) ||
        (resource->kind != HLSL_RESOURCE_CBUFFER && pairId < 0))
    {
        return 0;
    }
    resource->binding.slot = slot;
    resource->binding.pairId = pairId;
    return 1;
} // HlslBindResource

int HlslSetPackOffset(HlslModule *module, HlslDecl *field,
    HlslPackOffset offset)
{
    int aggregate;

    if (module == NULL || field == NULL || offset.vector < 0 ||
        offset.component < 0 || offset.component > 3 ||
        offset.componentCount <= 0)
    {
        return 0;
    }
    aggregate = field->type.arraySize > 0 || field->type.rows > 0 ||
                field->type.cols > 0 ||
                field->type.base == HLSL_BASE_STRUCT;
    if ((aggregate && offset.component != 0) ||
        (!aggregate &&
         (offset.componentCount > 4 ||
          offset.component + offset.componentCount > 4)))
    {
        return 0;
    }
    field->hasPackOffset = 1;
    field->packOffset = offset;
    return 1;
} // HlslSetPackOffset

static int HlslGeometryInputExtent(HlslGeometryInput input)
{
    switch (input) {
    case HLSL_GEOMETRY_INPUT_POINT: return 1;
    case HLSL_GEOMETRY_INPUT_LINE: return 2;
    case HLSL_GEOMETRY_INPUT_LINE_ADJ: return 4;
    case HLSL_GEOMETRY_INPUT_TRIANGLE: return 3;
    case HLSL_GEOMETRY_INPUT_TRIANGLE_ADJ: return 6;
    }
    return 0;
} // HlslGeometryInputExtent

int HlslSetGeometryLayout(HlslModule *module, HlslGeometryInput input,
    HlslGeometryStream stream, int inputCount, int maxVertices)
{
    if (module == NULL || module->stage != HLSL_STAGE_GEOMETRY ||
        input < HLSL_GEOMETRY_INPUT_POINT ||
        input > HLSL_GEOMETRY_INPUT_TRIANGLE_ADJ ||
        stream < HLSL_GEOMETRY_STREAM_POINT ||
        stream > HLSL_GEOMETRY_STREAM_TRIANGLE ||
        inputCount != HlslGeometryInputExtent(input) || maxVertices <= 0)
    {
        return 0;
    }
    module->geometryInput = input;
    module->geometryStream = stream;
    module->geometryInputCount = inputCount;
    module->geometryMaxVertices = maxVertices;
    return 1;
} // HlslSetGeometryLayout

HlslStmt *HlslNewAppend(HlslModule *module, HlslExpr *record,
    HlslFlatReplay *replay, HlslLoc loc)
{
    HlslStmt *statement;

    statement = HlslNewStmt(module, HLSL_STMT_APPEND);
    if (statement != NULL) {
        statement->loc = loc;
        statement->u.append.record = record;
        statement->u.append.replay = replay;
    }
    return statement;
} // HlslNewAppend

HlslStmt *HlslNewRestartStrip(HlslModule *module, HlslLoc loc)
{
    HlslStmt *statement;

    statement = HlslNewStmt(module, HLSL_STMT_RESTART_STRIP);
    if (statement != NULL)
        statement->loc = loc;
    return statement;
} // HlslNewRestartStrip

HlslFlatReplay *HlslNewFlatReplay(HlslModule *module, HlslDecl *target,
    HlslDecl *shadow, HlslDecl *defined)
{
    HlslFlatReplay *replay;

    replay = (HlslFlatReplay *) HlslAlloc(module, sizeof(HlslFlatReplay));
    if (replay != NULL) {
        replay->owner = module;
        replay->target = target;
        replay->shadow = shadow;
        replay->defined = defined;
    }
    return replay;
} // HlslNewFlatReplay

void HlslAppendDecl(HlslDecl **list, HlslDecl *decl)
{
    HlslDecl *current;

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

void HlslAppendExpr(HlslExpr **list, HlslExpr *expr)
{
    HlslExpr *current;

    if (list == NULL || expr == NULL)
        return;
    if (*list == NULL) {
        *list = expr;
        return;
    }
    for (current = *list; current->next != NULL; current = current->next)
        ;
    current->next = expr;
}

void HlslAppendStmt(HlslStmt **list, HlslStmt *stmt)
{
    HlslStmt *current;

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

void HlslAppendFunction(HlslFunction **list, HlslFunction *function)
{
    HlslFunction *current;

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

void HlslAppendBinding(HlslBinding **list, HlslBinding *binding)
{
    HlslBinding *current;

    if (list == NULL || binding == NULL)
        return;
    if (*list == NULL) {
        *list = binding;
        return;
    }
    for (current = *list; current->next != NULL; current = current->next)
        ;
    current->next = binding;
}

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
    "void", "volatile", "while", "yield"
};

static const char *reservedTypeBases[] = {
    "bool", "cfloat", "char", "cint", "double", "dword", "fixed",
    "float", "half", "int", "long", "short", "uchar", "uint", "ulong",
    "ushort"
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
    if (module != NULL) {
        module->errorKind = HLSL_ERROR_NAME_COLLISION;
        module->errorReason = source;
        memset(&module->errorLoc, 0, sizeof(module->errorLoc));
        module->errors++;
    }
    return NULL;
}

static const char *HlslAllocate(HlslModule *module, const void *nameSpace,
    const void *identity, const char *source)
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
    base = HlslLegalizeName(module, source);
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
                        &defaultNameIdentity, source);
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
                            source);
    }
    for (name = module->names; name != NULL; name = name->next) {
        if (name->nameSpace == nameSpace && name->identity == identity)
            return name->emitted;
    }
    return HlslAllocate(module, nameSpace, identity, source);
}

const char *HlslAllocateDistinctName(HlslModule *module,
    const char *source)
{
    return HlslAllocate(module, &defaultNameNamespace, NULL, source);
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

const char *HlslTypeName(const HlslType *type)
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

    if (type == NULL || type->arraySize < 0)
        return NULL;
    if (type->arraySize > 0) {
        if (type->elementType == NULL || type->elementType == type)
            return NULL;
        return HlslTypeName(type->elementType);
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
    case HLSL_BASE_STRUCT:
        return type->len == 0 && type->structName != NULL &&
               type->structName[0] != '\0' ? type->structName : NULL;
    }
    return NULL;
}

typedef struct HlslTypeSpanFrame_Rec {
    const struct HlslTypeSpanFrame_Rec *parent;
    const HlslType *type;
} HlslTypeSpanFrame;

static int HlslTypeSpanContains(const HlslTypeSpanFrame *frame,
    const HlslType *type)
{
    for (; frame != NULL; frame = frame->parent) {
        if (frame->type == type)
            return 1;
    }
    return 0;
}

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
}

static int HlslTypeRegisterSpanInner(const HlslType *type,
    const HlslTypeSpanFrame *parent)
{
    HlslTypeSpanFrame frame;
    const HlslDecl *member;
    int memberSpan;
    int span;

    if (type == NULL || type->arraySize < 0 ||
        HlslTypeSpanContains(parent, type))
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
    case HLSL_BASE_BOOL:
        return type->len >= 1 && type->len <= 4 ? 1 : 0;
    case HLSL_BASE_SAMPLER1D:
    case HLSL_BASE_SAMPLER2D:
    case HLSL_BASE_SAMPLER3D:
    case HLSL_BASE_SAMPLERCUBE:
        return type->len == 1 ? 1 : 0;
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
        binding->semantic = semantic;
    }
    return binding;
}

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

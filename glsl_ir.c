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

#include <string.h>
#include "glsl_ir.h"

static const int defaultNameIdentity = 0;

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

static int GlslNameInUse(const GlslModule *module, const char *name)
{
    const GlslName *current;

    for (current = module->names; current != NULL; current = current->next) {
        if (!strcmp(current->emitted, name))
            return 1;
    }
    return 0;
}

static GlslName *GlslFindName(const GlslModule *module, const void *identity,
    const char *source)
{
    GlslName *current;

    for (current = module->names; current != NULL; current = current->next) {
        if (current->identity == identity && !strcmp(current->source, source))
            return current;
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

static const char *GlslSanitizeName(GlslModule *module, const char *source)
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
    previousUnderscore = 0;
    while (*input != '\0') {
        if (*input != '_' || !previousUnderscore)
            *output++ = *input;
        previousUnderscore = *input == '_';
        input++;
    }
    *output = '\0';
    if (name[0] == '\0') {
        strcpy(name, "cg_");
    } else if (reserved) {
        if (name[0] == '_')
            memmove(name + 3, name + 1, strlen(name));
        else
            memmove(name + 3, name, strlen(name) + 1);
        memcpy(name, "cg_", 3);
    }
    return name;
}

static const char *GlslAllocate(GlslModule *module, const void *identity,
    const char *source)
{
    const char *base;
    const char *emitted;
    const char *sourceCopy;
    GlslName *name;
    char *candidate;
    size_t length;
    int separator;
    int suffix;

    if (module == NULL || source == NULL)
        return NULL;
    base = GlslSanitizeName(module, source);
    if (base == NULL)
        return NULL;
    emitted = base;
    suffix = 1;
    length = strlen(base);
    while (GlslNameInUse(module, emitted)) {
        separator = base[length - 1] != '_';
        candidate = (char *) GlslAlloc(module,
            length + GlslDecimalLength(suffix) + separator + 1);
        if (candidate == NULL)
            return NULL;
        memcpy(candidate, base, length);
        if (separator) {
            candidate[length] = '_';
            GlslWriteDecimal(candidate + length + 1, suffix);
        } else {
            GlslWriteDecimal(candidate + length, suffix);
        }
        emitted = candidate;
        suffix++;
    }
    name = (GlslName *) GlslAlloc(module, sizeof(GlslName));
    if (name == NULL)
        return NULL;
    sourceCopy = GlslDuplicate(module, source);
    if (sourceCopy == NULL)
        return NULL;
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
    name = GlslFindName(module, &defaultNameIdentity, source);
    if (name != NULL)
        return name->emitted;
    return GlslAllocate(module, &defaultNameIdentity, source);
}

const char *GlslAllocateSymbolName(GlslModule *module, const void *identity,
    const char *source)
{
    GlslName *name;

    if (identity == NULL)
        return GlslAllocateName(module, source);
    if (module == NULL || source == NULL)
        return NULL;
    for (name = module->names; name != NULL; name = name->next) {
        if (name->identity == identity)
            return name->emitted;
    }
    return GlslAllocate(module, identity, source);
}

const char *GlslAllocateDistinctName(GlslModule *module, const char *source)
{
    return GlslAllocate(module, NULL, source);
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
    switch (type->base) {
    case GLSL_BASE_VOID:
        if (type->len == 0 && type->rows == 0 && type->cols == 0)
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

GlslDecl *GlslNewDecl(GlslModule *module, GlslStorage storage, GlslType type,
    const char *name)
{
    GlslDecl *decl;

    decl = (GlslDecl *) GlslAlloc(module, sizeof(GlslDecl));
    if (decl != NULL) {
        decl->storage = storage;
        decl->type = type;
        decl->name = name;
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

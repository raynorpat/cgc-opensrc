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
// glsl_ir_test.c
//

#if defined(NDEBUG)
#undef NDEBUG
#endif
#include <assert.h>
#include <float.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "glsl_ir.h"

int GlslWriteModule(FILE *out, const GlslModule *module);

static void *TestAlloc(void *arg, size_t size)
{
    return calloc(1, size);
}

static int allocationCount;

static void *CountingAlloc(void *arg, size_t size)
{
    allocationCount++;
    return calloc(1, size);
}

static void *DirtyAlloc(void *arg, size_t size)
{
    void *memory;

    memory = malloc(size);
    memset(memory, 0xa5, size);
    return memory;
}

int main(void)
{
    GlslModule module;
    GlslModule dirtyModule;
    GlslModule collisionModule;
    GlslModule countModule;
    GlslModule gapModule;
    GlslModule overflowModule;
    GlslModule scopedModule;
    GlslModule visibleModule;
    GlslModule nonfiniteModule;
    GlslType type;
    GlslDecl *decl;
    GlslDecl *secondDecl;
    GlslExpr *expr;
    GlslStmt *stmt;
    GlslStmt *secondStmt;
    GlslFunction *function;
    GlslFunction *secondFunction;
    GlslBinding *binding;
    GlslDecl *decls;
    GlslStmt *stmts;
    GlslFunction *functions;
    const char *emitted;
    int firstIdentity;
    int secondIdentity;
    int localNamespace;
    int memberNamespace;
    int functionIdentity;
    int typeIdentity;
    int globalIdentity;
    int localIdentity;
    int index;
    FILE *writer;
    float infinity;
    float nanValue;
    GlslType builtinParams[3];
    GlslType arrayElement;
    GlslType arrayType;
    GlslType structType;
    GlslDecl structMembers[2];
    static const char *builtinSpellings[] = {
        NULL,
        "mul", "dot", "cross", "normalize", "reflect", "refract",
        "length", "distance", "min", "max", "clamp", "abs", "sign",
        "floor", "ceil", "sqrt", "exp", "exp2", "log", "log2",
        "sin", "cos", "tan", "asin", "acos", "atan", "inversesqrt",
        "mix", "fract", "clamp", "texture1D", "texture2D",
        "texture3D", "textureCube"
    };

    GlslInitModule(&module, GLSL_STAGE_VERTEX, TestAlloc, NULL);
    assert(!strcmp(GlslAllocateName(&module, "position"), "position"));
    assert(!strcmp(GlslAllocateName(&module, "attribute"), "cg_attribute"));
    assert(!strcmp(GlslAllocateName(&module, "gl_Position"), "cg_gl_Position"));
    assert(!strcmp(GlslAllocateName(&module, "position"), "position"));
    assert(!strcmp(GlslAllocateDistinctName(&module, "position"), "position_1"));

    type = GlslNumericType(GLSL_BASE_FLOAT, 1);
    assert(!strcmp(GlslTypeName(&type), "float"));
    type = GlslNumericType(GLSL_BASE_FLOAT, 4);
    assert(!strcmp(GlslTypeName(&type), "vec4"));
    type = GlslNumericType(GLSL_BASE_INT, 3);
    assert(!strcmp(GlslTypeName(&type), "ivec3"));
    type = GlslMatrixType(3);
    assert(!strcmp(GlslTypeName(&type), "mat3"));

    for (index = GLSL_BUILTIN_MUL; index <= GLSL_BUILTIN_TEXCUBE;
         index++)
    {
        assert(GlslBuiltinSpelling((GlslBuiltin) index) != NULL);
        assert(!strcmp(GlslBuiltinSpelling((GlslBuiltin) index),
                       builtinSpellings[index]));
    }
    assert(GlslBuiltinSpelling(GLSL_BUILTIN_NONE) == NULL);
    assert(GlslBuiltinSpelling((GlslBuiltin) 999) == NULL);

    builtinParams[0] = GlslMatrixType(4);
    builtinParams[1] = GlslNumericType(GLSL_BASE_FLOAT, 4);
    type = GlslNumericType(GLSL_BASE_FLOAT, 4);
    assert(GlslLookupBuiltin("mul", &type, builtinParams, 2) ==
           GLSL_BUILTIN_MUL);
    builtinParams[0] = GlslNumericType(GLSL_BASE_FLOAT, 3);
    builtinParams[1] = GlslNumericType(GLSL_BASE_FLOAT, 3);
    type = GlslNumericType(GLSL_BASE_FLOAT, 1);
    assert(GlslLookupBuiltin("dot", &type, builtinParams, 2) ==
           GLSL_BUILTIN_DOT);
    builtinParams[0] = GlslNumericType(GLSL_BASE_FLOAT, 3);
    type = GlslNumericType(GLSL_BASE_FLOAT, 3);
    assert(GlslLookupBuiltin("normalize", &type, builtinParams, 1) ==
           GLSL_BUILTIN_NORMALIZE);
    builtinParams[0] = GlslNumericType(GLSL_BASE_FLOAT, 3);
    builtinParams[1] = GlslNumericType(GLSL_BASE_FLOAT, 3);
    builtinParams[2] = GlslNumericType(GLSL_BASE_FLOAT, 1);
    type = GlslNumericType(GLSL_BASE_FLOAT, 3);
    assert(GlslLookupBuiltin("refract", &type, builtinParams, 3) ==
           GLSL_BUILTIN_REFRACT);
    builtinParams[0] = GlslNumericType(GLSL_BASE_FLOAT, 3);
    builtinParams[1] = GlslNumericType(GLSL_BASE_FLOAT, 1);
    builtinParams[2] = GlslNumericType(GLSL_BASE_FLOAT, 1);
    assert(GlslLookupBuiltin("clamp", &type, builtinParams, 3) ==
           GLSL_BUILTIN_CLAMP);
    builtinParams[1] = GlslNumericType(GLSL_BASE_FLOAT, 3);
    assert(GlslLookupBuiltin("lerp", &type, builtinParams, 3) ==
           GLSL_BUILTIN_LERP);
    assert(GlslLookupBuiltin("not_a_builtin", &type, builtinParams, 3) ==
           GLSL_BUILTIN_NONE);
    builtinParams[0] = GlslNumericType(GLSL_BASE_INT, 3);
    assert(GlslLookupBuiltin("normalize", &type, builtinParams, 1) ==
           GLSL_BUILTIN_NONE);
    builtinParams[0] = GlslNumericType(GLSL_BASE_FLOAT, 3);
    builtinParams[0].members = structMembers;
    assert(GlslLookupBuiltin("normalize", &type, builtinParams, 1) ==
           GLSL_BUILTIN_NONE);
    builtinParams[0] = GlslMatrixType(3);
    builtinParams[1] = GlslNumericType(GLSL_BASE_FLOAT, 4);
    assert(GlslLookupBuiltin("mul", &type, builtinParams, 2) ==
           GLSL_BUILTIN_NONE);

    type = GlslNumericType(GLSL_BASE_FLOAT, 4);
    builtinParams[0] = GlslNumericType(GLSL_BASE_SAMPLER1D, 1);
    builtinParams[1] = GlslNumericType(GLSL_BASE_FLOAT, 1);
    assert(GlslLookupBuiltin("tex1D", &type, builtinParams, 2) ==
           GLSL_BUILTIN_TEX1D);
    builtinParams[0] = GlslNumericType(GLSL_BASE_SAMPLER2D, 1);
    builtinParams[1] = GlslNumericType(GLSL_BASE_FLOAT, 2);
    assert(GlslLookupBuiltin("tex2D", &type, builtinParams, 2) ==
           GLSL_BUILTIN_TEX2D);
    builtinParams[0] = GlslNumericType(GLSL_BASE_SAMPLER3D, 1);
    builtinParams[1] = GlslNumericType(GLSL_BASE_FLOAT, 3);
    assert(GlslLookupBuiltin("tex3D", &type, builtinParams, 2) ==
           GLSL_BUILTIN_TEX3D);
    builtinParams[0] = GlslNumericType(GLSL_BASE_SAMPLERCUBE, 1);
    assert(GlslLookupBuiltin("texCUBE", &type, builtinParams, 2) ==
           GLSL_BUILTIN_TEXCUBE);
    builtinParams[1] = GlslNumericType(GLSL_BASE_FLOAT, 2);
    assert(GlslLookupBuiltin("texCUBE", &type, builtinParams, 2) ==
           GLSL_BUILTIN_NONE);
    builtinParams[0] = GlslNumericType(GLSL_BASE_SAMPLER2D, 1);
    builtinParams[1] = GlslNumericType(GLSL_BASE_FLOAT, 3);
    assert(GlslLookupBuiltin("tex2D", &type, builtinParams, 2) ==
           GLSL_BUILTIN_NONE);
    type = GlslNumericType(GLSL_BASE_FLOAT, 3);
    builtinParams[1] = GlslNumericType(GLSL_BASE_FLOAT, 2);
    assert(GlslLookupBuiltin("tex2D", &type, builtinParams, 2) ==
           GLSL_BUILTIN_NONE);
    type = GlslNumericType(GLSL_BASE_FLOAT, 4);
    builtinParams[0] = GlslNumericType(GLSL_BASE_SAMPLER3D, 1);
    assert(GlslLookupBuiltin("tex2D", &type, builtinParams, 2) ==
           GLSL_BUILTIN_NONE);
    builtinParams[0] = GlslNumericType(GLSL_BASE_SAMPLER2D, 1);
    builtinParams[2] = GlslNumericType(GLSL_BASE_FLOAT, 1);
    assert(GlslLookupBuiltin("tex2D", &type, builtinParams, 3) ==
           GLSL_BUILTIN_NONE);
    assert(GlslLookupBuiltin("texture2D", &type, builtinParams, 2) ==
           GLSL_BUILTIN_NONE);

    type = GlslNumericType(GLSL_BASE_FLOAT, 4);
    assert(GlslTypeComponentCount(&type) == 4);
    type = GlslMatrixType(3);
    assert(GlslTypeComponentCount(&type) == 9);
    arrayElement = GlslMatrixType(2);
    arrayType = GlslNumericType(GLSL_BASE_VOID, 0);
    arrayType.arraySize = 3;
    arrayType.elementType = &arrayElement;
    assert(GlslTypeComponentCount(&arrayType) == 12);
    arrayType.arraySize = 0;
    assert(GlslTypeComponentCount(&arrayType) == 0);
    arrayElement = GlslNumericType(GLSL_BASE_FLOAT, 2);
    arrayType.arraySize = INT_MAX;
    arrayType.elementType = &arrayElement;
    assert(GlslTypeComponentCount(&arrayType) == 0);
    arrayType.arraySize = 2;
    arrayType.elementType = &arrayType;
    assert(GlslTypeComponentCount(&arrayType) == 0);
    type = GlslNumericType(GLSL_BASE_SAMPLER2D, 1);
    assert(GlslTypeComponentCount(&type) == 0);
    memset(structMembers, 0, sizeof(structMembers));
    structMembers[0].type = GlslNumericType(GLSL_BASE_FLOAT, 3);
    structMembers[0].next = &structMembers[1];
    arrayElement = GlslMatrixType(2);
    arrayType = GlslNumericType(GLSL_BASE_VOID, 0);
    arrayType.arraySize = 2;
    arrayType.elementType = &arrayElement;
    structMembers[1].type = arrayType;
    structType = GlslNumericType(GLSL_BASE_STRUCT, 0);
    structType.structName = "ResourceBlock";
    structType.members = structMembers;
    assert(GlslTypeComponentCount(&structType) == 11);
    memset(structMembers, 0, sizeof(structMembers));
    structType = GlslNumericType(GLSL_BASE_STRUCT, 0);
    structType.structName = "RecursiveBlock";
    structType.members = structMembers;
    structMembers[0].type = structType;
    assert(GlslTypeComponentCount(&structType) == 0);

    assert(!strcmp(GlslAllocateSymbolName(&module, &firstIdentity, "value"),
        "value"));
    assert(!strcmp(GlslAllocateSymbolName(&module, &firstIdentity, "other"),
        "value"));
    assert(!strcmp(GlslAllocateSymbolName(&module, &secondIdentity, "value"),
        "value_1"));

    GlslInitModule(&visibleModule, GLSL_STAGE_VERTEX, TestAlloc, NULL);
    assert(!strcmp(GlslAllocateSymbolName(&visibleModule,
                   &functionIdentity, "main"), "main"));
    assert(!strcmp(GlslAllocateSymbolName(&visibleModule,
                   &typeIdentity, "main"), "main_1"));
    assert(!strcmp(GlslAllocateSymbolName(&visibleModule,
                   &globalIdentity, "cg_ATTRIB0"), "cg_ATTRIB0"));
    assert(!strcmp(GlslAllocateSymbolName(&visibleModule,
                   &localIdentity, "cg_ATTRIB0"), "cg_ATTRIB0_1"));
    assert(!strcmp(GlslAllocateSymbolName(&visibleModule,
                   &globalIdentity, "renamed"), "cg_ATTRIB0"));

    GlslInitModule(&scopedModule, GLSL_STAGE_VERTEX, TestAlloc, NULL);
    assert(!strcmp(GlslAllocateScopedSymbolName(&scopedModule,
                   &localNamespace, &firstIdentity, "position"),
                   "position"));
    assert(!strcmp(GlslAllocateScopedSymbolName(&scopedModule,
                   &memberNamespace, &secondIdentity, "position"),
                   "position"));
    assert(!strcmp(GlslAllocateScopedSymbolName(&scopedModule,
                   &localNamespace, &secondIdentity, "output"),
                   "cg_output"));
    assert(!strcmp(GlslAllocateScopedSymbolName(&scopedModule,
                   &localNamespace, &secondIdentity, "other"),
                   "cg_output"));
    assert(!strcmp(GlslAllocateScopedSymbolName(&scopedModule,
                   &localNamespace, &localNamespace, "output"),
                   "cg_output_1"));
    assert(!strcmp(GlslAllocateScopedSymbolName(&scopedModule,
                   &memberNamespace, NULL, "member"), "member"));
    assert(!strcmp(GlslAllocateScopedSymbolName(&scopedModule,
                   &memberNamespace, NULL, "otherMember"),
                   "otherMember"));
    assert(!strcmp(GlslAllocateSymbolName(&module, &secondIdentity, "value"),
        "value_1"));
    assert(GlslIsReservedName("attribute"));
    assert(GlslIsReservedName("gl_Position"));
    assert(GlslIsReservedName("user__name"));
    assert(!GlslIsReservedName("user_name"));
    emitted = GlslAllocateName(&module, "user__name");
    assert(!strcmp(emitted, "cg_user_name"));
    assert(!GlslIsReservedName(emitted));
    emitted = GlslAllocateName(&module, "__foo");
    assert(!strcmp(emitted, "cg_foo"));
    assert(!GlslIsReservedName(emitted));
    emitted = GlslAllocateDistinctName(&module, "foo_");
    assert(!strcmp(emitted, "foo_"));
    assert(!GlslIsReservedName(emitted));
    emitted = GlslAllocateDistinctName(&module, "foo_");
    assert(!strcmp(emitted, "foo_1"));
    assert(!GlslIsReservedName(emitted));
    emitted = GlslAllocateDistinctName(&module, "gl");
    assert(!strcmp(emitted, "gl"));
    emitted = GlslAllocateDistinctName(&module, "gl");
    assert(!strcmp(emitted, "cg_gl_1"));
    assert(!GlslIsReservedName(emitted));

    GlslInitModule(&collisionModule, GLSL_STAGE_VERTEX, TestAlloc, NULL);
    assert(!strcmp(GlslAllocateDistinctName(&collisionModule, "gl"), "gl"));
    assert(!strcmp(GlslAllocateName(&collisionModule, "cg_gl_1"),
        "cg_gl_1"));
    emitted = GlslAllocateDistinctName(&collisionModule, "gl");
    assert(!strcmp(emitted, "cg_gl_2"));
    assert(!GlslIsReservedName(emitted));

    GlslInitModule(&gapModule, GLSL_STAGE_VERTEX, TestAlloc, NULL);
    assert(!strcmp(GlslAllocateName(&gapModule, "item_2"), "item_2"));
    assert(!strcmp(GlslAllocateDistinctName(&gapModule, "item"), "item"));
    assert(!strcmp(GlslAllocateDistinctName(&gapModule, "item"), "item_1"));

    GlslInitModule(&overflowModule, GLSL_STAGE_VERTEX, TestAlloc, NULL);
    assert(!strcmp(GlslAllocateName(&overflowModule, "item_2147483648"),
        "item_2147483648"));
    emitted = GlslAllocateDistinctName(&overflowModule, "item");
    assert(emitted != NULL);
    assert(!strcmp(emitted, "item"));
    emitted = GlslAllocateDistinctName(&overflowModule, "item");
    assert(emitted != NULL);
    assert(!strcmp(emitted, "item_1"));

    allocationCount = 0;
    GlslInitModule(&countModule, GLSL_STAGE_VERTEX, CountingAlloc, NULL);
    for (index = 0; index < 64; index++) {
        emitted = GlslAllocateDistinctName(&countModule, "item");
        if (index == 0)
            assert(!strcmp(emitted, "item"));
        if (index == 63)
            assert(!strcmp(emitted, "item_63"));
        assert(!GlslIsReservedName(emitted));
    }
    assert(allocationCount <= 5 * 64);

    type = GlslNumericType(GLSL_BASE_BOOL, 2);
    assert(!strcmp(GlslTypeName(&type), "bvec2"));
    type = GlslNumericType(GLSL_BASE_SAMPLER2D, 1);
    assert(!strcmp(GlslTypeName(&type), "sampler2D"));
    type.base = GLSL_BASE_STRUCT;
    type.len = 0;
    type.rows = 0;
    type.cols = 0;
    type.arraySize = 0;
    type.structName = "Light";
    type.elementType = NULL;
    assert(!strcmp(GlslTypeName(&type), "Light"));
    type = GlslNumericType(GLSL_BASE_FLOAT, 5);
    assert(GlslTypeName(&type) == NULL);
    type = GlslMatrixType(5);
    assert(GlslTypeName(&type) == NULL);
    type = GlslMatrixType(3);
    type.cols = 2;
    assert(GlslTypeName(&type) == NULL);
    type = GlslNumericType(GLSL_BASE_VOID, 0);
    type.arraySize = 1;
    assert(GlslTypeName(&type) == NULL);
    type = GlslNumericType(GLSL_BASE_VOID, 0);
    type.structName = "Bad";
    assert(GlslTypeName(&type) == NULL);
    type = GlslNumericType(GLSL_BASE_VOID, 0);
    type.elementType = &type;
    assert(GlslTypeName(&type) == NULL);
    type = GlslNumericType(GLSL_BASE_FLOAT, 1);
    type.structName = "Bad";
    assert(GlslTypeName(&type) == NULL);
    type = GlslNumericType(GLSL_BASE_FLOAT, 1);
    type.elementType = &type;
    assert(GlslTypeName(&type) == NULL);
    type = GlslNumericType(GLSL_BASE_STRUCT, 0);
    assert(GlslTypeName(&type) == NULL);
    type.structName = "";
    assert(GlslTypeName(&type) == NULL);
    type = GlslNumericType((GlslBase) 99, 1);
    assert(GlslTypeName(&type) == NULL);

    GlslInitModule(&dirtyModule, GLSL_STAGE_FRAGMENT, DirtyAlloc, NULL);
    type = GlslNumericType(GLSL_BASE_FLOAT, 4);
    decl = GlslNewDecl(&dirtyModule, GLSL_STORAGE_ATTRIBUTE, type, "decl");
    assert(decl->next == NULL);
    assert(decl->storage == GLSL_STORAGE_ATTRIBUTE);
    assert(decl->type.base == GLSL_BASE_FLOAT);
    assert(decl->type.len == 4);
    assert(!strcmp(decl->name, "decl"));
    assert(decl->loc.file == 0);
    assert(decl->loc.line == 0);
    assert(decl->initializer == NULL);
    assert(decl->identity == NULL);
    assert(decl->members == NULL);
    assert(decl->parameterQualifier == GLSL_PARAMETER_IN);
    expr = GlslNewExpr(&dirtyModule, GLSL_EXPR_FLOAT, type);
    assert(expr->next == NULL);
    assert(expr->kind == GLSL_EXPR_FLOAT);
    assert(expr->type.base == GLSL_BASE_FLOAT);
    assert(expr->type.len == 4);
    assert(expr->loc.file == 0);
    assert(expr->loc.line == 0);
    assert(expr->u.literalFloat == 0.0f);
    expr = GlslNewExpr(&dirtyModule, GLSL_EXPR_SYMBOL, type);
    assert(expr->u.symbol == NULL);
    expr = GlslNewExpr(&dirtyModule, GLSL_EXPR_INT, type);
    assert(expr->u.literalInt == 0);
    expr = GlslNewExpr(&dirtyModule, GLSL_EXPR_BOOL, type);
    assert(expr->u.literalBool == 0);
    expr = GlslNewExpr(&dirtyModule, GLSL_EXPR_UNARY, type);
    assert(expr->u.unary.op == 0);
    assert(expr->u.unary.operand == NULL);
    expr = GlslNewExpr(&dirtyModule, GLSL_EXPR_BINARY, type);
    assert(expr->u.binary.op == 0);
    assert(expr->u.binary.left == NULL);
    assert(expr->u.binary.right == NULL);
    expr = GlslNewExpr(&dirtyModule, GLSL_EXPR_CONDITIONAL, type);
    assert(expr->u.conditional.condition == NULL);
    assert(expr->u.conditional.trueExpr == NULL);
    assert(expr->u.conditional.falseExpr == NULL);
    expr = GlslNewExpr(&dirtyModule, GLSL_EXPR_MEMBER, type);
    assert(expr->u.member.object == NULL);
    assert(expr->u.member.decl == NULL);
    assert(expr->u.member.name == NULL);
    expr = GlslNewExpr(&dirtyModule, GLSL_EXPR_CALL, type);
    assert(expr->u.call.target == NULL);
    assert(expr->u.call.name == NULL);
    assert(expr->u.call.arguments == NULL);
    expr = GlslNewExpr(&dirtyModule, GLSL_EXPR_CONSTRUCT, type);
    assert(expr->u.construct.arguments == NULL);
    expr = GlslNewExpr(&dirtyModule, GLSL_EXPR_INDEX, type);
    assert(expr->u.index.object == NULL);
    assert(expr->u.index.index == NULL);
    expr = GlslNewExpr(&dirtyModule, GLSL_EXPR_SWIZZLE, type);
    assert(expr->u.swizzle.object == NULL);
    assert(expr->u.swizzle.mask == NULL);
    stmt = GlslNewStmt(&dirtyModule, GLSL_STMT_RETURN);
    assert(stmt->next == NULL);
    assert(stmt->kind == GLSL_STMT_RETURN);
    assert(stmt->loc.file == 0);
    assert(stmt->loc.line == 0);
    assert(stmt->u.returnExpr == NULL);
    stmt = GlslNewStmt(&dirtyModule, GLSL_STMT_EXPRESSION);
    assert(stmt->u.expression == NULL);
    stmt = GlslNewStmt(&dirtyModule, GLSL_STMT_IF);
    assert(stmt->u.ifStmt.condition == NULL);
    assert(stmt->u.ifStmt.trueBranch == NULL);
    assert(stmt->u.ifStmt.falseBranch == NULL);
    stmt = GlslNewStmt(&dirtyModule, GLSL_STMT_WHILE);
    assert(stmt->u.loop.condition == NULL);
    assert(stmt->u.loop.body == NULL);
    stmt = GlslNewStmt(&dirtyModule, GLSL_STMT_DO);
    assert(stmt->u.loop.condition == NULL);
    assert(stmt->u.loop.body == NULL);
    stmt = GlslNewStmt(&dirtyModule, GLSL_STMT_FOR);
    assert(stmt->u.forStmt.init == NULL);
    assert(stmt->u.forStmt.condition == NULL);
    assert(stmt->u.forStmt.step == NULL);
    assert(stmt->u.forStmt.body == NULL);
    stmt = GlslNewStmt(&dirtyModule, GLSL_STMT_BLOCK);
    assert(stmt->u.block == NULL);
    stmt = GlslNewStmt(&dirtyModule, GLSL_STMT_RETURN);
    function = GlslNewFunction(&dirtyModule, type, "main");
    assert(function->next == NULL);
    assert(function->result.base == GLSL_BASE_FLOAT);
    assert(function->result.len == 4);
    assert(!strcmp(function->name, "main"));
    assert(function->loc.file == 0);
    assert(function->loc.line == 0);
    assert(function->identity == NULL);
    assert(function->parameters == NULL);
    assert(function->locals == NULL);
    assert(function->body == NULL);
    assert(function->isEntry == 0);
    binding = GlslNewBinding(&dirtyModule, GLSL_STORAGE_BUILTIN,
        "gl_Position", "POSITION");
    assert(binding->next == NULL);
    assert(binding->storage == GLSL_STORAGE_BUILTIN);
    assert(!strcmp(binding->name, "gl_Position"));
    assert(!strcmp(binding->semantic, "POSITION"));
    assert(binding->interfaceKey == NULL);
    assert(binding->loc.file == 0);
    assert(binding->loc.line == 0);
    assert(binding->declaration == NULL);
    assert(binding->isOutput == 0);

    secondDecl = GlslNewDecl(&dirtyModule, GLSL_STORAGE_UNIFORM, type,
        "second");
    decls = NULL;
    GlslAppendDecl(&decls, decl);
    GlslAppendDecl(&decls, secondDecl);
    assert(decls == decl);
    assert(decls->next == secondDecl);
    assert(secondDecl->next == NULL);
    secondStmt = GlslNewStmt(&dirtyModule, GLSL_STMT_BREAK);
    stmts = NULL;
    GlslAppendStmt(&stmts, stmt);
    GlslAppendStmt(&stmts, secondStmt);
    assert(stmts == stmt);
    assert(stmts->next == secondStmt);
    assert(secondStmt->next == NULL);
    secondFunction = GlslNewFunction(&dirtyModule, type, "helper");
    functions = NULL;
    GlslAppendFunction(&functions, function);
    GlslAppendFunction(&functions, secondFunction);
    assert(functions == function);
    assert(functions->next == secondFunction);
    assert(secondFunction->next == NULL);

    infinity = FLT_MAX;
    infinity = infinity * 2.0f;
    nanValue = infinity - infinity;
    assert(infinity > FLT_MAX);
    assert(nanValue != nanValue);

    GlslInitModule(&nonfiniteModule, GLSL_STAGE_VERTEX, TestAlloc, NULL);
    type = GlslNumericType(GLSL_BASE_VOID, 0);
    function = GlslNewFunction(&nonfiniteModule, type, "main");
    assert(function != NULL);
    function->isEntry = 1;
    nonfiniteModule.entry = function;
    GlslAppendFunction(&nonfiniteModule.functions, function);
    stmt = GlslNewStmt(&nonfiniteModule, GLSL_STMT_EXPRESSION);
    type = GlslNumericType(GLSL_BASE_FLOAT, 1);
    expr = GlslNewExpr(&nonfiniteModule, GLSL_EXPR_FLOAT, type);
    expr->u.literalFloat = infinity;
    stmt->u.expression = expr;
    GlslAppendStmt(&function->body, stmt);
    writer = tmpfile();
    assert(writer != NULL);
    assert(!GlslWriteModule(writer, &nonfiniteModule));
    assert(ftell(writer) == 0);
    assert(!fclose(writer));

    GlslInitModule(&nonfiniteModule, GLSL_STAGE_VERTEX, TestAlloc, NULL);
    type = GlslNumericType(GLSL_BASE_VOID, 0);
    function = GlslNewFunction(&nonfiniteModule, type, "main");
    assert(function != NULL);
    function->isEntry = 1;
    nonfiniteModule.entry = function;
    GlslAppendFunction(&nonfiniteModule.functions, function);
    stmt = GlslNewStmt(&nonfiniteModule, GLSL_STMT_EXPRESSION);
    type = GlslNumericType(GLSL_BASE_FLOAT, 1);
    expr = GlslNewExpr(&nonfiniteModule, GLSL_EXPR_FLOAT, type);
    expr->u.literalFloat = nanValue;
    stmt->u.expression = expr;
    GlslAppendStmt(&function->body, stmt);
    writer = tmpfile();
    assert(writer != NULL);
    assert(!GlslWriteModule(writer, &nonfiniteModule));
    assert(ftell(writer) == 0);
    assert(!fclose(writer));

    GlslInitModule(&module, GLSL_STAGE_VERTEX, TestAlloc, NULL);
    writer = tmpfile();
    assert(writer != NULL);
    assert(!GlslWriteModule(writer, &module));
    assert(ftell(writer) == 0);
    assert(!fclose(writer));

    GlslInitModule(&module, GLSL_STAGE_VERTEX, TestAlloc, NULL);
    type = GlslNumericType(GLSL_BASE_VOID, 0);
    function = GlslNewFunction(&module, type, "main");
    assert(function != NULL);
    function->isEntry = 1;
    module.entry = function;
    GlslAppendFunction(&module.functions, function);
    stmt = GlslNewStmt(&module, GLSL_STMT_DISCARD);
    assert(stmt != NULL);
    GlslAppendStmt(&function->body, stmt);
    writer = tmpfile();
    assert(writer != NULL);
    assert(!GlslWriteModule(writer, &module));
    assert(ftell(writer) == 0);
    assert(!fclose(writer));

    GlslInitModule(&module, GLSL_STAGE_FRAGMENT, TestAlloc, NULL);
    type = GlslNumericType(GLSL_BASE_VOID, 0);
    function = GlslNewFunction(&module, type, "main");
    assert(function != NULL);
    function->isEntry = 1;
    module.entry = function;
    GlslAppendFunction(&module.functions, function);
    stmt = GlslNewStmt(&module, GLSL_STMT_DISCARD);
    assert(stmt != NULL);
    GlslAppendStmt(&function->body, stmt);
    writer = tmpfile();
    assert(writer != NULL);
    assert(GlslWriteModule(writer, &module));
    assert(ftell(writer) > 0);
    assert(!fclose(writer));
    return 0;
}

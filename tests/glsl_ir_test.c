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

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include "glsl_ir.h"

static void *TestAlloc(void *arg, size_t size)
{
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
    int firstIdentity;
    int secondIdentity;

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

    assert(!strcmp(GlslAllocateSymbolName(&module, &firstIdentity, "value"),
        "value"));
    assert(!strcmp(GlslAllocateSymbolName(&module, &firstIdentity, "other"),
        "value"));
    assert(!strcmp(GlslAllocateSymbolName(&module, &secondIdentity, "value"),
        "value_1"));
    assert(!strcmp(GlslAllocateSymbolName(&module, &secondIdentity, "value"),
        "value_1"));
    assert(GlslIsReservedName("attribute"));
    assert(GlslIsReservedName("gl_Position"));
    assert(GlslIsReservedName("user__name"));
    assert(!GlslIsReservedName("user_name"));
    assert(!strcmp(GlslAllocateName(&module, "user__name"), "user_name"));

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
    expr = GlslNewExpr(&dirtyModule, GLSL_EXPR_FLOAT, type);
    assert(expr->kind == GLSL_EXPR_FLOAT);
    assert(expr->type.base == GLSL_BASE_FLOAT);
    assert(expr->type.len == 4);
    assert(expr->loc.file == 0);
    assert(expr->loc.line == 0);
    stmt = GlslNewStmt(&dirtyModule, GLSL_STMT_RETURN);
    assert(stmt->next == NULL);
    assert(stmt->kind == GLSL_STMT_RETURN);
    assert(stmt->loc.file == 0);
    assert(stmt->loc.line == 0);
    function = GlslNewFunction(&dirtyModule, type, "main");
    assert(function->next == NULL);
    assert(function->result.base == GLSL_BASE_FLOAT);
    assert(function->result.len == 4);
    assert(!strcmp(function->name, "main"));
    assert(function->loc.file == 0);
    assert(function->loc.line == 0);
    binding = GlslNewBinding(&dirtyModule, GLSL_STORAGE_BUILTIN,
        "gl_Position", "POSITION");
    assert(binding->next == NULL);
    assert(binding->storage == GLSL_STORAGE_BUILTIN);
    assert(!strcmp(binding->name, "gl_Position"));
    assert(!strcmp(binding->semantic, "POSITION"));
    assert(binding->loc.file == 0);
    assert(binding->loc.line == 0);

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
    return 0;
}

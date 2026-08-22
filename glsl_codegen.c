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
license, under NVIDIA's copyrights in this original NVIDIA software
(the "NVIDIA Software"), to use, reproduce, modify and redistribute the
NVIDIA Software, with or without modifications, in source and/or binary
forms; provided that if you redistribute the NVIDIA Software, you must
retain the copyright notice of NVIDIA, this notice and the following
text and disclaimers in all such redistributions of the NVIDIA Software.
Neither the name, trademarks, service marks nor logos of NVIDIA
Corporation may be used to endorse or promote products derived from
this NVIDIA Software without specific prior written permission from
NVIDIA. Except as expressly stated in this notice, no other rights or
licenses express or implied, are granted by NVIDIA herein, including
but not limited to any patent rights that may be infringed by your
derivative works or by other works in which the NVIDIA Software may be
incorporated. No hardware is licensed hereunder.

THE NVIDIA SOFTWARE IS BEING PROVIDED ON AN "AS IS" BASIS, WITHOUT
WARRANTIES OR CONDITIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED,
INCLUDING WITHOUT LIMITATION, WARRANTIES OR CONDITIONS OF TITLE,
NON-INFRINGEMENT, MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE,
OR ITS USE AND OPERATION EITHER ALONE OR IN COMBINATION WITH OTHER
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
// glsl_codegen.c
//

#include <stdio.h>

#include "glsl_ir.h"

static const char *GlslStorageName(GlslStorage storage)
{
    switch (storage) {
    case GLSL_STORAGE_ATTRIBUTE:
        return "attribute";
    case GLSL_STORAGE_VARYING:
        return "varying";
    case GLSL_STORAGE_UNIFORM:
        return "uniform";
    case GLSL_STORAGE_CONST:
        return "const";
    case GLSL_STORAGE_BUILTIN:
        return "builtin";
    default:
        return NULL;
    }
}

static int GlslValidateExpr(const GlslExpr *expr)
{
    if (expr == NULL || GlslTypeName(&expr->type) == NULL)
        return 0;
    switch (expr->kind) {
    case GLSL_EXPR_SYMBOL:
        return expr->u.symbol != NULL && expr->u.symbol->name != NULL;
    case GLSL_EXPR_MEMBER:
        return expr->u.member.name != NULL &&
               GlslValidateExpr(expr->u.member.object);
    case GLSL_EXPR_BINARY:
        return expr->u.binary.op == '=' &&
               GlslValidateExpr(expr->u.binary.left) &&
               GlslValidateExpr(expr->u.binary.right);
    default:
        return 0;
    }
}

static int GlslValidateDecls(const GlslDecl *decl)
{
    for (; decl != NULL; decl = decl->next) {
        if (decl->name == NULL || GlslTypeName(&decl->type) == NULL ||
            decl->initializer != NULL)
        {
            return 0;
        }
    }
    return 1;
}

static int GlslValidateModule(const GlslModule *module)
{
    const GlslBinding *binding;
    const GlslDecl *decl;
    const GlslFunction *function;
    const GlslStmt *stmt;

    if (module == NULL || module->errors != 0 || module->entry == NULL)
        return 0;
    for (binding = module->bindings; binding != NULL;
         binding = binding->next)
    {
        if (GlslStorageName(binding->storage) == NULL ||
            binding->name == NULL || binding->semantic == NULL)
        {
            return 0;
        }
    }
    for (decl = module->structs; decl != NULL; decl = decl->next) {
        if (decl->name == NULL || !GlslValidateDecls(decl->members))
            return 0;
    }
    if (!GlslValidateDecls(module->globals))
        return 0;
    for (function = module->functions; function != NULL;
         function = function->next)
    {
        if (function->name == NULL ||
            GlslTypeName(&function->result) == NULL ||
            function->parameters != NULL ||
            !GlslValidateDecls(function->locals))
        {
            return 0;
        }
        for (stmt = function->body; stmt != NULL; stmt = stmt->next) {
            if (stmt->kind != GLSL_STMT_EXPRESSION ||
                !GlslValidateExpr(stmt->u.expression))
            {
                return 0;
            }
        }
    }
    return 1;
}

static int GlslWriteExpr(FILE *out, const GlslExpr *expr)
{
    switch (expr->kind) {
    case GLSL_EXPR_SYMBOL:
        return fprintf(out, "%s", expr->u.symbol->name) >= 0;
    case GLSL_EXPR_MEMBER:
        if (!GlslWriteExpr(out, expr->u.member.object))
            return 0;
        return fprintf(out, ".%s", expr->u.member.name) >= 0;
    case GLSL_EXPR_BINARY:
        if (!GlslWriteExpr(out, expr->u.binary.left) ||
            fprintf(out, " = ") < 0 ||
            !GlslWriteExpr(out, expr->u.binary.right))
        {
            return 0;
        }
        return 1;
    default:
        return 0;
    }
}

static int GlslWriteStruct(FILE *out, const GlslDecl *decl)
{
    const GlslDecl *member;

    if (fprintf(out, "struct %s\n{\n", decl->name) < 0)
        return 0;
    for (member = decl->members; member != NULL; member = member->next) {
        if (fprintf(out, "    %s %s;\n", GlslTypeName(&member->type),
                    member->name) < 0)
        {
            return 0;
        }
    }
    return fprintf(out, "};\n") >= 0;
}

static int GlslWriteGlobal(FILE *out, const GlslDecl *decl)
{
    return fprintf(out, "%s %s %s;\n", GlslStorageName(decl->storage),
                   GlslTypeName(&decl->type), decl->name) >= 0;
}

static int GlslWriteFunction(FILE *out, const GlslFunction *function)
{
    const GlslDecl *decl;
    const GlslStmt *stmt;

    if (fprintf(out, "%s %s()\n{\n", GlslTypeName(&function->result),
                function->name) < 0)
    {
        return 0;
    }
    for (decl = function->locals; decl != NULL; decl = decl->next) {
        if (fprintf(out, "    %s %s;\n", GlslTypeName(&decl->type),
                    decl->name) < 0)
        {
            return 0;
        }
    }
    for (stmt = function->body; stmt != NULL; stmt = stmt->next) {
        if (fprintf(out, "    ") < 0 ||
            !GlslWriteExpr(out, stmt->u.expression) ||
            fprintf(out, ";\n") < 0)
        {
            return 0;
        }
    }
    return fprintf(out, "}\n") >= 0;
}

int GlslWriteModule(FILE *out, const GlslModule *module)
{
    const GlslBinding *binding;
    const GlslDecl *decl;
    const GlslFunction *function;

    if (out == NULL || !GlslValidateModule(module))
        return 0;
    for (binding = module->bindings; binding != NULL;
         binding = binding->next)
    {
        if (fprintf(out, "// cgc-bind %s %s %s\n",
                    GlslStorageName(binding->storage), binding->name,
                    binding->semantic) < 0)
        {
            return 0;
        }
    }
    if (module->bindings != NULL && fprintf(out, "\n") < 0)
        return 0;
    for (decl = module->structs; decl != NULL; decl = decl->next) {
        if (!GlslWriteStruct(out, decl) || fprintf(out, "\n") < 0)
            return 0;
    }
    for (decl = module->globals; decl != NULL; decl = decl->next) {
        if (!GlslWriteGlobal(out, decl))
            return 0;
    }
    if (module->globals != NULL && fprintf(out, "\n") < 0)
        return 0;
    for (function = module->functions; function != NULL;
         function = function->next)
    {
        if (!GlslWriteFunction(out, function))
            return 0;
        if (function->next != NULL && fprintf(out, "\n") < 0)
            return 0;
    }
    return 1;
}

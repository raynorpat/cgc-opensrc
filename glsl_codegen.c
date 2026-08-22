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
// glsl_codegen.c
//

#include <stdio.h>
#include <string.h>

#include "glsl_ir.h"

static const char *GlslStorageName(GlslStorage storage)
{
    switch (storage) {
    case GLSL_STORAGE_ATTRIBUTE: return "attribute";
    case GLSL_STORAGE_VARYING: return "varying";
    case GLSL_STORAGE_UNIFORM: return "uniform";
    case GLSL_STORAGE_CONST: return "const";
    case GLSL_STORAGE_BUILTIN: return "builtin";
    default: return NULL;
    }
}

static int GlslValidOperator(GlslOperator op)
{
    return op >= GLSL_OP_ASSIGN && op <= GLSL_OP_LOGICAL_NOT;
}

static int GlslValidateExpr(const GlslExpr *expr);

static int GlslValidateExprList(const GlslExpr *expr)
{
    for (; expr != NULL; expr = expr->next) {
        if (!GlslValidateExpr(expr))
            return 0;
    }
    return 1;
}

static int GlslValidateExpr(const GlslExpr *expr)
{
    if (expr == NULL || GlslTypeName(&expr->type) == NULL)
        return 0;
    switch (expr->kind) {
    case GLSL_EXPR_SYMBOL:
        return expr->u.symbol != NULL && expr->u.symbol->name != NULL;
    case GLSL_EXPR_INT:
    case GLSL_EXPR_FLOAT:
    case GLSL_EXPR_BOOL:
        return 1;
    case GLSL_EXPR_UNARY:
        return GlslValidOperator(expr->u.unary.op) &&
               expr->u.unary.op >= GLSL_OP_NEGATE &&
               GlslValidateExpr(expr->u.unary.operand);
    case GLSL_EXPR_BINARY:
        return GlslValidOperator(expr->u.binary.op) &&
               expr->u.binary.op <= GLSL_OP_DIVIDE &&
               GlslValidateExpr(expr->u.binary.left) &&
               GlslValidateExpr(expr->u.binary.right);
    case GLSL_EXPR_CONDITIONAL:
        return GlslValidateExpr(expr->u.conditional.condition) &&
               GlslValidateExpr(expr->u.conditional.trueExpr) &&
               GlslValidateExpr(expr->u.conditional.falseExpr);
    case GLSL_EXPR_CALL:
        return expr->u.call.name != NULL &&
               GlslValidateExprList(expr->u.call.arguments);
    case GLSL_EXPR_CONSTRUCT:
        return expr->u.construct.arguments != NULL &&
               GlslValidateExprList(expr->u.construct.arguments);
    case GLSL_EXPR_MEMBER:
        return expr->u.member.name != NULL &&
               GlslValidateExpr(expr->u.member.object);
    case GLSL_EXPR_INDEX:
        return GlslValidateExpr(expr->u.index.object) &&
               GlslValidateExpr(expr->u.index.index);
    case GLSL_EXPR_SWIZZLE:
        return expr->u.swizzle.mask != NULL &&
               expr->u.swizzle.mask[0] != '\0' &&
               GlslValidateExpr(expr->u.swizzle.object);
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

static int GlslValidateStmtList(const GlslStmt *stmt);

static int GlslValidateForPart(const GlslStmt *stmt)
{
    for (; stmt != NULL; stmt = stmt->next) {
        if (stmt->kind != GLSL_STMT_EXPRESSION ||
            !GlslValidateExpr(stmt->u.expression))
        {
            return 0;
        }
    }
    return 1;
}

static int GlslValidateStmtList(const GlslStmt *stmt)
{
    for (; stmt != NULL; stmt = stmt->next) {
        switch (stmt->kind) {
        case GLSL_STMT_EXPRESSION:
            if (!GlslValidateExpr(stmt->u.expression)) return 0;
            break;
        case GLSL_STMT_IF:
            if (!GlslValidateExpr(stmt->u.ifStmt.condition) ||
                !GlslValidateStmtList(stmt->u.ifStmt.trueBranch) ||
                !GlslValidateStmtList(stmt->u.ifStmt.falseBranch)) return 0;
            break;
        case GLSL_STMT_WHILE:
        case GLSL_STMT_DO:
            if (!GlslValidateExpr(stmt->u.loop.condition) ||
                !GlslValidateStmtList(stmt->u.loop.body)) return 0;
            break;
        case GLSL_STMT_FOR:
            if (!GlslValidateForPart(stmt->u.forStmt.init) ||
                (stmt->u.forStmt.condition != NULL &&
                 !GlslValidateExpr(stmt->u.forStmt.condition)) ||
                !GlslValidateForPart(stmt->u.forStmt.step) ||
                !GlslValidateStmtList(stmt->u.forStmt.body)) return 0;
            break;
        case GLSL_STMT_BLOCK:
            if (!GlslValidateStmtList(stmt->u.block)) return 0;
            break;
        case GLSL_STMT_RETURN:
            if (stmt->u.returnExpr != NULL &&
                !GlslValidateExpr(stmt->u.returnExpr)) return 0;
            break;
        case GLSL_STMT_DISCARD:
        case GLSL_STMT_BREAK:
        case GLSL_STMT_CONTINUE:
            break;
        default:
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

    if (module == NULL || module->errors != 0 || module->entry == NULL)
        return 0;
    for (binding = module->bindings; binding != NULL;
         binding = binding->next)
    {
        if (GlslStorageName(binding->storage) == NULL ||
            binding->name == NULL || binding->semantic == NULL) return 0;
    }
    for (decl = module->structs; decl != NULL; decl = decl->next) {
        if (decl->name == NULL || !GlslValidateDecls(decl->members)) return 0;
    }
    if (!GlslValidateDecls(module->globals)) return 0;
    for (function = module->functions; function != NULL;
         function = function->next)
    {
        if (function->name == NULL ||
            GlslTypeName(&function->result) == NULL ||
            !GlslValidateDecls(function->parameters) ||
            !GlslValidateDecls(function->locals) ||
            !GlslValidateStmtList(function->body)) return 0;
        if (function->isEntry &&
            (function != module->entry || function->parameters != NULL ||
             strcmp(function->name, "main") ||
             function->result.base != GLSL_BASE_VOID)) return 0;
    }
    return 1;
}

static int GlslOperatorPrecedence(GlslOperator op)
{
    switch (op) {
    case GLSL_OP_ASSIGN: return 1;
    case GLSL_OP_LOGICAL_OR: return 3;
    case GLSL_OP_LOGICAL_AND: return 4;
    case GLSL_OP_EQUAL:
    case GLSL_OP_NOT_EQUAL: return 5;
    case GLSL_OP_LESS:
    case GLSL_OP_GREATER:
    case GLSL_OP_LESS_EQUAL:
    case GLSL_OP_GREATER_EQUAL: return 6;
    case GLSL_OP_ADD:
    case GLSL_OP_SUBTRACT: return 7;
    case GLSL_OP_MULTIPLY:
    case GLSL_OP_DIVIDE: return 8;
    case GLSL_OP_NEGATE:
    case GLSL_OP_POSITIVE:
    case GLSL_OP_LOGICAL_NOT: return 9;
    default: return 0;
    }
}

static const char *GlslOperatorText(GlslOperator op)
{
    switch (op) {
    case GLSL_OP_ASSIGN: return "=";
    case GLSL_OP_LOGICAL_OR: return "||";
    case GLSL_OP_LOGICAL_AND: return "&&";
    case GLSL_OP_EQUAL: return "==";
    case GLSL_OP_NOT_EQUAL: return "!=";
    case GLSL_OP_LESS: return "<";
    case GLSL_OP_GREATER: return ">";
    case GLSL_OP_LESS_EQUAL: return "<=";
    case GLSL_OP_GREATER_EQUAL: return ">=";
    case GLSL_OP_ADD: return "+";
    case GLSL_OP_SUBTRACT: return "-";
    case GLSL_OP_MULTIPLY: return "*";
    case GLSL_OP_DIVIDE: return "/";
    case GLSL_OP_NEGATE: return "-";
    case GLSL_OP_POSITIVE: return "+";
    case GLSL_OP_LOGICAL_NOT: return "!";
    default: return NULL;
    }
}

static int GlslExprPrecedence(const GlslExpr *expr)
{
    switch (expr->kind) {
    case GLSL_EXPR_BINARY: return GlslOperatorPrecedence(expr->u.binary.op);
    case GLSL_EXPR_CONDITIONAL: return 2;
    case GLSL_EXPR_UNARY: return 9;
    case GLSL_EXPR_CALL:
    case GLSL_EXPR_CONSTRUCT:
    case GLSL_EXPR_MEMBER:
    case GLSL_EXPR_INDEX:
    case GLSL_EXPR_SWIZZLE: return 10;
    default: return 11;
    }
}

static int GlslWriteExprPrec(FILE *out, const GlslExpr *expr,
    int parentPrecedence, int rightChild, GlslOperator parentOperator);

static int GlslWriteExprList(FILE *out, const GlslExpr *expr)
{
    int first = 1;
    for (; expr != NULL; expr = expr->next) {
        if ((!first && fprintf(out, ", ") < 0) ||
            !GlslWriteExprPrec(out, expr, 0, 0, GLSL_OP_NONE)) return 0;
        first = 0;
    }
    return 1;
}

static int GlslWriteFloat(FILE *out, float value)
{
    char text[64];
    size_t length;

    if (snprintf(text, sizeof(text), "%.9g", value) < 0) return 0;
    if (strchr(text, '.') == NULL && strchr(text, 'e') == NULL &&
        strchr(text, 'E') == NULL)
    {
        length = strlen(text);
        if (length + 2 >= sizeof(text)) return 0;
        text[length] = '.';
        text[length + 1] = '0';
        text[length + 2] = '\0';
    }
    return fprintf(out, "%s", text) >= 0;
}

static int GlslWriteExprPrec(FILE *out, const GlslExpr *expr,
    int parentPrecedence, int rightChild, GlslOperator parentOperator)
{
    const char *operatorText;
    int precedence = GlslExprPrecedence(expr);
    int needParens = precedence < parentPrecedence;

    if (rightChild && precedence == parentPrecedence &&
        (parentOperator == GLSL_OP_SUBTRACT ||
         parentOperator == GLSL_OP_DIVIDE)) needParens = 1;
    if (needParens && fprintf(out, "(") < 0) return 0;
    switch (expr->kind) {
    case GLSL_EXPR_SYMBOL:
        if (fprintf(out, "%s", expr->u.symbol->name) < 0) return 0;
        break;
    case GLSL_EXPR_INT:
        if (fprintf(out, "%d", expr->u.literalInt) < 0) return 0;
        break;
    case GLSL_EXPR_FLOAT:
        if (!GlslWriteFloat(out, expr->u.literalFloat)) return 0;
        break;
    case GLSL_EXPR_BOOL:
        if (fprintf(out, "%s", expr->u.literalBool ? "true" : "false") < 0) return 0;
        break;
    case GLSL_EXPR_UNARY:
        operatorText = GlslOperatorText(expr->u.unary.op);
        if (fprintf(out, "%s", operatorText) < 0 ||
            !GlslWriteExprPrec(out, expr->u.unary.operand, precedence, 1,
                               expr->u.unary.op)) return 0;
        break;
    case GLSL_EXPR_BINARY:
        operatorText = GlslOperatorText(expr->u.binary.op);
        if (!GlslWriteExprPrec(out, expr->u.binary.left, precedence, 0,
                               expr->u.binary.op) ||
            fprintf(out, " %s ", operatorText) < 0 ||
            !GlslWriteExprPrec(out, expr->u.binary.right, precedence, 1,
                               expr->u.binary.op)) return 0;
        break;
    case GLSL_EXPR_CONDITIONAL:
        if (!GlslWriteExprPrec(out, expr->u.conditional.condition,
                               precedence, 0, GLSL_OP_NONE) ||
            fprintf(out, " ? ") < 0 ||
            !GlslWriteExprPrec(out, expr->u.conditional.trueExpr,
                               precedence, 0, GLSL_OP_NONE) ||
            fprintf(out, " : ") < 0 ||
            !GlslWriteExprPrec(out, expr->u.conditional.falseExpr,
                               precedence, 1, GLSL_OP_NONE)) return 0;
        break;
    case GLSL_EXPR_CALL:
        if (fprintf(out, "%s(", expr->u.call.name) < 0 ||
            !GlslWriteExprList(out, expr->u.call.arguments) ||
            fprintf(out, ")") < 0) return 0;
        break;
    case GLSL_EXPR_CONSTRUCT:
        if (fprintf(out, "%s(", GlslTypeName(&expr->type)) < 0 ||
            !GlslWriteExprList(out, expr->u.construct.arguments) ||
            fprintf(out, ")") < 0) return 0;
        break;
    case GLSL_EXPR_MEMBER:
        if (!GlslWriteExprPrec(out, expr->u.member.object, precedence, 0,
                               GLSL_OP_NONE) ||
            fprintf(out, ".%s", expr->u.member.name) < 0) return 0;
        break;
    case GLSL_EXPR_INDEX:
        if (!GlslWriteExprPrec(out, expr->u.index.object, precedence, 0,
                               GLSL_OP_NONE) || fprintf(out, "[") < 0 ||
            !GlslWriteExprPrec(out, expr->u.index.index, 0, 0,
                               GLSL_OP_NONE) || fprintf(out, "]") < 0) return 0;
        break;
    case GLSL_EXPR_SWIZZLE:
        if (!GlslWriteExprPrec(out, expr->u.swizzle.object, precedence, 0,
                               GLSL_OP_NONE) ||
            fprintf(out, ".%s", expr->u.swizzle.mask) < 0) return 0;
        break;
    default:
        return 0;
    }
    if (needParens && fprintf(out, ")") < 0) return 0;
    return 1;
}

static int GlslWriteIndent(FILE *out, int level)
{
    int i;
    for (i = 0; i < level; i++) {
        if (fprintf(out, "    ") < 0) return 0;
    }
    return 1;
}

static int GlslWriteStruct(FILE *out, const GlslDecl *decl)
{
    const GlslDecl *member;
    if (fprintf(out, "struct %s\n{\n", decl->name) < 0) return 0;
    for (member = decl->members; member != NULL; member = member->next) {
        if (fprintf(out, "    %s %s;\n", GlslTypeName(&member->type),
                    member->name) < 0) return 0;
    }
    return fprintf(out, "};\n") >= 0;
}

static int GlslWriteGlobal(FILE *out, const GlslDecl *decl)
{
    return fprintf(out, "%s %s %s;\n", GlslStorageName(decl->storage),
                   GlslTypeName(&decl->type), decl->name) >= 0;
}

static const char *GlslParameterQualifierName(GlslParameterQualifier qualifier)
{
    switch (qualifier) {
    case GLSL_PARAMETER_OUT: return "out ";
    case GLSL_PARAMETER_INOUT: return "inout ";
    default: return "";
    }
}

static int GlslWriteFunctionSignature(FILE *out,
    const GlslFunction *function)
{
    const GlslDecl *parameter;
    int first = 1;

    if (fprintf(out, "%s %s(", GlslTypeName(&function->result),
                function->name) < 0) return 0;
    for (parameter = function->parameters; parameter != NULL;
         parameter = parameter->next)
    {
        if ((!first && fprintf(out, ", ") < 0) ||
            fprintf(out, "%s%s %s",
                    GlslParameterQualifierName(parameter->parameterQualifier),
                    GlslTypeName(&parameter->type), parameter->name) < 0) return 0;
        first = 0;
    }
    return fprintf(out, ")") >= 0;
}

static int GlslWriteStmtList(FILE *out, const GlslStmt *stmt, int level);

static int GlslWriteBracedBody(FILE *out, const GlslStmt *body, int level)
{
    return GlslWriteIndent(out, level) && fprintf(out, "{\n") >= 0 &&
           GlslWriteStmtList(out, body, level + 1) &&
           GlslWriteIndent(out, level) && fprintf(out, "}\n") >= 0;
}

static int GlslWriteForPart(FILE *out, const GlslStmt *stmt)
{
    int first = 1;
    for (; stmt != NULL; stmt = stmt->next) {
        if ((!first && fprintf(out, ", ") < 0) ||
            !GlslWriteExprPrec(out, stmt->u.expression, 0, 0,
                               GLSL_OP_NONE)) return 0;
        first = 0;
    }
    return 1;
}

static int GlslWriteStmt(FILE *out, const GlslStmt *stmt, int level)
{
    if (!GlslWriteIndent(out, level)) return 0;
    switch (stmt->kind) {
    case GLSL_STMT_EXPRESSION:
        return GlslWriteExprPrec(out, stmt->u.expression, 0, 0,
                                 GLSL_OP_NONE) && fprintf(out, ";\n") >= 0;
    case GLSL_STMT_IF:
        if (fprintf(out, "if (") < 0 ||
            !GlslWriteExprPrec(out, stmt->u.ifStmt.condition, 0, 0,
                               GLSL_OP_NONE) || fprintf(out, ")\n") < 0 ||
            !GlslWriteBracedBody(out, stmt->u.ifStmt.trueBranch, level)) return 0;
        if (stmt->u.ifStmt.falseBranch != NULL &&
            (!GlslWriteIndent(out, level) || fprintf(out, "else\n") < 0 ||
             !GlslWriteBracedBody(out, stmt->u.ifStmt.falseBranch, level))) return 0;
        return 1;
    case GLSL_STMT_WHILE:
        if (fprintf(out, "while (") < 0 ||
            !GlslWriteExprPrec(out, stmt->u.loop.condition, 0, 0,
                               GLSL_OP_NONE) || fprintf(out, ")\n") < 0) return 0;
        return GlslWriteBracedBody(out, stmt->u.loop.body, level);
    case GLSL_STMT_DO:
        if (fprintf(out, "do\n") < 0 ||
            !GlslWriteBracedBody(out, stmt->u.loop.body, level) ||
            !GlslWriteIndent(out, level) || fprintf(out, "while (") < 0 ||
            !GlslWriteExprPrec(out, stmt->u.loop.condition, 0, 0,
                               GLSL_OP_NONE) || fprintf(out, ");\n") < 0) return 0;
        return 1;
    case GLSL_STMT_FOR:
        if (fprintf(out, "for (") < 0 ||
            !GlslWriteForPart(out, stmt->u.forStmt.init) ||
            fprintf(out, "; ") < 0 ||
            (stmt->u.forStmt.condition != NULL &&
             !GlslWriteExprPrec(out, stmt->u.forStmt.condition, 0, 0,
                                GLSL_OP_NONE)) ||
            fprintf(out, "; ") < 0 ||
            !GlslWriteForPart(out, stmt->u.forStmt.step) ||
            fprintf(out, ")\n") < 0) return 0;
        return GlslWriteBracedBody(out, stmt->u.forStmt.body, level);
    case GLSL_STMT_BLOCK:
        return fprintf(out, "{\n") >= 0 &&
               GlslWriteStmtList(out, stmt->u.block, level + 1) &&
               GlslWriteIndent(out, level) && fprintf(out, "}\n") >= 0;
    case GLSL_STMT_RETURN:
        if (fprintf(out, "return") < 0) return 0;
        if (stmt->u.returnExpr != NULL &&
            (fprintf(out, " ") < 0 ||
             !GlslWriteExprPrec(out, stmt->u.returnExpr, 0, 0,
                                GLSL_OP_NONE))) return 0;
        return fprintf(out, ";\n") >= 0;
    case GLSL_STMT_DISCARD: return fprintf(out, "discard;\n") >= 0;
    case GLSL_STMT_BREAK: return fprintf(out, "break;\n") >= 0;
    case GLSL_STMT_CONTINUE: return fprintf(out, "continue;\n") >= 0;
    default: return 0;
    }
}

static int GlslWriteStmtList(FILE *out, const GlslStmt *stmt, int level)
{
    for (; stmt != NULL; stmt = stmt->next) {
        if (!GlslWriteStmt(out, stmt, level)) return 0;
    }
    return 1;
}

static int GlslWriteFunction(FILE *out, const GlslFunction *function)
{
    const GlslDecl *decl;
    if (!GlslWriteFunctionSignature(out, function) ||
        fprintf(out, "\n{\n") < 0) return 0;
    for (decl = function->locals; decl != NULL; decl = decl->next) {
        if (fprintf(out, "    %s %s;\n", GlslTypeName(&decl->type),
                    decl->name) < 0) return 0;
    }
    if (!GlslWriteStmtList(out, function->body, 1)) return 0;
    return fprintf(out, "}\n") >= 0;
}

int GlslWriteModule(FILE *out, const GlslModule *module)
{
    const GlslBinding *binding;
    const GlslDecl *decl;
    const GlslFunction *function;
    int wrotePrototype = 0;

    if (out == NULL || !GlslValidateModule(module)) return 0;
    for (binding = module->bindings; binding != NULL;
         binding = binding->next)
    {
        if (fprintf(out, "// cgc-bind %s %s %s\n",
                    GlslStorageName(binding->storage), binding->name,
                    binding->semantic) < 0) return 0;
    }
    if (module->bindings != NULL && fprintf(out, "\n") < 0) return 0;
    for (decl = module->structs; decl != NULL; decl = decl->next) {
        if (!GlslWriteStruct(out, decl) || fprintf(out, "\n") < 0) return 0;
    }
    for (decl = module->globals; decl != NULL; decl = decl->next) {
        if (!GlslWriteGlobal(out, decl)) return 0;
    }
    if (module->globals != NULL && fprintf(out, "\n") < 0) return 0;
    for (function = module->functions; function != NULL;
         function = function->next)
    {
        if (!function->isEntry && function->needsPrototype) {
            if (!GlslWriteFunctionSignature(out, function) ||
                fprintf(out, ";\n") < 0) return 0;
            wrotePrototype = 1;
        }
    }
    if (wrotePrototype && fprintf(out, "\n") < 0) return 0;
    for (function = module->functions; function != NULL;
         function = function->next)
    {
        if (!GlslWriteFunction(out, function)) return 0;
        if (function->next != NULL && fprintf(out, "\n") < 0) return 0;
    }
    return 1;
}

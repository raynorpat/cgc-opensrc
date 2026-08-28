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
#include <float.h>

#include "glsl_ir.h"

static const char *GlslStorageName(GlslStorage storage)
{
    switch (storage) {
    case GLSL_STORAGE_INPUT: return "input";
    case GLSL_STORAGE_OUTPUT: return "output";
    case GLSL_STORAGE_UNIFORM: return "uniform";
    case GLSL_STORAGE_SAMPLER: return "sampler";
    case GLSL_STORAGE_CONST: return "const";
    case GLSL_STORAGE_BUILTIN: return "builtin";
    default: return NULL;
    }
}

static const char *GlslInterpolationText(GlslInterpolation interpolation)
{
    switch (interpolation) {
    case GLSL_INTERPOLATION_FLAT: return "flat ";
    case GLSL_INTERPOLATION_NOPERSPECTIVE: return "noperspective ";
    case GLSL_INTERPOLATION_SMOOTH: return "smooth ";
    default: return "";
    }
}

static int GlslValidOperator(GlslOperator op)
{
    return op >= GLSL_OP_ASSIGN && op <= GLSL_OP_LOGICAL_NOT;
}

static int GlslFiniteFloat(float value)
{
    return value == value && value <= FLT_MAX && value >= -FLT_MAX;
}

static int GlslSimpleType(const GlslType *type, GlslBase base, int len)
{
    return type != NULL && type->base == base && type->len == len &&
           type->rows == 0 && type->cols == 0 && type->arraySize == 0 &&
           type->structName == NULL && type->elementType == NULL &&
           type->members == NULL;
}

static int GlslSamplerType(const GlslType *type)
{
    return GlslSimpleType(type, GLSL_BASE_SAMPLER1D, 1) ||
           GlslSimpleType(type, GLSL_BASE_SAMPLER2D, 1) ||
           GlslSimpleType(type, GLSL_BASE_SAMPLER3D, 1) ||
           GlslSimpleType(type, GLSL_BASE_SAMPLERCUBE, 1);
}

static int GlslSymbolTypesEqual(const GlslType *left,
                                const GlslType *right, int depth)
{
    if (left == NULL || right == NULL || depth > 64 ||
        left->base != right->base || left->len != right->len ||
        left->rows != right->rows || left->cols != right->cols ||
        left->arraySize != right->arraySize ||
        left->members != right->members ||
        (left->structName == NULL) != (right->structName == NULL) ||
        (left->elementType == NULL) != (right->elementType == NULL))
    {
        return 0;
    }
    if (left->structName != NULL &&
        strcmp(left->structName, right->structName)) return 0;
    if (left->elementType != NULL)
        return GlslSymbolTypesEqual(left->elementType,
                                    right->elementType, depth + 1);
    return 1;
}

static int GlslTypeContainsSampler(const GlslType *type, int depth)
{
    const GlslDecl *member;

    if (type == NULL || depth > 64)
        return 1;
    if (GlslSamplerType(type))
        return 1;
    if (type->elementType != NULL)
        return GlslTypeContainsSampler(type->elementType, depth + 1);
    if (type->base == GLSL_BASE_STRUCT) {
        for (member = type->members; member != NULL; member = member->next) {
            if (GlslTypeContainsSampler(&member->type, depth + 1))
                return 1;
        }
    }
    return 0;
}

static int GlslDeclInList(const GlslDecl *list, const GlslDecl *target)
{
    for (; list != NULL; list = list->next) {
        if (list == target)
            return 1;
    }
    return 0;
}

static int GlslSamplerUnitText(const char *text)
{
    int unit;

    return GlslParseSamplerUnit(text, &unit);
}

static const GlslBinding *GlslFindSamplerBinding(const GlslModule *module,
                                                 const GlslDecl *decl)
{
    const GlslBinding *binding;

    for (binding = module->bindings; binding != NULL;
         binding = binding->next)
    {
        if (binding->storage == GLSL_STORAGE_SAMPLER &&
            binding->declaration == decl) return binding;
    }
    return NULL;
}

static int GlslValidateSamplerSymbol(const GlslModule *module,
    const GlslExpr *expr, GlslBase expectedBase)
{
    const GlslBinding *binding;
    const GlslDecl *decl;

    if (expr == NULL || expr->kind != GLSL_EXPR_SYMBOL ||
        !GlslSimpleType(&expr->type, expectedBase, 1) ||
        expr->u.symbol == NULL)
    {
        return 0;
    }
    decl = expr->u.symbol;
    if (decl->storage != GLSL_STORAGE_SAMPLER ||
        !GlslSimpleType(&decl->type, expectedBase, 1) ||
        decl->name == NULL ||
        !GlslDeclInList(module->globals, decl)) return 0;
    binding = GlslFindSamplerBinding(module, decl);
    return binding != NULL && binding->name != NULL &&
           !strcmp(binding->name, decl->name) &&
           GlslSamplerUnitText(binding->semantic);
}

static int GlslValidateExpr(const GlslModule *module,
                            const GlslExpr *expr);

static int GlslValidateExprList(const GlslModule *module,
                                const GlslExpr *expr)
{
    for (; expr != NULL; expr = expr->next) {
        if (!GlslValidateExpr(module, expr))
            return 0;
    }
    return 1;
}

static int GlslValidateTextureCall(const GlslModule *module,
                                   const GlslExpr *expr)
{
    const GlslExpr *sampler;
    const GlslExpr *coord;
    GlslBase samplerBase;
    int coordLen;

    if ((module->stage != GLSL_STAGE_FRAGMENT &&
         module->stage != GLSL_STAGE_GEOMETRY) ||
        expr->u.call.name == NULL ||
        strcmp(expr->u.call.name,
               GlslBuiltinSpelling(expr->u.call.builtin)) ||
        !GlslSimpleType(&expr->type, GLSL_BASE_FLOAT, 4)) return 0;
    switch (expr->u.call.builtin) {
    case GLSL_BUILTIN_TEX1D:
        samplerBase = GLSL_BASE_SAMPLER1D;
        coordLen = 1;
        break;
    case GLSL_BUILTIN_TEX2D:
        samplerBase = GLSL_BASE_SAMPLER2D;
        coordLen = 2;
        break;
    case GLSL_BUILTIN_TEX3D:
        samplerBase = GLSL_BASE_SAMPLER3D;
        coordLen = 3;
        break;
    case GLSL_BUILTIN_TEXCUBE:
        samplerBase = GLSL_BASE_SAMPLERCUBE;
        coordLen = 3;
        break;
    case GLSL_BUILTIN_TEX1D_PROJ:
        samplerBase = GLSL_BASE_SAMPLER1D;
        coordLen = 4;
        break;
    case GLSL_BUILTIN_TEX2D_PROJ:
        samplerBase = GLSL_BASE_SAMPLER2D;
        coordLen = 4;
        break;
    case GLSL_BUILTIN_TEX3D_PROJ:
        samplerBase = GLSL_BASE_SAMPLER3D;
        coordLen = 4;
        break;
    case GLSL_BUILTIN_TEXCUBE_PROJ:
        samplerBase = GLSL_BASE_SAMPLERCUBE;
        coordLen = 4;
        break;
    default:
        return 0;
    }
    sampler = expr->u.call.arguments;
    coord = sampler != NULL ? sampler->next : NULL;
    return coord != NULL && coord->next == NULL &&
           GlslValidateSamplerSymbol(module, sampler, samplerBase) &&
           GlslSimpleType(&coord->type, GLSL_BASE_FLOAT, coordLen) &&
           GlslValidateExpr(module, coord);
}

static int GlslValidateExpr(const GlslModule *module,
                            const GlslExpr *expr)
{
    const char *builtinName;

    if (expr == NULL || GlslTypeName(&expr->type) == NULL ||
        GlslTypeContainsSampler(&expr->type, 0)) return 0;
    switch (expr->kind) {
    case GLSL_EXPR_SYMBOL:
        return expr->u.symbol != NULL && expr->u.symbol->name != NULL &&
               !GlslTypeContainsSampler(&expr->u.symbol->type, 0) &&
               GlslSymbolTypesEqual(&expr->type,
                                    &expr->u.symbol->type, 0);
    case GLSL_EXPR_INT:
    case GLSL_EXPR_BOOL:
        return 1;
    case GLSL_EXPR_FLOAT:
        return GlslFiniteFloat(expr->u.literalFloat);
    case GLSL_EXPR_UNARY:
        return GlslValidOperator(expr->u.unary.op) &&
               expr->u.unary.op >= GLSL_OP_NEGATE &&
               GlslValidateExpr(module, expr->u.unary.operand);
    case GLSL_EXPR_BINARY:
        return GlslValidOperator(expr->u.binary.op) &&
               expr->u.binary.op <= GLSL_OP_DIVIDE &&
               GlslValidateExpr(module, expr->u.binary.left) &&
               GlslValidateExpr(module, expr->u.binary.right);
    case GLSL_EXPR_CONDITIONAL:
        return GlslValidateExpr(module, expr->u.conditional.condition) &&
               GlslValidateExpr(module, expr->u.conditional.trueExpr) &&
               GlslValidateExpr(module, expr->u.conditional.falseExpr);
    case GLSL_EXPR_CALL:
        if (expr->u.call.builtin >= GLSL_BUILTIN_TEX1D &&
            expr->u.call.builtin <= GLSL_BUILTIN_TEXCUBE_PROJ)
        {
            return GlslValidateTextureCall(module, expr);
        }
        if (expr->u.call.builtin != GLSL_BUILTIN_NONE) {
            builtinName = GlslBuiltinSpelling(expr->u.call.builtin);
            if (builtinName == NULL || expr->u.call.name == NULL ||
                strcmp(expr->u.call.name, builtinName)) return 0;
        }
        return expr->u.call.name != NULL &&
               GlslValidateExprList(module, expr->u.call.arguments);
    case GLSL_EXPR_CONSTRUCT:
        return expr->u.construct.arguments != NULL &&
               GlslValidateExprList(module, expr->u.construct.arguments);
    case GLSL_EXPR_MEMBER:
        return expr->u.member.name != NULL &&
               GlslValidateExpr(module, expr->u.member.object);
    case GLSL_EXPR_INDEX:
        return GlslValidateExpr(module, expr->u.index.object) &&
               GlslValidateExpr(module, expr->u.index.index);
    case GLSL_EXPR_SWIZZLE:
        return expr->u.swizzle.mask != NULL &&
               expr->u.swizzle.mask[0] != '\0' &&
               GlslValidateExpr(module, expr->u.swizzle.object);
    default:
        return 0;
    }
}

static int GlslValidateDecls(const GlslDecl *decl, int samplerGlobals)
{
    int containsSampler;

    for (; decl != NULL; decl = decl->next) {
        containsSampler = GlslTypeContainsSampler(&decl->type, 0);
        if (decl->name == NULL || GlslTypeName(&decl->type) == NULL ||
            decl->initializer != NULL ||
            (containsSampler &&
             (!samplerGlobals || !GlslSamplerType(&decl->type) ||
              decl->storage != GLSL_STORAGE_SAMPLER)) ||
            (!containsSampler && decl->storage == GLSL_STORAGE_SAMPLER))
        {
            return 0;
        }
    }
    return 1;
}

static int GlslValidateStmtList(const GlslModule *module,
                                const GlslStmt *stmt);

static int GlslValidateForPart(const GlslModule *module,
                               const GlslStmt *stmt)
{
    for (; stmt != NULL; stmt = stmt->next) {
        if (stmt->kind != GLSL_STMT_EXPRESSION ||
            !GlslValidateExpr(module, stmt->u.expression))
        {
            return 0;
        }
    }
    return 1;
}

static int GlslValidateStmtList(const GlslModule *module,
                                const GlslStmt *stmt)
{
    for (; stmt != NULL; stmt = stmt->next) {
        switch (stmt->kind) {
        case GLSL_STMT_EXPRESSION:
            if (!GlslValidateExpr(module, stmt->u.expression)) return 0;
            break;
        case GLSL_STMT_IF:
            if (!GlslValidateExpr(module, stmt->u.ifStmt.condition) ||
                !GlslValidateStmtList(module,
                                      stmt->u.ifStmt.trueBranch) ||
                !GlslValidateStmtList(module,
                                      stmt->u.ifStmt.falseBranch))
            {
                return 0;
            }
            break;
        case GLSL_STMT_WHILE:
        case GLSL_STMT_DO:
            if (!GlslValidateExpr(module, stmt->u.loop.condition) ||
                !GlslValidateStmtList(module, stmt->u.loop.body)) return 0;
            break;
        case GLSL_STMT_FOR:
            if (!GlslValidateForPart(module, stmt->u.forStmt.init) ||
                (stmt->u.forStmt.condition != NULL &&
                 !GlslValidateExpr(module, stmt->u.forStmt.condition)) ||
                !GlslValidateForPart(module, stmt->u.forStmt.step) ||
                !GlslValidateStmtList(module, stmt->u.forStmt.body)) return 0;
            break;
        case GLSL_STMT_BLOCK:
            if (!GlslValidateStmtList(module, stmt->u.block)) return 0;
            break;
        case GLSL_STMT_RETURN:
            if (stmt->u.returnExpr != NULL &&
                !GlslValidateExpr(module, stmt->u.returnExpr)) return 0;
            break;
        case GLSL_STMT_DISCARD:
            if (module->stage != GLSL_STAGE_FRAGMENT) return 0;
            break;
        case GLSL_STMT_BREAK:
        case GLSL_STMT_CONTINUE:
            break;
        case GLSL_STMT_GEOMETRY_EMIT:
        case GLSL_STMT_GEOMETRY_RESTART:
            if (module->stage != GLSL_STAGE_GEOMETRY) return 0;
            break;
        default:
            return 0;
        }
    }
    return 1;
}

static int GlslValidateSamplerBindings(const GlslModule *module)
{
    const GlslBinding *binding;
    const GlslBinding *match;
    const GlslDecl *decl;
    int bindingCount;
    int expectedUnit;
    int matchCount;
    int namedDeclCount;
    int samplerCount;
    int unit;

    bindingCount = 0;
    for (binding = module->bindings; binding != NULL;
         binding = binding->next)
    {
        if (binding->storage != GLSL_STORAGE_SAMPLER)
            continue;
        bindingCount++;
        if ((module->stage != GLSL_STAGE_FRAGMENT &&
             module->stage != GLSL_STAGE_GEOMETRY) ||
            bindingCount > 16 ||
            binding->declaration == NULL ||
            !GlslDeclInList(module->globals, binding->declaration) ||
            binding->declaration->storage != GLSL_STORAGE_SAMPLER ||
            !GlslSamplerType(&binding->declaration->type) ||
            binding->name == NULL || binding->declaration->name == NULL ||
            strcmp(binding->name, binding->declaration->name) ||
            !GlslParseSamplerUnit(binding->semantic, &unit) ||
            binding->interfaceKey != NULL || binding->isOutput ||
            binding->defaultCount != 0 ||
            binding->defaultValues != NULL) return 0;
        namedDeclCount = 0;
        for (decl = module->globals; decl != NULL; decl = decl->next) {
            if (decl->storage == GLSL_STORAGE_SAMPLER &&
                decl->name != NULL && !strcmp(binding->name, decl->name))
            {
                namedDeclCount++;
            }
        }
        if (namedDeclCount != 1)
            return 0;
    }

    samplerCount = 0;
    expectedUnit = 0;
    for (decl = module->globals; decl != NULL; decl = decl->next) {
        if (decl->storage != GLSL_STORAGE_SAMPLER)
            continue;
        samplerCount++;
        if ((module->stage != GLSL_STAGE_FRAGMENT &&
             module->stage != GLSL_STAGE_GEOMETRY) ||
            samplerCount > 16 ||
            !GlslSamplerType(&decl->type)) return 0;
        match = NULL;
        matchCount = 0;
        for (binding = module->bindings; binding != NULL;
             binding = binding->next)
        {
            if (binding->storage == GLSL_STORAGE_SAMPLER &&
                binding->declaration == decl)
            {
                match = binding;
                matchCount++;
            }
        }
        if (matchCount != 1 || match == NULL ||
            !GlslSamplerUnitMatches(match->semantic,
                                    expectedUnit)) return 0;
        expectedUnit++;
    }
    return samplerCount == bindingCount;
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
        int i;

        if (GlslStorageName(binding->storage) == NULL ||
            binding->name == NULL || binding->semantic == NULL) return 0;
        if (binding->storage == GLSL_STORAGE_SAMPLER &&
            (binding->declaration == NULL ||
             binding->declaration->storage != GLSL_STORAGE_SAMPLER ||
             !GlslSamplerType(&binding->declaration->type) ||
             !GlslDeclInList(module->globals, binding->declaration) ||
             binding->declaration->name == NULL ||
             strcmp(binding->name, binding->declaration->name) ||
             !GlslSamplerUnitText(binding->semantic))) return 0;
        if (binding->storage != GLSL_STORAGE_SAMPLER &&
            binding->declaration != NULL &&
            GlslTypeContainsSampler(&binding->declaration->type, 0)) return 0;
        if (binding->defaultCount < 0 ||
            (binding->defaultCount > 0 && binding->defaultValues == NULL))
            return 0;
        for (i = 0; i < binding->defaultCount; i++) {
            if (!GlslFiniteFloat(binding->defaultValues[i])) return 0;
        }
    }
    for (decl = module->structs; decl != NULL; decl = decl->next) {
        if (decl->name == NULL ||
            !GlslValidateDecls(decl->members, 0)) return 0;
    }
    if (!GlslValidateDecls(module->globals, 1) ||
        !GlslValidateSamplerBindings(module)) return 0;
    for (function = module->functions; function != NULL;
         function = function->next)
    {
        if (function->name == NULL ||
            GlslTypeName(&function->result) == NULL ||
            GlslTypeContainsSampler(&function->result, 0) ||
            !GlslValidateDecls(function->parameters, 0) ||
            !GlslValidateDecls(function->locals, 0) ||
            !GlslValidateStmtList(module, function->body)) return 0;
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
    char *decimal;
    char text[64];
    size_t length;

    if (!GlslFiniteFloat(value) ||
        snprintf(text, sizeof(text), "%.9g", value) < 0) return 0;
    decimal = strchr(text, ',');
    if (decimal != NULL)
        *decimal = '.';
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

    if (!rightChild && precedence == parentPrecedence &&
        parentOperator == GLSL_OP_ASSIGN) needParens = 1;
    if (rightChild && precedence == parentPrecedence) {
        switch (parentOperator) {
        case GLSL_OP_EQUAL:
        case GLSL_OP_NOT_EQUAL:
        case GLSL_OP_LESS:
        case GLSL_OP_GREATER:
        case GLSL_OP_LESS_EQUAL:
        case GLSL_OP_GREATER_EQUAL:
        case GLSL_OP_ADD:
        case GLSL_OP_SUBTRACT:
        case GLSL_OP_MULTIPLY:
        case GLSL_OP_DIVIDE:
            needParens = 1;
            break;
        default:
            break;
        }
    }
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
            !GlslWriteExprPrec(out, expr->u.unary.operand, precedence + 1, 1,
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
                               precedence + 1, 0, GLSL_OP_NONE) ||
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

static int GlslWriteDeclarator(FILE *out, const GlslType *type,
                               const char *name)
{
    const GlslType *current;

    if (fprintf(out, "%s %s", GlslTypeName(type), name) < 0)
        return 0;
    for (current = type; current != NULL && current->elementType != NULL;
         current = current->elementType)
    {
        if (fprintf(out, "[%d]", current->arraySize) < 0)
            return 0;
    }
    return 1;
}

static int GlslWriteStruct(FILE *out, const GlslDecl *decl)
{
    const GlslDecl *member;
    if (fprintf(out, "struct %s\n{\n", decl->name) < 0) return 0;
    for (member = decl->members; member != NULL; member = member->next) {
        if (fprintf(out, "    ") < 0 ||
            !GlslWriteDeclarator(out, &member->type, member->name) ||
            fprintf(out, ";\n") < 0) return 0;
    }
    return fprintf(out, "};\n") >= 0;
}

static int GlslWriteGlobal(FILE *out, GlslStage stage, const GlslDecl *decl)
{
    const char *interpolation;
    const char *storage;

    interpolation = GlslInterpolationText(decl->interpolation);
    if (decl->storage == GLSL_STORAGE_INPUT ||
        decl->storage == GLSL_STORAGE_OUTPUT)
    {
        /* Core 1.50 interfaces: optional interpolation qualifier, then
         * the stage-directional spelling from the shared table. */
        storage = GlslStorageSpelling(stage, decl->storage);
    } else {
        storage = decl->storage == GLSL_STORAGE_SAMPLER ? "uniform" :
                  GlslStorageName(decl->storage);
    }
    if (interpolation[0] != '\0' &&
        fprintf(out, "%s", interpolation) < 0)
    {
        return 0;
    }
    if (storage != NULL && fprintf(out, "%s ", storage) < 0)
        return 0;
    return GlslWriteDeclarator(out, &decl->type, decl->name) &&
           fprintf(out, ";\n") >= 0;
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
            fprintf(out, "%s",
                    GlslParameterQualifierName(parameter->parameterQualifier)) < 0 ||
            !GlslWriteDeclarator(out, &parameter->type, parameter->name)) return 0;
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
    case GLSL_STMT_GEOMETRY_EMIT: {
        const GlslStmt *assign;
        const GlslFlatReplay *replay;

        for (assign = stmt->u.emit.assignments;
             assign != NULL; assign = assign->next)
        {
            if (assign != stmt->u.emit.assignments &&
                !GlslWriteIndent(out, level)) return 0;
            if (!GlslWriteExprPrec(out,
                    assign->u.expression, 0, 0, GLSL_OP_NONE) ||
                fprintf(out, ";\n") < 0) return 0;
        }
        for (replay = stmt->u.emit.replay;
             replay != NULL; replay = replay->next)
        {
            if (!GlslWriteIndent(out, level) ||
                fprintf(out, "if (%s) {\n",
                        replay->defined->name) < 0 ||
                !GlslWriteIndent(out, level + 1) ||
                fprintf(out, "%s = %s;\n",
                        replay->target->name,
                        replay->shadow->name) < 0 ||
                !GlslWriteIndent(out, level) ||
                fprintf(out, "}\n") < 0) return 0;
        }
        if (!GlslWriteIndent(out, level) ||
            fprintf(out, "EmitVertex();\n") < 0) return 0;
        return 1;
    }
    case GLSL_STMT_GEOMETRY_RESTART:
        return fprintf(out, "EndPrimitive();\n") >= 0;
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
        if (fprintf(out, "    ") < 0 ||
            !GlslWriteDeclarator(out, &decl->type, decl->name) ||
            fprintf(out, ";\n") < 0) return 0;
    }
    if (!GlslWriteStmtList(out, function->body, 1)) return 0;
    return fprintf(out, "}\n") >= 0;
}

int GlslWriteModule(FILE *out, const GlslModule *module)
{
    static const GlslStorage globalOrder[] = {
        GLSL_STORAGE_INPUT,
        GLSL_STORAGE_OUTPUT,
        GLSL_STORAGE_UNIFORM,
        GLSL_STORAGE_SAMPLER,
        GLSL_STORAGE_CONST,
        GLSL_STORAGE_NONE
    };
    const GlslBinding *binding;
    const GlslDecl *decl;
    const GlslFunction *function;
    int i;
    int wrotePrototype = 0;

    if (out == NULL || !GlslValidateModule(module)) return 0;
    /* The writer owns the only version directive; it is the first
     * output after the module structural check succeeds.  Core GLSL
     * 1.50, matching VERSION_STRING_GLSL in glsl_hal.h. */
    if (fprintf(out, "#version 150\n") < 0) return 0;
    if (module->stage == GLSL_STAGE_GEOMETRY) {
        const char *inputToken = NULL;
        const char *outputToken = NULL;

        if (module->geometry != NULL) {
            switch (module->geometry->inputTopology) {
                case GLSL_GEOMETRY_INPUT_POINTS:   inputToken = "points"; break;
                case GLSL_GEOMETRY_INPUT_LINES:    inputToken = "lines"; break;
                case GLSL_GEOMETRY_INPUT_LINES_ADJACENCY:  inputToken = "lines_adjacency"; break;
                case GLSL_GEOMETRY_INPUT_TRIANGLES: inputToken = "triangles"; break;
                case GLSL_GEOMETRY_INPUT_TRIANGLES_ADJACENCY: inputToken = "triangles_adjacency"; break;
            }
            switch (module->geometry->outputTopology) {
                case GLSL_GEOMETRY_OUTPUT_POINTS:       outputToken = "points"; break;
                case GLSL_GEOMETRY_OUTPUT_LINE_STRIP:   outputToken = "line_strip"; break;
                case GLSL_GEOMETRY_OUTPUT_TRIANGLE_STRIP: outputToken = "triangle_strip"; break;
            }
        }
        if (inputToken != NULL &&
            fprintf(out, "layout(%s) in;\n", inputToken) < 0) return 0;
        if (outputToken != NULL) {
            if (module->geometry->maxOutputVertices > 0) {
                if (fprintf(out, "layout(%s, max_vertices = %d) out;\n",
                        outputToken,
                        module->geometry->maxOutputVertices) < 0) return 0;
            } else {
                if (fprintf(out, "layout(%s) out;\n", outputToken) < 0)
                    return 0;
            }
        }
        if (fprintf(out, "\n") < 0) return 0;
    }
    for (binding = module->bindings; binding != NULL;
         binding = binding->next)
    {
        int i;

        if (fprintf(out, "// cgc-bind %s %s",
                    GlslStorageName(binding->storage), binding->name) < 0)
            return 0;
        if (binding->semantic[0] != '\0' &&
            fprintf(out, " %s", binding->semantic) < 0) return 0;
        if (fprintf(out, "\n") < 0) return 0;
        if (binding->defaultCount > 0) {
            if (fprintf(out, "// cgc-default %s", binding->name) < 0)
                return 0;
            for (i = 0; i < binding->defaultCount; i++) {
                if (fprintf(out, " ") < 0 ||
                    !GlslWriteFloat(out, binding->defaultValues[i])) return 0;
            }
            if (fprintf(out, "\n") < 0) return 0;
        }
    }
    if (module->bindings != NULL && fprintf(out, "\n") < 0) return 0;
    for (decl = module->structs; decl != NULL; decl = decl->next) {
        if (!GlslWriteStruct(out, decl) || fprintf(out, "\n") < 0) return 0;
    }
    for (i = 0; i < (int) (sizeof(globalOrder) / sizeof(globalOrder[0]));
         i++)
    {
        for (decl = module->globals; decl != NULL; decl = decl->next) {
            if (decl->storage == globalOrder[i] &&
                !GlslWriteGlobal(out, module->stage, decl))
            {
                return 0;
            }
        }
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

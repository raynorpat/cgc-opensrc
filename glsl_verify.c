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
/* glsl_verify.c - Standalone structural and stage-aware verification
 *       of the shared GLSL target IR.  Precedes code generation so a
 *       broken target module writes no bytes at all. */

#include "glsl_ir.h"

#include <float.h>
#include <string.h>

static int GlslVerifyFail(GlslVerifyDiagnostic *diagnostic,
    GlslVerifyReason reason, const GlslLoc *loc, const void *node)
{
    diagnostic->reason = reason;
    if (loc != NULL)
        diagnostic->loc = *loc;
    diagnostic->node = node;
    return 0;
}

static int GlslVerifyLoc(GlslVerifyDiagnostic *diagnostic,
    const GlslLoc *loc, const void *node)
{
    if (loc->file < 0 || loc->line < 0)
        return GlslVerifyFail(diagnostic, GLSL_VERIFY_LOCATION,
                              loc, node);
    return 1;
}

static int GlslVerifyTypesEqual(const GlslType *left,
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
        strcmp(left->structName, right->structName))
    {
        return 0;
    }
    if (left->elementType != NULL) {
        return GlslVerifyTypesEqual(left->elementType,
                                    right->elementType, depth + 1);
    }
    return 1;
}

static int GlslVerifySimpleType(const GlslType *type,
    GlslBase base, int len)
{
    return type != NULL && type->base == base && type->len == len &&
           type->rows == 0 && type->cols == 0 &&
           type->arraySize == 0 && type->structName == NULL &&
           type->elementType == NULL && type->members == NULL;
}

static int GlslVerifySamplerType(const GlslType *type)
{
    return GlslVerifySimpleType(type, GLSL_BASE_SAMPLER1D, 1) ||
           GlslVerifySimpleType(type, GLSL_BASE_SAMPLER2D, 1) ||
           GlslVerifySimpleType(type, GLSL_BASE_SAMPLER3D, 1) ||
           GlslVerifySimpleType(type, GLSL_BASE_SAMPLERCUBE, 1);
}

static int GlslVerifyTypeContainsSampler(const GlslType *type,
    int depth)
{
    const GlslDecl *member;

    if (type == NULL || depth > 64)
        return 1;
    if (GlslVerifySamplerType(type))
        return 1;
    if (type->elementType != NULL)
        return GlslVerifyTypeContainsSampler(type->elementType,
                                             depth + 1);
    if (type->base == GLSL_BASE_STRUCT) {
        for (member = type->members; member != NULL;
             member = member->next)
        {
            if (GlslVerifyTypeContainsSampler(&member->type, depth + 1))
                return 1;
        }
    }
    return 0;
}

static int GlslVerifyType(GlslVerifyDiagnostic *diagnostic,
    const GlslType *type, const GlslLoc *loc, const void *node)
{
    if (type == NULL || GlslTypeName(type) == NULL)
        return GlslVerifyFail(diagnostic, GLSL_VERIFY_TYPE, loc, node);
    if (type->elementType != NULL)
        return GlslVerifyType(diagnostic, type->elementType, loc, node);
    return 1;
}

static int GlslVerifyDeclInList(const GlslDecl *list,
    const GlslDecl *target)
{
    for (; list != NULL; list = list->next) {
        if (list == target)
            return 1;
    }
    return 0;
}

static int GlslVerifyDeclList(GlslVerifyDiagnostic *diagnostic,
    const GlslDecl *decl, int samplerGlobals)
{
    for (; decl != NULL; decl = decl->next) {
        if (!GlslVerifyLoc(diagnostic, &decl->loc, decl))
            return 0;
        if (decl->name == NULL || decl->name[0] == '\0' ||
            decl->initializer != NULL ||
            decl->storage < GLSL_STORAGE_NONE ||
            decl->storage > GLSL_STORAGE_BUILTIN ||
            decl->interpolation < GLSL_INTERPOLATION_DEFAULT ||
            decl->interpolation > GLSL_INTERPOLATION_SMOOTH)
        {
            return GlslVerifyFail(diagnostic, GLSL_VERIFY_DECLARATION,
                                  &decl->loc, decl);
        }
        if (!GlslVerifyType(diagnostic, &decl->type,
                            &decl->loc, decl))
        {
            return 0;
        }
        if (GlslVerifyTypeContainsSampler(&decl->type, 0)) {
            if (!samplerGlobals ||
                decl->storage != GLSL_STORAGE_SAMPLER ||
                !GlslVerifySamplerType(&decl->type))
            {
                return GlslVerifyFail(diagnostic, GLSL_VERIFY_TYPE,
                                      &decl->loc, decl);
            }
        } else if (decl->storage == GLSL_STORAGE_SAMPLER) {
            return GlslVerifyFail(diagnostic, GLSL_VERIFY_TYPE,
                                  &decl->loc, decl);
        }
    }
    return 1;
}

static int GlslVerifyExpr(const GlslModule *module,
    GlslVerifyDiagnostic *diagnostic, const GlslExpr *expr);

static int GlslVerifyTextureCall(const GlslModule *module,
    GlslVerifyDiagnostic *diagnostic, const GlslExpr *expr)
{
    const GlslExpr *sampler;
    const GlslExpr *coord;
    const GlslBinding *binding;
    GlslBase samplerBase;
    int bindingCount;
    int coordLen;
    int unit;

    if ((module->stage != GLSL_STAGE_FRAGMENT &&
         module->stage != GLSL_STAGE_GEOMETRY) ||
        expr->u.call.name == NULL ||
        strcmp(expr->u.call.name,
               GlslBuiltinSpelling(expr->u.call.builtin)) ||
        !GlslVerifySimpleType(&expr->type, GLSL_BASE_FLOAT, 4))
    {
        return GlslVerifyFail(diagnostic, GLSL_VERIFY_TYPE,
                              &expr->loc, expr);
    }
    switch (expr->u.call.builtin) {
    case GLSL_BUILTIN_TEX1D:
        samplerBase = GLSL_BASE_SAMPLER1D; coordLen = 1; break;
    case GLSL_BUILTIN_TEX2D:
        samplerBase = GLSL_BASE_SAMPLER2D; coordLen = 2; break;
    case GLSL_BUILTIN_TEX3D:
        samplerBase = GLSL_BASE_SAMPLER3D; coordLen = 3; break;
    case GLSL_BUILTIN_TEXCUBE:
        samplerBase = GLSL_BASE_SAMPLERCUBE; coordLen = 3; break;
    case GLSL_BUILTIN_TEX1D_PROJ:
        samplerBase = GLSL_BASE_SAMPLER1D; coordLen = 4; break;
    case GLSL_BUILTIN_TEX2D_PROJ:
        samplerBase = GLSL_BASE_SAMPLER2D; coordLen = 4; break;
    case GLSL_BUILTIN_TEX3D_PROJ:
        samplerBase = GLSL_BASE_SAMPLER3D; coordLen = 4; break;
    case GLSL_BUILTIN_TEXCUBE_PROJ:
        samplerBase = GLSL_BASE_SAMPLERCUBE; coordLen = 4; break;
    default:
        return GlslVerifyFail(diagnostic, GLSL_VERIFY_TYPE,
                              &expr->loc, expr);
    }
    sampler = expr->u.call.arguments;
    coord = sampler != NULL ? sampler->next : NULL;
    if (sampler == NULL || coord == NULL || coord->next != NULL ||
        sampler->kind != GLSL_EXPR_SYMBOL ||
        sampler->u.symbol == NULL ||
        !GlslVerifySimpleType(&sampler->type, samplerBase, 1) ||
        !GlslVerifyTypesEqual(&sampler->type,
                              &sampler->u.symbol->type, 0) ||
        sampler->u.symbol->storage != GLSL_STORAGE_SAMPLER ||
        !GlslVerifyDeclInList(module->globals, sampler->u.symbol) ||
        !GlslVerifySimpleType(&coord->type, GLSL_BASE_FLOAT, coordLen))
    {
        return GlslVerifyFail(diagnostic, GLSL_VERIFY_TYPE,
                              &expr->loc, expr);
    }
    bindingCount = 0;
    for (binding = module->bindings; binding != NULL;
         binding = binding->next)
    {
        if (binding->storage == GLSL_STORAGE_SAMPLER &&
            binding->declaration == sampler->u.symbol &&
            binding->name != NULL && sampler->u.symbol->name != NULL &&
            !strcmp(binding->name, sampler->u.symbol->name) &&
            GlslParseSamplerUnit(binding->semantic, &unit))
        {
            bindingCount++;
        }
    }
    if (bindingCount != 1)
        return GlslVerifyFail(diagnostic, GLSL_VERIFY_INTERFACE,
                              &expr->loc, expr);
    return GlslVerifyExpr(module, diagnostic, coord);
}

static int GlslVerifyExpr(const GlslModule *module,
    GlslVerifyDiagnostic *diagnostic, const GlslExpr *expr)
{
    const GlslExpr *argument;

    if (expr == NULL)
        return GlslVerifyFail(diagnostic, GLSL_VERIFY_CONTROL,
                              NULL, expr);
    if (!GlslVerifyLoc(diagnostic, &expr->loc, expr) ||
        !GlslVerifyType(diagnostic, &expr->type, &expr->loc, expr))
    {
        return 0;
    }
    if (GlslVerifyTypeContainsSampler(&expr->type, 0))
        return GlslVerifyFail(diagnostic, GLSL_VERIFY_TYPE,
                              &expr->loc, expr);
    switch (expr->kind) {
    case GLSL_EXPR_SYMBOL:
        if (expr->u.symbol == NULL || expr->u.symbol->name == NULL ||
            !GlslVerifyTypesEqual(&expr->type,
                                  &expr->u.symbol->type, 0))
        {
            return GlslVerifyFail(diagnostic, GLSL_VERIFY_DECLARATION,
                                  &expr->loc, expr);
        }
        return 1;
    case GLSL_EXPR_INT:
    case GLSL_EXPR_BOOL:
        return 1;
    case GLSL_EXPR_FLOAT:
        if (expr->u.literalFloat != expr->u.literalFloat ||
            expr->u.literalFloat > FLT_MAX ||
            expr->u.literalFloat < -FLT_MAX)
        {
            return GlslVerifyFail(diagnostic, GLSL_VERIFY_TYPE,
                                  &expr->loc, expr);
        }
        return 1;
    case GLSL_EXPR_UNARY:
        if (expr->u.unary.op < GLSL_OP_NEGATE ||
            expr->u.unary.op > GLSL_OP_LOGICAL_NOT)
        {
            return GlslVerifyFail(diagnostic, GLSL_VERIFY_CONTROL,
                                  &expr->loc, expr);
        }
        return GlslVerifyExpr(module, diagnostic, expr->u.unary.operand);
    case GLSL_EXPR_BINARY:
        if (expr->u.binary.op < GLSL_OP_ASSIGN ||
            expr->u.binary.op > GLSL_OP_DIVIDE)
        {
            return GlslVerifyFail(diagnostic, GLSL_VERIFY_CONTROL,
                                  &expr->loc, expr);
        }
        return GlslVerifyExpr(module, diagnostic, expr->u.binary.left) &&
               GlslVerifyExpr(module, diagnostic, expr->u.binary.right);
    case GLSL_EXPR_CONDITIONAL:
        return GlslVerifyExpr(module, diagnostic,
                              expr->u.conditional.condition) &&
               GlslVerifyExpr(module, diagnostic,
                              expr->u.conditional.trueExpr) &&
               GlslVerifyExpr(module, diagnostic,
                              expr->u.conditional.falseExpr);
    case GLSL_EXPR_CALL:
        if (expr->u.call.name == NULL)
            return GlslVerifyFail(diagnostic, GLSL_VERIFY_CONTROL,
                                  &expr->loc, expr);
        if (expr->u.call.builtin >= GLSL_BUILTIN_TEX1D &&
            expr->u.call.builtin <= GLSL_BUILTIN_TEXCUBE_PROJ)
        {
            return GlslVerifyTextureCall(module, diagnostic, expr);
        }
        if (expr->u.call.builtin != GLSL_BUILTIN_NONE &&
            (GlslBuiltinSpelling(expr->u.call.builtin) == NULL ||
             strcmp(expr->u.call.name,
                    GlslBuiltinSpelling(expr->u.call.builtin))))
        {
            return GlslVerifyFail(diagnostic, GLSL_VERIFY_CONTROL,
                                  &expr->loc, expr);
        }
        for (argument = expr->u.call.arguments; argument != NULL;
             argument = argument->next)
        {
            if (!GlslVerifyExpr(module, diagnostic, argument))
                return 0;
        }
        return 1;
    case GLSL_EXPR_CONSTRUCT:
        if (expr->u.construct.arguments == NULL)
            return GlslVerifyFail(diagnostic, GLSL_VERIFY_CONTROL,
                                  &expr->loc, expr);
        if (expr->type.elementType != NULL) {
            int count;

            count = 0;
            for (argument = expr->u.construct.arguments;
                 argument != NULL; argument = argument->next)
            {
                if (!GlslVerifyTypesEqual(&argument->type,
                                          expr->type.elementType, 0))
                {
                    return GlslVerifyFail(diagnostic, GLSL_VERIFY_TYPE,
                                          &argument->loc, argument);
                }
                count++;
            }
            if (count != expr->type.arraySize)
                return GlslVerifyFail(diagnostic, GLSL_VERIFY_TYPE,
                                      &expr->loc, expr);
        }
        for (argument = expr->u.construct.arguments; argument != NULL;
             argument = argument->next)
        {
            if (!GlslVerifyExpr(module, diagnostic, argument))
                return 0;
        }
        return 1;
    case GLSL_EXPR_MEMBER:
        if (expr->u.member.name == NULL)
            return GlslVerifyFail(diagnostic, GLSL_VERIFY_DECLARATION,
                                  &expr->loc, expr);
        return GlslVerifyExpr(module, diagnostic,
                              expr->u.member.object);
    case GLSL_EXPR_INDEX:
        return GlslVerifyExpr(module, diagnostic,
                              expr->u.index.object) &&
               GlslVerifyExpr(module, diagnostic,
                              expr->u.index.index);
    case GLSL_EXPR_SWIZZLE:
        if (expr->u.swizzle.mask == NULL ||
            expr->u.swizzle.mask[0] == '\0')
        {
            return GlslVerifyFail(diagnostic, GLSL_VERIFY_CONTROL,
                                  &expr->loc, expr);
        }
        return GlslVerifyExpr(module, diagnostic,
                              expr->u.swizzle.object);
    default:
        return GlslVerifyFail(diagnostic, GLSL_VERIFY_CONTROL,
                              &expr->loc, expr);
    }
}

static int GlslVerifyAssignments(const GlslModule *module,
    GlslVerifyDiagnostic *diagnostic, const GlslStmt *stmt)
{
    for (; stmt != NULL; stmt = stmt->next) {
        if (stmt->kind != GLSL_STMT_EXPRESSION ||
            stmt->u.expression == NULL ||
            stmt->u.expression->kind != GLSL_EXPR_BINARY ||
            stmt->u.expression->u.binary.op != GLSL_OP_ASSIGN)
        {
            return GlslVerifyFail(diagnostic, GLSL_VERIFY_GEOMETRY,
                                  &stmt->loc, stmt);
        }
        if (!GlslVerifyExpr(module, diagnostic, stmt->u.expression))
            return 0;
    }
    return 1;
}

static int GlslVerifyReplay(const GlslModule *module,
    GlslVerifyDiagnostic *diagnostic, const GlslStmt *emit)
{
    const GlslFlatReplay *replay;

    for (replay = emit->u.emit.replay; replay != NULL;
         replay = replay->next)
    {
        if (replay->target == NULL || replay->shadow == NULL ||
            replay->defined == NULL ||
            !GlslVerifyDeclInList(module->globals, replay->target) ||
            !GlslVerifyDeclInList(module->globals, replay->shadow) ||
            !GlslVerifyDeclInList(module->globals, replay->defined) ||
            replay->target->storage != GLSL_STORAGE_OUTPUT ||
            replay->shadow->storage != GLSL_STORAGE_NONE ||
            replay->defined->storage != GLSL_STORAGE_NONE)
        {
            return GlslVerifyFail(diagnostic, GLSL_VERIFY_DECLARATION,
                                  &emit->loc, replay);
        }
        if (!GlslVerifyTypesEqual(&replay->target->type,
                                  &replay->shadow->type, 0) ||
            replay->defined->type.base != GLSL_BASE_BOOL ||
            replay->defined->type.len != 1 ||
            replay->defined->type.rows != 0 ||
            replay->defined->type.cols != 0 ||
            replay->defined->type.elementType != NULL)
        {
            return GlslVerifyFail(diagnostic, GLSL_VERIFY_TYPE,
                                  &emit->loc, replay);
        }
    }
    return 1;
}

static int GlslVerifyForPart(const GlslModule *module,
    GlslVerifyDiagnostic *diagnostic, const GlslStmt *stmt)
{
    for (; stmt != NULL; stmt = stmt->next) {
        if (!GlslVerifyLoc(diagnostic, &stmt->loc, stmt))
            return 0;
        if (stmt->kind != GLSL_STMT_EXPRESSION ||
            !GlslVerifyExpr(module, diagnostic, stmt->u.expression))
        {
            return GlslVerifyFail(diagnostic, GLSL_VERIFY_CONTROL,
                                  &stmt->loc, stmt);
        }
    }
    return 1;
}

static int GlslVerifyStmtList(const GlslModule *module,
    GlslVerifyDiagnostic *diagnostic, const GlslStmt *stmt,
    int loopDepth)
{
    for (; stmt != NULL; stmt = stmt->next) {
        if (!GlslVerifyLoc(diagnostic, &stmt->loc, stmt))
            return 0;
        switch (stmt->kind) {
        case GLSL_STMT_EXPRESSION:
            if (!GlslVerifyExpr(module, diagnostic, stmt->u.expression))
                return 0;
            break;
        case GLSL_STMT_IF:
            if (!GlslVerifyExpr(module, diagnostic,
                                stmt->u.ifStmt.condition) ||
                !GlslVerifyStmtList(module, diagnostic,
                    stmt->u.ifStmt.trueBranch, loopDepth) ||
                !GlslVerifyStmtList(module, diagnostic,
                    stmt->u.ifStmt.falseBranch, loopDepth)) return 0;
            break;
        case GLSL_STMT_WHILE:
        case GLSL_STMT_DO:
            if (!GlslVerifyExpr(module, diagnostic,
                                stmt->u.loop.condition) ||
                !GlslVerifyStmtList(module, diagnostic,
                    stmt->u.loop.body, loopDepth + 1)) return 0;
            break;
        case GLSL_STMT_FOR:
            if (!GlslVerifyForPart(module, diagnostic,
                                   stmt->u.forStmt.init) ||
                (stmt->u.forStmt.condition != NULL &&
                 !GlslVerifyExpr(module, diagnostic,
                                 stmt->u.forStmt.condition)) ||
                !GlslVerifyForPart(module, diagnostic,
                                   stmt->u.forStmt.step) ||
                !GlslVerifyStmtList(module, diagnostic,
                    stmt->u.forStmt.body, loopDepth + 1)) return 0;
            break;
        case GLSL_STMT_BLOCK:
            if (!GlslVerifyStmtList(module, diagnostic,
                                    stmt->u.block, loopDepth)) return 0;
            break;
        case GLSL_STMT_RETURN:
            if (stmt->u.returnExpr != NULL &&
                !GlslVerifyExpr(module, diagnostic,
                                stmt->u.returnExpr)) return 0;
            break;
        case GLSL_STMT_DISCARD:
            if (module->stage != GLSL_STAGE_FRAGMENT)
                return GlslVerifyFail(diagnostic, GLSL_VERIFY_CONTROL,
                                      &stmt->loc, stmt);
            break;
        case GLSL_STMT_BREAK:
        case GLSL_STMT_CONTINUE:
            if (loopDepth <= 0)
                return GlslVerifyFail(diagnostic, GLSL_VERIFY_CONTROL,
                                      &stmt->loc, stmt);
            break;
        case GLSL_STMT_GEOMETRY_EMIT:
            if (module->stage != GLSL_STAGE_GEOMETRY)
                return GlslVerifyFail(diagnostic, GLSL_VERIFY_CONTROL,
                                      &stmt->loc, stmt);
            if (!GlslVerifyAssignments(module, diagnostic,
                                        stmt->u.emit.assignments) ||
                !GlslVerifyReplay(module, diagnostic, stmt)) return 0;
            break;
        case GLSL_STMT_GEOMETRY_RESTART:
            if (module->stage != GLSL_STAGE_GEOMETRY)
                return GlslVerifyFail(diagnostic, GLSL_VERIFY_CONTROL,
                                      &stmt->loc, stmt);
            break;
        default:
            return GlslVerifyFail(diagnostic, GLSL_VERIFY_CONTROL,
                                  &stmt->loc, stmt);
        }
    }
    return 1;
}

static int GlslVerifyGeometry(const GlslModule *module,
    GlslVerifyDiagnostic *diagnostic)
{
    const GlslGeometryInfo *geometry;
    const GlslDecl *decl;
    int vertexCount;

    geometry = module->geometry;
    if (module->stage != GLSL_STAGE_GEOMETRY) {
        if (geometry != NULL)
            return GlslVerifyFail(diagnostic, GLSL_VERIFY_GEOMETRY,
                                  &geometry->inputLoc, geometry);
        return 1;
    }
    if (geometry == NULL)
        return GlslVerifyFail(diagnostic, GLSL_VERIFY_GEOMETRY,
                              &module->errorLoc, module);
    switch (geometry->inputTopology) {
    case GLSL_GEOMETRY_INPUT_POINTS: vertexCount = 1; break;
    case GLSL_GEOMETRY_INPUT_LINES: vertexCount = 2; break;
    case GLSL_GEOMETRY_INPUT_LINES_ADJACENCY: vertexCount = 4; break;
    case GLSL_GEOMETRY_INPUT_TRIANGLES: vertexCount = 3; break;
    case GLSL_GEOMETRY_INPUT_TRIANGLES_ADJACENCY: vertexCount = 6; break;
    default:
        return GlslVerifyFail(diagnostic, GLSL_VERIFY_GEOMETRY,
                              &geometry->inputLoc, geometry);
    }
    if (geometry->inputVertexCount != vertexCount)
        return GlslVerifyFail(diagnostic, GLSL_VERIFY_GEOMETRY,
                              &geometry->inputLoc, geometry);
    if (geometry->outputTopology < GLSL_GEOMETRY_OUTPUT_POINTS ||
        geometry->outputTopology >
            GLSL_GEOMETRY_OUTPUT_TRIANGLE_STRIP)
    {
        return GlslVerifyFail(diagnostic, GLSL_VERIFY_GEOMETRY,
                              &geometry->outputLoc, geometry);
    }
    if (geometry->maxOutputVertices <= 0)
        return GlslVerifyFail(diagnostic, GLSL_VERIFY_GEOMETRY,
                              &geometry->maxVerticesLoc, geometry);
    for (decl = module->globals; decl != NULL; decl = decl->next) {
        if (decl->storage == GLSL_STORAGE_INPUT &&
            (decl->type.elementType == NULL ||
             decl->type.arraySize != vertexCount))
        {
            return GlslVerifyFail(diagnostic, GLSL_VERIFY_INTERFACE,
                                  &decl->loc, decl);
        }
    }
    return 1;
}

static int lVerifyModule(const GlslModule *module,
    GlslVerifyDiagnostic *diagnostic)
{
    const GlslBinding *binding;
    const GlslDecl *decl;
    const GlslFunction *function;
    int entryCount;
    int samplerCount;
    int samplerDeclCount;

    if (module == NULL) {
        diagnostic->reason = GLSL_VERIFY_STAGE;
        return 0;
    }
    switch (module->stage) {
    case GLSL_STAGE_VERTEX:
    case GLSL_STAGE_FRAGMENT:
    case GLSL_STAGE_GEOMETRY:
        break;
    default:
        return GlslVerifyFail(diagnostic, GLSL_VERIFY_STAGE,
                              &module->errorLoc, module);
    }
    if (!GlslVerifyGeometry(module, diagnostic))
        return 0;
    if (module->errors != 0 || module->entry == NULL)
        return GlslVerifyFail(diagnostic, GLSL_VERIFY_DECLARATION,
                              &module->errorLoc, module);
    for (decl = module->structs; decl != NULL; decl = decl->next) {
        if (decl->name == NULL || decl->name[0] == '\0' ||
            !GlslVerifyLoc(diagnostic, &decl->loc, decl))
        {
            return GlslVerifyFail(diagnostic, GLSL_VERIFY_DECLARATION,
                                  &decl->loc, decl);
        }
        if (!GlslVerifyDeclList(diagnostic, decl->members, 0))
            return 0;
    }
    if (!GlslVerifyDeclList(diagnostic, module->globals, 1))
        return 0;
    samplerCount = 0;
    for (binding = module->bindings; binding != NULL;
         binding = binding->next)
    {
        if (!GlslVerifyLoc(diagnostic, &binding->loc, binding))
            return 0;
        if (binding->name == NULL || binding->semantic == NULL ||
            binding->storage < GLSL_STORAGE_CONST ||
            binding->storage > GLSL_STORAGE_BUILTIN ||
            binding->defaultCount < 0 ||
            (binding->defaultCount > 0 &&
             binding->defaultValues == NULL) ||
            (binding->declaration != NULL &&
             binding->storage != GLSL_STORAGE_BUILTIN &&
             !GlslVerifyDeclInList(module->globals,
                                   binding->declaration)) ||
            (binding->declaration != NULL &&
             binding->interpolation !=
                 binding->declaration->interpolation))
        {
            return GlslVerifyFail(diagnostic, GLSL_VERIFY_INTERFACE,
                                  &binding->loc, binding);
        }
        if (binding->storage == GLSL_STORAGE_SAMPLER) {
            int unit;
            int namedDeclCount;
            const GlslDecl *namedDecl;

            samplerCount++;
            namedDeclCount = 0;
            for (namedDecl = module->globals; namedDecl != NULL;
                 namedDecl = namedDecl->next)
            {
                if (namedDecl->storage == GLSL_STORAGE_SAMPLER &&
                    namedDecl->name != NULL &&
                    !strcmp(binding->name, namedDecl->name))
                {
                    namedDeclCount++;
                }
            }
            if ((module->stage != GLSL_STAGE_FRAGMENT &&
                 module->stage != GLSL_STAGE_GEOMETRY) ||
                samplerCount > 16 || binding->declaration == NULL ||
                binding->declaration->storage != GLSL_STORAGE_SAMPLER ||
                !GlslVerifySamplerType(&binding->declaration->type) ||
                strcmp(binding->name, binding->declaration->name) ||
                namedDeclCount != 1 ||
                !GlslParseSamplerUnit(binding->semantic, &unit) ||
                unit != samplerCount - 1 ||
                binding->interfaceKey != NULL || binding->isOutput ||
                binding->defaultCount != 0 ||
                binding->defaultValues != NULL)
            {
                return GlslVerifyFail(diagnostic,
                    GLSL_VERIFY_INTERFACE, &binding->loc, binding);
            }
        } else if (binding->declaration != NULL &&
                   GlslVerifyTypeContainsSampler(
                       &binding->declaration->type, 0))
        {
            return GlslVerifyFail(diagnostic, GLSL_VERIFY_TYPE,
                                  &binding->loc, binding);
        }
        if (binding->defaultCount > 0) {
            int i;

            for (i = 0; i < binding->defaultCount; i++) {
                float value;

                value = binding->defaultValues[i];
                if (value != value || value > FLT_MAX ||
                    value < -FLT_MAX)
                {
                    return GlslVerifyFail(diagnostic, GLSL_VERIFY_TYPE,
                                          &binding->loc, binding);
                }
            }
        }
    }
    samplerDeclCount = 0;
    for (decl = module->globals; decl != NULL; decl = decl->next) {
        if (decl->storage == GLSL_STORAGE_SAMPLER) {
            const GlslBinding *match;
            int matchCount;

            samplerDeclCount++;
            matchCount = 0;
            for (match = module->bindings; match != NULL;
                 match = match->next)
            {
                if (match->storage == GLSL_STORAGE_SAMPLER &&
                    match->declaration == decl)
                {
                    matchCount++;
                }
            }
            if (matchCount != 1)
                return GlslVerifyFail(diagnostic,
                    GLSL_VERIFY_INTERFACE, &decl->loc, decl);
        }
    }
    if (samplerDeclCount != samplerCount)
        return GlslVerifyFail(diagnostic, GLSL_VERIFY_INTERFACE,
                              &module->errorLoc, module);
    entryCount = 0;
    for (function = module->functions; function != NULL;
         function = function->next)
    {
        if (!GlslVerifyLoc(diagnostic, &function->loc, function) ||
            function->name == NULL ||
            !GlslVerifyType(diagnostic, &function->result,
                            &function->loc, function) ||
            GlslVerifyTypeContainsSampler(&function->result, 0) ||
            !GlslVerifyDeclList(diagnostic, function->parameters, 0) ||
            !GlslVerifyDeclList(diagnostic, function->locals, 0) ||
            !GlslVerifyStmtList(module, diagnostic,
                                function->body, 0))
        {
            return 0;
        }
        if (function->isEntry) {
            entryCount++;
            if (function != module->entry ||
                function->parameters != NULL ||
                strcmp(function->name, "main") ||
                function->result.base != GLSL_BASE_VOID ||
                function->result.len != 0)
            {
                return GlslVerifyFail(diagnostic,
                    GLSL_VERIFY_DECLARATION, &function->loc, function);
            }
        }
    }
    if (entryCount != 1)
        return GlslVerifyFail(diagnostic, GLSL_VERIFY_DECLARATION,
                              &module->errorLoc, module->entry);
    return 1;
}

int GlslVerifyModule(const GlslModule *module,
    GlslVerifyDiagnostic *diagnostic)
{
    if (diagnostic == NULL)
        return 0;
    diagnostic->reason = GLSL_VERIFY_OK;
    diagnostic->loc.file = 0;
    diagnostic->loc.line = 0;
    diagnostic->node = NULL;
    return lVerifyModule(module, diagnostic);
}

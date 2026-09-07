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
// glsl_lower_ir_stmt.c
//

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <float.h>
#include <limits.h>

#include "glsl_lower_internal.h"



static int GlslIRLowerStatement(GlslLowerContext *context,
                                const CgIRStmt *stmt, GlslStmt **list);
static int GlslIRBranch(GlslLowerContext *context, const CgIRStmt *branch,
                        GlslStmt **out);
static int GlslIRForPart(GlslLowerContext *context, const CgIRStmt *init,
                         GlslStmt **out);

static int GlslIRLowerStatement(GlslLowerContext *context,
                                const CgIRStmt *source, GlslStmt **list)
{
    GlslStmt *target;
    GlslExpr *condition;
    GlslType boolType;

    if (source == NULL)
        return 1;
    context->statementLoc = source->loc;
    switch (source->kind) {
    case CGIR_STMT_DECL:
        /* Declarations are consumed by the enclosing block walker;
         * reaching this point means an unexpected producer shape. */
        GlslRecordFailure(context, "GLSL profile declaration placement");
        return 0;
    case CGIR_STMT_EXPR:
        if (source->u.expression == NULL)
            return 1;
        /* Struct assignments flatten member-wise exactly as the legacy
         * FlattenStructAssignments pass arranged before tree lowering
         * saw them; targets that are themselves native aggregate
         * temporaries (the return rewrite's cg_return) stay whole. */
        if (source->u.expression->kind == CGIR_EXPR_ASSIGN &&
            source->u.expression->u.assign.op == CGIR_OP_ASSIGN &&
            source->u.expression->type != NULL &&
            IsStruct(source->u.expression->type) &&
            !(source->u.expression->u.assign.target != NULL &&
              source->u.expression->u.assign.target->kind ==
                  CGIR_EXPR_SYMBOL &&
              source->u.expression->u.assign.target->u.symbol != NULL &&
              (source->u.expression->u.assign.target->u.symbol->properties &
               SYMB_IS_NATIVE_AGGREGATE_TEMP)))
        {
            return GlslIRLowerAggregateAssign(context,
                                              source->u.expression, list);
        }
        target = GlslNewStmt(context->module, GLSL_STMT_EXPRESSION);
        if (target == NULL)
            return 0;
        target->u.expression = GlslIRLowerExpr(context,
                                               source->u.expression);
        if (target->u.expression == NULL)
            return 0;
        break;
    case CGIR_STMT_IF:
        target = GlslNewStmt(context->module, GLSL_STMT_IF);
        if (target == NULL)
            return 0;
        target->u.ifStmt.condition = GlslIRLowerExpr(
            context, source->u.ifStmt.condition);
        if (target->u.ifStmt.condition == NULL)
            return 0;
        if (!GlslIRBranch(context, source->u.ifStmt.trueBranch,
                          &target->u.ifStmt.trueBranch) ||
            !GlslIRBranch(context, source->u.ifStmt.falseBranch,
                          &target->u.ifStmt.falseBranch)) return 0;
        break;
    case CGIR_STMT_WHILE:
    case CGIR_STMT_DO:
        target = GlslNewStmt(context->module,
            source->kind == CGIR_STMT_WHILE ? GLSL_STMT_WHILE :
                                              GLSL_STMT_DO);
        if (target == NULL)
            return 0;
        target->u.loop.condition = GlslIRLowerExpr(
            context, source->u.loop.condition);
        if (target->u.loop.condition == NULL)
            return 0;
        context->loopDepth++;
        if (!GlslIRBranch(context, source->u.loop.body,
                          &target->u.loop.body))
        {
            context->loopDepth--;
            return 0;
        }
        context->loopDepth--;
        break;
    case CGIR_STMT_FOR:
        target = GlslNewStmt(context->module, GLSL_STMT_FOR);
        if (target == NULL)
            return 0;
        if (source->u.forStmt.init != NULL &&
            !GlslIRForPart(context, source->u.forStmt.init,
                           &target->u.forStmt.init))
            return 0;
        if (source->u.forStmt.condition != NULL) {
            target->u.forStmt.condition = GlslIRLowerExpr(
                context, source->u.forStmt.condition);
            if (target->u.forStmt.condition == NULL)
                return 0;
        }
        if (source->u.forStmt.step != NULL) {
            GlslStmt *stepStmt;

            stepStmt = GlslNewStmt(context->module,
                                   GLSL_STMT_EXPRESSION);
            if (stepStmt == NULL)
                return 0;
            stepStmt->u.expression = GlslIRLowerExpr(
                context, source->u.forStmt.step);
            if (stepStmt->u.expression == NULL)
                return 0;
            target->u.forStmt.step = stepStmt;
        }
        context->loopDepth++;
        if (!GlslIRBranch(context, source->u.forStmt.body,
                          &target->u.forStmt.body))
        {
            context->loopDepth--;
            return 0;
        }
        context->loopDepth--;
        break;
    case CGIR_STMT_BLOCK:
        target = GlslNewStmt(context->module, GLSL_STMT_BLOCK);
        if (target == NULL)
            return 0;
        if (!GlslIRBlockBody(context, source->u.block, &target->u.block))
            return 0;
        break;
    case CGIR_STMT_RETURN:
        target = GlslNewStmt(context->module, GLSL_STMT_RETURN);
        if (target == NULL)
            return 0;
        if (source->u.returnExpr != NULL) {
            target->u.returnExpr = GlslIRLowerExpr(
                context, source->u.returnExpr);
            if (target->u.returnExpr == NULL)
                return 0;
        }
        break;
    case CGIR_STMT_DISCARD:
        if (context->module->stage != GLSL_STAGE_FRAGMENT) {
            GlslRecordFailureKind(context, GLSL_ERROR_STAGE_OPERATION,
                                  "discard");
            return 0;
        }
        if (source->u.discard.condition == NULL) {
            target = GlslNewStmt(context->module, GLSL_STMT_DISCARD);
            if (target == NULL)
                return 0;
        } else {
            GlslStmt *discard;
            GlslExpr *reduction;

            target = GlslNewStmt(context->module, GLSL_STMT_IF);
            discard = GlslNewStmt(context->module, GLSL_STMT_DISCARD);
            if (target == NULL || discard == NULL)
                return 0;
            condition = GlslIRLowerExpr(context,
                                        source->u.discard.condition);
            if (condition == NULL)
                return 0;
            if (condition->type.base != GLSL_BASE_BOOL ||
                condition->type.rows != 0 || condition->type.cols != 0 ||
                condition->type.arraySize != 0 ||
                condition->type.structName != NULL ||
                condition->type.elementType != NULL ||
                condition->type.len < 1 || condition->type.len > 4)
            {
                GlslRecordFailureKind(context,
                                      GLSL_ERROR_UNSUPPORTED_TYPE,
                                      "discard condition type");
                return 0;
            }
            if (condition->type.len > 1) {
                boolType = GlslNumericType(GLSL_BASE_BOOL, 1);
                reduction = GlslNewExpr(context->module,
                                        GLSL_EXPR_CALL, boolType);
                if (reduction == NULL)
                    return 0;
                reduction->u.call.name = "any";
                reduction->u.call.arguments = condition;
                condition = reduction;
            }
            target->u.ifStmt.condition = condition;
            GlslSetLoc(&discard->loc, &source->loc);
            target->u.ifStmt.trueBranch = discard;
        }
        break;
    case CGIR_STMT_BREAK:
        if (context->loopDepth == 0) {
            GlslRecordFailure(context, "break outside loop");
            return 0;
        }
        target = GlslNewStmt(context->module, GLSL_STMT_BREAK);
        break;
    case CGIR_STMT_CONTINUE:
        if (context->loopDepth == 0) {
            GlslRecordFailure(context, "continue outside loop");
            return 0;
        }
        target = GlslNewStmt(context->module, GLSL_STMT_CONTINUE);
        break;
    case CGIR_STMT_GEOMETRY_EMIT:
        if (context->module->stage != GLSL_STAGE_GEOMETRY) {
            GlslRecordFailureKind(context, GLSL_ERROR_STAGE_OPERATION,
                                  "emitVertex");
            return 0;
        }
        target = GlslNewGeometryEmit(context->module,
            GlslIRGeometryAssignments(context, source->u.geometry.values),
            GlslIRGeometryFlatReplay(context));
        if (target == NULL || target->u.emit.assignments == NULL)
            return 0;
        break;
    case CGIR_STMT_GEOMETRY_RESTART:
        if (context->module->stage != GLSL_STAGE_GEOMETRY) {
            GlslRecordFailureKind(context, GLSL_ERROR_STAGE_OPERATION,
                                  "restartStrip");
            return 0;
        }
        target = GlslNewGeometryRestart(context->module);
        break;
    case CGIR_STMT_GEOMETRY_FLAT:
        if (context->module->stage != GLSL_STAGE_GEOMETRY) {
            GlslRecordFailureKind(context, GLSL_ERROR_STAGE_OPERATION,
                                  "flatAttrib");
            return 0;
        }
        target = GlslNewStmt(context->module, GLSL_STMT_BLOCK);
        if (target == NULL)
            return 0;
        target->u.block = GlslIRGeometryFlatAssignments(context,
            source->u.geometry.values);
        if (target->u.block == NULL)
            return 0;
        break;
    default:
        GlslRecordFailure(context, "GLSL profile statement");
        return 0;
    }
    if (target == NULL)
        return 0;
    GlslSetLoc(&target->loc, &source->loc);
    GlslAppendStmt(list, target);
    return 1;
} // GlslIRLowerStatement

/*
 * GlslIRBlockBody() - Walk one IR block: declarations become function
 *          locals (or feed the group-write reconstruction), everything
 *          else lowers statement by statement.
 */

int GlslIRBlockBody(GlslLowerContext *context,
                           const CgIRStmt *stmt, GlslStmt **out)
{
    while (stmt != NULL) {
        const CgIRStmt *next = stmt->next;
        int consumed;

        /* Group writes may start at a bare store run (pure case) or at
         * a synthesized "$" temporary declaration (hoisted case). */
        consumed = GlslIRTryGroupWrite(context, stmt, &next, out);
        if (consumed < 0)
            return 0;
        if (consumed > 0) {
            stmt = next;
            continue;
        }
        if (stmt->kind == CGIR_STMT_DECL) {
            if (!GlslIRLocalDeclaration(context, stmt, &next, out))
                return 0;
        } else {
            if (!GlslIRLowerStatement(context, stmt, out)) return 0;
        }
        stmt = next;
    }
    return 1;
} // GlslIRBlockBody

/*
 * GlslIRBranch() - An IR branch is one statement or a wrapped block;
 *          the GLSL branch slot takes the flat statement list.
 */

static int GlslIRBranch(GlslLowerContext *context, const CgIRStmt *branch,
                        GlslStmt **out)
{
    if (branch == NULL)
        return 1;
    if (branch->kind == CGIR_STMT_BLOCK)
        return GlslIRBlockBody(context, branch->u.block, out);
    return GlslIRLowerStatement(context, branch, out);
} // GlslIRBranch

/*
 * GlslIRForPart() - The for-header init keeps expression statements
 *      only, exactly as the legacy for-part walker demanded.
 */

static int GlslIRForPart(GlslLowerContext *context, const CgIRStmt *init,
                         GlslStmt **out)
{
    GlslStmt *target;

    if (init == NULL)
        return 1;
    if (init->kind == CGIR_STMT_BLOCK)
        init = init->u.block;
    for (; init != NULL; init = init->next) {
        if (init->kind != CGIR_STMT_EXPR ||
            init->u.expression == NULL)
        {
            GlslRecordFailure(context, "GLSL for expression");
            return 0;
        }
        target = GlslNewStmt(context->module, GLSL_STMT_EXPRESSION);
        if (target == NULL)
            return 0;
        target->u.expression = GlslIRLowerExpr(context,
                                               init->u.expression);
        if (target->u.expression == NULL)
            return 0;
        GlslSetLoc(&target->loc, &init->loc);
        GlslAppendStmt(out, target);
    }
    return 1;
} // GlslIRForPart

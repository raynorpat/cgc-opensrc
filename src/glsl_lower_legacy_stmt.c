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
// glsl_lower_legacy_stmt.c
//

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <float.h>
#include <limits.h>

#include "glsl_lower_internal.h"



static int GlslLowerMatrixAssignment(GlslLowerContext *context,
    expr *source, const SourceLoc *loc, GlslStmt **list)
{
    GlslMatrixSelectorHelper *helper;
    GlslExpr *leftMatrix;
    GlslExpr *rightMatrix;
    GlslExpr *rightValue;
    GlslExpr *left;
    GlslExpr *right;
    GlslExpr *assignment;
    GlslExpr *call;
    GlslStmt *statement;
    GlslType scalarType;
    int count;
    int rightCount;
    int mask;
    int i;

    count = GlslMatrixSelectorCount(source->bin.left);
    if (count <= 1)
        return 0;
    rightCount = GlslMatrixSelectorCount(source->bin.right);
    if (rightCount != 0 && rightCount != count)
        return 0;
    leftMatrix = GlslLowerExpr(context, source->bin.left->un.arg);
    if (leftMatrix == NULL)
        return 0;
    if (!source->bin.left->un.arg->common.HasSideEffects &&
        rightCount != 0 &&
        !source->bin.right->un.arg->common.HasSideEffects &&
        source->bin.left->un.arg->common.kind == SYMB_N &&
        source->bin.left->un.arg->sym.op == VARIABLE_OP &&
        source->bin.right->un.arg->common.kind == SYMB_N &&
        source->bin.right->un.arg->sym.op == VARIABLE_OP &&
        source->bin.left->un.arg->sym.symbol !=
            source->bin.right->un.arg->sym.symbol)
    {
        rightMatrix = GlslLowerExpr(context, source->bin.right->un.arg);
        if (rightMatrix == NULL)
            return 0;
        scalarType = GlslNumericType(GLSL_BASE_FLOAT, 1);
        for (i = 0; i < count; i++) {
            left = GlslMatrixSelectorComponent(context, leftMatrix,
                                                source->bin.left, i);
            right = GlslMatrixSelectorComponent(context, rightMatrix,
                                                 source->bin.right, i);
            assignment = GlslNewExpr(context->module, GLSL_EXPR_BINARY,
                                     scalarType);
            statement = GlslNewStmt(context->module,
                                    GLSL_STMT_EXPRESSION);
            if (left == NULL || right == NULL || assignment == NULL ||
                statement == NULL) return 0;
            assignment->u.binary.op = GLSL_OP_ASSIGN;
            assignment->u.binary.left = left;
            assignment->u.binary.right = right;
            statement->u.expression = assignment;
            GlslSetLoc(&statement->loc, loc);
            GlslAppendStmt(list, statement);
        }
        return 1;
    }
    rightValue = GlslLowerExpr(context, source->bin.right);
    if (rightValue == NULL)
        return 0;
    mask = SUBOP_GET_MASK16(source->bin.left->un.subop);
    helper = GlslGetMatrixSelectorHelper(context,
        GLSL_MATRIX_SELECTOR_SET, &leftMatrix->type, &rightValue->type,
        count, mask);
    if (helper == NULL)
        return 0;
    call = GlslNewExpr(context->module, GLSL_EXPR_CALL,
                       helper->function->result);
    statement = GlslNewStmt(context->module, GLSL_STMT_EXPRESSION);
    if (call == NULL || statement == NULL)
        return 0;
    call->u.call.name = helper->function->name;
    call->u.call.arguments = leftMatrix;
    leftMatrix->next = rightValue;
    statement->u.expression = call;
    GlslSetLoc(&statement->loc, loc);
    GlslAppendStmt(list, statement);
    return 1;
}

static int GlslLowerBranch(GlslLowerContext *context, stmt *source,
                           GlslStmt **list)
{
    if (source != NULL && source->commonst.next == NULL &&
        source->commonst.kind == BLOCK_STMT)
    {
        source = source->blockst.body;
    }
    return GlslLowerStatementList(context, source, list);
}

static int GlslLowerForPart(GlslLowerContext *context, stmt *source,
                            GlslStmt **list)
{
    GlslStmt *target;

    for (; source != NULL; source = source->commonst.next) {
        context->statementLoc = source->commonst.loc;
        if (source->commonst.kind != EXPR_STMT || source->exprst.exp == NULL) {
            GlslRecordFailure(context, "GLSL for expression");
            return 0;
        }
        target = GlslNewStmt(context->module, GLSL_STMT_EXPRESSION);
        if (target == NULL)
            return 0;
        GlslSetLoc(&target->loc, &source->commonst.loc);
        target->u.expression = GlslLowerExpr(context, source->exprst.exp);
        if (target->u.expression == NULL)
            return 0;
        GlslAppendStmt(list, target);
    }
    return 1;
}

int GlslLowerStatementList(GlslLowerContext *context, stmt *source,
                                  GlslStmt **list)
{
    GlslStmt *target;

    for (; source != NULL; source = source->commonst.next) {
        context->statementLoc = source->commonst.loc;
        if (source->commonst.kind == COMMENT_STMT)
            continue;
        switch (source->commonst.kind) {
        case EXPR_STMT:
            if (source->exprst.exp == NULL)
                continue;
            if (source->exprst.exp->common.kind == BINARY_N &&
                (source->exprst.exp->bin.op == ASSIGN_OP ||
                 source->exprst.exp->bin.op == ASSIGN_V_OP ||
                 source->exprst.exp->bin.op == ASSIGN_GEN_OP) &&
                GlslMatrixSelectorCount(
                    source->exprst.exp->bin.left) > 1)
            {
                if (!GlslLowerMatrixAssignment(context,
                                                source->exprst.exp,
                                                &source->commonst.loc,
                                                list)) return 0;
                continue;
            }
            target = GlslNewStmt(context->module, GLSL_STMT_EXPRESSION);
            if (target == NULL)
                return 0;
            target->u.expression = GlslLowerExpr(context, source->exprst.exp);
            if (target->u.expression == NULL)
                return 0;
            break;
        case IF_STMT:
            target = GlslNewStmt(context->module, GLSL_STMT_IF);
            if (target == NULL)
                return 0;
            target->u.ifStmt.condition = GlslLowerExpr(context,
                                                       source->ifst.cond);
            if (target->u.ifStmt.condition == NULL ||
                !GlslLowerBranch(context, source->ifst.thenstmt,
                                 &target->u.ifStmt.trueBranch) ||
                !GlslLowerBranch(context, source->ifst.elsestmt,
                                 &target->u.ifStmt.falseBranch)) return 0;
            break;
        case WHILE_STMT:
        case DO_STMT:
            target = GlslNewStmt(context->module,
                source->commonst.kind == WHILE_STMT ?
                GLSL_STMT_WHILE : GLSL_STMT_DO);
            if (target == NULL)
                return 0;
            target->u.loop.condition = GlslLowerExpr(context,
                                                     source->whilest.cond);
            if (target->u.loop.condition == NULL)
                return 0;
            context->loopDepth++;
            if (!GlslLowerBranch(context, source->whilest.body,
                                 &target->u.loop.body)) {
                context->loopDepth--;
                return 0;
            }
            context->loopDepth--;
            break;
        case FOR_STMT:
            target = GlslNewStmt(context->module, GLSL_STMT_FOR);
            if (target == NULL)
                return 0;
            if (!GlslLowerForPart(context, source->forst.init,
                                  &target->u.forStmt.init) ||
                (source->forst.cond != NULL &&
                 (target->u.forStmt.condition = GlslLowerExpr(
                    context, source->forst.cond)) == NULL) ||
                !GlslLowerForPart(context, source->forst.step,
                                  &target->u.forStmt.step)) return 0;
            context->loopDepth++;
            if (!GlslLowerBranch(context, source->forst.body,
                                 &target->u.forStmt.body)) {
                context->loopDepth--;
                return 0;
            }
            context->loopDepth--;
            break;
        case BLOCK_STMT:
            target = GlslNewStmt(context->module, GLSL_STMT_BLOCK);
            if (target == NULL)
                return 0;
            if (!GlslLowerStatementList(context, source->blockst.body,
                                        &target->u.block)) return 0;
            break;
        case RETURN_STMT:
            target = GlslNewStmt(context->module, GLSL_STMT_RETURN);
            if (target == NULL)
                return 0;
            if (source->returnst.exp != NULL) {
                target->u.returnExpr = GlslLowerExpr(context,
                                                     source->returnst.exp);
                if (target->u.returnExpr == NULL)
                    return 0;
            }
            break;
        case DISCARD_STMT:
            if (context->module->stage != GLSL_STAGE_FRAGMENT) {
                GlslRecordFailureKind(context, GLSL_ERROR_STAGE_OPERATION,
                                      "discard");
                return 0;
            }
            if (source->discardst.cond == NULL ||
                source->discardst.cond->common.kind != UNARY_N ||
                source->discardst.cond->un.op != KILL_OP)
            {
                GlslRecordFailure(context, "GLSL discard statement");
                return 0;
            }
            if (source->discardst.cond->un.arg == NULL) {
                target = GlslNewStmt(context->module, GLSL_STMT_DISCARD);
            } else {
                GlslExpr *condition;
                GlslExpr *reduction;
                GlslStmt *discard;
                GlslType boolType;

                target = GlslNewStmt(context->module, GLSL_STMT_IF);
                discard = GlslNewStmt(context->module, GLSL_STMT_DISCARD);
                if (target == NULL || discard == NULL)
                    return 0;
                condition = GlslLowerExpr(
                    context, source->discardst.cond->un.arg);
                if (condition == NULL ||
                    condition->type.base != GLSL_BASE_BOOL ||
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
                GlslSetLoc(&discard->loc, &source->commonst.loc);
                target->u.ifStmt.trueBranch = discard;
            }
            break;
        case BREAK_STMT:
            if (context->loopDepth == 0) {
                GlslRecordFailure(context, "break outside loop");
                return 0;
            }
            target = GlslNewStmt(context->module, GLSL_STMT_BREAK);
            break;
        case CONTINUE_STMT:
            if (context->loopDepth == 0) {
                GlslRecordFailure(context, "continue outside loop");
                return 0;
            }
            target = GlslNewStmt(context->module, GLSL_STMT_CONTINUE);
            break;
        default:
            GlslRecordFailure(context, "GLSL profile statement");
            return 0;
        }
        if (target == NULL)
            return 0;
        GlslSetLoc(&target->loc, &source->commonst.loc);
        GlslAppendStmt(list, target);
    }
    return 1;
}

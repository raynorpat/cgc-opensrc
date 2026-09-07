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
license, under NVIDIA's copyrights in this original NVIDIA software,
to use, reproduce, modify and redistribute the NVIDIA Software, with or
without modifications, in source and/or binary forms; provided that if
you redistribute the NVIDIA Software, you must retain the copyright
notice of NVIDIA, this notice and the following text and disclaimers in
all such redistributions of the NVIDIA Software.  Neither the name,
trademarks, service marks nor logos of NVIDIA Corporation may be used
to endorse or promote products derived from the NVIDIA Software without
specific prior written permission from NVIDIA.  Except as expressly
stated in this notice, no other rights or licenses express or implied,
are granted by NVIDIA herein, including but not limited to any patent
rights that may be infringed by your derivative works or by other works
in which the NVIDIA Software may be incorporated. No hardware is
licensed hereunder.

THE NVIDIA SOFTWARE IS BEING PROVIDED ON AN "AS IS" BASIS, WITHOUT
WARRANTIES OR CONDITIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED,
INCLUDING WITHOUT LIMITATION, WARRANTIES OR CONDITIONS OF TITLE,
NON-INFRINGEMENT, MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE,
OR ITS USE AND OPERATION EITHER ALONE OR IN COMBINATION WITH OTHER
PRODUCTS.

IN NO EVENT SHALL NVIDIA BE LIABLE FOR ANY SPECIAL, INDIRECT,
INCIDENTAL, EXEMPLARY, CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, LOST PROFITS; PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) ARISING IN
ANY WAY OUT OF THE USE, REPRODUCTION, MODIFICATION AND/OR DISTRIBUTION
OF THE NVIDIA SOFTWARE, HOWEVER CAUSED AND WHETHER UNDER THEORY OF
CONTRACT, TORT (INCLUDING NEGLIGENCE), STRICT LIABILITY OR OTHERWISE,
EVEN IF NVIDIA HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
\****************************************************************************/

#include <float.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>

#include "hlsl_lower_internal.h"

static int HlslIsSyntheticInputAssignment(expr *source)
{
    expr *right;

    if (source == NULL || source->common.kind != BINARY_N ||
        (source->bin.op != ASSIGN_OP && source->bin.op != ASSIGN_V_OP &&
         source->bin.op != ASSIGN_GEN_OP))
    {
        return 0;
    }
    right = source->bin.right;
    while (right != NULL && right->common.kind == UNARY_N &&
           (right->un.op == CAST_CS_OP || right->un.op == CAST_CV_OP ||
            right->un.op == CAST_SHAPE_OP))
    {
        right = right->un.arg;
    }
    return right != NULL && right->common.kind == BINARY_N &&
           right->bin.op == MEMBER_SELECTOR_OP &&
           right->bin.left != NULL &&
           right->bin.left->common.kind == SYMB_N &&
           right->bin.left->sym.symbol == Cg->theHAL->varyingIn;
} // HlslIsSyntheticInputAssignment

static HlslExpr *HlslNewUnaryExpr(HlslLowerContext *context,
                                  HlslOperator op, HlslExpr *operand)
{
    HlslExpr *expression;

    if (operand == NULL)
        return NULL;
    expression = HlslNewSourceExpr(context, HLSL_EXPR_UNARY,
                                   operand->type);
    if (expression != NULL) {
        expression->u.unary.op = op;
        expression->u.unary.operand = operand;
        expression->hasSideEffects = operand->hasSideEffects;
    }
    return expression;
} // HlslNewUnaryExpr

static HlslStmt *HlslNewBreakGuard(HlslLowerContext *context,
                                   HlslExpr *condition)
{
    HlslStmt *guard;
    HlslStmt *jump;

    guard = HlslNewStmt(context->module, HLSL_STMT_IF);
    jump = HlslNewStmt(context->module, HLSL_STMT_BREAK);
    if (guard == NULL || jump == NULL)
        return NULL;
    guard->u.ifStmt.condition = HlslNewUnaryExpr(
        context, HLSL_OP_LOGICAL_NOT, condition);
    if (guard->u.ifStmt.condition == NULL)
        return NULL;
    guard->u.ifStmt.trueBranch = jump;
    HlslSetLoc(&guard->loc, &context->statementLoc);
    HlslSetLoc(&jump->loc, &context->statementLoc);
    return guard;
} // HlslNewBreakGuard

HlslStmt *HlslNewBoolAssignment(HlslLowerContext *context,
                                       HlslDecl *decl, int value)
{
    HlslExpr *left;
    HlslExpr *right;
    HlslExpr *assignment;

    if (decl == NULL)
        return NULL;
    left = HlslNewSymbolExpr(context, decl);
    right = HlslNewLiteral(context, HLSL_BASE_BOOL, value, 0.0f);
    assignment = HlslNewAssignment(context, left, right);
    return HlslNewExpressionStmt(context, assignment);
} // HlslNewBoolAssignment

static int HlslStatementsAreForParts(const HlslStmt *statement)
{
    for (; statement != NULL; statement = statement->next) {
        if (statement->kind != HLSL_STMT_EXPRESSION)
            return 0;
    }
    return 1;
} // HlslStatementsAreForParts

static int HlslRewriteLoopBreaks(HlslLowerContext *context,
                                 HlslStmt *statement, HlslDecl *flag,
                                 int nestedLoopDepth)
{
    HlslStmt *assignment;
    HlslStmt *jump;

    for (; statement != NULL; statement = statement->next) {
        if (statement->kind == HLSL_STMT_BREAK && nestedLoopDepth == 0) {
            assignment = HlslNewBoolAssignment(context, flag, 1);
            jump = HlslNewStmt(context->module, HLSL_STMT_BREAK);
            if (assignment == NULL || jump == NULL)
                return 0;
            HlslSetLoc(&jump->loc, &context->statementLoc);
            assignment->next = jump;
            statement->kind = HLSL_STMT_BLOCK;
            statement->u.block = assignment;
        } else if (statement->kind == HLSL_STMT_IF) {
            if (!HlslRewriteLoopBreaks(context,
                    statement->u.ifStmt.trueBranch, flag,
                    nestedLoopDepth) ||
                !HlslRewriteLoopBreaks(context,
                    statement->u.ifStmt.falseBranch, flag,
                    nestedLoopDepth))
            {
                return 0;
            }
        } else if (statement->kind == HLSL_STMT_BLOCK) {
            if (!HlslRewriteLoopBreaks(context, statement->u.block,
                                       flag, nestedLoopDepth))
            {
                return 0;
            }
        } else if (statement->kind == HLSL_STMT_WHILE ||
                   statement->kind == HLSL_STMT_DO)
        {
            if (!HlslRewriteLoopBreaks(context, statement->u.loop.body,
                                       flag, nestedLoopDepth + 1))
            {
                return 0;
            }
        } else if (statement->kind == HLSL_STMT_FOR) {
            if (!HlslRewriteLoopBreaks(context,
                    statement->u.forStmt.body, flag,
                    nestedLoopDepth + 1))
            {
                return 0;
            }
        }
    }
    return 1;
} // HlslRewriteLoopBreaks

static int HlslAppendIRInitializerCopy(HlslLowerContext *context,
                                       const HlslType *type,
                                       HlslExpr *target,
                                       HlslExpr *source,
                                       HlslStmt **list)
{
    HlslDecl *member;
    HlslExpr *item;
    int i;

    if (context == NULL || type == NULL || target == NULL ||
        source == NULL || list == NULL)
    {
        return 0;
    }
    if (type->arraySize > 0 && source->kind == HLSL_EXPR_CONSTRUCT) {
        item = source->u.construct.arguments;
        for (i = 0; i < type->arraySize; i++) {
            if (item == NULL ||
                !HlslIRTypeIsIdentical(type->elementType, &item->type) ||
                !HlslAppendIRInitializerCopy(context, type->elementType,
                    HlslCopyIndex(context, target, type->elementType, i),
                    item, list))
            {
                return 0;
            }
            item = item->next;
        }
        return item == NULL;
    }
    if (type->base == HLSL_BASE_STRUCT &&
        source->kind == HLSL_EXPR_CONSTRUCT)
    {
        item = source->u.construct.arguments;
        for (member = type->members; member != NULL;
             member = member->next)
        {
            if (item == NULL ||
                !HlslIRTypeIsIdentical(&member->type, &item->type) ||
                !HlslAppendIRInitializerCopy(context, &member->type,
                    HlslCopyMember(context, target, member), item, list))
            {
                return 0;
            }
            item = item->next;
        }
        return item == NULL;
    }
    if (HlslTypeNeedsRecursiveCopy(type)) {
        if (!HlslIsStableAggregateSource(source)) {
            source = HlslCaptureValue(context, list, source);
            if (source == NULL)
                return 0;
        }
        return HlslAppendRecursiveCopy(context, type, target, source, list);
    }
    source = HlslNewAssignment(context, target, source);
    return source != NULL && HlslAppendExpression(context, list, source);
} // HlslAppendIRInitializerCopy

static int HlslLowerIRDeclarationInitializer(
    HlslLowerContext *context, const CgIRDecl *source, HlslStmt **list)
{
    HlslDecl *declaration;
    HlslExpr *left;
    HlslExpr *right;

    if (context == NULL || source == NULL || source->initializer == NULL ||
        list == NULL)
    {
        return 0;
    }
    declaration = HlslFindDecl(context, source->symbol);
    right = HlslLowerIRExpr(context, source->initializer, list,
                            HLSL_VALUE_RVALUE);
    left = declaration != NULL ?
        HlslNewSymbolExpr(context, declaration) : NULL;
    if (left == NULL || right == NULL)
        return 0;
    return HlslAppendIRInitializerCopy(context, &declaration->type,
                                       left, right, list);
} // HlslLowerIRDeclarationInitializer

int HlslLowerStatements(HlslLowerContext *context, stmt *source,
                               HlslStmt **list)
{
    HlslStmt *target;
    HlslStmt *discard;
    HlslStmt *conditionPrefix;
    HlslStmt *stepStatements;
    HlslStmt *body;
    HlslStmt *guard;
    HlslStmt *firstTest;
    HlslStmt *firstAssignment;
    HlslStmt *breakAssignment;
    HlslStmt *wrapper;
    HlslStmt *breakTest;
    HlslStmt *jump;
    HlslExpr *condition;
    HlslExpr *loopCondition;
    HlslExpr *flagExpr;
    HlslDecl *flag;
    HlslType boolType;
    expr *discardCondition;

    for (; source != NULL; source = source->commonst.next) {
        context->statementLoc = source->commonst.loc;
        if (source->commonst.kind == COMMENT_STMT)
            continue;
        if (source->commonst.kind == EXPR_STMT &&
            HlslIsSyntheticInputAssignment(source->exprst.exp))
        {
            continue;
        }
        if (source->commonst.kind == EXPR_STMT) {
            if (source->exprst.exp == NULL)
                continue;
            target = HlslNewStmt(context->module,
                                 HLSL_STMT_EXPRESSION);
            if (target != NULL)
                target->u.expression = HlslLowerExpr(
                    context, source->exprst.exp, list, 0);
            if (target == NULL || target->u.expression == NULL)
                return 0;
        } else if (source->commonst.kind == IF_STMT) {
            target = HlslNewStmt(context->module, HLSL_STMT_IF);
            if (target == NULL)
                return 0;
            target->u.ifStmt.condition = HlslLowerExpr(
                context, source->ifst.cond, list, 1);
            if (target->u.ifStmt.condition == NULL ||
                !HlslLowerStatements(context, source->ifst.thenstmt,
                                     &target->u.ifStmt.trueBranch) ||
                !HlslLowerStatements(context, source->ifst.elsestmt,
                                     &target->u.ifStmt.falseBranch))
            {
                return 0;
            }
        } else if (source->commonst.kind == WHILE_STMT ||
                   source->commonst.kind == DO_STMT)
        {
            conditionPrefix = NULL;
            body = NULL;
            target = HlslNewStmt(context->module,
                source->commonst.kind == WHILE_STMT ?
                HLSL_STMT_WHILE : HLSL_STMT_DO);
            if (target == NULL)
                return 0;
            loopCondition = HlslLowerExpr(
                context, source->whilest.cond, &conditionPrefix, 1);
            if (loopCondition == NULL)
                return 0;
            context->loopDepth++;
            if (!HlslLowerStatements(context, source->whilest.body,
                                     &body))
            {
                context->loopDepth--;
                return 0;
            }
            context->loopDepth--;
            context->statementLoc = source->commonst.loc;
            if (conditionPrefix == NULL) {
                target->u.loop.condition = loopCondition;
                target->u.loop.body = body;
            } else if (source->commonst.kind == WHILE_STMT) {
                target->u.loop.condition = HlslNewLiteral(
                    context, HLSL_BASE_BOOL, 1, 0.0f);
                guard = HlslNewBreakGuard(context, loopCondition);
                if (target->u.loop.condition == NULL || guard == NULL)
                    return 0;
                target->u.loop.body = conditionPrefix;
                HlslAppendStmt(&target->u.loop.body, guard);
                HlslAppendStmt(&target->u.loop.body, body);
            } else {
                boolType = HlslNumericType(HLSL_BASE_BOOL, 1);
                flag = HlslNewTemporary(context, &boolType);
                firstAssignment = HlslNewBoolAssignment(context, flag, 1);
                firstTest = HlslNewStmt(context->module, HLSL_STMT_IF);
                target->kind = HLSL_STMT_WHILE;
                target->u.loop.condition = HlslNewLiteral(
                    context, HLSL_BASE_BOOL, 1, 0.0f);
                guard = HlslNewBreakGuard(context, loopCondition);
                flagExpr = flag != NULL ?
                    HlslNewSymbolExpr(context, flag) : NULL;
                if (firstAssignment == NULL || firstTest == NULL ||
                    target->u.loop.condition == NULL || guard == NULL ||
                    flagExpr == NULL)
                {
                    return 0;
                }
                firstTest->u.ifStmt.condition = HlslNewUnaryExpr(
                    context, HLSL_OP_LOGICAL_NOT, flagExpr);
                if (firstTest->u.ifStmt.condition == NULL)
                    return 0;
                firstTest->u.ifStmt.trueBranch = conditionPrefix;
                HlslAppendStmt(&firstTest->u.ifStmt.trueBranch, guard);
                target->u.loop.body = firstTest;
                breakAssignment = HlslNewBoolAssignment(context, flag, 0);
                if (breakAssignment == NULL)
                    return 0;
                HlslAppendStmt(&target->u.loop.body, breakAssignment);
                HlslAppendStmt(&target->u.loop.body, body);
                HlslSetLoc(&firstTest->loc, &source->commonst.loc);
                HlslSetLoc(&firstAssignment->loc, &source->commonst.loc);
                HlslAppendStmt(list, firstAssignment);
            }
        } else if (source->commonst.kind == FOR_STMT) {
            conditionPrefix = NULL;
            stepStatements = NULL;
            body = NULL;
            target = HlslNewStmt(context->module, HLSL_STMT_FOR);
            if (target == NULL ||
                !HlslLowerStatements(context, source->forst.init,
                                     &target->u.forStmt.init) ||
                (source->forst.cond != NULL &&
                 (loopCondition = HlslLowerExpr(
                    context, source->forst.cond,
                    &conditionPrefix, 1)) == NULL) ||
                !HlslLowerStatements(context, source->forst.step,
                                     &stepStatements))
            {
                return 0;
            }
            context->loopDepth++;
            if (!HlslLowerStatements(context, source->forst.body,
                                     &body))
            {
                context->loopDepth--;
                return 0;
            }
            context->loopDepth--;
            context->statementLoc = source->commonst.loc;
            if (!HlslStatementsAreForParts(target->u.forStmt.init)) {
                HlslAppendStmt(list, target->u.forStmt.init);
                target->u.forStmt.init = NULL;
            }
            if (conditionPrefix == NULL) {
                target->u.forStmt.condition = source->forst.cond != NULL ?
                    loopCondition : NULL;
            } else {
                guard = HlslNewBreakGuard(context, loopCondition);
                if (guard == NULL)
                    return 0;
                HlslAppendStmt(&conditionPrefix, guard);
            }
            if (HlslStatementsAreForParts(stepStatements)) {
                target->u.forStmt.step = stepStatements;
                target->u.forStmt.body = conditionPrefix;
                HlslAppendStmt(&target->u.forStmt.body, body);
            } else {
                boolType = HlslNumericType(HLSL_BASE_BOOL, 1);
                flag = HlslNewTemporary(context, &boolType);
                breakAssignment = HlslNewBoolAssignment(context, flag, 0);
                wrapper = HlslNewStmt(context->module, HLSL_STMT_DO);
                breakTest = HlslNewStmt(context->module, HLSL_STMT_IF);
                jump = HlslNewStmt(context->module, HLSL_STMT_BREAK);
                flagExpr = flag != NULL ?
                    HlslNewSymbolExpr(context, flag) : NULL;
                if (flag == NULL || breakAssignment == NULL ||
                    wrapper == NULL || breakTest == NULL || jump == NULL ||
                    flagExpr == NULL ||
                    !HlslRewriteLoopBreaks(context, body, flag, 0))
                {
                    return 0;
                }
                wrapper->u.loop.condition = HlslNewLiteral(
                    context, HLSL_BASE_BOOL, 0, 0.0f);
                if (wrapper->u.loop.condition == NULL)
                    return 0;
                wrapper->u.loop.body = body;
                breakTest->u.ifStmt.condition = flagExpr;
                breakTest->u.ifStmt.trueBranch = jump;
                target->u.forStmt.body = conditionPrefix;
                HlslAppendStmt(&target->u.forStmt.body, breakAssignment);
                HlslAppendStmt(&target->u.forStmt.body, wrapper);
                HlslAppendStmt(&target->u.forStmt.body, breakTest);
                HlslAppendStmt(&target->u.forStmt.body, stepStatements);
                HlslSetLoc(&wrapper->loc, &source->commonst.loc);
                HlslSetLoc(&breakTest->loc, &source->commonst.loc);
                HlslSetLoc(&jump->loc, &source->commonst.loc);
            }
        } else if (source->commonst.kind == RETURN_STMT) {
            target = HlslNewStmt(context->module, HLSL_STMT_RETURN);
            if (target != NULL && source->returnst.exp != NULL)
                target->u.returnExpr = HlslLowerExpr(
                    context, source->returnst.exp, list, 1);
            if (target == NULL || (source->returnst.exp != NULL &&
                                   target->u.returnExpr == NULL))
            {
                return 0;
            }
        } else if (source->commonst.kind == DISCARD_STMT) {
            if (context->module->stage != HLSL_STAGE_PIXEL)
                return HlslLowerFailure(context, HLSL_ERROR_STAGE_OPERATION,
                                        "discard", NULL);
            discardCondition = source->discardst.cond;
            if (discardCondition != NULL &&
                discardCondition->common.kind == UNARY_N &&
                discardCondition->un.op == KILL_OP)
            {
                discardCondition = discardCondition->un.arg;
            }
            if (discardCondition == NULL) {
                target = HlslNewStmt(context->module, HLSL_STMT_DISCARD);
            } else {
                condition = HlslLowerExpr(context, discardCondition,
                                          list, 1);
                if (condition != NULL)
                    condition = HlslScalarizeVectorCondition(
                        context, list, condition);
                discard = HlslNewStmt(context->module,
                                      HLSL_STMT_DISCARD);
                target = HlslNewStmt(context->module, HLSL_STMT_IF);
                if (condition == NULL || discard == NULL || target == NULL)
                    return 0;
                target->u.ifStmt.condition = condition;
                target->u.ifStmt.trueBranch = discard;
                HlslSetLoc(&discard->loc, &source->commonst.loc);
            }
        } else if (source->commonst.kind == BREAK_STMT) {
            if (context->loopDepth == 0)
                return HlslLowerFailure(context,
                    HLSL_ERROR_UNSUPPORTED_OPERATION,
                    "break outside loop", NULL);
            target = HlslNewStmt(context->module, HLSL_STMT_BREAK);
        } else if (source->commonst.kind == CONTINUE_STMT) {
            if (context->loopDepth == 0)
                return HlslLowerFailure(context,
                    HLSL_ERROR_UNSUPPORTED_OPERATION,
                    "continue outside loop", NULL);
            target = HlslNewStmt(context->module, HLSL_STMT_CONTINUE);
        } else if (source->commonst.kind == BLOCK_STMT) {
            target = HlslNewStmt(context->module, HLSL_STMT_BLOCK);
            if (target == NULL ||
                !HlslLowerStatements(context, source->blockst.body,
                                     &target->u.block))
            {
                return 0;
            }
        } else {
            return HlslLowerFailure(context,
                                    HLSL_ERROR_UNSUPPORTED_OPERATION,
                                    "HLSL statement", NULL);
        }
        HlslSetLoc(&target->loc, &source->commonst.loc);
        HlslAppendStmt(list, target);
    }
    return 1;
} // HlslLowerStatements

int HlslLowerIRStatements(HlslLowerContext *context,
                                 const CgIRStmt *source,
                                 HlslStmt **list)
{
    HlslStmt *target;
    HlslStmt *step;
    HlslStmt *conditionPrefix;
    HlslStmt *stepStatements;
    HlslStmt *body;
    HlslStmt *guard;
    HlslStmt *firstTest;
    HlslStmt *firstAssignment;
    HlslStmt *breakAssignment;
    HlslStmt *wrapper;
    HlslStmt *breakTest;
    HlslStmt *jump;
    HlslExpr *condition;
    HlslExpr *flagExpr;
    HlslDecl *flag;
    HlslType boolType;

    for (; source != NULL; source = source->next) {
        context->statementLoc = source->loc;
        target = NULL;
        switch (source->kind) {
        case CGIR_STMT_DECL:
            if (source->u.decl != NULL &&
                source->u.decl->initializer != NULL &&
                !HlslLowerIRDeclarationInitializer(context,
                    source->u.decl, list))
            {
                return 0;
            }
            continue;
        case CGIR_STMT_EXPR:
            target = HlslNewStmt(context->module, HLSL_STMT_EXPRESSION);
            if (target != NULL)
                target->u.expression = HlslLowerIRExpr(
                    context, source->u.expression, list,
                    HLSL_VALUE_DISCARD);
            if (target == NULL || target->u.expression == NULL)
                return 0;
            break;
        case CGIR_STMT_IF:
            target = HlslNewStmt(context->module, HLSL_STMT_IF);
            if (target != NULL)
                target->u.ifStmt.condition = HlslLowerIRExpr(context,
                    source->u.ifStmt.condition, list,
                    HLSL_VALUE_RVALUE);
            if (target == NULL || target->u.ifStmt.condition == NULL ||
                !HlslLowerIRStatements(context,
                    source->u.ifStmt.trueBranch,
                    &target->u.ifStmt.trueBranch) ||
                !HlslLowerIRStatements(context,
                    source->u.ifStmt.falseBranch,
                    &target->u.ifStmt.falseBranch))
            {
                return 0;
            }
            break;
        case CGIR_STMT_WHILE:
        case CGIR_STMT_DO:
            conditionPrefix = NULL;
            body = NULL;
            target = HlslNewStmt(context->module,
                source->kind == CGIR_STMT_WHILE ? HLSL_STMT_WHILE :
                                                  HLSL_STMT_DO);
            if (target != NULL)
                condition = HlslLowerIRExpr(context,
                    source->u.loop.condition, &conditionPrefix,
                    HLSL_VALUE_RVALUE);
            if (target == NULL || condition == NULL)
                return 0;
            context->loopDepth++;
            if (!HlslLowerIRStatements(context, source->u.loop.body,
                                        &body))
            {
                context->loopDepth--;
                return 0;
            }
            context->loopDepth--;
            if (conditionPrefix == NULL) {
                target->u.loop.condition = condition;
                target->u.loop.body = body;
            } else if (source->kind == CGIR_STMT_WHILE) {
                target->u.loop.condition = HlslNewLiteral(
                    context, HLSL_BASE_BOOL, 1, 0.0f);
                guard = HlslNewBreakGuard(context, condition);
                if (target->u.loop.condition == NULL || guard == NULL)
                    return 0;
                target->u.loop.body = conditionPrefix;
                HlslAppendStmt(&target->u.loop.body, guard);
                HlslAppendStmt(&target->u.loop.body, body);
            } else {
                boolType = HlslNumericType(HLSL_BASE_BOOL, 1);
                flag = HlslNewTemporary(context, &boolType);
                firstAssignment = HlslNewBoolAssignment(context, flag, 1);
                firstTest = HlslNewStmt(context->module, HLSL_STMT_IF);
                target->kind = HLSL_STMT_WHILE;
                target->u.loop.condition = HlslNewLiteral(
                    context, HLSL_BASE_BOOL, 1, 0.0f);
                guard = HlslNewBreakGuard(context, condition);
                flagExpr = flag != NULL ?
                    HlslNewSymbolExpr(context, flag) : NULL;
                if (firstAssignment == NULL || firstTest == NULL ||
                    target->u.loop.condition == NULL || guard == NULL ||
                    flagExpr == NULL)
                {
                    return 0;
                }
                firstTest->u.ifStmt.condition = HlslNewUnaryExpr(
                    context, HLSL_OP_LOGICAL_NOT, flagExpr);
                if (firstTest->u.ifStmt.condition == NULL)
                    return 0;
                firstTest->u.ifStmt.trueBranch = conditionPrefix;
                HlslAppendStmt(&firstTest->u.ifStmt.trueBranch, guard);
                target->u.loop.body = firstTest;
                breakAssignment = HlslNewBoolAssignment(context, flag, 0);
                if (breakAssignment == NULL)
                    return 0;
                HlslAppendStmt(&target->u.loop.body, breakAssignment);
                HlslAppendStmt(&target->u.loop.body, body);
                HlslSetLoc(&firstTest->loc, &source->loc);
                HlslSetLoc(&firstAssignment->loc, &source->loc);
                HlslAppendStmt(list, firstAssignment);
            }
            break;
        case CGIR_STMT_FOR:
            conditionPrefix = NULL;
            stepStatements = NULL;
            body = NULL;
            target = HlslNewStmt(context->module, HLSL_STMT_FOR);
            if (target == NULL ||
                !HlslLowerIRStatements(context, source->u.forStmt.init,
                                        &target->u.forStmt.init))
            {
                return 0;
            }
            if (source->u.forStmt.condition != NULL) {
                condition = HlslLowerIRExpr(context,
                    source->u.forStmt.condition, &conditionPrefix,
                    HLSL_VALUE_RVALUE);
                if (condition == NULL)
                    return 0;
            }
            if (source->u.forStmt.step != NULL) {
                step = HlslNewStmt(context->module,
                                   HLSL_STMT_EXPRESSION);
                if (step != NULL)
                    step->u.expression = HlslLowerIRExpr(context,
                        source->u.forStmt.step, &stepStatements,
                        HLSL_VALUE_DISCARD);
                if (step == NULL || step->u.expression == NULL)
                    return 0;
                HlslAppendStmt(&stepStatements, step);
            }
            context->loopDepth++;
            if (!HlslLowerIRStatements(context, source->u.forStmt.body,
                                        &body))
            {
                context->loopDepth--;
                return 0;
            }
            context->loopDepth--;
            if (!HlslStatementsAreForParts(target->u.forStmt.init)) {
                HlslAppendStmt(list, target->u.forStmt.init);
                target->u.forStmt.init = NULL;
            }
            if (conditionPrefix == NULL) {
                target->u.forStmt.condition =
                    source->u.forStmt.condition != NULL ? condition : NULL;
            } else {
                guard = HlslNewBreakGuard(context, condition);
                if (guard == NULL)
                    return 0;
                HlslAppendStmt(&conditionPrefix, guard);
            }
            if (HlslStatementsAreForParts(stepStatements)) {
                target->u.forStmt.step = stepStatements;
                target->u.forStmt.body = conditionPrefix;
                HlslAppendStmt(&target->u.forStmt.body, body);
            } else {
                boolType = HlslNumericType(HLSL_BASE_BOOL, 1);
                flag = HlslNewTemporary(context, &boolType);
                breakAssignment = HlslNewBoolAssignment(context, flag, 0);
                wrapper = HlslNewStmt(context->module, HLSL_STMT_DO);
                breakTest = HlslNewStmt(context->module, HLSL_STMT_IF);
                jump = HlslNewStmt(context->module, HLSL_STMT_BREAK);
                flagExpr = flag != NULL ?
                    HlslNewSymbolExpr(context, flag) : NULL;
                if (flag == NULL || breakAssignment == NULL ||
                    wrapper == NULL || breakTest == NULL || jump == NULL ||
                    flagExpr == NULL ||
                    !HlslRewriteLoopBreaks(context, body, flag, 0))
                {
                    return 0;
                }
                wrapper->u.loop.condition = HlslNewLiteral(
                    context, HLSL_BASE_BOOL, 0, 0.0f);
                if (wrapper->u.loop.condition == NULL)
                    return 0;
                wrapper->u.loop.body = body;
                breakTest->u.ifStmt.condition = flagExpr;
                breakTest->u.ifStmt.trueBranch = jump;
                target->u.forStmt.body = conditionPrefix;
                HlslAppendStmt(&target->u.forStmt.body, breakAssignment);
                HlslAppendStmt(&target->u.forStmt.body, wrapper);
                HlslAppendStmt(&target->u.forStmt.body, breakTest);
                HlslAppendStmt(&target->u.forStmt.body, stepStatements);
                HlslSetLoc(&wrapper->loc, &source->loc);
                HlslSetLoc(&breakTest->loc, &source->loc);
                HlslSetLoc(&jump->loc, &source->loc);
            }
            break;
        case CGIR_STMT_BLOCK:
            target = HlslNewStmt(context->module, HLSL_STMT_BLOCK);
            if (target == NULL ||
                !HlslLowerIRStatements(context, source->u.block,
                                        &target->u.block))
            {
                return 0;
            }
            break;
        case CGIR_STMT_RETURN:
            target = HlslNewStmt(context->module, HLSL_STMT_RETURN);
            if (target != NULL && source->u.returnExpr != NULL)
                target->u.returnExpr = HlslLowerIRExpr(context,
                    source->u.returnExpr, list, HLSL_VALUE_RVALUE);
            if (target == NULL || (source->u.returnExpr != NULL &&
                                   target->u.returnExpr == NULL))
            {
                return 0;
            }
            break;
        case CGIR_STMT_BREAK:
            target = HlslNewStmt(context->module, HLSL_STMT_BREAK);
            break;
        case CGIR_STMT_CONTINUE:
            target = HlslNewStmt(context->module, HLSL_STMT_CONTINUE);
            break;
        case CGIR_STMT_DISCARD:
            return HlslLowerFailure(context,
                HLSL_ERROR_STAGE_OPERATION, "discard", &source->loc);
        case CGIR_STMT_GEOMETRY_EMIT:
        case CGIR_STMT_GEOMETRY_FLAT:
        case CGIR_STMT_GEOMETRY_RESTART:
            target = HlslLowerGeometryOperation(context, source);
            if (target == NULL)
                return HlslLowerFailure(context, HLSL_ERROR_INVALID_IR,
                    "geometry operation lowering", &source->loc);
            break;
        default:
            return HlslLowerFailure(context,
                HLSL_ERROR_UNSUPPORTED_OPERATION,
                "geometry Cg IR statement", &source->loc);
        }
        HlslSetLoc(&target->loc, &source->loc);
        HlslAppendStmt(list, target);
    }
    return 1;
} // HlslLowerIRStatements

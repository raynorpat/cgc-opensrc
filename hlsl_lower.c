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
// hlsl_lower.c
//

#include <float.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>

#include "slglobals.h"
#include "cg_stdlib.h"
#include "cg_ir.h"
#include "hlsl_hal.h"
#include "hlsl_lower_internal.h"

static int HlslLowerIRStatements(HlslLowerContext *context,
                                 const CgIRStmt *source,
                                 HlslStmt **list);

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

static HlslStmt *HlslNewBoolAssignment(HlslLowerContext *context,
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


static HlslDecl *HlslGeometryOutputMember(HlslLowerContext *context,
                                           int semantic)
{
    HlslDecl *member;
    const char *canonical;

    if (context == NULL || context->module->geometryOutputStruct == NULL)
        return NULL;
    canonical = GetAtomString(atable, semantic);
    if (canonical == NULL)
        return NULL;
    for (member = context->module->geometryOutputStruct->members;
         member != NULL; member = member->next)
    {
        if (member->publicName != NULL &&
            !strcmp(member->publicName, canonical))
        {
            return member;
        }
    }
    return NULL;
} // HlslGeometryOutputMember

static HlslFlatReplay *HlslGeometryFlatState(HlslLowerContext *context,
                                              HlslDecl *target)
{
    HlslFlatReplay *state;

    if (context == NULL || context->function == NULL)
        return NULL;
    for (state = context->function->geometryFlatState; state != NULL;
         state = state->next)
    {
        if (state->target == target)
            return state;
    }
    return NULL;
} // HlslGeometryFlatState

static HlslExpr *HlslGeometryMemberExpr(HlslLowerContext *context,
                                         HlslDecl *object,
                                         HlslDecl *member)
{
    HlslExpr *expression;

    if (object == NULL || member == NULL)
        return NULL;
    expression = HlslNewSourceExpr(context, HLSL_EXPR_MEMBER,
                                   member->type);
    if (expression != NULL) {
        expression->u.member.object = HlslNewSymbolExpr(context, object);
        expression->u.member.decl = member;
        expression->u.member.name = member->name;
        if (expression->u.member.object == NULL)
            return NULL;
    }
    return expression;
} // HlslGeometryMemberExpr

static HlslExpr *HlslGeometryConvert(HlslLowerContext *context,
                                      HlslExpr *value,
                                      const HlslType *type)
{
    HlslExpr *conversion;

    if (value == NULL || type == NULL)
        return NULL;
    if (value->type.base == type->base && value->type.len == type->len &&
        value->type.rows == type->rows && value->type.cols == type->cols &&
        value->type.arraySize == type->arraySize)
    {
        return value;
    }
    conversion = HlslNewSourceExpr(context, HLSL_EXPR_CAST, *type);
    if (conversion != NULL)
        conversion->u.cast.expression = value;
    return conversion;
} // HlslGeometryConvert

static const CgIRExpr *HlslGeometryValueRoot(const CgIRExpr *value)
{
    while (value != NULL && value->kind == CGIR_EXPR_MEMBER)
        value = value->u.member.object;
    return value;
} // HlslGeometryValueRoot

static HlslExpr *HlslGeometryCapturedMember(
    HlslLowerContext *context, const CgIRExpr *value,
    const CgIRExpr *root,
    HlslDecl *capturedRoot)
{
    HlslExpr *target;
    HlslExpr *object;
    HlslDecl *member;
    HlslType type;

    if (context == NULL || value == NULL || root == NULL ||
        capturedRoot == NULL)
    {
        return NULL;
    }
    if (value == root)
        return HlslNewSymbolExpr(context, capturedRoot);
    if (value->kind != CGIR_EXPR_MEMBER ||
        value->u.member.object == NULL ||
        value->u.member.member == NULL ||
        !HlslLowerType(context, value->type, &type, &value->loc))
    {
        return NULL;
    }
    object = HlslGeometryCapturedMember(context, value->u.member.object,
                                        root, capturedRoot);
    member = HlslFindDecl(context, value->u.member.member);
    target = object != NULL && member != NULL ?
        HlslNewSourceExpr(context, HLSL_EXPR_MEMBER, type) : NULL;
    if (target != NULL) {
        target->u.member.object = object;
        target->u.member.decl = member;
        target->u.member.name = member->name;
    }
    return target;
} // HlslGeometryCapturedMember

static HlslStmt *HlslLowerGeometryOperation(
    HlslLowerContext *context, const CgIRStmt *operation)
{
    const CgIRGeometryValue *value;
    HlslFlatReplay *state;
    HlslDecl *target;
    HlslExpr *lowered;
    HlslExpr *left;
    HlslExpr *assignment;
    HlslExpr *record;
    HlslExpr *zero;
    HlslStmt *statement;
    HlslStmt *body;
    HlslDecl **targets;
    HlslExpr **captured;
    HlslExpr **rootCaptures;
    const CgIRExpr **roots;
    HlslLoc loc;
    int valueCount;
    int valueIndex;
    int rootIndex;
    int rootUses;

    if (context == NULL || operation == NULL || context->function == NULL ||
        !context->function->geometryEffect ||
        context->function->geometryStream == NULL ||
        context->function->geometryOutputRecord == NULL)
    {
        return NULL;
    }
    HlslSetLoc(&loc, &operation->loc);
    if (operation->kind == CGIR_STMT_GEOMETRY_RESTART)
        return HlslNewRestartStrip(context->module, loc);

    valueCount = 0;
    for (value = operation->u.geometry.values; value != NULL;
         value = value->next)
    {
        valueCount++;
    }
    if (valueCount <= 0 ||
        (size_t) valueCount > (size_t) -1 / sizeof(HlslDecl *) ||
        (size_t) valueCount > (size_t) -1 / sizeof(HlslExpr *))
    {
        return NULL;
    }
    targets = (HlslDecl **) HlslLowerAlloc(context,
        (size_t) valueCount * sizeof(HlslDecl *));
    captured = (HlslExpr **) HlslLowerAlloc(context,
        (size_t) valueCount * sizeof(HlslExpr *));
    rootCaptures = (HlslExpr **) HlslLowerAlloc(context,
        (size_t) valueCount * sizeof(HlslExpr *));
    roots = (const CgIRExpr **) HlslLowerAlloc(context,
        (size_t) valueCount * sizeof(CgIRExpr *));
    if (targets == NULL || captured == NULL || rootCaptures == NULL ||
        roots == NULL)
    {
        return NULL;
    }

    valueIndex = 0;
    for (value = operation->u.geometry.values; value != NULL;
         value = value->next)
    {
        roots[valueIndex] = HlslGeometryValueRoot(value->value);
        valueIndex++;
    }

    body = NULL;
    valueIndex = 0;
    for (value = operation->u.geometry.values; value != NULL;
         value = value->next)
    {
        target = HlslGeometryOutputMember(context,
                                           value->canonicalSemantic);
        if (target == NULL)
            return NULL;
        context->statementLoc = value->loc;
        rootUses = 0;
        for (rootIndex = 0; rootIndex < valueCount; rootIndex++) {
            if (roots[rootIndex] == roots[valueIndex])
                rootUses++;
        }
        if (rootUses > 1) {
            rootIndex = 0;
            while (rootIndex < valueIndex &&
                   roots[rootIndex] != roots[valueIndex])
            {
                rootIndex++;
            }
            if (rootIndex == valueIndex) {
                lowered = HlslLowerIRExpr(context, roots[valueIndex],
                                          &body, HLSL_VALUE_RVALUE);
                rootCaptures[valueIndex] = lowered != NULL ?
                    HlslCaptureValue(context, &body, lowered) : NULL;
            } else {
                rootCaptures[valueIndex] = rootCaptures[rootIndex];
            }
            lowered = rootCaptures[valueIndex] != NULL &&
                      rootCaptures[valueIndex]->kind == HLSL_EXPR_SYMBOL ?
                HlslGeometryCapturedMember(context, value->value,
                    roots[valueIndex], rootCaptures[valueIndex]->u.symbol) :
                NULL;
        } else {
            lowered = HlslLowerIRExpr(context, value->value, &body,
                                      HLSL_VALUE_RVALUE);
        }
        if (lowered == NULL)
            return NULL;
        lowered = HlslGeometryConvert(context, lowered, &target->type);
        captured[valueIndex] = lowered != NULL ?
            HlslCaptureValue(context, &body, lowered) : NULL;
        targets[valueIndex] = target;
        if (captured[valueIndex] == NULL)
            return NULL;
        valueIndex++;
    }
    context->statementLoc = operation->loc;
    if (operation->kind == CGIR_STMT_GEOMETRY_EMIT) {
        left = HlslNewSymbolExpr(context,
            context->function->geometryOutputRecord);
        zero = HlslNewLiteral(context, HLSL_BASE_INT, 0, 0.0f);
        record = HlslNewSourceExpr(context, HLSL_EXPR_CAST,
            context->function->geometryOutputRecord->type);
        if (record != NULL)
            record->u.cast.expression = zero;
        assignment = HlslNewAssignment(context, left, record);
        statement = HlslNewExpressionStmt(context, assignment);
        if (statement == NULL)
            return NULL;
        HlslAppendStmt(&body, statement);
    }
    for (valueIndex = 0; valueIndex < valueCount; valueIndex++) {
        target = targets[valueIndex];
        if (operation->kind == CGIR_STMT_GEOMETRY_FLAT) {
            state = HlslGeometryFlatState(context, target);
            if (state == NULL)
                return NULL;
            left = HlslNewSymbolExpr(context, state->shadow);
        } else {
            left = HlslGeometryMemberExpr(context,
                context->function->geometryOutputRecord, target);
        }
        assignment = HlslNewAssignment(context, left,
                                        captured[valueIndex]);
        statement = HlslNewExpressionStmt(context, assignment);
        if (statement == NULL)
            return NULL;
        HlslAppendStmt(&body, statement);
        if (operation->kind == CGIR_STMT_GEOMETRY_FLAT) {
            statement = HlslNewBoolAssignment(context, state->defined, 1);
            if (statement == NULL)
                return NULL;
            HlslAppendStmt(&body, statement);
        }
    }
    if (operation->kind == CGIR_STMT_GEOMETRY_EMIT) {
        record = HlslNewSymbolExpr(context,
            context->function->geometryOutputRecord);
        statement = HlslNewAppend(context->module, record,
            context->function->geometryFlatState, loc);
        if (statement == NULL)
            return NULL;
        HlslAppendStmt(&body, statement);
    }
    statement = HlslNewStmt(context->module, HLSL_STMT_BLOCK);
    if (statement != NULL) {
        statement->u.block = body;
        statement->loc = loc;
    }
    return statement;
} // HlslLowerGeometryOperation

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

static int HlslLowerStatements(HlslLowerContext *context, stmt *source,
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

static int HlslLowerIRStatements(HlslLowerContext *context,
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

static int HlslCollectCallsInExpr(HlslLowerContext *context, expr *source);
static int HlslCollectIRCallsInStatements(HlslLowerContext *context,
                                           const CgIRStmt *source);

static const CgIRFunction *HlslFindSourceIRFunction(
    const HlslLowerContext *context, const Symbol *symbol)
{
    const CgIRFunction *function;

    if (context == NULL || context->sourceIR == NULL || symbol == NULL)
        return NULL;
    for (function = context->sourceIR->functions; function != NULL;
         function = function->next)
    {
        if (function->symbol == symbol)
            return function;
    }
    return NULL;
} // HlslFindSourceIRFunction

static int HlslCollectCallsInStatements(HlslLowerContext *context,
                                        stmt *source)
{
    for (; source != NULL; source = source->commonst.next) {
        context->statementLoc = source->commonst.loc;
        switch (source->commonst.kind) {
        case EXPR_STMT:
            if (!HlslCollectCallsInExpr(context, source->exprst.exp))
                return 0;
            break;
        case BLOCK_STMT:
            if (!HlslCollectCallsInStatements(context,
                                               source->blockst.body))
            {
                return 0;
            }
            break;
        case IF_STMT:
            if (!HlslCollectCallsInExpr(context, source->ifst.cond) ||
                !HlslCollectCallsInStatements(context,
                                               source->ifst.thenstmt) ||
                !HlslCollectCallsInStatements(context,
                                               source->ifst.elsestmt))
            {
                return 0;
            }
            break;
        case WHILE_STMT:
        case DO_STMT:
            if (!HlslCollectCallsInExpr(context, source->whilest.cond) ||
                !HlslCollectCallsInStatements(context,
                                               source->whilest.body))
            {
                return 0;
            }
            break;
        case FOR_STMT:
            if (!HlslCollectCallsInStatements(context,
                                               source->forst.init) ||
                !HlslCollectCallsInExpr(context, source->forst.cond) ||
                !HlslCollectCallsInStatements(context,
                                               source->forst.step) ||
                !HlslCollectCallsInStatements(context,
                                               source->forst.body))
            {
                return 0;
            }
            break;
        case RETURN_STMT:
            if (!HlslCollectCallsInExpr(context, source->returnst.exp))
                return 0;
            break;
        case COMMENT_STMT:
        case DISCARD_STMT:
        case BREAK_STMT:
        case CONTINUE_STMT:
            if (source->commonst.kind == DISCARD_STMT &&
                !HlslCollectCallsInExpr(context,
                                        source->discardst.cond))
            {
                return 0;
            }
            break;
        default:
            return HlslLowerFailure(context,
                                    HLSL_ERROR_UNSUPPORTED_OPERATION,
                                    "HLSL helper statement", NULL);
        }
    }
    return 1;
} // HlslCollectCallsInStatements

static int HlslCollectHelper(HlslLowerContext *context, Symbol *symbol,
                             const SourceLoc *callLoc)
{
    const CgIRFunction *sourceFunction;
    HlslFunction *function;
    HlslType result;
    char *generated;
    const char *name;
    Symbol *caller;
    SourceLoc callerLoc;

    if (symbol == NULL || symbol->kind != FUNCTION_S)
        return 0;
    if (symbol->properties & SYMB_IS_BUILTIN)
        return 1;
    if (!HlslRejectStorage(context, symbol))
        return 0;
    function = HlslFindFunction(context->module, symbol);
    if (function != NULL) {
        if (function->visitState == 1)
            return HlslLowerFailure(context,
                                    HLSL_ERROR_UNSUPPORTED_OPERATION,
                                    "recursive HLSL helper",
                                    callLoc);
        return 1;
    }
    if (!HlslEnsureType(context, symbol->type->fun.rettype) ||
        !HlslLowerType(context, symbol->type->fun.rettype,
                       &result, &symbol->loc))
    {
        return 0;
    }
    generated = HlslGeneratedSource(context,
                                    GetAtomString(atable, symbol->name));
    name = generated != NULL ?
           HlslAllocateGeneratedName(context->module, symbol,
                                     generated) : NULL;
    function = HlslNewFunction(context->module, result, name);
    if (function == NULL)
        return 0;
    function->identity = symbol;
    function->visitState = 1;
    caller = context->collectingHelper;
    callerLoc = context->statementLoc;
    if (caller != NULL)
        function->needsPrototype = 1;
    HlslSetLoc(&function->loc, &symbol->loc);
    HlslAppendFunction(&context->module->functions, function);
    context->collectingHelper = symbol;
    sourceFunction = HlslFindSourceIRFunction(context, symbol);
    if ((context->sourceIR != NULL && sourceFunction == NULL) ||
        (sourceFunction != NULL &&
         !HlslCollectIRCallsInStatements(context, sourceFunction->body)) ||
        (sourceFunction == NULL &&
         !HlslCollectCallsInStatements(context,
                                       symbol->details.fun.statements)))
    {
        context->collectingHelper = caller;
        context->statementLoc = callerLoc;
        return 0;
    }
    context->collectingHelper = caller;
    context->statementLoc = callerLoc;
    function->visitState = 2;
    return 1;
} // HlslCollectHelper

static int HlslCollectCallsInExpr(HlslLowerContext *context, expr *source)
{
    const SourceLoc *callLoc;
    HlslBuiltin builtin;
    HlslType result;
    HlslType params[HLSL_MAX_BUILTIN_ARGS];
    int paramCount;
    int builtinStatus;

    if (source == NULL)
        return 1;
    switch (source->common.kind) {
    case DECL_N:
    case SYMB_N:
    case CONST_N:
        return 1;
    case UNARY_N:
        return HlslCollectCallsInExpr(context, source->un.arg);
    case BINARY_N:
        callLoc = source->bin.op == FUN_CALL_OP ?
                  GetExprCallSite(source) : NULL;
        if (source->bin.op == FUN_CALL_OP && source->bin.left != NULL &&
            source->bin.left->common.kind == SYMB_N)
        {
            builtinStatus = HlslResolveBuiltinSymbol(context,
                source->bin.left->sym.symbol, callLoc, &builtin,
                &result, params, &paramCount);
            if (builtinStatus < 0)
                return 0;
            if (builtinStatus == 0 &&
                !HlslCollectHelper(context,
                    source->bin.left->sym.symbol, callLoc))
            {
                return 0;
            }
        }
        return HlslCollectCallsInExpr(context, source->bin.left) &&
               HlslCollectCallsInExpr(context, source->bin.right);
    case TRINARY_N:
        return HlslCollectCallsInExpr(context, source->tri.arg1) &&
               HlslCollectCallsInExpr(context, source->tri.arg2) &&
               HlslCollectCallsInExpr(context, source->tri.arg3);
    default:
        return 0;
    }
} // HlslCollectCallsInExpr

static int HlslCollectIRCallsInExpr(HlslLowerContext *context,
                                    const CgIRExpr *source)
{
    const CgIRExpr *argument;
    HlslBuiltin builtin;
    HlslType result;
    HlslType params[HLSL_MAX_BUILTIN_ARGS];
    int paramCount;
    int builtinStatus;

    if (source == NULL)
        return 1;
    switch (source->kind) {
    case CGIR_EXPR_CONSTANT:
    case CGIR_EXPR_SYMBOL:
        return 1;
    case CGIR_EXPR_MEMBER:
        return HlslCollectIRCallsInExpr(context, source->u.member.object);
    case CGIR_EXPR_INDEX:
        return HlslCollectIRCallsInExpr(context, source->u.index.object) &&
               HlslCollectIRCallsInExpr(context, source->u.index.index);
    case CGIR_EXPR_LENGTH:
        return HlslCollectIRCallsInExpr(context, source->u.length.object);
    case CGIR_EXPR_SWIZZLE:
        return HlslCollectIRCallsInExpr(context, source->u.swizzle.object);
    case CGIR_EXPR_CONSTRUCT:
        argument = source->u.construct.arguments;
        break;
    case CGIR_EXPR_CAST:
        return HlslCollectIRCallsInExpr(context, source->u.cast.operand);
    case CGIR_EXPR_UNARY:
        return HlslCollectIRCallsInExpr(context, source->u.unary.operand);
    case CGIR_EXPR_BINARY:
        return HlslCollectIRCallsInExpr(context, source->u.binary.left) &&
               HlslCollectIRCallsInExpr(context, source->u.binary.right);
    case CGIR_EXPR_ASSIGN:
        return HlslCollectIRCallsInExpr(context, source->u.assign.target) &&
               HlslCollectIRCallsInExpr(context, source->u.assign.value);
    case CGIR_EXPR_CONDITIONAL:
        return HlslCollectIRCallsInExpr(context,
                   source->u.conditional.condition) &&
               HlslCollectIRCallsInExpr(context,
                   source->u.conditional.trueExpr) &&
               HlslCollectIRCallsInExpr(context,
                   source->u.conditional.falseExpr);
    case CGIR_EXPR_CALL:
        builtinStatus = HlslResolveBuiltinSymbol(context,
            source->u.call.callee, &source->loc, &builtin, &result,
            params, &paramCount);
        if (builtinStatus < 0 ||
            (builtinStatus == 0 && !HlslCollectHelper(context,
                source->u.call.callee, &source->loc)))
        {
            return 0;
        }
        argument = source->u.call.arguments;
        break;
    case CGIR_EXPR_INTERFACE_CALL:
        if (!HlslCollectIRCallsInExpr(context,
                                      source->u.interfaceCall.receiver))
        {
            return 0;
        }
        argument = source->u.interfaceCall.arguments;
        break;
    case CGIR_EXPR_INTRINSIC:
        argument = source->u.intrinsicCall.arguments;
        break;
    default:
        return 0;
    }
    for (; argument != NULL; argument = argument->next) {
        if (!HlslCollectIRCallsInExpr(context, argument))
            return 0;
    }
    return 1;
} // HlslCollectIRCallsInExpr

static int HlslCollectIRCallsInStatements(HlslLowerContext *context,
                                           const CgIRStmt *source)
{
    const CgIRGeometryValue *value;

    for (; source != NULL; source = source->next) {
        context->statementLoc = source->loc;
        switch (source->kind) {
        case CGIR_STMT_BLOCK:
            if (!HlslCollectIRCallsInStatements(context, source->u.block))
                return 0;
            break;
        case CGIR_STMT_DECL:
            if (source->u.decl != NULL &&
                !HlslCollectIRCallsInExpr(context,
                                          source->u.decl->initializer))
            {
                return 0;
            }
            break;
        case CGIR_STMT_EXPR:
            if (!HlslCollectIRCallsInExpr(context, source->u.expression))
                return 0;
            break;
        case CGIR_STMT_IF:
            if (!HlslCollectIRCallsInExpr(context,
                    source->u.ifStmt.condition) ||
                !HlslCollectIRCallsInStatements(context,
                    source->u.ifStmt.trueBranch) ||
                !HlslCollectIRCallsInStatements(context,
                    source->u.ifStmt.falseBranch))
            {
                return 0;
            }
            break;
        case CGIR_STMT_WHILE:
        case CGIR_STMT_DO:
            if (!HlslCollectIRCallsInExpr(context,
                    source->u.loop.condition) ||
                !HlslCollectIRCallsInStatements(context,
                    source->u.loop.body))
            {
                return 0;
            }
            break;
        case CGIR_STMT_FOR:
            if (!HlslCollectIRCallsInStatements(context,
                    source->u.forStmt.init) ||
                !HlslCollectIRCallsInExpr(context,
                    source->u.forStmt.condition) ||
                !HlslCollectIRCallsInExpr(context,
                    source->u.forStmt.step) ||
                !HlslCollectIRCallsInStatements(context,
                    source->u.forStmt.body))
            {
                return 0;
            }
            break;
        case CGIR_STMT_RETURN:
            if (!HlslCollectIRCallsInExpr(context, source->u.returnExpr))
                return 0;
            break;
        case CGIR_STMT_DISCARD:
            if (!HlslCollectIRCallsInExpr(context,
                                          source->u.discard.condition))
            {
                return 0;
            }
            break;
        case CGIR_STMT_GEOMETRY_EMIT:
        case CGIR_STMT_GEOMETRY_FLAT:
            for (value = source->u.geometry.values; value != NULL;
                 value = value->next)
            {
                if (!HlslCollectIRCallsInExpr(context, value->value))
                    return 0;
            }
            break;
        case CGIR_STMT_GEOMETRY_RESTART:
        case CGIR_STMT_BREAK:
        case CGIR_STMT_CONTINUE:
            break;
        default:
            return 0;
        }
    }
    return 1;
} // HlslCollectIRCallsInStatements

static int HlslGeometryIRHasDirectEffect(const CgIRStmt *source)
{
    for (; source != NULL; source = source->next) {
        if (source->kind == CGIR_STMT_GEOMETRY_EMIT ||
            source->kind == CGIR_STMT_GEOMETRY_FLAT ||
            source->kind == CGIR_STMT_GEOMETRY_RESTART)
            return 1;
        switch (source->kind) {
        case CGIR_STMT_BLOCK:
            if (HlslGeometryIRHasDirectEffect(source->u.block))
                return 1;
            break;
        case CGIR_STMT_IF:
            if (HlslGeometryIRHasDirectEffect(source->u.ifStmt.trueBranch) ||
                HlslGeometryIRHasDirectEffect(source->u.ifStmt.falseBranch))
            {
                return 1;
            }
            break;
        case CGIR_STMT_WHILE:
        case CGIR_STMT_DO:
            if (HlslGeometryIRHasDirectEffect(source->u.loop.body))
                return 1;
            break;
        case CGIR_STMT_FOR:
            if (HlslGeometryIRHasDirectEffect(source->u.forStmt.init) ||
                HlslGeometryIRHasDirectEffect(source->u.forStmt.body))
            {
                return 1;
            }
            break;
        default:
            break;
        }
    }
    return 0;
} // HlslGeometryIRHasDirectEffect

static int HlslGeometryIRExprCallsEffect(HlslLowerContext *context,
                                         const CgIRExpr *source)
{
    const CgIRExpr *argument;
    HlslFunction *callee;

    if (source == NULL)
        return 0;
    if (source->kind == CGIR_EXPR_CALL) {
        callee = HlslFindFunction(context->module, source->u.call.callee);
        if (callee != NULL && callee->geometryEffect)
            return 1;
    }
    switch (source->kind) {
    case CGIR_EXPR_MEMBER:
        return HlslGeometryIRExprCallsEffect(context,
                                             source->u.member.object);
    case CGIR_EXPR_INDEX:
        return HlslGeometryIRExprCallsEffect(context,
                    source->u.index.object) ||
               HlslGeometryIRExprCallsEffect(context,
                    source->u.index.index);
    case CGIR_EXPR_LENGTH:
        return HlslGeometryIRExprCallsEffect(context,
                                             source->u.length.object);
    case CGIR_EXPR_SWIZZLE:
        return HlslGeometryIRExprCallsEffect(context,
                                             source->u.swizzle.object);
    case CGIR_EXPR_CAST:
        return HlslGeometryIRExprCallsEffect(context,
                                             source->u.cast.operand);
    case CGIR_EXPR_UNARY:
        return HlslGeometryIRExprCallsEffect(context,
                                             source->u.unary.operand);
    case CGIR_EXPR_BINARY:
        return HlslGeometryIRExprCallsEffect(context,
                    source->u.binary.left) ||
               HlslGeometryIRExprCallsEffect(context,
                    source->u.binary.right);
    case CGIR_EXPR_ASSIGN:
        return HlslGeometryIRExprCallsEffect(context,
                    source->u.assign.target) ||
               HlslGeometryIRExprCallsEffect(context,
                    source->u.assign.value);
    case CGIR_EXPR_CONDITIONAL:
        return HlslGeometryIRExprCallsEffect(context,
                    source->u.conditional.condition) ||
               HlslGeometryIRExprCallsEffect(context,
                    source->u.conditional.trueExpr) ||
               HlslGeometryIRExprCallsEffect(context,
                    source->u.conditional.falseExpr);
    case CGIR_EXPR_CONSTRUCT:
        argument = source->u.construct.arguments;
        break;
    case CGIR_EXPR_CALL:
        argument = source->u.call.arguments;
        break;
    case CGIR_EXPR_INTERFACE_CALL:
        if (HlslGeometryIRExprCallsEffect(context,
                                         source->u.interfaceCall.receiver))
        {
            return 1;
        }
        argument = source->u.interfaceCall.arguments;
        break;
    case CGIR_EXPR_INTRINSIC:
        argument = source->u.intrinsicCall.arguments;
        break;
    default:
        return 0;
    }
    for (; argument != NULL; argument = argument->next) {
        if (HlslGeometryIRExprCallsEffect(context, argument))
            return 1;
    }
    return 0;
} // HlslGeometryIRExprCallsEffect

static int HlslGeometryIRCallsEffect(HlslLowerContext *context,
                                     const CgIRStmt *source)
{
    const CgIRGeometryValue *value;

    for (; source != NULL; source = source->next) {
        switch (source->kind) {
        case CGIR_STMT_BLOCK:
            if (HlslGeometryIRCallsEffect(context, source->u.block))
                return 1;
            break;
        case CGIR_STMT_DECL:
            if (source->u.decl != NULL &&
                HlslGeometryIRExprCallsEffect(context,
                                              source->u.decl->initializer))
            {
                return 1;
            }
            break;
        case CGIR_STMT_EXPR:
            if (HlslGeometryIRExprCallsEffect(context,
                                              source->u.expression))
                return 1;
            break;
        case CGIR_STMT_IF:
            if (HlslGeometryIRExprCallsEffect(context,
                    source->u.ifStmt.condition) ||
                HlslGeometryIRCallsEffect(context,
                    source->u.ifStmt.trueBranch) ||
                HlslGeometryIRCallsEffect(context,
                    source->u.ifStmt.falseBranch))
            {
                return 1;
            }
            break;
        case CGIR_STMT_WHILE:
        case CGIR_STMT_DO:
            if (HlslGeometryIRExprCallsEffect(context,
                    source->u.loop.condition) ||
                HlslGeometryIRCallsEffect(context,
                    source->u.loop.body))
            {
                return 1;
            }
            break;
        case CGIR_STMT_FOR:
            if (HlslGeometryIRCallsEffect(context,
                    source->u.forStmt.init) ||
                HlslGeometryIRExprCallsEffect(context,
                    source->u.forStmt.condition) ||
                HlslGeometryIRExprCallsEffect(context,
                    source->u.forStmt.step) ||
                HlslGeometryIRCallsEffect(context,
                    source->u.forStmt.body))
            {
                return 1;
            }
            break;
        case CGIR_STMT_RETURN:
            if (HlslGeometryIRExprCallsEffect(context,
                                              source->u.returnExpr))
                return 1;
            break;
        case CGIR_STMT_DISCARD:
            if (HlslGeometryIRExprCallsEffect(context,
                                              source->u.discard.condition))
                return 1;
            break;
        case CGIR_STMT_GEOMETRY_EMIT:
        case CGIR_STMT_GEOMETRY_FLAT:
            for (value = source->u.geometry.values; value != NULL;
                 value = value->next)
            {
                if (HlslGeometryIRExprCallsEffect(context, value->value))
                    return 1;
            }
            break;
        default:
            break;
        }
    }
    return 0;
} // HlslGeometryStatementsCallEffect

static int HlslMarkGeometryEffects(HlslLowerContext *context)
{
    const CgIRFunction *sourceFunction;
    HlslFunction *function;
    int changed;

    for (function = context->module->functions; function != NULL;
         function = function->next)
    {
        sourceFunction = HlslFindSourceIRFunction(context,
            (Symbol *) function->identity);
        function->geometryEffect = sourceFunction != NULL &&
            HlslGeometryIRHasDirectEffect(sourceFunction->body);
    }
    do {
        changed = 0;
        for (function = context->module->functions; function != NULL;
             function = function->next)
        {
            sourceFunction = HlslFindSourceIRFunction(context,
                (Symbol *) function->identity);
            if (!function->geometryEffect && sourceFunction != NULL &&
                HlslGeometryIRCallsEffect(context, sourceFunction->body))
            {
                function->geometryEffect = 1;
                changed = 1;
            }
        }
    } while (changed);
    return 1;
} // HlslMarkGeometryEffects

static int HlslGeometrySemanticInfo(HlslLowerContext *context,
    const CgIRGeometryValue *value, HlslType *type, const char **spelling,
    HlslSemanticKind *kindOut, int *indexOut)
{
    const char *canonical;
    char root[64];
    char emitted[96];
    HlslSemanticKind kind;
    HlslBase abiBase;
    int index;

    canonical = GetAtomString(atable, value->canonicalSemantic);
    if (canonical == NULL ||
        !HlslParseSemantic(canonical, root, sizeof(root), &index) ||
        !HlslLowerType(context, value->type, type, &value->loc))
    {
        return 0;
    }
    kind = HlslModernSemantic(HLSL_STAGE_GEOMETRY,
                              HLSL_DIRECTION_OUTPUT, root, index);
    if (kind == HLSL_SEMANTIC_UNSUPPORTED ||
        !HlslModernSemanticIsLegal(HLSL_STAGE_GEOMETRY,
                                   HLSL_DIRECTION_OUTPUT, kind))
    {
        return 0;
    }
    if (kind == HLSL_SEMANTIC_USER) {
        if (sprintf(emitted, "%s%d", root, index) < 0)
            return 0;
    } else if (!HlslModernSemanticSpelling(kind, index, emitted,
                                           sizeof(emitted))) {
        return 0;
    }
    abiBase = HlslModernAbiBase(kind);
    if (abiBase == HLSL_BASE_UINT || abiBase == HLSL_BASE_BOOL)
        type->base = abiBase;
    *spelling = HlslCopyText(context, emitted);
    *kindOut = kind;
    *indexOut = index;
    return *spelling != NULL;
} // HlslGeometrySemanticInfo

static const char *HlslGeometryMemberName(HlslLowerContext *context,
                                           const char *semantic)
{
    char name[128];
    size_t i;

    if (semantic == NULL || strlen(semantic) + 1 > sizeof(name))
        return NULL;
    for (i = 0; semantic[i] != '\0'; i++) {
        name[i] = semantic[i] >= 'A' && semantic[i] <= 'Z' ?
                  (char) (semantic[i] - 'A' + 'a') : semantic[i];
    }
    name[i] = '\0';
    return HlslAllocateScopedSymbolName(context->module,
        context->module->geometryOutputStruct, semantic, name);
} // HlslGeometryMemberName

static int HlslCollectGeometryOutputStatements(HlslLowerContext *context,
                                                const CgIRStmt *source,
                                                HlslDecl *structure)
{
    const CgIRGeometryValue *value;
    HlslDecl *member;
    HlslType memberType;
    HlslSemanticKind kind;
    const char *canonical;
    const char *sourceSemantic;
    const char *semantic;
    const char *name;
    int semanticIndex;

    for (; source != NULL; source = source->next) {
        switch (source->kind) {
        case CGIR_STMT_BLOCK:
            if (!HlslCollectGeometryOutputStatements(context,
                    source->u.block, structure))
                return 0;
            break;
        case CGIR_STMT_IF:
            if (!HlslCollectGeometryOutputStatements(context,
                    source->u.ifStmt.trueBranch, structure) ||
                !HlslCollectGeometryOutputStatements(context,
                    source->u.ifStmt.falseBranch, structure))
            {
                return 0;
            }
            break;
        case CGIR_STMT_WHILE:
        case CGIR_STMT_DO:
            if (!HlslCollectGeometryOutputStatements(context,
                    source->u.loop.body, structure))
                return 0;
            break;
        case CGIR_STMT_FOR:
            if (!HlslCollectGeometryOutputStatements(context,
                    source->u.forStmt.init, structure) ||
                !HlslCollectGeometryOutputStatements(context,
                    source->u.forStmt.body, structure))
            {
                return 0;
            }
            break;
        case CGIR_STMT_GEOMETRY_EMIT:
        case CGIR_STMT_GEOMETRY_FLAT:
            for (value = source->u.geometry.values; value != NULL;
                 value = value->next)
            {
                canonical = GetAtomString(atable,
                                          value->canonicalSemantic);
                sourceSemantic = GetAtomString(atable,
                                               value->sourceSemantic);
                if (canonical == NULL ||
                    !HlslGeometrySemanticInfo(context, value,
                        &memberType, &semantic, &kind, &semanticIndex))
                {
                    return 0;
                }
                member = HlslGeometryOutputMember(context,
                                                   value->canonicalSemantic);
                if (member == NULL) {
                    name = HlslGeometryMemberName(context, canonical);
                    member = name != NULL ? HlslNewDecl(context->module,
                        HLSL_STORAGE_NONE, memberType, name) : NULL;
                    if (member == NULL)
                        return 0;
                    member->identity = value;
                    member->publicName = canonical;
                    member->semantic = semantic;
                    member->canonicalSemantic = semantic;
                    member->inputSemantic = sourceSemantic;
                    member->semanticKind = kind;
                    member->semanticIndex = semanticIndex;
                    member->interpolation =
                        HlslModernRequiredTargetInterpolation(
                            memberType.base);
                    HlslSetLoc(&member->loc, &value->loc);
                    HlslAppendDecl(&structure->members, member);
                } else if (!HlslIRTypeIsIdentical(&member->type,
                                                   &memberType)) {
                    HlslLoc conflictLoc;

                    HlslSetLoc(&conflictLoc, &value->loc);
                    return HlslFailRelated(context->module,
                        HLSL_ERROR_INTERFACE_CONFLICT, &conflictLoc,
                        &member->loc, canonical);
                }
                if (source->kind == CGIR_STMT_GEOMETRY_FLAT) {
                    member->geometryRole =
                        HLSL_GEOMETRY_DECL_FLAT_TARGET;
                    member->interpolation =
                        HLSL_INTERPOLATION_NOINTERPOLATION;
                }
            }
            break;
        default:
            break;
        }
    }
    return 1;
} // HlslCollectGeometryOutputStatements

static int HlslCollectGeometryOutput(HlslLowerContext *context)
{
    static int outputIdentity;
    static int placeholderIdentity;
    const CgIRFunction *function;
    HlslDecl *structure;
    HlslDecl *member;
    HlslType structureType;
    const char *name;
    const char *structureName;

    if (context->module->stage != HLSL_STAGE_GEOMETRY ||
        context->sourceIR == NULL)
    {
        return 1;
    }
    structureName = HlslAllocateGeneratedName(context->module,
        &outputIdentity, "cg_GeometryOut");
    structureType = HlslNumericType(HLSL_BASE_STRUCT, 0);
    structureType.structName = structureName;
    structure = structureName != NULL ? HlslNewDecl(context->module,
        HLSL_STORAGE_OUTPUT, structureType, structureName) : NULL;
    if (structure == NULL)
        return 0;
    context->module->geometryOutputStruct = structure;

    for (function = context->sourceIR->functions; function != NULL;
         function = function->next)
    {
        if (!HlslCollectGeometryOutputStatements(context, function->body,
                                                  structure))
            return 0;
    }
    if (structure->members == NULL) {
        name = HlslAllocateScopedSymbolName(context->module, structure,
                                            &placeholderIdentity,
                                            "cgc_placeholder");
        member = name != NULL ? HlslNewDecl(context->module,
            HLSL_STORAGE_NONE, HlslNumericType(HLSL_BASE_FLOAT, 1),
            name) : NULL;
        if (member == NULL)
            return 0;
        member->identity = &placeholderIdentity;
        member->semantic = HlslCopyText(context, "TEXCOORD0");
        member->canonicalSemantic = member->semantic;
        member->semanticKind = HLSL_SEMANTIC_USER;
        member->semanticIndex = 0;
        member->geometryRole = HLSL_GEOMETRY_DECL_OUTPUT_PLACEHOLDER;
        HlslAppendDecl(&structure->members, member);
    }
    structure->type.members = structure->members;
    HlslAppendDecl(&context->module->structs, structure);
    return 1;
} // HlslCollectGeometryOutput

static int HlslPrepareGeometryFunctionState(HlslLowerContext *context,
                                             HlslFunction *function)
{
    HlslFlatReplay **tail;
    HlslFlatReplay *state;
    HlslDecl *member;
    HlslDecl *shadow;
    HlslDecl *defined;
    HlslType streamType;
    HlslType boolType;
    const char *name;
    char generated[192];

    if (!function->geometryEffect)
        return 1;
    if (context->module->geometryOutputStruct == NULL ||
        context->module->geometryOutputStruct->members == NULL)
    {
        return 0;
    }
    streamType = HlslNumericType(HLSL_BASE_GEOMETRY_STREAM,
        (int) context->module->geometryStream + 1);
    name = HlslAllocateScopedSymbolName(context->module, function,
                                        function, "cgc_stream");
    function->geometryStream = name != NULL ? HlslNewDecl(
        context->module, HLSL_STORAGE_NONE, streamType, name) : NULL;
    name = HlslAllocateScopedSymbolName(context->module, function,
        context->module->geometryOutputStruct, "cgc_output");
    function->geometryOutputRecord = name != NULL ? HlslNewDecl(
        context->module, HLSL_STORAGE_NONE,
        context->module->geometryOutputStruct->type, name) : NULL;
    if (function->geometryStream == NULL ||
        function->geometryOutputRecord == NULL)
    {
        return 0;
    }
    function->geometryStream->parameterQualifier = HLSL_PARAMETER_INOUT;
    function->geometryStream->geometryRole = HLSL_GEOMETRY_DECL_STREAM;
    function->geometryStream->publicName =
        context->module->geometryOutputStruct->name;
    function->geometryOutputRecord->parameterQualifier =
        HLSL_PARAMETER_INOUT;
    function->geometryOutputRecord->geometryRole =
        HLSL_GEOMETRY_DECL_OUTPUT_RECORD;

    tail = &function->geometryFlatState;
    boolType = HlslNumericType(HLSL_BASE_BOOL, 1);
    for (member = context->module->geometryOutputStruct->members;
         member != NULL; member = member->next)
    {
        if (member->geometryRole != HLSL_GEOMETRY_DECL_FLAT_TARGET)
            continue;
        if (strlen(member->name) + 18 > sizeof(generated))
            return 0;
        sprintf(generated, "cgc_flat_%s", member->name);
        name = HlslAllocateScopedSymbolName(context->module, function,
                                            member, generated);
        shadow = name != NULL ? HlslNewDecl(context->module,
            HLSL_STORAGE_NONE, member->type, name) : NULL;
        sprintf(generated, "cgc_flat_%s_defined", member->name);
        name = HlslAllocateScopedSymbolName(context->module, function,
                                            shadow, generated);
        defined = name != NULL ? HlslNewDecl(context->module,
            HLSL_STORAGE_NONE, boolType, name) : NULL;
        state = shadow != NULL && defined != NULL ?
            HlslNewFlatReplay(context->module, member, shadow, defined) :
            NULL;
        if (state == NULL)
            return 0;
        shadow->parameterQualifier = HLSL_PARAMETER_INOUT;
        shadow->geometryRole = HLSL_GEOMETRY_DECL_FLAT_SHADOW;
        defined->parameterQualifier = HLSL_PARAMETER_INOUT;
        defined->geometryRole = HLSL_GEOMETRY_DECL_FLAT_DEFINED;
        *tail = state;
        tail = &state->next;
    }
    return 1;
} // HlslPrepareGeometryFunctionState

static int HlslPrepareGeometryFunctions(HlslLowerContext *context)
{
    HlslFunction *function;

    HlslMarkGeometryEffects(context);
    for (function = context->module->functions; function != NULL;
         function = function->next)
    {
        if (!HlslPrepareGeometryFunctionState(context, function))
            return 0;
    }
    return 1;
} // HlslPrepareGeometryFunctions

static int HlslDeclListContains(const HlslDecl *declarations,
                                const HlslDecl *target)
{
    for (; declarations != NULL; declarations = declarations->next) {
        if (declarations == target)
            return 1;
    }
    return 0;
} // HlslDeclListContains

static int HlslInitializeReturnedStructs(HlslModule *module,
                                         HlslFunction *function,
                                         HlslStmt *statements)
{
    HlslDecl *declaration;
    HlslExpr *zero;
    HlslExpr *initializer;

    for (; statements != NULL; statements = statements->next) {
        switch (statements->kind) {
        case HLSL_STMT_IF:
            if (!HlslInitializeReturnedStructs(module, function,
                    statements->u.ifStmt.trueBranch) ||
                !HlslInitializeReturnedStructs(module, function,
                    statements->u.ifStmt.falseBranch))
            {
                return 0;
            }
            break;
        case HLSL_STMT_WHILE:
        case HLSL_STMT_DO:
            if (!HlslInitializeReturnedStructs(module, function,
                                                statements->u.loop.body))
            {
                return 0;
            }
            break;
        case HLSL_STMT_FOR:
            if (!HlslInitializeReturnedStructs(module, function,
                    statements->u.forStmt.init) ||
                !HlslInitializeReturnedStructs(module, function,
                    statements->u.forStmt.body))
            {
                return 0;
            }
            break;
        case HLSL_STMT_BLOCK:
            if (!HlslInitializeReturnedStructs(module, function,
                                                statements->u.block))
            {
                return 0;
            }
            break;
        case HLSL_STMT_RETURN:
            if (statements->u.returnExpr == NULL ||
                statements->u.returnExpr->kind != HLSL_EXPR_SYMBOL)
            {
                break;
            }
            declaration = statements->u.returnExpr->u.symbol;
            if (declaration == NULL || declaration->initializer != NULL ||
                declaration->type.base != HLSL_BASE_STRUCT ||
                !HlslDeclListContains(function->locals, declaration))
            {
                break;
            }
            zero = HlslNewExpr(module, HLSL_EXPR_INT,
                               HlslNumericType(HLSL_BASE_INT, 1));
            initializer = HlslNewExpr(module, HLSL_EXPR_CAST,
                                      declaration->type);
            if (zero == NULL || initializer == NULL)
                return 0;
            zero->u.literalInt = 0;
            initializer->u.cast.expression = zero;
            declaration->initializer = initializer;
            break;
        default:
            break;
        }
    }
    return 1;
} // HlslInitializeReturnedStructs

static int HlslLowerFunction(HlslLowerContext *context,
                             HlslFunction *function)
{
    const CgIRFunction *sourceFunction;
    Symbol *symbol;
    const char *name;

    symbol = (Symbol *) function->identity;
    name = symbol != NULL ? GetAtomString(atable, symbol->name) : NULL;
    context->function = function;
    context->statementLoc = symbol->loc;
    if (!HlslCollectParameters(context, symbol->details.fun.params, 0))
        return context->module->errors != 0 ? 0 :
            HlslLowerFailure(context, HLSL_ERROR_UNSUPPORTED_OPERATION,
                             name, &symbol->loc);
    if (symbol->details.fun.locals == NULL)
        return HlslLowerFailure(context, HLSL_ERROR_UNSUPPORTED_OPERATION,
                                name, &symbol->loc);
    if (!HlslCollectLocals(context, symbol->details.fun.locals->symbols))
        return context->module->errors != 0 ? 0 :
            HlslLowerFailure(context, HLSL_ERROR_UNSUPPORTED_OPERATION,
                             name, &symbol->loc);
    sourceFunction = HlslFindSourceIRFunction(context, symbol);
    if ((sourceFunction != NULL &&
         !HlslLowerIRStatements(context,
             sourceFunction->body != NULL &&
             sourceFunction->body->kind == CGIR_STMT_BLOCK &&
             sourceFunction->body->next == NULL ?
                 sourceFunction->body->u.block : sourceFunction->body,
                                &function->body)) ||
        (sourceFunction == NULL &&
         !HlslLowerStatements(context, symbol->details.fun.statements,
                              &function->body)))
    {
        return context->module->errors != 0 ? 0 :
            HlslLowerFailure(context, HLSL_ERROR_UNSUPPORTED_OPERATION,
                             name, &symbol->loc);
    }
    return HlslInitializeReturnedStructs(context->module, function,
                                         function->body);
} // HlslLowerFunction

static Type *HlslOriginalEntryResult(Symbol *program)
{
    Type *result;
    const char *tag;
    stmt *statement;

    result = program->type->fun.rettype;
    if (GetCategory(result) != TYPE_CATEGORY_STRUCT)
        return result;
    tag = GetAtomString(atable, result->str.tag);
    if (tag == NULL || strcmp(tag, "$progret"))
        return result;
    statement = program->details.fun.statements;
    while (statement != NULL && statement->commonst.next != NULL)
        statement = statement->commonst.next;
    if (statement != NULL && statement->commonst.kind == RETURN_STMT &&
        statement->returnst.exp != NULL)
    {
        return statement->returnst.exp->common.type;
    }
    return result;
} // HlslOriginalEntryResult

static int HlslIsEmptyEntry(Symbol *program)
{
    stmt *statement;

    if (program->details.fun.params != NULL)
        return 0;
    statement = program->details.fun.statements;
    while (statement != NULL && statement->commonst.kind == COMMENT_STMT)
        statement = statement->commonst.next;
    return IsVoid(program->type->fun.rettype) &&
           (statement == NULL ||
            (statement->commonst.kind == RETURN_STMT &&
             statement->returnst.exp == NULL &&
             statement->commonst.next == NULL));
} // HlslIsEmptyEntry

int HlslLowerProgramWithIR(HlslModule *module,
                     const HlslProfileDesc *profile, SourceLoc *loc,
                     Scope *scope, Symbol *program,
                     const CgIRModule *sourceIR)
{
    HlslLowerContext context;
    HlslFunction *function;
    HlslFunction *helper;
    HlslType result;
    Type *sourceResult;
    const char *name;
    int emptyEntry;
    HlslGeometryInput geometryInput;
    HlslGeometryStream geometryStream;
    int geometryExtent;

    if (module == NULL || profile == NULL || scope == NULL ||
        program == NULL || program->kind != FUNCTION_S ||
        module->stage != profile->stage)
    {
        memset(&context, 0, sizeof(context));
        context.module = module;
        return HlslLowerFailure(&context, HLSL_ERROR_ENTRY_ABI,
                                "HLSL entry program", loc);
    }
    memset(&context, 0, sizeof(context));
    context.module = module;
    context.profile = profile;
    context.scope = scope;
    context.sourceIR = sourceIR;
    context.statementLoc = program->loc;
    context.entryFile = program->loc.file;
    if (!HlslRejectStorage(&context, program))
        return 0;
    if (profile->stage == HLSL_STAGE_GEOMETRY) {
        if (sourceIR == NULL || sourceIR->stage != CGIR_STAGE_GEOMETRY ||
            sourceIR->entry == NULL || sourceIR->entry->symbol != program ||
            sourceIR->geometry == NULL)
        {
            return HlslLowerFailure(&context, HLSL_ERROR_GEOMETRY_LAYOUT,
                                    "verified geometry layout",
                                    &program->loc);
        }
        if (!sourceIR->geometry->hasMaxOutputVertices ||
            sourceIR->geometry->maxOutputVertices == 0)
        {
            return HlslLowerFailure(&context,
                                    HLSL_ERROR_GEOMETRY_MISSING_MAX,
                                    "Vertices=N", &program->loc);
        }
        if (sourceIR->geometry->maxOutputVertices > (unsigned int) INT_MAX ||
            !HlslModernGeometryInput(sourceIR->geometry->inputTopology,
                                     &geometryInput, &geometryExtent) ||
            !HlslModernGeometryStream(sourceIR->geometry->outputTopology,
                                      &geometryStream) ||
            geometryExtent !=
                (int) sourceIR->geometry->inputVertexCount ||
            !HlslSetGeometryLayout(module, geometryInput, geometryStream,
                geometryExtent,
                (int) sourceIR->geometry->maxOutputVertices))
        {
            return HlslLowerFailure(&context, HLSL_ERROR_GEOMETRY_LAYOUT,
                                    "verified geometry layout",
                                    &program->loc);
        }
        context.geometryInputExtent = geometryExtent;
        if (!HlslCollectGeometryOutput(&context))
            return HlslLowerFailure(&context, HLSL_ERROR_INVALID_IR,
                                    "geometry output interface",
                                    &program->loc);
    }
    emptyEntry = profile->stage == HLSL_STAGE_GEOMETRY ? 0 :
                 HlslIsEmptyEntry(program);
    sourceResult = HlslOriginalEntryResult(program);
    if (!HlslEnsureType(&context, sourceResult) ||
        !HlslLowerType(&context, sourceResult, &result, &program->loc) ||
        (profile->stage == HLSL_STAGE_GEOMETRY ?
         !HlslCollectIRCallsInStatements(&context, sourceIR->entry->body) :
         !HlslCollectCallsInStatements(&context,
                                       program->details.fun.statements)))
    {
        return 0;
    }
    if (profile->stage != HLSL_STAGE_GEOMETRY &&
        program->details.fun.geometry.output !=
            CG_GEOMETRY_OUTPUT_UNKNOWN)
    {
        return HlslLowerFailure(&context,
                                HLSL_ERROR_UNSUPPORTED_OPERATION,
                                "geometry output modifier",
                                &program->details.fun.geometry.outputLoc);
    }
    if (profile->stage != HLSL_STAGE_GEOMETRY &&
        program->details.fun.geometry.input !=
            CG_GEOMETRY_INPUT_UNKNOWN)
    {
        return HlslLowerFailure(&context,
                                HLSL_ERROR_UNSUPPORTED_OPERATION,
                                "geometry input modifier",
                                &program->details.fun.geometry.inputLoc);
    }
    if (!HlslCollectUniformList(&context, Cg->theHAL->uniformParam) ||
        !HlslCollectUniformList(&context, Cg->theHAL->uniformGlobal) ||
        !HlslCollectUniformTree(&context, scope->symbols) ||
        !HlslCollectDefaults(&context))
    {
        return 0;
    }
    if (profile->stage != HLSL_STAGE_GEOMETRY) {
        for (helper = module->functions; helper != NULL;
             helper = helper->next)
        {
            if (helper->identity != NULL &&
                !HlslLowerFunction(&context, helper))
            {
                return 0;
            }
        }
    }
    name = emptyEntry ? "main" :
           HlslAllocateGeneratedName(module, program, "cg_entry");
    if (name == NULL)
        return HlslLowerFailure(&context, HLSL_ERROR_INVALID_IR,
                                "HLSL entry name", &program->loc);
    function = HlslNewFunction(module, result, name);
    if (function == NULL)
        return HlslLowerFailure(&context, HLSL_ERROR_INVALID_IR,
                                "HLSL entry allocation", &program->loc);
    function->identity = program;
    function->isEntry = 1;
    function->semantic = HlslFunctionSemantic(&context, program);
    HlslSetLoc(&function->loc, loc != NULL ? loc : &program->loc);
    module->entry = function;
    HlslAppendFunction(&module->functions, function);
    if (profile->stage == HLSL_STAGE_GEOMETRY &&
        !HlslPrepareGeometryFunctions(&context))
    {
        return HlslLowerFailure(&context, HLSL_ERROR_INVALID_IR,
                                "geometry function state", &program->loc);
    }
    if (profile->stage == HLSL_STAGE_GEOMETRY) {
        for (helper = module->functions;
             helper != NULL && helper != function;
             helper = helper->next)
        {
            if (helper->identity != NULL &&
                !HlslLowerFunction(&context, helper))
            {
                return 0;
            }
        }
    }
    context.function = function;
    if (!HlslCollectParameters(&context, program->details.fun.params, 1) ||
        program->details.fun.locals == NULL ||
        !HlslCollectLocals(&context,
                           program->details.fun.locals->symbols) ||
        (!emptyEntry &&
         !(profile->stage == HLSL_STAGE_GEOMETRY ?
           HlslLowerIRStatements(&context,
               sourceIR->entry->body != NULL &&
               sourceIR->entry->body->kind == CGIR_STMT_BLOCK &&
               sourceIR->entry->body->next == NULL ?
                   sourceIR->entry->body->u.block : sourceIR->entry->body,
                                 &function->body) :
           HlslLowerStatements(&context, program->details.fun.statements,
                               &function->body))) ||
        (!emptyEntry &&
         !HlslInitializeReturnedStructs(module, function, function->body)))
    {
        return 0;
    }
    return HlslSortStructs(&context) && module->errors == 0;
} // HlslLowerProgramWithIR

int HlslLowerProgram(HlslModule *module, const HlslProfileDesc *profile,
                     SourceLoc *loc, Scope *scope, Symbol *program)
{
    return HlslLowerProgramWithIR(module, profile, loc, scope, program,
                                  NULL);
} // HlslLowerProgram

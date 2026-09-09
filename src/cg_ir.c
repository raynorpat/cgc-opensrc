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
NON-INFRINGEMENT, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL NVIDIA BE LIABLE FOR ANY SPECIAL,
INDIRECT, INCIDENTAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
\****************************************************************************/

//
// cg_ir.c - Backend-neutral Cg IR ownership and builders.  Every node
//        is allocated through its owning module's allocator callback
//        and zero-filled first, so builders never depend on allocator
//        initialization and dirty allocators are safe.  Allocation
//        failure marks the module failed; failure is sticky and no
//        builder returns a partially initialized node.
//

#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "slglobals.h"
#include "cg_ir.h"

//////////////////////////// Allocation core /////////////////////////////

/*
 * CgIRAllocate() - One module-owned zero-filled block, or NULL after
 *          marking the module failed.
 */

static void *CgIRAllocate(CgIRModule *module, size_t size)
{
    void *memory;

    assert(module != NULL);
    assert(module->alloc != NULL);
    memory = module->alloc(module->allocArg, size);
    if (memory == NULL) {
        module->failed = 1;
        return NULL;
    }
    memset(memory, 0, size);
    return memory;
} // CgIRAllocate

/*
 * CgIRNewExprNode() / CgIRNewStmtNode() - Shared node constructors:
 *          sticky-failure check, one allocation, stable prefix fields.
 */

static CgIRExpr *CgIRNewExprNode(CgIRModule *module, CgIRExprKind kind,
                                 Type *type, const SourceLoc *loc)
{
    CgIRExpr *expr;

    assert(module != NULL);
    assert(type != NULL);
    if (module->failed)
        return NULL;
    expr = (CgIRExpr *) CgIRAllocate(module, sizeof(CgIRExpr));
    if (expr == NULL)
        return NULL;
    expr->kind = kind;
    expr->type = type;
    if (loc != NULL)
        expr->loc = *loc;
    return expr;
} // CgIRNewExprNode

static CgIRStmt *CgIRNewStmtNode(CgIRModule *module, CgIRStmtKind kind,
                                 const SourceLoc *loc)
{
    CgIRStmt *stmt;

    assert(module != NULL);
    if (module->failed)
        return NULL;
    stmt = (CgIRStmt *) CgIRAllocate(module, sizeof(CgIRStmt));
    if (stmt == NULL)
        return NULL;
    stmt->kind = kind;
    if (loc != NULL)
        stmt->loc = *loc;
    return stmt;
} // CgIRNewStmtNode

//////////////////////// Module initialization /////////////////////////

void CgIRInitModule(CgIRModule *module, void *(*alloc)(void *, size_t),
                    void *allocArg)
{
    assert(module != NULL);
    assert(alloc != NULL);
    memset(module, 0, sizeof(*module));
    module->alloc = alloc;
    module->allocArg = allocArg;
} // CgIRInitModule

int CgIRModuleFailed(const CgIRModule *module)
{
    assert(module != NULL);
    return module->failed;
} // CgIRModuleFailed

/*
 * Geometry metadata and operations.
 */

/*
 * lCopyGeometryValues() - Module-owned deep copy of one ordered bundle
 *          list: every node is duplicated through the module
 *          allocator while each copy retains the original semantic
 *          atoms, canonical type, expression reference, and location.
 *          Returns NULL after marking the module failed; partial
 *          copies stay in the failed module's arena.
 */

static CgIRGeometryValue *lCopyGeometryValues(CgIRModule *module,
                                              const CgIRGeometryValue *values)
{
    CgIRGeometryValue *head;
    CgIRGeometryValue *tail;
    const CgIRGeometryValue *cursor;

    assert(module != NULL);
    head = NULL;
    tail = NULL;
    for (cursor = values; cursor != NULL; cursor = cursor->next) {
        CgIRGeometryValue *copy;

        copy = (CgIRGeometryValue *)
               CgIRAllocate(module, sizeof(CgIRGeometryValue));
        if (copy == NULL) {
            return NULL;
        }
        copy->next = NULL;
        copy->canonicalSemantic = cursor->canonicalSemantic;
        copy->sourceSemantic = cursor->sourceSemantic;
        copy->value = cursor->value;
        copy->type = cursor->type;
        copy->loc = cursor->loc;
        if (tail == NULL) {
            head = copy;
        } else {
            tail->next = copy;
        }
        tail = copy;
    }
    return head;
} /* lCopyGeometryValues */

int CgIRSetStage(CgIRModule *module, CgIRStage stage)
{
    assert(module != NULL);
    if ((int) stage < (int) CGIR_STAGE_UNKNOWN ||
        (int) stage > (int) CGIR_STAGE_FRAGMENT)
    {
        return 0;
    }
    if (module->failed)
        return 0;
    module->stage = stage;
    return 1;
} /* CgIRSetStage */

int CgIRSetGeometryInfo(CgIRModule *module,
                        const CgIRGeometryInfo *geometry)
{
    CgIRGeometryInfo *copy;

    assert(module != NULL);
    assert(geometry != NULL);
    if (module->failed)
        return 0;
    copy = (CgIRGeometryInfo *)
           CgIRAllocate(module, sizeof(CgIRGeometryInfo));
    if (copy == NULL)
        return 0;
    *copy = *geometry;
    module->geometry = copy;
    return 1;
} /* CgIRSetGeometryInfo */

CgIRGeometryValue *CgIRNewGeometryValue(CgIRModule *module,
                        int canonicalSemantic, int sourceSemantic,
                        Type *type, CgIRExpr *value, SourceLoc loc)
{
    CgIRGeometryValue *node;

    assert(module != NULL);
    assert(type != NULL);
    if (module->failed)
        return NULL;
    node = (CgIRGeometryValue *)
           CgIRAllocate(module, sizeof(CgIRGeometryValue));
    if (node == NULL)
        return NULL;
    node->next = NULL;
    node->canonicalSemantic = canonicalSemantic;
    node->sourceSemantic = sourceSemantic;
    node->value = value;
    node->type = type;
    node->loc = loc;
    return node;
} /* CgIRNewGeometryValue */

static CgIRStmt *CgIRNewGeometryBundleStmt(CgIRModule *module,
                                           CgIRStmtKind kind,
                                           CgIRGeometryValue *values,
                                           SourceLoc loc)
{
    CgIRStmt *stmt;
    CgIRGeometryValue *copy;

    assert(module != NULL);
    assert(values != NULL);
    if (module->failed)
        return NULL;
    copy = lCopyGeometryValues(module, values);
    if (copy == NULL)
        return NULL;
    stmt = CgIRNewStmtNode(module, kind, &loc);
    if (stmt == NULL)
        return NULL;
    stmt->u.geometry.values = copy;
    return stmt;
} /* CgIRNewGeometryBundleStmt */

CgIRStmt *CgIRNewGeometryEmit(CgIRModule *module,
                              CgIRGeometryValue *values, SourceLoc loc)
{
    return CgIRNewGeometryBundleStmt(module, CGIR_STMT_GEOMETRY_EMIT,
                                     values, loc);
} /* CgIRNewGeometryEmit */

CgIRStmt *CgIRNewGeometryFlat(CgIRModule *module,
                              CgIRGeometryValue *values, SourceLoc loc)
{
    return CgIRNewGeometryBundleStmt(module, CGIR_STMT_GEOMETRY_FLAT,
                                     values, loc);
} /* CgIRNewGeometryFlat */

CgIRStmt *CgIRNewGeometryRestart(CgIRModule *module, SourceLoc loc)
{
    CgIRStmt *stmt;

    assert(module != NULL);
    stmt = CgIRNewStmtNode(module, CGIR_STMT_GEOMETRY_RESTART, &loc);
    if (stmt == NULL)
        return NULL;
    assert(stmt->u.geometry.values == NULL);
    return stmt;
} /* CgIRNewGeometryRestart */

///////////////////// Declarations and functions ///////////////////////

CgIRDecl *CgIRNewDecl(CgIRModule *module, Symbol *symbol, int name,
                      Type *type, CgIRStorage storage, CgIRDomain domain,
                      int semantic, CgIRExpr *initializer,
                      const SourceLoc *loc)
{
    CgIRDecl *decl;

    assert(module != NULL);
    assert(type != NULL);
    if (module->failed)
        return NULL;
    decl = (CgIRDecl *) CgIRAllocate(module, sizeof(CgIRDecl));
    if (decl == NULL)
        return NULL;
    decl->symbol = symbol;
    decl->name = name;
    decl->type = type;
    decl->storage = storage;
    decl->domain = domain;
    decl->semantic = semantic;
    decl->initializer = initializer;
    if (loc != NULL)
        decl->loc = *loc;
    return decl;
} // CgIRNewDecl

CgIRFunction *CgIRNewFunction(CgIRModule *module, Symbol *symbol,
                              Type *resultType, const SourceLoc *loc)
{
    CgIRFunction *function;

    assert(module != NULL);
    assert(resultType != NULL);
    if (module->failed)
        return NULL;
    function = (CgIRFunction *) CgIRAllocate(module, sizeof(CgIRFunction));
    if (function == NULL)
        return NULL;
    function->symbol = symbol;
    function->resultType = resultType;
    if (loc != NULL)
        function->loc = *loc;
    return function;
} // CgIRNewFunction

void CgIRAppendDecl(CgIRDecl **list, CgIRDecl *decl)
{
    assert(list != NULL);
    assert(decl != NULL);
    while (*list)
        list = &(*list)->next;
    *list = decl;
} // CgIRAppendDecl

void CgIRAppendFunction(CgIRFunction **list, CgIRFunction *function)
{
    assert(list != NULL);
    assert(function != NULL);
    while (*list)
        list = &(*list)->next;
    *list = function;
} // CgIRAppendFunction

void CgIRAppendStmt(CgIRStmt **list, CgIRStmt *stmt)
{
    assert(list != NULL);
    assert(stmt != NULL);
    while (*list)
        list = &(*list)->next;
    *list = stmt;
} // CgIRAppendStmt

void CgIRAppendExpr(CgIRExpr **list, CgIRExpr *expr)
{
    assert(list != NULL);
    assert(expr != NULL);
    while (*list)
        list = &(*list)->next;
    *list = expr;
} // CgIRAppendExpr

//////////////////////// Expression builders ///////////////////////////

CgIRExpr *CgIRNewConstant(CgIRModule *module, Type *type,
                          const SourceLoc *loc,
                          const CgNumericValue *value)
{
    CgIRExpr *expr;

    expr = CgIRNewExprNode(module, CGIR_EXPR_CONSTANT, type, loc);
    if (expr == NULL)
        return NULL;
    if (value != NULL)
        expr->u.constant = *value;
    return expr;
} // CgIRNewConstant

CgIRExpr *CgIRNewSymbol(CgIRModule *module, Type *type,
                        const SourceLoc *loc, Symbol *symbol)
{
    CgIRExpr *expr;

    assert(symbol != NULL);
    expr = CgIRNewExprNode(module, CGIR_EXPR_SYMBOL, type, loc);
    if (expr == NULL)
        return NULL;
    expr->u.symbol = symbol;
    return expr;
} // CgIRNewSymbol

CgIRExpr *CgIRNewMember(CgIRModule *module, Type *type,
                        const SourceLoc *loc, CgIRExpr *object,
                        Symbol *member)
{
    CgIRExpr *expr;

    assert(object != NULL);
    assert(member != NULL);
    expr = CgIRNewExprNode(module, CGIR_EXPR_MEMBER, type, loc);
    if (expr == NULL)
        return NULL;
    expr->u.member.object = object;
    expr->u.member.member = member;
    return expr;
} // CgIRNewMember

CgIRExpr *CgIRNewIndex(CgIRModule *module, Type *type,
                       const SourceLoc *loc, CgIRExpr *object,
                       CgIRExpr *index)
{
    CgIRExpr *expr;

    assert(object != NULL);
    assert(index != NULL);
    expr = CgIRNewExprNode(module, CGIR_EXPR_INDEX, type, loc);
    if (expr == NULL)
        return NULL;
    expr->u.index.object = object;
    expr->u.index.index = index;
    return expr;
} // CgIRNewIndex

CgIRExpr *CgIRNewLength(CgIRModule *module, Type *type,
                        const SourceLoc *loc, CgIRExpr *object)
{
    CgIRExpr *expr;

    assert(object != NULL);
    expr = CgIRNewExprNode(module, CGIR_EXPR_LENGTH, type, loc);
    if (expr == NULL)
        return NULL;
    expr->u.length.object = object;
    return expr;
} // CgIRNewLength

CgIRExpr *CgIRNewSwizzle(CgIRModule *module, Type *type,
                         const SourceLoc *loc, CgIRExpr *object,
                         int mask, int componentCount)
{
    CgIRExpr *expr;

    assert(object != NULL);
    expr = CgIRNewExprNode(module, CGIR_EXPR_SWIZZLE, type, loc);
    if (expr == NULL)
        return NULL;
    expr->u.swizzle.object = object;
    expr->u.swizzle.mask = mask;
    expr->u.swizzle.componentCount = componentCount;
    return expr;
} // CgIRNewSwizzle

CgIRExpr *CgIRNewConstruct(CgIRModule *module, Type *type,
                           const SourceLoc *loc, CgIRExpr *arguments)
{
    CgIRExpr *expr;

    expr = CgIRNewExprNode(module, CGIR_EXPR_CONSTRUCT, type, loc);
    if (expr == NULL)
        return NULL;
    expr->u.construct.arguments = arguments;
    return expr;
} // CgIRNewConstruct

CgIRExpr *CgIRNewCast(CgIRModule *module, Type *type,
                      const SourceLoc *loc, CgIRExpr *operand)
{
    CgIRExpr *expr;

    assert(operand != NULL);
    expr = CgIRNewExprNode(module, CGIR_EXPR_CAST, type, loc);
    if (expr == NULL)
        return NULL;
    expr->u.cast.operand = operand;
    return expr;
} // CgIRNewCast

CgIRExpr *CgIRNewUnary(CgIRModule *module, Type *type,
                       const SourceLoc *loc, CgIROp op, CgIRExpr *operand)
{
    CgIRExpr *expr;

    assert(operand != NULL);
    expr = CgIRNewExprNode(module, CGIR_EXPR_UNARY, type, loc);
    if (expr == NULL)
        return NULL;
    expr->u.unary.op = op;
    expr->u.unary.operand = operand;
    return expr;
} // CgIRNewUnary

CgIRExpr *CgIRNewBinary(CgIRModule *module, Type *type,
                        const SourceLoc *loc, CgIROp op, CgIRExpr *left,
                        CgIRExpr *right)
{
    CgIRExpr *expr;

    assert(left != NULL);
    assert(right != NULL);
    expr = CgIRNewExprNode(module, CGIR_EXPR_BINARY, type, loc);
    if (expr == NULL)
        return NULL;
    expr->u.binary.op = op;
    expr->u.binary.left = left;
    expr->u.binary.right = right;
    return expr;
} // CgIRNewBinary

CgIRExpr *CgIRNewAssign(CgIRModule *module, Type *type,
                        const SourceLoc *loc, CgIROp op, CgIRExpr *target,
                        CgIRExpr *value)
{
    CgIRExpr *expr;

    assert(target != NULL);
    assert(value != NULL);
    expr = CgIRNewExprNode(module, CGIR_EXPR_ASSIGN, type, loc);
    if (expr == NULL)
        return NULL;
    expr->u.assign.op = op;
    expr->u.assign.target = target;
    expr->u.assign.value = value;
    return expr;
} // CgIRNewAssign

CgIRExpr *CgIRNewConditional(CgIRModule *module, Type *type,
                             const SourceLoc *loc, CgIRExpr *condition,
                             CgIRExpr *trueExpr, CgIRExpr *falseExpr)
{
    CgIRExpr *expr;

    assert(condition != NULL);
    assert(trueExpr != NULL);
    assert(falseExpr != NULL);
    expr = CgIRNewExprNode(module, CGIR_EXPR_CONDITIONAL, type, loc);
    if (expr == NULL)
        return NULL;
    expr->u.conditional.condition = condition;
    expr->u.conditional.trueExpr = trueExpr;
    expr->u.conditional.falseExpr = falseExpr;
    return expr;
} // CgIRNewConditional

CgIRExpr *CgIRNewCall(CgIRModule *module, Type *type,
                      const SourceLoc *loc, Symbol *callee,
                      CgIRExpr *arguments)
{
    CgIRExpr *expr;

    assert(callee != NULL);
    expr = CgIRNewExprNode(module, CGIR_EXPR_CALL, type, loc);
    if (expr == NULL)
        return NULL;
    expr->u.call.callee = callee;
    expr->u.call.arguments = arguments;
    return expr;
} // CgIRNewCall

CgIRExpr *CgIRNewInterfaceCall(CgIRModule *module, Type *type,
                               const SourceLoc *loc, Symbol *method,
                               CgIRExpr *receiver, CgIRExpr *arguments)
{
    CgIRExpr *expr;

    assert(method != NULL);
    assert(receiver != NULL);
    expr = CgIRNewExprNode(module, CGIR_EXPR_INTERFACE_CALL, type, loc);
    if (expr == NULL)
        return NULL;
    expr->u.interfaceCall.method = method;
    expr->u.interfaceCall.receiver = receiver;
    expr->u.interfaceCall.arguments = arguments;
    return expr;
} // CgIRNewInterfaceCall

CgIRExpr *CgIRNewIntrinsicCall(CgIRModule *module, Type *type,
                               const SourceLoc *loc, CgIntrinsic intrinsic,
                               const CgIntrinsicSignature *signature,
                               CgIRExpr *arguments)
{
    CgIRExpr *expr;

    assert(signature != NULL);
    assert(signature->intrinsic == intrinsic);
    expr = CgIRNewExprNode(module, CGIR_EXPR_INTRINSIC, type, loc);
    if (expr == NULL)
        return NULL;
    expr->u.intrinsicCall.intrinsic = intrinsic;
    expr->u.intrinsicCall.signature = signature;
    expr->u.intrinsicCall.arguments = arguments;
    return expr;
} // CgIRNewIntrinsicCall

//////////////////////// Statement builders ////////////////////////////

CgIRStmt *CgIRNewBlockStmt(CgIRModule *module, const SourceLoc *loc)
{
    return CgIRNewStmtNode(module, CGIR_STMT_BLOCK, loc);
} // CgIRNewBlockStmt

CgIRStmt *CgIRNewDeclStmt(CgIRModule *module, const SourceLoc *loc,
                          CgIRDecl *decl)
{
    CgIRStmt *stmt;

    assert(decl != NULL);
    stmt = CgIRNewStmtNode(module, CGIR_STMT_DECL, loc);
    if (stmt == NULL)
        return NULL;
    stmt->u.decl = decl;
    return stmt;
} // CgIRNewDeclStmt

CgIRStmt *CgIRNewExprStmt(CgIRModule *module, const SourceLoc *loc,
                          CgIRExpr *expression)
{
    CgIRStmt *stmt;

    assert(expression != NULL);
    stmt = CgIRNewStmtNode(module, CGIR_STMT_EXPR, loc);
    if (stmt == NULL)
        return NULL;
    stmt->u.expression = expression;
    return stmt;
} // CgIRNewExprStmt

CgIRStmt *CgIRNewIfStmt(CgIRModule *module, const SourceLoc *loc,
                        CgIRExpr *condition, CgIRStmt *trueBranch,
                        CgIRStmt *falseBranch)
{
    CgIRStmt *stmt;

    assert(condition != NULL);
    assert(trueBranch != NULL);
    stmt = CgIRNewStmtNode(module, CGIR_STMT_IF, loc);
    if (stmt == NULL)
        return NULL;
    stmt->u.ifStmt.condition = condition;
    stmt->u.ifStmt.trueBranch = trueBranch;
    stmt->u.ifStmt.falseBranch = falseBranch;
    return stmt;
} // CgIRNewIfStmt

CgIRStmt *CgIRNewWhileStmt(CgIRModule *module, const SourceLoc *loc,
                           CgIRExpr *condition, CgIRStmt *body)
{
    CgIRStmt *stmt;

    assert(condition != NULL);
    assert(body != NULL);
    stmt = CgIRNewStmtNode(module, CGIR_STMT_WHILE, loc);
    if (stmt == NULL)
        return NULL;
    stmt->u.loop.condition = condition;
    stmt->u.loop.body = body;
    return stmt;
} // CgIRNewWhileStmt

CgIRStmt *CgIRNewDoStmt(CgIRModule *module, const SourceLoc *loc,
                        CgIRExpr *condition, CgIRStmt *body)
{
    CgIRStmt *stmt;

    assert(condition != NULL);
    assert(body != NULL);
    stmt = CgIRNewStmtNode(module, CGIR_STMT_DO, loc);
    if (stmt == NULL)
        return NULL;
    stmt->u.loop.condition = condition;
    stmt->u.loop.body = body;
    return stmt;
} // CgIRNewDoStmt

CgIRStmt *CgIRNewForStmt(CgIRModule *module, const SourceLoc *loc,
                         CgIRStmt *init, CgIRExpr *condition,
                         CgIRExpr *step, CgIRStmt *body)
{
    CgIRStmt *stmt;

    assert(body != NULL);
    stmt = CgIRNewStmtNode(module, CGIR_STMT_FOR, loc);
    if (stmt == NULL)
        return NULL;
    stmt->u.forStmt.init = init;
    stmt->u.forStmt.condition = condition;
    stmt->u.forStmt.step = step;
    stmt->u.forStmt.body = body;
    return stmt;
} // CgIRNewForStmt

CgIRStmt *CgIRNewReturnStmt(CgIRModule *module, const SourceLoc *loc,
                            CgIRExpr *value)
{
    CgIRStmt *stmt;

    stmt = CgIRNewStmtNode(module, CGIR_STMT_RETURN, loc);
    if (stmt == NULL)
        return NULL;
    stmt->u.returnExpr = value;
    return stmt;
} // CgIRNewReturnStmt

CgIRStmt *CgIRNewBreakStmt(CgIRModule *module, const SourceLoc *loc)
{
    return CgIRNewStmtNode(module, CGIR_STMT_BREAK, loc);
} // CgIRNewBreakStmt

CgIRStmt *CgIRNewContinueStmt(CgIRModule *module, const SourceLoc *loc)
{
    return CgIRNewStmtNode(module, CGIR_STMT_CONTINUE, loc);
} // CgIRNewContinueStmt

CgIRStmt *CgIRNewDiscardStmt(CgIRModule *module, const SourceLoc *loc,
                             CgIRExpr *condition)
{
    CgIRStmt *stmt;

    stmt = CgIRNewStmtNode(module, CGIR_STMT_DISCARD, loc);
    if (stmt == NULL)
        return NULL;
    stmt->u.discard.condition = condition;
    return stmt;
} // CgIRNewDiscardStmt

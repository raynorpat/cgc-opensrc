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
INCLUDING WITHOUT LIMITATION, WARRANTIES OF TITLE, NON-INFRINGEMENT,
MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL NVIDIA BE LIABLE FOR ANY SPECIAL, INDIRECT,
INCIDENTAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
\****************************************************************************/

//
// cg_reach.c - Deterministic reachability over the typed frontend tree.
//        Bodies are read-only here: the walk observes calls and global
//        references without rewriting anything.  Membership tests scan
//        the node list linearly; lowered programs carry tens of nodes,
//        so the quadratic behavior never matters at this scale.
//

#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#include "slglobals.h"
#include "cg_reach.h"

///////////////////////////// Graph bookkeeping /////////////////////////////

/*
 * lReachFind() - Slot of "symbol" in the graph, or -1.
 */

static int lReachFind(const CgReachGraph *graph, const Symbol *symbol)
{
    int index;

    for (index = 0; index < graph->nodeCount; index++) {
        if (graph->nodes[index].symbol == symbol)
            return index;
    }
    return -1;
} // lReachFind

/*
 * lReachAdmit() - Admit "symbol" with witness "from" unless it is
 *          already present; returns its slot, or -1 on allocation
 *          failure.  Discovery order is preserved: existing nodes are
 *          never reordered or re-witnessed.
 */

static int lReachAdmit(CgReachGraph *graph, Symbol *symbol, Symbol *from)
{
    CgReachNode *grown;
    int slot;

    slot = lReachFind(graph, symbol);
    if (slot >= 0)
        return slot;
    if (graph->nodeCount >= graph->capacity) {
        int capacity;
        CgReachNode *grown;

        capacity = graph->capacity > 0 ? graph->capacity * 2 : 16;
        /* Parenthesized calls bypass the pool macros in memory.h: the
         * graph owns plain heap storage for its whole lifetime. */
        grown = (CgReachNode *) (realloc)(graph->nodes,
                                          capacity * sizeof(CgReachNode));
        if (grown == NULL) {
            graph->failed = 1;
            return -1;
        }
        graph->nodes = grown;
        graph->capacity = capacity;
    }
    slot = graph->nodeCount++;
    graph->nodes[slot].symbol = symbol;
    graph->nodes[slot].witness.from = from;
    graph->nodes[slot].witness.to = symbol;
    graph->nodes[slot].expanded = 0;
    return slot;
} // lReachAdmit

/////////////////////////////// Body walking ////////////////////////////////

/*
 * lIsGlobalSymbol() - Nonzero when "symbol" names a variable declared
 *          at file scope.  Globals live in GlobalScope's symbol tree,
 *          ordered by bit-reversed atom (lAddToTree); identity, not
 *          name, decides membership so shadowed locals never masquerade
 *          as globals.
 */

static int lIsGlobalSymbol(const Symbol *symbol)
{
    const Symbol *cursor;
    int rev, cursorRev;

    if (symbol == NULL || symbol->kind != VARIABLE_S || GlobalScope == NULL)
        return 0;
    rev = GetReversedAtom(atable, symbol->name);
    cursor = GlobalScope->symbols;
    while (cursor != NULL) {
        cursorRev = GetReversedAtom(atable, cursor->name);
        if (cursorRev == rev)
            return cursor == symbol;
        if (cursorRev > rev)
            cursor = cursor->left;
        else
            cursor = cursor->right;
    }
    return 0;
} // lIsGlobalSymbol

/*
 * lIsLowerableCall() - Nonzero for a direct call whose callee carries a
 *          defined body -- user functions and portable standard-library
 *          definitions alike (those may also carry SYMB_IS_BUILTIN).
 *          Catalog intrinsics without bodies stay out of the IR
 *          function list.
 */

static int lIsLowerableCall(const Symbol *callee)
{
    return callee != NULL && callee->kind == FUNCTION_S &&
           callee->details.fun.statements != NULL;
} // lIsLowerableCall

static void lWalkExpr(CgReachGraph *graph, Symbol *function, expr *fExpr);

/*
 * lWalkCallArguments() - FUN_ARG_OP chains: each link's left operand is
 *          one actual argument and its right operand the next link.
 */

static void lWalkCallArguments(CgReachGraph *graph, Symbol *function,
                               expr *fArgs)
{
    while (fArgs != NULL && fArgs->common.kind == BINARY_N &&
           fArgs->bin.op == FUN_ARG_OP)
    {
        lWalkExpr(graph, function, fArgs->bin.left);
        fArgs = fArgs->bin.right;
    }
} // lWalkCallArguments

static void lWalkExpr(CgReachGraph *graph, Symbol *function, expr *fExpr)
{
    if (fExpr == NULL || graph->failed)
        return;
    switch (fExpr->common.kind) {
    case SYMB_N:
        if (fExpr->sym.op == VARIABLE_OP &&
            lIsGlobalSymbol(fExpr->sym.symbol))
        {
            lReachAdmit(graph, fExpr->sym.symbol, function);
        }
        break;
    case DECL_N:
    case CONST_N:
        break;
    case UNARY_N:
        /* Interface dispatch keeps the method symbolic: no concrete
         * body answers at compile time, so no function edge exists. */
        lWalkExpr(graph, function, fExpr->un.arg);
        break;
    case BINARY_N:
        if ((fExpr->bin.op == FUN_CALL_OP ||
             fExpr->bin.op == FUN_INTRINSIC_OP ||
             fExpr->bin.op == INTERFACE_CALL_OP) &&
            fExpr->bin.left != NULL &&
            fExpr->bin.left->common.kind == SYMB_N &&
            fExpr->bin.left->sym.symbol != NULL &&
            lIsLowerableCall(fExpr->bin.left->sym.symbol))
        {
            lReachAdmit(graph, fExpr->bin.left->sym.symbol, function);
        }
        lWalkExpr(graph, function, fExpr->bin.left);
        lWalkExpr(graph, function, fExpr->bin.right);
        break;
    case TRINARY_N:
        lWalkExpr(graph, function, fExpr->tri.arg1);
        lWalkExpr(graph, function, fExpr->tri.arg2);
        lWalkExpr(graph, function, fExpr->tri.arg3);
        break;
    default:
        assert(!"bad kind walking reachability");
        break;
    }
} // lWalkExpr

static void lWalkStmt(CgReachGraph *graph, Symbol *function, stmt *fStmt);

static void lWalkStmtList(CgReachGraph *graph, Symbol *function, stmt *fStmt)
{
    while (fStmt != NULL && !graph->failed) {
        lWalkStmt(graph, function, fStmt);
        fStmt = fStmt->commonst.next;
    }
} // lWalkStmtList

static void lWalkStmt(CgReachGraph *graph, Symbol *function, stmt *fStmt)
{
    if (fStmt == NULL || graph->failed)
        return;
    switch (fStmt->commonst.kind) {
    case EXPR_STMT:
        lWalkExpr(graph, function, fStmt->exprst.exp);
        break;
    case IF_STMT:
        lWalkExpr(graph, function, fStmt->ifst.cond);
        lWalkStmtList(graph, function, fStmt->ifst.thenstmt);
        lWalkStmtList(graph, function, fStmt->ifst.elsestmt);
        break;
    case WHILE_STMT:
    case DO_STMT:
        lWalkExpr(graph, function, fStmt->whilest.cond);
        lWalkStmtList(graph, function, fStmt->whilest.body);
        break;
    case FOR_STMT:
        lWalkStmtList(graph, function, fStmt->forst.init);
        lWalkExpr(graph, function, fStmt->forst.cond);
        lWalkStmtList(graph, function, fStmt->forst.step);
        lWalkStmtList(graph, function, fStmt->forst.body);
        break;
    case BLOCK_STMT:
        lWalkStmtList(graph, function, fStmt->blockst.body);
        break;
    case RETURN_STMT:
        lWalkExpr(graph, function, fStmt->returnst.exp);
        break;
    case DISCARD_STMT:
        lWalkExpr(graph, function, fStmt->discardst.cond);
        break;
    case COMMENT_STMT:
    case BREAK_STMT:
    case CONTINUE_STMT:
        break;
    default:
        assert(!"bad statement walking reachability");
        break;
    }
} // lWalkStmt

/*
 * lExpandFunction() - Walk one admitted function's body, admitting the
 *          symbols it references.  The callee of a direct call is
 *          admitted before the call's arguments, fixing discovery order
 *          inside an expression.
 */

static void lExpandFunction(CgReachGraph *graph, Symbol *function)
{
    lWalkStmtList(graph, function, function->details.fun.statements);
} // lExpandFunction

///////////////////////////////// Public API /////////////////////////////////

int CgReachBuild(Symbol *entry, CgReachGraph *graph)
{
    int next;

    if (entry == NULL || graph == NULL)
        return 0;
    (free)(graph->nodes);
    graph->nodes = NULL;
    graph->nodeCount = 0;
    graph->capacity = 0;
    graph->failed = 0;

    /* The entry occupies slot zero and expands first.  Its prologue --
     * the global-initialization statements legacy control concatenates
     * ahead of the body -- walks with it. */
    if (lReachAdmit(graph, entry, NULL) < 0)
        return 0;
    graph->nodes[0].expanded = 1;
    lExpandFunction(graph, entry);
    if (GlobalScope != NULL && !graph->failed)
        lWalkStmtList(graph, entry, GlobalScope->initStmts);

    /*
     * Deterministic core: among everything discovered but unexpanded,
     * always expand the smallest sourceOrdinal next.  Ties keep
     * discovery order.  The visited set absorbs recursive cycles.
     */
    while (!graph->failed) {
        next = -1;
        {
            int index;

            for (index = 1; index < graph->nodeCount; index++) {
                if (graph->nodes[index].expanded)
                    continue;
                if (next < 0 ||
                    graph->nodes[index].symbol->sourceOrdinal <
                        graph->nodes[next].symbol->sourceOrdinal)
                {
                    next = index;
                }
            }
        }
        if (next < 0)
            break;
        graph->nodes[next].expanded = 1;
        lExpandFunction(graph, graph->nodes[next].symbol);
    }

    return !graph->failed;
} // CgReachBuild

int CgReachContainsSymbol(const CgReachGraph *graph, const Symbol *symbol)
{
    if (graph == NULL || symbol == NULL)
        return 0;
    return lReachFind(graph, symbol) >= 0;
} // CgReachContainsSymbol

const CgReachEdge *CgReachWitness(const CgReachGraph *graph,
                                  const Symbol *symbol)
{
    int slot;

    if (graph == NULL || symbol == NULL)
        return NULL;
    slot = lReachFind(graph, symbol);
    if (slot < 0)
        return NULL;
    return &graph->nodes[slot].witness;
} // CgReachWitness

void CgReachDestroy(CgReachGraph *graph)
{
    if (graph == NULL)
        return;
    (free)(graph->nodes);
    graph->nodes = NULL;
    graph->nodeCount = 0;
    graph->capacity = 0;
    graph->failed = 0;
} // CgReachDestroy

static CgReachGraph *lActiveGraph = NULL;

void CgReachSetActiveGraph(CgReachGraph *graph)
{
    lActiveGraph = graph;
} // CgReachSetActiveGraph

const CgReachGraph *CgReachActiveGraph(void)
{
    return lActiveGraph;
} // CgReachActiveGraph

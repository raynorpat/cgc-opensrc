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
// cg_reach.h - Deterministic reachable-function/global graph for one
//        selected program entry.  The graph answers exactly which
//        symbols lowering must carry into the IR and records, for each
//        admitted symbol, the first edge that discovered it as its
//        diagnostic witness.
//
// Traversal contract: the entry is admitted and expanded first; every
// later expansion picks the discovered-but-unexpanded symbol with the
// smallest sourceOrdinal, so the result never depends on discovery
// luck.  Walking a function body admits direct callees (defined user
// functions) and referenced global variables.  Recursive cycles are
// absorbed by the visited set -- recursion stays a later profile
// decision, so cycles neither fail the build nor traverse forever.
//
// Nodes appear in the graph in discovery order: the entry occupies
// slot zero.  The graph owns its node array; release it with
// CgReachDestroy().
//

#if !defined(__CG_REACH_H)
#define __CG_REACH_H 1

#include "slglobals.h"

/*
 * CgReachEdge - One parent link: "from" discovered "to".  The entry's
 *          witness uses from == NULL.
 */

typedef struct CgReachEdge_Rec {
    const Symbol *from;
    const Symbol *to;
} CgReachEdge;

typedef struct CgReachNode_Rec {
    Symbol *symbol;
    CgReachEdge witness;
    int expanded;
} CgReachNode;

typedef struct CgReachGraph_Rec {
    CgReachNode *nodes;        // discovery order; entry is nodes[0]
    int nodeCount;
    int capacity;
    int failed;                // sticky allocation-failure flag
} CgReachGraph;

/*
 * CgReachBuild() - Compute the graph reachable from program "entry"
 *          by walking bodies for direct calls and global references.
 *          Rebuilding over an existing graph resets it first.  Returns
 *          nonzero on success; zero when "entry" or "graph" is NULL or
 *          an allocation failed (graph->failed turns sticky).
 */

int CgReachBuild(Symbol *entry, CgReachGraph *graph);

/*
 * CgReachContainsSymbol() - Nonzero when "symbol" is part of the
 *          reachable set.  NULL symbols are never contained.
 */

int CgReachContainsSymbol(const CgReachGraph *graph, const Symbol *symbol);

/*
 * CgReachWitness() - The first parent edge that discovered "symbol"
 *          (from == NULL for the entry), or NULL when the symbol is
 *          not in the graph.
 */

const CgReachEdge *CgReachWitness(const CgReachGraph *graph,
                                  const Symbol *symbol);

/*
 * CgReachDestroy() - Release the graph's node storage and reset it to
 *          the empty state.  Safe on an already-empty graph.
 */

void CgReachDestroy(CgReachGraph *graph);

#endif // !defined(__CG_REACH_H)

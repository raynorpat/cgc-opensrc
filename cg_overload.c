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
NON-INFRINGEMENT, MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE OR
USE, ARE DISCLAIMED. IN NO EVENT SHALL NVIDIA BE LIABLE FOR ANY SPECIAL,
INDIRECT, INCIDENTAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
\****************************************************************************/
// cg_overload.c
//
// Non-mutating ordered overload resolution for Cg 2.0.  The legacy
// resolver stamped match quality into FunSymbol.flags and returned on
// the first argument position that left a single survivor; this
// replacement keeps all working state in a temporary candidate list so
// every argument participates in the ranking before anything is chosen.
//

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "slglobals.h"
#include "cg_overload.h"

/*
 * Argument match tiers, worst to best.  The ordering follows the task
 * ruling: exact unqualified types beat compatible dynamic/interface
 * (and sampler-family) bindings, which beat promotions, which beat
 * ordinary implicit conversions.
 */

#define CG_MATCH_NONE          0
#define CG_MATCH_CONVERSION    1
#define CG_MATCH_PROMOTION     2
#define CG_MATCH_DYNAMIC       3
#define CG_MATCH_EXACT         4

/*
 * Profile selector classes after filtering against the target identity.
 */

#define CG_PROFILE_CLASS_DEAD      0
#define CG_PROFILE_CLASS_OPEN      1
#define CG_PROFILE_CLASS_WILDCARD  2
#define CG_PROFILE_CLASS_EXACT     3

typedef struct CgOverloadCandidateRec {
    struct CgOverloadCandidateRec *next;
    Symbol *sym;
    int alive;
    int matchTier;
    int paramCount;
    int defaultCount;
    int usedDefaults;
    int profileClass;
} CgOverloadCandidate;

/*
 * lIsShapeConversion() - TRUE when binding "from" to "to" would change
 *                        shape between scalar and aggregate categories.
 *                        Shape-introducing conversions stay out of
 *                        overload probing (Task 7 boundary ruling,
 *                        reaffirmed for the ranked resolver): they are
 *                        accepted by assignment semantics but admitting
 *                        them as candidates would let scalar
 *                        replication ties surface as new ambiguities
 *                        across heavily overloaded stdlib names while
 *                        single-candidate calls still reject them at
 *                        binding time.
 */

static int lIsShapeConversion(Type *from, Type *to)
{
    int fromCategory = GetCategory(from);
    int toCategory = GetCategory(to);

    return (fromCategory == TYPE_CATEGORY_SCALAR &&
            toCategory == TYPE_CATEGORY_ARRAY) ||
           (fromCategory == TYPE_CATEGORY_ARRAY &&
            toCategory == TYPE_CATEGORY_SCALAR);
} // lIsShapeConversion

/*
 * lExactOrUnsizedCompat() - The exact-shape fast paths of ConvertType:
 *                          identical unqualified types with matching
 *                          packedness, or a concrete array binding an
 *                          unsized array formal of the same element
 *                          shape.
 */

static int lExactOrUnsizedCompat(Type *formal, Type *actual)
{
    if (IsSameUnqualifiedType(formal, actual)) {
        if (IsPacked(formal) == IsPacked(actual)) {
            return 1;
        }
    }
    if (GetCategory(formal) == TYPE_CATEGORY_ARRAY &&
        GetCategory(actual) == TYPE_CATEGORY_ARRAY &&
        IsUnsizedArray(formal) &&
        IsPacked(formal) == IsPacked(actual) &&
        IsSameUnqualifiedType(formal->arr.eltype, actual->arr.eltype))
    {
        return 1;
    }
    return 0;
} // lExactOrUnsizedCompat

/*
 * lArgumentMatch() - Rank one actual against one formal.  Returns a
 *                    CG_MATCH tier, or CG_MATCH_NONE when the formal
 *                    cannot bind the actual at all.
 */

static int lArgumentMatch(Type *formal, Type *actual)
{
    CgSamplerKind formalKind, actualKind;
    CgConversionRank rank;

    if (!formal || !actual)
        return CG_MATCH_NONE;
    if (lExactOrUnsizedCompat(formal, actual))
        return CG_MATCH_EXACT;
    /* Boundary ruling: no scalar<->aggregate shape conversions as
     * ranked candidates (see lIsShapeConversion). */
    if (lIsShapeConversion(actual, formal))
        return CG_MATCH_NONE;
    /* Sampler-family compatibility is the dynamic tier for opaque
     * samplers: any specific sampler binds a deprecated base-sampler
     * formal; only identical kinds bind each other. */
    if (IsSampler(formal, &formalKind) && IsSampler(actual, &actualKind) &&
        CgSamplerCompatible(formalKind, actualKind))
    {
        return CG_MATCH_DYNAMIC;
    }
    rank = CgClassifyConversion(actual, formal, 0);
    switch (rank) {
    case CG_CONVERSION_DYNAMIC:
        return CG_MATCH_DYNAMIC;
    case CG_CONVERSION_PROMOTION:
        return CG_MATCH_PROMOTION;
    case CG_CONVERSION_IMPLICIT:
    case CG_CONVERSION_IMPLICIT_WARN:
        return CG_MATCH_CONVERSION;
    default:
        return CG_MATCH_NONE;
    }
} // lArgumentMatch

/*
 * lBuildCandidates() - Snapshot "first"'s overload chain into temporary
 *                      storage.  malloc() resolves through memory.h to
 *                      the current scope's pool when compiling, so the
 *                      list dies with the parse scope; standalone
 *                      harnesses fall back to the heap.
 */

static CgOverloadCandidate *lBuildCandidates(Symbol *first)
{
    CgOverloadCandidate *head = NULL, **tail = &head;
    Symbol *lSymb;

    for (lSymb = first; lSymb; lSymb = lSymb->details.fun.overload) {
        CgOverloadCandidate *cand =
            (CgOverloadCandidate *) malloc(sizeof(CgOverloadCandidate));
        TypeList *formals;
        Symbol *params;
        int trailingDefaults;

        assert(cand);
        cand->sym = lSymb;
        cand->alive = 1;
        cand->matchTier = CG_MATCH_NONE;
        cand->profileClass = CG_PROFILE_CLASS_DEAD;

        cand->paramCount = 0;
        for (formals = lSymb->type->fun.paramtypes; formals;
             formals = formals->next)
        {
            /* A lone void formal is the language's spelling of "no
             * parameters" and never participates in binding. */
            if (IsVoid(formals->type)) {
                break;
            }
            cand->paramCount++;
        }

        /* Defaults are validated to be trailing at declaration time;
         * counting the contiguous run from the end keeps stdlib or
         * hand-built symbol lists honest regardless. */
        trailingDefaults = 0;
        for (params = lSymb->details.fun.params; params; params = params->next) {
            if (params->details.var.init) {
                trailingDefaults++;
            } else {
                trailingDefaults = 0;
            }
        }
        cand->defaultCount = trailingDefaults;
        cand->usedDefaults = 0;

        cand->next = NULL;
        *tail = cand;
        tail = &cand->next;
    }
    return head;
} // lBuildCandidates

/*
 * lFreeCandidates() - Release the temporary list.  Pool-allocated
 *                     nodes vanish with the scope anyway; freeing here
 *                     also does no harm because pool blocks are not
 *                     individually reclaimed, so this is a no-op under
 *                     the pool allocator and exact under the heap.
 */

static void lFreeCandidates(CgOverloadCandidate *cands)
{
    while (cands) {
        CgOverloadCandidate *dead = cands;

        cands = cands->next;
        free(dead);
    }
} // lFreeCandidates

/*
 * lProfileSelectorClass() - Classify one candidate's stored selector
 *                           against the target identity.
 */

static void lProfileSelectorClass(CgOverloadCandidate *cand,
                                  const CgProfileIdentity *profile)
{
    const CgProfileSelector *sel;
    int ii;

    sel = &cand->sym->details.fun.profileSelector;
    if (!profile || sel->isOpen || sel->name == 0) {
        cand->profileClass = CG_PROFILE_CLASS_OPEN;
        return;
    }
    if (sel->name == profile->exactName) {
        cand->profileClass = CG_PROFILE_CLASS_EXACT;
        return;
    }
    for (ii = 0; ii < profile->wildcardCount; ii++) {
        if (profile->wildcards && profile->wildcards[ii] == sel->name) {
            cand->profileClass = CG_PROFILE_CLASS_WILDCARD;
            cand->matchTier = profile->specificity ?
                              profile->specificity[ii] : 0;
            return;
        }
    }
    cand->profileClass = CG_PROFILE_CLASS_DEAD;
} // lProfileSelectorClass

int CgResolveOverload(const CgProfileIdentity *profile,
                      struct Symbol_Rec *first,
                      union expr_rec *actuals,
                      CgOverloadResult *result)
{
    CgOverloadCandidate *cands, *cand, *best;
    expr *arg;
    int argno, actualCount, bestTier, bestClass, bestWildcard;

    memset(result, 0, sizeof(*result));
    result->symbol = NULL;
    result->ambiguous = 0;

    cands = lBuildCandidates(first);
    if (!cands) {
        return 0;
    }

    /*
     * Phase 1 - rank arguments left to right.  At each position every
     * surviving candidate must still bind the actual; candidates that
     * bind it better than the rest keep the set, worse ones drop.
     */

    actualCount = 0;
    argno = 0;
    for (arg = actuals; arg; arg = arg->bin.right, argno++) {
        Type *actualType = arg->common.type;

        actualCount++;
        bestTier = CG_MATCH_NONE;
        for (cand = cands; cand; cand = cand->next) {
            TypeList *formals;
            int ii;

            if (!cand->alive) {
                continue;
            }
            formals = cand->sym->type->fun.paramtypes;
            for (ii = 0; ii < argno && formals; ii++) {
                formals = formals->next;
            }
            if (!formals) {
                /* Ran out of formals: too many actuals for this
                 * candidate even before defaults could help. */
                cand->alive = 0;
                continue;
            }
            cand->matchTier = lArgumentMatch(formals->type, actualType);
            if (cand->matchTier == CG_MATCH_NONE) {
                cand->alive = 0;
            } else if (cand->matchTier > bestTier) {
                bestTier = cand->matchTier;
            }
        }
        if (bestTier == CG_MATCH_NONE) {
            /* Nothing binds this argument: no candidate works. */
            lFreeCandidates(cands);
            return 0;
        }
        for (cand = cands; cand; cand = cand->next) {
            if (cand->alive && cand->matchTier < bestTier) {
                cand->alive = 0;
            }
        }
    }

    /*
     * Phase 2 - arity and defaults.  Extra formals beyond the supplied
     * actuals must be covered by the candidate's own trailing defaults.
     */

    for (cand = cands; cand; cand = cand->next) {
        if (!cand->alive) {
            continue;
        }
        cand->usedDefaults = cand->paramCount - actualCount;
        if (cand->usedDefaults > cand->defaultCount || cand->usedDefaults < 0) {
            cand->alive = 0;
        }
    }

    /*
     * Phase 3 - profile selectors: exact beats most-specific wildcard
     * beats open.  Candidates whose selector cannot name the target
     * profile at all were never usable.
     */

    bestClass = CG_PROFILE_CLASS_DEAD;
    for (cand = cands; cand; cand = cand->next) {
        if (!cand->alive) {
            continue;
        }
        lProfileSelectorClass(cand, profile);
        if (cand->profileClass == CG_PROFILE_CLASS_DEAD) {
            cand->alive = 0;
        } else if (cand->profileClass > bestClass) {
            bestClass = cand->profileClass;
        }
    }
    if (bestClass != CG_PROFILE_CLASS_DEAD) {
        bestWildcard = -1;
        for (cand = cands; cand; cand = cand->next) {
            if (!cand->alive) {
                continue;
            }
            if (cand->profileClass < bestClass) {
                cand->alive = 0;
            } else if (cand->profileClass == CG_PROFILE_CLASS_WILDCARD) {
                /* Within the wildcard class the HAL-provided integer
                 * picks the most specific expansion. */
                if (bestWildcard < 0 || cand->matchTier > bestWildcard) {
                    bestWildcard = cand->matchTier;
                }
            }
        }
        if (bestWildcard >= 0) {
            for (cand = cands; cand; cand = cand->next) {
                if (cand->alive &&
                    cand->profileClass == CG_PROFILE_CLASS_WILDCARD &&
                    cand->matchTier < bestWildcard)
                {
                    cand->alive = 0;
                }
            }
        }
    }

    /*
     * Verdict.
     */

    best = NULL;
    for (cand = cands; cand; cand = cand->next) {
        if (cand->alive) {
            if (best) {
                result->ambiguous = 1;
                result->symbol = NULL;
                result->usedDefaults = 0;
                lFreeCandidates(cands);
                return 0;
            }
            best = cand;
        }
    }
    if (best) {
        result->symbol = best->sym;
        result->usedDefaults = best->usedDefaults;
        result->ambiguous = 0;
        lFreeCandidates(cands);
        return 1;
    }
    lFreeCandidates(cands);
    return 0;
} // CgResolveOverload

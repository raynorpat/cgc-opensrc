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
// cg_overload.h
//
// Profile identities, profile selectors, and the non-mutating ordered
// overload resolver for Cg 2.0.  The types here are consumed by the
// symbol table (FunSymbol.profileSelector) and the hardware
// abstraction layer (slHAL.profileIdentity), so this header defines
// them standalone and refers to symbols through their elaborated
// tags; both symbols.h and hal.h include it before using them.
//

#if !defined(__CG_OVERLOAD_H)
#define __CG_OVERLOAD_H 1

union expr_rec;

/*
 * CgProfileStage - coarse pipeline stage a profile serves.  Neutral
 * profiles (generic) impose no stage constraint on selector matching.
 */

typedef enum CgProfileStage_Rec {
    CG_PROFILE_STAGE_NEUTRAL = 0,
    CG_PROFILE_STAGE_VERTEX,
    CG_PROFILE_STAGE_GEOMETRY,
    CG_PROFILE_STAGE_FRAGMENT
} CgProfileStage;

/*
 * Selector specificity ordering.  An exact profile name outranks every
 * wildcard; wildcards are ordered among themselves by the HAL-provided
 * integer; an omitted selector (open) matches everything and loses to
 * both.
 */

#define CG_PROFILE_OPEN_SPECIFICITY       0
#define CG_PROFILE_EXACT_SPECIFICITY    1000

/*
 * CgProfileSelector - the profile qualification recorded on a function
 * declaration.  A declaration with no specifier stores the open
 * selector (isOpen = 1); otherwise "name" holds either an exact
 * profile atom or a registered wildcard atom.
 */

typedef struct CgProfileSelector_Rec {
    int name;
    int specificity;
    int isOpen;
} CgProfileSelector;

/*
 * CgProfileIdentity - what is known about the profile currently being
 * compiled.  "exactName" is this profile's own atom; "wildcards" lists
 * the selector atoms this profile expands from (e.g. glslv answers to
 * "vs"), paired positionally with "specificity".
 */

typedef struct CgProfileIdentity_Rec {
    int exactName;
    CgProfileStage stage;
    const int *wildcards;
    const int *specificity;
    int wildcardCount;
} CgProfileIdentity;

/*
 * CgOverloadResult - outcome of one resolution attempt.  On success
 * "symbol" names the selected overload and "usedDefaults" counts how
 * many trailing parameters were filled from stored defaults.  On
 * ambiguity "ambiguous" is set and "symbol" is NULL.
 */

typedef struct CgOverloadResult_Rec {
    struct Symbol_Rec *symbol;
    int usedDefaults;
    int ambiguous;
} CgOverloadResult;

/*
 * CgResolveOverload() - Select one overload from "first"'s overload
 *          chain for the given target profile and argument list.
 *
 *          Ranking order per argument, left to right: exact
 *          unqualified type, compatible dynamic/interface (and
 *          sampler-family) type, promotion, then implicit conversion.
 *          After the arguments: arity/default feasibility, then exact
 *          profile, most-specific wildcard, then open.  Returns 1 with
 *          "result" filled when exactly one candidate survives, and 0
 *          otherwise ("ambiguous" distinguishes ties from no match).
 *
 *          Resolution never mutates the candidates: FunSymbol.flags is
 *          left untouched and all working state lives in temporary
 *          pool-backed storage.
 */

int CgResolveOverload(const CgProfileIdentity *profile,
                      struct Symbol_Rec *first,
                      union expr_rec *actuals,
                      CgOverloadResult *result);

#endif // !defined(__CG_OVERLOAD_H)

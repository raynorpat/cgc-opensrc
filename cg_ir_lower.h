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
// cg_ir_lower.h - Lower the typed frontend tree of one reachable
//        program into verified-shaped Cg IR.  Lowering is one-for-one:
//        canonical types, resolved symbols, intrinsic signatures,
//        binding semantics, lvalue status, and side-effect flags ride
//        along unchanged, while array copies, dynamic-array
//        assignment, interface dispatch, constructors, swizzles, and
//        structured control flow stay explicit.  Nothing here runs the
//        legacy inliner or any flattening pass.
//
// Visibility authority: every visible local routes through a DECL
// statement; parameters and module globals declare themselves.  Method
// receivers are never cloned -- interface-call nodes keep the receiver
// in its own field and bind arguments to the declared formals alone.
//

#if !defined(__CG_IR_LOWER_H)
#define __CG_IR_LOWER_H 1

#include "slglobals.h"
#include "cg_ir.h"
#include "cg_reach.h"

typedef struct CgIRLowerContext_Rec {
    CgIRModule *module;
    const CgReachGraph *reach;
    /* Borrowed selected-program analysis state: resolved geometry
     * configuration, attribute-array type views, and operation
     * records.  NULL when no analysis ran or the profile is not
     * geometry-enabled. */
    const CgGeometryProgram *geometry;
    CgIRVerifyDiagnostic verifyDiagnostic;
} CgIRLowerContext;

/*
 * CgIRLowerProgram() - Lower "entry" plus everything "reach" admits
 *          into "context->module": globals in discovery order,
 *          functions headed by the entry.  Global initialization
 *          statements run as part of the entry body, matching the
 *          legacy prologue concatenation.  The module's program stage
 *          comes from "geometry"->config when one is supplied and from
 *          the live profile identity otherwise; a supplied geometry
 *          stage also installs the module-owned metadata copy before
 *          any declaration lowers.  Geometry operation statements are
 *          intercepted through "geometry"'s records, so the special
 *          intrinsic calls never reach Cg IR.  Returns nonzero when
 *          the module finished without allocation failure; the caller
 *          must still verify with CgIRVerifyModule before any other
 *          use.
 */

int CgIRLowerProgram(CgIRLowerContext *context, Scope *globalScope,
                     Symbol *entry, const CgGeometryProgram *geometry);

/* Copy parsed uniform-default data into caller-owned IR storage. */
CgIRExpr *CgIRLowerUniformDefault(CgIRModule *module, const CgIRDecl *decl);

#endif // !defined(__CG_IR_LOWER_H)

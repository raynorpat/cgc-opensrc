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
INCLUDING WITHOUT LIMITATION, WARRANTIES OF CONDITIONS OF TITLE,
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
// cg_stdlib.h - Stable intrinsic identities for the declarative Cg 2.0
//        standard library.  The catalog rows live in cg_stdlib.def;
//        cg_stdlib.c expands them into immutable signatures installed
//        as ordinary internal function symbols during standard-library
//        initialization.
//
// This header is deliberately NOT included from symbols.h: the symbol
// table only forward-declares struct CgIntrinsicSignature_Rec and
// stores a const pointer to it in FunSymbol, which avoids the header
// cycle while keeping the selected signature immutable.
//

#if !defined(__CG_STDLIB_H)
#define __CG_STDLIB_H 1

#include "symbols.h"

/*
 * Fixed behavior flags.  They describe how a profile may treat an
 * intrinsic: pure results, constant-foldable arguments, derivative
 * instructions, texture fetches, out-parameter forms, and side
 * effects that must survive dead-code elimination.
 */

#define CG_INTRINSIC_PURE          0x00000001
#define CG_INTRINSIC_FOLDABLE      0x00000002
#define CG_INTRINSIC_DERIVATIVE    0x00000004
#define CG_INTRINSIC_TEXTURE       0x00000008
#define CG_INTRINSIC_OUT_PARAMS    0x00000010
#define CG_INTRINSIC_SIDE_EFFECTS  0x00000020

typedef enum CgIntrinsic_Rec {
    CG_INTRINSIC_NONE = 0,
#define CG_INTRINSIC(id, name, flags) CG_INTRINSIC_##id,
#include "cg_stdlib.def"
#undef CG_INTRINSIC
    CG_INTRINSIC_COUNT
} CgIntrinsic;

typedef struct CgIntrinsicSignature_Rec {
    CgIntrinsic intrinsic;
    const char *name;
    Type *result;
    TypeList *parameters;
    unsigned flags;
} CgIntrinsicSignature;

/*
 * InitCgStdlib() - Expand the catalog into "scope": one internal
 *          function symbol per expanded signature carrying its
 *          immutable signature pointer, plus the helper-structure
 *          variants selected by the current profile family.  Repeated
 *          installation over the same scope is idempotent.
 */

int InitCgStdlib(Scope *scope);

/*
 * CgIntrinsicSignatureForSymbol() - The immutable catalog signature of
 *          an installed intrinsic symbol, or NULL for any other
 *          symbol (including user-declared internal functions).
 */

const CgIntrinsicSignature *CgIntrinsicSignatureForSymbol(
    const Symbol *symbol);

/*
 * CgStdlibCatalogCount() / CgStdlibCatalogName() - Enumerate the
 *          distinct intrinsic names of the pinned catalog, one entry
 *          per stable opcode, in def-row order.
 */

int CgStdlibCatalogCount(void);
const char *CgStdlibCatalogName(int index);

/*
 * CgStdlibHelperCount() / CgStdlibHelperName() /
 * CgStdlibHelperStage() - Enumerate the predefined helper-structure
 *          rows together with the profile family stage that selects
 *          them during installation.
 */

int CgStdlibHelperCount(void);
const char *CgStdlibHelperName(int index);
CgProfileStage CgStdlibHelperStage(int index);

#endif // !defined(__CG_STDLIB_H)

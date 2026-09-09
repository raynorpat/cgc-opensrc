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

#ifndef __MSL_LOWER_INTERNAL_H
#define __MSL_LOWER_INTERNAL_H 1
#include <ctype.h>
#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include "slglobals.h"
#include "cg_ir.h"
#include "cg_ir_lower.h"
#include "msl_hal.h"
typedef struct DeclMap_Rec {
    struct DeclMap_Rec *next; Symbol *source; MslDecl *target;
} DeclMap;
typedef struct FunMap_Rec {
    struct FunMap_Rec *next; const CgIRFunction *source; MslFunction *target;
} FunMap;
typedef struct TypeMap_Rec { struct TypeMap_Rec *next; const Type *source; MslRecord *target; int building; } TypeMap;
typedef struct Lower_Rec {
    MslModule *m; MslFunction *current; MslStmt *temporaries; TypeMap *types; DeclMap *decls; FunMap *functions; int ordinal;
} Lower;

MslType MslLLowerRecord(Lower *l, const Type *t, const SourceLoc *loc);
MslType MslLLowerArray(Lower *l,const Type *t,const SourceLoc *loc);
MslType MslLLowerType(Lower *l, const Type *t, const SourceLoc *loc);
const char *MslLSourceName(Lower *,const char *,const char *);
const char *MslLName(Lower *l, const char *prefix);
MslDecl *MslLFindDecl(Lower *l, Symbol *s);
MslFunction *MslLFindFunction(Lower *l, Symbol *s);
MslDecl *MslLLowerDecl(Lower *l, const CgIRDecl *s);
MslDecl *MslLLowerDecls(Lower *l, const CgIRDecl *s);
const char *MslLOp(CgIROp op);
const char *MslLIntrinsic(Lower *l, const CgIRExpr *e);
MslExpr *MslLArgs(Lower *l, const CgIRExpr *s);
MslExpr *MslLLowerTexture(Lower *,const CgIRExpr *,MslExpr *,int);
MslExpr *MslLMaterialize(Lower *,MslExpr *,MslExpr ***);
MslExpr *MslLExpandOnce(Lower *,MslExpr *,int);
MslExpr *MslLTemporary(Lower *l,MslType type,const SourceLoc *loc);
MslExpr *MslLCopyExpr(Lower *l,const MslExpr *e);
MslExpr *MslLStore(Lower *l,MslExpr *to,MslExpr *from);
void MslLSequenceAppend(MslExpr ***tail,MslExpr *e);
int MslLStabilize(Lower *l,MslExpr *e,MslExpr ***tail);
MslExpr *MslLCopyCall(Lower *l,MslExpr *call);
MslExpr *MslLConvert(Lower *, MslExpr *, MslType);
MslExpr *MslLLowerExpr(Lower *l, const CgIRExpr *s);
int MslLSameExpr(const CgIRExpr *a,const CgIRExpr *b);
int MslLSelectorGroup(const CgIRStmt *s);
MslStmt *MslLLowerStmt(Lower *l, const CgIRStmt *s);
const char *MslLSemantic(Lower *l, int atom, const SourceLoc *loc);
int MslLVisitExpr(MslModule *m, MslExpr *e);
int MslLVisitStmt(MslModule *m, MslStmt *s);
int MslLVisitFunction(MslModule *m, MslFunction *f);
int MslLInterface(Lower *l, MslInterface **list, MslType type,
                     const char *path, const char *semantic, const SourceLoc *loc, int output);
int MslLowerCgIR(MslModule *m, const MslProfileDesc *profile, const CgIRModule *source);
#endif

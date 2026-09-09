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
NON-INFRINGEMENT, MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, OR
ITS USE AND OPERATION EITHER ALONE OR IN COMBINATION WITH OTHER
PRODUCTS.

IN NO EVENT SHALL NVIDIA BE LIABLE FOR ANY SPECIAL, INDIRECT,
INCIDENTAL, EXEMPLARY, CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
TO, LOST PROFITS; PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) OR ARISING IN ANY WAY
OUT OF THE USE, REPRODUCTION, MODIFICATION AND/OR DISTRIBUTION OF THE
NVIDIA SOFTWARE, HOWEVER CAUSED AND WHETHER UNDER THEORY OF CONTRACT,
TORT (INCLUDING NEGLIGENCE), STRICT LIABILITY OR OTHERWISE, EVEN IF
NVIDIA HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
\****************************************************************************/

#include "msl_lower_internal.h"

MslExpr *MslLLowerTexture(Lower *l,const CgIRExpr *s,MslExpr *e,int explicitLod)
{
    MslExpr *coord,*sample,*seq,*store,*uv,*lod; MslType type;
    if(explicitLod) {
        coord=MslLLowerExpr(l,s->u.call.arguments->next);
        if(!coord) return e;
        sample=e; sample->kind=MSL_SAMPLE; sample->a=MslLLowerExpr(l,s->u.call.arguments);
        seq=(MslExpr *)MslAlloc(l->m,sizeof(*seq));
        uv=(MslExpr *)MslAlloc(l->m,sizeof(*uv)); lod=(MslExpr *)MslAlloc(l->m,sizeof(*lod));
        if(!seq || !uv || !lod) return e;
        type=coord->type; e=MslLTemporary(l,type,&s->loc); if(!e) return NULL;
        store=MslLStore(l,MslLCopyExpr(l,e),coord);
        uv->kind=MSL_SWIZZLE; uv->type=type;
        uv->type.lanes=s->u.call.callee->details.fun.index==MSL_BUILTIN_TEX2DLOD?2:3;
        uv->text=uv->type.lanes==2?"xy":"xyz"; uv->a=MslLCopyExpr(l,e);
        lod->kind=MSL_SWIZZLE; lod->type=type; lod->type.lanes=1; lod->text="w"; lod->a=e;
        sample->b=uv; sample->c=lod;
        seq->kind=MSL_SEQUENCE; seq->type=sample->type; seq->loc=s->loc;
        seq->a=store; if(store) store->next=sample; return seq;
    }
    if(!s->u.intrinsicCall.arguments || !s->u.intrinsicCall.arguments->next || s->u.intrinsicCall.arguments->next->next) {
        MslFail(l->m,6602,&s->loc,"gradient texture signatures are unsupported"); return e;
    }
    if(l->m->stage!=MSL_FRAGMENT) { MslFail(l->m,6602,&s->loc,"implicit texture sampling requires fragment stage"); return e; }
    e->kind=MSL_SAMPLE; e->a=MslLLowerExpr(l,s->u.intrinsicCall.arguments);
    e->b=MslLLowerExpr(l,s->u.intrinsicCall.arguments->next);
    if(!e->a || e->a->kind!=MSL_SYMBOL || e->a->type.base<MSL_TEXTURE2D)
        MslFail(l->m,6602,&s->loc,"sampling requires a resolved texture/sampler pair");
    return e;
}

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

#include "msl_lower_internal.h"

int MslLowerCgIR(MslModule *m, const MslProfileDesc *profile, const CgIRModule *source)
{
    Lower l; FunMap *p, **mapTail; MslFunction **tail;
    const CgIRFunction *f; const CgIRDecl *d; MslDecl *q, *r;
    char exportName[256]; int used[16], slot, i; const char *semantic;
    memset(&l, 0, sizeof(l)); memset(used, 0, sizeof(used)); l.m = m;
    if (!source || !source->entry || source->stage !=
        (profile->stage == MSL_VERTEX ? CGIR_STAGE_VERTEX : CGIR_STAGE_FRAGMENT))
        return MslFail(m, 6606, NULL, "Metal requires the selected vertex or fragment stage");
    m->sourceEntry = MslString(m, GetAtomString(atable, source->entry->symbol->name));
    if (m->failed) return 0;
    if (strlen(m->sourceEntry) > 200) return MslFail(m, 6603, NULL, "entry name too long");
    sprintf(exportName, "cg_%s_%s", profile->name, m->sourceEntry);
    m->exportName = MslString(m, exportName);
    m->globals=MslLLowerDecls(&l,source->globals);
    for(q=m->globals;q && !m->failed;q=q->next) {
        char qualified[1024];
        if(strlen(q->sourceName)>990) return MslFail(m,6604,&q->loc,"global name too long");
        sprintf(qualified,"global.%s",q->sourceName); q->sourceName=MslString(m,qualified); q->readOnly=1;
    }
    tail = &m->functions; mapTail = &l.functions;
    for (f = source->functions; f && !m->failed; f = f->next) {
        p = (FunMap *) MslAlloc(m, sizeof(*p)); if (!p) break;
        p->source = f; p->target = (MslFunction *) MslAlloc(m, sizeof(*p->target));
        if (!p->target) break;
        *mapTail = p; mapTail = &p->next; *tail = p->target; tail = &p->target->next;
        p->target->name = MslLName(&l, "fn"); p->target->loc = f->loc;
        p->target->result = MslLLowerType(&l, f->resultType, &f->loc);
        if(p->target->result.base>=MSL_TEXTURE2D) MslFail(m,6601,&f->loc,"sampler return values are unsupported");
        p->target->entry = f == source->entry; p->target->globals=m->globals;
        p->target->parameters = MslLLowerDecls(&l, f->parameters);
        p->target->locals = MslLLowerDecls(&l, f->locals);
        if (p->target->entry) m->entry = p->target;
    }
    if (m->failed) return 0;
    for (p = l.functions; p && !m->failed; p = p->next) {
        l.current=p->target; l.temporaries=NULL;
        p->target->body = MslLLowerStmt(&l, p->source->body);
        if(l.temporaries && !m->failed) {
            MslStmt *last=l.temporaries;
            while(last->next) last=last->next;
            last->next=p->target->body->body;
            p->target->body->body=l.temporaries;
        }
    }
    if (m->failed) return 0;
    if (!MslLVisitFunction(m,m->entry)) return 0;
    semantic = NULL;
    if(m->entry->result.base!=MSL_RECORD && m->entry->result.base!=MSL_VOID)
        semantic=MslLSemantic(&l,source->entry->symbol->details.fun.semantics,&source->entry->loc);
    if(m->failed) return 0;
    if(m->entry->result.base!=MSL_VOID && !MslLInterface(&l,&m->outputs,m->entry->result,"cg_value",semantic,&source->entry->loc,1)) return 0;
    d=source->entry->parameters;
    for(q=m->entry->parameters;q && !m->failed;q=q->next,d=d->next) {
        if(q->type.base>=MSL_TEXTURE2D) {
            q->resourceSlot=-2;
            if(d->semantic) {
                semantic=MslLSemantic(&l,d->semantic,&d->loc);
                if(m->failed) return 0;
                for(i=0;i<16;i++) { char binding[20]; sprintf(binding,"TEXUNIT%d",i);
                    if(!strcmp(binding,semantic)) q->resourceSlot=i;
                }
                if(q->resourceSlot<0) return MslFail(m,6604,&d->loc,"sampler binding requires TEXUNIT0..15");
                if(used[q->resourceSlot]) return MslFail(m,6604,&d->loc,"duplicate texture/sampler binding");
                used[q->resourceSlot]=1;
            }
            continue;
        }
        if(d->domain==CGIR_DOMAIN_UNIFORM) {
            if(d->semantic) return MslFail(m,6604,&d->loc,"explicit numeric uniform bindings are unsupported");
            q->uniformSlot=0; continue;
        }
        semantic=NULL;
        if(q->type.base!=MSL_RECORD) semantic=MslLSemantic(&l,d->semantic,&d->loc);
        if(m->failed) return 0;
        if(q->direction!=1 && !MslLInterface(&l,&m->inputs,q->type,q->name,semantic,&d->loc,0)) return 0;
        if(q->direction && !MslLInterface(&l,&m->outputs,q->type,q->name,semantic,&d->loc,1)) return 0;
    }
    if(profile->stage==MSL_VERTEX) {
        MslInterface *v;
        for(v=m->outputs;v && v->builtin!=1;v=v->next) {}
        if(!v) return MslFail(m,6603,&m->entry->loc,"Metal vertex output requires POSITION");
    }
    if(!m->outputs) return MslFail(m,6603,&m->entry->loc,"Metal entry requires an output");
    { MslInterface *v,*lowest; int assigned[16]; memset(assigned,0,sizeof(assigned));
      for(v=m->inputs;v;v=v->next) if(v->attribute>=0) assigned[v->attribute]=1;
      for(;;) {
        lowest=NULL;
        for(v=m->inputs;v;v=v->next) if(v->attribute==-2 && (!lowest || strcmp(v->semantic,lowest->semantic)<0)) lowest=v;
        if(!lowest) break;
        for(i=0;i<16 && assigned[i];i++) {}
        if(i==16) return MslFail(m,6605,&m->entry->loc,"Metal exceeds 16 vertex attributes");
        lowest->attribute=i; assigned[i]=1;
      }
    }
    { MslInterface *v; int components=0;
      for(v=m->inputs;v;v=v->next) if(v->attribute<0) components+=v->type.lanes;
      if(components>64) return MslFail(m,6605,&m->entry->loc,"Metal input exceeds 64 components");
      components=0;
      for(v=m->outputs;v;v=v->next) if(!v->builtin) components+=v->type.lanes;
      if(components>64) return MslFail(m,6605,&m->entry->loc,"Metal output exceeds 64 components");
    }
    d=source->globals;
    for(q=m->globals;q && !m->failed;q=q->next,d=d->next) {
        if(d->storage==CGIR_STORAGE_CONST)
            return MslFail(m,6608,&d->loc,"global constants require compile-time folding");
        if(q->type.base>=MSL_TEXTURE2D) {
            q->resourceSlot=-2;
            if(d->semantic) {
                semantic=MslLSemantic(&l,d->semantic,&d->loc);
                if(m->failed) return 0;
                for(i=0;i<16;i++) { char binding[20]; sprintf(binding,"TEXUNIT%d",i);
                    if(!strcmp(binding,semantic)) q->resourceSlot=i;
                }
                if(q->resourceSlot<0 || used[q->resourceSlot]) return MslFail(m,6604,&d->loc,"invalid or duplicate texture/sampler binding");
                used[q->resourceSlot]=1;
            }
        } else {
            if(d->semantic) return MslFail(m,6604,&d->loc,"explicit numeric uniform bindings are unsupported");
            q->uniformSlot=0;
        }
    }
    { MslDecl **end=&m->bindings;
      for(q=m->entry->parameters;q;q=q->next) if(q->uniformSlot>=0 || q->type.base>=MSL_TEXTURE2D) { *end=q; end=&q->bindingNext; }
      for(q=m->globals;q;q=q->next) { *end=q; end=&q->bindingNext; }
    }
    for(;;) {
        MslDecl *lowest=NULL;
        for(q=m->bindings;q;q=q->bindingNext)
            if(q->resourceSlot==-2 && (!lowest || strcmp(q->sourceName,lowest->sourceName)<0)) lowest=q;
        if(!lowest) break;
        for(i=0;i<16 && used[i];i++) {}
        if(i==16) return MslFail(m,6605,&lowest->loc,"Metal exceeds 16 texture/sampler pairs");
        lowest->resourceSlot=i; used[i]=1;
    }
    /* MslLName-sorted 16-byte slots, independent of declaration order. */
    for (q = m->bindings; q; q = q->bindingNext) if (q->uniformSlot >= 0) {
        slot = 0;
        for (r = m->bindings; r; r = r->bindingNext)
            if (r->uniformSlot >= 0 && strcmp(r->sourceName, q->sourceName) < 0) {
                int n=MslTypeSlots(r->type);
                if(n<0 || slot>256-n) return MslFail(m,6605,&r->loc,"uniform buffer exceeds 4096 bytes");
                slot+=n;
            }
        q->uniformSlot = slot;
        i=MslTypeSlots(q->type);
        if(i<0 || m->uniformSlots>256-i) return MslFail(m,6605,&q->loc,"uniform buffer exceeds 4096 bytes");
        m->uniformSlots+=i;
    }
    if (m->uniformSlots > 256) return MslFail(m, 6605, &m->entry->loc, "uniform buffer exceeds 4096 bytes");
    return !m->failed;
}

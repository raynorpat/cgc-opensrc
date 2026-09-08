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

const char *MslLName(Lower *l, const char *prefix)
{
    char buf[64]; sprintf(buf, "cg_%s_%d", prefix, l->ordinal++);
    return MslString(l->m, buf);
}

MslDecl *MslLFindDecl(Lower *l, Symbol *s)
{
    DeclMap *p;
    for (p = l->decls; p; p = p->next) if (p->source == s) return p->target;
    return NULL;
}

MslFunction *MslLFindFunction(Lower *l, Symbol *s)
{
    FunMap *p;
    for (p = l->functions; p; p = p->next)
        if (p->source->symbol == s) return p->target;
    return NULL;
}

MslExpr *MslLTemporary(Lower *l,MslType type,const SourceLoc *loc)
{
    MslDecl *d=(MslDecl *)MslAlloc(l->m,sizeof(*d));
    MslStmt *s=(MslStmt *)MslAlloc(l->m,sizeof(*s));
    MslExpr *e=(MslExpr *)MslAlloc(l->m,sizeof(*e));
    if(!d || !s || !e) return NULL;
    d->name=MslLName(l,"copy"); d->type=type; d->loc=*loc; d->uniformSlot=-1;
    d->next=l->current->locals; l->current->locals=d;
    s->kind=MSL_DECL; s->decl=d; s->loc=*loc; s->next=l->temporaries; l->temporaries=s;
    e->kind=MSL_SYMBOL; e->decl=d; e->type=type; e->loc=*loc; return e;
}

MslExpr *MslLCopyExpr(Lower *l,const MslExpr *e)
{
    MslExpr *r=(MslExpr *)MslAlloc(l->m,sizeof(*r));
    if(r) { *r=*e; r->next=NULL; } return r;
}

MslExpr *MslLStore(Lower *l,MslExpr *to,MslExpr *from)
{
    MslExpr *e=(MslExpr *)MslAlloc(l->m,sizeof(*e)); if(!e) return NULL;
    e->kind=MSL_ASSIGN; e->type=to->type; e->loc=to->loc;
    e->a=to; e->b=from; e->text="=";
    if(to->kind==MSL_ROW) l->m->rowSetters|=1u<<to->type.lanes;
    return e;
}

void MslLSequenceAppend(MslExpr ***tail,MslExpr *e)
{ **tail=e; if(e) *tail=&e->next; }

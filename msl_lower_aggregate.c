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

MslType MslLLowerRecord(Lower *l, const Type *t, const SourceLoc *loc)
{
    TypeMap *map; MslType r; MslDecl **tail, *d; Symbol *member; DeclMap *dm;
    memset(&r,0,sizeof(r)); r.base=MSL_RECORD; r.lanes=1;
    if(t->str.unqualifiedtype) t=t->str.unqualifiedtype;
    for(map=l->types;map;map=map->next) if(map->source==t) {
        if(map->building) MslFail(l->m,6601,loc,"recursive Metal struct");
        r.record=map->target; return r;
    }
    map=(TypeMap *)MslAlloc(l->m,sizeof(*map));
    r.record=(MslRecord *)MslAlloc(l->m,sizeof(*r.record));
    if(!map || !r.record) return r;
    map->source=t; map->target=r.record; map->building=1; map->next=l->types; l->types=map;
    r.record->name=MslLName(l,"type"); tail=&r.record->members;
    for(member=t->str.members->symbols;member && !l->m->failed;member=member->next) {
        if(member->kind!=VARIABLE_S) continue;
        d=(MslDecl *)MslAlloc(l->m,sizeof(*d)); dm=(DeclMap *)MslAlloc(l->m,sizeof(*dm));
        if(!d || !dm) break;
        d->name=MslLSourceName(l,"field",GetAtomString(atable,member->name)); d->sourceName=MslString(l->m,GetAtomString(atable,member->name));
        d->type=MslLLowerType(l,member->type,&member->loc); d->loc=member->loc;
        if(d->type.base>=MSL_TEXTURE2D) MslFail(l->m,6601,loc,"resource struct members are unsupported");
        if(member->details.var.semantics) d->semantic=MslLSemantic(l,member->details.var.semantics,&member->loc);
        d->attribute=-1; d->uniformSlot=-1; *tail=d; tail=&d->next;
        dm->source=member; dm->target=d; dm->next=l->decls; l->decls=dm;
    }
    map->building=0;
    /* Append after dependencies so nested records are defined first. */
    { MslRecord **end=&l->m->records; while(*end) end=&(*end)->next; *end=r.record; }
    return r;
}

MslType MslLLowerArray(Lower *l,const Type *t,const SourceLoc *loc)
{
    MslType r; TypeMap *p; MslDecl *element;
    memset(&r,0,sizeof(r)); r.base=MSL_ARRAY; r.lanes=1;
    for(p=l->types;p;p=p->next) if(IsArray(p->source) && IsSameUnqualifiedType(p->source,t)) {
        r.record=p->target; return r;
    }
    if(t->arr.numels<1) { MslFail(l->m,6601,loc,"Metal requires fixed-size arrays"); return r; }
    p=(TypeMap *)MslAlloc(l->m,sizeof(*p)); r.record=(MslRecord *)MslAlloc(l->m,sizeof(*r.record));
    element=(MslDecl *)MslAlloc(l->m,sizeof(*element));
    if(!p || !r.record || !element) return r;
    p->source=t; p->target=r.record; p->next=l->types; l->types=p;
    r.record->name=MslLName(l,"array"); r.record->arrayCount=t->arr.numels; r.record->members=element;
    element->name="elements"; element->sourceName="elements";
    element->type=MslLLowerType(l,t->arr.eltype,loc);
    if(element->type.base>=MSL_TEXTURE2D) MslFail(l->m,6601,loc,"resource arrays are unsupported");
    { MslRecord **tail=&l->m->records; while(*tail) tail=&(*tail)->next; *tail=r.record; }
    return r;
}

/* Expressions expanded into multiple column accesses must evaluate their
 * operands once, in source order, at the original expression position. */
MslExpr *MslLMaterialize(Lower *l, MslExpr *value, MslExpr ***tail)
{
    MslExpr *temp;
    if(!value || l->m->failed) return NULL;
    temp=MslLTemporary(l,value->type,&value->loc);
    if (!temp) return NULL;
    MslLSequenceAppend(tail,MslLStore(l,MslLCopyExpr(l,temp),value));
    return temp;
}
MslExpr *MslLExpandOnce(Lower *l, MslExpr *e, int arguments)
{
    MslExpr *seq,*arg,*next,**tail,**args;
    seq=(MslExpr *)MslAlloc(l->m,sizeof(*seq));
    if (!seq) return NULL;
    seq->kind=MSL_SEQUENCE; seq->type=e->type; seq->loc=e->loc; tail=&seq->a;
    if (arguments) {
        args=&e->a;
        for (arg=e->a;arg;arg=next) {
            next=arg->next; arg->next=NULL;
            *args=MslLMaterialize(l,arg,&tail);
            if (!*args) return NULL;
            args=&(*args)->next;
        }
    } else {
        e->a=MslLMaterialize(l,e->a,&tail);
        if (e->b) e->b=MslLMaterialize(l,e->b,&tail);
    }
    *tail=e;
    return seq;
}
int MslLSameExpr(const CgIRExpr *a,const CgIRExpr *b)
{
    const CgIRExpr *x,*y;
    if(!a || !b) return a==b;
    if(a->kind!=b->kind || a->sideEffects || b->sideEffects) return 0;
    switch(a->kind) {
    case CGIR_EXPR_SYMBOL: return a->u.symbol==b->u.symbol;
    case CGIR_EXPR_CONSTANT: return a->u.constant.kind==b->u.constant.kind &&
        (CgScalarIsFloating(a->u.constant.kind)?a->u.constant.value.f==b->u.constant.value.f:a->u.constant.value.u==b->u.constant.value.u);
    case CGIR_EXPR_INDEX: return MslLSameExpr(a->u.index.object,b->u.index.object) && MslLSameExpr(a->u.index.index,b->u.index.index);
    case CGIR_EXPR_MEMBER: return a->u.member.member==b->u.member.member && MslLSameExpr(a->u.member.object,b->u.member.object);
    case CGIR_EXPR_CONSTRUCT:
        for(x=a->u.construct.arguments,y=b->u.construct.arguments;x&&y;x=x->next,y=y->next) if(!MslLSameExpr(x,y)) return 0;
        return !x&&!y;
    case CGIR_EXPR_CAST: return IsSameUnqualifiedType(a->type,b->type) && MslLSameExpr(a->u.cast.operand,b->u.cast.operand);
    default: return 0;
    }
}

int MslLSelectorGroup(const CgIRStmt *s)
{
    const CgIRExpr *a;
    if(!s || s->kind!=CGIR_STMT_EXPR || !(a=s->u.expression) || a->kind!=CGIR_EXPR_ASSIGN) return 0;
    return a->u.assign.target->selectorRead && IsScalar(a->u.assign.target->type) && IsVector(a->u.assign.value->type,NULL);
}

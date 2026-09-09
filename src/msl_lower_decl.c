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

MslType MslLLowerType(Lower *l, const Type *t, const SourceLoc *loc)
{
    MslType r; int n = 1, columns; CgSamplerKind sampler;
    memset(&r,0,sizeof(r));
    r.base = MSL_VOID; r.lanes = 1;
    if(GetScalarKind(t)==CG_SCALAR_HALF) r.promotion=1;
    if(GetScalarKind(t)==CG_SCALAR_FIXED) r.promotion=2;
    if (IsVoid(t)) return r;
    if(IsSampler(t,&sampler)) {
        if(sampler==CG_SAMPLER_2D) r.base=MSL_TEXTURE2D;
        else if(sampler==CG_SAMPLER_CUBE) r.base=MSL_TEXTURECUBE;
        else MslFail(l->m,6601,loc,"Metal supports only sampler2D and samplerCUBE");
        return r;
    }
    if (IsStruct(t)) return MslLLowerRecord(l,t,loc);
    if (IsMatrix(t,&n,&columns)) {
        if(n!=columns || n<2 || n>4 || (GetScalarKind(t)!=CG_SCALAR_FLOAT && GetScalarKind(t)!=CG_SCALAR_CFLOAT && GetScalarKind(t)!=CG_SCALAR_HALF && GetScalarKind(t)!=CG_SCALAR_FIXED))
            MslFail(l->m,6601,loc,"Metal requires square floating matrices");
        r.base=MSL_MATRIX; r.lanes=n; return r;
    }
    if(IsArray(t) && !IsVector(t,&n)) return MslLLowerArray(l,t,loc);
    if (!IsScalar(t) && !IsVector(t, &n)) {
        MslFail(l->m, 6601, loc, "Metal aggregate type is not implemented"); return r;
    }
    r.lanes = n;
    switch (GetScalarKind(t)) {
    case CG_SCALAR_BOOL: r.base = MSL_BOOL; break;
    case CG_SCALAR_INT: case CG_SCALAR_CINT: r.base = MSL_INT; break;
    case CG_SCALAR_UINT: r.base = MSL_UINT; break;
    case CG_SCALAR_FLOAT: case CG_SCALAR_CFLOAT:
    case CG_SCALAR_HALF: case CG_SCALAR_FIXED: r.base = MSL_FLOAT; break;
    default: MslFail(l->m, 6601, loc, "unsupported Metal numeric type"); break;
    }
    return r;
}

static void *DefaultAlloc(void *arg,size_t size)
{
    return MslAlloc((MslModule *)arg,size);
}
static int MslDefaultIsConstant(const CgIRExpr *e)
{
    const CgIRExpr *a;
    if(!e) return 0;
    if(e->kind==CGIR_EXPR_CONSTANT) return 1;
    if(e->kind==CGIR_EXPR_CAST) return MslDefaultIsConstant(e->u.cast.operand);
    if(e->kind!=CGIR_EXPR_CONSTRUCT) return 0;
    for(a=e->u.construct.arguments;a;a=a->next) if(!MslDefaultIsConstant(a)) return 0;
    return 1;
}
MslDecl *MslLLowerDecl(Lower *l, const CgIRDecl *s)
{
    MslDecl *d = (MslDecl *) MslAlloc(l->m, sizeof(*d));
    DeclMap *p = (DeclMap *) MslAlloc(l->m, sizeof(*p));
    if (!d || !p) return NULL;
    d->name = MslLSourceName(l, "v",GetAtomString(atable,s->name)); d->sourceName = MslString(l->m, GetAtomString(atable, s->name));
    d->type = MslLLowerType(l, s->type, &s->loc); d->loc = s->loc;
    d->readOnly=s->domain==CGIR_DOMAIN_UNIFORM;
    d->constant=s->storage==CGIR_STORAGE_CONST;
    d->role=d->type.base>=MSL_TEXTURE2D?MSL_RESOURCE_DECL:d->readOnly?MSL_UNIFORM_DECL:MSL_VALUE_DECL;
    d->uniformSlot = -1; d->resourceSlot=-1; d->attribute = -1;
    if (GetQualifiers(s->type) & TYPE_QUALIFIER_OUT)
        d->direction=(GetQualifiers(s->type) & TYPE_QUALIFIER_IN) ? 2 : 1;
    if(d->type.base>=MSL_TEXTURE2D && d->direction)
        MslFail(l->m,6602,&s->loc,"sampler output parameters are unsupported");
    if(s->domain==CGIR_DOMAIN_UNIFORM && s->symbol && s->symbol->details.var.init) {
        CgIRModule defaults; CgIRExpr *value;
        CgIRInitModule(&defaults,DefaultAlloc,l->m);
        value=CgIRLowerUniformDefault(&defaults,s);
        if(defaults.failed) MslFail(l->m,6604,&s->loc,"unsupported uniform default initializer");
        else if(!MslDefaultIsConstant(value))
            MslFail(l->m,6604,&s->loc,"uniform default requires constant constructor data");
        else {
            MslExpr *target=MslLLowerExpr(l,value);
            /* Constant folding promotes half/fixed constructors to float.
             * Compare target shapes, not the pre-promotion source identities. */
            if(target && target->type.base==d->type.base && target->type.lanes==d->type.lanes && target->type.record==d->type.record)
                d->defaultValue=target;
            else if(!l->m->failed) MslFail(l->m,6604,&s->loc,"uniform default has incompatible target shape");
        }
    }
    p->source = s->symbol; p->target = d; p->next = l->decls; l->decls = p;
    return d;
}

MslDecl *MslLLowerDecls(Lower *l, const CgIRDecl *s)
{
    MslDecl *first = NULL, **tail = &first;
    for (; s && !l->m->failed; s = s->next) {
        *tail = MslLLowerDecl(l, s); if (!*tail) break; tail = &(*tail)->next;
    }
    return first;
}

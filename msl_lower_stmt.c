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

MslStmt *MslLLowerStmt(Lower *l, const CgIRStmt *s)
{
    MslStmt *first = NULL, **tail = &first, *t;
    for (; s && !l->m->failed; s = s->next) {
        t = (MslStmt *) MslAlloc(l->m, sizeof(*t)); if (!t) break;
        *tail = t; tail = &t->next; t->loc = s->loc;
        if(MslLSelectorGroup(s)) {
            const CgIRStmt *item=s; const CgIRExpr *value=s->u.expression->u.assign.value;
            int n=0,j; MslDecl *temp; MslStmt *assign; MslExpr *rhs,*sym;
            IsVector(value->type,&n);
            for(j=0;j<n;j++,item=item->next)
                if(!MslLSelectorGroup(item) || !MslLSameExpr(value,item->u.expression->u.assign.value)) {
                    MslFail(l->m,6602,&s->loc,"unrecognized Metal matrix selector store group"); return first;
                }
            temp=(MslDecl *)MslAlloc(l->m,sizeof(*temp)); if(!temp) return first;
            temp->name=MslLName(l,"selector"); temp->type=MslLLowerType(l,value->type,&s->loc);
            temp->next=l->current->locals; l->current->locals=temp;
            t->kind=MSL_DECL; t->decl=temp; t->value=MslLLowerExpr(l,value);
            for(j=0;j<n;j++) {
                char mask[2];
                assign=(MslStmt *)MslAlloc(l->m,sizeof(*assign)); rhs=(MslExpr *)MslAlloc(l->m,sizeof(*rhs));
                sym=(MslExpr *)MslAlloc(l->m,sizeof(*sym));
                if(!assign || !rhs || !sym) return first;
                assign->kind=MSL_EXPR; assign->loc=s->loc;
                assign->value=MslLLowerExpr(l,s->u.expression);
                if(!assign->value) return first;
                sym->kind=MSL_SYMBOL; sym->decl=temp; sym->type=temp->type;
                rhs->kind=MSL_SWIZZLE; rhs->a=sym; rhs->type=assign->value->type;
                mask[0]="xyzw"[j]; mask[1]=0; rhs->text=MslString(l->m,mask);
                assign->value->b=rhs; *tail=assign; tail=&assign->next;
                if(j+1<n) s=s->next;
            }
            continue;
        }

        switch (s->kind) {
        case CGIR_STMT_BLOCK: t->kind = MSL_BLOCK; t->body = MslLLowerStmt(l, s->u.block); break;
        case CGIR_STMT_DECL:
            t->kind = MSL_DECL; t->decl = MslLFindDecl(l, s->u.decl->symbol);
            t->value = MslLConvert(l,MslLLowerExpr(l, s->u.decl->initializer),t->decl->type); break;
        case CGIR_STMT_EXPR: t->kind = MSL_EXPR; t->value = MslLLowerExpr(l, s->u.expression); break;
        case CGIR_STMT_IF:
            t->kind = MSL_IF; t->value = MslLLowerExpr(l, s->u.ifStmt.condition);
            t->body = MslLLowerStmt(l, s->u.ifStmt.trueBranch); t->other = MslLLowerStmt(l, s->u.ifStmt.falseBranch); break;
        case CGIR_STMT_WHILE: case CGIR_STMT_DO:
            t->kind = s->kind == CGIR_STMT_WHILE ? MSL_WHILE : MSL_DO;
            t->value = MslLLowerExpr(l, s->u.loop.condition); t->body = MslLLowerStmt(l, s->u.loop.body); break;
        case CGIR_STMT_FOR:
            t->kind = MSL_FOR; t->init = MslLLowerStmt(l, s->u.forStmt.init);
            t->value = MslLLowerExpr(l, s->u.forStmt.condition); t->step = MslLLowerExpr(l, s->u.forStmt.step);
            t->body = MslLLowerStmt(l, s->u.forStmt.body); break;
        case CGIR_STMT_RETURN: t->kind = MSL_RETURN; t->value = MslLConvert(l,MslLLowerExpr(l, s->u.returnExpr),l->current->result); break;
        case CGIR_STMT_BREAK: t->kind = MSL_BREAK; break;
        case CGIR_STMT_CONTINUE: t->kind = MSL_CONTINUE; break;
        case CGIR_STMT_DISCARD:
            t->kind = MSL_DISCARD; t->value = MslLLowerExpr(l, s->u.discard.condition);
            if (l->m->stage != MSL_FRAGMENT) MslFail(l->m, 6606, &s->loc, "discard requires fragment stage");
            break;
        default: MslFail(l->m, 6606, &s->loc, "unsupported Metal statement or stage"); break;
        }
    }
    return first;
}

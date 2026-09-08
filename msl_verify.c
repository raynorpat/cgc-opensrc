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

#include <string.h>
#include <stdio.h>
#include <stddef.h>
#include "slglobals.h"
#include "msl_ir.h"
static int TypeDepthOK(MslType t, int depth)
{
    const MslDecl *p,*q; int count=0;
    if (depth>64 || t.base<MSL_VOID || t.base>MSL_TEXTURECUBE ||
        t.lanes<1 || t.lanes>4 || t.promotion<0 || t.promotion>2) return 0;
    if (t.promotion && t.base!=MSL_FLOAT && t.base!=MSL_MATRIX) return 0;
    if (t.base==MSL_MATRIX) return t.lanes>=2 && !t.record;
    if (t.base==MSL_RECORD || t.base==MSL_ARRAY) {
        if (t.lanes!=1 || !t.record || !t.record->name || !t.record->members) return 0;
        if (t.base==MSL_ARRAY && (t.record->arrayCount<1 || t.record->members->next)) return 0;
        if (t.base==MSL_RECORD && t.record->arrayCount) return 0;
        for (p=t.record->members;p;p=p->next) {
            if (++count>4096 || !p->name || p->type.base==MSL_VOID ||
                p->type.base>=MSL_TEXTURE2D || !TypeDepthOK(p->type,depth+1)) return 0;
            for (q=t.record->members;q!=p;q=q->next)
                if (!strcmp(q->name,p->name)) return 0;
        }
        return 1;
    }
    return !t.record && ((t.base!=MSL_VOID && t.base<MSL_TEXTURE2D) || t.lanes==1);
}
static int TypeOK(MslType t) { return TypeDepthOK(t,0); }
static int Numeric(MslType t) { return t.base>=MSL_BOOL && t.base<=MSL_FLOAT; }
static int Integer(MslType t) { return (t.base==MSL_INT || t.base==MSL_UINT) && t.lanes==1; }
static int WordIn(const char *word,const char *list)
{
    size_t n;
    if (!word || !*word) return 0;
    n=strlen(word);
    while (*list) {
        if (!strncmp(list,word,n) && (list[n]==' ' || !list[n])) return 1;
        while (*list && *list!=' ') ++list;
        if (*list) ++list;
    }
    return 0;
}
static int TypeRegistered(const MslModule *m,MslType type)
{
    const MslRecord *r; int count=0;
    if(type.base!=MSL_RECORD && type.base!=MSL_ARRAY) return 1;
    for(r=m->records;r;r=r->next) {
        if(++count>4096) return 0;
        if(r==type.record) return 1;
    }
    return 0;
}
static int Same(MslType a, MslType b) { return a.base == b.base && a.lanes == b.lanes && a.record==b.record; }
static int DeclKnown(const MslFunction *f, const MslDecl *d)
{
    const MslDecl *p;
    for (p = f->parameters; p; p = p->next) if (p == d) return 1;
    for (p=f->globals;p;p=p->next) if(p==d) return 1;
    for (p = f->locals; p; p = p->next) if (p == d) return 1;
    return 0;
}
static int Lvalue(const MslExpr *e)
{
    const char *p,*q;
    if (!e) return 0;
    if (e->kind==MSL_SYMBOL) return e->decl && !e->decl->readOnly;
    if (e->kind==MSL_SWIZZLE) {
        if (!e->text) return 0;
        for (p=e->text;*p;p++) for (q=p+1;*q;q++) if (*p==*q) return 0;
    }
    return (e->kind==MSL_INDEX || e->kind==MSL_SWIZZLE || e->kind==MSL_MEMBER ||
            e->kind==MSL_ROW) && Lvalue(e->a);
}
static int ExprOK(const MslModule *m, const MslFunction *f, const MslExpr *e, int depth)
{
    const MslFunction *g; const MslDecl *p; const MslExpr *a,*last; int count,lanes; const char *ch;
    if (!e || depth > 512 || !TypeOK(e->type) || !TypeRegistered(m,e->type)) return 0;
    switch (e->kind) {
    case MSL_LITERAL: return e->text != NULL && Numeric(e->type) && e->type.lanes==1;
    case MSL_SYMBOL: return e->decl && DeclKnown(f, e->decl) && Same(e->type, e->decl->type);
    case MSL_CONSTRUCT:
        if (!e->a) return 0;
        count=0; lanes=0;
        for (a=e->a;a;a=a->next) {
            if (++count>64 || !ExprOK(m,f,a,depth+1)) return 0;
            lanes+=a->type.lanes;
        }
        if (Numeric(e->type))
            return (lanes==e->type.lanes || (count==1 && e->a->type.lanes==1)) && Numeric(e->a->type);
        if (e->type.base==MSL_MATRIX)
            return (count==1 && (e->a->type.base==MSL_MATRIX || lanes==1)) || lanes==e->type.lanes*e->type.lanes;
        return e->type.base==MSL_RECORD || e->type.base==MSL_ARRAY;
    case MSL_MEMBER:
        if(!e->a || e->a->type.base!=MSL_RECORD || !ExprOK(m,f,e->a,depth+1)) return 0;
        for(p=e->a->type.record->members;p;p=p->next) if(p==e->decl) return Same(e->type,p->type);
        return 0;
    case MSL_ROW:
        return ExprOK(m,f,e->a,depth+1) && ExprOK(m,f,e->b,depth+1) &&
            e->a->type.base==MSL_MATRIX && Integer(e->b->type) &&
            e->type.base==MSL_FLOAT && e->type.lanes==e->a->type.lanes;
    case MSL_SWIZZLE:
        if (!e->text || !ExprOK(m,f,e->a,depth+1) || !Numeric(e->a->type) ||
            e->type.base!=e->a->type.base || strlen(e->text)!=(size_t)e->type.lanes) return 0;
        for (ch=e->text;*ch;ch++) {
            int lane=*ch=='w'?3:*ch-'x';
            if (lane<0 || lane>=e->a->type.lanes) return 0;
        }
        return 1;
    case MSL_UNARY:
        if (!WordIn(e->text,"+ - ! ~ ++ --") || !ExprOK(m,f,e->a,depth+1)) return 0;
        if (WordIn(e->text,"++ --") && !Lvalue(e->a)) return 0;
        return Same(e->type,e->a->type);
    case MSL_ASSIGN:
        return WordIn(e->text,"= += -= *= /= %=") && Lvalue(e->a) && ExprOK(m,f,e->a,depth+1) &&
            ExprOK(m,f,e->b,depth+1) && Same(e->a->type,e->b->type) && Same(e->type,e->a->type);
    case MSL_INDEX:
        if (!ExprOK(m,f,e->a,depth+1) || !ExprOK(m,f,e->b,depth+1) || !Integer(e->b->type)) return 0;
        if (e->a->type.base==MSL_ARRAY) return Same(e->type,e->a->type.record->members->type);
        if (e->a->type.base==MSL_MATRIX) return e->type.base==MSL_FLOAT && e->type.lanes==e->a->type.lanes;
        return Numeric(e->a->type) && e->type.base==e->a->type.base && e->type.lanes==1;
    case MSL_BINARY:
        return WordIn(e->text,"* / % + - << >> < > <= >= == != & ^ | && ||") &&
            ExprOK(m,f,e->a,depth+1) && ExprOK(m,f,e->b,depth+1) &&
            (Numeric(e->type) || e->type.base==MSL_MATRIX);
    case MSL_SELECT:
        return ExprOK(m,f,e->a,depth+1) && ExprOK(m,f,e->b,depth+1) && ExprOK(m,f,e->c,depth+1) &&
               e->a->type.base==MSL_BOOL && (e->a->type.lanes==1 || e->a->type.lanes==e->type.lanes) &&
               Same(e->b->type,e->c->type) && Same(e->type,e->b->type);
    case MSL_SAMPLE:
        return e->a && e->a->kind==MSL_SYMBOL && e->a->type.base>=MSL_TEXTURE2D &&
            ExprOK(m,f,e->a,depth+1) && ExprOK(m,f,e->b,depth+1) &&
            e->b->type.base==MSL_FLOAT && e->b->type.lanes==(e->a->type.base==MSL_TEXTURE2D?2:3) &&
            e->type.base==MSL_FLOAT && e->type.lanes==4 &&
            (e->c ? ExprOK(m,f,e->c,depth+1) && e->c->type.base==MSL_FLOAT && e->c->type.lanes==1 : m->stage==MSL_FRAGMENT);
    case MSL_SEQUENCE:
        if(!e->a) return 0;
        count=0; last=NULL;
        for(a=e->a;a;a=a->next) {
            if(++count>4096 || !ExprOK(m,f,a,depth+1)) return 0;
            last=a;
        }
        return e->type.base==MSL_VOID || Same(e->type,last->type);
    case MSL_CALL:
        count=0;
        for (a=e->a;a;a=a->next) if (++count>64 || !ExprOK(m,f,a,depth+1)) return 0;
        if (!e->function) {
            if (WordIn(e->text,"dfdx dfdy") && m->stage!=MSL_FRAGMENT) return 0;
            if (WordIn(e->text,"abs saturate floor ceil fract sqrt rsqrt exp exp2 log log2 sin cos tan asin acos atan length normalize transpose any all dfdx dfdy")) return count==1;
            if (WordIn(e->text,"min max fmod pow atan2 dot cross distance reflect step")) return count==2;
            if (WordIn(e->text,"clamp refract mix smoothstep")) return count==3;
            return 0;
        }
        for (g=m->functions; g && g != e->function; g=g->next) {}
        if (!g || !Same(g->result,e->type)) return 0;
        for (p=g->parameters,a=e->a; p && a; p=p->next,a=a->next)
            if (!Same(p->type,a->type) || (p->direction && !Lvalue(a))) return 0;
        return !p && !a;
    default: return 0;
    }
}
static int StmtOK(const MslModule *m, const MslFunction *f, const MslStmt *s, int loop, int depth)
{
    int count=0;
    if (depth > 512) return 0;
    for (; s; s=s->next) {
        if (++count>65536) return 0;
        if (s->value && !ExprOK(m,f,s->value,0)) return 0;
        switch (s->kind) {
        case MSL_BLOCK: if (!StmtOK(m,f,s->body,loop,depth+1)) return 0; break;
        case MSL_DECL:
            if (!s->decl || !DeclKnown(f,s->decl) || (s->value && !Same(s->decl->type,s->value->type))) return 0;
            break;
        case MSL_EXPR: if (!s->value) return 0; break;
        case MSL_RETURN:
            if (s->value ? !Same(s->value->type,f->result) : f->result.base != MSL_VOID) return 0;
            break;
        case MSL_IF:
            if (!s->value || s->value->type.base != MSL_BOOL || s->value->type.lanes != 1 ||
                !StmtOK(m,f,s->body,loop,depth+1) || !StmtOK(m,f,s->other,loop,depth+1)) return 0;
            break;
        case MSL_WHILE: case MSL_DO: case MSL_FOR:
            if ((s->value && (s->value->type.base != MSL_BOOL || s->value->type.lanes != 1)) ||
                (s->kind != MSL_FOR && !s->value) ||
                !StmtOK(m,f,s->body,loop+1,depth+1) || !StmtOK(m,f,s->init,loop,depth+1) ||
                (s->step && !ExprOK(m,f,s->step,0))) return 0;
            break;
        case MSL_BREAK: case MSL_CONTINUE: if (!loop) return 0; break;
        case MSL_DISCARD: if (m->stage != MSL_FRAGMENT || (s->value && (s->value->type.base!=MSL_BOOL || s->value->type.lanes!=1))) return 0; break;
        default: return 0;
        }
    }
    return 1;
}
static int InterfaceOK(const MslModule *m,const MslInterface *list,int output)
{
    const MslInterface *v,*p; int count=0,components=0,attributes=0;
    for(v=list;v;v=v->next) {
        if (++count>65 || !TypeOK(v->type) || !Numeric(v->type) || !v->semantic || !v->path) return 0;
        for(p=list;p!=v;p=p->next) if(!strcmp(v->semantic,p->semantic)) return 0;
        if(v->attribute>=0) {
            if(output || m->stage!=MSL_VERTEX || v->attribute>=16 || v->builtin || v->type.base==MSL_BOOL || (attributes & (1<<v->attribute))) return 0;
            attributes|=1<<v->attribute;
        } else if(v->builtin) {
            if(!output || v->builtin<1 || v->builtin>2 || v->type.base!=MSL_FLOAT ||
                v->type.lanes!=(v->builtin==1?4:1) || (v->builtin==2 && m->stage!=MSL_FRAGMENT)) return 0;
        } else {
            if(v->type.base==MSL_BOOL) return 0;
            components+=v->type.lanes;
        }
    }
    return components<=64;
}
static int BindingsOK(const MslModule *m)
{
    const MslDecl *p; int count=0,i,n,slots[256],resources=0,total=0;
    memset(slots,0,sizeof(slots));
    if(m->uniformSlots<0 || m->uniformSlots>256) return 0;
    for(p=m->bindings;p;p=p->bindingNext) {
        if(++count>272 || !p->sourceName || !p->name || !TypeOK(p->type)) return 0;
        if(p->type.base>=MSL_TEXTURE2D) {
            if(p->resourceSlot<0 || p->resourceSlot>=16 || (resources & (1<<p->resourceSlot))) return 0;
            resources|=1<<p->resourceSlot;
        } else {
            n=MslTypeSlots(p->type);
            if(n<=0 || p->uniformSlot<0 || p->uniformSlot>256-n) return 0;
            for(i=p->uniformSlot;i<p->uniformSlot+n;i++) if(slots[i]++) return 0;
            total+=n;
        }
    }
    return total==m->uniformSlots;
}
int MslVerifyModule(const MslModule *m, MslDiagnostic *d)
{
    const MslFunction *f,*g; const MslDecl *p,*q; int found=0;
    memset(d,0,sizeof(*d)); d->reason="invalid Metal target module";
    if (!m || m->failed || !m->entry || !m->exportName || !m->sourceEntry ||
        (m->stage != MSL_VERTEX && m->stage != MSL_FRAGMENT)) return 0;
    if (!InterfaceOK(m,m->inputs,0) || !InterfaceOK(m,m->outputs,1) || !BindingsOK(m)) return 0;
    for (f=m->functions; f; f=f->next) {
        d->loc=f->loc;
        if (f==m->entry) found=1;
        if (!f->name || !TypeOK(f->result) || !TypeRegistered(m,f->result) || !f->body) return 0;
        for (g=f->next;g;g=g->next) if (!strcmp(f->name,g->name)) return 0;
        for (p=f->parameters;p;p=p->next) {
            if (!p->name || !TypeOK(p->type) || !TypeRegistered(m,p->type) || p->type.base == MSL_VOID) return 0;
            for (q=p->next;q;q=q->next) if (!strcmp(p->name,q->name)) return 0;
        }
        for(p=f->locals;p;p=p->next) {
            if(!p->name || !TypeOK(p->type) || !TypeRegistered(m,p->type) || p->type.base==MSL_VOID || p->type.base>=MSL_TEXTURE2D) return 0;
            for(q=p->next;q;q=q->next) if(!strcmp(p->name,q->name)) return 0;
            for(q=f->parameters;q;q=q->next) if(!strcmp(p->name,q->name)) return 0;
        }
        if (!StmtOK(m,f,f->body,0,0)) return 0;
    }
    return found;
}

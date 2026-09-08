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
static void TypeName(FILE *o, MslType t)
{
    static const char *names[] = { "void", "bool", "int", "uint", "float" };
    if(t.base>=MSL_TEXTURE2D) { fputs(t.base==MSL_TEXTURE2D?"texture2d<float>":"texturecube<float>",o); return; }
    if(t.base==MSL_RECORD || t.base==MSL_ARRAY) { fputs(t.record->name,o); return; }
    if(t.base==MSL_MATRIX) { fprintf(o,"float%dx%d",t.lanes,t.lanes); return; }
    fputs(names[t.base],o); if(t.lanes>1) fprintf(o,"%d",t.lanes);
}
static void Expr(FILE *o, const MslExpr *e);
static void Args(FILE *o,const MslExpr *a)
{ for (;a;a=a->next) { Expr(o,a); if(a->type.base>=MSL_TEXTURE2D) fprintf(o,", %s_s",a->decl->name); if(a->next) fputs(", ",o); } }
static void Element(FILE *o,const MslExpr *a,int index)
{
    const MslExpr *first=a;
    while(a && index>=a->type.lanes) { index-=a->type.lanes; a=a->next; }
    if(!a) { a=first; index=0; }
    fputc('(',o); Expr(o,a); fputc(')',o);
    if(a->type.lanes>1) fprintf(o,"[%d]",index);
}
static void GlobalArgs(FILE *o,const MslDecl *p,int comma)
{
    for(;p;p=p->next) {
        if(comma) fputs(", ",o); comma=1;
        fputs(p->name,o); if(p->type.base>=MSL_TEXTURE2D) fprintf(o,", %s_s",p->name);
    }
}
static void Expr(FILE *o,const MslExpr *e)
{
    int row,column;

    if (!e) return;
    switch(e->kind) {
    case MSL_LITERAL: fputs(e->text,o); break;
    case MSL_SYMBOL: fputs(e->decl->name,o); break;
    case MSL_CONSTRUCT:
        TypeName(o,e->type);
        if(e->type.base==MSL_MATRIX && e->a->type.base!=MSL_MATRIX) {
            fputc('(',o);
            for(column=0;column<e->type.lanes;column++) {
                fprintf(o,"float%d(",e->type.lanes);
                for(row=0;row<e->type.lanes;row++) {
                    Element(o,e->a,row*e->type.lanes+column);
                    if(row+1<e->type.lanes) fputs(", ",o);
                }
                fputc(')',o); if(column+1<e->type.lanes) fputs(", ",o);
            }
            fputc(')',o);
        } else {
            fputc(e->type.base==MSL_RECORD?'{':'(',o); Args(o,e->a);
            fputc(e->type.base==MSL_RECORD?'}':')',o);
        }
        break;
    case MSL_UNARY:
        fputc('(',o); if(!e->postfix) fputs(e->text,o); Expr(o,e->a);
        if(e->postfix) fputs(e->text,o); fputc(')',o); break;
    case MSL_BINARY:
        if(e->type.base==MSL_MATRIX && !e->postfix) {
            TypeName(o,e->type); fputc('(',o);
            for(column=0;column<e->type.lanes;column++) {
                fputc('(',o); Expr(o,e->a); fputc(')',o);
                if(e->a->type.base==MSL_MATRIX) fprintf(o,"[%d]",column);
                fprintf(o," %s ",e->text);
                fputc('(',o); Expr(o,e->b); fputc(')',o);
                if(e->b->type.base==MSL_MATRIX) fprintf(o,"[%d]",column);
                if(column+1<e->type.lanes) fputs(", ",o);
            }
            fputc(')',o); break;
        }
        /* fall through */
    case MSL_ASSIGN:
        if(e->a->kind==MSL_ROW) {
            fprintf(o,"cg_setrow%d(",e->a->type.lanes); Expr(o,e->a->a); fputs(", ",o);
            Expr(o,e->a->b); fputs(", ",o); Expr(o,e->b); fputc(')',o); break;
        }
        fputc('(',o); Expr(o,e->a); fprintf(o," %s ",e->text); Expr(o,e->b); fputc(')',o); break;
    case MSL_SELECT:
        if(e->a->type.lanes>1) {
            fputs("select(",o); Expr(o,e->c); fputs(", ",o); Expr(o,e->b); fputs(", ",o); Expr(o,e->a); fputc(')',o); break;
        }
        fputc('(',o); Expr(o,e->a); fputs(" ? ",o); Expr(o,e->b); fputs(" : ",o); Expr(o,e->c); fputc(')',o); break;
    case MSL_SWIZZLE: fputc('(',o); Expr(o,e->a); fprintf(o,").%s",e->text); break;
    case MSL_MEMBER:
        fputc('(',o); Expr(o,e->a); fprintf(o,").%s",e->decl->name); break;
    case MSL_ROW:
        TypeName(o,e->type); fputc('(',o);
        for(column=0;column<e->type.lanes;column++) {
            fputc('(',o); Expr(o,e->a); fprintf(o,")[%d][",column); Expr(o,e->b); fputc(']',o);
            if(column+1<e->type.lanes) fputs(", ",o);
        }
        fputc(')',o); break;
    case MSL_INDEX: fputc('(',o); Expr(o,e->a); fputs(e->a->type.base==MSL_ARRAY?").elements[":")[",o); Expr(o,e->b); fputc(']',o); break;
    case MSL_SAMPLE:
        Expr(o,e->a); fprintf(o,".sample(%s_s, ",e->a->decl->name); Expr(o,e->b);
        if(e->c) { fputs(", level(",o); Expr(o,e->c); fputc(')',o); }
        fputc(')',o); break;
    case MSL_SEQUENCE:
        if(e->type.base==MSL_VOID) fputs("(void)",o);
        fputc('(',o); Args(o,e->a); fputc(')',o); break;
    case MSL_CALL:
        fputs(e->function ? e->function->name : e->text,o); fputc('(',o); Args(o,e->a);
        if(e->function) GlobalArgs(o,e->function->globals,e->a!=NULL);
        fputc(')',o); break;
    }
}
static void Indent(FILE *o,int n) { while(n--) fputs("    ",o); }
static void Stmts(FILE *,const MslStmt *,int);
static void Block(FILE *o,const MslStmt *s,int n)
{ fputs("{\n",o); Stmts(o,s,n+1); Indent(o,n); fputs("}\n",o); }
static void Stmts(FILE *o,const MslStmt *s,int n)
{
    for(;s;s=s->next) {
        Indent(o,n);
        switch(s->kind) {
        case MSL_BLOCK: Block(o,s->body,n); break;
        case MSL_DECL:
            TypeName(o,s->decl->type); fprintf(o," %s",s->decl->name);
            if(s->value) { fputs(" = ",o); Expr(o,s->value); } fputs(";\n",o); break;
        case MSL_EXPR: Expr(o,s->value); fputs(";\n",o); break;
        case MSL_IF:
            fputs("if (",o); Expr(o,s->value); fputs(") ",o); Block(o,s->body,n);
            if(s->other) { Indent(o,n); fputs("else ",o); Block(o,s->other,n); } break;
        case MSL_WHILE:
            fputs("while (",o); Expr(o,s->value); fputs(") ",o); Block(o,s->body,n); break;
        case MSL_DO:
            fputs("do ",o); Block(o,s->body,n); Indent(o,n); fputs("while (",o); Expr(o,s->value); fputs(");\n",o); break;
        case MSL_FOR:
            fputs("{\n",o); Stmts(o,s->init,n+1); Indent(o,n+1); fputs("for (; ",o);
            Expr(o,s->value); fputs("; ",o); Expr(o,s->step); fputs(") ",o); Block(o,s->body,n+1);
            Indent(o,n); fputs("}\n",o); break;
        case MSL_RETURN: fputs("return",o); if(s->value) { fputc(' ',o); Expr(o,s->value); } fputs(";\n",o); break;
        case MSL_BREAK: fputs("break;\n",o); break;
        case MSL_CONTINUE: fputs("continue;\n",o); break;
        case MSL_DISCARD:
            if(s->value) { fputs("if (",o); Expr(o,s->value); fputs(") ",o); } fputs("discard_fragment();\n",o); break;
        }
    }
}
static void Signature(FILE *o,const MslFunction *f)
{
    const MslDecl *p;
    TypeName(o,f->result); fprintf(o," %s(",f->name);
    for(p=f->parameters;p;p=p->next) {
        if(p->direction) fputs("thread ",o);
        TypeName(o,p->type); fprintf(o," %s%s",p->direction?"&":"",p->name);
        if(p->type.base>=MSL_TEXTURE2D) fprintf(o,", sampler %s_s",p->name);
        if(p->next) fputs(", ",o);
    }
    { int comma=f->parameters!=NULL;
      for(p=f->globals;p;p=p->next) {
        if(comma) fputs(", ",o); comma=1; TypeName(o,p->type); fprintf(o," %s",p->name);
        if(p->type.base>=MSL_TEXTURE2D) fprintf(o,", sampler %s_s",p->name);
      }
    }
    fputc(')',o);
}
static void Attribute(FILE *o,const MslModule *m,const MslInterface *v,int output)
{
    if(v->attribute>=0) fprintf(o,"attribute(%d)",v->attribute);
    else if(v->builtin==2) fputs("depth(any)",o);
    else if(v->builtin) fputs(m->stage==MSL_VERTEX?"position":"color(0)",o);
    else {
        fprintf(o,"user(cg_%s)",v->semantic);
        if(v->type.base==MSL_INT || v->type.base==MSL_UINT) fputs(", flat",o);
    }
    (void)output;
}
/* Uniform traversal is shared by wire declaration, metadata and decode. */
static void Uniform(FILE *o,MslType type,const char *path,const char *source,int *slot,int mode)
{
    const MslDecl *d; int i,first=*slot; char child[2048],sourceChild[2048];
    if(type.base==MSL_ARRAY) {
        for(i=0;i<type.record->arrayCount;i++) {
            snprintf(child,sizeof(child),"%s.elements[%d]",path,i);
            snprintf(sourceChild,sizeof(sourceChild),"%s[%d]",source,i);
            Uniform(o,type.record->members->type,child,sourceChild,slot,mode);
        }
        return;
    }
    if(type.base==MSL_RECORD) {
        for(d=type.record->members;d;d=d->next) {
            snprintf(child,sizeof(child),"%s.%s",path,d->name);
            snprintf(sourceChild,sizeof(sourceChild),"%s.%s",source,d->sourceName);
            Uniform(o,d->type,child,sourceChild,slot,mode);
        }
        return;
    }
    if(mode==0) {
        fprintf(o,"// cgc-msl-uniform path=%s offset=%d slots=%d type=",source,first*16,MslTypeSlots(type));
        TypeName(o,type);
        fprintf(o," wire=%s4",type.base==MSL_INT?"int":type.base==MSL_UINT || type.base==MSL_BOOL?"uint":"float");
        if(type.base==MSL_MATRIX) fputs(" column_stride=16",o);
        if(type.promotion) {
            fprintf(o," source_type=%s",type.promotion==1?"half":"fixed");
            if(type.lanes>1) fprintf(o,"%d",type.lanes);
            if(type.base==MSL_MATRIX) fprintf(o,"x%d",type.lanes);
        }
        fputc('\n',o);
    } else if(mode==1) {
        for(i=0;i<MslTypeSlots(type);i++)
            fprintf(o,"    %s4 slot%d;\n",type.base==MSL_INT?"int":type.base==MSL_UINT || type.base==MSL_BOOL?"uint":"float",first+i);
    } else {
        fprintf(o,"    %s = ",path); TypeName(o,type); fputc('(',o);
        for(i=0;i<MslTypeSlots(type);i++) {
            fprintf(o,"cg_u.slot%d.%s",first+i,type.lanes==1?"x":type.lanes==2?"xy":type.lanes==3?"xyz":"xyzw");
            if(i+1<MslTypeSlots(type)) fputs(", ",o);
        }
        fputs(");\n",o);
    }
    *slot+=MslTypeSlots(type);
}
static void DefaultValues(FILE *o,const MslExpr *e,int *comma)
{
    const MslExpr *a; size_t len;
    if(e->kind==MSL_LITERAL) {
        if(*comma) fputc(',',o); *comma=1;
        len=strlen(e->text); if(len && e->text[len-1]=='f') --len;
        fprintf(o,"%.*s",(int)len,e->text);
    } else for(a=e->a;a;a=a->next) DefaultValues(o,a,comma);
}
static void InterfaceMetadata(FILE *o,const MslModule *m,const MslInterface *list,int output)
{
    const MslInterface *v,*next; const char *after=NULL;
    for (;;) {
        next=NULL;
        for(v=list;v;v=v->next)
            if((!after || strcmp(v->semantic,after)>0) && (!next || strcmp(v->semantic,next->semantic)<0)) next=v;
        if(!next) break;
        v=next; after=v->semantic;
        if(v->attribute>=0) fprintf(o,"// cgc-msl-attribute semantic=%s index=%d",v->semantic,v->attribute);
        else if(v->builtin) {
            fprintf(o,"// cgc-msl-builtin semantic=%s attribute=",v->semantic); Attribute(o,m,v,output);
        } else fprintf(o,"// cgc-msl-varying semantic=%s field=cg_%s",v->semantic,v->semantic);
        fputs(" type=",o); TypeName(o,v->type);
        if(v->attribute<0 && !v->builtin) fprintf(o," interpolation=%s",v->type.base==MSL_FLOAT?"perspective":"flat");
        fputc('\n',o);
    }
}
int MslWriteModule(FILE *o,const MslModule *m)
{
    MslDiagnostic diag; const MslFunction *f; const MslDecl *p,*field;
    const MslRecord *record; const MslInterface *v; int i,slot;
    if(!o || !MslVerifyModule(m,&diag)) return 0;
    fprintf(o,"// cgc-msl-abi version=1 profile=%s language=2.0\n",m->stage==MSL_VERTEX?"mslv":"mslf");
    fprintf(o,"// cgc-msl-entry source=%s metal=%s\n",m->sourceEntry,m->exportName);
    for(i=0;i<16;i++) for(p=m->bindings;p;p=p->bindingNext) if(p->resourceSlot==i)
        fprintf(o,"// cgc-msl-resource path=%s texture=%d sampler=%d type=%s\n",p->sourceName,p->resourceSlot,p->resourceSlot,p->type.base==MSL_TEXTURE2D?"2d":"cube");
    if(m->uniformSlots) fprintf(o,"// cgc-msl-buffer index=0 bytes=%d alignment=16\n",16*m->uniformSlots);
    for(i=0;i<m->uniformSlots;i++) for(p=m->bindings;p;p=p->bindingNext) if(p->uniformSlot==i) {
        slot=i; Uniform(o,p->type,p->name,p->sourceName,&slot,0);
    }
    for(i=0;i<m->uniformSlots;i++) for(p=m->bindings;p;p=p->bindingNext) if(p->uniformSlot==i && p->defaultValue) {
        int comma=0; fprintf(o,"// cgc-msl-default path=%s values=",p->sourceName);
        DefaultValues(o,p->defaultValue,&comma); fputc('\n',o);
    }
    InterfaceMetadata(o,m,m->inputs,0);
    InterfaceMetadata(o,m,m->outputs,1);
    fputs("#include <metal_stdlib>\nusing namespace metal;\n\n",o);
    for(record=m->records;record;record=record->next) {
        fprintf(o,"struct %s {\n",record->name);
        for(field=record->members;field;field=field->next) {
            fputs("    ",o); TypeName(o,field->type); fprintf(o," %s",field->name);
            if(record->arrayCount) fprintf(o,"[%d]",record->arrayCount);
            fputs(";\n",o);
        }
        fputs("};\n",o);
    }
    if(m->uniformSlots) {
        fputs("struct cg_Uniforms {\n",o);
        for(i=0;i<m->uniformSlots;i++) for(p=m->bindings;p;p=p->bindingNext) if(p->uniformSlot==i) {
            slot=i; Uniform(o,p->type,p->name,p->sourceName,&slot,1);
        }
        fputs("};\n",o);
    }
    if(m->inputs) {
        fputs("struct cg_Input {\n",o);
        for(v=m->inputs;v;v=v->next) {
            fputs("    ",o); TypeName(o,v->type); fprintf(o," cg_%s [[",v->semantic);
            Attribute(o,m,v,0); fputs("]];\n",o);
        }
        fputs("};\n",o);
    }
    fputs("struct cg_Output {\n",o);
    for(v=m->outputs;v;v=v->next) {
        fputs("    ",o); TypeName(o,v->type); fprintf(o," cg_%s [[",v->semantic);
        Attribute(o,m,v,1); fputs("]];\n",o);
    }
    fputs("};\n",o);
    for(i=2;i<=4;i++) if(m->rowSetters & (1u<<i)) {
        int column;
        fprintf(o,"inline float%d cg_setrow%d(thread float%dx%d &matrix, int row, float%d value) {\n",i,i,i,i,i);
        for(column=0;column<i;column++) fprintf(o,"    matrix[%d][row] = value[%d];\n",column,column);
        fputs("    return value;\n}\n",o);
    }
    for(f=m->functions;f;f=f->next) { Signature(o,f); fputs(";\n",o); }
    for(f=m->functions;f;f=f->next) { Signature(o,f); fputc('\n',o); Stmts(o,f->body,0); fputc('\n',o); }
    fprintf(o,"%s cg_Output %s(",m->stage==MSL_VERTEX?"vertex":"fragment",m->exportName);
    if(m->inputs) fputs("cg_Input cg_in [[stage_in]]",o);
    if(m->uniformSlots) {
        if(m->inputs) fputs(", ",o);
        fputs("constant cg_Uniforms &cg_u [[buffer(0)]]",o);
    }
    { int comma=m->inputs!=NULL || m->uniformSlots!=0;
      for(p=m->bindings;p;p=p->bindingNext) if(p->resourceSlot>=0) {
        if(comma) fputs(", ",o); comma=1; TypeName(o,p->type);
        fprintf(o," %s [[texture(%d)]], sampler %s_s [[sampler(%d)]]",p->name,p->resourceSlot,p->name,p->resourceSlot);
      }
    }
    fputs(")\n{\n",o);
    for(p=m->entry->parameters;p;p=p->next) {
        if(p->resourceSlot>=0) continue;
        fputs("    ",o); TypeName(o,p->type); fprintf(o," %s;\n",p->name);
        if(p->uniformSlot>=0) { slot=p->uniformSlot; Uniform(o,p->type,p->name,p->sourceName,&slot,2); }
    }
    for(p=m->globals;p;p=p->next) if(p->resourceSlot<0) {
        fputs("    ",o); TypeName(o,p->type); fprintf(o," %s;\n",p->name);
        slot=p->uniformSlot; Uniform(o,p->type,p->name,p->sourceName,&slot,2);
    }
    for(v=m->inputs;v;v=v->next) fprintf(o,"    %s = cg_in.cg_%s;\n",v->path,v->semantic);
    fputs("    ",o);
    if(m->entry->result.base!=MSL_VOID) { TypeName(o,m->entry->result); fputs(" cg_value = ",o); }
    fprintf(o,"%s(",m->entry->name);
    for(p=m->entry->parameters;p;p=p->next) { fputs(p->name,o); if(p->resourceSlot>=0) fprintf(o,", %s_s",p->name); if(p->next) fputs(", ",o); }
    GlobalArgs(o,m->globals,m->entry->parameters!=NULL);
    fputs(");\n    cg_Output cg_result;\n",o);
    for(v=m->outputs;v;v=v->next) fprintf(o,"    cg_result.cg_%s = %s;\n",v->semantic,v->path);
    fputs("    return cg_result;\n}\n",o);
    return !ferror(o);
}

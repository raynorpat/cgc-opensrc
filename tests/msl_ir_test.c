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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "slglobals.h"
#include "msl_ir.h"
#define CHECK(x) do { if (!(x)) { fprintf(stderr,"check failed at line %d: %s\n",__LINE__,#x); return 1; } } while(0)
typedef struct Alloc_Rec { void *ptr[64]; int count, fail; } Alloc;
static void *Allocate(void *arg,size_t size)
{
    Alloc *a=(Alloc *)arg; void *p;
    if (a->count == a->fail) return NULL;
    p=(malloc)(size); if(p) { memset(p,0xa5,size); a->ptr[a->count++]=p; } return p;
}
static int VerifyMutations(void)
{
    MslModule m; MslDiagnostic diag; MslFunction f,other;
    MslStmt statement,declStmt; MslDecl decl,binding,second,member;
    MslExpr expr,a,b; MslRecord record;
    MslInterface interfaces[17]; char names[17][16]; int i;
    MslType scalar,vector;
    memset(&m,0,sizeof(m)); memset(&f,0,sizeof(f)); memset(&other,0,sizeof(other));
    memset(&statement,0,sizeof(statement)); memset(&declStmt,0,sizeof(declStmt));
    memset(&decl,0,sizeof(decl)); memset(&binding,0,sizeof(binding));
    memset(&second,0,sizeof(second)); memset(&member,0,sizeof(member));
    memset(&record,0,sizeof(record)); memset(&expr,0,sizeof(expr));
    memset(&a,0,sizeof(a)); memset(&b,0,sizeof(b)); memset(&scalar,0,sizeof(scalar));
    scalar.base=MSL_FLOAT; scalar.lanes=1; vector=scalar; vector.lanes=4;
    m.stage=MSL_FRAGMENT; m.functions=m.entry=&f; m.sourceEntry="main"; m.exportName="cg_mslf_main";
    f.name="entry"; f.result=scalar; f.body=&statement;
    statement.kind=MSL_RETURN; statement.value=&expr;
    expr.kind=MSL_LITERAL; expr.type=scalar; expr.text="1.0f";
    CHECK(MslVerifyModule(&m,&diag));
    /* Type shape, recursive record and overflowing layout defenses. */
    expr.type.lanes=5; CHECK(!MslVerifyModule(&m,&diag)); expr.type=scalar;
    record.name="R"; record.members=&member; member.name="field"; member.type=scalar;
    { MslType t=scalar; t.base=MSL_RECORD; t.record=&record;
      CHECK(MslTypeSlots(t)==1); member.type=t;
      decl.name="local"; decl.type=t; f.locals=&decl;
      CHECK(!MslVerifyModule(&m,&diag)); CHECK(MslTypeSlots(t)<0); f.locals=NULL; member.type=scalar;
      t.base=MSL_ARRAY; record.arrayCount=256; CHECK(MslTypeSlots(t)==256);
      record.arrayCount=257; CHECK(MslTypeSlots(t)<0);
      record.arrayCount=2147483647; CHECK(MslTypeSlots(t)<0);
    }
    /* Binding coverage: exact buffer limit, overlap, missing slots, pairs. */
    binding.name="binding"; binding.sourceName="value"; binding.type=scalar;
    binding.uniformSlot=0; binding.resourceSlot=-1; m.bindings=&binding; m.uniformSlots=1;
    CHECK(MslVerifyModule(&m,&diag));
    binding.uniformSlot=256; CHECK(!MslVerifyModule(&m,&diag)); binding.uniformSlot=0;
    second=binding; second.name="second"; second.sourceName="second"; binding.bindingNext=&second;
    CHECK(!MslVerifyModule(&m,&diag)); second.uniformSlot=1; m.uniformSlots=2;
    CHECK(MslVerifyModule(&m,&diag)); binding.bindingNext=NULL; m.uniformSlots=0;
    binding.type.base=MSL_TEXTURE2D; binding.resourceSlot=15;
    CHECK(MslVerifyModule(&m,&diag)); binding.resourceSlot=16;
    CHECK(!MslVerifyModule(&m,&diag)); binding.resourceSlot=15; second=binding; binding.bindingNext=&second;
    CHECK(!MslVerifyModule(&m,&diag)); m.bindings=NULL;
    /* The source semantic allowlist cannot express 65 components; test the
       target ABI limit directly with distinct synthetic identities. */
    memset(interfaces,0,sizeof(interfaces));
    for(i=0;i<17;i++) {
        sprintf(names[i],"USER%d",i); interfaces[i].semantic=names[i];
        interfaces[i].path="value"; interfaces[i].type=vector; interfaces[i].attribute=-1;
        if(i<15) interfaces[i].next=&interfaces[i+1];
    }
    m.inputs=interfaces; CHECK(MslVerifyModule(&m,&diag));
    interfaces[15].next=&interfaces[16]; interfaces[16].type=scalar;
    CHECK(!MslVerifyModule(&m,&diag)); interfaces[15].next=NULL;
    interfaces[1].semantic=interfaces[0].semantic; CHECK(!MslVerifyModule(&m,&diag)); interfaces[1].semantic=names[1];
    m.stage=MSL_VERTEX; for(i=0;i<16;i++) interfaces[i].attribute=i;
    CHECK(MslVerifyModule(&m,&diag)); interfaces[15].attribute=16; CHECK(!MslVerifyModule(&m,&diag));
    interfaces[15].attribute=0; CHECK(!MslVerifyModule(&m,&diag)); m.inputs=NULL; m.stage=MSL_FRAGMENT;
    /* Declaration identity, type, writable-address and expression contracts. */
    decl.type=scalar; decl.name="value"; f.locals=&decl;
    a.kind=MSL_SYMBOL; a.type=scalar; a.decl=&decl;
    b.kind=MSL_LITERAL; b.type=scalar; b.text="2.0f";
    expr.kind=MSL_ASSIGN; expr.text="="; expr.a=&a; expr.b=&b;
    CHECK(MslVerifyModule(&m,&diag)); decl.readOnly=1;
    CHECK(!MslVerifyModule(&m,&diag)); decl.readOnly=0;
    a.decl=&second; CHECK(!MslVerifyModule(&m,&diag)); a.decl=&decl;
    expr.kind=MSL_UNARY; expr.text="++"; expr.b=NULL;
    CHECK(MslVerifyModule(&m,&diag)); expr.a=&b; CHECK(!MslVerifyModule(&m,&diag)); expr.a=&a;
    expr.kind=MSL_SWIZZLE; expr.text="z"; CHECK(!MslVerifyModule(&m,&diag));
    decl.type=vector; a.type=vector; CHECK(MslVerifyModule(&m,&diag));
    expr.text="q"; CHECK(!MslVerifyModule(&m,&diag));
    expr.kind=MSL_INDEX; expr.b=&b; CHECK(!MslVerifyModule(&m,&diag));
    b.type.base=MSL_INT; CHECK(MslVerifyModule(&m,&diag)); b.type=scalar;
    expr.kind=MSL_CALL; expr.a=&b; expr.b=NULL; expr.text="unknown";
    CHECK(!MslVerifyModule(&m,&diag)); expr.text="sqrt"; CHECK(MslVerifyModule(&m,&diag));
    b.next=&a; CHECK(!MslVerifyModule(&m,&diag)); b.next=NULL;
    expr.text="dfdx"; m.stage=MSL_VERTEX; CHECK(!MslVerifyModule(&m,&diag)); m.stage=MSL_FRAGMENT;
    expr.function=&other; other.name="missing"; other.result=scalar;
    CHECK(!MslVerifyModule(&m,&diag)); expr.function=NULL;
    expr.kind=MSL_SEQUENCE; b.type.base=MSL_INT; CHECK(!MslVerifyModule(&m,&diag)); b.type=scalar;
    CHECK(MslVerifyModule(&m,&diag));
    declStmt.kind=MSL_DECL; declStmt.decl=&decl; declStmt.value=&b; declStmt.next=&statement;
    f.body=&declStmt; CHECK(!MslVerifyModule(&m,&diag)); declStmt.value=NULL;
    CHECK(MslVerifyModule(&m,&diag)); f.body=&statement;
    statement.kind=MSL_BREAK; CHECK(!MslVerifyModule(&m,&diag));
    statement.kind=MSL_DISCARD; statement.value=NULL; m.stage=MSL_VERTEX;
    CHECK(!MslVerifyModule(&m,&diag));
    return 0;
}
int main(void)
{
    MslModule m; MslDiagnostic d; MslFunction *f; MslStmt *s; MslExpr *e;
    Alloc a; void *p; unsigned char *bytes; size_t i; int count;
    CHECK(VerifyMutations()==0);
    memset(&a,0,sizeof(a)); a.fail=64;
    MslInitModule(&m,MSL_FRAGMENT,Allocate,&a);
    CHECK(!MslVerifyModule(&m,&d));
    p=MslAlloc(&m,17); CHECK(p); bytes=(unsigned char *)p;
    for(i=0;i<17;i++) CHECK(bytes[i]==0);
    f=(MslFunction *)MslAlloc(&m,sizeof(*f)); CHECK(f);
    s=(MslStmt *)MslAlloc(&m,sizeof(*s)); CHECK(s);
    e=(MslExpr *)MslAlloc(&m,sizeof(*e)); CHECK(e);
    m.functions=m.entry=f; m.exportName="cg_mslf_main"; m.sourceEntry="main";
    f->name="helper"; f->result.base=MSL_FLOAT; f->result.lanes=1; f->body=s;
    s->kind=MSL_RETURN; s->value=e;
    e->kind=MSL_LITERAL; e->type=f->result; e->text="1.0f";
    CHECK(MslVerifyModule(&m,&d));
    e->type.base=MSL_INT; CHECK(!MslVerifyModule(&m,&d)); e->type=f->result;
    e->kind=MSL_SYMBOL; CHECK(!MslVerifyModule(&m,&d)); e->kind=MSL_LITERAL;
    m.stage=(MslStage)99; CHECK(!MslVerifyModule(&m,&d)); m.stage=MSL_FRAGMENT;
    s->kind=MSL_CONTINUE; CHECK(!MslVerifyModule(&m,&d)); s->kind=MSL_RETURN;
    a.fail=a.count; count=a.count;
    CHECK(!MslAlloc(&m,1)); CHECK(m.failed); CHECK(!MslAlloc(&m,1)); CHECK(a.count==count);
    CHECK(!MslVerifyModule(&m,&d));
    for(count=0;count<a.count;count++) (free)(a.ptr[count]);
    return 0;
}

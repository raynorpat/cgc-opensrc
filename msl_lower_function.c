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

static int Dependencies(MslModule *,MslFunction *);

int MslLVisitExpr(MslModule *m, MslExpr *e)
{
    for (; e; e=e->next) {
        if (e->function && !MslLVisitFunction(m,e->function)) return 0;
        if (!MslLVisitExpr(m,e->a) || !MslLVisitExpr(m,e->b) || !MslLVisitExpr(m,e->c)) return 0;
    }
    return 1;
}

int MslLVisitStmt(MslModule *m, MslStmt *s)
{
    for (; s; s=s->next)
        if (!MslLVisitExpr(m,s->value) || !MslLVisitExpr(m,s->step) ||
            !MslLVisitStmt(m,s->body) || !MslLVisitStmt(m,s->other) || !MslLVisitStmt(m,s->init)) return 0;
    return 1;
}

int MslLVisitFunction(MslModule *m, MslFunction *f)
{
    if (f->visit==1) return MslFail(m,6607,&f->loc,"Metal recursion is unsupported");
    if (f->visit==2) return 1;
    f->visit=1;
    if (!MslLVisitStmt(m,f->body)) return 0;
    if(!Dependencies(m,f)) return 0;
    f->visit=2; return 1;
}

static int UsesExpr(const MslExpr *e,const MslDecl *decl)
{
    const MslDependency *dep;
    for(;e;e=e->next) {
        if(e->kind==MSL_SYMBOL && e->decl==decl) return 1;
        if(e->function) for(dep=e->function->globals;dep;dep=dep->next)
            if(dep->decl==decl) return 1;
        if(UsesExpr(e->a,decl) || UsesExpr(e->b,decl) || UsesExpr(e->c,decl)) return 1;
    }
    return 0;
}
static int UsesStmt(const MslStmt *s,const MslDecl *decl)
{
    for(;s;s=s->next)
        if(UsesExpr(s->value,decl) || UsesExpr(s->step,decl) || UsesStmt(s->body,decl) ||
           UsesStmt(s->other,decl) || UsesStmt(s->init,decl)) return 1;
    return 0;
}
static int Dependencies(MslModule *m,MslFunction *f)
{
    MslDecl *decl; MslDependency *dep,**tail;
    for(decl=m->globals;decl;decl=decl->next) if(UsesStmt(f->body,decl)) {
        dep=(MslDependency *)MslAlloc(m,sizeof(*dep)); if(!dep) return 0;
        dep->decl=decl; tail=&f->globals;
        while(*tail && strcmp((*tail)->decl->sourceName,decl->sourceName)<0) tail=&(*tail)->next;
        dep->next=*tail; *tail=dep;
    }
    return 1;
}

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
void MslInitModule(MslModule *m, MslStage stage,
                   void *(*alloc)(void *, size_t), void *arg)
{
    memset(m, 0, sizeof(*m)); m->stage = stage; m->alloc = alloc; m->allocArg = arg;
    m->uniformAddressSpace=MSL_CONSTANT;
}
void *MslAlloc(MslModule *m, size_t size)
{
    void *p;
    if (m->failed) return NULL;
    p = m->alloc ? m->alloc(m->allocArg, size) : NULL;
    if (!p) { MslFail(m, 0, NULL, "Metal allocation failure"); return NULL; }
    memset(p, 0, size); return p;
}
const char *MslString(MslModule *m, const char *s)
{
    char *p = (char *) MslAlloc(m, strlen(s) + 1);
    if (p) strcpy(p, s);
    return p;
}
int MslFail(MslModule *m, int code, const SourceLoc *loc, const char *reason)
{
    if (!m->failed) {
        m->diagnostic.code = code; m->diagnostic.reason = reason;
        if (loc) m->diagnostic.loc = *loc;
    }
    m->failed = 1; return 0;
}

static int TypeSlots(MslType t,int depth)
{
    MslDecl *d; int size=0,n,count=0;
    if(depth>64 || t.lanes<1 || t.lanes>4) return -1;
    if(t.base==MSL_MATRIX) return t.lanes>=2?t.lanes:-1;
    if(t.base==MSL_ARRAY) {
        if(!t.record || !t.record->members || t.record->arrayCount<1) return -1;
        n=TypeSlots(t.record->members->type,depth+1);
        if(n<=0 || n>256 || t.record->arrayCount>256/n) return -1;
        return n*t.record->arrayCount;
    }
    if(t.base!=MSL_RECORD) return t.base>=MSL_BOOL && t.base<=MSL_FLOAT?1:-1;
    if(!t.record || !t.record->members) return -1;
    for(d=t.record->members;d;d=d->next) {
        if(++count>256) return -1;
        n=TypeSlots(d->type,depth+1);
        if(n<0 || n>256-size) return -1;
        size+=n;
    }
    return size;
}
int MslTypeSlots(MslType t)
{
    return TypeSlots(t,0);
}

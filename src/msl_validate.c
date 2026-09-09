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

#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include "slglobals.h"
#include "cg_ir.h"
#include "msl_hal.h"

#ifdef CGC_MSL_FAULT_TEST
static unsigned long mslAllocationCount;
#endif
static void *MslPoolAlloc(void *arg, size_t size)
{
#ifdef CGC_MSL_FAULT_TEST
    const char *fail=getenv("CGC_TEST_MSL_FAIL_AT");
    if (fail && mslAllocationCount++ == strtoul(fail,NULL,10)) return NULL;
#endif
    return mem_Alloc((MemoryPool *)arg,size);
}
static int Process(const MslProfileDesc *p,const CgIRModule *source,FILE *output,MslDiagnostic *diagnostic)
{
    MslModule m; MemoryPool *pool=mem_CreatePool(16384,8); int ok;
    memset(diagnostic,0,sizeof(*diagnostic));
    if(!pool) { diagnostic->reason="Metal allocation failure"; return 0; }
    MslInitModule(&m,p->stage,MslPoolAlloc,pool);
    ok=MslLowerCgIR(&m,p,source);
    if(ok) ok=MslVerifyModule(&m,diagnostic);
    else *diagnostic=m.diagnostic;
    if(ok && output) {
#ifdef CGC_MSL_FAULT_TEST
        if(getenv("CGC_TEST_MSL_WRITER_FAIL")) {
            fputs("deliberate partial writer output",output); ok=0;
        } else
#endif
            ok=MslWriteModule(output,&m);
        if(!ok) diagnostic->reason="Metal output write failure";
    }
    mem_FreePool(pool); return ok;
}
int MslValidateCgIR(const MslProfileDesc *profile,const CgIRModule *source,MslDiagnostic *diagnostic)
{
    return Process(profile,source,NULL,diagnostic);
}
int MslGenerateCgIR(const MslProfileDesc *profile,const CgIRModule *source,FILE *output,MslDiagnostic *diagnostic)
{
    return Process(profile,source,output,diagnostic);
}

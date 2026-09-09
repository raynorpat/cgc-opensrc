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
INCLUDING WITHOUT LIMITATION, WARRANTIES OF TITLE, NON-INFRINGEMENT,
MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL NVIDIA BE LIABLE FOR ANY SPECIAL, INDIRECT,
INCIDENTAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
\****************************************************************************/

/* Constant-default folding is a frontend service, separate from core IR. */
#include <stddef.h>
#include <stdio.h>
#include "slglobals.h"
#include "cg_ir.h"
#include "cg_ir_lower.h"

/* Resolve constant dependencies on a private tree. Source defaults must not
 * be mutated by validation; bound recursion also rejects cyclic constants. */
static expr *lResolveDefault(CgIRModule *module,expr *value,int depth)
{
    expr *copy; Symbol *symbol;
    if(!value || module->failed) return NULL;
    if(depth>1024) { module->failed=1; return NULL; }
    if(value->common.kind==SYMB_N) {
        symbol=value->sym.symbol;
        if(symbol && symbol->kind==VARIABLE_S &&
           (GetQualifiers(symbol->type)&TYPE_QUALIFIER_CONST) && symbol->details.var.init) {
            value=symbol->details.var.init;
            if(value->common.kind==BINARY_N && value->bin.op==EXPR_LIST_OP) value=value->bin.left;
            return lResolveDefault(module,value,depth+1);
        }
    }
    copy=(expr *)module->alloc(module->allocArg,sizeof(*copy));
    if(!copy) { module->failed=1; return NULL; }
    *copy=*value;
    switch(copy->common.kind) {
    case UNARY_N: copy->un.arg=lResolveDefault(module,value->un.arg,depth+1); break;
    case BINARY_N:
        copy->bin.left=lResolveDefault(module,value->bin.left,depth+1);
        copy->bin.right=lResolveDefault(module,value->bin.right,depth+1); break;
    case TRINARY_N:
        copy->tri.arg1=lResolveDefault(module,value->tri.arg1,depth+1);
        copy->tri.arg2=lResolveDefault(module,value->tri.arg2,depth+1);
        copy->tri.arg3=lResolveDefault(module,value->tri.arg3,depth+1); break;
    default: break;
    }
    if(module->failed) return NULL;
    if(IsStruct(copy->common.type)) return copy;
    return ConstantFoldNode(copy,NULL,0);
}

CgIRExpr *CgIRLowerUniformDefault(CgIRModule *module, const CgIRDecl *decl)
{
    expr *value;
    if (!module || !decl || !decl->symbol)
        return NULL;
    value = decl->symbol->details.var.init;
    if (!value)
        return NULL;
    if (value->common.kind == BINARY_N && value->bin.op == EXPR_LIST_OP)
        value = value->bin.left;
    value=lResolveDefault(module,value,0);
    if(module->failed) return NULL;
    return CgIRLowerDefaultExpression(module,value,decl->type,&decl->loc);
}

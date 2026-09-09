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

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include "slglobals.h"
#include "cg_ir.h"
#include "msl_hal.h"
static int MslCaps(int bit)
{
    switch (bit) {
    case CAPS_LATE_BINDINGS: case CAPS_INDEXED_ARRAYS:
    case CAPS_DONT_FLATTEN_IF_STATEMENTS: case CAPS_MATRIX_CONSTRUCTOR_AST:
    case CAPS_AGGREGATE_DEFAULT_BINDINGS: case CAPS_PRESERVE_ENTRY_RETURNS:
    case CAPS_PRESERVE_NATIVE_AGGREGATE_TEMPS:
    case CAPS_ENTRY_INOUT_PARAMETERS: case CAPS_PRESERVE_TERMINAL_ENTRY_RETURN:
    case CAPS_DEFER_RECURSION_DIAGNOSTICS: case CAPS_CONDITIONAL_SIDE_EFFECTS:
    case CAPS_TYPED_INC_DEC_EXPRESSIONS: case CAPS_PRESERVE_COMMA_EXPRESSIONS:
    case CAPS_PRESERVE_INLINE_HELPERS: case CAPS_AGGREGATE_DEFAULT_INITIALIZERS:
    case CAPS_NATIVE_VALUE_OPERATIONS:
    case CAPS_PRESERVE_SIDE_EFFECTING_AGGREGATE_TEMPS: return 1;
    default: return 0;
    }
}
static int NumericBuiltin(const char *name,Type *type,int arity,int index,int dot)
{
    SourceLoc loc={0,0}; Symbol *root,*symbol; Type *function; TypeList **tail,*param; int i,atom;
    function=NewType(TYPE_CATEGORY_FUNCTION | TYPE_MISC_INTERNAL,0);
    function->fun.rettype=dot?FloatType:type; tail=&function->fun.paramtypes;
    for(i=0;i<arity;i++) {
        param=(TypeList *)mem_Alloc(CurrentScope->pool,sizeof(*param)); if(!param) return 0;
        param->type=type; param->next=NULL; *tail=param; tail=&param->next;
    }
    atom=AddAtom(atable,name); root=LookUpSymbol(CurrentScope,atom);
    if(root) {
        symbol=NewSymbol(&loc,CurrentScope,atom,function,FUNCTION_S);
        symbol->details.fun.overload=root->details.fun.overload; root->details.fun.overload=symbol;
    } else symbol=AddSymbol(&loc,CurrentScope,atom,function,FUNCTION_S);
    if(!symbol) return 0;
    symbol->properties|=SYMB_IS_BUILTIN;
    symbol->details.fun.group=MSL_BUILTIN_GROUP; symbol->details.fun.index=index;
    return 1;
}
static int MslNames(slHAL *hal)
{
    int i; SourceLoc loc={0,0}; Type *type; TypeList *sampler,*coord; Symbol *symbol;
    const char *names[]={"tex2Dlod","texCUBElod"};
    (void)hal;
    /* These selected-profile builtins are absent from the shared Cg 2.0
       catalog. Use resolved builtin group/index identities, not call names. */
    for(i=0;i<2;i++) {
        sampler=(TypeList *)mem_Alloc(CurrentScope->pool,sizeof(*sampler));
        coord=(TypeList *)mem_Alloc(CurrentScope->pool,sizeof(*coord));
        if(!sampler || !coord) return 0;
        sampler->type=GetSamplerType(i?CG_SAMPLER_CUBE:CG_SAMPLER_2D); sampler->next=coord;
        coord->type=Float4Type; coord->next=NULL;
        type=NewType(TYPE_CATEGORY_FUNCTION | TYPE_MISC_INTERNAL,0);
        type->fun.rettype=Float4Type; type->fun.paramtypes=sampler;
        symbol=AddSymbol(&loc,CurrentScope,AddAtom(atable,names[i]),type,FUNCTION_S);
        if(!symbol) return 0;
        symbol->properties|=SYMB_IS_BUILTIN;
        symbol->details.fun.group=MSL_BUILTIN_GROUP;
        symbol->details.fun.index=i?MSL_BUILTIN_TEXCUBELOD:MSL_BUILTIN_TEX2DLOD;
        symbol->details.fun.profileSelector.isOpen=1;
    }
    for(i=0;i<4;i++) {
        Type *signedType=GetStandardTypeKind(CG_SCALAR_INT,i?i+1:0,0);
        Type *unsignedType=GetStandardTypeKind(CG_SCALAR_UINT,i?i+1:0,0);
        if(!NumericBuiltin("min",signedType,2,MSL_BUILTIN_MIN,0) ||
           !NumericBuiltin("max",signedType,2,MSL_BUILTIN_MAX,0) ||
           !NumericBuiltin("clamp",signedType,3,MSL_BUILTIN_CLAMP,0) ||
           !NumericBuiltin("min",unsignedType,2,MSL_BUILTIN_MIN,0) ||
           !NumericBuiltin("max",unsignedType,2,MSL_BUILTIN_MAX,0) ||
           !NumericBuiltin("clamp",unsignedType,3,MSL_BUILTIN_CLAMP,0)) return 0;
        if(i && !NumericBuiltin("mul",GetStandardTypeKind(CG_SCALAR_FLOAT,i+1,0),2,MSL_BUILTIN_DOT,1)) return 0;
    }
    return 1;
}
static int MslFree(slHAL *hal) { hal->localData = NULL; return 1; }
static int MslHeader(FILE *out) { (void) out; return 1; }
static int MslVarying(SourceLoc *loc, Symbol *sym, int semantic,
                      Binding *bind, int output)
{
    /* Interface binding belongs to verified IR lowering, not the legacy
       connector rewrite. Hidden bindings prevent synthetic $vin copies. */
    (void) loc; (void) sym;
    bind->conn.kind = BK_CONNECTOR;
    bind->conn.rname = semantic;
    bind->conn.properties |= BIND_IS_BOUND | BIND_HIDDEN | BIND_VARYING |
                            (output ? BIND_OUTPUT : BIND_INPUT);
    return 1;
}
static int MslUniform(SourceLoc *loc, Symbol *sym, Binding *bind)
{
    (void) loc; (void) sym;
    bind->none.properties |= BIND_IS_BOUND | BIND_UNIFORM;
    return 1;
}
static int MslProcess(const CgIRModule *source,int emit)
{
    MslDiagnostic diagnostic;
    const MslProfileDesc *profile=(const MslProfileDesc *)Cg->theHAL->localData;
    int ok=emit?MslGenerateCgIR(profile,source,Cg->options.outfd,&diagnostic):MslValidateCgIR(profile,source,&diagnostic);
    if(!ok) {
        if(diagnostic.code) SemanticError(&diagnostic.loc,diagnostic.code,"%s",diagnostic.reason);
        else InternalError(&diagnostic.loc,ERROR_S_CG_IR_INVARIANT,diagnostic.reason?diagnostic.reason:"Metal lowering");
        ReportProfileCallPath(diagnostic.symbol);
    }
    return ok;
}
static int MslValidate(SourceLoc *loc, const CgIRModule *source)
{ (void) loc; return MslProcess(source, 0); }
static int MslGenerate(SourceLoc *loc, const CgIRModule *source)
{ (void) loc; return MslProcess(source, 1); }
int MslInitHAL(slHAL *hal, const MslProfileDesc *profile)
{
    if (Cg->options.languageVersion != CG_LANGUAGE_2_0) {
        /* Diagnostics are not initialized during profile selection. */
        fprintf(stderr, "cgc: error C6600: Metal profiles require Cg 2.0\n");
        return 0;
    }
    hal->localData = (void *) profile;
    hal->vendor = "Apple Metal"; hal->version = "2.0"; hal->comment = "//";
    hal->FreeHAL = MslFree; hal->RegisterNames = MslNames;
    hal->GetCapsBit = MslCaps; hal->PrintCodeHeader = MslHeader;
    hal->BindVaryingSemantic = MslVarying; hal->BindUniformUnbound = MslUniform;
    hal->incid = profile->stage == MSL_VERTEX ? 36 : 38;
    hal->outcid = hal->incid + 1;
    hal->ValidateIR = MslValidate; hal->GenerateIR = MslGenerate;
    return 1;
}
int RegisterProfiles_msl(void)
{
    RegisterProfile(InitHAL_mslv, "mslv", PROFILE_MSLV_ID);
    RegisterProfile(InitHAL_mslf, "mslf", PROFILE_MSLF_ID);
    SetProfileIdentity("mslv", CG_PROFILE_STAGE_VERTEX, "vs", 10);
    SetProfileIdentity("mslf", CG_PROFILE_STAGE_FRAGMENT, "ps", 10);
    return 1;
}

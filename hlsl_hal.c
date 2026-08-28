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
// hlsl_hal.c
//

#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "slglobals.h"
#include "hlsl_hal.h"

#define NUMELS(x) (sizeof(x) / sizeof((x)[0]))

// Static HAL callbacks:
static int InitHAL_hlsl(slHAL *fHAL);
static int FreeHAL_hlsl(slHAL *fHAL);
static int RegisterNames_hlsl(slHAL *fHAL);
static int GetConnectorID_hlsl(int name);
static int GetConnectorAtom_hlsl(int name);
static int GetConnectorUses_hlsl(int cid, int pid);
static int GetConnectorRegister_hlsl(int cid, int ByIndex, int ratom, Binding *fBind);
static int GetCapsBit_hlsl(int bitNumber);
static int CheckInternalFunction_hlsl(Symbol *fSymb, int *group);
static int BindUniformUnbound_hlsl(SourceLoc *loc, Symbol *fSymb, Binding *fBind);
static int BindVaryingSemantic_hlsl(SourceLoc *loc, Symbol *fSymb, int semantic,
                                    Binding *fBind, int IsOutVal);
static int PrintCodeHeader_hlsl(FILE *out);
static int GenerateCode_hlsl(SourceLoc *loc, Scope *fScope, Symbol *program);

// Temporary HlslModule for the skeleton; Task 2 replaces these with
// the permanent definitions from hlsl_ir.h:

typedef struct HlslLoc_Rec {
    int file;
    int line;
} HlslLoc;

typedef struct HlslModule_Rec {
    HlslStage stage;
    HlslLoc errorLoc;
    const char *errorReason;
    int errors;
} HlslModule;

// Forward declarations for the temporary skeleton phase functions.
// Task 2 moves these to their permanent files (hlsl_lower, etc.).
static int HlslSkeletonLower(HlslModule *module, const HlslProfileDesc *profile,
                             SourceLoc *loc, Scope *scope, Symbol *program);
static int HlslSkeletonEntryWrapper(HlslModule *module, const HlslProfileDesc *profile);
static int HlslSkeletonLegalize(HlslModule *module, const HlslProfileDesc *profile);
static int HlslSkeletonAllocate(HlslModule *module, const HlslProfileDesc *profile);
static int HlslSkeletonValidate(HlslModule *module, const HlslProfileDesc *profile);
static int HlslSkeletonWrite(FILE *out, const HlslModule *module, const HlslProfileDesc *profile);

static void HlslInitModule(HlslModule *module, HlslStage stage)
{
    module->stage = stage;
    module->errorLoc.file = 0;
    module->errorLoc.line = 0;
    module->errorReason = NULL;
    module->errors = 0;
}

///////////////////////////////////////////////////////////////////////////////
/////////////////////////// Profile Registration //////////////////////////////
///////////////////////////////////////////////////////////////////////////////

/*
 * RegisterProfiles_hlsl() - Register both HLSL profiles.
 */

int RegisterProfiles_hlsl(void)
{
    RegisterProfile(InitHAL_hlslv, PROFILE_HLSLV_NAME, PROFILE_HLSLV_ID);
    SetProfileIdentity(PROFILE_HLSLV_NAME, CG_PROFILE_STAGE_NEUTRAL, NULL, 0);
    RegisterProfile(InitHAL_hlslf, PROFILE_HLSLF_NAME, PROFILE_HLSLF_ID);
    SetProfileIdentity(PROFILE_HLSLF_NAME, CG_PROFILE_STAGE_NEUTRAL, NULL, 0);
    return 1;
} // RegisterProfiles_hlsl

/*
 * InitHAL_hlsl() - Shared HAL initialization.
 */

static int InitHAL_hlsl(slHAL *fHAL, const HlslProfileDesc *profile)
{
    // Override only the callbacks the HLSL profiles need.
    // The default HAL (hal.c) supplies the others.
    fHAL->FreeHAL = FreeHAL_hlsl;
    fHAL->RegisterNames = RegisterNames_hlsl;
    fHAL->GetConnectorID = GetConnectorID_hlsl;
    fHAL->GetConnectorAtom = GetConnectorAtom_hlsl;
    fHAL->GetConnectorUses = GetConnectorUses_hlsl;
    fHAL->GetConnectorRegister = GetConnectorRegister_hlsl;
    fHAL->GetCapsBit = GetCapsBit_hlsl;
    fHAL->CheckInternalFunction = CheckInternalFunction_hlsl;
    fHAL->BindUniformUnbound = BindUniformUnbound_hlsl;
    fHAL->BindVaryingSemantic = BindVaryingSemantic_hlsl;
    fHAL->PrintCodeHeader = PrintCodeHeader_hlsl;
    fHAL->GenerateCode = GenerateCode_hlsl;

    // Data members:
    fHAL->vendor = VENDOR_STRING_HLSL;
    fHAL->version = VERSION_STRING_HLSL;
    fHAL->comment = "//";

    // Point to the stage descriptor:
    fHAL->localData = (void *) profile;

    return 1;
} // InitHAL_hlsl

/*
 * FreeHAL_hlsl()
 */

static int FreeHAL_hlsl(slHAL *fHAL)
{
    (void) fHAL;
    return 1;
} // FreeHAL_hlsl

/*
 * RegisterNames_hlsl() - Register atoms for connectors and connector registers.
 * For the skeleton, there are no connectors yet, so this is a no-op.
 * Task 3 replaces this with real atom registration.
 */

static int RegisterNames_hlsl(slHAL *fHAL)
{
    const HlslProfileDesc *profile;
    int i, j;

    profile = (const HlslProfileDesc *) fHAL->localData;
    for (i = 0; i < profile->numConnectors; i++) {
        ConnectorDescriptor *conn = &profile->connectors[i];
        conn->name = AddAtom(atable, conn->sname);
        for (j = 0; j < conn->numregs; j++)
            conn->registers[j].name = AddAtom(atable, conn->registers[j].sname);
    }
    return 1;
} // RegisterNames_hlsl

/*
 * GetConnectorID_hlsl()
 */

static int GetConnectorID_hlsl(int name)
{
    int i;
    const HlslProfileDesc *profile;

    profile = (const HlslProfileDesc *) Cg->theHAL->localData;
    for (i = 0; i < profile->numConnectors; i++)
        if (name == profile->connectors[i].name)
            return profile->connectors[i].cid;

    return 0;
} // GetConnectorID_hlsl

/*
 * GetConnectorAtom_hlsl()
 */

static int GetConnectorAtom_hlsl(int name)
{
    const HlslProfileDesc *profile;
    const ConnectorDescriptor *conn;

    profile = (const HlslProfileDesc *) Cg->theHAL->localData;
    conn = LookupConnectorHAL(profile->connectors, name, profile->numConnectors);
    return conn ? conn->name : 0;
} // GetConnectorAtom_hlsl

/*
 * GetConnectorUses_hlsl()
 */

static int GetConnectorUses_hlsl(int cid, int pid)
{
    const HlslProfileDesc *profile;
    const ConnectorDescriptor *conn;

    (void) pid;
    profile = (const HlslProfileDesc *) Cg->theHAL->localData;
    conn = LookupConnectorHAL(profile->connectors, cid, profile->numConnectors);
    return conn ? conn->properties : CONNECTOR_IS_USELESS;
} // GetConnectorUses_hlsl

/*
 * GetConnectorRegister_hlsl()
 */

static int GetConnectorRegister_hlsl(int cid, int ByIndex, int ratom, Binding *fBind)
{
    const HlslProfileDesc *profile;
    ConnectorDescriptor *conn;
    ConnectorRegisters *regs;
    int i;

    (void) ByIndex;
    profile = (const HlslProfileDesc *) Cg->theHAL->localData;
    conn = LookupConnectorHAL(profile->connectors, cid, profile->numConnectors);
    if (!conn)
        return 0;

    regs = conn->registers;
    if (!regs)
        return 0;

    for (i = 0; i < conn->numregs; i++) {
        if (ratom == regs[i].name) {
            SetSymbolConnectorBindingHAL(fBind, &regs[i]);
            return 1;
        }
    }
    return 0;
} // GetConnectorRegister_hlsl

/*
 * GetCapsBit_hlsl()
 */

static int GetCapsBit_hlsl(int bitNumber)
{
    switch (bitNumber) {
    case CAPS_LATE_BINDINGS:
    case CAPS_INDEXED_ARRAYS:
    case CAPS_DONT_FLATTEN_IF_STATEMENTS:
    case CAPS_MATRIX_CONSTRUCTOR_AST:
    case CAPS_AGGREGATE_DEFAULT_BINDINGS:
    case CAPS_PRESERVE_ENTRY_RETURNS:
    case CAPS_PRESERVE_NATIVE_AGGREGATE_TEMPS:
        return 1;
    default:
        return 0;
    }
} // GetCapsBit_hlsl

/*
 * CheckInternalFunction_hlsl() - Check for internally implemented function.
 */

static int CheckInternalFunction_hlsl(Symbol *fSymb, int *group)
{
    (void) fSymb;
    *group = HLSL_BUILTIN_GROUP;
    return 1;
} // CheckInternalFunction_hlsl

/*
 * BindUniformUnbound_hlsl() - Mark uniforms as bound.
 */

static int BindUniformUnbound_hlsl(SourceLoc *loc, Symbol *fSymb,
                                   Binding *fBind)
{
    (void) loc;
    (void) fSymb;
    fBind->none.properties |= BIND_IS_BOUND;
    return 1;
} // BindUniformUnbound_hlsl

/*
 * BindVaryingSemantic_hlsl()
 */

static int BindVaryingSemantic_hlsl(SourceLoc *loc, Symbol *fSymb,
                                    int semantic, Binding *fBind,
                                    int IsOutVal)
{
    // Skeleton: accept nothing; report unknown semantic.
    // Task 3 implements the full table.
    (void) loc;
    (void) fSymb;
    (void) semantic;
    (void) fBind;
    (void) IsOutVal;
    return 0;
} // BindVaryingSemantic_hlsl

/*
 * PrintCodeHeader_hlsl() - Write nothing; header is part of codegen.
 */

static int PrintCodeHeader_hlsl(FILE *out)
{
    (void) out;
    return 1;
} // PrintCodeHeader_hlsl

/*
 * GenerateCode_hlsl() - Orchestrate the backend pipeline.
 */

static int GenerateCode_hlsl(SourceLoc *loc, Scope *fScope, Symbol *program)
{
    HlslModule module;
    const HlslProfileDesc *profile;

    profile = (const HlslProfileDesc *) Cg->theHAL->localData;
    HlslInitModule(&module, profile->stage);
    if (!HlslSkeletonLower(&module, profile, loc, fScope, program) ||
        !HlslSkeletonEntryWrapper(&module, profile) ||
        !HlslSkeletonLegalize(&module, profile) ||
        !HlslSkeletonAllocate(&module, profile) ||
        !HlslSkeletonValidate(&module, profile))
    {
        // Report failure: on error we still continue to preserve
        // transactional output rule.
        return 1;
    }
    if (!HlslSkeletonWrite(Cg->options.outfd, &module, profile))
        return 1;
    return 1;
} // GenerateCode_hlsl

///////////////////////////////////////////////////////////////////////////////
////////////////////////// Skeleton Pipeline Phases ////////////////////////////
///////////////////////////////////////////////////////////////////////////////

/*
 * HlslSkeletonLower() - Temporary lowering stub.
 */

static int HlslSkeletonLower(HlslModule *module, const HlslProfileDesc *profile,
                             SourceLoc *loc, Scope *scope, Symbol *program)
{
    (void) module;
    (void) profile;
    (void) loc;
    (void) scope;
    (void) program;
    return 1;
} // HlslSkeletonLower

/*
 * HlslSkeletonEntryWrapper() - Temporary entry wrapper stub.
 */

static int HlslSkeletonEntryWrapper(HlslModule *module, const HlslProfileDesc *profile)
{
    (void) module;
    (void) profile;
    return 1;
} // HlslSkeletonEntryWrapper

/*
 * HlslSkeletonLegalize() - Temporary legalization stub.
 */

static int HlslSkeletonLegalize(HlslModule *module, const HlslProfileDesc *profile)
{
    (void) module;
    (void) profile;
    return 1;
} // HlslSkeletonLegalize

/*
 * HlslSkeletonAllocate() - Temporary allocation stub.
 */

static int HlslSkeletonAllocate(HlslModule *module, const HlslProfileDesc *profile)
{
    (void) module;
    (void) profile;
    return 1;
} // HlslSkeletonAllocate

/*
 * HlslSkeletonValidate() - Temporary validation stub.
 */

static int HlslSkeletonValidate(HlslModule *module, const HlslProfileDesc *profile)
{
    (void) module;
    (void) profile;
    return 1;
} // HlslSkeletonValidate

/*
 * HlslSkeletonWrite() - Write the minimal stage header and empty entry.
 */

static int HlslSkeletonWrite(FILE *out, const HlslModule *module,
                             const HlslProfileDesc *profile)
{
    (void) module;
    fprintf(out, "// profile %s\n", profile->name);
    fprintf(out, "// target %s\n", profile->target);
    fprintf(out, "void main()\n{\n}\n");
    return 1;
} // HlslSkeletonWrite

///////////////////////////////////////////////////////////////////////////////
///////////////////////// InitHAL_hlslv / InitHAL_hlslf ////////////////////////
///////////////////////////////////////////////////////////////////////////////

/*
 * InitHAL_hlslv() - Initialize HAL for the vertex profile.
 */

int InitHAL_hlslv(slHAL *fHAL)
{
    return InitHAL_hlsl(fHAL, &HlslProfile_hlslv);
} // InitHAL_hlslv

/*
 * InitHAL_hlslf() - Initialize HAL for the pixel profile.
 */

int InitHAL_hlslf(slHAL *fHAL)
{
    return InitHAL_hlsl(fHAL, &HlslProfile_hlslf);
} // InitHAL_hlslf

///////////////////////////////////////////////////////////////////////////////
////////////////////////// End of hlsl_hal.c //////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
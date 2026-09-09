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
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "slglobals.h"
#include "cg_stdlib.h"
#include "hlsl_hal.h"
#include "hlsl_modern.h"

#if !defined(HLSL_CANONICALIZATION_ONLY)

/*
 * RegisterProfiles_hlsl() - Register all HLSL profiles.
 */

int RegisterProfiles_hlsl(void)
{
    RegisterProfile(InitHAL_hlslv, PROFILE_HLSLV_NAME, PROFILE_HLSLV_ID);
    SetProfileIdentity(PROFILE_HLSLV_NAME, CG_PROFILE_STAGE_VERTEX, "vs", 10);
    RegisterProfile(InitHAL_hlslf, PROFILE_HLSLF_NAME, PROFILE_HLSLF_ID);
    SetProfileIdentity(PROFILE_HLSLF_NAME, CG_PROFILE_STAGE_FRAGMENT, "ps", 10);
    RegisterProfile(InitHAL_hlslv40, PROFILE_HLSLV40_NAME,
                    PROFILE_HLSLV40_ID);
    SetProfileIdentity(PROFILE_HLSLV40_NAME, CG_PROFILE_STAGE_VERTEX,
                       "vs", 10);
    RegisterProfile(InitHAL_hlslg40, PROFILE_HLSLG40_NAME,
                    PROFILE_HLSLG40_ID);
    SetProfileIdentity(PROFILE_HLSLG40_NAME, CG_PROFILE_STAGE_GEOMETRY,
                       "gs", 10);
    RegisterProfile(InitHAL_hlslf40, PROFILE_HLSLF40_NAME,
                    PROFILE_HLSLF40_ID);
    SetProfileIdentity(PROFILE_HLSLF40_NAME, CG_PROFILE_STAGE_FRAGMENT,
                       "ps", 10);
    RegisterProfile(InitHAL_hlslv50, PROFILE_HLSLV50_NAME,
                    PROFILE_HLSLV50_ID);
    SetProfileIdentity(PROFILE_HLSLV50_NAME, CG_PROFILE_STAGE_VERTEX,
                       "vs", 10);
    RegisterProfile(InitHAL_hlslg50, PROFILE_HLSLG50_NAME,
                    PROFILE_HLSLG50_ID);
    SetProfileIdentity(PROFILE_HLSLG50_NAME, CG_PROFILE_STAGE_GEOMETRY,
                       "gs", 10);
    RegisterProfile(InitHAL_hlslf50, PROFILE_HLSLF50_NAME,
                    PROFILE_HLSLF50_ID);
    SetProfileIdentity(PROFILE_HLSLF50_NAME, CG_PROFILE_STAGE_FRAGMENT,
                       "ps", 10);
    return 1;
} // RegisterProfiles_hlsl

#endif // !defined(HLSL_CANONICALIZATION_ONLY)

#if !defined(HLSL_PROFILE_REGISTRATION_ONLY)

#define NUMELS(x) (sizeof(x) / sizeof((x)[0]))
#define HLSL_SEMANTIC_NAME_MAX 128

/*
 * HlslParseSemantic() - Split a semantic name into its root and optional
 *         decimal index.  An omitted index denotes zero.  The caller owns
 *         all storage; malformed, overflowing, or truncated names fail.
 */

int HlslParseSemantic(const char *semantic, char *root, size_t rootSize,
                      int *index)
{
    size_t length, rootLength, current;
    int value, digit;

    if (semantic == NULL || root == NULL || rootSize == 0 || index == NULL)
        return 0;
    length = strlen(semantic);
    if (length == 0)
        return 0;
    rootLength = length;
    while (rootLength > 0 && semantic[rootLength - 1] >= '0' &&
           semantic[rootLength - 1] <= '9')
    {
        rootLength--;
    }
    if (rootLength == 0 || rootLength >= rootSize)
        return 0;

    value = 0;
    for (current = rootLength; current < length; current++) {
        digit = semantic[current] - '0';
        if (value > (INT_MAX - digit) / 10)
            return 0;
        value = value * 10 + digit;
    }
    memcpy(root, semantic, rootLength);
    root[rootLength] = '\0';
    *index = value;
    return 1;
} // HlslParseSemantic

static int HlslUpperSemantic(const char *source, char *target, size_t size)
{
    size_t index;
    char ch;

    if (source == NULL || target == NULL || size == 0)
        return 0;
    for (index = 0; source[index] != '\0'; index++) {
        if (index + 1 >= size)
            return 0;
        ch = source[index];
        if (ch >= 'a' && ch <= 'z')
            ch = (char) (ch - 'a' + 'A');
        target[index] = ch;
    }
    if (index == 0)
        return 0;
    target[index] = '\0';
    return 1;
} // HlslUpperSemantic

/*
 * HlslCanonicalSemantic() - Return the stage-and-direction canonical
 *         connector spelling for a source semantic, or NULL when no legal
 *         DirectX 9 interface location exists.  Returned strings are owned
 *         by the immutable stage descriptor.
 */

const char *HlslCanonicalSemantic(const HlslProfileDesc *profile,
                                  const char *semantic, int IsOutVal)
{
    const HlslSemanticDesc *semantics;
    const HlslSemanticAlias *aliases;
    ConnectorRegisters *registers;
    const char *candidate;
    char upper[HLSL_SEMANTIC_NAME_MAX];
    char root[HLSL_SEMANTIC_NAME_MAX];
    char registerRoot[HLSL_SEMANTIC_NAME_MAX];
    int numSemantics, numAliases, numRegisters;
    int semanticIndex, registerIndex, i, j;

    if (profile != NULL &&
        profile->semanticPolicy == HLSL_SEMANTIC_POLICY_MODERN)
    {
        HlslSemanticKind kind;

        if (!HlslUpperSemantic(semantic, upper, sizeof(upper)) ||
            !HlslParseSemantic(upper, root, sizeof(root), &semanticIndex))
        {
            return NULL;
        }
        kind = HlslModernSemantic(profile->stage,
            IsOutVal ? HLSL_DIRECTION_OUTPUT : HLSL_DIRECTION_INPUT,
            root, semanticIndex);
        return kind != HLSL_SEMANTIC_UNSUPPORTED ? semantic : NULL;
    }
    if (profile == NULL ||
        (profile->stage != HLSL_STAGE_VERTEX &&
         profile->stage != HLSL_STAGE_PIXEL) ||
        !HlslUpperSemantic(semantic, upper, sizeof(upper)))
    {
        return NULL;
    }
    if (IsOutVal) {
        semantics = profile->outputSemantics;
        numSemantics = profile->numOutputSemantics;
        aliases = profile->outputAliases;
        numAliases = profile->numOutputAliases;
        registers = profile->outputRegs;
        numRegisters = profile->numOutputRegs;
    } else {
        semantics = profile->inputSemantics;
        numSemantics = profile->numInputSemantics;
        aliases = profile->inputAliases;
        numAliases = profile->numInputAliases;
        registers = profile->inputRegs;
        numRegisters = profile->numInputRegs;
    }
    candidate = upper;
    for (i = 0; i < numAliases; i++) {
        if (!strcmp(upper, aliases[i].source)) {
            candidate = aliases[i].target;
            break;
        }
    }
    if (!HlslParseSemantic(candidate, root, sizeof(root), &semanticIndex))
        return NULL;
    for (i = 0; i < numSemantics; i++) {
        if (strcmp(root, semantics[i].root) ||
            semanticIndex < semantics[i].firstIndex ||
            semanticIndex >= semantics[i].firstIndex + semantics[i].count)
        {
            continue;
        }
        for (j = 0; j < numRegisters; j++) {
            if (HlslParseSemantic(registers[j].sname, registerRoot,
                                  sizeof(registerRoot), &registerIndex) &&
                !strcmp(registerRoot, root) &&
                registerIndex == semanticIndex)
            {
                return registers[j].sname;
            }
        }
        return NULL;
    }
    return NULL;
} // HlslCanonicalSemantic

int HlslProfileHasCapability(const HlslProfileDesc *profile,
                             unsigned int capability)
{
    return profile != NULL &&
           (profile->capabilities & capability) != 0;
} // HlslProfileHasCapability

int HlslHALUsesStableOutputHeader(const slHAL *hal)
{
    if (hal == NULL)
        return 0;
    return hal->pid == PROFILE_HLSLV40_ID ||
           hal->pid == PROFILE_HLSLG40_ID ||
           hal->pid == PROFILE_HLSLF40_ID ||
           hal->pid == PROFILE_HLSLV50_ID ||
           hal->pid == PROFILE_HLSLG50_ID ||
           hal->pid == PROFILE_HLSLF50_ID;
} // HlslHALUsesStableOutputHeader

int HlslProfileIsValid(const HlslProfileDesc *profile)
{
    if (profile == NULL ||
        (profile->stage != HLSL_STAGE_VERTEX &&
         profile->stage != HLSL_STAGE_PIXEL &&
         profile->stage != HLSL_STAGE_GEOMETRY) ||
        (profile->model != HLSL_SHADER_MODEL_3 &&
         profile->model != HLSL_SHADER_MODEL_4 &&
         profile->model != HLSL_SHADER_MODEL_5) ||
        (profile->syntax != HLSL_SYNTAX_LEGACY &&
         profile->syntax != HLSL_SYNTAX_MODERN) ||
        (profile->semanticPolicy != HLSL_SEMANTIC_POLICY_DX9 &&
         profile->semanticPolicy != HLSL_SEMANTIC_POLICY_MODERN) ||
        (profile->resourcePolicy != HLSL_RESOURCE_POLICY_DX9 &&
         profile->resourcePolicy != HLSL_RESOURCE_POLICY_MODERN) ||
        profile->target == NULL || profile->target[0] == '\0' ||
        profile->version == NULL || profile->version[0] == '\0')
    {
        return 0;
    }
    if ((profile->model == HLSL_SHADER_MODEL_3) !=
        (profile->syntax == HLSL_SYNTAX_LEGACY))
    {
        return 0;
    }
    if (profile->model == HLSL_SHADER_MODEL_3) {
        if (profile->semanticPolicy != HLSL_SEMANTIC_POLICY_DX9 ||
            profile->resourcePolicy != HLSL_RESOURCE_POLICY_DX9)
        {
            return 0;
        }
    } else if (profile->semanticPolicy != HLSL_SEMANTIC_POLICY_MODERN ||
               profile->resourcePolicy != HLSL_RESOURCE_POLICY_MODERN)
    {
        return 0;
    }
    if (profile->stage == HLSL_STAGE_GEOMETRY &&
        !HlslProfileHasCapability(profile, HLSL_CAP_GEOMETRY))
    {
        return 0;
    }
    return 1;
} // HlslProfileIsValid

int HlslProfileAllowsBuiltin(const HlslProfileDesc *profile,
                             HlslBuiltin builtin)
{
    if (profile == NULL || builtin == HLSL_BUILTIN_NONE)
        return 0;
    if (profile->stage == HLSL_STAGE_GEOMETRY &&
        HlslBuiltinIsTexture(builtin))
    {
        HlslTextureForm form;

        form = HlslBuiltinTextureForm(builtin);
        return HlslProfileHasCapability(profile, HLSL_CAP_TEXTURE_METHODS) &&
               (form == HLSL_TEXTURE_LOD ||
                form == HLSL_TEXTURE_GRADIENT);
    }
    if (profile->model == HLSL_SHADER_MODEL_3 &&
        profile->stage != HLSL_STAGE_PIXEL &&
        HlslBuiltinIsTexture(builtin) &&
        HlslBuiltinTextureForm(builtin) == HLSL_TEXTURE_GRADIENT)
    {
        return 0;
    }
    return 1;
} // HlslProfileAllowsBuiltin

#if !defined(HLSL_CANONICALIZATION_ONLY)

#define HLSL_MAX_INTERFACE_REGISTERS 32

typedef struct HlslDiagnosticMap_Rec {
    HlslErrorKind kind;
    int code;
    const char *format;
} HlslDiagnosticMap;

static const HlslDiagnosticMap hlslDiagnosticMap[] = {
    { HLSL_ERROR_NONE,                  0, NULL },
    { HLSL_ERROR_UNSUPPORTED_TYPE,      ERROR_S_HLSL_UNSUPPORTED_TYPE },
    { HLSL_ERROR_UNSUPPORTED_OPERATION, ERROR_S_HLSL_UNSUPPORTED_OPERATION },
    { HLSL_ERROR_STAGE_OPERATION,       ERROR_SS_HLSL_STAGE_OPERATION },
    { HLSL_ERROR_SEMANTIC,              ERROR_S_HLSL_SEMANTIC },
    { HLSL_ERROR_INTERFACE_CONFLICT,    ERROR_S_HLSL_INTERFACE_CONFLICT },
    { HLSL_ERROR_REQUIRED_POSITION,     ERROR___HLSL_REQUIRED_POSITION },
    { HLSL_ERROR_ENTRY_ABI,             ERROR_S_HLSL_ENTRY_ABI },
    { HLSL_ERROR_REGISTER_COLLISION,    ERROR_S_HLSL_REGISTER_COLLISION },
    { HLSL_ERROR_RESOURCE_LIMIT,        ERROR_SII_HLSL_RESOURCE_LIMIT },
    { HLSL_ERROR_SAMPLER,               ERROR_S_HLSL_SAMPLER },
    { HLSL_ERROR_INTRINSIC,             ERROR_S_HLSL_INTRINSIC },
    { HLSL_ERROR_NAME_COLLISION,        ERROR_S_HLSL_NAME_COLLISION },
    { HLSL_ERROR_SYSTEM_SEMANTIC,       ERROR_S_HLSL_SYSTEM_SEMANTIC },
    { HLSL_ERROR_INTERPOLATION,         ERROR_S_HLSL_INTERPOLATION },
    { HLSL_ERROR_CBUFFER,               ERROR_S_HLSL_CBUFFER },
    { HLSL_ERROR_RESOURCE_PAIR,         ERROR_S_HLSL_RESOURCE_PAIR },
    { HLSL_ERROR_GEOMETRY_LAYOUT,       ERROR_S_HLSL_GEOMETRY_LAYOUT },
    { HLSL_ERROR_GEOMETRY_LIMIT,        ERROR_S_HLSL_GEOMETRY_LIMIT },
    { HLSL_ERROR_PROFILE_STAGE,         ERROR_SS_HLSL_PROFILE_STAGE },
    { HLSL_ERROR_MODEL_CAPABILITY,      ERROR_SS_HLSL_MODEL_CAPABILITY },
    { HLSL_ERROR_TEXTURE_STAGE,         ERROR_SS_HLSL_TEXTURE_STAGE },
    { HLSL_ERROR_TEXTURE_SIGNATURE,     ERROR_S_HLSL_TEXTURE_SIGNATURE },
    { HLSL_ERROR_GEOMETRY_MISSING_MAX,  ERROR___HLSL_GEOMETRY_MISSING_MAX },
    { HLSL_ERROR_GEOMETRY_MAX_LIMIT,    ERROR_SII_HLSL_GEOMETRY_MAX_LIMIT },
    { HLSL_ERROR_GEOMETRY_TOTAL_OUTPUT_LIMIT,
                                         ERROR_SII_HLSL_GEOMETRY_TOTAL_LIMIT },
    { HLSL_ERROR_INVALID_IR,            ERROR___HLSL_INVALID_IR }
};

typedef struct HlslHALData_Rec {
    const HlslProfileDesc *profile;
    Symbol *inputUsed[HLSL_MAX_INTERFACE_REGISTERS];
    Symbol *outputUsed[HLSL_MAX_INTERFACE_REGISTERS];
    HlslErrorKind errorKind;
    const char *errorReason;
    SourceLoc errorLoc;
} HlslHALData;

// Static HAL callbacks:
static int FreeHAL_hlsl(slHAL *fHAL);
static int RegisterNames_hlsl(slHAL *fHAL);
static int GetConnectorID_hlsl(int name);
static int GetConnectorAtom_hlsl(int name);
static int GetConnectorUses_hlsl(int cid, int pid);
static int GetConnectorRegister_hlsl(int cid, int ByIndex, int ratom, Binding *fBind);
static int GetCapsBit_hlsl(int bitNumber);
static int CheckInternalFunction_hlsl(Symbol *fSymb, int *group);
static int HandleParameterTypeError_hlsl(SourceLoc *loc,
                                         const Symbol *fSymb, int paramno);
static void HlslAppendSignatureText(char *target, size_t size,
                                    size_t *used, const char *text);
static void ReportHlslModelDiagnostic(SourceLoc *loc,
    const HlslProfileDesc *profile, HlslErrorKind kind, const char *reason);
static int BindUniformUnbound_hlsl(SourceLoc *loc, Symbol *fSymb, Binding *fBind);
static int BindUniformPragma_hlsl(SourceLoc *loc, Symbol *fSymb,
                                  Binding *lBind, const Binding *fBind);
static int BindVaryingSemantic_hlsl(SourceLoc *loc, Symbol *fSymb, int semantic,
                                    Binding *fBind, int IsOutVal);
static int BindVaryingUnbound_hlsl(SourceLoc *loc, Symbol *fSymb, int name,
                                   int semantic, Binding *fBind,
                                   int IsOutVal);
static int PrintCodeHeader_hlsl(FILE *out);
static int GenerateCode_hlsl(SourceLoc *loc, Scope *fScope, Symbol *program);
static int GenerateCodeIR_hlsl(SourceLoc *loc, Scope *fScope,
                               Symbol *program,
                               const CgIRModule *sourceIR);
static void *HlslCompilerAlloc(void *arg, size_t size);
static int ReportHlslFailure(const HlslModule *module,
                             const HlslProfileDesc *profile,
                             const Symbol *program);

static HlslHALData *GetHlslData(void)
{
    if (Cg == NULL || Cg->theHAL == NULL)
        return NULL;
    return (HlslHALData *) Cg->theHAL->localData;
} // GetHlslData

static const HlslProfileDesc *GetHlslProfile(void)
{
    HlslHALData *data;

    data = GetHlslData();
    return data != NULL ? data->profile : NULL;
} // GetHlslProfile

/*
 * InitHAL_hlsl_profile() - Shared HAL initialization.
 */

int InitHAL_hlsl_profile(slHAL *fHAL, const HlslProfileDesc *profile)
{
    HlslHALData *data;

    if (fHAL == NULL || !HlslProfileIsValid(profile))
        return 0;
    data = (HlslHALData *) malloc(sizeof(HlslHALData));
    if (data == NULL) {
        FatalError("malloc failed");
        return 0;
    }
    memset(data, 0, sizeof(HlslHALData));
    data->profile = profile;

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
    fHAL->HandleParameterTypeError = HandleParameterTypeError_hlsl;
    fHAL->BindUniformUnbound = BindUniformUnbound_hlsl;
    fHAL->BindUniformPragma = BindUniformPragma_hlsl;
    fHAL->BindVaryingSemantic = BindVaryingSemantic_hlsl;
    fHAL->BindVaryingUnbound = BindVaryingUnbound_hlsl;
    fHAL->PrintCodeHeader = PrintCodeHeader_hlsl;
    fHAL->GenerateCode = GenerateCode_hlsl;
    fHAL->GenerateCodeIR = GenerateCodeIR_hlsl;

    // Data members:
    fHAL->vendor = VENDOR_STRING_HLSL;
    fHAL->version = profile->version;
    fHAL->comment = "//";

    // Point to per-compilation state and expose the stage connectors:
    fHAL->localData = data;
    fHAL->incid = profile->inputCid;
    fHAL->inputCRegs = profile->inputRegs;
    fHAL->numInputCRegs = profile->numInputRegs;
    fHAL->outcid = profile->outputCid;
    fHAL->outputCRegs = profile->outputRegs;
    fHAL->numOutputCRegs = profile->numOutputRegs;

    return 1;
} // InitHAL_hlsl_profile

/*
 * FreeHAL_hlsl()
 */

static int FreeHAL_hlsl(slHAL *fHAL)
{
    free(fHAL->localData);
    fHAL->localData = NULL;
    return 1;
} // FreeHAL_hlsl

/*
 * RegisterNames_hlsl() - Register every connector, canonical semantic, and
 *         accepted source alias as an atom.
 */

static int RegisterNames_hlsl(slHAL *fHAL)
{
    HlslHALData *data;
    const HlslProfileDesc *profile;
    int i, j;

    data = (HlslHALData *) fHAL->localData;
    profile = data->profile;
    for (i = 0; i < profile->numConnectors; i++) {
        ConnectorDescriptor *conn = &profile->connectors[i];
        conn->name = AddAtom(atable, conn->sname);
        for (j = 0; j < conn->numregs; j++)
            conn->registers[j].name = AddAtom(atable, conn->registers[j].sname);
    }
    for (i = 0; i < profile->numInputSemantics; i++)
        AddAtom(atable, profile->inputSemantics[i].root);
    for (i = 0; i < profile->numOutputSemantics; i++)
        AddAtom(atable, profile->outputSemantics[i].root);
    for (i = 0; i < profile->numInputAliases; i++) {
        AddAtom(atable, profile->inputAliases[i].source);
        AddAtom(atable, profile->inputAliases[i].target);
    }
    for (i = 0; i < profile->numOutputAliases; i++) {
        AddAtom(atable, profile->outputAliases[i].source);
        AddAtom(atable, profile->outputAliases[i].target);
    }
    assert(fHAL->GetConnectorRegister(profile->inputCid, 1, -1, NULL) ==
           profile->numInputRegs);
    assert(fHAL->GetConnectorRegister(profile->outputCid, 1, -1, NULL) ==
           profile->numOutputRegs);
    return 1;
} // RegisterNames_hlsl

/*
 * GetConnectorID_hlsl()
 */

static int GetConnectorID_hlsl(int name)
{
    int i;
    const HlslProfileDesc *profile;

    profile = GetHlslProfile();
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

    profile = GetHlslProfile();
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
    profile = GetHlslProfile();
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

    profile = GetHlslProfile();
    conn = LookupConnectorHAL(profile->connectors, cid, profile->numConnectors);
    if (!conn)
        return 0;

    regs = conn->registers;
    if (!regs)
        return 0;

    if (ByIndex) {
        if (ratom < 0)
            return conn->numregs;
        i = ratom;
    } else {
        for (i = 0; i < conn->numregs; i++) {
            if (ratom == regs[i].name)
                break;
        }
    }
    if (fBind == NULL)
        return 0;
    if (i < 0 || i >= conn->numregs)
        return 0;
    SetSymbolConnectorBindingHAL(fBind, &regs[i]);
    return 1;
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
    case CAPS_CANONICAL_OUTPUT_SEMANTIC_CONFLICTS:
    case CAPS_ENTRY_INOUT_PARAMETERS:
    case CAPS_PRESERVE_TERMINAL_ENTRY_RETURN:
    case CAPS_DEFER_RECURSION_DIAGNOSTICS:
    case CAPS_CONDITIONAL_SIDE_EFFECTS:
    case CAPS_TYPED_INC_DEC_EXPRESSIONS:
    case CAPS_PRESERVE_COMMA_EXPRESSIONS:
    case CAPS_PRESERVE_INLINE_HELPERS:
    case CAPS_AGGREGATE_DEFAULT_INITIALIZERS:
    case CAPS_PRESERVE_SIDE_EFFECTING_AGGREGATE_TEMPS:
        return 1;
    case CAPS_HLSL_GEOMETRY_ENTRY_ABI:
        return GetHlslProfile()->stage == HLSL_STAGE_GEOMETRY;
    default:
        return 0;
    }
} // GetCapsBit_hlsl

/*
 * CheckInternalFunction_hlsl() - Check for internally implemented function.
 */

static HlslSourceBase HlslSourceBaseForType(const Type *source)
{
    CgScalarKind kind;
    int sourceBase;

    if (source == NULL)
        return HLSL_SOURCE_BASE_NONE;
    if (GetCategory(source) == TYPE_CATEGORY_SAMPLER) {
        switch (source->samp.samplerKind) {
        case CG_SAMPLER_1D: return HLSL_SOURCE_BASE_SAMPLER1D;
        case CG_SAMPLER_2D: return HLSL_SOURCE_BASE_SAMPLER2D;
        case CG_SAMPLER_3D: return HLSL_SOURCE_BASE_SAMPLER3D;
        case CG_SAMPLER_CUBE: return HLSL_SOURCE_BASE_SAMPLERCUBE;
        default: return HLSL_SOURCE_BASE_NONE;
        }
    }
    kind = source->co.scalarKind;
    if (kind == CG_SCALAR_NONE &&
        GetCategory(source) == TYPE_CATEGORY_ARRAY &&
        source->arr.eltype != NULL)
    {
        kind = source->arr.eltype->co.scalarKind;
        if (kind == CG_SCALAR_NONE &&
            GetCategory(source->arr.eltype) == TYPE_CATEGORY_ARRAY &&
            source->arr.eltype->arr.eltype != NULL)
        {
            kind = source->arr.eltype->arr.eltype->co.scalarKind;
        }
    }
    switch (kind) {
    case CG_SCALAR_CFLOAT: return HLSL_SOURCE_BASE_CFLOAT;
    case CG_SCALAR_CINT: return HLSL_SOURCE_BASE_CINT;
    case CG_SCALAR_BOOL: return HLSL_SOURCE_BASE_BOOL;
    case CG_SCALAR_INT: return HLSL_SOURCE_BASE_INT;
    case CG_SCALAR_FIXED: return HLSL_SOURCE_BASE_FIXED;
    case CG_SCALAR_HALF: return HLSL_SOURCE_BASE_HALF;
    case CG_SCALAR_FLOAT: return HLSL_SOURCE_BASE_FLOAT;
    default:
        if (kind != CG_SCALAR_NONE)
            return HLSL_SOURCE_BASE_NONE;
        break;
    }
    sourceBase = GetBase(source);
    switch (sourceBase) {
    case TYPE_BASE_CFLOAT: return HLSL_SOURCE_BASE_CFLOAT;
    case TYPE_BASE_CINT: return HLSL_SOURCE_BASE_CINT;
    case TYPE_BASE_BOOLEAN: return HLSL_SOURCE_BASE_BOOL;
    case TYPE_BASE_INT: return HLSL_SOURCE_BASE_INT;
    case TYPE_BASE_FLOAT: return HLSL_SOURCE_BASE_FLOAT;
    default: return HLSL_SOURCE_BASE_NONE;
    }
} // HlslSourceBaseForType

int HlslDescribeSourceType(const Type *source, HlslSourceType *target)
{
    HlslSourceBase base;
    int rows;
    int cols;
    int len;

    if (source == NULL || target == NULL)
        return 0;
    if (IsVoid(source)) {
        memset(target, 0, sizeof(*target));
        target->shape = HLSL_SOURCE_SHAPE_VOID;
        return 1;
    }
    base = HlslSourceBaseForType(source);
    if (base == HLSL_SOURCE_BASE_NONE)
        return 0;
    if (base >= HLSL_SOURCE_BASE_SAMPLER1D &&
        base <= HLSL_SOURCE_BASE_SAMPLERCUBE)
    {
        *target = HlslSourceScalarType(base);
        return 1;
    }
    if (IsMatrix(source, &cols, &rows)) {
        if (rows < 1 || rows > 4 || cols < 1 || cols > 4)
        {
            return 0;
        }
        *target = HlslSourceMatrixType(base, rows, cols);
        return 1;
    }
    if (IsScalar(source)) {
        *target = HlslSourceScalarType(base);
        return 1;
    }
    if (IsVector(source, &len) && len >= 1 && len <= 4) {
        *target = HlslSourceVectorType(base, len);
        return 1;
    }
    return 0;
} // HlslDescribeSourceType

static void HlslAppendSignatureText(char *target, size_t size,
                                    size_t *used, const char *text)
{
    size_t available;
    size_t length;

    if (target == NULL || used == NULL || text == NULL ||
        *used >= size || size == 0)
    {
        return;
    }
    available = size - *used - 1;
    length = strlen(text);
    if (length > available)
        length = available;
    memcpy(target + *used, text, length);
    *used += length;
    target[*used] = '\0';
} // HlslAppendSignatureText

static const char *HlslShaderModelDiagnosticName(
    const HlslProfileDesc *profile)
{
    if (profile == NULL)
        return "unknown";
    switch (profile->model) {
    case HLSL_SHADER_MODEL_3:
        return "3";
    case HLSL_SHADER_MODEL_4:
        return "4";
    case HLSL_SHADER_MODEL_5:
        return "5";
    default:
        return "unknown";
    }
} // HlslShaderModelDiagnosticName

static void ReportHlslModelDiagnostic(SourceLoc *loc,
    const HlslProfileDesc *profile, HlslErrorKind kind, const char *reason)
{
    const char *model;

    model = HlslShaderModelDiagnosticName(profile);
    switch (kind) {
    case HLSL_ERROR_UNSUPPORTED_TYPE:
        SemanticError(loc, ERROR_SS_HLSL_UNSUPPORTED_TYPE_MODEL,
                      model, reason);
        break;
    case HLSL_ERROR_UNSUPPORTED_OPERATION:
        SemanticError(loc, ERROR_SS_HLSL_UNSUPPORTED_OPERATION_MODEL,
                      model, reason);
        break;
    case HLSL_ERROR_SAMPLER:
        SemanticError(loc, ERROR_SS_HLSL_SAMPLER_MODEL, model, reason);
        break;
    case HLSL_ERROR_INTRINSIC:
        SemanticError(loc, ERROR_SS_HLSL_INTRINSIC_MODEL, model, reason);
        break;
    default:
        InternalError(loc, ERROR___HLSL_INVALID_IR);
        break;
    }
} // ReportHlslModelDiagnostic

static void ReportHlslTextureDiagnostic(SourceLoc *loc,
    const HlslProfileDesc *profile, HlslErrorKind kind, const char *reason)
{
    if (profile != NULL && profile->syntax == HLSL_SYNTAX_MODERN) {
        if (kind == HLSL_ERROR_TEXTURE_STAGE)
            SemanticError(loc, ERROR_SS_HLSL_TEXTURE_STAGE,
                          profile->name, reason);
        else
            SemanticError(loc, ERROR_S_HLSL_TEXTURE_SIGNATURE, reason);
        return;
    }
    ReportHlslModelDiagnostic(loc, profile, HLSL_ERROR_SAMPLER, reason);
} // ReportHlslTextureDiagnostic

static int CheckInternalFunction_hlsl(Symbol *fSymb, int *group)
{
    const HlslProfileDesc *profile;
    HlslSourceType result;
    HlslSourceType params[HLSL_MAX_BUILTIN_ARGS];
    TypeList *param;
    HlslBuiltin builtin;
    const char *name;
    char signature[256];
    size_t used;
    int count;

    if (fSymb == NULL || fSymb->kind != FUNCTION_S ||
        fSymb->type == NULL || group == NULL ||
        GetCategory(fSymb->type) != TYPE_CATEGORY_FUNCTION)
    {
        return 0;
    }
    name = GetAtomString(atable, fSymb->name);
    if (!HlslIsBuiltinName(name))
        return 0;
    profile = GetHlslProfile();
    if (profile == NULL)
        return 0;

    count = 0;
    if (!HlslDescribeSourceType(fSymb->type->fun.rettype, &result))
        count = -1;
    for (param = fSymb->type->fun.paramtypes;
         count >= 0 && param != NULL; param = param->next)
    {
        if (count >= (int) NUMELS(params) ||
            !HlslDescribeSourceType(param->type, &params[count]))
        {
            count = -1;
            break;
        }
        count++;
    }
    builtin = count >= 0 ?
        HlslLookupSourceBuiltin(profile->stage, name,
                                &result, params, count) :
        HLSL_BUILTIN_NONE;
    if (!HlslProfileAllowsBuiltin(profile, builtin))
        builtin = HLSL_BUILTIN_NONE;
    if (builtin != HLSL_BUILTIN_NONE) {
        *group = HLSL_BUILTIN_GROUP;
        return (int) builtin;
    }
    if (count >= 0) {
        HlslBuiltin otherStage;

        otherStage = HlslLookupSourceBuiltin(
            profile->stage == HLSL_STAGE_PIXEL ?
            HLSL_STAGE_VERTEX : HLSL_STAGE_PIXEL,
            name, &result, params, count);
        if (otherStage != HLSL_BUILTIN_NONE) {
            if (HlslBuiltinIsTexture(otherStage) ||
                HlslIsTextureName(name))
                ReportHlslTextureDiagnostic(&fSymb->loc, profile,
                                            HLSL_ERROR_TEXTURE_STAGE,
                                            name);
            else
                SemanticError(&fSymb->loc, ERROR_SS_HLSL_STAGE_OPERATION,
                              profile->name, name);
            return 0;
        }
    }
    if (HlslIsTextureName(name)) {
        ReportHlslTextureDiagnostic(&fSymb->loc, profile,
                                    HLSL_ERROR_TEXTURE_SIGNATURE, name);
        return 0;
    }

    signature[0] = '\0';
    used = 0;
    HlslAppendSignatureText(signature, sizeof(signature), &used, name);
    HlslAppendSignatureText(signature, sizeof(signature), &used, "(");
    if (count >= 0) {
        int i;

        for (i = 0; i < count; i++) {
            if (i != 0)
                HlslAppendSignatureText(signature, sizeof(signature),
                                        &used, ", ");
            HlslAppendSignatureText(signature, sizeof(signature), &used,
                                    HlslSourceTypeName(&params[i]));
        }
    } else {
        HlslAppendSignatureText(signature, sizeof(signature), &used,
                                "unsupported");
    }
    HlslAppendSignatureText(signature, sizeof(signature), &used, ")");
    ReportHlslModelDiagnostic(&fSymb->loc, profile,
                              HLSL_ERROR_INTRINSIC, signature);
    return 0;
} // CheckInternalFunction_hlsl

static int HandleParameterTypeError_hlsl(SourceLoc *loc,
                                         const Symbol *fSymb, int paramno)
{
    const CgIntrinsicSignature *signature;
    const char *name;

    (void) paramno;
    signature = fSymb != NULL ? fSymb->details.fun.intrinsic : NULL;
    if (signature != NULL &&
        (signature->flags & CG_INTRINSIC_TEXTURE) != 0 &&
        HlslIsTextureName(signature->name))
    {
        name = signature->name;
    } else {
        HlslBuiltin builtin;
        const char *spelling;

        if (fSymb == NULL || fSymb->kind != FUNCTION_S ||
            fSymb->type == NULL ||
            GetCategory(fSymb->type) != TYPE_CATEGORY_FUNCTION ||
            (fSymb->type->properties & TYPE_MISC_INTERNAL) == 0 ||
            (fSymb->properties & (SYMB_IS_BUILTIN | SYMB_IS_DEFINED)) !=
                (SYMB_IS_BUILTIN | SYMB_IS_DEFINED) ||
            fSymb->details.fun.group != HLSL_BUILTIN_GROUP ||
            fSymb->details.fun.index <= HLSL_BUILTIN_NONE ||
            fSymb->details.fun.index >= HLSL_BUILTIN_COUNT)
        {
            return 0;
        }
        builtin = (HlslBuiltin) fSymb->details.fun.index;
        spelling = HlslBuiltinSpelling(builtin);
        name = GetAtomString(atable, fSymb->name);
        if (!HlslBuiltinIsTexture(builtin) || spelling == NULL ||
            name == NULL || strcmp(name, spelling) != 0)
        {
            return 0;
        }
    }
    ReportHlslTextureDiagnostic(loc, GetHlslProfile(),
                                HLSL_ERROR_TEXTURE_SIGNATURE, name);
    return 1;
} // HandleParameterTypeError_hlsl

/*
 * BindUniformUnbound_hlsl() - Mark uniforms as bound.
 */

static int BindUniformUnbound_hlsl(SourceLoc *loc, Symbol *fSymb,
                                   Binding *fBind)
{
    (void) loc;
    (void) fSymb;
    fBind->none.properties |= BIND_IS_BOUND | BIND_UNIFORM;
    return 1;
} // BindUniformUnbound_hlsl

static int HlslAddPragmaDefault(Symbol *symbol, const Binding *source)
{
    BindingList **tail;
    BindingList *item;
    Binding *record;

    if (Cg == NULL || Cg->theHAL == NULL || symbol == NULL || source == NULL)
        return 0;
    record = (Binding *) malloc(sizeof(Binding));
    item = (BindingList *) malloc(sizeof(BindingList));
    if (record == NULL || item == NULL) {
        free(record);
        free(item);
        return 0;
    }
    *record = *source;
    record->constdef.kind = BK_DEFAULT;
    record->none.properties = BIND_IS_BOUND | BIND_INPUT | BIND_UNIFORM;
    item->next = NULL;
    item->binding = record;
    item->identity = symbol;
    item->initializer = NULL;
    item->type = symbol->type;
    tail = &Cg->theHAL->defaultBindings;
    while (*tail != NULL)
        tail = &(*tail)->next;
    *tail = item;
    return 1;
} // HlslAddPragmaDefault

static char HlslPragmaTypeBank(const Type *type);

static char HlslPragmaStructBank(const Scope *members)
{
    const Symbol *member;
    char bank = 0;

    if (members == NULL)
        return 0;
    for (member = members->params; member != NULL; member = member->next) {
        char memberBank = HlslPragmaTypeBank(member->type);

        if (memberBank == 0 || (bank != 0 && bank != memberBank))
            return 0;
        bank = memberBank;
    }
    return bank;
} // HlslPragmaStructBank

static char HlslPragmaTypeBank(const Type *type)
{
    CgScalarKind kind;

    if (type == NULL)
        return 0;
    if (GetCategory(type) == TYPE_CATEGORY_SAMPLER)
        return 's';
    if (GetCategory(type) == TYPE_CATEGORY_STRUCT)
        return HlslPragmaStructBank(type->str.members);
    kind = type->co.scalarKind;
    if (kind == CG_SCALAR_NONE && GetCategory(type) == TYPE_CATEGORY_ARRAY)
        return HlslPragmaTypeBank(type->arr.eltype);
    switch (kind) {
    case CG_SCALAR_CFLOAT:
    case CG_SCALAR_FIXED:
    case CG_SCALAR_HALF:
    case CG_SCALAR_FLOAT:
    case CG_SCALAR_DOUBLE:
        return 'c';
    case CG_SCALAR_CINT:
    case CG_SCALAR_CHAR:
    case CG_SCALAR_UCHAR:
    case CG_SCALAR_SHORT:
    case CG_SCALAR_USHORT:
    case CG_SCALAR_INT:
    case CG_SCALAR_UINT:
    case CG_SCALAR_LONG:
    case CG_SCALAR_ULONG:
        return 'i';
    case CG_SCALAR_BOOL:
        return 'b';
    case CG_SCALAR_NONE:
        break;
    }
    return 0;
} // HlslPragmaTypeBank

/*
 * BindUniformPragma_hlsl() - Preserve explicit DirectX register, texture
 *         unit, and numeric default pragmas for the HLSL lowering pass.
 */

static int BindUniformPragma_hlsl(SourceLoc *loc, Symbol *fSymb,
                                  Binding *lBind, const Binding *fBind)
{
    const char *rname;
    char requestedBank;
    char typeBank;
    int count;
    int len;

    (void) loc;
    if (fSymb == NULL || lBind == NULL || fBind == NULL)
        return 0;
    switch (fBind->none.kind) {
    case BK_REGARRAY:
        rname = GetAtomString(atable, fBind->reg.rname);
        if (rname == NULL || fBind->reg.regno < 0 ||
            (strcmp(rname, "c") != 0 && strcmp(rname, "C") != 0 &&
             strcmp(rname, "i") != 0 && strcmp(rname, "I") != 0 &&
             strcmp(rname, "b") != 0 && strcmp(rname, "B") != 0 &&
             strcmp(rname, "s") != 0 && strcmp(rname, "S") != 0))
        {
            return 0;
        }
        requestedBank = rname[0];
        if (requestedBank >= 'A' && requestedBank <= 'Z')
            requestedBank += 'a' - 'A';
        typeBank = HlslPragmaTypeBank(fSymb->type);
        if (typeBank == 0 || typeBank != requestedBank)
            return 0;
        *lBind = *fBind;
        lBind->none.properties |= BIND_IS_BOUND | BIND_INPUT | BIND_UNIFORM;
        return 1;
    case BK_TEXUNIT:
        if (GetCategory(fSymb->type) != TYPE_CATEGORY_SAMPLER ||
            fBind->texunit.unitno < 0)
        {
            return 0;
        }
        *lBind = *fBind;
        lBind->none.properties |= BIND_IS_BOUND | BIND_INPUT | BIND_UNIFORM;
        return 1;
    case BK_DEFAULT:
        if (IsScalar(fSymb->type)) {
            count = 1;
        } else if (IsVector(fSymb->type, &len)) {
            count = len;
        } else {
            return 0;
        }
        if (count < 1 || count > 4 || fBind->constdef.size != count)
            return 0;
        *lBind = *fBind;
        lBind->none.properties |= BIND_IS_BOUND | BIND_INPUT | BIND_UNIFORM;
        return HlslAddPragmaDefault(fSymb, fBind);
    default:
        return 0;
    }
} // BindUniformPragma_hlsl

static void RecordHlslHALError(HlslHALData *data, SourceLoc *loc,
                               HlslErrorKind kind, const char *reason)
{
    if (data == NULL || data->errorKind != HLSL_ERROR_NONE)
        return;
    data->errorKind = kind;
    data->errorReason = reason;
    if (loc != NULL)
        data->errorLoc = *loc;
} // RecordHlslHALError

static int ReportHlslInterfaceError(SourceLoc *loc, HlslErrorKind kind,
                                    const char *reason)
{
    HlslHALData *data;

    data = GetHlslData();
    RecordHlslHALError(data, loc, kind, reason);
    if (kind == HLSL_ERROR_INTERFACE_CONFLICT)
        SemanticError(loc, ERROR_S_HLSL_INTERFACE_CONFLICT, reason);
    else if (kind == HLSL_ERROR_SYSTEM_SEMANTIC)
        SemanticError(loc, ERROR_S_HLSL_SYSTEM_SEMANTIC, reason);
    else if (kind == HLSL_ERROR_INTERPOLATION)
        SemanticError(loc, ERROR_S_HLSL_INTERPOLATION, reason);
    else
        SemanticError(loc, ERROR_S_HLSL_SEMANTIC, reason);
    return 0;
} // ReportHlslInterfaceError

static const HlslSemanticDesc *FindHlslSemanticDesc(
    const HlslProfileDesc *profile, const char *canonical, int IsOutVal)
{
    const HlslSemanticDesc *semantics;
    char root[HLSL_SEMANTIC_NAME_MAX];
    int count, index, i;

    if (!HlslParseSemantic(canonical, root, sizeof(root), &index))
        return NULL;
    if (IsOutVal) {
        semantics = profile->outputSemantics;
        count = profile->numOutputSemantics;
    } else {
        semantics = profile->inputSemantics;
        count = profile->numInputSemantics;
    }
    for (i = 0; i < count; i++) {
        if (!strcmp(root, semantics[i].root) &&
            index >= semantics[i].firstIndex &&
            index < semantics[i].firstIndex + semantics[i].count)
        {
            return &semantics[i];
        }
    }
    return NULL;
} // FindHlslSemanticDesc

static ConnectorRegisters *FindHlslSemanticRegister(
    const HlslProfileDesc *profile, const char *canonical, int IsOutVal,
    int *slot)
{
    ConnectorRegisters *registers;
    int count, i;

    if (IsOutVal) {
        registers = profile->outputRegs;
        count = profile->numOutputRegs;
    } else {
        registers = profile->inputRegs;
        count = profile->numInputRegs;
    }
    for (i = 0; i < count; i++) {
        if (!strcmp(registers[i].sname, canonical)) {
            *slot = i;
            return &registers[i];
        }
    }
    return NULL;
} // FindHlslSemanticRegister

static int HlslSemanticTypeIsValid(const HlslSemanticDesc *semantic,
                                   const Type *type)
{
    int len, isScalar;

    isScalar = IsScalar(type);
    if (isScalar)
        len = 1;
    else if (!IsVector(type, &len))
        return 0;
    if (GetBase(type) != TYPE_BASE_FLOAT)
        return 0;

    switch (semantic->interfaceKind) {
    case HLSL_INTERFACE_POSITION:
    case HLSL_INTERFACE_PIXEL_POSITION:
    case HLSL_INTERFACE_COLOR:
        return !isScalar && len == semantic->width;
    case HLSL_INTERFACE_POINT_SIZE:
    case HLSL_INTERFACE_FACE:
    case HLSL_INTERFACE_DEPTH:
        return isScalar && semantic->width == 1;
    case HLSL_INTERFACE_VARYING:
    default:
        return len <= semantic->width;
    }
} // HlslSemanticTypeIsValid

static int ClaimHlslSemantic(HlslHALData *data, Symbol *owner, int slot,
                             int IsOutVal)
{
    Symbol **used;
    int i;

    for (i = 0; i < HLSL_MAX_INTERFACE_REGISTERS; i++) {
        if (data->inputUsed[i] == owner)
            return !IsOutVal && i == slot;
        if (data->outputUsed[i] == owner)
            return IsOutVal && i == slot;
    }
    used = IsOutVal ? data->outputUsed : data->inputUsed;
    if (used[slot] != NULL)
        return 0;
    used[slot] = owner;
    return 1;
} // ClaimHlslSemantic

static int HlslModernSemanticSlot(HlslSemanticKind kind, int index)
{
    switch (kind) {
    case HLSL_SEMANTIC_SV_POSITION: return 0;
    case HLSL_SEMANTIC_SV_TARGET: return index >= 0 && index < 8 ? 1 + index : -1;
    case HLSL_SEMANTIC_SV_DEPTH: return 9;
    case HLSL_SEMANTIC_SV_VERTEX_ID: return 10;
    case HLSL_SEMANTIC_SV_INSTANCE_ID: return 11;
    case HLSL_SEMANTIC_SV_PRIMITIVE_ID: return 12;
    case HLSL_SEMANTIC_SV_RT_ARRAY_INDEX: return 13;
    case HLSL_SEMANTIC_SV_IS_FRONT_FACE: return 14;
    case HLSL_SEMANTIC_SV_CLIP_DISTANCE:
        return index >= 0 && index < 8 ? 15 + index : -1;
    default:
        return -1;
    }
} // HlslModernSemanticSlot

static const char *HlslModernSemanticReason(HlslSemanticKind kind)
{
    switch (kind) {
    case HLSL_SEMANTIC_SV_POSITION: return "SV_Position";
    case HLSL_SEMANTIC_SV_TARGET: return "SV_Target";
    case HLSL_SEMANTIC_SV_DEPTH: return "SV_Depth";
    case HLSL_SEMANTIC_SV_VERTEX_ID: return "SV_VertexID";
    case HLSL_SEMANTIC_SV_INSTANCE_ID: return "SV_InstanceID";
    case HLSL_SEMANTIC_SV_PRIMITIVE_ID: return "SV_PrimitiveID";
    case HLSL_SEMANTIC_SV_RT_ARRAY_INDEX:
        return "SV_RenderTargetArrayIndex";
    case HLSL_SEMANTIC_SV_IS_FRONT_FACE: return "SV_IsFrontFace";
    case HLSL_SEMANTIC_SV_CLIP_DISTANCE: return "SV_ClipDistance";
    default: return "modern semantic";
    }
} // HlslModernSemanticReason

static int BindModernVaryingSemantic(HlslHALData *data, SourceLoc *loc,
                                     Symbol *symbol, int semantic,
                                     Binding *binding, int isOutput)
{
    char upper[HLSL_SEMANTIC_NAME_MAX];
    char root[HLSL_SEMANTIC_NAME_MAX];
    const char *source;
    HlslSemanticKind kind;
    int index;
    int slot;

    source = GetAtomString(atable, semantic);
    if (data == NULL || symbol == NULL || binding == NULL || source == NULL ||
        !HlslUpperSemantic(source, upper, sizeof(upper)) ||
        !HlslParseSemantic(upper, root, sizeof(root), &index))
    {
        return ReportHlslInterfaceError(loc, HLSL_ERROR_SEMANTIC,
                                        source != NULL ? source :
                                                         "unknown semantic");
    }
    kind = HlslModernSemantic(data->profile->stage,
        isOutput ? HLSL_DIRECTION_OUTPUT : HLSL_DIRECTION_INPUT,
        root, index);
    if (kind == HLSL_SEMANTIC_UNSUPPORTED)
        return ReportHlslInterfaceError(loc,
                                        HLSL_ERROR_SYSTEM_SEMANTIC,
                                        source);
    slot = HlslModernSemanticSlot(kind, index);
    if (slot >= 0 && !ClaimHlslSemantic(data, symbol, slot, isOutput)) {
        return ReportHlslInterfaceError(loc, HLSL_ERROR_INTERFACE_CONFLICT,
                                        HlslModernSemanticReason(kind));
    }
    binding->conn.kind = BK_CONNECTOR;
    binding->conn.rname = semantic;
    binding->conn.regno = slot >= 0 ? slot : 0;
    binding->conn.size = 4;
    binding->conn.properties |= BIND_IS_BOUND | BIND_VARYING |
        (isOutput ? BIND_OUTPUT : BIND_INPUT);
    symbol->properties |= SYMB_IS_CONNECTOR_REGISTER |
                          SYMB_CONNECTOR_CAN_READ |
                          SYMB_CONNECTOR_CAN_WRITE;
    return 1;
} // BindModernVaryingSemantic

/*
 * BindVaryingSemantic_hlsl() - Canonicalize and claim one stage interface
 *         location transactionally.  Every failure is reported here so the
 *         common semantic layer's error-count guards suppress fallbacks.
 */

static int BindVaryingSemantic_hlsl(SourceLoc *loc, Symbol *fSymb,
                                    int semantic, Binding *fBind,
                                    int IsOutVal)
{
    HlslHALData *data;
    const HlslProfileDesc *profile;
    const HlslSemanticDesc *descriptor;
    ConnectorRegisters *reg;
    const char *source, *canonical;
    int slot;

    data = GetHlslData();
    profile = data != NULL ? data->profile : NULL;
    if (profile != NULL &&
        profile->semanticPolicy == HLSL_SEMANTIC_POLICY_MODERN)
    {
        return BindModernVaryingSemantic(data, loc, fSymb, semantic,
                                         fBind, IsOutVal);
    }
    source = GetAtomString(atable, semantic);
    if (source == NULL)
        source = "unknown semantic";
    canonical = HlslCanonicalSemantic(profile, source, IsOutVal);
    if (canonical == NULL) {
        /* A varying-domain global has no declared direction.  The common
         * front end probes input first, then output; keep only that known
         * opposite-direction probe silent. */
        if (fBind != NULL && fBind->none.gname == 0 &&
            HlslCanonicalSemantic(profile, source, !IsOutVal) != NULL)
        {
            return 0;
        }
        return ReportHlslInterfaceError(loc, HLSL_ERROR_SEMANTIC, source);
    }
    descriptor = FindHlslSemanticDesc(profile, canonical, IsOutVal);
    reg = FindHlslSemanticRegister(profile, canonical, IsOutVal, &slot);
    if (fSymb == NULL || fBind == NULL || descriptor == NULL || reg == NULL ||
        slot < 0 || slot >= HLSL_MAX_INTERFACE_REGISTERS ||
        !HlslSemanticTypeIsValid(descriptor, fSymb->type))
    {
        return ReportHlslInterfaceError(loc, HLSL_ERROR_SEMANTIC, source);
    }

    if (!ClaimHlslSemantic(data, fSymb, slot, IsOutVal)) {
        return ReportHlslInterfaceError(loc,
                                        HLSL_ERROR_INTERFACE_CONFLICT,
                                        canonical);
    }
    fBind->conn.kind = BK_CONNECTOR;
    fBind->conn.rname = reg->name;
    fBind->conn.regno = reg->regno;
    fBind->conn.base = reg->base;
    fBind->conn.size = descriptor->width;
    fBind->conn.properties |= BIND_IS_BOUND;
    if (descriptor->properties & SEM_VARYING)
        fBind->conn.properties |= BIND_VARYING;
    if (descriptor->properties & SEM_IN)
        fBind->conn.properties |= BIND_INPUT;
    if (descriptor->properties & SEM_OUT)
        fBind->conn.properties |= BIND_OUTPUT;
    if (descriptor->properties & SEM_HIDDEN)
        fBind->conn.properties |= BIND_HIDDEN;
    if (descriptor->properties & SEM_REQUIRED)
        fBind->conn.properties |= BIND_WRITE_REQUIRED;
    fSymb->properties |= SYMB_IS_CONNECTOR_REGISTER |
                         SYMB_CONNECTOR_CAN_READ |
                         SYMB_CONNECTOR_CAN_WRITE;
    return 1;
} // BindVaryingSemantic_hlsl

/*
 * BindVaryingUnbound_hlsl() - HLSL public interface members always require
 *         an explicit semantic.
 */

static int BindVaryingUnbound_hlsl(SourceLoc *loc, Symbol *fSymb, int name,
                                   int semantic, Binding *fBind,
                                   int IsOutVal)
{
    const char *source;

    (void) semantic;
    (void) fBind;
    (void) IsOutVal;
    source = fSymb != NULL ? GetAtomString(atable, fSymb->name) : NULL;
    if (source == NULL)
        source = GetAtomString(atable, name);
    if (source == NULL)
        source = "anonymous interface member";
    return ReportHlslInterfaceError(loc, HLSL_ERROR_SEMANTIC, source);
} // BindVaryingUnbound_hlsl

/*
 * PrintCodeHeader_hlsl() - Write nothing; header is part of codegen.
 */

static int PrintCodeHeader_hlsl(FILE *out)
{
    (void) out;
    return 1;
} // PrintCodeHeader_hlsl

static void *HlslCompilerAlloc(void *arg, size_t size)
{
    return mem_Calloc((MemoryPool *) arg, size, 1);
} // HlslCompilerAlloc

/*
 * ReportHlslFailure() - Emit one diagnostic for a failed HLSL phase.
 */

static int ReportHlslFailure(const HlslModule *module,
                             const HlslProfileDesc *profile,
                             const Symbol *program)
{
    const HlslDiagnosticMap *mapping;
    SourceLoc failureLoc;
    SourceLoc relatedLoc;
    const char *reason;
    int i;

    if (program != NULL)
        failureLoc = program->loc;
    else {
        failureLoc.file = 0;
        failureLoc.line = 0;
    }
    if (module->errorLoc.file != 0 || module->errorLoc.line != 0) {
        failureLoc.file = (unsigned short) module->errorLoc.file;
        failureLoc.line = (unsigned short) module->errorLoc.line;
    }
    reason = module->errorReason != NULL ?
             module->errorReason : "HLSL profile program";
    mapping = NULL;
    for (i = 0; i < (int) (sizeof(hlslDiagnosticMap) /
                           sizeof(hlslDiagnosticMap[0])); i++) {
        if (hlslDiagnosticMap[i].kind == module->errorKind) {
            mapping = &hlslDiagnosticMap[i];
            break;
        }
    }
    if (mapping == NULL || mapping->code != HlslErrorCode(module->errorKind)) {
        InternalError(&failureLoc, ERROR___HLSL_INVALID_IR);
        return 0;
    }
    if (module->errorKind == HLSL_ERROR_RESOURCE_LIMIT) {
        SemanticError(&failureLoc, ERROR_SII_HLSL_RESOURCE_LIMIT,
                      module->resourceName != NULL ?
                          module->resourceName : "resource",
                      module->resourceUsed,
                      module->resourceAvailable);
        return 0;
    }
    switch (module->errorKind) {
    case HLSL_ERROR_UNSUPPORTED_TYPE:
        ReportHlslModelDiagnostic(&failureLoc, profile,
                                  HLSL_ERROR_UNSUPPORTED_TYPE, reason);
        break;
    case HLSL_ERROR_STAGE_OPERATION:
        SemanticError(&failureLoc, ERROR_SS_HLSL_STAGE_OPERATION,
                      profile->name, reason);
        break;
    case HLSL_ERROR_SEMANTIC:
        SemanticError(&failureLoc, ERROR_S_HLSL_SEMANTIC, reason);
        break;
    case HLSL_ERROR_INTERFACE_CONFLICT:
        SemanticError(&failureLoc, ERROR_S_HLSL_INTERFACE_CONFLICT, reason);
        if (module->relatedErrorLoc.file != 0 ||
            module->relatedErrorLoc.line != 0)
        {
            relatedLoc.file = (unsigned short) module->relatedErrorLoc.file;
            relatedLoc.line = (unsigned short) module->relatedErrorLoc.line;
            SemanticNote(&relatedLoc, NOTICE_S_HLSL_INTERFACE_FIRST,
                         reason);
        }
        break;
    case HLSL_ERROR_REQUIRED_POSITION:
        SemanticError(&failureLoc, ERROR___HLSL_REQUIRED_POSITION);
        break;
    case HLSL_ERROR_ENTRY_ABI:
        SemanticError(&failureLoc, ERROR_S_HLSL_ENTRY_ABI, reason);
        break;
    case HLSL_ERROR_REGISTER_COLLISION:
        SemanticError(&failureLoc, ERROR_S_HLSL_REGISTER_COLLISION, reason);
        break;
    case HLSL_ERROR_SAMPLER:
        ReportHlslModelDiagnostic(&failureLoc, profile,
                                  HLSL_ERROR_SAMPLER, reason);
        break;
    case HLSL_ERROR_INTRINSIC:
        ReportHlslModelDiagnostic(&failureLoc, profile,
                                  HLSL_ERROR_INTRINSIC, reason);
        break;
    case HLSL_ERROR_NAME_COLLISION:
        SemanticError(&failureLoc, ERROR_S_HLSL_NAME_COLLISION, reason);
        break;
    case HLSL_ERROR_SYSTEM_SEMANTIC:
        SemanticError(&failureLoc, ERROR_S_HLSL_SYSTEM_SEMANTIC, reason);
        break;
    case HLSL_ERROR_INTERPOLATION:
        SemanticError(&failureLoc, ERROR_S_HLSL_INTERPOLATION, reason);
        break;
    case HLSL_ERROR_CBUFFER:
        if (module->resourceName != NULL)
            SemanticError(&failureLoc, ERROR_SII_HLSL_CBUFFER_LIMIT,
                          module->resourceName, module->resourceUsed,
                          module->resourceAvailable);
        else
            SemanticError(&failureLoc, ERROR_S_HLSL_CBUFFER, reason);
        break;
    case HLSL_ERROR_RESOURCE_PAIR:
        if (module->resourceName != NULL)
            SemanticError(&failureLoc, ERROR_SII_HLSL_RESOURCE_PAIR_LIMIT,
                          module->resourceName, module->resourceUsed,
                          module->resourceAvailable);
        else
            SemanticError(&failureLoc, ERROR_S_HLSL_RESOURCE_PAIR, reason);
        break;
    case HLSL_ERROR_GEOMETRY_LAYOUT:
        SemanticError(&failureLoc, ERROR_S_HLSL_GEOMETRY_LAYOUT, reason);
        break;
    case HLSL_ERROR_GEOMETRY_LIMIT:
        SemanticError(&failureLoc, ERROR_S_HLSL_GEOMETRY_LIMIT, reason);
        break;
    case HLSL_ERROR_PROFILE_STAGE:
        SemanticError(&failureLoc, ERROR_SS_HLSL_PROFILE_STAGE,
                      profile->name, reason);
        break;
    case HLSL_ERROR_MODEL_CAPABILITY:
        SemanticError(&failureLoc, ERROR_SS_HLSL_MODEL_CAPABILITY,
                      HlslShaderModelDiagnosticName(profile), reason);
        break;
    case HLSL_ERROR_TEXTURE_STAGE:
        SemanticError(&failureLoc, ERROR_SS_HLSL_TEXTURE_STAGE,
                      profile->name, reason);
        break;
    case HLSL_ERROR_TEXTURE_SIGNATURE:
        SemanticError(&failureLoc, ERROR_S_HLSL_TEXTURE_SIGNATURE, reason);
        break;
    case HLSL_ERROR_GEOMETRY_MISSING_MAX:
        SemanticError(&failureLoc, ERROR___HLSL_GEOMETRY_MISSING_MAX);
        break;
    case HLSL_ERROR_GEOMETRY_MAX_LIMIT:
        SemanticError(&failureLoc, ERROR_SII_HLSL_GEOMETRY_MAX_LIMIT,
                      reason, module->resourceUsed,
                      module->resourceAvailable);
        break;
    case HLSL_ERROR_GEOMETRY_TOTAL_OUTPUT_LIMIT:
        SemanticError(&failureLoc, ERROR_SII_HLSL_GEOMETRY_TOTAL_LIMIT,
                      reason, module->resourceUsed,
                      module->resourceAvailable);
        break;
    case HLSL_ERROR_INVALID_IR:
        InternalError(&failureLoc, ERROR___HLSL_INVALID_IR);
        break;
    case HLSL_ERROR_UNSUPPORTED_OPERATION:
        ReportHlslModelDiagnostic(&failureLoc, profile,
                                  HLSL_ERROR_UNSUPPORTED_OPERATION,
                                  reason);
        break;
    case HLSL_ERROR_NONE:
    case HLSL_ERROR_RESOURCE_LIMIT:
    default:
        InternalError(&failureLoc, ERROR___HLSL_INVALID_IR);
        break;
    }
    return 0;
} // ReportHlslFailure

#if defined(HLSL_DIAGNOSTIC_TESTING)
int HlslReportFailureForTesting(const HlslModule *module,
    const HlslProfileDesc *profile, const Symbol *program)
{
    return ReportHlslFailure(module, profile, program);
} // HlslReportFailureForTesting
#endif

/*
 * GenerateCode_hlsl() - Orchestrate the backend pipeline.
 */

static int GenerateCode_hlsl(SourceLoc *loc, Scope *fScope, Symbol *program)
{
    return GenerateCodeIR_hlsl(loc, fScope, program, NULL);
} // GenerateCode_hlsl

static int ReportHlslPhaseFailure(HlslModule *module,
    const HlslProfileDesc *profile, const Symbol *program,
    int errorsBefore)
{
    if (GetErrorCount() != errorsBefore)
        return 0;
    if (module->errors == 0)
        HlslFail(module, HLSL_ERROR_INVALID_IR, NULL,
                 "HLSL backend phase failed without a reason");
    return ReportHlslFailure(module, profile, program);
} // ReportHlslPhaseFailure

static int GenerateCodeIR_hlsl(SourceLoc *loc, Scope *fScope,
                               Symbol *program,
                               const CgIRModule *sourceIR)
{
    HlslModule module;
    const HlslProfileDesc *profile;
    int errorsBefore;

    profile = GetHlslProfile();
    HlslInitModule(&module, profile->stage, HlslCompilerAlloc,
                   CurrentScope->pool);
    module.selectedEntryName = GetAtomString(atable, program->name);
    errorsBefore = GetErrorCount();
    if (!((profile->stage == HLSL_STAGE_GEOMETRY && sourceIR != NULL) ?
          HlslLowerProgramWithIR(&module, profile, loc, fScope, program,
                                 sourceIR) :
          HlslLowerProgram(&module, profile, loc, fScope, program)))
        return ReportHlslPhaseFailure(&module, profile, program,
                                      errorsBefore);
    errorsBefore = GetErrorCount();
    if (!HlslBuildEntryWrapper(&module, profile))
        return ReportHlslPhaseFailure(&module, profile, program,
                                      errorsBefore);
    errorsBefore = GetErrorCount();
    if (!HlslLegalizeModule(&module, profile))
        return ReportHlslPhaseFailure(&module, profile, program,
                                      errorsBefore);
    errorsBefore = GetErrorCount();
    if (!HlslValidateSamplerUsage(&module, profile))
        return ReportHlslPhaseFailure(&module, profile, program,
                                      errorsBefore);
    errorsBefore = GetErrorCount();
    if (!HlslAllocateBindings(&module, profile))
        return ReportHlslPhaseFailure(&module, profile, program,
                                      errorsBefore);
    errorsBefore = GetErrorCount();
    if (!HlslLegalizeModernTextureAbi(&module, profile))
        return ReportHlslPhaseFailure(&module, profile, program,
                                      errorsBefore);
    errorsBefore = GetErrorCount();
    if (!HlslValidateModule(&module, profile))
        return ReportHlslPhaseFailure(&module, profile, program,
                                      errorsBefore);
    errorsBefore = GetErrorCount();
    if (!HlslWriteModule(Cg->options.outfd, &module, profile))
        return ReportHlslPhaseFailure(&module, profile, program,
                                      errorsBefore);
    return 1;
} // GenerateCodeIR_hlsl

///////////////////////////////////////////////////////////////////////////////
///////////////////////// InitHAL_hlslv / InitHAL_hlslf ////////////////////////
///////////////////////////////////////////////////////////////////////////////

/*
 * InitHAL_hlslv() - Initialize HAL for the vertex profile.
 */

int InitHAL_hlslv(slHAL *fHAL)
{
    return InitHAL_hlsl_profile(fHAL, &HlslProfile_hlslv);
} // InitHAL_hlslv

/*
 * InitHAL_hlslf() - Initialize HAL for the pixel profile.
 */

int InitHAL_hlslf(slHAL *fHAL)
{
    return InitHAL_hlsl_profile(fHAL, &HlslProfile_hlslf);
} // InitHAL_hlslf

///////////////////////////////////////////////////////////////////////////////
////////////////////////// End of hlsl_hal.c //////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

#endif // !defined(HLSL_CANONICALIZATION_ONLY)

#endif // !defined(HLSL_PROFILE_REGISTRATION_ONLY)

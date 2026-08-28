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
#include "hlsl_hal.h"

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

#if !defined(HLSL_CANONICALIZATION_ONLY)

#define HLSL_MAX_INTERFACE_REGISTERS 32

typedef struct HlslHALData_Rec {
    const HlslProfileDesc *profile;
    Symbol *inputUsed[HLSL_MAX_INTERFACE_REGISTERS];
    Symbol *outputUsed[HLSL_MAX_INTERFACE_REGISTERS];
    HlslErrorKind errorKind;
    const char *errorReason;
    SourceLoc errorLoc;
} HlslHALData;

// Static HAL callbacks:
static int InitHAL_hlsl(slHAL *fHAL, const HlslProfileDesc *profile);
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
static int BindVaryingUnbound_hlsl(SourceLoc *loc, Symbol *fSymb, int name,
                                   int semantic, Binding *fBind,
                                   int IsOutVal);
static int PrintCodeHeader_hlsl(FILE *out);
static int GenerateCode_hlsl(SourceLoc *loc, Scope *fScope, Symbol *program);
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
    HlslHALData *data;

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
    fHAL->BindUniformUnbound = BindUniformUnbound_hlsl;
    fHAL->BindVaryingSemantic = BindVaryingSemantic_hlsl;
    fHAL->BindVaryingUnbound = BindVaryingUnbound_hlsl;
    fHAL->PrintCodeHeader = PrintCodeHeader_hlsl;
    fHAL->GenerateCode = GenerateCode_hlsl;

    // Data members:
    fHAL->vendor = VENDOR_STRING_HLSL;
    fHAL->version = VERSION_STRING_HLSL;
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
} // InitHAL_hlsl

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
    fBind->none.properties |= BIND_IS_BOUND | BIND_UNIFORM;
    return 1;
} // BindUniformUnbound_hlsl

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
    SourceLoc failureLoc;
    const char *reason;

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
    if (module->resourceName != NULL) {
        SemanticError(&failureLoc, ERROR_SII_HLSL_RESOURCE_LIMIT,
                      module->resourceName, module->resourceUsed,
                      module->resourceAvailable);
        return 0;
    }
    switch (module->errorKind) {
    case HLSL_ERROR_UNSUPPORTED_TYPE:
        SemanticError(&failureLoc, ERROR_S_HLSL_UNSUPPORTED_TYPE, reason);
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
        SemanticError(&failureLoc, ERROR_S_HLSL_SAMPLER, reason);
        break;
    case HLSL_ERROR_INTRINSIC:
        SemanticError(&failureLoc, ERROR_S_HLSL_INTRINSIC, reason);
        break;
    case HLSL_ERROR_NAME_COLLISION:
        SemanticError(&failureLoc, ERROR_S_HLSL_NAME_COLLISION, reason);
        break;
    case HLSL_ERROR_INVALID_IR:
        InternalError(&failureLoc, ERROR___HLSL_INVALID_IR);
        break;
    case HLSL_ERROR_NONE:
    case HLSL_ERROR_UNSUPPORTED_OPERATION:
    case HLSL_ERROR_RESOURCE_LIMIT:
    default:
        SemanticError(&failureLoc, ERROR_S_HLSL_UNSUPPORTED_OPERATION,
                      reason);
        break;
    }
    return 0;
} // ReportHlslFailure

/*
 * GenerateCode_hlsl() - Orchestrate the backend pipeline.
 */

static int GenerateCode_hlsl(SourceLoc *loc, Scope *fScope, Symbol *program)
{
    HlslModule module;
    const HlslProfileDesc *profile;

    profile = GetHlslProfile();
    HlslInitModule(&module, profile->stage, HlslCompilerAlloc,
                   CurrentScope->pool);
    if (!HlslLowerProgram(&module, profile, loc, fScope, program) ||
        !HlslBuildEntryWrapper(&module, profile) ||
        !HlslLegalizeModule(&module, profile) ||
        !HlslAllocateBindings(&module, profile) ||
        !HlslValidateModule(&module, profile))
    {
        return ReportHlslFailure(&module, profile, program);
    }
    if (!HlslWriteModule(Cg->options.outfd, &module, profile))
        return ReportHlslFailure(&module, profile, program);
    return 1;
} // GenerateCode_hlsl

///////////////////////////////////////////////////////////////////////////////
//////////////////// Registration-Only Pipeline Phases ///////////////////////
///////////////////////////////////////////////////////////////////////////////

static int HlslRecordUnsupported(HlslModule *module, const HlslLoc *loc,
                                 const char *reason)
{
    if (module != NULL && module->errors == 0) {
        module->errorKind = HLSL_ERROR_UNSUPPORTED_OPERATION;
        module->errorReason = reason;
        if (loc != NULL)
            module->errorLoc = *loc;
    }
    if (module != NULL)
        module->errors++;
    return 0;
} // HlslRecordUnsupported

static int HlslIsIdentifier(const char *name)
{
    const char *current;

    if (name == NULL ||
        !((name[0] >= 'A' && name[0] <= 'Z') ||
          (name[0] >= 'a' && name[0] <= 'z') || name[0] == '_'))
    {
        return 0;
    }
    for (current = name + 1; *current != '\0'; current++) {
        if (!((*current >= 'A' && *current <= 'Z') ||
              (*current >= 'a' && *current <= 'z') ||
              (*current >= '0' && *current <= '9') || *current == '_'))
        {
            return 0;
        }
    }
    return !HlslIsReservedName(name) || !strcmp(name, "main") ||
           !strncmp(name, "cg_", 3);
}

static int HlslHasEmptyEntry(const HlslModule *module)
{
    const HlslFunction *entry;

    if (module == NULL || module->entry == NULL)
        return 0;
    entry = module->entry;
    if (module->structs != NULL || module->globals != NULL ||
        module->bindings != NULL || module->wrapper != NULL ||
        module->functions != entry || entry->next != NULL ||
        !HlslIsIdentifier(entry->name) || !entry->isEntry ||
        entry->parameters != NULL || entry->locals != NULL ||
        entry->body != NULL)
    {
        return 0;
    }
    return entry->result.base == HLSL_BASE_VOID &&
           entry->result.len == 0 && entry->result.rows == 0 &&
           entry->result.cols == 0 && entry->result.arraySize == 0 &&
           entry->result.structName == NULL &&
           entry->result.elementType == NULL &&
           entry->result.members == NULL;
} // HlslHasEmptyEntry

int HlslLowerProgram(HlslModule *module, const HlslProfileDesc *profile,
                     SourceLoc *loc, Scope *scope, Symbol *program)
{
    HlslFunction *entry;
    HlslLoc errorLoc;
    HlslType result;

    (void) scope;
    errorLoc.file = loc != NULL ? loc->file : 0;
    errorLoc.line = loc != NULL ? loc->line : 0;
    if (module == NULL || profile == NULL || program == NULL ||
        module->stage != profile->stage || program->kind != FUNCTION_S)
    {
        return HlslRecordUnsupported(module, &errorLoc,
                                     "HLSL entry program");
    }
    errorLoc.file = program->loc.file;
    errorLoc.line = program->loc.line;
    if (program->details.fun.params != NULL ||
        program->details.fun.statements != NULL ||
        program->details.fun.entryOutputAssignments != NULL)
    {
        return HlslRecordUnsupported(module, &errorLoc,
                                     "nonempty entry program");
    }
    result = HlslNumericType(HLSL_BASE_VOID, 0);
    entry = HlslNewFunction(module, result, "main");
    if (entry == NULL)
        return HlslRecordUnsupported(module, &errorLoc,
                                     "HLSL module allocation");
    entry->loc = errorLoc;
    entry->identity = program;
    entry->isEntry = 1;
    HlslAppendFunction(&module->functions, entry);
    module->entry = entry;
    return 1;
} // HlslLowerProgram

int HlslBuildEntryWrapper(HlslModule *module,
                          const HlslProfileDesc *profile)
{
    (void) profile;
    if (!HlslHasEmptyEntry(module))
        return HlslRecordUnsupported(module, NULL,
                                     "nonempty entry wrapper");
    return 1;
} // HlslBuildEntryWrapper

int HlslLegalizeModule(HlslModule *module,
                       const HlslProfileDesc *profile)
{
    (void) profile;
    if (!HlslHasEmptyEntry(module))
        return HlslRecordUnsupported(module, NULL,
                                     "nonempty HLSL legalization");
    return 1;
} // HlslLegalizeModule

int HlslValidateModule(HlslModule *module,
                       const HlslProfileDesc *profile)
{
    if (profile == NULL || module == NULL ||
        (module->stage != HLSL_STAGE_VERTEX &&
         module->stage != HLSL_STAGE_PIXEL) ||
        module->stage != profile->stage || profile->name == NULL ||
        profile->name[0] == '\0' || profile->target == NULL ||
        profile->target[0] == '\0' || !HlslHasEmptyEntry(module))
    {
        return HlslRecordUnsupported(module, NULL,
                                     "nonempty HLSL validation");
    }
    return 1;
} // HlslValidateModule

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

#endif // !defined(HLSL_CANONICALIZATION_ONLY)

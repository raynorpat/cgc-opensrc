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
express or implied, are granted by NVIDIA herein including but not
limited to any patent rights that may be infringed by your derivative
works. No hardware is licensed hereunder.

THE NVIDIA SOFTWARE IS BEING PROVIDED ON AN "AS IS" BASIS, WITHOUT
WARRANTIES OR CONDITIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED,
INCLUDING WITHOUT LIMITATION, WARRANTIES OF TITLE,
NON-INFRINGEMENT, MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR
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

//
// arb_hal.c
//

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "slglobals.h"
#include "arb_hal.h"

#define NUMELS(x) (sizeof(x) / sizeof((x)[0]))

// Static functions

static int FreeHAL_arb(slHAL *fHAL);
static int RegisterNames_arb(slHAL *fHAL);
static int GetConnectorID_arb(int name);
static int GetConnectorAtom_arb(int name);
static int GetConnectorUses_arb(int cid, int pid);
static int GetConnectorRegister_arb(int cid, int ByIndex, int ratom,
                                    Binding *fBind);
static int GetCapsBit_arb(int bitNumber);
static int CheckInternalFunction_arb(Symbol *fSymb, int *group);
static int IsTexobjBase_arb(int fBase);
static int IsValidRuntimeBase_arb(int fBase);
static int BindUniformUnbound_arb(SourceLoc *loc, Symbol *fSymb,
                                  Binding *fBind);
static int BindUniformPragma_arb(SourceLoc *loc, Symbol *fSymb, Binding *lBind,
                                 const Binding *fBind);
static int BindVaryingSemantic_arb(SourceLoc *loc, Symbol *fSymb,
                                   int semantic, Binding *fBind,
                                   int IsOutVal);
static int BindVaryingPragma_arb(SourceLoc *loc, Symbol *fSymb, Binding *lBind,
                                 const Binding *fBind, int IsOutVal);
static int PrintCodeHeader_arb(FILE *out);

/*
 * FindConnector_arb() - Return the input or output connector descriptor
 *         for this profile.
 */

static ConnectorDescriptor *FindConnector_arb(const ArbProfileDesc *profile,
                                              int IsOutVal)
{
    int ii;
    int want = IsOutVal ? CONNECTOR_IS_OUTPUT : CONNECTOR_IS_INPUT;

    for (ii = 0; ii < profile->numConnectors; ii++) {
        if (profile->connectors[ii].properties == want)
            return &profile->connectors[ii];
    }
    return NULL;
} // FindConnector_arb

/*
 * ResolveRegisterName_arb() - Return the connector register name atom to
 *         record in a semantic binding.  A semantic spelled exactly like a
 *         register (generic ATTRn aliases, WPOS, and so on) keeps its own
 *         spelling; anything else uses the canonical register name for the
 *         resolved register number.
 */

static int ResolveRegisterName_arb(ConnectorDescriptor *conn,
                                   int semanticAtom, int regno)
{
    int ii;

    if (conn) {
        for (ii = 0; ii < conn->numregs; ii++) {
            if (conn->registers[ii].name == semanticAtom)
                return semanticAtom;
        }
        for (ii = 0; ii < conn->numregs; ii++) {
            if (conn->registers[ii].regno == regno)
                return conn->registers[ii].name;
        }
    }
    return semanticAtom;
} // ResolveRegisterName_arb

/*
 * RegisterProfiles_arb() - Register both ARB profiles with the compiler.
 */

int RegisterProfiles_arb(void)
{
    RegisterProfile(InitHAL_arbvp1, PROFILE_ARBVP1_NAME, PROFILE_ARBVP1_ID);
    RegisterProfile(InitHAL_arbfp1, PROFILE_ARBFP1_NAME, PROFILE_ARBFP1_ID);
    return 1;
} // RegisterProfiles_arb

/*
 * InitHAL_arb() - Initialize shared ARB profile state for one stage.
 */

int InitHAL_arb(slHAL *fHAL, const ArbProfileDesc *profile)
{
    ArbHALData *data;
    ConnectorDescriptor *conn;

    data = (ArbHALData *) malloc(sizeof(ArbHALData));
    if (!data) {
        FatalError("malloc failed");
        return 0;
    }
    memset(data, 0, sizeof(ArbHALData));
    data->profile = profile;

    fHAL->FreeHAL = FreeHAL_arb;
    fHAL->RegisterNames = RegisterNames_arb;
    fHAL->GetConnectorID = GetConnectorID_arb;
    fHAL->GetConnectorAtom = GetConnectorAtom_arb;
    fHAL->GetConnectorUses = GetConnectorUses_arb;
    fHAL->GetConnectorRegister = GetConnectorRegister_arb;
    fHAL->GetCapsBit = GetCapsBit_arb;
    fHAL->CheckInternalFunction = CheckInternalFunction_arb;
    fHAL->IsTexobjBase = IsTexobjBase_arb;
    fHAL->IsValidRuntimeBase = IsValidRuntimeBase_arb;
    fHAL->BindUniformUnbound = BindUniformUnbound_arb;
    fHAL->BindUniformPragma = BindUniformPragma_arb;
    fHAL->BindVaryingSemantic = BindVaryingSemantic_arb;
    fHAL->BindVaryingPragma = BindVaryingPragma_arb;
    fHAL->PrintCodeHeader = PrintCodeHeader_arb;
    fHAL->GenerateCode = GenerateCode_arb;

    fHAL->vendor = "NVIDIA Corporation";
    fHAL->version = "1.0";

    fHAL->semantics = profile->semantics;
    fHAL->numSemantics = profile->numSemantics;

    conn = FindConnector_arb(profile, 0);
    fHAL->incid = conn ? conn->cid : CID_NONE_ID;
    fHAL->inputCRegs = conn ? conn->registers : NULL;
    fHAL->numInputCRegs = conn ? conn->numregs : 0;

    conn = FindConnector_arb(profile, 1);
    fHAL->outcid = conn ? conn->cid : CID_NONE_ID;
    fHAL->outputCRegs = conn ? conn->registers : NULL;
    fHAL->numOutputCRegs = conn ? conn->numregs : 0;

    fHAL->comment = "#";
    fHAL->localData = data;
    return 1;
} // InitHAL_arb

/*
 * FreeHAL_arb()
 */

static int FreeHAL_arb(slHAL *fHAL)
{
    free(fHAL->localData);
    fHAL->localData = NULL;
    return 1;
} // FreeHAL_arb

/*
 * RegisterNames_arb() - Atomize every connector name, connector register
 *         name, and semantic name.  The fragment stage adds its sampler
 *         base types.
 */

static void RegisterSamplerType_arb(const char *name, int base)
{
    SourceLoc loc = { 0, 0 };
    Type *type = NewType(TYPE_CATEGORY_SCALAR | base, 1);
    int atom = LookUpAddString(atable, name);

    SetScalarTypeName(base, atom, type);
    AddSymbol(&loc, CurrentScope, atom, type, TYPEDEF_S);
} // RegisterSamplerType_arb

static int RegisterNames_arb(slHAL *fHAL)
{
    ArbHALData *data = (ArbHALData *) fHAL->localData;
    const ArbProfileDesc *profile = data->profile;
    int i, j;

    for (i = 0; i < profile->numConnectors; i++) {
        ConnectorDescriptor *conn = &profile->connectors[i];
        conn->name = AddAtom(atable, conn->sname);
        for (j = 0; j < conn->numregs; j++)
            conn->registers[j].name = AddAtom(atable, conn->registers[j].sname);
    }

    if (profile->stage == ARB_STAGE_FRAGMENT) {
        RegisterSamplerType_arb("sampler1D", TYPE_BASE_SAMPLER1D);
        RegisterSamplerType_arb("sampler2D", TYPE_BASE_SAMPLER2D);
        RegisterSamplerType_arb("sampler3D", TYPE_BASE_SAMPLER3D);
        RegisterSamplerType_arb("samplerCUBE", TYPE_BASE_SAMPLERCUBE);
        RegisterSamplerType_arb("samplerRECT", TYPE_BASE_SAMPLERRECT);
    }
    return 1;
} // RegisterNames_arb

/*
 * GetConnectorID_arb()
 */

static int GetConnectorID_arb(int name)
{
    ArbHALData *data = (ArbHALData *) Cg->theHAL->localData;
    const ArbProfileDesc *profile = data->profile;
    int i;

    for (i = 0; i < profile->numConnectors; i++) {
        if (name == profile->connectors[i].name)
            return profile->connectors[i].cid;
    }
    return 0;
} // GetConnectorID_arb

/*
 * GetConnectorAtom_arb()
 */

static int GetConnectorAtom_arb(int name)
{
    ArbHALData *data = (ArbHALData *) Cg->theHAL->localData;
    const ArbProfileDesc *profile = data->profile;
    ConnectorDescriptor *conn
        = LookupConnectorHAL(profile->connectors, name, profile->numConnectors);
    return conn ? conn->name : 0;
} // GetConnectorAtom_arb

/*
 * GetConnectorUses_arb()
 */

static int GetConnectorUses_arb(int cid, int pid)
{
    ArbHALData *data = (ArbHALData *) Cg->theHAL->localData;
    const ArbProfileDesc *profile = data->profile;
    ConnectorDescriptor *conn
        = LookupConnectorHAL(profile->connectors, cid, profile->numConnectors);
    return conn ? conn->properties : 0;
} // GetConnectorUses_arb

/*
 * GetConnectorRegister_arb()
 */

static int GetConnectorRegister_arb(int cid, int ByIndex, int ratom,
                                    Binding *fBind)
{
    ArbHALData *data = (ArbHALData *) Cg->theHAL->localData;
    const ArbProfileDesc *profile = data->profile;
    ConnectorDescriptor *conn;
    int i;

    if (!fBind)
        return 0;
    conn = LookupConnectorHAL(profile->connectors, cid, profile->numConnectors);
    if (!conn || !conn->registers)
        return 0;
    for (i = 0; i < conn->numregs; i++) {
        if (ratom == conn->registers[i].name) {
            SetSymbolConnectorBindingHAL(fBind, &conn->registers[i]);
            return 1;
        }
    }
    return 0;
} // GetConnectorRegister_arb

/*
 * GetCapsBit_arb() - Capabilities shared by both ARB stages.  Indexed
 *         arrays are advertised because the preserved frontend uses this
 *         bit to permit unpacked array declarations; the backend enforces
 *         the per-stage indexing rules itself.
 */

static int GetCapsBit_arb(int bitNumber)
{
    switch (bitNumber) {
    case CAPS_INLINE_ALL_FUNCTIONS:
    case CAPS_RESTRICT_RETURNS:
    case CAPS_DECONSTRUCT_MATRICES:
    case CAPS_LATE_BINDINGS:
    case CAPS_INDEXED_ARRAYS:
        return 1;
    default:
        return 0;
    }
} // GetCapsBit_arb

/*
 * CheckInternalFunction_arb() - Recognize internally implemented functions.
 *         Texture built-ins are added with the same stable IDs in the
 *         fragment stage; rsqrt is available to both stages.
 */

static int CheckInternalFunction_arb(Symbol *fSymb, int *group)
{
    const char *name = GetAtomString(atable, fSymb->name);

    if (!strcmp(name, "rsqrt")) {
        *group = ARB_BUILTIN_GROUP;
        return ARB_BUILTIN_RSQ;
    }
    return 0;
} // CheckInternalFunction_arb

/*
 * IsTexobjBase_arb() - Sampler base types are registered by the fragment
 *         stage's RegisterNames hook; until then no base is a texture
 *         object base.
 */

static int IsTexobjBase_arb(int fBase)
{
    switch (fBase) {
    case TYPE_BASE_SAMPLER1D:
    case TYPE_BASE_SAMPLER2D:
    case TYPE_BASE_SAMPLER3D:
    case TYPE_BASE_SAMPLERCUBE:
    case TYPE_BASE_SAMPLERRECT:
        return 1;
    default:
        return 0;
    }
} // IsTexobjBase_arb

/*
 * IsValidRuntimeBase_arb() - Float, bool, and int runtime variables are
 *         representable.  Int is needed so the backend can distinguish
 *         statically eliminated loop/index values from illegal surviving
 *         runtime integer arithmetic.
 */

static int IsValidRuntimeBase_arb(int fBase)
{
    switch (fBase) {
    case TYPE_BASE_FLOAT:
    case TYPE_BASE_BOOLEAN:
    case TYPE_BASE_INT:
        return 1;
    default:
        return 0;
    }
} // IsValidRuntimeBase_arb

/*
 * BindUniformUnbound_arb() - Deterministic numeric uniform allocation is
 *         implemented with the uniform-packing support.
 */

static int BindUniformUnbound_arb(SourceLoc *loc, Symbol *fSymb,
                                  Binding *fBind)
{
    return 0;
} // BindUniformUnbound_arb

/*
 * BindUniformPragma_arb() - Explicit uniform pragmas are implemented with
 *         the uniform-packing support.
 */

static int BindUniformPragma_arb(SourceLoc *loc, Symbol *fSymb, Binding *lBind,
                                 const Binding *fBind)
{
    return 0;
} // BindUniformPragma_arb

/*
 * BindVaryingSemantic_arb() - Bind a varying variable with a semantic to
 *         an ARB vertex attribute or program result register.
 */

static int BindVaryingSemantic_arb(SourceLoc *loc, Symbol *fSymb,
                                   int semantic, Binding *fBind,
                                   int IsOutVal)
{
    const ArbProfileDesc *profile
        = ((ArbHALData *) Cg->theHAL->localData)->profile;
    SemanticsDescriptor *semantics = profile->semantics;
    ConnectorDescriptor *conn;
    const char *pname, *match;
    char root[128];
    Type *lType;
    int ii, index, len, rname, regno, HasSuffix;

    pname = GetAtomString(atable, semantic);
    HasSuffix = HasNumericSuffix(pname, root, sizeof(root), &index);
    for (ii = 0; ii < profile->numSemantics; ii++, semantics++) {
        match = semantics->numregs > 0 ? root : pname;
        if (!strcmp(match, semantics->sname)) {
            if (semantics->numregs > 0) {
                if (index >= semantics->numregs) {
                    SemanticError(loc, ERROR_S_SEMANTICS_INDEX_TOO_BIG, pname);
                    return 0;
                }
            } else {
                index = 0;
            }

            // Found a match.  See if the type is compatible:

            lType = fSymb->type;
            if (IsScalar(lType)) {
                len = 1;
            } else if (IsVector(lType, &len)) {
            } else {
                SemanticError(loc, ERROR_S_SEM_VAR_NOT_SCALAR_VECTOR,
                              GetAtomString(atable, fSymb->name));
                return 0;
            }
            if (GetBase(lType) != TYPE_BASE_FLOAT)
                return 0;
            if (IsOutVal && !(semantics->properties & SEM_OUT))
                continue;
            if (!IsOutVal && !(semantics->properties & SEM_IN))
                continue;

            regno = semantics->regno + index;
            conn = FindConnector_arb(profile, IsOutVal);
            rname = ResolveRegisterName_arb(conn, semantic, regno);

            fBind->none.kind = BK_CONNECTOR;
            fBind->conn.rname = rname;
            fBind->conn.regno = regno;
            fBind->none.properties |= BIND_IS_BOUND | BIND_VARYING;
            if (semantics->properties & SEM_IN)
                fBind->none.properties |= BIND_INPUT;
            if (semantics->properties & SEM_OUT)
                fBind->none.properties |= BIND_OUTPUT;
            if (semantics->properties & SEM_REQUIRED)
                fBind->none.properties |= BIND_WRITE_REQUIRED;
            fBind->none.base = semantics->base;
            fBind->none.size = semantics->size;
            fSymb->properties |= SYMB_IS_CONNECTOR_REGISTER;
            fSymb->properties |= SYMB_CONNECTOR_CAN_READ;
            fSymb->properties |= SYMB_CONNECTOR_CAN_WRITE;
            return 1;
        }
    }
    return 0;
} // BindVaryingSemantic_arb

/*
 * BindVaryingPragma_arb() - Accept only BK_CONNECTOR binding trees whose
 *         named register exists in this stage and whose direction matches.
 */

static int BindVaryingPragma_arb(SourceLoc *loc, Symbol *fSymb, Binding *lBind,
                                 const Binding *fBind, int IsOutVal)
{
    const ArbProfileDesc *profile
        = ((ArbHALData *) Cg->theHAL->localData)->profile;
    ConnectorDescriptor *conn;
    Type *lType;
    int i, len;

    if (fBind->none.kind != BK_CONNECTOR)
        return 0;
    lType = fSymb->type;
    if (!IsScalar(lType) && !IsVector(lType, &len))
        return 0;
    conn = FindConnector_arb(profile, IsOutVal);
    if (!conn)
        return 0;
    for (i = 0; i < conn->numregs; i++) {
        if (conn->registers[i].name == fBind->conn.rname) {
            lBind->none.kind = BK_CONNECTOR;
            lBind->conn.properties |= BIND_IS_BOUND | BIND_VARYING;
            if (IsOutVal)
                lBind->none.properties |= BIND_OUTPUT;
            else
                lBind->none.properties |= BIND_INPUT;
            lBind->conn.base = conn->registers[i].base;
            lBind->conn.size = conn->registers[i].size;
            lBind->conn.rname = fBind->conn.rname;
            lBind->conn.regno = conn->registers[i].regno;
            fSymb->properties |= SYMB_IS_CONNECTOR_REGISTER;
            fSymb->properties |= SYMB_CONNECTOR_CAN_READ;
            fSymb->properties |= SYMB_CONNECTOR_CAN_WRITE;
            return 1;
        }
    }
    return 0;
} // BindVaryingPragma_arb

/*
 * PrintCodeHeader_arb() - Write the ARB profile header.
 */

static int PrintCodeHeader_arb(FILE *out)
{
    ArbHALData *data = (ArbHALData *) Cg->theHAL->localData;

    fprintf(out, "%s\n", data->profile->header);
    return 1;
} // PrintCodeHeader_arb

/*
 * GenerateCode_arb() - Placeholder until the lowering and code-generation
 *         modules are added.
 */

int GenerateCode_arb(SourceLoc *loc, Scope *fScope, Symbol *program)
{
    return 1;
} // GenerateCode_arb

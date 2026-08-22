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
// glsl_hal.c
//

#include <stdio.h>
#include <string.h>

#include "slglobals.h"
#include "glsl_hal.h"

static const GlslProfileDesc *GetProfile_glsl(void)
{
    return (const GlslProfileDesc *) Cg->theHAL->localData;
}

static int GlslSemanticParts(const char *name, int *rootLength, int *index)
{
    const char *digits;
    const char *p;
    int value;

    if (name == NULL || name[0] == '\0')
        return 0;
    digits = name + strlen(name);
    while (digits > name && digits[-1] >= '0' && digits[-1] <= '9')
        digits--;
    if (digits == name)
        return 0;
    value = 0;
    for (p = digits; *p != '\0'; p++) {
        if (value > 1000000)
            return 0;
        value = value * 10 + (*p - '0');
    }
    *rootLength = (int) (digits - name);
    *index = value;
    return 1;
}

static int GlslSemanticRootEquals(const char *name, int rootLength,
                                  const char *root)
{
    return (int) strlen(root) == rootLength &&
           !strncmp(name, root, rootLength);
}

static const char *GlslConnectorName(const GlslProfileDesc *profile,
                                     const GlslSemanticDesc *semantic,
                                     int index, int isOutput)
{
    ConnectorRegisters *registers;
    const char *name;
    int count;
    int i;
    int rootLength;
    int registerIndex;

    if (isOutput) {
        registers = profile->outputRegs;
        count = profile->numOutputRegs;
    } else {
        registers = profile->inputRegs;
        count = profile->numInputRegs;
    }
    for (i = 0; i < count; i++) {
        name = registers[i].sname;
        if (GlslSemanticParts(name, &rootLength, &registerIndex) &&
            registerIndex == index &&
            GlslSemanticRootEquals(name, rootLength,
                                   semantic->canonicalRoot))
        {
            return name;
        }
    }
    return NULL;
}

const char *GlslCanonicalInterfaceName(const GlslProfileDesc *profile,
                                       int semantic, int isOutput)
{
    const GlslSemanticDesc *desc;
    const char *name;
    int direction;
    int i;
    int index;
    int rootLength;

    if (profile == NULL || semantic == 0)
        return NULL;
    name = GetAtomString(atable, semantic);
    if (name == NULL)
        return NULL;
    for (i = 0; i < profile->numAliases; i++) {
        if (!strcmp(name, profile->aliases[i].alias)) {
            name = profile->aliases[i].canonical;
            break;
        }
    }
    if (!GlslSemanticParts(name, &rootLength, &index))
        return NULL;
    direction = isOutput ? SEM_OUT : SEM_IN;
    for (i = 0; i < profile->numSemanticMap; i++) {
        desc = &profile->semanticMap[i];
        if (!(desc->properties & direction) ||
            !GlslSemanticRootEquals(name, rootLength, desc->root))
        {
            continue;
        }
        if (index < desc->firstIndex ||
            index >= desc->firstIndex + desc->count)
        {
            return NULL;
        }
        switch (desc->interfaceKind) {
        case GLSL_INTERFACE_POSITION:
            return "gl_Position";
        case GLSL_INTERFACE_POINT_SIZE:
            return "gl_PointSize";
        case GLSL_INTERFACE_FRAG_COORD:
            return "gl_FragCoord";
        case GLSL_INTERFACE_FRONT_FACING:
            return "gl_FrontFacing";
        case GLSL_INTERFACE_FRAG_COLOR:
            return "gl_FragColor";
        case GLSL_INTERFACE_FRAG_DEPTH:
            return "gl_FragDepth";
        case GLSL_INTERFACE_ATTRIBUTE:
        case GLSL_INTERFACE_VARYING:
            return GlslConnectorName(profile, desc, index, isOutput);
        }
    }
    return NULL;
}

static int RegisterNames_glsl(slHAL *hal)
{
    const GlslProfileDesc *profile;
    ConnectorDescriptor *connector;
    GlslSemanticDesc *semantic;
    GlslSemanticAlias *alias;
    int i, j;

    profile = (const GlslProfileDesc *) hal->localData;
    for (i = 0; i < profile->numConnectors; i++) {
        connector = &profile->connectors[i];
        connector->name = AddAtom(atable, connector->sname);
        for (j = 0; j < connector->numregs; j++)
            connector->registers[j].name = AddAtom(atable,
                                                    connector->registers[j].sname);
    }
    for (i = 0; i < profile->numSemanticMap; i++) {
        semantic = &profile->semanticMap[i];
        AddAtom(atable, semantic->root);
        AddAtom(atable, semantic->canonicalRoot);
    }
    for (i = 0; i < profile->numAliases; i++) {
        alias = &profile->aliases[i];
        AddAtom(atable, alias->alias);
        AddAtom(atable, alias->canonical);
    }
    return 1;
}

static int GetConnectorID_glsl(int name)
{
    const GlslProfileDesc *profile;
    int i;

    profile = GetProfile_glsl();
    for (i = 0; i < profile->numConnectors; i++) {
        if (name == profile->connectors[i].name)
            return profile->connectors[i].cid;
    }
    return CID_NONE_ID;
}

static int GetConnectorAtom_glsl(int cid)
{
    const GlslProfileDesc *profile;
    ConnectorDescriptor *connector;

    profile = GetProfile_glsl();
    connector = LookupConnectorHAL(profile->connectors, cid,
                                   profile->numConnectors);
    return connector ? connector->name : 0;
}

static int GetConnectorUses_glsl(int cid, int pid)
{
    const GlslProfileDesc *profile;
    ConnectorDescriptor *connector;

    profile = GetProfile_glsl();
    connector = LookupConnectorHAL(profile->connectors, cid,
                                   profile->numConnectors);
    return connector ? connector->properties : CONNECTOR_IS_USELESS;
}

static int GetConnectorRegister_glsl(int cid, int ByIndex, int ratom,
                                     Binding *fBind)
{
    const GlslProfileDesc *profile;
    ConnectorDescriptor *connector;
    int i;

    profile = GetProfile_glsl();
    connector = LookupConnectorHAL(profile->connectors, cid,
                                   profile->numConnectors);
    if (!connector)
        return 0;

    if (ByIndex) {
        if (ratom < 0)
            return connector->numregs;
        i = ratom;
    } else {
        for (i = 0; i < connector->numregs; i++) {
            if (ratom == connector->registers[i].name)
                break;
        }
    }
    if (i < 0 || i >= connector->numregs || !fBind)
        return 0;

    SetSymbolConnectorBindingHAL(fBind, &connector->registers[i]);
    return 1;
}

static int BindVaryingSemantic_glsl(SourceLoc *loc, Symbol *fSymb,
                                    int semanticAtom, Binding *fBind,
                                    int IsOutVal)
{
    const GlslProfileDesc *profile;
    const char *semanticName;
    const char *canonicalName;
    GlslSemanticDesc *semantic;
    Type *type;
    char root[128];
    char registerName[128];
    int i, index, len, base, direction, cid, rname;

    profile = GetProfile_glsl();
    semanticName = GetAtomString(atable, semanticAtom);
    canonicalName = semanticName;
    for (i = 0; i < profile->numAliases; i++) {
        if (!strcmp(semanticName, profile->aliases[i].alias)) {
            canonicalName = profile->aliases[i].canonical;
            break;
        }
    }

    HasNumericSuffix(canonicalName, root, sizeof(root), &index);
    direction = IsOutVal ? SEM_OUT : SEM_IN;
    semantic = profile->semanticMap;
    for (i = 0; i < profile->numSemanticMap; i++, semantic++) {
        if (!strcmp(root, semantic->root) &&
            (semantic->properties & direction))
        {
            if (index < semantic->firstIndex ||
                index >= semantic->firstIndex + semantic->count)
            {
                SemanticError(loc, ERROR_S_SEMANTICS_INDEX_TOO_BIG,
                              semanticName);
                return 0;
            }

            type = fSymb->type;
            if (IsScalar(type)) {
                len = 1;
            } else if (!IsVector(type, &len)) {
                SemanticError(loc, ERROR_S_SEM_VAR_NOT_SCALAR_VECTOR,
                              GetAtomString(atable, fSymb->name));
                return 0;
            }
            base = GetBase(type);
            if (semantic->interfaceKind == GLSL_INTERFACE_FRONT_FACING) {
                if (!IsScalar(type) || base != TYPE_BASE_BOOLEAN)
                    return 0;
            } else if (base != TYPE_BASE_FLOAT || len > semantic->size) {
                return 0;
            }

            sprintf(registerName, "%s%d", semantic->canonicalRoot, index);
            rname = AddAtom(atable, registerName);
            cid = IsOutVal ? profile->outputCid : profile->inputCid;
            if (!GetConnectorRegister_glsl(cid, 0, rname, fBind))
                return 0;

            fBind->none.properties |= BIND_VARYING;
            fSymb->properties |= SYMB_IS_CONNECTOR_REGISTER |
                                 SYMB_CONNECTOR_CAN_READ |
                                 SYMB_CONNECTOR_CAN_WRITE;
            if (semantic->properties & SEM_IN) {
                fBind->none.properties |= BIND_INPUT;
            }
            if (semantic->properties & SEM_OUT) {
                fBind->none.properties |= BIND_OUTPUT;
            }
            if (semantic->properties & SEM_REQUIRED)
                fBind->none.properties |= BIND_WRITE_REQUIRED;
            return 1;
        }
    }
    return 0;
}

static int BindVaryingUnbound_glsl(SourceLoc *loc, Symbol *fSymb, int name,
                                   int semantic, Binding *fBind, int IsOutVal)
{
    return 0;
}

static int CheckInternalFunction_glsl(Symbol *symbol, int *group)
{
    const char *name;

    name = GetAtomString(atable, symbol->name);
    if (!strcmp(name, "rsqrt")) {
        *group = 3;
        return 1;
    }
    return 0;
}

static int FreeHAL_glsl(slHAL *hal)
{
    hal->localData = NULL;
    return 1;
}

static int GetCapsBit_glsl(int bitNumber)
{
    switch (bitNumber) {
    case CAPS_LATE_BINDINGS:
    case CAPS_INDEXED_ARRAYS:
    case CAPS_DONT_FLATTEN_IF_STATEMENTS:
        return 1;
    default:
        return 0;
    }
}

static int PrintCodeHeader_glsl(FILE *out)
{
    fprintf(out, "#version 110\n");
    return 1;
}

static void *GlslCompilerAlloc(void *arg, size_t size)
{
    return mem_Calloc((MemoryPool *) arg, size, 1);
}

static int GenerateCode_glsl(SourceLoc *loc, Scope *scope, Symbol *program)
{
    const GlslProfileDesc *profile;
    GlslModule module;

    profile = (const GlslProfileDesc *) Cg->theHAL->localData;
    GlslInitModule(&module, profile->stage, GlslCompilerAlloc,
                   CurrentScope->pool);
    if (!GlslLowerProgram(&module, profile, loc, scope, program))
        return 0;
    return GlslWriteModule(Cg->options.outfd, &module);
}

int GlslInitHAL(slHAL *hal, const GlslProfileDesc *profile)
{
    hal->FreeHAL = FreeHAL_glsl;
    hal->RegisterNames = RegisterNames_glsl;
    hal->GetCapsBit = GetCapsBit_glsl;
    hal->GetConnectorID = GetConnectorID_glsl;
    hal->GetConnectorAtom = GetConnectorAtom_glsl;
    hal->GetConnectorUses = GetConnectorUses_glsl;
    hal->GetConnectorRegister = GetConnectorRegister_glsl;
    hal->CheckInternalFunction = CheckInternalFunction_glsl;
    hal->BindVaryingSemantic = BindVaryingSemantic_glsl;
    hal->BindVaryingUnbound = BindVaryingUnbound_glsl;
    hal->PrintCodeHeader = PrintCodeHeader_glsl;
    hal->GenerateCode = GenerateCode_glsl;

    hal->vendor = VENDOR_STRING_GLSL;
    hal->version = VERSION_STRING_GLSL;

    hal->semantics = NULL;
    hal->numSemantics = 0;

    hal->incid = profile->inputCid;
    hal->inputCRegs = profile->inputRegs;
    hal->numInputCRegs = profile->numInputRegs;

    hal->outcid = profile->outputCid;
    hal->outputCRegs = profile->outputRegs;
    hal->numOutputCRegs = profile->numOutputRegs;

    hal->comment = "//";
    hal->localData = (void *) profile;

    return 1;
}

int RegisterProfiles_glsl(void)
{
    RegisterProfile(InitHAL_glslv, PROFILE_GLSLV_NAME, PROFILE_GLSLV_ID);
    RegisterProfile(InitHAL_glslf, PROFILE_GLSLF_NAME, PROFILE_GLSLF_ID);
    return 1;
}

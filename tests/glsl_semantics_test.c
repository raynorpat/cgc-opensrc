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
license, under NVIDIA's copyrights in this original NVIDIA software
(the "NVIDIA Software"), to use, reproduce, modify and redistribute the
NVIDIA Software, with or without modifications, in source and/or binary
forms; provided that if you redistribute the NVIDIA Software, you must
retain the copyright notice of NVIDIA, this notice and the following
text and disclaimers in all such redistributions of the NVIDIA Software.
Neither the name, trademarks, service marks nor logos of NVIDIA
Corporation may be used to endorse or promote products derived from
this NVIDIA Software without specific prior written permission from
NVIDIA. Except as expressly stated in this notice, no other rights or
licenses express or implied, are granted by NVIDIA herein, including
but not limited to any patent rights that may be infringed by your
derivative works or by other works in which the NVIDIA Software may be
incorporated. No hardware is licensed hereunder.

THE NVIDIA SOFTWARE IS BEING PROVIDED ON AN "AS IS" BASIS, WITHOUT
WARRANTIES OR CONDITIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED,
INCLUDING WITHOUT LIMITATION, WARRANTIES OR CONDITIONS OF TITLE,
NON-INFRINGEMENT, MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE,
OR ITS USE AND OPERATION EITHER ALONE OR IN COMBINATION WITH OTHER
PRODUCTS.

IN NO EVENT SHALL NVIDIA BE LIABLE FOR ANY SPECIAL, INDIRECT,
INCIDENTAL, EXEMPLARY, CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, LOST PROFITS; PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) OR ARISING IN
ANY WAY OUT OF THE USE, REPRODUCTION, MODIFICATION AND/OR DISTRIBUTION
OF THE NVIDIA SOFTWARE, HOWEVER CAUSED AND WHETHER UNDER THEORY OF
CONTRACT, TORT (INCLUDING NEGLIGENCE), STRICT LIABILITY OR OTHERWISE,
EVEN IF NVIDIA HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
\****************************************************************************/
// glsl_semantics_test.c
//

#if defined(NDEBUG)
#undef NDEBUG
#endif
#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "slglobals.h"
#include "glsl_hal.h"

#define NUMELS(x) ((int) (sizeof(x) / sizeof((x)[0])))

typedef struct ExpectedRegister_Rec {
    const char *name;
    int base;
    int size;
    int properties;
} ExpectedRegister;

static CgStruct testCg;
CgStruct *Cg = &testCg;
Scope *CurrentScope = NULL;

static int semanticErrorCount;
static int lastSemanticError;
static int lowerProgramResult;
static int writeModuleResult;
static int writeModuleDiagnostic;

static const GlslSemanticDesc vertexSemantics[] = {
    { "ATTRIB",       "ATTRIB",       0, 16, SEM_IN | SEM_VARYING, 4, GLSL_INTERFACE_ATTRIBUTE },
    { "POSITION",     "POSITION",     0,  1, SEM_IN | SEM_VARYING, 4, GLSL_INTERFACE_ATTRIBUTE },
    { "NORMAL",       "NORMAL",       0,  1, SEM_IN | SEM_VARYING, 3, GLSL_INTERFACE_ATTRIBUTE },
    { "COLOR",        "COLOR",        0,  2, SEM_IN | SEM_VARYING, 4, GLSL_INTERFACE_ATTRIBUTE },
    { "TEXCOORD",     "TEXCOORD",     0,  8, SEM_IN | SEM_VARYING, 4, GLSL_INTERFACE_ATTRIBUTE },
    { "TANGENT",      "TANGENT",      0,  1, SEM_IN | SEM_VARYING, 3, GLSL_INTERFACE_ATTRIBUTE },
    { "BINORMAL",     "BINORMAL",     0,  1, SEM_IN | SEM_VARYING, 3, GLSL_INTERFACE_ATTRIBUTE },
    { "BLENDWEIGHT",  "BLENDWEIGHT",  0,  1, SEM_IN | SEM_VARYING, 4, GLSL_INTERFACE_ATTRIBUTE },
    { "BLENDINDICES", "BLENDINDICES", 0,  1, SEM_IN | SEM_VARYING, 4, GLSL_INTERFACE_ATTRIBUTE },
    { "POSITION",     "POSITION",     0,  1, SEM_OUT | SEM_VARYING | SEM_REQUIRED, 4, GLSL_INTERFACE_POSITION },
    { "COLOR",        "COLOR",        0,  2, SEM_OUT | SEM_VARYING, 4, GLSL_INTERFACE_VARYING },
    { "TEXCOORD",     "TEXCOORD",     0,  8, SEM_OUT | SEM_VARYING, 4, GLSL_INTERFACE_VARYING },
    { "FOG",          "FOG",          0,  1, SEM_OUT | SEM_VARYING, 1, GLSL_INTERFACE_VARYING },
    { "PSIZE",        "PSIZE",        0,  1, SEM_OUT | SEM_VARYING, 1, GLSL_INTERFACE_POINT_SIZE }
};

static const GlslSemanticAlias vertexAliases[] = {
    { "DIFFUSE",  "COLOR0" },
    { "SPECULAR", "COLOR1" },
    { "FOGCOORD", "FOG0" },
    { "HPOS",     "POSITION0" }
};

static const ExpectedRegister vertexInput[] = {
    { "ATTRIB0",       TYPE_BASE_FLOAT, 4, REG_INPUT },
    { "ATTRIB1",       TYPE_BASE_FLOAT, 4, REG_INPUT },
    { "ATTRIB2",       TYPE_BASE_FLOAT, 4, REG_INPUT },
    { "ATTRIB3",       TYPE_BASE_FLOAT, 4, REG_INPUT },
    { "ATTRIB4",       TYPE_BASE_FLOAT, 4, REG_INPUT },
    { "ATTRIB5",       TYPE_BASE_FLOAT, 4, REG_INPUT },
    { "ATTRIB6",       TYPE_BASE_FLOAT, 4, REG_INPUT },
    { "ATTRIB7",       TYPE_BASE_FLOAT, 4, REG_INPUT },
    { "ATTRIB8",       TYPE_BASE_FLOAT, 4, REG_INPUT },
    { "ATTRIB9",       TYPE_BASE_FLOAT, 4, REG_INPUT },
    { "ATTRIB10",      TYPE_BASE_FLOAT, 4, REG_INPUT },
    { "ATTRIB11",      TYPE_BASE_FLOAT, 4, REG_INPUT },
    { "ATTRIB12",      TYPE_BASE_FLOAT, 4, REG_INPUT },
    { "ATTRIB13",      TYPE_BASE_FLOAT, 4, REG_INPUT },
    { "ATTRIB14",      TYPE_BASE_FLOAT, 4, REG_INPUT },
    { "ATTRIB15",      TYPE_BASE_FLOAT, 4, REG_INPUT },
    { "POSITION0",     TYPE_BASE_FLOAT, 4, REG_INPUT },
    { "NORMAL0",       TYPE_BASE_FLOAT, 3, REG_INPUT },
    { "COLOR0",        TYPE_BASE_FLOAT, 4, REG_INPUT },
    { "COLOR1",        TYPE_BASE_FLOAT, 4, REG_INPUT },
    { "TEXCOORD0",     TYPE_BASE_FLOAT, 4, REG_INPUT },
    { "TEXCOORD1",     TYPE_BASE_FLOAT, 4, REG_INPUT },
    { "TEXCOORD2",     TYPE_BASE_FLOAT, 4, REG_INPUT },
    { "TEXCOORD3",     TYPE_BASE_FLOAT, 4, REG_INPUT },
    { "TEXCOORD4",     TYPE_BASE_FLOAT, 4, REG_INPUT },
    { "TEXCOORD5",     TYPE_BASE_FLOAT, 4, REG_INPUT },
    { "TEXCOORD6",     TYPE_BASE_FLOAT, 4, REG_INPUT },
    { "TEXCOORD7",     TYPE_BASE_FLOAT, 4, REG_INPUT },
    { "TANGENT0",      TYPE_BASE_FLOAT, 3, REG_INPUT },
    { "BINORMAL0",     TYPE_BASE_FLOAT, 3, REG_INPUT },
    { "BLENDWEIGHT0",  TYPE_BASE_FLOAT, 4, REG_INPUT },
    { "BLENDINDICES0", TYPE_BASE_FLOAT, 4, REG_INPUT }
};

static const ExpectedRegister vertexOutput[] = {
    { "POSITION0", TYPE_BASE_FLOAT, 4, REG_OUTPUT | REG_WRITE_REQUIRED },
    { "COLOR0",    TYPE_BASE_FLOAT, 4, REG_OUTPUT },
    { "COLOR1",    TYPE_BASE_FLOAT, 4, REG_OUTPUT },
    { "TEXCOORD0", TYPE_BASE_FLOAT, 4, REG_OUTPUT },
    { "TEXCOORD1", TYPE_BASE_FLOAT, 4, REG_OUTPUT },
    { "TEXCOORD2", TYPE_BASE_FLOAT, 4, REG_OUTPUT },
    { "TEXCOORD3", TYPE_BASE_FLOAT, 4, REG_OUTPUT },
    { "TEXCOORD4", TYPE_BASE_FLOAT, 4, REG_OUTPUT },
    { "TEXCOORD5", TYPE_BASE_FLOAT, 4, REG_OUTPUT },
    { "TEXCOORD6", TYPE_BASE_FLOAT, 4, REG_OUTPUT },
    { "TEXCOORD7", TYPE_BASE_FLOAT, 4, REG_OUTPUT },
    { "FOG0",      TYPE_BASE_FLOAT, 1, REG_OUTPUT },
    { "PSIZE0",    TYPE_BASE_FLOAT, 1, REG_OUTPUT }
};

static const GlslSemanticDesc fragmentSemantics[] = {
    { "COLOR",    "COLOR",    0, 2, SEM_IN | SEM_VARYING, 4, GLSL_INTERFACE_VARYING },
    { "TEXCOORD", "TEXCOORD", 0, 8, SEM_IN | SEM_VARYING, 4, GLSL_INTERFACE_VARYING },
    { "FOG",      "FOG",      0, 1, SEM_IN | SEM_VARYING, 1, GLSL_INTERFACE_VARYING },
    { "POSITION", "POSITION", 0, 1, SEM_IN | SEM_VARYING, 4, GLSL_INTERFACE_FRAG_COORD },
    { "WPOS",     "WPOS",     0, 1, SEM_IN | SEM_VARYING, 4, GLSL_INTERFACE_FRAG_COORD },
    { "FACE",     "FACE",     0, 1, SEM_IN | SEM_VARYING, 1, GLSL_INTERFACE_FRONT_FACING },
    { "COLOR",    "COLOR",    0, 1, SEM_OUT | SEM_VARYING, 4, GLSL_INTERFACE_FRAG_COLOR },
    { "DEPTH",    "DEPTH",    0, 1, SEM_OUT | SEM_VARYING, 1, GLSL_INTERFACE_FRAG_DEPTH }
};

static const GlslSemanticAlias fragmentAliases[] = {
    { "DIFFUSE",  "COLOR0" },
    { "SPECULAR", "COLOR1" },
    { "FOGCOORD", "FOG0" }
};

static const ExpectedRegister fragmentInput[] = {
    { "COLOR0",    TYPE_BASE_FLOAT,   4, REG_INPUT },
    { "COLOR1",    TYPE_BASE_FLOAT,   4, REG_INPUT },
    { "TEXCOORD0", TYPE_BASE_FLOAT,   4, REG_INPUT },
    { "TEXCOORD1", TYPE_BASE_FLOAT,   4, REG_INPUT },
    { "TEXCOORD2", TYPE_BASE_FLOAT,   4, REG_INPUT },
    { "TEXCOORD3", TYPE_BASE_FLOAT,   4, REG_INPUT },
    { "TEXCOORD4", TYPE_BASE_FLOAT,   4, REG_INPUT },
    { "TEXCOORD5", TYPE_BASE_FLOAT,   4, REG_INPUT },
    { "TEXCOORD6", TYPE_BASE_FLOAT,   4, REG_INPUT },
    { "TEXCOORD7", TYPE_BASE_FLOAT,   4, REG_INPUT },
    { "FOG0",      TYPE_BASE_FLOAT,   1, REG_INPUT },
    { "POSITION0", TYPE_BASE_FLOAT,   4, REG_INPUT },
    { "WPOS0",     TYPE_BASE_FLOAT,   4, REG_INPUT },
    { "FACE0",     TYPE_BASE_BOOLEAN, 1, REG_INPUT }
};

static const ExpectedRegister fragmentOutput[] = {
    { "COLOR0", TYPE_BASE_FLOAT, 4, REG_OUTPUT },
    { "DEPTH0", TYPE_BASE_FLOAT, 1, REG_OUTPUT }
};

static void CheckSemanticMap(const GlslProfileDesc *profile,
                             const GlslSemanticDesc *expected, int count)
{
    const GlslSemanticDesc *actual;
    int i;

    assert(profile->numSemanticMap == count);
    for (i = 0; i < count; i++) {
        actual = &profile->semanticMap[i];
        assert(!strcmp(actual->root, expected[i].root));
        assert(!strcmp(actual->canonicalRoot, expected[i].canonicalRoot));
        assert(actual->firstIndex == expected[i].firstIndex);
        assert(actual->count == expected[i].count);
        assert(actual->properties == expected[i].properties);
        assert(actual->size == expected[i].size);
        assert(actual->interfaceKind == expected[i].interfaceKind);
    }
}

static void CheckAliases(const GlslProfileDesc *profile,
                         const GlslSemanticAlias *expected, int count)
{
    int i;

    assert(profile->numAliases == count);
    for (i = 0; i < count; i++) {
        assert(!strcmp(profile->aliases[i].alias, expected[i].alias));
        assert(!strcmp(profile->aliases[i].canonical,
                       expected[i].canonical));
    }
}

static void CheckConnector(slHAL *hal, ConnectorDescriptor *connector,
                           const char *name, int cid, int uses,
                           const ExpectedRegister *expected, int count)
{
    Binding byIndex;
    Binding byName;
    int i;
    int mask;

    assert(!strcmp(connector->sname, name));
    assert(!strcmp(GetAtomString(atable, connector->name), name));
    assert(connector->cid == cid);
    assert(connector->properties == uses);
    assert(connector->numregs == count);
    assert(hal->GetConnectorID(connector->name) == cid);
    assert(hal->GetConnectorAtom(cid) == connector->name);
    assert(hal->GetConnectorUses(cid, hal->pid) == uses);
    assert(hal->GetConnectorRegister(cid, 1, -1, NULL) == count);

    mask = REG_INPUT | REG_OUTPUT | REG_WRITE_REQUIRED;
    for (i = 0; i < count; i++) {
        assert(!strcmp(connector->registers[i].sname, expected[i].name));
        assert(!strcmp(GetAtomString(atable, connector->registers[i].name),
                       expected[i].name));
        assert(connector->registers[i].regno == i);
        assert(connector->registers[i].base == expected[i].base);
        assert(connector->registers[i].size == expected[i].size);
        assert((connector->registers[i].properties & mask) ==
               expected[i].properties);

        memset(&byIndex, 0, sizeof(byIndex));
        assert(hal->GetConnectorRegister(cid, 1, i, &byIndex));
        assert(byIndex.conn.rname == connector->registers[i].name);
        assert(byIndex.conn.regno == i);
        assert(byIndex.conn.base == expected[i].base);
        assert(byIndex.conn.size == expected[i].size);

        memset(&byName, 0, sizeof(byName));
        assert(hal->GetConnectorRegister(cid, 0,
               connector->registers[i].name, &byName));
        assert(byName.conn.rname == connector->registers[i].name);
        assert(byName.conn.regno == i);
    }

    assert(!hal->GetConnectorRegister(cid, 1, count, &byIndex));
    assert(!hal->GetConnectorRegister(cid, 0,
           AddAtom(atable, "NOT_A_REGISTER"), &byName));
    assert(!hal->GetConnectorRegister(cid, 1, 0, NULL));
    assert(!hal->GetConnectorRegister(cid, 0,
           connector->registers[0].name, NULL));
}

static void MakeScalar(Type *type, int base)
{
    memset(type, 0, sizeof(*type));
    type->properties = TYPE_CATEGORY_SCALAR | base;
    type->co.size = 1;
}

static void MakeVector(Type *type, Type *element, int base, int len)
{
    MakeScalar(element, base);
    memset(type, 0, sizeof(*type));
    type->arr.properties = TYPE_CATEGORY_ARRAY | TYPE_MISC_PACKED | base;
    type->arr.size = len;
    type->arr.eltype = element;
    type->arr.numels = len;
}

static void MakeMatrix(Type *type, Type *row, Type *element, int size)
{
    MakeVector(row, element, TYPE_BASE_FLOAT, size);
    memset(type, 0, sizeof(*type));
    type->arr.properties = TYPE_CATEGORY_ARRAY | TYPE_MISC_PACKED |
                           TYPE_BASE_FLOAT;
    type->arr.size = size * size;
    type->arr.eltype = row;
    type->arr.numels = size;
}

static void CheckUniformBinding(slHAL *hal)
{
    SourceLoc loc;
    Symbol symbol;
    Binding binding;
    Type type;
    Type element;

    memset(&loc, 0, sizeof(loc));
    memset(&symbol, 0, sizeof(symbol));
    memset(&binding, 0, sizeof(binding));
    MakeVector(&type, &element, TYPE_BASE_FLOAT, 3);
    symbol.name = AddAtom(atable, "output");
    symbol.type = &type;
    assert(hal->BindUniformUnbound(&loc, &symbol, &binding));
    assert(binding.none.kind == BK_SEMANTIC);
    assert(binding.none.properties ==
           (BIND_IS_BOUND | BIND_INPUT | BIND_UNIFORM));
    assert(binding.none.base == TYPE_BASE_FLOAT);
    assert(binding.none.size == 3);
    assert(!strcmp(GetAtomString(atable, binding.sem.sname), "cg_output"));
    assert(binding.sem.sregno == 0);
}

static void CheckInternalFunctions(slHAL *hal)
{
    Symbol symbol;
    Type functionType;
    Type result;
    Type matrix;
    Type row;
    Type rowElement;
    Type vector;
    Type vectorElement;
    TypeList first;
    TypeList second;
    int group;

    memset(&symbol, 0, sizeof(symbol));
    memset(&functionType, 0, sizeof(functionType));
    MakeVector(&result, &vectorElement, TYPE_BASE_FLOAT, 4);
    MakeMatrix(&matrix, &row, &rowElement, 4);
    MakeVector(&vector, &vectorElement, TYPE_BASE_FLOAT, 4);
    first.type = &matrix;
    first.next = &second;
    second.type = &vector;
    second.next = NULL;
    functionType.fun.properties = TYPE_CATEGORY_FUNCTION;
    functionType.fun.rettype = &result;
    functionType.fun.paramtypes = &first;
    symbol.kind = FUNCTION_S;
    symbol.name = AddAtom(atable, "mul");
    symbol.type = &functionType;
    group = 0;
    assert(hal->CheckInternalFunction(&symbol, &group) == GLSL_BUILTIN_MUL);
    assert(group == GLSL_BUILTIN_GROUP);

    MakeVector(&vector, &vectorElement, TYPE_BASE_FLOAT, 3);
    group = 0;
    assert(hal->CheckInternalFunction(&symbol, &group) == 0);
    assert(group == 0);

    MakeVector(&result, &vectorElement, TYPE_BASE_FLOAT, 3);
    MakeMatrix(&matrix, &row, &rowElement, 3);
    matrix.arr.properties = (matrix.arr.properties & ~TYPE_BASE_MASK) |
                            TYPE_BASE_INT;
    group = 0;
    assert(hal->CheckInternalFunction(&symbol, &group) == 0);
    assert(group == 0);

    symbol.name = AddAtom(atable, "user_mul");
    MakeVector(&vector, &vectorElement, TYPE_BASE_FLOAT, 4);
    group = 0;
    assert(hal->CheckInternalFunction(&symbol, &group) == 0);
    assert(group == 0);
}

static ConnectorRegisters *FindRegister(const GlslProfileDesc *profile,
                                        int IsOutVal, const char *name)
{
    ConnectorRegisters *registers;
    int count;
    int i;

    if (IsOutVal) {
        registers = profile->outputRegs;
        count = profile->numOutputRegs;
    } else {
        registers = profile->inputRegs;
        count = profile->numInputRegs;
    }
    for (i = 0; i < count; i++) {
        if (!strcmp(registers[i].sname, name))
            return &registers[i];
    }
    return NULL;
}

static void CheckNamedBinding(slHAL *hal, const char *semanticName,
                              int IsOutVal, int base, int len,
                              const char *registerName, int required)
{
    const GlslProfileDesc *profile;
    ConnectorRegisters *reg;
    SourceLoc loc;
    Symbol symbol;
    Binding binding;
    Type type;
    Type element;
    int expectedProperties;
    int propertyMask;

    profile = (const GlslProfileDesc *) hal->localData;
    reg = FindRegister(profile, IsOutVal, registerName);
    assert(reg != NULL);
    memset(&loc, 0, sizeof(loc));
    memset(&symbol, 0, sizeof(symbol));
    memset(&binding, 0, sizeof(binding));
    if (len == 1)
        MakeScalar(&type, base);
    else
        MakeVector(&type, &element, base, len);
    symbol.name = AddAtom(atable, "value");
    symbol.type = &type;

    assert(hal->BindVaryingSemantic(&loc, &symbol,
           AddAtom(atable, semanticName), &binding, IsOutVal));
    assert(binding.none.kind == BK_CONNECTOR);
    assert(binding.conn.rname == reg->name);
    assert(!strcmp(GetAtomString(atable, binding.conn.rname), registerName));
    assert(binding.conn.regno == reg->regno);
    assert(binding.none.base == reg->base);
    assert(binding.none.size == reg->size);

    expectedProperties = BIND_IS_BOUND | BIND_VARYING |
                         (IsOutVal ? BIND_OUTPUT : BIND_INPUT);
    if (required)
        expectedProperties |= BIND_WRITE_REQUIRED;
    propertyMask = BIND_IS_BOUND | BIND_VARYING | BIND_INPUT | BIND_OUTPUT |
                   BIND_WRITE_REQUIRED;
    assert((binding.none.properties & propertyMask) == expectedProperties);
    assert((symbol.properties & (SYMB_IS_CONNECTOR_REGISTER |
           SYMB_CONNECTOR_CAN_READ | SYMB_CONNECTOR_CAN_WRITE)) ==
           (SYMB_IS_CONNECTOR_REGISTER | SYMB_CONNECTOR_CAN_READ |
           SYMB_CONNECTOR_CAN_WRITE));
}

static void CheckRejectedBinding(slHAL *hal, const char *semanticName,
                                 int IsOutVal, int base, int len)
{
    SourceLoc loc;
    Symbol symbol;
    Binding binding;
    Type type;
    Type element;

    memset(&loc, 0, sizeof(loc));
    memset(&symbol, 0, sizeof(symbol));
    memset(&binding, 0, sizeof(binding));
    if (len == 1)
        MakeScalar(&type, base);
    else
        MakeVector(&type, &element, base, len);
    symbol.name = AddAtom(atable, "rejected");
    symbol.type = &type;
    assert(!hal->BindVaryingSemantic(&loc, &symbol,
            AddAtom(atable, semanticName), &binding, IsOutVal));
    assert(!(binding.none.properties & BIND_IS_BOUND));
}

static void CheckRejectedVectorBinding(slHAL *hal, const char *semanticName,
                                       int IsOutVal, int base, int len)
{
    SourceLoc loc;
    Symbol symbol;
    Binding binding;
    Type type;
    Type element;

    memset(&loc, 0, sizeof(loc));
    memset(&symbol, 0, sizeof(symbol));
    memset(&binding, 0, sizeof(binding));
    MakeVector(&type, &element, base, len);
    symbol.name = AddAtom(atable, "rejectedVector");
    symbol.type = &type;
    assert(!hal->BindVaryingSemantic(&loc, &symbol,
            AddAtom(atable, semanticName), &binding, IsOutVal));
    assert(!(binding.none.properties & BIND_IS_BOUND));
}

static void CheckSemanticBoundaries(slHAL *hal)
{
    const GlslProfileDesc *profile;
    const GlslSemanticDesc *semantic;
    char semanticName[64];
    char registerName[64];
    int i, index, base, IsOutVal, required;

    profile = (const GlslProfileDesc *) hal->localData;
    for (i = 0; i < profile->numSemanticMap; i++) {
        semantic = &profile->semanticMap[i];
        IsOutVal = (semantic->properties & SEM_OUT) != 0;
        required = (semantic->properties & SEM_REQUIRED) != 0;
        base = semantic->interfaceKind == GLSL_INTERFACE_FRONT_FACING ?
               TYPE_BASE_BOOLEAN : TYPE_BASE_FLOAT;

        index = semantic->firstIndex;
        sprintf(semanticName, "%s%d", semantic->root, index);
        sprintf(registerName, "%s%d", semantic->canonicalRoot, index);
        CheckNamedBinding(hal, semanticName, IsOutVal, base,
                          semantic->size, registerName, required);

        index = semantic->firstIndex + semantic->count - 1;
        sprintf(semanticName, "%s%d", semantic->root, index);
        sprintf(registerName, "%s%d", semantic->canonicalRoot, index);
        CheckNamedBinding(hal, semanticName, IsOutVal, base,
                          semantic->size, registerName, required);
    }
}

static const GlslProfileDesc *InitStage(slHAL *hal, int vertex)
{
    int result;

    memset(hal, 0, sizeof(*hal));
    Cg->theHAL = hal;
    if (vertex)
        result = InitHAL_glslv(hal);
    else
        result = InitHAL_glslf(hal);
    assert(result);
    result = hal->RegisterNames(hal);
    assert(result);
    assert(hal->semantics == NULL);
    assert(hal->numSemantics == 0);
    return (const GlslProfileDesc *) hal->localData;
}

static void CheckVertex(void)
{
    slHAL hal;
    const GlslProfileDesc *profile;
    SourceLoc loc;
    Symbol symbol;
    Binding binding;

    profile = InitStage(&hal, 1);
    assert(profile->stage == GLSL_STAGE_VERTEX);
    assert(!strcmp(profile->name, PROFILE_GLSLV_NAME));
    assert(profile->pid == PROFILE_GLSLV_ID);
    assert(profile->inputCid == CID_GLSLV_IN_ID);
    assert(profile->outputCid == CID_GLSLV_OUT_ID);
    assert(profile->numConnectors == 2);
    assert(profile->inputRegs == profile->connectors[0].registers);
    assert(profile->numInputRegs == NUMELS(vertexInput));
    assert(profile->outputRegs == profile->connectors[1].registers);
    assert(profile->numOutputRegs == NUMELS(vertexOutput));
    CheckSemanticMap(profile, vertexSemantics, NUMELS(vertexSemantics));
    CheckAliases(profile, vertexAliases, NUMELS(vertexAliases));
    CheckConnector(&hal, &profile->connectors[0], CID_GLSLV_IN_NAME,
                   CID_GLSLV_IN_ID, CONNECTOR_IS_INPUT, vertexInput,
                   NUMELS(vertexInput));
    CheckConnector(&hal, &profile->connectors[1], CID_GLSLV_OUT_NAME,
                   CID_GLSLV_OUT_ID, CONNECTOR_IS_OUTPUT, vertexOutput,
                   NUMELS(vertexOutput));
    assert(hal.GetConnectorID(AddAtom(atable, "FOO")) == CID_NONE_ID);
    assert(hal.GetConnectorAtom(CID_NONE_ID) == 0);
    assert(hal.GetConnectorUses(CID_NONE_ID, hal.pid) ==
           CONNECTOR_IS_USELESS);
    CheckSemanticBoundaries(&hal);
    CheckUniformBinding(&hal);
    CheckInternalFunctions(&hal);

    assert(!strcmp(GlslCanonicalInterfaceName(profile,
                   AddAtom(atable, "ATTRIB0"), 0), "ATTRIB0"));
    assert(!strcmp(GlslCanonicalInterfaceName(profile,
                   AddAtom(atable, "POSITION"), 0), "POSITION0"));
    assert(!strcmp(GlslCanonicalInterfaceName(profile,
                   AddAtom(atable, "POSITION"), 1), "gl_Position"));
    assert(!strcmp(GlslCanonicalInterfaceName(profile,
                   AddAtom(atable, "HPOS"), 1), "gl_Position"));
    assert(!strcmp(GlslCanonicalInterfaceName(profile,
                   AddAtom(atable, "PSIZE"), 1), "gl_PointSize"));
    assert(GlslCanonicalInterfaceName(profile,
           AddAtom(atable, "ATTRIB16"), 0) == NULL);
    assert(GlslCanonicalInterfaceName(profile,
           AddAtom(atable, "NOT_A_SEMANTIC"), 0) == NULL);

    CheckNamedBinding(&hal, "TEXCOORD", 0, TYPE_BASE_FLOAT, 2,
                      "TEXCOORD0", 0);
    CheckNamedBinding(&hal, "POSITION", 0, TYPE_BASE_FLOAT, 4,
                      "POSITION0", 0);
    CheckNamedBinding(&hal, "POSITION", 1, TYPE_BASE_FLOAT, 4,
                      "POSITION0", 1);
    CheckNamedBinding(&hal, "COLOR1", 0, TYPE_BASE_FLOAT, 4,
                      "COLOR1", 0);
    CheckNamedBinding(&hal, "COLOR1", 1, TYPE_BASE_FLOAT, 4,
                      "COLOR1", 0);

    CheckNamedBinding(&hal, "DIFFUSE", 0, TYPE_BASE_FLOAT, 4,
                      "COLOR0", 0);
    CheckNamedBinding(&hal, "DIFFUSE", 1, TYPE_BASE_FLOAT, 4,
                      "COLOR0", 0);
    CheckNamedBinding(&hal, "SPECULAR", 0, TYPE_BASE_FLOAT, 4,
                      "COLOR1", 0);
    CheckNamedBinding(&hal, "SPECULAR", 1, TYPE_BASE_FLOAT, 4,
                      "COLOR1", 0);
    CheckNamedBinding(&hal, "FOGCOORD", 1, TYPE_BASE_FLOAT, 1,
                      "FOG0", 0);
    CheckNamedBinding(&hal, "HPOS", 0, TYPE_BASE_FLOAT, 4,
                      "POSITION0", 0);
    CheckNamedBinding(&hal, "HPOS", 1, TYPE_BASE_FLOAT, 4,
                      "POSITION0", 1);

    semanticErrorCount = 0;
    CheckRejectedBinding(&hal, "TEXCOORD8", 0, TYPE_BASE_FLOAT, 4);
    assert(semanticErrorCount == 1);
    assert(lastSemanticError == 5102);
    CheckRejectedBinding(&hal, "NORMAL", 1, TYPE_BASE_FLOAT, 3);
    CheckRejectedBinding(&hal, "NORMAL", 0, TYPE_BASE_FLOAT, 4);
    CheckRejectedBinding(&hal, "POSITION", 1, TYPE_BASE_FLOAT, 3);
    CheckRejectedBinding(&hal, "HPOS", 1, TYPE_BASE_FLOAT, 2);
    CheckRejectedBinding(&hal, "PSIZE", 1, TYPE_BASE_FLOAT, 2);
    CheckRejectedVectorBinding(&hal, "PSIZE", 1,
                               TYPE_BASE_FLOAT, 1);

    memset(&loc, 0, sizeof(loc));
    memset(&symbol, 0, sizeof(symbol));
    memset(&binding, 0, sizeof(binding));
    assert(!hal.BindVaryingUnbound(&loc, &symbol, 0, 0, &binding, 0));
}

static void CheckFragment(void)
{
    slHAL hal;
    const GlslProfileDesc *profile;

    profile = InitStage(&hal, 0);
    assert(profile->stage == GLSL_STAGE_FRAGMENT);
    assert(!strcmp(profile->name, PROFILE_GLSLF_NAME));
    assert(profile->pid == PROFILE_GLSLF_ID);
    assert(profile->inputCid == CID_GLSLF_IN_ID);
    assert(profile->outputCid == CID_GLSLF_OUT_ID);
    assert(profile->numConnectors == 2);
    assert(profile->inputRegs == profile->connectors[0].registers);
    assert(profile->numInputRegs == NUMELS(fragmentInput));
    assert(profile->outputRegs == profile->connectors[1].registers);
    assert(profile->numOutputRegs == NUMELS(fragmentOutput));
    CheckSemanticMap(profile, fragmentSemantics, NUMELS(fragmentSemantics));
    CheckAliases(profile, fragmentAliases, NUMELS(fragmentAliases));
    CheckConnector(&hal, &profile->connectors[0], CID_GLSLF_IN_NAME,
                   CID_GLSLF_IN_ID, CONNECTOR_IS_INPUT, fragmentInput,
                   NUMELS(fragmentInput));
    CheckConnector(&hal, &profile->connectors[1], CID_GLSLF_OUT_NAME,
                   CID_GLSLF_OUT_ID, CONNECTOR_IS_OUTPUT, fragmentOutput,
                   NUMELS(fragmentOutput));
    CheckSemanticBoundaries(&hal);

    assert(!strcmp(GlslCanonicalInterfaceName(profile,
                   AddAtom(atable, "POSITION"), 0), "gl_FragCoord"));
    assert(!strcmp(GlslCanonicalInterfaceName(profile,
                   AddAtom(atable, "WPOS"), 0), "gl_FragCoord"));
    assert(!strcmp(GlslCanonicalInterfaceName(profile,
                   AddAtom(atable, "FACE"), 0), "gl_FrontFacing"));
    assert(!strcmp(GlslCanonicalInterfaceName(profile,
                   AddAtom(atable, "COLOR"), 1), "gl_FragColor"));
    assert(!strcmp(GlslCanonicalInterfaceName(profile,
                   AddAtom(atable, "DEPTH"), 1), "gl_FragDepth"));
    assert(!strcmp(GlslCanonicalInterfaceName(profile,
                   AddAtom(atable, "COLOR1"), 0), "COLOR1"));
    assert(GlslCanonicalInterfaceName(profile,
           AddAtom(atable, "COLOR1"), 1) == NULL);

    CheckNamedBinding(&hal, "COLOR", 0, TYPE_BASE_FLOAT, 4,
                      "COLOR0", 0);
    CheckNamedBinding(&hal, "COLOR", 1, TYPE_BASE_FLOAT, 4,
                      "COLOR0", 0);
    CheckNamedBinding(&hal, "DIFFUSE", 0, TYPE_BASE_FLOAT, 4,
                      "COLOR0", 0);
    CheckNamedBinding(&hal, "DIFFUSE", 1, TYPE_BASE_FLOAT, 4,
                      "COLOR0", 0);
    CheckNamedBinding(&hal, "SPECULAR", 0, TYPE_BASE_FLOAT, 4,
                      "COLOR1", 0);
    CheckNamedBinding(&hal, "FOGCOORD", 0, TYPE_BASE_FLOAT, 1,
                      "FOG0", 0);
    CheckNamedBinding(&hal, "FACE", 0, TYPE_BASE_BOOLEAN, 1,
                      "FACE0", 0);

    CheckRejectedBinding(&hal, "SPECULAR", 1, TYPE_BASE_FLOAT, 4);
    CheckRejectedBinding(&hal, "DEPTH", 0, TYPE_BASE_FLOAT, 1);
    CheckRejectedBinding(&hal, "COLOR1", 1, TYPE_BASE_FLOAT, 4);
    CheckRejectedBinding(&hal, "COLOR", 0, TYPE_BASE_FLOAT, 5);
    CheckRejectedVectorBinding(&hal, "FACE", 0, TYPE_BASE_BOOLEAN, 1);
    CheckRejectedVectorBinding(&hal, "FACE", 0, TYPE_BASE_BOOLEAN, 2);
    CheckRejectedBinding(&hal, "FACE", 0, TYPE_BASE_FLOAT, 1);
    CheckRejectedBinding(&hal, "WPOS", 0, TYPE_BASE_FLOAT, 3);
    CheckRejectedBinding(&hal, "POSITION", 0, TYPE_BASE_FLOAT, 2);
    CheckRejectedBinding(&hal, "COLOR", 1, TYPE_BASE_FLOAT, 3);
    CheckRejectedVectorBinding(&hal, "DEPTH", 1,
                               TYPE_BASE_FLOAT, 1);

    assert(!hal.BindVaryingUnbound(NULL, NULL, 0, 0, NULL, 1));
}

static void CheckOperatorFilter(void)
{
    static const int rejected[] = {
        MOD_OP, MOD_V_OP, MOD_SV_OP, MOD_VS_OP, ASSIGNMOD_OP,
        SHL_OP, SHL_V_OP, SHR_OP, SHR_V_OP,
        NOT_OP, NOT_V_OP,
        AND_OP, AND_V_OP, AND_SV_OP, AND_VS_OP,
        XOR_OP, XOR_V_OP, XOR_SV_OP, XOR_VS_OP,
        OR_OP, OR_V_OP, OR_SV_OP, OR_VS_OP
    };
    static const int accepted[] = {
        BNOT_OP, BNOT_V_OP, BAND_OP, BOR_OP, ADD_OP, ASSIGN_OP
    };
    slHAL hal;
    SourceLoc loc;
    int i;

    InitStage(&hal, 1);
    memset(&loc, 0, sizeof(loc));
    for (i = 0; i < NUMELS(rejected); i++) {
        semanticErrorCount = 0;
        assert(!hal.IsValidOperator(&loc, 0, rejected[i], 0));
        assert(semanticErrorCount == 1);
        assert(lastSemanticError == 5508);
    }
    semanticErrorCount = 0;
    for (i = 0; i < NUMELS(accepted); i++)
        assert(hal.IsValidOperator(&loc, 0, accepted[i], 0));
    assert(semanticErrorCount == 0);
}

static void CheckGenerateCodeWriterFailure(void)
{
    slHAL hal;
    Scope scope;
    Symbol program;
    SourceLoc loc;
    FILE *out;

    InitStage(&hal, 1);
    memset(&scope, 0, sizeof(scope));
    memset(&program, 0, sizeof(program));
    memset(&loc, 0, sizeof(loc));
    loc.line = 41;
    program.loc.line = 37;
    CurrentScope = &scope;
    out = tmpfile();
    assert(out != NULL);
    Cg->options.outfd = out;
    lowerProgramResult = 1;
    writeModuleResult = 0;
    semanticErrorCount = 0;
    assert(!hal.GenerateCode(&loc, &scope, &program));
    if (semanticErrorCount != 1 || lastSemanticError != 5508) {
        fprintf(stderr,
                "writer failure produced %d diagnostics, last C%04d\n",
                semanticErrorCount, lastSemanticError);
        exit(1);
    }
    semanticErrorCount = 0;
    writeModuleDiagnostic = 1;
    assert(!hal.GenerateCode(&loc, &scope, &program));
    if (semanticErrorCount != 1 || lastSemanticError != 5508) {
        fprintf(stderr,
                "accounted writer failure produced %d diagnostics, last C%04d\n",
                semanticErrorCount, lastSemanticError);
        exit(1);
    }
    writeModuleDiagnostic = 0;
    assert(!fclose(out));
    CurrentScope = NULL;
}

int main(void)
{
    int result;

    memset(&testCg, 0, sizeof(testCg));
    result = InitAtomTable(atable, 0);
    assert(result);
    CheckVertex();
    CheckFragment();
    CheckOperatorFilter();
    CheckGenerateCodeWriterFailure();
    FreeAtomTable(atable);
    return 0;
}

int GlslLowerProgram(GlslModule *module, const GlslProfileDesc *profile,
                     SourceLoc *loc, Scope *scope, Symbol *program)
{
    return lowerProgramResult;
}

int GlslWriteModule(FILE *out, const GlslModule *module)
{
    SourceLoc loc;

    if (writeModuleDiagnostic) {
        memset(&loc, 0, sizeof(loc));
        SemanticError(&loc, ERROR_S_UNSUPPORTED_PROFILE_OP,
                      "test writer");
    }
    return writeModuleResult;
}

int GetErrorCount(void)
{
    return semanticErrorCount;
}

slProfile *RegisterProfile(int (*InitHAL)(slHAL *), const char *name, int id)
{
    return NULL;
}

ConnectorDescriptor *LookupConnectorHAL(ConnectorDescriptor *connectors,
                                        int cid, int count)
{
    int i;

    for (i = 0; i < count; i++) {
        if (connectors[i].cid == cid)
            return &connectors[i];
    }
    return NULL;
}

void SetSymbolConnectorBindingHAL(Binding *binding,
                                  ConnectorRegisters *connector)
{
    binding->conn.properties = BIND_IS_BOUND;
    binding->conn.kind = BK_CONNECTOR;
    if (connector->properties & REG_WRITE_REQUIRED)
        binding->conn.properties |= BIND_WRITE_REQUIRED;
    binding->conn.base = connector->base;
    binding->conn.size = connector->size;
    binding->conn.rname = connector->name;
    binding->conn.regno = connector->regno;
}

int HasNumericSuffix(const char *text, char *root, int size, int *suffix)
{
    int value, hasSuffix, len, scale;
    char *s, ch;

    strncpy(root, text, size - 1);
    len = strlen(text);
    if (len >= size)
        len = size - 1;
    root[len] = 0;
    value = 0;
    hasSuffix = 0;
    scale = 1;
    s = &root[len];
    while (1) {
        ch = *--s;
        if (ch >= '0' && ch <= '9' && s >= root) {
            value = value + scale * (ch - '0');
            scale *= 10;
            hasSuffix = 1;
        } else {
            break;
        }
    }
    s[1] = '\0';
    *suffix = value;
    return hasSuffix;
}

int IsScalar(const Type *type)
{
    return type &&
           (type->properties & TYPE_CATEGORY_MASK) == TYPE_CATEGORY_SCALAR;
}

int IsVector(const Type *type, int *len)
{
    if (type &&
        (type->properties & TYPE_CATEGORY_MASK) == TYPE_CATEGORY_ARRAY &&
        (type->properties & TYPE_MISC_PACKED) &&
        type->arr.eltype &&
        (type->arr.eltype->properties & TYPE_CATEGORY_MASK) !=
        TYPE_CATEGORY_ARRAY)
    {
        if (len)
            *len = type->arr.numels;
        return 1;
    }
    return 0;
}

int IsMatrix(const Type *type, int *rows, int *cols)
{
    int rowLength;

    if (type &&
        (type->properties & TYPE_CATEGORY_MASK) == TYPE_CATEGORY_ARRAY &&
        (type->properties & TYPE_MISC_PACKED) &&
        IsVector(type->arr.eltype, &rowLength))
    {
        if (rows)
            *rows = rowLength;
        if (cols)
            *cols = type->arr.numels;
        return 1;
    }
    return 0;
}

int GetCategory(const Type *type)
{
    return type ? type->properties & TYPE_CATEGORY_MASK : TYPE_CATEGORY_NONE;
}

int GetBase(const Type *type)
{
    return type ? type->properties & TYPE_BASE_MASK : TYPE_BASE_NO_TYPE;
}

void SemanticError(SourceLoc *loc, int number, const char *message, ...)
{
    semanticErrorCount++;
    lastSemanticError = number;
}

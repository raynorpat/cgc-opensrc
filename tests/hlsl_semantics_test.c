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
INCIDENTAL, EXEMPLARY, CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, LOST PROFITS; PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) OR ARISING IN
ANY WAY OUT OF THE USE, REPRODUCTION, MODIFICATION AND/OR DISTRIBUTION
OF THE NVIDIA SOFTWARE, HOWEVER CAUSED AND WHETHER UNDER THEORY OF
CONTRACT, TORT (INCLUDING NEGLIGENCE), STRICT LIABILITY OR OTHERWISE,
EVEN IF NVIDIA HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
\****************************************************************************/
// hlsl_semantics_test.c
//

#if defined(NDEBUG)
#undef NDEBUG
#endif
#include <assert.h>
#include <limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "slglobals.h"
#include "cg_stdlib.h"
#include "hlsl_hal.h"

#define NUMELS(x) ((int) (sizeof(x) / sizeof((x)[0])))

static CgStruct testCg;
CgStruct *Cg = &testCg;
Scope *CurrentScope = NULL;

static int semanticErrorCount;
static int lastSemanticError;
static SourceLoc lastSemanticLoc;

static void Require(int condition, const char *message)
{
    if (!condition) {
        fprintf(stderr, "%s\n", message);
        exit(1);
    }
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

static void SetTestScalarKind(Type *type, CgScalarKind kind)
{
    type->co.scalarKind = kind;
    if (IsVector(type, NULL))
        type->arr.eltype->co.scalarKind = kind;
}

static int TestLegacyBaseForKind(CgScalarKind kind)
{
    switch (kind) {
    case CG_SCALAR_CFLOAT:
    case CG_SCALAR_FIXED:
    case CG_SCALAR_HALF:
    case CG_SCALAR_FLOAT:
    case CG_SCALAR_DOUBLE:
        return TYPE_BASE_FLOAT;
    case CG_SCALAR_BOOL:
        return TYPE_BASE_BOOLEAN;
    default:
        return TYPE_BASE_INT;
    }
}

static void RequireRejectedIntrinsicKinds(slHAL *hal, Symbol *symbol,
                                          Type *functionType,
                                          TypeList *parameter,
                                          Type *result, Type *argument,
                                          const char *name,
                                          CgScalarKind resultKind,
                                          CgScalarKind argumentKind,
                                          const char *message)
{
    int group;

    MakeScalar(result, TestLegacyBaseForKind(resultKind));
    MakeScalar(argument, TestLegacyBaseForKind(argumentKind));
    SetTestScalarKind(result, resultKind);
    SetTestScalarKind(argument, argumentKind);
    symbol->name = AddAtom(atable, name);
    functionType->fun.rettype = result;
    functionType->fun.paramtypes = parameter;
    parameter->type = argument;
    parameter->next = NULL;
    semanticErrorCount = 0;
    group = 0;
    Require(hal->CheckInternalFunction(symbol, &group) == 0 && group == 0 &&
            semanticErrorCount == 1 && lastSemanticError == 6410,
            message);
}

static const HlslProfileDesc *InitStage(slHAL *hal, int vertex)
{
    const HlslProfileDesc *profile;

    memset(hal, 0, sizeof(*hal));
    Cg->theHAL = hal;
    if (vertex) {
        assert(InitHAL_hlslv(hal));
        profile = &HlslProfile_hlslv;
    } else {
        assert(InitHAL_hlslf(hal));
        profile = &HlslProfile_hlslf;
    }
    assert(hal->RegisterNames(hal));
    return profile;
}

static void FreeStage(slHAL *hal)
{
    Cg->theHAL = hal;
    assert(hal->FreeHAL(hal));
}

static const char *FindCanonicalRegister(const HlslProfileDesc *profile,
                                         const char *root, int index,
                                         int isOutput)
{
    ConnectorRegisters *registers;
    char registerRoot[64];
    int count, registerIndex, i;

    if (isOutput) {
        registers = profile->outputRegs;
        count = profile->numOutputRegs;
    } else {
        registers = profile->inputRegs;
        count = profile->numInputRegs;
    }
    for (i = 0; i < count; i++) {
        if (HlslParseSemantic(registers[i].sname, registerRoot,
                              sizeof(registerRoot), &registerIndex) &&
            !strcmp(registerRoot, root) && registerIndex == index)
        {
            return registers[i].sname;
        }
    }
    return NULL;
}

static void CheckParser(void)
{
    char semantic[64];
    char root[32];
    unsigned int overflow;
    int index;

    assert(HlslParseSemantic("TEXCOORD15", root, sizeof(root), &index));
    assert(!strcmp(root, "TEXCOORD") && index == 15);
    assert(HlslParseSemantic("POSITION", root, sizeof(root), &index));
    assert(!strcmp(root, "POSITION") && index == 0);
    sprintf(semantic, "T%d", INT_MAX);
    assert(HlslParseSemantic(semantic, root, sizeof(root), &index));
    assert(!strcmp(root, "T") && index == INT_MAX);
    overflow = (unsigned int) INT_MAX + 1U;
    sprintf(semantic, "T%u", overflow);
    assert(!HlslParseSemantic(semantic, root, sizeof(root), &index));
    assert(!HlslParseSemantic(NULL, root, sizeof(root), &index));
    assert(!HlslParseSemantic("POSITION", NULL, sizeof(root), &index));
    assert(!HlslParseSemantic("POSITION", root, sizeof(root), NULL));
    assert(!HlslParseSemantic("POSITION", root, 0, &index));
    assert(!HlslParseSemantic("", root, sizeof(root), &index));
    assert(!HlslParseSemantic("123", root, sizeof(root), &index));
    assert(!HlslParseSemantic("TEXCOORD0", root, 4, &index));
}

static void CheckSemanticTable(const HlslProfileDesc *profile, int isOutput)
{
    const HlslSemanticDesc *semantics;
    const HlslSemanticAlias *aliases;
    const char *expected, *actual;
    char name[64];
    int semanticCount, aliasCount;
    int i, j, onePast;

    if (isOutput) {
        semantics = profile->outputSemantics;
        semanticCount = profile->numOutputSemantics;
        aliases = profile->outputAliases;
        aliasCount = profile->numOutputAliases;
    } else {
        semantics = profile->inputSemantics;
        semanticCount = profile->numInputSemantics;
        aliases = profile->inputAliases;
        aliasCount = profile->numInputAliases;
    }
    for (i = 0; i < semanticCount; i++) {
        for (j = 0; j < semantics[i].count; j++) {
            sprintf(name, "%s%d", semantics[i].root,
                    semantics[i].firstIndex + j);
            expected = FindCanonicalRegister(profile, semantics[i].root,
                    semantics[i].firstIndex + j, isOutput);
            actual = HlslCanonicalSemantic(profile, name, isOutput);
            if (expected == NULL || actual != expected)
                fprintf(stderr, "canonical mismatch %s output=%d expected=%s actual=%s\n",
                        name, isOutput, expected ? expected : "NULL",
                        actual ? actual : "NULL");
            assert(expected != NULL && actual == expected);
        }
        onePast = semantics[i].firstIndex + semantics[i].count;
        sprintf(name, "%s%d", semantics[i].root, onePast);
        assert(HlslCanonicalSemantic(profile, name, isOutput) == NULL);
    }
    for (i = 0; i < aliasCount; i++) {
        actual = HlslCanonicalSemantic(profile, aliases[i].source, isOutput);
        assert(actual != NULL);
        assert(!strcmp(actual, aliases[i].target));
    }
}

typedef struct ExpectedSemantic_Rec {
    const char *root;
    int firstIndex;
    int count;
    int properties;
    int width;
    HlslInterface interfaceKind;
} ExpectedSemantic;

static void CheckExpectedSemantics(const HlslProfileDesc *profile,
                                   const ExpectedSemantic *expected,
                                   int expectedCount, int isOutput)
{
    const HlslSemanticDesc *semantics;
    char name[64];
    int semanticCount, i, j;

    if (isOutput) {
        semantics = profile->outputSemantics;
        semanticCount = profile->numOutputSemantics;
    } else {
        semantics = profile->inputSemantics;
        semanticCount = profile->numInputSemantics;
    }
    assert(semanticCount == expectedCount);
    for (i = 0; i < expectedCount; i++) {
        for (j = 0; j < semanticCount; j++) {
            if (!strcmp(semantics[j].root, expected[i].root))
                break;
        }
        assert(j < semanticCount);
        assert(semantics[j].firstIndex == expected[i].firstIndex);
        assert(semantics[j].count == expected[i].count);
        assert(semantics[j].properties == expected[i].properties);
        assert(semantics[j].width == expected[i].width);
        assert(semantics[j].interfaceKind == expected[i].interfaceKind);
        sprintf(name, "%s%d", expected[i].root, expected[i].firstIndex);
        assert(HlslCanonicalSemantic(profile, name, isOutput) != NULL);
        sprintf(name, "%s%d", expected[i].root,
                expected[i].firstIndex + expected[i].count - 1);
        assert(HlslCanonicalSemantic(profile, name, isOutput) != NULL);
        sprintf(name, "%s%d", expected[i].root,
                expected[i].firstIndex + expected[i].count);
        assert(HlslCanonicalSemantic(profile, name, isOutput) == NULL);
    }
}

static void CheckDescriptorFamilies(void)
{
    static const ExpectedSemantic vertexInput[] = {
        { "POSITION", 0, 1, SEM_IN | SEM_VARYING, 4,
          HLSL_INTERFACE_VARYING },
        { "BLENDWEIGHT", 0, 1, SEM_IN | SEM_VARYING, 4,
          HLSL_INTERFACE_VARYING },
        { "BLENDINDICES", 0, 1, SEM_IN | SEM_VARYING, 4,
          HLSL_INTERFACE_VARYING },
        { "NORMAL", 0, 1, SEM_IN | SEM_VARYING, 3,
          HLSL_INTERFACE_VARYING },
        { "PSIZE", 0, 1, SEM_IN | SEM_VARYING, 1,
          HLSL_INTERFACE_POINT_SIZE },
        { "TEXCOORD", 0, 16, SEM_IN | SEM_VARYING, 4,
          HLSL_INTERFACE_VARYING },
        { "TANGENT", 0, 1, SEM_IN | SEM_VARYING, 3,
          HLSL_INTERFACE_VARYING },
        { "BINORMAL", 0, 1, SEM_IN | SEM_VARYING, 3,
          HLSL_INTERFACE_VARYING },
        { "COLOR", 0, 2, SEM_IN | SEM_VARYING, 4,
          HLSL_INTERFACE_COLOR }
    };
    static const ExpectedSemantic vertexOutput[] = {
        { "POSITION", 0, 1, SEM_OUT | SEM_VARYING | SEM_REQUIRED, 4,
          HLSL_INTERFACE_POSITION },
        { "PSIZE", 0, 1, SEM_OUT | SEM_VARYING, 1,
          HLSL_INTERFACE_POINT_SIZE },
        { "FOG", 0, 1, SEM_OUT | SEM_VARYING, 1,
          HLSL_INTERFACE_VARYING },
        { "COLOR", 0, 2, SEM_OUT | SEM_VARYING, 4,
          HLSL_INTERFACE_COLOR },
        { "TEXCOORD", 0, 8, SEM_OUT | SEM_VARYING, 4,
          HLSL_INTERFACE_VARYING }
    };
    static const ExpectedSemantic pixelInput[] = {
        { "COLOR", 0, 2, SEM_IN | SEM_VARYING, 4,
          HLSL_INTERFACE_COLOR },
        { "TEXCOORD", 0, 8, SEM_IN | SEM_VARYING, 4,
          HLSL_INTERFACE_VARYING },
        { "FOG", 0, 1, SEM_IN | SEM_VARYING, 1,
          HLSL_INTERFACE_VARYING },
        { "VPOS", 0, 1, SEM_IN | SEM_VARYING, 2,
          HLSL_INTERFACE_PIXEL_POSITION },
        { "VFACE", 0, 1, SEM_IN | SEM_VARYING, 1,
          HLSL_INTERFACE_FACE }
    };
    static const ExpectedSemantic pixelOutput[] = {
        { "COLOR", 0, 5, SEM_OUT | SEM_VARYING, 4,
          HLSL_INTERFACE_COLOR },
        { "DEPTH", 0, 1, SEM_OUT | SEM_VARYING, 1,
          HLSL_INTERFACE_DEPTH }
    };

    CheckExpectedSemantics(&HlslProfile_hlslv, vertexInput,
                           NUMELS(vertexInput), 0);
    CheckExpectedSemantics(&HlslProfile_hlslv, vertexOutput,
                           NUMELS(vertexOutput), 1);
    CheckExpectedSemantics(&HlslProfile_hlslf, pixelInput,
                           NUMELS(pixelInput), 0);
    CheckExpectedSemantics(&HlslProfile_hlslf, pixelOutput,
                           NUMELS(pixelOutput), 1);
    assert(HlslProfile_hlslv.numInputAliases == 50);
    assert(HlslProfile_hlslv.numOutputAliases == 11);
    assert(HlslProfile_hlslf.numInputAliases == 12);
    assert(HlslProfile_hlslf.numOutputAliases == 4);
}

static void CheckCanonicalization(void)
{
    HlslProfileDesc invalid;

    CheckDescriptorFamilies();
    CheckSemanticTable(&HlslProfile_hlslv, 0);
    CheckSemanticTable(&HlslProfile_hlslv, 1);
    CheckSemanticTable(&HlslProfile_hlslf, 0);
    CheckSemanticTable(&HlslProfile_hlslf, 1);
    assert(HlslCanonicalSemantic(&HlslProfile_hlslv, "NORMAL0", 1) == NULL);
    assert(HlslCanonicalSemantic(&HlslProfile_hlslv, "VPOS", 0) == NULL);
    assert(HlslCanonicalSemantic(&HlslProfile_hlslf, "DEPTH0", 0) == NULL);
    assert(HlslCanonicalSemantic(&HlslProfile_hlslf, "COLOR4", 1) != NULL);
    assert(HlslCanonicalSemantic(&HlslProfile_hlslf, "COLOR5", 1) == NULL);
    assert(HlslProfile_hlslf.limits->colorOutputs == 4);
    assert(HlslCanonicalSemantic(&HlslProfile_hlslf, "VFACE", 1) == NULL);
    assert(HlslCanonicalSemantic(&HlslProfile_hlslf, "NORMAL0", 0) == NULL);
    assert(HlslCanonicalSemantic(NULL, "POSITION0", 0) == NULL);
    invalid = HlslProfile_hlslv;
    invalid.stage = (HlslStage) 99;
    assert(HlslCanonicalSemantic(&invalid, "POSITION0", 0) == NULL);
    assert(!strcmp(HlslCanonicalSemantic(&HlslProfile_hlslv,
        "COL0", 0), "COLOR0"));
    assert(!strcmp(HlslCanonicalSemantic(&HlslProfile_hlslv,
        "TEX15", 0), "TEXCOORD15"));
    assert(!strcmp(HlslCanonicalSemantic(&HlslProfile_hlslv,
        "ATTR15", 0), "TEXCOORD15"));
    assert(!strcmp(HlslCanonicalSemantic(&HlslProfile_hlslv,
        "ATTRIB0", 0), "TEXCOORD0"));
    assert(!strcmp(HlslCanonicalSemantic(&HlslProfile_hlslv,
        "ATTRIB15", 0), "TEXCOORD15"));
    assert(HlslCanonicalSemantic(&HlslProfile_hlslv,
        "ATTRIB16", 0) == NULL);
    assert(HlslCanonicalSemantic(&HlslProfile_hlslv,
        "ATTRIB0", 1) == NULL);
    assert(!strcmp(HlslCanonicalSemantic(&HlslProfile_hlslv,
        "HPOS", 1), "POSITION0"));
    assert(!strcmp(HlslCanonicalSemantic(&HlslProfile_hlslv,
        "TEX7", 1), "TEXCOORD7"));
    assert(!strcmp(HlslCanonicalSemantic(&HlslProfile_hlslf,
        "WPOS", 0), "VPOS"));
    assert(!strcmp(HlslCanonicalSemantic(&HlslProfile_hlslf,
        "FACE", 0), "VFACE"));
    assert(!strcmp(HlslCanonicalSemantic(&HlslProfile_hlslf,
        "TEX7", 0), "TEXCOORD7"));
    assert(!strcmp(HlslCanonicalSemantic(&HlslProfile_hlslf,
        "COL3", 1), "COLOR3"));
}

static void CheckConnector(slHAL *hal, ConnectorDescriptor *connector)
{
    Binding binding;
    int i;

    assert(connector->name != 0);
    assert(hal->GetConnectorID(connector->name) == connector->cid);
    assert(hal->GetConnectorAtom(connector->cid) == connector->name);
    assert(hal->GetConnectorUses(connector->cid, hal->pid) ==
           connector->properties);
    assert(hal->GetConnectorRegister(connector->cid, 1, -1, NULL) ==
           connector->numregs);
    for (i = 0; i < connector->numregs; i++) {
        assert(connector->registers[i].name != 0);
        memset(&binding, 0, sizeof(binding));
        assert(hal->GetConnectorRegister(connector->cid, 1, i, &binding));
        assert(binding.conn.rname == connector->registers[i].name);
        assert(binding.conn.regno == connector->registers[i].regno);
        memset(&binding, 0, sizeof(binding));
        assert(hal->GetConnectorRegister(connector->cid, 0,
               connector->registers[i].name, &binding));
        assert(binding.conn.rname == connector->registers[i].name);
    }
    assert(!hal->GetConnectorRegister(connector->cid, 1,
                                      connector->numregs, &binding));
    assert(!hal->GetConnectorRegister(connector->cid, 0,
           AddAtom(atable, "NOT_A_REGISTER"), &binding));
    assert(!hal->GetConnectorRegister(connector->cid, 1, 0, NULL));
}

static void CheckConnectors(void)
{
    slHAL hal;
    const HlslProfileDesc *profile;
    int i;

    profile = InitStage(&hal, 1);
    Require(hal.GetCapsBit(CAPS_CANONICAL_OUTPUT_SEMANTIC_CONFLICTS),
            "HLSL did not advertise canonical output conflict ownership");
    Require(hal.GetCapsBit(CAPS_ENTRY_INOUT_PARAMETERS),
            "HLSL did not advertise entry inout support");
    Require(hal.GetCapsBit(CAPS_PRESERVE_TERMINAL_ENTRY_RETURN),
            "HLSL did not advertise native terminal entry returns");
    Require(hal.GetCapsBit(CAPS_AGGREGATE_DEFAULT_INITIALIZERS),
            "HLSL did not advertise aggregate default initializers");
    Require(hal.GetCapsBit(
                CAPS_PRESERVE_SIDE_EFFECTING_AGGREGATE_TEMPS),
            "HLSL did not preserve side-effecting aggregate temporaries");
    assert(profile->numInputRegs == 25);
    assert(profile->numOutputRegs == 13);
    for (i = 0; i < profile->numConnectors; i++)
        CheckConnector(&hal, &profile->connectors[i]);
    assert(!hal.GetConnectorRegister(9999, 1, -1, NULL));
    FreeStage(&hal);

    profile = InitStage(&hal, 0);
    assert(profile->numInputRegs == 13);
    assert(profile->numOutputRegs == 6);
    for (i = 0; i < profile->numConnectors; i++)
        CheckConnector(&hal, &profile->connectors[i]);
    FreeStage(&hal);
}

static int BindSemantic(slHAL *hal, Symbol *symbol, Binding *binding,
                        const char *semantic, int isOutput)
{
    return hal->BindVaryingSemantic(&symbol->loc, symbol,
           AddAtom(atable, semantic), binding, isOutput);
}

static void CheckOneBinding(int vertex, const char *semantic, int isOutput,
                            int base, int len, int vector, int success,
                            int expectedProperties)
{
    slHAL hal;
    const HlslProfileDesc *profile;
    ConnectorRegisters *registers;
    const char *canonical;
    Symbol symbol;
    Binding binding;
    Type type, element;

    int registerCount, i;

    profile = InitStage(&hal, vertex);
    memset(&symbol, 0, sizeof(symbol));
    memset(&binding, 0, sizeof(binding));
    binding.none.gname = AddAtom(atable, "public_interface");
    if (vector)
        MakeVector(&type, &element, base, len);
    else
        MakeScalar(&type, base);
    symbol.name = AddAtom(atable, "value");
    symbol.type = &type;
    symbol.loc.line = 17;
    semanticErrorCount = 0;
    assert(BindSemantic(&hal, &symbol, &binding, semantic, isOutput) ==
           success);
    if (success) {
        assert(semanticErrorCount == 0);
        assert(binding.conn.kind == BK_CONNECTOR);
        assert((binding.conn.properties & expectedProperties) ==
               expectedProperties);
        canonical = HlslCanonicalSemantic(profile, semantic, isOutput);
        assert(canonical != NULL);
        assert(!strcmp(GetAtomString(atable, binding.conn.rname), canonical));
        if (isOutput) {
            registers = profile->outputRegs;
            registerCount = profile->numOutputRegs;
        } else {
            registers = profile->inputRegs;
            registerCount = profile->numInputRegs;
        }
        for (i = 0; i < registerCount; i++) {
            if (!strcmp(registers[i].sname, canonical))
                break;
        }
        assert(i < registerCount);
        assert(binding.conn.regno == registers[i].regno);
        assert(binding.conn.base == registers[i].base);
        assert(binding.conn.size == registers[i].size);
    } else {
        assert(semanticErrorCount == 1);
        assert(lastSemanticError == 6403);
        assert(binding.none.kind == BK_NONE);
        assert(binding.none.properties == 0);
        assert(lastSemanticLoc.line == 17);
    }
    FreeStage(&hal);
}

static void CheckBindingTypes(void)
{
    int inputProperties, outputProperties;

    inputProperties = BIND_IS_BOUND | BIND_VARYING | BIND_INPUT;
    outputProperties = BIND_IS_BOUND | BIND_VARYING | BIND_OUTPUT;
    CheckOneBinding(1, "NORMAL0", 0, TYPE_BASE_FLOAT, 3, 1, 1,
                    inputProperties);
    CheckOneBinding(1, "NORMAL0", 0, TYPE_BASE_FLOAT, 4, 1, 0, 0);
    CheckOneBinding(1, "NORMAL0", 1, TYPE_BASE_FLOAT, 3, 1, 0, 0);
    CheckOneBinding(1, "TEXCOORD0", 0, TYPE_BASE_FLOAT, 1, 0, 1,
                    inputProperties);
    CheckOneBinding(1, "TEXCOORD0", 0, TYPE_BASE_FLOAT, 5, 1, 0, 0);
    CheckOneBinding(1, "ATTRIB0", 0, TYPE_BASE_FLOAT, 4, 1, 1,
                    inputProperties);
    CheckOneBinding(1, "TEXCOORD0", 0, TYPE_BASE_INT, 4, 1, 0, 0);
    CheckOneBinding(1, "POSITION0", 1, TYPE_BASE_FLOAT, 4, 1, 1,
                    outputProperties | BIND_WRITE_REQUIRED);
    CheckOneBinding(1, "POSITION0", 1, TYPE_BASE_FLOAT, 3, 1, 0, 0);
    CheckOneBinding(1, "PSIZE0", 1, TYPE_BASE_FLOAT, 1, 0, 1,
                    outputProperties);
    CheckOneBinding(1, "PSIZE0", 1, TYPE_BASE_FLOAT, 1, 1, 0, 0);
    CheckOneBinding(0, "VPOS", 0, TYPE_BASE_FLOAT, 2, 1, 1,
                    inputProperties);
    CheckOneBinding(0, "VPOS", 0, TYPE_BASE_FLOAT, 3, 1, 0, 0);
    CheckOneBinding(0, "VFACE", 0, TYPE_BASE_FLOAT, 1, 0, 1,
                    inputProperties);
    CheckOneBinding(0, "VFACE", 0, TYPE_BASE_BOOLEAN, 1, 0, 0, 0);
    CheckOneBinding(0, "COLOR0", 1, TYPE_BASE_FLOAT, 3, 1, 0, 0);
    CheckOneBinding(0, "DEPTH0", 1, TYPE_BASE_FLOAT, 1, 0, 1,
                    outputProperties);
    CheckOneBinding(0, "DEPTH0", 1, TYPE_BASE_FLOAT, 1, 1, 0, 0);
    CheckOneBinding(0, "DEPTH0", 0, TYPE_BASE_FLOAT, 1, 0, 0, 0);
    CheckOneBinding(0, "NORMAL0", 0, TYPE_BASE_FLOAT, 3, 1, 0, 0);
}

static void InitOwnershipTest(slHAL *hal, Type *type, Type *element,
                              Symbol *first, Symbol *second)
{
    InitStage(hal, 1);
    MakeVector(type, element, TYPE_BASE_FLOAT, 4);
    memset(first, 0, sizeof(*first));
    memset(second, 0, sizeof(*second));
    first->name = AddAtom(atable, "first");
    first->type = type;
    first->loc.line = 11;
    second->name = AddAtom(atable, "second");
    second->type = type;
    second->loc.line = 12;
    semanticErrorCount = 0;
}

static void CheckSameOwnerSameSlot(void)
{
    slHAL hal;
    Symbol first, second;
    Binding probe, realBinding;
    Type type, element;

    InitOwnershipTest(&hal, &type, &element, &first, &second);
    memset(&probe, 0, sizeof(probe));
    memset(&realBinding, 0, sizeof(realBinding));
    Require(BindSemantic(&hal, &first, &probe, "ATTR0", 0),
            "initial same-owner bind failed");
    Require(BindSemantic(&hal, &first, &realBinding, "TEXCOORD0", 0),
            "same symbol and slot were not idempotent");
    Require(semanticErrorCount == 0,
            "idempotent bind emitted a diagnostic");
    Require(realBinding.conn.kind == BK_CONNECTOR,
            "idempotent bind did not populate the new binding");
    FreeStage(&hal);
}

static void CheckSameOwnerDifferentSlot(void)
{
    slHAL hal;
    Symbol first, second;
    Binding original, rejected, available;
    Type type, element;

    InitOwnershipTest(&hal, &type, &element, &first, &second);
    memset(&original, 0, sizeof(original));
    memset(&rejected, 0, sizeof(rejected));
    memset(&available, 0, sizeof(available));
    Require(BindSemantic(&hal, &first, &original, "TEXCOORD0", 0),
            "initial different-slot bind failed");
    Require(!BindSemantic(&hal, &first, &rejected, "TEXCOORD1", 0),
            "same symbol claimed a second slot");
    Require(semanticErrorCount == 1 &&
            lastSemanticError == 6404,
            "different-slot rejection did not emit one C6404");
    Require(rejected.none.kind == BK_NONE && rejected.none.properties == 0,
            "different-slot rejection mutated its binding");
    Require(BindSemantic(&hal, &second, &available, "TEXCOORD1", 0),
            "rejected different-slot attempt left stale ownership");
    FreeStage(&hal);
}

static void CheckSameOwnerDifferentDirection(void)
{
    slHAL hal;
    Symbol first, second;
    Binding input, output, available;
    Type type, element;

    InitOwnershipTest(&hal, &type, &element, &first, &second);
    memset(&input, 0, sizeof(input));
    memset(&output, 0, sizeof(output));
    memset(&available, 0, sizeof(available));
    Require(BindSemantic(&hal, &first, &input, "COLOR0", 0),
            "initial different-direction bind failed");
    Require(!BindSemantic(&hal, &first, &output, "COLOR0", 1),
            "same symbol claimed both directions");
    Require(semanticErrorCount == 1 &&
            lastSemanticError == 6404,
            "different-direction rejection did not emit one C6404");
    Require(output.none.kind == BK_NONE && output.none.properties == 0,
            "different-direction rejection mutated its binding");
    Require(BindSemantic(&hal, &second, &available, "COLOR0", 1),
            "rejected different-direction attempt left stale ownership");
    FreeStage(&hal);
}

static void CheckDifferentOwnerSameSlot(void)
{
    slHAL hal;
    Symbol first, second;
    Binding shared, snapshot;
    Type type, element;

    InitOwnershipTest(&hal, &type, &element, &first, &second);
    memset(&shared, 0, sizeof(shared));
    Require(BindSemantic(&hal, &first, &shared, "TEXCOORD0", 0),
            "initial different-owner bind failed");
    snapshot = shared;
    Require(!BindSemantic(&hal, &second, &shared, "TEXCOORD0", 0),
            "binding-address reuse hid a different-symbol collision");
    Require(semanticErrorCount == 1 &&
            lastSemanticError == 6404,
            "different-owner rejection did not emit one C6404");
    Require(!memcmp(&shared, &snapshot, sizeof(shared)),
            "different-owner rejection mutated the existing binding");
    Require(lastSemanticLoc.line == 12,
            "different-owner rejection used the wrong source location");
    FreeStage(&hal);
}

static void CheckFreshHALIsolation(void)
{
    slHAL firstHal, secondHal;
    Symbol first, second;
    Binding binding, conflict;
    Type type, element;

    InitOwnershipTest(&firstHal, &type, &element, &first, &second);
    memset(&binding, 0, sizeof(binding));
    memset(&conflict, 0, sizeof(conflict));
    assert(BindSemantic(&firstHal, &first, &binding, "TEXCOORD0", 0));
    assert(!BindSemantic(&firstHal, &second, &conflict, "TEXCOORD0", 0));
    assert(semanticErrorCount == 1 && lastSemanticError == 6404);
    FreeStage(&firstHal);

    InitOwnershipTest(&secondHal, &type, &element, &first, &second);
    memset(&binding, 0, sizeof(binding));
    assert(BindSemantic(&secondHal, &second, &binding, "TEXCOORD0", 0));
    assert(semanticErrorCount == 0);
    FreeStage(&secondHal);
}

static void CheckUnbound(void)
{
    slHAL hal;
    Symbol symbol;
    Binding binding;

    InitStage(&hal, 1);
    memset(&symbol, 0, sizeof(symbol));
    memset(&binding, 0, sizeof(binding));
    symbol.name = AddAtom(atable, "missingSemantic");
    symbol.loc.line = 29;
    semanticErrorCount = 0;
    assert(!hal.BindVaryingUnbound(&symbol.loc, &symbol, symbol.name, 0,
                                   &binding, 0));
    assert(semanticErrorCount == 1);
    assert(lastSemanticError == 6403);
    assert(lastSemanticLoc.line == 29);
    FreeStage(&hal);
}

static void CheckUniformUnbound(void)
{
    slHAL hal;
    Symbol symbol;
    Binding binding;
    Type type;

    InitStage(&hal, 1);
    MakeScalar(&type, TYPE_BASE_FLOAT);
    memset(&symbol, 0, sizeof(symbol));
    memset(&binding, 0, sizeof(binding));
    symbol.name = AddAtom(atable, "lateUniform");
    symbol.type = &type;
    symbol.loc.line = 37;
    assert(hal.BindUniformUnbound(&symbol.loc, &symbol, &binding));
    assert((binding.none.properties & (BIND_IS_BOUND | BIND_UNIFORM)) ==
           (BIND_IS_BOUND | BIND_UNIFORM));
    assert(binding.none.kind == BK_NONE);
    assert(binding.reg.rname == 0 && binding.reg.regno == 0 &&
           binding.reg.count == 0 && binding.texunit.unitno == 0);
    FreeStage(&hal);
}

static void CheckInternalFunctions(void)
{
    slHAL hal;
    Symbol symbol;
    Type functionType;
    Type scalarResult;
    Type vectorResult;
    Type resultElement;
    Type scalarLeft;
    Type scalarRight;
    Type vectorLeft;
    Type vectorRight;
    Type leftElement;
    Type rightElement;
    TypeList first;
    TypeList second;
    int group;

    InitStage(&hal, 1);
    memset(&symbol, 0, sizeof(symbol));
    memset(&functionType, 0, sizeof(functionType));
    MakeScalar(&scalarResult, TYPE_BASE_FLOAT);
    MakeVector(&vectorResult, &resultElement, TYPE_BASE_FLOAT, 1);
    MakeScalar(&scalarLeft, TYPE_BASE_FLOAT);
    MakeScalar(&scalarRight, TYPE_BASE_FLOAT);
    MakeVector(&vectorLeft, &leftElement, TYPE_BASE_FLOAT, 1);
    MakeVector(&vectorRight, &rightElement, TYPE_BASE_FLOAT, 1);
    first.next = &second;
    second.next = NULL;
    functionType.fun.properties = TYPE_CATEGORY_FUNCTION;
    functionType.fun.paramtypes = &first;
    symbol.kind = FUNCTION_S;
    symbol.type = &functionType;

    symbol.name = AddAtom(atable, "dot");
    functionType.fun.rettype = &scalarResult;
    first.type = &scalarLeft;
    second.type = &scalarRight;
    semanticErrorCount = 0;
    group = 0;
    Require(hal.CheckInternalFunction(&symbol, &group) == HLSL_BUILTIN_DOT &&
            group == HLSL_BUILTIN_GROUP && semanticErrorCount == 0,
            "scalar dot source signature was not recognized");

    first.type = &vectorLeft;
    second.type = &vectorRight;
    semanticErrorCount = 0;
    group = 0;
    Require(hal.CheckInternalFunction(&symbol, &group) == HLSL_BUILTIN_DOT &&
            group == HLSL_BUILTIN_GROUP && semanticErrorCount == 0,
            "declared float1 dot source signature was not recognized");

    first.type = &scalarLeft;
    second.type = &vectorRight;
    semanticErrorCount = 0;
    group = 0;
    Require(hal.CheckInternalFunction(&symbol, &group) == 0 && group == 0 &&
            semanticErrorCount == 1 && lastSemanticError == 6410,
            "mixed scalar/float1 dot signature was accepted");

    functionType.fun.rettype = &vectorResult;
    first.type = &vectorLeft;
    semanticErrorCount = 0;
    group = 0;
    Require(hal.CheckInternalFunction(&symbol, &group) == 0 && group == 0 &&
            semanticErrorCount == 1 && lastSemanticError == 6410,
            "float1 dot result was accepted instead of scalar result");

    symbol.name = AddAtom(atable, "mul");
    functionType.fun.rettype = &scalarResult;
    first.type = &scalarLeft;
    second.type = &scalarRight;
    SetTestScalarKind(&scalarResult, CG_SCALAR_HALF);
    SetTestScalarKind(&scalarLeft, CG_SCALAR_HALF);
    SetTestScalarKind(&scalarRight, CG_SCALAR_HALF);
    semanticErrorCount = 0;
    group = 0;
    Require(hal.CheckInternalFunction(&symbol, &group) == 0 && group == 0 &&
            semanticErrorCount == 1 && lastSemanticError == 6410,
            "half mul signature absent from the catalog was accepted");

    symbol.name = AddAtom(atable, "cross");
    functionType.fun.rettype = &vectorResult;
    MakeVector(&vectorResult, &resultElement, TYPE_BASE_FLOAT, 3);
    MakeVector(&vectorLeft, &leftElement, TYPE_BASE_FLOAT, 3);
    MakeVector(&vectorRight, &rightElement, TYPE_BASE_FLOAT, 3);
    SetTestScalarKind(&vectorResult, CG_SCALAR_FIXED);
    SetTestScalarKind(&vectorLeft, CG_SCALAR_FIXED);
    SetTestScalarKind(&vectorRight, CG_SCALAR_FIXED);
    first.type = &vectorLeft;
    second.type = &vectorRight;
    semanticErrorCount = 0;
    group = 0;
    Require(hal.CheckInternalFunction(&symbol, &group) == 0 && group == 0 &&
            semanticErrorCount == 1 && lastSemanticError == 6410,
            "fixed3 cross signature absent from the catalog was accepted");

    symbol.name = AddAtom(atable, "dot");
    functionType.fun.rettype = &scalarResult;
    MakeScalar(&scalarResult, TYPE_BASE_FLOAT);
    MakeVector(&vectorLeft, &leftElement, TYPE_BASE_FLOAT, 1);
    MakeVector(&vectorRight, &rightElement, TYPE_BASE_FLOAT, 1);
    SetTestScalarKind(&scalarResult, CG_SCALAR_HALF);
    SetTestScalarKind(&vectorLeft, CG_SCALAR_HALF);
    SetTestScalarKind(&vectorRight, CG_SCALAR_HALF);
    first.type = &vectorLeft;
    second.type = &vectorRight;
    semanticErrorCount = 0;
    group = 0;
    Require(hal.CheckInternalFunction(&symbol, &group) == HLSL_BUILTIN_DOT &&
            group == HLSL_BUILTIN_GROUP && semanticErrorCount == 0,
            "catalog-declared half1 dot signature was not recognized");

    MakeScalar(&scalarResult, TYPE_BASE_FLOAT);
    MakeScalar(&scalarLeft, TYPE_BASE_FLOAT);
    MakeScalar(&scalarRight, TYPE_BASE_FLOAT);
    SetTestScalarKind(&scalarResult, CG_SCALAR_FIXED);
    SetTestScalarKind(&scalarLeft, CG_SCALAR_FIXED);
    SetTestScalarKind(&scalarRight, CG_SCALAR_FIXED);
    functionType.fun.rettype = &scalarResult;
    functionType.fun.paramtypes = &first;
    first.type = &scalarLeft;
    first.next = &second;
    second.type = &scalarRight;
    semanticErrorCount = 0;
    group = 0;
    Require(hal.CheckInternalFunction(&symbol, &group) == HLSL_BUILTIN_DOT &&
            group == HLSL_BUILTIN_GROUP && semanticErrorCount == 0,
            "catalog-declared fixed dot signature was not recognized");

    symbol.name = AddAtom(atable, "abs");
    MakeScalar(&scalarResult, TYPE_BASE_INT);
    MakeScalar(&scalarLeft, TYPE_BASE_INT);
    SetTestScalarKind(&scalarResult, CG_SCALAR_INT);
    SetTestScalarKind(&scalarLeft, CG_SCALAR_INT);
    functionType.fun.rettype = &scalarResult;
    functionType.fun.paramtypes = &first;
    first.type = &scalarLeft;
    first.next = NULL;
    semanticErrorCount = 0;
    group = 0;
    Require(hal.CheckInternalFunction(&symbol, &group) == HLSL_BUILTIN_ABS &&
            group == HLSL_BUILTIN_GROUP && semanticErrorCount == 0,
            "catalog-declared int abs signature was not recognized");

    symbol.name = AddAtom(atable, "any");
    MakeScalar(&scalarResult, TYPE_BASE_BOOLEAN);
    MakeScalar(&scalarLeft, TYPE_BASE_BOOLEAN);
    SetTestScalarKind(&scalarResult, CG_SCALAR_BOOL);
    SetTestScalarKind(&scalarLeft, CG_SCALAR_BOOL);
    functionType.fun.rettype = &scalarResult;
    first.type = &scalarLeft;
    semanticErrorCount = 0;
    group = 0;
    Require(hal.CheckInternalFunction(&symbol, &group) == HLSL_BUILTIN_ANY &&
            group == HLSL_BUILTIN_GROUP && semanticErrorCount == 0,
            "catalog-declared bool any signature was not recognized");

    RequireRejectedIntrinsicKinds(&hal, &symbol, &functionType, &first,
        &scalarResult, &scalarLeft, "abs", CG_SCALAR_UINT, CG_SCALAR_INT,
        "uint intrinsic result collapsed to legacy int");
    RequireRejectedIntrinsicKinds(&hal, &symbol, &functionType, &first,
        &scalarResult, &scalarLeft, "abs", CG_SCALAR_INT, CG_SCALAR_UINT,
        "uint intrinsic parameter collapsed to legacy int");
    RequireRejectedIntrinsicKinds(&hal, &symbol, &functionType, &first,
        &scalarResult, &scalarLeft, "sqrt",
        CG_SCALAR_DOUBLE, CG_SCALAR_FLOAT,
        "double intrinsic result collapsed to legacy float");
    RequireRejectedIntrinsicKinds(&hal, &symbol, &functionType, &first,
        &scalarResult, &scalarLeft, "sqrt",
        CG_SCALAR_FLOAT, CG_SCALAR_DOUBLE,
        "double intrinsic parameter collapsed to legacy float");
    RequireRejectedIntrinsicKinds(&hal, &symbol, &functionType, &first,
        &scalarResult, &scalarLeft, "abs", CG_SCALAR_CHAR, CG_SCALAR_CHAR,
        "char intrinsic signature collapsed to legacy int");
    RequireRejectedIntrinsicKinds(&hal, &symbol, &functionType, &first,
        &scalarResult, &scalarLeft, "abs", CG_SCALAR_SHORT,
        CG_SCALAR_SHORT,
        "short intrinsic signature collapsed to legacy int");
    RequireRejectedIntrinsicKinds(&hal, &symbol, &functionType, &first,
        &scalarResult, &scalarLeft, "abs", CG_SCALAR_LONG, CG_SCALAR_LONG,
        "long intrinsic signature collapsed to legacy int");
    FreeStage(&hal);
}

static void CheckTextureParameterErrorHandling(void)
{
    slHAL hal;
    Symbol symbol;
    CgIntrinsicSignature signature;
    SourceLoc loc;
    Type functionType;

    InitStage(&hal, 0);
    memset(&symbol, 0, sizeof(symbol));
    memset(&signature, 0, sizeof(signature));
    memset(&loc, 0, sizeof(loc));
    memset(&functionType, 0, sizeof(functionType));
    loc.line = 61;
    signature.name = "tex2D";
    signature.flags = CG_INTRINSIC_TEXTURE;
    symbol.details.fun.intrinsic = &signature;
    semanticErrorCount = 0;
    Require(hal.HandleParameterTypeError != NULL &&
            hal.HandleParameterTypeError(&loc, &symbol, 2) &&
            semanticErrorCount == 1 && lastSemanticError == 6409 &&
            lastSemanticLoc.line == 61,
            "HLSL did not handle catalog texture parameter diagnostics");
    signature.flags = CG_INTRINSIC_PURE;
    semanticErrorCount = 0;
    Require(!hal.HandleParameterTypeError(&loc, &symbol, 2) &&
            semanticErrorCount == 0,
            "HLSL handled a nontexture intrinsic parameter diagnostic");
    symbol.details.fun.intrinsic = NULL;
    Require(!hal.HandleParameterTypeError(&loc, &symbol, 2) &&
            semanticErrorCount == 0,
            "HLSL handled a user function parameter diagnostic");

    functionType.properties = TYPE_CATEGORY_FUNCTION | TYPE_MISC_INTERNAL;
    symbol.name = AddAtom(atable, "tex2Dbias");
    symbol.type = &functionType;
    symbol.kind = FUNCTION_S;
    symbol.properties = SYMB_IS_BUILTIN | SYMB_IS_DEFINED;
    symbol.details.fun.group = HLSL_BUILTIN_GROUP;
    symbol.details.fun.index = HLSL_BUILTIN_TEX2DBIAS;
    semanticErrorCount = 0;
    Require(hal.HandleParameterTypeError(&loc, &symbol, 2) &&
            semanticErrorCount == 1 && lastSemanticError == 6409,
            "HLSL did not handle a standard-library texture marker");
    symbol.name = AddAtom(atable, "tex2Dlod");
    symbol.details.fun.index = HLSL_BUILTIN_TEX2DLOD;
    semanticErrorCount = 0;
    Require(hal.HandleParameterTypeError(&loc, &symbol, 2) &&
            semanticErrorCount == 1 && lastSemanticError == 6409,
            "HLSL did not handle a vertex standard-library texture marker");
    symbol.name = AddAtom(atable, "tex2Dbias");
    semanticErrorCount = 0;
    Require(!hal.HandleParameterTypeError(&loc, &symbol, 2) &&
            semanticErrorCount == 0,
            "HLSL handled a mismatched texture marker identity");
    symbol.properties = 0;
    semanticErrorCount = 0;
    Require(!hal.HandleParameterTypeError(&loc, &symbol, 2) &&
            semanticErrorCount == 0,
            "HLSL handled a user function with a texture marker name");
    symbol.name = AddAtom(atable, "dot");
    symbol.properties = SYMB_IS_BUILTIN | SYMB_IS_DEFINED;
    symbol.details.fun.index = HLSL_BUILTIN_DOT;
    Require(!hal.HandleParameterTypeError(&loc, &symbol, 2) &&
            semanticErrorCount == 0,
            "HLSL handled a nontexture builtin marker");
    FreeStage(&hal);
}

int main(int argc, char **argv)
{
    if (argc == 2 && !strcmp(argv[1], "--verify-assertions-active")) {
        int assertionsActive;

        assertionsActive = 0;
        assert((assertionsActive = 1) != 0);
        if (!assertionsActive)
            return 2;
        puts("glsl-ir-assertions-active");
        return 0;
    }
    memset(&testCg, 0, sizeof(testCg));
    assert(InitAtomTable(atable, 0));
    if (argc == 2 && !strcmp(argv[1], "same-slot"))
        CheckSameOwnerSameSlot();
    else if (argc == 2 && !strcmp(argv[1], "different-slot"))
        CheckSameOwnerDifferentSlot();
    else if (argc == 2 && !strcmp(argv[1], "different-direction"))
        CheckSameOwnerDifferentDirection();
    else if (argc == 2 && !strcmp(argv[1], "different-owner"))
        CheckDifferentOwnerSameSlot();
    else if (argc == 2 && !strcmp(argv[1], "fresh-hal"))
        CheckFreshHALIsolation();
    else {
        CheckParser();
        CheckCanonicalization();
        CheckConnectors();
        CheckBindingTypes();
        CheckUnbound();
        CheckUniformUnbound();
        CheckInternalFunctions();
        CheckTextureParameterErrorHandling();
    }
    FreeAtomTable(atable);
    return 0;
}

void *mem_Calloc(MemoryPool *pool, size_t size, int count)
{
    (void) pool;
    return calloc((size_t) count, size);
}

void *mem_Alloc(MemoryPool *pool, size_t size)
{
    (void) pool;
    return calloc(1, size);
}

slProfile *RegisterProfile(int (*InitHALFunc)(slHAL *), const char *name,
                           int id)
{
    (void) InitHALFunc;
    (void) name;
    (void) id;
    return NULL;
}

void SetProfileIdentity(const char *name, CgProfileStage stage,
                        const char *wildcardName, int wildcardSpecificity)
{
    (void) name;
    (void) stage;
    (void) wildcardName;
    (void) wildcardSpecificity;
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
    binding->conn.kind = BK_CONNECTOR;
    binding->conn.properties = BIND_IS_BOUND;
    binding->conn.base = connector->base;
    binding->conn.size = connector->size;
    binding->conn.rname = connector->name;
    binding->conn.regno = connector->regno;
}

int IsScalar(const Type *type)
{
    return type != NULL &&
           (type->properties & TYPE_CATEGORY_MASK) == TYPE_CATEGORY_SCALAR;
}

int IsVector(const Type *type, int *len)
{
    if (type != NULL &&
        (type->properties & TYPE_CATEGORY_MASK) == TYPE_CATEGORY_ARRAY &&
        (type->properties & TYPE_MISC_PACKED) != 0 &&
        type->arr.eltype != NULL &&
        (type->arr.eltype->properties & TYPE_CATEGORY_MASK) !=
        TYPE_CATEGORY_ARRAY)
    {
        if (len != NULL)
            *len = type->arr.numels;
        return 1;
    }
    return 0;
}

int IsMatrix(const Type *type, int *rows, int *cols)
{
    int rowLength;

    if (type != NULL &&
        (type->properties & TYPE_CATEGORY_MASK) == TYPE_CATEGORY_ARRAY &&
        (type->properties & TYPE_MISC_PACKED) != 0 &&
        IsVector(type->arr.eltype, &rowLength))
    {
        if (rows != NULL)
            *rows = rowLength;
        if (cols != NULL)
            *cols = type->arr.numels;
        return 1;
    }
    return 0;
}

int IsVoid(const Type *type)
{
    return type != NULL && (type->properties & TYPE_MISC_VOID) != 0;
}

int GetCategory(const Type *type)
{
    return type != NULL ?
           type->properties & TYPE_CATEGORY_MASK : TYPE_CATEGORY_NONE;
}

int GetBase(const Type *type)
{
    return type != NULL ?
           type->properties & TYPE_BASE_MASK : TYPE_BASE_NO_TYPE;
}

void SemanticError(SourceLoc *loc, int number, const char *message, ...)
{
    (void) message;
    semanticErrorCount++;
    lastSemanticError = number;
    if (loc != NULL)
        lastSemanticLoc = *loc;
    else
        memset(&lastSemanticLoc, 0, sizeof(lastSemanticLoc));
}

void InternalError(SourceLoc *loc, int number, const char *message, ...)
{
    SemanticError(loc, number, message);
}

void FatalError(const char *message, ...)
{
    (void) message;
    abort();
}

int HlslWriteModule(FILE *out, const HlslModule *module,
                    const HlslProfileDesc *profile)
{
    (void) out;
    (void) module;
    (void) profile;
    return 1;
}

int HlslLowerProgram(HlslModule *module, const HlslProfileDesc *profile,
                     SourceLoc *loc, Scope *scope, Symbol *program)
{
    (void) module;
    (void) profile;
    (void) loc;
    (void) scope;
    (void) program;
    return 0;
}

int HlslLegalizeModule(HlslModule *module,
                       const HlslProfileDesc *profile)
{
    (void) module;
    (void) profile;
    return 0;
}

int HlslValidateSamplerUsage(HlslModule *module,
                             const HlslProfileDesc *profile)
{
    (void) module;
    (void) profile;
    return 1;
}

int HlslValidateModule(HlslModule *module,
                       const HlslProfileDesc *profile)
{
    (void) module;
    (void) profile;
    return 0;
}

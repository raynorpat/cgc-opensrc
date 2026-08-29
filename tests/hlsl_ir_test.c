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
// hlsl_ir_test.c
//

#if defined(NDEBUG)
#undef NDEBUG
#endif
#include <assert.h>
#include <float.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "hlsl_ir.h"
#include "slglobals.h"
#include "hlsl_hal.h"

#undef malloc
#undef calloc

static void *TestAlloc(void *arg, size_t size)
{
    (void) arg;
    return calloc(1, size);
}

static void *DirtyAlloc(void *arg, size_t size)
{
    void *memory;

    (void) arg;
    memory = malloc(size);
    if (memory != NULL)
        memset(memory, 0xa5, size);
    return memory;
}

static void *FaultAlloc(void *arg, size_t size)
{
    (void) arg;
    (void) size;
    return NULL;
}

static void *InitializerFaultAlloc(void *arg, size_t size)
{
    (void) arg;
    if (size == sizeof(HlslExpr))
        return NULL;
    return calloc(1, size);
}

typedef struct LimitedAllocState_Rec {
    int calls;
    int limit;
} LimitedAllocState;

static void *LimitedAlloc(void *arg, size_t size)
{
    LimitedAllocState *state;

    state = (LimitedAllocState *) arg;
    state->calls++;
    if (state->calls > state->limit)
        return NULL;
    return calloc(1, size);
}

static void TestReservedNames(void)
{
    static const char *explicitReservedNames[] = {
        "AttribArray", "LINE", "LINE_ADJ", "LINE_OUT", "POINT",
        "POINT_OUT", "PixelShader", "TRIANGLE", "TRIANGLE_ADJ",
        "TRIANGLE_OUT", "VertexShader", "__internal", "__packed", "asm",
        "asm_fragment", "attribute", "auto", "bool", "break", "case",
        "cast", "catch", "const_cast", "centroid", "char", "class",
        "column_major", "compile", "compile_fragment", "const", "continue",
        "default", "discard", "decl", "delete", "do", "double", "dword",
        "dynamic_cast", "else", "emit", "enum", "explicit", "export",
        "extern", "external", "false", "fixed", "float", "for", "foreach",
        "friend", "get", "goto", "half", "if", "in", "inline", "inout",
        "input", "int", "interface", "invariant", "is", "long", "main",
        "matrix", "mutable", "namespace", "new", "noinline", "null",
        "nointerpolation", "operator", "out", "output", "packoffset",
        "packed", "pass", "pixelfragment", "pixelshader", "precise",
        "private", "protected", "public", "ref", "register",
        "reinterpret_cast", "return", "row_major", "sampler", "sampler1D",
        "sampler2D", "sampler3D", "samplerCUBE", "samplerCube",
        "samplerRECT", "sampler_state", "set", "shared", "short", "signed",
        "sizeof", "snorm", "stateblock", "stateblock_state", "static",
        "static_cast", "string", "struct", "switch", "technique",
        "technique10", "technique11", "template", "texture", "texture1D",
        "texture2D", "texture3D", "textureCUBE", "textureCube",
        "textureRECT", "this", "throw", "true", "try", "typedef",
        "typename", "uchar", "uint", "ulong", "uniform", "union", "unorm",
        "unsigned", "ushort", "using", "varying", "vector",
        "vertexfragment", "vertexshader", "virtual", "void", "volatile",
        "while", "yield",
        "AppendStructuredBuffer", "BlendState", "Buffer",
        "ByteAddressBuffer", "cbuffer", "CompileShader", "ComputeShader",
        "ConsumeStructuredBuffer", "DepthStencilState", "DepthStencilView",
        "DomainShader", "fxgroup", "GeometryShader", "groupshared",
        "Hullshader", "HullShader", "InputPatch", "line", "lineadj",
        "linear", "LineStream", "min16float", "min10float", "min16int",
        "min12int", "min16uint", "noperspective", "NULL", "OutputPatch",
        "point", "PointStream", "RasterizerState", "RenderTargetView",
        "RWBuffer", "RWByteAddressBuffer", "RWStructuredBuffer",
        "RWTexture1D", "RWTexture1DArray", "RWTexture2D",
        "RWTexture2DArray", "RWTexture3D", "sample", "SamplerState",
        "SamplerComparisonState", "StructuredBuffer", "tbuffer", "Texture1D",
        "Texture1DArray", "Texture2D", "Texture2DArray", "Texture2DMS",
        "Texture2DMSArray", "Texture3D", "TextureCube", "TextureCubeArray",
        "triangle", "triangleadj", "TriangleStream", "DWORD", "FLOAT",
        "VECTOR", "MATRIX", "STRING", "TEXTURE", "PIXELSHADER",
        "VERTEXSHADER"
    };
    static const char *numericBases[] = {
        "bool", "cfloat", "char", "cint", "double", "dword", "fixed",
        "float", "half", "int", "long", "min10float", "min16float",
        "min12int", "min16int", "min16uint", "short", "uchar", "uint",
        "ulong", "ushort"
    };
    HlslModule module;
    char spelling[32];
    const char *emitted;
    const char *reserved;
    int candidate;
    int expectedCount;
    int found;
    int index;
    int reservedCount;

    HlslInitModule(&module, HLSL_STAGE_VERTEX, TestAlloc, NULL);
    expectedCount = (int) (sizeof(explicitReservedNames) /
                           sizeof(explicitReservedNames[0]));
    reservedCount = HlslReservedNameCount();
    assert(reservedCount == expectedCount);
    for (index = 0; index < reservedCount; index++) {
        reserved = HlslReservedNameAt(index);
        assert(reserved != NULL);
        assert(HlslIsReservedName(reserved));
        found = 0;
        for (candidate = 0; candidate < expectedCount; candidate++) {
            if (!strcmp(reserved, explicitReservedNames[candidate])) {
                found = 1;
                break;
            }
        }
        assert(found);
        emitted = HlslAllocateDistinctName(&module, reserved);
        assert(emitted != NULL);
        assert(!strncmp(emitted, "cg_", 3));
    }
    for (index = 0; index < expectedCount; index++) {
        found = 0;
        for (candidate = 0; candidate < reservedCount; candidate++) {
            reserved = HlslReservedNameAt(candidate);
            if (!strcmp(explicitReservedNames[index], reserved)) {
                found = 1;
                break;
            }
        }
        assert(found);
    }
    assert(HlslReservedNameAt(-1) == NULL);
    assert(HlslReservedNameAt(HlslReservedNameCount()) == NULL);
    for (index = 0; index < (int) (sizeof(numericBases) /
                                    sizeof(numericBases[0])); index++)
    {
        assert(HlslIsReservedName(numericBases[index]));
        sprintf(spelling, "%s2", numericBases[index]);
        assert(HlslIsReservedName(spelling));
        sprintf(spelling, "%s3x4", numericBases[index]);
        assert(HlslIsReservedName(spelling));
    }
}

static void TestTypeRegisterSpans(void)
{
    HlslType scalar;
    HlslType matrix;
    HlslType array;
    HlslType structure;
    HlslType arrayCycleA;
    HlslType arrayCycleB;
    HlslType structureCycle;
    HlslType overflowElement;
    HlslType invalid;
    HlslDecl firstMember;
    HlslDecl secondMember;
    HlslDecl cycleMember;
    HlslDecl cycleMemberA;
    HlslDecl cycleMemberB;

    scalar = HlslNumericType(HLSL_BASE_FLOAT, 4);
    matrix = HlslMatrixType(3, 4);
    memset(&array, 0, sizeof(array));
    array.arraySize = 2;
    array.elementType = &matrix;
    assert(HlslTypeRegisterSpan(&array) == 6);

    memset(&structure, 0, sizeof(structure));
    memset(&firstMember, 0, sizeof(firstMember));
    memset(&secondMember, 0, sizeof(secondMember));
    structure.base = HLSL_BASE_STRUCT;
    structure.structName = "Pair";
    structure.members = &firstMember;
    firstMember.type = scalar;
    firstMember.next = &secondMember;
    secondMember.type = matrix;
    assert(HlslTypeRegisterSpan(&structure) == 4);
    structure.structName = NULL;
    assert(HlslTypeRegisterSpan(&structure) == 0);
    structure.structName = "Pair";

    invalid = scalar;
    invalid.arraySize = -1;
    assert(HlslTypeRegisterSpan(&invalid) == 0);
    memset(&invalid, 0, sizeof(invalid));
    invalid.arraySize = 2;
    assert(HlslTypeRegisterSpan(&invalid) == 0);
    invalid = HlslNumericType(HLSL_BASE_VOID, 0);
    assert(HlslTypeRegisterSpan(&invalid) == 0);
    invalid = HlslNumericType(HLSL_BASE_FLOAT, 5);
    assert(HlslTypeRegisterSpan(&invalid) == 0);
    invalid = HlslMatrixType(2, 0);
    assert(HlslTypeRegisterSpan(&invalid) == 0);

    memset(&arrayCycleA, 0, sizeof(arrayCycleA));
    memset(&arrayCycleB, 0, sizeof(arrayCycleB));
    arrayCycleA.arraySize = 2;
    arrayCycleA.elementType = &arrayCycleB;
    arrayCycleB.arraySize = 3;
    arrayCycleB.elementType = &arrayCycleA;
    assert(HlslTypeRegisterSpan(&arrayCycleA) == 0);

    memset(&structureCycle, 0, sizeof(structureCycle));
    memset(&cycleMember, 0, sizeof(cycleMember));
    structureCycle.base = HLSL_BASE_STRUCT;
    structureCycle.structName = "Cycle";
    structureCycle.members = &cycleMember;
    cycleMember.type.arraySize = 1;
    cycleMember.type.elementType = &structureCycle;
    assert(HlslTypeRegisterSpan(&structureCycle) == 0);

    memset(&structureCycle, 0, sizeof(structureCycle));
    memset(&cycleMemberA, 0, sizeof(cycleMemberA));
    memset(&cycleMemberB, 0, sizeof(cycleMemberB));
    structureCycle.base = HLSL_BASE_STRUCT;
    structureCycle.structName = "RootCycle";
    structureCycle.members = &cycleMemberA;
    cycleMemberA.type.base = HLSL_BASE_STRUCT;
    cycleMemberA.type.structName = "CycleA";
    cycleMemberA.type.members = &cycleMemberB;
    cycleMemberB.type.base = HLSL_BASE_STRUCT;
    cycleMemberB.type.structName = "CycleB";
    cycleMemberB.type.members = &cycleMemberA;
    assert(HlslTypeRegisterSpan(&structureCycle) == 0);

    overflowElement = HlslMatrixType(2, 2);
    memset(&invalid, 0, sizeof(invalid));
    invalid.arraySize = INT_MAX;
    invalid.elementType = &overflowElement;
    assert(HlslTypeRegisterSpan(&invalid) == 0);

    firstMember.next = &secondMember;
    firstMember.type = scalar;
    memset(&secondMember, 0, sizeof(secondMember));
    memset(&array, 0, sizeof(array));
    array.arraySize = INT_MAX;
    array.elementType = &scalar;
    assert(HlslTypeRegisterSpan(&array) == INT_MAX);
    firstMember.type = array;
    secondMember.type = scalar;
    structure.members = &firstMember;
    assert(HlslTypeRegisterSpan(&structure) == 0);

    firstMember.next = &firstMember;
    firstMember.type = scalar;
    structure.members = &firstMember;
    assert(HlslTypeRegisterSpan(&structure) == 0);
}

static void TestDeclarationQualifiers(void)
{
    HlslModule module;
    HlslDecl *declaration;
    HlslType type;

    HlslInitModule(&module, HLSL_STAGE_VERTEX, TestAlloc, NULL);
    type = HlslNumericType(HLSL_BASE_FLOAT, 1);
    declaration = HlslNewDecl(&module, HLSL_STORAGE_NONE, type, "local");
    assert(declaration != NULL);
    assert(declaration->storageClass == HLSL_STORAGE_CLASS_AUTO);
    assert(declaration->typeQualifier == HLSL_TYPE_QUALIFIER_NONE);
    assert(declaration->parameterQualifier == HLSL_PARAMETER_IN);

    declaration->storageClass = HLSL_STORAGE_CLASS_STATIC;
    declaration->typeQualifier = HLSL_TYPE_QUALIFIER_CONST;
    assert(declaration->storage == HLSL_STORAGE_NONE);
    assert(declaration->storageClass == HLSL_STORAGE_CLASS_STATIC);
    assert(declaration->typeQualifier == HLSL_TYPE_QUALIFIER_CONST);

    declaration->storage = HLSL_STORAGE_UNIFORM;
    declaration->storageClass = HLSL_STORAGE_CLASS_EXTERN;
    declaration->typeQualifier = HLSL_TYPE_QUALIFIER_NONE;
    assert(declaration->storage == HLSL_STORAGE_UNIFORM);
    assert(declaration->storageClass == HLSL_STORAGE_CLASS_EXTERN);
}

static long StreamLength(FILE *stream)
{
    long length;

    assert(fflush(stream) == 0);
    assert(fseek(stream, 0, SEEK_END) == 0);
    length = ftell(stream);
    assert(length >= 0);
    return length;
}

static void AssertWriteFailureLeavesEmpty(const HlslModule *module,
    const HlslProfileDesc *profile)
{
    FILE *stream;

    stream = tmpfile();
    assert(stream != NULL);
    assert(!HlslWriteModule(stream, module, profile));
    assert(StreamLength(stream) == 0);
    assert(fclose(stream) == 0);
}

static void TestModuleWriter(void)
{
    static const char expected[] =
        "// profile hlslv\n"
        "// target vs_3_0\n"
        "void main()\n"
        "{\n"
        "}\n";
    HlslModule module;
    HlslProfileDesc profile;
    HlslFunction *entry;
    HlslDecl parameter;
    HlslType result;
    HlslType resultCycle;
    FILE *stream;
    char output[sizeof(expected)];
    size_t count;

    HlslInitModule(&module, HLSL_STAGE_VERTEX, TestAlloc, NULL);
    memset(&profile, 0, sizeof(profile));
    profile.stage = HLSL_STAGE_VERTEX;
    profile.name = "hlslv";
    profile.target = "vs_3_0";
    result = HlslNumericType(HLSL_BASE_VOID, 0);
    entry = HlslNewFunction(&module, result, "main");
    assert(entry != NULL);
    entry->isEntry = 1;
    module.functions = entry;
    module.entry = entry;

    stream = tmpfile();
    assert(stream != NULL);
    assert(HlslWriteModule(stream, &module, &profile));
    assert(StreamLength(stream) == (long) (sizeof(expected) - 1));
    rewind(stream);
    count = fread(output, 1, sizeof(expected) - 1, stream);
    assert(count == sizeof(expected) - 1);
    output[count] = '\0';
    assert(!strcmp(output, expected));
    assert(fclose(stream) == 0);

    entry->name = NULL;
    AssertWriteFailureLeavesEmpty(&module, &profile);
    entry->name = "";
    AssertWriteFailureLeavesEmpty(&module, &profile);
    entry->name = "bad name";
    AssertWriteFailureLeavesEmpty(&module, &profile);
    entry->name = "9main";
    AssertWriteFailureLeavesEmpty(&module, &profile);
    entry->name = "register";
    AssertWriteFailureLeavesEmpty(&module, &profile);
    entry->name = "main";

    profile.name = NULL;
    AssertWriteFailureLeavesEmpty(&module, &profile);
    profile.name = "hlslv";
    profile.target = NULL;
    AssertWriteFailureLeavesEmpty(&module, &profile);
    profile.target = "vs_3_0";
    AssertWriteFailureLeavesEmpty(&module, NULL);

    profile.stage = HLSL_STAGE_PIXEL;
    AssertWriteFailureLeavesEmpty(&module, &profile);
    profile.stage = HLSL_STAGE_VERTEX;
    module.stage = (HlslStage) 99;
    AssertWriteFailureLeavesEmpty(&module, &profile);
    module.stage = HLSL_STAGE_VERTEX;

    memset(&parameter, 0, sizeof(parameter));
    entry->parameters = &parameter;
    AssertWriteFailureLeavesEmpty(&module, &profile);
    entry->parameters = NULL;
    entry->result = HlslNumericType(HLSL_BASE_FLOAT, 1);
    AssertWriteFailureLeavesEmpty(&module, &profile);
    memset(&entry->result, 0, sizeof(entry->result));
    memset(&resultCycle, 0, sizeof(resultCycle));
    entry->result.arraySize = 1;
    entry->result.elementType = &resultCycle;
    resultCycle.arraySize = 1;
    resultCycle.elementType = &entry->result;
    AssertWriteFailureLeavesEmpty(&module, &profile);
    entry->result = result;
    entry->isEntry = 0;
    AssertWriteFailureLeavesEmpty(&module, &profile);
    entry->isEntry = 1;
    module.functions = NULL;
    AssertWriteFailureLeavesEmpty(&module, &profile);
    module.functions = entry;
    assert(!HlslWriteModule(NULL, &module, &profile));
}

static void TestModuleValidationRejectsUnownedEntry(void)
{
    HlslModule module;
    HlslProfileDesc profile;
    HlslType voidType;
    HlslType float4Type;
    HlslType inputType;
    HlslType outputType;
    HlslDecl *inputStruct;
    HlslDecl *outputStruct;
    HlslDecl *inputMember;
    HlslDecl *outputMember;
    HlslFunction *entry;
    HlslFunction *wrapper;
    HlslFunction *unowned;
    HlslExpr *call;
    HlslExpr *helperCall;
    HlslStmt *statement;
    HlslStmt *helperStatement;

    HlslInitModule(&module, HLSL_STAGE_VERTEX, TestAlloc, NULL);
    memset(&profile, 0, sizeof(profile));
    profile.stage = HLSL_STAGE_VERTEX;
    profile.name = "hlslv";
    profile.target = "vs_3_0";
    voidType = HlslNumericType(HLSL_BASE_VOID, 0);
    float4Type = HlslNumericType(HLSL_BASE_FLOAT, 4);
    inputType = HlslNumericType(HLSL_BASE_STRUCT, 0);
    inputType.structName = "cg_VertexIn";
    outputType = HlslNumericType(HLSL_BASE_STRUCT, 0);
    outputType.structName = "cg_VertexOut";

    inputStruct = HlslNewDecl(&module, HLSL_STORAGE_INPUT, inputType,
                              inputType.structName);
    outputStruct = HlslNewDecl(&module, HLSL_STORAGE_OUTPUT, outputType,
                               outputType.structName);
    inputMember = HlslNewDecl(&module, HLSL_STORAGE_NONE, float4Type,
                              "position");
    outputMember = HlslNewDecl(&module, HLSL_STORAGE_NONE, float4Type,
                               "position");
    assert(inputStruct != NULL && outputStruct != NULL &&
           inputMember != NULL && outputMember != NULL);
    inputMember->semantic = "POSITION0";
    outputMember->semantic = "POSITION0";
    HlslAppendDecl(&inputStruct->members, inputMember);
    HlslAppendDecl(&outputStruct->members, outputMember);
    inputStruct->type.members = inputStruct->members;
    outputStruct->type.members = outputStruct->members;
    HlslAppendDecl(&module.structs, inputStruct);
    HlslAppendDecl(&module.structs, outputStruct);

    entry = HlslNewFunction(&module, voidType, "cg_entry");
    wrapper = HlslNewFunction(&module, outputType, "main");
    call = HlslNewExpr(&module, HLSL_EXPR_CALL, voidType);
    statement = HlslNewStmt(&module, HLSL_STMT_EXPRESSION);
    assert(entry != NULL && wrapper != NULL && call != NULL &&
           statement != NULL);
    entry->isEntry = 1;
    call->u.call.function = entry;
    call->u.call.name = entry->name;
    statement->u.expression = call;
    HlslAppendStmt(&wrapper->body, statement);
    module.entry = entry;
    module.wrapper = wrapper;
    module.functions = wrapper;

    assert(!HlslValidateModule(&module, &profile));
    assert(module.errorKind == HLSL_ERROR_INVALID_IR);

    module.errors = 0;
    module.errorKind = HLSL_ERROR_NONE;
    module.errorReason = NULL;
    module.functions = entry;
    entry->next = wrapper;
    unowned = HlslNewFunction(&module, voidType, "cg_helper");
    helperCall = HlslNewExpr(&module, HLSL_EXPR_CALL, voidType);
    helperStatement = HlslNewStmt(&module, HLSL_STMT_EXPRESSION);
    assert(unowned != NULL && helperCall != NULL && helperStatement != NULL);
    helperCall->u.call.function = unowned;
    helperCall->u.call.name = unowned->name;
    helperStatement->u.expression = helperCall;
    HlslAppendStmt(&entry->body, helperStatement);
    assert(!HlslValidateModule(&module, &profile));
    assert(module.errorKind == HLSL_ERROR_INVALID_IR);
}

static void TestNamesAndAllocationFailure(void)
{
    HlslModule module;
    HlslModule failedModule;
    HlslType type;
    const char *first;
    const char *same;
    int firstIdentity;
    int secondIdentity;
    int firstNamespace;
    int secondNamespace;

    HlslInitModule(&module, HLSL_STAGE_VERTEX, TestAlloc, NULL);
    first = HlslAllocateScopedSymbolName(&module, &firstNamespace,
                                         &firstIdentity, "value");
    assert(first != NULL && !strcmp(first, "value"));
    same = HlslAllocateScopedSymbolName(&module, &firstNamespace,
                                        &firstIdentity, "renamed");
    assert(same == first);
    assert(!strcmp(HlslAllocateScopedSymbolName(&module, &firstNamespace,
                                                &secondIdentity, "value"),
                   "value_1"));
    assert(!strcmp(HlslAllocateScopedSymbolName(&module, &secondNamespace,
                                                &secondIdentity, "value"),
                   "value"));
    assert(!strcmp(HlslAllocateDistinctName(&module, "temporary"),
                   "temporary"));
    assert(!strcmp(HlslAllocateDistinctName(&module, "temporary"),
                   "temporary_1"));
    assert(!strcmp(HlslAllocateDistinctName(&module, "temporary"),
                   "temporary_2"));
    first = HlslAllocateName(&module, "stable");
    same = HlslAllocateName(&module, "stable");
    assert(first != NULL && same == first);
    assert(!strcmp(HlslAllocateDistinctName(&module, "cg_owned"),
                   "cg_cg_owned"));

    HlslInitModule(&failedModule, HLSL_STAGE_PIXEL, FaultAlloc, NULL);
    type = HlslNumericType(HLSL_BASE_FLOAT, 1);
    assert(HlslAllocateName(&failedModule, "name") == NULL);
    assert(HlslAllocateSymbolName(&failedModule, &firstIdentity,
                                  "name") == NULL);
    assert(HlslAllocateScopedSymbolName(&failedModule, &firstNamespace,
                                        &firstIdentity, "name") == NULL);
    assert(HlslAllocateDistinctName(&failedModule, "name") == NULL);
    assert(HlslNewDecl(&failedModule, HLSL_STORAGE_NONE, type, "decl") ==
           NULL);
    assert(HlslNewExpr(&failedModule, HLSL_EXPR_INT, type) == NULL);
    assert(HlslNewStmt(&failedModule, HLSL_STMT_RETURN) == NULL);
    assert(HlslNewFunction(&failedModule, type, "function") == NULL);
    assert(HlslNewBinding(&failedModule, HLSL_STORAGE_UNIFORM, type,
                          "binding", NULL) == NULL);
    assert(failedModule.names == NULL);
}

static void TestTypesAndLists(void)
{
    HlslModule module;
    HlslType type;
    HlslType invalid;
    HlslType element;
    HlslType innerArray;
    HlslType outerArray;
    HlslType arrayCycleA;
    HlslType arrayCycleB;
    HlslDecl *decl1;
    HlslDecl *decl2;
    HlslExpr *expr1;
    HlslExpr *expr2;
    HlslStmt *stmt1;
    HlslStmt *stmt2;
    HlslFunction *function1;
    HlslFunction *function2;
    HlslBinding *binding1;
    HlslBinding *binding2;
    HlslDecl *declList;
    HlslExpr *exprList;
    HlslStmt *stmtList;
    HlslFunction *functionList;
    HlslBinding *bindingList;

    memset(&module, 0xa5, sizeof(module));
    HlslInitModule(&module, HLSL_STAGE_PIXEL, DirtyAlloc, NULL);
    assert(module.stage == HLSL_STAGE_PIXEL);
    assert(module.alloc == DirtyAlloc);
    assert(module.allocArg == NULL);
    assert(module.names == NULL && module.structs == NULL &&
           module.globals == NULL && module.functions == NULL &&
           module.entry == NULL && module.wrapper == NULL &&
           module.bindings == NULL);
    assert(module.errorKind == HLSL_ERROR_NONE && module.errors == 0);

    type = HlslNumericType(HLSL_BASE_INT, 3);
    assert(type.base == HLSL_BASE_INT && type.len == 3);
    assert(type.rows == 0 && type.cols == 0 && type.arraySize == 0);
    assert(type.structName == NULL && type.elementType == NULL &&
           type.members == NULL);
    assert(!strcmp(HlslTypeName(&type), "int3"));
    assert(HlslTypeRegisterSpan(&type) == 1);
    invalid = HlslNumericType(HLSL_BASE_BOOL, 0);
    assert(HlslTypeName(&invalid) == NULL);
    assert(HlslTypeRegisterSpan(&invalid) == 0);
    invalid = HlslMatrixType(5, 4);
    assert(HlslTypeName(&invalid) == NULL);
    assert(HlslTypeRegisterSpan(&invalid) == 0);
    invalid = HlslNumericType(HLSL_BASE_SAMPLER2D, 1);
    assert(!strcmp(HlslTypeName(&invalid), "sampler2D"));
    assert(HlslTypeRegisterSpan(&invalid) == 1);
    element = HlslNumericType(HLSL_BASE_FLOAT, 1);
    memset(&invalid, 0, sizeof(invalid));
    invalid.arraySize = 4;
    invalid.elementType = &element;
    assert(!strcmp(HlslTypeName(&invalid), "float"));
    assert(HlslTypeRegisterSpan(&invalid) == 4);
    invalid.elementType = &invalid;
    assert(HlslTypeName(&invalid) == NULL);
    assert(HlslTypeRegisterSpan(&invalid) == 0);
    memset(&innerArray, 0, sizeof(innerArray));
    memset(&outerArray, 0, sizeof(outerArray));
    innerArray.arraySize = 2;
    innerArray.elementType = &element;
    outerArray.arraySize = 3;
    outerArray.elementType = &innerArray;
    assert(!strcmp(HlslTypeName(&outerArray), "float"));
    memset(&arrayCycleA, 0, sizeof(arrayCycleA));
    memset(&arrayCycleB, 0, sizeof(arrayCycleB));
    arrayCycleA.arraySize = 2;
    arrayCycleA.elementType = &arrayCycleB;
    arrayCycleB.arraySize = 3;
    arrayCycleB.elementType = &arrayCycleA;
    assert(HlslTypeName(&arrayCycleA) == NULL);
    assert(HlslTypeName(NULL) == NULL);
    assert(HlslTypeRegisterSpan(NULL) == 0);

    decl1 = HlslNewDecl(&module, HLSL_STORAGE_INPUT, type, "decl1");
    decl2 = HlslNewDecl(&module, HLSL_STORAGE_OUTPUT, type, "decl2");
    expr1 = HlslNewExpr(&module, HLSL_EXPR_SYMBOL, type);
    expr2 = HlslNewExpr(&module, HLSL_EXPR_INT, type);
    stmt1 = HlslNewStmt(&module, HLSL_STMT_DECLARATION);
    stmt2 = HlslNewStmt(&module, HLSL_STMT_DISCARD);
    function1 = HlslNewFunction(&module, type, "function1");
    function2 = HlslNewFunction(&module, type, "function2");
    binding1 = HlslNewBinding(&module, HLSL_STORAGE_UNIFORM, type,
                              "binding1", "C0");
    binding2 = HlslNewBinding(&module, HLSL_STORAGE_SAMPLER, type,
                              "binding2", NULL);
    assert(decl1 != NULL && decl2 != NULL && expr1 != NULL &&
           expr2 != NULL && stmt1 != NULL && stmt2 != NULL &&
           function1 != NULL && function2 != NULL && binding1 != NULL &&
           binding2 != NULL);
    assert(decl1->next == NULL && decl1->storage == HLSL_STORAGE_INPUT &&
           !strcmp(decl1->name, "decl1"));
    assert(decl1->storageClass == HLSL_STORAGE_CLASS_AUTO &&
           decl1->typeQualifier == HLSL_TYPE_QUALIFIER_NONE &&
           decl1->parameterQualifier == HLSL_PARAMETER_IN &&
           decl1->initializer == NULL && decl1->physical.bank == 0);
    assert(expr1->next == NULL && expr1->kind == HLSL_EXPR_SYMBOL &&
           expr1->u.symbol == NULL && expr1->loc.file == 0);
    assert(stmt1->next == NULL && stmt1->kind == HLSL_STMT_DECLARATION &&
           stmt1->u.declaration == NULL && stmt1->loc.line == 0);
    assert(function1->next == NULL && !strcmp(function1->name, "function1") &&
           function1->parameters == NULL &&
           function1->body == NULL && !function1->isEntry);
    assert(binding1->next == NULL &&
           binding1->storage == HLSL_STORAGE_UNIFORM &&
           !strcmp(binding1->publicName, "binding1") &&
           !strcmp(binding1->semantic, "C0") &&
           binding1->declaration == NULL && binding1->physical.bank == 0);

    declList = NULL;
    exprList = NULL;
    stmtList = NULL;
    functionList = NULL;
    bindingList = NULL;
    HlslAppendDecl(&declList, decl1);
    HlslAppendDecl(&declList, decl2);
    HlslAppendExpr(&exprList, expr1);
    HlslAppendExpr(&exprList, expr2);
    HlslAppendStmt(&stmtList, stmt1);
    HlslAppendStmt(&stmtList, stmt2);
    HlslAppendFunction(&functionList, function1);
    HlslAppendFunction(&functionList, function2);
    HlslAppendBinding(&bindingList, binding1);
    HlslAppendBinding(&bindingList, binding2);
    assert(declList == decl1 && decl1->next == decl2 && decl2->next == NULL);
    assert(exprList == expr1 && expr1->next == expr2 && expr2->next == NULL);
    assert(stmtList == stmt1 && stmt1->next == stmt2 && stmt2->next == NULL);
    assert(functionList == function1 && function1->next == function2 &&
           function2->next == NULL);
    assert(bindingList == binding1 && binding1->next == binding2 &&
           binding2->next == NULL);
    HlslAppendDecl(NULL, decl1);
    HlslAppendExpr(NULL, expr1);
    HlslAppendStmt(NULL, stmt1);
    HlslAppendFunction(NULL, function1);
    HlslAppendBinding(NULL, binding1);
    HlslAppendDecl(&declList, NULL);
    HlslAppendExpr(&exprList, NULL);
    HlslAppendStmt(&stmtList, NULL);
    HlslAppendFunction(&functionList, NULL);
    HlslAppendBinding(&bindingList, NULL);
}

static int CountDeclarations(const HlslDecl *declaration)
{
    int count;

    count = 0;
    for (; declaration != NULL; declaration = declaration->next)
        count++;
    return count;
}

static void TestBindingBanks(void)
{
    HlslModule module;
    HlslProfileDesc profile;
    HlslBinding *float4Binding;
    HlslBinding *matrixBinding;
    HlslBinding *intBinding;
    HlslBinding *boolBinding;
    HlslBinding *samplerBinding;
    HlslBinding *collisionBinding;
    HlslType type;

    HlslInitModule(&module, HLSL_STAGE_VERTEX, TestAlloc, NULL);
    profile = HlslProfile_hlslv;
    type = HlslNumericType(HLSL_BASE_FLOAT, 4);
    float4Binding = HlslNewBinding(&module, HLSL_STORAGE_UNIFORM, type,
                                  "color", NULL);
    assert(float4Binding != NULL);
    assert(HlslAllocateOneBinding(&module, &profile, float4Binding));
    assert(float4Binding->physical.bank == HLSL_REGISTER_C);
    assert(float4Binding->physical.regno == 0);
    assert(float4Binding->physical.span == 1);

    type = HlslMatrixType(4, 4);
    matrixBinding = HlslNewBinding(&module, HLSL_STORAGE_UNIFORM, type,
                                  "transform", NULL);
    assert(matrixBinding != NULL);
    assert(HlslAllocateOneBinding(&module, &profile, matrixBinding));
    assert(matrixBinding->physical.bank == HLSL_REGISTER_C);
    assert(matrixBinding->physical.regno == 1);
    assert(matrixBinding->physical.span == 4);

    type = HlslNumericType(HLSL_BASE_INT, 4);
    intBinding = HlslNewBinding(&module, HLSL_STORAGE_UNIFORM, type,
                               "indices", NULL);
    assert(intBinding != NULL);
    assert(HlslAllocateOneBinding(&module, &profile, intBinding));
    assert(intBinding->physical.bank == HLSL_REGISTER_I);
    assert(intBinding->physical.regno == 0);
    assert(intBinding->physical.span == 1);

    type = HlslNumericType(HLSL_BASE_BOOL, 1);
    boolBinding = HlslNewBinding(&module, HLSL_STORAGE_UNIFORM, type,
                                "enabled", NULL);
    assert(boolBinding != NULL);
    assert(HlslAllocateOneBinding(&module, &profile, boolBinding));
    assert(boolBinding->physical.bank == HLSL_REGISTER_B);
    assert(boolBinding->physical.regno == 0);
    assert(boolBinding->physical.span == 1);

    type = HlslNumericType(HLSL_BASE_SAMPLER2D, 1);
    samplerBinding = HlslNewBinding(&module, HLSL_STORAGE_SAMPLER, type,
                                   "image", NULL);
    assert(samplerBinding != NULL);
    assert(HlslAllocateOneBinding(&module, &profile, samplerBinding));
    assert(samplerBinding->physical.bank == HLSL_REGISTER_S);
    assert(samplerBinding->physical.regno == 0);
    assert(samplerBinding->physical.span == 1);

    type = HlslNumericType(HLSL_BASE_FLOAT, 1);
    collisionBinding = HlslNewBinding(&module, HLSL_STORAGE_UNIFORM, type,
                                     "collision", NULL);
    assert(collisionBinding != NULL);
    collisionBinding->loc.file = 3;
    collisionBinding->loc.line = 19;
    collisionBinding->hasExplicitRegister = 1;
    collisionBinding->physical.bank = HLSL_REGISTER_C;
    collisionBinding->physical.regno = 0;
    assert(!HlslAllocateOneBinding(&module, &profile, collisionBinding));
    assert(module.errorKind == HLSL_ERROR_REGISTER_COLLISION);
    assert(module.errorReason != NULL);
    assert(strstr(module.errorReason, "collision") != NULL);
    assert(strstr(module.errorReason, "c0") != NULL);
    assert(module.errorLoc.file == 3 && module.errorLoc.line == 19);
}

static void TestAggregateAndSamplerBindings(void)
{
    HlslModule module;
    HlslProfileDesc profile;
    HlslBinding *arrayBinding;
    HlslBinding *matrixBinding;
    HlslBinding *structBinding;
    HlslBinding *explicitSampler;
    HlslBinding *implicitSampler;
    HlslBinding *leaf;
    HlslDecl floatMember;
    HlslDecl intMember;
    HlslDecl boolMember;
    HlslType arrayType;
    HlslType elementType;
    HlslType structType;
    HlslType type;

    HlslInitModule(&module, HLSL_STAGE_PIXEL, TestAlloc, NULL);
    profile = HlslProfile_hlslf;

    type = HlslNumericType(HLSL_BASE_SAMPLER2D, 1);
    implicitSampler = HlslNewBinding(&module, HLSL_STORAGE_SAMPLER, type,
                                    "implicitImage", NULL);
    explicitSampler = HlslNewBinding(&module, HLSL_STORAGE_SAMPLER, type,
                                    "explicitImage", "TEXUNIT3");
    assert(implicitSampler != NULL && explicitSampler != NULL);
    implicitSampler->sourceOrdinal = 0;
    explicitSampler->sourceOrdinal = 4;
    explicitSampler->hasExplicitRegister = 1;
    explicitSampler->physical.bank = HLSL_REGISTER_S;
    explicitSampler->physical.regno = 3;

    elementType = HlslNumericType(HLSL_BASE_FLOAT, 4);
    memset(&arrayType, 0, sizeof(arrayType));
    arrayType.arraySize = 3;
    arrayType.elementType = &elementType;
    arrayBinding = HlslNewBinding(&module, HLSL_STORAGE_UNIFORM, arrayType,
                                 "weights", NULL);
    assert(arrayBinding != NULL);
    arrayBinding->sourceOrdinal = 1;

    type = HlslMatrixType(4, 4);
    matrixBinding = HlslNewBinding(&module, HLSL_STORAGE_UNIFORM, type,
                                  "matrix", NULL);
    assert(matrixBinding != NULL);
    matrixBinding->sourceOrdinal = 2;

    memset(&floatMember, 0, sizeof(floatMember));
    memset(&intMember, 0, sizeof(intMember));
    memset(&boolMember, 0, sizeof(boolMember));
    floatMember.name = "scale";
    floatMember.type = HlslNumericType(HLSL_BASE_FLOAT, 1);
    floatMember.next = &intMember;
    intMember.name = "index";
    intMember.type = HlslNumericType(HLSL_BASE_INT, 1);
    intMember.next = &boolMember;
    boolMember.name = "enabled";
    boolMember.type = HlslNumericType(HLSL_BASE_BOOL, 1);
    memset(&structType, 0, sizeof(structType));
    structType.base = HLSL_BASE_STRUCT;
    structType.structName = "Parameters";
    structType.members = &floatMember;
    structBinding = HlslNewBinding(&module, HLSL_STORAGE_UNIFORM, structType,
                                  "parameters", NULL);
    assert(structBinding != NULL);
    structBinding->sourceOrdinal = 3;

    HlslAppendBinding(&module.bindings, explicitSampler);
    HlslAppendBinding(&module.bindings, structBinding);
    HlslAppendBinding(&module.bindings, matrixBinding);
    HlslAppendBinding(&module.bindings, implicitSampler);
    HlslAppendBinding(&module.bindings, arrayBinding);
    assert(HlslAllocateBindings(&module, &profile));

    assert(explicitSampler->physical.bank == HLSL_REGISTER_S);
    assert(explicitSampler->physical.regno == 3);
    assert(explicitSampler->physical.span == 1);
    assert(implicitSampler->physical.bank == HLSL_REGISTER_S);
    assert(implicitSampler->physical.regno == 0);
    assert(arrayBinding->physical.bank == HLSL_REGISTER_C);
    assert(arrayBinding->physical.regno == 0);
    assert(arrayBinding->physical.span == 3);
    assert(matrixBinding->physical.bank == HLSL_REGISTER_C);
    assert(matrixBinding->physical.regno == 3);
    assert(matrixBinding->physical.span == 4);
    assert(structBinding->physical.bank == HLSL_REGISTER_NONE);

    leaf = structBinding->leafBindings;
    assert(leaf != NULL);
    assert(!strcmp(leaf->publicName, "parameters.scale"));
    assert(leaf->recursiveOffset == 0);
    assert(leaf->physical.bank == HLSL_REGISTER_C);
    assert(leaf->physical.regno == 7);
    leaf = leaf->next;
    assert(leaf != NULL);
    assert(!strcmp(leaf->publicName, "parameters.index"));
    assert(leaf->recursiveOffset == 1);
    assert(leaf->physical.bank == HLSL_REGISTER_I);
    assert(leaf->physical.regno == 0);
    leaf = leaf->next;
    assert(leaf != NULL);
    assert(!strcmp(leaf->publicName, "parameters.enabled"));
    assert(leaf->recursiveOffset == 2);
    assert(leaf->physical.bank == HLSL_REGISTER_B);
    assert(leaf->physical.regno == 0);
    assert(leaf->next == NULL);

    assert(arrayBinding->leafBindings != NULL);
    assert(!strcmp(arrayBinding->leafBindings->publicName, "weights"));
    assert(arrayBinding->leafBindings->physical.span == 3);
    assert(matrixBinding->leafBindings != NULL);
    assert(!strcmp(matrixBinding->leafBindings->publicName, "matrix"));
    assert(matrixBinding->leafBindings->physical.span == 4);
    assert(CountDeclarations(module.globals) == 7);
    for (leaf = module.allocatedBindings; leaf != NULL;
         leaf = leaf->allocationNext)
    {
        assert(leaf->declaration != NULL);
        assert(leaf->declaration->physical.bank == leaf->physical.bank);
        assert(leaf->declaration->typeQualifier ==
               HLSL_TYPE_QUALIFIER_NONE);
        assert(leaf->declaration->storageClass ==
               HLSL_STORAGE_CLASS_AUTO);
    }
}

static void TestDefaultBindingAndResourceFailure(void)
{
    HlslModule module;
    HlslProfileDesc profile;
    HlslLimits limits;
    HlslBinding *binding;
    HlslBinding *overflow;
    HlslExpr *value;
    HlslType type;
    float defaults[4];
    float extraDefaults[5];

    HlslInitModule(&module, HLSL_STAGE_VERTEX, TestAlloc, NULL);
    profile = HlslProfile_hlslv;
    limits = *profile.limits;
    limits.floatConstants = 1;
    profile.limits = &limits;
    defaults[0] = 1.0f;
    defaults[1] = 2.0f;
    defaults[2] = 3.0f;
    defaults[3] = 4.0f;
    type = HlslNumericType(HLSL_BASE_FLOAT, 4);
    binding = HlslNewBinding(&module, HLSL_STORAGE_UNIFORM, type,
                            "color", NULL);
    assert(binding != NULL);
    binding->defaultCount = 4;
    binding->defaultValues = defaults;
    assert(HlslAllocateOneBinding(&module, &profile, binding));
    assert(binding->leafBindings != NULL);
    assert(binding->leafBindings->defaultCount == 4);
    assert(binding->leafBindings->defaultValues[0] == 1.0f);
    assert(binding->leafBindings->defaultValues[1] == 2.0f);
    assert(binding->leafBindings->defaultValues[2] == 3.0f);
    assert(binding->leafBindings->defaultValues[3] == 4.0f);
    assert(binding->leafBindings->declaration->initializer != NULL);
    value = binding->leafBindings->declaration->initializer->
            u.construct.arguments;
    assert(value != NULL && value->u.literalFloat == 1.0f);
    value = value->next;
    assert(value != NULL && value->u.literalFloat == 2.0f);
    value = value->next;
    assert(value != NULL && value->u.literalFloat == 3.0f);
    value = value->next;
    assert(value != NULL && value->u.literalFloat == 4.0f);
    assert(value->next == NULL);
    assert(binding->leafBindings->declaration->typeQualifier ==
           HLSL_TYPE_QUALIFIER_NONE);

    overflow = HlslNewBinding(&module, HLSL_STORAGE_UNIFORM, type,
                             "overflow", NULL);
    assert(overflow != NULL);
    assert(!HlslAllocateOneBinding(&module, &profile, overflow));
    assert(module.errorKind == HLSL_ERROR_RESOURCE_LIMIT);
    assert(!strcmp(module.resourceName, "c"));
    assert(module.resourceUsed == 2);
    assert(module.resourceAvailable == 1);

    HlslInitModule(&module, HLSL_STAGE_VERTEX, TestAlloc, NULL);
    profile = HlslProfile_hlslv;
    memcpy(extraDefaults, defaults, sizeof(defaults));
    extraDefaults[4] = 5.0f;
    binding = HlslNewBinding(&module, HLSL_STORAGE_UNIFORM, type,
                            "badDefault", NULL);
    assert(binding != NULL);
    binding->defaultCount = 5;
    binding->defaultValues = extraDefaults;
    assert(!HlslAllocateOneBinding(&module, &profile, binding));
    assert(module.errorKind == HLSL_ERROR_INVALID_IR);
}

static void TestCyclicBindingListIsRejected(void)
{
    HlslModule module;
    HlslBinding *binding;
    HlslType type;

    HlslInitModule(&module, HLSL_STAGE_VERTEX, TestAlloc, NULL);
    type = HlslNumericType(HLSL_BASE_FLOAT, 1);
    binding = HlslNewBinding(&module, HLSL_STORAGE_UNIFORM, type,
                            "cycle", NULL);
    assert(binding != NULL);
    binding->next = binding;
    module.bindings = binding;
    assert(!HlslAllocateBindings(&module, &HlslProfile_hlslv));
    assert(module.errorKind == HLSL_ERROR_INVALID_IR);
}

static void TestAggregateBankClassification(void)
{
    HlslModule module;
    HlslProfileDesc profile;
    HlslLimits limits;
    HlslBinding *firstBlocker;
    HlslBinding *secondBlocker;
    HlslBinding *homogeneous;
    HlslBinding *mixedArray;
    HlslBinding *leaf;
    HlslDecl firstMember;
    HlslDecl secondMember;
    HlslType homogeneousType;
    HlslType mixedType;
    HlslType mixedArrayType;
    HlslType type;

    HlslInitModule(&module, HLSL_STAGE_VERTEX, TestAlloc, NULL);
    profile = HlslProfile_hlslv;
    limits = *profile.limits;
    limits.floatConstants = 6;
    profile.limits = &limits;
    type = HlslNumericType(HLSL_BASE_FLOAT, 1);
    firstBlocker = HlslNewBinding(&module, HLSL_STORAGE_UNIFORM, type,
                                  "firstBlocker", NULL);
    secondBlocker = HlslNewBinding(&module, HLSL_STORAGE_UNIFORM, type,
                                   "secondBlocker", NULL);
    assert(firstBlocker != NULL && secondBlocker != NULL);
    firstBlocker->hasExplicitRegister = 1;
    firstBlocker->physical.bank = HLSL_REGISTER_C;
    firstBlocker->physical.regno = 1;
    secondBlocker->hasExplicitRegister = 1;
    secondBlocker->physical.bank = HLSL_REGISTER_C;
    secondBlocker->physical.regno = 3;
    assert(HlslAllocateOneBinding(&module, &profile, firstBlocker));
    assert(HlslAllocateOneBinding(&module, &profile, secondBlocker));

    memset(&firstMember, 0, sizeof(firstMember));
    memset(&secondMember, 0, sizeof(secondMember));
    firstMember.name = "first";
    firstMember.type = HlslNumericType(HLSL_BASE_FLOAT, 4);
    firstMember.next = &secondMember;
    secondMember.name = "second";
    secondMember.type = HlslNumericType(HLSL_BASE_FLOAT, 4);
    memset(&homogeneousType, 0, sizeof(homogeneousType));
    homogeneousType.base = HLSL_BASE_STRUCT;
    homogeneousType.structName = "Homogeneous";
    homogeneousType.members = &firstMember;
    homogeneous = HlslNewBinding(&module, HLSL_STORAGE_UNIFORM,
                                 homogeneousType, "pair", NULL);
    assert(homogeneous != NULL);
    assert(HlslAllocateOneBinding(&module, &profile, homogeneous));
    assert(homogeneous->physical.bank == HLSL_REGISTER_C);
    assert(homogeneous->physical.regno == 4);
    assert(homogeneous->physical.span == 2);
    assert(homogeneous->leafBindings != NULL);
    assert(homogeneous->leafBindings->next == NULL);
    assert(!strcmp(homogeneous->leafBindings->publicName, "pair"));

    HlslInitModule(&module, HLSL_STAGE_VERTEX, TestAlloc, NULL);
    profile = HlslProfile_hlslv;
    memset(&firstMember, 0, sizeof(firstMember));
    memset(&secondMember, 0, sizeof(secondMember));
    firstMember.name = "weight";
    firstMember.type = HlslNumericType(HLSL_BASE_FLOAT, 1);
    firstMember.next = &secondMember;
    secondMember.name = "index";
    secondMember.type = HlslNumericType(HLSL_BASE_INT, 1);
    memset(&mixedType, 0, sizeof(mixedType));
    mixedType.base = HLSL_BASE_STRUCT;
    mixedType.structName = "Mixed";
    mixedType.members = &firstMember;
    memset(&mixedArrayType, 0, sizeof(mixedArrayType));
    mixedArrayType.arraySize = 2;
    mixedArrayType.elementType = &mixedType;
    mixedArray = HlslNewBinding(&module, HLSL_STORAGE_UNIFORM,
                               mixedArrayType, "items", NULL);
    assert(mixedArray != NULL);
    assert(HlslAllocateOneBinding(&module, &profile, mixedArray));
    assert(mixedArray->physical.bank == HLSL_REGISTER_NONE);
    leaf = mixedArray->leafBindings;
    assert(leaf != NULL && !strcmp(leaf->publicName, "items[0].weight"));
    assert(leaf->physical.bank == HLSL_REGISTER_C &&
           leaf->physical.regno == 0);
    leaf = leaf->next;
    assert(leaf != NULL && !strcmp(leaf->publicName, "items[0].index"));
    assert(leaf->physical.bank == HLSL_REGISTER_I &&
           leaf->physical.regno == 0);
    leaf = leaf->next;
    assert(leaf != NULL && !strcmp(leaf->publicName, "items[1].weight"));
    assert(leaf->physical.bank == HLSL_REGISTER_C &&
           leaf->physical.regno == 1);
    leaf = leaf->next;
    assert(leaf != NULL && !strcmp(leaf->publicName, "items[1].index"));
    assert(leaf->physical.bank == HLSL_REGISTER_I &&
           leaf->physical.regno == 1 && leaf->next == NULL);
}

static void TestBindingDiagnosticArithmetic(void)
{
    HlslModule module;
    HlslBinding binding;
    HlslBinding *occupied;
    HlslProfileDesc profile;
    HlslType elementType;
    HlslType hugeType;
    HlslType type;

    HlslInitModule(&module, HLSL_STAGE_VERTEX, TestAlloc, NULL);
    profile = HlslProfile_hlslv;
    memset(&binding, 0, sizeof(binding));
    binding.storage = HLSL_STORAGE_UNIFORM;
    binding.type = HlslNumericType(HLSL_BASE_FLOAT, 1);
    binding.name = "internalExplicit";
    binding.publicName = "publicExplicit";
    binding.loc.file = 7;
    binding.loc.line = 31;
    binding.hasExplicitRegister = 1;
    binding.physical.bank = HLSL_REGISTER_C;
    binding.physical.regno = INT_MAX;
    assert(!HlslAllocateOneBinding(&module, &profile, &binding));
    assert(module.errorKind == HLSL_ERROR_RESOURCE_LIMIT);
    assert(module.resourceUsed == INT_MAX);
    assert(module.resourceAvailable == profile.limits->floatConstants);
    assert(module.errorLoc.file == 7 && module.errorLoc.line == 31);

    HlslInitModule(&module, HLSL_STAGE_VERTEX, TestAlloc, NULL);
    profile = HlslProfile_hlslv;
    type = HlslNumericType(HLSL_BASE_FLOAT, 1);
    occupied = HlslNewBinding(&module, HLSL_STORAGE_UNIFORM, type,
                             "occupied", NULL);
    assert(occupied != NULL);
    assert(HlslAllocateOneBinding(&module, &profile, occupied));
    elementType = HlslNumericType(HLSL_BASE_FLOAT, 1);
    memset(&hugeType, 0, sizeof(hugeType));
    hugeType.arraySize = INT_MAX;
    hugeType.elementType = &elementType;
    memset(&binding, 0, sizeof(binding));
    binding.storage = HLSL_STORAGE_UNIFORM;
    binding.type = hugeType;
    binding.name = "internalHuge";
    binding.publicName = "publicHuge";
    assert(!HlslAllocateOneBinding(&module, &profile, &binding));
    assert(module.errorKind == HLSL_ERROR_RESOURCE_LIMIT);
    assert(module.resourceUsed == INT_MAX);
}

static void TestOversizedMixedArrayPreflight(void)
{
    HlslModule module;
    HlslBinding binding;
    HlslBinding bindingBefore;
    HlslProfileDesc profile;
    HlslLimits limits;
    HlslDecl floatMember;
    HlslDecl intMember;
    HlslType elementType;
    HlslType arrayType;
    LimitedAllocState allocState;
    unsigned char cBefore[HLSL_MAX_FLOAT_CONSTANTS];
    unsigned char iBefore[HLSL_MAX_INT_CONSTANTS];
    unsigned char bBefore[HLSL_MAX_BOOL_CONSTANTS];
    unsigned char sBefore[HLSL_MAX_SAMPLERS];

    allocState.calls = 0;
    allocState.limit = 32;
    HlslInitModule(&module, HLSL_STAGE_VERTEX, LimitedAlloc, &allocState);
    profile = HlslProfile_hlslv;
    limits = *profile.limits;
    limits.floatConstants = 1;
    limits.intConstants = 1;
    profile.limits = &limits;

    memset(&floatMember, 0, sizeof(floatMember));
    memset(&intMember, 0, sizeof(intMember));
    floatMember.name = "weight";
    floatMember.type = HlslNumericType(HLSL_BASE_FLOAT, 1);
    floatMember.next = &intMember;
    intMember.name = "index";
    intMember.type = HlslNumericType(HLSL_BASE_INT, 1);
    memset(&elementType, 0, sizeof(elementType));
    elementType.base = HLSL_BASE_STRUCT;
    elementType.structName = "HugeMixedElement";
    elementType.members = &floatMember;
    memset(&arrayType, 0, sizeof(arrayType));
    arrayType.arraySize = INT_MAX / 2;
    arrayType.elementType = &elementType;
    memset(&binding, 0, sizeof(binding));
    binding.storage = HLSL_STORAGE_UNIFORM;
    binding.type = arrayType;
    binding.name = "internalHugeMixed";
    binding.publicName = "publicHugeMixed";
    binding.loc.file = 11;
    binding.loc.line = 52;
    bindingBefore = binding;
    memcpy(cBefore, module.cRegisterUsed, sizeof(cBefore));
    memcpy(iBefore, module.iRegisterUsed, sizeof(iBefore));
    memcpy(bBefore, module.bRegisterUsed, sizeof(bBefore));
    memcpy(sBefore, module.sRegisterUsed, sizeof(sBefore));

    assert(!HlslAllocateOneBinding(&module, &profile, &binding));
    if (module.errorKind != HLSL_ERROR_RESOURCE_LIMIT) {
        fprintf(stderr,
                "oversized mixed array: error %d after %d allocations\n",
                module.errorKind, allocState.calls);
        exit(1);
    }
    assert(!strcmp(module.resourceName, "c"));
    assert(module.resourceUsed == 2);
    assert(module.resourceAvailable == 1);
    assert(module.errorLoc.file == 11 && module.errorLoc.line == 52);
    assert(allocState.calls == 0);
    assert(!memcmp(&binding, &bindingBefore, sizeof(binding)));
    assert(!memcmp(module.cRegisterUsed, cBefore, sizeof(cBefore)));
    assert(!memcmp(module.iRegisterUsed, iBefore, sizeof(iBefore)));
    assert(!memcmp(module.bRegisterUsed, bBefore, sizeof(bBefore)));
    assert(!memcmp(module.sRegisterUsed, sBefore, sizeof(sBefore)));
    assert(module.names == NULL && module.globals == NULL &&
           module.allocatedBindings == NULL);
}

static void TestDefaultValidationIsTransactional(void)
{
    HlslModule module;
    HlslBinding binding;
    HlslProfileDesc profile;
    HlslType type;
    volatile float zero;
    float value;

    profile = HlslProfile_hlslv;
    type = HlslNumericType(HLSL_BASE_FLOAT, 1);
    HlslInitModule(&module, HLSL_STAGE_VERTEX, InitializerFaultAlloc, NULL);
    memset(&binding, 0, sizeof(binding));
    binding.storage = HLSL_STORAGE_UNIFORM;
    binding.type = type;
    binding.name = "initializerFailure";
    binding.publicName = "initializerFailure";
    binding.defaultCount = 1;
    value = 1.0f;
    binding.defaultValues = &value;
    assert(!HlslAllocateOneBinding(&module, &profile, &binding));
    assert(module.errorKind == HLSL_ERROR_INVALID_IR);
    assert(module.names == NULL && module.globals == NULL &&
           module.allocatedBindings == NULL);
    assert(binding.leafBindings == NULL && !binding.isAllocated);
    assert(module.cRegisterUsed[0] == 0);

    type = HlslNumericType(HLSL_BASE_INT, 1);
    HlslInitModule(&module, HLSL_STAGE_VERTEX, TestAlloc, NULL);
    memset(&binding, 0, sizeof(binding));
    binding.storage = HLSL_STORAGE_UNIFORM;
    binding.type = type;
    binding.name = "largeInteger";
    binding.defaultCount = 1;
    value = FLT_MAX;
    binding.defaultValues = &value;
    assert(!HlslAllocateOneBinding(&module, &profile, &binding));
    assert(module.errorKind == HLSL_ERROR_INVALID_IR);

    type = HlslNumericType(HLSL_BASE_FLOAT, 1);
    HlslInitModule(&module, HLSL_STAGE_VERTEX, TestAlloc, NULL);
    memset(&binding, 0, sizeof(binding));
    binding.storage = HLSL_STORAGE_UNIFORM;
    binding.type = type;
    binding.name = "notANumber";
    binding.defaultCount = 1;
    zero = 0.0f;
    value = zero / zero;
    binding.defaultValues = &value;
    assert(!HlslAllocateOneBinding(&module, &profile, &binding));
    assert(module.errorKind == HLSL_ERROR_INVALID_IR);

    type = HlslNumericType(HLSL_BASE_BOOL, 1);
    HlslInitModule(&module, HLSL_STAGE_VERTEX, TestAlloc, NULL);
    memset(&binding, 0, sizeof(binding));
    binding.storage = HLSL_STORAGE_UNIFORM;
    binding.type = type;
    binding.name = "invalidBoolean";
    binding.defaultCount = 1;
    value = 2.0f;
    binding.defaultValues = &value;
    assert(!HlslAllocateOneBinding(&module, &profile, &binding));
    assert(module.errorKind == HLSL_ERROR_INVALID_IR);
}

static void TestPublicBindingNames(void)
{
    HlslModule module;
    HlslBinding *occupied;
    HlslBinding *binding;
    HlslProfileDesc profile;
    HlslType type;

    HlslInitModule(&module, HLSL_STAGE_VERTEX, TestAlloc, NULL);
    profile = HlslProfile_hlslv;
    type = HlslNumericType(HLSL_BASE_FLOAT, 1);
    binding = HlslNewBinding(&module, HLSL_STORAGE_UNIFORM, type,
                            "internalColor", NULL);
    assert(binding != NULL);
    binding->publicName = "publicColor";
    assert(HlslAllocateOneBinding(&module, &profile, binding));
    assert(binding->leafBindings != NULL);
    assert(!strcmp(binding->leafBindings->publicName, "publicColor"));
    occupied = binding;

    binding = HlslNewBinding(&module, HLSL_STORAGE_UNIFORM, type,
                            "fallbackColor", NULL);
    assert(binding != NULL);
    binding->publicName = "";
    assert(HlslAllocateOneBinding(&module, &profile, binding));
    assert(!strcmp(binding->leafBindings->publicName, "fallbackColor"));

    binding = HlslNewBinding(&module, HLSL_STORAGE_UNIFORM, type,
                            "otherInternalColor", NULL);
    assert(binding != NULL && occupied->physical.regno == 0);
    binding->publicName = "otherPublicColor";
    binding->loc.file = 9;
    binding->loc.line = 44;
    binding->hasExplicitRegister = 1;
    binding->physical.bank = HLSL_REGISTER_C;
    binding->physical.regno = 0;
    assert(!HlslAllocateOneBinding(&module, &profile, binding));
    assert(module.errorKind == HLSL_ERROR_REGISTER_COLLISION);
    assert(strstr(module.errorReason, "otherPublicColor") != NULL);
    assert(strstr(module.errorReason, "otherInternalColor") == NULL);
    assert(module.errorLoc.file == 9 && module.errorLoc.line == 44);

    HlslInitModule(&module, HLSL_STAGE_VERTEX, TestAlloc, NULL);
    binding = HlslNewBinding(&module, HLSL_STORAGE_UNIFORM, type,
                            "validInternal", NULL);
    assert(binding != NULL);
    binding->publicName = "invalid public name";
    assert(!HlslAllocateOneBinding(&module, &profile, binding));
    assert(module.errorKind == HLSL_ERROR_INVALID_IR);
}

int main(int argc, char **argv)
{
    HlslModule module;
    HlslType scalar;
    HlslType vector;
    HlslType matrix;
    char semanticRoot[32];
    int semanticIndex;
    int firstIdentity;
    int secondIdentity;
    int cfloatIdentity;
    int cintIdentity;
    int generatedIdentity;
    int secondGeneratedIdentity;

    if (argc == 2 && !strcmp(argv[1], "--verify-assertions-active")) {
        int assertionsActive;

        assertionsActive = 0;
        assert((assertionsActive = 1) != 0);
        if (!assertionsActive)
            return 2;
        puts("glsl-ir-assertions-active");
        return 0;
    }

    HlslInitModule(&module, HLSL_STAGE_VERTEX, TestAlloc, NULL);
    scalar = HlslNumericType(HLSL_BASE_FLOAT, 1);
    vector = HlslNumericType(HLSL_BASE_FLOAT, 4);
    matrix = HlslMatrixType(3, 4);
    assert(!strcmp(HlslTypeName(&scalar), "float"));
    assert(!strcmp(HlslTypeName(&vector), "float4"));
    assert(!strcmp(HlslTypeName(&matrix), "row_major float3x4"));
    assert(HlslTypeRegisterSpan(&matrix) == 3);
    assert(!strcmp(HlslAllocateSymbolName(&module, &firstIdentity, "main"),
                   "cg_main"));
    assert(!strcmp(HlslAllocateSymbolName(&module, &firstIdentity, "main"),
                   "cg_main"));
    assert(!strcmp(HlslAllocateSymbolName(&module, &secondIdentity, "main"),
                   "cg_main_1"));
    assert(HlslIsReservedName("register"));
    assert(HlslIsReservedName("row_major"));
    assert(HlslIsReservedName("varying"));
    assert(HlslIsReservedName("uchar4"));
    assert(HlslIsReservedName("float3x4"));
    assert(HlslIsReservedName("samplerRECT"));
    assert(HlslIsReservedName("cfloat"));
    assert(HlslIsReservedName("cint"));
    assert(HlslParseSemantic("TEXCOORD15", semanticRoot,
                             sizeof(semanticRoot), &semanticIndex));
    assert(!strcmp(semanticRoot, "TEXCOORD") && semanticIndex == 15);
    assert(HlslParseSemantic("VPOS", semanticRoot,
                             sizeof(semanticRoot), &semanticIndex));
    assert(!strcmp(semanticRoot, "VPOS") && semanticIndex == 0);
    assert(!HlslParseSemantic("", semanticRoot, sizeof(semanticRoot),
                              &semanticIndex));
    assert(!HlslParseSemantic("TEXCOORD0", semanticRoot, 4,
                              &semanticIndex));
    assert(!strcmp(HlslCanonicalSemantic(&HlslProfile_hlslv,
        "HPOS", 1), "POSITION0"));
    assert(!strcmp(HlslCanonicalSemantic(&HlslProfile_hlslv,
        "ATTR7", 0), "TEXCOORD7"));
    assert(!strcmp(HlslCanonicalSemantic(&HlslProfile_hlslv,
        "COL1", 1), "COLOR1"));
    assert(!strcmp(HlslCanonicalSemantic(&HlslProfile_hlslf,
        "WPOS", 0), "VPOS"));
    assert(!strcmp(HlslCanonicalSemantic(&HlslProfile_hlslf,
        "FACE", 0), "VFACE"));
    assert(HlslCanonicalSemantic(&HlslProfile_hlslf,
        "NORMAL0", 0) == NULL);
    assert(!strcmp(HlslAllocateSymbolName(&module, &cfloatIdentity,
                                          "cfloat"), "cg_cfloat"));
    assert(!strcmp(HlslAllocateSymbolName(&module, &cintIdentity,
                                          "cint"), "cg_cint"));
    assert(!strcmp(HlslAllocateGeneratedName(&module, &generatedIdentity,
                                             "cg_entry"), "cg_entry"));
    assert(!strcmp(HlslAllocateGeneratedName(&module, &generatedIdentity,
                                             "ignored"), "cg_entry"));
    assert(!strcmp(HlslAllocateGeneratedName(&module,
                                             &secondGeneratedIdentity,
                                             "cg_entry"), "cg_entry_1"));
    TestReservedNames();
    TestTypeRegisterSpans();
    TestDeclarationQualifiers();
    TestModuleWriter();
    TestModuleValidationRejectsUnownedEntry();
    TestNamesAndAllocationFailure();
    TestTypesAndLists();
    TestBindingBanks();
    TestAggregateAndSamplerBindings();
    TestDefaultBindingAndResourceFailure();
    TestCyclicBindingListIsRejected();
    TestAggregateBankClassification();
    TestBindingDiagnosticArithmetic();
    TestOversizedMixedArrayPreflight();
    TestDefaultValidationIsTransactional();
    TestPublicBindingNames();
    return 0;
}

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
    profile = HlslProfile_hlslv;
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

static HlslExpr *NewFloatVectorConstruct(HlslModule *module, int length)
{
    HlslExpr *construct;
    HlslExpr *component;
    int i;

    construct = HlslNewExpr(module, HLSL_EXPR_CONSTRUCT,
                            HlslNumericType(HLSL_BASE_FLOAT, length));
    if (construct == NULL)
        return NULL;
    for (i = 0; i < length; i++) {
        component = HlslNewExpr(module, HLSL_EXPR_FLOAT,
                                HlslNumericType(HLSL_BASE_FLOAT, 1));
        if (component == NULL)
            return NULL;
        HlslAppendExpr(&construct->u.construct.arguments, component);
    }
    return construct;
}

static void TestModuleValidationRejectsUnownedEntry(void)
{
    HlslModule module;
    HlslProfileDesc profile;
    HlslType voidType;
    HlslType floatType;
    HlslType float2Type;
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
    HlslExpr *firstArgument;
    HlslExpr *secondArgument;
    HlslStmt *statement;
    HlslStmt *helperStatement;

    HlslInitModule(&module, HLSL_STAGE_VERTEX, TestAlloc, NULL);
    profile = HlslProfile_hlslv;
    voidType = HlslNumericType(HLSL_BASE_VOID, 0);
    floatType = HlslNumericType(HLSL_BASE_FLOAT, 1);
    float2Type = HlslNumericType(HLSL_BASE_FLOAT, 2);
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
    wrapper = HlslNewFunction(&module, outputStruct->type, "main");
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

    module.errors = 0;
    module.errorKind = HLSL_ERROR_NONE;
    module.errorReason = NULL;
    helperCall = HlslNewExpr(&module, HLSL_EXPR_CALL, float2Type);
    firstArgument = NewFloatVectorConstruct(&module, 2);
    secondArgument = NewFloatVectorConstruct(&module, 2);
    helperStatement = HlslNewStmt(&module, HLSL_STMT_EXPRESSION);
    assert(helperCall != NULL && firstArgument != NULL &&
           secondArgument != NULL && helperStatement != NULL);
    helperCall->u.call.name = "cross";
    helperCall->u.call.builtin = HLSL_BUILTIN_CROSS;
    helperCall->u.call.arguments = firstArgument;
    HlslAppendExpr(&helperCall->u.call.arguments, secondArgument);
    helperStatement->u.expression = helperCall;
    entry->body = helperStatement;
    assert(!HlslValidateModule(&module, &profile));
    assert(module.errorKind == HLSL_ERROR_INVALID_IR);

    module.errors = 0;
    module.errorKind = HLSL_ERROR_NONE;
    module.errorReason = NULL;
    helperCall = HlslNewExpr(&module, HLSL_EXPR_CALL, floatType);
    firstArgument = NewFloatVectorConstruct(&module, 2);
    secondArgument = NewFloatVectorConstruct(&module, 2);
    helperStatement = HlslNewStmt(&module, HLSL_STMT_EXPRESSION);
    assert(helperCall != NULL && firstArgument != NULL &&
           secondArgument != NULL && helperStatement != NULL);
    helperCall->u.call.name = "dot";
    helperCall->u.call.arguments = firstArgument;
    HlslAppendExpr(&helperCall->u.call.arguments, secondArgument);
    helperStatement->u.expression = helperCall;
    entry->body = helperStatement;
    assert(!HlslValidateModule(&module, &profile));
    assert(module.errorKind == HLSL_ERROR_INVALID_IR);

    module.errors = 0;
    module.errorKind = HLSL_ERROR_NONE;
    module.errorReason = NULL;
    helperCall->u.call.builtin = HLSL_BUILTIN_DOT;
    assert(HlslValidateModule(&module, &profile));
}

static void TestLogicalBindingSurvivesLegalizeAndAllocation(void)
{
    HlslModule module;
    HlslBinding *binding;
    HlslDecl *logical;
    HlslFunction *entry;
    HlslExpr *reference;
    HlslStmt *statement;
    HlslType floatType;
    HlslType voidType;
    int bindingIdentity;
    int reservedIdentity;

    HlslInitModule(&module, HLSL_STAGE_VERTEX, TestAlloc, NULL);
    floatType = HlslNumericType(HLSL_BASE_FLOAT, 1);
    voidType = HlslNumericType(HLSL_BASE_VOID, 0);
    binding = HlslNewBinding(&module, HLSL_STORAGE_UNIFORM, floatType,
                             "value", NULL);
    logical = HlslNewDecl(&module, HLSL_STORAGE_UNIFORM, floatType,
                          "cg_value");
    entry = HlslNewFunction(&module, voidType, "cg_entry");
    reference = HlslNewExpr(&module, HLSL_EXPR_SYMBOL, floatType);
    statement = HlslNewStmt(&module, HLSL_STMT_EXPRESSION);
    assert(binding != NULL && logical != NULL && entry != NULL &&
           reference != NULL && statement != NULL);
    logical->identity = &bindingIdentity;
    binding->declaration = logical;
    module.bindings = binding;
    entry->isEntry = 1;
    reference->u.symbol = logical;
    statement->u.expression = reference;
    entry->body = statement;
    module.entry = entry;
    module.functions = entry;
    assert(!strcmp(HlslAllocateGeneratedName(&module, &reservedIdentity,
                                             "cg_value"), "cg_value"));

    assert(HlslLegalizeModule(&module, &HlslProfile_hlslv));
    assert(module.globals == NULL);
    assert(reference->u.symbol == logical);
    assert(!strcmp(logical->name, "cg_value"));

    assert(HlslAllocateBindings(&module, &HlslProfile_hlslv));
    assert(module.globals == logical);
    assert(reference->u.symbol == logical);
    assert(!strcmp(logical->name, "cg_value_1"));
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

static void TestLocatedExpression(void)
{
    HlslModule module;
    HlslExpr *expression;
    HlslLoc loc;
    HlslType type;

    HlslInitModule(&module, HLSL_STAGE_VERTEX, TestAlloc, NULL);
    type = HlslNumericType(HLSL_BASE_FLOAT, 1);
    loc.file = 7;
    loc.line = 23;
    expression = HlslNewLocatedExpr(&module, HLSL_EXPR_FLOAT, type, &loc);
    assert(expression != NULL);
    assert(expression->loc.file == 7 && expression->loc.line == 23);
}

static void ExpectBuiltin(HlslStage stage, const char *name,
                          HlslBuiltin expected, HlslType result,
                          const HlslType *params, int count)
{
    HlslBuiltin actual;

    actual = HlslLookupBuiltin(stage, name, &result, params, count);
    assert(actual == expected);
    assert(!strcmp(HlslBuiltinSpelling(actual), name));
}

static void TestBuiltinSignatures(void)
{
    HlslType f1;
    HlslType f2;
    HlslType f3;
    HlslType f4;
    HlslType i3;
    HlslType b1;
    HlslType b3;
    HlslType m34;
    HlslType params[4];

    f1 = HlslNumericType(HLSL_BASE_FLOAT, 1);
    f2 = HlslNumericType(HLSL_BASE_FLOAT, 2);
    f3 = HlslNumericType(HLSL_BASE_FLOAT, 3);
    f4 = HlslNumericType(HLSL_BASE_FLOAT, 4);
    i3 = HlslNumericType(HLSL_BASE_INT, 3);
    b1 = HlslNumericType(HLSL_BASE_BOOL, 1);
    b3 = HlslNumericType(HLSL_BASE_BOOL, 3);
    m34 = HlslMatrixType(3, 4);

    params[0] = m34; params[1] = f4;
    ExpectBuiltin(HLSL_STAGE_VERTEX, "mul", HLSL_BUILTIN_MUL,
                  f3, params, 2);
    params[0] = f3; params[1] = f3;
    ExpectBuiltin(HLSL_STAGE_VERTEX, "dot", HLSL_BUILTIN_DOT,
                  f1, params, 2);
    ExpectBuiltin(HLSL_STAGE_VERTEX, "cross", HLSL_BUILTIN_CROSS,
                  f3, params, 2);
    params[0] = f3;
    ExpectBuiltin(HLSL_STAGE_VERTEX, "normalize", HLSL_BUILTIN_NORMALIZE,
                  f3, params, 1);
    params[1] = f3;
    ExpectBuiltin(HLSL_STAGE_VERTEX, "reflect", HLSL_BUILTIN_REFLECT,
                  f3, params, 2);
    params[2] = f1;
    ExpectBuiltin(HLSL_STAGE_VERTEX, "refract", HLSL_BUILTIN_REFRACT,
                  f3, params, 3);
    ExpectBuiltin(HLSL_STAGE_VERTEX, "length", HLSL_BUILTIN_LENGTH,
                  f1, params, 1);
    ExpectBuiltin(HLSL_STAGE_VERTEX, "distance", HLSL_BUILTIN_DISTANCE,
                  f1, params, 2);

    params[0] = f3; params[1] = f1;
    ExpectBuiltin(HLSL_STAGE_VERTEX, "min", HLSL_BUILTIN_MIN,
                  f3, params, 2);
    ExpectBuiltin(HLSL_STAGE_VERTEX, "max", HLSL_BUILTIN_MAX,
                  f3, params, 2);
    params[2] = f3;
    ExpectBuiltin(HLSL_STAGE_VERTEX, "clamp", HLSL_BUILTIN_CLAMP,
                  f3, params, 3);

#define EXPECT_UNARY(name, id) \
    params[0] = f3; \
    ExpectBuiltin(HLSL_STAGE_VERTEX, name, id, f3, params, 1)
    EXPECT_UNARY("abs", HLSL_BUILTIN_ABS);
    EXPECT_UNARY("sign", HLSL_BUILTIN_SIGN);
    EXPECT_UNARY("floor", HLSL_BUILTIN_FLOOR);
    EXPECT_UNARY("ceil", HLSL_BUILTIN_CEIL);
    EXPECT_UNARY("round", HLSL_BUILTIN_ROUND);
    EXPECT_UNARY("trunc", HLSL_BUILTIN_TRUNC);
    EXPECT_UNARY("sqrt", HLSL_BUILTIN_SQRT);
    EXPECT_UNARY("rsqrt", HLSL_BUILTIN_RSQRT);
    EXPECT_UNARY("exp", HLSL_BUILTIN_EXP);
    EXPECT_UNARY("exp2", HLSL_BUILTIN_EXP2);
    EXPECT_UNARY("log", HLSL_BUILTIN_LOG);
    EXPECT_UNARY("log2", HLSL_BUILTIN_LOG2);
    EXPECT_UNARY("sin", HLSL_BUILTIN_SIN);
    EXPECT_UNARY("cos", HLSL_BUILTIN_COS);
    EXPECT_UNARY("tan", HLSL_BUILTIN_TAN);
    EXPECT_UNARY("asin", HLSL_BUILTIN_ASIN);
    EXPECT_UNARY("acos", HLSL_BUILTIN_ACOS);
    EXPECT_UNARY("atan", HLSL_BUILTIN_ATAN);
    EXPECT_UNARY("sinh", HLSL_BUILTIN_SINH);
    EXPECT_UNARY("cosh", HLSL_BUILTIN_COSH);
    EXPECT_UNARY("tanh", HLSL_BUILTIN_TANH);
    EXPECT_UNARY("frac", HLSL_BUILTIN_FRAC);
    EXPECT_UNARY("saturate", HLSL_BUILTIN_SATURATE);
#undef EXPECT_UNARY

    params[0] = i3;
    ExpectBuiltin(HLSL_STAGE_VERTEX, "abs", HLSL_BUILTIN_ABS,
                  i3, params, 1);
    ExpectBuiltin(HLSL_STAGE_VERTEX, "sign", HLSL_BUILTIN_SIGN,
                  i3, params, 1);
    params[0] = f3; params[1] = f1;
#define EXPECT_BINARY(name, id) \
    ExpectBuiltin(HLSL_STAGE_VERTEX, name, id, f3, params, 2)
    EXPECT_BINARY("pow", HLSL_BUILTIN_POW);
    EXPECT_BINARY("atan2", HLSL_BUILTIN_ATAN2);
    EXPECT_BINARY("fmod", HLSL_BUILTIN_FMOD);
    EXPECT_BINARY("step", HLSL_BUILTIN_STEP);
#undef EXPECT_BINARY
    params[2] = f3;
    ExpectBuiltin(HLSL_STAGE_VERTEX, "lerp", HLSL_BUILTIN_LERP,
                  f3, params, 3);
    ExpectBuiltin(HLSL_STAGE_VERTEX, "smoothstep", HLSL_BUILTIN_SMOOTHSTEP,
                  f3, params, 3);

    params[0] = b3;
    ExpectBuiltin(HLSL_STAGE_VERTEX, "any", HLSL_BUILTIN_ANY,
                  b1, params, 1);
    ExpectBuiltin(HLSL_STAGE_VERTEX, "all", HLSL_BUILTIN_ALL,
                  b1, params, 1);
    params[0] = f2;
    ExpectBuiltin(HLSL_STAGE_PIXEL, "ddx", HLSL_BUILTIN_DDX,
                  f2, params, 1);
    ExpectBuiltin(HLSL_STAGE_PIXEL, "ddy", HLSL_BUILTIN_DDY,
                  f2, params, 1);
    assert(HlslLookupBuiltin(HLSL_STAGE_VERTEX, "ddx", &f2,
                            params, 1) == HLSL_BUILTIN_NONE);
    params[0] = f2; params[1] = f2;
    assert(HlslLookupBuiltin(HLSL_STAGE_VERTEX, "cross", &f2,
                            params, 2) == HLSL_BUILTIN_NONE);
    assert(HlslLookupBuiltin(HLSL_STAGE_VERTEX, "unknown", &f2,
                            params, 2) == HLSL_BUILTIN_NONE);
    assert(HlslIsBuiltinName("smoothstep"));
    assert(!HlslIsBuiltinName("user_smoothstep"));
    assert(HlslBuiltinLoweringKind(HLSL_BUILTIN_RSQRT) ==
           HLSL_BUILTIN_LOWER_HELPER);
    assert(HlslBuiltinLoweringKind(HLSL_BUILTIN_SATURATE) ==
           HLSL_BUILTIN_LOWER_EXPANSION);
}

static void TestTextureBuiltinSignatures(void)
{
    static const char *baseNames[] = {
        "tex1D", "tex2D", "tex3D", "texCUBE"
    };
    static const char *projNames[] = {
        "tex1Dproj", "tex2Dproj", "tex3Dproj", "texCUBEproj"
    };
    static const char *biasNames[] = {
        "tex1Dbias", "tex2Dbias", "tex3Dbias", "texCUBEbias"
    };
    static const char *lodNames[] = {
        "tex1Dlod", "tex2Dlod", "tex3Dlod", "texCUBElod"
    };
    static const char *gradNames[] = {
        "tex1Dgrad", "tex2Dgrad", "tex3Dgrad", "texCUBEgrad"
    };
    static const char *halfNames[] = {
        "h4tex1D", "h4tex2D", "h4tex3D", "h4texCUBE"
    };
    static const char *fixedNames[] = {
        "x4tex1D", "x4tex2D", "x4tex3D", "x4texCUBE"
    };
    static const char *halfProjNames[] = {
        "h4tex1Dproj", "h4tex2Dproj", "h4tex3Dproj", "h4texCUBEproj"
    };
    static const char *fixedProjNames[] = {
        "x4tex1Dproj", "x4tex2Dproj", "x4tex3Dproj", "x4texCUBEproj"
    };
    static const HlslBuiltin baseIds[] = {
        HLSL_BUILTIN_TEX1D, HLSL_BUILTIN_TEX2D,
        HLSL_BUILTIN_TEX3D, HLSL_BUILTIN_TEXCUBE
    };
    static const HlslBuiltin projIds[] = {
        HLSL_BUILTIN_TEX1DPROJ, HLSL_BUILTIN_TEX2DPROJ,
        HLSL_BUILTIN_TEX3DPROJ, HLSL_BUILTIN_TEXCUBEPROJ
    };
    static const HlslBuiltin biasIds[] = {
        HLSL_BUILTIN_TEX1DBIAS, HLSL_BUILTIN_TEX2DBIAS,
        HLSL_BUILTIN_TEX3DBIAS, HLSL_BUILTIN_TEXCUBEBIAS
    };
    static const HlslBuiltin lodIds[] = {
        HLSL_BUILTIN_TEX1DLOD, HLSL_BUILTIN_TEX2DLOD,
        HLSL_BUILTIN_TEX3DLOD, HLSL_BUILTIN_TEXCUBELOD
    };
    static const HlslBuiltin gradIds[] = {
        HLSL_BUILTIN_TEX1DGRAD, HLSL_BUILTIN_TEX2DGRAD,
        HLSL_BUILTIN_TEX3DGRAD, HLSL_BUILTIN_TEXCUBEGRAD
    };
    static const HlslSourceBase samplerBases[] = {
        HLSL_SOURCE_BASE_SAMPLER1D, HLSL_SOURCE_BASE_SAMPLER2D,
        HLSL_SOURCE_BASE_SAMPLER3D, HLSL_SOURCE_BASE_SAMPLERCUBE
    };
    static const HlslBase normalizedSamplerBases[] = {
        HLSL_BASE_SAMPLER1D, HLSL_BASE_SAMPLER2D,
        HLSL_BASE_SAMPLER3D, HLSL_BASE_SAMPLERCUBE
    };
    static const int coordWidths[] = { 1, 2, 3, 3 };
    HlslType f1;
    HlslType f2;
    HlslType f3;
    HlslType f4;
    HlslType s1;
    HlslType s2;
    HlslType s3;
    HlslType sc;
    HlslType params[5];
    HlslType coord;
    HlslType wrongCoord;
    HlslSourceType sf1;
    HlslSourceType sf2;
    HlslSourceType sf4;
    HlslSourceType ss2;
    HlslSourceType sourceParams[5];
    HlslSourceType sourceCoord;
    HlslSourceType sourceWrongCoord;
    HlslSourceType sourceHalf4;
    HlslSourceType sourceFixed4;
    int i;

    f1 = HlslNumericType(HLSL_BASE_FLOAT, 1);
    f2 = HlslNumericType(HLSL_BASE_FLOAT, 2);
    f3 = HlslNumericType(HLSL_BASE_FLOAT, 3);
    f4 = HlslNumericType(HLSL_BASE_FLOAT, 4);
    s1 = HlslNumericType(HLSL_BASE_SAMPLER1D, 1);
    s2 = HlslNumericType(HLSL_BASE_SAMPLER2D, 1);
    s3 = HlslNumericType(HLSL_BASE_SAMPLER3D, 1);
    sc = HlslNumericType(HLSL_BASE_SAMPLERCUBE, 1);

    params[0] = s1; params[1] = f1;
    assert(HlslLookupBuiltin(HLSL_STAGE_PIXEL, "tex1D", &f4,
                            params, 2) == HLSL_BUILTIN_TEX1D);
    params[0] = s2; params[1] = f2;
    assert(HlslLookupBuiltin(HLSL_STAGE_PIXEL, "tex2D", &f4,
                            params, 2) == HLSL_BUILTIN_TEX2D);
    params[0] = s3; params[1] = f3;
    assert(HlslLookupBuiltin(HLSL_STAGE_PIXEL, "tex3D", &f4,
                            params, 2) == HLSL_BUILTIN_TEX3D);
    params[0] = sc; params[1] = f3;
    assert(HlslLookupBuiltin(HLSL_STAGE_PIXEL, "texCUBE", &f4,
                            params, 2) == HLSL_BUILTIN_TEXCUBE);

    params[0] = s2; params[1] = f4;
    assert(HlslLookupBuiltin(HLSL_STAGE_PIXEL, "tex2Dproj", &f4,
                            params, 2) == HLSL_BUILTIN_TEX2DPROJ);
    assert(HlslLookupBuiltin(HLSL_STAGE_PIXEL, "tex2Dbias", &f4,
                            params, 2) == HLSL_BUILTIN_TEX2DBIAS);
    assert(HlslLookupBuiltin(HLSL_STAGE_PIXEL, "tex2Dlod", &f4,
                            params, 2) == HLSL_BUILTIN_TEX2DLOD);
    assert(HlslLookupBuiltin(HLSL_STAGE_VERTEX, "tex2Dlod", &f4,
                            params, 2) == HLSL_BUILTIN_TEX2DLOD);
    assert(HlslLookupBuiltin(HLSL_STAGE_VERTEX, "tex2D", &f4,
                            params, 2) == HLSL_BUILTIN_NONE);
    params[1] = f2; params[2] = f2; params[3] = f2;
    assert(HlslLookupBuiltin(HLSL_STAGE_PIXEL, "tex2D", &f4,
                            params, 4) == HLSL_BUILTIN_TEX2DGRAD);
    assert(!strcmp(HlslBuiltinSpelling(HLSL_BUILTIN_TEX2DGRAD),
                   "tex2Dgrad"));
    assert(HlslLookupBuiltin(HLSL_STAGE_PIXEL, "tex2D", &f4,
                            params, 3) == HLSL_BUILTIN_NONE);
    params[4] = f2;
    assert(HlslLookupBuiltin(HLSL_STAGE_PIXEL, "tex2D", &f4,
                            params, 5) == HLSL_BUILTIN_NONE);
    params[1] = f3;
    assert(HlslLookupBuiltin(HLSL_STAGE_PIXEL, "tex2D", &f4,
                            params, 2) == HLSL_BUILTIN_NONE);

    sf1 = HlslSourceScalarType(HLSL_SOURCE_BASE_FLOAT);
    sf2 = HlslSourceVectorType(HLSL_SOURCE_BASE_FLOAT, 2);
    sf4 = HlslSourceVectorType(HLSL_SOURCE_BASE_FLOAT, 4);
    ss2 = HlslSourceScalarType(HLSL_SOURCE_BASE_SAMPLER2D);
    assert(!strcmp(HlslSourceTypeName(&ss2), "sampler2D"));
    sourceCoord = HlslSourceVectorType(HLSL_SOURCE_BASE_SAMPLER2D, 2);
    assert(HlslSourceTypeName(&sourceCoord) == NULL);
    sourceParams[0] = ss2; sourceParams[1] = sf2;
    assert(HlslLookupSourceBuiltin(HLSL_STAGE_PIXEL, "tex2D", &sf4,
        sourceParams, 2) == HLSL_BUILTIN_TEX2D);
    sourceParams[2] = sf2; sourceParams[3] = sf2;
    assert(HlslLookupSourceBuiltin(HLSL_STAGE_PIXEL, "tex2D", &sf4,
        sourceParams, 4) == HLSL_BUILTIN_TEX2DGRAD);
    sourceParams[4] = sf2;
    assert(HlslLookupSourceBuiltin(HLSL_STAGE_PIXEL, "tex2D", &sf4,
        sourceParams, 5) == HLSL_BUILTIN_NONE);
    sourceParams[1] = sf4;
    assert(HlslLookupSourceBuiltin(HLSL_STAGE_VERTEX, "tex2Dlod", &sf4,
        sourceParams, 2) == HLSL_BUILTIN_TEX2DLOD);
    sourceParams[1] = sf1;
    assert(HlslLookupSourceBuiltin(HLSL_STAGE_PIXEL, "tex2D", &sf4,
        sourceParams, 2) == HLSL_BUILTIN_NONE);
    assert(HlslBuiltinIsTexture(HLSL_BUILTIN_TEX2D));
    assert(!HlslBuiltinIsTexture(HLSL_BUILTIN_DOT));
    assert(HlslIsBuiltinName("texRECT"));

    sourceHalf4 = HlslSourceVectorType(HLSL_SOURCE_BASE_HALF, 4);
    sourceFixed4 = HlslSourceVectorType(HLSL_SOURCE_BASE_FIXED, 4);
    for (i = 0; i < 4; i++) {
        assert(HlslBuiltinIsTexture(baseIds[i]));
        assert(HlslBuiltinTextureForm(baseIds[i]) == HLSL_TEXTURE_IMPLICIT);
        assert(HlslBuiltinTextureForm(projIds[i]) == HLSL_TEXTURE_PROJECTED);
        assert(HlslBuiltinTextureForm(biasIds[i]) == HLSL_TEXTURE_BIAS);
        assert(HlslBuiltinTextureForm(lodIds[i]) == HLSL_TEXTURE_LOD);
        assert(HlslBuiltinTextureForm(gradIds[i]) == HLSL_TEXTURE_GRADIENT);
        assert(HlslBuiltinSamplerBase(baseIds[i]) ==
               normalizedSamplerBases[i]);
        assert(HlslBuiltinTextureCoordWidth(baseIds[i]) == coordWidths[i]);
        assert(HlslBuiltinTextureCoordWidth(projIds[i]) == 4);
        assert(HlslBuiltinTextureCoordWidth(biasIds[i]) == 4);
        assert(HlslBuiltinTextureCoordWidth(lodIds[i]) == 4);
        assert(HlslBuiltinTextureCoordWidth(gradIds[i]) == coordWidths[i]);
        assert(!strcmp(HlslBuiltinSpelling(baseIds[i]), baseNames[i]));
        assert(!strcmp(HlslBuiltinSpelling(projIds[i]), projNames[i]));
        assert(!strcmp(HlslBuiltinSpelling(biasIds[i]), biasNames[i]));
        assert(!strcmp(HlslBuiltinSpelling(lodIds[i]), lodNames[i]));
        assert(!strcmp(HlslBuiltinSpelling(gradIds[i]), gradNames[i]));
        params[0] = HlslNumericType(normalizedSamplerBases[i], 1);
        coord = HlslNumericType(HLSL_BASE_FLOAT, coordWidths[i]);
        wrongCoord = HlslNumericType(HLSL_BASE_FLOAT,
                                     coordWidths[i] == 2 ? 3 : 2);
        params[1] = coord;
        assert(HlslBuiltinAccepts(HLSL_STAGE_PIXEL, baseIds[i],
                                  &f4, params, 2));
        params[1] = wrongCoord;
        assert(!HlslBuiltinAccepts(HLSL_STAGE_PIXEL, baseIds[i],
                                   &f4, params, 2));
        params[1] = f4;
        assert(HlslBuiltinAccepts(HLSL_STAGE_PIXEL, projIds[i],
                                  &f4, params, 2));
        assert(HlslBuiltinAccepts(HLSL_STAGE_PIXEL, biasIds[i],
                                  &f4, params, 2));
        assert(HlslBuiltinAccepts(HLSL_STAGE_PIXEL, lodIds[i],
                                  &f4, params, 2));
        params[1] = f3;
        assert(!HlslBuiltinAccepts(HLSL_STAGE_PIXEL, projIds[i],
                                   &f4, params, 2));
        assert(!HlslBuiltinAccepts(HLSL_STAGE_PIXEL, biasIds[i],
                                   &f4, params, 2));
        assert(!HlslBuiltinAccepts(HLSL_STAGE_PIXEL, lodIds[i],
                                   &f4, params, 2));
        params[1] = coord;
        params[2] = coord;
        params[3] = coord;
        assert(HlslBuiltinAccepts(HLSL_STAGE_PIXEL, gradIds[i],
                                  &f4, params, 4));
        params[3] = wrongCoord;
        assert(!HlslBuiltinAccepts(HLSL_STAGE_PIXEL, gradIds[i],
                                   &f4, params, 4));
        params[1] = wrongCoord;
        params[3] = coord;
        assert(!HlslBuiltinAccepts(HLSL_STAGE_PIXEL, gradIds[i],
                                   &f4, params, 4));
        assert(!HlslBuiltinAccepts(HLSL_STAGE_VERTEX, baseIds[i],
                                   &f4, params, 2));
        sourceParams[0] = HlslSourceScalarType(samplerBases[i]);
        sourceCoord = coordWidths[i] == 1 ? sf1 :
            HlslSourceVectorType(HLSL_SOURCE_BASE_FLOAT, coordWidths[i]);
        sourceWrongCoord = HlslSourceVectorType(HLSL_SOURCE_BASE_FLOAT,
                                                coordWidths[i] == 2 ? 3 : 2);
        sourceParams[1] = sourceCoord;
        assert(HlslLookupSourceBuiltin(HLSL_STAGE_PIXEL, baseNames[i],
            &sf4, sourceParams, 2) == baseIds[i]);
        assert(HlslLookupSourceBuiltin(HLSL_STAGE_PIXEL, halfNames[i],
            &sourceHalf4, sourceParams, 2) == baseIds[i]);
        assert(HlslLookupSourceBuiltin(HLSL_STAGE_PIXEL, fixedNames[i],
            &sourceFixed4, sourceParams, 2) == baseIds[i]);
        sourceParams[2] = sourceCoord;
        sourceParams[3] = sourceCoord;
        assert(HlslLookupSourceBuiltin(HLSL_STAGE_PIXEL, baseNames[i],
            &sf4, sourceParams, 4) == gradIds[i]);
        sourceParams[3] = sourceWrongCoord;
        assert(HlslLookupSourceBuiltin(HLSL_STAGE_PIXEL, baseNames[i],
            &sf4, sourceParams, 4) == HLSL_BUILTIN_NONE);
        sourceParams[1] = sourceWrongCoord;
        sourceParams[3] = sourceCoord;
        assert(HlslLookupSourceBuiltin(HLSL_STAGE_PIXEL, baseNames[i],
            &sf4, sourceParams, 4) == HLSL_BUILTIN_NONE);
        sourceParams[1] = sf4;
        assert(HlslLookupSourceBuiltin(HLSL_STAGE_PIXEL, projNames[i],
            &sf4, sourceParams, 2) == projIds[i]);
        assert(HlslLookupSourceBuiltin(HLSL_STAGE_PIXEL, halfProjNames[i],
            &sourceHalf4, sourceParams, 2) == projIds[i]);
        assert(HlslLookupSourceBuiltin(HLSL_STAGE_PIXEL, fixedProjNames[i],
            &sourceFixed4, sourceParams, 2) == projIds[i]);
        assert(HlslLookupSourceBuiltin(HLSL_STAGE_PIXEL, biasNames[i],
            &sf4, sourceParams, 2) == biasIds[i]);
        assert(HlslLookupSourceBuiltin(HLSL_STAGE_PIXEL, lodNames[i],
            &sf4, sourceParams, 2) == lodIds[i]);
        assert(HlslLookupSourceBuiltin(HLSL_STAGE_VERTEX, lodNames[i],
            &sf4, sourceParams, 2) == lodIds[i]);
        sourceParams[1] = sourceWrongCoord;
        assert(HlslLookupSourceBuiltin(HLSL_STAGE_PIXEL, baseNames[i],
            &sf4, sourceParams, 2) == HLSL_BUILTIN_NONE);
        assert(HlslLookupSourceBuiltin(HLSL_STAGE_PIXEL, projNames[i],
            &sf4, sourceParams, 2) == HLSL_BUILTIN_NONE);
        assert(HlslLookupSourceBuiltin(HLSL_STAGE_PIXEL, biasNames[i],
            &sf4, sourceParams, 2) == HLSL_BUILTIN_NONE);
        assert(HlslLookupSourceBuiltin(HLSL_STAGE_PIXEL, lodNames[i],
            &sf4, sourceParams, 2) == HLSL_BUILTIN_NONE);
        sourceParams[1] = sourceCoord;
        assert(HlslLookupSourceBuiltin(HLSL_STAGE_VERTEX, baseNames[i],
            &sf4, sourceParams, 2) == HLSL_BUILTIN_NONE);
    }
    assert(HlslIsTextureName("texRECT"));
    assert(HlslIsTextureName("h4texRECTproj"));
    assert(!HlslIsTextureName("texture2D"));
}

static void ExpectTextureLegalization(HlslStage stage,
                                      HlslBuiltin builtin,
                                      HlslType samplerType,
                                      HlslType coordType, int accepted)
{
    HlslModule module;
    HlslFunction *entry;
    HlslDecl *sampler;
    HlslDecl *coord;
    HlslExpr *call;
    HlslExpr *argument;
    HlslStmt *statement;
    HlslType voidType;
    HlslType float4Type;
    const HlslProfileDesc *profile;

    HlslInitModule(&module, stage, TestAlloc, NULL);
    profile = stage == HLSL_STAGE_VERTEX ? &HlslProfile_hlslv :
                                          &HlslProfile_hlslf;
    voidType = HlslNumericType(HLSL_BASE_VOID, 0);
    float4Type = HlslNumericType(HLSL_BASE_FLOAT, 4);
    entry = HlslNewFunction(&module, voidType, "cg_entry");
    sampler = HlslNewDecl(&module, HLSL_STORAGE_SAMPLER,
                          samplerType, "image");
    coord = HlslNewDecl(&module, HLSL_STORAGE_NONE, coordType, "coord");
    call = HlslNewLocatedExpr(&module, HLSL_EXPR_CALL, float4Type, NULL);
    statement = HlslNewStmt(&module, HLSL_STMT_EXPRESSION);
    assert(entry != NULL && sampler != NULL && coord != NULL &&
           call != NULL && statement != NULL);
    HlslAppendDecl(&entry->parameters, sampler);
    HlslAppendDecl(&entry->parameters, coord);
    argument = HlslNewExpr(&module, HLSL_EXPR_SYMBOL, samplerType);
    assert(argument != NULL);
    argument->u.symbol = sampler;
    HlslAppendExpr(&call->u.call.arguments, argument);
    argument = HlslNewExpr(&module, HLSL_EXPR_SYMBOL, coordType);
    assert(argument != NULL);
    argument->u.symbol = coord;
    HlslAppendExpr(&call->u.call.arguments, argument);
    call->u.call.name = HlslBuiltinSpelling(builtin);
    call->u.call.builtin = builtin;
    statement->u.expression = call;
    entry->body = statement;
    entry->isEntry = 1;
    module.entry = entry;
    module.functions = entry;
    if (accepted) {
        assert(HlslLegalizeModule(&module, profile));
    } else {
        assert(!HlslLegalizeModule(&module, profile));
        assert(module.errorKind == HLSL_ERROR_SAMPLER);
    }
}

static void TestTextureAndSamplerRejections(void)
{
    HlslModule module;
    HlslBinding *firstBinding;
    HlslBinding *secondBinding;
    HlslFunction *entry;
    HlslDecl *first;
    HlslDecl *second;
    HlslExpr *left;
    HlslExpr *right;
    HlslExpr *assignment;
    HlslStmt *statement;
    HlslType voidType;
    HlslType f2;
    HlslType f3;
    HlslType f4;
    HlslType s2;
    HlslType s3;
    HlslType cyclicTypes[2];

    voidType = HlslNumericType(HLSL_BASE_VOID, 0);
    f2 = HlslNumericType(HLSL_BASE_FLOAT, 2);
    f3 = HlslNumericType(HLSL_BASE_FLOAT, 3);
    f4 = HlslNumericType(HLSL_BASE_FLOAT, 4);
    s2 = HlslNumericType(HLSL_BASE_SAMPLER2D, 1);
    s3 = HlslNumericType(HLSL_BASE_SAMPLER3D, 1);

    ExpectTextureLegalization(HLSL_STAGE_PIXEL, HLSL_BUILTIN_TEX2D,
                              s2, f2, 1);
    ExpectTextureLegalization(HLSL_STAGE_VERTEX, HLSL_BUILTIN_TEX2D,
                              s2, f2, 0);
    ExpectTextureLegalization(HLSL_STAGE_PIXEL, HLSL_BUILTIN_TEX2D,
                              s2, f3, 0);
    ExpectTextureLegalization(HLSL_STAGE_VERTEX, HLSL_BUILTIN_TEX2DLOD,
                              s2, f4, 1);

    HlslInitModule(&module, HLSL_STAGE_PIXEL, TestAlloc, NULL);
    entry = HlslNewFunction(&module, voidType, "cg_entry");
    first = HlslNewDecl(&module, HLSL_STORAGE_SAMPLER, s2, "first");
    second = HlslNewDecl(&module, HLSL_STORAGE_SAMPLER, s2, "second");
    left = HlslNewExpr(&module, HLSL_EXPR_SYMBOL, s2);
    right = HlslNewExpr(&module, HLSL_EXPR_SYMBOL, s2);
    assignment = HlslNewExpr(&module, HLSL_EXPR_BINARY, s2);
    statement = HlslNewStmt(&module, HLSL_STMT_EXPRESSION);
    assert(entry != NULL && first != NULL && second != NULL &&
           left != NULL && right != NULL && assignment != NULL &&
           statement != NULL);
    HlslAppendDecl(&entry->parameters, first);
    HlslAppendDecl(&entry->parameters, second);
    left->u.symbol = first;
    right->u.symbol = second;
    assignment->u.binary.op = HLSL_OP_ASSIGN;
    assignment->u.binary.left = left;
    assignment->u.binary.right = right;
    statement->u.expression = assignment;
    entry->body = statement;
    entry->isEntry = 1;
    module.entry = entry;
    module.functions = entry;
    assert(!HlslLegalizeModule(&module, &HlslProfile_hlslf));
    assert(module.errorKind == HLSL_ERROR_SAMPLER);
    assert(!strcmp(module.errorReason, "sampler assignment"));

    HlslInitModule(&module, HLSL_STAGE_PIXEL, TestAlloc, NULL);
    entry = HlslNewFunction(&module, voidType, "cg_entry");
    first = HlslNewDecl(&module, HLSL_STORAGE_NONE, s2, "localImage");
    assert(entry != NULL && first != NULL);
    entry->locals = first;
    entry->isEntry = 1;
    module.entry = entry;
    module.functions = entry;
    assert(!HlslLegalizeModule(&module, &HlslProfile_hlslf));
    assert(module.errorKind == HLSL_ERROR_SAMPLER);
    assert(!strcmp(module.errorReason, "local sampler"));

    HlslInitModule(&module, HLSL_STAGE_PIXEL, TestAlloc, NULL);
    entry = HlslNewFunction(&module, voidType, "cg_entry");
    first = HlslNewDecl(&module, HLSL_STORAGE_NONE, s2, "image");
    assert(entry != NULL && first != NULL);
    entry->parameters = first;
    entry->isEntry = 1;
    module.entry = entry;
    module.functions = entry;
    assert(!HlslLegalizeModule(&module, &HlslProfile_hlslf));
    assert(module.errorKind == HLSL_ERROR_SAMPLER);
    assert(!strcmp(module.errorReason, "local sampler"));

    HlslInitModule(&module, HLSL_STAGE_PIXEL, TestAlloc, NULL);
    entry = HlslNewFunction(&module, s2, "cg_entry");
    assert(entry != NULL);
    entry->isEntry = 1;
    module.entry = entry;
    module.functions = entry;
    assert(!HlslLegalizeModule(&module, &HlslProfile_hlslf));
    assert(module.errorKind == HLSL_ERROR_SAMPLER);
    assert(!strcmp(module.errorReason, "sampler return"));

    HlslInitModule(&module, HLSL_STAGE_PIXEL, TestAlloc, NULL);
    firstBinding = HlslNewBinding(&module, HLSL_STORAGE_SAMPLER, s2,
                                  "surface", "TEXUNIT0");
    secondBinding = HlslNewBinding(&module, HLSL_STORAGE_SAMPLER, s3,
                                   "volume", "TEXUNIT0");
    assert(firstBinding != NULL && secondBinding != NULL);
    firstBinding->hasExplicitRegister = 1;
    firstBinding->physical.bank = HLSL_REGISTER_S;
    firstBinding->physical.regno = 0;
    secondBinding->hasExplicitRegister = 1;
    secondBinding->physical.bank = HLSL_REGISTER_S;
    secondBinding->physical.regno = 0;
    module.bindings = firstBinding;
    firstBinding->next = secondBinding;
    assert(!HlslValidateSamplerUsage(&module, &HlslProfile_hlslf));
    assert(module.errorKind == HLSL_ERROR_SAMPLER);
    assert(!strcmp(module.errorReason, "TEXUNIT0"));

    HlslInitModule(&module, HLSL_STAGE_PIXEL, TestAlloc, NULL);
    firstBinding = HlslNewBinding(&module, HLSL_STORAGE_SAMPLER, s2,
                                  "surface", "TEXUNIT16");
    assert(firstBinding != NULL);
    firstBinding->hasExplicitRegister = 1;
    firstBinding->physical.bank = HLSL_REGISTER_S;
    firstBinding->physical.regno = 16;
    module.bindings = firstBinding;
    assert(!HlslValidateSamplerUsage(&module, &HlslProfile_hlslf));
    assert(module.errorKind == HLSL_ERROR_SAMPLER);

    memset(cyclicTypes, 0, sizeof(cyclicTypes));
    cyclicTypes[0].arraySize = 1;
    cyclicTypes[0].elementType = &cyclicTypes[1];
    cyclicTypes[1].arraySize = 1;
    cyclicTypes[1].elementType = &cyclicTypes[0];
    HlslInitModule(&module, HLSL_STAGE_PIXEL, TestAlloc, NULL);
    firstBinding = HlslNewBinding(&module, HLSL_STORAGE_SAMPLER,
                                  cyclicTypes[0], "cycle", "TEXUNIT0");
    assert(firstBinding != NULL);
    module.bindings = firstBinding;
    assert(!HlslValidateSamplerUsage(&module, &HlslProfile_hlslf));
    assert(module.errorKind == HLSL_ERROR_INVALID_IR);
}

typedef struct ValidationFixture_Rec {
    HlslModule module;
    HlslDecl *inputStruct;
    HlslDecl *outputStruct;
    HlslFunction *entry;
    HlslFunction *wrapper;
    HlslExpr *entryCall;
    HlslStmt *callStatement;
} ValidationFixture;

static void InitValidationFixture(ValidationFixture *fixture,
                                  HlslStage stage)
{
    HlslType voidType;
    HlslType float4Type;
    HlslType inputType;
    HlslType outputType;
    HlslDecl *inputMember;
    HlslDecl *outputMember;
    HlslDecl *outputValue;
    HlslStmt *returnStatement;
    HlslExpr *returnValue;

    memset(fixture, 0, sizeof(*fixture));
    HlslInitModule(&fixture->module, stage, TestAlloc, NULL);
    voidType = HlslNumericType(HLSL_BASE_VOID, 0);
    float4Type = HlslNumericType(HLSL_BASE_FLOAT, 4);
    inputType = HlslNumericType(HLSL_BASE_STRUCT, 0);
    inputType.structName = stage == HLSL_STAGE_VERTEX ?
                           "cg_VertexIn" : "cg_PixelIn";
    outputType = HlslNumericType(HLSL_BASE_STRUCT, 0);
    outputType.structName = stage == HLSL_STAGE_VERTEX ?
                            "cg_VertexOut" : "cg_PixelOut";
    fixture->inputStruct = HlslNewDecl(&fixture->module,
        HLSL_STORAGE_INPUT, inputType, inputType.structName);
    fixture->outputStruct = HlslNewDecl(&fixture->module,
        HLSL_STORAGE_OUTPUT, outputType, outputType.structName);
    inputMember = HlslNewDecl(&fixture->module, HLSL_STORAGE_NONE,
                              float4Type, "input0");
    outputMember = HlslNewDecl(&fixture->module, HLSL_STORAGE_NONE,
                               float4Type, "output0");
    assert(fixture->inputStruct != NULL && fixture->outputStruct != NULL &&
           inputMember != NULL && outputMember != NULL);
    inputMember->semantic = stage == HLSL_STAGE_VERTEX ?
                            "POSITION0" : "COLOR0";
    outputMember->semantic = stage == HLSL_STAGE_VERTEX ?
                             "POSITION0" : "COLOR0";
    fixture->inputStruct->members = inputMember;
    fixture->outputStruct->members = outputMember;
    fixture->inputStruct->type.members = inputMember;
    fixture->outputStruct->type.members = outputMember;
    fixture->inputStruct->next = fixture->outputStruct;
    fixture->module.structs = fixture->inputStruct;

    fixture->entry = HlslNewFunction(&fixture->module, voidType,
                                     "cg_entry");
    fixture->wrapper = HlslNewFunction(&fixture->module,
                                       fixture->outputStruct->type,
                                       "main");
    outputValue = HlslNewDecl(&fixture->module, HLSL_STORAGE_NONE,
                              fixture->outputStruct->type, "outputValue");
    fixture->entryCall = HlslNewExpr(&fixture->module, HLSL_EXPR_CALL,
                                     voidType);
    fixture->callStatement = HlslNewStmt(&fixture->module,
                                         HLSL_STMT_EXPRESSION);
    returnStatement = HlslNewStmt(&fixture->module, HLSL_STMT_RETURN);
    returnValue = HlslNewExpr(&fixture->module, HLSL_EXPR_SYMBOL,
                              fixture->outputStruct->type);
    assert(fixture->entry != NULL && fixture->wrapper != NULL &&
           outputValue != NULL && fixture->entryCall != NULL &&
           fixture->callStatement != NULL && returnStatement != NULL &&
           returnValue != NULL);
    fixture->entry->isEntry = 1;
    fixture->entryCall->u.call.function = fixture->entry;
    fixture->entryCall->u.call.name = fixture->entry->name;
    fixture->callStatement->u.expression = fixture->entryCall;
    returnValue->u.symbol = outputValue;
    returnStatement->u.returnExpr = returnValue;
    fixture->callStatement->next = returnStatement;
    fixture->wrapper->locals = outputValue;
    fixture->wrapper->body = fixture->callStatement;
    fixture->entry->next = fixture->wrapper;
    fixture->module.functions = fixture->entry;
    fixture->module.entry = fixture->entry;
    fixture->module.wrapper = fixture->wrapper;
}

static void TestInterfaceMetadataWriter(void)
{
    ValidationFixture fixture;
    HlslBinding *countBinding;
    HlslBinding *enabledBinding;
    HlslBinding *valuesBinding;
    HlslBinding *pairsBinding;
    HlslDecl *countDecl;
    HlslDecl *enabledDecl;
    HlslDecl *valuesDecl;
    HlslDecl *pairsDecl;
    HlslExpr *reference;
    HlslExpr *object;
    HlslExpr *subscript;
    HlslExpr *element;
    HlslExpr *component;
    HlslExpr *zero;
    HlslExpr *zeroStruct;
    HlslStmt *statement;
    HlslType countType;
    HlslType enabledType;
    HlslType valuesType;
    HlslType pairType;
    HlslType pairsType;
    float countDefault;
    float enabledDefault;
    int countIdentity;
    int enabledIdentity;
    int valuesIdentity;
    int pairsIdentity;
    FILE *stream;
    long length;
    char *output;

    InitValidationFixture(&fixture, HLSL_STAGE_VERTEX);
    fixture.inputStruct->members->publicName = "sourcePosition";
    countType = HlslNumericType(HLSL_BASE_INT, 1);
    enabledType = HlslNumericType(HLSL_BASE_BOOL, 1);
    valuesType = HlslNumericType(HLSL_BASE_INT, 3);
    pairType = HlslNumericType(HLSL_BASE_INT, 2);
    memset(&pairsType, 0, sizeof(pairsType));
    pairsType.arraySize = 2;
    pairsType.elementType = &pairType;
    countBinding = HlslNewBinding(&fixture.module, HLSL_STORAGE_UNIFORM,
                                  countType, "count", NULL);
    enabledBinding = HlslNewBinding(&fixture.module, HLSL_STORAGE_UNIFORM,
                                    enabledType, "enabled", NULL);
    valuesBinding = HlslNewBinding(&fixture.module, HLSL_STORAGE_UNIFORM,
                                   valuesType, "values", NULL);
    pairsBinding = HlslNewBinding(&fixture.module, HLSL_STORAGE_UNIFORM,
                                  pairsType, "pairs", NULL);
    countDecl = HlslNewDecl(&fixture.module, HLSL_STORAGE_UNIFORM,
                            countType, "cg_count");
    enabledDecl = HlslNewDecl(&fixture.module, HLSL_STORAGE_UNIFORM,
                              enabledType, "cg_enabled");
    valuesDecl = HlslNewDecl(&fixture.module, HLSL_STORAGE_UNIFORM,
                             valuesType, "cg_values");
    pairsDecl = HlslNewDecl(&fixture.module, HLSL_STORAGE_UNIFORM,
                            pairsType, "cg_pairs");
    assert(countBinding != NULL && enabledBinding != NULL &&
           valuesBinding != NULL && pairsBinding != NULL &&
           countDecl != NULL && enabledDecl != NULL &&
           valuesDecl != NULL && pairsDecl != NULL);
    countDecl->identity = &countIdentity;
    enabledDecl->identity = &enabledIdentity;
    valuesDecl->identity = &valuesIdentity;
    pairsDecl->identity = &pairsIdentity;
    countBinding->declaration = countDecl;
    enabledBinding->declaration = enabledDecl;
    valuesBinding->declaration = valuesDecl;
    pairsBinding->declaration = pairsDecl;
    countDefault = 7.0f;
    enabledDefault = 1.0f;
    countBinding->defaultCount = 1;
    countBinding->defaultValues = &countDefault;
    enabledBinding->defaultCount = 1;
    enabledBinding->defaultValues = &enabledDefault;
    valuesBinding->sourceOrdinal = 1;
    pairsBinding->sourceOrdinal = 2;
    enabledBinding->sourceOrdinal = 3;
    fixture.module.bindings = countBinding;
    countBinding->next = valuesBinding;
    valuesBinding->next = pairsBinding;
    pairsBinding->next = enabledBinding;
    assert(HlslAllocateOneBinding(&fixture.module, &HlslProfile_hlslv,
                                  countBinding));
    assert(HlslAllocateOneBinding(&fixture.module, &HlslProfile_hlslv,
                                  valuesBinding));
    assert(HlslAllocateOneBinding(&fixture.module, &HlslProfile_hlslv,
                                  pairsBinding));
    assert(HlslAllocateOneBinding(&fixture.module, &HlslProfile_hlslv,
                                  enabledBinding));
    reference = HlslNewExpr(&fixture.module, HLSL_EXPR_SYMBOL, countType);
    statement = HlslNewStmt(&fixture.module, HLSL_STMT_EXPRESSION);
    assert(reference != NULL && statement != NULL);
    reference->u.symbol = countBinding->declaration;
    statement->u.expression = reference;
    HlslAppendStmt(&fixture.entry->body, statement);
    reference = HlslNewExpr(&fixture.module, HLSL_EXPR_SYMBOL, valuesType);
    statement = HlslNewStmt(&fixture.module, HLSL_STMT_EXPRESSION);
    assert(reference != NULL && statement != NULL);
    reference->u.symbol = valuesBinding->declaration;
    statement->u.expression = reference;
    HlslAppendStmt(&fixture.entry->body, statement);
    object = HlslNewExpr(&fixture.module, HLSL_EXPR_SYMBOL, valuesType);
    subscript = HlslNewExpr(&fixture.module, HLSL_EXPR_INT, countType);
    component = HlslNewExpr(&fixture.module, HLSL_EXPR_INDEX, countType);
    statement = HlslNewStmt(&fixture.module, HLSL_STMT_EXPRESSION);
    assert(object != NULL && subscript != NULL && component != NULL &&
           statement != NULL);
    object->u.symbol = valuesBinding->declaration;
    subscript->u.literalInt = 1;
    component->u.index.object = object;
    component->u.index.index = subscript;
    statement->u.expression = component;
    HlslAppendStmt(&fixture.entry->body, statement);
    object = HlslNewExpr(&fixture.module, HLSL_EXPR_SYMBOL, pairsType);
    subscript = HlslNewExpr(&fixture.module, HLSL_EXPR_INT, countType);
    element = HlslNewExpr(&fixture.module, HLSL_EXPR_INDEX, pairType);
    statement = HlslNewStmt(&fixture.module, HLSL_STMT_EXPRESSION);
    assert(object != NULL && subscript != NULL && element != NULL &&
           statement != NULL);
    object->u.symbol = pairsBinding->declaration;
    subscript->u.literalInt = 1;
    element->u.index.object = object;
    element->u.index.index = subscript;
    statement->u.expression = element;
    HlslAppendStmt(&fixture.entry->body, statement);
    object = HlslNewExpr(&fixture.module, HLSL_EXPR_SYMBOL, pairsType);
    subscript = HlslNewExpr(&fixture.module, HLSL_EXPR_INT, countType);
    element = HlslNewExpr(&fixture.module, HLSL_EXPR_INDEX, pairType);
    component = HlslNewExpr(&fixture.module, HLSL_EXPR_SWIZZLE, countType);
    statement = HlslNewStmt(&fixture.module, HLSL_STMT_EXPRESSION);
    assert(object != NULL && subscript != NULL && element != NULL &&
           component != NULL && statement != NULL);
    object->u.symbol = pairsBinding->declaration;
    subscript->u.literalInt = 1;
    element->u.index.object = object;
    element->u.index.index = subscript;
    component->u.swizzle.object = element;
    component->u.swizzle.mask = "y";
    statement->u.expression = component;
    HlslAppendStmt(&fixture.entry->body, statement);
    reference = HlslNewExpr(&fixture.module, HLSL_EXPR_SYMBOL, enabledType);
    statement = HlslNewStmt(&fixture.module, HLSL_STMT_EXPRESSION);
    assert(reference != NULL && statement != NULL);
    reference->u.symbol = enabledBinding->declaration;
    statement->u.expression = reference;
    HlslAppendStmt(&fixture.entry->body, statement);
    zero = HlslNewExpr(&fixture.module, HLSL_EXPR_INT,
                       HlslNumericType(HLSL_BASE_INT, 1));
    zeroStruct = HlslNewExpr(&fixture.module, HLSL_EXPR_CAST,
                             fixture.outputStruct->type);
    assert(zero != NULL && zeroStruct != NULL &&
           fixture.wrapper->locals != NULL);
    zero->u.literalInt = 0;
    zeroStruct->u.cast.expression = zero;
    fixture.wrapper->locals->initializer = zeroStruct;
    stream = tmpfile();
    assert(stream != NULL);
    assert(HlslWriteModule(stream, &fixture.module, &HlslProfile_hlslv));
    length = StreamLength(stream);
    output = (char *) malloc((size_t) length + 1);
    assert(output != NULL);
    rewind(stream);
    assert(fread(output, 1, (size_t) length, stream) == (size_t) length);
    output[length] = '\0';
    assert(strstr(output,
        "// cgc-bind interface in sourcePosition float4 POSITION0\n") !=
        NULL);
    assert(strstr(output,
        "// cgc-bind interface out output0 float4 POSITION0\n") != NULL);
    assert(strstr(output, "// cgc-default count 7.0\n") != NULL);
    assert(strstr(output, "// cgc-default enabled 1.0\n") != NULL);
    assert(strstr(output,
        "uniform int4 cg_count : register(i0);\n") != NULL);
    assert(strstr(output,
        "uniform int4 cg_values : register(i1);\n") != NULL);
    assert(strstr(output,
        "uniform int4 cg_pairs[2] : register(i2);\n") != NULL);
    assert(strstr(output,
        "uniform bool cg_enabled : register(b0);\n") != NULL);
    assert(strstr(output, "cg_count.x;\n") != NULL);
    assert(strstr(output, "cg_values.xyz;\n") != NULL);
    assert(strstr(output, "cg_values[1];\n") != NULL);
    assert(strstr(output, "cg_values.xyz[1]") == NULL);
    assert(strstr(output, "cg_pairs[1].xy;\n") != NULL);
    assert(strstr(output, "cg_pairs[1].y;\n") != NULL);
    assert(strstr(output, ".xy.y") == NULL);
    assert(strstr(output,
        "cg_VertexOut outputValue = (cg_VertexOut) 0;\n") != NULL);
    assert(strstr(output, "register(i0) =") == NULL);
    assert(strstr(output, "register(b0) =") == NULL);
    free(output);
    assert(fclose(stream) == 0);
}

static void TestLoopAttributeWriter(void)
{
    ValidationFixture fixture;
    HlslDecl *dynamicCondition;
    HlslExpr *condition;
    HlslStmt *loop;
    HlslStmt *body;
    HlslType boolType;
    FILE *stream;
    long length;
    char *output;

    InitValidationFixture(&fixture, HLSL_STAGE_VERTEX);
    boolType = HlslNumericType(HLSL_BASE_BOOL, 1);
    dynamicCondition = HlslNewDecl(&fixture.module, HLSL_STORAGE_NONE,
                                   boolType, "cg_dynamicCondition");
    assert(dynamicCondition != NULL);
    fixture.entry->locals = dynamicCondition;

    condition = HlslNewExpr(&fixture.module, HLSL_EXPR_BOOL, boolType);
    loop = HlslNewStmt(&fixture.module, HLSL_STMT_WHILE);
    assert(condition != NULL && loop != NULL);
    condition->u.literalBool = 0;
    loop->u.loop.condition = condition;
    HlslAppendStmt(&fixture.entry->body, loop);

    condition = HlslNewExpr(&fixture.module, HLSL_EXPR_BOOL, boolType);
    loop = HlslNewStmt(&fixture.module, HLSL_STMT_WHILE);
    assert(condition != NULL && loop != NULL);
    condition->u.literalBool = 1;
    loop->u.loop.condition = condition;
    HlslAppendStmt(&fixture.entry->body, loop);

    condition = HlslNewExpr(&fixture.module, HLSL_EXPR_SYMBOL, boolType);
    loop = HlslNewStmt(&fixture.module, HLSL_STMT_WHILE);
    assert(condition != NULL && loop != NULL);
    condition->u.symbol = dynamicCondition;
    loop->u.loop.condition = condition;
    HlslAppendStmt(&fixture.entry->body, loop);

    loop = HlslNewStmt(&fixture.module, HLSL_STMT_FOR);
    body = HlslNewStmt(&fixture.module, HLSL_STMT_BREAK);
    assert(loop != NULL && body != NULL);
    loop->u.forStmt.body = body;
    HlslAppendStmt(&fixture.entry->body, loop);

    stream = tmpfile();
    assert(stream != NULL);
    assert(HlslWriteModule(stream, &fixture.module, &HlslProfile_hlslv));
    length = StreamLength(stream);
    output = (char *) malloc((size_t) length + 1);
    assert(output != NULL);
    rewind(stream);
    assert(fread(output, 1, (size_t) length, stream) == (size_t) length);
    output[length] = '\0';
    assert(strstr(output, "[unroll]\n    while (false)") != NULL);
    assert(strstr(output, "[unroll]\n    for (; ; )") != NULL);
    assert(strstr(output, "[unroll]\n    while (true)") == NULL);
    assert(strstr(output,
                  "[unroll]\n    while (cg_dynamicCondition)") == NULL);
    free(output);
    assert(fclose(stream) == 0);
}

static void AssertInvalidValidationFixture(ValidationFixture *fixture,
                                           const HlslProfileDesc *profile)
{
    assert(!HlslValidateModule(&fixture->module, profile));
    assert(fixture->module.errorKind == HLSL_ERROR_INVALID_IR);
    assert(fixture->module.errors == 1);
}

static void AssertPhasePreservesFirstFailure(int phase)
{
    HlslModule module;
    HlslBinding binding;
    HlslLoc firstLoc;

    HlslInitModule(&module, HLSL_STAGE_VERTEX, TestAlloc, NULL);
    firstLoc.file = 3;
    firstLoc.line = 7;
    assert(!HlslFail(&module, HLSL_ERROR_UNSUPPORTED_TYPE, &firstLoc,
                     "first"));
    if (phase == 0) {
        memset(&binding, 0, sizeof(binding));
        binding.name = "invalidBinding";
        binding.type = HlslNumericType(HLSL_BASE_VOID, 0);
        assert(!HlslAllocateOneBinding(&module, &HlslProfile_hlslv,
                                       &binding));
    } else if (phase == 1) {
        assert(!HlslLegalizeModule(&module, &HlslProfile_hlslv));
    } else {
        assert(!HlslValidateModule(&module, &HlslProfile_hlslv));
    }
    assert(module.errors == 2);
    assert(module.errorKind == HLSL_ERROR_UNSUPPORTED_TYPE);
    assert(!strcmp(module.errorReason, "first"));
    assert(module.errorLoc.file == 3 && module.errorLoc.line == 7);
}

static void TestHlslErrorMappingAndFirstFailure(void)
{
    static const int expectedCodes[] = {
        0, 6400, 6401, 6402, 6403, 6404, 6405,
        6406, 6407, 6408, 6409, 6410, 6411, 9013
    };
    HlslModule module;
    HlslLoc firstLoc;
    HlslLoc secondLoc;
    int i;

    assert((int) (sizeof(expectedCodes) / sizeof(expectedCodes[0])) ==
           HLSL_ERROR_INVALID_IR + 1);
    for (i = HLSL_ERROR_NONE; i <= HLSL_ERROR_INVALID_IR; i++)
        assert(HlslErrorCode((HlslErrorKind) i) == expectedCodes[i]);
    assert(HlslErrorCode((HlslErrorKind) -1) == 0);
    assert(HlslErrorCode((HlslErrorKind) 99) == 0);

    HlslInitModule(&module, HLSL_STAGE_VERTEX, TestAlloc, NULL);
    firstLoc.file = 3;
    firstLoc.line = 7;
    secondLoc.file = 9;
    secondLoc.line = 11;
    assert(!HlslFail(&module, HLSL_ERROR_UNSUPPORTED_TYPE, &firstLoc,
                     "first"));
    assert(!HlslFail(&module, HLSL_ERROR_SAMPLER, &secondLoc, "second"));
    assert(module.errors == 2);
    assert(module.errorKind == HLSL_ERROR_UNSUPPORTED_TYPE);
    assert(!strcmp(module.errorReason, "first"));
    assert(module.errorLoc.file == 3 && module.errorLoc.line == 7);

    AssertPhasePreservesFirstFailure(0);
    AssertPhasePreservesFirstFailure(1);
    AssertPhasePreservesFirstFailure(2);
}

static void TestStructuralValidatorRejectsMalformedGraphs(void)
{
    ValidationFixture fixture;
    HlslExpr *badExpression;
    HlslExpr *leftExpression;
    HlslExpr *rightExpression;
    HlslStmt *badStatement;
    HlslDecl *parameter;
    HlslDecl *foreignMember;
    HlslType arrayType;
    HlslType floatType;
    HlslType voidType;
    HlslName nameCycle;

    InitValidationFixture(&fixture, HLSL_STAGE_VERTEX);
    assert(HlslValidateModule(&fixture.module, &HlslProfile_hlslv));

    InitValidationFixture(&fixture, HLSL_STAGE_VERTEX);
    badExpression = HlslNewExpr(&fixture.module, HLSL_EXPR_UNARY,
                                HlslNumericType(HLSL_BASE_FLOAT, 1));
    assert(badExpression != NULL);
    badExpression->u.unary.op = HLSL_OP_NEGATE;
    badExpression->u.unary.operand = badExpression;
    fixture.callStatement->u.expression = badExpression;
    AssertInvalidValidationFixture(&fixture, &HlslProfile_hlslv);

    InitValidationFixture(&fixture, HLSL_STAGE_VERTEX);
    fixture.callStatement->next = fixture.callStatement;
    AssertInvalidValidationFixture(&fixture, &HlslProfile_hlslv);

    InitValidationFixture(&fixture, HLSL_STAGE_VERTEX);
    fixture.inputStruct->members->next = fixture.inputStruct->members;
    AssertInvalidValidationFixture(&fixture, &HlslProfile_hlslv);

    InitValidationFixture(&fixture, HLSL_STAGE_VERTEX);
    fixture.wrapper->next = fixture.entry;
    AssertInvalidValidationFixture(&fixture, &HlslProfile_hlslv);

    InitValidationFixture(&fixture, HLSL_STAGE_VERTEX);
    memset(&nameCycle, 0, sizeof(nameCycle));
    nameCycle.next = &nameCycle;
    fixture.module.names = &nameCycle;
    AssertInvalidValidationFixture(&fixture, &HlslProfile_hlslv);

    InitValidationFixture(&fixture, HLSL_STAGE_VERTEX);
    memset(&arrayType, 0, sizeof(arrayType));
    arrayType.arraySize = 1;
    arrayType.elementType = &arrayType;
    fixture.entry->result = arrayType;
    AssertInvalidValidationFixture(&fixture, &HlslProfile_hlslv);

    InitValidationFixture(&fixture, HLSL_STAGE_VERTEX);
    floatType = HlslNumericType(HLSL_BASE_FLOAT, 1);
    foreignMember = HlslNewDecl(&fixture.module, HLSL_STORAGE_NONE,
                                floatType, "foreignMember");
    assert(foreignMember != NULL);
    memset(&arrayType, 0, sizeof(arrayType));
    arrayType.base = HLSL_BASE_STRUCT;
    arrayType.structName = "ForeignType";
    arrayType.members = foreignMember;
    parameter = HlslNewDecl(&fixture.module, HLSL_STORAGE_NONE,
                            arrayType, "foreignTypedLocal");
    assert(parameter != NULL);
    fixture.entry->locals = parameter;
    AssertInvalidValidationFixture(&fixture, &HlslProfile_hlslv);

    InitValidationFixture(&fixture, HLSL_STAGE_VERTEX);
    badExpression = HlslNewExpr(&fixture.module, HLSL_EXPR_BINARY,
                                HlslNumericType(HLSL_BASE_FLOAT, 1));
    assert(badExpression != NULL);
    badExpression->u.binary.op = HLSL_OP_ADD;
    fixture.callStatement->u.expression = badExpression;
    AssertInvalidValidationFixture(&fixture, &HlslProfile_hlslv);

    InitValidationFixture(&fixture, HLSL_STAGE_VERTEX);
    floatType = HlslNumericType(HLSL_BASE_FLOAT, 1);
    badExpression = HlslNewExpr(&fixture.module, HLSL_EXPR_BINARY,
                                HlslNumericType(HLSL_BASE_BOOL, 1));
    leftExpression = HlslNewExpr(&fixture.module, HLSL_EXPR_FLOAT,
                                 floatType);
    rightExpression = HlslNewExpr(&fixture.module, HLSL_EXPR_FLOAT,
                                  floatType);
    assert(badExpression != NULL && leftExpression != NULL &&
           rightExpression != NULL);
    badExpression->u.binary.op = HLSL_OP_ADD;
    badExpression->u.binary.left = leftExpression;
    badExpression->u.binary.right = rightExpression;
    fixture.callStatement->u.expression = badExpression;
    AssertInvalidValidationFixture(&fixture, &HlslProfile_hlslv);

    InitValidationFixture(&fixture, HLSL_STAGE_VERTEX);
    floatType = HlslNumericType(HLSL_BASE_FLOAT, 1);
    badExpression = HlslNewExpr(&fixture.module, HLSL_EXPR_UNARY,
                                floatType);
    leftExpression = HlslNewExpr(&fixture.module, HLSL_EXPR_FLOAT,
                                 floatType);
    assert(badExpression != NULL && leftExpression != NULL);
    badExpression->u.unary.op = HLSL_OP_LOGICAL_NOT;
    badExpression->u.unary.operand = leftExpression;
    fixture.callStatement->u.expression = badExpression;
    AssertInvalidValidationFixture(&fixture, &HlslProfile_hlslv);

    InitValidationFixture(&fixture, HLSL_STAGE_VERTEX);
    floatType = HlslNumericType(HLSL_BASE_FLOAT, 1);
    badExpression = HlslNewExpr(&fixture.module, HLSL_EXPR_CONDITIONAL,
                                floatType);
    leftExpression = HlslNewExpr(&fixture.module, HLSL_EXPR_FLOAT,
                                 floatType);
    rightExpression = HlslNewExpr(&fixture.module, HLSL_EXPR_FLOAT,
                                  floatType);
    assert(badExpression != NULL && leftExpression != NULL &&
           rightExpression != NULL);
    badExpression->u.conditional.condition = leftExpression;
    badExpression->u.conditional.trueExpr = rightExpression;
    badExpression->u.conditional.falseExpr = rightExpression;
    fixture.callStatement->u.expression = badExpression;
    AssertInvalidValidationFixture(&fixture, &HlslProfile_hlslv);

    InitValidationFixture(&fixture, HLSL_STAGE_VERTEX);
    badExpression = HlslNewExpr(&fixture.module, HLSL_EXPR_CONSTRUCT,
                                HlslNumericType(HLSL_BASE_FLOAT, 4));
    assert(badExpression != NULL);
    fixture.callStatement->u.expression = badExpression;
    AssertInvalidValidationFixture(&fixture, &HlslProfile_hlslv);

    InitValidationFixture(&fixture, HLSL_STAGE_VERTEX);
    floatType = HlslNumericType(HLSL_BASE_FLOAT, 1);
    badExpression = HlslNewExpr(&fixture.module, HLSL_EXPR_CAST,
                                HlslNumericType(HLSL_BASE_FLOAT, 2));
    leftExpression = HlslNewExpr(&fixture.module, HLSL_EXPR_FLOAT,
                                 floatType);
    assert(badExpression != NULL && leftExpression != NULL);
    badExpression->u.cast.expression = leftExpression;
    fixture.callStatement->u.expression = badExpression;
    AssertInvalidValidationFixture(&fixture, &HlslProfile_hlslv);

    InitValidationFixture(&fixture, HLSL_STAGE_VERTEX);
    badExpression = HlslNewExpr(&fixture.module, HLSL_EXPR_SWIZZLE,
                                HlslNumericType(HLSL_BASE_FLOAT, 1));
    leftExpression = HlslNewExpr(&fixture.module, HLSL_EXPR_CONSTRUCT,
                                 HlslNumericType(HLSL_BASE_FLOAT, 2));
    assert(badExpression != NULL && leftExpression != NULL);
    leftExpression->u.construct.arguments = HlslNewExpr(&fixture.module,
        HLSL_EXPR_FLOAT, HlslNumericType(HLSL_BASE_FLOAT, 1));
    rightExpression = HlslNewExpr(&fixture.module, HLSL_EXPR_FLOAT,
                                  HlslNumericType(HLSL_BASE_FLOAT, 1));
    assert(leftExpression->u.construct.arguments != NULL &&
           rightExpression != NULL);
    HlslAppendExpr(&leftExpression->u.construct.arguments,
                   rightExpression);
    badExpression->u.swizzle.object = leftExpression;
    badExpression->u.swizzle.mask = "z";
    fixture.callStatement->u.expression = badExpression;
    AssertInvalidValidationFixture(&fixture, &HlslProfile_hlslv);

    InitValidationFixture(&fixture, HLSL_STAGE_VERTEX);
    parameter = HlslNewDecl(&fixture.module, HLSL_STORAGE_NONE,
                            HlslNumericType(HLSL_BASE_VOID, 0),
                            "invalidVoidLocal");
    assert(parameter != NULL);
    fixture.entry->locals = parameter;
    AssertInvalidValidationFixture(&fixture, &HlslProfile_hlslv);

    InitValidationFixture(&fixture, HLSL_STAGE_VERTEX);
    badStatement = HlslNewStmt(&fixture.module, HLSL_STMT_BREAK);
    assert(badStatement != NULL);
    fixture.entry->body = badStatement;
    AssertInvalidValidationFixture(&fixture, &HlslProfile_hlslv);

    InitValidationFixture(&fixture, HLSL_STAGE_VERTEX);
    badStatement = HlslNewStmt(&fixture.module, HLSL_STMT_IF);
    badExpression = HlslNewExpr(&fixture.module, HLSL_EXPR_FLOAT,
                                HlslNumericType(HLSL_BASE_FLOAT, 1));
    assert(badStatement != NULL && badExpression != NULL);
    badStatement->u.ifStmt.condition = badExpression;
    fixture.entry->body = badStatement;
    AssertInvalidValidationFixture(&fixture, &HlslProfile_hlslv);

    InitValidationFixture(&fixture, HLSL_STAGE_VERTEX);
    floatType = HlslNumericType(HLSL_BASE_FLOAT, 1);
    parameter = HlslNewDecl(&fixture.module, HLSL_STORAGE_NONE,
                            floatType, "required");
    assert(parameter != NULL);
    fixture.entry->parameters = parameter;
    AssertInvalidValidationFixture(&fixture, &HlslProfile_hlslv);

    InitValidationFixture(&fixture, HLSL_STAGE_VERTEX);
    voidType = HlslNumericType(HLSL_BASE_VOID, 0);
    badExpression = HlslNewExpr(&fixture.module, HLSL_EXPR_CALL, voidType);
    badStatement = HlslNewStmt(&fixture.module, HLSL_STMT_EXPRESSION);
    assert(badExpression != NULL && badStatement != NULL);
    badExpression->u.call.function = fixture.entry;
    badExpression->u.call.name = fixture.entry->name;
    badStatement->u.expression = badExpression;
    fixture.entry->body = badStatement;
    assert(!HlslValidateModule(&fixture.module, &HlslProfile_hlslv));
    assert(fixture.module.errorKind == HLSL_ERROR_ENTRY_ABI);
    assert(fixture.module.errors == 1);
}

static void AppendInterfaceMember(HlslModule *module, HlslDecl *structure,
                                  const char *name, const char *semantic)
{
    HlslDecl *member;

    member = HlslNewDecl(module, HLSL_STORAGE_NONE,
                         HlslNumericType(HLSL_BASE_FLOAT, 4), name);
    assert(member != NULL);
    member->semantic = semantic;
    HlslAppendDecl(&structure->members, member);
    structure->type.members = structure->members;
}

static void TestTargetValidatorInterfaceLimits(void)
{
    static const char *texcoords[] = {
        "TEXCOORD0", "TEXCOORD1", "TEXCOORD2", "TEXCOORD3",
        "TEXCOORD4", "TEXCOORD5", "TEXCOORD6", "TEXCOORD7",
        "TEXCOORD8", "TEXCOORD9", "TEXCOORD10", "TEXCOORD11",
        "TEXCOORD12", "TEXCOORD13", "TEXCOORD14", "TEXCOORD15"
    };
    ValidationFixture fixture;
    HlslDecl *local;
    HlslExpr *derivative;
    HlslExpr *argument;
    char names[16][16];
    int i;

    InitValidationFixture(&fixture, HLSL_STAGE_VERTEX);
    for (i = 0; i < 16; i++) {
        sprintf(names[i], "input%d", i + 1);
        AppendInterfaceMember(&fixture.module, fixture.inputStruct,
                              names[i], texcoords[i]);
    }
    assert(!HlslValidateModule(&fixture.module, &HlslProfile_hlslv));
    assert(fixture.module.errorKind == HLSL_ERROR_RESOURCE_LIMIT);
    assert(!strcmp(fixture.module.resourceName, "inputs"));
    assert(fixture.module.resourceUsed == 17);
    assert(fixture.module.resourceAvailable == 16);

    InitValidationFixture(&fixture, HLSL_STAGE_PIXEL);
    AppendInterfaceMember(&fixture.module, fixture.outputStruct,
                          "color1", "COLOR1");
    AppendInterfaceMember(&fixture.module, fixture.outputStruct,
                          "color2", "COLOR2");
    AppendInterfaceMember(&fixture.module, fixture.outputStruct,
                          "color3", "COLOR3");
    AppendInterfaceMember(&fixture.module, fixture.outputStruct,
                          "color4", "COLOR4");
    assert(!HlslValidateModule(&fixture.module, &HlslProfile_hlslf));
    assert(fixture.module.errorKind == HLSL_ERROR_RESOURCE_LIMIT);
    assert(!strcmp(fixture.module.resourceName, "color outputs"));
    assert(fixture.module.resourceUsed == 5);
    assert(fixture.module.resourceAvailable == 4);

    InitValidationFixture(&fixture, HLSL_STAGE_VERTEX);
    local = HlslNewDecl(&fixture.module, HLSL_STORAGE_NONE,
                        HlslNumericType(HLSL_BASE_FLOAT, 1), "derivative");
    derivative = HlslNewExpr(&fixture.module, HLSL_EXPR_CALL,
                             HlslNumericType(HLSL_BASE_FLOAT, 1));
    argument = HlslNewExpr(&fixture.module, HLSL_EXPR_FLOAT,
                           HlslNumericType(HLSL_BASE_FLOAT, 1));
    assert(local != NULL && derivative != NULL && argument != NULL);
    derivative->u.call.name = "ddx";
    derivative->u.call.builtin = HLSL_BUILTIN_DDX;
    derivative->u.call.arguments = argument;
    local->initializer = derivative;
    fixture.entry->locals = local;
    assert(!HlslValidateModule(&fixture.module, &HlslProfile_hlslv));
    assert(fixture.module.errorKind == HLSL_ERROR_STAGE_OPERATION);
}

static void TestStructuralValidatorRejectsCyclicSignatureGraphs(void)
{
    ValidationFixture fixture;
    HlslDecl leftMember;
    HlslDecl rightMember;
    HlslDecl *parameter;
    HlslExpr *argument;
    HlslExpr *call;
    HlslFunction *callee;
    HlslStmt *statement;
    HlslType leftCycle;
    HlslType rightCycle;
    HlslType voidType;

    InitValidationFixture(&fixture, HLSL_STAGE_VERTEX);
    memset(&leftMember, 0, sizeof(leftMember));
    memset(&rightMember, 0, sizeof(rightMember));
    memset(&leftCycle, 0, sizeof(leftCycle));
    memset(&rightCycle, 0, sizeof(rightCycle));
    leftMember.name = "left";
    leftMember.type = HlslNumericType(HLSL_BASE_FLOAT, 1);
    leftMember.next = &leftMember;
    rightMember.name = "right";
    rightMember.type = HlslNumericType(HLSL_BASE_FLOAT, 1);
    rightMember.next = &rightMember;
    leftCycle.base = HLSL_BASE_STRUCT;
    leftCycle.structName = "Cycle";
    leftCycle.members = &leftMember;
    rightCycle.base = HLSL_BASE_STRUCT;
    rightCycle.structName = "Cycle";
    rightCycle.members = &rightMember;
    voidType = HlslNumericType(HLSL_BASE_VOID, 0);
    callee = HlslNewFunction(&fixture.module, voidType, "cyclicCallee");
    parameter = HlslNewDecl(&fixture.module, HLSL_STORAGE_NONE,
                            leftCycle, "value");
    call = HlslNewExpr(&fixture.module, HLSL_EXPR_CALL, voidType);
    argument = HlslNewExpr(&fixture.module, HLSL_EXPR_FLOAT, rightCycle);
    statement = HlslNewStmt(&fixture.module, HLSL_STMT_EXPRESSION);
    assert(callee != NULL && parameter != NULL && call != NULL &&
           argument != NULL && statement != NULL);
    callee->parameters = parameter;
    call->u.call.function = callee;
    call->u.call.name = callee->name;
    call->u.call.arguments = argument;
    statement->u.expression = call;
    fixture.entry->body = statement;
    fixture.wrapper->next = callee;
    AssertInvalidValidationFixture(&fixture, &HlslProfile_hlslv);

    InitValidationFixture(&fixture, HLSL_STAGE_VERTEX);
    memset(&leftCycle, 0, sizeof(leftCycle));
    memset(&rightCycle, 0, sizeof(rightCycle));
    leftCycle.arraySize = 1;
    leftCycle.elementType = &leftCycle;
    rightCycle.arraySize = 1;
    rightCycle.elementType = &rightCycle;
    callee = HlslNewFunction(&fixture.module, voidType, "arrayCallee");
    parameter = HlslNewDecl(&fixture.module, HLSL_STORAGE_NONE,
                            leftCycle, "value");
    call = HlslNewExpr(&fixture.module, HLSL_EXPR_CALL, voidType);
    argument = HlslNewExpr(&fixture.module, HLSL_EXPR_FLOAT, rightCycle);
    statement = HlslNewStmt(&fixture.module, HLSL_STMT_EXPRESSION);
    assert(callee != NULL && parameter != NULL && call != NULL &&
           argument != NULL && statement != NULL);
    callee->parameters = parameter;
    call->u.call.function = callee;
    call->u.call.name = callee->name;
    call->u.call.arguments = argument;
    statement->u.expression = call;
    fixture.entry->body = statement;
    fixture.wrapper->next = callee;
    AssertInvalidValidationFixture(&fixture, &HlslProfile_hlslv);
}

static void InstallCyclicCallInitializer(ValidationFixture *fixture,
    HlslDecl *target, HlslDecl *leftMember, HlslDecl *rightMember)
{
    HlslDecl *parameter;
    HlslExpr *argument;
    HlslExpr *call;
    HlslFunction *callee;
    HlslType leftCycle;
    HlslType rightCycle;

    memset(leftMember, 0, sizeof(*leftMember));
    memset(rightMember, 0, sizeof(*rightMember));
    memset(&leftCycle, 0, sizeof(leftCycle));
    memset(&rightCycle, 0, sizeof(rightCycle));
    leftMember->name = "leftInitializerMember";
    leftMember->type = HlslNumericType(HLSL_BASE_FLOAT, 1);
    leftMember->next = leftMember;
    rightMember->name = "rightInitializerMember";
    rightMember->type = HlslNumericType(HLSL_BASE_FLOAT, 1);
    rightMember->next = rightMember;
    leftCycle.base = HLSL_BASE_STRUCT;
    leftCycle.structName = "InitializerCycle";
    leftCycle.members = leftMember;
    rightCycle.base = HLSL_BASE_STRUCT;
    rightCycle.structName = "InitializerCycle";
    rightCycle.members = rightMember;
    callee = HlslNewFunction(&fixture->module, target->type,
                             "cyclicInitializerCallee");
    parameter = HlslNewDecl(&fixture->module, HLSL_STORAGE_NONE,
                            leftCycle, "value");
    call = HlslNewExpr(&fixture->module, HLSL_EXPR_CALL, target->type);
    argument = HlslNewExpr(&fixture->module, HLSL_EXPR_FLOAT,
                           rightCycle);
    assert(callee != NULL && parameter != NULL && call != NULL &&
           argument != NULL);
    callee->parameters = parameter;
    call->u.call.function = callee;
    call->u.call.name = callee->name;
    call->u.call.arguments = argument;
    target->initializer = call;
    fixture->wrapper->next = callee;
} // InstallCyclicCallInitializer

static void TestStructuralValidatorPreflightsBeforeInitializers(void)
{
    ValidationFixture fixture;
    HlslDecl leftMember;
    HlslDecl rightMember;
    HlslDecl *global;

    InitValidationFixture(&fixture, HLSL_STAGE_VERTEX);
    global = HlslNewDecl(&fixture.module, HLSL_STORAGE_NONE,
        HlslNumericType(HLSL_BASE_FLOAT, 4), "globalInitializer");
    assert(global != NULL);
    fixture.module.globals = global;
    InstallCyclicCallInitializer(&fixture, global, &leftMember,
                                 &rightMember);
    AssertInvalidValidationFixture(&fixture, &HlslProfile_hlslv);

    InitValidationFixture(&fixture, HLSL_STAGE_VERTEX);
    InstallCyclicCallInitializer(&fixture,
        fixture.inputStruct->members, &leftMember, &rightMember);
    AssertInvalidValidationFixture(&fixture, &HlslProfile_hlslv);
}

static HlslBinding *AddValidationBinding(ValidationFixture *fixture)
{
    HlslBinding *binding;

    binding = HlslNewBinding(&fixture->module, HLSL_STORAGE_UNIFORM,
        HlslNumericType(HLSL_BASE_FLOAT, 4), "uniformValue", NULL);
    assert(binding != NULL);
    fixture->module.bindings = binding;
    assert(HlslAllocateBindings(&fixture->module, &HlslProfile_hlslv));
    assert(binding->leafBindings != NULL &&
           binding->leafBindings->allocationNext == NULL);
    return binding;
}

static void TestTargetValidatorRejectsMalformedBindingGraphs(void)
{
    ValidationFixture fixture;
    HlslBinding duplicate;
    HlslBinding *binding;
    HlslBinding *leaf;
    HlslDecl foreign;
    float defaultValue;

    InitValidationFixture(&fixture, HLSL_STAGE_VERTEX);
    binding = AddValidationBinding(&fixture);
    assert(HlslValidateModule(&fixture.module, &HlslProfile_hlslv));

    InitValidationFixture(&fixture, HLSL_STAGE_VERTEX);
    binding = AddValidationBinding(&fixture);
    memset(&foreign, 0, sizeof(foreign));
    foreign.name = "foreign";
    foreign.type = binding->type;
    binding->declaration = &foreign;
    AssertInvalidValidationFixture(&fixture, &HlslProfile_hlslv);

    InitValidationFixture(&fixture, HLSL_STAGE_VERTEX);
    AddValidationBinding(&fixture);
    fixture.module.bindings = NULL;
    AssertInvalidValidationFixture(&fixture, &HlslProfile_hlslv);

    InitValidationFixture(&fixture, HLSL_STAGE_VERTEX);
    AddValidationBinding(&fixture);
    fixture.module.allocatedBindings = NULL;
    AssertInvalidValidationFixture(&fixture, &HlslProfile_hlslv);

    InitValidationFixture(&fixture, HLSL_STAGE_VERTEX);
    binding = AddValidationBinding(&fixture);
    duplicate = *binding;
    duplicate.next = NULL;
    binding->next = &duplicate;
    AssertInvalidValidationFixture(&fixture, &HlslProfile_hlslv);

    InitValidationFixture(&fixture, HLSL_STAGE_VERTEX);
    binding = AddValidationBinding(&fixture);
    binding->storage = HLSL_STORAGE_SAMPLER;
    AssertInvalidValidationFixture(&fixture, &HlslProfile_hlslv);

    InitValidationFixture(&fixture, HLSL_STAGE_VERTEX);
    binding = AddValidationBinding(&fixture);
    binding->type = HlslNumericType(HLSL_BASE_FLOAT, 3);
    AssertInvalidValidationFixture(&fixture, &HlslProfile_hlslv);

    InitValidationFixture(&fixture, HLSL_STAGE_VERTEX);
    binding = AddValidationBinding(&fixture);
    binding->name = "differentRootName";
    AssertInvalidValidationFixture(&fixture, &HlslProfile_hlslv);

    InitValidationFixture(&fixture, HLSL_STAGE_VERTEX);
    binding = AddValidationBinding(&fixture);
    binding->publicName = "different.public.name";
    AssertInvalidValidationFixture(&fixture, &HlslProfile_hlslv);

    InitValidationFixture(&fixture, HLSL_STAGE_VERTEX);
    binding = AddValidationBinding(&fixture);
    binding->sourceOrdinal++;
    AssertInvalidValidationFixture(&fixture, &HlslProfile_hlslv);

    InitValidationFixture(&fixture, HLSL_STAGE_VERTEX);
    binding = AddValidationBinding(&fixture);
    binding->loc.line++;
    AssertInvalidValidationFixture(&fixture, &HlslProfile_hlslv);

    InitValidationFixture(&fixture, HLSL_STAGE_VERTEX);
    binding = AddValidationBinding(&fixture);
    defaultValue = 1.0f;
    binding->defaultCount = 1;
    binding->defaultValues = &defaultValue;
    AssertInvalidValidationFixture(&fixture, &HlslProfile_hlslv);

    InitValidationFixture(&fixture, HLSL_STAGE_VERTEX);
    binding = AddValidationBinding(&fixture);
    binding->physical.span++;
    AssertInvalidValidationFixture(&fixture, &HlslProfile_hlslv);

    InitValidationFixture(&fixture, HLSL_STAGE_VERTEX);
    binding = AddValidationBinding(&fixture);
    leaf = binding->leafBindings;
    leaf->declaration->name = "differentDeclarationName";
    AssertInvalidValidationFixture(&fixture, &HlslProfile_hlslv);

    InitValidationFixture(&fixture, HLSL_STAGE_VERTEX);
    binding = AddValidationBinding(&fixture);
    leaf = binding->leafBindings;
    leaf->declaration->storage = HLSL_STORAGE_SAMPLER;
    AssertInvalidValidationFixture(&fixture, &HlslProfile_hlslv);

    InitValidationFixture(&fixture, HLSL_STAGE_VERTEX);
    binding = AddValidationBinding(&fixture);
    leaf = binding->leafBindings;
    leaf->declaration->type = HlslNumericType(HLSL_BASE_FLOAT, 3);
    AssertInvalidValidationFixture(&fixture, &HlslProfile_hlslv);
}

static void TestTargetValidatorPreflightsAllBindingLeafLists(void)
{
    ValidationFixture fixture;
    HlslBinding secondLeaf;
    HlslBinding secondRoot;
    HlslBinding *firstRoot;

    InitValidationFixture(&fixture, HLSL_STAGE_VERTEX);
    firstRoot = AddValidationBinding(&fixture);
    secondRoot = *firstRoot;
    secondLeaf = *firstRoot->leafBindings;
    secondRoot.next = NULL;
    secondRoot.leafBindings = &secondLeaf;
    secondLeaf.next = &secondLeaf;
    secondLeaf.allocationNext = NULL;
    firstRoot->next = &secondRoot;
    AssertInvalidValidationFixture(&fixture, &HlslProfile_hlslv);
}

static void AssertSemanticValidationFixture(ValidationFixture *fixture,
                                            const HlslProfileDesc *profile)
{
    assert(!HlslValidateModule(&fixture->module, profile));
    assert(fixture->module.errorKind == HLSL_ERROR_SEMANTIC);
    assert(fixture->module.errors == 1);
}

static void TestTargetValidatorCanonicalResourcesAndTypes(void)
{
    static const char *mixedColors[] = {
        "cOl1", "COL2", "col3", "color4"
    };
    ValidationFixture fixture;
    HlslDecl *member;
    char names[20][16];
    char semantics[16][16];
    int i;

    InitValidationFixture(&fixture, HLSL_STAGE_PIXEL);
    fixture.outputStruct->members->semantic = "col0";
    for (i = 0; i < 4; i++) {
        sprintf(names[i], "color%d", i + 1);
        AppendInterfaceMember(&fixture.module, fixture.outputStruct,
                              names[i], mixedColors[i]);
    }
    assert(!HlslValidateModule(&fixture.module, &HlslProfile_hlslf));
    assert(fixture.module.errorKind == HLSL_ERROR_RESOURCE_LIMIT);
    assert(!strcmp(fixture.module.resourceName, "color outputs"));
    assert(fixture.module.resourceUsed == 5);

    InitValidationFixture(&fixture, HLSL_STAGE_PIXEL);
    fixture.outputStruct->members->semantic = "color4";
    assert(!HlslValidateModule(&fixture.module, &HlslProfile_hlslf));
    assert(fixture.module.errorKind == HLSL_ERROR_RESOURCE_LIMIT);
    assert(fixture.module.resourceUsed == 5);

    InitValidationFixture(&fixture, HLSL_STAGE_VERTEX);
    member = fixture.outputStruct->members;
    member->type = HlslNumericType(HLSL_BASE_FLOAT, 2);
    AssertSemanticValidationFixture(&fixture, &HlslProfile_hlslv);

    InitValidationFixture(&fixture, HLSL_STAGE_VERTEX);
    member = fixture.outputStruct->members;
    member->type = HlslNumericType(HLSL_BASE_INT, 4);
    AssertSemanticValidationFixture(&fixture, &HlslProfile_hlslv);

    InitValidationFixture(&fixture, HLSL_STAGE_VERTEX);
    for (i = 0; i < 16; i++) {
        sprintf(names[i], "input%d", i + 1);
        sprintf(semantics[i], i == 15 ? "INVALID0" : "TEXCOORD%d", i);
        AppendInterfaceMember(&fixture.module, fixture.inputStruct,
                              names[i], semantics[i]);
    }
    AssertSemanticValidationFixture(&fixture, &HlslProfile_hlslv);
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
    assert(!strcmp(HlslCanonicalSemantic(&HlslProfile_hlslf,
        "COLOR4", 1), "COLOR4"));
    assert(HlslCanonicalSemantic(&HlslProfile_hlslf,
        "COLOR5", 1) == NULL);
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
    TestLogicalBindingSurvivesLegalizeAndAllocation();
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
    TestLocatedExpression();
    TestBuiltinSignatures();
    TestTextureBuiltinSignatures();
    TestTextureAndSamplerRejections();
    TestHlslErrorMappingAndFirstFailure();
    TestStructuralValidatorRejectsMalformedGraphs();
    TestTargetValidatorInterfaceLimits();
    TestStructuralValidatorRejectsCyclicSignatureGraphs();
    TestStructuralValidatorPreflightsBeforeInitializers();
    TestTargetValidatorRejectsMalformedBindingGraphs();
    TestTargetValidatorPreflightsAllBindingLeafLists();
    TestTargetValidatorCanonicalResourcesAndTypes();
    TestInterfaceMetadataWriter();
    TestLoopAttributeWriter();
    return 0;
}

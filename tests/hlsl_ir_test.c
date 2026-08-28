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

static void TestReservedNames(void)
{
    static const char *requiredNames[] = {
        "const_cast", "delete", "dynamic_cast", "export", "mutable",
        "PixelShader", "pixelshader", "pixelfragment", "static_cast",
        "VertexShader", "vertexshader", "vertexfragment"
    };
    static const char *numericBases[] = {
        "bool", "cfloat", "char", "cint", "double", "dword", "fixed",
        "float", "half", "int", "long", "short", "uchar", "uint",
        "ulong", "ushort"
    };
    HlslModule module;
    char spelling[32];
    const char *emitted;
    const char *reserved;
    int index;

    HlslInitModule(&module, HLSL_STAGE_VERTEX, TestAlloc, NULL);
    for (index = 0; index < HlslReservedNameCount(); index++) {
        reserved = HlslReservedNameAt(index);
        assert(reserved != NULL);
        assert(HlslIsReservedName(reserved));
        emitted = HlslAllocateDistinctName(&module, reserved);
        assert(emitted != NULL);
        assert(!strncmp(emitted, "cg_", 3));
    }
    assert(HlslReservedNameAt(-1) == NULL);
    assert(HlslReservedNameAt(HlslReservedNameCount()) == NULL);
    for (index = 0; index < (int) (sizeof(requiredNames) /
                                    sizeof(requiredNames[0])); index++)
    {
        assert(HlslIsReservedName(requiredNames[index]));
    }
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

int main(int argc, char **argv)
{
    HlslModule module;
    HlslType scalar;
    HlslType vector;
    HlslType matrix;
    int firstIdentity;
    int secondIdentity;
    int cfloatIdentity;
    int cintIdentity;

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
    assert(!strcmp(HlslAllocateSymbolName(&module, &cfloatIdentity,
                                          "cfloat"), "cg_cfloat"));
    assert(!strcmp(HlslAllocateSymbolName(&module, &cintIdentity,
                                          "cint"), "cg_cint"));
    TestReservedNames();
    TestTypeRegisterSpans();
    TestDeclarationQualifiers();
    TestModuleWriter();
    TestNamesAndAllocationFailure();
    TestTypesAndLists();
    return 0;
}

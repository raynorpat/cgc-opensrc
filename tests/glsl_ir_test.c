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
// glsl_ir_test.c
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
#include "errors.h"
#include "glsl_ir.h"

int GlslWriteModule(FILE *out, const GlslModule *module);

static void *TestAlloc(void *arg, size_t size)
{
    return calloc(1, size);
}

static int allocationCount;

static void *CountingAlloc(void *arg, size_t size)
{
    allocationCount++;
    return calloc(1, size);
}

static void *DirtyAlloc(void *arg, size_t size)
{
    void *memory;

    memory = malloc(size);
    memset(memory, 0xa5, size);
    return memory;
}

static void *FailingAlloc(void *arg, size_t size)
{
    return NULL;
}

static void ExpectModuleRejected(const char *label,
                                 const GlslModule *module)
{
    FILE *writer;
    int wrote;
    long size;

    writer = tmpfile();
    assert(writer != NULL);
    wrote = GlslWriteModule(writer, module);
    size = ftell(writer);
    if (wrote || size != 0) {
        fprintf(stderr, "%s unexpectedly wrote %ld bytes\n", label, size);
        fclose(writer);
        exit(2);
    }
    assert(!fclose(writer));
}

static int DiagnosticMatches(int code, const char *message,
                             int expectedCode,
                             const char *expectedMessage)
{
    return code == expectedCode && !strcmp(message, expectedMessage);
}

int main(int argc, char **argv)
{
    GlslModule module;
    GlslModule dirtyModule;
    GlslModule collisionModule;
    GlslModule countModule;
    GlslModule gapModule;
    GlslModule overflowModule;
    GlslModule identityModule;
    GlslModule nameLocationModule;
    GlslModule failedNameLocationModule;
    GlslModule nullIdentityModule;
    GlslModule collisionLocationModule;
    GlslModule compatibilityCollisionLocationModule;
    GlslModule scopedModule;
    GlslModule visibleModule;
    GlslModule nonfiniteModule;
    GlslModule samplerModule;
    GlslType type;
    GlslDecl *decl;
    GlslDecl *secondDecl;
    GlslDecl *samplerDecl;
    GlslDecl *secondSamplerDecl;
    GlslDecl *thirdSamplerDecl;
    GlslDecl *numericDecl;
    GlslExpr *expr;
    GlslExpr *secondExpr;
    GlslExpr *conditionExpr;
    GlslExpr *coordExpr;
    GlslStmt *stmt;
    GlslStmt *secondStmt;
    GlslFunction *function;
    GlslFunction *secondFunction;
    GlslBinding *binding;
    GlslBinding *firstBinding;
    GlslBinding *secondBinding;
    GlslBinding *thirdBinding;
    GlslDecl *decls;
    GlslStmt *stmts;
    GlslFunction *functions;
    const char *emitted;
    int firstIdentity;
    int secondIdentity;
    int localNamespace;
    int memberNamespace;
    int functionIdentity;
    int typeIdentity;
    int globalIdentity;
    int localIdentity;
    int index;
    FILE *writer;
    float infinity;
    float nanValue;
    float samplerDefault;
    GlslType builtinParams[3];
    GlslType arrayElement;
    GlslType arrayType;
    GlslType structType;
    GlslLoc nameLocation;
    GlslLoc priorNameLocation;
    GlslDecl structMembers[2];
    GlslStmt *savedBody;
    static const char *builtinSpellings[] = {
        NULL,
        "mul", "dot", "cross", "normalize", "reflect", "refract",
        "length", "distance", "min", "max", "clamp", "abs", "sign",
        "floor", "ceil", "sqrt", "exp", "exp2", "log", "log2",
        "sin", "cos", "tan", "asin", "acos", "atan", "inversesqrt",
        "mix", "fract", "clamp",
        "texture", "texture", "texture", "texture",
        "textureProj", "textureProj", "textureProj", "textureProj",
        "textureLod", "textureLod", "textureLod", "textureLod"
    };

    if (argc == 2 && !strcmp(argv[1], "--verify-assertions-active")) {
        int assertionsActive;

        assertionsActive = 0;
        assert((assertionsActive = 1) != 0);
        if (!assertionsActive)
            return 2;
        puts("glsl-ir-assertions-active");
        return 0;
    }

    GlslInitModule(&module, GLSL_STAGE_VERTEX, TestAlloc, NULL);
    assert(DiagnosticMatches(ERROR_S_GLSL_UNSUPPORTED_TYPE, 6200,
        "GLSL 1.10 does not support type \"%s\""));
    assert(DiagnosticMatches(ERROR_S_GLSL_UNSUPPORTED_OPERATION, 6201,
        "GLSL 1.10 does not support operation \"%s\""));
    assert(DiagnosticMatches(ERROR_SS_GLSL_STAGE_OPERATION, 6202,
        "%s profile does not support operation \"%s\""));
    assert(DiagnosticMatches(ERROR_S_GLSL_SEMANTIC, 6203,
        "GLSL profile cannot bind semantic \"%s\""));
    assert(DiagnosticMatches(ERROR_S_GLSL_INTERFACE_CONFLICT, 6204,
        "GLSL interface conflicts at semantic \"%s\""));
    assert(DiagnosticMatches(ERROR_S_GLSL_NAME_COLLISION, 6205,
        "GLSL name cannot be resolved for \"%s\""));
    assert(DiagnosticMatches(ERROR_S_GLSL_INTRINSIC, 6206,
        "GLSL 1.10 has no exact intrinsic for \"%s\""));
    assert(DiagnosticMatches(ERROR_SII_GLSL_RESOURCE_LIMIT, 6207,
        "GLSL portable %s limit exceeded: %d used, %d available"));
    assert(DiagnosticMatches(ERROR_S_GLSL_SAMPLER, 6208,
        "GLSL 1.10 does not support sampler feature \"%s\""));
    assert(DiagnosticMatches(ERROR_S_GLSL_NON_SQUARE_MATRIX, 6209,
        "GLSL 1.10 requires a square matrix, found \"%s\""));
    assert(GlslSamplerUnitMatches("0", 0));
    assert(GlslSamplerUnitMatches("1", 1));
    assert(!GlslSamplerUnitMatches("0", 1));
    assert(!GlslSamplerUnitMatches("1", 0));
    assert(!GlslSamplerUnitMatches("01", 1));
    assert(!GlslSamplerUnitMatches("2147483648", 1));
    assert(!strcmp(GlslAllocateName(&module, "position"), "position"));
    assert(!strcmp(GlslAllocateName(&module, "attribute"), "cg_attribute"));
    assert(!strcmp(GlslAllocateName(&module, "gl_Position"), "cg_gl_Position"));
    assert(!strcmp(GlslAllocateName(&module, "position"), "position"));
    assert(!strcmp(GlslAllocateDistinctName(&module, "position"), "position_1"));

    type = GlslNumericType(GLSL_BASE_FLOAT, 1);
    assert(!strcmp(GlslTypeName(&type), "float"));
    type = GlslNumericType(GLSL_BASE_FLOAT, 4);
    assert(!strcmp(GlslTypeName(&type), "vec4"));
    type = GlslNumericType(GLSL_BASE_INT, 3);
    assert(!strcmp(GlslTypeName(&type), "ivec3"));
    type = GlslMatrixType(3);
    assert(!strcmp(GlslTypeName(&type), "mat3"));

    for (index = GLSL_BUILTIN_MUL; index <= GLSL_BUILTIN_TEXCUBE_LOD;
         index++)
    {
        assert(GlslBuiltinSpelling((GlslBuiltin) index) != NULL);
        assert(!strcmp(GlslBuiltinSpelling((GlslBuiltin) index),
                       builtinSpellings[index]));
    }
    assert(GlslBuiltinSpelling(GLSL_BUILTIN_NONE) == NULL);
    assert(GlslBuiltinSpelling((GlslBuiltin) 999) == NULL);

    /* Core 1.50 contract: stage-directional interface spellings and
     * modern texture intrinsic names come from the shared tables. */
    assert(strcmp(GlslStorageSpelling(GLSL_STAGE_VERTEX,
              GLSL_STORAGE_INPUT), "in") == 0);
    assert(strcmp(GlslStorageSpelling(GLSL_STAGE_VERTEX,
              GLSL_STORAGE_OUTPUT), "out") == 0);
    assert(strcmp(GlslStorageSpelling(GLSL_STAGE_FRAGMENT,
              GLSL_STORAGE_INPUT), "in") == 0);
    assert(strcmp(GlslStorageSpelling(GLSL_STAGE_FRAGMENT,
              GLSL_STORAGE_OUTPUT), "out") == 0);
    assert(GlslStorageSpelling(GLSL_STAGE_VERTEX,
                               GLSL_STORAGE_UNIFORM) == NULL);
    assert(GlslStorageSpelling(GLSL_STAGE_FRAGMENT,
                               GLSL_STORAGE_SAMPLER) == NULL);
    assert(strcmp(GlslBuiltinSpelling(GLSL_BUILTIN_TEX2D),
                  "texture") == 0);
    assert(strcmp(GlslBuiltinSpelling(GLSL_BUILTIN_TEX2D_PROJ),
                  "textureProj") == 0);
    assert(strcmp(GlslBuiltinSpelling(GLSL_BUILTIN_TEX2D_LOD),
                  "textureLod") == 0);
    assert(strcmp(GlslBuiltinSpelling(GLSL_BUILTIN_TEX1D),
                  "texture") == 0);
    assert(strcmp(GlslBuiltinSpelling(GLSL_BUILTIN_TEX3D),
                  "texture") == 0);
    assert(strcmp(GlslBuiltinSpelling(GLSL_BUILTIN_TEXCUBE),
                  "texture") == 0);

    type = GlslNumericType(GLSL_BASE_FLOAT, 4);
    builtinParams[0] = type;
    assert(GlslLookupBuiltin("rsqrt", &type, builtinParams, 1) ==
           GLSL_BUILTIN_RSQRT);
    builtinParams[1] = type;
    builtinParams[2] = GlslNumericType(GLSL_BASE_FLOAT, 1);
    assert(GlslLookupBuiltin("lerp", &type, builtinParams, 3) ==
           GLSL_BUILTIN_LERP);
    assert(GlslLookupBuiltin("unknown", &type, builtinParams, 1) ==
           GLSL_BUILTIN_NONE);
    assert(GlslIsBuiltinName("rsqrt"));
    assert(GlslIsBuiltinName("lerp"));
    assert(!GlslIsBuiltinName("unknown"));

    builtinParams[0] = GlslMatrixType(4);
    builtinParams[1] = GlslNumericType(GLSL_BASE_FLOAT, 4);
    type = GlslNumericType(GLSL_BASE_FLOAT, 4);
    assert(GlslLookupBuiltin("mul", &type, builtinParams, 2) ==
           GLSL_BUILTIN_MUL);
    builtinParams[0] = GlslNumericType(GLSL_BASE_FLOAT, 3);
    builtinParams[1] = GlslNumericType(GLSL_BASE_FLOAT, 3);
    type = GlslNumericType(GLSL_BASE_FLOAT, 1);
    assert(GlslLookupBuiltin("dot", &type, builtinParams, 2) ==
           GLSL_BUILTIN_DOT);
    builtinParams[0] = GlslNumericType(GLSL_BASE_FLOAT, 3);
    type = GlslNumericType(GLSL_BASE_FLOAT, 3);
    assert(GlslLookupBuiltin("normalize", &type, builtinParams, 1) ==
           GLSL_BUILTIN_NORMALIZE);
    builtinParams[0] = GlslNumericType(GLSL_BASE_FLOAT, 3);
    builtinParams[1] = GlslNumericType(GLSL_BASE_FLOAT, 3);
    builtinParams[2] = GlslNumericType(GLSL_BASE_FLOAT, 1);
    type = GlslNumericType(GLSL_BASE_FLOAT, 3);
    assert(GlslLookupBuiltin("refract", &type, builtinParams, 3) ==
           GLSL_BUILTIN_REFRACT);
    builtinParams[0] = GlslNumericType(GLSL_BASE_FLOAT, 3);
    builtinParams[1] = GlslNumericType(GLSL_BASE_FLOAT, 1);
    builtinParams[2] = GlslNumericType(GLSL_BASE_FLOAT, 1);
    assert(GlslLookupBuiltin("clamp", &type, builtinParams, 3) ==
           GLSL_BUILTIN_CLAMP);
    builtinParams[1] = GlslNumericType(GLSL_BASE_FLOAT, 3);
    assert(GlslLookupBuiltin("lerp", &type, builtinParams, 3) ==
           GLSL_BUILTIN_LERP);
    assert(GlslLookupBuiltin("not_a_builtin", &type, builtinParams, 3) ==
           GLSL_BUILTIN_NONE);
    builtinParams[0] = GlslNumericType(GLSL_BASE_INT, 3);
    assert(GlslLookupBuiltin("normalize", &type, builtinParams, 1) ==
           GLSL_BUILTIN_NONE);
    builtinParams[0] = GlslNumericType(GLSL_BASE_FLOAT, 3);
    builtinParams[0].members = structMembers;
    assert(GlslLookupBuiltin("normalize", &type, builtinParams, 1) ==
           GLSL_BUILTIN_NONE);
    builtinParams[0] = GlslMatrixType(3);
    builtinParams[1] = GlslNumericType(GLSL_BASE_FLOAT, 4);
    assert(GlslLookupBuiltin("mul", &type, builtinParams, 2) ==
           GLSL_BUILTIN_NONE);

    type = GlslNumericType(GLSL_BASE_FLOAT, 4);
    builtinParams[0] = GlslNumericType(GLSL_BASE_SAMPLER1D, 1);
    builtinParams[1] = GlslNumericType(GLSL_BASE_FLOAT, 1);
    assert(GlslLookupBuiltin("tex1D", &type, builtinParams, 2) ==
           GLSL_BUILTIN_TEX1D);
    builtinParams[0] = GlslNumericType(GLSL_BASE_SAMPLER2D, 1);
    builtinParams[1] = GlslNumericType(GLSL_BASE_FLOAT, 2);
    assert(GlslLookupBuiltin("tex2D", &type, builtinParams, 2) ==
           GLSL_BUILTIN_TEX2D);
    builtinParams[0] = GlslNumericType(GLSL_BASE_SAMPLER3D, 1);
    builtinParams[1] = GlslNumericType(GLSL_BASE_FLOAT, 3);
    assert(GlslLookupBuiltin("tex3D", &type, builtinParams, 2) ==
           GLSL_BUILTIN_TEX3D);
    builtinParams[0] = GlslNumericType(GLSL_BASE_SAMPLERCUBE, 1);
    assert(GlslLookupBuiltin("texCUBE", &type, builtinParams, 2) ==
           GLSL_BUILTIN_TEXCUBE);
    builtinParams[1] = GlslNumericType(GLSL_BASE_FLOAT, 2);
    assert(GlslLookupBuiltin("texCUBE", &type, builtinParams, 2) ==
           GLSL_BUILTIN_NONE);
    builtinParams[0] = GlslNumericType(GLSL_BASE_SAMPLER2D, 1);
    builtinParams[1] = GlslNumericType(GLSL_BASE_FLOAT, 3);
    assert(GlslLookupBuiltin("tex2D", &type, builtinParams, 2) ==
           GLSL_BUILTIN_NONE);
    type = GlslNumericType(GLSL_BASE_FLOAT, 3);
    builtinParams[1] = GlslNumericType(GLSL_BASE_FLOAT, 2);
    assert(GlslLookupBuiltin("tex2D", &type, builtinParams, 2) ==
           GLSL_BUILTIN_NONE);
    type = GlslNumericType(GLSL_BASE_FLOAT, 4);
    builtinParams[0] = GlslNumericType(GLSL_BASE_SAMPLER3D, 1);
    assert(GlslLookupBuiltin("tex2D", &type, builtinParams, 2) ==
           GLSL_BUILTIN_NONE);
    builtinParams[0] = GlslNumericType(GLSL_BASE_SAMPLER2D, 1);
    builtinParams[2] = GlslNumericType(GLSL_BASE_FLOAT, 1);
    assert(GlslLookupBuiltin("tex2D", &type, builtinParams, 3) ==
           GLSL_BUILTIN_NONE);
    assert(GlslLookupBuiltin("texture2D", &type, builtinParams, 2) ==
           GLSL_BUILTIN_NONE);

    type = GlslNumericType(GLSL_BASE_FLOAT, 4);
    assert(GlslTypeComponentCount(&type) == 4);
    arrayType = type;
    arrayType.arraySize = 17;
    assert(GlslTypeComponentCount(&arrayType) == 68);
    type = GlslMatrixType(3);
    assert(GlslTypeComponentCount(&type) == 9);
    arrayElement = GlslMatrixType(2);
    arrayType = GlslNumericType(GLSL_BASE_VOID, 0);
    arrayType.arraySize = 3;
    arrayType.elementType = &arrayElement;
    assert(GlslTypeComponentCount(&arrayType) == 12);
    arrayType.arraySize = 0;
    assert(GlslTypeComponentCount(&arrayType) == 0);
    arrayElement = GlslNumericType(GLSL_BASE_FLOAT, 2);
    arrayType.arraySize = INT_MAX;
    arrayType.elementType = &arrayElement;
    assert(GlslTypeComponentCount(&arrayType) == 0);
    arrayType.arraySize = 2;
    arrayType.elementType = &arrayType;
    assert(GlslTypeComponentCount(&arrayType) == 0);
    type = GlslNumericType(GLSL_BASE_SAMPLER2D, 1);
    assert(GlslTypeComponentCount(&type) == 0);
    memset(structMembers, 0, sizeof(structMembers));
    structMembers[0].type = GlslNumericType(GLSL_BASE_FLOAT, 3);
    structMembers[0].next = &structMembers[1];
    arrayElement = GlslMatrixType(2);
    arrayType = GlslNumericType(GLSL_BASE_VOID, 0);
    arrayType.arraySize = 2;
    arrayType.elementType = &arrayElement;
    structMembers[1].type = arrayType;
    structType = GlslNumericType(GLSL_BASE_STRUCT, 0);
    structType.structName = "ResourceBlock";
    structType.members = structMembers;
    assert(GlslTypeComponentCount(&structType) == 11);
    memset(structMembers, 0, sizeof(structMembers));
    structType = GlslNumericType(GLSL_BASE_STRUCT, 0);
    structType.structName = "RecursiveBlock";
    structType.members = structMembers;
    structMembers[0].type = structType;
    assert(GlslTypeComponentCount(&structType) == 0);

    assert(!strcmp(GlslAllocateSymbolName(&module, &firstIdentity, "value"),
        "value"));
    assert(!strcmp(GlslAllocateSymbolName(&module, &firstIdentity, "other"),
        "value"));
    assert(!strcmp(GlslAllocateSymbolName(&module, &secondIdentity, "value"),
        "value_1"));
    GlslInitModule(&identityModule, GLSL_STAGE_VERTEX, TestAlloc, NULL);
    assert(!strcmp(GlslAllocateSymbolName(&identityModule,
                                          &globalIdentity,
                                          "position"), "position"));
    assert(!strcmp(GlslAllocateSymbolName(&identityModule,
                                          &globalIdentity,
                                          "position"),
                   "position"));
    assert(!strcmp(GlslAllocateSymbolName(&identityModule,
                                          &localIdentity,
                                          "position"), "position_1"));

    GlslInitModule(&visibleModule, GLSL_STAGE_VERTEX, TestAlloc, NULL);
    assert(!strcmp(GlslAllocateSymbolName(&visibleModule,
                   &functionIdentity, "main"), "main"));
    assert(!strcmp(GlslAllocateSymbolName(&visibleModule,
                   &typeIdentity, "main"), "main_1"));
    assert(!strcmp(GlslAllocateSymbolName(&visibleModule,
                   &globalIdentity, "cg_ATTRIB0"), "cg_ATTRIB0"));
    assert(!strcmp(GlslAllocateSymbolName(&visibleModule,
                   &localIdentity, "cg_ATTRIB0"), "cg_ATTRIB0_1"));
    assert(!strcmp(GlslAllocateSymbolName(&visibleModule,
                   &globalIdentity, "renamed"), "cg_ATTRIB0"));

    GlslInitModule(&scopedModule, GLSL_STAGE_VERTEX, TestAlloc, NULL);
    assert(!strcmp(GlslAllocateScopedSymbolName(&scopedModule,
                   &localNamespace, &firstIdentity, "position"),
                   "position"));
    assert(!strcmp(GlslAllocateScopedSymbolName(&scopedModule,
                   &memberNamespace, &secondIdentity, "position"),
                   "position"));
    assert(!strcmp(GlslAllocateScopedSymbolName(&scopedModule,
                   &localNamespace, &secondIdentity, "output"),
                   "cg_output"));
    assert(!strcmp(GlslAllocateScopedSymbolName(&scopedModule,
                   &localNamespace, &secondIdentity, "other"),
                   "cg_output"));
    assert(!strcmp(GlslAllocateScopedSymbolName(&scopedModule,
                   &localNamespace, &localNamespace, "output"),
                   "cg_output_1"));
    assert(!strcmp(GlslAllocateScopedSymbolName(&scopedModule,
                   &memberNamespace, NULL, "member"), "member"));
    assert(!strcmp(GlslAllocateScopedSymbolName(&scopedModule,
                   &memberNamespace, NULL, "otherMember"),
                   "otherMember"));
    assert(!strcmp(GlslAllocateSymbolName(&module, &secondIdentity, "value"),
        "value_1"));
    GlslInitModule(&nameLocationModule, GLSL_STAGE_VERTEX, TestAlloc, NULL);
    nameLocationModule.errorKind = GLSL_ERROR_UNSUPPORTED_OPERATION;
    nameLocationModule.errorReason = "prior diagnostic";
    priorNameLocation.file = 5;
    priorNameLocation.line = 7;
    nameLocationModule.errorLoc = priorNameLocation;
    nameLocation.file = 17;
    nameLocation.line = 23;
    assert(!strcmp(GlslAllocateNameAt(&nameLocationModule, "ordinary",
                                     &nameLocation), "ordinary"));
    assert(!strcmp(GlslAllocateDistinctNameAt(&nameLocationModule, "distinct",
                                             &nameLocation), "distinct"));
    assert(!strcmp(GlslAllocateSymbolNameAt(&nameLocationModule,
                                           &firstIdentity, "symbol",
                                           &nameLocation), "symbol"));
    assert(!strcmp(GlslAllocateScopedSymbolNameAt(&nameLocationModule,
                                                 &memberNamespace, NULL,
                                                 "member", &nameLocation),
                   "member"));
    assert(nameLocationModule.errorKind == GLSL_ERROR_UNSUPPORTED_OPERATION);
    assert(!strcmp(nameLocationModule.errorReason, "prior diagnostic"));
    assert(nameLocationModule.errorLoc.file == priorNameLocation.file);
    assert(nameLocationModule.errorLoc.line == priorNameLocation.line);

    GlslInitModule(&failedNameLocationModule, GLSL_STAGE_VERTEX, FailingAlloc,
                   NULL);
    failedNameLocationModule.errorKind = GLSL_ERROR_NAME_COLLISION;
    failedNameLocationModule.errorReason = "prior name collision";
    assert(GlslAllocateNameAt(&failedNameLocationModule, "ordinary",
                              &nameLocation) == NULL);
    assert(failedNameLocationModule.errorLoc.file == 0);
    assert(failedNameLocationModule.errorLoc.line == 0);
    assert(GlslAllocateDistinctNameAt(&failedNameLocationModule, "distinct",
                                      &nameLocation) == NULL);
    assert(failedNameLocationModule.errorLoc.file == 0);
    assert(failedNameLocationModule.errorLoc.line == 0);
    assert(GlslAllocateSymbolNameAt(&failedNameLocationModule,
                                    &firstIdentity, "symbol",
                                    &nameLocation) == NULL);
    assert(failedNameLocationModule.errorLoc.file == 0);
    assert(failedNameLocationModule.errorLoc.line == 0);
    failedNameLocationModule.errorLoc = priorNameLocation;
    assert(GlslAllocateNameAt(&failedNameLocationModule, NULL,
                              &nameLocation) == NULL);
    assert(GlslAllocateScopedSymbolNameAt(&failedNameLocationModule, NULL,
                                          NULL, "member",
                                          &nameLocation) == NULL);
    assert(GlslAllocateScopedSymbolNameAt(&failedNameLocationModule,
                                          &memberNamespace, NULL, "member",
                                          &nameLocation) == NULL);
    assert(failedNameLocationModule.errorLoc.file == priorNameLocation.file);
    assert(failedNameLocationModule.errorLoc.line == priorNameLocation.line);
    assert(failedNameLocationModule.errorKind == GLSL_ERROR_NAME_COLLISION);
    assert(!strcmp(failedNameLocationModule.errorReason,
                   "prior name collision"));

    GlslInitModule(&nullIdentityModule, GLSL_STAGE_VERTEX, TestAlloc, NULL);
    assert(!strcmp(GlslAllocateScopedSymbolNameAt(&nullIdentityModule,
                                                 &memberNamespace, NULL,
                                                 "member", &nameLocation),
                   "member"));
    nameLocation.line = 29;
    assert(!strcmp(GlslAllocateScopedSymbolNameAt(&nullIdentityModule,
                                                 &memberNamespace, NULL,
                                                 "member", &nameLocation),
                   "member"));
    assert(nullIdentityModule.errorKind == GLSL_ERROR_NONE);
    assert(nullIdentityModule.errorLoc.file == 0);
    assert(nullIdentityModule.errorLoc.line == 0);

    GlslInitModule(&collisionLocationModule, GLSL_STAGE_VERTEX, TestAlloc,
                   NULL);
    collisionLocationModule.errorKind = GLSL_ERROR_UNSUPPORTED_OPERATION;
    collisionLocationModule.errorReason = "prior diagnostic";
    collisionLocationModule.errorLoc = priorNameLocation;
    assert(GlslTestRaiseNameCollision(&collisionLocationModule,
                                      "colliding name",
                                      &nameLocation) == NULL);
    assert(collisionLocationModule.errorKind == GLSL_ERROR_NAME_COLLISION);
    assert(!strcmp(collisionLocationModule.errorReason, "colliding name"));
    assert(collisionLocationModule.errorLoc.file == nameLocation.file);
    assert(collisionLocationModule.errorLoc.line == nameLocation.line);

    GlslInitModule(&compatibilityCollisionLocationModule, GLSL_STAGE_VERTEX,
                   TestAlloc, NULL);
    compatibilityCollisionLocationModule.errorLoc = priorNameLocation;
    assert(GlslTestRaiseNameCollision(&compatibilityCollisionLocationModule,
                                      "fallback collision", NULL) == NULL);
    assert(compatibilityCollisionLocationModule.errorKind ==
           GLSL_ERROR_NAME_COLLISION);
    assert(!strcmp(compatibilityCollisionLocationModule.errorReason,
                   "fallback collision"));
    assert(compatibilityCollisionLocationModule.errorLoc.file == 0);
    assert(compatibilityCollisionLocationModule.errorLoc.line == 0);
    assert(GlslIsReservedName("attribute"));
    assert(GlslIsReservedName("gl_Position"));
    assert(GlslIsReservedName("user__name"));
    assert(!GlslIsReservedName("user_name"));
    for (index = 0; index < GlslReservedNameCount(); index++) {
        const char *reserved;

        reserved = GlslReservedNameAt(index);
        assert(reserved != NULL);
        assert(GlslIsReservedName(reserved));
        emitted = GlslAllocateDistinctName(&module, reserved);
        assert(emitted != NULL);
        assert(!strncmp(emitted, "cg_", 3));
        assert(!GlslIsReservedName(emitted));
    }
    assert(GlslReservedNameAt(-1) == NULL);
    assert(GlslReservedNameAt(GlslReservedNameCount()) == NULL);
    emitted = GlslAllocateName(&module, "user__name");
    assert(!strcmp(emitted, "cg_user_name"));
    assert(!GlslIsReservedName(emitted));
    emitted = GlslAllocateName(&module, "__foo");
    assert(!strcmp(emitted, "cg_foo"));
    assert(!GlslIsReservedName(emitted));
    emitted = GlslAllocateDistinctName(&module, "foo_");
    assert(!strcmp(emitted, "foo_"));
    assert(!GlslIsReservedName(emitted));
    emitted = GlslAllocateDistinctName(&module, "foo_");
    assert(!strcmp(emitted, "foo_1"));
    assert(!GlslIsReservedName(emitted));
    emitted = GlslAllocateDistinctName(&module, "gl");
    assert(!strcmp(emitted, "gl"));
    emitted = GlslAllocateDistinctName(&module, "gl");
    assert(!strcmp(emitted, "cg_gl_1"));
    assert(!GlslIsReservedName(emitted));

    GlslInitModule(&collisionModule, GLSL_STAGE_VERTEX, TestAlloc, NULL);
    assert(!strcmp(GlslAllocateDistinctName(&collisionModule, "gl"), "gl"));
    assert(!strcmp(GlslAllocateName(&collisionModule, "cg_gl_1"),
        "cg_gl_1"));
    emitted = GlslAllocateDistinctName(&collisionModule, "gl");
    assert(!strcmp(emitted, "cg_gl_2"));
    assert(!GlslIsReservedName(emitted));

    GlslInitModule(&gapModule, GLSL_STAGE_VERTEX, TestAlloc, NULL);
    assert(!strcmp(GlslAllocateName(&gapModule, "item_2"), "item_2"));
    assert(!strcmp(GlslAllocateDistinctName(&gapModule, "item"), "item"));
    assert(!strcmp(GlslAllocateDistinctName(&gapModule, "item"), "item_1"));

    GlslInitModule(&overflowModule, GLSL_STAGE_VERTEX, TestAlloc, NULL);
    assert(!strcmp(GlslAllocateName(&overflowModule, "item_2147483648"),
        "item_2147483648"));
    emitted = GlslAllocateDistinctName(&overflowModule, "item");
    assert(emitted != NULL);
    assert(!strcmp(emitted, "item"));
    emitted = GlslAllocateDistinctName(&overflowModule, "item");
    assert(emitted != NULL);
    assert(!strcmp(emitted, "item_1"));

    allocationCount = 0;
    GlslInitModule(&countModule, GLSL_STAGE_VERTEX, CountingAlloc, NULL);
    for (index = 0; index < 64; index++) {
        emitted = GlslAllocateDistinctName(&countModule, "item");
        if (index == 0)
            assert(!strcmp(emitted, "item"));
        if (index == 63)
            assert(!strcmp(emitted, "item_63"));
        assert(!GlslIsReservedName(emitted));
    }
    assert(allocationCount <= 5 * 64);

    type = GlslNumericType(GLSL_BASE_BOOL, 2);
    assert(!strcmp(GlslTypeName(&type), "bvec2"));
    type = GlslNumericType(GLSL_BASE_SAMPLER2D, 1);
    assert(!strcmp(GlslTypeName(&type), "sampler2D"));
    type.base = GLSL_BASE_STRUCT;
    type.len = 0;
    type.rows = 0;
    type.cols = 0;
    type.arraySize = 0;
    type.structName = "Light";
    type.elementType = NULL;
    assert(!strcmp(GlslTypeName(&type), "Light"));
    type = GlslNumericType(GLSL_BASE_FLOAT, 5);
    assert(GlslTypeName(&type) == NULL);
    type = GlslMatrixType(5);
    assert(GlslTypeName(&type) == NULL);
    type = GlslMatrixType(3);
    type.cols = 2;
    assert(GlslTypeName(&type) == NULL);
    type = GlslNumericType(GLSL_BASE_VOID, 0);
    type.arraySize = 1;
    assert(GlslTypeName(&type) == NULL);
    type = GlslNumericType(GLSL_BASE_VOID, 0);
    type.structName = "Bad";
    assert(GlslTypeName(&type) == NULL);
    type = GlslNumericType(GLSL_BASE_VOID, 0);
    type.elementType = &type;
    assert(GlslTypeName(&type) == NULL);
    type = GlslNumericType(GLSL_BASE_FLOAT, 1);
    type.structName = "Bad";
    assert(GlslTypeName(&type) == NULL);
    type = GlslNumericType(GLSL_BASE_FLOAT, 1);
    type.elementType = &type;
    assert(GlslTypeName(&type) == NULL);
    type = GlslNumericType(GLSL_BASE_STRUCT, 0);
    assert(GlslTypeName(&type) == NULL);
    type.structName = "";
    assert(GlslTypeName(&type) == NULL);
    type = GlslNumericType((GlslBase) 99, 1);
    assert(GlslTypeName(&type) == NULL);

    GlslInitModule(&dirtyModule, GLSL_STAGE_FRAGMENT, DirtyAlloc, NULL);
    type = GlslNumericType(GLSL_BASE_FLOAT, 4);
    decl = GlslNewDecl(&dirtyModule, GLSL_STORAGE_INPUT, type, "decl");
    assert(decl->next == NULL);
    assert(decl->storage == GLSL_STORAGE_INPUT);
    assert(decl->type.base == GLSL_BASE_FLOAT);
    assert(decl->type.len == 4);
    assert(!strcmp(decl->name, "decl"));
    assert(decl->loc.file == 0);
    assert(decl->loc.line == 0);
    assert(decl->initializer == NULL);
    assert(decl->identity == NULL);
    assert(decl->members == NULL);
    assert(decl->parameterQualifier == GLSL_PARAMETER_IN);
    assert(decl->interpolation == GLSL_INTERPOLATION_DEFAULT);
    expr = GlslNewExpr(&dirtyModule, GLSL_EXPR_FLOAT, type);
    assert(expr->next == NULL);
    assert(expr->kind == GLSL_EXPR_FLOAT);
    assert(expr->type.base == GLSL_BASE_FLOAT);
    assert(expr->type.len == 4);
    assert(expr->loc.file == 0);
    assert(expr->loc.line == 0);
    assert(expr->u.literalFloat == 0.0f);
    expr = GlslNewExpr(&dirtyModule, GLSL_EXPR_SYMBOL, type);
    assert(expr->u.symbol == NULL);
    expr = GlslNewExpr(&dirtyModule, GLSL_EXPR_INT, type);
    assert(expr->u.literalInt == 0);
    expr = GlslNewExpr(&dirtyModule, GLSL_EXPR_BOOL, type);
    assert(expr->u.literalBool == 0);
    expr = GlslNewExpr(&dirtyModule, GLSL_EXPR_UNARY, type);
    assert(expr->u.unary.op == 0);
    assert(expr->u.unary.operand == NULL);
    expr = GlslNewExpr(&dirtyModule, GLSL_EXPR_BINARY, type);
    assert(expr->u.binary.op == 0);
    assert(expr->u.binary.left == NULL);
    assert(expr->u.binary.right == NULL);
    expr = GlslNewExpr(&dirtyModule, GLSL_EXPR_CONDITIONAL, type);
    assert(expr->u.conditional.condition == NULL);
    assert(expr->u.conditional.trueExpr == NULL);
    assert(expr->u.conditional.falseExpr == NULL);
    expr = GlslNewExpr(&dirtyModule, GLSL_EXPR_MEMBER, type);
    assert(expr->u.member.object == NULL);
    assert(expr->u.member.decl == NULL);
    assert(expr->u.member.name == NULL);
    expr = GlslNewExpr(&dirtyModule, GLSL_EXPR_CALL, type);
    assert(expr->u.call.target == NULL);
    assert(expr->u.call.name == NULL);
    assert(expr->u.call.arguments == NULL);
    assert(expr->u.call.builtin == GLSL_BUILTIN_NONE);
    expr = GlslNewExpr(&dirtyModule, GLSL_EXPR_CONSTRUCT, type);
    assert(expr->u.construct.arguments == NULL);
    expr = GlslNewExpr(&dirtyModule, GLSL_EXPR_INDEX, type);
    assert(expr->u.index.object == NULL);
    assert(expr->u.index.index == NULL);
    expr = GlslNewExpr(&dirtyModule, GLSL_EXPR_SWIZZLE, type);
    assert(expr->u.swizzle.object == NULL);
    assert(expr->u.swizzle.mask == NULL);
    stmt = GlslNewStmt(&dirtyModule, GLSL_STMT_RETURN);
    assert(stmt->next == NULL);
    assert(stmt->kind == GLSL_STMT_RETURN);
    assert(stmt->loc.file == 0);
    assert(stmt->loc.line == 0);
    assert(stmt->u.returnExpr == NULL);
    stmt = GlslNewStmt(&dirtyModule, GLSL_STMT_EXPRESSION);
    assert(stmt->u.expression == NULL);
    stmt = GlslNewStmt(&dirtyModule, GLSL_STMT_IF);
    assert(stmt->u.ifStmt.condition == NULL);
    assert(stmt->u.ifStmt.trueBranch == NULL);
    assert(stmt->u.ifStmt.falseBranch == NULL);
    stmt = GlslNewStmt(&dirtyModule, GLSL_STMT_WHILE);
    assert(stmt->u.loop.condition == NULL);
    assert(stmt->u.loop.body == NULL);
    stmt = GlslNewStmt(&dirtyModule, GLSL_STMT_DO);
    assert(stmt->u.loop.condition == NULL);
    assert(stmt->u.loop.body == NULL);
    stmt = GlslNewStmt(&dirtyModule, GLSL_STMT_FOR);
    assert(stmt->u.forStmt.init == NULL);
    assert(stmt->u.forStmt.condition == NULL);
    assert(stmt->u.forStmt.step == NULL);
    assert(stmt->u.forStmt.body == NULL);
    stmt = GlslNewStmt(&dirtyModule, GLSL_STMT_BLOCK);
    assert(stmt->u.block == NULL);
    stmt = GlslNewStmt(&dirtyModule, GLSL_STMT_RETURN);
    function = GlslNewFunction(&dirtyModule, type, "main");
    assert(function->next == NULL);
    assert(function->result.base == GLSL_BASE_FLOAT);
    assert(function->result.len == 4);
    assert(!strcmp(function->name, "main"));
    assert(function->loc.file == 0);
    assert(function->loc.line == 0);
    assert(function->identity == NULL);
    assert(function->parameters == NULL);
    assert(function->locals == NULL);
    assert(function->body == NULL);
    assert(function->isEntry == 0);
    binding = GlslNewBinding(&dirtyModule, GLSL_STORAGE_BUILTIN,
        "gl_Position", "POSITION");
    assert(binding->next == NULL);
    assert(binding->storage == GLSL_STORAGE_BUILTIN);
    assert(!strcmp(binding->name, "gl_Position"));
    assert(!strcmp(binding->semantic, "POSITION"));
    assert(binding->interfaceKey == NULL);
    assert(binding->loc.file == 0);
    assert(binding->loc.line == 0);
    assert(binding->declaration == NULL);
    assert(binding->isOutput == 0);
    assert(binding->interpolation == GLSL_INTERPOLATION_DEFAULT);

    secondDecl = GlslNewDecl(&dirtyModule, GLSL_STORAGE_UNIFORM, type,
        "second");
    decls = NULL;
    GlslAppendDecl(&decls, decl);
    GlslAppendDecl(&decls, secondDecl);
    assert(decls == decl);
    assert(decls->next == secondDecl);
    assert(secondDecl->next == NULL);
    secondStmt = GlslNewStmt(&dirtyModule, GLSL_STMT_BREAK);
    stmts = NULL;
    GlslAppendStmt(&stmts, stmt);
    GlslAppendStmt(&stmts, secondStmt);
    assert(stmts == stmt);
    assert(stmts->next == secondStmt);
    assert(secondStmt->next == NULL);
    secondFunction = GlslNewFunction(&dirtyModule, type, "helper");
    functions = NULL;
    GlslAppendFunction(&functions, function);
    GlslAppendFunction(&functions, secondFunction);
    assert(functions == function);
    assert(functions->next == secondFunction);
    assert(secondFunction->next == NULL);

    infinity = FLT_MAX;
    infinity = infinity * 2.0f;
    nanValue = infinity - infinity;
    assert(infinity > FLT_MAX);
    assert(nanValue != nanValue);

    GlslInitModule(&nonfiniteModule, GLSL_STAGE_VERTEX, TestAlloc, NULL);
    type = GlslNumericType(GLSL_BASE_VOID, 0);
    function = GlslNewFunction(&nonfiniteModule, type, "main");
    assert(function != NULL);
    function->isEntry = 1;
    nonfiniteModule.entry = function;
    GlslAppendFunction(&nonfiniteModule.functions, function);
    stmt = GlslNewStmt(&nonfiniteModule, GLSL_STMT_EXPRESSION);
    type = GlslNumericType(GLSL_BASE_FLOAT, 1);
    expr = GlslNewExpr(&nonfiniteModule, GLSL_EXPR_FLOAT, type);
    expr->u.literalFloat = infinity;
    stmt->u.expression = expr;
    GlslAppendStmt(&function->body, stmt);
    writer = tmpfile();
    assert(writer != NULL);
    assert(!GlslWriteModule(writer, &nonfiniteModule));
    assert(ftell(writer) == 0);
    assert(!fclose(writer));

    GlslInitModule(&nonfiniteModule, GLSL_STAGE_VERTEX, TestAlloc, NULL);
    type = GlslNumericType(GLSL_BASE_VOID, 0);
    function = GlslNewFunction(&nonfiniteModule, type, "main");
    assert(function != NULL);
    function->isEntry = 1;
    nonfiniteModule.entry = function;
    GlslAppendFunction(&nonfiniteModule.functions, function);
    stmt = GlslNewStmt(&nonfiniteModule, GLSL_STMT_EXPRESSION);
    type = GlslNumericType(GLSL_BASE_FLOAT, 1);
    expr = GlslNewExpr(&nonfiniteModule, GLSL_EXPR_FLOAT, type);
    expr->u.literalFloat = nanValue;
    stmt->u.expression = expr;
    GlslAppendStmt(&function->body, stmt);
    writer = tmpfile();
    assert(writer != NULL);
    assert(!GlslWriteModule(writer, &nonfiniteModule));
    assert(ftell(writer) == 0);
    assert(!fclose(writer));

    GlslInitModule(&samplerModule, GLSL_STAGE_FRAGMENT, TestAlloc, NULL);
    type = GlslNumericType(GLSL_BASE_VOID, 0);
    function = GlslNewFunction(&samplerModule, type, "main");
    assert(function != NULL);
    function->isEntry = 1;
    samplerModule.entry = function;
    GlslAppendFunction(&samplerModule.functions, function);
    type = GlslNumericType(GLSL_BASE_SAMPLER2D, 1);
    samplerDecl = GlslNewDecl(&samplerModule, GLSL_STORAGE_SAMPLER,
                              type, "first");
    secondSamplerDecl = GlslNewDecl(&samplerModule, GLSL_STORAGE_SAMPLER,
                                    type, "second");
    assert(samplerDecl != NULL && secondSamplerDecl != NULL);
    GlslAppendDecl(&samplerModule.globals, samplerDecl);
    GlslAppendDecl(&samplerModule.globals, secondSamplerDecl);
    binding = GlslNewBinding(&samplerModule, GLSL_STORAGE_SAMPLER,
                             "first", "0");
    assert(binding != NULL);
    binding->declaration = samplerDecl;
    samplerModule.bindings = binding;
    binding = GlslNewBinding(&samplerModule, GLSL_STORAGE_SAMPLER,
                             "second", "1");
    assert(binding != NULL);
    binding->declaration = secondSamplerDecl;
    samplerModule.bindings->next = binding;
    expr = GlslNewExpr(&samplerModule, GLSL_EXPR_BINARY, type);
    assert(expr != NULL);
    expr->u.binary.op = GLSL_OP_ASSIGN;
    expr->u.binary.left = GlslNewExpr(&samplerModule, GLSL_EXPR_SYMBOL,
                                      type);
    expr->u.binary.right = GlslNewExpr(&samplerModule, GLSL_EXPR_SYMBOL,
                                       type);
    assert(expr->u.binary.left != NULL && expr->u.binary.right != NULL);
    expr->u.binary.left->u.symbol = samplerDecl;
    expr->u.binary.right->u.symbol = secondSamplerDecl;
    stmt = GlslNewStmt(&samplerModule, GLSL_STMT_EXPRESSION);
    assert(stmt != NULL);
    stmt->u.expression = expr;
    function->body = stmt;
    writer = tmpfile();
    assert(writer != NULL);
    assert(!GlslWriteModule(writer, &samplerModule));
    assert(ftell(writer) == 0);
    assert(!fclose(writer));

    GlslInitModule(&samplerModule, GLSL_STAGE_FRAGMENT, TestAlloc, NULL);
    type = GlslNumericType(GLSL_BASE_VOID, 0);
    function = GlslNewFunction(&samplerModule, type, "main");
    assert(function != NULL);
    function->isEntry = 1;
    samplerModule.entry = function;
    GlslAppendFunction(&samplerModule.functions, function);
    type = GlslNumericType(GLSL_BASE_SAMPLER2D, 1);
    samplerDecl = GlslNewDecl(&samplerModule, GLSL_STORAGE_SAMPLER,
                              type, "first");
    secondSamplerDecl = GlslNewDecl(&samplerModule, GLSL_STORAGE_SAMPLER,
                                    type, "second");
    assert(samplerDecl != NULL && secondSamplerDecl != NULL);
    GlslAppendDecl(&samplerModule.globals, samplerDecl);
    GlslAppendDecl(&samplerModule.globals, secondSamplerDecl);
    firstBinding = GlslNewBinding(&samplerModule, GLSL_STORAGE_SAMPLER,
                                  "first", "0");
    assert(firstBinding != NULL);
    firstBinding->declaration = samplerDecl;
    samplerModule.bindings = firstBinding;
    secondBinding = GlslNewBinding(&samplerModule, GLSL_STORAGE_SAMPLER,
                                   "second", "1");
    assert(secondBinding != NULL);
    secondBinding->declaration = secondSamplerDecl;
    samplerModule.bindings->next = secondBinding;
    conditionExpr = GlslNewExpr(&samplerModule, GLSL_EXPR_BOOL,
                                GlslNumericType(GLSL_BASE_BOOL, 1));
    expr = GlslNewExpr(&samplerModule, GLSL_EXPR_CONDITIONAL, type);
    assert(conditionExpr != NULL && expr != NULL);
    conditionExpr->u.literalBool = 1;
    expr->u.conditional.condition = conditionExpr;
    expr->u.conditional.trueExpr = GlslNewExpr(
        &samplerModule, GLSL_EXPR_SYMBOL, type);
    expr->u.conditional.falseExpr = GlslNewExpr(
        &samplerModule, GLSL_EXPR_SYMBOL, type);
    assert(expr->u.conditional.trueExpr != NULL &&
           expr->u.conditional.falseExpr != NULL);
    expr->u.conditional.trueExpr->u.symbol = samplerDecl;
    expr->u.conditional.falseExpr->u.symbol = secondSamplerDecl;
    coordExpr = GlslNewExpr(&samplerModule, GLSL_EXPR_CONSTRUCT,
                            GlslNumericType(GLSL_BASE_FLOAT, 2));
    assert(coordExpr != NULL);
    coordExpr->u.construct.arguments = GlslNewExpr(
        &samplerModule, GLSL_EXPR_FLOAT,
        GlslNumericType(GLSL_BASE_FLOAT, 1));
    secondExpr = GlslNewExpr(&samplerModule, GLSL_EXPR_FLOAT,
                             GlslNumericType(GLSL_BASE_FLOAT, 1));
    assert(coordExpr->u.construct.arguments != NULL && secondExpr != NULL);
    coordExpr->u.construct.arguments->next = secondExpr;
    expr->next = coordExpr;
    secondExpr = GlslNewExpr(&samplerModule, GLSL_EXPR_CALL,
                             GlslNumericType(GLSL_BASE_FLOAT, 4));
    assert(secondExpr != NULL);
    secondExpr->u.call.name = "texture";
    secondExpr->u.call.arguments = expr;
    secondExpr->u.call.builtin = GLSL_BUILTIN_TEX2D;
    stmt = GlslNewStmt(&samplerModule, GLSL_STMT_EXPRESSION);
    assert(stmt != NULL);
    stmt->u.expression = secondExpr;
    function->body = stmt;
    writer = tmpfile();
    assert(writer != NULL);
    assert(!GlslWriteModule(writer, &samplerModule));
    assert(ftell(writer) == 0);
    assert(!fclose(writer));

    expr = expr->u.conditional.trueExpr;
    expr->next = coordExpr;
    secondExpr->u.call.arguments = expr;
    samplerModule.bindings->storage = GLSL_STORAGE_UNIFORM;
    writer = tmpfile();
    assert(writer != NULL);
    assert(!GlslWriteModule(writer, &samplerModule));
    assert(ftell(writer) == 0);
    assert(!fclose(writer));
    samplerModule.bindings->storage = GLSL_STORAGE_SAMPLER;

    secondExpr->u.call.builtin = (GlslBuiltin) 999;
    writer = tmpfile();
    assert(writer != NULL);
    assert(!GlslWriteModule(writer, &samplerModule));
    assert(ftell(writer) == 0);
    assert(!fclose(writer));
    secondExpr->u.call.builtin = GLSL_BUILTIN_TEX2D;

    writer = tmpfile();
    assert(writer != NULL);
    assert(GlslWriteModule(writer, &samplerModule));
    assert(ftell(writer) > 0);
    assert(!fclose(writer));

    samplerDecl->name = NULL;
    firstBinding->name = "s";
    ExpectModuleRejected("sampler declaration without name",
                         &samplerModule);
    samplerDecl->name = "first";
    firstBinding->name = "first";

    expr->next = GlslNewExpr(&samplerModule, GLSL_EXPR_SYMBOL,
                             GlslNumericType(GLSL_BASE_FLOAT, 2));
    assert(expr->next != NULL);
    expr->next->u.symbol = secondSamplerDecl;
    secondExpr->u.call.arguments = expr;
    ExpectModuleRejected("sampler coordinate advertised as float2",
                         &samplerModule);
    expr->next = coordExpr;

    stmt->u.expression = GlslNewExpr(&samplerModule, GLSL_EXPR_SYMBOL,
                                     GlslNumericType(GLSL_BASE_FLOAT, 1));
    assert(stmt->u.expression != NULL);
    stmt->u.expression->u.symbol = samplerDecl;
    ExpectModuleRejected("sampler symbol advertised as float",
                         &samplerModule);
    stmt->u.expression = GlslNewExpr(&samplerModule, GLSL_EXPR_SYMBOL,
                                     GlslNumericType(GLSL_BASE_INT, 1));
    assert(stmt->u.expression != NULL);
    stmt->u.expression->u.symbol = samplerDecl;
    ExpectModuleRejected("sampler symbol advertised as int",
                         &samplerModule);
    stmt->u.expression = GlslNewExpr(&samplerModule, GLSL_EXPR_SYMBOL,
                                     GlslNumericType(GLSL_BASE_BOOL, 1));
    assert(stmt->u.expression != NULL);
    stmt->u.expression->u.symbol = samplerDecl;
    ExpectModuleRejected("sampler symbol advertised as bool",
                         &samplerModule);

    numericDecl = GlslNewDecl(&samplerModule, GLSL_STORAGE_UNIFORM,
        GlslNumericType(GLSL_BASE_FLOAT, 1), "number");
    assert(numericDecl != NULL);
    secondSamplerDecl->next = numericDecl;
    stmt->u.expression = GlslNewExpr(&samplerModule, GLSL_EXPR_SYMBOL,
                                     GlslNumericType(GLSL_BASE_INT, 1));
    assert(stmt->u.expression != NULL);
    stmt->u.expression->u.symbol = numericDecl;
    ExpectModuleRejected("ordinary symbol type mismatch", &samplerModule);
    secondSamplerDecl->next = NULL;
    stmt->u.expression = secondExpr;

    firstBinding->next = NULL;
    ExpectModuleRejected("sampler global without binding", &samplerModule);
    firstBinding->next = secondBinding;

    secondBinding->declaration = NULL;
    ExpectModuleRejected("sampler binding without declaration",
                         &samplerModule);
    secondBinding->declaration = secondSamplerDecl;

    secondBinding->declaration = samplerDecl;
    secondBinding->name = "first";
    ExpectModuleRejected("duplicate sampler declaration binding",
                         &samplerModule);
    secondBinding->declaration = secondSamplerDecl;
    secondBinding->name = "second";

    secondSamplerDecl->name = "first";
    secondBinding->name = "first";
    ExpectModuleRejected("duplicate sampler global name", &samplerModule);
    secondSamplerDecl->name = "second";
    secondBinding->name = "second";

    secondBinding->semantic = "0";
    ExpectModuleRejected("duplicate sampler unit", &samplerModule);
    secondBinding->semantic = "2";
    ExpectModuleRejected("fragment sampler unit two", &samplerModule);
    secondBinding->semantic = "01";
    ExpectModuleRejected("leading-zero sampler unit", &samplerModule);
    secondBinding->semantic = "+1";
    ExpectModuleRejected("noncanonical sampler unit", &samplerModule);
    secondBinding->semantic = "2147483648";
    ExpectModuleRejected("overflowing sampler unit", &samplerModule);
    secondBinding->semantic = "1";

    secondBinding->name = "wrong";
    ExpectModuleRejected("wrong sampler binding name", &samplerModule);
    secondBinding->name = "second";

    secondSamplerDecl->next = numericDecl;
    secondBinding->declaration = numericDecl;
    secondBinding->name = "number";
    ExpectModuleRejected("wrong sampler binding type", &samplerModule);
    secondBinding->declaration = secondSamplerDecl;
    secondBinding->name = "second";
    secondSamplerDecl->next = NULL;

    samplerDefault = 1.0f;
    secondBinding->defaultCount = 1;
    secondBinding->defaultValues = &samplerDefault;
    ExpectModuleRejected("sampler binding default metadata",
                         &samplerModule);
    secondBinding->defaultCount = 0;
    secondBinding->defaultValues = NULL;

    savedBody = function->body;
    function->body = NULL;
    samplerModule.stage = GLSL_STAGE_VERTEX;
    samplerModule.bindings = NULL;
    ExpectModuleRejected("vertex sampler global", &samplerModule);
    samplerModule.bindings = firstBinding;
    ExpectModuleRejected("vertex sampler binding", &samplerModule);
    samplerModule.stage = GLSL_STAGE_FRAGMENT;
    function->body = savedBody;

    samplerDecl->next = NULL;
    firstBinding->next = NULL;
    firstBinding->semantic = "1";
    ExpectModuleRejected("single sampler on unit one", &samplerModule);
    firstBinding->semantic = "0";
    samplerDecl->next = secondSamplerDecl;
    firstBinding->next = secondBinding;

    firstBinding->semantic = "1";
    secondBinding->semantic = "0";
    ExpectModuleRejected("reversed sampler units", &samplerModule);
    firstBinding->semantic = "0";
    secondBinding->semantic = "2";
    ExpectModuleRejected("noncontiguous sampler units", &samplerModule);
    secondBinding->semantic = "1";

    samplerDecl->next = NULL;
    ExpectModuleRejected("sampler binding without sampler global",
                         &samplerModule);
    samplerDecl->next = secondSamplerDecl;

    thirdSamplerDecl = GlslNewDecl(&samplerModule, GLSL_STORAGE_SAMPLER,
        GlslNumericType(GLSL_BASE_SAMPLER2D, 1), "third");
    thirdBinding = GlslNewBinding(&samplerModule, GLSL_STORAGE_SAMPLER,
                                  "third", "2");
    assert(thirdSamplerDecl != NULL && thirdBinding != NULL);
    thirdBinding->declaration = thirdSamplerDecl;
    secondSamplerDecl->next = thirdSamplerDecl;
    secondBinding->next = thirdBinding;

    /* Core 1.50 portable limit: sixteen texture units per stage, so
     * three samplers write successfully and seventeen are rejected. */
    writer = tmpfile();
    assert(writer != NULL);
    assert(GlslWriteModule(writer, &samplerModule));
    assert(ftell(writer) > 0);
    assert(!fclose(writer));
    secondSamplerDecl->next = NULL;
    secondBinding->next = NULL;

    {
        static char unitTexts[17][4];
        GlslDecl *manyDecls[17];
        GlslBinding *manyBindings[17];
        GlslFunction *unitFunction;
        GlslBinding *bindingTail;
        int samplerIndex;

        GlslInitModule(&module, GLSL_STAGE_FRAGMENT, TestAlloc, NULL);
        type = GlslNumericType(GLSL_BASE_VOID, 0);
        unitFunction = GlslNewFunction(&module, type, "main");
        assert(unitFunction != NULL);
        unitFunction->isEntry = 1;
        module.entry = unitFunction;
        GlslAppendFunction(&module.functions, unitFunction);
        bindingTail = NULL;
        /* Sixteen samplers sit exactly at the portable texture-unit
         * limit; adding one more must fail the writer's structural
         * check without emitting anything. */
        for (samplerIndex = 0; samplerIndex < 17; samplerIndex++) {
            sprintf(unitTexts[samplerIndex], "%d", samplerIndex);
            manyDecls[samplerIndex] = NULL;
            manyBindings[samplerIndex] = NULL;
            if (samplerIndex == 16)
                break;
            manyDecls[samplerIndex] = GlslNewDecl(&module,
                GLSL_STORAGE_SAMPLER,
                GlslNumericType(GLSL_BASE_SAMPLER2D, 1), "image");
            assert(manyDecls[samplerIndex] != NULL);
            manyDecls[samplerIndex]->name = GlslAllocateDistinctName(
                &module, "image");
            assert(manyDecls[samplerIndex]->name != NULL);
            GlslAppendDecl(&module.globals, manyDecls[samplerIndex]);
            manyBindings[samplerIndex] = GlslNewBinding(&module,
                GLSL_STORAGE_SAMPLER, manyDecls[samplerIndex]->name,
                unitTexts[samplerIndex]);
            assert(manyBindings[samplerIndex] != NULL);
            manyBindings[samplerIndex]->declaration =
                manyDecls[samplerIndex];
            if (bindingTail == NULL)
                module.bindings = manyBindings[samplerIndex];
            else
                bindingTail->next = manyBindings[samplerIndex];
            bindingTail = manyBindings[samplerIndex];
        }
        writer = tmpfile();
        assert(writer != NULL);
        assert(GlslWriteModule(writer, &module));
        assert(ftell(writer) > 0);
        assert(!fclose(writer));
        manyDecls[16] = GlslNewDecl(&module, GLSL_STORAGE_SAMPLER,
            GlslNumericType(GLSL_BASE_SAMPLER2D, 1), "image");
        assert(manyDecls[16] != NULL);
        manyDecls[16]->name = GlslAllocateDistinctName(&module, "image");
        assert(manyDecls[16]->name != NULL);
        GlslAppendDecl(&module.globals, manyDecls[16]);
        manyBindings[16] = GlslNewBinding(&module, GLSL_STORAGE_SAMPLER,
                                          manyDecls[16]->name, "16");
        assert(manyBindings[16] != NULL);
        manyBindings[16]->declaration = manyDecls[16];
        assert(bindingTail != NULL);
        bindingTail->next = manyBindings[16];
        writer = tmpfile();
        assert(writer != NULL);
        assert(!GlslWriteModule(writer, &module));
        assert(ftell(writer) == 0);
        assert(!fclose(writer));
    }

    GlslInitModule(&module, GLSL_STAGE_VERTEX, TestAlloc, NULL);
    writer = tmpfile();
    assert(writer != NULL);
    assert(!GlslWriteModule(writer, &module));
    assert(ftell(writer) == 0);
    assert(!fclose(writer));

    GlslInitModule(&module, GLSL_STAGE_VERTEX, TestAlloc, NULL);
    type = GlslNumericType(GLSL_BASE_VOID, 0);
    function = GlslNewFunction(&module, type, "main");
    assert(function != NULL);
    function->isEntry = 1;
    module.entry = function;
    GlslAppendFunction(&module.functions, function);
    stmt = GlslNewStmt(&module, GLSL_STMT_DISCARD);
    assert(stmt != NULL);
    GlslAppendStmt(&function->body, stmt);
    writer = tmpfile();
    assert(writer != NULL);
    assert(!GlslWriteModule(writer, &module));
    assert(ftell(writer) == 0);
    assert(!fclose(writer));

    GlslInitModule(&module, GLSL_STAGE_FRAGMENT, TestAlloc, NULL);
    type = GlslNumericType(GLSL_BASE_VOID, 0);
    function = GlslNewFunction(&module, type, "main");
    assert(function != NULL);
    function->isEntry = 1;
    module.entry = function;
    GlslAppendFunction(&module.functions, function);
    stmt = GlslNewStmt(&module, GLSL_STMT_DISCARD);
    assert(stmt != NULL);
    GlslAppendStmt(&function->body, stmt);
    writer = tmpfile();
    assert(writer != NULL);
    assert(GlslWriteModule(writer, &module));
    assert(ftell(writer) > 0);
    assert(!fclose(writer));

    /* ---- Geometry module: builders and basic writer exercise ---- */
    {
        GlslModule geoModule;
        GlslGeometryInfo geoInfo;
        GlslDecl *outDecl;
        GlslStmt *emitStmt, *restartStmt;
        GlslFlatReplay *replay;
        GlslFunction *geoFunc;
        GlslType float4Type;
        GlslDecl *shadowDecl, *definedDecl;

        GlslInitModule(&geoModule, GLSL_STAGE_GEOMETRY, TestAlloc, NULL);
        geoInfo.inputTopology = GLSL_GEOMETRY_INPUT_TRIANGLES_ADJACENCY;
        geoInfo.outputTopology = GLSL_GEOMETRY_OUTPUT_TRIANGLE_STRIP;
        geoInfo.inputVertexCount = 6;
        geoInfo.maxOutputVertices = 12;
        geoInfo.inputLoc.file = 0;
        geoInfo.inputLoc.line = 0;
        geoInfo.outputLoc.file = 0;
        geoInfo.outputLoc.line = 0;
        geoInfo.maxVerticesLoc.file = 0;
        geoInfo.maxVerticesLoc.line = 0;
        geoModule.geometry = &geoInfo;

        float4Type = GlslNumericType(GLSL_BASE_FLOAT, 4);
        outDecl = GlslNewDecl(&geoModule, GLSL_STORAGE_OUTPUT,
                              float4Type, "cg_COLOR0");
        assert(outDecl != NULL);
        GlslAppendDecl(&geoModule.globals, outDecl);
        shadowDecl = GlslNewDecl(&geoModule, GLSL_STORAGE_CONST,
                                 float4Type, "cg_flat_COLOR0");
        assert(shadowDecl != NULL);
        GlslAppendDecl(&geoModule.globals, shadowDecl);
        definedDecl = GlslNewDecl(&geoModule, GLSL_STORAGE_CONST,
                                  GlslNumericType(GLSL_BASE_BOOL, 0),
                                  "cg_flat_COLOR0_defined");
        assert(definedDecl != NULL);
        GlslAppendDecl(&geoModule.globals, definedDecl);

        replay = GlslNewFlatReplay(&geoModule, outDecl,
                                   shadowDecl, definedDecl);
        assert(replay != NULL);
        assert(replay->target == outDecl);
        assert(replay->shadow == shadowDecl);
        assert(replay->defined == definedDecl);

        emitStmt = GlslNewGeometryEmit(&geoModule, NULL, replay);
        assert(emitStmt != NULL);
        assert(emitStmt->kind == GLSL_STMT_GEOMETRY_EMIT);
        assert(emitStmt->u.emit.replay == replay);

        restartStmt = GlslNewGeometryRestart(&geoModule);
        assert(restartStmt != NULL);
        assert(restartStmt->kind == GLSL_STMT_GEOMETRY_RESTART);

        geoFunc = GlslNewFunction(&geoModule, float4Type, "main");
        assert(geoFunc != NULL);
        geoFunc->isEntry = 1;
        geoModule.entry = geoFunc;
        GlslAppendFunction(&geoModule.functions, geoFunc);
        GlslAppendStmt(&geoFunc->body, emitStmt);
        GlslAppendStmt(&geoFunc->body, restartStmt);

        /* Verify list shape: first statement is emit, second is restart. */
        assert(geoFunc->body->kind == GLSL_STMT_GEOMETRY_EMIT);
        assert(geoFunc->body->next->kind == GLSL_STMT_GEOMETRY_RESTART);
        assert(geoFunc->body->next->next == NULL);
    }

    /* ---- Geometry verifier: rejects missing geometry metadata ---- */
    {
        GlslModule badGeo;
        GlslVerifyDiagnostic diag;

        GlslInitModule(&badGeo, GLSL_STAGE_GEOMETRY, TestAlloc, NULL);
        badGeo.geometry = NULL;
        assert(!GlslVerifyModule(&badGeo, &diag));
        assert(diag.reason == GLSL_VERIFY_GEOMETRY);
    }

    return 0;
}

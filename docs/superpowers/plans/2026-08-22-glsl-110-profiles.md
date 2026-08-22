# GLSL 1.10 Profiles Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add strict, readable GLSL 1.10 vertex and fragment profiles named glslv and glslf without changing generic-profile behavior.

**Architecture:** Register two HAL profiles that share a typed, structured GLSL source representation, AST lowering, name allocation, resource validation, binding metadata, and deterministic source emission. Stage descriptors own connector semantics, built-ins, portable limits, sampler policy, and stage-specific diagnostics.

**Tech Stack:** C90-compatible C, the existing NVIDIA Cg front end and HAL, CMake 3.16+, CTest, Bison-compatible winflexbison 2.5.25 for synchronized parser regeneration, the checked-in tokenizer-generated standard library, and optional glslangValidator

---

## Approved Design and Standards

- Approved design: **docs/superpowers/specs/2026-08-22-glsl-110-profiles-design.md**
- HAL reference: **generic_hal.c** and **generic_hal.h**
- Semantic binding flow: **semantic.c:157-335** and **semantic.c:373-559**
- Final compile pipeline: **compile.c:2529-2615**
- AST node definitions: **support.h:69-247** and **support.h:332-489**
- Binding representation: **binding.h**
- GLSL 1.10 specification: <https://registry.khronos.org/OpenGL/specs/gl/GLSLangSpec.1.10.pdf>
- OpenGL 2.0 portable limits: <https://registry.khronos.org/OpenGL/specs/gl/glspec20.pdf>

## File Structure

### New backend files

- **glsl_ir.h** — compiler-independent GLSL stage, type, name, declaration,
  expression, statement, function, binding, and module data structures.
- **glsl_ir.c** — allocator-backed node construction, deterministic name
  allocation, keyword checks, type spelling, and IR unit-test seams.
- **glsl_hal.h** — profile IDs, connector IDs, sampler bases, descriptors,
  limits, intrinsic IDs, and integration entry points.
- **glsl_hal.c** — shared HAL callbacks, registration, common binding,
  intrinsic recognition, uniform allocation, and GenerateCode orchestration.
- **glslv_hal.c** — vertex connector/semantic tables, aliases, built-ins, and
  portable vertex limits.
- **glslf_hal.c** — fragment connector/semantic tables, aliases, built-ins,
  sampler rules, and portable fragment limits.
- **glsl_lower.c** — reachable-symbol collection and typed compiler-AST to
  GlslModule lowering.
- **glsl_codegen.c** — precedence-aware, deterministic, indented GLSL 1.10
  source emission.

### Modified compiler files

- **cgcmain.c:56-63** — register both profiles.
- **errors.h:265-275** — add GLSL diagnostics 6200 through 6209.
- **semantic.c:247-305** — suppress generic follow-on binding errors when a
  HAL callback already emitted a specific diagnostic.
- **support.h**, **support.c**, **support_iter.c**, **compile.c**,
  **inline.c**, and **printutils.c** — represent and traverse break/continue
  leaf statements already tokenized by the scanner.
- **parser.y**, **parser.c**, and **parser.h** — accept break and continue and
  keep generated grammar sources synchronized.
- **stdlib.cg** — use a shared GLSL guard for exact internal intrinsic
  overloads while retaining generic definitions outside that guard.
- **stdlib.c** — regenerate from stdlib.cg with the repository tokenizer.
- **CMakeLists.txt:13-40** — build GLSL backend sources.
- **Makefile:1** — build GLSL backend objects.
- **README.txt** — document profiles, interface naming, metadata, matrix
  uploads, limitations, and validation.

### New and modified tests

- **tests/check_generic.cmake** — normalize driver provenance and verify frozen
  generic SHA-256 baselines.
- **tests/check_glsl.cmake** — compile a successful fixture, normalize only
  driver provenance, optionally update an expectation, and compare exact GLSL.
- **tests/check_diagnostic.cmake** — require failure, an exact diagnostic code,
  and absence of a partial shader body.
- **tests/validate_glsl.cmake** — compile and invoke optional
  glslangValidator with the correct stage.
- **tests/check_glsl_interface.cmake** — compare vertex and fragment varying
  declarations.
- **tests/glsl_ir_test.c** — unit tests for name allocation, type spelling,
  semantic names, and IR construction.
- **tests/glsl/** — focused Cg fixtures and deterministic expectations.
- **tests/CMakeLists.txt** — helper functions and all new tests.

Use **build-glsl** as the implementation build directory. Do not add it to
Git. Preserve the unrelated untracked build-win32 directory and existing
untracked plan files.

### Task 1: Freeze generic-profile behavior

**Files:**
- Create: **tests/check_generic.cmake**
- Modify: **tests/CMakeLists.txt**

- [ ] **Step 1: Add the generic regression script**

Create **tests/check_generic.cmake** with this complete script:

~~~cmake
foreach(required CGC SOURCE EXPECTED_SHA256)
    if(NOT DEFINED ${required})
        message(FATAL_ERROR "${required} must be defined")
    endif()
endforeach()

execute_process(
    COMMAND "${CGC}" -quiet -profile generic "${SOURCE}"
    RESULT_VARIABLE result
    OUTPUT_VARIABLE output
    ERROR_VARIABLE error
)

if(NOT result EQUAL 0)
    message(FATAL_ERROR "generic compile failed (${result}):\n${output}${error}")
endif()

string(REPLACE "\r\n" "\n" output "${output}")
string(REPLACE "\r" "\n" output "${output}")
string(REGEX REPLACE "(^|\n)// cgc version [^\n]*\n" "\\1" output "${output}")
string(REGEX REPLACE "(^|\n)// command line args:[^\n]*\n" "\\1" output "${output}")
string(SHA256 actual_sha256 "${output}")

if(NOT actual_sha256 STREQUAL EXPECTED_SHA256)
    message(FATAL_ERROR
        "generic output changed for ${SOURCE}\n"
        "expected: ${EXPECTED_SHA256}\n"
        "actual:   ${actual_sha256}"
    )
endif()
~~~

- [ ] **Step 2: Replace smoke-only registrations with four frozen baselines**

Replace **tests/CMakeLists.txt** with:

~~~cmake
function(add_generic_regression name source expected_sha256)
    add_test(
        NAME generic_${name}
        COMMAND ${CMAKE_COMMAND}
            -DCGC=$<TARGET_FILE:cgc>
            -DSOURCE=${PROJECT_SOURCE_DIR}/${source}
            -DEXPECTED_SHA256=${expected_sha256}
            -P ${CMAKE_CURRENT_SOURCE_DIR}/check_generic.cmake
    )
endfunction()

add_generic_regression(position position.cg
    fbd55194fb43822d993f60915df94f7a0627fd9bea27013a8c2d1d0ccdfe7b8e)
add_generic_regression(reflection reflection.cg
    aa2c396563920d50a7b45a6a06c359ec865faa338de4ad42294143a1243b6d8b)
add_generic_regression(vertexlight vertexlight.cg
    a973a0f2820dd1777d0e2cdb593a93f1e0c6d3f5f8640b2017724e37f8fd3c81)
add_generic_regression(vertexlight4 vertexlight4.cg
    5454876aedd830f1d07b65e26cd51e7570035ee01d7d4b784ba4769632e31961)
~~~

- [ ] **Step 3: Configure and prove the baselines pass before GLSL work**

Run:

~~~powershell
cmake -S . -B build-glsl -DBUILD_TESTING=ON
cmake --build build-glsl --config Debug --target cgc
ctest --test-dir build-glsl -C Debug -R '^generic_' --output-on-failure
~~~

Expected: four tests run and CTest reports 100% passed.

- [ ] **Step 4: Commit the generic safety net**

Run:

~~~powershell
git add -- tests/CMakeLists.txt tests/check_generic.cmake
git commit -m "Freeze generic profile output"
~~~

Expected: one commit containing only the generic test harness and registrations.

### Task 2: Register glslv and glslf profile skeletons

**Files:**
- Create: **glsl_ir.h**
- Create: **glsl_hal.h**
- Create: **glsl_hal.c**
- Create: **glslv_hal.c**
- Create: **glslf_hal.c**
- Create: **tests/glsl/profile/empty.cg**
- Create: **tests/glsl/profile/check_header.cmake**
- Modify: **cgcmain.c:56-63**
- Modify: **CMakeLists.txt:13-40**
- Modify: **Makefile:1**
- Modify: **tests/CMakeLists.txt**

- [ ] **Step 1: Add one source fixture that needs no interface bindings**

Create **tests/glsl/profile/empty.cg**:

~~~c
void main()
{
}
~~~

Create **tests/glsl/profile/check_header.cmake**:

~~~cmake
foreach(required CGC PROFILE SOURCE)
    if(NOT DEFINED ${required})
        message(FATAL_ERROR "${required} must be defined")
    endif()
endforeach()

execute_process(
    COMMAND "${CGC}" -quiet -nocode -profile "${PROFILE}" "${SOURCE}"
    RESULT_VARIABLE result
    OUTPUT_VARIABLE output
    ERROR_VARIABLE error
)
if(NOT result EQUAL 0)
    message(FATAL_ERROR "${PROFILE} registration failed: ${output}${error}")
endif()
string(REPLACE "\r\n" "\n" output "${output}")
if(NOT output MATCHES "^#version 110\n")
    message(FATAL_ERROR "${PROFILE} did not emit #version 110 first:\n${output}")
endif()
~~~

Register tests named glslv_registration and glslf_registration using the same
fixture and script.

- [ ] **Step 2: Run the registrations in the red state**

Run:

~~~powershell
cmake -S . -B build-glsl -DBUILD_TESTING=ON
cmake --build build-glsl --config Debug --target cgc
ctest --test-dir build-glsl -C Debug -R '^glsl(v|f)_registration$' --output-on-failure
~~~

Expected: both tests fail because glslv and glslf are unknown profiles.

- [ ] **Step 3: Define shared stage and profile interfaces**

Create **glsl_ir.h** with the license notice copied exactly from
**generic_hal.h**, the include guard __GLSL_IR_H, and these declarations:

~~~c
#if !defined(__GLSL_IR_H)
#define __GLSL_IR_H 1

#include <stddef.h>

typedef enum GlslStage_Enum {
    GLSL_STAGE_VERTEX,
    GLSL_STAGE_FRAGMENT
} GlslStage;

typedef struct GlslModule_Rec GlslModule;

#endif
~~~

Create **glsl_hal.h** with the NVIDIA redistribution notice copied verbatim
from **generic_hal.h**, guard __GLSL_HAL_H, and:

~~~c
#if !defined(__GLSL_HAL_H)
#define __GLSL_HAL_H 1

#include "hal.h"
#include "glsl_ir.h"

#define VENDOR_STRING_GLSL "OpenGL"
#define VERSION_STRING_GLSL "1.10"

#define PROFILE_GLSLV_NAME "glslv"
#define PROFILE_GLSLV_ID 12
#define PROFILE_GLSLF_NAME "glslf"
#define PROFILE_GLSLF_ID 13

#define CID_GLSLV_IN_NAME "glslv_in"
#define CID_GLSLV_IN_ID 14
#define CID_GLSLV_OUT_NAME "glslv_out"
#define CID_GLSLV_OUT_ID 15
#define CID_GLSLF_IN_NAME "glslf_in"
#define CID_GLSLF_IN_ID 16
#define CID_GLSLF_OUT_NAME "glslf_out"
#define CID_GLSLF_OUT_ID 17

typedef struct GlslLimits_Rec {
    int attributes;
    int uniformComponents;
    int varyingComponents;
    int textureUnits;
    int colorOutputs;
} GlslLimits;

typedef struct GlslProfileDesc_Rec {
    GlslStage stage;
    const char *name;
    int pid;
    int inputCid;
    int outputCid;
    ConnectorDescriptor *connectors;
    int numConnectors;
    SemanticsDescriptor *semantics;
    int numSemantics;
    ConnectorRegisters *inputRegs;
    int numInputRegs;
    ConnectorRegisters *outputRegs;
    int numOutputRegs;
    GlslLimits limits;
} GlslProfileDesc;

int RegisterProfiles_glsl(void);
int GlslInitHAL(slHAL *hal, const GlslProfileDesc *profile);
int InitHAL_glslv(slHAL *hal);
int InitHAL_glslf(slHAL *hal);

#endif
~~~

- [ ] **Step 4: Implement common skeleton callbacks**

Create **glsl_hal.c** with the NVIDIA license, standard includes, and these
callbacks. The semantic, connector, uniform, type, and intrinsic callbacks
remain at their InitHAL_HAL defaults until their named tasks replace them:

~~~c
#include <stdio.h>
#include <string.h>
#include "slglobals.h"
#include "glsl_hal.h"

static int RegisterNames_glsl(slHAL *hal)
{
    return 1;
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

static int GetCapsBit_glsl(int bit)
{
    switch (bit) {
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

static int GenerateCode_glsl(SourceLoc *loc, Scope *scope, Symbol *program)
{
    return 1;
}

int GlslInitHAL(slHAL *hal, const GlslProfileDesc *profile)
{
    hal->RegisterNames = RegisterNames_glsl;
    hal->FreeHAL = FreeHAL_glsl;
    hal->GetCapsBit = GetCapsBit_glsl;
    hal->CheckInternalFunction = CheckInternalFunction_glsl;
    hal->PrintCodeHeader = PrintCodeHeader_glsl;
    hal->GenerateCode = GenerateCode_glsl;
    hal->vendor = VENDOR_STRING_GLSL;
    hal->version = VERSION_STRING_GLSL;
    hal->comment = "//";
    hal->incid = profile->inputCid;
    hal->outcid = profile->outputCid;
    hal->inputCRegs = profile->inputRegs;
    hal->numInputCRegs = profile->numInputRegs;
    hal->outputCRegs = profile->outputRegs;
    hal->numOutputCRegs = profile->numOutputRegs;
    hal->semantics = profile->semantics;
    hal->numSemantics = profile->numSemantics;
    hal->localData = (void *) profile;
    return 1;
}

int RegisterProfiles_glsl(void)
{
    RegisterProfile(InitHAL_glslv, PROFILE_GLSLV_NAME, PROFILE_GLSLV_ID);
    RegisterProfile(InitHAL_glslf, PROFILE_GLSLF_NAME, PROFILE_GLSLF_ID);
    return 1;
}
~~~

- [ ] **Step 5: Add stage descriptors with portable limits**

Create **glslv_hal.c** and **glslf_hal.c** with the NVIDIA license. For this
registration milestone, use zero-length behavior through NULL connector and
semantic arrays, and define:

~~~c
/* glslv_hal.c */
#include "slglobals.h"
#include "glsl_hal.h"

static GlslProfileDesc profile_glslv = {
    GLSL_STAGE_VERTEX, PROFILE_GLSLV_NAME, PROFILE_GLSLV_ID,
    CID_GLSLV_IN_ID, CID_GLSLV_OUT_ID,
    NULL, 0, NULL, 0, NULL, 0, NULL, 0,
    { 16, 512, 32, 0, 0 }
};

int InitHAL_glslv(slHAL *hal)
{
    return GlslInitHAL(hal, &profile_glslv);
}
~~~

~~~c
/* glslf_hal.c */
#include "slglobals.h"
#include "glsl_hal.h"

static GlslProfileDesc profile_glslf = {
    GLSL_STAGE_FRAGMENT, PROFILE_GLSLF_NAME, PROFILE_GLSLF_ID,
    CID_GLSLF_IN_ID, CID_GLSLF_OUT_ID,
    NULL, 0, NULL, 0, NULL, 0, NULL, 0,
    { 0, 64, 32, 2, 1 }
};

int InitHAL_glslf(slHAL *hal)
{
    return GlslInitHAL(hal, &profile_glslf);
}
~~~

- [ ] **Step 6: Register and build the new sources**

Declare RegisterProfiles_glsl and add it after RegisterProfiles_generic in
**cgcmain.c**. Add glsl_hal.c, glslv_hal.c, and glslf_hal.c to the cgc target
in **CMakeLists.txt**. Add glsl_hal.o, glslv_hal.o, and glslf_hal.o to OBJS in
**Makefile**.

- [ ] **Step 7: Build and prove both registrations are green**

Run:

~~~powershell
cmake -S . -B build-glsl -DBUILD_TESTING=ON
cmake --build build-glsl --config Debug --target cgc
ctest --test-dir build-glsl -C Debug -R '^(glsl(v|f)_registration|generic_)' --output-on-failure
~~~

Expected: both GLSL registration tests and all four generic regressions pass.

- [ ] **Step 8: Commit profile registration**

Run:

~~~powershell
git add -- glsl_ir.h glsl_hal.h glsl_hal.c glslv_hal.c glslf_hal.c cgcmain.c CMakeLists.txt Makefile tests/CMakeLists.txt tests/glsl/profile
git commit -m "Register GLSL 1.10 profiles"
~~~

### Task 3: Build compiler-independent GLSL IR primitives

**Files:**
- Modify: **glsl_ir.h**
- Create: **glsl_ir.c**
- Create: **tests/glsl_ir_test.c**
- Modify: **CMakeLists.txt**
- Modify: **tests/CMakeLists.txt**

- [ ] **Step 1: Add failing name and type unit tests**

Create **tests/glsl_ir_test.c** with the NVIDIA license and:

~~~c
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include "glsl_ir.h"

static void *TestAlloc(void *arg, size_t size)
{
    return calloc(1, size);
}

int main(void)
{
    GlslModule module;
    GlslType type;

    GlslInitModule(&module, GLSL_STAGE_VERTEX, TestAlloc, NULL);
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
    return 0;
}
~~~

Register a glsl_ir_unit executable from **glsl_ir.c** and
**tests/glsl_ir_test.c**, give it the repository root include directory, set
C_STANDARD 90, and register a CTest named glsl_ir_unit.

- [ ] **Step 2: Run the unit target in the red state**

Run:

~~~powershell
cmake -S . -B build-glsl -DBUILD_TESTING=ON
cmake --build build-glsl --config Debug --target glsl_ir_unit
~~~

Expected: compilation fails because the GlslModule, GlslType, and helper APIs
do not exist.

- [ ] **Step 3: Define the structured IR public data**

Expand **glsl_ir.h** with these exact public enums and constructors:

~~~c
typedef void *(*GlslAllocFn)(void *arg, size_t size);

typedef enum GlslBase_Enum {
    GLSL_BASE_VOID,
    GLSL_BASE_FLOAT,
    GLSL_BASE_INT,
    GLSL_BASE_BOOL,
    GLSL_BASE_SAMPLER1D,
    GLSL_BASE_SAMPLER2D,
    GLSL_BASE_SAMPLER3D,
    GLSL_BASE_SAMPLERCUBE,
    GLSL_BASE_STRUCT
} GlslBase;

typedef struct GlslType_Rec {
    GlslBase base;
    int len;
    int rows;
    int cols;
    int arraySize;
    const char *structName;
    struct GlslType_Rec *elementType;
} GlslType;

typedef struct GlslName_Rec {
    struct GlslName_Rec *next;
    const void *identity;
    const char *source;
    const char *emitted;
} GlslName;

typedef enum GlslStorage_Enum {
    GLSL_STORAGE_NONE,
    GLSL_STORAGE_CONST,
    GLSL_STORAGE_ATTRIBUTE,
    GLSL_STORAGE_VARYING,
    GLSL_STORAGE_UNIFORM,
    GLSL_STORAGE_SAMPLER,
    GLSL_STORAGE_BUILTIN
} GlslStorage;

typedef enum GlslParameterQualifier_Enum {
    GLSL_PARAMETER_IN,
    GLSL_PARAMETER_OUT,
    GLSL_PARAMETER_INOUT
} GlslParameterQualifier;

typedef enum GlslExprKind_Enum {
    GLSL_EXPR_SYMBOL,
    GLSL_EXPR_INT,
    GLSL_EXPR_FLOAT,
    GLSL_EXPR_BOOL,
    GLSL_EXPR_UNARY,
    GLSL_EXPR_BINARY,
    GLSL_EXPR_CONDITIONAL,
    GLSL_EXPR_CALL,
    GLSL_EXPR_CONSTRUCT,
    GLSL_EXPR_MEMBER,
    GLSL_EXPR_INDEX,
    GLSL_EXPR_SWIZZLE
} GlslExprKind;

typedef enum GlslStmtKind_Enum {
    GLSL_STMT_EXPRESSION,
    GLSL_STMT_IF,
    GLSL_STMT_WHILE,
    GLSL_STMT_DO,
    GLSL_STMT_FOR,
    GLSL_STMT_BLOCK,
    GLSL_STMT_RETURN,
    GLSL_STMT_DISCARD,
    GLSL_STMT_BREAK,
    GLSL_STMT_CONTINUE
} GlslStmtKind;

typedef struct GlslExpr_Rec GlslExpr;
typedef struct GlslStmt_Rec GlslStmt;
typedef struct GlslDecl_Rec GlslDecl;
typedef struct GlslFunction_Rec GlslFunction;
typedef struct GlslBinding_Rec GlslBinding;

struct GlslModule_Rec {
    GlslStage stage;
    GlslAllocFn alloc;
    void *allocArg;
    GlslName *names;
    GlslDecl *structs;
    GlslDecl *globals;
    GlslFunction *functions;
    GlslFunction *entry;
    GlslBinding *bindings;
    int errors;
};

void GlslInitModule(GlslModule *module, GlslStage stage,
                    GlslAllocFn alloc, void *allocArg);
const char *GlslAllocateName(GlslModule *module, const char *source);
const char *GlslAllocateSymbolName(GlslModule *module, const void *identity,
                                   const char *source);
const char *GlslAllocateDistinctName(GlslModule *module, const char *source);
GlslType GlslNumericType(GlslBase base, int len);
GlslType GlslMatrixType(int size);
const char *GlslTypeName(const GlslType *type);
int GlslIsReservedName(const char *name);
~~~

Keep node internals in this header because both lowering and code generation
need them, but expose construction through GlslNewExpr, GlslNewStmt,
GlslNewDecl, GlslNewFunction, and append helpers. Each node stores a compact
source location made of file atom and line number.

- [ ] **Step 4: Implement allocation, reserved words, and type spelling**

In **glsl_ir.c**, implement a module allocator wrapper, string duplication,
identity-aware name reuse, distinct-name suffixing, and a sorted static GLSL
1.10 keyword table. GlslIsReservedName must reject the language keywords,
names beginning gl_, and these active storage/type words:

~~~c
static const char *reservedNames[] = {
    "asm", "attribute", "bool", "break", "bvec2", "bvec3", "bvec4",
    "cast", "class", "const", "continue", "default", "discard", "do",
    "double", "dvec2", "dvec3", "dvec4", "else", "enum", "extern",
    "external", "false", "fixed", "float", "for", "fvec2", "fvec3",
    "fvec4", "goto", "half", "hvec2", "hvec3", "hvec4", "if", "in",
    "inline", "inout", "input", "int", "interface", "invariant", "ivec2",
    "ivec3", "ivec4", "long", "mat2", "mat3", "mat4", "namespace",
    "noinline", "out", "output", "packed", "public", "return",
    "sampler1D", "sampler1DShadow", "sampler2D", "sampler2DRect",
    "sampler2DRectShadow", "sampler2DShadow", "sampler3D",
    "sampler3DRect", "samplerCube", "short", "sizeof", "static",
    "struct", "switch", "template", "this", "true", "typedef", "uniform",
    "union", "unsigned", "using", "varying", "vec2", "vec3", "vec4",
    "void", "volatile", "while"
};
~~~

GlslTypeName returns exactly void, float/vecN, int/ivecN, bool/bvecN,
mat2/mat3/mat4, sampler1D/2D/3D/samplerCube, or the stored structName.
GlslIsReservedName also rejects any identifier containing two consecutive
underscores and any identifier beginning gl_.

- [ ] **Step 5: Build and run the unit test**

Run:

~~~powershell
cmake --build build-glsl --config Debug --target glsl_ir_unit
ctest --test-dir build-glsl -C Debug -R '^glsl_ir_unit$' --output-on-failure
~~~

Expected: glsl_ir_unit passes.

- [ ] **Step 6: Commit the IR foundation**

Run:

~~~powershell
git add -- glsl_ir.h glsl_ir.c CMakeLists.txt tests/CMakeLists.txt tests/glsl_ir_test.c
git commit -m "Add structured GLSL source IR"
~~~

### Task 4: Bind stage semantics and canonical interfaces

**Files:**
- Modify: **glsl_ir.h**
- Modify: **glsl_ir.c**
- Modify: **glsl_hal.h**
- Modify: **glsl_hal.c**
- Modify: **glslv_hal.c**
- Modify: **glslf_hal.c**
- Create: **tests/glsl/semantics/vp_semantics.cg**
- Create: **tests/glsl/semantics/fp_semantics.cg**
- Create: **tests/glsl/diagnostics/unbound_input.cg**
- Modify: **tests/CMakeLists.txt**

- [ ] **Step 1: Add successful no-code semantic fixtures**

Create **tests/glsl/semantics/vp_semantics.cg**:

~~~c
struct AppData {
    float4 position : POSITION;
    float3 normal : NORMAL;
    float4 color : COLOR0;
    float2 texcoord : TEXCOORD0;
};

struct VertexOut {
    float4 position : POSITION;
    float4 color : COLOR0;
    float2 texcoord : TEXCOORD0;
};

VertexOut main(AppData input)
{
    VertexOut output;
    output.position = input.position;
    output.color = input.color;
    output.texcoord = input.texcoord;
    return output;
}
~~~

Create **tests/glsl/semantics/fp_semantics.cg**:

~~~c
struct FragmentOut {
    float4 color : COLOR0;
    float depth : DEPTH;
};

FragmentOut main(float4 color : COLOR0,
                 float2 texcoord : TEXCOORD0,
                 float4 windowPosition : WPOS)
{
    FragmentOut output;
    output.color = color + float4(texcoord, 0.0, 0.0);
    output.depth = windowPosition.z;
    return output;
}
~~~

Register glslv_semantics and glslf_semantics as direct cgc invocations with
-quiet, -nocode, the matching profile, and the fixture.

- [ ] **Step 2: Add the missing-semantic negative fixture**

Create **tests/glsl/diagnostics/unbound_input.cg**:

~~~c
struct VertexOut {
    float4 position : POSITION;
};

VertexOut main(float4 position)
{
    VertexOut output;
    output.position = position;
    return output;
}
~~~

Register unbound_input with WILL_FAIL TRUE for this milestone. It must exit
nonzero and include C5116 until the dedicated diagnostic runner is added.

- [ ] **Step 3: Run the semantic tests in the red state**

Run:

~~~powershell
cmake --build build-glsl --config Debug --target cgc
ctest --test-dir build-glsl -C Debug -R '^(glsl(v|f)_semantics|unbound_input)$' --output-on-failure
~~~

Expected: both success fixtures fail with unknown semantics; unbound_input
already fails.

- [ ] **Step 4: Add a direction-aware semantic descriptor**

Add these declarations to **glsl_hal.h** and replace the
SemanticsDescriptor fields in GlslProfileDesc with semanticMap and
numSemanticMap:

~~~c
typedef enum GlslInterface_Enum {
    GLSL_INTERFACE_ATTRIBUTE,
    GLSL_INTERFACE_VARYING,
    GLSL_INTERFACE_POSITION,
    GLSL_INTERFACE_POINT_SIZE,
    GLSL_INTERFACE_FRAG_COORD,
    GLSL_INTERFACE_FRONT_FACING,
    GLSL_INTERFACE_FRAG_COLOR,
    GLSL_INTERFACE_FRAG_DEPTH
} GlslInterface;

typedef struct GlslSemanticDesc_Rec {
    const char *root;
    const char *canonicalRoot;
    int firstIndex;
    int count;
    int properties;
    int size;
    GlslInterface interfaceKind;
} GlslSemanticDesc;

typedef struct GlslSemanticAlias_Rec {
    const char *alias;
    const char *canonical;
} GlslSemanticAlias;
~~~

Each stage descriptor also stores aliases and numAliases. The common binder
chooses a row whose SEM_IN or SEM_OUT property matches IsOutVal before it
canonicalizes the semantic.

Update GlslInitHAL to set hal->semantics to NULL and hal->numSemantics to zero;
the new direction-aware map is consumed only by the GLSL callbacks. Update the
two skeleton descriptor initializers to supply semanticMap, numSemanticMap,
aliases, and numAliases in the new field order.

- [ ] **Step 5: Define the exact vertex semantic map**

In **glslv_hal.c**, define these rows:

| Root | Direction | Count | Width | Interface |
| --- | --- | ---: | ---: | --- |
| ATTRIB | input | 16 | 4 | attribute |
| POSITION | input | 1 | 4 | attribute |
| NORMAL | input | 1 | 3 | attribute |
| COLOR | input | 2 | 4 | attribute |
| TEXCOORD | input | 8 | 4 | attribute |
| TANGENT | input | 1 | 3 | attribute |
| BINORMAL | input | 1 | 3 | attribute |
| BLENDWEIGHT | input | 1 | 4 | attribute |
| BLENDINDICES | input | 1 | 4 | attribute |
| POSITION | output|required | 1 | 4 | gl_Position |
| COLOR | output | 2 | 4 | varying |
| TEXCOORD | output | 8 | 4 | varying |
| FOG | output | 1 | 1 | varying |
| PSIZE | output | 1 | 1 | gl_PointSize |

Use aliases DIFFUSE to COLOR0, SPECULAR to COLOR1, FOGCOORD to FOG0, and HPOS
to POSITION0. Canonical names are ATTRIB0 through ATTRIB15, POSITION0,
NORMAL0, COLOR0/COLOR1, TEXCOORD0 through TEXCOORD7, TANGENT0, BINORMAL0,
BLENDWEIGHT0, BLENDINDICES0, FOG0, and PSIZE0.

- [ ] **Step 6: Define the exact fragment semantic map**

In **glslf_hal.c**, define:

| Root | Direction | Count | Width | Interface |
| --- | --- | ---: | ---: | --- |
| COLOR | input | 2 | 4 | varying |
| TEXCOORD | input | 8 | 4 | varying |
| FOG | input | 1 | 1 | varying |
| POSITION | input | 1 | 4 | gl_FragCoord |
| WPOS | input | 1 | 4 | gl_FragCoord |
| FACE | input | 1 | 1 | gl_FrontFacing |
| COLOR | output | 1 | 4 | gl_FragColor |
| DEPTH | output | 1 | 1 | gl_FragDepth |

Use DIFFUSE, SPECULAR, and FOGCOORD aliases as above. FACE is the only Boolean
interface value; all other interface values must be float scalars or packed
float vectors.

- [ ] **Step 7: Implement shared connector and binding callbacks**

In **glsl_hal.c**, install and implement RegisterNames, GetConnectorID,
GetConnectorAtom, GetConnectorUses, GetConnectorRegister,
BindVaryingSemantic, and BindVaryingUnbound.

In each stage file, create one ConnectorRegisters row for every canonical
semantic/direction row in Steps 5 and 6, using the semantic width and
REG_INPUT or REG_OUTPUT. Mark vertex POSITION output with REG_WRITE_REQUIRED.
Create the two ConnectorDescriptor rows for that stage and store all arrays and
counts in GlslProfileDesc. RegisterNames atomizes every connector name,
canonical register name, semantic root, and alias.

BindVaryingSemantic performs this exact sequence:

1. Convert aliases to canonical spelling.
2. Parse a numeric suffix with HasNumericSuffix; missing suffix means zero.
3. Select a descriptor row matching root and requested direction.
4. Reject an index outside firstIndex through firstIndex + count - 1.
5. Accept only float scalar/vector types, except Boolean scalar FACE.
6. Set BK_CONNECTOR, canonical rname, descriptor-relative regno,
   BIND_IS_BOUND | BIND_VARYING, and input/output/required properties.
7. Mark the connector symbol readable and writable using existing symbol bits.

BindVaryingUnbound returns zero so entry inputs without semantics fail
deterministically. GetConnectorRegister resolves both canonical names and
descriptor indices and calls SetSymbolConnectorBindingHAL on success.

- [ ] **Step 8: Run semantic and generic tests**

Run:

~~~powershell
cmake --build build-glsl --config Debug --target cgc
ctest --test-dir build-glsl -C Debug -R '^(glsl(v|f)_(registration|semantics)|unbound_input|generic_)' --output-on-failure
~~~

Expected: both stage semantic tests pass, unbound_input fails as expected, and
all generic regressions pass.

- [ ] **Step 9: Commit stage binding**

Run:

~~~powershell
git add -- glsl_hal.h glsl_hal.c glslv_hal.c glslf_hal.c tests/CMakeLists.txt tests/glsl/semantics tests/glsl/diagnostics/unbound_input.cg
git commit -m "Bind GLSL stage semantics"
~~~

### Task 5: Emit the first complete vertex shader

**Files:**
- Create: **glsl_lower.c**
- Create: **glsl_codegen.c**
- Create: **tests/check_glsl.cmake**
- Create: **tests/glsl/profile/vp_passthrough.cg**
- Create: **tests/glsl/profile/vp_passthrough.expected**
- Modify: **glsl_ir.h**
- Modify: **glsl_ir.c**
- Modify: **glsl_hal.h**
- Modify: **glsl_hal.c**
- Modify: **CMakeLists.txt**
- Modify: **Makefile**
- Modify: **tests/CMakeLists.txt**

- [ ] **Step 1: Add the exact successful-fixture runner**

Create **tests/check_glsl.cmake**:

~~~cmake
foreach(required CGC PROFILE SOURCE EXPECTED ACTUAL)
    if(NOT DEFINED ${required})
        message(FATAL_ERROR "${required} must be defined")
    endif()
endforeach()

file(REMOVE "${ACTUAL}")
execute_process(
    COMMAND "${CGC}" -quiet -profile "${PROFILE}" -o "${ACTUAL}" "${SOURCE}"
    RESULT_VARIABLE result
    OUTPUT_VARIABLE stdout
    ERROR_VARIABLE stderr
)
if(NOT result EQUAL 0)
    message(FATAL_ERROR
        "${PROFILE} compile failed (${result}):\n${stdout}${stderr}")
endif()
if(NOT EXISTS "${ACTUAL}")
    message(FATAL_ERROR "cgc did not create ${ACTUAL}")
endif()

file(READ "${ACTUAL}" actual_text)
string(REPLACE "\r\n" "\n" actual_text "${actual_text}")
string(REPLACE "\r" "\n" actual_text "${actual_text}")
string(REGEX REPLACE "(^|\n)// cgc version [^\n]*\n" "\\1"
    actual_text "${actual_text}")
string(REGEX REPLACE "(^|\n)// command line args:[^\n]*\n" "\\1"
    actual_text "${actual_text}")
string(REGEX REPLACE "(^|\n)// End of program\n?$" "\\1"
    actual_text "${actual_text}")

if(DEFINED UPDATE_EXPECTED AND UPDATE_EXPECTED)
    file(WRITE "${EXPECTED}" "${actual_text}")
    return()
endif()

file(READ "${EXPECTED}" expected_text)
string(REPLACE "\r\n" "\n" expected_text "${expected_text}")
string(REPLACE "\r" "\n" expected_text "${expected_text}")
if(NOT actual_text STREQUAL expected_text)
    file(WRITE "${ACTUAL}.normalized" "${actual_text}")
    message(FATAL_ERROR
        "GLSL differs from ${EXPECTED}; actual: ${ACTUAL}.normalized")
endif()
~~~

Add an add_glsl_fixture function to **tests/CMakeLists.txt** that passes a
unique ACTUAL path under the binary tests directory.

- [ ] **Step 2: Add the pass-through source and expected GLSL**

Create **tests/glsl/profile/vp_passthrough.cg**:

~~~c
struct VertexOut {
    float4 position : POSITION;
};

VertexOut main(float4 position : ATTRIB0)
{
    VertexOut output;
    output.position = position;
    return output;
}
~~~

Create **tests/glsl/profile/vp_passthrough.expected**:

~~~glsl
#version 110
// cgc-bind attribute cg_ATTRIB0 ATTRIB0
// cgc-bind builtin gl_Position POSITION0

struct VertexOut
{
    vec4 position;
};

attribute vec4 cg_ATTRIB0;

void main()
{
    vec4 position;
    VertexOut output;
    position = cg_ATTRIB0;
    output.position = position;
    gl_Position = output.position;
}
~~~

Register glslv_passthrough with add_glsl_fixture.

- [ ] **Step 3: Run the pass-through test in the red state**

Run:

~~~powershell
cmake --build build-glsl --config Debug --target cgc
ctest --test-dir build-glsl -C Debug -R '^glslv_passthrough$' --output-on-failure
~~~

Expected: failure because GenerateCode_glsl still emits no declarations or
main function.

- [ ] **Step 4: Complete the IR nodes needed by pass-through**

Define GlslDecl, GlslBinding, GlslExpr, GlslStmt, and GlslFunction in
**glsl_ir.h**. Each list node has next; each expression has kind, type, compact
source location, and a union for symbol/literal/unary/binary/call/member/index
payloads. Each statement has kind, location, next, and a union for expression,
branch, loop, block, and return payloads. A declaration stores storage,
type, emitted name, optional initializer, source identity, struct members, and
parameter qualifier when it belongs to a helper signature.

Add and implement:

~~~c
GlslDecl *GlslNewDecl(GlslModule *module, GlslStorage storage,
                      GlslType type, const char *name);
GlslExpr *GlslNewExpr(GlslModule *module, GlslExprKind kind, GlslType type);
GlslStmt *GlslNewStmt(GlslModule *module, GlslStmtKind kind);
GlslFunction *GlslNewFunction(GlslModule *module, GlslType result,
                              const char *name);
GlslBinding *GlslNewBinding(GlslModule *module, GlslStorage storage,
                            const char *name, const char *semantic);
void GlslAppendDecl(GlslDecl **list, GlslDecl *decl);
void GlslAppendStmt(GlslStmt **list, GlslStmt *stmt);
void GlslAppendFunction(GlslFunction **list, GlslFunction *function);
~~~

- [ ] **Step 5: Add lowering and code-generation integration APIs**

Add to **glsl_hal.h**:

~~~c
int GlslLowerProgram(GlslModule *module, const GlslProfileDesc *profile,
                     SourceLoc *loc, Scope *scope, Symbol *program);
int GlslWriteModule(FILE *out, const GlslModule *module);
const char *GlslCanonicalInterfaceName(const GlslProfileDesc *profile,
                                       int semantic, int isOutput);
~~~

Replace GenerateCode_glsl with:

~~~c
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
~~~

- [ ] **Step 6: Lower the pass-through AST**

In **glsl_lower.c**, implement:

- source-order collection of the entry struct, formal, and local symbols;
- float scalar/vector and struct type conversion;
- identity-based name allocation;
- $vin connector member to named attribute conversion;
- $vout POSITION member to gl_Position conversion;
- VARIABLE_OP, MEMBER_SELECTOR_OP, ASSIGN_OP, ASSIGN_V_OP, and
  ASSIGN_GEN_OP lowering; and
- entry conversion to parameterless void main while declaring non-uniform
  formals as locals.

Sort declarations by SourceLoc line and then source spelling when the legacy
symbol tree does not retain declaration order. Do not emit $vin or $vout
struct declarations.

- [ ] **Step 7: Emit deterministic GLSL for the pass-through subset**

In **glsl_codegen.c**, emit version-independent body content only:

1. cgc-bind comments;
2. struct definitions;
3. globals;
4. helper functions; and
5. main.

Use four spaces per level, braces on their own lines, one blank line between
top-level units, and a semicolon after declarations and expression statements.
The existing PrintCodeHeader callback remains the only writer of #version 110.
Emit cgc-bind records for built-in destinations and sources as well as user
attributes, varyings, uniforms, and samplers. Use storage word builtin, the
GLSL built-in name, and the canonical Cg semantic.

- [ ] **Step 8: Add sources, run focused and generic tests**

Add glsl_ir.c, glsl_lower.c, and glsl_codegen.c to the cgc target and Makefile
OBJS. glsl_ir.c remains separately compiled into glsl_ir_unit.

Run:

~~~powershell
cmake --build build-glsl --config Debug --target cgc glsl_ir_unit
ctest --test-dir build-glsl -C Debug -R '^(glslv_passthrough|glsl_ir_unit|generic_)' --output-on-failure
~~~

Expected: pass-through, IR unit, and four generic tests pass.

- [ ] **Step 9: Commit first source emission**

Run:

~~~powershell
git add -- glsl_ir.h glsl_ir.c glsl_hal.h glsl_hal.c glsl_lower.c glsl_codegen.c CMakeLists.txt Makefile tests/CMakeLists.txt tests/check_glsl.cmake tests/glsl/profile/vp_passthrough.*
git commit -m "Emit basic GLSL vertex shaders"
~~~

### Task 6: Lower structured expressions, statements, and helpers

**Files:**
- Create: **tests/glsl/expressions/vp_control.cg**
- Create: **tests/glsl/expressions/vp_control.expected**
- Create: **tests/glsl/expressions/vp_jumps.cg**
- Create: **tests/glsl/expressions/vp_jumps.expected**
- Create: **tests/glsl/expressions/vp_qualifiers.cg**
- Create: **tests/glsl/expressions/vp_qualifiers.expected**
- Modify: **glsl_ir.h**
- Modify: **glsl_lower.c**
- Modify: **glsl_codegen.c**
- Modify: **support.h:69-72, 421-489, 517-525**
- Modify: **support.c:482-669**
- Modify: **support_iter.c**
- Modify: **compile.c**
- Modify: **inline.c**
- Modify: **printutils.c**
- Modify: **parser.y:811-840**
- Regenerate: **parser.c**
- Regenerate: **parser.h**
- Modify: **tests/CMakeLists.txt**

- [ ] **Step 1: Add a readable helper/control-flow fixture**

Create **tests/glsl/expressions/vp_control.cg**:

~~~c
struct VertexOut {
    float4 position : POSITION;
    float4 color : COLOR0;
};

float4 adjust(float4 color, float scale)
{
    if (scale < 0.0)
        scale = 0.0;
    return color * scale;
}

VertexOut main(float4 position : ATTRIB0, float4 color : ATTRIB1)
{
    VertexOut output;
    float scale;
    int index;
    scale = 0.0;
    for (index = 0; index < 2; index = index + 1) {
        scale = scale + 0.5;
    }
    output.position = position;
    output.color = adjust(color, scale);
    return output;
}
~~~

Create **tests/glsl/expressions/vp_control.expected**:

~~~glsl
#version 110
// cgc-bind attribute cg_ATTRIB0 ATTRIB0
// cgc-bind attribute cg_ATTRIB1 ATTRIB1
// cgc-bind varying cg_COLOR0 COLOR0
// cgc-bind builtin gl_Position POSITION0

struct VertexOut
{
    vec4 position;
    vec4 color;
};

attribute vec4 cg_ATTRIB0;
attribute vec4 cg_ATTRIB1;
varying vec4 cg_COLOR0;

vec4 adjust(vec4 color, float scale)
{
    if (scale < 0.0)
    {
        scale = 0.0;
    }
    return color * scale;
}

void main()
{
    vec4 position;
    vec4 color;
    VertexOut output;
    float scale;
    int index;
    position = cg_ATTRIB0;
    color = cg_ATTRIB1;
    scale = 0.0;
    for (index = 0; index < 2; index = index + 1)
    {
        scale = scale + 0.5;
    }
    output.position = position;
    output.color = adjust(color, scale);
    gl_Position = output.position;
    cg_COLOR0 = output.color;
}
~~~

Register glslv_control and run it.

Expected red state: the lowerer reports or fails on function calls, return,
if, or for nodes.

- [ ] **Step 2: Implement reachable helper collection**

Walk resolved FUN_CALL_OP symbols from main, mark each non-builtin helper once,
and visit its body before emission. Reject recursion if the existing front-end
check did not already do so. Allocate overload names from the mapped parameter
signature; only collapsed signatures receive a stable suffix such as
adjust_float_1.

Emit prototypes only when a helper references one that appears later in stable
source order. Never emit standard-library functions as user helpers.

Create **tests/glsl/expressions/vp_qualifiers.cg**:

~~~c
struct VertexOut { float4 position : POSITION; };
void scalePosition(inout float4 value, float factor)
{
    value = value * factor;
}
void copyPosition(float4 value, out float4 result)
{
    result = value;
}
VertexOut main(float4 position : ATTRIB0)
{
    VertexOut output;
    float4 copied;
    scalePosition(position, 2.0);
    copyPosition(position, copied);
    output.position = copied;
    return output;
}
~~~

Register glslv_qualifiers. Map helper parameter TYPE_QUALIFIER_OUT to out and
the combined in/out bits to inout; omit the optional in spelling for ordinary
input parameters. Its expectation must contain the two readable helper
signatures, both calls, and no parameter qualifiers on GLSL main.

- [ ] **Step 3: Implement expression lowering by opcode family**

Add exhaustive switches for the AST opcodes accepted by this milestone:

- constants: ICONST, ICONST_V, BCONST, BCONST_V, FCONST, FCONST_V;
- constructors/casts: VECTOR_V, CAST_CS, CAST_CV;
- unary: NEG, NEG_V, POS, POS_V, BNOT, BNOT_V;
- selection: MEMBER_SELECTOR, ARRAY_INDEX, SWIZZLE_Z;
- calls and argument lists: FUN_CALL, FUN_BUILTIN, FUN_ARG, EXPR_LIST;
- arithmetic: MUL, DIV, ADD, SUB and scalar/vector variants;
- comparisons: LT, GT, LE, GE, EQ, NE and scalar/vector variants;
- logical: BAND and BOR scalar variants;
- assignments: ASSIGN, ASSIGN_V, ASSIGN_GEN, ASSIGN_MASKED_KV; and
- conditionals: COND, COND_V, COND_SV, COND_GEN.

Map vector comparisons to lessThan, greaterThan, lessThanEqual, greaterThanEqual,
equal, and notEqual. Lower a vector conditional to a constructor containing
one scalar conditional per component. Reject shifts, bitwise operators, and
remainder operators because strict GLSL 1.10 has no exact operator contract for
this backend.

- [ ] **Step 4: Add the two tokenized but missing jump statements**

The scanner already returns BREAK_SY and CONTINUE_SY, but parser.y does not
accept them and stmtkind has no nodes. Add BREAK_STMT and CONTINUE_STMT before
LAST_STMTKIND in **support.h**, use common_stmt storage, and declare:

~~~c
common_stmt *NewSimpleStmt(SourceLoc *loc, stmtkind kind);
~~~

Implement NewSimpleStmt in **support.c** by allocating common_stmt, setting
kind, next to NULL, and copying loc. Add this grammar production:

~~~yacc
jump_statement:           BREAK_SY ';'
                              { $$ = (stmt *) NewSimpleStmt(Cg->tokenLoc, BREAK_STMT); }
                        | CONTINUE_SY ';'
                              { $$ = (stmt *) NewSimpleStmt(Cg->tokenLoc, CONTINUE_STMT); }
;
~~~

Add jump_statement to balanced_statement. In every statement-kind switch in
support_iter.c, compile.c, inline.c, and printutils.c, handle BREAK_STMT and
CONTINUE_STMT as leaf statements with no expressions or children. Printutils
prints break; and continue; at the current indentation.

Create **tests/glsl/expressions/vp_jumps.cg**:

~~~c
struct VertexOut { float4 position : POSITION; };
VertexOut main(float4 position : ATTRIB0)
{
    VertexOut output;
    int index;
    for (index = 0; index < 4; index = index + 1) {
        if (index == 1)
            continue;
        if (index == 3)
            break;
        position.x = position.x + 1.0;
    }
    output.position = position;
    return output;
}
~~~

Its exact expectation retains the for loop and emits continue; and break; in
their respective if blocks. Register glslv_jumps.

Regenerate the parser and record the generator version:

~~~powershell
$bison = Get-Command bison -ErrorAction SilentlyContinue
if (-not $bison) {
    throw 'Bison is required. Obtain user approval, then run: scoop install winflexbison'
}
bison --version | Select-Object -First 1
cmake --build build-glsl --config Debug --target regenerate_parser
~~~

Expected: parser.c and parser.h are synchronized with parser.y. Record the
reported Bison version in the implementation handoff and final commit notes.
The current machine does not have Bison installed; Scoop's local
winflexbison manifest provides the bison shim at version 2.5.25. Installing it
changes the user's tool environment, so execution must obtain approval before
running scoop install winflexbison.

- [ ] **Step 5: Implement statement lowering**

Handle EXPR_STMT, IF_STMT, WHILE_STMT, DO_STMT, FOR_STMT, BLOCK_STMT, and
RETURN_STMT. COMMENT_STMT is intentionally dropped because source comments are
not preserved. Keep early returns in helpers; entry returns have already been
replaced by connector assignments.

Map BREAK_STMT and CONTINUE_STMT directly to GLSL_STMT_BREAK and
GLSL_STMT_CONTINUE.

- [ ] **Step 6: Add precedence-aware emission**

Use these precedence levels from low to high:

| Level | Forms |
| ---: | --- |
| 1 | assignment |
| 2 | conditional |
| 3 | logical OR |
| 4 | logical AND |
| 5 | equality |
| 6 | relational |
| 7 | addition/subtraction |
| 8 | multiplication/division |
| 9 | unary |
| 10 | call/member/index/swizzle |
| 11 | primary |

Parenthesize a child when its precedence is lower than its parent, and
parenthesize a right child of non-associative subtraction/division when equal.
Emit float constants with snprintf("%.9g"), forcing a decimal suffix when the
result contains neither a decimal point nor exponent.

- [ ] **Step 7: Generate, inspect, and freeze the jump expectation**

The hand-written vp_control expectation is the red-state contract. After jump
and qualifier implementation, use UPDATE_EXPECTED=ON for vp_jumps.expected and
vp_qualifiers.expected, inspect both, and then run all three exact tests. Run:

~~~powershell
$cgc = (Resolve-Path '.\build-glsl\Debug\cgc.exe').Path
$expectedDir = (Resolve-Path '.\tests\glsl\expressions').Path
$actualDir = (Resolve-Path '.\build-glsl').Path
foreach ($case in 'vp_jumps','vp_qualifiers') {
    cmake -DUPDATE_EXPECTED=ON -DCGC="$cgc" -DPROFILE=glslv -DSOURCE="$expectedDir\$case.cg" -DEXPECTED="$expectedDir\$case.expected" -DACTUAL="$actualDir\$case.glsl" -P tests/check_glsl.cmake
    Get-Content "$expectedDir\$case.expected"
}
~~~

Inspect that the output matches the fixture structurally and contains no cgc
pseudo connectors. The checked-in expected file is the complete reviewed
output.

- [ ] **Step 8: Run and commit structured lowering**

Run:

~~~powershell
cmake --build build-glsl --config Debug --target cgc
ctest --test-dir build-glsl -C Debug -R '^(glslv_(passthrough|control|jumps|qualifiers)|glsl_ir_unit|generic_)' --output-on-failure
git add -- glsl_ir.h glsl_lower.c glsl_codegen.c support.h support.c support_iter.c compile.c inline.c printutils.c parser.y parser.c parser.h tests/CMakeLists.txt tests/glsl/expressions
git commit -m "Lower structured GLSL expressions"
~~~

### Task 7: Add uniforms, matrices, numeric intrinsics, and bundled shaders

**Files:**
- Modify: **glsl_hal.h**
- Modify: **glsl_hal.c**
- Modify: **glsl_lower.c**
- Modify: **glsl_codegen.c**
- Modify: **stdlib.cg**
- Regenerate: **stdlib.c**
- Create: **tests/glsl/uniforms/vp_defaults.cg**
- Create: **tests/glsl/uniforms/vp_defaults.expected**
- Create: **tests/glsl/vertex/position.expected**
- Create: **tests/glsl/vertex/reflection.expected**
- Create: **tests/glsl/vertex/vertexlight.expected**
- Create: **tests/glsl/vertex/vertexlight4.expected**
- Modify: **tests/CMakeLists.txt**

- [ ] **Step 1: Add a failing uniform/default fixture**

Create **tests/glsl/uniforms/vp_defaults.cg**:

~~~c
struct VertexOut {
    float4 position : POSITION;
};

VertexOut main(float4 position : ATTRIB0,
               uniform float4x4 transform,
               uniform float scale = 1.0)
{
    VertexOut output;
    output.position = mul(transform, position) * scale;
    return output;
}
~~~

Register glslv_defaults with an expected file requiring:

~~~glsl
#version 110
// cgc-bind attribute cg_ATTRIB0 ATTRIB0
// cgc-bind uniform transform
// cgc-bind uniform scale
// cgc-default scale 1.0
// cgc-bind builtin gl_Position POSITION0

struct VertexOut
{
    vec4 position;
};

attribute vec4 cg_ATTRIB0;
uniform mat4 transform;
uniform float scale;

void main()
{
    vec4 position;
    VertexOut output;
    position = cg_ATTRIB0;
    output.position = transform * position * scale;
    gl_Position = output.position;
}
~~~

Run the test and expect failure before uniform and mul lowering exist.

- [ ] **Step 2: Bind and collect numeric uniforms**

Install BindUniformUnbound_glsl in **glsl_hal.c**. For non-samplers, set
BK_SEMANTIC, BIND_IS_BOUND | BIND_INPUT | BIND_UNIFORM, source base/size, and
the sanitized source name atom. Do not assign register numbers.

In **glsl_lower.c**, collect Cg->theHAL->uniformParam and uniformGlobal plus
direct uniform symbols referenced by reachable helpers. Deduplicate by Symbol
identity. Emit one GlslDecl per uniform and one cgc-bind record. Walk
defaultBindings and emit cgc-default records using the final uniform name and
locale-independent values.

- [ ] **Step 3: Recognize compiler matrix typedefs**

Use IsMatrix(type, &rows, &cols) before ordinary array handling. Accept only
2x2, 3x3, and 4x4 and map them to mat2, mat3, and mat4. Preserve ordinary
fixed-size arrays as arrays.

Lower SWIZMAT_Z_OP by decoding SUBOP_GET_MASK16. A Cg _mRC selector becomes
matrix[C][R]. For example, _m00_m01_m02 becomes
vec3(matrix[0][0], matrix[1][0], matrix[2][0]). Reject non-square matrices
later with C6209; this task may return failure through the module error count.

- [ ] **Step 4: Define stable intrinsic IDs and exact profile declarations**

Add GLSL_BUILTIN_GROUP 3 to **glsl_hal.h** and this compiler-independent enum
to **glsl_ir.h**:

~~~c
typedef enum GlslBuiltin_Enum {
    GLSL_BUILTIN_NONE,
    GLSL_BUILTIN_MUL,
    GLSL_BUILTIN_DOT,
    GLSL_BUILTIN_CROSS,
    GLSL_BUILTIN_NORMALIZE,
    GLSL_BUILTIN_REFLECT,
    GLSL_BUILTIN_REFRACT,
    GLSL_BUILTIN_LENGTH,
    GLSL_BUILTIN_DISTANCE,
    GLSL_BUILTIN_MIN,
    GLSL_BUILTIN_MAX,
    GLSL_BUILTIN_CLAMP,
    GLSL_BUILTIN_ABS,
    GLSL_BUILTIN_SIGN,
    GLSL_BUILTIN_FLOOR,
    GLSL_BUILTIN_CEIL,
    GLSL_BUILTIN_SQRT,
    GLSL_BUILTIN_EXP,
    GLSL_BUILTIN_EXP2,
    GLSL_BUILTIN_LOG,
    GLSL_BUILTIN_LOG2,
    GLSL_BUILTIN_SIN,
    GLSL_BUILTIN_COS,
    GLSL_BUILTIN_TAN,
    GLSL_BUILTIN_ASIN,
    GLSL_BUILTIN_ACOS,
    GLSL_BUILTIN_ATAN,
    GLSL_BUILTIN_RSQRT,
    GLSL_BUILTIN_LERP,
    GLSL_BUILTIN_FRAC,
    GLSL_BUILTIN_SATURATE,
    GLSL_BUILTIN_TEX1D,
    GLSL_BUILTIN_TEX2D,
    GLSL_BUILTIN_TEX3D,
    GLSL_BUILTIN_TEXCUBE
} GlslBuiltin;
~~~

Also declare:

~~~c
GlslBuiltin GlslLookupBuiltin(const char *name, const GlslType *result,
                              const GlslType *params, int paramCount);
const char *GlslBuiltinSpelling(GlslBuiltin builtin);
int GlslTypeComponentCount(const GlslType *type);
~~~

Implement the exact name/signature table in **glsl_ir.c** so unit tests can
exercise intrinsic selection and recursive scalar-component counting without
linking the compiler front end. The HAL callback converts resolved Cg types to
GlslType and delegates to GlslLookupBuiltin.

At the top of the function portion of **stdlib.cg**, derive PROFILE_GLSL from
PROFILE_GLSLV or PROFILE_GLSLF. Under PROFILE_GLSL, add this exact numeric
prototype block:

~~~c
__internal float2 mul(float2x2 matrix, float2 value);
__internal float3 mul(float3x3 matrix, float3 value);
__internal float4 mul(float4x4 matrix, float4 value);

__internal float dot(float left, float right);
__internal float dot(float2 left, float2 right);
__internal float dot(float3 left, float3 right);
__internal float dot(float4 left, float4 right);
__internal float3 cross(float3 left, float3 right);

__internal float normalize(float value);
__internal float2 normalize(float2 value);
__internal float3 normalize(float3 value);
__internal float4 normalize(float4 value);
__internal float length(float value);
__internal float length(float2 value);
__internal float length(float3 value);
__internal float length(float4 value);
__internal float distance(float left, float right);
__internal float distance(float2 left, float2 right);
__internal float distance(float3 left, float3 right);
__internal float distance(float4 left, float4 right);
__internal float reflect(float incident, float normal);
__internal float2 reflect(float2 incident, float2 normal);
__internal float3 reflect(float3 incident, float3 normal);
__internal float4 reflect(float4 incident, float4 normal);
__internal float refract(float incident, float normal, float eta);
__internal float2 refract(float2 incident, float2 normal, float eta);
__internal float3 refract(float3 incident, float3 normal, float eta);
__internal float4 refract(float4 incident, float4 normal, float eta);

__internal float min(float left, float right);
__internal float2 min(float2 left, float2 right);
__internal float3 min(float3 left, float3 right);
__internal float4 min(float4 left, float4 right);
__internal float max(float left, float right);
__internal float2 max(float2 left, float2 right);
__internal float3 max(float3 left, float3 right);
__internal float4 max(float4 left, float4 right);
__internal float clamp(float value, float low, float high);
__internal float2 clamp(float2 value, float2 low, float2 high);
__internal float3 clamp(float3 value, float3 low, float3 high);
__internal float4 clamp(float4 value, float4 low, float4 high);
__internal float2 clamp(float2 value, float low, float high);
__internal float3 clamp(float3 value, float low, float high);
__internal float4 clamp(float4 value, float low, float high);
__internal float lerp(float left, float right, float amount);
__internal float2 lerp(float2 left, float2 right, float2 amount);
__internal float3 lerp(float3 left, float3 right, float3 amount);
__internal float4 lerp(float4 left, float4 right, float4 amount);
__internal float2 lerp(float2 left, float2 right, float amount);
__internal float3 lerp(float3 left, float3 right, float amount);
__internal float4 lerp(float4 left, float4 right, float amount);

__internal float abs(float value);
__internal float2 abs(float2 value);
__internal float3 abs(float3 value);
__internal float4 abs(float4 value);
__internal float sign(float value);
__internal float2 sign(float2 value);
__internal float3 sign(float3 value);
__internal float4 sign(float4 value);
__internal float floor(float value);
__internal float2 floor(float2 value);
__internal float3 floor(float3 value);
__internal float4 floor(float4 value);
__internal float ceil(float value);
__internal float2 ceil(float2 value);
__internal float3 ceil(float3 value);
__internal float4 ceil(float4 value);
__internal float sqrt(float value);
__internal float2 sqrt(float2 value);
__internal float3 sqrt(float3 value);
__internal float4 sqrt(float4 value);
__internal float rsqrt(float value);
__internal float2 rsqrt(float2 value);
__internal float3 rsqrt(float3 value);
__internal float4 rsqrt(float4 value);
__internal float frac(float value);
__internal float2 frac(float2 value);
__internal float3 frac(float3 value);
__internal float4 frac(float4 value);
__internal float saturate(float value);
__internal float2 saturate(float2 value);
__internal float3 saturate(float3 value);
__internal float4 saturate(float4 value);

__internal float exp(float value);
__internal float2 exp(float2 value);
__internal float3 exp(float3 value);
__internal float4 exp(float4 value);
__internal float exp2(float value);
__internal float2 exp2(float2 value);
__internal float3 exp2(float3 value);
__internal float4 exp2(float4 value);
__internal float log(float value);
__internal float2 log(float2 value);
__internal float3 log(float3 value);
__internal float4 log(float4 value);
__internal float log2(float value);
__internal float2 log2(float2 value);
__internal float3 log2(float3 value);
__internal float4 log2(float4 value);
__internal float sin(float value);
__internal float2 sin(float2 value);
__internal float3 sin(float3 value);
__internal float4 sin(float4 value);
__internal float cos(float value);
__internal float2 cos(float2 value);
__internal float3 cos(float3 value);
__internal float4 cos(float4 value);
__internal float tan(float value);
__internal float2 tan(float2 value);
__internal float3 tan(float3 value);
__internal float4 tan(float4 value);
__internal float asin(float value);
__internal float2 asin(float2 value);
__internal float3 asin(float3 value);
__internal float4 asin(float4 value);
__internal float acos(float value);
__internal float2 acos(float2 value);
__internal float3 acos(float3 value);
__internal float4 acos(float4 value);
__internal float atan(float value);
__internal float2 atan(float2 value);
__internal float3 atan(float3 value);
__internal float4 atan(float4 value);
~~~

Under #else retain the existing dot, mul, rsqrt, normalize, and max bodies
unchanged. Close the guard after the current standard-library function bodies.

- [ ] **Step 5: Recognize and lower numeric built-ins**

CheckInternalFunction_glsl matches both name and resolved parameter/result
shape, sets group to GLSL_BUILTIN_GROUP, and returns the GlslBuiltin value.
Return zero for a declaration absent from the exact table.

Lower same-name GLSL built-ins directly. Apply these rewrites:

- the front end's square-matrix/vector mul overloads to *;
- scalar dot(a, b) to a * b;
- rsqrt to inversesqrt;
- lerp to mix;
- frac to fract; and
- saturate(x) to clamp(x, shape-correct zero, shape-correct one).

Do not rewrite a user helper with the same spelling; require
SYMB_IS_BUILTIN and the assigned group/index.

- [ ] **Step 6: Regenerate and verify the standard library**

Run:

~~~powershell
cmake --build build-glsl --config Debug --target tokenize
cmake --build build-glsl --config Debug --target regenerate_stdlib
git diff --check -- stdlib.cg stdlib.c
~~~

Expected: stdlib.c changes only because of the intentional stdlib.cg token
stream update.

- [ ] **Step 7: Register all four bundled vertex shaders**

Use add_glsl_fixture to register:

- glslv_position from **position.cg**;
- glslv_reflection from **reflection.cg**;
- glslv_vertexlight from **vertexlight.cg**; and
- glslv_vertexlight4 from **vertexlight4.cg**.

Run them before completing lowering. Expected red state: at least matrix,
uniform, intrinsic, or array lowering fails.

- [ ] **Step 8: Complete array, matrix, uniform, and intrinsic emission**

Emit arrays with the size after the declarator, emit matN constructors with
GLSL column-major component order, and use explicit component constructors for
Cg row selectors. Count matrix uniform components as N * N and array elements
recursively. Emit cgc-default metadata before declarations.

- [ ] **Step 9: Generate and review bundled expectations**

For each bundled shader, invoke **tests/check_glsl.cmake** with
UPDATE_EXPECTED=ON, the glslv profile, its root source path, its
**tests/glsl/vertex/<name>.expected** path, and a unique build-glsl actual path.
Review all four files for:

- #version 110 first;
- only named attributes;
- gl_Position assignment;
- semantic-named varyings;
- native mat3/mat4 uniforms;
- readable helper diffuse in vertexlight4;
- no $vin, $vout, Cg type spellings, or standard-library helper definitions.

- [ ] **Step 10: Run vertex and generic coverage**

Run:

~~~powershell
cmake --build build-glsl --config Debug --target cgc glsl_ir_unit
ctest --test-dir build-glsl -C Debug -R '^(glslv_|glsl_ir_unit|generic_)' --output-on-failure
~~~

Expected: all vertex, unit, and generic tests pass.

- [ ] **Step 11: Commit practical vertex translation**

Run:

~~~powershell
git add -- glsl_ir.h glsl_ir.c glsl_hal.h glsl_hal.c glsl_lower.c glsl_codegen.c stdlib.cg stdlib.c tests/CMakeLists.txt tests/glsl/uniforms tests/glsl/vertex
git commit -m "Translate practical Cg vertex shaders"
~~~

### Task 8: Emit fragment arithmetic, built-ins, control flow, and discard

**Files:**
- Create: **tests/glsl/profile/fp_passthrough.cg**
- Create: **tests/glsl/profile/fp_passthrough.expected**
- Create: **tests/glsl/expressions/fp_control.cg**
- Create: **tests/glsl/expressions/fp_control.expected**
- Create: **tests/glsl/semantics/fp_builtins.cg**
- Create: **tests/glsl/semantics/fp_builtins.expected**
- Modify: **glsl_lower.c**
- Modify: **glsl_codegen.c**
- Modify: **tests/CMakeLists.txt**

- [ ] **Step 1: Add a basic fragment expectation**

Create **tests/glsl/profile/fp_passthrough.cg**:

~~~c
struct FragmentOut {
    float4 color : COLOR0;
};

FragmentOut main(float2 texcoord : TEXCOORD0,
                 uniform float4 tint)
{
    FragmentOut output;
    output.color = float4(texcoord, 0.0, 1.0) * tint;
    return output;
}
~~~

Create **tests/glsl/profile/fp_passthrough.expected**:

~~~glsl
#version 110
// cgc-bind varying cg_TEXCOORD0 TEXCOORD0
// cgc-bind uniform tint
// cgc-bind builtin gl_FragColor COLOR0

struct FragmentOut
{
    vec4 color;
};

varying vec2 cg_TEXCOORD0;
uniform vec4 tint;

void main()
{
    vec2 texcoord;
    FragmentOut output;
    texcoord = cg_TEXCOORD0;
    output.color = vec4(texcoord, 0.0, 1.0) * tint;
    gl_FragColor = output.color;
}
~~~

Register glslf_passthrough and run it. Expected red state: fragment connector
members are not yet translated by the lowerer.

- [ ] **Step 2: Lower fragment input and output built-ins**

Map canonical fragment interfaces as follows:

~~~c
static const char *FragmentBuiltinName(GlslInterface kind)
{
    switch (kind) {
    case GLSL_INTERFACE_FRAG_COORD:
        return "gl_FragCoord";
    case GLSL_INTERFACE_FRONT_FACING:
        return "gl_FrontFacing";
    case GLSL_INTERFACE_FRAG_COLOR:
        return "gl_FragColor";
    case GLSL_INTERFACE_FRAG_DEPTH:
        return "gl_FragDepth";
    default:
        return NULL;
    }
}
~~~

Translate COLOR0 output and DEPTH writes directly. Translate WPOS/POSITION and
FACE reads directly. Emit ordinary fragment input varyings with the same
canonical names and exact types as vertex output varyings.

- [ ] **Step 3: Add fragment control flow and discard**

Create **tests/glsl/expressions/fp_control.cg**:

~~~c
struct FragmentOut {
    float4 color : COLOR0;
};

FragmentOut main(float2 texcoord : TEXCOORD0,
                 uniform float4 tint)
{
    FragmentOut output;
    if (texcoord.x < 0.0)
        discard;
    if (texcoord.y > 1.0)
        output.color = tint;
    else
        output.color = tint * texcoord.y;
    return output;
}
~~~

Register glslf_control. Lower DISCARD_STMT and KILL_OP to
GLSL_STMT_DISCARD only when module.stage is GLSL_STAGE_FRAGMENT. The exact
expected file is:

~~~glsl
#version 110
// cgc-bind varying cg_TEXCOORD0 TEXCOORD0
// cgc-bind uniform tint
// cgc-bind builtin gl_FragColor COLOR0

struct FragmentOut
{
    vec4 color;
};

varying vec2 cg_TEXCOORD0;
uniform vec4 tint;

void main()
{
    vec2 texcoord;
    FragmentOut output;
    texcoord = cg_TEXCOORD0;
    if (texcoord.x < 0.0)
    {
        discard;
    }
    if (texcoord.y > 1.0)
    {
        output.color = tint;
    }
    else
    {
        output.color = tint * texcoord.y;
    }
    gl_FragColor = output.color;
}
~~~

- [ ] **Step 4: Add gl_FragCoord and gl_FrontFacing coverage**

Create **tests/glsl/semantics/fp_builtins.cg**:

~~~c
struct FragmentOut {
    float4 color : COLOR0;
    float depth : DEPTH;
};

FragmentOut main(float4 windowPosition : WPOS, bool front : FACE)
{
    FragmentOut output;
    if (front)
        output.color = float4(1.0, 1.0, 1.0, 1.0);
    else
        output.color = float4(0.0, 0.0, 0.0, 1.0);
    output.depth = windowPosition.z;
    return output;
}
~~~

Register glslf_builtins. Its expected output must initialize windowPosition
from gl_FragCoord and front from gl_FrontFacing, write gl_FragColor, write
gl_FragDepth, and declare no varying for WPOS or FACE. Use this exact metadata
and body shape:

~~~glsl
#version 110
// cgc-bind builtin gl_FragCoord WPOS0
// cgc-bind builtin gl_FrontFacing FACE0
// cgc-bind builtin gl_FragColor COLOR0
// cgc-bind builtin gl_FragDepth DEPTH0

struct FragmentOut
{
    vec4 color;
    float depth;
};

void main()
{
    vec4 windowPosition;
    bool front;
    FragmentOut output;
    windowPosition = gl_FragCoord;
    front = gl_FrontFacing;
    if (front)
    {
        output.color = vec4(1.0, 1.0, 1.0, 1.0);
    }
    else
    {
        output.color = vec4(0.0, 0.0, 0.0, 1.0);
    }
    output.depth = windowPosition.z;
    gl_FragColor = output.color;
    gl_FragDepth = output.depth;
}
~~~

- [ ] **Step 5: Generate, review, and freeze fragment expectations**

Run the two exact tests against the hand-written expectations. If formatting
differs, change the emitter to the established contract rather than updating
these expectations. Review that the output has #version 110, no extensions,
no output varying declarations, and no pseudo connector names.

- [ ] **Step 6: Run and commit fragment source generation**

Run:

~~~powershell
cmake --build build-glsl --config Debug --target cgc
ctest --test-dir build-glsl -C Debug -R '^(glslf_(passthrough|control|builtins)|glslv_|generic_)' --output-on-failure
git add -- glsl_lower.c glsl_codegen.c tests/CMakeLists.txt tests/glsl/profile/fp_passthrough.* tests/glsl/expressions/fp_control.* tests/glsl/semantics/fp_builtins.*
git commit -m "Emit GLSL fragment shaders"
~~~

Expected: all listed fragment, vertex, and generic tests pass.

### Task 9: Register samplers and translate fragment texture calls

**Files:**
- Modify: **glsl_hal.h**
- Modify: **glsl_hal.c**
- Modify: **glsl_lower.c**
- Modify: **stdlib.cg**
- Regenerate: **stdlib.c**
- Create: **tests/glsl/textures/fp_tex2d.cg**
- Create: **tests/glsl/textures/fp_tex2d.expected**
- Create: **tests/glsl/textures/fp_texcube.cg**
- Create: **tests/glsl/textures/fp_texcube.expected**
- Create: **tests/glsl/textures/fp_tex1d.cg**
- Create: **tests/glsl/textures/fp_tex3d.cg**
- Modify: **tests/CMakeLists.txt**

- [ ] **Step 1: Add failing 2D and cube texture fixtures**

Create **tests/glsl/textures/fp_tex2d.cg**:

~~~c
struct FragmentOut {
    float4 color : COLOR0;
};

FragmentOut main(float2 texcoord : TEXCOORD0,
                 uniform sampler2D image)
{
    FragmentOut output;
    output.color = tex2D(image, texcoord);
    return output;
}
~~~

Create **tests/glsl/textures/fp_texcube.cg**:

~~~c
struct FragmentOut {
    float4 color : COLOR0;
};

FragmentOut main(float3 direction : TEXCOORD0,
                 uniform samplerCUBE environment)
{
    FragmentOut output;
    output.color = texCUBE(environment, direction);
    return output;
}
~~~

Register glslf_tex2d and glslf_texcube. Run them and expect unknown sampler
types or intrinsic declarations.

Create **tests/glsl/textures/fp_tex2d.expected**:

~~~glsl
#version 110
// cgc-bind varying cg_TEXCOORD0 TEXCOORD0
// cgc-bind sampler image 0
// cgc-bind builtin gl_FragColor COLOR0

struct FragmentOut
{
    vec4 color;
};

varying vec2 cg_TEXCOORD0;
uniform sampler2D image;

void main()
{
    vec2 texcoord;
    FragmentOut output;
    texcoord = cg_TEXCOORD0;
    output.color = texture2D(image, texcoord);
    gl_FragColor = output.color;
}
~~~

Create **tests/glsl/textures/fp_texcube.expected**:

~~~glsl
#version 110
// cgc-bind varying cg_TEXCOORD0 TEXCOORD0
// cgc-bind sampler environment 0
// cgc-bind builtin gl_FragColor COLOR0

struct FragmentOut
{
    vec4 color;
};

varying vec3 cg_TEXCOORD0;
uniform samplerCube environment;

void main()
{
    vec3 direction;
    FragmentOut output;
    direction = cg_TEXCOORD0;
    output.color = textureCube(environment, direction);
    gl_FragColor = output.color;
}
~~~

- [ ] **Step 2: Register the four sampler base types in both profiles**

Add to **glsl_hal.h**:

~~~c
#define TYPE_BASE_GLSL_SAMPLER1D   (TYPE_BASE_FIRST_USER + 0)
#define TYPE_BASE_GLSL_SAMPLER2D   (TYPE_BASE_FIRST_USER + 1)
#define TYPE_BASE_GLSL_SAMPLER3D   (TYPE_BASE_FIRST_USER + 2)
#define TYPE_BASE_GLSL_SAMPLERCUBE (TYPE_BASE_FIRST_USER + 3)
~~~

In RegisterNames_glsl, call this helper once for each profile:

~~~c
static void RegisterSamplerType(const char *name, int base)
{
    SourceLoc loc = { 0, 0 };
    Type *type;
    int atom;

    type = NewType(TYPE_CATEGORY_SCALAR | base, 1);
    atom = LookUpAddString(atable, name);
    SetScalarTypeName(base, atom, type);
    AddSymbol(&loc, CurrentScope, atom, type, TYPEDEF_S);
}
~~~

Register sampler1D, sampler2D, sampler3D, and samplerCUBE. Install
IsTexobjBase_glsl and IsValidRuntimeBase_glsl; the former recognizes exactly
the four bases, and the latter accepts the existing float/int/bool bases plus
those samplers.

- [ ] **Step 3: Add exact texture prototypes to the GLSL stdlib branch**

Add these declarations under PROFILE_GLSL:

~~~c
__internal float4 tex1D(sampler1D image, float coord);
__internal float4 tex2D(sampler2D image, float2 coord);
__internal float4 tex3D(sampler3D image, float3 coord);
__internal float4 texCUBE(samplerCUBE image, float3 coord);
~~~

Map them to GLSL_BUILTIN_TEX1D, TEX2D, TEX3D, and TEXCUBE only when the
resolved parameter types exactly match.

- [ ] **Step 4: Allocate deterministic fragment texture units**

When collecting uniform declarations, recognize sampler bases before numeric
uniform counting. In source declaration order:

1. assign unit zero to the first sampler and unit one to the second;
2. change the source Binding to BK_TEXUNIT with BIND_IS_BOUND |
   BIND_INPUT | BIND_UNIFORM and unitno;
3. emit cgc-bind sampler lines containing the emitted name and unit; and
4. reject a third sampler during preflight resource validation.

For glslv, preserve the sampler declaration long enough to report the portable
zero-unit stage/resource diagnostic instead of an unknown-type parser error.

- [ ] **Step 5: Lower exact base texture calls**

Lower:

| Builtin ID | GLSL call | Coordinate |
| --- | --- | --- |
| TEX1D | texture1D | float |
| TEX2D | texture2D | vec2 |
| TEX3D | texture3D | vec3 |
| TEXCUBE | textureCube | vec3 |

Require fragment stage, the exact sampler type, the exact coordinate shape,
and float4 result. Do not accept projective, bias, gradient, explicit-level,
rectangle, or shadow variants.

- [ ] **Step 6: Add 1D and 3D coverage**

Create **tests/glsl/textures/fp_tex1d.cg** and
**tests/glsl/textures/fp_tex3d.cg**:

~~~c
/* fp_tex1d.cg */
struct FragmentOut { float4 color : COLOR0; };
FragmentOut main(float coord : TEXCOORD0, uniform sampler1D image)
{
    FragmentOut output;
    output.color = tex1D(image, coord);
    return output;
}
~~~

~~~c
/* fp_tex3d.cg */
struct FragmentOut { float4 color : COLOR0; };
FragmentOut main(float3 coord : TEXCOORD0, uniform sampler3D image)
{
    FragmentOut output;
    output.color = tex3D(image, coord);
    return output;
}
~~~

Register exact golden tests for both. Generate only these two expectations
after the implementation step using the command in Step 7.

- [ ] **Step 7: Regenerate stdlib and freeze texture outputs**

Run:

~~~powershell
cmake --build build-glsl --config Debug --target regenerate_stdlib
cmake --build build-glsl --config Debug --target cgc
$cgc = (Resolve-Path '.\build-glsl\Debug\cgc.exe').Path
$textureDir = (Resolve-Path '.\tests\glsl\textures').Path
$actualDir = (Resolve-Path '.\build-glsl').Path
foreach ($case in 'fp_tex1d','fp_tex3d') {
    cmake -DUPDATE_EXPECTED=ON -DCGC="$cgc" -DPROFILE=glslf -DSOURCE="$textureDir\$case.cg" -DEXPECTED="$textureDir\$case.expected" -DACTUAL="$actualDir\$case.glsl" -P tests/check_glsl.cmake
    Get-Content "$textureDir\$case.expected"
}
~~~

Run the hand-written 2D and cube expectations first. Use UPDATE_EXPECTED=ON
only for the 1D and 3D expectations, then inspect that they declare the
matching GLSL sampler uniform, emit texture1D or texture3D, and contain a
cgc-bind sampler line with unit zero.

- [ ] **Step 8: Run and commit texture support**

Run:

~~~powershell
ctest --test-dir build-glsl -C Debug -R '^(glslf_tex|glslf_|glslv_|generic_)' --output-on-failure
git add -- glsl_hal.h glsl_hal.c glsl_lower.c stdlib.cg stdlib.c tests/CMakeLists.txt tests/glsl/textures
git commit -m "Translate GLSL texture sampling"
~~~

### Task 10: Enforce diagnostics and portable resource limits

**Files:**
- Modify: **errors.h**
- Modify: **semantic.c:247-305**
- Modify: **check.c:350-380**
- Modify: **glsl_hal.c**
- Modify: **glsl_lower.c**
- Create: **tests/check_diagnostic.cmake**
- Create: **tests/glsl/diagnostics/fp_color1.cg**
- Create: **tests/glsl/diagnostics/vp_non_square.cg**
- Create: **tests/glsl/diagnostics/vp_texture.cg**
- Create: **tests/glsl/diagnostics/vp_bitwise.cg**
- Create: **tests/glsl/diagnostics/vp_duplicate_semantic.cg**
- Create: **tests/glsl/limits/vp_attributes17.cg**
- Create: **tests/glsl/limits/vp_varyings36.cg**
- Create: **tests/glsl/limits/fp_uniforms68.cg**
- Create: **tests/glsl/limits/fp_samplers3.cg**
- Modify: **tests/glsl_ir_test.c**
- Modify: **tests/CMakeLists.txt**

- [ ] **Step 1: Add the exact diagnostic runner**

Create **tests/check_diagnostic.cmake**:

~~~cmake
foreach(required CGC PROFILE SOURCE CODE ACTUAL)
    if(NOT DEFINED ${required})
        message(FATAL_ERROR "${required} must be defined")
    endif()
endforeach()

file(REMOVE "${ACTUAL}")
execute_process(
    COMMAND "${CGC}" -quiet -profile "${PROFILE}" -o "${ACTUAL}" "${SOURCE}"
    RESULT_VARIABLE result
    OUTPUT_VARIABLE stdout
    ERROR_VARIABLE stderr
)
if(result EQUAL 0)
    message(FATAL_ERROR "expected ${PROFILE} compile to fail for ${SOURCE}")
endif()
set(diagnostics "${stdout}${stderr}")
if(NOT diagnostics MATCHES "error C${CODE}:")
    message(FATAL_ERROR
        "expected error C${CODE} for ${SOURCE}:\n${diagnostics}")
endif()

if(EXISTS "${ACTUAL}")
    file(READ "${ACTUAL}" shader)
    string(REPLACE "\r\n" "\n" shader "${shader}")
    if(shader MATCHES "\n(attribute|varying|uniform|struct|void main)[ \t\n]")
        message(FATAL_ERROR "failed translation emitted a partial shader body")
    endif()
endif()
~~~

Add add_glsl_diagnostic(name profile source code) to tests/CMakeLists.txt.

- [ ] **Step 2: Define the GLSL diagnostic range**

Add to **errors.h**:

~~~c
#define ERROR_S_GLSL_UNSUPPORTED_TYPE       6200, "GLSL 1.10 does not support type \"%s\""
#define ERROR_S_GLSL_UNSUPPORTED_OPERATION  6201, "GLSL 1.10 does not support operation \"%s\""
#define ERROR_SS_GLSL_STAGE_OPERATION       6202, "%s profile does not support operation \"%s\""
#define ERROR_S_GLSL_SEMANTIC               6203, "GLSL profile cannot bind semantic \"%s\""
#define ERROR_S_GLSL_INTERFACE_CONFLICT     6204, "GLSL interface conflicts at semantic \"%s\""
#define ERROR_S_GLSL_NAME_COLLISION         6205, "GLSL name cannot be resolved for \"%s\""
#define ERROR_S_GLSL_INTRINSIC              6206, "GLSL 1.10 has no exact intrinsic for \"%s\""
#define ERROR_SII_GLSL_RESOURCE_LIMIT       6207, "GLSL portable %s limit exceeded: %d used, %d available"
#define ERROR_S_GLSL_SAMPLER                6208, "GLSL 1.10 does not support sampler feature \"%s\""
#define ERROR_S_GLSL_NON_SQUARE_MATRIX      6209, "GLSL 1.10 requires a square matrix, found \"%s\""
~~~

- [ ] **Step 3: Prevent duplicate generic binding diagnostics**

In **semantic.c**, add int errorsBefore to lBindUniformVariable and
lBindVaryingVariable. Immediately before BindUniformPragma,
BindVaryingSemantic, BindVaryingPragma, and BindVaryingUnbound, assign
errorsBefore = GetErrorCount(). Change each callback-failure else branch to
else if (GetErrorCount() == errorsBefore), leaving its existing SemanticError
call and success branch unchanged.

In **check.c** BindUnboundUniformMembers, add int errorsBefore, assign it
immediately before BindUniformUnbound, and emit
WARNING_S_CANT_BIND_UNIFORM_VAR only when the callback returned zero and
GetErrorCount() still equals errorsBefore.

Generic callbacks emit no errors themselves, so their existing fallback
diagnostics and warnings remain identical.

- [ ] **Step 4: Add unsupported feature fixtures**

Create **tests/glsl/diagnostics/vp_non_square.cg**:

~~~c
struct VertexOut { float4 position : POSITION; };
VertexOut main(float4 position : ATTRIB0, uniform float4x3 transform)
{
    VertexOut output;
    output.position = position;
    return output;
}
~~~

Expect C6209.

Create **tests/glsl/diagnostics/vp_texture.cg**:

~~~c
struct VertexOut { float4 position : POSITION; };
VertexOut main(float4 position : ATTRIB0,
               float2 texcoord : ATTRIB1,
               uniform sampler2D image)
{
    VertexOut output;
    output.position = tex2D(image, texcoord) + position;
    return output;
}
~~~

Expect C6202.

Create **tests/glsl/diagnostics/vp_bitwise.cg**:

~~~c
struct VertexOut { float4 position : POSITION; };
VertexOut main(float4 position : ATTRIB0, uniform int mask)
{
    VertexOut output;
    int value;
    value = mask & 3;
    output.position = position * float(value);
    return output;
}
~~~

Expect C6201.

Create **tests/glsl/diagnostics/fp_color1.cg**:

~~~c
struct FragmentOut { float4 color : COLOR1; };
FragmentOut main()
{
    FragmentOut output;
    output.color = float4(1.0, 0.0, 0.0, 1.0);
    return output;
}
~~~

Expect C6203.

Create **tests/glsl/diagnostics/vp_duplicate_semantic.cg**:

~~~c
struct AppData {
    float4 first : ATTRIB0;
    float2 second : ATTRIB0;
};
struct VertexOut { float4 position : POSITION; };
VertexOut main(AppData input)
{
    VertexOut output;
    output.position = input.first + float4(input.second, 0.0, 0.0);
    return output;
}
~~~

Expect C6204 at the second ATTRIB0 declaration.

- [ ] **Step 5: Add exact portable-limit fixtures**

Create **tests/glsl/limits/vp_attributes17.cg**:

~~~c
struct AppData {
    float4 position : POSITION;
    float4 a0 : ATTRIB0;
    float4 a1 : ATTRIB1;
    float4 a2 : ATTRIB2;
    float4 a3 : ATTRIB3;
    float4 a4 : ATTRIB4;
    float4 a5 : ATTRIB5;
    float4 a6 : ATTRIB6;
    float4 a7 : ATTRIB7;
    float4 a8 : ATTRIB8;
    float4 a9 : ATTRIB9;
    float4 a10 : ATTRIB10;
    float4 a11 : ATTRIB11;
    float4 a12 : ATTRIB12;
    float4 a13 : ATTRIB13;
    float4 a14 : ATTRIB14;
    float4 a15 : ATTRIB15;
};
struct VertexOut { float4 position : POSITION; };
VertexOut main(AppData input)
{
    VertexOut output;
    output.position = input.position + input.a0 + input.a1 + input.a2 +
        input.a3 + input.a4 + input.a5 + input.a6 + input.a7 + input.a8 +
        input.a9 + input.a10 + input.a11 + input.a12 + input.a13 +
        input.a14 + input.a15;
    return output;
}
~~~

Expect C6207 with 17 used and 16 available.

Create **tests/glsl/limits/vp_varyings36.cg**:

~~~c
struct VertexOut {
    float4 position : POSITION;
    float4 t0 : TEXCOORD0;
    float4 t1 : TEXCOORD1;
    float4 t2 : TEXCOORD2;
    float4 t3 : TEXCOORD3;
    float4 t4 : TEXCOORD4;
    float4 t5 : TEXCOORD5;
    float4 t6 : TEXCOORD6;
    float4 t7 : TEXCOORD7;
    float4 color : COLOR0;
};
VertexOut main(float4 position : ATTRIB0)
{
    VertexOut output;
    output.position = position;
    output.t0 = position;
    output.t1 = position;
    output.t2 = position;
    output.t3 = position;
    output.t4 = position;
    output.t5 = position;
    output.t6 = position;
    output.t7 = position;
    output.color = position;
    return output;
}
~~~

Expect C6207 with 36 varying components used and 32 available.

Create **tests/glsl/limits/fp_uniforms68.cg**:

~~~c
struct FragmentOut { float4 color : COLOR0; };
FragmentOut main(uniform float4 values[17])
{
    FragmentOut output;
    output.color = values[0];
    return output;
}
~~~

Expect C6207 with 68 used and 64 available.

Create **tests/glsl/limits/fp_samplers3.cg**:

~~~c
struct FragmentOut { float4 color : COLOR0; };
FragmentOut main(float2 texcoord : TEXCOORD0,
                 uniform sampler2D first,
                 uniform sampler2D second,
                 uniform sampler2D third)
{
    FragmentOut output;
    output.color = tex2D(first, texcoord) +
        tex2D(second, texcoord) + tex2D(third, texcoord);
    return output;
}
~~~

Expect C6207 with 3 used and 2 available.

- [ ] **Step 6: Validate resources before code generation**

Add one recursive counter for attributes, varying float components, numeric
uniform components, sampler units, and fragment color outputs. Count each
interface declaration once after semantic canonicalization. Compare against
the selected GlslLimits and emit C6207 at the declaration that crosses the
limit.

Run preflight after the full module is lowered but before GlslWriteModule.
Unsupported type/op/stage/intrinsic errors use the nearest AST SourceLoc.

- [ ] **Step 7: Complete semantic and stage checks**

Emit C6203 inside the profile binder for recognized but invalid semantic
indices/directions, C6204 for duplicate canonical interfaces with conflicting
types, C6202 for discard or texture in glslv, C6206 for a recognized internal
function signature without an exact lowering, C6208 for unsupported sampler
features, and C6209 for non-square matrices.

Return callback failure after emitting the specific error; the semantic.c
error-count guard prevents a second generic message.

- [ ] **Step 8: Extend name unit coverage**

Add assertions that two different identities named position receive position
and position_1, a repeated identity reuses its first name, every reserved word
in the static table receives cg_, and a source name beginning gl_ receives
cg_gl_. Add these exact intrinsic/resource assertions:

~~~c
{
    GlslType result = GlslNumericType(GLSL_BASE_FLOAT, 4);
    GlslType params[3];
    GlslType arrayType;

    params[0] = result;
    assert(GlslLookupBuiltin("rsqrt", &result, params, 1) ==
           GLSL_BUILTIN_RSQRT);
    params[1] = result;
    params[2] = GlslNumericType(GLSL_BASE_FLOAT, 1);
    assert(GlslLookupBuiltin("lerp", &result, params, 3) ==
           GLSL_BUILTIN_LERP);
    assert(GlslLookupBuiltin("unknown", &result, params, 1) ==
           GLSL_BUILTIN_NONE);

    arrayType = result;
    arrayType.arraySize = 17;
    assert(GlslTypeComponentCount(&arrayType) == 68);
}
~~~

- [ ] **Step 9: Run diagnostics, successful profiles, and generic tests**

Run:

~~~powershell
cmake --build build-glsl --config Debug --target cgc glsl_ir_unit
ctest --test-dir build-glsl -C Debug -R '^(glsl_.*diagnostic|glsl_.*limit|glsl(v|f)_|glsl_ir_unit|generic_)' --output-on-failure
~~~

Expected: every negative test sees its exact code and no partial body; every
successful GLSL test, the unit test, and every generic test passes.

- [ ] **Step 10: Commit diagnostics and limits**

Run:

~~~powershell
git add -- errors.h semantic.c check.c glsl_hal.c glsl_lower.c tests/CMakeLists.txt tests/check_diagnostic.cmake tests/glsl/diagnostics tests/glsl/limits tests/glsl_ir_test.c
git commit -m "Validate GLSL profile constraints"
~~~

### Task 11: Add interface linking, optional validation, documentation, and final verification

**Files:**
- Create: **tests/validate_glsl.cmake**
- Create: **tests/check_glsl_interface.cmake**
- Create: **tests/glsl/link/vp_link.cg**
- Create: **tests/glsl/link/fp_link.cg**
- Modify: **tests/CMakeLists.txt**
- Modify: **README.txt**

- [ ] **Step 1: Add matching cross-stage fixtures**

Create **tests/glsl/link/vp_link.cg**:

~~~c
struct VertexOut {
    float4 position : POSITION;
    float4 color : COLOR0;
    float2 texcoord : TEXCOORD0;
};

VertexOut main(float4 position : ATTRIB0,
               float4 color : ATTRIB1,
               float2 texcoord : ATTRIB2)
{
    VertexOut output;
    output.position = position;
    output.color = color;
    output.texcoord = texcoord;
    return output;
}
~~~

Create **tests/glsl/link/fp_link.cg**:

~~~c
struct FragmentOut {
    float4 color : COLOR0;
};

FragmentOut main(float4 color : COLOR0, float2 texcoord : TEXCOORD0)
{
    FragmentOut output;
    output.color = color * float4(texcoord, 1.0, 1.0);
    return output;
}
~~~

- [ ] **Step 2: Compare emitted varying interfaces**

Create **tests/check_glsl_interface.cmake**. Require CGC, VERTEX_SOURCE,
FRAGMENT_SOURCE, VERTEX_OUTPUT, and FRAGMENT_OUTPUT. Compile each source with
glslv/glslf and -quiet. Read both files, normalize CRLF, extract:

~~~cmake
string(REGEX MATCHALL "varying [^;\n]+;" vertex_varyings "${vertex_text}")
string(REGEX MATCHALL "varying [^;\n]+;" fragment_varyings "${fragment_text}")
list(SORT vertex_varyings)
list(SORT fragment_varyings)
if(NOT vertex_varyings STREQUAL fragment_varyings)
    message(FATAL_ERROR
        "varying mismatch\nvertex: ${vertex_varyings}\n"
        "fragment: ${fragment_varyings}")
endif()
~~~

Register glsl_link_interface and run it. Expected: exact equality for
cg_COLOR0 and cg_TEXCOORD0 declarations.

- [ ] **Step 3: Add the optional validator runner**

Create **tests/validate_glsl.cmake**:

~~~cmake
foreach(required CGC VALIDATOR PROFILE STAGE SOURCE OUTPUT)
    if(NOT DEFINED ${required})
        message(FATAL_ERROR "${required} must be defined")
    endif()
endforeach()

file(REMOVE "${OUTPUT}")
execute_process(
    COMMAND "${CGC}" -quiet -profile "${PROFILE}" -o "${OUTPUT}" "${SOURCE}"
    RESULT_VARIABLE cgc_result
    OUTPUT_VARIABLE cgc_stdout
    ERROR_VARIABLE cgc_stderr
)
if(NOT cgc_result EQUAL 0)
    message(FATAL_ERROR "cgc failed: ${cgc_stdout}${cgc_stderr}")
endif()

execute_process(
    COMMAND "${VALIDATOR}" -S "${STAGE}" "${OUTPUT}"
    RESULT_VARIABLE validator_result
    OUTPUT_VARIABLE validator_stdout
    ERROR_VARIABLE validator_stderr
)
if(NOT validator_result EQUAL 0)
    message(FATAL_ERROR
        "glslangValidator failed:\n${validator_stdout}${validator_stderr}")
endif()
~~~

- [ ] **Step 4: Detect glslangValidator and register every success fixture**

In **tests/CMakeLists.txt**:

~~~cmake
find_program(GLSLANG_VALIDATOR glslangValidator)

function(add_glsl_validation name profile stage source)
    if(GLSLANG_VALIDATOR)
        add_test(
            NAME validate_${name}
            COMMAND ${CMAKE_COMMAND}
                -DCGC=$<TARGET_FILE:cgc>
                -DVALIDATOR=${GLSLANG_VALIDATOR}
                -DPROFILE=${profile}
                -DSTAGE=${stage}
                -DSOURCE=${source}
                -DOUTPUT=${CMAKE_CURRENT_BINARY_DIR}/${name}.validated.glsl
                -P ${CMAKE_CURRENT_SOURCE_DIR}/validate_glsl.cmake
        )
    endif()
endfunction()
~~~

Register all successful vertex fixtures with stage vert and all successful
fragment fixtures with stage frag. If the validator is absent, print one
configure-time STATUS message and leave all golden/unit/negative tests active.

- [ ] **Step 5: Document the two profiles**

Replace the README statement that generic is the only profile with a Profiles
section containing:

~~~text
The compiler provides these profiles:

  generic  performs the historical semantic checks and prints the compiler tree
  glslv    translates a Cg vertex entry point to strict GLSL 1.10
  glslf    translates a Cg fragment entry point to strict GLSL 1.10

Examples:

  cgc -quiet -profile glslv -entry main -o shader.vert shader.cg
  cgc -quiet -profile glslf -entry main -o shader.frag shader.cg

GLSL output begins with #version 110 and requires no extensions. Vertex inputs
are named attributes such as cg_ATTRIB0. Vertex outputs and fragment inputs use
matching semantic names such as cg_TEXCOORD0. POSITION output, fragment COLOR0,
and fragment DEPTH map to gl_Position, gl_FragColor, and gl_FragDepth.

Uniform defaults and semantic mappings are emitted as // cgc-default and
// cgc-bind comments. Applications must apply defaults after linking and must
upload matrices using the GLSL matrix convention.

The first release supports readable structs, helpers, arrays, square matrices,
structured control flow, common numeric intrinsics, and base 1D/2D/3D/cube
fragment texture sampling. It rejects extensions, non-square matrices,
multiple fragment colors, vertex texture sampling, recursion, shadow or
rectangle samplers, and resource use above the OpenGL 2.0 portable minima.
~~~

Also document that glslangValidator is optional and discovered only for tests.

- [ ] **Step 6: Verify standard-library synchronization**

Run:

~~~powershell
cmake --build build-glsl --config Debug --target regenerate_stdlib
git diff --exit-code -- stdlib.c
~~~

Expected: regeneration makes no new change; stdlib.c is synchronized with the
checked-in stdlib.cg.

- [ ] **Step 7: Run complete Debug and Release verification**

Run:

~~~powershell
cmake -S . -B build-glsl -DBUILD_TESTING=ON
cmake --build build-glsl --config Debug
ctest --test-dir build-glsl -C Debug --output-on-failure
cmake --build build-glsl --config Release
ctest --test-dir build-glsl -C Release --output-on-failure
~~~

Expected: all registered tests pass in both configurations. When
glslangValidator is present, every validate_ test also passes.

- [ ] **Step 8: Perform the manual acceptance conversions**

Run:

~~~powershell
$cgc = Resolve-Path '.\build-glsl\Release\cgc.exe'
foreach ($shader in 'position.cg','reflection.cg','vertexlight.cg','vertexlight4.cg') {
    $name = [IO.Path]::GetFileNameWithoutExtension($shader)
    & $cgc -quiet -profile glslv -o "build-glsl\$name.vert" $shader
    if ($LASTEXITCODE -ne 0) { throw "glslv failed: $shader" }
}
& $cgc -quiet -profile glslf -o build-glsl\texture.frag tests\glsl\textures\fp_tex2d.cg
if ($LASTEXITCODE -ne 0) { throw 'glslf texture conversion failed' }
~~~

Expected: five output files begin with #version 110 and contain readable main
functions.

- [ ] **Step 9: Check the final patch and commit documentation/integration**

Run:

~~~powershell
git diff --check
git status --short
git add -- README.txt tests/CMakeLists.txt tests/validate_glsl.cmake tests/check_glsl_interface.cmake tests/glsl/link
git commit -m "Document and validate GLSL profiles"
~~~

Expected: Git commits only the intended documentation and final test
integration. Pre-existing untracked build-win32 and unrelated plan files remain
uncommitted.

- [ ] **Step 10: Run final evidence before declaring completion**

Run:

~~~powershell
git status --short
ctest --test-dir build-glsl -C Debug --output-on-failure
ctest --test-dir build-glsl -C Release --output-on-failure
git log --oneline --decorate -12
~~~

Expected: both full suites pass. Status contains only pre-existing unrelated
untracked files. The recent history contains focused commits for generic
protection, registration, IR, semantics, vertex emission, fragment emission,
textures, validation, and documentation.

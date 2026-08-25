# DirectX 9.0c HLSL Shader Model 3 Profiles Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add `hlslv` and `hlslf` profiles that translate the current Cg front end into deterministic HLSL source accepted by `fxc` targets `vs_3_0` and `ps_3_0`.

**Architecture:** Add a private typed HLSL IR and a staged backend: Cg lowering, entry-wrapper construction, SM3 legalization, deterministic `c/i/b/s` allocation, whole-module validation, and source emission. The vertex and pixel HAL descriptors share the backend but own their legal semantics, operations, and resource limits; existing generic, GLSL, and ARB behavior remains isolated.

**Tech Stack:** C90, the existing Cg AST and HAL interfaces, CMake/CTest, checked-in golden HLSL, optional Microsoft `fxc.exe`, and an optional locally installed NVIDIA Cg oracle.

---

## Working conventions

Run the implementation in a dedicated worktree. Use a build directory inside
that worktree, such as `build-hlsl`; do not reuse the checked-in `build-win32`
directory. Preserve the NVIDIA notice in every new C or header file. All test
commands below assume PowerShell at the worktree root.

Before changing code, run:

```powershell
cmake -S . -B build-hlsl -DBUILD_TESTING=ON
cmake --build build-hlsl --config Debug
ctest --test-dir build-hlsl -C Debug --output-on-failure
```

Expected: configure and build succeed and the complete pre-HLSL test suite
passes. Record the test count in the implementation notes.

## File map

### New backend files

- `hlsl_hal.h`: public profile IDs, stage/profile descriptors, limits, and
  shared backend entry points.
- `hlsl_hal.c`: common HAL callbacks, profile registration, builtin discovery,
  backend orchestration, and diagnostic translation.
- `hlslv_hal.c`: vertex connectors, semantic aliases, stage policy, and SM3
  limits.
- `hlslf_hal.c`: pixel connectors, semantic aliases, stage policy, and SM3
  limits.
- `hlsl_ir.h` / `hlsl_ir.c`: typed structured IR, allocation helpers, source
  identities, deterministic names, type utilities, and IR error state.
- `hlsl_lower.c`: Cg AST to HLSL IR translation.
- `hlsl_legalize.c`: exact Cg-to-SM3 semantic rewrites and builtin selection.
- `hlsl_bind.c`: interface canonicalization, wrapper construction, physical
  register allocation, defaults, and metadata.
- `hlsl_validate.c`: IR invariants, stage checks, interface checks, and source-
  visible SM3 limits.
- `hlsl_codegen.c`: deterministic HLSL and metadata writer.

### New test and documentation files

- `tests/hlsl_ir_test.c`: IR, name, type, builtin, wrapper, binding, and limit
  unit tests.
- `tests/check_hlsl.cmake`: exact successful-output runner.
- `tests/check_hlsl_failure.cmake`: exact diagnostic and transactional-output
  runner.
- `tests/check_hlsl_link.cmake`: vertex/pixel interface compatibility runner.
- `tests/validate_hlsl.cmake`: optional `fxc` runner.
- `tests/check_hlsl_oracle.cmake`: optional NVIDIA Cg comparison runner.
- `tests/hlsl/`: profile, interface, expression, aggregate, matrix, intrinsic,
  texture, binding, limit, and diagnostic fixtures.
- `docs/hlsl-sm3-compatibility.md`: exhaustive support/rejection matrix.

### Existing files changed

- `CMakeLists.txt`: compile the backend and expose the optional oracle cache
  variable.
- `tests/CMakeLists.txt`: build the unit target and register every fixture.
- `cgcmain.c`: register the two profiles.
- `errors.h`: reserve and define HLSL diagnostics.
- `README.md`: document profiles, target commands, bindings, defaults, matrix
  layout, validation, and limitations.

## Stable interfaces used throughout the plan

Task 1 uses a registration-only local representation. Task 2 introduces these
permanent definitions in `hlsl_ir.h`; every later task extends behavior without
renaming them:

```c
typedef enum HlslStage_Enum {
    HLSL_STAGE_VERTEX,
    HLSL_STAGE_PIXEL
} HlslStage;

typedef enum HlslBase_Enum {
    HLSL_BASE_VOID,
    HLSL_BASE_FLOAT,
    HLSL_BASE_INT,
    HLSL_BASE_BOOL,
    HLSL_BASE_SAMPLER1D,
    HLSL_BASE_SAMPLER2D,
    HLSL_BASE_SAMPLER3D,
    HLSL_BASE_SAMPLERCUBE,
    HLSL_BASE_STRUCT
} HlslBase;

typedef enum HlslRegisterBank_Enum {
    HLSL_REGISTER_NONE,
    HLSL_REGISTER_C,
    HLSL_REGISTER_I,
    HLSL_REGISTER_B,
    HLSL_REGISTER_S
} HlslRegisterBank;

typedef enum HlslErrorKind_Enum {
    HLSL_ERROR_NONE,
    HLSL_ERROR_UNSUPPORTED_TYPE,
    HLSL_ERROR_UNSUPPORTED_OPERATION,
    HLSL_ERROR_STAGE_OPERATION,
    HLSL_ERROR_SEMANTIC,
    HLSL_ERROR_INTERFACE_CONFLICT,
    HLSL_ERROR_REQUIRED_POSITION,
    HLSL_ERROR_ENTRY_ABI,
    HLSL_ERROR_REGISTER_COLLISION,
    HLSL_ERROR_RESOURCE_LIMIT,
    HLSL_ERROR_SAMPLER,
    HLSL_ERROR_INTRINSIC,
    HLSL_ERROR_NAME_COLLISION,
    HLSL_ERROR_INVALID_IR
} HlslErrorKind;

typedef struct HlslProfileDesc_Rec HlslProfileDesc;
typedef struct HlslModule_Rec HlslModule;
typedef void *(*HlslAllocFn)(void *arg, size_t size);

void HlslInitModule(HlslModule *module, HlslStage stage,
    HlslAllocFn alloc, void *allocArg);
int HlslLowerProgram(HlslModule *module, const HlslProfileDesc *profile,
    SourceLoc *loc, Scope *scope, Symbol *program);
int HlslBuildEntryWrapper(HlslModule *module,
    const HlslProfileDesc *profile);
int HlslLegalizeModule(HlslModule *module,
    const HlslProfileDesc *profile);
int HlslAllocateBindings(HlslModule *module,
    const HlslProfileDesc *profile);
int HlslValidateModule(HlslModule *module,
    const HlslProfileDesc *profile);
int HlslWriteModule(FILE *out, const HlslModule *module,
    const HlslProfileDesc *profile);
```

`GenerateCode_hlsl` must call those functions in exactly that order and must
call `HlslWriteModule` only if all preceding calls returned true.

### Task 1: Add profile registration, runners, and a transactional skeleton

**Files:**
- Create: `hlsl_hal.h`
- Create: `hlsl_hal.c`
- Create: `hlslv_hal.c`
- Create: `hlslf_hal.c`
- Create: `tests/check_hlsl.cmake`
- Create: `tests/check_hlsl_failure.cmake`
- Create: `tests/hlsl/profile/empty.cg`
- Create: `tests/hlsl/profile/empty-vs.expected`
- Create: `tests/hlsl/profile/empty-ps.expected`
- Modify: `CMakeLists.txt`
- Modify: `tests/CMakeLists.txt`
- Modify: `cgcmain.c:59-67`
- Modify: `errors.h:268-292`

- [ ] **Step 1: Write the failing registration fixtures**

Create `tests/hlsl/profile/empty.cg`:

```c
void main()
{
}
```

Create both expected files with the stage-specific target and minimal wrapper:

```hlsl
// profile hlslv
// target vs_3_0
void main()
{
}
```

```hlsl
// profile hlslf
// target ps_3_0
void main()
{
}
```

Create `tests/check_hlsl.cmake` by using the same required-variable,
configuration-directory, line-ending, version-comment, command-line-comment,
and end-marker normalization as `tests/check_glsl.cmake`. Change only the
failure message and file label from GLSL to HLSL. The runner must accept
`UPDATE_EXPECTED` but normal CTest registrations must never set it.

Create `tests/check_hlsl_failure.cmake` with required variables `CGC`,
`PROFILE`, `SOURCE`, `CODE`, `EXPECTED_LINE`, `MESSAGE`, and `ACTUAL`. It must
require nonzero compiler status, exactly one compiler diagnostic, the requested
code, source basename and line, and message regex. After removing compiler
version, command-line, and end comments, require the output file to be empty.

Add this exact registration helper to `tests/CMakeLists.txt`:

```cmake
function(add_hlsl_fixture name profile source expected)
    add_test(
        NAME ${name}
        COMMAND ${CMAKE_COMMAND}
            -DCGC=$<TARGET_FILE:cgc>
            -DPROFILE=${profile}
            -DSOURCE=${PROJECT_SOURCE_DIR}/${source}
            -DEXPECTED=${PROJECT_SOURCE_DIR}/${expected}
            "-DCONFIG=$<CONFIG>"
            "-DACTUAL=${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/${name}.hlsl"
            -P ${CMAKE_CURRENT_SOURCE_DIR}/check_hlsl.cmake)
endfunction()

add_hlsl_fixture(hlslv_registration hlslv
    tests/hlsl/profile/empty.cg tests/hlsl/profile/empty-vs.expected)
add_hlsl_fixture(hlslf_registration hlslf
    tests/hlsl/profile/empty.cg tests/hlsl/profile/empty-ps.expected)
```

- [ ] **Step 2: Run the registration tests and verify they fail**

Run:

```powershell
cmake -S . -B build-hlsl -DBUILD_TESTING=ON
cmake --build build-hlsl --config Debug
ctest --test-dir build-hlsl -C Debug -R '^hlsl(v|f)_registration$' --output-on-failure
```

Expected: both tests fail because `hlslv` and `hlslf` are unknown profiles.

- [ ] **Step 3: Define profile constants and stage descriptors**

In `hlsl_hal.h`, define:

```c
#ifndef __HLSL_IR_H
typedef enum HlslStage_Enum {
    HLSL_STAGE_VERTEX,
    HLSL_STAGE_PIXEL
} HlslStage;
#endif

#define VENDOR_STRING_HLSL "Microsoft"
#define VERSION_STRING_HLSL "DirectX 9.0c Shader Model 3"
#define PROFILE_HLSLV_NAME "hlslv"
#define PROFILE_HLSLV_ID 14
#define PROFILE_HLSLF_NAME "hlslf"
#define PROFILE_HLSLF_ID 15
#define CID_HLSLV_IN_ID 18
#define CID_HLSLV_OUT_ID 19
#define CID_HLSLF_IN_ID 20
#define CID_HLSLF_OUT_ID 21
#define HLSL_BUILTIN_GROUP 4

typedef struct HlslLimits_Rec {
    int inputs;
    int outputs;
    int floatConstants;
    int intConstants;
    int boolConstants;
    int samplers;
    int colorOutputs;
} HlslLimits;

struct HlslProfileDesc_Rec {
    HlslStage stage;
    const char *name;
    const char *target;
    int pid;
    int inputCid;
    int outputCid;
    ConnectorDescriptor *connectors;
    int numConnectors;
    ConnectorRegisters *inputRegs;
    int numInputRegs;
    ConnectorRegisters *outputRegs;
    int numOutputRegs;
    const HlslLimits *limits;
};

extern const HlslProfileDesc HlslProfile_hlslv;
extern const HlslProfileDesc HlslProfile_hlslf;
int RegisterProfiles_hlsl(void);
int InitHAL_hlslv(slHAL *hal);
int InitHAL_hlslf(slHAL *hal);
```

Reserve the diagnostic range in `errors.h` now so every later failing test can
name its final stable code:

```c
// Numbers 6400 to 6499 are reserved for the HLSL profile messages
#define ERROR_S_HLSL_UNSUPPORTED_TYPE       6400, "HLSL Shader Model 3 does not support type \"%s\""
#define ERROR_S_HLSL_UNSUPPORTED_OPERATION  6401, "HLSL Shader Model 3 does not support operation \"%s\""
#define ERROR_SS_HLSL_STAGE_OPERATION       6402, "%s profile does not support operation \"%s\""
#define ERROR_S_HLSL_SEMANTIC               6403, "HLSL profile cannot bind semantic \"%s\""
#define ERROR_S_HLSL_INTERFACE_CONFLICT     6404, "HLSL interface conflicts at semantic \"%s\""
#define ERROR___HLSL_REQUIRED_POSITION      6405, "HLSL vertex entry must write POSITION0"
#define ERROR_S_HLSL_ENTRY_ABI              6406, "HLSL entry interface cannot represent \"%s\""
#define ERROR_S_HLSL_REGISTER_COLLISION     6407, "HLSL register binding conflicts at \"%s\""
#define ERROR_SII_HLSL_RESOURCE_LIMIT       6408, "HLSL %s limit exceeded: %d used, %d available"
#define ERROR_S_HLSL_SAMPLER                6409, "HLSL Shader Model 3 does not support sampler feature \"%s\""
#define ERROR_S_HLSL_INTRINSIC              6410, "HLSL Shader Model 3 has no exact intrinsic for \"%s\""
#define ERROR_S_HLSL_NAME_COLLISION         6411, "HLSL name cannot be resolved for \"%s\""
#define ERROR___HLSL_INVALID_IR              9011, "invalid HLSL intermediate representation"
```

Later tasks progressively make each path reachable.

For the skeleton descriptors, use `{ 16, 12, 256, 16, 16, 4, 0 }` for vertex
and `{ 10, 5, 224, 16, 16, 16, 4 }` for pixel: the five pixel outputs are four
colors plus depth. Define two connector descriptors
per stage with the correct input/output IDs, zero register counts, and `NULL`
register pointers; do not use a zero-length C array. Task 3 replaces them with
complete tables.

- [ ] **Step 4: Add shared HAL initialization and a minimal generator**

In `hlsl_hal.c`, rely on `hal.c`'s already installed default size, alignment,
declarator, definition, statement, numeric, cast, and pragma-binding callbacks;
override only the HLSL capabilities, connector lookup, internal functions,
unbound bindings, semantic bindings, header, and generator. The orchestration
body must have this final shape from its first commit:

```c
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
        return 1;
    default:
        return 0;
    }
}
```

```c
static int GenerateCode_hlsl(SourceLoc *loc, Scope *scope, Symbol *program)
{
    HlslModule module;
    const HlslProfileDesc *profile;

    profile = (const HlslProfileDesc *) Cg->theHAL->localData;
    HlslInitModule(&module, profile->stage, HlslCompilerAlloc,
                   CurrentScope->pool);
    if (!HlslLowerProgram(&module, profile, loc, scope, program) ||
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
}
```

For Task 1 only, keep the five phase functions private in `hlsl_hal.c` and make
them accept an empty entry. `HlslWriteModule` prints the exact expected stage
text. Task 2 moves the module and phase interfaces into their permanent files.

`PrintCodeHeader_hlsl` returns true without writing text; this preserves the
transactional rule. Set `hal->comment = "//"`, `hal->vendor`, `hal->version`,
and `hal->localData = (void *) profile`.

- [ ] **Step 5: Register and compile the new files**

Add `int RegisterProfiles_hlsl(void);` and `RegisterProfiles_hlsl` after GLSL
in the `profileRegistrationFunctions` array in `cgcmain.c`. Add the three new C
files to the `cgc` target in `CMakeLists.txt`.

- [ ] **Step 6: Run registration and all existing profiles**

Run:

```powershell
cmake --build build-hlsl --config Debug --target cgc
ctest --test-dir build-hlsl -C Debug -R '^(hlsl(v|f)_registration|generic_|glsl|arb)' --output-on-failure
```

Expected: both new registration tests pass and all selected existing tests
pass.

- [ ] **Step 7: Commit the registration skeleton**

```powershell
git add -- CMakeLists.txt cgcmain.c errors.h hlsl_hal.h hlsl_hal.c hlslv_hal.c hlslf_hal.c tests/CMakeLists.txt tests/check_hlsl.cmake tests/check_hlsl_failure.cmake tests/hlsl/profile
git commit -m "Register DirectX 9 HLSL profiles"
```

### Task 2: Add the permanent typed IR, names, types, and writer unit target

**Files:**
- Create: `hlsl_ir.h`
- Create: `hlsl_ir.c`
- Create: `hlsl_codegen.c`
- Create: `tests/hlsl_ir_test.c`
- Modify: `hlsl_hal.h`
- Modify: `hlsl_hal.c`
- Modify: `CMakeLists.txt`
- Modify: `tests/CMakeLists.txt`

- [ ] **Step 1: Write failing IR, type, and name assertions**

Create `tests/hlsl_ir_test.c` with assertions for these exact contracts:

```c
#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "hlsl_ir.h"

static void *TestAlloc(void *arg, size_t size)
{
    (void) arg;
    return calloc(1, size);
}

int main(void)
{
    HlslModule module;
    HlslType scalar;
    HlslType vector;
    HlslType matrix;
    int firstIdentity;
    int secondIdentity;

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
    return 0;
}
```

Add a C90 `hlsl_ir_unit` executable from `hlsl_ir.c`, `hlsl_codegen.c`, and the
test file, plus an assertions-active test using
`tests/check_assertions_active.cmake`.

- [ ] **Step 2: Run the unit target and verify it fails to build**

Run:

```powershell
cmake -S . -B build-hlsl -DBUILD_TESTING=ON
cmake --build build-hlsl --config Debug --target hlsl_ir_unit
```

Expected: compilation fails because `hlsl_ir.h` and its types do not exist.

- [ ] **Step 3: Define the complete IR shape**

In `hlsl_ir.h`, define the stable enums above plus:

```c
typedef struct HlslLoc_Rec { int file; int line; } HlslLoc;
typedef struct HlslType_Rec HlslType;
typedef struct HlslName_Rec HlslName;
typedef struct HlslDecl_Rec HlslDecl;
typedef struct HlslExpr_Rec HlslExpr;
typedef struct HlslStmt_Rec HlslStmt;
typedef struct HlslFunction_Rec HlslFunction;
typedef struct HlslBinding_Rec HlslBinding;

struct HlslType_Rec {
    HlslBase base;
    int len;
    int rows;
    int cols;
    int arraySize;
    const char *structName;
    HlslType *elementType;
    HlslDecl *members;
};

typedef struct HlslPhysicalBinding_Rec {
    HlslRegisterBank bank;
    int regno;
    int span;
    int component;
} HlslPhysicalBinding;

struct HlslModule_Rec {
    HlslStage stage;
    HlslAllocFn alloc;
    void *allocArg;
    HlslName *names;
    HlslDecl *structs;
    HlslDecl *globals;
    HlslFunction *functions;
    HlslFunction *entry;
    HlslFunction *wrapper;
    HlslBinding *bindings;
    HlslLoc errorLoc;
    HlslErrorKind errorKind;
    const char *errorReason;
    const char *resourceName;
    int resourceUsed;
    int resourceAvailable;
    int errors;
};
```

Model the same expression and statement categories currently required by
`glsl_ir.h`, adding remainder, bitwise, shifts, increment/decrement, discard,
and explicit declaration statements. Define constructors and append functions
for every list type; do not expose raw allocation to later phases.

- [ ] **Step 4: Implement deterministic names and type utilities**

In `hlsl_ir.c`, use the identity-and-namespace allocation algorithm already
specified by the design. The reserved table must include all Cg/HLSL control
words and backend names, including `main`, `register`, `row_major`,
`column_major`, `sampler`, `technique`, `compile`, `texture`, and every scalar
or sampler type spelling. Prefix a reserved or `cg_` source spelling with
`cg_`; suffix collisions `_1`, `_2`, and so on.

`HlslTypeRegisterSpan` must return:

```c
if (type->arraySize > 0)
    return type->arraySize * HlslTypeRegisterSpan(type->elementType);
if (type->base == HLSL_BASE_STRUCT) {
    int span = 0;
    HlslDecl *member;
    for (member = type->members; member != NULL; member = member->next)
        span += HlslTypeRegisterSpan(&member->type);
    return span;
}
if (type->rows > 0 && type->cols > 0)
    return type->rows;
return 1;
```

Always spell Cg `half` and `fixed` IR values as HLSL `float`; preserve their
source base only in binding metadata.

- [ ] **Step 5: Move the skeleton to the permanent module API**

Move `HlslModule`, allocation, types, names, and `HlslWriteModule` out of Task
1's private definitions. Include `hlsl_ir.h` from `hlsl_hal.h`, compile
`hlsl_ir.c` and `hlsl_codegen.c`, and make the minimal writer traverse the
module entry instead of printing hard-coded text. Keep registration-only
implementations of lowering, wrapper construction, legalization, allocation,
and validation in `hlsl_hal.c`: they accept only an empty entry and report
`HLSL_ERROR_UNSUPPORTED_OPERATION` for any nonempty construct. Delete each
registration-only implementation in the task that introduces its permanent
module.

- [ ] **Step 6: Run unit, registration, and assertion tests**

```powershell
cmake --build build-hlsl --config Debug --target hlsl_ir_unit cgc
ctest --test-dir build-hlsl -C Debug -R '^(hlsl_ir_|hlsl(v|f)_registration)$' --output-on-failure
```

Expected: four tests pass: the two registrations, IR unit, and assertion seam.

- [ ] **Step 7: Commit the IR foundation**

```powershell
git add -- CMakeLists.txt hlsl_hal.h hlsl_hal.c hlsl_ir.h hlsl_ir.c hlsl_codegen.c tests/CMakeLists.txt tests/hlsl_ir_test.c
git commit -m "Add structured HLSL intermediate representation"
```

### Task 3: Implement DirectX 9 semantic tables and interface binding

**Files:**
- Modify: `hlsl_hal.h`
- Modify: `hlsl_hal.c`
- Modify: `hlslv_hal.c`
- Modify: `hlslf_hal.c`
- Modify: `tests/hlsl_ir_test.c`
- Create: `tests/hlsl/diagnostics/vp_attr_texcoord_conflict.cg`
- Modify: `tests/CMakeLists.txt`

- [ ] **Step 1: Add failing semantic canonicalization assertions**

Add assertions for these exact mappings:

```c
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
```

Run `hlsl_ir_unit`; expected: link or assertion failure because semantic
canonicalization is not implemented.

- [ ] **Step 2: Define semantic descriptor types**

Add to `hlsl_hal.h`:

```c
typedef enum HlslInterface_Enum {
    HLSL_INTERFACE_VARYING,
    HLSL_INTERFACE_POSITION,
    HLSL_INTERFACE_POINT_SIZE,
    HLSL_INTERFACE_PIXEL_POSITION,
    HLSL_INTERFACE_FACE,
    HLSL_INTERFACE_COLOR,
    HLSL_INTERFACE_DEPTH
} HlslInterface;

typedef struct HlslSemanticDesc_Rec {
    const char *root;
    int firstIndex;
    int count;
    int properties;
    int width;
    HlslInterface interfaceKind;
} HlslSemanticDesc;

typedef struct HlslSemanticAlias_Rec {
    const char *source;
    const char *target;
} HlslSemanticAlias;
```

Extend `HlslProfileDesc` with semantic and alias arrays and counts. Export
`HlslCanonicalSemantic` and a non-allocating semantic parser that splits a root
and optional decimal suffix.

- [ ] **Step 3: Fill complete stage tables**

In `hlslv_hal.c`, define vertex inputs for `POSITION0`, `BLENDWEIGHT0`,
`BLENDINDICES0`, `NORMAL0`, `PSIZE0`, `TEXCOORD0` through `TEXCOORD15`,
`TANGENT0`, `BINORMAL0`, and `COLOR0` through `COLOR1`. Define vertex outputs
for required `POSITION0`, `PSIZE0`, `FOG0`, `COLOR0` through `COLOR1`, and
`TEXCOORD0` through `TEXCOORD7`. Use aliases `HPOS`, `COL0`, `COL1`, `TEX0`
through `TEX15`, and `ATTR0` through `ATTR15` according to the approved design.

In `hlslf_hal.c`, define inputs `COLOR0` through `COLOR1`, `TEXCOORD0` through
`TEXCOORD7`, `FOG0`, `VPOS`, and `VFACE`; define outputs `COLOR0` through
`COLOR3` and `DEPTH0`. Add aliases `COLn`, `TEXn`, `WPOS`, and `FACE`.

Use `REG_WRITE_REQUIRED` on vertex `POSITION0`. Register every connector name
as an atom in `RegisterNames_hlsl`.

- [ ] **Step 4: Implement HAL semantic callbacks**

`BindVaryingSemantic_hlsl` must canonicalize before it mutates `Binding`. On
success, set `BK_CONNECTOR`, canonical `rname`, register number, base, width,
direction, varying, bound, hidden, and write-required flags from the descriptor.
On invalid stage, index, width, or duplicate canonical location, set the module-
independent HAL error record and emit one HLSL semantic diagnostic. Return zero
after emitting it so existing `semantic.c` error-count guards suppress generic
follow-up diagnostics.

`BindVaryingUnbound_hlsl` accepts no anonymous public interface member: emit
the HLSL semantic diagnostic naming the source variable.

- [ ] **Step 5: Add the alias-conflict fixture**

Create `vp_attr_texcoord_conflict.cg`:

```c
struct VertexIn {
    float4 generic : ATTR0;
    float4 explicitTexcoord : TEXCOORD0;
};
struct VertexOut { float4 position : POSITION; };
VertexOut main(VertexIn input)
{
    VertexOut output;
    output.position = input.generic + input.explicitTexcoord;
    return output;
}
```

Register it as diagnostic 6404 on the `explicitTexcoord` line. The conflict is
detected during front-end semantic binding, before the registration-only
lowerer sees the nonempty body. Successful all-semantics source fixtures are
added in Task 5 after permanent module lowering exists.

- [ ] **Step 6: Run semantic tests**

```powershell
cmake --build build-hlsl --config Debug --target hlsl_ir_unit cgc
ctest --test-dir build-hlsl -C Debug -R '^hlsl.*(semantic|interface|registration)' --output-on-failure
```

Expected: canonicalization assertions, the conflict diagnostic, and both
registrations pass.

- [ ] **Step 7: Commit stage interfaces**

```powershell
git add -- hlsl_hal.h hlsl_hal.c hlslv_hal.c hlslf_hal.c tests/CMakeLists.txt tests/hlsl_ir_test.c tests/hlsl/diagnostics/vp_attr_texcoord_conflict.cg
git commit -m "Bind DirectX 9 HLSL interfaces"
```

### Task 4: Add deterministic uniform, sampler, default, and aggregate allocation

**Files:**
- Create: `hlsl_bind.c`
- Modify: `hlsl_ir.h`
- Modify: `hlsl_ir.c`
- Modify: `hlsl_hal.c`
- Modify: `CMakeLists.txt`
- Modify: `tests/CMakeLists.txt`
- Modify: `tests/hlsl_ir_test.c`

- [ ] **Step 1: Write failing allocator unit tests**

Construct bindings directly in `hlsl_ir_test.c` and assert:

```c
assert(HlslAllocateOneBinding(&module, &profile, &float4Binding));
assert(float4Binding.physical.bank == HLSL_REGISTER_C);
assert(float4Binding.physical.regno == 0);
assert(float4Binding.physical.span == 1);
assert(HlslAllocateOneBinding(&module, &profile, &matrixBinding));
assert(matrixBinding.physical.regno == 1);
assert(matrixBinding.physical.span == 4);
assert(HlslAllocateOneBinding(&module, &profile, &intBinding));
assert(intBinding.physical.bank == HLSL_REGISTER_I);
assert(intBinding.physical.regno == 0);
assert(HlslAllocateOneBinding(&module, &profile, &boolBinding));
assert(boolBinding.physical.bank == HLSL_REGISTER_B);
assert(boolBinding.physical.regno == 0);
assert(HlslAllocateOneBinding(&module, &profile, &samplerBinding));
assert(samplerBinding.physical.bank == HLSL_REGISTER_S);
assert(samplerBinding.physical.regno == 0);
assert(!HlslAllocateOneBinding(&module, &profile, &collisionBinding));
assert(module.errorKind == HLSL_ERROR_REGISTER_COLLISION);
```

Run the unit target; expected: missing allocator symbols.

- [ ] **Step 2: Implement independent bank bitmaps**

In `hlsl_bind.c`, represent each bank as a fixed-size byte array using the
active profile limits. Explicit bindings reserve their complete recursive span
first. Implicit bindings scan from zero for the first contiguous free span.
Allocation order is source ordinal, then recursive member order.

Move `HlslAllocateBindings` from its registration-only implementation in
`hlsl_hal.c` into this file and delete the old definition.

Use this bank selection rule:

```c
switch (type->base) {
case HLSL_BASE_INT:  return HLSL_REGISTER_I;
case HLSL_BASE_BOOL: return HLSL_REGISTER_B;
case HLSL_BASE_SAMPLER1D:
case HLSL_BASE_SAMPLER2D:
case HLSL_BASE_SAMPLER3D:
case HLSL_BASE_SAMPLERCUBE:
    return HLSL_REGISTER_S;
default:
    return HLSL_REGISTER_C;
}
```

Mixed structs recurse into leaves. Homogeneous arrays reserve
`arraySize * elementSpan` contiguous registers. Matrices reserve one `c`
register per row. A collision records the public binding name and the first
colliding register; exhaustion records the bank name, used span, and limit.

- [ ] **Step 3: Connect HAL uniform binding to late allocation**

`BindUniformUnbound_hlsl` must mark the front-end binding bound and uniform but
must not choose a physical register. Preserve explicit register, semantic,
default, and source-ordinal information in `HlslBinding` during lowering.
`HlslAllocateBindings` creates generated globals, assigns annotations, and
produces one metadata record per public leaf.

Default values must remain in source type order. Emit both an initializer when
the declaration is legal and a metadata record with the exact flattened
values. Do not mark a defaulted uniform `static const`, because that would
remove its public register contract.

- [ ] **Step 4: Complete direct-IR binding coverage**

Extend `hlsl_ir_test.c` with a homogeneous float array of length three, a
row-major `float4x4`, a structure containing float/int/Boolean leaves, explicit
sampler unit 3, and an implicit sampler. Assert contiguous `c` spans, separate
`i` and `b` banks, `s3`, first-free `s0`, recursive public paths, and collision
location. Source-level binding fixtures are added in Task 5 after permanent
lowering can populate `HlslBinding` records.

- [ ] **Step 5: Run allocator and binding tests**

```powershell
cmake --build build-hlsl --config Debug --target hlsl_ir_unit cgc
ctest --test-dir build-hlsl -C Debug -R '^hlsl.*(binding|register|sampler)' --output-on-failure
```

Expected: all direct-IR bank, aggregate, sampler, and collision assertions pass.

- [ ] **Step 6: Commit deterministic bindings**

```powershell
git add -- CMakeLists.txt hlsl_bind.c hlsl_ir.h hlsl_ir.c hlsl_hal.c tests/CMakeLists.txt tests/hlsl_ir_test.c
git commit -m "Allocate HLSL constant and sampler registers"
```

### Task 5: Lower modules, build the public wrapper, and emit simple shaders

**Files:**
- Create: `hlsl_lower.c`
- Create: `hlsl_legalize.c`
- Create: `hlsl_validate.c`
- Modify: `hlsl_bind.c`
- Modify: `hlsl_codegen.c`
- Modify: `hlsl_hal.c`
- Modify: `CMakeLists.txt`
- Modify: `tests/CMakeLists.txt`
- Create: `tests/hlsl/profile/vp_passthrough.cg`
- Create: `tests/hlsl/profile/vp_passthrough.expected`
- Create: `tests/hlsl/profile/fp_passthrough.cg`
- Create: `tests/hlsl/profile/fp_passthrough.expected`
- Create: `tests/hlsl/profile/vp_out_inout.cg`
- Create: `tests/hlsl/profile/vp_out_inout.expected`
- Create: `tests/hlsl/interfaces/vp_all_semantics.cg`
- Create: `tests/hlsl/interfaces/vp_all_semantics.expected`
- Create: `tests/hlsl/interfaces/fp_all_semantics.cg`
- Create: `tests/hlsl/interfaces/fp_all_semantics.expected`
- Create: `tests/hlsl/bindings/vp_register_banks.cg`
- Create: `tests/hlsl/bindings/vp_register_banks.expected`
- Create: `tests/hlsl/bindings/vp_mixed_struct.cg`
- Create: `tests/hlsl/bindings/vp_mixed_struct.expected`
- Create: `tests/hlsl/bindings/fp_samplers.cg`
- Create: `tests/hlsl/bindings/fp_samplers.expected`
- Create: `tests/hlsl/diagnostics/vp_register_collision.cg`

- [ ] **Step 1: Add failing wrapper fixtures**

Use this vertex fixture:

```c
struct VertexOut { float4 position : POSITION; };
VertexOut main(float4 position : POSITION, uniform float scale)
{
    VertexOut output;
    output.position = position * scale;
    return output;
}
```

The golden must contain a bound global, internal entry, and wrapper in this
shape:

```hlsl
float cg_scale : register(c0);
struct cg_VertexIn { float4 position : POSITION0; };
struct cg_VertexOut { float4 position : POSITION0; };
cg_VertexOut cg_entry(float4 position, float scale);
cg_VertexOut main(cg_VertexIn input)
{
    return cg_entry(input.position, cg_scale);
}
```

The pixel passthrough returns `COLOR0`. The `vp_out_inout` fixture combines a
returned position with varying `out` and `inout` parameters and checks that the
wrapper calls `cg_entry` exactly once and copies all results afterward.

- [ ] **Step 2: Run fixtures and verify lowering fails**

Run the three tests. Expected: registration still passes, but each non-empty
fixture fails because the skeleton cannot lower parameters, fields, or returns.

- [ ] **Step 3: Implement symbol, type, declaration, and function lowering**

In `hlsl_lower.c`, add identity maps from `Type *` and `Symbol *` to IR types
and declarations. Lower structures before any declaration that references
them. Lower reachable helpers in dependency order and mark a prototype only
for a forward call cycle that is not recursive. Detect recursion during the
visit-state walk and report entry/operation failure rather than emitting it.

Move `HlslLowerProgram` from its registration-only definition in `hlsl_hal.c`
into this file and delete the old definition.

Lower the selected program into an internal function named through the
identity allocator. Record uniform parameters as bindings and varying
parameters as raw interface fields; do not emit the public wrapper here.

Create `hlsl_legalize.c`, move `HlslLegalizeModule` out of `hlsl_hal.c`, and
implement identity legalization plus exact checks for the declarations,
field/index access, scalar/vector multiply, calls, assignments, and returns
used by this task. Create `hlsl_validate.c`, move `HlslValidateModule` out of
`hlsl_hal.c`, and validate the module ownership, wrapper call count, interface
uniqueness, required position, and bindings used by these fixtures. Later tasks
extend these permanent phases; no registration-only phase remains after this
task.

- [ ] **Step 4: Build the wrapper without duplicating evaluation**

Move `HlslBuildEntryWrapper` from `hlsl_hal.c` into `hlsl_bind.c` and delete the
registration-only definition. It must create one input structure, one output structure,
and one public `main`. For each parameter:

- `in` varying: read the canonical input field;
- `uniform`: read the generated bound global;
- `out`: create a typed local and pass it once;
- `inout`: initialize one typed local from the input field, pass it once, then
  copy it to the output field;
- returned semantic value or structure: store the call result once before
  copying its fields.

The wrapper must contain exactly one call expression to the internal entry.

- [ ] **Step 5: Emit deterministic declarations and functions**

In `hlsl_codegen.c`, emit sections in the approved order. Use four spaces,
brace-on-next-line functions, one declaration per line, `row_major` on every
matrix, and lowercase Boolean literals. Emit public metadata before globals.
Never write a section until `HlslValidateModule` has succeeded.

Add the source-level interface and binding fixtures deferred from Tasks 3 and
4. `vp_all_semantics.cg` declares every vertex input family and returns
position, point size, fog, two colors, and eight texture coordinates.
`fp_all_semantics.cg` consumes both colors, eight texture coordinates, fog,
`WPOS`, and `FACE`, and returns four colors plus depth. Their goldens use only
canonical DirectX 9 semantics.

`vp_register_banks.cg` contains `uniform float4 color`, `uniform float4x4
matrix`, `uniform int4 indices`, and `uniform bool enabled`; its golden requires
`c0`, `c1-c4`, `i0`, and `b0`. `fp_samplers.cg` contains explicit `TEXUNIT3`
and one unbound sampler; require `s3` and `s0`. `vp_mixed_struct.cg` contains a
uniform structure with float, int, Boolean, matrix, and float-array leaves and
requires recursive metadata. `vp_register_collision.cg` binds two uniforms to
`C0` and expects 6407 at the second declaration.

- [ ] **Step 6: Run wrapper, binding, and existing regression tests**

```powershell
cmake --build build-hlsl --config Debug --target cgc
ctest --test-dir build-hlsl -C Debug -R '^(hlsl|generic_|glsl|arb)' --output-on-failure
```

Expected: all HLSL profile/wrapper/binding tests and every existing selected
test pass.

- [ ] **Step 7: Commit module lowering and wrapper emission**

```powershell
git add -- CMakeLists.txt hlsl_lower.c hlsl_legalize.c hlsl_validate.c hlsl_bind.c hlsl_codegen.c hlsl_hal.c tests/CMakeLists.txt tests/hlsl/profile tests/hlsl/interfaces tests/hlsl/bindings tests/hlsl/diagnostics/vp_register_collision.cg
git commit -m "Emit HLSL entry wrappers"
```

### Task 6: Lower expressions, statements, helpers, overloads, and control flow

**Files:**
- Modify: `hlsl_ir.h`
- Modify: `hlsl_ir.c`
- Modify: `hlsl_lower.c`
- Modify: `hlsl_legalize.c`
- Modify: `hlsl_codegen.c`
- Modify: `CMakeLists.txt`
- Modify: `tests/CMakeLists.txt`
- Create: `tests/hlsl/expressions/vp_arithmetic.cg`
- Create: `tests/hlsl/expressions/vp_side_effects.cg`
- Create: `tests/hlsl/control/vp_control.cg`
- Create: `tests/hlsl/control/fp_discard.cg`
- Create: `tests/hlsl/functions/vp_helpers.cg`
- Create: `tests/hlsl/diagnostics/vp_recursion.cg`
- Create: matching `.expected` files for successful fixtures

- [ ] **Step 1: Add failing language fixtures**

`vp_arithmetic.cg` must cover unary plus/minus, add, subtract, multiply,
divide, remainder, logical operators, scalar comparisons, vector comparisons,
casts, constructors, indexing, member selection, and swizzles.

`vp_side_effects.cg` must use post-increment in both sides of a comma or
conditional expression and prove from the golden that every side effect occurs
once in Cg order. Cg conditional expressions with vector Boolean conditions
must evaluate both value operands before component selection, matching the Cg
language rule.

`vp_control.cg` must cover `if/else`, `for`, `while`, `do`, `break`, `continue`,
and early return. `fp_discard.cg` must discard conditionally and return
`COLOR0`. `vp_helpers.cg` must contain two overloads and an `out` helper
parameter. `vp_recursion.cg` must contain direct recursion and expect 6401.

- [ ] **Step 2: Run the new fixtures and verify representative failures**

Expected: missing expression/statement lowering or unsupported-operation
failures; no successful fixture may accidentally match a golden.

- [ ] **Step 3: Implement expression and statement lowering**

Lower pure HLSL-equivalent expressions directly. For comma, assignment-valued
expressions, increment/decrement, aggregate operands, and Cg conditional
selection, introduce typed temporaries and prepend explicit statements to the
current statement list. Never clone an impure source subtree.

Preserve structured loops and jumps. Track loop depth during lowering so common
front-end diagnostics remain authoritative for out-of-loop jumps. Lower pixel
discard to `HLSL_STMT_DISCARD`; return 6402 for vertex discard.

- [ ] **Step 4: Implement helper reachability and overload emission**

Use source symbol identity as the call target. Emit overloaded HLSL helpers
with sanitized names and their complete parameter types. Generate prototypes
for forward references. A gray visit-state edge is recursion and must report
6401 at the call expression; a completed helper can be called repeatedly.

- [ ] **Step 5: Legalize vector conditions and Cg conditional evaluation**

In `hlsl_legalize.c`, convert vector comparisons and Boolean-vector selection
component-wise. Capture both second and third operands of `?:` before selecting
when either can have side effects. Keep scalar HLSL `?:` only when both branches
are pure. Emit explicit casts at int/float and bool/numeric boundaries.
Extend `HlslLegalizeModule` with the new expression and control-flow forms.

- [ ] **Step 6: Run focused and regression tests**

```powershell
cmake --build build-hlsl --config Debug --target cgc
ctest --test-dir build-hlsl -C Debug -R '^hlsl.*(arithmetic|side_effect|control|discard|helper|recursion)' --output-on-failure
ctest --test-dir build-hlsl -C Debug -R '^(generic_|glsl|arb)' --output-on-failure
```

Expected: every focused fixture and all existing regressions pass.

- [ ] **Step 7: Commit core language lowering**

```powershell
git add -- CMakeLists.txt hlsl_ir.h hlsl_ir.c hlsl_lower.c hlsl_legalize.c hlsl_codegen.c tests/CMakeLists.txt tests/hlsl/expressions tests/hlsl/control tests/hlsl/functions tests/hlsl/diagnostics/vp_recursion.cg
git commit -m "Translate structured Cg into HLSL"
```

### Task 7: Preserve aggregates, arrays, rectangular matrices, and defaults

**Files:**
- Modify: `hlsl_lower.c`
- Modify: `hlsl_legalize.c`
- Modify: `hlsl_bind.c`
- Modify: `hlsl_codegen.c`
- Modify: `tests/CMakeLists.txt`
- Create: `tests/hlsl/aggregates/vp_struct_array.cg`
- Create: `tests/hlsl/aggregates/vp_nested_out.cg`
- Create: `tests/hlsl/matrices/vp_rectangular.cg`
- Create: `tests/hlsl/matrices/vp_matrix_order.cg`
- Create: `tests/hlsl/bindings/vp_defaults.cg`
- Create: matching `.expected` files
- Create: `tests/hlsl/vertex/position.expected`
- Create: `tests/hlsl/vertex/reflection.expected`
- Create: `tests/hlsl/vertex/vertexlight.expected`
- Create: `tests/hlsl/vertex/vertexlight4.expected`

- [ ] **Step 1: Add failing aggregate and matrix fixtures**

`vp_struct_array.cg` must assign nested structures and arrays while using an
index expression with a side effect. `vp_nested_out.cg` must pass a nested
aggregate through `out` and `inout` helper parameters. `vp_rectangular.cg`
must cover `float2x3`, `float3x2`, row selection, scalar construction,
matrix-vector multiplication, and vector-matrix multiplication.

`vp_matrix_order.cg` must bind a `float4x4` named `transform` at `c4`, multiply
it by a known input, and require
`row_major float4x4 cg_transform : register(c4)` in the golden.
`vp_defaults.cg` must contain scalar, vector, matrix, array, and structure
defaults and check both initializer spelling and flattened `// cgc-default`
records.

- [ ] **Step 2: Run focused fixtures and verify they fail**

Expected: missing aggregate/matrix legalization, not parser errors. If the
front end rejects a fixture, replace its spelling with the already accepted
form used by `tests/glsl/expressions/vp_struct_array_assignment.cg` or the
existing `vp_matrix_*` fixtures while keeping the same aggregate or matrix
behavior under test.

- [ ] **Step 3: Implement native aggregate lowering with fallback copies**

Keep native structure and fixed-array declarations. When HLSL cannot accept an
aggregate assignment or constructor in the required position, synthesize a
destination local and recursively emit member or element assignments. Capture
every impure base and index before recursion. For helper `out` and `inout`, copy
in once, call once, and copy out once.

- [ ] **Step 4: Implement row-major matrix behavior**

Map `TYPErowsxcols` to `row_major floatrowsxcols`. Treat `m[i]` as Cg row
selection. Preserve argument order for `mul(matrix, vector)` and
`mul(vector, matrix)`. Lower matrix constructors in row order and reserve one
`c` register per row. Do not depend on `/Zpr` or `/Zpc`.

- [ ] **Step 5: Emit defaults and mixed aggregate reconstruction**

Emit legal homogeneous defaults directly. For mixed-bank structures, emit
leaf globals and construct one logical local at the first use in the internal
entry. Metadata paths use `public.member[index]` and physical bank/register
locations. Preserve integer and Boolean values without float conversion.

- [ ] **Step 6: Run aggregates, matrices, bindings, and four bundled shaders**

Register `position.cg`, `reflection.cg`, `vertexlight.cg`, and
`vertexlight4.cg` as exact `hlslv` golden fixtures. Run:

```powershell
cmake --build build-hlsl --config Debug --target cgc
ctest --test-dir build-hlsl -C Debug -R '^hlslv_(position|reflection|vertexlight|struct|nested|rectangular|matrix|defaults)' --output-on-failure
```

Expected: all focused fixtures and all four bundled conversions pass.

- [ ] **Step 7: Commit aggregate and matrix compatibility**

```powershell
git add -- hlsl_lower.c hlsl_legalize.c hlsl_bind.c hlsl_codegen.c tests/CMakeLists.txt tests/hlsl/aggregates tests/hlsl/matrices tests/hlsl/bindings/vp_defaults.cg tests/hlsl/bindings/vp_defaults.expected tests/hlsl/vertex
git commit -m "Preserve HLSL aggregates and matrices"
```

### Task 8: Complete numeric, geometric, comparison, and derivative intrinsics

**Files:**
- Modify: `hlsl_ir.h`
- Modify: `hlsl_ir.c`
- Modify: `hlsl_hal.c`
- Modify: `hlsl_legalize.c`
- Modify: `stdlib.cg`
- Modify: `stdlib.c`
- Modify: `tests/hlsl_ir_test.c`
- Modify: `tests/CMakeLists.txt`
- Create: `tests/hlsl/intrinsics/vp_numeric.cg`
- Create: `tests/hlsl/intrinsics/vp_geometric.cg`
- Create: `tests/hlsl/intrinsics/fp_derivatives.cg`
- Create: `tests/hlsl/diagnostics/vp_derivative.cg`
- Create: `tests/hlsl/diagnostics/vp_bad_intrinsic.cg`
- Create: matching `.expected` files for successful fixtures

- [ ] **Step 1: Add failing signature lookup assertions**

Assert exact mappings for `mul`, `dot`, `cross`, `normalize`, `reflect`,
`refract`, `length`, `distance`, `min`, `max`, `clamp`, `abs`, `sign`, `floor`,
`ceil`, `round`, `trunc`, `sqrt`, `rsqrt`, `pow`, `exp`, `exp2`, `log`, `log2`,
`sin`, `cos`, `tan`, `asin`, `acos`, `atan`, `atan2`, `sinh`, `cosh`, `tanh`,
`lerp`, `frac`, `fmod`, `saturate`, `step`, `smoothstep`, `any`, `all`, `ddx`,
and `ddy`. Assert `ddx` is stage-invalid for vertex and an unknown overload
returns `HLSL_BUILTIN_NONE`.

- [ ] **Step 2: Run the unit test and verify missing mappings fail**

Expected: assertions fail at the first unimplemented mapping.

- [ ] **Step 3: Implement a signature-driven builtin table**

Each table row contains source spelling, parameter count, allowed base/width
pattern, result pattern, stage mask, HLSL spelling, and lowering kind: native,
helper, or expansion. `CheckInternalFunction_hlsl` resolves the complete
function type and returns group 4 plus a stable builtin enum only for an exact
row.

Use generated helpers for functions whose exact Cg behavior differs. Examples:

```hlsl
float cg_rsqrt(float value)
{
    return rsqrt(value);
}

float cg_saturate(float value)
{
    return min(max(value, 0.0), 1.0);
}
```

Generate vector overloads only when used. Never approximate a missing overload.

- [ ] **Step 4: Synchronize standard-library internal markers**

Add or adjust only the intrinsic declarations that must be internal for HLSL,
then run:

```powershell
cmake --build build-hlsl --config Debug --target regenerate_stdlib
git diff -- stdlib.cg stdlib.c
```

Expected: `stdlib.c` changes only as the deterministic tokenized form of the
reviewed `stdlib.cg` changes.

- [ ] **Step 5: Add exact intrinsic and stage diagnostic fixtures**

The numeric and geometric fixtures must exercise every table family at scalar
and representative vector widths. The pixel derivative fixture uses both
`ddx` and `ddy`. The vertex derivative fixture expects 6402. The bad-overload
fixture expects 6410 and names the intrinsic signature.

- [ ] **Step 6: Run intrinsics and stdlib synchronization tests**

```powershell
cmake --build build-hlsl --config Debug --target cgc hlsl_ir_unit
ctest --test-dir build-hlsl -C Debug -R '^hlsl.*intrinsic|^hlsl.*derivative|stdlib' --output-on-failure
cmake --build build-hlsl --config Debug --target regenerate_stdlib
git diff --exit-code -- stdlib.c
```

Expected: all intrinsic tests pass and a second regeneration produces no diff.

- [ ] **Step 7: Commit intrinsic coverage**

```powershell
git add -- hlsl_ir.h hlsl_ir.c hlsl_hal.c hlsl_legalize.c stdlib.cg stdlib.c tests/CMakeLists.txt tests/hlsl_ir_test.c tests/hlsl/intrinsics tests/hlsl/diagnostics/vp_derivative.cg tests/hlsl/diagnostics/vp_bad_intrinsic.cg
git commit -m "Translate HLSL Shader Model 3 intrinsics"
```

### Task 9: Add complete legal Shader Model 3 texture operations

**Files:**
- Modify: `hlsl_hal.h`
- Modify: `hlsl_hal.c`
- Modify: `hlsl_legalize.c`
- Modify: `hlsl_validate.c`
- Modify: `tests/CMakeLists.txt`
- Create: `tests/hlsl/textures/fp_tex1d.cg`
- Create: `tests/hlsl/textures/fp_tex2d.cg`
- Create: `tests/hlsl/textures/fp_tex3d.cg`
- Create: `tests/hlsl/textures/fp_texcube.cg`
- Create: `tests/hlsl/textures/fp_project_bias_lod_grad.cg`
- Create: `tests/hlsl/textures/vp_tex2dlod.cg`
- Create: `tests/hlsl/diagnostics/vp_implicit_texture.cg`
- Create: `tests/hlsl/diagnostics/fp_sampler_mismatch.cg`
- Create: `tests/hlsl/diagnostics/fp_sampler_array.cg`
- Create: matching `.expected` files for successful fixtures

- [ ] **Step 1: Add failing texture fixtures**

Use one sampler and one exact call per base-dimension fixture. The combined
pixel fixture must cover projected, bias, explicit LOD, and gradient forms with
the correct coordinate widths. The vertex fixture must use only `tex2Dlod` and
a four-component coordinate. Negative fixtures expect 6409 for implicit vertex
sampling, sampler/coordinate dimension mismatch, and sampler arrays.

- [ ] **Step 2: Run texture fixtures and verify they fail**

Expected: unrecognized builtin or sampler diagnostics; no partial shader body.

- [ ] **Step 3: Add exact texture builtin signatures**

Add table rows for every texture function exposed by the current standard
library. Each row declares sampler base, coordinate width, optional projection,
bias, LOD, or gradient arguments, allowed stages, and emitted HLSL spelling.
Pixel implicit forms are legal; vertex forms require explicit LOD. Reject
shadow/comparison, rectangle, sampler arrays, local samplers, sampler return
values, and sampler assignment.

- [ ] **Step 4: Validate sampler declarations and calls before emission**

Confirm physical `s#` range, unique unit/type agreement, coordinate width,
stage, and texture form. A single sampler may be used repeatedly with one
dimension. Two public names may not claim one unit with incompatible sampler
dimensions. Record the nearest sampler or call location in 6409.

Extend `HlslValidateModule` with the sampler and texture stage checks needed by
this task. Task 10 extends the same validator with all remaining IR invariants
and resource categories.

- [ ] **Step 5: Run all texture, binding, and stage tests**

```powershell
cmake --build build-hlsl --config Debug --target cgc
ctest --test-dir build-hlsl -C Debug -R '^hlsl.*(tex|sampler)' --output-on-failure
```

Expected: all six successful texture cases and all sampler diagnostics pass.

- [ ] **Step 6: Commit texture support**

```powershell
git add -- hlsl_hal.h hlsl_hal.c hlsl_legalize.c hlsl_validate.c tests/CMakeLists.txt tests/hlsl/textures tests/hlsl/diagnostics/vp_implicit_texture.cg tests/hlsl/diagnostics/fp_sampler_mismatch.cg tests/hlsl/diagnostics/fp_sampler_array.cg
git commit -m "Translate HLSL Shader Model 3 textures"
```

### Task 10: Complete diagnostics, validation, limits, and transactional output

**Files:**
- Modify: `hlsl_validate.c`
- Modify: `hlsl_hal.c`
- Modify: `hlsl_bind.c`
- Modify: `hlsl_legalize.c`
- Modify: `tests/check_hlsl_failure.cmake`
- Modify: `tests/hlsl_ir_test.c`
- Modify: `tests/CMakeLists.txt`
- Create: `tests/hlsl/limits/vp_inputs16.cg`
- Create: `tests/hlsl/limits/vp_inputs17.cg`
- Create: `tests/hlsl/limits/vp_outputs12.cg`
- Create: `tests/hlsl/limits/vp_outputs13.cg`
- Create: `tests/hlsl/limits/fp_inputs10.cg`
- Create: `tests/hlsl/limits/fp_inputs11.cg`
- Create: `tests/hlsl/limits/vp_constants256.cg`
- Create: `tests/hlsl/limits/vp_constants257.cg`
- Create: `tests/hlsl/limits/fp_constants224.cg`
- Create: `tests/hlsl/limits/fp_constants225.cg`
- Create: `tests/hlsl/limits/vp_int16.cg`
- Create: `tests/hlsl/limits/vp_int17.cg`
- Create: `tests/hlsl/limits/vp_bool16.cg`
- Create: `tests/hlsl/limits/vp_bool17.cg`
- Create: `tests/hlsl/limits/vp_samplers4.cg`
- Create: `tests/hlsl/limits/vp_samplers5.cg`
- Create: `tests/hlsl/limits/fp_samplers16.cg`
- Create: `tests/hlsl/limits/fp_samplers17.cg`
- Create: `tests/hlsl/limits/fp_colors4.cg`
- Create: `tests/hlsl/limits/fp_colors5.cg`

- [ ] **Step 1: Audit the exact diagnostic range and enum mapping**

Confirm `errors.h` still contains the Task 1 definitions exactly:

```c
// Numbers 6400 to 6499 are reserved for the HLSL profile messages
#define ERROR_S_HLSL_UNSUPPORTED_TYPE       6400, "HLSL Shader Model 3 does not support type \"%s\""
#define ERROR_S_HLSL_UNSUPPORTED_OPERATION  6401, "HLSL Shader Model 3 does not support operation \"%s\""
#define ERROR_SS_HLSL_STAGE_OPERATION       6402, "%s profile does not support operation \"%s\""
#define ERROR_S_HLSL_SEMANTIC               6403, "HLSL profile cannot bind semantic \"%s\""
#define ERROR_S_HLSL_INTERFACE_CONFLICT     6404, "HLSL interface conflicts at semantic \"%s\""
#define ERROR___HLSL_REQUIRED_POSITION      6405, "HLSL vertex entry must write POSITION0"
#define ERROR_S_HLSL_ENTRY_ABI              6406, "HLSL entry interface cannot represent \"%s\""
#define ERROR_S_HLSL_REGISTER_COLLISION     6407, "HLSL register binding conflicts at \"%s\""
#define ERROR_SII_HLSL_RESOURCE_LIMIT       6408, "HLSL %s limit exceeded: %d used, %d available"
#define ERROR_S_HLSL_SAMPLER                6409, "HLSL Shader Model 3 does not support sampler feature \"%s\""
#define ERROR_S_HLSL_INTRINSIC              6410, "HLSL Shader Model 3 has no exact intrinsic for \"%s\""
#define ERROR_S_HLSL_NAME_COLLISION         6411, "HLSL name cannot be resolved for \"%s\""
#define ERROR___HLSL_INVALID_IR              9011, "invalid HLSL intermediate representation"
```

Add a compile-time or unit assertion table that maps every `HlslErrorKind` to
the corresponding code and verifies invalid IR maps only to 9011. This task
must not renumber a code already used by an earlier fixture.

- [ ] **Step 2: Make the failure runner require one exact diagnostic**

`tests/check_hlsl_failure.cmake` must require `CGC`, `PROFILE`, `SOURCE`,
`CODE`, `EXPECTED_LINE`, `MESSAGE`, and `ACTUAL`; require nonzero exit; require
exactly one `error C####:`; require the expected source basename and line;
require the message regex; strip ordinary version/command/end comments; and
require the remaining file to be empty. Unlike GLSL, HLSL `PrintCodeHeader`
writes nothing before validation.

- [ ] **Step 3: Implement both IR validation passes**

The structural pass verifies non-null required child pointers, legal types,
declaration ownership, function result/return agreement, call arity, expression
type agreement, loop-only jumps, and one wrapper call. The target pass verifies
stage operations, canonical unique semantics, required `POSITION0`, binding
bank/type agreement, register spans, sampler calls, and the active limits.

On failure, set exactly one module error with `HlslFail`:

```c
int HlslFail(HlslModule *module, HlslErrorKind kind,
             const HlslLoc *loc, const char *reason)
{
    if (module->errors == 0) {
        module->errorKind = kind;
        module->errorReason = reason;
        if (loc != NULL)
            module->errorLoc = *loc;
    }
    module->errors++;
    return 0;
}
```

`ReportHlslFailure` maps every enum to the exact `errors.h` code. It must not
emit 9011 for user input; invalid IR is an internal compiler error path.

- [ ] **Step 4: Add every exact boundary and one-over fixture**

Use generated declarations with a final expression that reads every resource,
preventing an external compiler from optimizing the declarations away during
validation. Register the boundary file as a success and the one-over file as
6408 with exact `used` and `available` numbers. Cover every file listed in this
task and pixel color outputs 4/5.

- [ ] **Step 5: Verify transactional cleanup and one diagnostic**

For unsupported type, operation, stage operation, semantic, interface
conflict, missing position, ABI conflict, register collision, each resource,
sampler, intrinsic, and name failure, register one exact diagnostic fixture.
After running them, inspect at least one produced `.hlsl` per category and
confirm it contains no metadata or HLSL declaration.

- [ ] **Step 6: Run all HLSL tests and existing regressions**

```powershell
cmake --build build-hlsl --config Debug --target cgc hlsl_ir_unit
ctest --test-dir build-hlsl -C Debug -R '^hlsl' --output-on-failure
ctest --test-dir build-hlsl -C Debug -R '^(generic_|glsl|arb)' --output-on-failure
```

Expected: every HLSL unit, success, boundary, and diagnostic test passes; all
existing profiles remain green.

- [ ] **Step 7: Commit diagnostics and validation**

```powershell
git add -- hlsl_validate.c hlsl_hal.c hlsl_bind.c hlsl_legalize.c tests/check_hlsl_failure.cmake tests/hlsl_ir_test.c tests/CMakeLists.txt tests/hlsl/limits tests/hlsl/diagnostics
git commit -m "Validate HLSL Shader Model 3 constraints"
```

### Task 11: Add cross-stage checks, optional `fxc`, and optional Cg oracle

**Files:**
- Create: `tests/check_hlsl_link.cmake`
- Create: `tests/validate_hlsl.cmake`
- Create: `tests/check_hlsl_oracle.cmake`
- Create: `tests/hlsl/link/vp_link.cg`
- Create: `tests/hlsl/link/fp_link.cg`
- Create: `tests/hlsl/oracle/vp_uniform.cg`
- Create: `tests/hlsl/oracle/fp_texture.cg`
- Modify: `tests/CMakeLists.txt`

- [ ] **Step 1: Add a matching linked pair**

The vertex fixture returns `POSITION0`, `COLOR0`, and `TEXCOORD0`; the pixel
fixture consumes `COLOR0` and `TEXCOORD0` and returns `COLOR0`. The link runner
compiles both, extracts normalized `// cgc-bind interface` records, sorts them,
removes vertex `POSITION0` and pixel result records, and requires exact key,
type, width, and semantic equality.

Register `hlsl_link_interface`. Run it before implementing the runner;
expected: CTest fails because the script is absent.

- [ ] **Step 2: Implement the cross-stage runner**

Require `CGC`, both sources, and both outputs. Compile with `hlslv` and
`hlslf`; fail on either diagnostic. Extract records matching:

```text
// cgc-bind interface (in|out) <public> <type> <semantic>
```

Compare vertex `out` against pixel `in`. Add a second deliberately mismatched
pair and require the runner to fail under `WILL_FAIL TRUE` with `COLOR0 type
mismatch` in its diagnostic output.

- [ ] **Step 3: Add optional `fxc` discovery and runner**

At the top of `tests/CMakeLists.txt`:

```cmake
find_program(FXC_EXECUTABLE NAMES fxc fxc.exe)
if(NOT FXC_EXECUTABLE)
    message(STATUS "fxc not found; HLSL SM3 validation tests are disabled")
endif()
```

`validate_hlsl.cmake` compiles the Cg source, then runs:

```cmake
execute_process(
    COMMAND "${FXC}" /nologo /Ges /WX /T "${TARGET}" /E main
        /Fo "${BYTECODE}" "${OUTPUT}"
    RESULT_VARIABLE fxc_result
    OUTPUT_VARIABLE fxc_stdout
    ERROR_VARIABLE fxc_stderr)
```

Require a zero result and a nonempty bytecode file. For matrix fixtures,
register separate `/Zpr` and `/Zpc` validations. Register one validation for
every successful golden and boundary fixture; do not register validation for
diagnostic fixtures.

- [ ] **Step 4: Add optional NVIDIA Cg oracle routing**

Reuse the existing `CGC_REFERENCE_EXECUTABLE` cache path and existence check in
`tests/CMakeLists.txt`; do not define a second variable. When it exists, label
the new HLSL oracle tests `oracle` and run the reference compiler with the
equivalent `hlslv` or `hlslf` profile. Compare acceptance and normalized public
semantic, register, and default records only. Do not compare helper names,
whitespace, or function factoring.

- [ ] **Step 5: Run link, optional validation, and oracle routing**

```powershell
cmake -S . -B build-hlsl -DBUILD_TESTING=ON
cmake --build build-hlsl --config Debug
ctest --test-dir build-hlsl -C Debug -R '^hlsl_(link|validate|oracle)' --output-on-failure
```

Expected: link tests pass; `fxc` tests exist and pass when discovered; oracle
tests exist only when the cache path is explicitly provided.

- [ ] **Step 6: Commit external validation integration**

```powershell
git add -- tests/CMakeLists.txt tests/check_hlsl_link.cmake tests/validate_hlsl.cmake tests/check_hlsl_oracle.cmake tests/hlsl/link tests/hlsl/oracle
git commit -m "Validate generated DirectX 9 HLSL"
```

### Task 12: Complete the compatibility matrix, documentation, and final audit

**Files:**
- Create: `docs/hlsl-sm3-compatibility.md`
- Modify: `README.md`
- Modify: `tests/CMakeLists.txt`
- Add focused fixtures below `tests/hlsl/` only for matrix rows not already
  linked to a test

- [ ] **Step 1: Write the compatibility matrix with no unclassified row**

Create these sections and classify every row as `native`, `legalized`,
`vertex`, `pixel`, or `rejected C####`, with at least one test name in the last
column:

```markdown
| Category | Feature | Status | Test |
|---|---|---|---|
| Type | float scalar/vector | native | hlslv_arithmetic |
| Type | half/fixed | legalized to float | hlslv_numeric_types |
| Type | rectangular matrix | native row_major | hlslv_rectangular |
| Control | recursion | rejected C6401 | hlslv_recursion |
| Stage | discard | pixel | hlslf_discard |
| Stage | derivatives | pixel | hlslf_derivatives |
| Texture | implicit sampling | pixel | hlslf_tex2d |
| Texture | explicit LOD | vertex,pixel | hlslv_tex2dlod |
| Texture | sampler arrays | rejected C6409 | hlslf_sampler_array |
```

Expand the table to every parser-exposed type, qualifier, statement, operator,
semantic family, standard-library intrinsic family, texture form, default,
pragma binding, aggregate shape, and resource bank. Search `parser.y`,
`tokens.h`, `compile.c`, and `stdlib.cg` and add one row for every exposed
feature. There must be no blank status or test cell.

- [ ] **Step 2: Add focused tests for audit gaps**

For each matrix row without an existing fixture, add the smallest Cg input and
either an exact golden or exact diagnostic registration. Run only that new
test immediately, then run `^hlsl` after all gaps close. Do not weaken a row to
"untested"; classify unsupported constructs with one of the defined diagnostics.

- [ ] **Step 3: Document user-facing behavior**

Add an HLSL section to `README.md` with these commands and contracts:

```powershell
.\build\Release\cgc.exe -quiet -profile hlslv -entry main -o shader.vs.hlsl shader.cg
fxc /nologo /T vs_3_0 /E main /Fo shader.vso shader.vs.hlsl
.\build\Release\cgc.exe -quiet -profile hlslf -entry main -o shader.ps.hlsl shader.cg
fxc /nologo /T ps_3_0 /E main /Fo shader.pso shader.ps.hlsl
```

Document public `main`, explicit register banks, first-fit allocation, defaults,
row-major matrices, semantic aliases, the compatibility-matrix link, optional
`fxc`, and the non-goals from the approved design.

- [ ] **Step 4: Verify standard-library and generated-source hygiene**

```powershell
cmake --build build-hlsl --config Debug --target regenerate_stdlib
git diff --exit-code -- stdlib.c
git diff --check
rg -n "TB[D]|TO[D]O|FIXM[E]|not implemented|placeholde[r]" docs/hlsl-sm3-compatibility.md README.md hlsl_*.c hlsl_*.h
```

Expected: regeneration is clean, `git diff --check` is clean, and the scan has
no matches in new HLSL code or documentation.

- [ ] **Step 5: Run complete Debug verification**

```powershell
cmake -S . -B build-hlsl -DBUILD_TESTING=ON
cmake --build build-hlsl --config Debug
ctest --test-dir build-hlsl -C Debug --output-on-failure
```

Expected: the entire suite passes. If `fxc` is installed, every generated HLSL
validation test also passes.

- [ ] **Step 6: Run complete Release verification**

```powershell
cmake --build build-hlsl --config Release
ctest --test-dir build-hlsl -C Release --output-on-failure
```

Expected: the entire suite passes in Release.

- [ ] **Step 7: Perform manual acceptance conversions**

```powershell
$cgcHlsl = (Resolve-Path '.\build-hlsl\Release\cgc.exe').Path
foreach ($shader in 'position.cg','reflection.cg','vertexlight.cg','vertexlight4.cg') {
    $name = [IO.Path]::GetFileNameWithoutExtension($shader)
    & $cgcHlsl -quiet -profile hlslv -entry main -o "build-hlsl\$name.hlsl" $shader
    if ($LASTEXITCODE -ne 0) { throw "hlslv failed: $shader" }
}
& $cgcHlsl -quiet -profile hlslf -entry main -o build-hlsl\texture.hlsl tests\hlsl\textures\fp_tex2d.cg
if ($LASTEXITCODE -ne 0) { throw 'hlslf texture conversion failed' }
```

Expected: five readable HLSL files contain profile/target metadata, explicit
bindings where used, an internal entry, and one public `main`.

- [ ] **Step 8: Commit documentation and audit fixtures**

```powershell
git add -- README.md docs/hlsl-sm3-compatibility.md tests/CMakeLists.txt tests/hlsl
git commit -m "Document DirectX 9 HLSL profiles"
```

- [ ] **Step 9: Run final evidence before completion**

```powershell
git status --short
git diff --check HEAD^ HEAD
ctest --test-dir build-hlsl -C Debug --output-on-failure
ctest --test-dir build-hlsl -C Release --output-on-failure
git log --oneline --decorate -15
```

Expected: both full suites pass; status shows only pre-existing unrelated files;
the recent history contains focused commits for registration, IR, interfaces,
bindings, wrapper emission, core language lowering, aggregates/matrices,
intrinsics, textures, validation, external validation, and documentation.

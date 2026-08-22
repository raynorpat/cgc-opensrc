# OpenGL ARBVP1 and ARBFP1 Profile Support Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add portable `arbvp1` and `arbfp1` compiler profiles that translate the repository's existing Cg dialect into valid base ARB assembly, with deterministic bindings, resource validation, CTest coverage, an optional NVIDIA Cg 3.1 oracle, and a Windows/WGL load smoke test.

**Architecture:** Register two HAL profiles that share a private four-component virtual-register IR, AST lowering, legalization, linear-scan allocation, resource accounting, metadata formatting, and assembly emission. Stage-specific modules own connector semantics, sampler policy, legal instructions, and the minimum limits guaranteed by the Khronos ARB program specifications.

**Tech Stack:** C90, the existing Cg AST/HAL infrastructure, CMake 3.16+, CTest, PowerShell for developer commands, Win32/WGL and `opengl32` for optional driver smoke tests

---

## Reference Material

- Approved design: `docs/superpowers/specs/2026-08-22-opengl-arb-profiles-design.md`
- Khronos vertex grammar and limits: <https://registry.khronos.org/OpenGL/extensions/ARB/ARB_vertex_program.txt>
- Khronos fragment grammar and limits: <https://registry.khronos.org/OpenGL/extensions/ARB/ARB_fragment_program.txt>
- Local HAL example: `generic_hal.c` and `generic_hal.h`
- Frontend normalization order: `compile.c:2540-2608`
- Binding construction and metadata: `semantic.c:118-330`, `binding.c`, and `compile.c:2180-2445`
- AST node and opcode definitions: `support.h:53-430`

## File Map

Every new `.c` and `.h` file below begins with the repository's existing NVIDIA
license notice and follows the neighboring C90 include/brace style.

### Compiler modules

- Create `arb_hal.h`: public registration entry point, profile descriptors, limits, shared HAL state, sampler base IDs, and the backend entry point.
- Create `arb_hal.c`: shared HAL initialization, connector lookup, semantics binding, uniform allocation, pragma binding, capability checks, intrinsic identification, and common cleanup.
- Create `arbvp1_hal.c`: ARBVP1 connector registers, semantic aliases, instruction policy, limits, header, and profile descriptor.
- Create `arbfp1_hal.c`: ARBFP1 connector registers, semantic aliases, texture targets, limits, sampler registration, header, and profile descriptor.
- Create `arb_ir.h`: private IR enums, operands, instructions, parameters, resource counters, and function declarations.
- Create `arb_ir.c`: IR allocation, constant interning, instruction construction, validation, and destruction.
- Create `arb_lower.c`: statement/expression lowering, lvalue resolution, static-loop evaluation, conditional conversion, built-in lowering, texture lowering, and source-location propagation.
- Create `arb_codegen.c`: instruction legalization, dead temporary removal, move coalescing, live interval construction, TEMP allocation, resource validation, declarations, metadata, instructions, `END`, and statistics.
- Modify `cgcmain.c:56-62`: register the two ARB profiles.
- Modify `errors.h:255-290`: add ARB semantic/resource diagnostics and one internal IR diagnostic.
- Modify `stdlib.cg:54-103`: declare fragment sampler intrinsics behind `PROFILE_ARBFP1`.
- Regenerate `stdlib.c` from `stdlib.cg` with the checked-in tokenizer flow.

### Build and documentation

- Modify `CMakeLists.txt:13-36`: compile the ARB modules and add the Windows smoke executable.
- Modify `tests/CMakeLists.txt`: replace the two existing smoke-only registrations with frozen generic regressions, then add ARB fixtures, IR tests, smoke tests, and optional oracle tests.
- Modify `Makefile:1-8`: include every new compiler object in the legacy make build.
- Modify `README.txt:1-35`: document CMake, the new profiles, portable limits, and smoke/oracle test options.

### Test infrastructure and fixtures

- Create `tests/check_generic.cmake`: verify normalized SHA-256 baselines for all four bundled generic-profile shaders.
- Create `tests/run_arb_test.cmake`: run success/failure fixtures, normalize volatile header lines, compare checked-in expected output or diagnostics, and optionally invoke the smoke executable.
- Create `tests/compare_arb_oracle.cmake`: compare success class and public binding metadata against an explicitly configured NVIDIA `cgc` executable.
- Create `tests/arb_ir_tests.c`: exercise IR construction, validation, constant interning, and list ownership without the parser.
- Create `tests/arb_smoke.c`: dependency-free hidden-window WGL program loader.
- Create fixtures and expectations below `tests/arb/profile`, `vp-semantics`, `fp-semantics`, `arithmetic`, `control-flow`, `uniforms`, `textures`, `limits`, `diagnostics`, and `oracle` as named by the tasks below.

## Portable Base Limits

Encode these values in the stage descriptors and cite the Khronos specifications in comments:

```c
/* Minimum limits guaranteed by ARB_vertex_program. */
#define ARBVP_MAX_INSTRUCTIONS 128
#define ARBVP_MAX_TEMPORARIES 12
#define ARBVP_MAX_PARAMETERS 96
#define ARBVP_MAX_ATTRIBUTES 16
#define ARBVP_MAX_ADDRESS_REGISTERS 1

/* Minimum limits guaranteed by ARB_fragment_program. */
#define ARBFP_MAX_INSTRUCTIONS 72
#define ARBFP_MAX_ALU_INSTRUCTIONS 48
#define ARBFP_MAX_TEX_INSTRUCTIONS 24
#define ARBFP_MAX_TEX_INDIRECTIONS 4
#define ARBFP_MAX_TEMPORARIES 16
#define ARBFP_MAX_PARAMETERS 24
#define ARBFP_MAX_ATTRIBUTES 10
#define ARBFP_MAX_TEXTURE_UNITS 2
```

### Task 1: Establish CTest and freeze generic-profile behavior

**Files:**
- Create: `tests/check_generic.cmake`
- Modify: `tests/CMakeLists.txt`

- [ ] **Step 1: Confirm the two existing smoke-only tests**

Run:

```powershell
cmake -S . -B build-arb -A Win32 -DBUILD_TESTING=ON
cmake --build build-arb --config Debug
ctest --test-dir build-arb -C Debug -N
```

Expected: configuration and compilation succeed; CTest lists
`cgc_vertexlight` and `cgc_vertexlight4` and reports `Total Tests: 2`.

- [ ] **Step 2: Add the generic regression checker**

Create `tests/check_generic.cmake` with this complete script:

```cmake
foreach(required CGC SOURCE SOURCE_ROOT EXPECTED_SHA256)
    if(NOT DEFINED ${required})
        message(FATAL_ERROR "${required} is required")
    endif()
endforeach()

execute_process(
    COMMAND "${CGC}" -quiet -profile generic "${SOURCE}"
    WORKING_DIRECTORY "${SOURCE_ROOT}"
    RESULT_VARIABLE result
    OUTPUT_VARIABLE output
    ERROR_VARIABLE error
)

if(NOT result EQUAL 0)
    message(FATAL_ERROR "generic compile failed (${result}):\n${output}${error}")
endif()

string(REPLACE "\r\n" "\n" output "${output}${error}")
string(REGEX REPLACE "(^|\n)// cgc version[^\n]*\n" "\\1" output "${output}")
string(REGEX REPLACE "(^|\n)// command line args:[^\n]*\n" "\\1" output "${output}")
string(SHA256 actual_sha256 "${output}")

if(NOT actual_sha256 STREQUAL EXPECTED_SHA256)
    message(FATAL_ERROR
        "generic output changed for ${SOURCE}\n"
        "expected ${EXPECTED_SHA256}\n"
        "actual   ${actual_sha256}"
    )
endif()
```

- [ ] **Step 3: Register the four known baselines**

Replace the contents of `tests/CMakeLists.txt` with this block. `include(CTest)`
already exists in the root CMake file and must remain there.

```cmake
function(add_generic_regression name source expected_sha256)
    add_test(
        NAME generic_${name}
        COMMAND ${CMAKE_COMMAND}
            -DCGC=$<TARGET_FILE:cgc>
            -DSOURCE=${PROJECT_SOURCE_DIR}/${source}
            -DSOURCE_ROOT=${PROJECT_SOURCE_DIR}
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
```

- [ ] **Step 4: Run the frozen baselines**

Run:

```powershell
cmake -S . -B build-arb -A Win32 -DBUILD_TESTING=ON
cmake --build build-arb --config Debug
ctest --test-dir build-arb -C Debug -R '^generic_' --output-on-failure
```

Expected: four tests run and all four pass.

- [ ] **Step 5: Commit the test foundation**

```powershell
git add -- tests/CMakeLists.txt tests/check_generic.cmake
git commit -m "Add generic profile regression tests"
```

### Task 2: Add the reusable ARB fixture runner

**Files:**
- Create: `tests/run_arb_test.cmake`
- Modify: `tests/CMakeLists.txt`

- [ ] **Step 1: Confirm the runner dependency is absent**

Run:

```powershell
Test-Path tests/run_arb_test.cmake
```

Expected: `False`. The runner is a new test-infrastructure dependency.

Then add this helper to `tests/CMakeLists.txt`:

```cmake
function(add_arb_fixture name profile source expected)
    add_test(
        NAME ${name}
        COMMAND ${CMAKE_COMMAND}
            -DTEST_NAME=${name}
            -DCGC=$<TARGET_FILE:cgc>
            -DPROFILE=${profile}
            -DSOURCE=${PROJECT_SOURCE_DIR}/${source}
            -DEXPECT_SUCCESS=TRUE
            -DEXPECTED_OUTPUT=${PROJECT_SOURCE_DIR}/${expected}
            -DWORK_DIR=${CMAKE_CURRENT_BINARY_DIR}
            -P ${CMAKE_CURRENT_SOURCE_DIR}/run_arb_test.cmake
    )
endfunction()
```

Do not register a real ARB fixture until Task 4. Configure now to verify that
defining the helper alone does not disturb the existing build:

Run:

```powershell
cmake -S . -B build-arb -A Win32 -DBUILD_TESTING=ON
```

Expected: configuration succeeds. The helper will begin invoking the runner when
Task 4 registers the first ARB fixture.

- [ ] **Step 2: Implement the fixture runner**

Create `tests/run_arb_test.cmake`:

```cmake
foreach(required TEST_NAME CGC PROFILE SOURCE EXPECT_SUCCESS WORK_DIR)
    if(NOT DEFINED ${required})
        message(FATAL_ERROR "${required} is required")
    endif()
endforeach()

file(MAKE_DIRECTORY "${WORK_DIR}")
set(output_file "${WORK_DIR}/${TEST_NAME}.arb")
file(REMOVE "${output_file}")

execute_process(
    COMMAND "${CGC}" -quiet -profile "${PROFILE}" -entry main
            -o "${output_file}" ${EXTRA_ARGS} "${SOURCE}"
    RESULT_VARIABLE result
    OUTPUT_VARIABLE stdout
    ERROR_VARIABLE stderr
)

function(normalize_arb input output_name)
    string(REPLACE "\r\n" "\n" normalized "${input}")
    string(REGEX REPLACE "(^|\n)# cgc version[^\n]*\n" "\\1"
        normalized "${normalized}")
    string(REGEX REPLACE "(^|\n)# command line args:[^\n]*\n" "\\1"
        normalized "${normalized}")
    set(${output_name} "${normalized}" PARENT_SCOPE)
endfunction()

if(EXPECT_SUCCESS)
    if(NOT result EQUAL 0)
        message(FATAL_ERROR "${TEST_NAME} failed (${result}):\n${stdout}${stderr}")
    endif()
    if(NOT EXISTS "${output_file}")
        message(FATAL_ERROR "${TEST_NAME} did not create ${output_file}")
    endif()
    file(READ "${output_file}" actual)
    normalize_arb("${actual}" actual)
    if(DEFINED EXPECTED_OUTPUT)
        file(READ "${EXPECTED_OUTPUT}" expected)
        normalize_arb("${expected}" expected)
        if(NOT actual STREQUAL expected)
            file(WRITE "${WORK_DIR}/${TEST_NAME}.actual" "${actual}")
            message(FATAL_ERROR
                "${TEST_NAME} output mismatch; actual saved to "
                "${WORK_DIR}/${TEST_NAME}.actual"
            )
        endif()
    endif()
    if(DEFINED SMOKE AND DEFINED STAGE)
        execute_process(
            COMMAND "${SMOKE}" "${STAGE}" "${output_file}"
            RESULT_VARIABLE smoke_result
            OUTPUT_VARIABLE smoke_stdout
            ERROR_VARIABLE smoke_stderr
        )
        if(smoke_result EQUAL 77)
            message("SKIP: ${smoke_stdout}${smoke_stderr}")
        elseif(NOT smoke_result EQUAL 0)
            message(FATAL_ERROR "ARB driver load failed:\n${smoke_stdout}${smoke_stderr}")
        endif()
    endif()
else()
    if(result EQUAL 0)
        message(FATAL_ERROR "${TEST_NAME} unexpectedly succeeded")
    endif()
    set(diagnostics "${stdout}${stderr}")
    if(DEFINED EXPECTED_DIAGNOSTICS)
        file(STRINGS "${EXPECTED_DIAGNOSTICS}" fragments)
        foreach(fragment IN LISTS fragments)
            if(NOT fragment STREQUAL "")
                string(FIND "${diagnostics}" "${fragment}" found)
                if(found EQUAL -1)
                    message(FATAL_ERROR
                        "missing diagnostic fragment '${fragment}':\n${diagnostics}"
                    )
                endif()
            endif()
        endforeach()
    endif()
endif()
```

- [ ] **Step 3: Add a failure-fixture helper**

Add this function beside `add_arb_fixture`:

```cmake
function(add_arb_failure name profile source diagnostics)
    add_test(
        NAME ${name}
        COMMAND ${CMAKE_COMMAND}
            -DTEST_NAME=${name}
            -DCGC=$<TARGET_FILE:cgc>
            -DPROFILE=${profile}
            -DSOURCE=${PROJECT_SOURCE_DIR}/${source}
            -DEXPECT_SUCCESS=FALSE
            -DEXPECTED_DIAGNOSTICS=${PROJECT_SOURCE_DIR}/${diagnostics}
            -DWORK_DIR=${CMAKE_CURRENT_BINARY_DIR}
            -P ${CMAKE_CURRENT_SOURCE_DIR}/run_arb_test.cmake
    )
endfunction()
```

- [ ] **Step 4: Reconfigure and keep the generic tests green**

Run:

```powershell
cmake -S . -B build-arb -A Win32 -DBUILD_TESTING=ON
cmake --build build-arb --config Debug
ctest --test-dir build-arb -C Debug -R '^generic_' --output-on-failure
```

Expected: configuration succeeds and all four generic regressions pass.

- [ ] **Step 5: Commit the fixture runner**

```powershell
git add -- tests/CMakeLists.txt tests/run_arb_test.cmake
git commit -m "Add ARB compiler fixture runner"
```

### Task 3: Register ARB profile skeletons and semantic connectors

**Files:**
- Create: `arb_hal.h`
- Create: `arb_hal.c`
- Create: `arbvp1_hal.c`
- Create: `arbfp1_hal.c`
- Create: `tests/arb/profile/vp_passthrough.cg`
- Create: `tests/arb/profile/fp_passthrough.cg`
- Modify: `cgcmain.c:56-62`
- Modify: `CMakeLists.txt:13-36`
- Modify: `tests/CMakeLists.txt`
- Modify: `Makefile:1-3`

- [ ] **Step 1: Add profile-selection tests before registration**

Create `tests/arb/profile/vp_passthrough.cg`:

```c
struct appin {
    float4 position : POSITION;
};

struct vout {
    float4 position : POSITION;
};

vout main(appin input)
{
    vout output;
    output.position = input.position;
    return output;
}
```

Create `tests/arb/profile/fp_passthrough.cg`:

```c
float4 main(float4 color : COLOR0) : COLOR
{
    return color;
}
```

Add direct `-nocode` tests in `tests/CMakeLists.txt`:

```cmake
add_test(NAME arbvp1_registration
    COMMAND $<TARGET_FILE:cgc> -quiet -nocode -profile arbvp1
            ${PROJECT_SOURCE_DIR}/tests/arb/profile/vp_passthrough.cg)
add_test(NAME arbfp1_registration
    COMMAND $<TARGET_FILE:cgc> -quiet -nocode -profile arbfp1
            ${PROJECT_SOURCE_DIR}/tests/arb/profile/fp_passthrough.cg)
```

Run:

```powershell
cmake --build build-arb --config Debug
ctest --test-dir build-arb -C Debug -R '^arb(vp|fp)1_registration$' --output-on-failure
```

Expected: both tests fail with `unknown profile`.

- [ ] **Step 2: Define the shared public profile model**

Create `arb_hal.h` with the repository's NVIDIA license header and these declarations:

```c
#if !defined(__ARB_HAL_H)
#define __ARB_HAL_H 1

#include "hal.h"

#define PROFILE_ARBVP1_NAME "arbvp1"
#define PROFILE_ARBVP1_ID 10
#define PROFILE_ARBFP1_NAME "arbfp1"
#define PROFILE_ARBFP1_ID 11

#define CID_ARBVP1_IN_ID 10
#define CID_ARBVP1_OUT_ID 11
#define CID_ARBFP1_IN_ID 12
#define CID_ARBFP1_OUT_ID 13

#define TYPE_BASE_SAMPLER1D   (TYPE_BASE_FIRST_USER + 0)
#define TYPE_BASE_SAMPLER2D   (TYPE_BASE_FIRST_USER + 1)
#define TYPE_BASE_SAMPLER3D   (TYPE_BASE_FIRST_USER + 2)
#define TYPE_BASE_SAMPLERCUBE (TYPE_BASE_FIRST_USER + 3)
#define TYPE_BASE_SAMPLERRECT (TYPE_BASE_FIRST_USER + 4)

typedef enum ArbStage_Enum {
    ARB_STAGE_VERTEX,
    ARB_STAGE_FRAGMENT
} ArbStage;

#define ARB_BUILTIN_GROUP 1

typedef enum ArbBuiltin_Enum {
    ARB_BUILTIN_RSQ = 1,
    ARB_BUILTIN_TEX1D,
    ARB_BUILTIN_TEX1DPROJ,
    ARB_BUILTIN_TEX1DBIAS,
    ARB_BUILTIN_TEX2D,
    ARB_BUILTIN_TEX2DPROJ,
    ARB_BUILTIN_TEX2DBIAS,
    ARB_BUILTIN_TEX3D,
    ARB_BUILTIN_TEX3DPROJ,
    ARB_BUILTIN_TEX3DBIAS,
    ARB_BUILTIN_TEXCUBE,
    ARB_BUILTIN_TEXCUBEPROJ,
    ARB_BUILTIN_TEXCUBEBIAS,
    ARB_BUILTIN_TEXRECT,
    ARB_BUILTIN_TEXRECTPROJ
} ArbBuiltin;

typedef struct ArbLimits_Rec {
    int instructions;
    int aluInstructions;
    int texInstructions;
    int texIndirections;
    int temporaries;
    int parameters;
    int attributes;
    int addressRegisters;
    int textureUnits;
} ArbLimits;

typedef struct ArbProfileDesc_Rec {
    const char *name;
    int pid;
    ArbStage stage;
    const char *header;
    const ArbLimits *limits;
    ConnectorDescriptor *connectors;
    int numConnectors;
    SemanticsDescriptor *semantics;
    int numSemantics;
} ArbProfileDesc;

typedef struct ArbHALData_Rec {
    const ArbProfileDesc *profile;
    int nextUniform;
    unsigned char uniformUsed[ARBVP_MAX_PARAMETERS];
    int nextTextureUnit;
    signed char textureTarget[ARBFP_MAX_TEXTURE_UNITS];
} ArbHALData;

extern const ArbProfileDesc ArbProfile_arbvp1;
extern const ArbProfileDesc ArbProfile_arbfp1;

int RegisterProfiles_arb(void);
int InitHAL_arbvp1(slHAL *fHAL);
int InitHAL_arbfp1(slHAL *fHAL);
int InitHAL_arb(slHAL *fHAL, const ArbProfileDesc *profile);
int GenerateCode_arb(SourceLoc *loc, Scope *fScope, Symbol *program);

#endif /* !defined(__ARB_HAL_H) */
```

Place the portable-limit macros shown near the top of this plan before `ArbHALData` so its arrays have constant C90 sizes.

- [ ] **Step 3: Implement common HAL initialization and connector lookup**

In `arb_hal.c`, implement `InitHAL_arb` by allocating and zeroing `ArbHALData`, setting `profile`, and assigning the following HAL members:

```c
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
fHAL->comment = "#";
fHAL->localData = data;
```

Use `profile->connectors` with `LookupConnectorHAL` in the four connector functions. `RegisterNames_arb` must atomize every connector name, connector register name, and semantic name. `FreeHAL_arb` frees `localData` and sets it to `NULL`.

For the skeleton, `CheckInternalFunction_arb` recognizes the existing `rsqrt`
internal declaration, stores `ARB_BUILTIN_GROUP` through `group`, and returns
`ARB_BUILTIN_RSQ`. It returns zero for every other name. Task 10 adds the texture
names without changing these stable IDs.

For both stages, `IsValidRuntimeBase_arb` initially permits float, bool, and int;
int is needed so the backend can distinguish statically eliminated loop/index
values from illegal surviving runtime integer arithmetic. `IsTexobjBase_arb`
returns false until Task 10 registers the fragment sampler bases.

Return these capability values from `GetCapsBit_arb`:

```c
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
```

Both stages must advertise indexed arrays because the preserved frontend uses
this bit to permit any unpacked array declaration. The backend then enforces the
stage distinction: ARBVP1 may lower legal relative uniform indexing, while
ARBFP1 accepts only indices resolved to constants.

For this task, `GenerateCode_arb` returns `1` without writing instructions; the registration tests use `-nocode`. Task 5 replaces this no-op with the real pipeline.

- [ ] **Step 4: Define stage descriptors and semantic tables**

In `arbvp1_hal.c`, define connector register names for conventional vertex
inputs, `ATTR0` through `ATTR15`, and these outputs: `HPOS`, `COL0`, `COL1`,
`BFC0`, `BFC1`, `FOGC`, `PSIZ`, and `TEX0` through `TEX7`. Define semantics with
these bindings:

```text
Input:  POSITION, BLENDWEIGHT, NORMAL, COLOR0, COLOR1, FOG, TEXCOORD0..7, ATTR0..15
Output: POSITION->HPOS, COLOR0->COL0, COLOR1->COL1, FOG->FOGC,
        BCOL0->BFC0, BCOL1->BFC1, PSIZE->PSIZ, TEXCOORD0..7->TEX0..7
```

Do not expose `CLP0..CLP5`: clip-distance result registers are not part of the
base `ARB_vertex_program` grammar and require later profile/extension support.

Mark vertex `POSITION` output `SEM_REQUIRED`. Set the descriptor limits to:

```c
static const ArbLimits limits_arbvp1 = {
    128, 0, 0, 0, 12, 96, 16, 1, 0
};
```

The zero ALU field means ARBVP1 tracks its single instruction limit only; the
separate ALU/texture counters are fragment-profile concepts.

In `arbfp1_hal.c`, define inputs `WPOS`, `COL0`, `COL1`, `FOGC`, and `TEX0` through `TEX7`, with outputs `COL` and `DEPR`. Bind:

```text
Input:  POSITION/WPOS->WPOS, COLOR0->COL0, COLOR1->COL1,
        FOG->FOGC, TEXCOORD0..7->TEX0..7
Output: COLOR/COLOR0->COL, DEPTH->DEPR
```

Set fragment limits to:

```c
static const ArbLimits limits_arbfp1 = {
    72, 48, 24, 4, 16, 24, 10, 0, 2
};
```

Each stage initializer sets `incid`, `outcid`, the input/output register arrays and counts, then calls `InitHAL_arb` with its descriptor.

- [ ] **Step 5: Bind varying semantics without duplicating stage logic**

Implement `BindVaryingSemantic_arb` with the same numeric-suffix parsing and scalar/vector type checks used by `BindVaryingSemantic_generic`, plus all of these checks:

```c
if (GetBase(fSymb->type) != TYPE_BASE_FLOAT)
    return 0;
if (IsOutVal && !(semantics->properties & SEM_OUT))
    return 0;
if (!IsOutVal && !(semantics->properties & SEM_IN))
    return 0;
```

On success, set `BK_CONNECTOR`, `BIND_IS_BOUND | BIND_VARYING`, input/output and required properties, the connector register atom and number, and the symbol's connector read/write flags. `BindVaryingPragma_arb` accepts only `BK_CONNECTOR` trees whose named register exists in the stage and whose direction matches `IsOutVal`.

- [ ] **Step 6: Register and build the profiles**

Implement registration in `arb_hal.c`:

```c
int RegisterProfiles_arb(void)
{
    RegisterProfile(InitHAL_arbvp1, PROFILE_ARBVP1_NAME, PROFILE_ARBVP1_ID);
    RegisterProfile(InitHAL_arbfp1, PROFILE_ARBFP1_NAME, PROFILE_ARBFP1_ID);
    return 1;
}
```

Add `RegisterProfiles_arb` to `RegistrationFunctions` in `cgcmain.c`. Add `arb_hal.c`, `arbvp1_hal.c`, and `arbfp1_hal.c` to the `cgc` target and to `Makefile`'s `OBJS`.

- [ ] **Step 7: Run registration and generic tests**

Run:

```powershell
cmake -S . -B build-arb -A Win32 -DBUILD_TESTING=ON
cmake --build build-arb --config Debug
ctest --test-dir build-arb -C Debug -R '^(arb(vp|fp)1_registration|generic_)' --output-on-failure
```

Expected: both registration tests and all four generic tests pass.

- [ ] **Step 8: Commit the profile skeletons**

```powershell
git add -- arb_hal.h arb_hal.c arbvp1_hal.c arbfp1_hal.c cgcmain.c CMakeLists.txt tests/CMakeLists.txt Makefile tests/arb/profile
git commit -m "Register ARB vertex and fragment profiles"
```

### Task 4: Add the private vector IR and validator

**Files:**
- Create: `arb_ir.h`
- Create: `arb_ir.c`
- Create: `tests/arb_ir_tests.c`
- Modify: `tests/CMakeLists.txt`

- [ ] **Step 1: Write the IR unit test before the IR exists**

Create `tests/arb_ir_tests.c` with the repository license header and this test body:

```c
#include <assert.h>
#include <string.h>

#define NO_PARSER 1
#include "slglobals.h"
#include "arb_ir.h"

int main(void)
{
    ArbProgram program;
    ArbInstruction *inst;
    ArbOperand dst, src;
    SourceLoc loc = { 0, 7 };
    int t0;
    int c0, c1;
    float value[4] = { 1.0f, 2.0f, 3.0f, 4.0f };

    ArbInitProgram(&program, ARB_STAGE_VERTEX);
    t0 = ArbNewTemp(&program);
    assert(t0 == 0);

    dst = ArbTempOperand(t0);
    src = ArbInputOperand(0);
    inst = ArbAppendInstruction(&program, ARB_OP_MOV, &loc, dst);
    assert(inst != NULL);
    assert(ArbAddSource(inst, src));
    assert(ArbValidateIR(&program) == ARB_IR_VALID);

    c0 = ArbInternConstant(&program, value, 4);
    c1 = ArbInternConstant(&program, value, 4);
    assert(c0 == c1);
    assert(program.numConstants == 1);

    inst->srcCount = 4;
    assert(ArbValidateIR(&program) == ARB_IR_BAD_SOURCE_COUNT);
    ArbFreeProgram(&program);
    return 0;
}
```

Register it:

```cmake
add_executable(arb_ir_tests arb_ir_tests.c ${PROJECT_SOURCE_DIR}/arb_ir.c)
target_include_directories(arb_ir_tests PRIVATE ${PROJECT_SOURCE_DIR})
set_target_properties(arb_ir_tests PROPERTIES
    C_STANDARD 90
    C_STANDARD_REQUIRED YES
    C_EXTENSIONS YES
)
add_test(NAME arb_ir_unit COMMAND arb_ir_tests)
```

Run:

```powershell
cmake --build build-arb --config Debug
```

Expected: compilation fails because `arb_ir.h` and `arb_ir.c` do not exist.

- [ ] **Step 2: Define the IR types**

Create `arb_ir.h` with these exact public types, plus the repository license and `__ARB_IR_H` guard:

```c
#include "arb_hal.h"

#define ARB_MASK_X 0x1
#define ARB_MASK_Y 0x2
#define ARB_MASK_Z 0x4
#define ARB_MASK_W 0x8
#define ARB_MASK_XYZW 0xf

typedef enum ArbOpcode_Enum {
    ARB_OP_ABS, ARB_OP_ADD, ARB_OP_ARL, ARB_OP_CMP, ARB_OP_COS,
    ARB_OP_DP3, ARB_OP_DP4, ARB_OP_DPH, ARB_OP_DST, ARB_OP_EX2,
    ARB_OP_EXP, ARB_OP_FLR, ARB_OP_FRC, ARB_OP_KIL, ARB_OP_LG2,
    ARB_OP_LIT, ARB_OP_LOG, ARB_OP_LRP, ARB_OP_MAD, ARB_OP_MAX,
    ARB_OP_MIN, ARB_OP_MOV, ARB_OP_MUL, ARB_OP_POW, ARB_OP_RCP,
    ARB_OP_RSQ, ARB_OP_SCS, ARB_OP_SGE, ARB_OP_SIN, ARB_OP_SLT,
    ARB_OP_SUB, ARB_OP_SWZ, ARB_OP_TEX, ARB_OP_TXB, ARB_OP_TXP,
    ARB_OP_XPD, ARB_OP_LAST
} ArbOpcode;

typedef enum ArbRegisterFile_Enum {
    ARB_REG_NONE, ARB_REG_TEMP, ARB_REG_INPUT, ARB_REG_OUTPUT,
    ARB_REG_PARAM, ARB_REG_CONST, ARB_REG_ADDRESS
} ArbRegisterFile;

typedef enum ArbTextureTarget_Enum {
    ARB_TEX_NONE, ARB_TEX_1D, ARB_TEX_2D, ARB_TEX_3D,
    ARB_TEX_CUBE, ARB_TEX_RECT
} ArbTextureTarget;

typedef enum ArbIRStatus_Enum {
    ARB_IR_VALID, ARB_IR_BAD_OPCODE, ARB_IR_BAD_DESTINATION,
    ARB_IR_BAD_SOURCE_COUNT, ARB_IR_BAD_SOURCE,
    ARB_IR_BAD_MASK, ARB_IR_BAD_TEXTURE, ARB_IR_STAGE_OPCODE
} ArbIRStatus;

typedef struct ArbOperand_Rec {
    ArbRegisterFile file;
    int index;
    int bindingName;
    signed char swizzle[4];
    unsigned char negate;
    unsigned char absolute;
    unsigned char relative;
    signed char relativeOffset;
} ArbOperand;

typedef struct ArbInstruction_Rec {
    struct ArbInstruction_Rec *next;
    SourceLoc loc;
    ArbOpcode opcode;
    ArbOperand dst;
    ArbOperand src[3];
    unsigned char srcCount;
    unsigned char mask;
    unsigned char saturate;
    signed char textureUnit;
    ArbTextureTarget textureTarget;
    int physicalTemp;
} ArbInstruction;

typedef struct ArbConstant_Rec {
    struct ArbConstant_Rec *next;
    float value[4];
    int size;
    int index;
} ArbConstant;

typedef struct ArbProgram_Rec {
    ArbStage stage;
    ArbInstruction *first;
    ArbInstruction *last;
    ArbConstant *constants;
    int numInstructions;
    int numVirtualTemps;
    int numConstants;
    int numPhysicalTemps;
} ArbProgram;

void ArbInitProgram(ArbProgram *program, ArbStage stage);
void ArbFreeProgram(ArbProgram *program);
int ArbNewTemp(ArbProgram *program);
ArbOperand ArbTempOperand(int index);
ArbOperand ArbInputOperand(int index);
ArbOperand ArbOutputOperand(int index);
ArbOperand ArbParamOperand(int index);
ArbOperand ArbConstOperand(int index);
ArbInstruction *ArbAppendInstruction(ArbProgram *program, ArbOpcode opcode,
                                     const SourceLoc *loc, ArbOperand dst);
int ArbAddSource(ArbInstruction *instruction, ArbOperand source);
int ArbInternConstant(ArbProgram *program, const float *value, int size);
ArbIRStatus ArbValidateIR(const ArbProgram *program);
```

- [ ] **Step 3: Implement construction, interning, validation, and cleanup**

In `arb_ir.c`, use `malloc`, `free`, and `memset`. Initialize every operand to identity swizzle `{ 0, 1, 2, 3 }`, full mask, `textureUnit = -1`, `textureTarget = ARB_TEX_NONE`, and `physicalTemp = -1`. `ArbInternConstant` must expand a scalar to all four components and compare the four stored floats plus `size` before allocating another node.

Implement `ArbValidateIR` using this source-count table:

```c
static const signed char sourceCounts[ARB_OP_LAST] = {
    1, 2, 1, 3, 1, 2, 2, 2, 2, 1,
    1, 1, 1, 1, 1, 1, 1, 3, 3, 2,
    2, 1, 2, 2, 1, 1, 1, 2, 1, 2,
    2, 1, 1, 1, 1, 2
};
```

Special rules: `KIL` has no destination; `TEX`, `TXB`, and `TXP` require a texture unit and non-`NONE` target; `ARL` must target `ARB_REG_ADDRESS`; every other instruction must target TEMP or OUTPUT. Reject swizzle components outside `0..3`, a zero destination mask, and a TEMP index outside `numVirtualTemps`.

Apply the base-profile stage table during validation. Vertex IR rejects `CMP`,
`COS`, `KIL`, `LRP`, `SCS`, `SIN`, `TEX`, `TXB`, and `TXP`. Fragment IR rejects
`ARL`, `EXP`, and `LOG`. Return `ARB_IR_STAGE_OPCODE` before emission; user-source
lowering paths still issue their specific C6001/C6006/C6007 diagnostic before
this internal safety net is reached.

- [ ] **Step 4: Build and run the IR unit test**

Run:

```powershell
cmake -S . -B build-arb -A Win32 -DBUILD_TESTING=ON
cmake --build build-arb --config Debug
ctest --test-dir build-arb -C Debug -R '^arb_ir_unit$' --output-on-failure
```

Expected: `arb_ir_unit` passes.

- [ ] **Step 5: Commit the IR foundation**

```powershell
git add -- arb_ir.h arb_ir.c tests/arb_ir_tests.c tests/CMakeLists.txt
git commit -m "Add ARB vector instruction IR"
```

### Task 5: Emit a complete ARBVP1 pass-through program

**Files:**
- Create: `arb_lower.c`
- Create: `arb_codegen.c`
- Create: `tests/arb/profile/vp_passthrough.expected`
- Modify: `arb_hal.c`
- Modify: `arb_ir.h`
- Modify: `errors.h`
- Modify: `CMakeLists.txt`
- Modify: `tests/CMakeLists.txt`
- Modify: `Makefile`

- [ ] **Step 1: Add the first exact assembly fixture**

Create `tests/arb/profile/vp_passthrough.expected`:

```text
!!ARBvp1.0
#vendor NVIDIA Corporation
#version 1.0
#profile arbvp1
#program main
#var float4 input.position : $vin.POSITION : POSITION : 0 : 1
#var float4 main.position : $vout.POSITION : HPOS : -1 : 1
MOV result.position, vertex.position;
END
# 1 instructions, 0 R-regs
```

Register the fixture after the profile-registration tests:

```cmake
add_arb_fixture(arbvp1_passthrough arbvp1
    tests/arb/profile/vp_passthrough.cg
    tests/arb/profile/vp_passthrough.expected)
```

Run:

```powershell
cmake --build build-arb --config Debug
ctest --test-dir build-arb -C Debug -R '^arbvp1_passthrough$' --output-on-failure
```

Expected: failure because the Task 3 code generator emits no `MOV` or `END`.

- [ ] **Step 2: Define the lowering context and entry point**

At the top of `arb_lower.c`, define:

```c
typedef struct ArbLowerContext_Rec {
    ArbProgram *ir;
    const ArbProfileDesc *profile;
    Symbol *program;
    struct ArbStaticValue_Rec *staticValues;
} ArbLowerContext;

typedef struct ArbStaticValue_Rec {
    struct ArbStaticValue_Rec *next;
    Symbol *symbol;
    int value;
} ArbStaticValue;

int ArbLowerProgram(ArbProgram *ir, const ArbProfileDesc *profile,
                    Symbol *program);
```

Declare `ArbLowerProgram` in `arb_ir.h`. `ArbLowerProgram` walks
`program->details.fun.statements` in source order and returns `0` immediately
after any failed statement or expression lowering.

- [ ] **Step 3: Resolve the pass-through lvalue and source**

Implement these private lowering functions with the exact responsibilities shown:

```c
static int LowerStatement(ArbLowerContext *ctx, stmt *statement);
static int LowerAssignment(ArbLowerContext *ctx, expr *left, expr *right,
                           int mask, const SourceLoc *loc);
static int LowerLValue(ArbLowerContext *ctx, expr *expression,
                       ArbOperand *operand, int *mask);
static int LowerExpression(ArbLowerContext *ctx, expr *expression,
                           ArbOperand *operand);
static int LowerConnectorMember(ArbLowerContext *ctx, expr *expression,
                                ArbOperand *operand);
```

For `MEMBER_SELECTOR_OP`, inspect the right-side member symbol's `Binding`.
Map `BIND_INPUT` to `ArbInputOperand(bind->conn.regno)` and `BIND_OUTPUT` to
`ArbOutputOperand(bind->conn.regno)`, then copy `bind->conn.rname` into the
operand's `bindingName`. This preserves whether a vertex input used its
conventional spelling or its generic `ATTRn` alias. `LowerLValue` accepts only
output or local TEMP destinations. `LowerExpression` accepts input, uniform
PARAM, CONST, and TEMP sources.

For `ASSIGN_OP`, `ASSIGN_V_OP`, and `ASSIGN_MASKED_KV_OP`, lower the right side,
resolve the left side, combine the AST write mask with the destination mask, and
append `ARB_OP_MOV`.

- [ ] **Step 4: Implement stage register spelling and instruction output**

In `arb_codegen.c`, define complete stage register-name helpers:

```c
static const char *VertexInputName(int regno);
static const char *VertexOutputName(int regno);
static const char *FragmentInputName(int regno);
static const char *FragmentOutputName(int regno);
static const char *OpcodeName(ArbStage stage, ArbOpcode opcode);
static int WriteOperand(FILE *out, const ArbProgram *program,
                        const ArbOperand *operand, int destination);
static int WriteInstruction(FILE *out, const ArbProgram *program,
                            const ArbInstruction *instruction);
```

Use this exact vertex spelling table; a generic input binding always selects the
right-hand spelling:

```text
POSITION / ATTR0       -> vertex.position / vertex.attrib[0]
BLENDWEIGHT / ATTR1    -> vertex.weight / vertex.attrib[1]
NORMAL / ATTR2         -> vertex.normal / vertex.attrib[2]
COLOR0 / ATTR3         -> vertex.color.primary / vertex.attrib[3]
COLOR1 / ATTR4         -> vertex.color.secondary / vertex.attrib[4]
FOG / ATTR5            -> vertex.fogcoord / vertex.attrib[5]
ATTR6..ATTR7            -> vertex.attrib[6]..vertex.attrib[7]
TEXCOORD0..7 / ATTR8..15 -> vertex.texcoord[0]..vertex.texcoord[7] /
                            vertex.attrib[8]..vertex.attrib[15]
HPOS                    -> result.position
COL0 / COL1             -> result.color.front.primary / result.color.front.secondary
BFC0 / BFC1             -> result.color.back.primary / result.color.back.secondary
FOGC / PSIZ             -> result.fogcoord / result.pointsize
TEX0..TEX7              -> result.texcoord[0]..result.texcoord[7]
```

The pass-through mapping must therefore produce `vertex.position` and
`result.position`. `WriteOperand` prints destination masks only when they are not
`xyzw`; it prints source negation before the register and source swizzles only
when not identity. Every instruction ends in `;\n`.

- [ ] **Step 5: Implement the validated generation transaction**

Add the complete backend diagnostic vocabulary before the transaction can call
lowering and validation failures:

```c
#define ERROR_S_ARB_UNSUPPORTED_OPERATION 6001, "ARB profile does not support operation \"%s\""
#define ERROR___ARB_LOOP_NOT_UNROLLABLE   6002, "loop cannot be unrolled at compile time"
#define ERROR___ARB_FRAGMENT_DYNAMIC_INDEX 6003, "ARB fragment profile requires a constant uniform index"
#define ERROR_DS_ARB_TEXTURE_CONFLICT     6004, "texture unit %d already has target %s"
#define ERROR_SDD_ARB_RESOURCE_LIMIT      6005, "%s resource use %d exceeds portable limit %d"
#define ERROR___ARB_VERTEX_DISCARD        6006, "discard is not supported by the ARB vertex profile"
#define ERROR___ARB_VERTEX_TEXTURE        6007, "texture access is not supported by the ARB vertex profile"
#define ERROR___ARB_REQUIRED_POSITION     6008, "ARB vertex program must write POSITION"
#define ERROR___ARB_ATTRIBUTE_ALIAS       6009, "generic and conventional vertex attributes alias"
#define ERROR_S_ARB_INVALID_MODIFIER      6010, "ARB instruction cannot represent modifier combination \"%s\""
#define ERROR___ARB_INVALID_IR            9010, "invalid ARB intermediate representation"
```

Replace the Task 3 no-op `GenerateCode_arb` with this control flow:

Include `arb_ir.h` from `arb_hal.c` before adding the transaction.

```c
int GenerateCode_arb(SourceLoc *loc, Scope *fScope, Symbol *program)
{
    ArbHALData *data = (ArbHALData *) Cg->theHAL->localData;
    ArbProgram ir;
    int ok;

    ArbInitProgram(&ir, data->profile->stage);
    ok = ArbLowerProgram(&ir, data->profile, program);
    if (ok)
        ok = ArbLegalizeAndAllocate(&ir, data->profile, loc);
    if (ok)
        ok = ArbValidateResources(&ir, data->profile, loc);
    if (ok) {
        OutputBindings(Cg->options.outfd, Cg->theHAL, program);
        ok = ArbWriteProgram(Cg->options.outfd, &ir, data->profile);
    }
    ArbFreeProgram(&ir);
    return ok;
}
```

Declare the three `arb_codegen.c` functions in `arb_ir.h`, after the complete
`ArbProgram` definition, to avoid a circular public-header dependency. For this task,
`ArbLegalizeAndAllocate` accepts programs with no virtual TEMP values;
`ArbValidateResources` checks only total instruction count; and
`ArbWriteProgram` writes instructions, `END`, and the exact statistics line used
by the expected fixture.

- [ ] **Step 6: Add the new modules to both builds**

Add `arb_ir.c`, `arb_lower.c`, and `arb_codegen.c` to the CMake `cgc` target and
add `arb_ir.o arb_lower.o arb_codegen.o` to `Makefile`'s `OBJS`.

- [ ] **Step 7: Run the first code-generation test and all earlier tests**

Run:

```powershell
cmake -S . -B build-arb -A Win32 -DBUILD_TESTING=ON
cmake --build build-arb --config Debug
ctest --test-dir build-arb -C Debug -R '^(arbvp1_passthrough|arb_ir_unit|generic_)' --output-on-failure
```

Expected: the pass-through, IR unit, and four generic tests all pass.

- [ ] **Step 8: Commit basic ARBVP1 emission**

```powershell
git add -- arb_hal.h arb_hal.c arb_ir.h arb_ir.c arb_lower.c arb_codegen.c errors.h CMakeLists.txt tests/CMakeLists.txt Makefile tests/arb/profile/vp_passthrough.expected
git commit -m "Emit basic ARB vertex programs"
```

### Task 6: Lower arithmetic, masks, swizzles, constants, and RSQ

**Files:**
- Create: `tests/arb/arithmetic/vp_arithmetic.cg`
- Create: `tests/arb/arithmetic/vp_arithmetic.expected`
- Create: `tests/arb/arithmetic/vp_swizzle.cg`
- Create: `tests/arb/arithmetic/vp_swizzle.expected`
- Modify: `arb_lower.c`
- Modify: `arb_codegen.c`
- Modify: `arb_hal.c`
- Modify: `tests/CMakeLists.txt`

- [ ] **Step 1: Write arithmetic and component tests**

Create `tests/arb/arithmetic/vp_arithmetic.cg`:

```c
struct appin {
    float4 position : POSITION;
    float4 color : COLOR0;
};

struct vout {
    float4 position : POSITION;
    float4 color : COLOR0;
};

vout main(appin input)
{
    vout output;
    float scale = rsqrt(input.position.x * input.position.x);
    output.position = input.position;
    output.color = input.color * scale + float4(0.25, 0.25, 0.25, 0.0);
    return output;
}
```

Create `tests/arb/arithmetic/vp_swizzle.cg`:

```c
struct appin { float4 position : POSITION; };
struct vout { float4 position : POSITION; float4 color : COLOR0; };

vout main(appin input)
{
    vout output;
    float4 value;
    value.xy = input.position.yx;
    value.zw = float2(0.0, 1.0);
    output.position = input.position;
    output.color = -value;
    return output;
}
```

Register both fixtures before creating their expected files, then run:

```powershell
cmake --build build-arb --config Debug
ctest --test-dir build-arb -C Debug -R '^arbvp1_(arithmetic|swizzle)$' --output-on-failure
```

Expected: both tests fail because the lowerer handles only connector moves.

- [ ] **Step 2: Add symbol-to-virtual-register ownership**

Use `Symbol.tempptr` as the ARB backend's mapping from local variables to a
heap-allocated `int` virtual TEMP index. At the start and end of
`ArbLowerProgram`, call `ClearAllSymbolTempptr`. Resolve a local variable like
this:

```c
static int GetSymbolTemp(ArbLowerContext *ctx, Symbol *symbol)
{
    int *index;
    if (symbol->tempptr)
        return *(int *) symbol->tempptr;
    index = (int *) malloc(sizeof(int));
    if (!index)
        return -1;
    *index = ArbNewTemp(ctx->ir);
    symbol->tempptr = index;
    return *index;
}
```

Track allocated index pointers in the lowering context and free them before
clearing symbol pointers. Never leave backend-owned pointers attached after
`ArbLowerProgram` returns.

- [ ] **Step 3: Implement the expression opcode map**

Lower the normalized AST using this exact map:

```text
FCONST/ICONST/BCONST scalar or vector -> interned ARB_REG_CONST
NEG/NEG_V                              -> toggle source negate
POS/POS_V and float casts              -> source unchanged
SWIZZLE_Z                              -> compose source swizzle
VECTOR_V                               -> MOV components into one new TEMP
ADD variants                           -> ADD
SUB variants                           -> SUB
MUL variants                           -> MUL
DIV with scalar divisor                -> RCP(right.x), then MUL(left, reciprocal)
DIV with vector divisor                -> masked RCP per used divisor component,
                                           then MUL(left, reciprocal TEMP)
LT variants                            -> SLT(left, right)
GE variants                            -> SGE(left, right)
GT variants                            -> SLT(right, left)
LE variants                            -> SGE(right, left)
EQ variants                            -> SGE both directions, MUL results
NE variants                            -> EQ sequence followed by SUB(1, result)
FUN_BUILTIN rsqrt                      -> RSQ of the first argument's x component
```

Reject bit shifts, modulus, bitwise integer operators, and any runtime integer
value that remains after loop lowering. Boolean `AND` multiplies 0/1 masks;
boolean `OR` adds then clamps with `MIN(value, 1)`; boolean NOT subtracts from
one.

For side-effect-free commutative expressions (`ADD`, `MUL`, `MAX`, `MIN`, and
recognized dot-product trees), compute a Sethi-Ullman cost and lower the
higher-cost subtree first while retaining the original operands in the emitted
instruction. Preserve source order for assignments, function calls, texture
operations, discard, and every noncommutative opcode.

- [ ] **Step 4: Add deterministic live intervals and linear scan**

In `ArbLegalizeAndAllocate`, number instructions from zero. Record first
definition and last source use for each component selected by destination masks
and source swizzles, then collapse each virtual TEMP's used components to one
full-register interval using the earliest first and latest last. Iterate
intervals by first definition, expire intervals whose last use is lower than the
current first definition, and assign the lowest free physical register. Do not
spill or pack unrelated values into separate lanes of one physical register.

Use these C90 records:

```c
typedef struct ArbInterval_Rec {
    int virtualTemp;
    int first;
    int last;
    int physical;
} ArbInterval;

typedef struct ArbActive_Rec {
    int interval;
    struct ArbActive_Rec *next;
} ArbActive;
```

Write the assigned physical index into each instruction's TEMP destination and
source operands. Set `program->numPhysicalTemps` to one plus the highest assigned
index. If more than `profile->limits->temporaries` simultaneously live intervals
exist, report C6005, declared in Task 5, and return `0`.

- [ ] **Step 5: Add safe local optimizations**

Before interval construction, perform these passes until one pass makes no
change:

```text
Remove MOV t, t with identical mask and swizzle.
Replace a TEMP source with its source when its only definition is a full-mask MOV,
the source has no relative addressing, and modifier composition remains legal.
Remove a TEMP-defining instruction whose written components have no later use.
Fold MUL(x, 1) and SUB(x, +0).
```

Do not fold ADD with zero or MUL with zero because signed-zero, infinity, and NaN
behavior would change. Do not reorder output writes or texture instructions.

- [ ] **Step 6: Create exact deterministic expectations**

Run the new compiler once for each fixture into the build tree:

```powershell
$cgc = (Resolve-Path build-arb/Debug/cgc.exe).Path
& $cgc -quiet -profile arbvp1 -o build-arb/vp_arithmetic.arb tests/arb/arithmetic/vp_arithmetic.cg
& $cgc -quiet -profile arbvp1 -o build-arb/vp_swizzle.arb tests/arb/arithmetic/vp_swizzle.cg
```

Expected: both commands exit `0`; output contains `RSQ`, `MUL`, `ADD`, masked
destination writes, source swizzles, `END`, and a TEMP count no greater than 3.
Copy the normalized text—removing only `# cgc version` and `# command line
args`—into the two `.expected` files using `apply_patch`. Re-run the compiler and
confirm `tests/run_arb_test.cmake` performs an exact match.

- [ ] **Step 7: Run arithmetic and regression coverage**

Run:

```powershell
cmake --build build-arb --config Debug
ctest --test-dir build-arb -C Debug -R '^(arbvp1_|arb_ir_unit|generic_)' --output-on-failure
```

Expected: every selected test passes.

- [ ] **Step 8: Commit arithmetic lowering and allocation**

```powershell
git add -- arb_lower.c arb_codegen.c arb_hal.c tests/CMakeLists.txt tests/arb/arithmetic
git commit -m "Lower ARB vertex arithmetic"
```

### Task 7: Pack uniforms, matrices, constants, and defaults

**Files:**
- Create: `tests/arb/uniforms/vp_matrix.cg`
- Create: `tests/arb/uniforms/vp_matrix.expected`
- Create: `tests/arb/uniforms/vp_bindings.cg`
- Create: `tests/arb/uniforms/vp_bindings.expected`
- Modify: `arb_hal.c`
- Modify: `arb_lower.c`
- Modify: `arb_codegen.c`
- Modify: `tests/CMakeLists.txt`

- [ ] **Step 1: Write failing matrix and pragma fixtures**

Create `tests/arb/uniforms/vp_matrix.cg`:

```c
float4 main(float4 position : POSITION,
            uniform float4x4 modelViewProjection) : POSITION
{
    return mul(modelViewProjection, position);
}
```

Create `tests/arb/uniforms/vp_bindings.cg`:

```c
#pragma bind main.scale = c 4
#pragma bind main.tint = default 1.0 1.0 1.0 1.0
#pragma bind main.bias = const 0.1 0.2 0.3 0.0

struct appin { float4 position : POSITION; float4 color : COLOR0; };
struct vout { float4 position : POSITION; float4 color : COLOR0; };

vout main(appin input, uniform float4 scale, uniform float4 tint,
          uniform float4 bias)
{
    vout output;
    output.position = input.position * scale;
    output.color = input.color * tint + bias;
    return output;
}
```

Register both fixtures and run them before implementation.

Expected: failure because unbound uniforms do not receive `BK_REGARRAY`
bindings and the lowerer cannot resolve uniform members.

- [ ] **Step 2: Allocate numeric uniforms deterministically**

Implement `BindUniformUnbound_arb` using `GetQuadRegSize(fSymb->type)`. Scan the
profile's `uniformUsed` bitmap from zero for the first contiguous free range,
reserve the range, and populate:

```c
fBind->reg.kind = BK_REGARRAY;
fBind->reg.properties = BIND_IS_BOUND | BIND_INPUT | BIND_UNIFORM;
fBind->reg.base = GetBase(fSymb->type);
fBind->reg.size = GetQuadRegSize(fSymb->type);
fBind->reg.rname = AddAtom(atable, "c");
fBind->reg.regno = first;
fBind->reg.count = count;
```

Return `0` without reserving a partial range when the profile's parameter limit
cannot hold the whole value, after reporting C6005 with resource name
`parameters`, the first-free base plus requested count, and the profile limit.

- [ ] **Step 3: Support explicit `c`, constant, and default pragmas**

In `BindUniformPragma_arb`:

```text
BK_REGARRAY named c: reserve exactly regno..regno+count-1 and copy the binding.
BK_CONSTANT: allocate a numeric range, create a BK_CONSTANT binding with
             NewConstDefaultBinding, and call AddConstantBinding.
BK_DEFAULT:  allocate a numeric range, create a BK_DEFAULT binding with
             NewConstDefaultBinding, and call AddDefaultBinding.
BK_TEXUNIT:  reject here until sampler support in Task 10.
Any connector or semantic binding: return 0.
```

Reject overlapping explicit ranges. Report C6005 when an explicit range ends
above the portable parameter limit; use the existing binding-conflict diagnostic
only for an in-range overlap. A default binding still receives a
`BK_REGARRAY` runtime location; its `BK_DEFAULT` record is additional metadata.

- [ ] **Step 4: Lower uniform accesses and decomposed matrices**

For a uniform variable or member with `BK_REGARRAY`, compute its vector offset
from `Symbol.details.var.addr >> 2` plus array/matrix indices and return
`ArbParamOperand(bind->reg.regno + offset)`. Constant indices use the existing
`ARRAY_INDEX_OP` shape.

The existing `DeconstructMatrices` and inlined `mul` functions leave dot-product
shapes. Recognize the scalar multiply/add chains created by `dot` only when every
term uses matching components from two source vectors; otherwise keep the legal
MUL/ADD sequence. Emit DP3 for three-component dots and DP4 for four-component
dots. Because ARB has no DP2, retain the exact two-component MUL/ADD sequence
instead of introducing extra instructions or a fabricated third component.

- [ ] **Step 5: Emit PARAM declarations in binding order**

Before instructions, emit one declaration covering the highest used numeric
uniform slot:

```text
PARAM c[5] = { program.local[0..4] };
```

Emit interned literal constants after numeric uniforms with stable names
`literal0`, `literal1`, and increasing indices. Do not count multiple uses of one
interned literal as multiple parameter bindings.

- [ ] **Step 6: Generate and check in exact expectations**

Run both fixtures to build-tree outputs, verify the matrix file contains four
`DP4` instructions and `PARAM c[4]`, and verify the pragma file maps `scale` to
`c[4]` plus a `#default main.tint` record. Normalize only volatile header lines
and add the complete outputs to the two expected files with `apply_patch`.

- [ ] **Step 7: Run uniform, arithmetic, and generic tests**

Run:

```powershell
cmake --build build-arb --config Debug
ctest --test-dir build-arb -C Debug -R '^(arbvp1_|arb_ir_unit|generic_)' --output-on-failure
```

Expected: all selected tests pass.

- [ ] **Step 8: Commit uniform and matrix support**

```powershell
git add -- arb_hal.c arb_lower.c arb_codegen.c tests/CMakeLists.txt tests/arb/uniforms
git commit -m "Add ARB vertex uniform bindings"
```

### Task 8: Convert conditionals and statically bounded loops

**Files:**
- Create: `tests/arb/control-flow/vp_conditional.cg`
- Create: `tests/arb/control-flow/vp_conditional.expected`
- Create: `tests/arb/control-flow/vp_static_loop.cg`
- Create: `tests/arb/control-flow/vp_static_loop.expected`
- Create: `tests/arb/control-flow/vp_static_while.cg`
- Create: `tests/arb/control-flow/vp_static_while.expected`
- Create: `tests/arb/control-flow/vp_static_do.cg`
- Create: `tests/arb/control-flow/vp_static_do.expected`
- Create: `tests/arb/diagnostics/dynamic_loop.cg`
- Create: `tests/arb/diagnostics/dynamic_loop.txt`
- Modify: `arb_lower.c`
- Modify: `tests/CMakeLists.txt`

- [ ] **Step 1: Add conditional, static-loop, and rejection fixtures**

Create `tests/arb/control-flow/vp_conditional.cg`:

```c
struct appin { float4 position : POSITION; float4 color : COLOR0; };
struct vout { float4 position : POSITION; float4 color : COLOR0; };

vout main(appin input, uniform float threshold)
{
    vout output;
    output.position = input.position;
    output.color = input.position.x >= threshold ? input.color : 0.0;
    return output;
}
```

Create `tests/arb/control-flow/vp_static_loop.cg`:

```c
float4 main(float4 position : POSITION) : POSITION
{
    float4 result = position;
    for (int i = 0; i < 3; i++)
        result.xyz = result.xyz * 2.0;
    return result;
}
```

Create `tests/arb/diagnostics/dynamic_loop.cg`:

```c
float4 main(float4 position : POSITION, uniform int count) : POSITION
{
    float4 result = position;
    for (int i = 0; i < count; i++)
        result.xyz = result.xyz * 2.0;
    return result;
}
```

Create `vp_static_while.cg` with a local `int i = 0`, a `while (i < 2)`
whose final body statement is `i++`, and one masked vector multiply before the
step. Create `vp_static_do.cg` with a local `int i = 0`, a `do` body with one
masked vector multiply and final `i++`, and condition `while (i < 2)`. Both return
POSITION and have complete expected outputs.

Create `tests/arb/diagnostics/dynamic_loop.txt`:

```text
error C6002
loop cannot be unrolled at compile time
```

Register the conditional, for-loop, while-loop, and do-loop sources as success
fixtures and the dynamic loop with `add_arb_failure`. Run all five.

Expected: conditional and static-loop compilation fail in lowering; the dynamic
loop fails without the stable C6002 diagnostic.

- [ ] **Step 2: Route rejection paths through the stable diagnostics**

Use the profile-specific diagnostics declared in Task 5. Unsupported normalized
statements use C6001 with the AST opcode name, and a loop whose trip count cannot
be proven uses C6002 at the loop's source location. Do not fall back to a generic
internal error for a source-level restriction.

- [ ] **Step 3: Lower flattened conditional assignments**

The existing `FlattenIfStatements` pass produces `ASSIGN_COND_OP`,
`ASSIGN_COND_V_OP`, `ASSIGN_COND_SV_OP`, or `ASSIGN_COND_GEN_OP`. Lower both
candidate values before the condition. For ARBVP1:

```text
mask = SGE(condition, 0.0)
inverse = SUB(1.0, mask)
leftPart = MUL(trueValue, mask)
rightPart = MUL(falseValue, inverse)
result = ADD(leftPart, rightPart)
```

Use scalar smearing when the condition is scalar and the value is a vector.
Preserve the assignment's destination mask.

- [ ] **Step 4: Evaluate canonical loop controls**

Add a C90 integer constant evaluator supporting literal integers, the active loop
symbol, unary plus/minus/not, and `+`, `-`, `*`, `/`, `%`, `<`, `<=`, `>`,
`>=`, `==`, `!=`, `&&`, and `||`. Division or modulus by zero returns failure.

Recognize `FOR_STMT` only when:

```text
init assigns one local int symbol a constant;
condition is evaluable from that symbol and constants;
step assigns the same symbol an evaluable new value;
each step changes the value;
the comparison and step direction prove that the condition must become false.
```

Accept canonical comparisons of the induction symbol with an integer constant:
`<` and `<=` require a positive step, `>` and `>=` require a negative step,
`!=` requires the step to move toward and exactly reach the bound, and `==` has
at most one iteration. Check every addition/subtraction for signed `int`
overflow before simulating.

For `WHILE_STMT` and `DO_STMT`, require an immediately preceding constant
initializer for the local induction symbol and a canonical update as the final
statement in the body. The `while` tests before its first body; the `do` tests
after its first body. Push an `ArbStaticValue` for each simulated induction value
and restore the prior stack on exit so nested canonical loops work. When
`LowerExpression` finds a symbol in this stack, return an interned constant
rather than a TEMP. Do not emit induction initialization or step instructions.
Any ambiguous write to the induction symbol, nonconstant bound/step, wrong
direction, overflow, or nonterminating form reports C6002.

Perform a recursive analysis pass over each statement list before IR emission to
pair while/do loops with their initializer and mark only the matched initializer
and update nodes as compile-time control. This prevents the ordinary assignment
lowerer from emitting them while leaving unrelated local integer assignments to
the normal unsupported-runtime-integer diagnostic.

- [ ] **Step 5: Create deterministic success expectations**

Compile all four success fixtures into the build tree. Verify conditional output
contains `SGE`, `SUB`, `MUL`, and `ADD`; verify the for loop contains three copies
of the masked multiply, and the while/do loops contain two copies each. No loop
fixture may contain an address register or emitted induction update. Add their
complete normalized outputs to the expected files with `apply_patch`.

- [ ] **Step 6: Run control-flow and diagnostic tests**

Run:

```powershell
cmake --build build-arb --config Debug
ctest --test-dir build-arb -C Debug -R '^(arbvp1_|arb_.*dynamic_loop|generic_)' --output-on-failure
```

Expected: all four success fixtures pass, the dynamic loop fails as expected,
and all generic tests pass.

- [ ] **Step 7: Commit static control-flow lowering**

```powershell
git add -- arb_lower.c tests/CMakeLists.txt tests/arb/control-flow tests/arb/diagnostics/dynamic_loop.*
git commit -m "Lower static ARB control flow"
```

### Task 9: Add vertex relative indexing and full ARBVP1 resource checks

**Files:**
- Create: `tests/arb/uniforms/vp_relative_index.cg`
- Create: `tests/arb/uniforms/vp_relative_index.expected`
- Create: `tests/arb/limits/vp_params_96.cg`
- Create: `tests/arb/limits/vp_params_96.expected`
- Create: `tests/arb/limits/vp_params_97.cg`
- Create: `tests/arb/limits/vp_params_97.txt`
- Create: `tests/arb/limits/vp_instructions_128.cg`
- Create: `tests/arb/limits/vp_instructions_128.expected`
- Create: `tests/arb/limits/vp_instructions_129.cg`
- Create: `tests/arb/limits/vp_instructions_129.txt`
- Modify: `arb_lower.c`
- Modify: `arb_codegen.c`
- Modify: `tests/arb_ir_tests.c`
- Modify: `tests/CMakeLists.txt`

- [ ] **Step 1: Write a failing relative-index fixture**

Create `tests/arb/uniforms/vp_relative_index.cg`:

```c
float4 main(float4 position : POSITION,
            uniform float4 offsets[4],
            uniform int index) : POSITION
{
    return position + offsets[index];
}
```

Register it as `arbvp1_relative_index` and run it.

Expected: lowering fails because nonconstant `ARRAY_INDEX_OP` has no address
register representation.

- [ ] **Step 2: Represent and emit relative PARAM operands**

When the left side of `ARRAY_INDEX_OP` resolves to a `BK_REGARRAY` uniform:

```text
constant index: add the constant to the base PARAM index;
vertex nonconstant index: lower the scalar index; when base is in -64..63 emit
                          ARL A0.x, index.x and use that legal relative offset;
                          otherwise ADD the base to the index before ARL and use
                          relative offset zero;
fragment nonconstant index: emit C6003 and fail.
```

Extend `WriteOperand` so a relative parameter prints as:

```text
c[A0.x + 0]
```

Use `c[A0.x - n]` for a negative offset and omit `+ 0` only if the exact-output
fixtures establish that convention consistently. Validate the base grammar's
relative-offset range of -64 through 63 in `ArbValidateIR`; the rebase sequence
ensures larger uniform layouts never emit an illegal operand. `ArbWriteProgram`
emits `ADDRESS A0;` before `PARAM` declarations whenever an `ARB_OP_ARL` exists.

- [ ] **Step 3: Account for every vertex resource**

Implement `ArbValidateResources` for vertex programs with these counters:

```c
resources.instructions = program->numInstructions;
resources.temporaries = program->numPhysicalTemps;
resources.parameters = highestParameterIndex + 1 + program->numConstants;
resources.attributes = CountDistinctInputRegisters(program);
resources.addressRegisters = ProgramUsesOpcode(program, ARB_OP_ARL) ? 1 : 0;
```

Compare each field to `profile->limits`. Report C6005 with resource names
`instructions`, `temporaries`, `parameters`, `attributes`, or
`address registers` and the actual and allowed numbers.

- [ ] **Step 4: Extend the pure IR test with exact TEMP boundaries**

In `tests/arb_ir_tests.c`, construct a second program whose first twelve
instructions define virtual TEMP 0 through 11 and whose next twelve instructions
use each TEMP, making all twelve intervals overlap. Assert allocation succeeds
with the ARBVP1 limit. Add a thirteenth overlapping interval and assert
allocation returns `ARB_ALLOC_TEMP_LIMIT`.

Expose a pure allocator result enum in `arb_ir.h`:

```c
typedef enum ArbAllocStatus_Enum {
    ARB_ALLOC_OK,
    ARB_ALLOC_TEMP_LIMIT,
    ARB_ALLOC_INVALID_IR
} ArbAllocStatus;

ArbAllocStatus ArbAllocateTemporaries(ArbProgram *program, int maxTemporaries);
```

Move the interval allocator from `arb_codegen.c` to `arb_ir.c` so the unit target
can test it without linking compiler globals. `arb_codegen.c` remains the caller
responsible for converting allocation status into diagnostics.

- [ ] **Step 5: Add parameter-boundary fixtures**

Create `vp_params_96.cg` with `uniform float4 values[96]`, read
`values[95]`, and return the sum with POSITION. Create `vp_params_97.cg` with
`uniform float4 values[97]` and the same final element access. The failure
expectation contains:

```text
error C6005
parameters resource use 97 exceeds portable limit 96
```

Run both tests before adding exact success output.

Expected: the 96-vector program succeeds; the 97-vector program fails with
C6005 and writes no `#var` metadata after the volatile header comments.

- [ ] **Step 6: Add exact instruction-boundary fixtures**

Use preprocessor expansion to keep the source readable while producing a fixed
number of output writes:

```c
#define WRITE1 output = position;
#define WRITE2 WRITE1 WRITE1
#define WRITE4 WRITE2 WRITE2
#define WRITE8 WRITE4 WRITE4
#define WRITE16 WRITE8 WRITE8
#define WRITE32 WRITE16 WRITE16
#define WRITE64 WRITE32 WRITE32

void main(float4 position : POSITION, out float4 output : POSITION)
{
    WRITE64 WRITE32 WRITE16 WRITE8 WRITE4 WRITE2 WRITE1
}
```

The shown body produces 127 writes. Add one explicit `output = position;` for
the 128-instruction success file and two for the 129-instruction failure file.
The failure expectation contains:

```text
error C6005
instructions resource use 129 exceeds portable limit 128
```

Do not dead-code output writes in the optimizer; they are externally visible IR
instructions for resource accounting.

- [ ] **Step 7: Check in deterministic success outputs and run limits**

Generate normalized exact outputs for `vp_relative_index`, `vp_params_96`, and
`vp_instructions_128` into their expected files with `apply_patch`. Verify the
relative-index output contains one `ADDRESS A0;`, one `ARL`, and a relative
`c[A0.x + n]` read.

Run:

```powershell
cmake --build build-arb --config Debug
ctest --test-dir build-arb -C Debug -R '^(arbvp1_|arb_.*vp_(params|instructions)|arb_ir_unit|generic_)' --output-on-failure
```

Expected: every boundary success passes, each one-over case fails as expected,
the IR unit test passes, and generic behavior is unchanged.

- [ ] **Step 8: Commit relative addressing and vertex limits**

```powershell
git add -- arb_ir.h arb_ir.c arb_lower.c arb_codegen.c tests/arb_ir_tests.c tests/CMakeLists.txt tests/arb/uniforms/vp_relative_index.* tests/arb/limits/vp_*
git commit -m "Validate ARB vertex program resources"
```

### Task 10: Register fragment sampler types and intrinsics

**Files:**
- Create: `tests/arb/profile/fp_sampler_nocode.cg`
- Modify: `arb_hal.h`
- Modify: `arb_hal.c`
- Modify: `arbfp1_hal.c`
- Modify: `stdlib.cg:54-103`
- Regenerate: `stdlib.c`
- Modify: `tests/CMakeLists.txt`

- [ ] **Step 1: Add a failing sampler type test**

Create `tests/arb/profile/fp_sampler_nocode.cg`:

```c
float4 main(float2 texcoord : TEXCOORD0,
            uniform sampler2D image : TEXUNIT0) : COLOR
{
    return tex2D(image, texcoord);
}
```

Register a direct `-nocode` CTest invocation named `arbfp1_sampler_types`.

Run:

```powershell
cmake --build build-arb --config Debug
ctest --test-dir build-arb -C Debug -R '^arbfp1_sampler_types$' --output-on-failure
```

Expected: failure with an unknown `sampler2D` type or `tex2D` function.

- [ ] **Step 2: Register the five sampler base types**

In the fragment branch of `RegisterNames_arb`, call this helper for each type ID
defined in `arb_hal.h`:

```c
static void RegisterSamplerType(const char *name, int base)
{
    SourceLoc loc = { 0, 0 };
    Type *type = NewType(TYPE_CATEGORY_SCALAR | base, 1);
    int atom = LookUpAddString(atable, name);

    SetScalarTypeName(base, atom, type);
    AddSymbol(&loc, CurrentScope, atom, type, TYPEDEF_S);
}
```

Register `sampler1D`, `sampler2D`, `sampler3D`, `samplerCUBE`, and
`samplerRECT`. `IsTexobjBase_arb` returns true for exactly these five bases.
`IsValidRuntimeBase_arb` permits float, bool, int, and these sampler bases so the
preserved frontend can type-check static integer expressions. The lowerer must
reject any integer value that survives constant folding and static-loop lowering
as a runtime arithmetic value.

- [ ] **Step 3: Add profile-guarded standard-library declarations**

Append this block to `stdlib.cg`:

```c
#ifdef PROFILE_ARBFP1
__internal float4 tex1D(sampler1D image, float coord);
__internal float4 tex1Dproj(sampler1D image, float2 coord);
__internal float4 tex1Dbias(sampler1D image, float4 coord);

__internal float4 tex2D(sampler2D image, float2 coord);
__internal float4 tex2Dproj(sampler2D image, float3 coord);
__internal float4 tex2Dproj(sampler2D image, float4 coord);
__internal float4 tex2Dbias(sampler2D image, float4 coord);

__internal float4 tex3D(sampler3D image, float3 coord);
__internal float4 tex3Dproj(sampler3D image, float4 coord);
__internal float4 tex3Dbias(sampler3D image, float4 coord);

__internal float4 texCUBE(samplerCUBE image, float3 coord);
__internal float4 texCUBEproj(samplerCUBE image, float4 coord);
__internal float4 texCUBEbias(samplerCUBE image, float4 coord);

__internal float4 texRECT(samplerRECT image, float2 coord);
__internal float4 texRECTproj(samplerRECT image, float3 coord);
#endif
```

- [ ] **Step 4: Map texture declarations to the stable built-in IDs**

Extend `CheckInternalFunction_arb` to return the matching `ArbBuiltin` declared
in Task 3 after comparing the function atom string. Keep `rsqrt` enabled for both
stages, recognize texture names only in ARBFP1, and set
`*group = ARB_BUILTIN_GROUP` only for a recognized function. Return `0` for any
other internal declaration.

- [ ] **Step 5: Regenerate the checked-in standard library**

Run:

```powershell
cmake --build build-arb --target regenerate_stdlib --config Debug
git diff --check -- stdlib.cg stdlib.c
```

Expected: regeneration succeeds; `stdlib.c` changes deterministically and the
diff has no whitespace errors. Run the regeneration target a second time and
confirm `git diff --stat` does not grow.

- [ ] **Step 6: Run sampler parsing and generic regressions**

Run:

```powershell
cmake --build build-arb --config Debug
ctest --test-dir build-arb -C Debug -R '^(arbfp1_sampler_types|generic_)' --output-on-failure
```

Expected: the sampler test and all four generic tests pass. The profile macro
keeps sampler declarations out of the generic standard-library parse.

- [ ] **Step 7: Commit sampler types and declarations**

```powershell
git add -- arb_hal.h arb_hal.c arbfp1_hal.c stdlib.cg stdlib.c tests/CMakeLists.txt tests/arb/profile/fp_sampler_nocode.cg
git commit -m "Add ARB fragment sampler types"
```

### Task 11: Emit ARBFP1 arithmetic and conditional programs

**Files:**
- Create: `tests/arb/profile/fp_passthrough.expected`
- Create: `tests/arb/arithmetic/fp_conditional.cg`
- Create: `tests/arb/arithmetic/fp_conditional.expected`
- Modify: `arb_lower.c`
- Modify: `arb_codegen.c`
- Modify: `tests/CMakeLists.txt`

- [ ] **Step 1: Add exact fragment output tests**

Create `tests/arb/profile/fp_passthrough.expected`:

```text
!!ARBfp1.0
#vendor NVIDIA Corporation
#version 1.0
#profile arbfp1
#program main
#var float4 color : $vin.COLOR0 : COL0 : 0 : 1
#var float4 main : $vout.COLOR : COL : -1 : 1
MOV result.color, fragment.color.primary;
END
# 1 instructions, 0 R-regs
```

Create `tests/arb/arithmetic/fp_conditional.cg`:

```c
float4 main(float4 color : COLOR0, uniform float threshold) : COLOR
{
    return color.a >= threshold ? color : float4(0.0, 0.0, 0.0, 1.0);
}
```

Register `arbfp1_passthrough` and `arbfp1_conditional`, initially pointing the
conditional test at a missing expectation. Run both.

Expected: pass-through differs from the Task 3 no-op output; conditional lowering
uses the vertex select sequence rather than fragment `CMP`.

- [ ] **Step 2: Complete fragment register spelling**

Map fragment connector register numbers exactly as follows:

```text
WPOS -> fragment.position
COL0 -> fragment.color.primary
COL1 -> fragment.color.secondary
FOGC -> fragment.fogcoord
TEX0..TEX7 -> fragment.texcoord[0]..fragment.texcoord[7]
COL -> result.color
DEPR -> result.depth
```

Make `OpcodeName` stage-aware. The fragment table accepts `ABS`, `ADD`, `CMP`,
`COS`, `DP3`, `DP4`, `DPH`, `DST`, `EX2`, `FLR`, `FRC`, `KIL`, `LG2`, `LIT`,
`LRP`, `MAD`, `MAX`, `MIN`, `MOV`, `MUL`, `POW`, `RCP`, `RSQ`, `SCS`, `SGE`,
`SIN`, `SLT`, `SUB`, `SWZ`, `TEX`, `TXB`, `TXP`, and `XPD`. It rejects vertex
`ARL`, `EXP`, and `LOG` spellings.

- [ ] **Step 3: Select native fragment CMP**

For flattened conditional assignments in ARBFP1, lower the condition to a signed
comparison value and emit:

```text
CMP destination, condition, falseValue, trueValue;
```

Normalize boolean comparisons so true is nonnegative and false is negative
before CMP. Preserve scalar smearing and write masks. Both candidate expressions
must already be lowered, so their side effects occur unconditionally.

- [ ] **Step 4: Apply fragment resource accounting to arithmetic**

Count every non-texture instruction as ALU. Count `KIL`, `TEX`, `TXB`, and `TXP`
as texture instructions. Total instructions are ALU plus texture instructions.
Use the fragment limits in the profile descriptor and report C6005 for each
overflow class.

- [ ] **Step 5: Generate the conditional expectation and run both stages**

Compile `fp_conditional.cg`, verify it contains `CMP` and no vertex-only `ARL`,
then add its complete normalized output with `apply_patch`.

Run:

```powershell
cmake --build build-arb --config Debug
ctest --test-dir build-arb -C Debug -R '^(arb(vp|fp)1_|arb_ir_unit|generic_)' --output-on-failure
```

Expected: all ARBVP1, ARBFP1, IR, and generic tests pass.

- [ ] **Step 6: Commit fragment arithmetic emission**

```powershell
git add -- arb_lower.c arb_codegen.c tests/CMakeLists.txt tests/arb/profile/fp_passthrough.expected tests/arb/arithmetic/fp_conditional.*
git commit -m "Emit ARB fragment arithmetic"
```

### Task 12: Lower texture sampling, projection, bias, and discard

**Files:**
- Create: `tests/arb/textures/fp_tex2d.cg`
- Create: `tests/arb/textures/fp_tex2d.expected`
- Create: `tests/arb/textures/fp_targets.cg`
- Create: `tests/arb/textures/fp_target_1d.expected`
- Create: `tests/arb/textures/fp_target_3d.expected`
- Create: `tests/arb/textures/fp_target_cube.expected`
- Create: `tests/arb/textures/fp_target_rect.expected`
- Create: `tests/arb/textures/fp_target_projected_biased.expected`
- Create: `tests/arb/textures/fp_dependent.cg`
- Create: `tests/arb/textures/fp_dependent.expected`
- Create: `tests/arb/textures/fp_alu_dependent.cg`
- Create: `tests/arb/textures/fp_alu_dependent.expected`
- Create: `tests/arb/control-flow/fp_discard.cg`
- Create: `tests/arb/control-flow/fp_discard.expected`
- Create: `tests/arb/diagnostics/fp_texture_conflict.cg`
- Create: `tests/arb/diagnostics/fp_texture_conflict.txt`
- Modify: `arb_hal.c`
- Modify: `arb_lower.c`
- Modify: `arb_codegen.c`
- Modify: `tests/CMakeLists.txt`

- [ ] **Step 1: Write the basic texture fixture**

Create `tests/arb/textures/fp_tex2d.cg`:

```c
float4 main(float2 texcoord : TEXCOORD0,
            uniform sampler2D image : TEXUNIT0) : COLOR
{
    return tex2D(image, texcoord);
}
```

Register it as `arbfp1_tex2d` and run it.

Expected: failure because `BK_TEXUNIT` binding and texture built-in lowering are
not implemented.

- [ ] **Step 2: Bind explicit and implicit sampler units**

In `BindUniformPragma_arb`, accept `BK_TEXUNIT` only for a sampler base. Verify
`unitno` is in `0..profile->limits->textureUnits-1`. Convert the sampler base to
an `ArbTextureTarget`, reject a conflicting target already recorded for the unit,
and populate `fBind->texunit` with `BIND_IS_BOUND | BIND_INPUT | BIND_UNIFORM`.
An explicit unit above the portable range reports C6005 with resource name
`texture units`, actual value `unitno + 1`, and the profile limit before indexing
the fixed-size target array.

In `BindUniformUnbound_arb`, allocate samplers from the first unclaimed unit and
record the target. If both portable units are claimed, report the existing
`ERROR_S_NO_TEXUNITS_AVAILABLE` and return `0`.

- [ ] **Step 3: Lower texture built-ins**

Walk the `FUN_ARG_OP` list into a fixed two-element operand array. The first
argument must be a symbol with `BK_TEXUNIT`; the second is the coordinate source.
Use this map:

```text
tex1D/tex2D/tex3D/texCUBE/texRECT                         -> TEX
tex1Dproj/tex2Dproj/tex3Dproj/texCUBEproj/texRECTproj    -> TXP
tex1Dbias/tex2Dbias/tex3Dbias/texCUBEbias                -> TXB
```

Set `instruction->textureUnit` and `instruction->textureTarget`, emit to a new
virtual TEMP, and return that TEMP operand. Confirm coordinate vector widths are
at least 1, 2, 3, 3, and 2 for 1D, 2D, 3D, cube, and rectangle respectively.

- [ ] **Step 4: Add target-family coverage**

Create `fp_targets.cg` with five entry functions in separate preprocessor
branches selected by `-DTEST_1D`, `TEST_3D`, `TEST_CUBE`, `TEST_RECT`, and
`TEST_PROJECTED_BIASED`. Pass optional compiler arguments through the existing
fixture helper by adding this argument to its `COMMAND` list:

```cmake
"-DEXTRA_ARGS=${ARGN}"
```

Register one CTest fixture per branch, for example:

```cmake
add_arb_fixture(arbfp1_target_1d arbfp1 tests/arb/textures/fp_targets.cg
    tests/arb/textures/fp_target_1d.expected -DTEST_1D)
```

Repeat for the other four branches and their correspondingly named expectation
files. Cover every built-in declared in Task 10 at least once across the
branches. Each expected output must contain the correct `TEX`, `TXP`, or `TXB`
opcode and `1D`, `2D`, `3D`, `CUBE`, or `RECT` target token.

- [ ] **Step 5: Lower fragment discard to KIL**

Create `tests/arb/control-flow/fp_discard.cg`:

```c
float4 main(float4 color : COLOR0) : COLOR
{
    discard color.a < 0.5;
    return color;
}
```

Lower `DISCARD_STMT` and normalized `KILL_OP` by lowering the condition and
emitting a destination-free `ARB_OP_KIL`. If the condition is a 0/1 boolean,
convert it to a signed value where a negative component means discard. Reject
discard during ARBVP1 checking with C6006.

- [ ] **Step 6: Compute specification-defined texture indirections**

Run the ARB_fragment_program node algorithm after TEMP allocation, using bitsets
for the at-most-16 physical TEMP registers. Initialize `indirections = 1`,
`textureOutputs = 0`, and `aluTemps = 0`. For each instruction in source order:

```text
texture instruction (TEX/TXB/TXP/KIL):
    start a new node when its coordinate TEMP is in textureOutputs, or when its
    non-KIL destination TEMP is in aluTemps; on a new node, increment
    indirections and clear both bitsets
ALU instruction:
    add every TEMP source and TEMP destination to aluTemps
after either class:
    add each non-KIL TEMP destination to textureOutputs
```

Thus a program with no texture instruction still reports one indirection, as the
specification requires. `KIL` contributes one texture instruction but has no
destination.

Set `resources.textureUnits` to zero when no sampling instruction exists, or one
plus the highest referenced `textureUnit` otherwise; the portable limit bounds
the legal index range, not merely the number of distinct units.

Create `fp_dependent.cg` with a chain of four `tex2D` reads whose later
coordinates come from the preceding sample; it must report four indirections and
pass. Create `fp_alu_dependent.cg` with an ALU-computed TEMP coordinate followed
by a single texture read; it must report two indirections and pass. Create a
five-read chain in Task 13 that fails the portable limit.

- [ ] **Step 7: Reject target conflicts**

Create `fp_texture_conflict.cg` with a `sampler2D` and `samplerCUBE` both bound to
`TEXUNIT0`, and create this expectation:

```text
error C6004
texture unit 0 already has target
```

Run the conflict test and verify failure occurs before `#var` metadata.

- [ ] **Step 8: Generate expectations and run texture coverage**

Add exact normalized output for basic sampling, all target branches, the
four-read chain, the ALU-dependent read, and discard. Run:

```powershell
cmake --build build-arb --config Debug
ctest --test-dir build-arb -C Debug -R '^(arbfp1_(tex|discard)|arb_.*texture_conflict|generic_)' --output-on-failure
```

Expected: all valid texture/discard fixtures pass, the conflict fixture fails as
expected, and generic regressions pass.

- [ ] **Step 9: Commit fragment texture support**

```powershell
git add -- arb_hal.c arb_lower.c arb_codegen.c tests/CMakeLists.txt tests/arb/textures tests/arb/control-flow/fp_discard.* tests/arb/diagnostics/fp_texture_conflict.*
git commit -m "Add ARB fragment texture operations"
```

### Task 13: Complete semantic diagnostics and exact resource boundaries

**Files:**
- Create: `tests/arb/vp-semantics/vp_all_semantics.cg`
- Create: `tests/arb/vp-semantics/vp_all_semantics_conventional.expected`
- Create: `tests/arb/vp-semantics/vp_all_semantics_generic.expected`
- Create: `tests/arb/fp-semantics/fp_all_semantics.cg`
- Create: `tests/arb/fp-semantics/fp_all_semantics_primary.expected`
- Create: `tests/arb/fp-semantics/fp_all_semantics_secondary.expected`
- Create: `tests/arb/diagnostics/vp_missing_position.cg`
- Create: `tests/arb/diagnostics/vp_missing_position.txt`
- Create: `tests/arb/diagnostics/vp_attribute_alias.cg`
- Create: `tests/arb/diagnostics/vp_attribute_alias.txt`
- Create: `tests/arb/diagnostics/vp_discard.cg`
- Create: `tests/arb/diagnostics/vp_discard.txt`
- Create: `tests/arb/diagnostics/vp_clip_output.cg`
- Create: `tests/arb/diagnostics/vp_clip_output.txt`
- Create: `tests/arb/diagnostics/fp_dynamic_index.cg`
- Create: `tests/arb/diagnostics/fp_dynamic_index.txt`
- Create: `tests/arb/limits/fp_params_24.cg`
- Create: `tests/arb/limits/fp_params_24.expected`
- Create: `tests/arb/limits/fp_params_25.cg`
- Create: `tests/arb/limits/fp_params_25.txt`
- Create: `tests/arb/limits/fp_attributes_10.cg`
- Create: `tests/arb/limits/fp_attributes_10.expected`
- Create: `tests/arb/limits/fp_attributes_11.cg`
- Create: `tests/arb/limits/fp_attributes_11.txt`
- Create: `tests/arb/limits/fp_alu_48.cg`
- Create: `tests/arb/limits/fp_alu_48.expected`
- Create: `tests/arb/limits/fp_alu_49.cg`
- Create: `tests/arb/limits/fp_alu_49.txt`
- Create: `tests/arb/limits/fp_tex_24.cg`
- Create: `tests/arb/limits/fp_tex_24.expected`
- Create: `tests/arb/limits/fp_tex_25.cg`
- Create: `tests/arb/limits/fp_tex_25.txt`
- Create: `tests/arb/limits/fp_indirections_5.cg`
- Create: `tests/arb/limits/fp_indirections_5.txt`
- Create: `tests/arb/limits/fp_texture_units_2.cg`
- Create: `tests/arb/limits/fp_texture_units_2.expected`
- Create: `tests/arb/limits/fp_texture_units_3.cg`
- Create: `tests/arb/limits/fp_texture_units_3.txt`
- Modify: `arb_hal.c`
- Modify: `arbvp1_hal.c`
- Modify: `arbfp1_hal.c`
- Modify: `arb_ir.h`
- Modify: `arb_ir.c`
- Modify: `arb_lower.c`
- Modify: `arb_codegen.c`
- Modify: `tests/arb_ir_tests.c`
- Modify: `tests/run_arb_test.cmake`
- Modify: `tests/CMakeLists.txt`

- [ ] **Step 1: Make every expected failure assert atomic output**

Extend the failure branch of `tests/run_arb_test.cmake`. If the compiler created
an output file, read it and reject any backend material after the allowed header
and volatile comments:

```cmake
if(EXISTS "${output_file}")
    file(READ "${output_file}" failed_output)
    foreach(forbidden "#var " "#const " "#default " "TEMP " "PARAM "
                      "ATTRIB " "OUTPUT " "ADDRESS " "END")
        string(FIND "${failed_output}" "${forbidden}" found)
        if(NOT found EQUAL -1)
            message(FATAL_ERROR
                "${TEST_NAME} left partial backend output containing ${forbidden}"
            )
        endif()
    endforeach()
endif()
```

Run the existing dynamic-loop and texture-conflict failures. Expected: both
still pass, proving their failures leave no binding metadata or instruction
stream.

- [ ] **Step 2: Cover every accepted semantic mapping**

Create `vp_all_semantics.cg` with separate input and output structures. Reference
every field so none are optimized away. Cover conventional inputs `POSITION`,
`BLENDWEIGHT`, `NORMAL`, `COLOR0`, `COLOR1`, `FOG`, and `TEXCOORD0..7`; cover
outputs `POSITION`, `COLOR0`, `COLOR1`, `BCOL0`, `BCOL1`, `FOG`, `PSIZE`, and
`TEXCOORD0..7`. Use generic `ATTR0..ATTR15` in a separate preprocessor branch so
the fixture never mixes the two aliasing namespaces in one program. Compare the
default conventional branch against `vp_all_semantics_conventional.expected`
and the `-DTEST_GENERIC` branch against `vp_all_semantics_generic.expected`.

Create `fp_all_semantics.cg` with inputs `POSITION`, `COLOR0`, `COLOR1`, `FOG`,
and `TEXCOORD0..7`, and outputs `COLOR` and `DEPTH`. Use two preprocessor branches
so each invocation reads at most the portable ten-attribute limit. Compare the
default branch against `fp_all_semantics_primary.expected` and the
`-DTEST_SECONDARY` branch against `fp_all_semantics_secondary.expected`, and
verify the exact input/output spellings from Tasks 3 and 11.

- [ ] **Step 3: Add required-output, aliasing, and stage diagnostics**

Use these minimal sources and diagnostic fragments:

```c
/* vp_missing_position.cg */
float4 main(float4 color : COLOR0) : COLOR0 { return color; }

/* vp_attribute_alias.cg */
float4 main(float4 position : POSITION, float4 same : ATTR0) : POSITION
{ return position + same; }

/* vp_discard.cg */
float4 main(float4 position : POSITION) : POSITION
{ discard position.x < 0.0; return position; }

/* vp_clip_output.cg */
float4 main(float4 position : POSITION) : CLP0 { return position; }

/* fp_dynamic_index.cg */
float4 main(float4 color : COLOR0, uniform float4 values[2], uniform int i) : COLOR
{ return color + values[i]; }
```

The corresponding `.txt` files contain:

```text
vp_missing_position.txt: error C6008 | must write POSITION
vp_attribute_alias.txt:  error C6009 | generic and conventional vertex attributes alias
vp_discard.txt:          error C6006 | discard is not supported
vp_clip_output.txt:      error C5108 | CLP0
fp_dynamic_index.txt:    error C6003 | requires a constant uniform index
```

Store each `|`-separated fragment on its own line in the real files. Make the
alias checker compare resolved numeric input registers, not semantic spelling,
and run it after all varying bindings are known. Check required connector writes
after lowering and before metadata emission. Retain C6007 as the defensive
backend diagnostic for a vertex-stage texture IR instruction, even though the
profile-guarded standard library normally prevents source programs from reaching
that case.

- [ ] **Step 4: Factor pure resource comparison and test all nine counters**

Add these private, compiler-global-free definitions to `arb_ir.h`:

```c
typedef struct ArbResources_Rec {
    int instructions, aluInstructions, texInstructions, texIndirections;
    int temporaries, parameters, attributes, addressRegisters, textureUnits;
} ArbResources;

typedef enum ArbResourceStatus_Enum {
    ARB_RESOURCE_OK,
    ARB_RESOURCE_INSTRUCTIONS,
    ARB_RESOURCE_ALU_INSTRUCTIONS,
    ARB_RESOURCE_TEX_INSTRUCTIONS,
    ARB_RESOURCE_TEX_INDIRECTIONS,
    ARB_RESOURCE_TEMPORARIES,
    ARB_RESOURCE_PARAMETERS,
    ARB_RESOURCE_ATTRIBUTES,
    ARB_RESOURCE_ADDRESS_REGISTERS,
    ARB_RESOURCE_TEXTURE_UNITS
} ArbResourceStatus;

ArbResourceStatus ArbCheckResourceLimits(const ArbResources *resources,
                                          const ArbLimits *limits,
                                          int *actual, int *allowed);
```

Implement it in enum order and return the first overflow, setting `actual` and
`allowed`. In `tests/arb_ir_tests.c`, define local `ArbLimits` values from the
`ARBVP_*` and `ARBFP_*` macros rather than linking the stage HAL modules, then
define one table entry for every field in both profiles. For each nonzero limit,
set the field to the exact limit and
assert `ARB_RESOURCE_OK`, then set it to limit plus one and assert the matching
status and returned values. This gives direct boundary coverage for vertex
instructions, temporaries, parameters, attributes, and address registers, and
fragment total/ALU/texture instructions, indirections, temporaries, parameters,
attributes, and texture units. `ArbValidateResources` converts the returned enum
to the existing C6005 resource name.

- [ ] **Step 5: Add integrated parameter, attribute, and indexing limits**

Use a `uniform float4 values[N]` macro fixture to compile exactly 24 fragment
parameters and reject 25. Initialize the accumulator from `values[0]` and add
the remaining elements so allocation cannot discard them and no literal PARAM
changes the boundary. The failure file contains:

```text
error C6005
parameters resource use 25 exceeds portable limit 24
```

For attributes, make the ten-input success case read `POSITION`, `COLOR0`, and
`TEXCOORD0..7`; make the eleven-input failure add `COLOR1`. Sum at least one
component from every input into `COLOR`. The failure file contains:

```text
error C6005
attributes resource use 11 exceeds portable limit 10
```

Register these four fixtures plus `fp_dynamic_index` and run them before adding
the two complete success expectations.

- [ ] **Step 6: Add integrated fragment instruction and texture boundaries**

Build the checked-in sources with doubling macros, as in Task 9, so the emitted
IR counts are auditable:

```text
fp_alu_48:  48 externally visible ALU writes, succeeds
fp_alu_49:  49 externally visible ALU writes, C6005 "ALU instructions"
fp_tex_24:  24 independent TEX results accumulated into COLOR, succeeds
fp_tex_25:  25 independent TEX results accumulated into COLOR, C6005 "texture instructions"
```

Do not let dead-code elimination erase output writes or used texture reads. The
four-dependent-read fixture from Task 12 is the exact indirection success case;
`fp_indirections_5.cg` adds one dependent read and expects:

```text
error C6005
texture indirections resource use 5 exceeds portable limit 4
```

Bind two `sampler2D` parameters to `TEXUNIT0` and `TEXUNIT1` for the exact unit
success case. Bind a third to `TEXUNIT2` for the failure case and expect:

```text
error C6005
texture units resource use 3 exceeds portable limit 2
```

The pure resource table from Step 4 also tests total fragment instructions at 72
and 73; an integrated 73-instruction program would necessarily violate the
independently lower ALU or texture maximum first, so it is intentionally not a
redundant source fixture.

- [ ] **Step 7: Generate exact successes and run the conformance matrix**

Compile every new success input into the build tree, inspect instruction/resource
statistics, and add complete normalized expectations using `apply_patch`.

Run:

```powershell
cmake --build build-arb --config Debug
ctest --test-dir build-arb -C Debug -R '^(arb(vp|fp)1_|arb_.*(semantic|limit|diagnostic)|arb_ir_unit|generic_)' --output-on-failure
```

Expected: all accepted semantic and exact-limit cases pass, all one-over and
semantic-error cases fail with their stable fragments, the IR boundary table
passes, and generic hashes remain unchanged.

- [ ] **Step 8: Commit semantic and limit completion**

```powershell
git add -- arb_hal.c arbvp1_hal.c arbfp1_hal.c arb_ir.h arb_ir.c arb_lower.c arb_codegen.c tests/CMakeLists.txt tests/run_arb_test.cmake tests/arb_ir_tests.c tests/arb/vp-semantics tests/arb/fp-semantics tests/arb/diagnostics tests/arb/limits
git commit -m "Complete ARB profile conformance checks"
```

### Task 14: Add the dependency-free Windows/WGL load smoke test

**Files:**
- Create: `tests/arb_smoke.c`
- Modify: `CMakeLists.txt`
- Modify: `tests/CMakeLists.txt`

- [ ] **Step 1: Register smoke tests before the executable exists**

Add this guarded block to `tests/CMakeLists.txt`:

```cmake
if(WIN32)
    set_tests_properties(arbvp1_passthrough PROPERTIES
        FIXTURES_SETUP arbvp1_smoke_program)
    add_test(NAME arbvp1_wgl_smoke
        COMMAND $<TARGET_FILE:arb_smoke> vp
            ${CMAKE_CURRENT_BINARY_DIR}/arbvp1_passthrough.arb)
    set_tests_properties(arbvp1_wgl_smoke PROPERTIES
        FIXTURES_REQUIRED arbvp1_smoke_program
        SKIP_RETURN_CODE 77)

    set_tests_properties(arbfp1_tex2d PROPERTIES
        FIXTURES_SETUP arbfp1_smoke_program)
    add_test(NAME arbfp1_wgl_smoke
        COMMAND $<TARGET_FILE:arb_smoke> fp
            ${CMAKE_CURRENT_BINARY_DIR}/arbfp1_tex2d.arb)
    set_tests_properties(arbfp1_wgl_smoke PROPERTIES
        FIXTURES_REQUIRED arbfp1_smoke_program
        SKIP_RETURN_CODE 77)
endif()
```

The fixture runner writes `${TEST_NAME}.arb` into the tests binary directory,
so each setup fixture creates the exact path consumed by its smoke test. CTest
automatically includes fixture setup tests even when a developer selects only
the `*_wgl_smoke` tests with `-R`.

Run:

```powershell
cmake -S . -B build-arb -A Win32 -DBUILD_TESTING=ON
```

Expected: generation fails because target `arb_smoke` does not exist.

- [ ] **Step 2: Add the C90 WGL executable skeleton**

Create `tests/arb_smoke.c` with the repository license, these includes, and local
definitions for extension tokens absent from the platform OpenGL 1.1 header:

```c
#include <windows.h>
#include <GL/gl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define GL_VERTEX_PROGRAM_ARB          0x8620
#define GL_FRAGMENT_PROGRAM_ARB        0x8804
#define GL_PROGRAM_FORMAT_ASCII_ARB    0x8875
#define GL_PROGRAM_ERROR_POSITION_ARB  0x864B
#define GL_PROGRAM_ERROR_STRING_ARB    0x8874

typedef void (APIENTRY *PFNGLGENPROGRAMSARBPROC)(GLsizei, GLuint *);
typedef void (APIENTRY *PFNGLBINDPROGRAMARBPROC)(GLenum, GLuint);
typedef void (APIENTRY *PFNGLPROGRAMSTRINGARBPROC)(GLenum, GLenum,
                                                   GLsizei, const void *);
typedef void (APIENTRY *PFNGLDELETEPROGRAMSARBPROC)(GLsizei, const GLuint *);
```

The program takes exactly `vp|fp` and an assembly path, prints usage and returns
2 otherwise. Use one cleanup path that releases the program object, WGL context,
DC, window, registered class, and source buffer in reverse acquisition order.

- [ ] **Step 3: Create a hidden legacy context without helper libraries**

Register a private `CS_OWNDC` class using `DefWindowProc`, create a non-visible
window, get its DC, choose and set a pixel format with
`PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER`, then call
`wglCreateContext` and `wglMakeCurrent`. Return 77 with a concise reason when the
class/window/DC/pixel-format/context path is unavailable; that condition means
the test host cannot provide a driver smoke environment, not that generated
assembly is invalid.

Read `glGetString(GL_EXTENSIONS)` and test whole extension tokens, not substrings.
Require `GL_ARB_vertex_program` for `vp` or `GL_ARB_fragment_program` for `fp`.
Return 77 when the selected extension is absent.

- [ ] **Step 4: Resolve and submit the ARB program**

Resolve the four entry points with `wglGetProcAddress`. Treat `NULL`, `(PROC)1`,
`(PROC)2`, `(PROC)3`, and `(PROC)-1` as unavailable and return 77. Read the input
file in binary mode using `fseek`/`ftell`, reject a size above `INT_MAX`, and keep
the exact byte count rather than relying on NUL termination.

Select `GL_VERTEX_PROGRAM_ARB` or `GL_FRAGMENT_PROGRAM_ARB`, then execute:

```c
while (glGetError() != GL_NO_ERROR)
    ;
genPrograms(1, &program);
bindProgram(target, program);
programString(target, GL_PROGRAM_FORMAT_ASCII_ARB, (GLsizei)length, source);
error = glGetError();
```

Treat a zero generated program name as a smoke failure. When
`error != GL_NO_ERROR`, query
`GL_PROGRAM_ERROR_POSITION_ARB` with `glGetIntegerv`, query
`GL_PROGRAM_ERROR_STRING_ARB` with `glGetString`, print the stage, numeric OpenGL
error, byte position, and driver error string, then return 1. On success, print a
single `loaded <stage> program` line and return 0. This test validates driver
parse/load only; it deliberately does not create geometry or compare pixels.

- [ ] **Step 5: Add the guarded build target and run CTest**

Add this after the `cgc` target definition in the root `CMakeLists.txt`:

```cmake
if(BUILD_TESTING AND WIN32)
    add_executable(arb_smoke tests/arb_smoke.c)
    target_link_libraries(arb_smoke PRIVATE opengl32)
    set_target_properties(arb_smoke PROPERTIES
        C_STANDARD 90
        C_STANDARD_REQUIRED YES
        C_EXTENSIONS YES)
endif()
```

Run:

```powershell
cmake -S . -B build-arb -A Win32 -DBUILD_TESTING=ON
cmake --build build-arb --config Debug
ctest --test-dir build-arb -C Debug -R '^arb(vp|fp)1_wgl_smoke$' --output-on-failure
```

Expected: on a machine exposing both ARB program extensions, both assemblies
load and both tests pass. On a headless/basic-driver host, affected tests show as
Skipped via return code 77. Any `glProgramStringARB` parse error fails with the
driver's position and string.

- [ ] **Step 6: Commit the smoke executable**

```powershell
git add -- tests/arb_smoke.c CMakeLists.txt tests/CMakeLists.txt
git commit -m "Add WGL ARB program smoke tests"
```

### Task 15: Add the optional NVIDIA compatibility oracle

**Files:**
- Create: `tests/compare_arb_oracle.cmake`
- Create: `tests/arb/oracle/vp_passthrough.cg`
- Create: `tests/arb/oracle/vp_uniform.cg`
- Create: `tests/arb/oracle/fp_passthrough.cg`
- Create: `tests/arb/oracle/fp_texture.cg`
- Modify: `tests/CMakeLists.txt`

- [ ] **Step 1: Add an opt-in cache variable with no local path**

Add this to `tests/CMakeLists.txt`:

```cmake
set(CGC_REFERENCE_EXECUTABLE "" CACHE FILEPATH
    "Optional NVIDIA Cg cgc executable used for ARB compatibility tests")

if(CGC_REFERENCE_EXECUTABLE AND NOT EXISTS "${CGC_REFERENCE_EXECUTABLE}")
    message(FATAL_ERROR
        "CGC_REFERENCE_EXECUTABLE does not exist: ${CGC_REFERENCE_EXECUTABLE}")
endif()
```

Do not initialize it from an environment variable and do not check in the
developer machine's `Program Files` path. Configure without the variable and run
`ctest -N`; expected: no `oracle_` tests are listed.

- [ ] **Step 2: Implement acceptance and public-binding comparison**

Create `tests/compare_arb_oracle.cmake`. Require `CGC_UNDER_TEST`,
`REFERENCE_CGC`, `PROFILE`, `SOURCE`, `ENTRY`, and `WORK_DIR`. Run both compilers
with:

```text
-quiet -profile <profile> -entry <entry> -o <unique-output> <source>
```

If exactly one command succeeds, fail and include both statuses and captured
diagnostics. If both fail, pass: stable detailed diagnostics are already owned
by the self-contained failure fixtures, while the oracle establishes acceptance
class only.

If both succeed, read both output files, normalize CRLF, select lines beginning
with `#var `, `#const `, or `#default `, collapse runs of spaces, sort the lines,
and compare the resulting lists exactly. Also require the first line of each
file to match `!!ARBvp1.0` or `!!ARBfp1.0` for the selected profile. Do not
compare compiler version, command line, temporary names, declaration ordering,
instruction scheduling, whitespace, or statistics.

- [ ] **Step 3: Add a deliberately incompatible oracle fixture first**

Create a temporary copy of `vp_passthrough.cg` with `apply_patch`, rename its
POSITION input to an unsupported semantic, and register it through the helper
below. Resolve the standard local Cg installation without storing that path in
the repository:

```powershell
$referenceCgc = Join-Path `
  ([Environment]::GetFolderPath('ProgramFilesX86')) `
  'NVIDIA Corporation\Cg\bin\cgc.exe'
if (!(Test-Path -LiteralPath $referenceCgc)) { throw 'NVIDIA reference cgc not found' }
cmake -S . -B build-arb -A Win32 -DBUILD_TESTING=ON `
  "-DCGC_REFERENCE_EXECUTABLE=$referenceCgc"
ctest --test-dir build-arb -C Debug -R '^oracle_' --output-on-failure
```

Expected: the test fails when the two compilers disagree on acceptance or public
bindings. Remove the temporary fixture and registration before continuing; it is
not part of the commit.

- [ ] **Step 4: Register the common preserved-frontend corpus**

Create the four oracle sources exactly as follows:

```c
/* vp_passthrough.cg */
float4 main(float4 position : POSITION) : POSITION { return position; }

/* vp_uniform.cg */
float4 main(float4 position : POSITION, uniform float4 offset) : POSITION
{ return position + offset; }

/* fp_passthrough.cg */
float4 main(float4 color : COLOR0) : COLOR { return color; }

/* fp_texture.cg */
float4 main(float2 uv : TEXCOORD0,
            uniform sampler2D image : TEXUNIT0) : COLOR
{ return tex2D(image, uv); }
```

Add this helper and calls inside `if(CGC_REFERENCE_EXECUTABLE)`:

```cmake
function(add_arb_oracle name profile source)
    add_test(NAME oracle_${name}
        COMMAND ${CMAKE_COMMAND}
            -DCGC_UNDER_TEST=$<TARGET_FILE:cgc>
            -DREFERENCE_CGC=${CGC_REFERENCE_EXECUTABLE}
            -DPROFILE=${profile}
            -DSOURCE=${PROJECT_SOURCE_DIR}/${source}
            -DENTRY=main
            -DWORK_DIR=${CMAKE_CURRENT_BINARY_DIR}/oracle
            -P ${CMAKE_CURRENT_SOURCE_DIR}/compare_arb_oracle.cmake)
    set_tests_properties(oracle_${name} PROPERTIES LABELS oracle)
endfunction()

add_arb_oracle(vp_passthrough arbvp1 tests/arb/oracle/vp_passthrough.cg)
add_arb_oracle(vp_uniform arbvp1 tests/arb/oracle/vp_uniform.cg)
add_arb_oracle(fp_passthrough arbfp1 tests/arb/oracle/fp_passthrough.cg)
add_arb_oracle(fp_texture arbfp1 tests/arb/oracle/fp_texture.cg)
```

Keep sources within the repository's existing frontend and the portable base
resource envelope: vertex position pass-through, one scalar/vector uniform, one
fragment color pass-through, and one explicit `TEXUNIT0` 2D sample. Do not use
the bundled generic `ATTRIB` shaders because their semantics are not accepted by
the NVIDIA ARB profiles.

- [ ] **Step 5: Run optional and self-contained modes**

With the local reference configured, run:

```powershell
cmake --build build-arb --config Debug
ctest --test-dir build-arb -C Debug -R '^oracle_' --output-on-failure
```

Expected: four oracle tests pass against NVIDIA Cg 3.1.0013. Then configure a
second build without `CGC_REFERENCE_EXECUTABLE`:

```powershell
cmake -S . -B build-arb-selfcontained -A Win32 -DBUILD_TESTING=ON `
  -UCGC_REFERENCE_EXECUTABLE
cmake --build build-arb-selfcontained --config Debug
ctest --test-dir build-arb-selfcontained -C Debug -N
```

Expected: the normal suite is present, no oracle test is registered, and the
build has no proprietary compiler dependency.

- [ ] **Step 6: Commit the optional oracle**

```powershell
git add -- tests/compare_arb_oracle.cmake tests/CMakeLists.txt tests/arb/oracle
git commit -m "Add optional NVIDIA ARB compatibility tests"
```

### Task 16: Finish legacy build metadata, documentation, and release verification

**Files:**
- Modify: `README.txt`
- Verify: `CMakeLists.txt`
- Verify: `Makefile`
- Verify: `stdlib.c`
- Verify: all files below `tests/`

- [ ] **Step 1: Audit every build manifest before documentation**

Run:

```powershell
rg -n "arb_(hal|ir|lower|codegen)|arbvp1_hal|arbfp1_hal|arb_smoke" `
  CMakeLists.txt Makefile tests/CMakeLists.txt
```

Expected: root CMake and `Makefile` list all six compiler source modules
(`arb_hal.c`, `arbvp1_hal.c`, `arbfp1_hal.c`, `arb_ir.c`, `arb_lower.c`, and
`arb_codegen.c`), and CMake lists `arb_smoke`.

- [ ] **Step 2: Verify the two compiler source manifests stay synchronized**

Confirm the root `add_executable(cgc ...)` source list contains:

```text
arb_hal.c
arbvp1_hal.c
arbfp1_hal.c
arb_ir.c
arb_lower.c
arb_codegen.c
```

Confirm `Makefile`'s `OBJS` contains the matching `arb_hal.o`, `arbvp1_hal.o`,
`arbfp1_hal.o`, `arb_ir.o`, `arb_lower.o`, and `arb_codegen.o` entries. Re-run
the `rg` command and correct either manifest if any module is missing; do not
reorder or reformat unrelated entries.

- [ ] **Step 3: Document profiles, limits, and optional validation**

Update the opening and backend-description paragraphs so they no longer claim
that `generic` is the only profile or that the release has only a trivial
backend. Replace the obsolete "prints the output to stdout only" sentence with
the `-o` examples below. Then add concise profile/test sections containing these
exact user-facing commands:

```text
cgc -profile arbvp1 -entry main -o program.arb shader.cg
cgc -profile arbfp1 -entry main -o program.arb shader.cg

cmake -S . -B build -DBUILD_TESTING=ON
cmake --build build --config Debug
ctest --test-dir build -C Debug --output-on-failure
```

State that the backend preserves the repository's existing Cg frontend, targets
base `!!ARBvp1.0` and `!!ARBfp1.0`, rejects NVIDIA profile options, statically
unrolls bounded loops, allows relative uniform indexing only in vertex programs,
and enforces the portable minimum limits listed near the top of this plan. State
that Windows test builds include a hidden-context WGL load smoke test, which
CTest skips when the required driver extension is unavailable.

Document the optional oracle without a default path:

```text
cmake -S . -B build -DBUILD_TESTING=ON \
  -DCGC_REFERENCE_EXECUTABLE="path/to/NVIDIA/cgc"
ctest --test-dir build -C Debug -L oracle --output-on-failure
```

Explicitly say the normal build and test suite neither requires nor redistributes
the proprietary NVIDIA executable.

- [ ] **Step 4: Prove standard-library regeneration is deterministic**

Run the checked-in tokenizer target, hash the generated file, run it again, and
compare hashes:

```powershell
cmake --build build-arb --config Debug --target regenerate_stdlib
$stdlibHash1 = (Get-FileHash -Algorithm SHA256 stdlib.c).Hash
cmake --build build-arb --config Debug --target regenerate_stdlib
$stdlibHash2 = (Get-FileHash -Algorithm SHA256 stdlib.c).Hash
if ($stdlibHash1 -ne $stdlibHash2) { throw 'stdlib.c regeneration is unstable' }
git diff --check -- stdlib.cg stdlib.c
```

Expected: both hashes match and `git diff --check` reports no whitespace errors.
Do not regenerate parser sources because this feature changes no grammar.

- [ ] **Step 5: Run the complete self-contained verification suite**

Start from a fresh build directory so no generated fixture masks a dependency:

```powershell
cmake -S . -B build-arb-final -A Win32 -DBUILD_TESTING=ON `
  -UCGC_REFERENCE_EXECUTABLE
cmake --build build-arb-final --config Debug
ctest --test-dir build-arb-final -C Debug -N
ctest --test-dir build-arb-final -C Debug --output-on-failure
```

Expected: configuration and all targets compile; every generic hash, ARB exact
output, semantic failure, resource boundary, IR unit, and driver smoke test is
listed. All non-driver tests pass. WGL tests either pass or are explicitly
Skipped with return code 77; no test fails.

Compile the four repository shaders again through the unchanged generic profile
outside CTest and verify exit status explicitly:

```powershell
$cgc = (Resolve-Path 'build-arb-final/Debug/cgc.exe').Path
foreach ($shader in 'position.cg','reflection.cg','vertexlight.cg','vertexlight4.cg') {
    & $cgc -quiet -profile generic $shader | Out-Null
    if ($LASTEXITCODE -ne 0) { throw "generic compile failed: $shader" }
}
```

- [ ] **Step 6: Run the configured local oracle separately**

If NVIDIA Cg is installed on the verification host, configure it explicitly and
run only the oracle label:

```powershell
$referenceCgc = Join-Path `
  ([Environment]::GetFolderPath('ProgramFilesX86')) `
  'NVIDIA Corporation\Cg\bin\cgc.exe'
if (Test-Path -LiteralPath $referenceCgc) {
    cmake -S . -B build-arb-oracle -A Win32 -DBUILD_TESTING=ON `
      "-DCGC_REFERENCE_EXECUTABLE=$referenceCgc"
    cmake --build build-arb-oracle --config Debug
    ctest --test-dir build-arb-oracle -C Debug -L oracle --output-on-failure
}
```

Expected on the agreed NVIDIA Cg 3.1.0013 installation: all four common-subset
oracle tests pass. Absence of the executable does not affect the self-contained
suite from Step 5.

- [ ] **Step 7: Verify scope, formatting, and repository cleanliness**

Run:

```powershell
git diff --check
git diff --name-only -- parser.y parser.c parser.h scanner.c scanner.h semantic.c compile.c
git status --short
```

Expected: no whitespace errors; no parser, scanner, common semantic, or common
compile-pipeline file is changed; status shows only the intended backend, guarded
standard-library, build, documentation, and test files plus any pre-existing
unrelated user files. Inspect `git diff --stat` and ensure no generated binary or
machine-specific reference path is staged.

- [ ] **Step 8: Commit the delivery metadata**

```powershell
git add -- README.txt
git commit -m "Document OpenGL ARB profiles"
```

Finish by re-running:

```powershell
ctest --test-dir build-arb-final -C Debug --output-on-failure
git status --short
```

Expected: the self-contained suite remains green and only pre-existing unrelated
untracked files remain.

# Cg 2.0 Language and Backend-Neutral IR Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Make standalone Cg 2.0 the default language, retain explicit Cg 1.1 compatibility, represent the complete Cg 2.0 standard library and reachable program in verified backend-neutral Cg IR, and preserve existing supported GLSL output.

**Architecture:** Evolve the shared scanner, parser, symbol table, and semantic pipeline in place, with canonical types and profile-independent language rules. After entry selection, compute reachability and lower the typed frontend tree to Cg IR; the HAL validates that IR before the generic or GLSL backend lowers and emits it.

**Tech Stack:** ANSI C90 with existing compiler extensions, GNU Bison generated parser sources, CMake/CTest, existing HAL and GLSL IR infrastructure.

---

## Execution Preconditions

- Start from commit `c01d69a` or a descendant containing
  `docs/superpowers/specs/2026-08-23-cg-2-language-ir-design.md`.
- Use an isolated worktree for implementation.
- Configure a dedicated build directory so existing user builds remain
  untouched:

```powershell
cmake -S . -B build-cg20 -DBUILD_TESTING=ON
cmake --build build-cg20 --config Release
ctest --test-dir build-cg20 -C Release --output-on-failure
```

- Every new `.c` and `.h` file must begin with the exact NVIDIA redistribution
  notice already present at the top of `symbols.c`.
- Treat `parser.y` as authoritative. Regenerate `parser.c` and `parser.h` in the
  same commit as each grammar change.
- Keep each commit limited to the task that names it. Do not add `build-cg20/`
  or generated binaries to Git.

## File and Responsibility Map

### New core files

- `language.h`, `language.c`: language-version parsing and centralized feature
  policy.
- `cg_types.h`, `cg_types.c`: canonical scalar kinds, type traits, conversions,
  arithmetic result selection, and type interning helpers.
- `cg_numeric.h`, `cg_numeric.c`: typed literal values and deterministic Cg
  numeric conversion/folding primitives.
- `cg_overload.h`, `cg_overload.c`: ordered Cg 2.0 overload resolution and
  profile-selector ranking.
- `cg_stdlib.h`, `cg_stdlib.c`, `cg_stdlib.def`: declarative standard-library
  signature families and stable intrinsic identities.
- `cg_reach.h`, `cg_reach.c`: reachable function/global graph and call-path
  witnesses.
- `cg_ir.h`, `cg_ir.c`: backend-neutral types, expressions, statements,
  declarations, module ownership, and builders.
- `cg_ir_verify.c`: structural and type verifier.
- `cg_ir_lower.h`, `cg_ir_lower.c`: typed frontend tree to Cg IR lowering.
- `cg_ir_print.c`: deterministic normalized generic output.
- `output.h`, `output.c`: same-directory temporary output and commit/abort
  transaction.

### Existing integration files

- `compile.h`, `cgstruct.c`, `cgcmain.c`: language option and command-line
  behavior.
- `atom.c`, `scanner.c`, `scanner.h`, `parser.y`, `parser.c`, `parser.h`:
  keywords, literals, and syntax.
- `symbols.h`, `symbols.c`: canonical type storage, method/interface symbols,
  function defaults, and profile selectors.
- `support.h`, `support.c`: typed frontend expressions, conversions, arrays,
  member selection, calls, and assignments.
- `constfold.h`, `constfold.c`: delegate scalar arithmetic to `cg_numeric`.
- `semantic.c`, `compile.c`: two-phase call resolution, entry selection,
  reachability, IR construction, validation, and transactional generation.
- `hal.h`, `hal.c`, `generic_hal.c`, `generic_hal.h`: profile identity and Cg IR
  callbacks.
- `glsl_hal.c`, `glsl_lower.c`, `glsl_ir.[ch]`, `glsl_codegen.c`: consume Cg IR
  for the existing GLSL subset.
- `errors.h`, `printutils.c`: stable diagnostics and canonical type printing.
- `stdlib.cg`, `stdlib.c`, `cmake/RegenerateStdlib.cmake`: portable library
  definitions and reproducible token stream.
- `CMakeLists.txt`, `tests/CMakeLists.txt`: build and test registration.

### New tests and fixtures

- `tests/cg_language_test.c`: language-version policy.
- `tests/cg_types_test.c`: canonical types, conversions, and numeric folding.
- `tests/cg_overload_test.c`: ordered candidate resolution.
- `tests/cg_stdlib_test.c`: signature expansion and intrinsic identity.
- `tests/cg_ir_test.c`: builders, verifier, normalized printer, and assertion
  seam.
- `tests/check_cg20.cmake`, `tests/check_cg20_failure.cmake`: compiler fixture
  harnesses.
- `tests/check_cg20_manifest.cmake`: conformance manifest schema and final
  coverage validation.
- `tests/cg20/conformance.csv`: normative requirement classification and test
  mapping.
- `tests/cg20/{profile,literals,types,conversions,arrays,samplers,interfaces,
  overloads,intrinsics,ir,diagnostics}/`: focused shader fixtures.

## Task 1: Lock the Baseline and Add the Cg 2.0 Test Harness

**Files:**

- Create: `tests/check_cg20.cmake`
- Create: `tests/check_cg20_failure.cmake`
- Create: `tests/check_cg20_manifest.cmake`
- Create: `tests/cg20/conformance.csv`
- Create: `tests/cg20/profile/smoke.cg`
- Modify: `tests/CMakeLists.txt:1-120`

- [ ] **Step 1: Record the current baseline before changing test registration**

Run:

```powershell
cmake --build build-cg20 --config Release
ctest --test-dir build-cg20 -C Release --output-on-failure
```

Expected: all currently registered tests pass. Save the reported test count in
the implementation notes for the eventual pull request; do not commit CTest
logs.

- [ ] **Step 2: Add a minimal valid standalone fixture**

Create `tests/cg20/profile/smoke.cg`:

```c
float4 main(float4 position : POSITION) : POSITION
{
    return position;
}
```

- [ ] **Step 3: Add the success harness**

Create `tests/check_cg20.cmake`:

```cmake
execute_process(
    COMMAND "${CGC}" -quiet -nocode -profile "${PROFILE}" ${EXTRA_ARGS} "${SOURCE}"
    RESULT_VARIABLE result
    OUTPUT_VARIABLE output
    ERROR_VARIABLE error
)
if(NOT result EQUAL 0)
    message(FATAL_ERROR
        "Cg 2.0 success fixture failed (${result})\nstdout:\n${output}\nstderr:\n${error}")
endif()
if(DEFINED MESSAGE AND NOT "${output}${error}" MATCHES "${MESSAGE}")
    message(FATAL_ERROR
        "Cg 2.0 fixture did not match '${MESSAGE}'\nstdout:\n${output}\nstderr:\n${error}")
endif()
```

- [ ] **Step 4: Add the failure harness**

Create `tests/check_cg20_failure.cmake`:

```cmake
execute_process(
    COMMAND "${CGC}" -quiet -nocode -profile "${PROFILE}" ${EXTRA_ARGS} "${SOURCE}"
    RESULT_VARIABLE result
    OUTPUT_VARIABLE output
    ERROR_VARIABLE error
)
if(result EQUAL 0)
    message(FATAL_ERROR "Cg 2.0 failure fixture unexpectedly succeeded")
endif()
set(diagnostic "${output}${error}")
if(NOT diagnostic MATCHES "${CODE}")
    message(FATAL_ERROR "missing diagnostic code ${CODE}:\n${diagnostic}")
endif()
if(NOT diagnostic MATCHES "${MESSAGE}")
    message(FATAL_ERROR "missing diagnostic '${MESSAGE}':\n${diagnostic}")
endif()
```

- [ ] **Step 5: Add the initial normative manifest**

Create `tests/cg20/conformance.csv` with the fixed columns
`id,classification,test,source`. Start with these top-level requirements and
add one row for every numbered rule while implementing later tasks:

```csv
id,classification,test,source
LANG-DEFAULT,valid-generic,cg20_default_mode,Language policy
LANG-LEGACY,valid-generic,cg20_legacy_mode,Language policy
TYPE-SCALARS,valid-generic,cg20_scalar_types,Types
TYPE-CONVERSIONS,valid-generic,cg20_conversion_matrix,Type conversions
ARRAY-UNSIZED,valid-generic,cg20_unsized_array,Unsized Arrays
INTERFACE-CONFORMANCE,valid-generic,cg20_interface_dispatch,Struct and interface types
OVERLOAD-PROFILE,valid-generic,cg20_profile_overload,Function overloading
STDLIB-INTRINSICS,valid-generic,cg20_stdlib_coverage,Cg Standard Library Functions
PROFILE-UNREACHABLE,backend-reject,cg20_unreachable_profile_feature,Profiles
CGFX-TECHNIQUE,out-of-scope,cg20_manifest,CgFX exclusion
```

- [ ] **Step 6: Add a schema checker that rejects malformed classifications**

Create `tests/check_cg20_manifest.cmake`:

```cmake
file(STRINGS "${MANIFEST}" rows)
list(POP_FRONT rows header)
if(NOT header STREQUAL "id,classification,test,source")
    message(FATAL_ERROR "unexpected Cg 2.0 manifest header: ${header}")
endif()
foreach(row IN LISTS rows)
    if(NOT row MATCHES "^[A-Z0-9_-]+,(valid-generic|invalid-language|backend-reject|out-of-scope),[^,]+,[^,]+$")
        message(FATAL_ERROR "malformed Cg 2.0 manifest row: ${row}")
    endif()
endforeach()
```

- [ ] **Step 7: Register the harness helpers and initial tests**

Add to `tests/CMakeLists.txt` before existing fixtures:

```cmake
function(add_cg20_success name source)
    add_test(
        NAME ${name}
        COMMAND ${CMAKE_COMMAND}
            -DCGC=$<TARGET_FILE:cgc>
            -DPROFILE=generic
            -DSOURCE=${PROJECT_SOURCE_DIR}/${source}
            -P ${CMAKE_CURRENT_SOURCE_DIR}/check_cg20.cmake
    )
endfunction()

add_cg20_success(cg20_smoke tests/cg20/profile/smoke.cg)
add_test(
    NAME cg20_manifest
    COMMAND ${CMAKE_COMMAND}
        -DMANIFEST=${CMAKE_CURRENT_SOURCE_DIR}/cg20/conformance.csv
        -P ${CMAKE_CURRENT_SOURCE_DIR}/check_cg20_manifest.cmake
)
```

- [ ] **Step 8: Run the focused and full suites**

Run:

```powershell
cmake -S . -B build-cg20 -DBUILD_TESTING=ON
cmake --build build-cg20 --config Release
ctest --test-dir build-cg20 -C Release -R "^cg20_(smoke|manifest)$" --output-on-failure
ctest --test-dir build-cg20 -C Release --output-on-failure
```

Expected: the two new tests and the full suite pass.

- [ ] **Step 9: Commit the harness**

```powershell
git add tests/CMakeLists.txt tests/check_cg20.cmake tests/check_cg20_failure.cmake tests/check_cg20_manifest.cmake tests/cg20
git commit -m "Add Cg 2.0 conformance harness"
```

## Task 2: Add Default Cg 2.0 and Explicit Legacy Language Selection

**Files:**

- Create: `language.h`
- Create: `language.c`
- Create: `tests/cg_language_test.c`
- Modify: `compile.h:52-87`
- Modify: `cgstruct.c:58-83`
- Modify: `cgcmain.c:150-332`
- Modify: `CMakeLists.txt:10-55`
- Modify: `tests/CMakeLists.txt:120-220`

- [ ] **Step 1: Write the language-policy unit test first**

Create `tests/cg_language_test.c` with the repository license followed by:

```c
#if defined(NDEBUG)
#undef NDEBUG
#endif
#include <assert.h>
#include <string.h>
#include "language.h"

int main(void)
{
    CgLanguageVersion version;

    assert(CG_LANGUAGE_DEFAULT == CG_LANGUAGE_2_0);
    assert(ParseCgLanguageVersion("1.1", &version));
    assert(version == CG_LANGUAGE_1_1);
    assert(ParseCgLanguageVersion("2.0", &version));
    assert(version == CG_LANGUAGE_2_0);
    assert(!ParseCgLanguageVersion("2.1", &version));
    assert(!strcmp(CgLanguageVersionString(CG_LANGUAGE_2_0), "2.0"));
    return 0;
}
```

- [ ] **Step 2: Register and run the missing unit target**

Add to `tests/CMakeLists.txt`:

```cmake
add_executable(cg_language_unit
    ${PROJECT_SOURCE_DIR}/language.c
    cg_language_test.c
)
target_include_directories(cg_language_unit PRIVATE ${PROJECT_SOURCE_DIR})
set_target_properties(cg_language_unit PROPERTIES
    C_STANDARD 90 C_STANDARD_REQUIRED YES C_EXTENSIONS YES)
add_test(NAME cg_language_unit COMMAND cg_language_unit)
```

Run:

```powershell
cmake -S . -B build-cg20 -DBUILD_TESTING=ON
cmake --build build-cg20 --config Release --target cg_language_unit
```

Expected: compilation fails because `language.h` and `language.c` do not exist.

- [ ] **Step 3: Define the version API**

Create `language.h` with the repository license and:

```c
#if !defined(__LANGUAGE_H)
#define __LANGUAGE_H 1

typedef enum CgLanguageVersion_Rec {
    CG_LANGUAGE_1_1 = 101,
    CG_LANGUAGE_2_0 = 200
} CgLanguageVersion;

#define CG_LANGUAGE_DEFAULT CG_LANGUAGE_2_0

int ParseCgLanguageVersion(const char *text, CgLanguageVersion *version);
const char *CgLanguageVersionString(CgLanguageVersion version);
int CgLanguageAtLeast(CgLanguageVersion actual, CgLanguageVersion required);

#endif
```

Create `language.c` with the repository license and:

```c
#include <string.h>
#include "language.h"

int ParseCgLanguageVersion(const char *text, CgLanguageVersion *version)
{
    if (!strcmp(text, "1.1")) {
        *version = CG_LANGUAGE_1_1;
        return 1;
    }
    if (!strcmp(text, "2.0")) {
        *version = CG_LANGUAGE_2_0;
        return 1;
    }
    return 0;
}

const char *CgLanguageVersionString(CgLanguageVersion version)
{
    return version == CG_LANGUAGE_1_1 ? "1.1" : "2.0";
}

int CgLanguageAtLeast(CgLanguageVersion actual, CgLanguageVersion required)
{
    return actual >= required;
}
```

- [ ] **Step 4: Put the selected version in compiler options**

Include `language.h` from `compile.h`, then add this member to `Options` after
`entryName`:

```c
CgLanguageVersion languageVersion;
```

After zero-initializing options in `InitCgStruct`, set:

```c
Cg->options.languageVersion = CG_LANGUAGE_DEFAULT;
```

- [ ] **Step 5: Parse `-version` during command-line pass zero**

Update the help text to include `[-version 1.1|2.0]`. Add this branch before
`-profile` handling in `CommandLineArgs`:

```c
} else if (!strcmp(argv[ii], "-version")) {
    ii++;
    if (pass == 0) {
        if (ii >= argc ||
            !ParseCgLanguageVersion(argv[ii], &Cg->options.languageVersion)) {
            printf(OPENSL_TAG ": invalid language version after \"-version\"\n");
            return 0;
        }
    }
```

The second command-line pass must still increment `ii` so the version value is
not treated as an input file.

- [ ] **Step 6: Link language policy into the compiler and run tests**

Add `language.c` to the `cgc` source list in `CMakeLists.txt`.

Run:

```powershell
cmake -S . -B build-cg20 -DBUILD_TESTING=ON
cmake --build build-cg20 --config Release
ctest --test-dir build-cg20 -C Release -R "cg_language_unit|cg20_smoke" --output-on-failure
& .\build-cg20\Release\cgc.exe -version 2.1 tests\cg20\profile\smoke.cg
```

Expected: focused tests pass; the direct command exits nonzero and prints
`invalid language version`.

- [ ] **Step 7: Commit language selection**

```powershell
git add language.h language.c compile.h cgstruct.c cgcmain.c CMakeLists.txt tests/CMakeLists.txt tests/cg_language_test.c
git commit -m "Add Cg language version selection"
```

## Task 3: Make Canonical Scalar Kind Authoritative

**Files:**

- Create: `cg_types.h`
- Create: `cg_types.c`
- Create: `tests/cg_types_test.c`
- Modify: `symbols.h:56-365`
- Modify: `symbols.c:102-1150`
- Modify: `support.h:340-415`
- Modify: `support.c:98-215`
- Modify: `printutils.c:450-570`
- Modify: `CMakeLists.txt`
- Modify: `tests/CMakeLists.txt`

- [ ] **Step 1: Write a failing canonical-kind test**

Create `tests/cg_types_test.c` with the repository license and this initial
body:

```c
#if defined(NDEBUG)
#undef NDEBUG
#endif
#include <assert.h>
#include "slglobals.h"
#include "cg_types.h"

CgStruct *Cg;
Scope *CurrentScope;

int main(void)
{
    Type type;

    InitType(&type);
    SetScalarKind(&type, CG_SCALAR_FLOAT);
    assert(GetScalarKind(&type) == CG_SCALAR_FLOAT);
    assert(CgScalarIsFloating(CG_SCALAR_FLOAT));
    assert(!CgScalarIsIntegral(CG_SCALAR_FLOAT));
    assert(CG_SCALAR_DOUBLE > 15);
    return 0;
}
```

Register `cg_types_unit` using `cg_types.c`, `symbols.c`, `memory.c`, and
`atom.c`, plus the minimal existing stubs already used by
`glsl_semantics_unit`. Run the target and expect failure because the new API is
absent.

- [ ] **Step 2: Define scalar kinds independently of legacy base bits**

Create `cg_types.h`:

```c
#if !defined(__CG_TYPES_H)
#define __CG_TYPES_H 1

typedef union Type_Rec Type;

typedef enum CgScalarKind_Rec {
    CG_SCALAR_NONE = 0,
    CG_SCALAR_UNDEFINED,
    CG_SCALAR_CFLOAT,
    CG_SCALAR_CINT,
    CG_SCALAR_BOOL,
    CG_SCALAR_CHAR,
    CG_SCALAR_UCHAR,
    CG_SCALAR_SHORT,
    CG_SCALAR_USHORT,
    CG_SCALAR_INT,
    CG_SCALAR_UINT,
    CG_SCALAR_LONG,
    CG_SCALAR_ULONG,
    CG_SCALAR_FIXED,
    CG_SCALAR_HALF,
    CG_SCALAR_FLOAT,
    CG_SCALAR_DOUBLE,
    CG_SCALAR_COUNT
} CgScalarKind;

CgScalarKind GetScalarKind(const Type *type);
void SetScalarKind(Type *type, CgScalarKind kind);
int CgScalarIsCompileTime(CgScalarKind kind);
int CgScalarIsIntegral(CgScalarKind kind);
int CgScalarIsUnsigned(CgScalarKind kind);
int CgScalarIsFloating(CgScalarKind kind);
const char *CgScalarKindName(CgScalarKind kind);

#endif
```

Move the existing `typedef union Type_Rec Type;` declaration from `symbols.h`
to this header; do not leave a second typedef behind. This keeps the C90
include graph legal when `symbols.h` includes `cg_types.h`.

- [ ] **Step 3: Add `scalarKind` to every `Type` union prefix**

In `symbols.h`, add the same third member after `size` in `TypeCommon`,
`TypeScalar`, `TypeArray`, `TypeStruct`, and `TypeFunction`:

```c
CgScalarKind scalarKind;
```

Include `cg_types.h` before those structures. Update `InitType` to initialize
`scalarKind` to `CG_SCALAR_NONE`, and update `DupType` to preserve it through
the existing structure copy.

- [ ] **Step 4: Implement kind traits without consulting the HAL**

Create `cg_types.c` with a fixed trait table:

```c
#include "slglobals.h"
#include "cg_types.h"

typedef struct CgScalarTraits_Rec {
    const char *name;
    unsigned isCompileTime : 1;
    unsigned isIntegral : 1;
    unsigned isUnsigned : 1;
    unsigned isFloating : 1;
} CgScalarTraits;

static const CgScalarTraits traits[CG_SCALAR_COUNT] = {
    { "<none>", 0, 0, 0, 0 },
    { "<undefined>", 0, 0, 0, 0 },
    { "cfloat", 1, 0, 0, 1 },
    { "cint", 1, 1, 0, 0 },
    { "bool", 0, 0, 0, 0 },
    { "char", 0, 1, 0, 0 },
    { "unsigned char", 0, 1, 1, 0 },
    { "short", 0, 1, 0, 0 },
    { "unsigned short", 0, 1, 1, 0 },
    { "int", 0, 1, 0, 0 },
    { "unsigned int", 0, 1, 1, 0 },
    { "long", 0, 1, 0, 0 },
    { "unsigned long", 0, 1, 1, 0 },
    { "fixed", 0, 0, 0, 1 },
    { "half", 0, 0, 0, 1 },
    { "float", 0, 0, 0, 1 },
    { "double", 0, 0, 0, 1 }
};
```

Implement each public query as a bounds check followed by a table lookup.

- [ ] **Step 5: Bridge old bases while migrating consumers**

Add these helpers in `symbols.c`:

```c
static CgScalarKind lKindFromLegacyBase(int base)
{
    switch (base) {
    case TYPE_BASE_CFLOAT: return CG_SCALAR_CFLOAT;
    case TYPE_BASE_CINT: return CG_SCALAR_CINT;
    case TYPE_BASE_FLOAT: return CG_SCALAR_FLOAT;
    case TYPE_BASE_INT: return CG_SCALAR_INT;
    case TYPE_BASE_BOOLEAN: return CG_SCALAR_BOOL;
    default: return CG_SCALAR_NONE;
    }
}
```

`NewType` initializes `scalarKind` from this function. `NewPackedArrayType`
copies `GetScalarKind(elType)`. `GetBase` remains a compatibility accessor for
old consumers, but `IsSameUnqualifiedType`, `IsBoolean`, type printing, and
new code compare `GetScalarKind` rather than masking `properties`.

- [ ] **Step 6: Store a canonical type on constants and operators**

Keep the existing `Type *type` field in every expression node authoritative.
Update `NewIConstNode`, `NewBConstNode`, `NewFConstNode`, and
`NewFConstNodeV` so each assigns its canonical `Type *` immediately. Leave
`subop` populated for old lowering until Task 19 removes frontend dependence
on its four-bit type.

- [ ] **Step 7: Run migration audits and tests**

Run:

```powershell
rg -n "TYPE_BASE_MASK|SUBOP_GET_T|GetBase\(" --glob "*.c" --glob "*.h" .
cmake -S . -B build-cg20 -DBUILD_TESTING=ON
cmake --build build-cg20 --config Release
ctest --test-dir build-cg20 -C Release -R "cg_types_unit|generic_|glsl" --output-on-failure
```

Expected: the audit still lists compatibility consumers scheduled below; the
new unit test and all existing generic/GLSL tests pass unchanged.

- [ ] **Step 8: Commit canonical scalar identity**

```powershell
git add cg_types.h cg_types.c symbols.h symbols.c support.h support.c printutils.c CMakeLists.txt tests/CMakeLists.txt tests/cg_types_test.c
git commit -m "Make Cg scalar kinds canonical"
```

## Task 4: Intern the Complete Cg 2.0 Scalar, Vector, and Matrix Types

**Files:**

- Modify: `cg_types.h`, `cg_types.c`
- Modify: `symbols.h:273-365`
- Modify: `symbols.c:66-215,1085-1110`
- Modify: `generic_hal.c:155-193`
- Modify: `tests/cg_types_test.c`
- Create: `tests/cg20/types/scalars.cg`
- Modify: `tests/CMakeLists.txt`

- [ ] **Step 1: Extend the unit test with every required kind and shape**

Add to `tests/cg_types_test.c` after symbol-table initialization:

```c
assert(GetScalarKind(GetStandardTypeKind(CG_SCALAR_CHAR, 0, 0)) == CG_SCALAR_CHAR);
assert(GetScalarKind(GetStandardTypeKind(CG_SCALAR_ULONG, 4, 0)) == CG_SCALAR_ULONG);
assert(GetScalarKind(GetStandardTypeKind(CG_SCALAR_HALF, 3, 2)) == CG_SCALAR_HALF);
assert(IsVector(GetStandardTypeKind(CG_SCALAR_FIXED, 4, 0), NULL));
assert(IsMatrix(GetStandardTypeKind(CG_SCALAR_DOUBLE, 4, 4), NULL, NULL));
assert(GetStandardTypeKind(CG_SCALAR_FLOAT, 4, 4) ==
       GetStandardTypeKind(CG_SCALAR_FLOAT, 4, 4));
```

Run `cg_types_unit`; expect a link failure for `GetStandardTypeKind`.

- [ ] **Step 2: Add an interned standard-type registry**

Declare in `cg_types.h`:

```c
Type *GetStandardTypeKind(CgScalarKind kind, int rows, int columns);
int InitCgStandardTypes(void);
void FreeCgStandardTypes(void);
```

In `cg_types.c`, use this fixed registry:

```c
static Type *standardTypes[CG_SCALAR_COUNT][5][5];
```

Use indices `(0,0)` for a scalar, `(length,0)` for a vector, and
`(rows,columns)` for a matrix. Reject lengths outside 1-4. Construct vectors
as packed arrays of the scalar and matrices as packed arrays of packed row
vectors. Store the scalar kind on every layer.

- [ ] **Step 3: Register all scalar and aggregate typedef spellings**

Replace the hand-written float/int/bool initialization block in `symbols.c`
with loops over the supported declaration kinds:

```c
static const CgScalarKind declarationKinds[] = {
    CG_SCALAR_CHAR, CG_SCALAR_UCHAR, CG_SCALAR_SHORT, CG_SCALAR_USHORT,
    CG_SCALAR_INT, CG_SCALAR_UINT, CG_SCALAR_LONG, CG_SCALAR_ULONG,
    CG_SCALAR_FIXED, CG_SCALAR_HALF, CG_SCALAR_FLOAT, CG_SCALAR_DOUBLE,
    CG_SCALAR_BOOL
};
```

Register scalar names, `TYPE1` through `TYPE4`, and `TYPE1x1` through
`TYPE4x4`. Keep existing globals such as `Float4Type` as aliases into the
registry so old code continues to compile.

- [ ] **Step 4: Add the complete scalar fixture**

Create `tests/cg20/types/scalars.cg`:

```c
float4 main(float4 position : POSITION) : POSITION
{
    char c = 1;
    unsigned char uc = 2;
    short s = 3;
    unsigned short us = 4;
    int i = 5;
    unsigned int ui = 6;
    long l = 7;
    unsigned long ul = 8;
    half h = 0.5;
    fixed x = 0.5;
    float f = 0.5;
    double d = 0.5;
    return position + float4(c + uc + s + us + i + ui + l + ul) *
        float(h + x + f + d) * 0.0;
}
```

Register it as `cg20_scalar_types`. It will fail at the scanner in this task;
keep the fixture registered but mark its CTest property `DISABLED TRUE` until
Task 5 enables syntax. The unit-level type registry must pass before commit.

- [ ] **Step 5: Run type and legacy regression tests**

Run:

```powershell
cmake --build build-cg20 --config Release
ctest --test-dir build-cg20 -C Release -R "cg_types_unit|generic_|glsl" --output-on-failure
```

Expected: all selected enabled tests pass.

- [ ] **Step 6: Commit the complete standard type registry**

```powershell
git add cg_types.h cg_types.c symbols.h symbols.c generic_hal.c tests/cg_types_test.c tests/cg20/types/scalars.cg tests/CMakeLists.txt
git commit -m "Register Cg 2.0 standard types"
```

## Task 5: Recognize Cg 2.0 Keywords, Type Specifiers, and Reserved Words

**Files:**

- Modify: `atom.c:55-125,650-710`
- Modify: `scanner.c:570-650`
- Modify: `parser.y:55-185,325-455`
- Regenerate: `parser.c`, `parser.h`
- Modify: `errors.h`
- Create: `tests/cg20/diagnostics/reserved_word.cg`
- Modify: `tests/CMakeLists.txt`

- [ ] **Step 1: Add failing keyword and reserved-word fixtures**

Use the scalar fixture from Task 4. Create
`tests/cg20/diagnostics/reserved_word.cg`:

```c
float4 main(float4 position : POSITION) : POSITION
{
    float Technique = 1.0;
    return position * Technique;
}
```

Register a failure test expecting new diagnostic code `1300` and text
`reserved word`. Run both fixtures. Expected: scalar syntax fails generically,
and `Technique` is currently accepted instead of producing code 1300.

- [ ] **Step 2: Append tokens without renumbering existing tokens**

Keep token values 257-314 unchanged. Replace the old sentinel with these
appended declarations in `parser.y`:

```yacc
%token <sc_token> CHAR_SY 315
%token <sc_token> DOUBLE_SY 316
%token <sc_token> FIXED_SY 317
%token <sc_token> HALF_SY 318
%token <sc_token> INTERFACE_SY 319
%token <sc_token> LONG_SY 320
%token <sc_token> SHORT_SY 321
%token <sc_token> UNSIGNED_SY 322
%token <sc_token> RESERVED_SY 323
%token <sc_token> FIRST_USER_TOKEN_SY 324
```

Update the fixed token table in `atom.c` with the lower-case standalone
keywords. Keep `sampler*` spellings as predefined typedef symbols so the
scanner can continue returning `TYPEIDENT_SY` after symbol lookup.

- [ ] **Step 3: Add version-aware reserved-word recognition**

Add to `language.h`:

```c
int CgIsReservedWord(const char *text, CgLanguageVersion version);
```

Implement a sorted table in `language.c` containing every Cg 2.0 reserved word
from the specification. Store a flag for words marked case-insensitive. In the
identifier branch of `scanner.c`, return `RESERVED_SY` and preserve the atom in
`yylval.sc_ident` when `CgIsReservedWord` succeeds and the atom is not an
implemented keyword token.

- [ ] **Step 4: Parse all scalar specifier combinations**

Extend `type_specifier` so direct keywords map through canonical kinds. Add a
small parser helper `ResolveScalarTypeSpecifier(SourceLoc *, int token,
int isUnsigned)` in `support.c`; it returns `UndefinedType` with a language
diagnostic for illegal combinations such as `unsigned float`.

Add grammar support for:

```yacc
type_specifier: CHAR_SY
              | SHORT_SY
              | LONG_SY
              | HALF_SY
              | FIXED_SY
              | DOUBLE_SY
              | UNSIGNED_SY
              | UNSIGNED_SY CHAR_SY
              | UNSIGNED_SY SHORT_SY
              | UNSIGNED_SY INT_SY
              | UNSIGNED_SY LONG_SY
```

Add `ERROR_S_RESERVED_WORD` as code 1300 in `errors.h`, and add a parser
production that reports it when `RESERVED_SY` appears where an identifier is
required.

- [ ] **Step 5: Regenerate parser and standard-library tokens**

Run:

```powershell
cmake --build build-cg20 --config Release --target regenerate_parser
cmake --build build-cg20 --config Release --target regenerate_stdlib
```

Expected: `parser.c`, `parser.h`, and `stdlib.c` change deterministically. Run
the repository regeneration tests to prove the checked-in results match.

- [ ] **Step 6: Enable and run scalar/reserved fixtures**

Remove the `DISABLED` property from `cg20_scalar_types`.

Run:

```powershell
cmake --build build-cg20 --config Release
ctest --test-dir build-cg20 -C Release -R "cg20_scalar_types|cg20_reserved_word|parser|stdlib_regeneration" --output-on-failure
```

Expected: all selected tests pass.

- [ ] **Step 7: Commit syntax and generated sources together**

```powershell
git add language.h language.c atom.c scanner.c parser.y parser.c parser.h errors.h support.c stdlib.c tests/CMakeLists.txt tests/cg20
git commit -m "Parse Cg 2.0 type keywords"
```

## Task 6: Preserve Typed Literal Values and Fold Every Scalar Family

**Files:**

- Create: `cg_numeric.h`, `cg_numeric.c`
- Modify: `scanner.c:453-570,650-730`
- Modify: `parser.y:55-135,990-1020`
- Regenerate: `parser.c`, `parser.h`
- Modify: `support.h:340-380,490-510`
- Modify: `support.c:123-215`
- Modify: `constfold.h`, `constfold.c`
- Modify: `errors.h`
- Modify: `tests/cg_types_test.c`
- Create: `tests/cg20/literals/suffixes.cg`
- Create: `tests/cg20/diagnostics/literal_overflow.cg`
- Modify: `CMakeLists.txt`, `tests/CMakeLists.txt`

- [ ] **Step 1: Add failing literal-value unit cases**

Extend `tests/cg_types_test.c`:

```c
{
    CgNumericValue input;
    CgNumericValue output;

    CgNumericSetSigned(&input, CG_SCALAR_LONG, -2);
    assert(CgNumericConvert(&output, CG_SCALAR_ULONG, &input));
    assert(output.kind == CG_SCALAR_ULONG);
    CgNumericSetFloat(&input, CG_SCALAR_FIXED, 3.0);
    assert(CgNumericNormalize(&output, &input));
    assert(output.value.f < 2.0);
    CgNumericSetFloat(&input, CG_SCALAR_HALF, 1.0 / 3.0);
    assert(CgNumericNormalize(&output, &input));
    assert(output.kind == CG_SCALAR_HALF);
}
```

Run `cg_types_unit`. Expected: compilation fails for the missing numeric API.

- [ ] **Step 2: Define a host-independent numeric container**

Create `cg_numeric.h` with `stdint.h` when available and compiler-extension
fallbacks otherwise:

```c
#if !defined(__CG_NUMERIC_H)
#define __CG_NUMERIC_H 1

#include "cg_types.h"

#if defined(CGC_HAVE_STDINT_H)
#include <stdint.h>
typedef int64_t CgInt64;
typedef uint64_t CgUInt64;
#elif defined(_MSC_VER)
typedef __int64 CgInt64;
typedef unsigned __int64 CgUInt64;
#else
typedef long long CgInt64;
typedef unsigned long long CgUInt64;
#endif

typedef struct CgNumericValue_Rec {
    CgScalarKind kind;
    union {
        CgInt64 i;
        CgUInt64 u;
        double f;
    } value;
} CgNumericValue;

typedef enum CgNumericOp_Rec {
    CG_NUMERIC_NEG,
    CG_NUMERIC_NOT,
    CG_NUMERIC_BIT_NOT,
    CG_NUMERIC_ADD,
    CG_NUMERIC_SUB,
    CG_NUMERIC_MUL,
    CG_NUMERIC_DIV,
    CG_NUMERIC_MOD,
    CG_NUMERIC_BIT_AND,
    CG_NUMERIC_BIT_OR,
    CG_NUMERIC_BIT_XOR,
    CG_NUMERIC_SHIFT_LEFT,
    CG_NUMERIC_SHIFT_RIGHT
} CgNumericOp;

void CgNumericSetSigned(CgNumericValue *value, CgScalarKind kind, CgInt64 data);
void CgNumericSetUnsigned(CgNumericValue *value, CgScalarKind kind, CgUInt64 data);
void CgNumericSetFloat(CgNumericValue *value, CgScalarKind kind, double data);
int CgNumericNormalize(CgNumericValue *result, const CgNumericValue *value);
int CgNumericConvert(CgNumericValue *result, CgScalarKind kind,
                     const CgNumericValue *value);
int CgNumericBinary(CgNumericValue *result, CgNumericOp op,
                    const CgNumericValue *left,
                    const CgNumericValue *right);

#endif
```

- [ ] **Step 3: Implement deterministic ranges and `fixed` clamping**

In `cg_numeric.c`, define exact integer widths of 8, 16, 32, and 64 bits.
Normalize signed values with two's-complement truncation, unsigned values with
masking, `float` through a `float` temporary, `double` directly, and `fixed`
with:

```c
static double NormalizeFixed(double value)
{
    const double maximum = 2.0 - 1.0 / 1024.0;
    if (value < -2.0)
        return -2.0;
    if (value > maximum)
        return maximum;
    return floor(value * 1024.0 + (value >= 0.0 ? 0.5 : -0.5)) / 1024.0;
}
```

Reuse the existing `round_half` algorithm from `constfold.c` through a public
`CgNumericRoundHalf` helper; do not keep two implementations.

- [ ] **Step 4: Carry one typed literal through Bison**

Add `CgNumericValue sc_literal;` to the parser `%union`. Change
`INTCONST_SY`, `CFLOATCONST_SY`, `FLOATCONST_SY`, `FLOATHCONST_SY`, and
`FLOATXCONST_SY` to use `<sc_literal>`. Add `DOUBLECONST_SY` only if a distinct
token is required by the existing grammar; otherwise the kind inside
`sc_literal` distinguishes `double`.

Replace `lBuildFloatValue` with a double-returning implementation. Parse every
specified suffix into these kinds:

```text
none integer -> cint       none floating -> cfloat
t -> char                  ut -> unsigned char
s -> short                 us -> unsigned short
i -> int                   u or ui -> unsigned int
l -> long                  ul -> unsigned long
h -> half                  x -> fixed
f -> float                 d -> double
```

Reject repeated or incompatible suffix letters with one new malformed-literal
diagnostic.

- [ ] **Step 5: Replace scalar constant storage**

In `support.h`, replace the old `scalar_constant` union with:

```c
typedef CgNumericValue scalar_constant;
```

Add:

```c
constant *NewNumericConstNode(opcode op, const CgNumericValue *value);
```

Make the legacy integer/float constructors thin wrappers around this function.
The constructor sets both `constant.type` through `GetStandardTypeKind` and
the legacy `subop` cache when the kind has an old base.

- [ ] **Step 6: Delegate scalar folding to `cg_numeric`**

Replace the fixed `runtime_ops[TYPE_BASE_LAST_USER + 1]` lookup with a table
indexed by `CgScalarKind`. Map frontend `opcode` values to `CgNumericOp` inside
`constfold.c`, then route signed, unsigned, half, fixed, float, and double
unary/binary operations through `CgNumericBinary`; keep vector and matrix
iteration in `constfold.c`. This keeps `cg_numeric.h` independent of
`support.h` and avoids an include cycle.

Add explicit checks for division by zero, invalid shifts, and host conversion
failure before performing the host-C operation.

- [ ] **Step 7: Add shader fixtures and expected diagnostics**

Create `tests/cg20/literals/suffixes.cg`:

```c
float4 main(float4 p : POSITION) : POSITION
{
    const unsigned long mask = 0xfful;
    const half h = 0.333h;
    const fixed x = 3.0x;
    const double d = 1.25d;
    return p + float4(float(mask & 1ul), float(h), float(x), float(d)) * 0.0;
}
```

Create `tests/cg20/diagnostics/literal_overflow.cg` with an unsigned-long
literal larger than 64 bits. Register success and failure tests; the failure
must assert the new overflow code and the literal's source line.

- [ ] **Step 8: Regenerate, run tests, and commit**

Run:

```powershell
cmake --build build-cg20 --config Release --target regenerate_parser regenerate_stdlib
cmake --build build-cg20 --config Release
ctest --test-dir build-cg20 -C Release -R "cg_types_unit|cg20_.*literal|stdlib_regeneration" --output-on-failure
ctest --test-dir build-cg20 -C Release --output-on-failure
```

Expected: focused and full suites pass.

```powershell
git add cg_numeric.h cg_numeric.c scanner.c parser.y parser.c parser.h support.h support.c constfold.h constfold.c errors.h stdlib.c CMakeLists.txt tests/CMakeLists.txt tests/cg_types_test.c tests/cg20
git commit -m "Implement Cg 2.0 numeric literals"
```

## Task 7: Implement the Normative Conversion Matrix and Arithmetic Promotions

**Files:**

- Modify: `cg_types.h`, `cg_types.c`
- Modify: `support.h:570-600`
- Modify: `support.c:1970-2610,2850-2865,3150-3200`
- Modify: `hal.h:145-158`
- Modify: `hal.c`, `generic_hal.c`, `glsl_hal.c`
- Modify: `errors.h`
- Modify: `tests/cg_types_test.c`
- Create: `tests/cg20/conversions/numeric.cg`
- Create: `tests/cg20/conversions/aggregate_casts.cg`
- Create: `tests/cg20/diagnostics/invalid_conversion.cg`
- Modify: `tests/CMakeLists.txt`, `tests/cg20/conformance.csv`

- [ ] **Step 1: Write table-driven conversion tests**

Add to `tests/cg_types_test.c`:

```c
typedef struct ConversionCase_Rec {
    CgScalarKind from;
    CgScalarKind to;
    CgConversionRank implicitRank;
    CgConversionRank explicitRank;
} ConversionCase;

static const ConversionCase cases[] = {
    { CG_SCALAR_CINT, CG_SCALAR_HALF, CG_CONVERSION_PROMOTION, CG_CONVERSION_PROMOTION },
    { CG_SCALAR_INT, CG_SCALAR_FLOAT, CG_CONVERSION_IMPLICIT, CG_CONVERSION_IMPLICIT },
    { CG_SCALAR_DOUBLE, CG_SCALAR_HALF, CG_CONVERSION_IMPLICIT_WARN, CG_CONVERSION_EXPLICIT },
    { CG_SCALAR_BOOL, CG_SCALAR_FLOAT, CG_CONVERSION_IMPLICIT, CG_CONVERSION_IMPLICIT },
    { CG_SCALAR_FLOAT, CG_SCALAR_BOOL, CG_CONVERSION_IMPLICIT, CG_CONVERSION_IMPLICIT }
};
```

Loop over the table and assert `CgClassifyScalarConversion`. Also assert the
usual arithmetic results for `cint + half`, `cfloat + fixed`, signed/unsigned
equal-width pairs, and `int + float`. Run and expect missing API failures.

- [ ] **Step 2: Define conversion ranks and results**

Add to `cg_types.h`:

```c
typedef enum CgConversionRank_Rec {
    CG_CONVERSION_NONE = 0,
    CG_CONVERSION_EXPLICIT,
    CG_CONVERSION_IMPLICIT_WARN,
    CG_CONVERSION_IMPLICIT,
    CG_CONVERSION_PROMOTION,
    CG_CONVERSION_DYNAMIC,
    CG_CONVERSION_EXACT
} CgConversionRank;

CgConversionRank CgClassifyConversion(const Type *from, const Type *to,
                                      int explicitCast);
CgConversionRank CgClassifyScalarConversion(CgScalarKind from,
                                            CgScalarKind to,
                                            int explicitCast);
Type *CgUsualArithmeticType(const Type *left, const Type *right);
```

Implement the specification's ordered rules directly. Do not call HAL numeric
callbacks from these functions.

- [ ] **Step 3: Make `ConvertType` consume classification results**

Replace `lIsBaseCastValid` and the HAL scalar-cast query in `support.c` with
`CgClassifyConversion`. Build cast nodes from canonical source and target
types. Add a `Type *targetType` field to unary cast nodes if decoding the
target from `subop` would lose a kind above 15.

Emit a warning only for `CG_CONVERSION_IMPLICIT_WARN`; an explicit cast uses
`CG_CONVERSION_EXPLICIT` without warning.

- [ ] **Step 4: Implement shape and aggregate conversion cases**

Cover the complete specification table:

- Scalar replication to vector and matrix.
- Vector or matrix to scalar by first element, warning when implicit.
- Same-size vector element conversion.
- Smaller vector selection and equal-element-count vector/matrix conversion.
- Upper-left smaller matrix selection.
- Explicit structure-to-first-member and one-member structure casts.
- Explicit pairwise structure casts with equal member counts.
- Explicit equal-size array element conversion.
- Array-of-vector and matrix conversion when shapes match.

Use dedicated typed AST cast nodes for aggregate conversions; do not encode
them in four-bit scalar subops.

- [ ] **Step 5: Move arithmetic result selection out of HAL**

Change `ConvertNumericOperands`, unary arithmetic, comparisons, and conditional
selection to use `CgUsualArithmeticType`. Leave HAL `IsValidOperator` as a
post-language profile capability check. Mark `IsValidScalarCast`,
`GetBinOpBase`, and `ConvertConstant` deprecated in `hal.h`; remove each after
the last existing caller is migrated in this task.

- [ ] **Step 6: Add conversion fixtures**

`tests/cg20/conversions/numeric.cg` must cover compile-time constants,
signed/unsigned equal widths, scalar smearing, vector narrowing, and matrix
selection. `aggregate_casts.cg` must explicitly cast between two pairwise
compatible structs and between equal-size array element types.

Create `invalid_conversion.cg` using `sampler2D` to `float`; register a language
failure asserting a conversion diagnostic rather than a GLSL profile code.

- [ ] **Step 7: Run focused and full suites**

Run:

```powershell
cmake --build build-cg20 --config Release
ctest --test-dir build-cg20 -C Release -R "cg_types_unit|cg20_.*conversion|glslv_matrix|generic_matrix" --output-on-failure
ctest --test-dir build-cg20 -C Release --output-on-failure
```

Expected: all tests pass and existing GLSL matrix text remains unchanged.

- [ ] **Step 8: Commit language-level conversions**

```powershell
git add cg_types.h cg_types.c support.h support.c hal.h hal.c generic_hal.c glsl_hal.c errors.h tests/CMakeLists.txt tests/cg_types_test.c tests/cg20
git commit -m "Implement Cg 2.0 conversions"
```

## Task 8: Complete First-Class, Packed, Nested, and Unsized Arrays

**Files:**

- Modify: `cg_types.h`, `cg_types.c`
- Modify: `symbols.h:180-188`
- Modify: `symbols.c:633-760`
- Modify: `support.h`, `support.c:1100-1535,2870-2930,3150-3200`
- Modify: `parser.y:470-570`
- Regenerate: `parser.c`, `parser.h`
- Modify: `errors.h`
- Create: `tests/cg20/arrays/unsized.cg`
- Create: `tests/cg20/arrays/nested_struct.cg`
- Create: `tests/cg20/arrays/length.cg`
- Create: `tests/cg20/diagnostics/unsized_initializer.cg`
- Modify: `tests/CMakeLists.txt`, `tests/cg20/conformance.csv`

- [ ] **Step 1: Write array fixtures that fail under the current frontend**

Create `tests/cg20/arrays/unsized.cg`:

```c
float sum(float values[])
{
    float result = 0.0;
    int i;
    for (i = 0; i < values.length; ++i)
        result += values[i];
    return result;
}

float4 main(float4 p : POSITION) : POSITION
{
    float source[3] = { 1.0, 2.0, 3.0 };
    float dynamic[];
    dynamic = source;
    return p + sum(dynamic) * 0.0;
}
```

Add `nested_struct.cg` with a struct containing `float4 weights[8]`, and
`length.cg` covering vectors, matrices, sized arrays, and unsized parameters.
Register them and run; expect failures at unsized-array validation or `.length`.

- [ ] **Step 2: Give unsized arrays an explicit representation**

Add:

```c
#define CG_ARRAY_UNSIZED (-1)
```

Use `arr.numels == CG_ARRAY_UNSIZED` for a dynamic unsized declaration and
reserve zero for invalid/recovery types. Update `Array_Declarator`,
`IsUnsizedArray`, type equivalence, printing, and size computation accordingly.

- [ ] **Step 3: Implement initializer sizing and dynamic assignment**

When an empty-bracket declarator has an initializer list, count its top-level
elements and replace the declarator's type with an interned concrete array.
When it has no initializer, retain `CG_ARRAY_UNSIZED` and require a context
permitted by the specification.

For assignment to an unsized array, validate compatible element type, store a
typed dynamic-array assignment node, and give the destination the runtime shape
of the source without mutating the canonical declared type object.

- [ ] **Step 4: Make `.length` a typed operation**

In `NewMemberSelectorOrSwizzleOrWriteMaskOperator`, detect atom `length` on an
array before structure lookup. Return a new `ARRAY_LENGTH_OP` expression of
canonical `int` type. Constant-fold it for sized arrays and leave it explicit
for unsized arrays.

- [ ] **Step 5: Preserve nested packedness and array copy semantics**

Update `IsSameUnqualifiedType`, assignment, parameter passing, return values,
and aggregate initialization to distinguish packed from unpacked arrays at
every nesting layer. Permit full-array rvalue copy. Keep profile limits on
computed indexing and lvalue indexing out of these language helpers.

- [ ] **Step 6: Add the negative fixture**

Create `tests/cg20/diagnostics/unsized_initializer.cg` with an empty initializer
for an unsized array. Register a language diagnostic asserting the exact source
line and text `cannot infer array size`.

- [ ] **Step 7: Regenerate and verify**

Run:

```powershell
cmake --build build-cg20 --config Release --target regenerate_parser regenerate_stdlib
cmake --build build-cg20 --config Release
ctest --test-dir build-cg20 -C Release -R "cg20_.*array|glslv_.*struct_array|generic_" --output-on-failure
ctest --test-dir build-cg20 -C Release --output-on-failure
```

Expected: all array and existing aggregate regressions pass.

- [ ] **Step 8: Commit array completion**

```powershell
git add cg_types.h cg_types.c symbols.h symbols.c support.h support.c parser.y parser.c parser.h errors.h stdlib.c tests/CMakeLists.txt tests/cg20
git commit -m "Complete Cg 2.0 array semantics"
```

## Task 9: Model the Complete Sampler Family as Language Types

**Files:**

- Modify: `cg_types.h`, `cg_types.c`
- Modify: `symbols.h`, `symbols.c`
- Modify: `support.c:1970-2110,2930-3150`
- Modify: `semantic.c:150-610`
- Modify: `generic_hal.c`, `glsl_hal.c`, `glsl_lower.c`
- Modify: `errors.h`
- Create: `tests/cg20/samplers/compatible_base.cg`
- Create: `tests/cg20/samplers/rect.cg`
- Create: `tests/cg20/diagnostics/sampler_assignment.cg`
- Create: `tests/cg20/diagnostics/sampler_incompatible.cg`
- Modify: `tests/CMakeLists.txt`, `tests/cg20/conformance.csv`

- [ ] **Step 1: Add positive and negative sampler fixtures**

`compatible_base.cg` declares a helper taking base `sampler` and calls it with
`sampler1D`, `sampler2D`, `sampler3D`, `samplerCUBE`, and `samplerRECT` program
parameters in separate entry helpers. `rect.cg` passes `samplerRECT` through an
ordinary `in` helper.

`sampler_assignment.cg` assigns one sampler variable to another.
`sampler_incompatible.cg` passes `sampler2D` where `sampler3D` is required.
Register the positive fixtures under `generic` and the negative fixtures as
language errors. Run and record the current failures.

- [ ] **Step 2: Define sampler identity separately from scalar kind**

Add to `cg_types.h`:

```c
typedef enum CgSamplerKind_Rec {
    CG_SAMPLER_BASE = 0,
    CG_SAMPLER_1D,
    CG_SAMPLER_2D,
    CG_SAMPLER_3D,
    CG_SAMPLER_CUBE,
    CG_SAMPLER_RECT,
    CG_SAMPLER_COUNT
} CgSamplerKind;

int IsSampler(const Type *type, CgSamplerKind *kind);
Type *GetSamplerType(CgSamplerKind kind);
int CgSamplerCompatible(CgSamplerKind formalKind, CgSamplerKind actualKind);
```

Add `TYPE_CATEGORY_SAMPLER` and a `TypeSampler` union member with the common
prefix plus `CgSamplerKind samplerKind`.

- [ ] **Step 3: Register the six sampler typedefs**

During symbol-table initialization, register `sampler`, `sampler1D`,
`sampler2D`, `sampler3D`, `samplerCUBE`, and `samplerRECT` as canonical type
symbols. Preserve an adapter from each canonical sampler to the existing HAL
texture-object base only inside current backend validation/lowering.

- [ ] **Step 4: Enforce language-level sampler restrictions**

Permit samplers only as program/function formal parameters and permitted
global uniform declarations. Treat program sampler parameters as implicit
`const`. Allow only copying through parameter passing as an `in` argument.
Reject assignment, construction, comparison, conditional selection, return,
out/inout use, incompatible specific sampler conversion, and inconsistent use
of deprecated base `sampler`.

- [ ] **Step 5: Keep valid samplers frontend-neutral**

Remove sampler availability decisions from frontend HAL callbacks. Generic
accepts all six kinds. GLSL validation continues to reject `samplerRECT` and
stage-inappropriate sampling with existing profile diagnostics, after the
language phase succeeds.

- [ ] **Step 6: Run sampler and GLSL texture suites**

Run:

```powershell
cmake --build build-cg20 --config Release
ctest --test-dir build-cg20 -C Release -R "cg20_.*sampler|glsl.*sampler|glslf_tex|glslv_sampler" --output-on-failure
```

Expected: new language tests and all existing GLSL texture tests pass.

- [ ] **Step 7: Commit sampler language types**

```powershell
git add cg_types.h cg_types.c symbols.h symbols.c support.c semantic.c generic_hal.c glsl_hal.c glsl_lower.c errors.h tests/CMakeLists.txt tests/cg20
git commit -m "Model Cg 2.0 sampler types"
```

## Task 10: Enforce Core Binding-Semantic and Program Interface Rules

**Files:**

- Modify: `semantic.c:150-610`
- Modify: `symbols.h`, `symbols.c`
- Modify: `support.c:1330-1460`
- Modify: `errors.h`
- Create: `tests/cg20/profile/default_domains.cg`
- Create: `tests/cg20/profile/input_alias.cg`
- Create: `tests/cg20/profile/global_varying.cg`
- Create: `tests/cg20/diagnostics/output_alias.cg`
- Create: `tests/cg20/diagnostics/semantic_case_collision.cg`
- Modify: `tests/CMakeLists.txt`, `tests/cg20/conformance.csv`

- [ ] **Step 1: Add binding fixtures before changing semantics**

`default_domains.cg` declares one non-static global without a domain and a
top-level parameter without a domain; assert the global is uniform and the
parameter varying in generic output. `input_alias.cg` binds two input
parameters to `TEXCOORD0`, writes one local copy, and returns the unchanged
other copy. `global_varying.cg` reads a global varying input and writes a global
varying output.

`output_alias.cg` binds two outputs to `COLOR0`.
`semantic_case_collision.cg` binds outputs to `COLOR0` and `color0`. Register
success and exact failure diagnostics, then run them to expose any current
rule gaps.

- [ ] **Step 2: Centralize semantic-name identity**

Add helpers in `semantic.c`:

```c
static int CanonicalSemanticAtom(int semantic)
{
    const char *source = GetAtomString(atable, semantic);
    char text[MAX_SYMBOL_NAME_LEN + 1];
    int index;

    for (index = 0; source[index] && index < MAX_SYMBOL_NAME_LEN; ++index)
        text[index] = (char) toupper((unsigned char) source[index]);
    text[index] = '\0';
    return LookUpAddString(atable, text);
}
```

Include `<ctype.h>` for `toupper`.

Use the canonical atom for equality/conflict checks while retaining the source
atom for diagnostics and normalized output.

- [ ] **Step 3: Apply default program domains in one helper**

Implement and call:

```c
static int EffectiveProgramDomain(const Symbol *symbol, int isTopLevelParam)
{
    int domain = GetDomain(symbol->type);
    if (domain != TYPE_DOMAIN_UNKNOWN)
        return domain;
    return isTopLevelParam ? TYPE_DOMAIN_VARYING : TYPE_DOMAIN_UNIFORM;
}
```

Ignore program-domain qualifiers and binding semantics on non-top-level helper
parameters as required by the language. Keep `in`, `out`, and `inout` function
parameter directions intact.

- [ ] **Step 4: Enforce copy-in/copy-out aliasing**

Allow repeated input semantic atoms and synthesize distinct input copies for
each parameter. Track canonical output semantic atoms while building the
program interface; reject the second output using an output-alias diagnostic
with a note at the first declaration. Include return semantics, out parameters,
struct members, and global varying outputs in the same map.

- [ ] **Step 5: Keep allocation and target limits in HAL validation**

Language semantics records explicit or absent bindings but does not reject an
unbound connector or impose register counts. Existing GLSL automatic
allocation and resource limits remain profile checks after reachability.

- [ ] **Step 6: Run binding and existing interface tests**

Run:

```powershell
cmake --build build-cg20 --config Release
ctest --test-dir build-cg20 -C Release -R "cg20_.*(domain|alias|varying|semantic)|glsl.*semantics|glsl.*interface" --output-on-failure
ctest --test-dir build-cg20 -C Release --output-on-failure
```

Expected: new binding fixtures, existing GLSL semantics/interface tests, and
the full suite pass.

- [ ] **Step 7: Commit core interface semantics**

```powershell
git add semantic.c symbols.h symbols.c support.c errors.h tests/CMakeLists.txt tests/cg20
git commit -m "Enforce Cg program interface semantics"
```

## Task 11: Parse and Validate Interfaces, Struct Methods, and Inheritance

**Files:**

- Modify: `atom.c`
- Modify: `parser.y:400-455,790-810`
- Regenerate: `parser.c`, `parser.h`
- Modify: `symbols.h:130-270`, `symbols.c`
- Modify: `support.h`, `support.c:1630-1900,2870-3150`
- Modify: `errors.h`, `printutils.c`
- Create: `tests/cg20/interfaces/conformance.cg`
- Create: `tests/cg20/interfaces/dispatch.cg`
- Create: `tests/cg20/diagnostics/interface_data.cg`
- Create: `tests/cg20/diagnostics/interface_missing_method.cg`
- Create: `tests/cg20/diagnostics/interface_bad_signature.cg`
- Modify: `tests/CMakeLists.txt`, `tests/cg20/conformance.csv`

- [ ] **Step 1: Add failing interface fixtures**

Create `tests/cg20/interfaces/conformance.cg`:

```c
interface Scale
{
    float apply(float value);
};

struct Twice : Scale
{
    float apply(float value)
    {
        return value * 2.0;
    }
};

float run(Scale operation, float value)
{
    return operation.apply(value);
}

float4 main(float4 p : POSITION) : POSITION
{
    Twice operation;
    return p + run(operation, 1.0) * 0.0;
}
```

Add `dispatch.cg` assigning two different implementing structs to one
interface variable in opposite conditional branches. Register both under
generic and run; expect parser failures.

- [ ] **Step 2: Extend type and symbol records**

Add `TYPE_CATEGORY_INTERFACE`. Add `TypeInterface` with the common type prefix,
member scope, source location, and tag. Add to `TypeStruct`:

```c
Type *implementedInterface;
```

Add to `FunSymbol`:

```c
Type *ownerType;
int isMethod;
```

Add public helpers `InterfaceHeader`, `SetInterfaceMembers`,
`SetStructInterface`, and `CheckInterfaceConformance`.

- [ ] **Step 3: Add grammar for interface declarations and methods**

Parse:

```yacc
interface_specifier:
      INTERFACE_SY struct_identifier interface_compound_header
      interface_member_declaration_list '}'
;
```

Restrict interface members to function declarations ending in `;`. Extend
struct member productions to accept both data declarations and complete method
definitions. Interpret `struct Name : Type` as interface inheritance when
`Type` resolves to an interface; preserve the existing connector semantic form
only when the right-hand identifier is not a type.

- [ ] **Step 4: Add typed method selection and calls**

Member selection on a struct or interface searches its member scope. A method
selection expression stores both the receiver expression and selected method
symbol. Reuse ordinary argument conversion after prepending the implicit
receiver internally; do not expose that receiver as a source-level formal.

For an interface receiver, create `INTERFACE_CALL_OP` instead of an ordinary
direct call. Preserve its declared interface result type.

- [ ] **Step 5: Validate conformance at struct completion**

For every interface method, require exactly one struct method with the same
name, return type, parameter count, parameter directions, and unqualified
parameter types. Report a primary error at the struct with a note at the
interface method. Reject interface data members and method bodies.

- [ ] **Step 6: Add negative fixtures and run tests**

`interface_data.cg` puts a float field in an interface.
`interface_missing_method.cg` omits `apply`.
`interface_bad_signature.cg` implements `apply(int)` instead of
`apply(float)`. Register exact diagnostic codes and source lines.

Run:

```powershell
cmake --build build-cg20 --config Release --target regenerate_parser regenerate_stdlib
cmake --build build-cg20 --config Release
ctest --test-dir build-cg20 -C Release -R "cg20_.*interface|stdlib_regeneration" --output-on-failure
ctest --test-dir build-cg20 -C Release --output-on-failure
```

Expected: all focused and full tests pass.

- [ ] **Step 7: Commit interface support**

```powershell
git add atom.c parser.y parser.c parser.h symbols.h symbols.c support.h support.c errors.h printutils.c stdlib.c tests/CMakeLists.txt tests/cg20
git commit -m "Implement Cg 2.0 interfaces"
```

## Task 12: Implement Default Arguments and Profile-Qualified Overload Resolution

**Files:**

- Create: `cg_overload.h`, `cg_overload.c`
- Create: `tests/cg_overload_test.c`
- Modify: `symbols.h`, `symbols.c`
- Modify: `parser.y:275-385,485-545`
- Regenerate: `parser.c`, `parser.h`
- Modify: `support.h`, `support.c:1210-1245,1700-1815,2930-3150`
- Modify: `hal.h:85-215`, `hal.c:95-175`
- Modify: `generic_hal.c`, `glslv_hal.c`, `glslf_hal.c`
- Modify: `errors.h`
- Create: `tests/cg20/overloads/defaults.cg`
- Create: `tests/cg20/overloads/profile.cg`
- Create: `tests/cg20/diagnostics/default_after_optional.cg`
- Create: `tests/cg20/diagnostics/ambiguous_overload.cg`
- Modify: `CMakeLists.txt`, `tests/CMakeLists.txt`, `tests/cg20/conformance.csv`

- [ ] **Step 1: Write a resolver unit test before extracting current logic**

Create `tests/cg_overload_test.c` with the repository license. Construct three
candidate descriptors for `pick(float)`: open profile, `vs`, and exact
`glslv`. Assert that `CgResolveOverload` chooses exact `glslv`, then `vs` for a
different vertex profile, then open for a profile with no selector match. Add
an argument-ranking case where an exact `half` candidate beats a promotable
`float` candidate.

Use this public API in the test:

```c
typedef struct CgOverloadResult_Rec {
    Symbol *symbol;
    int usedDefaults;
    int ambiguous;
} CgOverloadResult;

int CgResolveOverload(const CgProfileIdentity *profile, Symbol *first,
                      expr *actuals, CgOverloadResult *result);
```

Register `cg_overload_unit`, run it, and expect missing header/API failures.

- [ ] **Step 2: Define profile identities and selectors**

Add to `cg_overload.h`:

```c
typedef enum CgProfileStage_Rec {
    CG_PROFILE_STAGE_NEUTRAL = 0,
    CG_PROFILE_STAGE_VERTEX,
    CG_PROFILE_STAGE_FRAGMENT
} CgProfileStage;

typedef struct CgProfileSelector_Rec {
    int name;
    int specificity;
    int isOpen;
} CgProfileSelector;

typedef struct CgProfileIdentity_Rec {
    int exactName;
    CgProfileStage stage;
    const int *wildcards;
    const int *specificity;
    int wildcardCount;
} CgProfileIdentity;
```

Store `CgProfileSelector profileSelector` in `FunSymbol`. Add a
`CgProfileIdentity profileIdentity` member to `slHAL`. Initialize generic as
neutral/open, `glslv` as vertex with wildcard `vs`, and `glslf` as fragment
with wildcard `ps`.

- [ ] **Step 3: Parse a profile name before a function return type**

Add a `profile_specifier` grammar production that accepts an identifier only
when it resolves through `EnumerateProfiles` or the registered wildcard atom
table. Attach it to the following function declaration/definition rather than
to ordinary variable declarations. If a typedef shadows the profile name, the
existing type interpretation wins.

Store open profile when no selector appears. Assign exact selectors a higher
specificity than wildcards; use the HAL-provided integer for wildcard ordering.

- [ ] **Step 4: Validate and store default parameters**

Use the existing parameter initializer expression as the stored default on its
parameter symbol. Enforce:

- Only trailing parameters may have defaults.
- A non-top-level helper default is allowed only for an `in` parameter.
- A top-level default is allowed only for a uniform parameter.
- Each default is a compile-time constant convertible to the parameter type.
- A redeclaration cannot change an existing default.

Clone the converted default expression into the call's actual-argument list so
later passes see an ordinary complete call.

- [ ] **Step 5: Replace the mutating legacy resolver**

Move `lResolveOverloadedFunction` out of `support.c`. In `cg_overload.c`, keep
candidate state in a temporary linked list allocated from the current scope's
pool; never mutate `FunSymbol.flags` during resolution.

For each actual argument from left to right, retain candidates in this order:
exact unqualified type, compatible dynamic/interface type, promotion, then
implicit conversion. After arguments, filter arity/defaults, exact profile,
most-specific wildcard, then open profile. Return ambiguity only when more
than one candidate remains.

- [ ] **Step 6: Add shader fixtures**

`defaults.cg` calls a helper with zero, one, and two explicit arguments where
the final two have constant defaults. `profile.cg` defines open, `vs`, `ps`,
`glslv`, and `glslf` overloads and compiles separate vertex and fragment entry
points with expected generic IR selection markers.

`default_after_optional.cg` places a required parameter after an optional one.
`ambiguous_overload.cg` supplies two candidates with identical conversion
rank. Register exact language diagnostics.

- [ ] **Step 7: Regenerate and run resolver tests**

Run:

```powershell
cmake --build build-cg20 --config Release --target regenerate_parser regenerate_stdlib
cmake --build build-cg20 --config Release
ctest --test-dir build-cg20 -C Release -R "cg_overload_unit|cg20_.*overload|cg20_.*default|glsl.*builtin" --output-on-failure
ctest --test-dir build-cg20 -C Release --output-on-failure
```

Expected: all focused and full tests pass.

- [ ] **Step 8: Commit overload resolution**

```powershell
git add cg_overload.h cg_overload.c symbols.h symbols.c parser.y parser.c parser.h support.h support.c hal.h hal.c generic_hal.c glslv_hal.c glslf_hal.c errors.h stdlib.c CMakeLists.txt tests/CMakeLists.txt tests/cg_overload_test.c tests/cg20
git commit -m "Implement Cg 2.0 overload resolution"
```

## Task 13: Add the Declarative Cg 2.0 Standard Library and Typed Intrinsics

**Files:**

- Create: `cg_stdlib.def`, `cg_stdlib.h`, `cg_stdlib.c`
- Create: `tests/cg_stdlib_test.c`
- Modify: `symbols.h:130-230`, `symbols.c:130-220`
- Modify: `support.h`, `support.c:3050-3145`
- Modify: `stdlib.cg`, regenerate `stdlib.c`
- Modify: `hal.h`, `hal.c`, `generic_hal.c`, `glslv_hal.c`, `glslf_hal.c`
- Modify: `glsl_ir.h`, `glsl_ir.c`, `glsl_lower.c`
- Modify: `CMakeLists.txt`, `tests/CMakeLists.txt`, `tests/cg20/conformance.csv`
- Create: `tests/cg20/intrinsics/numeric.cg`
- Create: `tests/cg20/intrinsics/out_params.cg`
- Create: `tests/cg20/intrinsics/texture_families.cg`
- Create: `tests/cg20/intrinsics/debug.cg`
- Create: `tests/cg20/intrinsics/predefined_outputs.cg`

- [ ] **Step 1: Write a completeness unit test around a stable catalog**

Create `tests/cg_stdlib_test.c`. Assert that every entry has a nonempty name,
nonzero intrinsic, at least one expanded signature, unique `(name, signature)`
identity, and a unique stable intrinsic opcode. Assert the catalog contains
every function named by Tables 1-5 of the Cg 2.0 User's Manual:

Pin the audit source to NVIDIA's archived 2.0.0010 manual at
`https://developer.download.nvidia.com/cg/Cg_2.0/2.0.0010/CgUsersManual.pdf`;
do not use the evolving live standard-library index as the version baseline.

```text
abs acos all any asin atan atan2 ceil clamp cos cosh cross degrees
determinant dot exp exp2 floor fmod frac frexp isfinite isinf isnan
ldexp lerp lit log log10 log2 max min modf mul noise pow radians round
rsqrt saturate sign sin sincos sinh smoothstep sqrt step tan tanh
transpose distance faceforward length normalize reflect refract ddx ddy
debug tex1D tex1Dproj tex2D tex2Dproj tex3D tex3Dproj texCUBE
texCUBEproj texRECT texRECTproj
```

For every texture name, also require the documented `h4` and `x4` prefixed
forms (for example, `h4tex2D` and `x4tex2D`). Assert that the predefined
`fragout` and `fragout_float` helper-structure variants are present in the
profile-qualified catalog. Assert that later names such as `clip` and `fwidth`
are absent so the pinned Cg 2.0 catalog cannot silently drift to Cg 3.1.

Register `cg_stdlib_unit`, run it, and expect missing catalog failures.

- [ ] **Step 2: Define stable intrinsic identities**

Create `cg_stdlib.h`:

```c
#if !defined(__CG_STDLIB_H)
#define __CG_STDLIB_H 1

#include "symbols.h"

typedef enum CgIntrinsic_Rec {
    CG_INTRINSIC_NONE = 0,
#define CG_INTRINSIC(id, name, flags) CG_INTRINSIC_##id,
#include "cg_stdlib.def"
#undef CG_INTRINSIC
    CG_INTRINSIC_COUNT
} CgIntrinsic;

typedef struct CgIntrinsicSignature_Rec {
    CgIntrinsic intrinsic;
    const char *name;
    Type *result;
    TypeList *parameters;
    unsigned flags;
} CgIntrinsicSignature;

int InitCgStdlib(Scope *scope);
const CgIntrinsicSignature *CgIntrinsicSignatureForSymbol(const Symbol *symbol);
int CgStdlibCatalogCount(void);
const char *CgStdlibCatalogName(int index);

#endif
```

Do not include `cg_stdlib.h` from `symbols.h`. Forward-declare
`struct CgIntrinsicSignature_Rec` there and store
`const struct CgIntrinsicSignature_Rec *intrinsic` in `FunSymbol`; this avoids
a header cycle while keeping the selected signature immutable.

Use fixed flags for pure, derivative, texture, out-parameter, and
constant-foldable behavior.

- [ ] **Step 3: Populate `cg_stdlib.def` by overload family**

Use macro rows such as:

```c
CG_INTRINSIC(ABS, "abs", CG_INTRINSIC_PURE | CG_INTRINSIC_FOLDABLE)
CG_INTRINSIC(SINCOS, "sincos", CG_INTRINSIC_OUT_PARAMS)
CG_INTRINSIC(TEX2D, "tex2D", CG_INTRINSIC_TEXTURE)
```

In `cg_stdlib.c`, expand families programmatically for scalar kinds `fixed`,
`half`, and `float`; vector lengths 1-4; square and non-square matrix shapes
where the documented operation permits them; and documented integer/bool
families. Define explicit signature builders for `cross`, `lit`, `mul`,
`sincos`, `frexp`, `modf`, derivative functions, and every documented Cg 2.0
texture coordinate, projection, depth-compare, and explicit-gradient form.
Generate `h4` and `x4` texture variants from the same rows with half4 and
fixed4 results. Do not import later `bias`, `lod`, `fetch`, `size`, array,
multisample, integer-sampler, `clip`, or `fwidth` entries from the live Cg 3.1
reference.

Represent `fragout` and `fragout_float` as declarative helper-structure rows
selected by the current profile family. Install the Cg 2.0 definitions during
profile standard-library setup, and diagnose use where the selected profile
does not define that variant. Keep these rows separate from intrinsic opcode
generation because they introduce types, not calls.

- [ ] **Step 4: Install catalog overloads as ordinary symbols**

During standard-library initialization, create internal function symbols for
every expanded signature. Store a pointer to its immutable
`CgIntrinsicSignature` in `FunSymbol`. Remove duplicate internal declarations
from `stdlib.cg`; retain portable function bodies such as `lerp`, `saturate`,
and matrix helpers when their implementation is itself valid Cg.

After overload selection, construct `FUN_INTRINSIC_OP` with the signature
pointer rather than the old `(group << 16) | index` encoding.

- [ ] **Step 5: Map the existing GLSL subset by intrinsic identity**

Replace name-based lookup in `glsl_ir.c`/`glsl_lower.c` with a switch on
`CgIntrinsic`. Preserve exact current spellings such as `lerp` to `mix`,
`frac` to `fract`, and `rsqrt` to `inversesqrt`. A cataloged intrinsic without
an exact GLSL 1.10 lowering reaches profile validation and emits the existing
GLSL intrinsic diagnostic.

- [ ] **Step 6: Add standard-library fixtures**

`numeric.cg` calls every ordinary non-texture name at least once using a
minimal documented signature. `out_params.cg` covers `sincos`, `frexp`, and
`modf` with real lvalues. `texture_families.cg` declares all sampler kinds and
calls every base, projected, explicit-gradient, `h4`, and `x4` texture family
from helpers; compile it through generic only. `debug.cg` calls `debug(float4)`
and verifies its side-effect flag. `predefined_outputs.cg` exercises each
profile-defined helper structure under a matching and a rejecting profile.

Generate unit-level signature coverage from the catalog rather than writing a
shader for every scalar/vector overload combination.

- [ ] **Step 7: Regenerate and run catalog tests**

Run:

```powershell
cmake --build build-cg20 --config Release --target regenerate_stdlib
cmake --build build-cg20 --config Release
ctest --test-dir build-cg20 -C Release -R "cg_stdlib_unit|cg20_.*intrinsic|glsl.*numeric_intrinsic|glslf_tex" --output-on-failure
ctest --test-dir build-cg20 -C Release --output-on-failure
```

Expected: catalog, fixtures, regeneration, and full suite pass.

- [ ] **Step 8: Commit the catalog and regenerated stream**

```powershell
git add cg_stdlib.def cg_stdlib.h cg_stdlib.c symbols.h symbols.c support.h support.c stdlib.cg stdlib.c hal.h hal.c generic_hal.c glslv_hal.c glslf_hal.c glsl_ir.h glsl_ir.c glsl_lower.c CMakeLists.txt tests/CMakeLists.txt tests/cg_stdlib_test.c tests/cg20
git commit -m "Add the Cg 2.0 intrinsic catalog"
```

## Task 14: Define Cg IR Types, Nodes, Ownership, and Builders

**Files:**

- Create: `cg_ir.h`, `cg_ir.c`
- Create: `tests/cg_ir_test.c`
- Modify: `CMakeLists.txt`, `tests/CMakeLists.txt`

- [ ] **Step 1: Write builder and ownership tests first**

Create `tests/cg_ir_test.c` with assertion forcing like `glsl_ir_test.c`.
Initialize a module with a counting allocator, add one float4 parameter, one
function, a return statement, and a float4 constructor. Assert node types,
source locations, canonical result types, declaration order, and allocation
count. Add a failing allocator case that leaves the module in a failed state
without returning partially initialized nodes.

Use this initialization API:

```c
void CgIRInitModule(CgIRModule *module, void *(*alloc)(void *, size_t),
                    void *allocArg);
int CgIRModuleFailed(const CgIRModule *module);
```

Register `cg_ir_unit`, run it, and expect missing files.

- [ ] **Step 2: Define the module and stable enums**

Create `cg_ir.h` with forward declarations plus:

```c
typedef enum CgIRExprKind_Rec {
    CGIR_EXPR_CONSTANT,
    CGIR_EXPR_SYMBOL,
    CGIR_EXPR_MEMBER,
    CGIR_EXPR_INDEX,
    CGIR_EXPR_LENGTH,
    CGIR_EXPR_SWIZZLE,
    CGIR_EXPR_CONSTRUCT,
    CGIR_EXPR_CAST,
    CGIR_EXPR_UNARY,
    CGIR_EXPR_BINARY,
    CGIR_EXPR_ASSIGN,
    CGIR_EXPR_CONDITIONAL,
    CGIR_EXPR_CALL,
    CGIR_EXPR_INTERFACE_CALL,
    CGIR_EXPR_INTRINSIC
} CgIRExprKind;

typedef enum CgIRStmtKind_Rec {
    CGIR_STMT_BLOCK,
    CGIR_STMT_DECL,
    CGIR_STMT_EXPR,
    CGIR_STMT_IF,
    CGIR_STMT_WHILE,
    CGIR_STMT_DO,
    CGIR_STMT_FOR,
    CGIR_STMT_RETURN,
    CGIR_STMT_BREAK,
    CGIR_STMT_CONTINUE,
    CGIR_STMT_DISCARD
} CgIRStmtKind;
```

Every expression begins with kind, canonical `Type *`, `SourceLoc`, synthesized
flag, lvalue flag, and side-effect flag. Every statement begins with kind,
`SourceLoc`, synthesized flag, and next pointer.

- [ ] **Step 3: Define declarations and module ownership**

`CgIRModule` stores allocator callback/argument, failed flag, selected entry,
ordered globals, ordered functions, and profile identity. `CgIRDecl` stores
symbol identity, source name atom, canonical type, binding semantic,
initializer, source location, and storage/domain flags. `CgIRFunction` stores
symbol identity, result type, ordered parameters, locals, body, and entry flag.

Use module allocation for every node and list link. Builders return `NULL` and
set `module->failed` after any allocation failure.

- [ ] **Step 4: Implement typed expression builders**

Provide one builder per expression kind. Each builder takes an explicit result
type and source location. `CgIRCall` stores the selected `Symbol *` identity;
`CgIRIntrinsicCall` stores `CgIntrinsic` and selected immutable signature;
`CgIRInterfaceCall` stores the declared interface method and receiver.

Builders perform only cheap local assertions such as non-null required
operands. Complete validation belongs to Task 15.

- [ ] **Step 5: Implement structured statement builders**

Provide block/list append, declaration, expression, if, loop, return, jump,
and discard builders. Preserve structured control flow and source order. Do not
inline, flatten, unroll, or lower aggregate operations in this layer.

- [ ] **Step 6: Test clean and dirty allocators**

Run:

```powershell
cmake -S . -B build-cg20 -DBUILD_TESTING=ON
cmake --build build-cg20 --config Release --target cg_ir_unit
ctest --test-dir build-cg20 -C Release -R "^cg_ir_unit$" --output-on-failure
```

Expected: unit test passes with normal, zeroed, dirty, and failing allocators.

- [ ] **Step 7: Commit the IR core**

```powershell
git add cg_ir.h cg_ir.c tests/cg_ir_test.c CMakeLists.txt tests/CMakeLists.txt
git commit -m "Add backend-neutral Cg IR"
```

## Task 15: Verify Every Cg IR Invariant Before Profile Validation

**Files:**

- Create: `cg_ir_verify.c`
- Modify: `cg_ir.h`
- Modify: `errors.h`
- Modify: `tests/cg_ir_test.c`
- Modify: `CMakeLists.txt`, `tests/CMakeLists.txt`

- [ ] **Step 1: Add rejected-module tests**

Extend `tests/cg_ir_test.c` with helpers that create and reject:

- A binary expression whose result type disagrees with its operands.
- An assignment to a non-lvalue.
- A call with the wrong arity.
- An intrinsic with a mismatched signature.
- A return whose value cannot convert to the function result.
- `break` outside a loop.
- A discard statement carrying a non-Boolean predicate.
- An interface call with a receiver of the wrong interface.
- A user-derived node with an empty source location.

Each case asserts `!CgIRVerifyModule(&module, &diagnostic)` and checks a stable
internal reason enum rather than user-facing text.

- [ ] **Step 2: Define verifier diagnostics**

Add to `cg_ir.h`:

```c
typedef enum CgIRVerifyReason_Rec {
    CGIR_VERIFY_OK = 0,
    CGIR_VERIFY_TYPE,
    CGIR_VERIFY_OWNER,
    CGIR_VERIFY_OPERAND,
    CGIR_VERIFY_LVALUE,
    CGIR_VERIFY_CALL,
    CGIR_VERIFY_INTRINSIC,
    CGIR_VERIFY_CONTROL,
    CGIR_VERIFY_INTERFACE,
    CGIR_VERIFY_LOCATION
} CgIRVerifyReason;

typedef struct CgIRVerifyDiagnostic_Rec {
    CgIRVerifyReason reason;
    SourceLoc loc;
    const void *node;
} CgIRVerifyDiagnostic;

int CgIRVerifyModule(const CgIRModule *module,
                     CgIRVerifyDiagnostic *diagnostic);
```

- [ ] **Step 3: Implement recursive type and ownership verification**

Walk module declarations and functions in source order. Track current
function, current loop depth, and declared symbol identities. Verify canonical
types, declaration ownership, operand counts/types, lvalues, unique write-mask
components, call directions/signatures, intrinsic identity/signature, return
compatibility, interface compatibility, and required source locations.

Stop at the first invariant failure so release builds produce one controlled
internal diagnostic.

- [ ] **Step 4: Add an assertion seam**

Make `cg_ir_unit --verify-assertions-active` use the same pattern as
`glsl_ir_unit`. Register an assertion-active CTest through
`check_assertions_active.cmake`.

- [ ] **Step 5: Run verifier tests**

Run:

```powershell
cmake --build build-cg20 --config Release --target cg_ir_unit
ctest --test-dir build-cg20 -C Release -R "cg_ir_(unit|assertions_active)" --output-on-failure
```

Expected: valid module passes, every malformed module is rejected, and
assertions are active in the unit binary.

- [ ] **Step 6: Commit the verifier**

```powershell
git add cg_ir.h cg_ir_verify.c errors.h tests/cg_ir_test.c CMakeLists.txt tests/CMakeLists.txt
git commit -m "Verify Cg IR invariants"
```

## Task 16: Compute Reachability and Lower the Typed Frontend Tree to Cg IR

**Files:**

- Create: `cg_reach.h`, `cg_reach.c`
- Create: `cg_ir_lower.h`, `cg_ir_lower.c`
- Modify: `compile.c:2740-2840`
- Modify: `support_iter.c`
- Modify: `errors.h`
- Modify: `tests/cg_ir_test.c`
- Create: `tests/cg20/ir/reachable.cg`
- Create: `tests/cg20/ir/unreachable_profile_feature.cg`
- Modify: `CMakeLists.txt`, `tests/CMakeLists.txt`, `tests/cg20/conformance.csv`

- [ ] **Step 1: Add reachability tests before implementation**

Create `reachable.cg` with `main -> helperA -> helperB`, one referenced uniform,
one unused helper, and one unused global. Create
`unreachable_profile_feature.cg` whose unused helper performs an operation
known to be rejected by `glslv`.

Add a unit test constructing the same symbol graph and asserting this API:

```c
int CgReachBuild(Symbol *entry, CgReachGraph *graph);
int CgReachContainsSymbol(const CgReachGraph *graph, const Symbol *symbol);
const CgReachEdge *CgReachWitness(const CgReachGraph *graph,
                                 const Symbol *symbol);
```

Run it and expect missing reachability API failures.

- [ ] **Step 2: Implement deterministic graph traversal**

Walk each function body for direct calls and global symbol references. Add the
entry first, then visit functions and globals in `sourceOrdinal` order. Record
the first parent edge for each node as its diagnostic witness. Detect recursive
cycles without infinite traversal; recursion remains a later profile decision.

- [ ] **Step 3: Define the lowering entry point**

Create `cg_ir_lower.h`:

```c
typedef struct CgIRLowerContext_Rec {
    CgIRModule *module;
    const CgReachGraph *reach;
    CgIRVerifyDiagnostic verifyDiagnostic;
} CgIRLowerContext;

int CgIRLowerProgram(CgIRLowerContext *context, Scope *globalScope,
                     Symbol *entry);
```

- [ ] **Step 4: Lower declarations, expressions, and statements one-for-one**

Map every typed frontend expression and statement to the corresponding Cg IR
kind. Preserve canonical type, selected symbol, intrinsic signature, source
location, binding semantic, lvalue status, and side-effect status. Keep array
copy, dynamic-array assignment, interface dispatch, constructors, swizzles,
and structured control flow explicit.

Do not run legacy inlining, comma flattening, matrix deconstruction, struct
assignment flattening, or if flattening before Cg IR construction.

- [ ] **Step 5: Integrate verified IR into compile control without generation**

After entry selection and language checking in `CompileProgram`, build the
reach graph, initialize the module, lower the program, and call
`CgIRVerifyModule`. On failure, emit one internal compiler diagnostic at the
verifier location and stop. Keep the legacy code-generation path active after
successful verification until Task 17 adds HAL hooks.

- [ ] **Step 6: Run reachability and full regression tests**

Run:

```powershell
cmake --build build-cg20 --config Release
ctest --test-dir build-cg20 -C Release -R "cg_ir_unit|cg20_.*reachable|glslv_unsupported" --output-on-failure
ctest --test-dir build-cg20 -C Release --output-on-failure
```

Expected: the unused unsupported helper does not cause a profile failure; all
legacy generation remains unchanged.

- [ ] **Step 7: Commit reachability and lowering**

```powershell
git add cg_reach.h cg_reach.c cg_ir_lower.h cg_ir_lower.c compile.c support_iter.c errors.h tests/cg_ir_test.c tests/CMakeLists.txt tests/cg20 CMakeLists.txt
git commit -m "Lower reachable programs to Cg IR"
```

## Task 17: Add IR-Oriented HAL Hooks and Normalized Generic Emission

**Files:**

- Create: `cg_ir_print.c`
- Modify: `cg_ir.h`
- Modify: `hal.h:132-215`, `hal.c:120-210`
- Modify: `compile.c:2740-2840`
- Modify: `generic_hal.c:155-193,410-450`
- Modify: `generic_hal.h`
- Modify: `tests/cg_ir_test.c`
- Create: `tests/cg20/ir/normalized.expected`
- Modify: `tests/check_cg20.cmake`, `tests/CMakeLists.txt`

- [ ] **Step 1: Add a normalized-printer unit golden**

Extend `tests/cg_ir_test.c` to build a module containing one uniform, one
struct, one helper, one intrinsic, an if, a loop, and an interface call. Print
to `tmpfile`, rewind it, and compare byte-for-byte with one literal expected
string. The expected form must include canonical type spellings, source names,
binding semantics, selected call identities, and structured indentation.

Declare and call:

```c
int CgIRPrintModule(FILE *out, const CgIRModule *module);
```

Run `cg_ir_unit`; expect a missing symbol failure.

- [ ] **Step 2: Implement deterministic normalized printing**

In `cg_ir_print.c`, print declarations in module order and functions in
reachability/source order. Use two-space indentation, decimal numeric values,
canonical `CgScalarKindName`, explicit packed/unpacked and sized/unsized array
notation, and stable intrinsic enum spellings. Never print pointer values,
allocator addresses, or traversal-dependent hash order.

Call `CgIRVerifyModule` before writing the first byte. Return failure without
partial node text when verification fails or the stream reports an error.

- [ ] **Step 3: Add HAL IR callbacks**

Forward-declare `CgIRModule` in `hal.h` and add:

```c
int (*ValidateIR)(SourceLoc *loc, const CgIRModule *module);
int (*GenerateIR)(SourceLoc *loc, const CgIRModule *module);
```

Initialize both to null in `InitHAL_HAL`. Add default helpers only where a
profile intentionally accepts all verified IR; do not silently claim support
for an unported target.

- [ ] **Step 4: Make generic the complete Cg 2.0 neutral backend**

`ValidateIR_generic` returns success for every verified Cg IR module.
`GenerateIR_generic` writes `CgIRPrintModule(Cg->options.outfd, module)`.

In `CompileProgram`, use the IR callbacks when language version is 2.0 and the
selected profile provides them. For `-version 1.1 -profile generic`, retain
the historical `GenerateCode_generic` tree output.

- [ ] **Step 5: Add a compiler-level normalized golden test**

Teach `tests/check_cg20.cmake` to accept `EXPECTED`, generate to `ACTUAL` via
`-o`, and compare files with `cmake -E compare_files` after success. Compile
`tests/cg20/ir/reachable.cg` and check in
`tests/cg20/ir/normalized.expected` containing its complete normalized output.

- [ ] **Step 6: Run generic legacy and Cg 2.0 tests**

Run:

```powershell
cmake --build build-cg20 --config Release
ctest --test-dir build-cg20 -C Release -R "cg_ir_unit|cg20_.*normalized|generic_" --output-on-failure
ctest --test-dir build-cg20 -C Release --output-on-failure
```

Expected: Cg 2.0 generic fixtures use normalized Cg IR, explicit 1.1 generic
fixtures retain historical hashes, and the full suite passes.

- [ ] **Step 7: Commit the generic IR backend**

```powershell
git add cg_ir.h cg_ir_print.c hal.h hal.c compile.c generic_hal.c generic_hal.h tests/cg_ir_test.c tests/cg20/ir/normalized.expected tests/check_cg20.cmake tests/CMakeLists.txt CMakeLists.txt
git commit -m "Emit normalized Cg IR from generic"
```

## Task 18: Make Diagnostics Layered and Output Transactional

**Files:**

- Create: `output.h`, `output.c`
- Modify: `compile.h:52-125`
- Modify: `compile.c:60-100,2740-2840`
- Modify: `cgcmain.c:95-145`
- Modify: `scanner.c:250-385`
- Modify: `errors.h`
- Modify: `cg_overload.c`, `cg_reach.c`, `cg_ir_lower.c`
- Create: `tests/check_output_transaction.cmake`
- Create: `tests/cg20/diagnostics/profile_call_path.cg`
- Modify: `CMakeLists.txt`, `tests/CMakeLists.txt`, `tests/cg20/conformance.csv`

- [ ] **Step 1: Write the preservation test before changing output**

Create `tests/check_output_transaction.cmake`. It writes `sentinel` to the
requested output, invokes `cgc` on a known-invalid shader with `-o`, then
asserts the command fails, the destination still equals `sentinel`, and no
file matching the destination's `.cgc-tmp-*` pattern remains.

Register `cg20_output_preserved_on_failure` and run it. Expected: failure
because current `OpenOutputFile` truncates the destination before parsing.

- [ ] **Step 2: Define the output transaction API**

Create `output.h`:

```c
#if !defined(__OUTPUT_H)
#define __OUTPUT_H 1

typedef struct OutputTransaction_Rec {
    const char *destination;
    char *temporary;
    FILE *stream;
    int isStdout;
} OutputTransaction;

int BeginOutputTransaction(OutputTransaction *transaction,
                           const char *destination);
int CommitOutputTransaction(OutputTransaction *transaction);
void AbortOutputTransaction(OutputTransaction *transaction);

#endif
```

- [ ] **Step 3: Implement same-directory temporary output**

In `output.c`, use stdout directly when no destination is supplied. Otherwise,
create `<destination>.cgc-tmp-<process-id>-<counter>` in the same directory,
opening the first nonexistent candidate for binary write. On commit, flush,
check `ferror`, close, then replace the destination. Use `MoveFileExA` with
`MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH` on Windows and `rename`
on POSIX. On any failure, close and remove only the exact temporary path.

- [ ] **Step 4: Integrate begin/commit/abort around the entire compilation**

Store an `OutputTransaction` in compiler options. Begin it where
`OpenOutputFile` currently runs, but do not expose the destination path to
backends. Set `options.outfd` to the transaction stream. Commit only after
`CompileProgram` returns zero errors and closing/listing operations succeed;
abort on every earlier return.

- [ ] **Step 5: Add diagnostic layer helpers and poison suppression**

Reserve new error-code groups in `errors.h` for language version/keyword,
canonical type/conversion, overload/interface, IR internal, and generic IR
profile errors. Add a canonical poison type query. Make conversion, overload,
interface, IR lowering, and profile validation return early without emitting a
second diagnostic when an operand is poison.

- [ ] **Step 6: Report overload notes and reachability witnesses**

Replace direct `printf` candidate dumping in the old resolver with diagnostic
notes emitted through scanner diagnostic helpers. For profile failures in a
helper, walk `CgReachWitness` back to the entry and print one note per call
edge, preserving source locations.

Create `profile_call_path.cg` with `main -> first -> second -> unsupported` and
assert the primary profile diagnostic plus notes for `second`, `first`, and
`main` in that order.

- [ ] **Step 7: Run failure-path tests**

Run:

```powershell
cmake --build build-cg20 --config Release
ctest --test-dir build-cg20 -C Release -R "cg20_output|cg20_.*diagnostic|glsl.*diagnostic" --output-on-failure
ctest --test-dir build-cg20 -C Release --output-on-failure
```

Expected: destinations survive failures, no temporary files remain, diagnostic
locations/codes match, and the full suite passes.

- [ ] **Step 8: Commit transactional generation**

```powershell
git add output.h output.c compile.h compile.c cgcmain.c scanner.c errors.h cg_overload.c cg_reach.c cg_ir_lower.c tests/check_output_transaction.cmake tests/CMakeLists.txt tests/cg20 CMakeLists.txt
git commit -m "Make compiler output transactional"
```

## Task 19: Migrate Existing GLSL Validation and Lowering to Cg IR

**Files:**

- Modify: `glsl_hal.h:120-137`
- Modify: `glsl_hal.c:640-790`
- Modify: `glsl_lower.c`
- Modify: `glsl_ir.h`, `glsl_ir.c`, `glsl_codegen.c`
- Modify: `hal.h`, `compile.c`
- Modify: `tests/glsl_ir_test.c`, `tests/glsl_semantics_test.c`
- Modify: `tests/CMakeLists.txt`
- Create: `tests/cg20/diagnostics/glsl_interface_dispatch.cg`
- Create: `tests/cg20/diagnostics/glsl_unsized_array.cg`

- [ ] **Step 1: Capture all current GLSL outputs before changing lowering**

Run:

```powershell
ctest --test-dir build-cg20 -C Release -R "^(glslv_|glslf_|glsl_|validate_glsl)" --output-on-failure
```

Expected: every existing GLSL golden, diagnostic, limit, interface, and
validator test passes. Do not update goldens during this task unless a prior
approved spec explicitly calls for different output.

- [ ] **Step 2: Change the lowering boundary**

Replace the public frontend-tree signature:

```c
int GlslLowerProgram(GlslModule *module, const GlslProfileDesc *profile,
                     SourceLoc *loc, Scope *scope, Symbol *program);
```

with:

```c
int GlslLowerCgIR(GlslModule *module, const GlslProfileDesc *profile,
                  const CgIRModule *source);
```

Change `GlslLowerContext` to store `const CgIRModule *source` and IR declaration
maps rather than `Scope *`/frontend statement traversal state.

- [ ] **Step 3: Port type, declaration, and interface collection**

Translate Cg IR scalar/vector/matrix/array/struct/sampler types to existing
`GlslType`. Preserve source symbol identity for name allocation. Collect
uniforms, defaults, entry inputs/outputs, bindings, helper signatures, and
locals from ordered Cg IR declarations instead of rescanning frontend trees.

Reject half/fixed/double, unsigned widths, interfaces, dynamic unsized arrays,
and samplerRECT only in GLSL profile validation, with the existing 6200-6209
diagnostic families.

- [ ] **Step 4: Port expression lowering by `CgIRExprKind`**

Replace opcode/subop decoding with a switch over Cg IR expression kind and
canonical types. Cover constants, names, members, indexing, length, swizzles,
constructors, casts, operators, assignments, conditional expressions, direct
calls, and typed intrinsics. Preserve existing single-evaluation helper logic
for side-effecting matrices and aggregates.

`CGIR_EXPR_INTERFACE_CALL` and runtime unsized-array operations produce
specific GLSL unsupported-operation reasons; they never reach code generation.

- [ ] **Step 5: Port structured statement lowering**

Translate Cg IR blocks, declarations, expressions, conditionals, loops,
returns, loop jumps, and discard into the existing `GlslStmt` forms. Preserve
the current early-return, out-parameter, and discard normalization algorithms,
but take their input from structured Cg IR.

- [ ] **Step 6: Add IR-oriented GLSL HAL callbacks**

Implement `ValidateIR_glsl` by running lowering into a scratch `GlslModule`
without writing. Cache no cross-call pointer. Implement `GenerateIR_glsl` by
lowering again into the compilation pool and calling `GlslWriteModule`.
Register both callbacks in `GlslInitHAL`; retain `GenerateCode_glsl` only for
explicit language 1.1 until the final parity tests pass.

- [ ] **Step 7: Add new valid-language/invalid-profile fixtures**

`glsl_interface_dispatch.cg` is the interface dispatch fixture from Task 11
compiled under `glslv`; assert a profile unsupported-operation diagnostic.
`glsl_unsized_array.cg` assigns and queries an unsized array under `glslv`;
assert the same diagnostic layer. Compile both successfully with generic to
prove frontend validity.

- [ ] **Step 8: Run unit, golden, validator, and full suites**

Run:

```powershell
cmake --build build-cg20 --config Release
ctest --test-dir build-cg20 -C Release -R "cg_ir_unit|glsl_ir_unit|glsl_semantics_unit|cg20_.*glsl|^(glslv_|glslf_|glsl_|validate_glsl)" --output-on-failure
ctest --test-dir build-cg20 -C Release --output-on-failure
```

Expected: every prior supported GLSL output is byte-identical, new unsupported
features fail at profile validation, and the full suite passes.

- [ ] **Step 9: Audit and remove frontend type-subop dependence from GLSL**

Run:

```powershell
rg -n "SUBOP_GET_T|SUBOP_GET_T1|SUBOP_GET_T2|TYPE_BASE_MASK|GetBase\(" glsl_lower.c glsl_hal.c glsl_ir.c
```

Expected: no GLSL lowering decision derives a Cg type from a four-bit subop;
any remaining `GetBase` use is isolated to explicit legacy 1.1 compatibility.

- [ ] **Step 10: Commit the Cg IR GLSL adapter**

```powershell
git add glsl_hal.h glsl_hal.c glsl_lower.c glsl_ir.h glsl_ir.c glsl_codegen.c hal.h compile.c tests/glsl_ir_test.c tests/glsl_semantics_test.c tests/CMakeLists.txt tests/cg20
git commit -m "Lower Cg IR to GLSL 1.10"
```

## Task 20: Close the Conformance Manifest and Qualify the Release

**Files:**

- Modify: `tests/cg20/conformance.csv`
- Modify: `tests/check_cg20_manifest.cmake`
- Modify: `tests/CMakeLists.txt`
- Add fixtures under: `tests/cg20/`
- Modify: `README.md`
- Modify: `hslversion.h`
- Modify: `docs/superpowers/specs/2026-08-23-cg-2-language-ir-design.md` only if
  implementation discovered an approved clarification that must be recorded

- [ ] **Step 1: Expand the manifest against every normative section**

Read the Cg 2.0 specification section by section. Add separate rows for:

- Every declaration specifier and permitted placement.
- Every scalar, vector, matrix, sampler, struct, interface, and array rule.
- Every literal suffix and constant category.
- Every implicit/explicit conversion table cell and warning case.
- Every arithmetic-promotion step.
- Every array operation and minimum requirement.
- Every overload-resolution step and profile precedence rule.
- Every binding-semantic and aliasing rule applicable to standalone shaders.
- Every statement/operator rule, swizzle form, and assignment behavior.
- ANSI C preprocessing requirements for object/function macros, conditional
  directives, token expansion, and `#include` using the repository's documented
  include search behavior.
- Every standard-library base name and special parameter/result rule.
- Vertex/fragment mandatory outputs defined by the core language document.
- Every CgFX-only reserved construct classified `out-of-scope`.

Each non-out-of-scope row names one registered CTest. Do not combine distinct
normative rules merely because one fixture happens to exercise both.

- [ ] **Step 2: Make manifest/test-name consistency executable**

In both `add_cg20_success` and `add_cg20_failure`, append the test name to a
global `CG20_TEST_NAMES` property. Pass that list to
`check_cg20_manifest.cmake`. Extend the script:

```cmake
foreach(row IN LISTS rows)
    string(REPLACE "," ";" fields "${row}")
    list(GET fields 1 classification)
    list(GET fields 2 test_name)
    if(NOT classification STREQUAL "out-of-scope")
        if(NOT test_name IN_LIST TEST_NAMES)
            message(FATAL_ERROR "manifest test is not registered: ${test_name}")
        endif()
    endif()
endforeach()
```

Also reject duplicate requirement IDs and duplicate intrinsic opcodes.

- [ ] **Step 3: Add focused fixtures for uncovered rows**

For each uncovered row, create the smallest positive or negative `.cg`
fixture in its matching `tests/cg20` directory and register it through the
harness. Each negative fixture asserts diagnostic code, source line, and a
stable message fragment. Each backend-reject row also has a generic success
test for the same source.

- [ ] **Step 4: Update user documentation**

Update `README.md` with:

- Cg 2.0 as the default language.
- `-version 1.1` and `-version 2.0` examples.
- Standalone-language and CgFX scope.
- Generic normalized Cg IR behavior.
- Current `glslv`/`glslf` subset and profile diagnostics.
- How to run the conformance, regeneration, and complete suites.

Update `hslversion.h` to `HSL_VERSION 2`, `HSL_SUB_VERSION 0`, and
`HSL_SUB_SUB_VERSION 0`; verify `cgc -v` prints the resulting 2.0 compiler
version.

- [ ] **Step 5: Run regeneration checks from a clean diff**

Run:

```powershell
cmake --build build-cg20 --config Release --target regenerate_parser regenerate_stdlib
git diff --check
git status --short
ctest --test-dir build-cg20 -C Release -R "cg20_manifest|stdlib_regeneration|parser" --output-on-failure
```

Expected: regeneration produces no uncommitted difference beyond intentional
files already staged for this task; manifest and regeneration tests pass.

- [ ] **Step 6: Run release and assertion-enabled qualification**

Run:

```powershell
cmake -S . -B build-cg20-release -DBUILD_TESTING=ON -DCMAKE_BUILD_TYPE=Release
cmake --build build-cg20-release --config Release
ctest --test-dir build-cg20-release -C Release --output-on-failure

cmake -S . -B build-cg20-debug -DBUILD_TESTING=ON -DCMAKE_BUILD_TYPE=Debug
cmake --build build-cg20-debug --config Debug
ctest --test-dir build-cg20-debug -C Debug --output-on-failure
```

Expected: both complete suites pass with zero failed tests. If
`glslangValidator` is installed, every generated GLSL validation test passes;
if absent, CMake reports the documented disabled-validator status while all
remaining tests pass.

- [ ] **Step 7: Manually validate all four bundled shaders in both modes**

Run:

```powershell
$shaders = 'position.cg','reflection.cg','vertexlight.cg','vertexlight4.cg'
foreach ($shader in $shaders) {
    & .\build-cg20-release\Release\cgc.exe -quiet -version 2.0 -profile generic $shader
    if ($LASTEXITCODE -ne 0) { throw "Cg 2.0 failed: $shader" }
    & .\build-cg20-release\Release\cgc.exe -quiet -version 1.1 -profile generic $shader
    if ($LASTEXITCODE -ne 0) { throw "Cg 1.1 failed: $shader" }
}
```

Expected: all eight invocations exit zero; Cg 2.0 prints normalized IR and Cg
1.1 prints the historical tree.

- [ ] **Step 8: Commit release qualification**

```powershell
git add tests/cg20 tests/check_cg20_manifest.cmake tests/CMakeLists.txt README.md hslversion.h parser.c parser.h stdlib.c
git commit -m "Complete Cg 2.0 language conformance"
```

## Final Verification Checklist

- [ ] `git diff --check` prints no errors.
- [ ] `rg -n "TBD|TODO|FIXME" tests/cg20 cg_*.c cg_*.h language.c language.h`
  finds no incomplete implementation markers.
- [ ] Every row in `tests/cg20/conformance.csv` passes manifest validation.
- [ ] Every `CgIntrinsic` has at least one expanded signature and a unit test.
- [ ] Every valid Cg 2.0 fixture succeeds under `generic`.
- [ ] Valid features outside GLSL 1.10 fail with profile diagnostics under
  `glslv`/`glslf`.
- [ ] Existing GLSL goldens are byte-identical.
- [ ] Explicit `-version 1.1` compatibility tests pass.
- [ ] Parser and standard-library regeneration leave no diff.
- [ ] Release and Debug CTest suites report zero failures.
- [ ] `git status --short` contains no compiled binaries or build-directory
  artifacts.

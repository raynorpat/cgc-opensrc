# CMake Build Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Replace the legacy Visual Studio project files with a portable CMake build for `cgc` and `tokenize`, including opt-in source regeneration.

**Architecture:** A root `CMakeLists.txt` owns two explicitly defined C executable targets and uses the checked-in generated C files for normal builds. A small CMake script safely captures `tokenize` output before updating `stdlib.c`; parser regeneration is an explicit target that uses Bison only when requested.

**Tech Stack:** C90, CMake 3.16+, command-line smoke verification, optional GNU Bison

---

## File Map

- Create `CMakeLists.txt`: declare the C project, executable targets, debug definition, and regeneration targets.
- Create `cmake/RegenerateStdlib.cmake`: run `tokenize`, check its exit status, and update `stdlib.c` only after successful generation.
- Modify `README.txt`: replace legacy Visual Studio instructions with CMake build and regeneration commands.
- Delete `cgc.sln`, `cgc.vcproj`, `tokenize.vcproj`, `cgc.dsw`, `cgc.dsp`, and `tokenize.dsp`: remove obsolete IDE metadata.
- Keep `Makefile`, `stdlib.c`, `parser.c`, and `parser.h`: preserve the existing portable build and dependency-free default inputs.

### Task 1: Add the CMake targets

**Files:**
- Create: `CMakeLists.txt`
- Create: `cmake/RegenerateStdlib.cmake`

- [ ] **Step 1: Verify the CMake entry point is absent**

Run:

```powershell
cmake -S . -B build
```

Expected: configuration fails because `D:/raynorpat/cgc` does not contain `CMakeLists.txt`.

- [ ] **Step 2: Add the root build definition**

Create `CMakeLists.txt` with:

```cmake
cmake_minimum_required(VERSION 3.16)

project(cgc LANGUAGES C)

add_executable(cgc
    atom.c
    binding.c
    cgcmain.c
    cgstruct.c
    check.c
    compile.c
    constfold.c
    cpp.c
    generic_hal.c
    hal.c
    inline.c
    memory.c
    parser.c
    printutils.c
    scanner.c
    semantic.c
    stdlib.c
    support.c
    support_iter.c
    symbols.c
    tokens.c
)

add_executable(tokenize
    atom.c
    cgstruct.c
    scanner.c
    tokenize.c
    tokens.c
)

set_target_properties(cgc tokenize PROPERTIES
    C_STANDARD 90
    C_STANDARD_REQUIRED YES
    C_EXTENSIONS YES
)

target_compile_definitions(cgc PRIVATE
    $<$<CONFIG:Debug>:CGC_DEBUG_THE_COMPILER>
)

add_custom_target(regenerate_stdlib
    COMMAND
        ${CMAKE_COMMAND}
        -DTOKENIZE=$<TARGET_FILE:tokenize>
        -DINPUT=${CMAKE_CURRENT_SOURCE_DIR}/stdlib.cg
        -DOUTPUT=${CMAKE_CURRENT_SOURCE_DIR}/stdlib.c
        -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/RegenerateStdlib.cmake
    DEPENDS tokenize stdlib.cg cmake/RegenerateStdlib.cmake
    COMMENT "Regenerating stdlib.c"
    VERBATIM
)

find_package(BISON QUIET)

if(BISON_FOUND)
    add_custom_target(regenerate_parser
        COMMAND
            ${BISON_EXECUTABLE}
            ${CMAKE_CURRENT_SOURCE_DIR}/parser.y
            --defines=${CMAKE_CURRENT_SOURCE_DIR}/parser.h
            --output=${CMAKE_CURRENT_SOURCE_DIR}/parser.c
        DEPENDS parser.y
        COMMENT "Regenerating parser.c and parser.h"
        VERBATIM
    )
else()
    add_custom_target(regenerate_parser
        COMMAND ${CMAKE_COMMAND} -E echo "Bison is required to regenerate parser.c and parser.h"
        COMMAND ${CMAKE_COMMAND} -E false
        VERBATIM
    )
endif()
```

- [ ] **Step 3: Add failure-safe standard-library generation**

Create `cmake/RegenerateStdlib.cmake` with:

```cmake
foreach(required_variable TOKENIZE INPUT OUTPUT)
    if(NOT DEFINED ${required_variable})
        message(FATAL_ERROR "${required_variable} is required")
    endif()
endforeach()

execute_process(
    COMMAND "${TOKENIZE}" "${INPUT}"
    RESULT_VARIABLE tokenize_result
    OUTPUT_VARIABLE generated_source
    ERROR_VARIABLE tokenize_error
)

if(NOT tokenize_result EQUAL 0)
    message(FATAL_ERROR "tokenize failed: ${tokenize_error}")
endif()

file(WRITE "${OUTPUT}" "${generated_source}")
```

- [ ] **Step 4: Configure and build both executables**

Run:

```powershell
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --config Debug
```

Expected: configuration succeeds and the build reports both `cgc` and `tokenize` as built targets. If the selected generator ignores `CMAKE_BUILD_TYPE`, the explicit `--config Debug` still selects the debug configuration.

- [ ] **Step 5: Confirm the debug definition is emitted**

Run:

```powershell
cmake --build build --config Debug --verbose 2>&1 | Select-String 'CGC_DEBUG_THE_COMPILER'
```

Expected: at least one `cgc` compiler command contains `CGC_DEBUG_THE_COMPILER`; `tokenize` compiler commands do not require it.

- [ ] **Step 6: Commit the CMake build**

```powershell
git add -- CMakeLists.txt cmake/RegenerateStdlib.cmake
git commit -m "Add CMake build for cgc and tokenize"
```

### Task 2: Document CMake and remove legacy MSVC files

**Files:**
- Modify: `README.txt:1-16`
- Delete: `cgc.sln`
- Delete: `cgc.vcproj`
- Delete: `tokenize.vcproj`
- Delete: `cgc.dsw`
- Delete: `cgc.dsp`
- Delete: `tokenize.dsp`

- [ ] **Step 1: Record the legacy documentation and files before removal**

Run:

```powershell
Select-String -Path README.txt -Pattern 'Visual C\+\+ 6.0'
Get-Item cgc.sln,cgc.vcproj,tokenize.vcproj,cgc.dsw,cgc.dsp,tokenize.dsp
```

Expected: the README still advertises Visual C++ 6.0 and all six legacy project files exist.

- [ ] **Step 2: Replace the README build guidance**

Replace the release-information build paragraphs before `RELEASE INFORMATION`'s parser explanation with this text, preserving the remainder of the file:

```text
This release builds the Cg compiler (cgc) with the "generic" profile,
which does some minimal semantic checks and prints out a tree representation
of the code. Build both cgc and the tokenize helper with CMake:

  cmake -S . -B build
  cmake --build build --config Release

The included Makefile remains available for systems using make.

The release contains a pre-built parser (parser.c and parser.h) built
from parser.y with GNU bison. Normal builds use the checked-in generated
sources and do not require bison. To regenerate the generated sources, run:

  cmake --build build --target regenerate_stdlib
  cmake --build build --target regenerate_parser

The parser target requires GNU bison.
```

- [ ] **Step 3: Delete only the six approved legacy files**

Use `apply_patch` delete operations for:

```text
cgc.sln
cgc.vcproj
tokenize.vcproj
cgc.dsw
cgc.dsp
tokenize.dsp
```

Do not delete `Makefile` or any generated C source.

- [ ] **Step 4: Verify documentation and removal**

Run:

```powershell
rg -n 'Visual Studio|Visual C\+\+|\.sln|\.vcproj|\.dsp|\.dsw' README.txt
@(Get-Item cgc.sln,cgc.vcproj,tokenize.vcproj,cgc.dsw,cgc.dsp,tokenize.dsp -ErrorAction SilentlyContinue).Count
```

Expected: `rg` returns no matches and the file count is `0`.

- [ ] **Step 5: Commit documentation and cleanup**

```powershell
git add -- README.txt cgc.sln cgc.vcproj tokenize.vcproj cgc.dsw cgc.dsp tokenize.dsp
git commit -m "Remove legacy MSVC project files"
```

### Task 3: Verify builds, smoke behavior, and regeneration

**Files:**
- Verify: `CMakeLists.txt`
- Verify: `cmake/RegenerateStdlib.cmake`
- Verify: `stdlib.c`
- Verify: `parser.c`
- Verify: `parser.h`

- [ ] **Step 1: Start from a clean out-of-source build directory**

Remove only the verified local build directory at `D:/raynorpat/cgc/build`, then run:

```powershell
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

Expected: both targets build successfully from scratch.

- [ ] **Step 2: Locate and smoke-test the compiler**

Run:

```powershell
$cgc = Get-ChildItem build -Recurse -Filter cgc.exe | Select-Object -First 1 -ExpandProperty FullName
if (-not $cgc) { $cgc = (Resolve-Path build/cgc).Path }
& $cgc -profile generic position.cg
if ($LASTEXITCODE -ne 0) { throw "cgc smoke test failed with exit code $LASTEXITCODE" }
```

Expected: `cgc` exits with code `0` and prints the generic-profile representation of `position.cg`.

- [ ] **Step 3: Verify standard-library regeneration is stable**

Run:

```powershell
$before = git hash-object stdlib.c
cmake --build build --target regenerate_stdlib --config Release
$after = git hash-object stdlib.c
if ($before -ne $after) { throw "stdlib.c changed during regeneration" }
```

Expected: the target succeeds and the two object hashes match. A mismatch is a failed verification and must be diagnosed before continuing.

- [ ] **Step 4: Verify parser regeneration behavior**

Run:

```powershell
cmake --build build --target regenerate_parser --config Release
```

Expected when Bison is installed: regeneration succeeds. Expected when Bison is absent: the target fails with `Bison is required to regenerate parser.c and parser.h`; ordinary configuration and builds remain successful.

- [ ] **Step 5: Run repository integrity checks**

Run:

```powershell
git diff --check
git status --short
git ls-files | rg '\.(sln|vcproj|dsp|dsw)$'
```

Expected: `git diff --check` is silent, status contains no unexpected source changes, and the legacy-file search has no matches.

- [ ] **Step 6: Commit any verification-driven correction**

If verification required a correction, stage only the relevant files and commit it:

```powershell
git add -- CMakeLists.txt cmake/RegenerateStdlib.cmake README.txt
git commit -m "Fix CMake build verification issues"
```

If no correction was required, do not create an empty commit.

# Vertexlight CTest Smoke Tests Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add two CTest smoke tests that require `cgc` to compile `vertexlight.cg` and `vertexlight4.cg` successfully with the generic profile.

**Architecture:** The root build enables standard CTest integration and conditionally adds a dedicated `tests` subdirectory. `tests/CMakeLists.txt` owns two explicit test registrations that invoke the built `cgc` target using absolute shader paths and rely on CTest's default exit-status handling.

**Tech Stack:** CMake 3.16+, CTest, the existing C `cgc` executable

---

## File Structure

- Modify `CMakeLists.txt` to enable CTest and include test configuration only when `BUILD_TESTING` is enabled.
- Create `tests/CMakeLists.txt` to register the two vertexlight smoke tests.

### Task 1: Demonstrate the Missing Test Registrations

**Files:**
- Inspect: `CMakeLists.txt`

- [ ] **Step 1: Configure a clean test build before adding the registrations**

Run:

```powershell
cmake -S . -B build-ctest -DBUILD_TESTING=ON
```

Expected: configuration succeeds. The current project does not consume
`BUILD_TESTING` because CTest integration is not present yet.

- [ ] **Step 2: Verify the requested tests are absent**

Run:

```powershell
ctest --test-dir build-ctest -N -C Debug -R '^cgc_vertexlight(4)?$'
```

Expected: CTest reports `Total Tests: 0`. This is the red state: neither
requested smoke test is registered.

### Task 2: Register the Vertexlight Smoke Tests

**Files:**
- Modify: `CMakeLists.txt`
- Create: `tests/CMakeLists.txt`

- [ ] **Step 1: Enable CTest and add the tests directory from the root build**

Add this block immediately after `project(cgc LANGUAGES C)` in
`CMakeLists.txt`:

```cmake
include(CTest)

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
```

- [ ] **Step 2: Add the two explicit test registrations**

Create `tests/CMakeLists.txt` with:

```cmake
add_test(
    NAME cgc_vertexlight
    COMMAND $<TARGET_FILE:cgc>
        -profile generic
        ${PROJECT_SOURCE_DIR}/vertexlight.cg
)

add_test(
    NAME cgc_vertexlight4
    COMMAND $<TARGET_FILE:cgc>
        -profile generic
        ${PROJECT_SOURCE_DIR}/vertexlight4.cg
)
```

- [ ] **Step 3: Reconfigure and build the compiler**

Run:

```powershell
cmake -S . -B build-ctest -DBUILD_TESTING=ON
cmake --build build-ctest --config Debug --target cgc
```

Expected: both commands exit with status zero and the `cgc` executable is
built for the selected configuration.

- [ ] **Step 4: Verify the tests are green**

Run:

```powershell
ctest --test-dir build-ctest -C Debug --output-on-failure -R '^cgc_vertexlight(4)?$'
```

Expected: CTest runs two tests, `cgc_vertexlight` and `cgc_vertexlight4`, and
reports `100% tests passed, 0 tests failed out of 2`.

- [ ] **Step 5: Commit the implementation**

Run:

```powershell
git add -- CMakeLists.txt tests/CMakeLists.txt
git commit -m "Add vertexlight CTest smoke tests"
```

Expected: Git creates one focused commit containing only the CTest wiring and
test registrations.

### Task 3: Verify Test Discovery Controls and Final State

**Files:**
- Verify: `CMakeLists.txt`
- Verify: `tests/CMakeLists.txt`

- [ ] **Step 1: Confirm exactly the requested tests are discovered**

Run:

```powershell
ctest --test-dir build-ctest -N -C Debug -R '^cgc_vertexlight(4)?$'
```

Expected: CTest lists `cgc_vertexlight` and `cgc_vertexlight4` and reports
`Total Tests: 2`.

- [ ] **Step 2: Confirm test registration can be disabled**

Run:

```powershell
cmake -S . -B build-ctest-off -DBUILD_TESTING=OFF
ctest --test-dir build-ctest-off -N -C Debug -R '^cgc_vertexlight(4)?$'
```

Expected: configuration succeeds and CTest reports `Total Tests: 0`.

- [ ] **Step 3: Check the final patch for formatting errors and unintended files**

Run:

```powershell
git diff --check HEAD^
git status --short
```

Expected: `git diff --check` exits with status zero. `git status --short` shows
only pre-existing unrelated untracked files, if any.

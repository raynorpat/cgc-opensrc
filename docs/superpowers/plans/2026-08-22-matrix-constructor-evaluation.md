# Matrix Constructor Evaluation Implementation Plan

> **For Codex:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task.

**Goal:** Translate side-effecting scalar and vector arguments of square matrix constructors to strict GLSL 1.10 while evaluating each original argument exactly once at its expression site.

**Architecture:** Keep the existing direct `matN(...)` lowering for pure constructors. For an impure constructor, call a deterministic, collision-safe synthesized GLSL helper whose parameters preserve the original mapped argument list and whose body performs the row-major Cg to column-major GLSL rearrangement using only pure parameter references. Deduplicate helpers by return type and complete ordered parameter types, collect them during lowering, and prepend their definitions after source functions have been lowered.

**Tech Stack:** C89-style C, CMake/CTest, existing cgc GLSL IR/code generator, glslangValidator.

---

### Task 1: Capture the side-effect regression

**Files:**
- Create: `tests/glsl/expressions/vp_matrix_side_effects.cg`
- Create: `tests/glsl/expressions/vp_matrix_side_effects.expected`
- Modify: `tests/CMakeLists.txt`

- [ ] Add a vertex source fixture containing two `float2x2` helper returns with the same `float2,float2` constructor signature, a user function colliding with the generated helper base name, and a mixed scalar/`float2`/`float3` `float3x3` helper return.
- [ ] Invoke one constructor-producing helper only inside a conditional branch and use distinct `inout` state values so the exact output exposes duplication and row/column placement.
- [ ] Register exact `glslv_matrix_side_effects` and frozen generic-profile tests. Initially use the reviewed expected GLSL shape and a placeholder generic hash.
- [ ] Run `ctest -R "glslv_matrix_side_effects|generic_matrix_side_effects" --output-on-failure` in Debug and record the expected RED diagnostic from the existing side-effect rejection.

### Task 2: Synthesize expression-site matrix helpers

**Files:**
- Modify: `glsl_lower.c`

- [ ] Add a lowering-private helper descriptor keyed by the requested matrix result type and the full ordered mapped parameter types, with an explicit maximum of sixteen constructor arguments.
- [ ] Detect side effects across scalar and vector source arguments before direct component rearrangement. Preserve the current direct lowering byte-for-byte when every argument is pure.
- [ ] Validate impure helper parameters as non-array numeric scalar/vector shapes, preserving mapped float/int base and width while rejecting boolean and other invalid shapes before helper creation.
- [ ] Allocate deterministic names such as `cg_construct_mat2_v2_v2` through `GlslAllocateDistinctName`, so user declarations share the same collision domain.
- [ ] Build one `GlslFunction` definition per signature. Its `argN` parameters are referenced purely, flattened in Cg row order, and rearranged to `matN` column order in a single return statement.
- [ ] Lower the original constructor arguments once, in source order, as the synthesized call argument list. Remove the old side-effect diagnostic.
- [ ] Prepend synthesized definitions in first-use order after all source functions and the entry function have been lowered, avoiding prototypes and leaving them outside source-helper traversal.

### Task 3: Freeze focused outputs

**Files:**
- Modify: `tests/glsl/expressions/vp_matrix_side_effects.expected`
- Modify: `tests/CMakeLists.txt`

- [ ] Generate the candidate GLSL, inspect every line, and freeze the exact expected output only after confirming each `next_*` call appears once per source constructor and the helper body uses correct column-major placement.
- [ ] Confirm the generated helper is deduplicated, the user-collision suffix is deterministic, and the conditional call remains inside its branch.
- [ ] Validate the generated output with `glslangValidator -S vert` through the exact fixture test.
- [ ] Run the generic profile fixture, replace its placeholder hash with the observed reviewed hash, and rerun both focused tests to GREEN.
- [ ] Rerun `glslv_matrix_constructors` to prove pure constructor output is unchanged.

### Task 4: Full verification and focused commit

**Files:**
- Verify: all modified files and generated outputs

- [ ] Build and run all Debug CTest tests, then all Release CTest tests.
- [ ] Run all generic hash tests and all GLSL matrix/bundled shader fixtures explicitly.
- [ ] Regenerate `stdlib.c`, verify its hash and worktree diff are unchanged, and restore nothing destructively.
- [ ] Run direct occurrence-count checks on the exact side-effect output, `git diff --check`, and review the complete diff from `b5193c5` plus the approved design/plan commits.
- [ ] Commit the implementation as `Preserve matrix constructor evaluation` and report hashes, test counts, RED/GREEN evidence, and regeneration status.

### Task 5: Extend matrix arguments across numeric bases

**Files:**
- Modify: `glsl_lower.c`
- Create: `tests/glsl/expressions/vp_matrix_numeric_args.cg`
- Create: `tests/glsl/expressions/vp_matrix_numeric_args.expected`
- Create: `tests/glsl/diagnostics/vp_matrix_bool_constructor.cg`
- Modify: `tests/CMakeLists.txt`

- [ ] Add exact and generic regressions for pure integer `float2x2`, `float3x3`, and `float4x4` constructors; mixed integer/floating arguments; and side-effecting integer scalar and vector-row arguments.
- [ ] Add an all-boolean constructor source that the generic profile accepts but `glslv` rejects explicitly as a nonnumeric matrix argument.
- [ ] Capture RED from the current float-only pure and helper parameter checks before changing production lowering.
- [ ] Generalize matrix scalar/vector validation, component extraction, helper signatures, and helper-name shape tokens across mapped float and int bases while preserving full ordered base and width.
- [ ] Keep pure constructors direct and readable, preserve exactly-once helper evaluation, and validate every exact success with strict `glslangValidator -S vert`.
- [ ] Run focused and full Debug/Release tests, all generic hashes, standard-library reproducibility, occurrence counts, and whitespace/worktree checks before the focused commit.

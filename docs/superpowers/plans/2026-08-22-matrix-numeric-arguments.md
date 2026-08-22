# Matrix Numeric Arguments Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Translate valid float-matrix constructors with mapped float or integer scalar/vector arguments while retaining exactly-once evaluation and explicit boolean rejection.

**Architecture:** Keep constructor argument bases intact in the GLSL IR. Extend the existing direct and synthesized-helper paths to accept float/int numeric shapes, derive component types from each argument base, and use strict GLSL 1.10 matrix-constructor numeric conversion. Preserve the existing readable float helper tokens and add distinct `i`/`ivN` integer tokens.

**Tech Stack:** C89-style C, CMake/CTest, cgc GLSL IR/code generator, glslangValidator.

---

### Task 1: Capture numeric and boolean regressions

**Files:**
- Create: `tests/glsl/expressions/vp_matrix_numeric_args.cg`
- Create: `tests/glsl/expressions/vp_matrix_numeric_args.expected`
- Create: `tests/glsl/diagnostics/vp_matrix_bool_constructor.cg`
- Modify: `tests/CMakeLists.txt`

- [ ] Add one exact vertex fixture containing pure all-integer `float2x2`, `float3x3`, and `float4x4` constructors, a mixed integer/floating `float2x2`, `next_int(inout int)`, `next_irow(inout int)`, and helper-return constructors using four integer scalars and two `int2` rows.
- [ ] Register `glslv_matrix_numeric_args` with a placeholder exact file and `generic_matrix_numeric_args` with an all-zero placeholder hash.
- [ ] Add an all-boolean `float2x2` source, register its frozen generic hash to prove common-front-end acceptance, and register `glslv_matrix_bool_constructor` expecting C5508 reason `matrix constructor argument type`.
- [ ] Build and run only these tests. Record RED from the float-only argument checks for the positive fixture and from the missing explicit boolean reason for the negative fixture.

### Task 2: Generalize profile-local numeric lowering

**Files:**
- Modify: `glsl_lower.c`

- [ ] Replace the float-only predicate with a numeric scalar/vector predicate accepting exactly `GLSL_BASE_FLOAT` and `GLSL_BASE_INT`, widths one through four, and no matrix/array/struct fields.
- [ ] Extend deterministic helper spelling as follows while retaining existing float goldens:

```c
if (parameters[i].base == GLSL_BASE_FLOAT)
    sprintf(end, parameters[i].len == 1 ? "_f" : "_v%d",
            parameters[i].len);
else
    sprintf(end, parameters[i].len == 1 ? "_i" : "_iv%d",
            parameters[i].len);
```

- [ ] Construct parameter swizzle types with `parameter->type.base`, so an `ivec2` row yields integer scalar components while float helpers remain unchanged.
- [ ] Apply the numeric predicate in both helper and pure paths. In the pure path, derive the swizzle scalar type from each argument base. Record `matrix constructor argument type` before rejecting boolean or other invalid shapes.
- [ ] Do not change `support.c`: common constructor behavior and generic hashes remain authoritative, and helper signatures must retain mapped integer bases.

### Task 3: Freeze output and verify evaluation

**Files:**
- Modify: `tests/glsl/expressions/vp_matrix_numeric_args.expected`
- Modify: `tests/CMakeLists.txt`

- [ ] Generate and inspect the exact GLSL. Require readable direct integer constructors in column-major order, `int`/`ivec2` helper parameters, helper names containing `_i_i_i_i` and `_iv2_iv2`, and unchanged mixed float output.
- [ ] Confirm each `next_int(stateN)` and `next_irow(stateN)` call occurs exactly once per source constructor, then freeze the expected output and numeric generic hash.
- [ ] Run the positive exact test through `glslangValidator -S vert`, the boolean generic/negative pair, and all existing matrix fixtures.

### Task 4: Full verification and commit

**Files:**
- Verify: `glsl_lower.c`, tests, matrix design/plan documents, and generated artifacts

- [ ] Run all 62 expected Debug tests and all 62 expected Release tests, all generic hashes, and the four bundled vertex fixtures.
- [ ] Regenerate `stdlib.c` twice and require the stable checked-in SHA-256 hash.
- [ ] Run direct occurrence checks, `git diff --check`, inspect the full diff, and require a clean worktree after commit.
- [ ] Commit the focused implementation as `Support integer matrix constructor arguments` and report RED/GREEN evidence, test counts, hash, and any deviation.

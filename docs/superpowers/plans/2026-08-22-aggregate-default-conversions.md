# Aggregate Uniform Default Conversions Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Emit conversion-correct, finite, range-safe metadata for aggregate GLSL uniform defaults.

**Architecture:** Replace float-only AST flattening with a GLSL-local typed component evaluator. Apply encoded Cg casts to typed components, then normalize them against recursively flattened destination GLSL leaf bases before writing the existing float binding metadata.

**Tech Stack:** C90, CMake/CTest, Cg AST opcodes and types, strict GLSL 1.10, `glslangValidator`.

---

### Task 1: Freeze conversion regressions

**Files:**
- Create: `tests/glsl/uniforms/vp_aggregate_default_conversions.cg`
- Create: `tests/glsl/uniforms/vp_aggregate_default_conversions.expected`
- Create: `tests/glsl/diagnostics/vp_aggregate_default_int_range.cg`
- Modify: `tests/CMakeLists.txt`

- [ ] Add one exact source whose scalar and aggregate defaults cover positive
  and negative float-to-int truncation, zero/nonzero numeric-to-bool
  normalization, int-to-float conversion, and supported explicit vector or
  matrix casts.  Require metadata such as `1.0 -1.0` for `1.9 -1.9` converted
  to integer and `0.0 1.0 1.0` for zero, positive, and negative values
  converted to boolean.

  ```c
  uniform int scalarInt = -1.9,
  uniform int intValues[2] = { { 1.9 }, { -1.9 } },
  uniform bool boolValues[3] = { { 0 }, { 2 }, { -3 } },
  uniform float floatValues[2] = { { 4 }, { -5 } }
  ```
- [ ] Add an out-of-range floating-to-integer aggregate default that must fail
  once with a source-located C5508 and leave only `#version 110` in the output.
- [ ] Register the exact success and failure tests in `tests/CMakeLists.txt`.
- [ ] Build Debug and run the new tests.  Record RED metadata containing the
  unconverted fractions and non-normalized boolean values.

### Task 2: Evaluate typed default components

**Files:**
- Modify: `glsl_lower.c` in the aggregate-default helpers near
  `GlslFlattenDefaultExpr` and `GlslCollectDefaults`.

- [ ] Add a local typed component record containing a Cg base and
  `scalar_constant` value, plus checked append and conversion helpers.

  ```c
  typedef struct GlslDefaultValue_Rec {
      int base;
      scalar_constant value;
  } GlslDefaultValue;

  static int GlslConvertDefaultValue(GlslLowerContext *context,
      GlslDefaultValue *value, int targetBase);
  ```
- [ ] Make constant/list/vector flattening preserve the source base and order.
- [ ] Use the following typed flattener boundary so cast evaluation can update
  only the range appended by its operand:

  ```c
  static int GlslFlattenDefaultExpr(GlslLowerContext *context,
      const expr *source, GlslDefaultValue *values, int capacity, int *count);
  ```
- [ ] For `CAST_CS_OP`, `CAST_CV_OP`, and `CAST_CM_OP`, evaluate the operand,
  validate `1`, `S1`, or `S1*S2` produced components respectively, and apply
  the target base from `SUBOP_GET_T2` component-wise.
- [ ] Recursively consume the declared `GlslType` to normalize each component
  to float, int, or bool metadata.  Reject nonfinite floats, float-to-int
  values outside `[INT_MIN, INT_MAX]`, unsupported bases, malformed shapes,
  overflows, and count mismatches before module writing.
- [ ] Run the focused tests and require exact GREEN output plus strict
  `glslangValidator` acceptance.

### Task 3: Verify and commit

**Files:**
- Verify: `glsl_lower.c`, tests, `stdlib.c`, `parser.c`, and `parser.h`

- [ ] Run full Debug and Release CTest and require every test to pass.
- [ ] Confirm the four bundled shaders, generic hash/baseline probes, prior
  aggregate defaults, matrix tests, selector tests, uniform limits, and uniform
  structures remain green through the suite.
- [ ] Regenerate `stdlib.c` twice and parser sources once; compare SHA-256
  hashes with the checked-in artifacts.
- [ ] Run `git diff --check`, inspect the complete focused diff, commit with an
  imperative subject, and require a clean worktree.

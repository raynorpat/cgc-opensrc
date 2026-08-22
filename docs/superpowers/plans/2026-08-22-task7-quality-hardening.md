# Task 7 Quality Hardening Implementation Plan

> Work regression-first.  Do not modify the legacy Makefile or begin Task 8.

1. Add exact selector fixtures for an impure rvalue base, an impure indexed
   lvalue, an overlapping swap, and a mat3 selector.  Capture duplicate or
   sequential RED output.
2. Add deduplicated structured getter/setter helpers in `glsl_lower.c`, route
   only the required selectors through them, reject unsupported assignment
   contexts, and validate exact-once output with `glslangValidator`.
3. Add matrix and array uniform-default fixtures plus a clean struct-default
   diagnostic.  Capture the legacy C9999/truncation RED behavior.
4. Extend default binding-list provenance, opt GLSL into retained aggregate
   initializers, add arena-backed recursive finite constant flattening, and
   remove the four-component IR/codegen limit.
5. Add 512-component success and 513/516-component resource failures.  Enforce
   the selected profile's named uniform limit after deduplicated collection and
   emit one source-located C6207 diagnostic before writing a body.
6. Add base-comparison generic probes for scalar and vector-row matrix
   constructors.  Capture current divergence, then gate the expanded matrix AST
   and constant-fold behavior behind a neutral GLSL-enabled HAL capability.
   Remove generic hashes that froze the broadened behavior and retain/add only
   baseline-compatible expectations.
7. Run focused RED/GREEN tests, all successful GLSL fixtures through
   `glslangValidator`, full Debug and Release CTest, generic hashes, stdlib
   regeneration twice with unchanged hash, parser stability checks, and
   `git diff --check`.  Review the complete Task 7 diff and commit the focused
   hardening changes.

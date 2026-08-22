# Task 7 Quality Hardening Design

## Goal

Close the remaining correctness gaps in the practical Cg-to-GLSL 1.10 vertex
subset without changing the generic profile or beginning fragment lowering.
The changes cover matrix selectors, aggregate uniform-default metadata,
numeric-uniform resource accounting, and profile isolation for matrix
constructors.

## Matrix selectors

A multi-component matrix selector cannot be emitted by repeating its matrix
expression.  Repetition duplicates calls and array indices, and emitting a
write as sequential scalar assignments also corrupts overlapping swaps.

The GLSL lowerer will synthesize two kinds of IR-only helpers:

- an `in` getter for an impure rvalue base, keyed by matrix dimension, ordered
  selector mask, result base, and result width; and
- an `inout` setter for every supported multi-component lvalue, keyed by
  matrix dimension, ordered selector mask, and value type.

The getter accepts the matrix expression once and returns the selected values.
The setter accepts the lvalue matrix expression and complete RHS value once.
Its value parameter is copied before component writes, preserving simultaneous
RHS semantics for overlapping selectors.  Helpers are deduplicated, receive
collision-safe shared names, are emitted before source functions, and remain
ordinary structured GLSL IR.  Pure rvalue selectors keep their readable direct
form.  Only normalized simple assignments are supported; any other multi-matrix
selector assignment form is rejected before code generation.

## Uniform defaults

The legacy binding stores only four floats, so it remains unchanged for
profiles that consume it.  A `BindingList` default record additionally carries
the folded initializer AST and declared type.  A neutral HAL capability opts a
profile into this aggregate-default path.  Generic continues to call the old
four-value evaluator exactly as before; GLSL retains the initializer without
triggering the unfinished legacy evaluator.

During GLSL lowering, the final declaration type determines an overflow-safe
component count.  Arena-backed storage of that exact size is filled by a
recursive constant flattener over initializer lists, constructors, casts, and
constant scalar/vector nodes.  The flattener requires exactly the declared
component count and finite values.  It records values in source aggregate
order and binds metadata to the final collision-resolved uniform name.  Invalid
or unsupported aggregate forms fail cleanly instead of warning, truncating, or
reading uninitialized storage.  Struct initialization remains the front end's
existing explicit rejection; fixed array defaults are supported where the
existing initializer checker accepts them.

## Uniform component limits

After uniform collection and identity deduplication, the lowerer walks uniform
declarations once.  It uses `GlslTypeComponentCount` for scalars, vectors,
matrices, arrays, and structures and performs subtraction-form overflow checks
against `profile->limits.uniformComponents`.  Samplers and non-uniform
interfaces are excluded.  The first declaration crossing the limit records its
source location, resource name, used count, and available count.  The profile
emits one C6207 diagnostic and never invokes the module writer.  The same path
uses the fragment profile's named limit, but no fragment lowering is added.

## Generic profile isolation

A new neutral HAL capability opts into aggregate matrix constructor ASTs.  The
GLSL HAL enables it; default and generic HALs do not.  Without the capability,
`NewConstructor` and constant folding follow the exact pre-Task-7 scalar-only,
four-lane matrix-constructor path, including its historical result type and
diagnostics.  With the capability, GLSL retains the requested matrix type,
supports validated scalar/vector rows up to sixteen components, and skips the
unsafe legacy matrix constant fold.  Common code never checks profile names.

## Verification

Exact fixtures cover impure getters, impure indexed setters, overlapping
swaps, mat3 selectors, mat2/mat3/mat4 defaults, fixed arrays, name collisions,
512/513/516 uniform boundaries, and restored generic constructor behavior.
Every successful GLSL fixture is validated as GLSL 1.10 by `glslangValidator`.
The full Debug and Release suites, bundled shaders, generic hashes, parser and
stdlib reproducibility, and diff hygiene remain release gates.

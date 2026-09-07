# HLSL Lowering Modularization Design

## Summary

Refactor the monolithic `hlsl_lower.c` implementation into private,
responsibility-focused C modules while preserving every observable compiler
behavior. The refactor covers HLSL lowering only. It does not change the HLSL
IR, public API, profile behavior, diagnostics, output, or any downstream
backend phase.

The existing `HlslLowerProgram` and `HlslLowerProgramWithIR` functions remain
the only public entry points. `hlsl_lower.c` retains their orchestration and
delegates the current operations to internal modules through a private
lowering interface.

## Motivation

`hlsl_lower.c` currently contains more than 6,000 lines spanning type and
declaration conversion, expression and statement lowering, helper discovery,
default-value handling, geometry behavior, and top-level orchestration. These
responsibilities share a context but do not need to share one translation
unit. Separating them will make changes easier to review, localize failures,
and establish a repeatable pattern for later backend maintenance.

The repository already has extensive SM3, SM4, and SM5 tests, compatibility
matrices, deterministic output checks, and external FXC validation. This makes
HLSL lowering the safest backend in which to establish the modularization
pattern before considering the larger GLSL lowerer.

## Goals

- Divide HLSL lowering into private modules with clear ownership.
- Keep top-level phase ordering and mutation order unchanged.
- Preserve generated HLSL bytes, diagnostics, locations, and exit status.
- Preserve declaration, binding, helper, allocation, and generated-name order.
- Keep the public headers and exported API surface unchanged.
- Give the compiler and relevant unit-test targets one canonical lowering
  source list.
- Make later HLSL work understandable without loading the complete lowerer.

## Non-Goals

- No new HLSL profile, shader stage, language feature, intrinsic, semantic,
  resource form, optimization, or metadata.
- No change to `HlslModule`, HLSL IR nodes, or profile descriptors.
- No change to binding, legalization, validation, or code generation.
- No shared GLSL/HLSL lowering framework.
- No GLSL refactor.
- No broad test-registry rewrite or CI rollout.
- No unrelated bug fixes. A latent bug found during extraction is documented
  and deferred unless it prevents behavior-preserving extraction.

## Considered Approaches

### 1. Private internal modules with a shared context

Create real C translation units for cohesive lowering responsibilities and a
private header for their shared context and narrow interfaces. This lets the C
compiler enforce declarations and separates implementation ownership without
changing the public backend contract.

This is the selected approach. Its primary cost is the need to identify and
maintain a small private API between the extracted modules.

### 2. Included source fragments

Move sections into `.inc` files included by `hlsl_lower.c`. This would be the
least disruptive mechanical split because all current `static` functions
would remain mutually visible. It would improve navigation but retain hidden
coupling and provide no translation-unit boundary, so it is not selected.

### 3. Pass or visitor redesign

Replace the procedural lowerer with explicit passes and dispatch tables. This
could offer a cleaner long-term model, but it would change control flow,
failure propagation, and likely mutation order. That risk conflicts with the
strict equivalence requirement, so it is deferred.

## Architecture

### Public facade

`hlsl_lower.c` remains the sole public facade. It defines:

- `HlslLowerProgram`
- `HlslLowerProgramWithIR`

It also retains the top-level orchestration needed to preserve the current
phase order. No other lowering source file defines public backend entry
points.

### Private interface

Add `hlsl_lower_internal.h`. It owns:

- `HlslLowerContext`;
- `HlslValueMode`;
- declarations for cross-module lowering helpers.

The header is private to the lowering subsystem. It is not included by
`hlsl_hal.c`, exposed through `hlsl_hal.h`, or used by downstream backend
phases. The context remains stack-owned by the public facade. No mutable global
state is added.

C has no private linkage spanning translation units, so helpers called across
module boundaries necessarily have external linkage within the compiler
executable. They are declared only in `hlsl_lower_internal.h`, are not exported
by the build, and are not part of the supported public API.

### Modules

#### `hlsl_lower_support.c`

Own source-location propagation, lowering allocation, failure recording,
generated text and names, declaration lookup, and small HLSL IR construction
helpers shared by multiple lowering concerns.

#### `hlsl_lower_decl.c`

Own source-type conversion, structure discovery and ordering, declarations,
parameters, locals, uniforms, register semantics, and uniform default-value
collection and conversion.

#### `hlsl_lower_expr.c`

Own source-AST and Cg-IR expression lowering, constants, operators, calls,
intrinsics, builtin helpers, ordered evaluation, aggregate copies, swizzles,
and vector-condition normalization.

#### `hlsl_lower_stmt.c`

Own source-AST and Cg-IR statement lowering, declaration initializers, loops,
break rewriting, and statement-list construction.

#### `hlsl_lower_geometry.c`

Own geometry output state, geometry-operation lowering, emit and restart
behavior, flat replay, geometry effect analysis, output-interface collection,
and per-function geometry-state preparation.

#### `hlsl_lower_function.c`

Own source and Cg-IR call discovery, helper reachability, helper creation,
function-body lowering, returned-structure initialization, and inspection of
the original entry result.

### Dependency direction

Shared support is the lowest layer. Declaration/type facilities depend on
support. Expression lowering depends on support and declarations. Statement
and function lowering consume the expression and declaration interfaces.
Geometry is a specialized consumer of the same private facilities.

Some existing expression and geometry routines call each other. Those calls
remain direct through narrow declarations in the private header; the design
does not introduce callbacks, a visitor framework, or duplicate lowering
logic merely to make the conceptual dependency graph perfectly acyclic.

## Data Flow

Each compilation creates one `HlslLowerContext` in `hlsl_lower.c`. The facade
initializes it with the destination `HlslModule`, profile descriptor, source
scope, optional verified `CgIRModule`, selected entry file, and geometry input
extent. Every internal module receives that same context explicitly.

The existing sequence remains:

1. Validate arguments, profile stage, entry storage, and geometry layout.
2. Resolve the entry result type and collect reachable helper functions.
3. Collect structures, uniforms, bindings, defaults, parameters, and locals.
4. Lower helper bodies and the selected entry from the source tree or verified
   Cg IR, according to the existing stage-specific path.
5. Prepare geometry state at the same points in the sequence as today.
6. Initialize returned structures and perform the existing structure sort.
7. Return the existing status and leave later backend phases unchanged.

The module continues to be mutated in the current order. The refactor adds no
temporary public representation and does not alter ownership or lifetime.

## Error Handling

`HlslLowerFailure` remains the single lowering failure mechanism. It records
the first error kind, reason, and source location, increments the existing
error count, and returns failure exactly as it does before extraction.

Pointer-returning helpers continue to use `NULL` for failure. Integer and
boolean helpers continue to use `0`. Callers preserve their current distinction
between a previously recorded error and a generic unsupported-operation
fallback. The refactor adds no recovery, assertion, logging, or diagnostic
normalization.

Because error ordering is observable, validation order and short-circuit
expressions in the public orchestration are preserved. Moving a function to a
new file must not reorder its internal checks.

## Behavioral Equivalence Contract

For every accepted or rejected shader, the refactored compiler must preserve:

- generated HLSL bytes;
- diagnostic code, text, source location, and ordering;
- process exit status;
- declaration, structure, helper, and binding order;
- allocation and generated-name order;
- profile capability decisions;
- success or failure of each downstream backend phase.

Public headers, HLSL IR layout, and exported API declarations remain unchanged.
The new private-interface functions are implementation details and carry no
compatibility promise outside the lowering subsystem.

## Migration Strategy

Perform extraction from the lowest-dependency responsibilities upward. Each
step is a focused commit:

1. Add the private context and shared-support boundary.
2. Extract declarations and type lowering.
3. Extract expression lowering.
4. Extract statement lowering.
5. Extract geometry lowering.
6. Extract reachability and function lowering.
7. Reduce `hlsl_lower.c` to the public facade and finalize build wiring.

Move implementations without opportunistic renaming, formatting, or logic
cleanup. A cross-module function loses `static` only when required and is
declared solely in the private header. Any later API cleanup is a separate
change after equivalence has been demonstrated.

## Build Integration

Define one CMake variable containing every HLSL lowering source. Use that
variable for `cgc` and for each unit-test target that compiles the real
lowerer. This prevents the compiler and geometry-lowering tests from silently
using different subsets after future modules are added.

The structural contract is:

- only `hlsl_lower.c` defines the two public lowering entry points;
- only lowering implementation files include `hlsl_lower_internal.h`;
- every lowering implementation appears exactly once in the canonical CMake
  source list;
- the original implementations do not remain duplicated after extraction.

## Verification

Before extraction, produce clean Debug and Release baselines from the current
`master`. Record the complete test inventory and representative SM3, SM4, and
SM5 generated output, diagnostics, and exit codes across vertex, geometry, and
pixel stages.

After each extraction commit:

- build Debug and Release;
- run directly affected unit and profile tests;
- compare representative output and diagnostics with the baseline;
- run `git diff --check`;
- confirm that no public header or exported API declaration changed.

Final qualification requires:

- the complete 1,369-test suite passes in Debug;
- the complete 1,369-test suite passes in Release;
- Windows HLSL testing requires FXC, compiling all registered SM3, SM4, and
  SM5 witnesses with their exact targets;
- compatibility-matrix row and witness counts remain unchanged;
- baseline output, diagnostics, source locations, and exit statuses remain
  identical;
- the structural contract checks pass.

## Completion Criteria

The refactor is complete when:

- `hlsl_lower.c` contains only the public entry points and top-level
  orchestration;
- each extracted source file has one documented responsibility;
- `hlsl_lower_internal.h` is the only shared private interface;
- no new mutable global state exists;
- the expression module may be the largest module but remains limited to
  expression-related behavior rather than becoming a replacement monolith;
- all relevant targets consume the canonical lowering source list;
- the public API and HLSL IR are unchanged;
- all behavioral-equivalence and qualification checks pass.

## Follow-On Work

Once this structure is proven, separate projects may modularize HLSL binding
and validation, apply the pattern to GLSL lowering, simplify the test registry,
and add continuous integration. None of that work is required to complete this
design.

# GLSL Lowering Modularization Design

## Goal and scope

Apply the completed HLSL lowerer modularization pattern to glsl_lower.c.
The current file has approximately 9,202 lines. This is the first of two
coordinated backend refactors; ARB follows as independently testable work.
Only lowering, its build wiring and structural checks are in scope.
Code generation, backend validation algorithms, HAL behavior, new features and a
shared cross-backend framework are excluded.

## Considered approaches

The selected approach uses real private C modules sharing one private header.
It provides explicit compiler-checked boundaries while preserving behavior.
Included source fragments would retain implicit translation-unit coupling.
A shared-pass redesign would change control flow and broaden behavioral risk;
neither alternative serves this mechanical extraction.

## Architecture and ownership

glsl_lower.c retains GlslLowerLegacyProgram and GlslLowerCgIR, with their
existing orchestration and error propagation unchanged. glsl_lower_internal.h
owns the existing context and shared private types and declares only helpers
needed across module boundaries. Context ownership and initialization stay at
their existing call sites.

Planned modules:

- glsl_lower_support.c: allocation, source locations, failure recording and lookups.
- glsl_lower_decl.c: source/IR types, structures, declarations, locals and defaults.
- glsl_lower_interface.c: entry bindings, uniforms, sampler allocation and existing resource limits.
- glsl_lower_legacy_expr.c: legacy AST expression translation.
- glsl_lower_legacy_stmt.c: legacy AST statement and control-flow translation.
- glsl_lower_ir_expr.c: normalized Cg IR expression translation.
- glsl_lower_ir_stmt.c: normalized Cg IR statements and declaration initialization.
- glsl_lower_aggregate.c: aggregate materialization/copies and matrix helpers,
  selectors and grouped writes.
- glsl_lower_geometry.c: geometry interfaces, emit/restart, flat state and replay.
- glsl_lower_function.c: call discovery, reachability, helper signatures/names,
  forward calls and function lowering.

These responsibilities are approved; the implementation plan must resolve exact
function ownership from callers before extraction. Shared expression/aggregate
and geometry/statement calls remain explicit private calls. Do not duplicate
legacy and IR logic or merge their algorithms as part of the move.

## Data flow and error handling

Both existing lowering routes retain their distinct traversal and initialization
sequences. Shared collection and helper routines run at the same points as today.
The same context/module objects flow through extracted functions. Preserve first
failure recording, fallback errors, NULL/integer failure returns and source
location propagation exactly.

## Build and backend-specific qualification

Define CGC_GLSL_LOWER_SOURCES before tests are configured, containing the facade
and every private lowering C module exactly once. Both cgc and glsl_lower_ir_unit
consume it; audit any other target compiling real lowering code.

Exercise legacy AST and normalized IR paths, vertex/fragment/geometry profiles,
matrix selectors and grouped writes, aggregate copies/materialization, helper
reachability, resource limits and negative diagnostics. Retain all existing GLSL
external validators and pipeline tests. Identify the exact existing test names
and validator availability requirements in the implementation plan.

## Behavioral contract

Preserve accepted and rejected programs, generated shader content, diagnostic text,
codes, locations and order, and exit status. Preserve evaluation, declaration,
binding, allocation, temporary naming and helper ordering. Public headers, IR
layouts, profile capabilities, ownership and lifetime remain unchanged. Add no
mutable global state.

Move complete function bodies and their comments mechanically. Change linkage only
for real cross-module calls. Preserve existing validation and short-circuit order;
do not repair latent compiler bugs during extraction unless they prevent the
mechanical split. Record such blockers and resolve them in separate focused work.

## Migration and dependency audit

Before assigning extraction tasks, inventory every definition, private type, forward
declaration and function-static object. Map every caller to its intended module,
including callers in the facade. Specify exact private declarations and linkage
changes for each intermediate commit so that every step compiles independently.
Keep direct dependencies where the existing code requires them; do not introduce
callbacks or duplicate logic to force an acyclic dependency graph.

Extract shared facilities first, then dependent responsibilities, then reduce the
facade. Each extraction has a focused commit and an exact-body/ownership audit.
Keep the NVIDIA redistribution notices and the existing C90-with-extensions style.

## Verification

Create a pristine detached baseline and a separate candidate build before work.
Never reuse candidate outputs as baseline evidence. Record the baseline commit,
toolchain, validator versions, test names and output paths. Preserve the complete
existing test inventory; added structural tests are the only planned count change.

After every extraction, build Debug and Release, run affected tests, compare
representative outputs and diagnostics, and check whitespace and public headers.
The four bundled shaders (position, reflection, vertexlight and vertexlight4)
remain required generic-profile regressions.

Final qualification runs complete Debug and Release suites with the existing
external validation configuration, including required FXC for HLSL regressions.
Compare complete artifact relative-path sets and SHA-256 hashes, rejecting missing
or extra files. Require raw equality for deterministic outputs. If legacy output
has volatile metadata, first classify every differing line, then replace only
identified timestamp or absolute-root values. No shader body, profile or semantic
content may be normalized. Compare failure diagnostics and exit status as well.

Structural tests enforce exact canonical source membership and uniqueness, use by
all real-lowerer targets, private-header ownership, facade definition ownership,
and a fixed facade line ceiling selected from the unchanged orchestration size
during planning. Do not raise the ceiling to accommodate leftover private logic.
Test the checker with positive and negative fixtures, including production
duplicates and private-header leaks, intentional test doubles, and ignored nested
.worktrees/build/generated directories. Worktree exclusions must match directory
boundaries. Test-double exceptions apply only to public-definition ownership.

## Completion and handoff

Completion requires unchanged behavior, public interfaces and state lifetime;
exact definition ownership; successful builds, full suites, output comparisons
and structural checks; and focused history without unrelated changes.
Run the structural check from the main checkout as part of integration verification.
Preserve existing user changes and unrelated worktrees.

The next deliverable is a separate implementation plan with exact function
inventories, caller-derived private APIs, file paths, extraction order, commands,
expected outcomes and review checkpoints.


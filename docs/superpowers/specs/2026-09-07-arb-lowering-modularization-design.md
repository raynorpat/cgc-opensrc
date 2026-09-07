# ARB Assembly Lowering Modularization Design

## Goal and scope

Apply the HLSL private-module pattern to arb_lower.c after the GLSL refactor.
The current file has approximately 2,884 lines. This is a separate implementation
plan and independently verifiable change, using a fresh baseline after GLSL
integration. Scope is lowering, its build wiring and structural checks.
arb_codegen.c, ARB IR layouts, HAL/profile behavior, compiler warning cleanup,
new assembly instructions and backend redesign are excluded.

## Considered approaches

The selected approach uses real private C modules with compiler-checked
interfaces. Included fragments would improve navigation but preserve hidden
coupling. A shared backend-pass redesign adds unnecessary behavioral risk.
ARB's boundaries follow assembly responsibilities rather than copying the
larger GLSL module inventory.

## Architecture and ownership

arb_lower.c retains ArbLowerProgram with unchanged setup, traversal and cleanup.
arb_lower_internal.h owns ArbLowerContext and the shared private tracking types,
plus declarations needed across modules.

- arb_lower_support.c: temporary-register tracking, tracked symbols,
  consumed statements and shared operand utilities.
- arb_lower_operand.c: binding resolution, connector members, lvalues and swizzles.
- arb_lower_expr.c: constants, arithmetic, comparisons, builtins, constructors,
  dot recognition, evaluation-cost logic and instruction selection.
- arb_lower_stmt.c: assignments, expression statements, conditionals, discard
  and statement traversal.
- arb_lower_loop.c: constant evaluation, static induction state, initializer
  pairing, termination checks, iteration simulation and loop expansion.

The plan will resolve exact helper/type ownership from all callers before editing.
Newly external helpers receive distinctive ArbLower-prefixed names where necessary
to avoid generic compiler symbol collisions. Record a complete old/new mapping and
update declarations, definitions and callers consistently. Helpers confined to
one module remain static and retain their existing names.

## Data flow and error handling

Preserve traversal order, instruction emission and temporary allocation/release
order. Preserve static-value push/pop, consumed-statement tracking and paired-loop
initializer behavior at the same call points. Every error and early return retains
its existing cleanup behavior and source location. No new optimization or change
to loop eligibility is included.

## Build and backend-specific qualification

Define CGC_ARB_LOWER_SOURCES before test configuration and use it in cgc and any
target compiling real ARB lowering. Do not add lowering dependencies to IR-only
tests or the assembly-loading smoke executable merely to share the list.

Exercise arbvp1 and arbfp1 assembly, bindings, relative indexing, swizzles,
arithmetic, dot recognition, comparison/select behavior, static for/while/do loops,
termination rejection, texture forms, resource boundaries and diagnostics.
Retain both existing WGL smoke tests and distinguish unavailable platform support
from an actual successful assembly validation. Compare all generated ARB assembly
artifacts and identify their exact extensions and test names during planning.

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


# OpenGL ARBVP1 and ARBFP1 Profile Support Design

## Objective

Add `arbvp1` and `arbfp1` profiles to this Cg compiler front end. The new
profiles will emit programs accepted by conforming implementations of
`ARB_vertex_program` and `ARB_fragment_program`, while following the observable
binding, metadata, and diagnostic conventions of NVIDIA Cg 3.1 where they
intersect with this repository's older Cg dialect.

The existing parser, type system, and standard library remain authoritative.
The work adds profile-specific types and built-ins through the existing HAL
registration hooks, but does not attempt to recreate the Cg 3.1 language or
standard library.

Success means that every construct accepted by the existing front end and
representable by the base ARB program specifications either produces valid,
resource-compliant assembly or receives a precise diagnostic. It does not mean
that every ARB assembly opcode must be emitted when no source-language construct
requires it.

The assembly contracts are the Khronos
[ARB_vertex_program](https://registry.khronos.org/OpenGL/extensions/ARB/ARB_vertex_program.txt)
and
[ARB_fragment_program](https://registry.khronos.org/OpenGL/extensions/ARB/ARB_fragment_program.txt)
specifications. An installed NVIDIA Cg 3.1 compiler may be used as an optional
differential-testing oracle, but is never a build or runtime dependency.

## Scope

The profiles target the base `!!ARBvp1.0` and `!!ARBfp1.0` languages. They use
the minimum resources guaranteed by the base specifications so emitted programs
remain portable across conforming implementations. NVIDIA profile options and
extension syntax are outside the initial scope.

Compatibility is observable rather than byte-for-byte. Within the guaranteed
base-profile resource envelope, the implementation will match accepted and
rejected common-dialect programs, semantic and uniform bindings, metadata,
resource-limit behavior, and driver acceptance. Programs that need resources
beyond that portable envelope are rejected even when NVIDIA Cg 3.1 would accept
them for a more capable implementation. Equivalent instruction ordering,
temporary naming, declaration choices, and whitespace may differ from NVIDIA
Cg 3.1.

## Architecture

The backend pipeline is:

```
normalized Cg AST
    -> vector IR
    -> profile legalization
    -> register allocation
    -> resource validation
    -> ARB assembly
```

The vector IR is private to the ARB backend. The parser, semantic analyzer, and
generic profile do not depend on it. ARBVP1 and ARBFP1 share lowering,
allocation, validation, and formatting infrastructure; their HAL modules supply
stage-specific semantics, instructions, resources, and binding policy.

### Components

- `arb_hal.h` and `arb_hal.c` register both profiles and implement shared HAL
  behavior, binding helpers, common capability checks, and ARB diagnostics.
- `arbvp1_hal.c` defines vertex inputs, outputs, semantics, address-register
  behavior, base limits, and `!!ARBvp1.0` policy.
- `arbfp1_hal.c` defines fragment inputs, outputs, sampler types and built-ins,
  texture bindings, base limits, and `!!ARBfp1.0` policy.
- `arb_ir.h` and `arb_ir.c` define and manage virtual registers, operands,
  source modifiers, swizzles, destination masks, instructions, parameters,
  texture targets, and source locations.
- `arb_lower.c` converts the compiler's normalized statements and expressions
  into vector IR. It handles static loop expansion, expression ordering,
  matrix expansion, conditional conversion, intrinsics, and texture operations.
- `arb_codegen.c` legalizes instructions, performs small correctness-oriented
  optimizations, allocates registers, validates resource use, emits metadata and
  declarations, and writes assembly.
- `tests/arb_smoke.c` is a dependency-free Windows/WGL program that creates a
  hidden OpenGL context, loads emitted vertex or fragment assembly, and reports
  the ARB error position and error string.

The source tree remains flat for compiler modules, matching the existing
project. Test programs, expected output, and test-only utilities live below
`tests/`.

## Frontend and HAL Integration

`cgcmain.c` adds `RegisterProfiles_arb` to the startup registration table.
Selection continues through the existing `-profile` option and `InitHAL`
dispatch.

Both profiles request full function inlining, restricted returns, matrix
deconstruction, struct-assignment flattening, and conditional flattening through
the existing compilation pipeline. They use late bindings so code generation
can finish validation before writing binding metadata or instructions.

The fragment profile's `RegisterNames` hook adds the sampler base types required
by the base ARB fragment language, using the reserved user-type slots in the
existing type system. It also registers the corresponding texture built-ins in
the super-global scope through the existing internal-function mechanism. The
initial set covers non-shadow 1D, 2D, 3D, cube, and rectangle texture targets.
The vertex profile does not expose texture functions.

Other standard-library operations continue to use the current inline
definitions where present. Internal operations such as reciprocal square root
are assigned stable built-in group and index values by the ARB HAL and selected
through instruction tables during lowering.

## Binding Model

Bindings are table-driven rather than embedded in the lowering code.

### Vertex Bindings

ARBVP1 accepts the conventional Cg input semantics supported by the ARB vertex
namespace, including position, blend weight, normal, primary and secondary
color, fog coordinate, and indexed texture coordinates. It also accepts generic
`ATTRn` inputs where the specification permits them. The profile validates the
ARB aliasing restrictions between generic and conventional vertex attributes.

Vertex outputs map to position, primary and secondary color, fog coordinate,
point size, clip distances, and indexed texture coordinates. Required output
rules and read/write permissions are represented in connector descriptors and
checked before lowering.

### Fragment Bindings

ARBFP1 accepts fragment position, primary and secondary color, fog coordinate,
and indexed texture-coordinate inputs. The base profile writes one color result
and may write depth. Multiple render targets are excluded because they require
extension syntax.

Sampler parameters bind through explicit `TEXUNITn` semantics or deterministic
first-free allocation. Two samplers may not claim the same unit with
incompatible targets. Texture target, coordinate width, projection behavior,
and sampler stage legality are verified before emitting a texture instruction.

### Uniform Bindings

Unbound numeric uniforms are allocated into contiguous `program.local[]`
vectors in declaration order. Scalars and vectors occupy one vector slot;
matrices, arrays, and structures use stable recursive layouts. Explicit
constant, default, and pragma bindings continue through the existing binding
tree where they map to base ARB program resources.

ARBVP1 permits legal relative indexing of uniform parameter arrays through
address registers. ARBFP1 requires uniform-array indices to resolve at compile
time. Binding metadata records the same public parameter names, semantics,
register names, offsets, and sizes expected by the existing Cg output format.

## Vector IR

The IR models the ARB machines without encoding either profile's spelling. An
instruction contains:

- a profile-neutral opcode;
- an optional destination virtual register and component mask;
- zero to three source operands;
- source swizzles, negation, and absolute modifiers where legal;
- optional saturation and texture target/unit information;
- a source location for diagnostics; and
- resource classification used by the validator.

Operands identify virtual temporaries, inputs, outputs, uniforms, constants,
address registers, or profile state. Constants are interned so repeated values
share `PARAM` declarations. Virtual registers are four-component values, but
component liveness and write masks are retained to avoid unnecessary moves and
temporaries.

IR construction and destruction use the compiler's existing memory-management
conventions. An internal validator runs after construction and after
legalization, ensuring that every instruction has a legal operand count,
initialized value, stage classification, and component shape before allocation
or emission.

## Lowering and Legalization

The existing pipeline first inlines functions, expands increment/decrement and
compound assignments, flattens comma and chained assignments, converts named
constants, deconstructs matrices, flattens structures and conditionals, and
folds constants.

`arb_lower.c` then performs ARB-specific normalization:

- Loops with compile-time-resolvable initialization, bound, step, and finite
  trip count are expanded. Data-dependent or otherwise non-unrollable loops are
  rejected.
- Conditional expressions evaluate both alternatives, matching the historical
  profile behavior. ARBFP1 uses `CMP` when legal; ARBVP1 uses compare and
  arithmetic select sequences.
- Fragment discard lowers to `KIL`. Vertex discard is rejected.
- Fragment uniform indexing must be constant. Vertex relative uniform indexing
  lowers through an address register and `ARL` subject to the ARB rules.
- Assignments preserve destination masks, and reads preserve source swizzles and
  modifiers.
- Matrix and structure values are emitted as their decomposed vector
  assignments.
- Texture built-ins lower to instructions carrying an explicit sampler unit and
  target. Texture operations in vertex programs are rejected.

Instruction selection is table-driven. Each source operator or internal
function identifies a preferred IR opcode. The profile descriptor either maps
that opcode to a native instruction, expands it into a legal sequence, or emits
an unsupported-operation diagnostic. No invalid operation is silently
approximated.

Before allocation, the backend folds legal source modifiers and constants,
coalesces safe moves, removes dead temporaries, and chooses an expression order
that reduces peak live values. These transformations exist to improve resource
fit and correctness; general-purpose optimization and NVIDIA-equivalent
scheduling are not goals.

## Register Allocation and Resource Validation

The allocator computes component-aware live intervals and assigns deterministic
physical `TEMP` registers with linear scan. Safe move coalescing is preferred,
but the backend does not spill because base ARB programs have no spill storage.
Failure to fit the guaranteed temporary-register count produces a source-located
resource diagnostic.

Address registers are allocated separately for vertex relative addressing.
Parameters and samplers use the binding allocations established before IR
lowering.

After legalization and allocation, the validator computes all resources defined
by the selected base specification. These include temporaries, address
registers, local parameters, total instructions, and the fragment profile's
math instructions, texture instructions, texture units, and texture-indirection
depth. Texture indirection is calculated from instruction dependencies, not
merely by counting texture operations.

Every limit is tested at its exact guaranteed boundary and one unit beyond it.
Resource overflow prevents binding metadata and assembly instructions from
being written.

## Output Contract

`PrintCodeHeader` writes `!!ARBvp1.0` or `!!ARBfp1.0`. The compiler's existing
version and command-line comments follow. Because the profiles request late
bindings, successful generation then writes:

1. vendor, version, profile, and program metadata;
2. `#var`, `#const`, and `#default` records;
3. required `TEMP`, `PARAM`, `ATTRIB`, and `OUTPUT` declarations;
4. legal ARB instructions;
5. `END`; and
6. commented instruction and resource statistics.

Formatting, declarations, and naming are deterministic. NVIDIA Cg conventions
are followed where doing so does not complicate correctness, but exact temporary
names, declaration elision, scheduling, and whitespace are not compatibility
requirements.

The output file may contain the profile header and compiler comments when an
earlier parse or backend error occurs because the current driver opens and
initializes output before parsing. It must not contain binding metadata or a
partial instruction stream after a backend validation failure.

## Diagnostics

New diagnostics follow the existing `errors.h` numbering and reporting style.
They cover:

- operations or data types unavailable in the selected stage;
- non-unrollable loops and illegal dynamic indexing;
- invalid, conflicting, or stage-inappropriate semantics;
- texture-unit and texture-target conflicts;
- writes to read-only inputs and absent required outputs;
- source or destination modifier combinations that cannot be represented;
- each independently tracked resource limit; and
- malformed IR, reported as an internal compiler error rather than a user
  semantic error.

Diagnostics retain the nearest source location carried into the IR. Tests match
stable diagnostic identifiers and meaningful fragments rather than relying on
the entire prose string.

## Test Design

`CMakeLists.txt` includes CTest support. Compiler fixtures run on every platform
where `cgc` builds. When `BUILD_TESTING` and `WIN32` are both true, CMake also
builds `tests/arb_smoke.c`, links it to `opengl32`, and registers driver-load
tests.

The fixture tree is organized as follows:

- `tests/arb/profile/` checks registration, profile selection, and headers.
- `tests/arb/vp-semantics/` and `tests/arb/fp-semantics/` cover accepted semantic
  mappings and conflicts.
- `tests/arb/arithmetic/` covers scalar and vector operations, masks, swizzles,
  constants, matrices, comparisons, and supported intrinsics.
- `tests/arb/control-flow/` covers conditional conversion, static loop
  expansion, rejected dynamic flow, and fragment discard.
- `tests/arb/uniforms/` covers packing, arrays, structures, constants, defaults,
  and vertex relative addressing.
- `tests/arb/textures/` covers every base texture target, sampler allocation,
  dependent reads, and invalid target or unit combinations.
- `tests/arb/limits/` contains exact-boundary and one-over-limit cases.
- `tests/arb/diagnostics/` checks expected failure status and diagnostic
  fragments.
- `tests/arb/oracle/` contains common-dialect programs used for optional NVIDIA
  Cg 3.1 comparisons.

A shared CMake script invokes `cgc`, captures status and output, normalizes the
documented non-semantic differences, and compares valid assembly or expected
diagnostics with checked-in expectations. Generated assembly is written only to
the build tree.

The NVIDIA oracle is configured through an optional CMake cache variable such as
`CGC_REFERENCE_EXECUTABLE`; no machine-specific path is checked in. Oracle tests
compare acceptance, public bindings, declarations, resource classifications,
and diagnostics for the common frontend subset. Normal CTest runs remain
independent of proprietary software.

`arb_smoke` accepts a stage and assembly-file path. It creates a hidden WGL
context, resolves the required ARB functions, submits the program with
`glProgramStringARB`, and reports `GL_PROGRAM_ERROR_POSITION_ARB` and
`GL_PROGRAM_ERROR_STRING_ARB`. It returns CTest's configured skip code when a
suitable context or extension is unavailable. If the extension exists, any ARB
parse/load failure fails the test.

The smoke executable validates driver acceptance only; it does not render and
compare pixels. Compiler regression coverage additionally runs all four bundled
shaders through the generic profile and compares status and output with
pre-change baselines.

## Delivery Milestones

1. Add the CTest fixture runner, optional oracle configuration, profile
   registration, and profile-header and semantic tests.
2. Add the shared IR, validator, emitter skeleton, and simple ARBVP1 arithmetic.
3. Complete ARBVP1 matrices, conditionals, static loops, uniform packing,
   relative addressing, register allocation, and resource validation.
4. Add ARBFP1 semantics, sampler types and intrinsics, arithmetic, uniforms, and
   color/depth output.
5. Add fragment textures, dependent-read accounting, discard, allocation, and
   all remaining base fragment limits.
6. Integrate WGL smoke tests, complete the oracle corpus, verify generic-profile
   behavior, update user documentation, and harden compatibility diagnostics.

Each milestone leaves the generic profile working and ends with focused CTest
and manual validation plus a single-purpose commit.

## Non-Goals

- Updating the parser or common standard library to Cg 3.1.
- NVIDIA `vp20`, `fp20`, or other vendor profiles.
- NVIDIA `-profileopts` and configurable resource overrides.
- Draw-buffer, fragment-coordinate-convention, shadow-texture, or other
  extension syntax beyond the base ARB program specifications.
- Runtime or data-dependent branching.
- Byte-identical NVIDIA assembly, scheduling, formatting, or diagnostics prose.
- General optimization beyond the small resource-fit transformations described
  above.
- Rendering-result validation in the WGL smoke executable.
- A build, test, or runtime dependency on the proprietary NVIDIA compiler.

# DirectX 9.0c HLSL Shader Model 3 Profiles Design

## Objective

Add `hlslv` and `hlslf` profiles to this Cg compiler front end. The profiles
translate vertex and fragment entry points into deterministic, standalone HLSL
source intended for the legacy DirectX 9 Shader Model 3 targets `vs_3_0` and
`ps_3_0`.

The compiler emits source only. It does not load a DirectX compiler, emit
bytecode, or add a runtime dependency on the DirectX SDK. An installed
`fxc.exe` is an optional test validator.

Broad Cg compatibility has a bounded meaning: every construct accepted by the
repository's current Cg front end and representable by Shader Model 3 must
either produce valid HLSL or receive a precise, source-located diagnostic. The
work does not add a geometry stage, implement DirectX effects, reproduce newer
Cg language revisions, or accept constructs that Shader Model 3 cannot
represent faithfully.

The authoritative target references are Microsoft's documentation for
[Shader Model 3](https://learn.microsoft.com/en-us/windows/win32/direct3dhlsl/dx-graphics-hlsl-sm3),
[Direct3D 9 semantics](https://learn.microsoft.com/en-us/windows/win32/direct3dhlsl/dx-graphics-hlsl-semantics),
[writing Direct3D 9 HLSL](https://learn.microsoft.com/en-us/windows/win32/direct3dhlsl/dx-graphics-hlsl-writing-shaders-9),
[vertex Shader Model 3 registers](https://learn.microsoft.com/en-us/windows/win32/direct3dhlsl/dx9-graphics-reference-asm-vs-registers-vs-3-0),
and
[pixel Shader Model 3 registers](https://learn.microsoft.com/en-us/windows/win32/direct3dhlsl/dx9-graphics-reference-asm-ps-registers-ps-3-0).

## User-Facing Profiles

The profiles are registered through the existing HAL mechanism:

- `hlslv`, profile ID 14, emits vertex HLSL for `vs_3_0`;
- `hlslf`, profile ID 15, emits pixel HLSL for `ps_3_0`;
- connector IDs 18 through 21 are reserved in stage and direction order for
  the new profiles.

Both profiles emit a public HLSL entry named `main`. Typical external
compilation is:

```text
fxc /T vs_3_0 /E main vertex.hlsl
fxc /T ps_3_0 /E main pixel.hlsl
```

The selected Cg entry name remains in compiler metadata. Generated identifiers
are deterministic but are not a public compatibility contract other than the
public HLSL entry name `main` and the documented binding records.

## Architecture

The backend is independent of the existing GLSL and ARB intermediate
representations:

```text
normalized Cg AST
    -> typed structured HLSL IR
    -> entry-wrapper and interface construction
    -> Shader Model 3 legalization
    -> deterministic binding allocation
    -> whole-module validation
    -> binding metadata and HLSL source
```

This avoids an invasive extraction of the mature, dialect-specific GLSL IR and
avoids mixing semantic decisions into a direct AST printer. The two HLSL
profiles share all common lowering, legalization, allocation, validation, and
emission code. Stage descriptors provide only the policies that genuinely
differ between vertex and pixel shaders.

### Components

- `hlsl_hal.h` and `hlsl_hal.c` register both profiles, install shared HAL
  callbacks, select the active stage descriptor, expose HLSL sampler types and
  internal functions, and configure common front-end normalization.
- `hlslv_hal.c` and `hlslf_hal.c` define stage semantics, aliases, legal
  operations, sampler policy, required outputs, and resource limits.
- `hlsl_ir.h` and `hlsl_ir.c` own typed declarations, structures, arrays,
  functions, statements, expressions, interface fields, physical bindings,
  generated names, and original source locations.
- `hlsl_lower.c` converts the normalized compiler tree into HLSL IR while
  preserving evaluation order and source identity.
- `hlsl_legalize.c` rewrites Cg-only constructs into exact Shader Model 3
  forms, specializes or inlines only when required, and rejects constructs
  without a faithful representation.
- `hlsl_bind.c` canonicalizes interface semantics, creates the public wrapper,
  allocates register banks, represents flattened aggregate bindings, and
  prepares stable metadata.
- `hlsl_validate.c` verifies IR invariants, stage rules, interface uniqueness,
  required results, binding overlap, and resource bounds.
- `hlsl_codegen.c` is a deterministic pretty-printer. It makes no new semantic
  or allocation decisions.

All files remain in the flat source root, following the repository's existing
compiler-module layout. Tests and test utilities live below `tests/hlsl/`.

## Front-End and HAL Integration

`cgcmain.c` registers the HLSL profiles during normal startup, and
`CMakeLists.txt` includes the new backend sources. Profile selection continues
through `-profile` and the existing `InitHAL` dispatch.

The profiles preserve native structured conditionals, loops, returns,
matrices, and aggregate temporaries. They request common normalization only
where the rewrite is semantics-preserving and simplifies lowering. In
particular, they do not request the aggressive matrix, structure, or control
flow flattening used by assembly targets.

The HAL `RegisterNames` path adds the legacy HLSL sampler types required by the
front end and registers internal functions that are not already expressed by
the checked-in standard library. Built-in selection is signature-driven so an
unsupported overload cannot accidentally lower to a similarly named HLSL
operation.

The HLSL profiles use late bindings. Generation builds and validates the whole
module before it writes binding records or shader declarations.

## Typed HLSL IR

The IR models structured HLSL rather than target assembly. Each node retains:

- its complete type and qualifiers;
- its source symbol identity independently of its emitted spelling;
- its nearest original `SourceLoc`;
- child expressions or statements in guaranteed evaluation order; and
- an optional interface semantic or physical register binding.

The IR supports scalar, vector, rectangular matrix, array, and structure
types; global and local declarations; overloaded helpers; entry interfaces;
structured control flow; calls; constructors; selectors; swizzles; indexing;
casts; and assignment forms. The implementation provides constructors,
destruction through the compiler's memory conventions, and an internal
validator used both before and after legalization.

A deterministic name allocator protects all HLSL reserved words and all
backend-reserved prefixes. Repeated references to the same source identity
reuse one emitted name. Different identities with the same spelling receive
stable numeric suffixes. Binding metadata always uses the public Cg identity,
not the sanitized HLSL identifier.

## Entry-Point ABI

The public HLSL `main` function is a generated wrapper. The selected Cg entry
becomes an internal implementation function.

The wrapper:

1. receives varying inputs through a generated stage-input structure;
2. creates and initializes values for `out` and `inout` parameters;
3. reads uniform arguments from explicitly bound generated globals;
4. calls the internal entry exactly once;
5. collects the return value and every varying `out` or `inout` value; and
6. returns a generated stage-output structure.

This design supports Cg entry functions that combine returned structures,
varying parameters, uniform parameters, and `out` or `inout` parameters. Each
source expression is evaluated once. The wrapper and internal entry use names
reserved through the identity-aware allocator, so a source function or local
named `main` cannot collide with the ABI.

Uniform entry parameters become globals because Direct3D 9 HLSL exposes global
uniforms through its constant table and permits explicit register placement.
Source-global uniforms use the same allocation path. Non-entry helper
parameters retain their logical source qualifiers after legalization.

## Interface Semantics

Interface mapping is table-driven and stage-specific. Valid DirectX 9
semantics, including `POSITION`, `NORMAL`, `COLORn`, `TEXCOORDn`, `TANGENTn`,
`BINORMALn`, `BLENDWEIGHTn`, `BLENDINDICESn`, `PSIZE`, `FOG`, `VPOS`, `VFACE`,
and pixel outputs `COLORn` and `DEPTH`, are accepted only in legal directions
and stages.

Cg aliases are canonicalized before conflict checking. `HPOS` maps to the
stage-appropriate `POSITION`, `COLn` to `COLORn`, `TEXn` to `TEXCOORDn`, `WPOS`
to pixel `VPOS`, and `FACE` to pixel `VFACE`. Generic vertex `ATTRn` inputs map
to `TEXCOORDn`; an explicitly declared `TEXCOORDn` therefore conflicts with
`ATTRn` at the same index instead of silently occupying a different vertex
register. Every alias is recorded in binding metadata so the application can
build a matching vertex declaration. Vertex-output and pixel-input
canonicalization uses the same table, ensuring linked varyings receive
identical semantics and compatible types.

The binder rejects:

- unknown semantics that are not valid DirectX 9 interface usages;
- recognized semantics in an invalid stage or direction;
- out-of-range semantic indices;
- two source fields that claim one canonical interface location;
- incompatible vertex-output and pixel-input declarations in link fixtures;
  and
- a vertex entry that does not produce the required homogeneous position.

Special pixel inputs `VPOS` and `VFACE` and pixel outputs `COLORn` and `DEPTH`
receive their Shader Model 3 width and type checks.

## Uniform and Sampler Bindings

The backend allocates four independent physical banks:

- `float`, `half`, and `fixed` uniform data use `c#` registers;
- integer uniform data use `i#` registers;
- Boolean uniform data use `b#` registers;
- sampler data use `s#` registers.

Explicit source bindings and sampler `TEXUNITn` semantics are honored first.
Unbound resources use deterministic first-fit allocation in source declaration
order. The allocator detects overlaps even when they arise through separate
aggregate leaves or through explicit and implicit bindings.

Scalars and vectors occupy a defined physical register span. Matrices use an
explicit packing convention and a contiguous, dimension-derived span. Arrays
remain contiguous to preserve legal dynamic indexing. Structures use a stable
recursive layout. Homogeneous aggregates are emitted intact when HLSL can
preserve their physical layout. Mixed-bank aggregates are represented by
physical leaf globals and are reconstructed into a logical value only where a
call or expression requires the complete aggregate.

Every emitted global carries an explicit legacy register annotation such as
`register(c0)`, `register(i0)`, `register(b0)`, or `register(s0)`. Binding
metadata records the public name, semantic, bank, base register, recursive
offset, type, and span. Default values are emitted as HLSL initializers where
the legacy compiler supports them and always as stable `// cgc-default`
metadata.

Allocation includes declarations in the public interface even when an
external optimizer may remove an unused HLSL global. The metadata therefore
remains a complete source-level contract, while the generated bytecode's
constant table remains the external compiler's responsibility.

## Language Lowering and Legalization

The backend retains every current front-end construct that Shader Model 3 can
represent faithfully.

### Types and Aggregates

`float` maps directly to HLSL `float`. `half` and `fixed` both promote to
`float`: this avoids compiler-dependent partial-precision behavior, and Shader
Model 3 HLSL has no portable fixed-point source type. Metadata retains the Cg
source type. Integers and Booleans remain distinct when their use and binding
banks require it.

Vectors, rectangular matrices, arrays, and structures remain native typed
values. Aggregate assignments and arguments remain native when accepted by the
target compiler; legalization synthesizes deterministic member-wise copies
when required. Array and structure expressions with side effects are captured
into temporaries so the source expression runs once.

Every matrix declaration has an explicit `row_major` qualifier, matching
[Cg's default matrix-parameter order](https://developer.download.nvidia.com/cg/cgGetMatrixParameterOrder.html).
The current front end does not expose Cg's matrix-packing pragma, so there is no
per-declaration packing override to preserve. Matrix construction, indexing,
selectors, casts, and multiplication are rewritten as needed so behavior is
independent of an external `/Zpr` or `/Zpc` default. Golden and external
validation tests cover square and rectangular matrices, matrix-vector and
vector-matrix multiplication, arrays, parameters, and uniform bindings.

### Expressions, Calls, and Control Flow

Constructors, casts, swizzles, vector comparisons, Boolean-vector operations,
conditional expressions, comma expressions, compound assignment, and
increment or decrement are emitted directly only when HLSL has the same value
and side-effect semantics. Otherwise the lowerer introduces typed temporaries
and structured statements.

Helpers remain functions. Overloads are resolved from the source symbol and
signature, not from emitted text. Legalization specializes or inlines a helper
only when a Shader Model 3 rule requires it. Recursion is rejected with a
source-located diagnostic.

Structured `if`, `for`, `while`, and `do` statements, early returns, `break`,
and `continue` remain structured. The backend relies on Shader Model 3 dynamic
flow instead of pre-emptively unrolling source loops. The external HLSL
compiler remains responsible for optimization-dependent instruction and
temporary-register fit.

### Intrinsics and Textures

Intrinsic mapping uses exact source and result signatures. A mapping table
selects a native HLSL intrinsic, a generated helper, a semantics-preserving
expansion, or a dedicated unsupported-intrinsic diagnostic. The table covers
all current numeric, geometric, comparison, derivative, and packing operations
exposed by the front end and standard library.

Pixel texture lowering covers each legacy sampler dimension supported by the
front end and all exact Shader Model 3 forms for ordinary, projected, biased,
explicit-LOD, and gradient sampling. Pixel discard lowers to HLSL `discard`.
Derivatives and discard are rejected in the vertex stage.

Vertex texture lowering accepts only the explicit-LOD sampler and coordinate
forms that compile for `vs_3_0`; implicit-derivative forms are rejected. The
stage descriptor checks sampler dimension, coordinate width, unit, and
intrinsic signature before emission. Sampler arrays, shadow or comparison
sampling, and other forms without a faithful DirectX 9 representation are
rejected rather than approximated.

## Resource Validation

The backend validates resources that are knowable from HLSL source and the
explicit binding contract:

- 16 vertex input registers;
- 12 vertex output registers;
- 10 pixel input registers;
- at least 256 vertex float constant registers;
- 224 pixel float constant registers;
- 16 integer constant registers per stage;
- 16 Boolean constant registers per stage;
- four vertex samplers; and
- 16 pixel samplers.

Each exact boundary and one-over-boundary case receives a test. Validation
counts canonical interface locations and allocated physical spans rather than
source declarations, so aliases and aggregates cannot bypass limits.

The backend does not guess the instruction count, temporary-register count,
compiled flow-control depth, or optimization-dependent relative-addressing
cost of generated HLSL. Those properties are decided by `fxc`; optional
external compilation tests detect failures. This division avoids rejecting a
source program that the HLSL compiler can optimize into a legal Shader Model 3
program.

## Output Contract

Successful output is ordered as follows:

1. compiler version, profile, target, and selected-program comments;
2. deterministic `// cgc-bind` and `// cgc-default` records;
3. explicitly registered global uniforms and samplers;
4. generated and source structure declarations;
5. helper prototypes and definitions in dependency-safe order;
6. the lowered internal entry function; and
7. the public `main` wrapper.

Formatting and identifier allocation are deterministic. The output contains no
effects framework, bytecode, generated C header, include dependency, or
machine-specific path.

The driver opens the output and prints its ordinary compiler header before
parsing. A failed compile may therefore leave header comments. The backend must
not write binding metadata, declarations, or a partial function body until the
complete IR has passed legalization, allocation, and validation.

## Diagnostics

Numbers 6400 through 6499 are reserved for HLSL user diagnostics. They cover:

- unsupported types, operations, and intrinsic signatures;
- operations invalid in the selected stage;
- unknown, invalid, or conflicting semantics;
- missing or malformed required vertex position output;
- entry-wrapper and qualifier conflicts;
- constant or sampler register collisions and bank exhaustion;
- sampler dimension, unit, coordinate, and operation mismatches;
- generated-name failures; and
- features whose behavior cannot be preserved by Shader Model 3.

Malformed HLSL IR receives a new internal-error number after the existing ARB
internal IR diagnostic. User diagnostics retain the nearest original source
location carried into the IR.

Backend callbacks record the error count before delegation. When a callback
emits a specific HLSL diagnostic and returns failure, common semantic code must
not add a second generic binding error. Every phase stops on failure, destroys
or abandons its private IR safely, and prevents subsequent output phases.

## Compatibility Matrix

The implementation adds `docs/hlsl-sm3-compatibility.md`. It inventories every
type, qualifier, statement form, operator family, semantic family, intrinsic
family, texture form, binding form, and relevant standard-library feature
currently exposed by the repository.

Each row is classified as one of:

- exact native support;
- support through a named legalization;
- stage-specific support with its legal stages;
- precise rejection with a diagnostic; or
- not exposed by the current front end.

Every support or rejection row links to at least one fixture. Completion
requires the matrix to contain no unclassified rows.

## Test Design

### Existing-Profile Regression

All existing generic, GLSL, and ARB tests run throughout development. The four
bundled shaders continue to run through `generic`, and their output,
diagnostics, and exit status remain unchanged.

### HLSL Unit Tests

A C90-compatible unit target covers:

- IR construction and both validation passes;
- Cg-to-HLSL type selection;
- identity-aware name allocation and reserved words;
- overload and intrinsic selection;
- semantic aliases and canonical interface keys;
- wrapper marshaling for return, `in`, `out`, and `inout` combinations;
- recursive `c`, `i`, `b`, and `s` layouts;
- explicit and implicit allocation collisions; and
- every exact resource boundary.

### Golden Fixtures

Successful fixtures compare exact normalized HLSL. Vertex coverage includes
`position.cg`, `reflection.cg`, `vertexlight.cg`, and `vertexlight4.cg`.
Focused fixtures cover both stages and every compatibility-matrix category:
types, aggregates, helpers, overloads, control flow, matrix forms, uniforms,
defaults, interface aliases, uncommon qualifiers, intrinsics, samplers,
textures, derivatives, discard, and generated-name collisions.

### Negative Fixtures

Each negative fixture checks nonzero exit status, the exact HLSL diagnostic
code, a stable message fragment, the expected source line, and the absence of
binding metadata or a partial HLSL body. Boundary fixtures cover each full
resource and one unit beyond it.

### Cross-Stage Fixtures

Paired vertex and pixel fixtures compare canonical output and input interface
keys, types, widths, and interpolation semantics. Matching pairs must agree;
mismatches must fail with a stable diagnostic before either result is treated
as link-compatible.

### Optional `fxc` Validation

CMake discovers `fxc.exe` without requiring it. When present, every successful
fixture is compiled with the appropriate `/T vs_3_0` or `/T ps_3_0` target,
`/E main`, strict checking, and warnings as errors. Output bytecode is written
only below the build tree.

Matrix fixtures compile under both `/Zpr` and `/Zpc`; both must succeed and the
checked source contract must remain the same. When `fxc` is absent, golden,
unit, negative, boundary, cross-stage, and existing-profile tests remain
active.

### Optional NVIDIA Cg Oracle

An optional CMake cache path may identify an installed NVIDIA Cg compiler.
Shared-dialect oracle cases compare acceptance, public semantics, constant and
sampler bindings, and default metadata for equivalent HLSL profiles. Generated
temporary names, whitespace, helper factoring, and instruction scheduling are
not compared. The proprietary compiler is never downloaded, bundled, or
required.

## Delivery Milestones

1. Freeze existing-profile regressions. Add HLSL registration, target headers,
   a compatibility-matrix skeleton, an empty IR, and IR assertion tests.
2. Add the type and name systems, stage descriptors, semantic binding, entry
   wrapper, deterministic register allocator, and their unit and negative
   tests.
3. Add declarations, helpers, overloads, expressions, rectangular matrices,
   aggregates, qualifiers, calls, and structured control flow with golden
   fixtures.
4. Complete numeric and geometric intrinsics, pixel-only operations, vertex
   and pixel textures, and every less-common but representable front-end
   construct.
5. Complete diagnostic coverage, exact resource boundaries, cross-stage
   checks, optional `fxc` validation, and optional oracle cases.
6. Audit every compatibility-matrix row, convert the four bundled shaders,
   document command-line usage and binding behavior, and run the full Debug and
   Release suites.

Every milestone preserves a passing repository and ends in focused commits.

## Completion Criteria

The work is complete when:

1. `hlslv` and `hlslf` register and select correctly;
2. both profiles emit deterministic standalone HLSL with public `main` entry
   points and explicit physical bindings;
3. all four bundled vertex shaders and the complete pixel fixture corpus pass
   exact golden comparison;
4. every successful fixture compiles with `fxc` when it is available;
5. every unsupported compatibility-matrix row fails with its intended
   source-located diagnostic and no partial body;
6. all interface and register resource boundaries are tested at and beyond the
   limit;
7. the compatibility matrix has no unclassified feature; and
8. all existing generic, GLSL, and ARB tests pass in Debug and Release builds.

## Non-Goals

- DirectX shader assembly or compiled bytecode output.
- Invoking `fxc`, D3DX, `D3DCompile`, or another compiler from `cgc`.
- Shader Models 1, 2, 4, 5, or 6.
- Geometry, hull, domain, or compute shaders.
- The DirectX effects framework, techniques, passes, or state blocks.
- DirectX 10+ `SV_*` semantics or DXIL compatibility.
- Runtime rendering tests or image comparison in the initial implementation.
- Updating the parser or standard library to a newer Cg release.
- Byte-identical output compared with NVIDIA Cg.
- Predicting optimization-dependent instruction or temporary-register usage
  without invoking an external HLSL compiler.

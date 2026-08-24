# Cg 2.0 Geometry Frontend, Neutral IR, and GLSL 1.50 Design

Date: 2026-08-24

Status: Design approved in dialogue; written review pending

## Purpose

Extend the completed Cg 2.0 language and backend-neutral IR implementation
with the complete Cg 2.0 geometry-program source surface. Geometry programs
will parse, receive profile-independent semantic validation, lower to typed Cg
IR, pass geometry-aware IR verification, print through the normalized
`generic` profile, and generate core GLSL 1.50 through a new `glslg` profile.

Upgrade the existing `glslv` and `glslf` profiles in place from GLSL 1.10 to
core GLSL 1.50. The shared GLSL target IR, lowering, validation, and code
generator gain geometry as a third stage rather than creating a separate
geometry-only backend.

## Relationship to the Cg 2.0 Plan

This project is a separate follow-on to:

- `docs/superpowers/specs/2026-08-23-cg-2-language-ir-design.md`
- `docs/superpowers/plans/2026-08-23-cg-2-language-ir.md`

Implementation begins only after that plan is complete and its release gates
pass. In particular, this design assumes the following foundations already
exist and are authoritative:

- Cg 2.0 is the default language and Cg 1.1 remains explicitly selectable.
- Canonical scalar, vector, matrix, aggregate, sampler, and function types.
- Declarative standard-library and typed intrinsic identities.
- Entry selection and deterministic reachability.
- Structured, typed `CgIRModule` construction.
- Standalone Cg IR verification.
- IR-oriented HAL validation and generation hooks.
- Deterministic normalized output from the Cg 2.0 `generic` profile.
- Transactional target output and the Cg 2.0 conformance manifest.
- Migration of existing GLSL lowering to consume verified Cg IR.

The geometry work extends those facilities. It does not reimplement or fork
them. If a prerequisite is incomplete when execution begins, geometry work
stops until the prerequisite is finished or the base plan is amended.

## Conformance Authority

The compatibility target is the geometry-program surface shipped with NVIDIA
Cg 2.0. Primary references are:

- [NVIDIA Cg 2.0 overview](https://developer.download.nvidia.com/cg/Cg_2.0/2.0.0010/Cg-2.0.pdf)
- [NVIDIA Cg 2.0 Language Specification](https://developer.download.nvidia.com/cg/Cg_2.0/2.0.0010/Cg-2.0_Dec2007_LanguageSpecification.pdf)
- [NVIDIA gp4gp profile reference](https://developer.download.nvidia.com/cg/gp4gp.html)
- [NVIDIA glslg profile reference](https://developer.download.nvidia.com/cg/glslg.html)
- [Khronos OpenGL Shading Language 1.50 Specification](https://registry.khronos.org/OpenGL/specs/gl/GLSLangSpec.1.50.pdf)

The Cg 2.0 material establishes attribute arrays, geometry topology
modifiers, `emitVertex`, `restartStrip`, `flatAttrib`, and the geometry
semantics as the feature surface. The gp4gp reference supplies the detailed
topology, profile-option, and semantic tables. Later Cg features such as
tessellation profiles, geometry invocations, and patch inputs do not enter
this compatibility target. The GLSL 1.50 specification is normative for all
generated vertex, geometry, and fragment source.

Reference-compiler behavior may be recorded as investigation data but cannot
override the published Cg 2.0 contract.

## Scope Decisions

| Area | Decision |
| --- | --- |
| Project form | Separate follow-on to the Cg 2.0 language/IR plan |
| Source coverage | Complete Cg 2.0 geometry-program surface |
| Targets | Verified backend-neutral Cg IR, normalized `generic`, and core GLSL 1.50 |
| IR design | Dedicated geometry metadata, type, and operations in shared Cg IR |
| GLSL backend design | Geometry stage added to the shared GLSL target IR and code generator |
| GLSL profiles | Upgrade `glslv` and `glslf` in place; add `glslg` |
| GLSL version | Core 1.50 only; no version selector and no retained 1.10 mode |
| GLSL capability scope | Existing vertex/fragment subset plus complete geometry lowering |
| Input topology | Point, line, line adjacency, triangle, triangle adjacency |
| Output topology | Points, line strip, triangle strip |
| Maximum vertices | Optional neutral metadata; required by `glslg` through `-po Vertices=N` |
| Geometry options | Source modifiers plus Cg-compatible repeated `-po` options |
| Output-count analysis | No static proof; a specified maximum is metadata, not a compile-time emission bound |
| Legacy language | Geometry syntax rejected in explicit Cg 1.1 mode |
| ARB backends | Controlled geometry-stage rejection after verified Cg IR |

## Goals

1. Accept every valid Cg 2.0 geometry source form in default Cg 2.0 mode.
2. Preserve the distinction between geometry attribute arrays and ordinary Cg
   arrays through the typed AST and Cg IR.
3. Resolve geometry topology, derived input sizes, output bindings, and
   operation placement before Cg IR construction.
4. Represent geometry stage metadata and side effects explicitly in shared Cg
   IR.
5. Verify every geometry-specific IR invariant independently of assertions.
6. Emit deterministic normalized geometry IR through `generic`.
7. Emit valid core GLSL 1.50 for supported vertex, geometry, and fragment
   programs.
8. Link representative generated vertex-geometry-fragment pipelines.
9. Keep language validity separate from backend capabilities and
   hardware limits.
10. Preserve non-GLSL Cg 2.0, Cg 1.1, generic, and ARB behavior.

## Non-Goals

- Emitting gp4gp, NV_gpu_program4, NV_geometry_program4, HLSL, SPIR-V, or GPU
  bytecode.
- Adding an OpenGL, Direct3D, or Cg runtime.
- Configuring transform feedback, framebuffer layers, or runtime primitive
  modes.
- Enforcing device-specific instruction, register, attribute, stream, or
  vertex limits beyond the portable core GLSL 1.50 contract.
- Proving a dynamic upper bound for calls to `emitVertex`.
- Adding CgFX techniques, passes, annotations, or runtime state.
- Adding gp5 geometry invocations, tessellation, patch, mesh, or task stages.
- Preserving GLSL 1.10 output or adding a GLSL version selector.
- Expanding the GLSL backend to every non-geometry Cg feature that GLSL 1.50
  could express.
- Making normalized Cg IR a stable public serialization format.
- Refactoring unrelated compiler stages.

## Architecture

The completed path is:

```text
preprocessor
    -> scanner and parser
    -> typed AST and symbol graph
    -> entry selection
    -> geometry configuration resolution
    -> reachability
    -> reachable geometry semantic validation
    -> Cg IR construction
    -> geometry-aware Cg IR verification
    -> HAL profile validation
        -> generic: normalized Cg IR output
        -> GLSL capability validation
            -> shared GLSL IR lowering
            -> GLSL IR verification
            -> core GLSL 1.50 output
```

Geometry is a program stage in shared Cg IR, not a target-specific sub-IR.
The selected entry determines the stage. A geometry entry carries input and
output topology, a derived input-vertex count, an optional output-vertex
maximum, and geometry interface bindings. The `generic` profile preserves an
unknown output maximum. The `glslg` profile requires a known positive maximum
before GLSL lowering because core GLSL 1.50 requires a `max_vertices` output
layout qualifier.

Non-geometry modules do not carry geometry metadata. Existing vertex and
fragment Cg IR nodes and normalized `generic` output remain byte-for-byte
stable. Existing GLSL output is intentionally upgraded rather than kept text
stable: `glslv`, `glslg`, and `glslf` form a single GLSL 1.50 profile family
using the shared GLSL target IR.

## Program Stage and Topology

Add a program-stage enum shared by entry resolution and Cg IR. It includes at
least unknown, vertex, geometry, and fragment. The unknown value is valid only
before entry resolution and is rejected by the IR verifier.

Add source-level input modifiers:

- `POINT`
- `LINE`
- `LINE_ADJ`
- `TRIANGLE`
- `TRIANGLE_ADJ`

Add source-level output modifiers:

- `POINT_OUT`
- `LINE_OUT`
- `TRIANGLE_OUT`

The modifiers precede a function declaration or definition. A function with
a geometry topology modifier is a program-entry candidate and cannot be
called as an ordinary helper. Repeated identical modifiers are diagnosed so
the source has one canonical spelling; multiple or contradictory input or
output modifiers are errors.

Input topology determines attribute-array length and the default output:

| Input modifier | IR input topology | Input vertices | Default output |
| --- | --- | ---: | --- |
| `POINT` | point | 1 | points |
| `LINE` | line | 2 | line strip |
| `LINE_ADJ` | line adjacency | 4 | line strip |
| `TRIANGLE` | triangle | 3 | triangle strip |
| `TRIANGLE_ADJ` | triangle adjacency | 6 | triangle strip |

The output modifier, when present, replaces only the default output topology.
It does not change input shape.

## Geometry Profile Options

Extend command-line parsing with repeatable `-po option` arguments. This
project recognizes only the Cg 2.0 geometry-domain options:

- `POINT`, `LINE`, `LINE_ADJ`, `TRIANGLE`, `TRIANGLE_ADJ`
- `POINT_OUT`, `LINE_OUT`, `TRIANGLE_OUT`
- `Vertices=N`, where `N` is a positive decimal integer

Source topology modifiers are authoritative. A matching command-line option
is redundant but valid; a contradictory option is a command-line/entry
configuration diagnostic. When source omits a topology, a corresponding
`-po` option supplies it. Input topology must be known after merging source
and options. Output topology defaults from the resolved input when both
source and options omit it.

`Vertices=N` has no source modifier in this compatibility surface. Omission is
valid for neutral IR and `generic`, where it becomes an unknown maximum. The
`glslg` profile requires an explicit known value and diagnoses omission before
GLSL lowering. Duplicate equal values are accepted; duplicate unequal values
are rejected. Integer overflow, zero, negative text, signs, suffixes, and
trailing characters are rejected by command-line parsing. GLSL profile
validation also checks the resolved value against the core GLSL 1.50 geometry
resource contract.

Unrecognized `-po` options remain errors in this project. There is no GLSL
version profile option: all three GLSL profiles always generate core 1.50.

## `AttribArray<T>`

Add a canonical attribute-array type distinct from sized, unsized, packed,
and unpacked ordinary arrays. It contains:

- A canonical element type.
- A geometry-input extent marker.
- The resolved topology-derived extent after entry configuration.

Equivalent attribute-array types are interned through the canonical Cg type
system. They retain their category through Cg IR; lowering must not erase them
to ordinary arrays.

The grammar accepts `AttribArray<T>` with whitespace around angle brackets.
Nested aggregate element types are supported, including structures whose
members carry binding semantics. `void`, function, sampler, and interface
element types are rejected. Attribute arrays are read-only geometry varying
inputs. They are invalid as globals, uniforms, outputs, return types, local
variables, and non-geometry entry parameters.

Passing an attribute array to a reachable helper is allowed when its formal
has an equivalent attribute-array type. Assignment to a whole attribute array
or one of its elements is rejected. Indexing produces an lvalue-disabled
element expression. Ordinary Cg index conversion rules apply, while bounds
remain a profile/runtime concern except for constant indices that the base Cg
2.0 rules already diagnose.

`.length` on an attribute array is a compile-time `int` equal to the resolved
input-vertex count. It is invalid before geometry entry configuration has
resolved the topology; the compiler therefore delays this fold until entry
resolution rather than inventing a placeholder value.

## Geometry Operations

The geometry operation names are reserved intrinsic identities in the Cg 2.0
standard-library catalog, but their call forms receive dedicated parsing and
semantic validation because ordinary fixed-signature overload resolution
cannot model semantic-bearing variadic arguments.

### `emitVertex`

`emitVertex` emits one output vertex. It accepts one or more typed values. An
argument may carry an inline binding semantic, and an aggregate argument may
contribute multiple semantically bound members.

Each argument is resolved to a semantic using this order:

1. Inline argument annotation.
2. A semantic on the directly referenced declaration.
3. A semantic on the selected aggregate member.
4. A semantic inherited by direct indexing of an `AttribArray` parameter.

General arithmetic does not propagate binding semantics. An arbitrary
expression without an inline annotation is therefore an error. This rule
keeps semantic resolution deterministic; a caller may always annotate the
argument explicitly or assign it to a semantically bound local first.

Aggregate expansion is recursive and follows declaration order. Every leaf
must resolve to a semantic. Semantic identity is case-insensitive for conflict
checking while the source spelling is retained for diagnostics. The expanded
bundle must not contain duplicate semantics.

### `flatAttrib`

`flatAttrib` uses the same argument parsing, semantic resolution, aggregate
expansion, type checking, and duplicate detection as `emitVertex`. It records
flat per-primitive attribute state for the provoking-vertex convention. It
does not emit a vertex and does not contribute to an output-vertex count.

### `restartStrip`

`restartStrip` takes no arguments and ends the current output strip. It does
not emit a vertex. It is legal for all output topologies in neutral IR so the
frontend does not impose a target-specific no-op or rejection policy for
point output.

### Placement

The three operations may appear in the geometry entry or a reachable helper.
Their ordinary argument arity and type validity are checked throughout the
translation unit. Their stage legality is checked after entry selection and
reachability:

- A reachable use requires a geometry entry.
- An unreachable geometry-only helper does not make a vertex or fragment
  entry invalid.
- A helper reached from entries of different stages is valid only in the
  geometry compilation and receives a stage diagnostic in the other.

Operations lower as void side effects. The frontend accepts them only as
complete expression statements, including as the controlled statement of an
`if` or loop. They cannot be nested inside arithmetic, constructors,
conditionals, assignments, return values, or call arguments.

## Geometry Semantics

Ordinary varying semantics keep the base Cg 2.0 interface rules. Geometry adds
stage-specific classification:

| Semantic | Direction and shape | Required language type |
| --- | --- | --- |
| `INSTANCEID` | Per-primitive scalar input | `int` |
| `VERTEXID` | Per-vertex attribute-array input | `AttribArray<int>` |
| `PRIMITIVEID` | Per-primitive scalar input or output attribute | `int` |
| `LAYER` | Output attribute | `int` |

Vertex attribute inputs such as `POSITION`, `COLOR`, `TEXCOORDn`, and
`ATTRn` use `AttribArray<T>`. The neutral frontend requires a legal storable
Cg value type but leaves target-specific component widths and resource aliases
to selected-profile validation.

An `emitVertex` output bundle may contain ordinary output semantics plus
`PRIMITIVEID` and `LAYER`. `INSTANCEID` and `VERTEXID` are input-only.
`flatAttrib` accepts output semantics other than `POSITION`; attempting to
mark position as flat is a semantic error.

The frontend does not require every emitted bundle to contain `POSITION`,
because a neutral backend may use geometry output for non-raster purposes.
The GLSL backend maps position when present and does not invent a value when
the source omits it.

## Cg IR Model

Add shared IR enums conceptually equivalent to:

```c
typedef enum CgIRStage_Rec {
    CGIR_STAGE_UNKNOWN = 0,
    CGIR_STAGE_VERTEX,
    CGIR_STAGE_GEOMETRY,
    CGIR_STAGE_FRAGMENT
} CgIRStage;

typedef enum CgIRGeometryInput_Rec {
    CGIR_GEOMETRY_INPUT_POINT,
    CGIR_GEOMETRY_INPUT_LINE,
    CGIR_GEOMETRY_INPUT_LINE_ADJACENCY,
    CGIR_GEOMETRY_INPUT_TRIANGLE,
    CGIR_GEOMETRY_INPUT_TRIANGLE_ADJACENCY
} CgIRGeometryInput;

typedef enum CgIRGeometryOutput_Rec {
    CGIR_GEOMETRY_OUTPUT_POINTS,
    CGIR_GEOMETRY_OUTPUT_LINE_STRIP,
    CGIR_GEOMETRY_OUTPUT_TRIANGLE_STRIP
} CgIRGeometryOutput;
```

A geometry module owns one record conceptually equivalent to:

```c
typedef struct CgIRGeometryInfo_Rec {
    CgIRGeometryInput inputTopology;
    CgIRGeometryOutput outputTopology;
    unsigned int inputVertexCount;
    unsigned int maxOutputVertices;
    int hasMaxOutputVertices;
    SourceLoc inputLoc;
    SourceLoc outputLoc;
    SourceLoc maxVerticesLoc;
} CgIRGeometryInfo;
```

The final implementation uses the base plan's allocation and source-location
conventions, but preserves these fields and meanings.

Add a Cg IR attribute-array type that references its canonical element type
and resolved extent. Add explicit geometry statement nodes:

```text
CgIRGeometryEmit
    outputs[] = { canonical semantic, source semantic, typed value, location }

CgIRGeometryFlat
    outputs[] = { canonical semantic, source semantic, typed value, location }

CgIRGeometryRestart
    location
```

Semantic analysis fully expands and resolves output bundles before lowering.
Cg IR never infers a semantic from an expression and never retains an
unexpanded aggregate geometry argument.

## IR Verification

Extend the standalone verifier with a stable geometry reason category and
checks for these invariants:

- Every module has a resolved non-unknown stage.
- Geometry metadata exists exactly for a geometry module.
- Geometry input and output topology enums are valid.
- `inputVertexCount` exactly matches input topology.
- A known maximum output count is positive.
- Attribute-array types appear only in geometry modules, have legal canonical
  element types, and carry the resolved module extent.
- Geometry operations appear only in functions reachable from the geometry
  entry.
- An emitted or flat bundle is non-empty and each item has a canonical type,
  canonical semantic, source semantic, typed value, and valid source location.
- Canonical semantics are unique within each bundle.
- Geometry-only semantics have valid direction, shape, and type.
- `flatAttrib` does not contain `POSITION`.
- `restartStrip` has no operands.
- Every geometry operation is a void side-effect statement.
- Non-geometry modules contain no geometry metadata, attribute arrays, or
  geometry operations.

Verifier failure is an internal compiler defect. User source errors must have
been rejected earlier by parser or semantic diagnostics.

The verifier deliberately does not count operations across control flow,
prove loop bounds, or compare possible emissions with
`maxOutputVertices`. NVIDIA's documented behavior permits excess output to be
discarded. GLSL profile validation checks declared resource metadata but does
not attempt a dynamic emission proof.

## Normalized Generic Output

The normalized printer emits a geometry header before declarations. Its
logical content is:

```text
module main stage geometry
geometry input triangle vertices 3
geometry output triangle_strip max_vertices unknown
```

A specified maximum prints as a decimal integer. Attribute-array types print
with canonical element type and resolved extent. Geometry operations print
their resolved items in source/declaration order, for example:

```text
emit_vertex {
  POSITION = %position
  TEXCOORD0 = %uv
}
restart_strip
```

The exact punctuation and identifier syntax follow the normalized printer
established by the base Cg 2.0 plan. Geometry output must contain no pointer
values, allocation-order identifiers, unresolved semantics, or
traversal-dependent ordering.

Non-geometry normalized output does not gain a new header or otherwise change
bytes as part of this project.

## Core GLSL 1.50 Profile Family

`glslv`, `glslg`, and `glslf` generate one core GLSL dialect and share target
IR, validation, interface naming, and code generation. Every generated shader
begins with exactly:

```glsl
#version 150
```

The compiler emits no geometry extension directive and provides no fallback
to compatibility syntax or GLSL 1.10. Each profile accepts only its matching
stage: `glslv` accepts vertex modules, `glslg` accepts geometry modules, and
`glslf` accepts fragment modules.

### Common GLSL 1.50 modernization

The in-place vertex and fragment upgrade applies to the already supported
GLSL capability subset:

- Vertex attributes and interstage varyings use stage-correct `in` and `out`
  declarations instead of `attribute` and `varying`.
- Fragment color results use generated, explicitly declared output variables
  instead of `gl_FragColor`.
- Supported legacy texture calls lower to the appropriate core `texture`,
  `textureProj`, or `textureLod` operation.
- Compatibility-only built-ins previously used as generic Cg semantic
  storage lower to generated user interface variables.
- Aggregate interfaces flatten recursively to semantic leaves in declaration
  order. The same deterministic semantic-to-name mapping is used by all
  stages so independently generated pipeline stages link.

The upgrade does not broaden the existing vertex/fragment capability subset
except where geometry linkage requires a core 1.50 spelling or interface.
Unsupported Cg types, intrinsics, semantics, or control-flow forms continue
to receive profile diagnostics.

### Geometry layouts and interfaces

A generated geometry shader declares its resolved topology and required
maximum before interface declarations. For example:

```glsl
#version 150
layout(triangles_adjacency) in;
layout(triangle_strip, max_vertices = 12) out;
```

Topology mapping is exact:

| Cg IR topology | GLSL 1.50 layout token |
| --- | --- |
| point input | `points` |
| line input | `lines` |
| line-adjacency input | `lines_adjacency` |
| triangle input | `triangles` |
| triangle-adjacency input | `triangles_adjacency` |
| point output | `points` |
| line-strip output | `line_strip` |
| triangle-strip output | `triangle_strip` |

Per-vertex geometry inputs become GLSL input arrays with the topology-derived
length. Ordinary semantic leaves use generated user variables; required GLSL
per-vertex built-ins use `gl_in[index]`. Scalar per-primitive inputs remain
scalar. Geometry outputs are generated user variables or stage built-ins as
specified below. Interface flattening and deterministic semantic names are
shared with `glslv` and `glslf` so matching semantic leaves have link-compatible
types, interpolation, and names.

### Geometry operations

`emitVertex` lowers to assignments for its resolved output bundle followed by
`EmitVertex()`. `restartStrip` lowers to `EndPrimitive()`.

`flatAttrib` updates generated shadow state for its resolved semantic leaves;
it does not emit. Before every later `EmitVertex()`, lowering replays the
currently defined flat shadow values into their GLSL output variables. This
replay is required because GLSL geometry outputs become undefined after a
vertex is emitted. Control flow preserves ordinary Cg assignment semantics:
only a path that executes `flatAttrib` updates the corresponding shadow state.

The GLSL target IR represents emit and restart as explicit side-effect nodes.
It also records flat-shadow declarations and the replay set attached to each
emit. Code generation never reconstructs geometry operations by recognizing
ordinary calls or assignments.

### Geometry semantic mapping

The focused core 1.50 mapping is:

| Cg geometry semantic | GLSL 1.50 representation |
| --- | --- |
| scalar input `INSTANCEID` | `gl_PrimitiveIDIn` |
| scalar input `PRIMITIVEID` | `gl_PrimitiveIDIn` |
| per-vertex input `VERTEXID` | generated integer varying array |
| per-vertex input `POSITION` | `gl_in[index].gl_Position` |
| output `POSITION` | `gl_Position` |
| output `PRIMITIVEID` | `gl_PrimitiveID` |
| output `LAYER` | `gl_Layer` |
| ordinary input/output semantic | deterministic user interface leaf |

For `glslv`, a vertex input with `VERTEXID` maps to `gl_VertexID`. When a
geometry entry consumes `VERTEXID`, the matching vertex program exports that
value through the generated integer varying and the geometry program consumes
its array form. This explicit bridge avoids treating `gl_VertexID` as a
geometry-stage built-in.

### GLSL target IR and profile verification

Extend the shared GLSL target IR with:

- A geometry value in `GlslStage`.
- Resolved geometry input/output topology and maximum-output metadata on
  `GlslModule`.
- Arrayed geometry inputs and built-in interface identities.
- Explicit emit and restart statements.
- Flat-shadow locals and per-emit replay metadata.

`GlslLowerProgram` continues the base plan's migration to verified Cg IR and
accepts the selected `CgIRModule` as its source. It first validates the chosen
profile's focused capability set, then performs stage-aware lowering. A
standalone GLSL IR verifier rejects stage/profile mismatches, missing or
invalid geometry layouts, a missing `glslg` maximum, illegal built-in
directions or types, inconsistent interface leaves, malformed flat replay,
and geometry operations outside a geometry module.

GLSL resource validation uses the mandated minimum geometry limits in the
OpenGL/GLSL 1.50 contract: 64 input components, 128 output components per
vertex, 256 output vertices, 1,024 total output components, 1,024 uniform
components, and 16 geometry texture-image units. The focused type/interface
lowering defines component accounting once and applies it consistently to
flattened user leaves and relevant built-ins. The declared maximum multiplied
by its counted per-vertex output components must not exceed 1,024. Values at
each boundary are accepted; values beyond these portable minima are rejected
deterministically rather than depending on the machine running the compiler.
This is target capability validation, not a neutral Cg language restriction.

No partial shader is published after GLSL capability, lowering, verification,
or generation failure.

## Components and Responsibilities

Create `cg_geometry.h` and `cg_geometry.c`. This module owns:

- Source/profile-option topology merging.
- Topology-to-input-count and default-output mappings.
- Geometry entry validation.
- Attribute-array placement and stage rules.
- Geometry semantic classification.
- Output-bundle resolution and recursive aggregate expansion.
- Reachable geometry-operation validation.

Extend existing or base-plan files only at their established seams:

- `cgcmain.c`, compiler options: collect repeatable `-po` values.
- `atom.c`, `scanner.c`, `parser.y`, generated `parser.c` and `parser.h`:
  recognize geometry modifiers, `AttribArray<T>`, and annotated arguments.
- Canonical type files: intern and query attribute-array types.
- Symbol/AST files: retain entry modifiers and semantic-bearing geometry
  arguments until semantic resolution.
- `cg_stdlib.def`: assign stable identities to geometry operations without
  pretending they have ordinary fixed overloads.
- `cg_ir.h`, `cg_ir.c`: stage, topology, metadata, type, and node ownership.
- `cg_ir_lower.c`: lower resolved geometry constructs one-for-one.
- `cg_ir_verify.c`: enforce geometry invariants.
- `cg_ir_print.c`: deterministic normalized geometry output.
- `hal.h` and profile validators: recognize geometry as a stage; route it to
  `glslg` or reject it cleanly in ARB backends.
- `glsl_ir.h`, `glsl_ir.c`: add the geometry stage, layouts, arrayed
  interfaces, explicit operations, flat state, and ownership.
- `glsl_lower.c`: lower the focused vertex, geometry, and fragment subset from
  verified Cg IR into shared GLSL IR.
- `glsl_codegen.c`: emit core GLSL 1.50 syntax for all three stages.
- `glsl_hal.h`, `glsl_hal.c`: share 1.50 capability validation and common
  profile behavior.
- `glslg_hal.c`: register the geometry profile, its connector metadata, stage
  validation, and `Vertices=N` requirement.
- CMake and CTest files: build the focused geometry module and tests.

`cg_geometry.c` is the only owner of geometry language policy. Parser actions
construct syntax records, semantic integration calls the module, Cg IR stores
resolved facts, and HALs consume verified facts. GLSL profile code owns only
target capability, resource, interface, and emission policy; it does not
reinterpret source geometry syntax.

## Diagnostics and Failure Handling

Diagnostics remain assigned by responsible layer:

- Command line: malformed, unknown, duplicate-conflicting, or overflowing
  `-po` values.
- Parser: malformed topology modifiers, template syntax, or annotated
  geometry arguments.
- Language semantics: conflicting declarations, missing input topology,
  illegal attribute-array placement, invalid element types, unresolved or
  duplicate output semantics, invalid semantic direction/type, illegal
  operation expression context, or reachable stage mismatch.
- IR verifier: a broken compiler invariant after successful semantic analysis.
- Profile validation: stage/profile mismatch; `glslg` without `Vertices=N`;
  a valid feature outside the focused GLSL capability set; a portable GLSL
  resource-limit violation; or a geometry module supplied to an ARB backend.
- GLSL IR verifier: malformed target layouts, interfaces, built-ins, geometry
  operations, or flat replay after successful target lowering.

New user diagnostics use a contiguous geometry-specific block in `errors.h`.
The primary location identifies the use or conflicting setting. Notes identify
the first modifier, first semantic in a duplicate pair, or selected entry and
reachability edge responsible for a stage mismatch.

Poison types and the base diagnostic-suppression rules prevent cascades.
Transactional output remains in force: no normalized or target output replaces
an existing file unless parsing, semantic analysis, lowering, verification,
profile validation, and any target-IR verification all succeed. `-nocode`
still performs geometry semantic analysis, Cg IR construction, Cg IR
verification, and selected-profile validation without publishing code.

## Testing Strategy

Geometry fixtures live under `tests/cg20/geometry/` and use the Cg 2.0 harness
and `tests/cg20/conformance.csv` from the base plan.

### Positive fixtures

Add focused fixtures for:

- Every input topology and its exact derived attribute-array length.
- Every explicit output topology and every input-derived default.
- Source modifiers, command-line topology options, matching combinations, and
  `Vertices=N`.
- Scalar and aggregate attribute-array elements.
- `.length` constant folding for lengths 1, 2, 3, 4, and 6.
- Inline, declaration-derived, member-derived, and attribute-derived output
  semantics.
- Recursive aggregate bundle flattening in declaration order.
- `emitVertex`, `flatAttrib`, and `restartStrip` in an entry.
- Each operation in a reachable helper.
- `INSTANCEID`, `VERTEXID`, input and output `PRIMITIVEID`, and `LAYER`.
- Pass-through, amplification, strip restart, flat-attribute, adjacency, and
  layered-output normalized IR goldens.
- An unreachable geometry-only helper coexisting with a vertex or fragment
  entry.
- Core GLSL 1.50 geometry goldens for every input and output topology.
- GLSL pass-through, amplification, repeated restart, conditional emit, and
  conditional flat-state replay.
- GLSL geometry interfaces containing scalar, vector, matrix, nested
  aggregate, ordinary semantic, and geometry built-in leaves.
- Supported uniforms, reachable helpers, structured control flow, and the
  existing supported texture subset in geometry programs.
- A linked vertex-geometry-fragment pipeline carrying `VERTEXID`, ordinary
  varyings, `PRIMITIVEID`, and `LAYER` as applicable.
- Fixtures exactly at each portable geometry input, output, vertex-count,
  total-output, uniform, and texture-unit resource boundary.

### Negative fixtures

Add exact diagnostic fixtures for:

- Repeated or conflicting source topology modifiers.
- Missing input topology after source/option merging.
- Source and command-line topology conflicts.
- Zero, negative, overflowing, malformed, or conflicting `Vertices` values.
- A `glslg` compilation with no `Vertices=N` option.
- GLSL geometry resources one beyond each portable input-component,
  output-component, output-vertex, total-output-component, uniform-component,
  and texture-unit boundary.
- Attribute arrays as globals, uniforms, outputs, returns, locals, writable
  values, or non-geometry entry parameters.
- Illegal attribute-array element types.
- Unresolved output semantics and duplicate case-insensitive semantics.
- Invalid direction or type for each geometry-only semantic.
- `POSITION` in `flatAttrib`.
- Arguments to `restartStrip`.
- Empty `emitVertex` and `flatAttrib` bundles.
- Geometry operations embedded in non-statement expression contexts.
- A reachable geometry operation under vertex or fragment entry selection.
- Calls to a topology-qualified entry as a helper.
- Every new source form under explicit Cg 1.1 mode.
- Stage/profile mismatches for `glslv`, `glslg`, and `glslf`.
- Geometry modules sent to every ARB profile.
- Representative cross-stage type, interpolation, and semantic link
  mismatches.

### Unit and invariant tests

Add a focused `cg_geometry_unit` executable for topology merging, default
selection, vertex-count parsing, semantic classification, and recursive bundle
expansion. Extend `cg_ir_unit` with valid geometry construction plus one
malformed module for every verifier invariant.

Extend the shared GLSL IR unit tests with valid vertex, geometry, and fragment
modules plus malformed geometry layout, built-in, interface, operation, and
flat-replay cases. Add focused lowering assertions that `emitVertex`,
`flatAttrib`, and `restartStrip` produce explicit target nodes and preserve
control-flow placement.

Unit tests assert stable enums and structured reason codes rather than parsing
human-readable diagnostics. Assertion-active seams remain enabled in the unit
binaries.

### Regression and reproducibility

Each milestone runs:

- Focused geometry unit and fixture tests.
- The complete Cg 2.0 suite.
- Explicit Cg 1.1 compatibility tests.
- Existing normalized generic goldens.
- Intentionally regenerated `glslv` and `glslf` goldens plus new `glslg`
  goldens, all containing exactly `#version 150`.
- GLSL syntax validation for every generated shader and link validation for
  representative vertex-geometry-fragment pipelines.
- Checks that generated GLSL contains no `attribute`, `varying`,
  `gl_FragColor`, compatibility-only interface built-ins, legacy texture
  function spelling, unnecessary extension directives, or accidental second
  version directive.
- GLSL portable-minimum boundary tests and ARB diagnostic tests.
- Parser and standard-library regeneration reproducibility tests.
- Release and assertion-enabled builds.

Parser regeneration uses the version pinned by the base Cg 2.0 plan. The
grammar conflict count must not increase without an explicit explanation and a
test that distinguishes the ambiguous forms.

The test harness invokes `glslangValidator` for all generated vertex,
geometry, and fragment shaders. It uses stage-correct invocation for syntax
tests, links representative complete pipelines, and includes negative link
fixtures proving that mismatched generated interfaces are detected. Golden
updates are reviewed for semantic equivalence to the old supported GLSL
programs; preserving GLSL 1.10 text is not a goal.

## Milestone Boundaries

The implementation plan should divide the work into these independently
testable milestones:

1. Verify base-plan prerequisites and extend the conformance manifest/harness.
2. Add repeatable geometry profile options and topology resolution.
3. Parse geometry modifiers, `AttribArray<T>`, and annotated arguments.
4. Add canonical attribute-array types and topology-derived `.length`.
5. Resolve geometry semantics and validate geometry operation placement.
6. Extend Cg IR ownership and builders with geometry metadata, types, and
   operations.
7. Lower geometry programs and verify every new invariant.
8. Print normalized geometry IR and add compiler-level goldens.
9. Upgrade shared GLSL IR, validation, and code generation for the existing
   vertex/fragment subset to core GLSL 1.50; intentionally regenerate and
   validate its goldens.
10. Add geometry layouts, interfaces, operations, flat-state lowering, and
    core 1.50 generation to shared GLSL IR and code generation.
11. Register `glslg`, enforce its stage, maximum-output, capability, and
    portable resource rules, then compile and link representative pipelines.
12. Add controlled geometry-stage rejection to ARB profiles and close the
    conformance manifest with full release and assertion-enabled qualification.

Every milestone starts with a failing focused test, ends with the full CTest
suite passing, and is committed independently.

## Completion Criteria

The project is complete only when:

1. Every Cg 2.0 geometry conformance row has a passing positive or negative
   test and no unresolved status.
2. Every valid geometry fixture reaches verified Cg IR through `generic`.
3. Every input topology produces its exact attribute-array length.
4. Every geometry operation reaches one explicit verified IR node.
5. Normalized output is deterministic across repeated builds and runs.
6. Existing non-geometry normalized output remains byte-for-byte unchanged.
7. `glslv`, `glslg`, and `glslf` each emit syntactically valid core GLSL 1.50
   for their complete focused capability set.
8. Every generated shader validates independently, representative generated
   vertex-geometry-fragment pipelines link, and generated source contains no
   prohibited compatibility-only construct.
9. Geometry semantics and operations have the specified GLSL mappings,
   including correct flat-state replay across every emitted vertex.
10. Portable GLSL geometry resource boundaries are tested at and beyond their
    mandated minima, and `glslg` rejects a missing `Vertices=N` without partial
    output.
11. ARB profiles reject reachable geometry modules with a controlled profile
    diagnostic and no partial output.
12. Explicit Cg 1.1 mode rejects geometry syntax without changing other legacy
   behavior.
13. Generated parser and standard-library sources reproduce exactly.
14. The complete release and assertion-enabled test suites pass.

## Risks and Mitigations

### Grammar ambiguity around angle brackets and annotations

Risk: `AttribArray<T>` can conflict with relational operators, while semantic
annotations inside argument lists add a colon form not accepted by ordinary
calls.

Mitigation: enter the attribute-array production only after the reserved
`AttribArray` token, keep annotated arguments as an explicit grammar branch,
and add parser fixtures that distinguish templates, comparisons, conditional
expressions, and ordinary calls.

### Premature loss of binding semantics

Risk: ordinary AST rewrites may erase the declaration/member relationship
needed to infer output semantics.

Mitigation: resolve and freeze geometry output bundles before transformations
that flatten expressions or aggregates. Cg IR accepts only fully resolved
bundles.

### Stage checks leaking into whole-translation-unit semantics

Risk: an unreachable geometry helper could incorrectly invalidate a selected
vertex or fragment program.

Mitigation: check intrinsic syntax and types globally, but perform stage
legality only against the selected entry's reachability graph.

### Attribute arrays collapsing into ordinary arrays

Risk: existing array helpers may accept writes, lose topology-derived extent,
or apply dynamic-array rules.

Mitigation: use a distinct canonical type category, explicit read-only rules,
and verifier checks tied to module geometry metadata.

### Backend policy entering neutral semantics

Risk: gp4gp register aliases, maximum vertices, position requirements, or
GLSL output limits could be enforced as language rules.

Mitigation: enforce only source-contract direction and type invariants in the
frontend. Preserve an unknown maximum in neutral IR and enforce the core GLSL
1.50 layout and portable resource contract only after `glslg` is selected.

### GLSL 1.10 migration churn

Risk: upgrading `glslv` and `glslf` in place can obscure geometry changes under
large golden diffs or accidentally retain compatibility-only constructs.

Mitigation: complete the common 1.50 target-IR upgrade as its own milestone,
regenerate every GLSL golden intentionally, run source-token exclusion checks,
and validate every shader before geometry lowering is added.

### Flat state across emission and control flow

Risk: a direct translation of `flatAttrib` to an output assignment loses state
after `EmitVertex()`, or replays a value on a path where it was never set.

Mitigation: represent flat shadow state explicitly in GLSL IR, preserve the
source control flow that mutates it, attach a verified replay set to each emit,
and test repeated and conditional updates around multiple emissions.

### Cross-stage interface disagreement

Risk: independent vertex, geometry, and fragment compilation can choose
different names, flattening orders, types, or interpolation for the same Cg
semantic.

Mitigation: centralize semantic-leaf flattening and deterministic interface
naming in the shared GLSL path, unit-test the mapping, and link representative
generated pipelines in the conformance suite.

### Geometry resource accounting

Risk: output-vertex and total-component validation can be platform-dependent
or count flattened aggregates inconsistently.

Mitigation: validate against the mandated core 1.50 portable minima, count the
same verified semantic leaves used by lowering, and include exact-boundary and
one-past-boundary fixtures.

### Capability creep

Risk: the GLSL 1.50 upgrade becomes an open-ended attempt to support every Cg
construct expressible by modern GLSL.

Mitigation: constrain common modernization to the existing supported
vertex/fragment subset and additions required for complete geometry lowering.
Every unrelated capability remains a controlled profile diagnostic and needs
a separate approved follow-on.

## Follow-On Work

After this design's completion gates pass, separate projects may add an
OpenGL runtime and rendering smoke tests, target additional GLSL versions or
extensions, broaden non-geometry GLSL capability, or consume verified geometry
IR in other backends such as SPIR-V or GPU assembly. Each follow-on must define
its own capability, resource, validation, and runtime contract without
changing the neutral geometry language rules established here.

# Cg 2.0 Geometry Frontend and Neutral IR Design

Date: 2026-08-24

Status: Design approved in dialogue; written review pending

## Purpose

Extend the completed Cg 2.0 language and backend-neutral IR implementation
with the complete Cg 2.0 geometry-program source surface. Geometry programs
will parse, receive profile-independent semantic validation, lower to typed Cg
IR, pass geometry-aware IR verification, and print through the normalized
`generic` profile.

This is a frontend and neutral-IR project. It deliberately stops before
executable geometry-shader code generation.

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

The geometry work extends those facilities. It does not reimplement or fork
them. If a prerequisite is incomplete when execution begins, geometry work
stops until the prerequisite is finished or the base plan is amended.

## Conformance Authority

The compatibility target is the geometry-program surface shipped with NVIDIA
Cg 2.0. Primary references are:

- [NVIDIA Cg 2.0 overview](https://developer.download.nvidia.com/cg/Cg_2.0/2.0.0010/Cg-2.0.pdf)
- [NVIDIA Cg 2.0 Language Specification](https://developer.download.nvidia.com/cg/Cg_2.0/2.0.0010/Cg-2.0_Dec2007_LanguageSpecification.pdf)
- [NVIDIA gp4gp profile reference](https://developer.download.nvidia.com/cg/gp4gp.html)

The Cg 2.0 material establishes attribute arrays, geometry topology
modifiers, `emitVertex`, `restartStrip`, `flatAttrib`, and the geometry
semantics as the feature surface. The gp4gp reference supplies the detailed
topology, profile-option, and semantic tables. Later Cg features such as
tessellation profiles, geometry invocations, and patch inputs do not enter
this compatibility target.

Reference-compiler behavior may be recorded as investigation data but cannot
override the published Cg 2.0 contract.

## Scope Decisions

| Area | Decision |
| --- | --- |
| Project form | Separate follow-on to the Cg 2.0 language/IR plan |
| Source coverage | Complete Cg 2.0 geometry-program surface |
| First and only target | Verified backend-neutral Cg IR and normalized `generic` output |
| IR design | Dedicated geometry metadata, type, and operations in shared Cg IR |
| Executable backend | Excluded |
| Input topology | Point, line, line adjacency, triangle, triangle adjacency |
| Output topology | Points, line strip, triangle strip |
| Maximum vertices | Optional neutral metadata supplied by `-po Vertices=N` |
| Geometry options | Source modifiers plus Cg-compatible repeated `-po` options |
| Output-count analysis | No static proof; a specified maximum is metadata, not a compile-time emission bound |
| Legacy language | Geometry syntax rejected in explicit Cg 1.1 mode |
| Existing backends | Controlled profile rejection after verified Cg IR |

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
7. Keep language validity separate from future backend capabilities and
   hardware limits.
8. Preserve all non-geometry Cg 2.0, Cg 1.1, GLSL, and ARB behavior.

## Non-Goals

- Emitting gp4gp, NV_gpu_program4, NV_geometry_program4, GLSL geometry source,
  HLSL, SPIR-V, or GPU bytecode.
- Adding an OpenGL, Direct3D, or Cg runtime.
- Configuring transform feedback, framebuffer layers, or runtime primitive
  modes.
- Enforcing hardware instruction, register, attribute, stream, or vertex
  limits.
- Proving a dynamic upper bound for calls to `emitVertex`.
- Adding CgFX techniques, passes, annotations, or runtime state.
- Adding gp5 geometry invocations, tessellation, patch, mesh, or task stages.
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
    -> normalized generic output
```

Geometry is a program stage in shared Cg IR, not a target-specific sub-IR.
The selected entry determines the stage. A geometry entry carries input and
output topology, a derived input-vertex count, an optional output-vertex
maximum, and geometry interface bindings.

Non-geometry modules do not carry geometry metadata. Existing vertex and
fragment IR nodes and normalized output remain byte-for-byte stable.

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
valid and becomes an unknown maximum in neutral IR. Duplicate equal values
are accepted; duplicate unequal values are rejected. Integer overflow, zero,
negative text, signs, suffixes, and trailing characters are rejected by
command-line parsing.

Unrecognized `-po` options remain errors in this project. A future production
profile may extend option recognition without changing geometry IR.

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
to future profile validation.

An `emitVertex` output bundle may contain ordinary output semantics plus
`PRIMITIVEID` and `LAYER`. `INSTANCEID` and `VERTEXID` are input-only.
`flatAttrib` accepts output semantics other than `POSITION`; attempting to
mark position as flat is a semantic error.

The frontend does not require every emitted bundle to contain `POSITION`,
because a neutral backend may use geometry output for non-raster purposes.
A future raster backend may require position during profile validation.

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
discarded, and target limits belong to a production backend.

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
- `hal.h` and current validators: recognize geometry as a stage and reject it
  cleanly in backends that do not support it.
- CMake and CTest files: build the focused geometry module and tests.

`cg_geometry.c` is the only owner of geometry language policy. Parser actions
construct syntax records, semantic integration calls the module, Cg IR stores
resolved facts, and HALs consume verified facts.

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
- Profile validation: a valid geometry module supplied to a backend without
  geometry capability.

New user diagnostics use a contiguous geometry-specific block in `errors.h`.
The primary location identifies the use or conflicting setting. Notes identify
the first modifier, first semantic in a duplicate pair, or selected entry and
reachability edge responsible for a stage mismatch.

Poison types and the base diagnostic-suppression rules prevent cascades.
Transactional output remains in force: no normalized or target output replaces
an existing file unless parsing, semantic analysis, lowering, verification,
and profile validation all succeed. `-nocode` still performs geometry semantic
analysis, IR construction, verification, and profile validation.

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

### Negative fixtures

Add exact diagnostic fixtures for:

- Repeated or conflicting source topology modifiers.
- Missing input topology after source/option merging.
- Source and command-line topology conflicts.
- Zero, negative, overflowing, malformed, or conflicting `Vertices` values.
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

### Unit and invariant tests

Add a focused `cg_geometry_unit` executable for topology merging, default
selection, vertex-count parsing, semantic classification, and recursive bundle
expansion. Extend `cg_ir_unit` with valid geometry construction plus one
malformed module for every verifier invariant.

Unit tests assert stable enums and structured reason codes rather than parsing
human-readable diagnostics. Assertion-active seams remain enabled in the unit
binaries.

### Regression and reproducibility

Each milestone runs:

- Focused geometry unit and fixture tests.
- The complete Cg 2.0 suite.
- Explicit Cg 1.1 compatibility tests.
- Existing normalized generic goldens.
- GLSL and ARB golden, validation, diagnostic, and limit tests.
- Parser and standard-library regeneration reproducibility tests.
- Release and assertion-enabled builds.

Parser regeneration uses the version pinned by the base Cg 2.0 plan. The
grammar conflict count must not increase without an explicit explanation and a
test that distinguishes the ambiguous forms.

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
9. Add controlled unsupported-stage diagnostics to existing production
   backends and close the conformance manifest.

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
7. GLSL 1.10 and ARB profiles reject reachable geometry modules with a
   controlled profile diagnostic and no partial output.
8. Explicit Cg 1.1 mode rejects geometry syntax without changing other legacy
   behavior.
9. Generated parser and standard-library sources reproduce exactly.
10. The complete release and assertion-enabled test suites pass.

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
hardware output limits could be enforced as language rules.

Mitigation: enforce only source-contract direction and type invariants in the
frontend. Leave raster requirements, aliases, limits, and emission behavior to
future profile validation.

### Scope expansion into a production backend

Risk: geometry IR work may attract opportunistic assembly or GLSL emission.

Mitigation: production output files, runtime tests, extension loading, and
hardware limits are explicit non-goals. Any executable backend requires a new
approved design and implementation plan.

## Follow-On Work

After this design's completion gates pass, a separate project may consume the
verified geometry IR in a production backend. That project must select a
concrete target and define its semantic mappings, output-position rules,
resource limits, texture/intrinsic support, lowering strategy, validation
tools, and runtime smoke tests. This design makes no target selection.

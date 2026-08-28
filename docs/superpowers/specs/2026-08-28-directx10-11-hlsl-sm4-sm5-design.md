# DirectX 10/11 HLSL Shader Model 4/5 Profiles Design

## Objective

Add six source-emitting HLSL profiles to the Cg compiler after the existing
DirectX 9 Shader Model 3 implementation plan is complete:

| Cg profile | Pipeline stage | HLSL target |
| --- | --- | --- |
| `hlslv40` | vertex | `vs_4_0` |
| `hlslg40` | geometry | `gs_4_0` |
| `hlslf40` | pixel | `ps_4_0` |
| `hlslv50` | vertex | `vs_5_0` |
| `hlslg50` | geometry | `gs_5_0` |
| `hlslf50` | pixel | `ps_5_0` |

The profiles cover the complete current Cg 2.0 surface that the selected
stage and shader model can represent faithfully. Supported programs produce
deterministic standalone HLSL. Unsupported programs receive one precise,
source-located diagnostic before any target body or binding metadata is
published.

The compiler emits source only. It does not load a DirectX compiler, emit DXBC,
create runtime objects, or add a required DirectX SDK dependency. An installed
`fxc.exe` is an optional external validator and is required only in a dedicated
qualification environment.

This design is a strict follow-on to
`docs/superpowers/plans/2026-08-24-directx9-hlsl-sm3-profiles.md`. Tasks 2
through 12 of that plan must be complete and its full test suite must pass
before implementation of this design begins.

## Authoritative Contracts

The target behavior follows Microsoft's documentation for:

- [Shader Model 4](https://learn.microsoft.com/en-us/windows/win32/direct3dhlsl/dx-graphics-hlsl-sm4)
- [shader models and profiles](https://learn.microsoft.com/en-us/windows/win32/direct3dhlsl/dx-graphics-hlsl-models)
- [the common-shader core](https://learn.microsoft.com/en-us/windows/win32/direct3dhlsl/dx-graphics-hlsl-common-core)
- [DirectX 10 and newer semantics](https://learn.microsoft.com/en-us/windows/win32/direct3dhlsl/dx-graphics-hlsl-semantics)
- [geometry stream-output objects](https://learn.microsoft.com/en-us/windows/win32/direct3dhlsl/dx-graphics-hlsl-so-type)
- [the Direct3D 11 geometry stage](https://learn.microsoft.com/en-us/windows/win32/direct3d11/geometry-shader-stage)
- [Direct3D 10 resource limits](https://learn.microsoft.com/en-us/windows/win32/direct3d10/d3d10-graphics-programming-guide-resources-limits)
- [Direct3D 11 resource limits](https://learn.microsoft.com/en-us/windows/win32/direct3d11/overviews-direct3d-11-resources-limits)
- [Shader Model 4 constant buffers](https://learn.microsoft.com/en-us/windows/win32/direct3dhlsl/dx-graphics-hlsl-constants)

The repository's verified Cg IR and Cg geometry analysis remain authoritative
for Cg source meaning. Target policy must not reinterpret geometry topology,
attribute arrays, or geometry operations from the original AST.

## Scope

### Included

- Vertex, geometry, and pixel output for Shader Models 4.0 and 5.0.
- Every current Cg 2.0 type, expression, statement, helper, aggregate,
  intrinsic, binding, and texture form that has a faithful representation in
  the selected target.
- The complete existing Cg geometry contract: five input topologies, three
  output topologies, `Vertices=N`, `AttribArray<T>`, `emitVertex`,
  `flatAttrib`, `restartStrip`, adjacency inputs, primitive IDs, vertex IDs,
  and layered output.
- Modern system-value semantics and deterministic user semantics.
- Modern texture-object and sampler-state resource pairs.
- Shader-model- and stage-specific validation, resource accounting,
  diagnostics, exact goldens, cross-stage checks, and optional `fxc`
  validation.

### Excluded

- DirectX effects and bytecode generation.
- Shader Models 4.1, 5.1, and 6.x.
- Hull, domain, and compute profiles.
- Tessellation, patches, geometry shader instancing, and multiple geometry
  output streams.
- UAVs, structured buffers, byte-address buffers, append/consume buffers, and
  other Shader Model 5 resource types not exposed by the current Cg frontend.
- New Cg language syntax or library APIs solely to expose DirectX 11 features.
- Runtime input-layout, resource-view, sampler-state, stream-output, or shader
  object creation.

## Compatibility Prerequisite and Stable IDs

The independently developed geometry and initial HLSL branches currently
collide: `glslg` and `hlslv` both use profile ID 14, while their connector IDs
also overlap. The first implementation task must repair the HLSL allocation
without changing existing GLSL IDs or public profile names.

Profile IDs are fixed as follows:

| Profile | ID |
| --- | ---: |
| `glslg` | 14 |
| `hlslv` | 15 |
| `hlslf` | 16 |
| `hlslv40` | 17 |
| `hlslg40` | 18 |
| `hlslf40` | 19 |
| `hlslv50` | 20 |
| `hlslg50` | 21 |
| `hlslf50` | 22 |

Connector IDs continue after `glslg`'s IDs 18 and 19:

| Profile | Input CID | Output CID |
| --- | ---: | ---: |
| `hlslv` | 20 | 21 |
| `hlslf` | 22 | 23 |
| `hlslv40` | 24 | 25 |
| `hlslg40` | 26 | 27 |
| `hlslf40` | 28 | 29 |
| `hlslv50` | 30 | 31 |
| `hlslg50` | 32 | 33 |
| `hlslf50` | 34 | 35 |

These integers are internal stable identities, not aliases for external D3D
enumerations. Unit tests pin uniqueness and the complete table so a later
profile cannot silently reuse one.

All HLSL profiles register an exact Cg pipeline stage. Their wildcard identity
is `vs`, `gs`, or `ps` with the same specificity used by the GLSL profiles.
The SM3 profiles cease being stage-neutral as part of the prerequisite repair.

## Architecture

The completed SM3 backend becomes a shader-model-aware shared HLSL backend:

```text
verified Cg IR
    -> shared typed HLSL IR
    -> selected stage/model policy
    -> public wrapper and interface construction
    -> model-aware legalization
    -> modern semantic and resource binding
    -> whole-module verification
    -> binding metadata and deterministic HLSL source
```

The common pipeline remains independent of GLSL IR. Reusing GLSL IR would
couple HLSL behavior to GLSL interface, sampler, matrix, and resource choices.
A separate modern-HLSL IR would duplicate the majority of the completed SM3
backend. Instead, the shared HLSL IR represents language constructs once and
uses explicit policy for the genuine DX9 versus DX10+ differences.

### Profile descriptors

`HlslProfileDesc` gains explicit, immutable policy fields:

- `HlslStage` with vertex, geometry, and pixel values;
- `HlslShaderModel` with SM3, SM4, and SM5 values;
- public profile name, compiler target, profile ID, connector IDs, and version
  comment;
- semantic-policy and resource-policy identities;
- capability flags for geometry, derivatives, discard, texture methods,
  modern constant buffers, and supported intrinsic families;
- a pointer to exact stage/model resource limits; and
- geometry input/output policy where applicable.

No lowering or code-generation path infers policy from a profile-name string.
Every switch over stage or model has a rejecting default and is covered by a
unit test.

### Thin HAL files

The six new files contain descriptors and connector tables only:

- `hlslv40_hal.c`, `hlslg40_hal.c`, `hlslf40_hal.c`;
- `hlslv50_hal.c`, `hlslg50_hal.c`, `hlslf50_hal.c`.

`hlsl_hal.c` registers all eight HLSL profiles, installs shared callbacks, and
routes generation through the completed backend. `hlsl_modern.h` and
`hlsl_modern.c` own shared DX10+ semantic, resource, interpolation, topology,
and texture-method policy. They do not own general expression lowering or
pretty-printing.

The completed SM3 files retain their established responsibilities:

- `hlsl_ir.h`/`.c`: typed target IR and builders;
- `hlsl_lower.c`: verified Cg IR to HLSL IR lowering;
- `hlsl_legalize.c`: target-equivalent rewrites;
- `hlsl_bind.c`: interfaces, wrappers, names, and physical bindings;
- `hlsl_validate.c`: structural and profile validation;
- `hlsl_codegen.c`: deterministic source output with no semantic decisions.

## Data Flow and Transactionality

The selected program is analyzed and lowered to verified Cg IR before the HLSL
backend runs. For geometry entries, Cg IR already contains resolved topology,
the topology-derived `AttribArray` extent, maximum-output metadata, canonical
semantic leaves, and explicit geometry statements.

The HLSL backend then:

1. validates the Cg module against the selected stage/model capability set;
2. lowers types, declarations, functions, expressions, statements, and exact
   source locations into shared HLSL IR;
3. builds the internal implementation entry and public target ABI wrapper;
4. legalizes only constructs whose HLSL spelling or evaluation model differs;
5. canonicalizes interfaces and allocates modern physical resources;
6. verifies the entire target module, including ownership and binding spans;
7. renders into a private output transaction; and
8. publishes binding records and HLSL only after every earlier phase succeeds.

An error in any phase prevents publication. The driver may retain its ordinary
compiler header behavior, but no `cgc-bind`, `cgc-default`, resource
declaration, structure, helper, wrapper, or partial target body is committed.

## Public Entry ABI

Every output has one public HLSL entry named `main`. The selected Cg entry is
lowered to an internal function whose identity is protected by the shared HLSL
name allocator. The wrapper calls the internal entry exactly once and
marshals all return, `in`, `out`, `inout`, varying, uniform, and aggregate
values without duplicating side effects.

Vertex and pixel wrappers use generated stage input and output structures.
Empty structures are omitted, and a void entry with no interface remains a
valid empty shader where the target permits it.

Modern system-value ABI fields use the HLSL-required type even when the Cg
surface uses `int`. For example, `SV_VertexID`, `SV_PrimitiveID`, and
`SV_RenderTargetArrayIndex` use `uint` at the public wrapper and explicit
numeric conversions at the internal Cg boundary. The conversion is represented
in IR and verified; code generation never inserts an implicit repair.

## Modern Interface Semantics

Semantic mapping is table-driven by stage and direction. Canonical semantic
identity is case-insensitive; source spelling remains available for metadata
and diagnostics.

### System values

| Cg meaning | Modern HLSL representation |
| --- | --- |
| vertex/geometry position output | `SV_Position` |
| pixel position input (`WPOS`/position input) | `SV_Position` |
| pixel color output `COLORn` | `SV_Targetn` |
| pixel depth output `DEPTH` | `SV_Depth` |
| vertex ID input | `SV_VertexID` |
| vertex instance ID input | `SV_InstanceID` |
| geometry primitive input | `SV_PrimitiveID` |
| geometry primitive output | `SV_PrimitiveID` |
| Cg geometry scalar `INSTANCEID` | `SV_PrimitiveID`, preserving its existing per-primitive Cg contract |
| geometry layer output `LAYER` | `SV_RenderTargetArrayIndex` |
| pixel face input `FACE` | `SV_IsFrontFace` |
| clip-distance outputs | `SV_ClipDistance` with exact component accounting |

Declaring Cg geometry `INSTANCEID` and `PRIMITIVEID` together conflicts because
both claim the same modern system input. A stable diagnostic names both source
locations.

`PSIZE` has no faithful rasterizer-controlled Shader Model 4/5 system-value
equivalent and is rejected when used for that purpose. Ordinary `FOG`,
`COLORn`, `TEXCOORDn`, `ATTRn`, tangent, binormal, blend, and user interface
values use deterministic user semantics when legal for their stage and
direction.

### Vertex ID bridge

`SV_VertexID` is a vertex-stage system input, not a per-vertex geometry system
input. When a geometry entry consumes `VERTEXID` as `AttribArray<int>`, the
matching vertex profile emits a generated `nointerpolation` user semantic and
the geometry profile consumes its topology-sized array. Cross-stage metadata
uses one canonical bridge key so mismatched or missing bridge declarations are
detected before external compilation.

### Interpolation

The target IR records `linear`, `centroid`, `noperspective`, and
`nointerpolation` independently of emitted text. Integral user varyings are
always `nointerpolation`. Flat geometry output leaves are also
`nointerpolation`. Producer and consumer qualifiers must match exactly under
the in-repository pipeline checker.

## Geometry Shader Lowering

Both geometry profiles consume the shared resolved Cg geometry contract. They
require a known positive `Vertices=N`, merging source declarations and repeated
profile options through the existing conflict rules.

### Input topology

The public geometry wrapper uses the exact input keyword and array extent:

| Cg topology | HLSL keyword | Extent |
| --- | --- | ---: |
| point | `point` | 1 |
| line | `line` | 2 |
| line adjacency | `lineadj` | 4 |
| triangle | `triangle` | 3 |
| triangle adjacency | `triangleadj` | 6 |

Per-vertex `AttribArray<T>` leaves are reconstructed from the corresponding
members of that input array. Scalar per-primitive system inputs are separate
wrapper parameters and are not repeated per vertex.

### Output topology and operations

| Cg output topology | HLSL stream type |
| --- | --- |
| points | `PointStream<Output>` |
| line strip | `LineStream<Output>` |
| triangle strip | `TriangleStream<Output>` |

The wrapper carries `[maxvertexcount(N)]` and one `inout` stream parameter.
Multiple output streams and geometry invocations are intentionally absent.

Each Cg geometry operation remains an explicit target-IR statement:

- `emitVertex` evaluates its bundle once in source order, assigns one complete
  generated output record, replays defined flat state, and calls
  `stream.Append(record)`;
- `flatAttrib` updates generated private shadow values and defined flags but
  emits no vertex;
- `restartStrip` calls `stream.RestartStrip()` with no operands.

Flat shadow state preserves ordinary control-flow behavior in entries and
reachable helpers. Every append receives the currently defined flat values.
The output fields are also marked `nointerpolation`. Position remains illegal
in `flatAttrib` under the shared frontend rule.

## Types, Expressions, and Control Flow

The modern profiles reuse the completed SM3 typed IR and general lowerer.
Scalars, vectors, rectangular matrices, arrays, structures, helpers,
overloads, constructors, selectors, swizzles, indexing, casts, assignments,
conditionals, loops, jumps, and returns preserve the same Cg evaluation order.

`float` remains `float`. `half` and `fixed` use the completed HLSL backend's
portable promotion policy and retain source type in metadata. The current Cg
frontend exposes no modern-HLSL `double` contract, so SM5 does not invent one.
Matrices retain explicit `row_major` policy and do not rely on external
compiler defaults.

Model-specific legalization is capability-driven. SM5 may accept a Cg
operation only when the current frontend already represents its meaning and
SM5 has an exact spelling. The backend does not expose a new intrinsic merely
because SM5 supports one.

Derivatives and discard remain pixel-stage-only. Geometry texture operations
must not require implicit screen-space derivatives. Vertex texture operations
likewise require an explicit level or another stage-legal exact form.

## Modern Uniform and Texture Resources

### Constant data

Modern numeric and Boolean uniforms are emitted in generated, explicitly bound
constant buffers. The binder uses deterministic HLSL constant-buffer packing
and explicit `packoffset(cN.component)` placement. It preserves the completed
SM3 backend's recursive logical identity and reconstructs a logical aggregate
when physical leaves must be emitted separately.

Explicit source bindings are allocated before implicit bindings. Unbound
values use deterministic first fit in source declaration order. Matrices,
arrays, and structures reserve their complete model-defined span; aliases and
aggregate leaves cannot overlap. Binding metadata records the public Cg name,
logical type, buffer slot, byte/component offset, span, source semantic, and
default value.

The default policy uses one generated application-visible constant buffer at
`b0`. Additional generated buffers are not introduced merely to preserve DX9's
separate `c`, `i`, and `b` register banks. If a source binding cannot map
unambiguously into the modern buffer contract, the profile emits a precise
binding diagnostic rather than choosing a silent reinterpretation.

### Texture and sampler pairs

Every Cg sampler becomes two declarations with one public logical binding:

```hlsl
Texture2D<float4> cgc_texture_name : register(tN);
SamplerState cgc_sampler_name : register(sN);
```

The dimension selects `Texture1D`, `Texture2D`, `Texture3D`, or `TextureCube`.
The texture and sampler use the same numeric index. Explicit `TEXUNITn`
bindings reserve both slots; implicit pairs use deterministic first fit where
both slots are free. Pairing makes the 16-sampler limit the effective maximum
number of Cg sampler objects even though more shader-resource slots exist.

Signature-driven legalization maps supported operations to exact methods:

- ordinary sampling -> `.Sample`;
- explicit level -> `.SampleLevel`;
- bias -> `.SampleBias` where stage-legal;
- gradients -> `.SampleGrad` where stage-legal;
- projected forms -> explicit coordinate division evaluated once, followed by
  the correct method.

Coordinate width, result type, sampler dimension, stage, and derivative
requirements are validated before a call node is accepted. Comparison
sampling, resource arrays, multisample textures, loads, gathers, and other
forms absent from the current Cg contract are not synthesized.

## Shader Model Differences and Limits

The SM4 and SM5 profiles share language lowering but never share one approximate
limit table. Each descriptor records the exact stage/model minima used by the
compiler.

The validator accounts for at least:

- input and output signature registers/components by stage;
- eight pixel render targets and one depth output;
- constant-buffer slots, maximum constant-buffer elements, and packed spans;
- 128 shader-resource slots and 16 sampler slots per stage;
- clip-distance component limits;
- geometry input and output registers;
- positive geometry `maxvertexcount`;
- the 1024-component geometry invocation budget; and
- the model-specific vertex-input and stage-output differences between D3D10
  and D3D11.

SM5's larger vertex and geometry input limits are available to SM5 profiles
only. SM5 support for multiple geometry streams, geometry invocations, newer
resource objects, and tessellation does not enter the capability table because
those features are outside this design.

Every count is derived from verified canonical interface leaves and physical
resource spans, never from source declaration count. Exact-boundary and
one-over-boundary tests pin every limit.

## HLSL IR and Verification

The shared HLSL IR gains only representations required by the modern targets:

- shader-model identity and modern resource declarations;
- constant-buffer declarations and pack offsets;
- separated texture and sampler objects;
- system-value and interpolation identities;
- geometry input topology, output stream topology, and maximum vertices;
- topology-sized interface arrays;
- explicit append and restart statements; and
- flat-shadow declarations, flags, and per-append replay sets.

The standalone verifier checks both common and dialect-specific invariants.
Among other rules, it rejects:

- profile/stage/model disagreement;
- geometry metadata on non-geometry modules or missing metadata on geometry;
- an illegal topology, array extent, stream type, or maximum;
- system values in an invalid stage or direction;
- system-value type disagreement at the public ABI;
- duplicate canonical semantics or mismatched producer/consumer qualifiers;
- malformed cbuffer placement, overlap, alignment, or ownership;
- unpaired or inconsistently indexed texture/sampler resources;
- texture methods with an invalid receiver, signature, coordinate, or stage;
- append/restart/flat nodes outside geometry;
- malformed flat replay or output-record ownership;
- expression-only nodes in statement positions and vice versa; and
- a missing or multiply defined public `main`.

`HlslWriteModule` invokes the verifier before writing its first target byte.
Unit tests prove the zero-byte gate for every new invariant.

## Diagnostics

Modern profile errors extend the HLSL block reserved by the SM3 design rather
than creating a disconnected numbering scheme. Structured reasons distinguish:

- selected profile versus program stage;
- missing or conflicting geometry configuration;
- unsupported stage/model capability;
- invalid or conflicting system semantics;
- interface type or interpolation mismatch;
- constant-buffer placement and exhaustion;
- texture/sampler pair collision or exhaustion;
- invalid texture method signature or derivative requirement;
- geometry maximum and component-budget violations; and
- malformed target IR.

User errors retain the nearest original `SourceLoc`. Conflicts include a note
for the first declaration. A callback that reports a specific HLSL error must
not trigger a second generic binding or code-generation error.

## Output Contract

Successful modern HLSL output is deterministic and ordered as follows:

1. compiler, profile, target, and selected-entry comments;
2. stable `// cgc-bind` and `// cgc-default` metadata;
3. constant buffers;
4. texture and sampler declarations;
5. generated and source structures;
6. helper prototypes and definitions in dependency order;
7. the lowered internal entry; and
8. the single public `main` wrapper.

Generated names are identity-based and protect HLSL reserved words plus all
backend prefixes. Machine-specific paths, effects syntax, compiled bytecode,
and SDK includes never appear in output.

## Testing Strategy

### Prerequisite regression

Before modern work begins, the completed SM3 Debug and Release suites pass and
its shader goldens remain byte-identical after the descriptor refactor except
for the intentional internal-ID repair where metadata exposes IDs.

### Unit tests

Focused C90-compatible tests cover:

- unique profile/connector IDs and real stage identities;
- descriptor selection for all eight HLSL profiles;
- modern system semantic tables and conflicts;
- topology and stream mappings;
- wrapper conversions for system-value `uint` fields;
- vertex-ID bridge construction;
- interpolation classification and cross-stage equality;
- constant-buffer packing, defaults, aggregate reconstruction, and collision;
- texture/sampler pair allocation and method selection;
- geometry append, flat state, and restart IR;
- SM4/SM5 capability and resource boundaries; and
- a malformed module for every verifier reason.

Assertion-active test variants remain enabled so release-only assertions
cannot hide invalid IR.

### Golden and negative fixtures

Every new profile has a registration golden. Vertex and pixel fixtures cover
the four bundled shaders, aggregates, helpers, control flow, matrices,
uniforms, defaults, textures, derivatives, discard, semantic aliases, and name
collisions. Geometry fixtures cover all five inputs, all three outputs,
pass-through, amplification, adjacency, conditional and repeated flat state,
strip restart, primitive IDs, vertex ID bridging, and layered output.

Negative fixtures assert nonzero status, one exact diagnostic code, source
line, stable message fragment, relevant note, and no published target body.
They cover every semantic, binding, texture, geometry, stage, and resource
failure class.

### Cross-stage qualification

Representative SM4 and SM5 vertex-geometry-pixel pipelines compare canonical
producer/consumer semantics, type shapes, component widths, system values, and
interpolation. Matching pipelines pass. Deliberate type, semantic,
interpolation, and vertex-ID bridge mismatches fail in the repository harness.

Because separately compiled DirectX shader stages are not linked by `fxc`, the
in-repository interface checker is the normative cross-stage gate. Each stage
is also compiled independently by `fxc` when it is available.

### External validation

The test harness locates `fxc.exe` without making configuration fail when it is
absent. When present, it compiles generated files with `/E main` and the exact
`/T vs_4_0`, `/T gs_4_0`, `/T ps_4_0`, `/T vs_5_0`, `/T gs_5_0`, or `/T ps_5_0`
target. A dedicated Windows qualification run requires all six targets and
reports external compiler diagnostics without normalizing them into cgc
diagnostics.

### Full regression

The complete Debug and Release suites include all generic, Cg 2.0, geometry,
GLSL 1.50, ARB, SM3, parser-regeneration, standard-library-regeneration, and
new SM4/SM5 tests. The four bundled shaders retain their existing generic and
GLSL behavior.

## Documentation

Implementation updates `README.md` with all six profiles, example commands,
geometry options, modern resource pairing, the source-only boundary, and
optional `fxc` validation.

`docs/hlsl-sm4-sm5-compatibility.md` classifies every current Cg 2.0 type,
operator, statement, semantic, intrinsic, texture form, geometry feature, and
binding form as:

- exact shared support;
- support through a named legalization;
- stage- or model-specific support;
- precise rejection with a diagnostic; or
- not exposed by the current frontend.

Every support or rejection row names at least one registered test. No row may
remain unclassified at completion.

## Implementation Sequence

1. Complete and verify the DirectX 9 HLSL plan.
2. Repair HLSL IDs and stage identities; pin uniqueness in tests.
3. Refactor shared descriptors around explicit stage/model policies with no
   SM3 output regression.
4. Register all six modern profiles and their thin HAL descriptors.
5. Add modern semantic, interpolation, wrapper, and cross-stage policy.
6. Add modern constant-buffer and texture/sampler resource policy.
7. Qualify SM4 vertex and pixel lowering and output.
8. Qualify SM5 vertex and pixel lowering and output.
9. Add shared geometry HLSL IR extensions and lowering.
10. Qualify SM4 geometry and complete SM4 pipelines.
11. Qualify SM5 geometry and complete SM5 pipelines.
12. Enforce exact model/stage limits, diagnostics, and transactions.
13. Add optional `fxc`, compatibility documentation, and complete Debug/Release
    qualification.

Each step begins with failing unit or compiler fixtures, implements the
smallest policy or representation needed to pass them, runs focused plus full
regressions, and ends in a focused commit.

## Completion Criteria

The work is complete only when:

1. The full SM3 plan is complete and its output remains stable after shared
   refactoring.
2. All profile and connector IDs are unique and pinned.
3. All six modern profiles register with their exact stage and target.
4. Every representable current Cg 2.0 construct produces verified,
   deterministic HLSL, and every non-representable construct has a precise
   diagnostic.
5. Modern semantics, system-value wrapper types, interpolation, and
   cross-stage interfaces match the specified contract.
6. Every Cg sampler produces one deterministic same-index `tN`/`sN` pair and
   every supported texture operation uses the correct method.
7. All five geometry inputs, three outputs, and three geometry operations reach
   explicit verified HLSL IR and valid stream-object source.
8. Flat state remains path-correct across helpers, branches, loops, repeated
   updates, and multiple emitted vertices.
9. Exact SM4 and SM5 resource boundaries and one-over cases pass their expected
   tests.
10. Failed translation publishes no HLSL body or binding metadata.
11. Representative SM4 and SM5 three-stage pipelines pass the repository
    interface checker, while deliberate mismatches fail.
12. Every generated qualification shader compiles under its exact `fxc` target
    when the validator is available.
13. The compatibility matrix contains no unclassified rows.
14. Parser and standard-library regeneration produce no diff.
15. Complete Debug and Release CTest suites report zero failures, `git
    diff --check` reports no errors, and the worktree contains no unintended
    artifacts.

## Principal Risks and Mitigations

### Shared-backend regressions

Risk: adding model switches subtly changes SM3 output.

Mitigation: refactor descriptors before adding features, pin SM3 goldens, and
run SM3 tests at every task boundary.

### Semantic name resemblance

Risk: similar Cg, DX9, and system-value names are treated as equivalent when
their stage or type rules differ.

Mitigation: canonical semantic identities carry stage, direction, system/user
class, ABI type, and interpolation; no behavior is selected by spelling alone.

### Modern resource pairing

Risk: a Cg sampler receives inconsistent texture and sampler slots or a partial
binding is published.

Mitigation: allocate a pair atomically, verify same-index ownership, and render
only after whole-module validation.

### Geometry side effects

Risk: direct call rewriting loses evaluation order, flat state, or stream
boundaries.

Mitigation: retain explicit append, flat, and restart nodes from Cg IR through
HLSL IR, attach verified replay to each append, and test nested control flow.

### External compiler availability

Risk: ordinary development environments do not contain legacy `fxc.exe`.

Mitigation: keep core tests self-contained, make discovery optional, and
maintain a separate qualification mode where missing `fxc` is a failure.

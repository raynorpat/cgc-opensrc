# Metal MSL Vertex and Fragment Output Design

## Status and scope

The user approved the direct Cg IR backend architecture and the vertex/fragment
subset on 2026-09-08, then selected macOS-first qualification. This document
records that design and makes the initial compatibility and application ABI
choices explicit. Implementation has not started; qualification results are
not implied by this specification.

Deliver `mslv` and `mslf`, translating Cg 2.0 to standalone Metal Shading
Language source. cgc continues to build on Windows and other existing hosts.
Apple compilation and execution are separate qualification steps on macOS.
Use MSL 2.0 as the fixed language baseline and macOS 13 as the initial minimum
deployment target for qualification. These are project scope choices, not a
claim that MSL 2.0 requires macOS 13. Record the actual SDK, compiler, OS, and
GPU used. iOS qualification is a later project.

Exclude geometry/compute/tessellation, argument buffers, binary metallib
generation inside cgc, a renderer library, CgFX, and Cg 1.1 MSL emission.
Valid Cg outside the supported subset receives a Metal profile diagnostic.
Do not broaden the grammar or change another backend to implement Metal.

## Existing architecture and alternatives

The checked-out repository has substantially more functionality than the
historical repository guidelines describe: CMake/CTest, verified typed Cg IR,
and modular GLSL, HLSL, and ARB backends. Keep the guidelines' four bundled
shader regression requirement as well as the actual automated suite.

The selected design is direct Cg IR -> MSL module -> verified MSL source.
`CompileProgram` already invokes the paired HAL `ValidateIR`/`GenerateIR`
hooks for Cg 2.0; `GlslLowerCgIR` is an architectural reference. Do not clone
GLSL's legacy tree route or adopt its target-specific limitations accidentally.

An external intermediate compiler would introduce toolchain dependencies and
another semantic boundary. Adapting the HLSL writer would save some syntax
work but would leave Metal address spaces, resources, and stage interfaces
entangled with DirectX assumptions. Neither alternative is selected.

## Profile integration and ownership

Reserve profile IDs 23/24 and connector IDs 36..39, after checking they are
still free at implementation time. Register exact names `mslv`/`mslf`, the
existing `vs`/`ps` overload selectors, and the correct vertex/fragment stage identities.
Use the same wildcard specificity as the existing GLSL and HLSL profiles.
Expose the existing style of profile preprocessor macros. Reject Cg 1.1
through a deliberate profile diagnostic, including `-nocode` compilation.

`msl_hal.*` owns profile descriptors, capabilities, and HAL adapters;
`mslv_hal.c`/`mslf_hal.c` own stage-specific descriptors.
`msl_ir.*` owns typed target nodes and arena allocation.
`msl_validate.c` owns source capability validation; `msl_verify.c` owns target
invariants. `msl_bind.*` owns semantics, resources, and byte layout.
`msl_lower.c` orchestrates private lowerers for declarations, expressions,
statements, functions, aggregates, and textures. `msl_codegen.c` only writes
verified nodes. One private context is defined in `msl_lower_internal.h`.

Use canonical Cg types, intrinsic IDs, source locations, and resolved function
identities. Do not identify overloads by text. Do not mutate the source IR.
Target nodes have module lifetime; validation uses disposable pool storage.
Generation may lower a second time, matching GLSL's existing lifecycle.
Avoid adding a shared backend framework or new mutable global state.

## Supported language surface

Runtime numeric types: bool, signed/unsigned 32-bit integers, float, and their
2/3/4-component vectors. Half and fixed source arithmetic is deliberately
promoted to float, including uniform storage; document that this profile does
not emulate fixed-point range or rounding. Compile-time numeric types resolve
to supported runtime types. Reject runtime double, long, and narrow integer
types for this release. Support float 2x2, 3x3, and 4x4 matrices.

Support fixed-size arrays and acyclic structs of supported values; constructors,
casts, member/index/swizzle access; scalar/vector arithmetic and comparisons;
integer bitwise operators; assignments; structured loops and branches;
break/continue; helpers, overload selection, and in/out/inout parameters.
Support local constant initializers and read-only uniform defaults.
Reject unsized arrays, interface dispatch, recursion, writable globals,
resource arrays/struct members/returns, and unsupported stage interfaces.

The initial numeric intrinsic allowlist is abs, min, max, clamp, saturate,
floor, ceil, frac, fmod, sqrt, rsqrt, pow, exp, exp2, log, log2, sin, cos,
tan, asin, acos, atan, atan2, dot, cross, length, distance, normalize,
reflect, refract, lerp, step, smoothstep, mul, transpose, any, and all.
Only compatible signatures from the shared catalog are admitted; integer
overloads are restricted to abs/min/max/clamp and bool reductions as applicable.
Matrix intrinsics are restricted to mul/transpose. Unsupported signatures get
a diagnostic even when the intrinsic name has other supported signatures.
Fragment-only operations are discard, ddx, and ddy. Other intrinsics are rejected.

Texture support: sampler2D and samplerCUBE, float4 sample results, ordinary
implicit sampling in fragment shaders and explicit LOD sampling in either
stage. Reject implicit sampling from a vertex shader, gradient/bias/projected
forms, comparison samplers, and other dimensions in this release. Samplers
may pass through helper input parameters; they cannot be copied into ordinary
numeric storage, returned, or dynamically selected.

## Evaluation and matrix behavior

Preserve the normalized IR's sequencing. Cg matrix constructors and row indexing
must be translated explicitly to MSL's column representation. Matrix `*`
retains Cg component-wise semantics; intrinsic `mul` retains its algebraic
semantics, including vector dot products. Lower matrix arithmetic per column
where the direct MSL operator has different behavior or is unavailable.

Materialize RHS values and lvalue indices before scatter writes, including
matrix selectors, to avoid changing overlapping assignments or evaluating a
side effect twice. Array value operations use generated wrapper structs and
element copies where raw MSL arrays lack the required value behavior.
Implement helper out/inout using caller-owned temporaries and ordered
copy-in/copy-out. Never substitute unrestricted reference aliasing for Cg
copy semantics. Uniform values entering writable formals are copied locally.

## Stage interface ABI

Emit `#include <metal_stdlib>` and `using namespace metal;` once. Entry exports
are `cg_mslv_<escaped-entry>` and `cg_mslf_<escaped-entry>`; record the exact
export in metadata. Mangle other names by resolved identity with deterministic
collision handling, independent of memory addresses and allocation order.

Flatten supported numeric entry structs into stage wrapper fields and rebuild
source values in thread-local storage. Entry outputs may be a return value,
out/inout formals, or a struct; detect semantic collisions across all of them.
Reject array/matrix-valued stage leaves. Canonicalize semantic case, implicit
zero indices, and HPOS/DIFFUSE/SPECULAR/FOGCOORD aliases consistently.

| Source role | Metal role |
| --- | --- |
| Vertex ATTRIB0..15 | attribute(0)..attribute(15) |
| Other supported named vertex inputs | Lowest free attribute slots, sorted by canonical semantic |
| Vertex POSITION/HPOS output | float4 position |
| COLOR0..1, TEXCOORD0..7, FOG0 between stages | Matching user(cg_SEMANTIC) fields |
| Fragment COLOR0 return/output | float4 color(0) |
| Fragment DEPTH output | float depth(any) |

Support vertex named inputs POSITION0, NORMAL0, COLOR0..1, TEXCOORD0..7,
TANGENT0, BINORMAL0, BLENDWEIGHT0, and BLENDINDICES0. Explicit ATTRIB slots
reserve first. Named inputs are not implicit aliases of ATTRIB slots; metadata
is authoritative for vertex descriptor construction. Integer varyings use flat
interpolation; float varyings use perspective interpolation. Reject unsupported
system semantics rather than mapping them to arbitrary attributes. Vertex
POSITION is required, and at least one fragment output is required.

Compiler policy limits are 16 vertex attributes, 64 user-varying scalar
components per stage, one color output, 16 texture/sampler pairs, and 4096
uniform bytes per stage. They bound this release; they are not advertised as
the complete hardware limits. Validate exact-limit and one-over-limit cases.

No implicit clip-depth remapping, Y inversion, texture-coordinate inversion,
or half-pixel offset. The application supplies Metal-compatible projection
and texture conventions. Derivatives are of the emitted fragment values in
Metal's coordinate system. A future compatibility option needs its own spec.

## Uniform and texture ABI version 1

Each stage uses at most one constant buffer at buffer(0). Vertex buffer slots
used by the application's vertex descriptor must therefore avoid slot 0.
No uniform buffer parameter is emitted when there are no numeric uniforms.
Avoid dependence on the compiler host's C ABI: wire storage consists only of
float4/int4/uint4 slots, with 16-byte alignment and size per slot.

Sort top-level uniforms by stable qualified name, retaining source member
order and increasing array indices within each value. Each scalar/vector leaf
occupies a complete slot; bool uses uint components 0/1. Each matrix column
occupies one float4 slot, padding unused lanes. Structs and arrays recursively
concatenate their leaves. Decode wire storage to source-value structs and
array wrappers through generated reads/copies, including dynamic array reads.
Uninitialized padding is never read as a source value. Sparse layouts are an
intentional first-release simplicity trade-off.

Retain every numeric uniform entry parameter and every global uniform referenced
by the reachable program. Entry parameter paths use their source names;
global paths use `global.<name>` to avoid shadowing collisions. Resource
collection uses the same entry/global reachability rule. An unused helper's
globals do not affect the ABI.

For float3 tint followed by float exposure in source member order, offsets
are 0 and 16, total 32 bytes. A float3x3 occupies 48 bytes with column stride
16. Never use the HLSL cbuffer packer or native float3 struct packing here.

Explicit TEXUNIT0..15 reserves matching texture/sampler indices. Allocate
unbound resources by stable qualified name into lowest free slots. Reject
duplicate explicit slots. Each pair is texture2d<float>/texturecube<float>
plus sampler; the application supplies filtering/addressing state. Propagate
both values through helper signatures, and propagate numeric global-uniform
dependencies through explicit constant references. No hidden shader globals.

Write deterministic line comments with a version header and records for
entry, attributes, varyings, buffer size, uniform leaves, and sampler pairs:

```text
// cgc-msl-abi version=1 profile=mslf language=2.0
// cgc-msl-entry source=main metal=cg_mslf_main
// cgc-msl-buffer index=0 bytes=32 alignment=16
// cgc-msl-uniform path=params.tint offset=0 slots=1 type=float3 wire=float4
// cgc-msl-uniform path=params.exposure offset=16 slots=1 type=float wire=float4
// cgc-msl-resource path=image texture=0 sampler=0 type=2d
```

Attribute records have the form `cgc-msl-attribute semantic=ATTRIB0 index=0 type=float4`.
Varying records have the form `cgc-msl-varying semantic=TEXCOORD0 field=cg_TEXCOORD0 type=float2 interpolation=perspective`.
Built-in records use `cgc-msl-builtin semantic=POSITION0 attribute=position type=float4`.
Uniform records add `source_type=half` or `source_type=fixed` (with vector
width where relevant) when numeric storage is promoted. Prefix all records
with `// ` and serialize each category in deterministic binding/path order.

Paths use source identifiers, dots, and bracketed constant array indices;
these cannot contain whitespace under the source grammar. Matrix records add
`column_stride=16`; array expansion records every element's leaf offset.
Emit defaults in logical source order as `cgc-msl-default path=... values=...`
with comma-separated round-trippable literals. Defaults are application
initialization data, never implicit device-buffer initialization. This release
ships a documented comment format rather than a new sidecar CLI or runtime API.

## Diagnostics, output, and verification

Use dedicated profile diagnostics with source locations for unsupported types,
operations, semantics, bindings, stages, language modes, and limit overflow.
Retain the existing reachability call-path notes. Validate under -nocode too.
Target verifier failures remain internal errors, not ordinary unsupported-Cg
errors. Allocation and writer failures must return failure consistently.

Reuse output.c's transaction for -o. Existing destinations survive failure;
new destinations are not installed. Validate source and target before writing
shader declarations to stdout. Stdout cannot be rolled back after an I/O
failure; report that failure without promising atomic stdout.

Cross-platform tests cover exact generated source, diagnostic code/location,
exit status, deterministic metadata, IR invariants, and resource layouts.
Compare stdout, stderr, and status for position.cg, reflection.cg,
vertexlight.cg, and vertexlight4.cg with the pre-change generic compiler,
including explicit Cg 1.1, and run existing CTest coverage.

Release qualification additionally compiles every positive MSL fixture with
Apple's compiler using an explicit language/deployment target, creates real
vertex/fragment pipelines, and renders small numerical fixtures. Compiler-only
success does not establish interstage or execution correctness. Missing Apple
tools/GPU may skip local optional tests but must fail strict qualification.

## References and evidence limits

- [Apple texture sampling example](https://developer.apple.com/documentation/metal/creating-and-sampling-textures): separate texture/sampler use.
- [Apple render command encoder guide](https://developer.apple.com/library/archive/documentation/Miscellaneous/Conceptual/MetalProgrammingGuide/Render-Ctx/Render-Ctx.html): pipeline construction and resource binding.
- [Apple Metal tools guide](https://developer.apple.com/library/archive/documentation/Miscellaneous/Conceptual/MetalProgrammingGuide/Dev-Technique/Dev-Technique.html): source -> AIR -> metallib workflow.
- [MSL language version 2.0](https://developer.apple.com/documentation/metal/mtllanguageversion/version2_0): selected language identity.
- [Metal Shading Language specification](https://developer.apple.com/metal/Metal-Shading-Language-Specification.pdf): implementation reference for types, address spaces, attributes, and intrinsics. The web reader could not retrieve the full PDF in this planning session; exact syntax/layout rules require the compiler probes in plan task 1.

No Apple compiler or Metal GPU validation was performed while writing this design.

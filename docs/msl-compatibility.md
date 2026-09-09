# Metal output: implemented and tested surface

`mslv` and `mslf` translate verified Cg IR to standalone MSL 2.0 source.
The current implementation has passed Apple compilation and offscreen rendering
on the Mac recorded in [the qualification record](msl-qualification.md).
The [coverage audit](msl-coverage.md) records 624 passing Metal tests and the
remaining language/frontend boundaries against the implementation plan.

## Running it

```sh
cmake -S . -B build/metal -DBUILD_TESTING=ON -DCMAKE_BUILD_TYPE=Release \
  -DCGC_REQUIRE_METAL=ON -DCGC_REQUIRE_METAL_RUNTIME=ON
cmake --build build/metal --config Release
ctest --test-dir build/metal -C Release -R '^msl_' --output-on-failure
build/metal/cgc -quiet -version 2.0 -profile mslv -o position.metal position.cg
build/metal/cgc -quiet -version 2.0 -profile mslf -o fragment.metal \
  tests/msl/profile/fragment.cg
xcrun --sdk macosx metal -std=macos-metal2.0 -mmacosx-version-min=13.0 \
  -c position.metal -o position.air
xcrun --sdk macosx metallib position.air -o position.metallib
```

The compiler emits text only; Apple compilation is external. Library entry
exports are `cg_mslv_<entry>` and `cg_mslf_<entry>`. Source `main` therefore
exports `cg_mslv_main` or `cg_mslf_main`. Cg 1.1 is rejected during profile
selection, including `-nocode`. Use `(void)` for a parameterless definition;
the existing parser treats empty `()` as abstract parameters.

## Evidence by feature family

Each `msl_profile_*` test compares exact source and, when Apple tools are
available, compiles and links that same output. GPU tests require those fixture
artifacts through CTest fixtures. Runtime expectations are independent CPU
constants, not values produced by this backend.

| Behavior | Representative tests |
| --- | --- |
| Vertex/fragment output, alternate entry | `msl_profile_vertex`, `msl_profile_fragment`, `msl_profile_alternate`, `msl_runtime_solid` |
| Extended acceptance matrix and fault sweeps | [Coverage audit](msl-coverage.md) |
| Half/fixed promotion metadata and execution | `msl_profile_half`, `msl_runtime_half` |
| Read-only uniform update rejection | `msl_diagnostic_read_only` |
| Numeric expressions, helpers, loops, break/continue | `msl_runtime_arithmetic` |
| Square matrix construction and both multiplication orders | `msl_runtime_matrix` |
| Whole-row matrix assignment | `msl_runtime_row_write` |
| Structs and 16-byte-slot uniform decoding | `msl_runtime_layout`, `msl_runtime_uniform` |
| Fixed-array uniform decoding | `msl_runtime_array` |
| Global uniform propagation through helpers | `msl_runtime_global` |
| Host-applied constant default metadata | `msl_profile_default`, `msl_runtime_default` |
| `inout` scalar swizzle copy-in/copy-out | `msl_runtime_swap` |
| Reordered varyings, flat integer interpolation | `msl_runtime_varying` |
| Deliberately incompatible stage interfaces | `msl_runtime_mismatch` |
| Fragment discard and depth output | `msl_runtime_discard`, `msl_runtime_depth` |
| Explicit texture/sampler slot 3, two helper levels | `msl_runtime_texture` |
| All six cube directions | `msl_runtime_cube0` through `msl_runtime_cube5` |
| Nonzero LOD in fragment and vertex stages | `msl_runtime_lod`, `msl_runtime_cube_lod`, `msl_runtime_lod_vertex` |
| All four unmodified bundled shaders | `msl_profile_position`, `msl_profile_reflection`, `msl_profile_vertexlight`, `msl_profile_vertexlight4`; matching `msl_runtime_pipeline_*` tests |
| Recursion rejection, also under `-nocode` | `msl_function_recursion_code`, `msl_function_recursion_nocode` |
| Unsupported texture stage/binding and uniform overflow | `msl_diagnostic_vertex_sampling`, `msl_diagnostic_resource_slot16`, `msl_diagnostic_uniform_overflow` |
| Output preservation and no leaked temporary files | `msl_transaction_preserves_output` |
| Invalid/stale Apple artifacts, wrong golden/status, wrong GPU pixels | `msl_apple_invalid_and_stale_control`, `msl_runner_wrong_source_and_status`, `msl_runtime_wrong_expected_control` |
| Dirty-memory allocation, allocation failure, malformed target nodes | `msl_ir_invariants_and_allocation` |

Runtime tests render an 8×8 RGBA32Float texture and check 16 interior pixels at
absolute-plus-relative tolerance `1e-5 * (1 + abs(expected))`. The depth test
also reads a Depth32Float target. The bundled examples are compilation and
pipeline-creation witnesses; their partially initialized outputs are not used
as numerical reference images.

The numeric type mapping covers bool, int, uint, float and vectors of width
2–4; half/fixed arithmetic and storage map to float, with original uniform
types recorded in `source_type` metadata. Square floating matrices
map to Metal column matrices while source row indexing and constructors retain
Cg behavior. Fixed arrays use value-wrapper structs. Struct members retain
source order. Unsupported runtime scalar kinds are rejected with C6601.

The intrinsic lowering allowlist is abs, min, max, clamp, saturate, floor, ceil,
frac, fmod, sqrt, rsqrt, pow, exp, exp2, log, log2, sin, cos, tan, asin, acos,
atan, atan2, dot, cross, length, distance, normalize, reflect, refract, lerp,
step, smoothstep, mul, transpose, any, all, and fragment ddx/ddy. This lists
implemented translations, not exhaustive qualification of every overload.
`frac` becomes `fract`, `lerp` becomes `mix`, and derivatives become `dfdx/dfdy`.

`sampler2D` and `samplerCUBE` become independent texture and sampler arguments.
Implicit sampling is fragment-only. `tex2Dlod` and `texCUBElod` are registered
as **Metal-profile builtins**, because they are absent from this repository's
shared Cg 2.0 catalog. They are resolved by builtin identity and use coordinate
`.w` as LOD; they do not extend other profiles' intrinsic catalog.

## Application ABI

Each stage reserves buffer index 0 for uniforms. Vertex attribute data must use
another vertex-buffer index; the runtime tests use index 1. No uniform parameter
is emitted for a stage with no numeric uniform data.

Each scalar/vector occupies one complete 16-byte slot; each matrix column uses
one float4 slot. Bool uses uint lanes. Arrays concatenate elements and structs
concatenate members in source order. Top-level uniforms are sorted by qualified
name; global names have the `global.` prefix. Only reachable globals are kept.
Numeric global dependencies are passed explicitly through helper parameters.

For `Params { float3 tint; float exposure; float3x3 basis; }`, the tested layout
is tint at byte 0, exposure at byte 16, basis at byte 32 with column stride 16,
and total size 80. A CPU upload can avoid native struct-layout assumptions:

```c
float slots[20] = {
    .25f, .5f, .75f, 0, /* tint, offset 0 */
    2, 0, 0, 0,        /* exposure, offset 16 */
    1, 4, 7, 0,        /* matrix column 0, offset 32 */
    2, 5, 8, 0,        /* column 1, offset 48 */
    3, 6, 10, 0        /* column 2, offset 64 */
};
/* Upload all 80 bytes to the stage's buffer(0). */
```

`cgc-msl-abi version=1` comments record exports, attributes, outputs, uniform
leaf offsets, buffer sizes, defaults, and texture/sampler pairs. A `TEXUNIT3`
binding reserves both texture(3) and sampler(3). Unbound resources use the
lowest free index in qualified-name order. Applications supply filtering and
address modes. Constant uniform defaults are metadata for the host to apply;
they never initialize device buffers implicitly.

Policy limits are 16 vertex attributes, 64 interstage scalar components per
stage, 16 texture/sampler pairs, 4096 uniform bytes, and one fragment color
output plus optional depth. Explicit ATTRIB indices reserve first; permitted
named vertex semantics use remaining slots in canonical name order. HPOS,
DIFFUSE, SPECULAR, and FOGCOORD aliases are canonicalized. Integer varyings are
flat; floating varyings use perspective interpolation. No depth remapping,
Y flip, texture-coordinate flip, or half-pixel correction is applied.

## Completed plan extensions

Metal enables local struct/array initializers, nested aggregate defaults,
read-only global constants (including dependent constant expressions), effectful
short-circuit expressions, and componentwise matrix arithmetic with scalar
lifting. Whole-matrix and whole-row compound assignments stabilize addresses
and values before stores. Selector writes preserve normalized source ordering.
`local_initializers`, `nested_initializers`, `aggregate_defaults`,
`global_aggregate_constant`, `dependent_constants`, `logical_effects`,
`matrix_compound`, `row_compound`, `matrix_scalar_arithmetic`, and
`matrix_binary_effects` have generated-source and numerical GPU witnesses.
Uniform defaults remain host-applied; global constants consume no buffer slots.

Metal-only overloads provide scalar/vector int/uint min/max/clamp and vector
`mul` as dot product. The shared catalog and other profiles retain their
acceptance rules, checked by `msl_plan_contract` and the full regression suite.
Helpers receive only their transitive global dependencies in deterministic
order. Variable, field and helper names retain escaped source spellings with
ordinal disambiguation. Aggregate and texture lowering have private modules;
scratch validation and emission share the same lowering/verifier path.

## Unsupported features and qualification limits

- Nonconstant uniform defaults are unsupported; defaults must resolve to
  constant scalar/vector or nested aggregate constructor data.
- Resource arrays, resource struct members/returns/copies, dynamic resource
  selection, other texture dimensions, comparison textures, gradient/bias/
  projected texture forms, recursion, writable globals, dynamic arrays,
  interfaces, and geometry/compute/tessellation are unsupported.
- Boundary, overload-selection, side-effect/aliasing, allocation-failure,
  metadata and diagnostic-location tests are recorded in the coverage audit.
  Passing that matrix does not establish every possible combination or input.
- Execution was tested on one M4 Max Mac and its installed OS. The macOS 13
  deployment flag was compiled successfully, but macOS 13 itself, other GPUs,
  Windows builds of the new backend, and iOS have not been qualified.

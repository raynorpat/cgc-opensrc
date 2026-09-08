# Metal MSL Output Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking. Use superpowers:subagent-driven-development only if the user authorizes delegation.

**Goal:** Add deterministic `mslv` and `mslf` source output for the documented Cg 2.0 subset, qualified with Apple's compiler and a Metal runtime on macOS.

**Architecture:** Lower the existing verified Cg IR into a separate typed MSL module. Validate its stage interfaces, uniform layout, resources, and target invariants before emission, using the existing HAL hooks and output transaction. Keep declaration, expression, statement, aggregate, texture, and function lowering in private C modules.

**Tech Stack:** Existing C90 compiler, CMake/CTest, Cg IR, MSL 2.0; macOS 13 deployment baseline; Apple Metal compiler/metallib and a macOS-only Objective-C test executable.

---

## Authority, assumptions, and execution boundary

Read [the design](../specs/2026-09-08-metal-msl-output-design.md) first. The user
approved native Cg IR lowering, a documented vertex/fragment subset, and macOS
first. MSL 2.0, the macOS 13 qualification baseline, the explicit 16-byte-slot
uniform ABI, and the detailed allowlist are concrete planning decisions recorded
in that design. They must be verified by task 1 before implementation depends
on them. Do not silently upgrade the target when a compiler rejects a probe.

This file is an implementation sequence and acceptance contract, not a claim
that a backend or tests already exist. The code blocks specify executable
fixtures, algorithms, and interfaces to implement; completing the lowerers
requires the corresponding Cg IR case handling described in each task.

Implement sequentially in an isolated development worktree. This documentation
was written in the user's existing checkout; do not reuse or clean its build
directories. At planning time `build-win32/Testing/Temporary/LastTest.log` was
modified and `build-hlsl/` was untracked. Neither belongs to this work.
Preserve NVIDIA notices in source files and keep generated parser files unchanged.

The tasks are dependency-ordered milestones. Within each milestone, introduce
one fixture/case at a time, demonstrate its current failure, implement it,
then rerun the focused test. Do not treat a complete backend milestone as a
single small edit. Commit only the files belonging to the passing milestone.

## File ownership

| Files | Responsibility |
| --- | --- |
| `msl_hal.h`, `msl_hal.c`, `mslv_hal.c`, `mslf_hal.c` | Profiles, stage descriptors, HAL integration and diagnostics |
| `msl_ir.h`, `msl_ir.c` | MSL types, expressions, statements, declarations, resources, arena builders |
| `msl_validate.c`, `msl_verify.c` | Source capability checks and target structural checks, respectively |
| `msl_bind.h`, `msl_bind.c` | Semantic canonicalization, slot assignment, uniform wire layouts |
| `msl_lower_internal.h`, `msl_lower.c`, `msl_lower_support.c` | Private context, orchestration, names, allocation and failure recording |
| `msl_lower_decl.c`, `msl_lower_expr.c`, `msl_lower_stmt.c` | Typed source declarations, expressions, structured control flow |
| `msl_lower_function.c`, `msl_lower_aggregate.c`, `msl_lower_texture.c` | Reachable helpers/call ABI, copies/matrices/arrays, sampling |
| `msl_codegen.c` | Source and ABI-comment serialization from verified MSL nodes |
| `cgcmain.c`, `CMakeLists.txt`, `errors.h` | Registration, build wiring, dedicated diagnostic definitions |
| `tests/msl.cmake`, `tests/CMakeLists.txt` | New test registrations; existing test file includes the new one |
| `tests/run_msl_test.cmake`, `tests/validate_msl.cmake` | Compiler assertions and Apple compiler validation |
| `tests/msl_ir_test.c`, `tests/msl_bind_test.c`, `tests/msl_lower_test.c` | Unit tests for builders, ABI and lowering invariants |
| `tests/msl_runtime.m`, `tests/run_msl_runtime.cmake` | macOS pipeline creation and numerical rendering qualification |
| `tests/msl/` | Cg fixtures, expected source, diagnostics and Apple syntax probes |
| `docs/msl-compatibility.md`, `docs/msl-qualification.md`, `README.md` | User contract, recorded validation evidence, usage |

Read-only references: `cg_ir.h`, `cg_ir_lower.c`, `cg_reach.*`, `cg_stdlib.*`,
`cg_types.*`, `glsl_lower_ir_expr.c`, `glsl_lower_ir_stmt.c`,
`glsl_lower_aggregate.c`, `glsl_hal.c`, `hlsl_bind.c`, `output.*`.
Do not link the new backend against GLSL/HLSL internals. If a frontend invariant
blocks valid Cg, isolate its reproduction and fix separately with its own
cross-backend regression evidence.

## Common build and test commands

Run these commands from the implementation worktree. CMake chooses the local
generator; `--config Release` also works with single-config generators.

```text
cmake -S . -B build-msl -DBUILD_TESTING=ON -DCMAKE_BUILD_TYPE=Release
cmake --build build-msl --config Release
ctest --test-dir build-msl -C Release --output-on-failure
ctest --test-dir build-msl -C Release -R "^msl_" --output-on-failure
```

CTest commands in tasks refer to test names that the task must register.
The Windows multi-config executable is `build-msl/Release/cgc.exe`;
single-config Unix builds normally use `build-msl/cgc`. Test scripts receive
`$<TARGET_FILE:cgc>` rather than guessing either path. Give each test a distinct
`${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/msl/<test-name>` output directory.

## Task 1: Freeze the baseline and prove the Apple target contract

**Files:** Create `tests/capture_msl_baseline.cmake`,
`tests/msl/probes/interface.metal`, `tests/msl/probes/resources.metal`,
`docs/msl-qualification.md`. Read `tests/CMakeLists.txt` and `output.c`.

- [ ] Build the unchanged compiler in a fresh baseline build and run existing
  CTest. Save its test list, results, compiler versions, and available external
  validators. Record failures as baseline failures rather than concealing them.
- [ ] Write the baseline script to loop over `position`, `reflection`,
  `vertexlight`, `vertexlight4` and language versions `1.1`, `2.0`; execute
  `cgc -quiet -version <version> -profile generic <absolute-source>`, saving
  stdout, stderr, and process status separately. Compare those same invocations
  after implementation. Normalize only CRLF/LF, not diagnostic contents.
- [ ] Add this independent Apple interface probe, including both stages:

```metal
#include <metal_stdlib>
using namespace metal;
struct Input { float4 p [[attribute(0)]]; };
struct Varyings {
    float4 p [[position]];
    float2 uv [[user(cg_TEXCOORD0)]];
};
vertex Varyings probe_v(Input x [[stage_in]]) {
    Varyings r; r.p = x.p; r.uv = x.p.xy; return r;
}
fragment float4 probe_f(Varyings x [[stage_in]]) {
    return float4(x.uv, 0.0f, 1.0f);
}
```

- [ ] Add a resources probe with `constant Uniforms &u [[buffer(0)]]`, where
  `Uniforms` contains two float4 fields; sample a texture2d<float> with an
  independent sampler. Add entry variants for texturecube<float>, explicit
  `level(lod)`, flat integer user fields, `depth(any)`, and `thread float &`
  helper formals. Use these variants to exercise the exact spellings in the
  design, including helper resource parameters.
- [ ] On macOS, inspect the installed compiler help and run:

```text
xcrun --sdk macosx --find metal
xcrun --sdk macosx metal -std=metal2.0 -mmacosx-version-min=13.0 -c tests/msl/probes/interface.metal -o interface.air
xcrun --sdk macosx metallib interface.air -o interface.metallib
```

  Expect exit 0 and newly created nonempty artifacts. If the installed tool
  requires the older `-std=macos-metal2.0` spelling, record and configure that
  exact spelling after the same probe passes. Both spellings request 2.0;
  accepting an unversioned default or a newer language is not equivalent.
  Prove a deliberately invalid probe fails. Keep artifacts in the build tree.
- [ ] Record selected SDK, compiler version, standard spelling and deployment
  target in qualification documentation. On a Windows-only implementation
  session, record Apple validation as unavailable; portable work may proceed,
  but completion cannot claim Apple qualification.
- [ ] Commit: `Record Metal target and regression baseline`.

## Task 2: Establish test infrastructure and typed MSL storage

**Files:** Create `msl_ir.h`, `msl_ir.c`, `msl_verify.c`,
`tests/msl_ir_test.c`, `tests/run_msl_test.cmake`, `tests/msl.cmake`;
modify both CMake entry points.

- [ ] Define enums for vertex/fragment stage, bool/int/uint/float bases,
  thread/constant address spaces, declaration role, interpolation, and
  supported expression/statement kinds. Define `MslType`, `MslDecl`,
  `MslExpr`, `MslStmt`, `MslFunction`, `MslModule`, and
  `MslDiagnostic` with typed references and SourceLoc. Resources and uniform
  wire declarations must be distinct from ordinary numeric values.
- [ ] Implement arena ownership using an allocator callback, as in Cg IR.
  Each builder zeroes the node and latches allocation failure; later builders
  return NULL. Add dirty-memory and fail-on-Nth-allocation tests before builders.
- [ ] Add verifier tests for a missing entry, invalid stage, expression/type
  mismatch, unresolved symbol/call, duplicate emitted names, invalid storage
  qualifier and resource-as-numeric-value. Source-located diagnostics distinguish
  internal target inconsistency from an unsupported source construct.
- [ ] Implement the compiler runner around this process invocation:

```cmake
execute_process(
    COMMAND "${CGC}" -quiet -profile "${PROFILE}" -entry "${ENTRY}"
            -o "${OUTPUT}" ${EXTRA_ARGS} "${SOURCE}"
    RESULT_VARIABLE status OUTPUT_VARIABLE out ERROR_VARIABLE err)
if(EXPECT_SUCCESS)
    if(NOT status EQUAL 0 OR NOT EXISTS "${OUTPUT}")
        message(FATAL_ERROR "Compilation failed: ${status}\n${out}${err}")
    endif()
else()
    if(status EQUAL 0)
        message(FATAL_ERROR "Expected rejection but compilation succeeded")
    endif()
endif()
```

  Add required-argument checks, exact expected source comparisons and separate
  diagnostic code/reason/location checks. Strip only known existing volatile
  version/command-line comments for snapshots; preserve MSL/ABI content.
  Do not delete arbitrary caller paths: derive OUTPUT from the per-test build
  directory and remove only that test's exact previous output.
- [ ] Run `ctest --test-dir build-msl -C Release -R "^msl_ir_" --output-on-failure`.
  Expect all new positive and malformed-module cases to pass. Include a runner
  self-test showing a wrong expected output and wrong expected status fail.
- [ ] Commit: `Add typed Metal module and validation harness`.

## Task 3: Register profiles and emit a minimal working shader pair

**Files:** Create the four `msl*hal*` files, `msl_validate.c`,
`msl_lower_internal.h`, `msl_lower.c`, `msl_lower_support.c`,
`msl_lower_decl.c`, `msl_lower_expr.c`, `msl_lower_stmt.c`,
`msl_codegen.c`, `tests/msl/profile/vertex.cg`,
`tests/msl/profile/fragment.cg`; modify `cgcmain.c`, `errors.h`, CMake wiring.

- [ ] Introduce these failures before registering the profiles:

```c
// tests/msl/profile/vertex.cg
float4 main(float4 p : ATTRIB0) : POSITION { return p; }
```

```c
// tests/msl/profile/fragment.cg
float4 main() : COLOR0 { return float4(1, 0, 0, 1); }
```

  Current compiler must reject `mslv`/`mslf` as unknown profiles.
- [ ] Reserve IDs 23/24 and connector IDs 36..39 after checking current headers.
  Register `SetProfileIdentity("mslv", CG_PROFILE_STAGE_VERTEX, "vs", 10)`
  and `SetProfileIdentity("mslf", CG_PROFILE_STAGE_FRAGMENT, "ps", 10)`.
  Unit-test exact/stage/open selector identities using the existing profile
  identity test pattern; verify profile macros with minimal shader fixtures.
  Full emitted helper-overload tests follow in task 6.
- [ ] Define public backend operations consistently:

```c
int MslInitHAL(slHAL *hal, const MslProfileDesc *profile);
int MslValidateCgIR(const MslProfileDesc *profile,
    const CgIRModule *source, MslDiagnostic *diagnostic);
int MslLowerCgIR(MslModule *module, const MslProfileDesc *profile,
    const CgIRModule *source);
int MslVerifyModule(const MslModule *module, MslDiagnostic *diagnostic);
int MslWriteModule(FILE *out, const MslModule *module);
```

  `MslProfileDesc` belongs to `msl_hal.h`; target types belong to `msl_ir.h`.
  Store descriptors in HAL localData; expose no frontend AST emission path.
- [ ] Implement `ValidateIR` as capability check, scratch lowering, target
  verification, and diagnostic translation before freeing scratch storage.
  Implement `GenerateIR` as the same lowering/verification followed by writing.
  `PrintCodeHeader` must not emit target declarations before validation.
  Use an explicit Cg 1.1 rejection in profile initialization so -nocode cannot
  bypass it. Initially reject every construct beyond these fixtures honestly.
- [ ] Define Metal diagnostics in the currently unused 6600 range: 6600 language,
  6601 type, 6602 operation/intrinsic, 6603 interface, 6604 binding, 6605 limit,
  6606 stage, 6607 recursion, 6608 writable global. Recheck collisions first.
  Internal target verifier failures use the compiler's internal-error route.
- [ ] Define `CGC_MSL_LOWER_SOURCES` before tests are configured and reuse the
  same source list in cgc and lowering tests. Keep test unit targets linked
  to the actual implementation rather than test-only substitutes.
- [ ] Compile both fixtures, verify exported entry names and attributes, and
  run `^msl_profile_` tests. Test invalid profile, Cg 1.1, -nocode rejection,
  and a non-main entry. Capture hand-reviewed full expected source files.
- [ ] Commit: `Add Metal vertex and fragment profiles`.

## Task 4: Implement semantic interfaces and deterministic names

**Files:** Create `msl_bind.h`, `msl_bind.c`, `tests/msl_bind_test.c`;
modify HAL descriptors, declaration lowering, verifier and writer;
create `tests/msl/interfaces/` fixtures.

- [ ] Port only the semantic rules specified in the design: case folding,
  implicit zero suffix and four named aliases. Test duplicate detection after
  canonicalization, not only literal duplicate strings.
- [ ] Reserve explicit ATTRIB0..15 first, then sort other named inputs and
  allocate lowest free slots. Unit-test deterministic results when source
  declaration order changes and collision/overflow detection.
- [ ] Flatten numeric struct leaves, reconstruct source inputs, and collect
  return plus out/inout outputs. Map POSITION, COLOR0 and DEPTH to their
  built-ins; map permitted interstage fields to canonical user attributes.
  Enforce float4 position/color, scalar float depth, flat integer varyings,
  and matching interpolation in both stages.
- [ ] Give names stable escaped source spellings plus deterministic ordinal
  disambiguation in separate namespaces. Test source identifiers named
  `vertex`, `fragment`, `thread`, `metal`, and generated-name collisions.
- [ ] Add a shader pair where the vertex declares TEXCOORD1 before TEXCOORD0
  and the fragment declares them in the opposite order. Expected emitted user
  identities still match. Add rejection fixtures for absent POSITION,
  COLOR1 fragment output, duplicate HPOS/POSITION, matrix stage leaves and
  unsupported system semantics.
- [ ] Exercise limits at 16/17 attributes and 64/65 user components; use
  internal binding tests for the latter when the semantic surface itself
  cannot express that many independent leaves. Register `msl_interface_*`.
- [ ] Commit: `Define Metal stage interface bindings`.

## Task 5: Lower numeric expressions and structured control flow

**Files:** Modify expression/statement/declaration lowerers, source validator,
target verifier and writer; add `tests/msl/expressions/`,
`tests/msl/statements/`, `tests/msl/diagnostics/`.

- [ ] Build the numeric type table from canonical types. Map half/fixed to
  float deliberately; reject runtime double/long/narrow integers. Add tests
  for each family, vector width, literal suffix, and signedness conversion.
- [ ] Handle every admitted CgIR expression kind explicitly. Reject unsupported
  kinds instead of emitting fallback text. Parenthesize expressions to preserve
  AST precedence. Preserve source conversions and scalar/vector result types.
- [ ] Lower declarations, blocks, if, while, do, for, return, break and continue.
  Preserve loop-condition and iteration-side-effect statement placement; do
  not move a for-loop increment before a continue incorrectly. Keep short
  circuit and conditional-branch effects under their controlling branch.
- [ ] Add and run this concrete behavior fixture:

```c
float4 main(float4 p : ATTRIB0) : POSITION {
    int sum = 0;
    for (int i = 0; i < 4; i++) {
        if (i == 1) continue;
        sum += i;
    }
    int k = 0;
    do { k++; } while (k < 2);
    while (k < 4) { k++; if (k == 3) break; }
    return p + float4(sum, k, 0, 0);
}
```

  Expected added vector is (5,3,0,0). Verify source now and add it to the
  numerical runtime corpus in task 11. Add side-effecting logical operands,
  vector comparisons, overlapping swizzle writes and early returns.
- [ ] Implement the design's numeric intrinsic allowlist by resolved intrinsic
  ID plus argument/result signature. Explicit translations include frac ->
  fract, lerp -> mix, saturate -> clamp(x,0,1), and fragment ddx/ddy -> dfdx/dfdy.
  Generate typed constants; do not accidentally choose integer clamp overloads.
  Reject every other signature with C6602. Matrix cases arrive in task 7.
- [ ] Run `^msl_(type|expression|statement|intrinsic)_` tests, including
  unsupported-signature and vertex-derivative rejection, then existing CTest.
- [ ] Commit: `Lower Metal numeric expressions and control flow`.

## Task 6: Preserve helper calls and out/inout semantics

**Files:** Create `msl_lower_function.c`; modify lowering context, expression
lowerer, verifier and writer; create `tests/msl/functions/`.

- [ ] Traverse resolved reachable functions, assign deterministic helper names,
  detect call-graph cycles, and order definitions or prototypes. Unreachable
  unsupported code must not trigger a backend capability error; ordinary
  frontend errors remain frontend errors.
- [ ] Adapt `tests/cg20/overloads/profile.cg` to add exact mslv/mslf overloads;
  verify exact > vs/ps > open selection with distinct emitted/runtime values.
  Do not add an `msl*` selector: the existing registration API has one stage
  wildcard per profile, and a new family is outside this plan's scope.
- [ ] Lower input formals by value. For out/inout, evaluate target addresses
  once, allocate thread-local temporaries, copy in for inout, call with
  references to temporaries, and copy out in the order required by normalized
  Cg IR/frontend semantics. Inspect `cg_ir_lower.c` before adding copies so
  already-normalized copy operations are not duplicated.
- [ ] Add this swap fixture and verify the materialized call/write ordering:

```c
void swap_values(inout float a, inout float b) {
    float saved = a; a = b; b = saved;
}
float4 main(float4 p : ATTRIB0) : POSITION {
    swap_values(p.x, p.y);
    return p;
}
```

  Input (1,2,3,1) must yield (2,1,3,1). Add array-index side effects, swizzle
  actuals, nested calls, forward declarations, and same-name overloads.
  If overlapping copy-out order is unspecified by Cg, test preservation of
  the existing normalized sequence rather than inventing a language promise.
- [ ] Propagate hidden dependency parameters in a deterministic transitive
  analysis. Numeric uniforms and texture pairs are introduced by later tasks;
  their dependencies must use this mechanism, not new global shader variables.
- [ ] Run `^msl_function_` and register `msl_function_recursion_rejected` plus
  `msl_function_unreachable_unsupported`. Expect recursion C6607, exit nonzero,
  and no installed output.
- [ ] Commit: `Preserve Metal helper and parameter semantics`.

## Task 7: Implement structs, arrays, and Cg matrix semantics

**Files:** Create `msl_lower_aggregate.c`; modify types, lowerers and writer;
add `tests/msl/aggregates/`, `tests/msl/matrices/`.

- [ ] Represent fixed array values by generated wrapper structs with an array
  field; translate indexing to that field. Lower copies recursively by value,
  with RHS temporaries for overlaps. Topologically order nested struct/wrapper
  declarations. Reject resource-containing aggregates and unsized arrays.
- [ ] Map square float matrices to MSL column matrices while preserving Cg
  row operations. For source element (r,c), emit target access `[c][r]`.
  Rebuild row reads as vectors; scatter row writes from a materialized vector.
- [ ] Use these explicit matrix translation rules in implementation/tests:

```text
Cg construct rows (a,b,c,d) of float2x2 -> float2x2(float2(a,c), float2(b,d))
Cg M[row][col]                        -> M[col][row]
Cg A * B, matrices                   -> matrix(A[0]*B[0], A[1]*B[1], ...)
Cg mul(A,B), matrices                -> A * B
Cg mul(A,v)                          -> A * v
Cg mul(v,A)                          -> v * A
Cg mul(v,w), vectors                 -> dot(v,w)
```

  Here the repeated columns are generated from the known matrix dimension,
  never emitted as literal ellipses. Apply component-wise scalar lifting where
  needed for other Cg matrix operators and casts.
- [ ] Add a constant matrix with rows (1,2) and (3,4) and vector (5,6).
  `mul(M,v)` must yield (17,39); `mul(v,M)` must yield (23,34).
  `M[0]` must yield (1,2). Use non-symmetric, non-identity data in runtime tests.
  Test 2x2/3x3/4x4 constructors, transpose, row/column selectors, partial writes,
  aliasing selectors, matrix element increments and constructor side effects.
- [ ] Adapt the matrix selector shapes in `reflection.cg` without changing
  that source. Once uniforms are available in task 8, it must compile directly.
- [ ] Run `^msl_(aggregate|matrix)_` tests and target verifier tests.
- [ ] Commit: `Preserve Cg aggregate and matrix behavior in Metal`.

## Task 8: Implement the uniform ABI and metadata

**Files:** Modify `msl_bind.*`, declaration/function/aggregate lowerers and
writer; add `tests/msl/uniforms/` and ABI unit cases.

- [ ] Add layout tests before implementation. Use 16-byte slots exclusively;
  never compute wire offsets using host `sizeof(Type)` or native struct packing.

```text
layout(scalar or vector) = one 16-byte float4/int4/uint4 slot
layout(floatNxN)         = N float4 slots, column_stride=16
layout(array[T,N])       = concatenate layout(T), N times
layout(struct)          = concatenate members in source order
buffer size             = 16 * total slot count
```

  Reject arithmetic overflow before multiplication/addition. Test an array
  whose computed size overflows and the 4096/4112-byte policy boundary.
- [ ] Sort top-level uniforms by qualified source name, preserve member/index
  order and generate flat wire fields. Map bool to uint lanes and half/fixed
  to float lanes. Generated decoders rebuild source values, including matrix
  columns and nested arrays, using the same binding table used for metadata.
  Retain all entry uniform formals and only globals referenced by reachable
  code. Use bare parameter names and `global.<name>` paths so source shadowing
  cannot collide; add unused-helper and shadowed-global layout tests.
- [ ] Bind one constant buffer at buffer(0), propagate it to helpers that need
  it, and emit no empty buffer. Writes to source uniform/global storage must
  fail C6608; inout copies of readable values remain thread-local.
- [ ] Use this exact layout witness:

```c
struct Params { float3 tint; float exposure; float3x3 basis; };
float4 main(float4 p : ATTRIB0, uniform Params params) : POSITION {
    float3 rgb = mul(params.basis, params.tint) * params.exposure;
    return p + float4(rgb, 0);
}
```

  Expected offsets: tint=0, exposure=16, basis=32; basis column stride=16;
  total buffer size=80, alignment=16. Test CPU-packed values and shader reads
  using this exact witness in task 11. Padding lanes cannot affect the result.
- [ ] Emit every ABI record described by the design, including original paths,
  exported entry, type promotion, buffer bytes, leaf offsets, matrix stride,
  attributes, varyings, resources and defaults. Format literals with enough
  precision to round-trip the supported value. Defaults remain host-applied.
- [ ] Add deterministic repeated-run tests and reordered unrelated declaration
  tests. Reject unsupported register/pragma bindings explicitly; do not
  reinterpret DirectX register numbers as Metal buffer offsets.
- [ ] Compile all four root example shaders under mslv and retain hand-reviewed
  snapshots. Run `^msl_(uniform|metadata|example)_` and the generic baseline
  comparison. Expected: all examples succeed and existing generic bytes/status
  match the captured baseline after line-ending normalization only.
- [ ] Commit: `Add deterministic Metal uniform layout and reflection comments`.

## Task 9: Split Cg samplers and lower texture operations

**Files:** Create `msl_lower_texture.c`; modify binding, helper lowering,
validator and writer; add `tests/msl/textures/`.

- [ ] Reserve explicit TEXUNIT indices before deterministic allocation of
  unbound samplers. Emit paired texture(N)/sampler(N) parameters. Keep the
  resource object identities distinct even when source names are similar.
- [ ] Lower this fixture with a texture/sampler pair passed through the helper:

```c
float4 sample_image(sampler2D image, float2 uv) { return tex2D(image, uv); }
float4 main(float2 uv : TEXCOORD0,
            uniform sampler2D image : TEXUNIT3) : COLOR0 {
    return sample_image(image, uv);
}
```

  Expect texture(3), sampler(3), both helper arguments, and `.sample(s, uv)`.
  Add a two-level helper call so transitive propagation is exercised.
- [ ] Support tex2D/texCUBE implicit fragment sampling and tex2Dlod/texCUBElod
  in either stage. For tex2Dlod use coordinate.xy and coordinate.w as LOD;
  for cube LOD use coordinate.xyz and coordinate.w. Evaluate the coordinate
  expression once before extracting multiple fields.
- [ ] Reject ordinary vertex sampling, projected/bias/gradient variants,
  sampler1D/3D/RECT, shadow/comparison forms, arrays, dynamic selection,
  sampler outputs, duplicate TEXUNIT, and slot 16. Verify 16 bindings succeed.
- [ ] Add 2D checkerboard and six-face cube runtime witnesses; use nearest
  sampling and exact texel centers so filtering does not obscure correctness.
  Add explicit nonzero LOD data distinct from the base level.
- [ ] Run `^msl_texture_`; verify rejection code 6602/6604/6605 as appropriate,
  deterministic paired metadata and no partial files.
- [ ] Commit: `Lower Metal texture and sampler resources`.

## Task 10: Harden diagnostics, output failure, and module boundaries

**Files:** Modify source/target validators and HAL adapters;
create `tests/msl/diagnostics/` cases, `tests/check_msl_output_transaction.cmake`,
`tests/check_msl_structure.cmake`; modify `tests/msl.cmake`.

- [ ] Inventory every CgIR expression/statement enum against validator and
  lowerer switches. Ensure no admitted case reaches an unimplemented writer
  branch; unsupported source kinds must fail with their source location.
- [ ] Exercise unsupported type in a reachable nested helper, recursion,
  geometry operations, writable globals and unsupported intrinsic signatures.
  Check diagnostic code, source filename/line, reason and existing call-path
  notes. Test matching -nocode rejection and unreachable backend-only cases.
- [ ] Seed an output file with `KEEP_EXISTING_OUTPUT`, compile each failure
  using -o, and assert the sentinel is unchanged. For a previously absent
  output assert it remains absent. Inspect the exact test directory for leaked
  cgc temporary files. Add a bad destination test and injected writer/allocation
  failures to ensure failed generation never reports success.
- [ ] Verify stdout has no target declarations/ABI records on validation
  failure. Do not mistake unavoidable partial stdout on an actual I/O failure
  for a transaction guarantee; assert error status in that case.
- [ ] Add a structural check forbidding frontend `expr`/`stmt` traversal in
  the MSL lowerers and GLSL/HLSL/ARB dependencies. It must also verify the
  production source list is shared with the actual lowering test target.
  Add a self-test proving the check detects a deliberately invalid specimen.
- [ ] Run `^msl_(diagnostic|transaction|structure|ir)_` followed by full CTest.
- [ ] Commit: `Enforce Metal lowering and output failure contracts`.

## Task 11: Require Apple compilation and real pipeline execution

**Files:** Create `tests/validate_msl.cmake`, `tests/msl_runtime.m`,
`tests/run_msl_runtime.cmake`; modify CMake files and qualification records.

- [ ] Add `CGC_REQUIRE_METAL` and `CGC_REQUIRE_METAL_RUNTIME`, both default OFF.
  Discover xcrun/metal/metallib and prove the configured standard/deployment
  flags with task 1's probe. An unavailable tool can disable optional tests;
  either strict option requiring it must fail configuration. Runtime strictness
  implies compiler strictness. Enable Objective-C only on APPLE for the runtime
  test target; keep cgc C90 and Windows build dependencies unchanged.
- [ ] Register Apple compilation for every positive fixture in the same
  manifest used for portable source tests. Compile fresh generated source to
  fresh AIR, then metallib; require exit 0 and nonempty current outputs.
  Delete only those test-owned outputs before invocation. Record tool version,
  exact target flags, fixture, profile, and resulting artifact in test evidence.
  Add invalid-MSL and stale-artifact self-tests that must fail.
- [ ] Build `msl_runtime` against Foundation and Metal. Accept vertex/fragment
  library paths, exported names and a fixture identifier as arguments. Load
  both functions, construct an MTLVertexDescriptor from expected attribute
  slots, bind vertex data at buffer(1), reserve buffer(0) for numeric uniforms,
  and create a pipeline with the required color/depth formats.
- [ ] Render a small offscreen target; wait for completion, check command-buffer
  errors, and read back results. Use independently constructed CPU data and
  expected values rather than computing expectations with the new backend.
  An unavailable device exits 77 only in optional mode; strict mode must fail.
  Do not mark strict qualification passed when CTest reports a skip.
- [ ] Cover: reordered matching varyings; matrix multiply/index witnesses;
  80-byte uniform struct; swap/out parameters; loops; 2D/cube/LOD textures;
  fragment discard retaining the clear color; fragment depth; and integer flat
  interpolation. For arithmetic witnesses use RGBA32Float, absolute/relative
  tolerance 1e-5, finite inputs, and pixel samples away from primitive edges.
  For normalized color texture witnesses use exact bytes or <=1 LSB tolerance.
- [ ] Create a mismatched varying-type pair and require pipeline creation to
  fail; separately prove a matching pair succeeds. Validate bundled vertex
  shaders with a companion fragment accepting their emitted COLOR0, and render
  fully initialized dedicated fixtures for numerical comparisons.
- [ ] Run strict qualification on macOS:

```text
cmake -S . -B build-msl-apple -DBUILD_TESTING=ON -DCMAKE_BUILD_TYPE=Release -DCGC_REQUIRE_METAL=ON -DCGC_REQUIRE_METAL_RUNTIME=ON
cmake --build build-msl-apple --config Release
ctest --test-dir build-msl-apple -C Release -R "^msl_" --output-on-failure
```

  Expected: all registered MSL tests pass, zero unavailable-oracle/runtime
  skips. Missing tools, invalid generated source, library failures or device
  failures block release qualification. Record the tested SDK/OS/GPU; do not
  claim untested hardware or iOS coverage.
- [ ] Commit: `Qualify Metal output with Apple compiler and runtime`.

## Task 12: Publish the compatibility contract and final evidence

**Files:** Create `docs/msl-compatibility.md`; update `README.md`,
`docs/msl-qualification.md`, and this plan's completion checkboxes.

- [ ] Document exactly the design's type, intrinsic, texture and stage
  allowlists with a success/rejection fixture name for every feature family.
  Explain half/fixed promotion, Cg matrix behavior, fixed policy limits,
  unsupported Cg 1.1, and macOS-only qualification.
- [ ] Include the uniform byte-layout witness, per-stage buffer(0) reservation,
  vertex-buffer slot separation, TEXUNIT pairing, default-value handling,
  entry-name mapping, metadata ABI version and coordinate conventions.
  Provide CPU upload examples using explicit byte offsets and column-major
  matrix packing; do not use an unverified host struct layout.
- [ ] Show source compilation and the separate Apple toolchain commands:

```text
cgc -quiet -version 2.0 -profile mslv -entry main -o position.metal position.cg
cgc -quiet -version 2.0 -profile mslf -entry main -o fragment.metal tests/msl/profile/fragment.cg
xcrun --sdk macosx metal -std=metal2.0 -mmacosx-version-min=13.0 -c position.metal -o position.air
xcrun --sdk macosx metallib position.air -o position.metallib
```

  Use the standard spelling actually verified in task 1 if it differs.
  State that the library export is `cg_mslv_main`, not necessarily `main`.
- [ ] Rerun full existing CTest on the implementation host and compare all
  eight generic example/version baseline records. Repeat Apple qualification
  only if subsequent compiler/test changes invalidate its evidence.
- [ ] Inspect diff for unrelated files, generated parser changes, binary
  artifacts, missing notices, and machine-specific paths. Review module
  boundaries, ABI consistency and the feature-to-test mapping before merging.
- [ ] Record command results and genuine limitations in qualification docs.
  A Windows-only green run is portable validation, not release qualification.
  Summarize affected stage, behavior, and representative bindings in the PR.
- [ ] Commit: `Document Metal shader compatibility and qualification`.

## Completion criteria

- [ ] Both profiles emit valid standalone MSL for the agreed subset.
- [ ] All four bundled vertex shaders translate without source modifications.
- [ ] Numeric semantics, resources and stage interfaces have independent tests.
- [ ] Unsupported programs have stable located profile diagnostics and failed
  -o compilations preserve existing files.
- [ ] Existing tests and generic baseline comparisons show no new regressions.
- [ ] Strict Apple compilation and runtime qualification passed with recorded
  toolchain/platform details and no missing-tool/device skips.
- [ ] Documentation explains the supported surface and application ABI completely.

## Review and handoff

Tasks 1-3 establish a small, working vertical slice; tasks 4-9 expand it to the
approved subset; tasks 10-12 harden and qualify it. Do not advertise the final
subset after only the vertical slice. No implementation is authorized by the
creation of this plan alone: continue when the user asks to implement it.

The written spec and this plan are ready for review. Inline execution is the
default continuation; delegated execution remains an explicit user choice.

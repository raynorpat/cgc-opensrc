# DirectX 10/11 HLSL Shader Model 4/5 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add deterministic source-emitting `hlslv40`, `hlslg40`, `hlslf40`, `hlslv50`, `hlslg50`, and `hlslf50` profiles for the complete representable Cg 2.0 vertex, geometry, and pixel surface.

**Architecture:** Build on the completed DirectX 9 HLSL implementation by evolving its typed HLSL backend into a stage- and shader-model-aware pipeline. Six thin profile HALs select shared modern semantic, resource, legalization, geometry, validation, and code-generation policies; verified Cg IR remains the only source of program meaning.

**Tech Stack:** C90-compatible C, CMake/CTest, existing Cg and HLSL IRs, PowerShell test commands, optional Microsoft `fxc.exe` validation.

| Public profile | Stage | Exact compiler target |
| --- | --- | --- |
| `hlslv40` | vertex | `vs_4_0` |
| `hlslg40` | geometry | `gs_4_0` |
| `hlslf40` | pixel | `ps_4_0` |
| `hlslv50` | vertex | `vs_5_0` |
| `hlslg50` | geometry | `gs_5_0` |
| `hlslf50` | pixel | `ps_5_0` |

Scope guardrails: emit standalone HLSL source only. Do not add a profile
version selector, DXBC/runtime integration, hull/domain/compute stages,
tessellation, UAVs, structured or byte-address buffers, multiple geometry
streams, geometry instancing, SM4.1, SM5.1, or Shader Model 6 features. `fxc`
remains an optional test oracle, never a compiler dependency.

---

## Required Baseline

This follow-on starts from the completed DirectX 9 HLSL implementation now on
`master`. Before changing it, verify that the full Debug and Release suites
pass and that the following production files remain present:

```text
hlsl_ir.h          hlsl_ir.c
hlsl_lower.c       hlsl_legalize.c
hlsl_bind.c        hlsl_validate.c
hlsl_codegen.c     tests/hlsl_ir_test.c
docs/hlsl-sm3-compatibility.md
```

The design source of truth is
`docs/superpowers/specs/2026-08-28-directx10-11-hlsl-sm4-sm5-design.md`.

## File Responsibility Map

### New production files

- `hlsl_modern.h`: public DX10+ semantic, topology, resource, interpolation,
  and texture-method policy interfaces.
- `hlsl_modern.c`: tables and pure policy queries shared by SM4 and SM5.
- `hlslv40_hal.c`, `hlslg40_hal.c`, `hlslf40_hal.c`: SM4 descriptor and
  connector definitions only.
- `hlslv50_hal.c`, `hlslg50_hal.c`, `hlslf50_hal.c`: SM5 descriptor and
  connector definitions only.

### Existing production files modified after the prerequisite

- `hlsl_hal.h`, `hlsl_hal.c`: unique IDs, model-aware descriptors,
  registration, shared initialization, and transactional orchestration.
- `hlsl_ir.h`, `hlsl_ir.c`: modern resources, system semantics,
  interpolation, geometry layouts, append/restart, and flat replay.
- `hlsl_lower.c`: verified Cg IR lowering into those target nodes.
- `hlsl_legalize.c`: modern texture methods, ABI conversions, and
  stage/model-specific rewrites.
- `hlsl_bind.c`: modern wrappers, interface keys, cbuffer packing, resource
  pairs, and binding metadata.
- `hlsl_validate.c`: structural, stage/model, semantic, resource, geometry,
  and limit validation.
- `hlsl_codegen.c`: deterministic modern declarations, methods, stream
  syntax, and wrappers.
- `CMakeLists.txt`: compile all six thin profile sources; existing startup
  continues to call the shared HLSL registration function.
- `errors.h`: modern HLSL diagnostics within the existing 6400–6499 block.
- `README.md`: public profile and usage documentation.

### Test files

- `tests/hlsl_ir_test.c`: all pure descriptor, policy, IR, binding, and
  verifier assertions.
- `tests/check_hlsl.cmake`: exact successful source runner with entry and
  repeated profile-option support.
- `tests/check_hlsl_failure.cmake`: exact diagnostic and zero-output runner.
- `tests/check_hlsl_link.cmake`: canonical two- and three-stage interface
  checker.
- `tests/validate_hlsl.cmake`: optional exact-target `fxc` runner.
- `tests/CMakeLists.txt`: fixtures, boundaries, external validation, and full
  suite registration.
- `tests/hlsl/modern/`: shared SM4/SM5 source fixtures and goldens.
- `tests/hlsl/geometry/`: geometry sources and per-target goldens.
- `tests/hlsl/link/`: matching and deliberately mismatched pipelines.
- `tests/hlsl/limits/`: exact-boundary and one-over fixtures.
- `docs/hlsl-sm4-sm5-compatibility.md`: closed compatibility manifest.

## Task 1: Prove the SM3 prerequisite and repair global identities

**Files:**
- Modify: `hlsl_hal.h`
- Modify: `hlsl_hal.c`
- Modify: `tests/hlsl_ir_test.c`
- Modify: `tests/CMakeLists.txt`

- [ ] **Step 1: Prove the completed SM3 backend exists**

Run:

```powershell
$requiredHlslFiles = @(
  'hlsl_ir.h','hlsl_ir.c','hlsl_lower.c','hlsl_legalize.c',
  'hlsl_bind.c','hlsl_validate.c','hlsl_codegen.c',
  'tests/hlsl_ir_test.c','docs/hlsl-sm3-compatibility.md'
)
$missingHlslFiles = $requiredHlslFiles | Where-Object { -not (Test-Path -LiteralPath $_) }
if ($missingHlslFiles.Count) { throw "Complete the SM3 plan first: $missingHlslFiles" }
rg -n "HlslSkeleton|Temporary HlslModule|Task 2 replaces" -g 'hlsl_*.c' -g 'hlsl_*.h' .
```

Expected: every required file exists and `rg` returns no skeleton marker. If
this gate fails, stop and execute the SM3 plan; do not emulate its missing
backend inside this follow-on.

- [ ] **Step 2: Capture the prerequisite regression baseline**

Run:

```powershell
git status --short
cmake -S . -B build-hlsl-modern
cmake --build build-hlsl-modern --config Debug
ctest --test-dir build-hlsl-modern -C Debug --output-on-failure
cmake --build build-hlsl-modern --config Release
ctest --test-dir build-hlsl-modern -C Release --output-on-failure
```

Expected: both complete suites pass before identity changes. Record the status
output in the task checkpoint; pre-existing unrelated paths are not part of
this plan and must not be staged, removed, or rewritten.

- [ ] **Step 3: Add failing identity uniqueness assertions**

In `tests/hlsl_ir_test.c`, add a table containing every profile and connector
ID through the new range and assert pairwise uniqueness:

```c
static void TestModernProfileIdentities(void)
{
    static const int profileIds[] = {
        PROFILE_GLSLG_ID, PROFILE_HLSLV_ID, PROFILE_HLSLF_ID,
        17, 18, 19, 20, 21, 22
    };
    static const int connectorIds[] = {
        CID_GLSLG_IN_ID, CID_GLSLG_OUT_ID,
        CID_HLSLV_IN_ID, CID_HLSLV_OUT_ID,
        CID_HLSLF_IN_ID, CID_HLSLF_OUT_ID,
        24, 25, 26, 27, 28, 29,
        30, 31, 32, 33, 34, 35
    };
    AssertUnique(profileIds, (int)(sizeof(profileIds) / sizeof(profileIds[0])));
    AssertUnique(connectorIds, (int)(sizeof(connectorIds) / sizeof(connectorIds[0])));
}
```

The literals are the reserved modern ranges. This first red test includes the
existing HLSL macros so it exposes the current collisions with `glslg` without
requiring production declarations that Task 3 has not added yet.

- [ ] **Step 4: Run the identity test and observe the collision**

Run:

```powershell
cmake --build build-hlsl-modern --config Debug --target hlsl_ir_unit
ctest --test-dir build-hlsl-modern -C Debug -R '^hlsl_ir_unit$' --output-on-failure
```

Expected: an assertion showing the existing
`PROFILE_HLSLV_ID == PROFILE_GLSLG_ID` collision and the corresponding input
connector collision.

- [ ] **Step 5: Install the stable identity table**

Replace the HLSL constants in `hlsl_hal.h` with:

```c
#define PROFILE_HLSLV_ID             15
#define PROFILE_HLSLF_ID             16
#define PROFILE_HLSLV40_ID           17
#define PROFILE_HLSLG40_ID           18
#define PROFILE_HLSLF40_ID           19
#define PROFILE_HLSLV50_ID           20
#define PROFILE_HLSLG50_ID           21
#define PROFILE_HLSLF50_ID           22

#define CID_HLSLV_IN_ID              20
#define CID_HLSLV_OUT_ID             21
#define CID_HLSLF_IN_ID              22
#define CID_HLSLF_OUT_ID             23
#define CID_HLSLV40_IN_ID            24
#define CID_HLSLV40_OUT_ID           25
#define CID_HLSLG40_IN_ID            26
#define CID_HLSLG40_OUT_ID           27
#define CID_HLSLF40_IN_ID            28
#define CID_HLSLF40_OUT_ID           29
#define CID_HLSLV50_IN_ID            30
#define CID_HLSLV50_OUT_ID           31
#define CID_HLSLG50_IN_ID            32
#define CID_HLSLG50_OUT_ID           33
#define CID_HLSLF50_IN_ID            34
#define CID_HLSLF50_OUT_ID           35
```

Change SM3 registration in `hlsl_hal.c` from neutral identities to:

```c
SetProfileIdentity(PROFILE_HLSLV_NAME, CG_PROFILE_STAGE_VERTEX, "vs", 10);
SetProfileIdentity(PROFILE_HLSLF_NAME, CG_PROFILE_STAGE_FRAGMENT, "ps", 10);
```

- [ ] **Step 6: Pin the exact values and rerun SM3 tests**

Replace the reservation literals in Step 3 with the production modern macros,
add direct assertions for every integer in Step 5, rebuild, and run:

```powershell
cmake --build build-hlsl-modern --config Debug --target hlsl_ir_unit cgc
ctest --test-dir build-hlsl-modern -C Debug -R 'hlsl|profile|geometry' --output-on-failure
```

Expected: identity tests pass; existing SM3 goldens remain byte-identical.

- [ ] **Step 7: Commit the prerequisite repair**

```powershell
git add -- hlsl_hal.h hlsl_hal.c tests/hlsl_ir_test.c tests/CMakeLists.txt
git commit -m "Repair HLSL profile identities"
```

## Task 2: Refactor the shared backend around stage/model policy

**Files:**
- Modify: `hlsl_ir.h`
- Modify: `hlsl_hal.h`
- Modify: `hlsl_hal.c`
- Modify: `hlslv_hal.c`
- Modify: `hlslf_hal.c`
- Modify: `hlsl_validate.c`
- Modify: `tests/hlsl_ir_test.c`

- [ ] **Step 1: Add failing descriptor tests**

Construct the two existing descriptors in `tests/hlsl_ir_test.c` and assert:

```c
assert(HlslProfile_hlslv.stage == HLSL_STAGE_VERTEX);
assert(HlslProfile_hlslv.model == HLSL_SHADER_MODEL_3);
assert(HlslProfile_hlslv.syntax == HLSL_SYNTAX_LEGACY);
assert(HlslProfile_hlslf.stage == HLSL_STAGE_PIXEL);
assert(HlslProfile_hlslf.model == HLSL_SHADER_MODEL_3);
assert(!HlslProfileHasCapability(&HlslProfile_hlslv, HLSL_CAP_GEOMETRY));
assert(HlslProfileHasCapability(&HlslProfile_hlslf, HLSL_CAP_DISCARD));
```

Run `hlsl_ir_unit`; expected: compile failure because the model, syntax, and
capability contracts do not exist.

- [ ] **Step 2: Define stable model and capability types**

Append `HLSL_STAGE_GEOMETRY` to the existing `HlslStage` enum in `hlsl_ir.h`;
do not move or renumber the two existing stage values. Add the remaining
policy types to `hlsl_hal.h`:

```c
typedef enum HlslShaderModel_Enum {
    HLSL_SHADER_MODEL_3 = 30,
    HLSL_SHADER_MODEL_4 = 40,
    HLSL_SHADER_MODEL_5 = 50
} HlslShaderModel;

typedef enum HlslSyntaxFamily_Enum {
    HLSL_SYNTAX_LEGACY,
    HLSL_SYNTAX_MODERN
} HlslSyntaxFamily;

typedef enum HlslSemanticPolicy_Enum {
    HLSL_SEMANTIC_POLICY_DX9,
    HLSL_SEMANTIC_POLICY_MODERN
} HlslSemanticPolicy;

typedef enum HlslResourcePolicy_Enum {
    HLSL_RESOURCE_POLICY_DX9,
    HLSL_RESOURCE_POLICY_MODERN
} HlslResourcePolicy;

#define HLSL_CAP_DISCARD          0x0001u
#define HLSL_CAP_DERIVATIVES      0x0002u
#define HLSL_CAP_GEOMETRY         0x0004u
#define HLSL_CAP_TEXTURE_METHODS  0x0008u
#define HLSL_CAP_CBUFFERS         0x0010u
```

Replace `HlslLimits` with this exact superset, preserving the first seven SM3
fields and appending modern limits:

```c
typedef struct HlslLimits_Rec {
    int inputs;
    int outputs;
    int floatConstants;
    int intConstants;
    int boolConstants;
    int samplers;
    int colorOutputs;
    int depthOutputs;
    int clipDistanceComponents;
    int constantBufferSlots;
    int constantBufferVectors;
    int resources;
    int geometryMaxVertices;
    int geometryTotalOutputComponents;
} HlslLimits;
```

Replace the existing `struct HlslProfileDesc_Rec` body with the exact ordered
contract below so all non-designated C90 initializers remain reviewable while
preserving the completed SM3 semantic tables and aliases:

```c
struct HlslProfileDesc_Rec {
    HlslStage stage;
    HlslShaderModel model;
    HlslSyntaxFamily syntax;
    HlslSemanticPolicy semanticPolicy;
    HlslResourcePolicy resourcePolicy;
    const char *name;
    const char *target;
    const char *version;
    int pid;
    int inputCid;
    int outputCid;
    ConnectorDescriptor *connectors;
    int numConnectors;
    const HlslSemanticDesc *inputSemantics;
    int numInputSemantics;
    const HlslSemanticAlias *inputAliases;
    int numInputAliases;
    const HlslSemanticDesc *outputSemantics;
    int numOutputSemantics;
    const HlslSemanticAlias *outputAliases;
    int numOutputAliases;
    ConnectorRegisters *inputRegs;
    int numInputRegs;
    ConnectorRegisters *outputRegs;
    int numOutputRegs;
    const HlslLimits *limits;
    unsigned int capabilities;
};
```

Add:

```c
int HlslProfileHasCapability(const HlslProfileDesc *profile,
                             unsigned int capability);
```

- [ ] **Step 3: Populate SM3 descriptors explicitly**

Rename the existing version literal to `VERSION_STRING_HLSL_SM3` and add
`VERSION_STRING_HLSL_SM4` (`DirectX 10 Shader Model 4`) and
`VERSION_STRING_HLSL_SM5` (`DirectX 11 Shader Model 5`). Set SM3 descriptors
to their SM3 version, `HLSL_SHADER_MODEL_3`, `HLSL_SYNTAX_LEGACY`, and exact
capability masks. Make `InitHAL_hlsl_profile` assign `fHAL->version` from the
descriptor. Do not select behavior by comparing `target` or version text.

- [ ] **Step 4: Make validation reject impossible descriptor combinations**

In `hlsl_validate.c`, reject a legacy descriptor with model 4/5, a modern
descriptor with model 3, geometry without `HLSL_CAP_GEOMETRY`, or an empty
target/version. Use the existing HLSL internal-verifier reason path.

- [ ] **Step 5: Prove refactoring leaves SM3 output unchanged**

Run:

```powershell
cmake --build build-hlsl-modern --config Debug --target cgc hlsl_ir_unit
ctest --test-dir build-hlsl-modern -C Debug -R '^hlsl|hlslv|hlslf' --output-on-failure
git diff -- tests/hlsl
```

Expected: focused tests pass and no SM3 golden changes appear.

- [ ] **Step 6: Commit the descriptor refactor**

```powershell
git add -- hlsl_ir.h hlsl_hal.h hlsl_hal.c hlslv_hal.c hlslf_hal.c hlsl_validate.c tests/hlsl_ir_test.c
git commit -m "Make HLSL descriptors shader-model aware"
```

## Task 3: Register six modern profiles and their thin descriptors

**Files:**
- Create: `hlslv40_hal.c`
- Create: `hlslg40_hal.c`
- Create: `hlslf40_hal.c`
- Create: `hlslv50_hal.c`
- Create: `hlslg50_hal.c`
- Create: `hlslf50_hal.c`
- Modify: `hlsl_hal.h`
- Modify: `hlsl_hal.c`
- Modify: `CMakeLists.txt`
- Modify: `tests/CMakeLists.txt`
- Create: `tests/check_hlsl_nocode.cmake`
- Create: `tests/hlsl/modern/empty.cg`
- Create: `tests/hlsl/modern/empty_geometry.cg`
- Create: `tests/hlsl/modern/empty-v40.expected`
- Create: `tests/hlsl/modern/empty-p40.expected`
- Create: `tests/hlsl/modern/empty-v50.expected`
- Create: `tests/hlsl/modern/empty-p50.expected`

- [ ] **Step 1: Register six failing profile-selection fixtures**

Add four `add_hlsl_fixture` calls for the vertex/pixel profiles. Their expected
goldens contain the exact profile and target comments plus an empty `main`.
Add `tests/check_hlsl_nocode.cmake` for the two geometry profiles; it invokes
the compiler with `-quiet -nocode`, an exact profile, and one `-po` pair per
list item, then requires status zero and empty stdout/stderr. This task proves
geometry registration and stage selection without pretending geometry HLSL
lowering exists before Tasks 9–10.

Use this exact vertex/pixel source:

```c
void main(void)
{
    return;
}
```

Use this exact geometry source with `PROFILE_OPTIONS=Vertices=1`:

```c
POINT POINT_OUT void main(void)
{
    return;
}
```

The normalized registration golden pattern is:

```hlsl
// profile hlslv40
// target vs_4_0
void main()
{
}
```

Substitute only the exact profile/target pair for the other three vertex/pixel
files. The geometry no-code cases have no golden because code generation is
deliberately disabled.

Run:

```powershell
cmake --build build-hlsl-modern --config Debug --target cgc
ctest --test-dir build-hlsl-modern -C Debug -R 'hlsl(v|g|f)(40|50)_registration' --output-on-failure
```

Expected: all six cases fail with “unknown profile”.

- [ ] **Step 2: Declare names, descriptors, and initializers**

Add exact public names and declarations to `hlsl_hal.h`:

```c
#define PROFILE_HLSLV40_NAME "hlslv40"
#define PROFILE_HLSLG40_NAME "hlslg40"
#define PROFILE_HLSLF40_NAME "hlslf40"
#define PROFILE_HLSLV50_NAME "hlslv50"
#define PROFILE_HLSLG50_NAME "hlslg50"
#define PROFILE_HLSLF50_NAME "hlslf50"

extern const HlslProfileDesc HlslProfile_hlslv40;
extern const HlslProfileDesc HlslProfile_hlslg40;
extern const HlslProfileDesc HlslProfile_hlslf40;
extern const HlslProfileDesc HlslProfile_hlslv50;
extern const HlslProfileDesc HlslProfile_hlslg50;
extern const HlslProfileDesc HlslProfile_hlslf50;
```

Declare `InitHAL_hlslv40` through `InitHAL_hlslf50` with the same naming.

- [ ] **Step 3: Create thin descriptor files**

Each new file contains the NVIDIA notice, an empty input/output connector pair,
one immutable `HlslProfileDesc`, and its initializer. `hlslg40_hal.c` uses this
complete descriptor data:

```c
static const HlslLimits limits_hlslg40 = {
    16, 32, 0, 0, 0, 16, 0,
    0, 8, 14, 4096, 128, 1024, 1024
};

static ConnectorRegisters inputRegs_hlslg40[] = {
    { NULL, 0, 0, 0, 0, 0 }
};
static ConnectorRegisters outputRegs_hlslg40[] = {
    { NULL, 0, 0, 0, 0, 0 }
};
static ConnectorDescriptor connectors_hlslg40[] = {
    { "hlslg40_in", 0, CID_HLSLG40_IN_ID, CONNECTOR_IS_INPUT,
      0, inputRegs_hlslg40 },
    { "hlslg40_out", 0, CID_HLSLG40_OUT_ID, CONNECTOR_IS_OUTPUT,
      0, outputRegs_hlslg40 }
};

const HlslProfileDesc HlslProfile_hlslg40 = {
    HLSL_STAGE_GEOMETRY,
    HLSL_SHADER_MODEL_4,
    HLSL_SYNTAX_MODERN,
    HLSL_SEMANTIC_POLICY_MODERN,
    HLSL_RESOURCE_POLICY_MODERN,
    PROFILE_HLSLG40_NAME,
    "gs_4_0",
    VERSION_STRING_HLSL_SM4,
    PROFILE_HLSLG40_ID,
    CID_HLSLG40_IN_ID,
    CID_HLSLG40_OUT_ID,
    connectors_hlslg40, 2,
    NULL, 0,
    NULL, 0,
    NULL, 0,
    NULL, 0,
    inputRegs_hlslg40, 0,
    outputRegs_hlslg40, 0,
    &limits_hlslg40,
    HLSL_CAP_GEOMETRY | HLSL_CAP_TEXTURE_METHODS | HLSL_CAP_CBUFFERS
};

int InitHAL_hlslg40(slHAL *hal)
{
    return InitHAL_hlsl_profile(hal, &HlslProfile_hlslg40);
}
```

Use the same capability masks in SM4 and SM5: vertex gets texture methods and
cbuffers; geometry adds geometry; pixel gets texture methods, cbuffers,
derivatives, and discard. Model differences live in limits and model policy,
not ad hoc capability tests.

Expose the shared initializer as `InitHAL_hlsl_profile` in `hlsl_hal.h`; it is
not a HAL registration entry itself.

- [ ] **Step 4: Compile and register all files**

Add all six `.c` files to `CGC_SOURCES`. In `RegisterProfiles_hlsl`, register
each exact name/ID and call `SetProfileIdentity` with `vs`, `gs`, or `ps` and
specificity 10.

- [ ] **Step 5: Route profile stage identity without premature lowering**

Initialize `HlslModule.stage` from the selected descriptor. Register
`SetProfileIdentity` records so overload resolution distinguishes vertex,
geometry, and pixel entries. The no-code geometry cases stop after the shared
Cg analysis has resolved a positive maximum and both topologies; this task
does not lower their bodies. Task 9 makes those resolved values mandatory in
HLSL IR when code generation is enabled.

- [ ] **Step 6: Run registration and transaction tests**

```powershell
cmake -S . -B build-hlsl-modern
cmake --build build-hlsl-modern --config Debug --target cgc hlsl_ir_unit
ctest --test-dir build-hlsl-modern -C Debug -R 'hlsl(v|g|f)(40|50)|hlsl_ir_unit' --output-on-failure
```

Expected: four source goldens, two no-code geometry registrations, and all
descriptor-stage assertions pass; every existing SM3 test remains unchanged.

- [ ] **Step 7: Commit profile registration**

```powershell
git add -- CMakeLists.txt hlsl_hal.h hlsl_hal.c hlslv40_hal.c hlslg40_hal.c hlslf40_hal.c hlslv50_hal.c hlslg50_hal.c hlslf50_hal.c tests/check_hlsl_nocode.cmake tests/CMakeLists.txt tests/hlsl/modern
git commit -m "Register Shader Model 4 and 5 HLSL profiles"
```

## Task 4: Add modern semantic and interpolation policy

**Files:**
- Create: `hlsl_modern.h`
- Create: `hlsl_modern.c`
- Modify: `hlsl_ir.h`
- Modify: `hlsl_ir.c`
- Modify: `hlsl_hal.h`
- Modify: `hlsl_bind.c`
- Modify: `hlsl_validate.c`
- Modify: `hlsl_codegen.c`
- Modify: `tests/hlsl_ir_test.c`
- Modify: `tests/check_hlsl.cmake`
- Modify: `tests/check_hlsl_failure.cmake`
- Create: `tests/hlsl/modern/semantics.cg`
- Create: `tests/hlsl/modern/semantics-v40.expected`
- Create: `tests/hlsl/modern/semantics-p40.expected`
- Create: `tests/hlsl/modern/semantics-v50.expected`
- Create: `tests/hlsl/modern/semantics-p50.expected`
- Create: `tests/hlsl/diagnostics/modern_psize.cg`
- Create: `tests/hlsl/diagnostics/modern_system_conflict.cg`

- [ ] **Step 1: Add failing pure-policy assertions**

Assert exact stage/direction mappings in `tests/hlsl_ir_test.c`:

```c
assert(HlslModernSemantic(HLSL_STAGE_VERTEX, HLSL_DIRECTION_OUTPUT,
                          "POSITION", 0) == HLSL_SEMANTIC_SV_POSITION);
assert(HlslModernSemantic(HLSL_STAGE_PIXEL, HLSL_DIRECTION_OUTPUT,
                          "COLOR", 3) == HLSL_SEMANTIC_SV_TARGET);
assert(HlslModernSemantic(HLSL_STAGE_PIXEL, HLSL_DIRECTION_OUTPUT,
                          "DEPTH", 0) == HLSL_SEMANTIC_SV_DEPTH);
assert(HlslModernSemantic(HLSL_STAGE_GEOMETRY, HLSL_DIRECTION_OUTPUT,
                          "LAYER", 0) == HLSL_SEMANTIC_SV_RT_ARRAY_INDEX);
assert(HlslModernRequiredInterpolation(TYPE_BASE_INT) ==
       HLSL_INTERPOLATION_NOINTERPOLATION);
```

Run `hlsl_ir_unit`; expected: missing header/API failure.

- [ ] **Step 2: Define semantic IR and policy types**

Add the target identities that declarations must retain to `hlsl_ir.h`:

```c
typedef enum HlslSemanticKind_Enum {
    HLSL_SEMANTIC_USER,
    HLSL_SEMANTIC_SV_POSITION,
    HLSL_SEMANTIC_SV_TARGET,
    HLSL_SEMANTIC_SV_DEPTH,
    HLSL_SEMANTIC_SV_VERTEX_ID,
    HLSL_SEMANTIC_SV_INSTANCE_ID,
    HLSL_SEMANTIC_SV_PRIMITIVE_ID,
    HLSL_SEMANTIC_SV_RT_ARRAY_INDEX,
    HLSL_SEMANTIC_SV_IS_FRONT_FACE,
    HLSL_SEMANTIC_SV_CLIP_DISTANCE,
    HLSL_SEMANTIC_UNSUPPORTED
} HlslSemanticKind;

typedef enum HlslInterpolation_Enum {
    HLSL_INTERPOLATION_DEFAULT,
    HLSL_INTERPOLATION_LINEAR,
    HLSL_INTERPOLATION_CENTROID,
    HLSL_INTERPOLATION_NOPERSPECTIVE,
    HLSL_INTERPOLATION_NOINTERPOLATION
} HlslInterpolation;
```

Add `semanticKind`, `semanticIndex`, `canonicalSemantic`, and `interpolation`
to `HlslDecl`; initialize them to user/zero/source-semantic/default in the
existing declaration builder. Create `hlsl_modern.h` with `HlslDirection`
and the table-query declarations:

```c
typedef enum HlslDirection_Enum {
    HLSL_DIRECTION_INPUT,
    HLSL_DIRECTION_OUTPUT
} HlslDirection;
```

Append `HLSL_BASE_UINT` after the existing `HLSL_BASE_STRUCT` value in
`HlslBase`, preserving every SM3 enum value. Teach `HlslTypeName`, numeric
validation, constructors, and casts about `uint`, but gate it to modern
descriptors; Cg-visible `int` remains signed and wrappers perform explicit
boundary casts.

Declare exact policy queries for semantic lookup, ABI base type, legal stage
and direction, emitted spelling, and required interpolation.

- [ ] **Step 3: Implement table-driven modern mappings**

In `hlsl_modern.c`, encode the design table. Map Cg geometry `INSTANCEID` and
`PRIMITIVEID` scalar inputs to the same `SV_PrimitiveID` identity; return a
conflict if both claim it. Map public ID ABI fields to `uint`, preserving
explicit conversions to the Cg `int` value. Return unsupported for rasterizer
`PSIZE` rather than emitting an inert user semantic.

- [ ] **Step 4: Add canonical cross-stage keys**

Extend the completed binder's interface key with semantic kind, numeric index,
type shape, width, and interpolation. Ensure user semantic comparison is
case-insensitive and system semantics compare by enum/index. For modern
profiles, extend the existing metadata line without changing SM3 output:

```text
// cgc-bind interface <in|out> <public-name> <hlsl-type> <emitted-semantic> <canonical-key> <system|user> <interpolation>
```

`hlsl_codegen.c` emits `default`, `linear`, `centroid`, `noperspective`, or
`nointerpolation` from the IR field; the canonical key is never reconstructed
from emitted spelling by a test script.

- [ ] **Step 5: Add compiler fixtures**

First teach `tests/check_hlsl.cmake` and `tests/check_hlsl_failure.cmake` to
accept optional `ENTRY`, appending `-entry <name>` to their existing argument
lists when present. Put named vertex and pixel entries in `semantics.cg` and
register all four non-geometry modern profiles with their exact entry.

The source covers `POSITION`, `WPOS`, `COLOR3`, `DEPTH`, `FACE`, integer
varyings, and `CLP0`. Goldens must contain `SV_Position`, `SV_Target3`,
`SV_Depth`, `SV_IsFrontFace`, `nointerpolation`, and `SV_ClipDistance0`
exactly. The two negative fixtures assert the assigned HLSL diagnostic,
source line, single error, and zero published body.

- [ ] **Step 6: Run focused tests**

```powershell
cmake --build build-hlsl-modern --config Debug --target cgc hlsl_ir_unit
ctest --test-dir build-hlsl-modern -C Debug -R 'hlsl_ir_unit|modern_semantic|modern_psize|modern_system_conflict' --output-on-failure
```

Expected: all policy and compiler fixtures pass.

- [ ] **Step 7: Commit modern semantics**

```powershell
git add -- hlsl_modern.h hlsl_modern.c hlsl_ir.h hlsl_ir.c hlsl_hal.h hlsl_bind.c hlsl_validate.c hlsl_codegen.c CMakeLists.txt tests/hlsl_ir_test.c tests/check_hlsl.cmake tests/check_hlsl_failure.cmake tests/CMakeLists.txt tests/hlsl/modern tests/hlsl/diagnostics
git commit -m "Define modern HLSL interface semantics"
```

## Task 5: Represent and verify modern resources and geometry in HLSL IR

**Files:**
- Modify: `hlsl_ir.h`
- Modify: `hlsl_ir.c`
- Modify: `hlsl_validate.c`
- Modify: `hlsl_codegen.c`
- Modify: `tests/hlsl_ir_test.c`

- [ ] **Step 1: Add malformed-module tests before builders**

Add one test for each invariant: cbuffer with an invalid `b` slot; overlapping
packoffset spans; texture without sampler; mismatched `tN`/`sN`; geometry
metadata on vertex; missing geometry metadata; wrong topology extent; append
outside geometry; restart with operands; flat replay with a non-Boolean flag;
and a writer invoked on an invalid module. Assert `HlslWriteModule` returns
false and the temporary stream remains zero bytes.

- [ ] **Step 2: Add modern resource IR types**

Append these values to the existing enums in `hlsl_ir.h`; do not insert them
between SM3 values or reuse `HLSL_REGISTER_B`, which is the legacy Boolean
constant bank:

```c
/* HlslBase, after the Task 4 HLSL_BASE_UINT value. */
HLSL_BASE_TEXTURE1D,
HLSL_BASE_TEXTURE2D,
HLSL_BASE_TEXTURE3D,
HLSL_BASE_TEXTURECUBE,
HLSL_BASE_SAMPLER_STATE

/* HlslRegisterBank, after HLSL_REGISTER_S. */
HLSL_REGISTER_T,
HLSL_REGISTER_CB
```

Update `HlslTypeName`, register-bank spelling, type validation, and zero-value
construction for these additions. The object types are legal only as global
resources, never as Cg-visible numeric values.

Then extend `hlsl_ir.h` with:

```c
typedef enum HlslResourceKind_Enum {
    HLSL_RESOURCE_CBUFFER,
    HLSL_RESOURCE_TEXTURE,
    HLSL_RESOURCE_SAMPLER
} HlslResourceKind;

typedef struct HlslPackOffset_Rec {
    int vector;
    int component;
    int componentCount;
} HlslPackOffset;

typedef struct HlslResourceBinding_Rec {
    HlslResourceKind kind;
    int slot;
    int pairId;
} HlslResourceBinding;

typedef struct HlslResource_Rec HlslResource;

struct HlslResource_Rec {
    HlslResource *next;
    HlslModule *owner;
    HlslResourceKind kind;
    HlslType type;
    const char *name;
    HlslLoc loc;
    HlslResourceBinding binding;
    HlslDecl *sourceDeclaration;
    HlslDecl *members;
};
```

Add `HlslResource *resources` to `HlslModule`. Add `hasPackOffset` and
`packOffset` to `HlslDecl` so cbuffer fields retain their physical placement.
Append structured error kinds for system semantics, interpolation, cbuffers,
resource pairs, geometry layouts, and geometry limits after the existing
`HLSL_ERROR_INVALID_IR` value. Add module-owned resource builders that reject
negative slots and retain source locations:

```c
HlslResource *HlslNewResource(HlslModule *module, HlslResourceKind kind,
                              HlslType type, const char *name, HlslLoc loc);
int HlslBindResource(HlslModule *module, HlslResource *resource,
                     int slot, int pairId);
int HlslSetPackOffset(HlslModule *module, HlslDecl *field,
                      HlslPackOffset offset);
```

- [ ] **Step 3: Add geometry IR types**

Define target-specific stable enums and fields:

```c
typedef enum HlslGeometryInput_Enum {
    HLSL_GEOMETRY_INPUT_POINT,
    HLSL_GEOMETRY_INPUT_LINE,
    HLSL_GEOMETRY_INPUT_LINE_ADJ,
    HLSL_GEOMETRY_INPUT_TRIANGLE,
    HLSL_GEOMETRY_INPUT_TRIANGLE_ADJ
} HlslGeometryInput;

typedef enum HlslGeometryStream_Enum {
    HLSL_GEOMETRY_STREAM_POINT,
    HLSL_GEOMETRY_STREAM_LINE,
    HLSL_GEOMETRY_STREAM_TRIANGLE
} HlslGeometryStream;
```

Add `geometryInput`, `geometryStream`, `geometryInputCount`, and
`geometryMaxVertices` to `HlslModule`. Append explicit `HLSL_STMT_APPEND` and
`HLSL_STMT_RESTART_STRIP` values to `HlslStmtKind`; the append union member
contains its output-record expression and replay-list head, while restart has
no payload. Define `HlslFlatReplay` with `next`, `owner`, target field, shadow
declaration, and Boolean defined-flag fields.

- [ ] **Step 4: Implement builders with module ownership**

Add:

```c
int HlslSetGeometryLayout(HlslModule *module, HlslGeometryInput input,
                          HlslGeometryStream stream, int inputCount,
                          int maxVertices);
HlslStmt *HlslNewAppend(HlslModule *module, HlslExpr *record,
                        HlslFlatReplay *replay, HlslLoc loc);
HlslStmt *HlslNewRestartStrip(HlslModule *module, HlslLoc loc);
HlslFlatReplay *HlslNewFlatReplay(HlslModule *module, HlslDecl *target,
                                  HlslDecl *shadow, HlslDecl *defined);
```

Builders allocate only from the module arena and never print diagnostics.

- [ ] **Step 5: Port exact verifier contracts**

In `hlsl_validate.c`, extend the existing `HlslValidateModule` path to validate
ownership, stage/model, layout/extent, positive maximum, resource pairing,
packoffset overlap/alignment, append record type, flat target/shadow type
equality, Boolean defined flags, and statement placement. Keep user capability
diagnostics outside structural-invalid-IR checks.

- [ ] **Step 6: Make the writer gate unconditional**

Change `HlslWriteModule` to take mutable `HlslModule *`, call
`HlslValidateModule(module, profile)` before creating its temporary stream,
and keep the existing private-stream publication transaction. Do not write a
profile comment before validation succeeds. Consolidate or remove duplicate
checks in the private `HlslCanWriteModule` only after equivalent assertions
exist in `HlslValidateModule`.

- [ ] **Step 7: Run unit tests in Debug and Release**

```powershell
cmake --build build-hlsl-modern --config Debug --target hlsl_ir_unit
ctest --test-dir build-hlsl-modern -C Debug -R '^hlsl_ir_unit$' --output-on-failure
cmake --build build-hlsl-modern --config Release --target hlsl_ir_unit
ctest --test-dir build-hlsl-modern -C Release -R '^hlsl_ir_unit$' --output-on-failure
```

Expected: builders, malformed modules, and zero-byte gates pass with assertions
active in the dedicated assertion test if the prerequisite plan provides one.

- [ ] **Step 8: Commit modern IR**

```powershell
git add -- hlsl_ir.h hlsl_ir.c hlsl_validate.c hlsl_codegen.c tests/hlsl_ir_test.c
git commit -m "Represent modern resources and geometry in HLSL IR"
```

## Task 6: Implement deterministic modern constant-buffer binding

**Files:**
- Modify: `hlsl_modern.h`
- Modify: `hlsl_modern.c`
- Modify: `hlsl_bind.c`
- Modify: `hlsl_codegen.c`
- Modify: `hlsl_validate.c`
- Modify: `tests/hlsl_ir_test.c`
- Create: `tests/hlsl/modern/uniforms.cg`
- Create: `tests/hlsl/modern/uniforms-v40.expected`
- Create: `tests/hlsl/modern/uniforms-v50.expected`
- Create: `tests/hlsl/diagnostics/modern_packoffset_conflict.cg`

- [ ] **Step 1: Add failing packing unit cases**

Assert these exact allocations in a fresh `b0` buffer: `float` at `c0.x`,
`float3` at `c0.y`, the next `float2` at `c1.x`, `row_major float3x2` at
`c2`–`c4`, and `float4[2]` at `c5`–`c6`. Add an explicit overlap between a
value at `c2` and the matrix span and assert the binding-collision reason.

- [ ] **Step 2: Define one modern packing API**

Declare in `hlsl_modern.h`:

```c
typedef struct HlslModernPackCursor_Rec {
    int vector;
    int component;
} HlslModernPackCursor;

int HlslModernPackType(const HlslType *type, HlslModernPackCursor *cursor,
                       HlslPackOffset *offset, int *vectorSpan);
```

Implement the documented HLSL cbuffer rules: a value never straddles a
four-component vector; arrays start each element on a new vector; each
row-major matrix row begins a vector; structures begin on a new vector and end
at a vector boundary.

- [ ] **Step 3: Bind explicit declarations before implicit declarations**

In `hlsl_bind.c`, reserve explicit `packoffset` spans first, then allocate
unbound values in source declaration order. Use one generated cbuffer named
`cgc_Uniforms` at `register(b0)`. Reject source bindings that cannot map
unambiguously to this contract.

- [ ] **Step 4: Preserve logical aggregates and defaults**

Keep one public metadata record per Cg logical declaration. When mixed or
nested aggregates require physical leaves, emit deterministic leaf fields and
reconstruct the logical value exactly once at its use boundary. Emit stable
`// cgc-default` metadata for source defaults; do not emit a cbuffer field
initializer or change the application-owned runtime initialization contract.

- [ ] **Step 5: Emit exact cbuffer syntax**

`hlsl_codegen.c` must produce:

```hlsl
cbuffer cgc_Uniforms : register(b0)
{
    float cgc_scale : packoffset(c0.x);
    row_major float3x2 cgc_transform : packoffset(c2);
};
```

The writer prints fields in physical order and metadata in logical source
order.

- [ ] **Step 6: Add compiler golden and negative fixture**

Compile the same named vertex entry under `hlslv40` and `hlslv50`; both
goldens cover scalars, vectors, row-major rectangular matrices, arrays, nested
structures, defaults, entry uniforms, and source-global uniforms. Apart from
profile/target comments they remain identical. The negative fixture
intentionally overlaps two explicit spans and asserts the exact collision
diagnostic and no published body.

- [ ] **Step 7: Run packing and regression tests**

```powershell
cmake --build build-hlsl-modern --config Debug --target cgc hlsl_ir_unit
ctest --test-dir build-hlsl-modern -C Debug -R 'hlsl_ir_unit|modern_uniform|modern_packoffset' --output-on-failure
ctest --test-dir build-hlsl-modern -C Debug -R '^hlslv_|^hlslf_' --output-on-failure
```

Expected: modern fixtures pass and SM3 fixtures remain unchanged.

- [ ] **Step 8: Commit constant buffers**

```powershell
git add -- hlsl_modern.h hlsl_modern.c hlsl_bind.c hlsl_codegen.c hlsl_validate.c tests/hlsl_ir_test.c tests/CMakeLists.txt tests/hlsl/modern tests/hlsl/diagnostics
git commit -m "Bind modern HLSL constant buffers"
```

## Task 7: Split Cg samplers into modern texture/sampler pairs

**Files:**
- Modify: `hlsl_modern.h`
- Modify: `hlsl_modern.c`
- Modify: `hlsl_ir.h`
- Modify: `hlsl_ir.c`
- Modify: `hlsl_legalize.c`
- Modify: `hlsl_bind.c`
- Modify: `hlsl_codegen.c`
- Modify: `hlsl_validate.c`
- Modify: `tests/hlsl_ir_test.c`
- Create: `tests/hlsl/modern/textures.cg`
- Create: `tests/hlsl/modern/textures-v40.expected`
- Create: `tests/hlsl/modern/textures-p40.expected`
- Create: `tests/hlsl/modern/textures-v50.expected`
- Create: `tests/hlsl/modern/textures-p50.expected`
- Create: `tests/hlsl/diagnostics/modern_texture_pair_conflict.cg`
- Create: `tests/hlsl/diagnostics/modern_texture_stage.cg`

- [ ] **Step 1: Add failing pair-allocation tests**

Create two implicit samplers and one explicit `TEXUNIT5`. Assert atomic pairs
`t0/s0`, `t1/s1`, and `t5/s5`. Reserve only `t2`, request another Cg sampler,
and assert it skips index 2 rather than creating `t2/s3`. Add exhaustion at
index 16 and assert the sampler-pair limit reason.

- [ ] **Step 2: Define texture dimensions and method selection**

Add the source-to-object dimension policy to `hlsl_modern.h`:

```c
typedef enum HlslTextureDimension_Enum {
    HLSL_TEXTURE_1D,
    HLSL_TEXTURE_2D,
    HLSL_TEXTURE_3D,
    HLSL_TEXTURE_CUBE
} HlslTextureDimension;
```

Add the emitted operation identity to `hlsl_ir.h`:

```c
typedef enum HlslTextureMethod_Enum {
    HLSL_TEXTURE_METHOD_SAMPLE,
    HLSL_TEXTURE_METHOD_SAMPLE_LEVEL,
    HLSL_TEXTURE_METHOD_SAMPLE_BIAS,
    HLSL_TEXTURE_METHOD_SAMPLE_GRAD
} HlslTextureMethod;
```

Append `HLSL_EXPR_TEXTURE_METHOD` to `HlslExprKind`. Its union member contains
the method enum plus explicit texture receiver, sampler state, coordinates,
and up to two additional method arguments. Add a located builder whose result
type is supplied by the legalizer. Code generation prints only this verified
node as object-method syntax; it never derives a method from a legacy builtin
name.

Declare a signature-driven selector that accepts stage, source intrinsic ID,
dimension, coordinate width, and result type; it returns a method or a
structured unsupported/stage/signature reason.

- [ ] **Step 3: Allocate pairs atomically**

In `hlsl_bind.c`, allocate one pair ID and same numeric slot for each logical
Cg sampler. `TEXUNITn` reserves both namespaces before implicit allocation.
Metadata records one public Cg sampler with both physical bindings. Expand a
sampler-typed helper parameter into adjacent typed `Texture*` and
`SamplerState` parameters and rewrite every reachable call site with the same
pair in source-parameter order. Preserve one logical Cg identity for overload
resolution and diagnostics; do not synthesize combined modern sampler types.

- [ ] **Step 4: Legalize exact texture forms**

Map ordinary, explicit-level, biased, gradient, and projected Cg forms to
`.Sample`, `.SampleLevel`, `.SampleBias`, and `.SampleGrad`. Capture projected
coordinates in a temporary, divide the spatial components by the projection
component once, and then build `HLSL_EXPR_TEXTURE_METHOD` with the selected
texture/sampler pair. Reject implicit derivative sampling in vertex/geometry
stages. Extend `HlslValidateModule` to verify receiver type, pair identity,
argument count and shapes, coordinate width, result type, and stage before the
writer sees the node.

- [ ] **Step 5: Emit paired declarations and method calls**

For a 2D sampler at slot 3, emit exactly:

```hlsl
Texture2D<float4> cgc_texture_diffuse : register(t3);
SamplerState cgc_sampler_diffuse : register(s3);
```

Calls use `cgc_texture_diffuse.Sample(cgc_sampler_diffuse, uv)` and never emit
legacy `sampler2D` syntax in SM4/SM5.

- [ ] **Step 6: Add complete method fixtures**

Put named vertex and pixel entries in the success source and compile them under
all four non-geometry modern profiles. Together they cover 1D, 2D, 3D, cube,
projected, LOD, bias, and gradient families in every currently executable
legal stage, including a sampler passed through a reachable helper. Unit cases
cover the geometry-stage selector now; add the
generated geometry texture case when the stage becomes executable in Task 11.
Negative fixtures cover mismatched dimension, coordinate width, return shape,
explicit pair collision, out-of-range unit, and derivative-dependent use in
vertex. Task 11 adds the matching geometry rejection.

- [ ] **Step 7: Run texture tests**

```powershell
cmake --build build-hlsl-modern --config Debug --target cgc hlsl_ir_unit
ctest --test-dir build-hlsl-modern -C Debug -R 'hlsl_ir_unit|modern_texture' --output-on-failure
```

Expected: all success and rejection cases pass transactionally.

- [ ] **Step 8: Commit modern resources**

```powershell
git add -- hlsl_modern.h hlsl_modern.c hlsl_ir.h hlsl_ir.c hlsl_legalize.c hlsl_bind.c hlsl_codegen.c hlsl_validate.c tests/hlsl_ir_test.c tests/CMakeLists.txt tests/hlsl/modern tests/hlsl/diagnostics
git commit -m "Lower Cg samplers to modern HLSL resources"
```

## Task 8: Qualify SM4 and SM5 vertex/pixel language output

**Files:**
- Modify: `hlsl_lower.c`
- Modify: `hlsl_legalize.c`
- Modify: `hlsl_bind.c`
- Modify: `hlsl_validate.c`
- Modify: `hlsl_codegen.c`
- Modify: `tests/CMakeLists.txt`
- Create: `tests/hlsl/modern/language.cg`
- Create: `tests/hlsl/modern/language-v40.expected`
- Create: `tests/hlsl/modern/language-p40.expected`
- Create: `tests/hlsl/modern/language-v50.expected`
- Create: `tests/hlsl/modern/language-p50.expected`
- Create: `tests/hlsl/modern/position-v40.expected`
- Create: `tests/hlsl/modern/position-v50.expected`
- Create: `tests/hlsl/modern/reflection-v40.expected`
- Create: `tests/hlsl/modern/reflection-v50.expected`
- Create: `tests/hlsl/modern/vertexlight-v40.expected`
- Create: `tests/hlsl/modern/vertexlight-v50.expected`
- Create: `tests/hlsl/modern/vertexlight4-v40.expected`
- Create: `tests/hlsl/modern/vertexlight4-v50.expected`

- [ ] **Step 1: Register failing complete-language goldens**

Register the four bundled vertex shaders under `hlslv40` and `hlslv50` plus
one focused vertex and pixel fixture under all four non-geometry modern
profiles. Expected initial failures identify common-lowering, wrapper, or
model-policy cases not yet qualified by the narrower preceding fixtures.

- [ ] **Step 2: Reuse common Cg-to-HLSL lowering**

Route SM4/SM5 through the completed SM3 lowerer for types, declarations,
helpers, overloads, expressions, evaluation-order temporaries, structured
control flow, returns, arrays, structures, and matrices. Branch only through
descriptor capability queries; do not duplicate lowering by target string.

- [ ] **Step 3: Build modern public wrappers**

Generate stage structures with modern semantics, call the internal entry once,
and marshal return/out/inout values. Add explicit `int`/`uint` boundary casts
for system values. Omit empty input/output structures and protect `main` via
the identity-aware name allocator.

- [ ] **Step 4: Complete intrinsic and statement legalization**

Run every intrinsic row from the SM3 compatibility matrix through the modern
selector. Accept shared exact operations, permit pixel derivatives/discard,
reject them in vertex, and retain row-major matrix behavior. Do not add SM5
intrinsics absent from Cg IR.

- [ ] **Step 5: Emit distinct profile/target metadata with otherwise stable source**

SM4 and SM5 goldens must differ only where the selected target, model limit,
or exact capability requires it. Both emit modern cbuffer and texture syntax.

- [ ] **Step 6: Run all vertex/pixel goldens and SM3 regression**

```powershell
cmake --build build-hlsl-modern --config Debug --target cgc hlsl_ir_unit
ctest --test-dir build-hlsl-modern -C Debug -R 'hlsl(v|f)(40|50)|hlslv_|hlslf_' --output-on-failure
```

Expected: bundled and focused modern fixtures pass; every SM3 test remains
unchanged.

- [ ] **Step 7: Commit vertex/pixel output**

```powershell
git add -- hlsl_lower.c hlsl_legalize.c hlsl_bind.c hlsl_validate.c hlsl_codegen.c tests/CMakeLists.txt tests/hlsl/modern
git commit -m "Emit Shader Model 4 and 5 vertex and pixel HLSL"
```

## Task 9: Lower Cg geometry layouts and system inputs

**Files:**
- Modify: `hlsl_modern.h`
- Modify: `hlsl_modern.c`
- Modify: `hlsl_lower.c`
- Modify: `hlsl_bind.c`
- Modify: `hlsl_validate.c`
- Modify: `tests/hlsl_ir_test.c`
- Modify: `tests/check_hlsl.cmake`
- Modify: `tests/check_hlsl_failure.cmake`
- Modify: `tests/CMakeLists.txt`
- Create: `tests/hlsl/geometry/topologies.cg`
- Create: `tests/hlsl/geometry/topologies-{point,line,lineadj,triangle,triangleadj}-{g40,g50}.expected`
- Create: `tests/hlsl/geometry/system_values.cg`
- Create: `tests/hlsl/geometry/system_values-{g40,g50}.expected`
- Create: `tests/hlsl/geometry/vertex_id_bridge.cg`

- [ ] **Step 1: Add failing topology assertions**

For each `CgGeometryInput`, assert the exact HLSL keyword and extent:
point/1, line/2, lineadj/4, triangle/3, triangleadj/6. Assert output mappings
PointStream, LineStream, and TriangleStream. Assert missing or non-positive
`Vertices=N` is rejected for both geometry profiles.

- [ ] **Step 2: Add pure topology conversion functions**

Declare and implement:

```c
int HlslModernGeometryInput(CgGeometryInput input,
                            HlslGeometryInput *result, int *extent);
int HlslModernGeometryStream(CgGeometryOutput output,
                             HlslGeometryStream *result);
```

Use exhaustive switches with rejecting defaults.

- [ ] **Step 3: Lower verified layout metadata**

In `hlsl_lower.c`, require `CGIR_STAGE_GEOMETRY`, convert the verified input and
output topologies, require a positive maximum, and call
`HlslSetGeometryLayout`. Reconstruct every `AttribArray<T>` from the public
input array with the exact topology extent.

- [ ] **Step 4: Bind geometry system values**

Use scalar wrapper parameters for `SV_PrimitiveID`. Map both Cg scalar
`INSTANCEID` and `PRIMITIVEID` to that identity and reject a simultaneous
claim. Map output `PRIMITIVEID` to `SV_PrimitiveID` and `LAYER` to
`SV_RenderTargetArrayIndex`, with explicit public-`uint`/internal-`int`
conversions.

- [ ] **Step 5: Implement the vertex-ID bridge**

When a geometry interface consumes `VERTEXID`, require matching vertex output
metadata named by canonical key `CG_VERTEXID0`, mark both sides
`nointerpolation`, write `SV_VertexID` into it in the vertex wrapper, and
reconstruct the geometry `AttribArray<int>` from the input array.

- [ ] **Step 6: Extend the fixture runners and add focused tests**

Extend the argument-list support added in Task 4 with a semicolon-list
`PROFILE_OPTIONS`. Append one `-po <value>` pair per option before the source
path; do not concatenate options into one shell string. Keep the existing
exact-output and zero-publication checks.

Assert all layouts and system mappings in `hlsl_ir_unit`. Register compiler
fixtures for each topology under both geometry profiles, passing the input
topology, output topology, and `Vertices=N` through `PROFILE_OPTIONS`. Add
negative tests for missing maximum, wrong extent, duplicate primitive system
input, invalid system type, and missing vertex-ID producer metadata.

- [ ] **Step 7: Run layout tests**

```powershell
cmake --build build-hlsl-modern --config Debug --target cgc hlsl_ir_unit
ctest --test-dir build-hlsl-modern -C Debug -R 'hlsl_ir_unit|hlslg.*topology|hlslg.*system|vertex_id' --output-on-failure
```

Expected: all topology/system tests pass or fail with their exact intended
diagnostic.

- [ ] **Step 8: Commit geometry interfaces**

```powershell
git add -- hlsl_modern.h hlsl_modern.c hlsl_lower.c hlsl_bind.c hlsl_validate.c tests/hlsl_ir_test.c tests/check_hlsl.cmake tests/check_hlsl_failure.cmake tests/CMakeLists.txt tests/hlsl/geometry tests/hlsl/diagnostics
git commit -m "Lower Cg geometry interfaces to HLSL"
```

## Task 10: Lower append, flat state, and strip restart

**Files:**
- Modify: `hlsl_lower.c`
- Modify: `hlsl_legalize.c`
- Modify: `hlsl_codegen.c`
- Modify: `hlsl_validate.c`
- Modify: `tests/hlsl_ir_test.c`
- Create: `tests/hlsl/geometry/pass_through.cg`
- Create: `tests/hlsl/geometry/amplify.cg`
- Create: `tests/hlsl/geometry/restart.cg`
- Create: `tests/hlsl/geometry/flat.cg`
- Create: `tests/hlsl/geometry/conditional_flat.cg`
- Create: `tests/hlsl/geometry/reachable_helper.cg`
- Create: `tests/hlsl/geometry/{pass_through,amplify,restart,flat,conditional_flat,reachable_helper}-{g40,g50}.expected`

- [ ] **Step 1: Add failing explicit-node assertions**

Lower a small verified Cg geometry module and assert one
`HLSL_STMT_APPEND` per `CGIR_STMT_GEOMETRY_EMIT`, one
`HLSL_STMT_RESTART_STRIP` per restart, and flat statements represented as
shadow assignments rather than calls. Assert the original control-flow parent
of every node is preserved.

- [ ] **Step 2: Lower `emitVertex` bundles once**

Evaluate resolved bundle leaves in source order into temporaries where needed,
assign a complete generated output record, attach the current flat replay list,
and build `HlslNewAppend`. If one semantic appears in both the emit bundle and
defined flat state, replay occurs last and therefore supplies the flat value.

- [ ] **Step 3: Lower path-correct flat state**

Before lowering bodies, compute a transitive geometry-effect bit for every
reachable function: direct or called use of emit, restart, or flat state marks
the caller. Closure-convert every marked internal function and call site with
hidden `inout` parameters for the selected stream, output record, and ordered
flat shadow/defined pairs. The public wrapper owns and initializes this state;
do not emit mutable global variables or duplicate a helper body.

For each canonical flat semantic, create a private typed shadow and Boolean
defined flag initialized false. `flatAttrib` assigns the shadow then true
inside its original branch/loop/helper path through those hidden parameters.
Each later append carries a replay triple; no undefined shadow is read.

- [ ] **Step 4: Lower restart explicitly**

Convert `CGIR_STMT_GEOMETRY_RESTART` to `HlslNewRestartStrip` with its source
location and no operands.

- [ ] **Step 5: Emit exact stream methods**

In `hlsl_codegen.c`, append emits guarded flat assignments followed by:

```hlsl
cgc_stream.Append(cgc_output);
```

Restart emits:

```hlsl
cgc_stream.RestartStrip();
```

The public geometry wrapper has `[maxvertexcount(N)]`, the exact input keyword
and array extent, and one `inout <Topology>Stream<Output>` parameter. Internal
helper signatures expose only the generated state required by the transitive
effect analysis, in deterministic declaration order.

- [ ] **Step 6: Register behavioral goldens**

For both `hlslg40` and `hlslg50`, register pass-through, loop amplification,
multiple restarts, repeated flat update, conditional flat update, and reachable
helper fixtures. Goldens assert source order and one method call per Cg
operation.

- [ ] **Step 7: Run operation tests**

```powershell
cmake --build build-hlsl-modern --config Debug --target cgc hlsl_ir_unit
ctest --test-dir build-hlsl-modern -C Debug -R 'hlsl_ir_unit|hlslg.*(pass|amplify|restart|flat|helper)' --output-on-failure
```

Expected: all explicit-node and compiler goldens pass.

- [ ] **Step 8: Commit geometry operations**

```powershell
git add -- hlsl_lower.c hlsl_legalize.c hlsl_codegen.c hlsl_validate.c tests/hlsl_ir_test.c tests/CMakeLists.txt tests/hlsl/geometry
git commit -m "Emit HLSL geometry stream operations"
```

## Task 11: Qualify complete SM4 and SM5 geometry pipelines

**Files:**
- Modify: `tests/check_hlsl_link.cmake`
- Modify: `tests/CMakeLists.txt`
- Create: `tests/hlsl/link/vp_geometry.cg`
- Create: `tests/hlsl/link/gp_geometry.cg`
- Create: `tests/hlsl/link/fp_geometry.cg`
- Create: `tests/hlsl/link/vp_vertex_id.cg`
- Create: `tests/hlsl/link/gp_vertex_id.cg`
- Create: `tests/hlsl/link/fp_vertex_id.cg`
- Create: `tests/hlsl/link/gp_type_mismatch.cg`
- Create: `tests/hlsl/link/fp_type_mismatch.cg`
- Create: `tests/hlsl/link/gp_interpolation_mismatch.cg`
- Create: `tests/hlsl/geometry/textures.cg`
- Create: `tests/hlsl/geometry/textures-{g40,g50}.expected`
- Create: `tests/hlsl/diagnostics/modern_texture_geometry_stage.cg`

- [ ] **Step 1: Extend the current link runner to three stages**

Preserve the current SM3 `VERTEX_SOURCE`/`FRAGMENT_SOURCE` path and legacy
four-field `// cgc-bind interface` parser. Add explicit `VERTEX_PROFILE`,
optional `GEOMETRY_PROFILE`/`GEOMETRY_SOURCE`/`GEOMETRY_OUTPUT`/
`GEOMETRY_OPTIONS`, and `PIXEL_PROFILE` inputs, defaulting the two-stage path
to `hlslv`/`hlslf`. Compile each selected source with its exact profile and
options. For modern profiles, parse the extended `// cgc-bind interface`
records emitted in Task 4, including canonical key, complete HLSL type,
system/user class, and interpolation. Compare vertex→geometry and
geometry→pixel independently; never infer canonical identity from source
names or `SV_` spelling.

- [ ] **Step 2: Add a representative matching pipeline**

The vertex stage exports position, float texture coordinates, color, and an
integer user varying. Geometry consumes the topology-sized array, writes
`SV_Position`, `SV_PrimitiveID`, `SV_RenderTargetArrayIndex`, and ordinary
varyings, then pixel consumes the rasterized interface and writes
`SV_Target0`. Register it for both SM4 and SM5.

Also register the deferred geometry texture fixture from Task 7 under both
geometry models. Its goldens contain stage-legal `.SampleLevel` and
`.SampleGrad` calls. The negative fixture uses implicit `.Sample` and asserts
the exact derivative/stage diagnostic with zero output.

- [ ] **Step 3: Add the vertex-ID bridge pipeline**

Compile `vp_vertex_id.cg` with `hlslv40/50`, `gp_vertex_id.cg` with
`hlslg40/50`, and `fp_vertex_id.cg` with `hlslf40/50`. Assert the VS public
input uses `SV_VertexID`, the VS→GS bridge uses one matching
`nointerpolation CG_VERTEXID0`, and the pixel interface does not expose the
bridge unless source explicitly forwards it.

- [ ] **Step 4: Add exact negative pipelines**

Register one type mismatch and one interpolation mismatch at each boundary.
The runner must report the canonical key and both conflicting declarations.
Each individual shader must still pass compiler generation and optional
external validation; only the repository cross-stage check fails.

- [ ] **Step 5: Run all pipeline tests**

```powershell
cmake --build build-hlsl-modern --config Debug --target cgc
ctest --test-dir build-hlsl-modern -C Debug -R 'hlsl_(sm4|sm5)_pipeline' --output-on-failure
```

Expected: matching and vertex-ID pipelines pass; mismatch tests pass by
observing their exact expected link failure.

- [ ] **Step 6: Commit pipeline qualification**

```powershell
git add -- tests/check_hlsl_link.cmake tests/CMakeLists.txt tests/hlsl/link tests/hlsl/geometry tests/hlsl/diagnostics
git commit -m "Qualify modern HLSL shader pipelines"
```

## Task 12: Enforce exact SM4/SM5 capabilities, limits, and diagnostics

**Files:**
- Modify: `hlslv40_hal.c`
- Modify: `hlslg40_hal.c`
- Modify: `hlslf40_hal.c`
- Modify: `hlslv50_hal.c`
- Modify: `hlslg50_hal.c`
- Modify: `hlslf50_hal.c`
- Modify: `hlsl_validate.c`
- Modify: `hlsl_hal.c`
- Modify: `errors.h`
- Modify: `tests/hlsl_ir_test.c`
- Modify: `tests/check_hlsl_failure.cmake`
- Modify: `tests/CMakeLists.txt`
- Create: `tests/hlsl/limits/modern_interface.cg`
- Create: `tests/hlsl/limits/modern_resources.cg`
- Create: `tests/hlsl/limits/modern_geometry.cg`
- Create: `tests/hlsl/diagnostics/modern_wrong_stage.cg`
- Create: `tests/hlsl/diagnostics/modern_unsupported.cg`

- [ ] **Step 1: Add descriptor boundary assertions**

Pin this exact descriptor table, with input/output counts measured in
four-component signature registers:

| Model/stage | Inputs | Outputs | Color targets | Depth outputs |
| --- | ---: | ---: | ---: | ---: |
| SM4 vertex | 16 | 16 | 0 | 0 |
| SM4 geometry | 16 | 32 | 0 | 0 |
| SM4 pixel | 32 | 8 | 8 | 1 |
| SM5 vertex | 32 | 32 | 0 | 0 |
| SM5 geometry | 32 | 32 | 0 | 0 |
| SM5 pixel | 32 | 8 | 8 | 1 |

All six use 14 application-visible cbuffer slots (`b0` through `b13`), 4096
four-component elements per cbuffer, 128 shader-resource slots, 16 sampler
slots, and eight combined clip/cull-distance components. Geometry descriptors
also use 1024 maximum emitted vertices and 1024 total output components per
invocation; non-geometry descriptors store zero for both fields. Assert every
field per descriptor so the SM4/SM5 signature differences cannot collapse
into one shared table.

- [ ] **Step 2: Generate exact-boundary and one-over compiler fixtures**

Use CMake loops to generate or parameterize one compiler test at each boundary
the current Cg surface can reach and one test one unit beyond it. Keep
non-exposed limits, such as multiple user cbuffers, as descriptor/IR unit tests
and mark them `not exposed` in the compatibility matrix rather than inventing
new Cg syntax. Geometry total-output tests compute:

```text
components per output vertex × Vertices
```

and exercise exactly 1024 and 1025. Interface tests count canonical physical
leaves, not declarations. Resource tests count paired Cg samplers and packed
cbuffer spans.

- [ ] **Step 3: Assign stable modern HLSL diagnostics**

Extend the existing 6400–6499 HLSL block with concrete macros for profile
stage mismatch, unsupported model capability, system semantic stage/type,
interface conflict, interpolation conflict, cbuffer placement/collision/limit,
resource-pair collision/limit, texture method stage/signature, missing geometry
maximum, geometry maximum limit, and geometry total-output limit.

- [ ] **Step 4: Map structured reasons exactly once**

`hlsl_hal.c` records the error count before each backend phase. A phase that
returns a structured user reason maps it to one diagnostic and source location;
common code does not append a generic error. Structural IR failures use the
existing internal HLSL IR diagnostic.

- [ ] **Step 5: Tighten the failure runner**

Require exact `CODE`, `EXPECTED_LINE`, and `MESSAGE`; optionally accept one
`NOTES` list. Count exactly one primary `error C####:` and assert the output
file is absent or zero length with no `cgc-bind`, cbuffer, texture, structure,
or function body.

- [ ] **Step 6: Run all boundaries and transactions**

```powershell
cmake --build build-hlsl-modern --config Debug --target cgc hlsl_ir_unit
ctest --test-dir build-hlsl-modern -C Debug -R 'hlsl.*(limit|boundary|transaction|diagnostic)|hlsl_ir_unit' --output-on-failure
```

Expected: every exact boundary succeeds, every one-over case emits its assigned
single error, and every failure is transactional.

- [ ] **Step 7: Run the full suite before committing**

```powershell
ctest --test-dir build-hlsl-modern -C Debug --output-on-failure
```

Expected: zero failures across all existing and new profiles.

- [ ] **Step 8: Commit validation and diagnostics**

```powershell
git add -- hlslv40_hal.c hlslg40_hal.c hlslf40_hal.c hlslv50_hal.c hlslg50_hal.c hlslf50_hal.c hlsl_validate.c hlsl_hal.c errors.h tests/hlsl_ir_test.c tests/check_hlsl_failure.cmake tests/CMakeLists.txt tests/hlsl/limits tests/hlsl/diagnostics
git commit -m "Validate Shader Model 4 and 5 HLSL limits"
```

## Task 13: Add optional `fxc`, close documentation, and run final qualification

**Files:**
- Modify: `tests/validate_hlsl.cmake`
- Modify: `tests/CMakeLists.txt`
- Modify: `README.md`
- Create: `docs/hlsl-sm4-sm5-compatibility.md`
- Modify: `docs/superpowers/plans/2026-09-05-directx10-11-hlsl-sm4-sm5.md`

- [ ] **Step 1: Extend the existing exact-target validator**

Keep `tests/validate_hlsl.cmake`'s current required `CGC`, `FXC`, `PROFILE`,
`TARGET`, `SOURCE`, `OUTPUT`, `BYTECODE`, and `CONFIG` inputs and public
`main` entry. Extend `add_hlsl_validation_case` so it maps all eight profiles
to their exact targets instead of treating every non-`hlslv` profile as
`ps_3_0`. Pass a `LEGACY_SYNTAX` Boolean: retain `/Gec` for the two SM3
profiles, but use `/Ges` for SM4/SM5. The external command remains:

```text
fxc /nologo /WX <syntax-mode> /E main /T <exact-target> /Fo <bytecode> <generated-source>
```

Continue preserving compiler and `fxc` stdout/stderr on failure. Add a
`CGC_REQUIRE_FXC` CMake option, default `OFF`: when `fxc` is absent, omit the
external tests as today; when the option is `ON`, fail configuration with a
clear Windows SDK requirement instead of silently omitting them.

- [ ] **Step 2: Register all six target validators**

Validate every tracked successful modern Cg fixture through the existing
compile-then-`fxc` runner and its exact target. At minimum, register one bundled
vertex shader, one texture-heavy pixel shader, and every geometry topology
under both models. Add a script self-test with an intentionally invalid
generated HLSL file to prove nonzero `fxc` status fails the harness.

- [ ] **Step 3: Fill the compatibility matrix completely**

Create `docs/hlsl-sm4-sm5-compatibility.md` with rows for every current Cg 2.0
type, qualifier, expression/operator family, statement, semantic, intrinsic,
texture form, binding form, and geometry feature. Each row contains one of
`native`, `legalized`, `stage/model-specific`, `rejected C####`, or `not
exposed`, plus at least one registered test name. Do not leave blank status or
test cells.

- [ ] **Step 4: Document the public profiles**

Update `README.md` with the six names/targets, source-only boundary, public
`main`, example commands, required geometry options, modern system semantics,
cbuffer policy, same-index `tN/sN` sampler pairs, and optional `fxc` behavior.

- [ ] **Step 5: Prove generated sources are reproducible**

Run parser and standard-library regeneration and require no tracked diff:

```powershell
cmake --build build-hlsl-modern --config Release --target regenerate_parser regenerate_stdlib
git diff -- parser.c parser.h stdlib.c
```

Expected: no output.

- [ ] **Step 6: Manually qualify representative output**

```powershell
$cgcModern = Resolve-Path 'build-hlsl-modern\Release\cgc.exe'
& $cgcModern -quiet -profile hlslv40 -entry main -o build-hlsl-modern\position-v40.hlsl position.cg
& $cgcModern -quiet -profile hlslv50 -entry main -o build-hlsl-modern\position-v50.hlsl position.cg
& $cgcModern -quiet -profile hlslg40 -entry main -po TRIANGLE -po TRIANGLE_OUT -po Vertices=3 -o build-hlsl-modern\geometry-g40.hlsl tests\hlsl\geometry\pass_through.cg
& $cgcModern -quiet -profile hlslg50 -entry main -po TRIANGLE -po TRIANGLE_OUT -po Vertices=3 -o build-hlsl-modern\geometry-g50.hlsl tests\hlsl\geometry\pass_through.cg
& $cgcModern -quiet -profile hlslf40 -entry main -o build-hlsl-modern\pixel-p40.hlsl tests\hlsl\modern\textures.cg
& $cgcModern -quiet -profile hlslf50 -entry main -o build-hlsl-modern\pixel-p50.hlsl tests\hlsl\modern\textures.cg
if ($LASTEXITCODE -ne 0) { throw 'modern HLSL manual qualification failed' }
```

Expected: six readable files with exact profile/target metadata, stable
bindings, valid resources, internal entry, and one public `main`.

- [ ] **Step 7: Run complete Release and Debug qualification**

```powershell
cmake --build build-hlsl-modern --config Release
ctest --test-dir build-hlsl-modern -C Release --output-on-failure
cmake --build build-hlsl-modern --config Debug
ctest --test-dir build-hlsl-modern -C Debug --output-on-failure
```

Expected: both complete suites report zero failed tests. In the dedicated
Windows SDK qualification environment, configure with
`-DCGC_REQUIRE_FXC=ON` and require every registered external validation test
to run.

- [ ] **Step 8: Run final repository audit**

```powershell
git diff --check
rg -n "HlslSkeleton|Temporary HlslModule|Task [0-9]+ replaces" -g 'hlsl_*.c' -g 'hlsl_*.h' -g '*.md' .
$repositoryRoot = (Resolve-Path '.').Path
$modernBuildRoot = (Resolve-Path 'build-hlsl-modern').Path
if ((Split-Path -Parent $modernBuildRoot) -ne $repositoryRoot -or
    (Split-Path -Leaf $modernBuildRoot) -ne 'build-hlsl-modern') {
    throw "Refusing to remove unexpected build path: $modernBuildRoot"
}
Remove-Item -LiteralPath $modernBuildRoot -Recurse -Force
git status --short
```

Expected: no whitespace errors or incomplete-work markers. Status contains
only intended documentation/test changes plus unrelated untracked paths that
were already recorded at the Task 1 baseline; the plan-created
`build-hlsl-modern` directory is gone.

- [ ] **Step 9: Mark this plan complete and commit documentation**

Change every completed checkbox in this file to `[x]`, then run:

```powershell
git add -- README.md docs/hlsl-sm4-sm5-compatibility.md docs/superpowers/plans/2026-09-05-directx10-11-hlsl-sm4-sm5.md tests/validate_hlsl.cmake tests/CMakeLists.txt
git commit -m "Document Shader Model 4 and 5 HLSL profiles"
```

## Final Acceptance Checklist

- [ ] The DirectX 9 HLSL plan is complete and all SM3 goldens remain stable.
- [ ] Profile IDs 14–22 and connector IDs 18–35 are unique and pinned.
- [ ] All six modern profiles register with exact stage, target, and model.
- [ ] Every representable current Cg 2.0 construct emits verified deterministic
  HLSL; every unsupported construct has one precise diagnostic.
- [ ] Modern system-value types, interpolation, and cross-stage keys match the
  design specification.
- [ ] Every Cg sampler owns one same-index `tN/sN` pair and every supported
  texture intrinsic selects the exact method and legal stage.
- [ ] All five geometry inputs, three output streams, positive maximum, and
  three geometry operations reach explicit verified HLSL IR.
- [ ] Flat state is path-correct through helpers, branches, loops, repeated
  updates, and multiple appends.
- [ ] Exact SM4/SM5 resource boundaries pass and one-over cases fail
  transactionally.
- [ ] Matching SM4 and SM5 vertex–geometry–pixel pipelines pass; deliberate
  type, semantic, interpolation, and bridge mismatches fail.
- [ ] Every external qualification shader compiles under its exact `fxc`
  target when `fxc` is available.
- [ ] The compatibility matrix has no blank classification or test cell.
- [ ] Parser and standard-library regeneration leave no diff.
- [ ] Complete Debug and Release suites report zero failures.
- [ ] `git diff --check` reports no errors and `git status --short` contains no
  unintended plan-created files; baseline unrelated paths remain untouched.

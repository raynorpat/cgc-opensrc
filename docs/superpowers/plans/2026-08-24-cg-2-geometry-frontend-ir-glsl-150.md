# Cg 2.0 Geometry Frontend, IR, and GLSL 1.50 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add the complete Cg 2.0 geometry-program surface to the frontend and verified Cg IR, add a `glslg` geometry profile, and upgrade the existing GLSL profile family in place to core GLSL 1.50.

**Architecture:** Resolve source modifiers and repeated `-po` settings into one profile-independent `CgGeometryProgram` after entry selection and reachability, then lower its typed metadata and explicit operations into shared Cg IR. `generic` prints that verified IR, while `glslv`, `glslg`, and `glslf` validate a focused capability set and lower through one geometry-aware GLSL IR and verifier to core GLSL 1.50.

**Tech Stack:** ANSI C90 with the repository's existing extensions, GNU Bison generated parser sources, CMake/CTest, the Cg IR introduced by the base Cg 2.0 plan, shared GLSL IR/code generation, and `glslangValidator` for shader compilation and pipeline linking.

---

## Execution Preconditions

- Complete every task and release gate in
  `docs/superpowers/plans/2026-08-23-cg-2-language-ir.md` first. In
  particular, this plan starts only after verified `CgIRModule`, IR-oriented
  HAL callbacks, transactional output, the Cg 2.0 fixture harness, and
  `GlslLowerCgIR` exist and pass their full suites.
- Start this follow-on in an isolated worktree. Do not implement it directly
  in a worktree containing unrelated user changes.
- Preserve the pre-existing untracked `build-win32/` and unrelated plan files;
  never stage them.
- Use a dedicated build directory:

```powershell
cmake -S . -B build-cg20-geometry -DBUILD_TESTING=ON
cmake --build build-cg20-geometry --config Release
ctest --test-dir build-cg20-geometry -C Release --output-on-failure
```

- Install `glslangValidator` before Tasks 8-12. Early tasks may configure
  without it, but the final completion gate requires every generated shader
  and representative three-stage pipeline to validate.
- Treat `parser.y` as authoritative. Regenerate and commit `parser.c` and
  `parser.h` with every grammar change, using the generator version pinned by
  the base plan.
- Give every new `.c` and `.h` file the NVIDIA redistribution notice copied
  verbatim from `symbols.c`.
- Keep the implementation C90: declarations at block beginnings, no `//`
  comments in new C source, no compound literals, no designated initializers,
  and no variable-length arrays.
- Every task begins with a focused failing test, ends with its focused tests
  plus the complete suite passing, and receives its own commit.

## File and Responsibility Map

### New core files

- `cg_geometry.h`, `cg_geometry.c`: geometry option parsing, topology merging,
  shared stage/topology enums, selected-program state, attribute-array rules,
  geometry semantic classification, resolved output bundles, and reachable
  operation validation.
- `glsl_verify.c`: standalone structural and stage-aware verification of
  shared GLSL IR, including geometry layouts, built-ins, emit/restart nodes,
  and flat replay.
- `glslg_hal.c`: the `glslg` connector tables, semantic map, focused limits,
  descriptor, and HAL initializer.

### Existing core files

- `compile.h`, `cgstruct.c`, `cgcmain.c`: own and free repeatable raw `-po`
  arguments without interpreting geometry policy.
- `cg_overload.h`, `generic_hal.c`, `glslv_hal.c`, `glslf_hal.c`,
  `arbvp1_hal.c`, `arbfp1_hal.c`: add the geometry stage to shared profile
  identity and enforce stage/profile boundaries.
- `atom.c`, `scanner.c`, `parser.y`, generated `parser.c`, generated
  `parser.h`: tokenize and parse topology modifiers, `AttribArray<T>`, and
  semantic-bearing call arguments.
- `support.h`, `support.c`, `symbols.h`, `symbols.c`, `cg_types.h`,
  `cg_types.c`, `semantic.c`: preserve modifier/type/argument syntax and call
  `cg_geometry` after entry selection instead of embedding geometry policy.
- `cg_stdlib.def`, `cg_stdlib.h`, `cg_stdlib.c`, `stdlib.cg`, generated
  `stdlib.c`: assign stable special intrinsic identities to `emitVertex`,
  `flatAttrib`, and `restartStrip`.
- `cg_reach.h`, `cg_reach.c`, `cg_ir.h`, `cg_ir.c`, `cg_ir_lower.h`,
  `cg_ir_lower.c`, `cg_ir_verify.c`, `cg_ir_print.c`: carry selected geometry
  metadata, attribute-array types, explicit operations, invariants, lowering,
  and deterministic normalized output.
- `hal.h`, `hal.c`, `glsl_hal.h`, `glsl_hal.c`, `glsl_ir.h`, `glsl_ir.c`,
  `glsl_lower.c`, `glsl_codegen.c`: share profile validation and core 1.50
  target IR/lowering/code generation across vertex, geometry, and fragment.
- `errors.h`, `CMakeLists.txt`, `README.md`: diagnostics, build integration,
  and user-facing profile documentation.

### New tests and fixtures

- `tests/cg_geometry_test.c`: pure option/topology/semantic helpers and
  selected-program ownership.
- `tests/cg20/geometry/`: frontend, Cg IR, generic, and negative geometry
  fixtures.
- `tests/glsl/geometry/`: core 1.50 geometry goldens for topology, operations,
  interfaces, helpers, control flow, uniforms, and textures.
- `tests/glsl/link/gp_*.cg`: geometry members of positive and negative linked
  vertex-geometry-fragment pipelines.
- `tests/check_glsl150.cmake`: exact core-1.50 source contract checks.
- `tests/check_glsl_pipeline.cmake`: compile and link three independently
  generated stages.

### Modified test infrastructure

- `tests/check_cg20.cmake`, `tests/check_cg20_failure.cmake`,
  `tests/check_output_transaction.cmake`: pass repeatable `PROFILE_OPTIONS`
  and keep output transactional.
- `tests/check_glsl.cmake`, `tests/check_glsl_failure.cmake`,
  `tests/validate_glsl.cmake`, `tests/check_glsl_interface.cmake`,
  `tests/check_glsl_link.cmake`: recognize `glslg`, core 1.50 syntax, `geom`,
  and three-stage interfaces.
- `tests/cg_ir_test.c`, `tests/glsl_ir_test.c`,
  `tests/glsl_semantics_test.c`, `tests/cg20/conformance.csv`,
  `tests/CMakeLists.txt`: unit invariants, fixtures, manifest coverage, and
  validation registration.

### Stable diagnostic contract

Reserve this contiguous geometry block in `errors.h`; tests refer to the
symbol and numeric code shown here:

```c
#define ERROR_S_GEOMETRY_PROFILE_OPTION       6300, "invalid geometry profile option \"%s\""
#define ERROR_S_GEOMETRY_PROFILE_CONFLICT     6301, "conflicting geometry profile option \"%s\""
#define ERROR___GEOMETRY_INPUT_REQUIRED       6302, "geometry input topology is required"
#define ERROR_S_GEOMETRY_REPEATED_MODIFIER    6303, "repeated geometry modifier \"%s\""
#define ERROR_SS_GEOMETRY_MODIFIER_CONFLICT   6304, "geometry modifier \"%s\" conflicts with \"%s\""
#define ERROR___GEOMETRY_MODIFIER_FUNCTION    6305, "geometry modifier requires a function"
#define ERROR_S_GEOMETRY_ATTRIB_PLACEMENT     6306, "AttribArray is not permitted on %s"
#define ERROR_S_GEOMETRY_ATTRIB_ELEMENT       6307, "invalid AttribArray element type \"%s\""
#define ERROR___GEOMETRY_ATTRIB_READ_ONLY     6308, "AttribArray values are read-only"
#define ERROR___GEOMETRY_ATTRIB_STAGE         6309, "AttribArray requires a geometry program"
#define ERROR_S_GEOMETRY_OPERATION_ARITY      6310, "invalid arguments to geometry operation \"%s\""
#define ERROR_S_GEOMETRY_OPERATION_CONTEXT    6311, "geometry operation \"%s\" must be a complete statement"
#define ERROR___GEOMETRY_OUTPUT_SEMANTIC      6312, "geometry output value has no binding semantic"
#define ERROR_S_GEOMETRY_DUPLICATE_SEMANTIC   6313, "duplicate geometry output semantic \"%s\""
#define ERROR_S_GEOMETRY_SEMANTIC              6314, "invalid geometry semantic \"%s\""
#define ERROR___GEOMETRY_FLAT_POSITION        6315, "POSITION cannot be passed to flatAttrib"
#define ERROR_S_GEOMETRY_ENTRY_CALL           6316, "geometry entry \"%s\" cannot be called"
#define ERROR_S_GEOMETRY_STAGE                6317, "geometry operation \"%s\" is reachable from a non-geometry entry"
#define ERROR_SS_GLSL_PROFILE_STAGE           6318, "%s profile cannot compile a %s program"
#define ERROR___GLSL_GEOMETRY_MAX_REQUIRED    6319, "glslg requires -po Vertices=N"
```

Portable limit failures continue using the base GLSL
`ERROR_SII_GLSL_RESOURCE_LIMIT`; explicit Cg 1.1 uses the base plan's Cg
2.0-feature diagnostic.

## Task 1: Add Repeatable Geometry Profile Options and Topology Resolution

**Files:**

- Create: `cg_geometry.h`
- Create: `cg_geometry.c`
- Create: `tests/cg_geometry_test.c`
- Modify: `compile.h`
- Modify: `cgstruct.c`
- Modify: `cgcmain.c`
- Modify: `cg_overload.h`
- Modify: `hal.h`
- Modify: `generic_hal.c`
- Modify: `glslv_hal.c`
- Modify: `glslf_hal.c`
- Modify: `errors.h`
- Modify: `CMakeLists.txt`
- Modify: `tests/CMakeLists.txt`
- Modify: `tests/cg20/conformance.csv`

- [ ] **Step 1: Record the post-base-plan baseline**

Run:

```powershell
cmake -S . -B build-cg20-geometry -DBUILD_TESTING=ON
cmake --build build-cg20-geometry --config Release
ctest --test-dir build-cg20-geometry -C Release --output-on-failure
git status --short
```

Expected: every base-plan test passes. Record the CTest count in the eventual
pull-request notes; do not commit logs. Status contains only the known
unrelated untracked paths.

- [ ] **Step 2: Write failing option/topology unit tests**

Create `tests/cg_geometry_test.c`. Construct linked `CgProfileOption` values
and assert these exact cases:

```c
static void TestTopologyDefaults(void)
{
    CgGeometryModifiers source;
    CgGeometryOptions options;
    CgGeometryConfig config;
    CgGeometryDiagnostic diagnostic;

    CgGeometryInitModifiers(&source);
    CgGeometryInitOptions(&options);
    source.input = CG_GEOMETRY_INPUT_TRIANGLE_ADJACENCY;
    assert(CgGeometryResolveConfig(&source, &options,
           CGIR_STAGE_NEUTRAL, &config, &diagnostic));
    assert(config.stage == CGIR_STAGE_GEOMETRY);
    assert(config.inputTopology == CG_GEOMETRY_INPUT_TRIANGLE_ADJACENCY);
    assert(config.outputTopology == CG_GEOMETRY_OUTPUT_TRIANGLE_STRIP);
    assert(config.inputVertexCount == 6);
    assert(!config.hasMaxOutputVertices);
}

static void TestGeometryOptions(void)
{
    CgProfileOption third;
    CgProfileOption second;
    CgProfileOption first;
    CgGeometryOptions options;
    CgGeometryDiagnostic diagnostic;

    third.next = NULL;
    third.text = "Vertices=12";
    third.ordinal = 2;
    second.next = &third;
    second.text = "TRIANGLE_OUT";
    second.ordinal = 1;
    first.next = &second;
    first.text = "TRIANGLE";
    first.ordinal = 0;
    CgGeometryInitOptions(&options);
    assert(CgGeometryParseOptions(&first, &options, &diagnostic));
    assert(options.input == CG_GEOMETRY_INPUT_TRIANGLE);
    assert(options.output == CG_GEOMETRY_OUTPUT_TRIANGLE_STRIP);
    assert(options.hasMaxOutputVertices);
    assert(options.maxOutputVertices == 12);
}
```

Also assert all input counts `1, 2, 4, 3, 6`, all defaults, duplicate-equal
acceptance, duplicate-conflicting rejection, source/option conflict, unknown
option, zero, sign, suffix, trailing text, and decimal overflow. Check stable
`CgGeometryDiagnostic.reason` and option ordinal, never diagnostic text.
Include `GLSLVersion=150` as an unknown option to prove there is no hidden
profile-version selector.

Register `cg_geometry_unit` and run:

```powershell
cmake --build build-cg20-geometry --config Release --target cg_geometry_unit
```

Expected: compilation fails because `cg_geometry.h` and the API are absent.

- [ ] **Step 3: Define shared stage, topology, option, and diagnostic types**

Create `cg_geometry.h` with these public definitions:

```c
#if !defined(__CG_GEOMETRY_H)
#define __CG_GEOMETRY_H 1

#include <stddef.h>

typedef struct Symbol_Rec Symbol;
typedef union Type_Rec Type;
typedef union expr_rec expr;
typedef union stmt_rec stmt;
typedef struct CgReachGraph_Rec CgReachGraph;

typedef enum CgIRStage_Rec {
    CGIR_STAGE_UNKNOWN = 0,
    CGIR_STAGE_NEUTRAL,
    CGIR_STAGE_VERTEX,
    CGIR_STAGE_GEOMETRY,
    CGIR_STAGE_FRAGMENT
} CgIRStage;

typedef enum CgGeometryInput_Rec {
    CG_GEOMETRY_INPUT_UNKNOWN = 0,
    CG_GEOMETRY_INPUT_POINT,
    CG_GEOMETRY_INPUT_LINE,
    CG_GEOMETRY_INPUT_LINE_ADJACENCY,
    CG_GEOMETRY_INPUT_TRIANGLE,
    CG_GEOMETRY_INPUT_TRIANGLE_ADJACENCY
} CgGeometryInput;

typedef enum CgGeometryOutput_Rec {
    CG_GEOMETRY_OUTPUT_UNKNOWN = 0,
    CG_GEOMETRY_OUTPUT_POINTS,
    CG_GEOMETRY_OUTPUT_LINE_STRIP,
    CG_GEOMETRY_OUTPUT_TRIANGLE_STRIP
} CgGeometryOutput;

typedef struct CgProfileOption_Rec {
    struct CgProfileOption_Rec *next;
    const char *text;
    int ordinal;
} CgProfileOption;

typedef struct CgGeometryModifiers_Rec {
    CgGeometryInput input;
    CgGeometryOutput output;
    SourceLoc inputLoc;
    SourceLoc outputLoc;
} CgGeometryModifiers;

typedef struct CgGeometryOptions_Rec {
    CgGeometryInput input;
    CgGeometryOutput output;
    unsigned int maxOutputVertices;
    int hasMaxOutputVertices;
    int inputOrdinal;
    int outputOrdinal;
    int verticesOrdinal;
} CgGeometryOptions;

typedef struct CgGeometryConfig_Rec {
    CgIRStage stage;
    CgGeometryInput inputTopology;
    CgGeometryOutput outputTopology;
    unsigned int inputVertexCount;
    unsigned int maxOutputVertices;
    int hasMaxOutputVertices;
    SourceLoc inputLoc;
    SourceLoc outputLoc;
    SourceLoc maxVerticesLoc;
} CgGeometryConfig;

typedef enum CgGeometryDiagnosticReason_Rec {
    CG_GEOMETRY_DIAGNOSTIC_NONE = 0,
    CG_GEOMETRY_DIAGNOSTIC_UNKNOWN_OPTION,
    CG_GEOMETRY_DIAGNOSTIC_MALFORMED_VERTICES,
    CG_GEOMETRY_DIAGNOSTIC_REPEATED_INPUT,
    CG_GEOMETRY_DIAGNOSTIC_REPEATED_OUTPUT,
    CG_GEOMETRY_DIAGNOSTIC_CONFLICTING_INPUT,
    CG_GEOMETRY_DIAGNOSTIC_CONFLICTING_OUTPUT,
    CG_GEOMETRY_DIAGNOSTIC_DUPLICATE_VERTICES,
    CG_GEOMETRY_DIAGNOSTIC_SOURCE_OPTION_CONFLICT,
    CG_GEOMETRY_DIAGNOSTIC_MISSING_INPUT,
    CG_GEOMETRY_DIAGNOSTIC_STAGE_CONFLICT,
    CG_GEOMETRY_DIAGNOSTIC_OPERATION_ARITY,
    CG_GEOMETRY_DIAGNOSTIC_OPERATION_CONTEXT,
    CG_GEOMETRY_DIAGNOSTIC_OUTPUT_SEMANTIC,
    CG_GEOMETRY_DIAGNOSTIC_DUPLICATE_SEMANTIC,
    CG_GEOMETRY_DIAGNOSTIC_SEMANTIC,
    CG_GEOMETRY_DIAGNOSTIC_FLAT_POSITION,
    CG_GEOMETRY_DIAGNOSTIC_ENTRY_CALL,
    CG_GEOMETRY_DIAGNOSTIC_REACHABLE_STAGE,
    CG_GEOMETRY_DIAGNOSTIC_ALLOCATION
} CgGeometryDiagnosticReason;

typedef struct CgGeometryDiagnostic_Rec {
    CgGeometryDiagnosticReason reason;
    SourceLoc loc;
    int optionOrdinal;
    const char *optionText;
} CgGeometryDiagnostic;

void CgGeometryInitModifiers(CgGeometryModifiers *modifiers);
void CgGeometryInitOptions(CgGeometryOptions *options);
int CgGeometryParseOptions(const CgProfileOption *first,
                           CgGeometryOptions *options,
                           CgGeometryDiagnostic *diagnostic);
int CgGeometryResolveConfig(const CgGeometryModifiers *source,
                            const CgGeometryOptions *options,
                            CgIRStage profileStage,
                            CgGeometryConfig *config,
                            CgGeometryDiagnostic *diagnostic);
unsigned int CgGeometryInputVertexCount(CgGeometryInput input);
CgGeometryOutput CgGeometryDefaultOutput(CgGeometryInput input);
const char *CgGeometryInputName(CgGeometryInput input);
const char *CgGeometryOutputName(CgGeometryOutput output);

#endif
```

`CGIR_STAGE_NEUTRAL` is the resolved stage for a non-geometry program under
`generic`; this preserves existing normalized output while keeping
`CGIR_STAGE_UNKNOWN` illegal after entry resolution.

- [ ] **Step 4: Implement strict option parsing and topology merging**

In `cg_geometry.c`, use fixed spelling tables and a digit-by-digit overflow
check. Do not call `strtol`, accept whitespace, signs, suffixes, or partial
matches. Implement `CgGeometryResolveConfig` in this order:

```c
resolvedInput = source->input != CG_GEOMETRY_INPUT_UNKNOWN ?
                source->input : options->input;
resolvedOutput = source->output != CG_GEOMETRY_OUTPUT_UNKNOWN ?
                 source->output : options->output;
isGeometry = resolvedInput != CG_GEOMETRY_INPUT_UNKNOWN ||
             resolvedOutput != CG_GEOMETRY_OUTPUT_UNKNOWN ||
             options->hasMaxOutputVertices ||
             profileStage == CGIR_STAGE_GEOMETRY;
```

Reject contradictory source/option values. If `isGeometry`, require an input,
derive a missing output from it, derive the exact input count, and set geometry
stage. Otherwise copy the resolved vertex, fragment, or neutral profile stage.
Never reject an unknown maximum here.

- [ ] **Step 5: Store every raw `-po` occurrence without policy**

Add `CgProfileOption *profileOptions` and `CgProfileOption **profileOptionsTail`
to `Options`; forward-declare `typedef struct CgProfileOption_Rec
CgProfileOption;` in `compile.h` so it need not include geometry policy.
Initialize the tail to `&options.profileOptions` in
`InitCgStruct`, append one heap-owned node for each `-po value` during command
line pass zero, and free the nodes in `FreeCgStruct`.

Add this branch before the generic bad-option branch in `CommandLineArgs`:

```c
} else if (!strcmp(argv[ii], "-po")) {
    ii++;
    if (ii >= argc) {
        printf(OPENSL_TAG ": missing profile option after \"-po\"\n");
        return 0;
    }
    if (pass == 0 && !AppendProfileOption(&Cg->options, argv[ii]))
        return 0;
```

Update help text to show `[-po option]...`. The second pass must increment
`ii` but must not append a second copy.

- [ ] **Step 6: Extend shared profile stage identity**

Add `CG_PROFILE_STAGE_GEOMETRY` to `CgProfileStage` in `cg_overload.h`. Add a
`CgProfileProgramStage(const CgProfileIdentity *identity)` conversion helper
to `hal.h`/`hal.c`, mapping neutral, vertex, geometry, and fragment profile
identity to the matching `CgIRStage`. `generic` therefore maps to neutral,
`glslv` to vertex, and `glslf` to fragment. Do not register `glslg` yet.

```c
typedef enum CgProfileStage_Rec {
    CG_PROFILE_STAGE_NEUTRAL = 0,
    CG_PROFILE_STAGE_VERTEX,
    CG_PROFILE_STAGE_GEOMETRY,
    CG_PROFILE_STAGE_FRAGMENT
} CgProfileStage;
```

- [ ] **Step 7: Connect option diagnostics and run tests**

Add the stable diagnostic block specified above to `errors.h`. The compile
integration in Task 5 maps structured reasons to these codes. This task tests
the pure parser plus command-line ownership:

```powershell
cmake --build build-cg20-geometry --config Release --target cg_geometry_unit cgc
ctest --test-dir build-cg20-geometry -C Release -R "cg_geometry_unit|cg20_manifest" --output-on-failure
& .\build-cg20-geometry\Release\cgc.exe -po
```

Expected: unit and manifest tests pass; the direct invocation exits nonzero
and reports the missing value without crashing or leaking under the available
runtime diagnostics.

- [ ] **Step 8: Commit option and topology infrastructure**

```powershell
git add cg_geometry.h cg_geometry.c compile.h cgstruct.c cgcmain.c cg_overload.h hal.h generic_hal.c glslv_hal.c glslf_hal.c errors.h CMakeLists.txt tests/CMakeLists.txt tests/cg_geometry_test.c tests/cg20/conformance.csv
git commit -m "Add geometry profile options"
```

## Task 2: Parse Geometry Modifiers and Preserve Them on Functions

**Files:**

- Modify: `atom.c`
- Modify: `parser.y`
- Regenerate: `parser.c`
- Regenerate: `parser.h`
- Modify: `support.h`
- Modify: `support.c`
- Modify: `symbols.h`
- Modify: `symbols.c`
- Modify: `cg_geometry.h`
- Modify: `cg_geometry.c`
- Modify: `language.c`
- Modify: `errors.h`
- Create: `tests/cg20/geometry/modifiers.cg`
- Create: `tests/cg20/geometry/options_only.cg`
- Create: `tests/cg20/geometry/default_outputs.cg`
- Create: `tests/cg20/diagnostics/geometry_duplicate_modifier.cg`
- Create: `tests/cg20/diagnostics/geometry_conflicting_modifier.cg`
- Create: `tests/cg20/diagnostics/geometry_modifier_on_variable.cg`
- Create: `tests/cg20/diagnostics/geometry_cg11.cg`
- Modify: `tests/CMakeLists.txt`
- Modify: `tests/cg20/conformance.csv`

- [ ] **Step 1: Add failing parser fixtures**

Create `tests/cg20/geometry/modifiers.cg`:

```c
POINT void point_main(AttribArray<float4> p : POSITION) { }
LINE LINE_OUT void line_main(AttribArray<float4> p : POSITION) { }
LINE_ADJ void line_adj_main(AttribArray<float4> p : POSITION) { }
TRIANGLE POINT_OUT void triangle_main(AttribArray<float4> p : POSITION) { }
TRIANGLE_ADJ TRIANGLE_OUT void triangle_adj_main(
    AttribArray<float4> p : POSITION) { }
```

Create one-entry `options_only.cg` with no source modifier and register it with
`PROFILE_OPTIONS` equal to `TRIANGLE;Vertices=3`. Create
`default_outputs.cg` with five entries and normalized expectations for points,
line strip, line strip, triangle strip, and triangle strip.

Add negative fixtures with repeated identical modifiers, contradictory input
modifiers, a modifier before a global variable, and `POINT` under
`-version 1.1`. Run the focused tests and expect scanner/parser failures.

- [ ] **Step 2: Add fixed token values without renumbering old tokens**

Append after the base plan's last fixed token in `parser.y` and add matching
spellings in `atom.c`:

```c
POINT_SY          "POINT"
LINE_SY           "LINE"
LINE_ADJ_SY       "LINE_ADJ"
TRIANGLE_SY       "TRIANGLE"
TRIANGLE_ADJ_SY   "TRIANGLE_ADJ"
POINT_OUT_SY      "POINT_OUT"
LINE_OUT_SY       "LINE_OUT"
TRIANGLE_OUT_SY   "TRIANGLE_OUT"
```

Assign explicit monotonically increasing token numbers. Move
`FIRST_USER_TOKEN_SY` after them. Do not alter any prior numeric token because
`stdlib.c` embeds token values.

- [ ] **Step 3: Carry modifiers through declarators**

Add `CgGeometryModifiers geometry` to `dtype` and
`CgGeometryModifiers geometry` to `FunSymbol`. Initialize it in `SetDType` and
copy it in `FunctionDeclHeader`, redeclaration merging, and function
definition. Ordinary non-functions with a nonempty modifier record receive
`ERROR___GEOMETRY_MODIFIER_REQUIRES_FUNCTION`.

Declare and implement:

```c
int CgGeometryApplyInputModifier(CgGeometryModifiers *modifiers,
                                 CgGeometryInput input,
                                 const SourceLoc *loc,
                                 CgGeometryDiagnostic *diagnostic);
int CgGeometryApplyOutputModifier(CgGeometryModifiers *modifiers,
                                  CgGeometryOutput output,
                                  const SourceLoc *loc,
                                  CgGeometryDiagnostic *diagnostic);
```

Repeated identical values and contradictory values both fail, but use
different structured reasons so diagnostics can name the first location.
Add parser-facing wrappers in `support.h`/`support.c`:

```c
int SetGeometryInputModifier(SourceLoc *loc, dtype *specifiers,
                             CgGeometryInput input);
int SetGeometryOutputModifier(SourceLoc *loc, dtype *specifiers,
                              CgGeometryOutput output);
```

Each wrapper calls the pure helper, maps its structured reason to C6303 or
C6304, and returns zero after a diagnostic so parser recovery can continue.

- [ ] **Step 4: Add grammar productions at declaration-specifier position**

Add `%type <sc_int> geometry_modifier` and these productions:

```yacc
geometry_modifier:
          POINT_SY        { $$ = SetGeometryInputModifier(Cg->tokenLoc, &CurrentDeclTypeSpecs, CG_GEOMETRY_INPUT_POINT); }
        | LINE_SY         { $$ = SetGeometryInputModifier(Cg->tokenLoc, &CurrentDeclTypeSpecs, CG_GEOMETRY_INPUT_LINE); }
        | LINE_ADJ_SY     { $$ = SetGeometryInputModifier(Cg->tokenLoc, &CurrentDeclTypeSpecs, CG_GEOMETRY_INPUT_LINE_ADJACENCY); }
        | TRIANGLE_SY     { $$ = SetGeometryInputModifier(Cg->tokenLoc, &CurrentDeclTypeSpecs, CG_GEOMETRY_INPUT_TRIANGLE); }
        | TRIANGLE_ADJ_SY { $$ = SetGeometryInputModifier(Cg->tokenLoc, &CurrentDeclTypeSpecs, CG_GEOMETRY_INPUT_TRIANGLE_ADJACENCY); }
        | POINT_OUT_SY    { $$ = SetGeometryOutputModifier(Cg->tokenLoc, &CurrentDeclTypeSpecs, CG_GEOMETRY_OUTPUT_POINTS); }
        | LINE_OUT_SY     { $$ = SetGeometryOutputModifier(Cg->tokenLoc, &CurrentDeclTypeSpecs, CG_GEOMETRY_OUTPUT_LINE_STRIP); }
        | TRIANGLE_OUT_SY { $$ = SetGeometryOutputModifier(Cg->tokenLoc, &CurrentDeclTypeSpecs, CG_GEOMETRY_OUTPUT_TRIANGLE_STRIP); }
;
```

Accept `geometry_modifier abstract_declaration_specifiers` in the same
modifier position as `inline`. The action updates
`CurrentDeclTypeSpecs.geometry`; it does not select a backend or derive a
default output during parsing.

- [ ] **Step 5: Gate all new tokens through Cg 2.0 language policy**

Extend the base plan's keyword table so these spellings are enabled only in
Cg 2.0. In explicit 1.1 mode, emit the stable Cg 2.0-feature diagnostic at the
token and recover at the declaration boundary.

```c
int CgLanguageAllowsGeometry(CgLanguageVersion version)
{
    return version == CG_LANGUAGE_2_0;
}
```

Declare the helper in `language.h`. Call it from both parser-facing modifier
wrappers and the `AttribArray`/special-operation paths so every new form uses
one version decision.

- [ ] **Step 6: Regenerate parser artifacts and check conflicts**

```powershell
cmake --build build-cg20-geometry --config Release --target regenerate_parser
git diff -- parser.c parser.h
cmake --build build-cg20-geometry --config Release
ctest --test-dir build-cg20-geometry -C Release -R "geometry_.*modifier|parser_regeneration" --output-on-failure
```

Expected: generated files change deterministically, no unexplained grammar
conflict is added, valid modifiers parse, and every negative fixture reports
its exact code and line.

- [ ] **Step 7: Commit modifier parsing and generated files**

```powershell
git add atom.c parser.y parser.c parser.h support.h support.c symbols.h symbols.c cg_geometry.h cg_geometry.c language.c errors.h tests/CMakeLists.txt tests/cg20
git commit -m "Parse geometry topology modifiers"
```

## Task 3: Add Canonical Read-Only `AttribArray<T>` Types

**Files:**

- Modify: `atom.c`
- Modify: `parser.y`
- Regenerate: `parser.c`
- Regenerate: `parser.h`
- Modify: `symbols.h`
- Modify: `symbols.c`
- Modify: `cg_types.h`
- Modify: `cg_types.c`
- Modify: `support.h`
- Modify: `support.c`
- Modify: `semantic.c`
- Modify: `cg_geometry.h`
- Modify: `cg_geometry.c`
- Modify: `printutils.c`
- Modify: `language.c`
- Modify: `errors.h`
- Modify: `tests/cg_types_test.c`
- Modify: `tests/cg_geometry_test.c`
- Create: `tests/cg20/geometry/attrib_array.cg`
- Create: `tests/cg20/geometry/attrib_array_aggregate.cg`
- Create: `tests/cg20/geometry/attrib_array_helper.cg`
- Create: `tests/cg20/geometry/attrib_array_length.cg`
- Create: `tests/cg20/diagnostics/geometry_attrib_array_global.cg`
- Create: `tests/cg20/diagnostics/geometry_attrib_array_uniform.cg`
- Create: `tests/cg20/diagnostics/geometry_attrib_array_output.cg`
- Create: `tests/cg20/diagnostics/geometry_attrib_array_return.cg`
- Create: `tests/cg20/diagnostics/geometry_attrib_array_local.cg`
- Create: `tests/cg20/diagnostics/geometry_attrib_array_member.cg`
- Create: `tests/cg20/diagnostics/geometry_attrib_array_non_geometry.cg`
- Create: `tests/cg20/diagnostics/geometry_attrib_array_void.cg`
- Create: `tests/cg20/diagnostics/geometry_attrib_array_sampler.cg`
- Create: `tests/cg20/diagnostics/geometry_attrib_array_function.cg`
- Create: `tests/cg20/diagnostics/geometry_attrib_array_interface.cg`
- Create: `tests/cg20/diagnostics/geometry_attrib_array_write_whole.cg`
- Create: `tests/cg20/diagnostics/geometry_attrib_array_write_element.cg`
- Create: `tests/cg20/diagnostics/geometry_attrib_array_cg11.cg`
- Modify: `tests/CMakeLists.txt`
- Modify: `tests/cg20/conformance.csv`

- [ ] **Step 1: Write failing canonical-type tests**

Extend `tests/cg_types_test.c`:

```c
Type *unresolved;
Type *resolved;

unresolved = CgGetAttribArrayType(Float4Type, 0);
resolved = CgGetAttribArrayType(Float4Type, 3);
assert(CgIsAttribArray(unresolved));
assert(CgAttribArrayElement(unresolved) == Float4Type);
assert(CgAttribArrayExtent(unresolved) == 0);
assert(CgAttribArrayExtent(resolved) == 3);
assert(resolved == CgGetAttribArrayType(Float4Type, 3));
assert(resolved != CgGetAttribArrayType(Float4Type, 6));
assert(!IsArray(resolved));
```

Extend `tests/cg_geometry_test.c` to assert the element-type predicate accepts
scalar, vector, matrix, ordinary array, and nested struct values but rejects
void, function, sampler, and interface types.

- [ ] **Step 2: Add a distinct canonical type category and interner**

Reserve `TYPE_CATEGORY_ATTRIB_ARRAY` in `symbols.h`; do not reuse
`TYPE_CATEGORY_ARRAY`. Add `TypeAttribArray` with the exact common prefix
`properties`, `size`, followed by `Type *eltype` and `int extent`; append its
`attrarr` member to `Type_Rec`. Do not extend or reinterpret `TypeArray`.

Declare in `cg_types.h`:

```c
Type *CgGetAttribArrayType(Type *element, unsigned int extent);
int CgIsAttribArray(const Type *type);
Type *CgAttribArrayElement(const Type *type);
unsigned int CgAttribArrayExtent(const Type *type);
```

Intern on `(canonical element pointer, extent)` in the symbol-table lifetime.
Extent zero means unresolved source shape; extents `1, 2, 3, 4, 6` are the
resolved topology shapes. Update type equality, canonical printing, and type
ownership without making ordinary `IsArray` helpers accept attribute arrays.

- [ ] **Step 3: Parse the template spelling only after its reserved token**

Add `ATTRIBARRAY_SY` with spelling `AttribArray` and grammar:

```yacc
type_specifier:
          ATTRIBARRAY_SY '<' type_specifier '>'
              { $$ = CgGetAttribArrayType($3, 0); }
```

Because parsing enters this production only after the reserved token, normal
relational `<` and `>` expressions remain untouched. Gate the token to Cg 2.0
and regenerate `parser.c`/`parser.h` in this step.

- [ ] **Step 4: Validate declaration placement and read-only behavior**

Implement `CgGeometryValidateAttribArrayDeclaration` and call it from normal
declaration checking. Permit only `in` varying formals on a geometry entry or
reachable helper. Reject globals, uniforms, outputs, returns, struct members,
locals, non-geometry entry formals, and illegal elements with separate error
codes.

```c
typedef enum CgGeometryDeclarationUse_Rec {
    CG_GEOMETRY_DECL_ENTRY_INPUT,
    CG_GEOMETRY_DECL_HELPER_INPUT,
    CG_GEOMETRY_DECL_GLOBAL,
    CG_GEOMETRY_DECL_UNIFORM,
    CG_GEOMETRY_DECL_OUTPUT,
    CG_GEOMETRY_DECL_RETURN,
    CG_GEOMETRY_DECL_MEMBER,
    CG_GEOMETRY_DECL_LOCAL
} CgGeometryDeclarationUse;

int CgGeometryValidateAttribArrayDeclaration(
    const CgGeometryProgram *program, const Symbol *symbol,
    CgGeometryDeclarationUse use, CgGeometryDiagnostic *diagnostic);
```

Extend assignment/lvalue checking so indexing an attribute array yields its
element type but `IsLValue == 0`; whole-array assignment also fails. Passing
one to an equivalent helper formal remains valid.

- [ ] **Step 5: Delay and then fold `.length`**

When base array member handling sees `AttribArray`, construct the existing
typed length expression without demanding a source extent. After topology
resolution, `CgGeometryResolveLengths` walks the selected reachability graph
and replaces each such node with an `int` constant equal to
`config.inputVertexCount`. It rejects a use outside a resolved geometry
program instead of returning zero.

Do not mutate the unresolved canonical source type. Record the selected
resolved view `(source Type *, resolved Type *)` in `CgGeometryProgram`; Cg IR
lowering uses the resolved type while other entry candidates retain their
unresolved source type.

```c
int CgGeometryResolveLengths(CgGeometryProgram *program,
                             const CgReachGraph *reach,
                             CgGeometryDiagnostic *diagnostic);
```

- [ ] **Step 6: Add positive and negative shader fixtures**

`attrib_array.cg` indexes a `float4` input and checks `.length`.
`attrib_array_aggregate.cg` uses a nested struct element with member semantics.
`attrib_array_helper.cg` passes the array to an equivalent reachable helper.
`attrib_array_length.cg` contains five topology-qualified entries and returns
or emits each exact length so compiler goldens cover `1, 2, 3, 4, 6`.

```c
TRIANGLE void triangle_length(AttribArray<float4> p : POSITION)
{
    emitVertex(p[0] : POSITION, p.length : TEXCOORD0);
}
```

Create one minimal negative file for each prohibited placement, illegal
element family, whole assignment, element assignment, non-geometry entry, and
explicit Cg 1.1 usage. Register exact code, line, and message fragment.

- [ ] **Step 7: Run type, parser, geometry, and full tests**

```powershell
cmake --build build-cg20-geometry --config Release --target regenerate_parser
cmake --build build-cg20-geometry --config Release
ctest --test-dir build-cg20-geometry -C Release -R "cg_types_unit|cg_geometry_unit|cg20_.*attrib_array|parser_regeneration" --output-on-failure
ctest --test-dir build-cg20-geometry -C Release --output-on-failure
```

Expected: all focused and full tests pass; parser regeneration leaves only the
checked-in intended changes.

- [ ] **Step 8: Commit attribute arrays**

```powershell
git add atom.c parser.y parser.c parser.h symbols.h symbols.c cg_types.h cg_types.c support.h support.c semantic.c cg_geometry.h cg_geometry.c printutils.c language.c errors.h tests/cg_types_test.c tests/cg_geometry_test.c tests/CMakeLists.txt tests/cg20
git commit -m "Add geometry attribute arrays"
```

## Task 4: Parse and Resolve Geometry Operations

**Files:**

- Modify: `parser.y`
- Regenerate: `parser.c`
- Regenerate: `parser.h`
- Modify: `support.h`
- Modify: `support.c`
- Modify: `cg_stdlib.def`
- Modify: `cg_stdlib.h`
- Modify: `cg_stdlib.c`
- Modify: `stdlib.cg`
- Regenerate: `stdlib.c`
- Modify: `cg_geometry.h`
- Modify: `cg_geometry.c`
- Modify: `semantic.c`
- Modify: `errors.h`
- Modify: `tests/cg_geometry_test.c`
- Modify: `tests/cg_stdlib_test.c`
- Create: `tests/cg20/geometry/operations.cg`
- Create: `tests/cg20/geometry/aggregate_bundles.cg`
- Create: `tests/cg20/geometry/semantic_sources.cg`
- Create: `tests/cg20/diagnostics/geometry_operation_empty_emit.cg`
- Create: `tests/cg20/diagnostics/geometry_operation_empty_flat.cg`
- Create: `tests/cg20/diagnostics/geometry_operation_restart_args.cg`
- Create: `tests/cg20/diagnostics/geometry_operation_unresolved_semantic.cg`
- Create: `tests/cg20/diagnostics/geometry_operation_duplicate_semantic.cg`
- Create: `tests/cg20/diagnostics/geometry_operation_flat_position.cg`
- Create: `tests/cg20/diagnostics/geometry_operation_regular_annotation.cg`
- Create: `tests/cg20/diagnostics/geometry_operation_nested_assignment.cg`
- Create: `tests/cg20/diagnostics/geometry_operation_nested_constructor.cg`
- Create: `tests/cg20/diagnostics/geometry_operation_nested_conditional.cg`
- Create: `tests/cg20/diagnostics/geometry_operation_nested_return.cg`
- Create: `tests/cg20/diagnostics/geometry_operation_nested_argument.cg`
- Create: `tests/cg20/diagnostics/geometry_operation_nested_arithmetic.cg`
- Create: `tests/cg20/diagnostics/geometry_operation_cg11.cg`
- Create: `tests/cg20/diagnostics/geometry_operation_annotation_cg11.cg`
- Modify: `tests/CMakeLists.txt`
- Modify: `tests/cg20/conformance.csv`

- [ ] **Step 1: Add failing intrinsic-identity and bundle tests**

Extend `tests/cg_stdlib_test.c` to assert the catalog contains exactly one
special identity for each name:

```c
assert(CgFindIntrinsicByName("emitVertex") == CG_INTRINSIC_EMIT_VERTEX);
assert(CgFindIntrinsicByName("flatAttrib") == CG_INTRINSIC_FLAT_ATTRIB);
assert(CgFindIntrinsicByName("restartStrip") == CG_INTRINSIC_RESTART_STRIP);
assert(CgIntrinsicIsGeometrySpecial(CG_INTRINSIC_EMIT_VERTEX));
assert(CgIntrinsicIsGeometrySpecial(CG_INTRINSIC_FLAT_ATTRIB));
assert(CgIntrinsicIsGeometrySpecial(CG_INTRINSIC_RESTART_STRIP));
```

Extend `tests/cg_geometry_test.c` with a synthetic nested struct whose leaves
are `POSITION`, `COLOR0`, and `TEXCOORD0`. Resolve it and assert declaration
order, canonical semantic atoms, source semantic atoms, canonical leaf types,
and source locations. Add unresolved and case-insensitive duplicate cases.

Run both units and expect missing identities and resolver APIs.

- [ ] **Step 2: Add special intrinsic catalog entries**

Extend the catalog DSL with a non-overloaded special-entry form:

```c
CG_STDLIB_SPECIAL(CG_INTRINSIC_EMIT_VERTEX, "emitVertex",
                  CG_INTRINSIC_FLAG_SIDE_EFFECT | CG_INTRINSIC_FLAG_GEOMETRY)
CG_STDLIB_SPECIAL(CG_INTRINSIC_FLAT_ATTRIB, "flatAttrib",
                  CG_INTRINSIC_FLAG_SIDE_EFFECT | CG_INTRINSIC_FLAG_GEOMETRY)
CG_STDLIB_SPECIAL(CG_INTRINSIC_RESTART_STRIP, "restartStrip",
                  CG_INTRINSIC_FLAG_SIDE_EFFECT | CG_INTRINSIC_FLAG_GEOMETRY)
```

Install symbols for these identities during standard-library initialization,
but do not expand ordinary fixed signatures. Remove any portable declaration
with these names from `stdlib.cg`; the semantic special-call path owns arity,
annotations, types, and placement.
Install the special symbols only when `CgLanguageAllowsGeometry` is true, so
explicit Cg 1.1 cannot reach a geometry intrinsic through the catalog.

Declare in `cg_stdlib.h` and implement by checking immutable catalog flags:

```c
int CgIntrinsicIsGeometrySpecial(CgIntrinsic intrinsic);
```

- [ ] **Step 3: Preserve semantic-bearing actual arguments**

Add a dedicated `GEOMETRY_ARGUMENT_OP` expression kind in `support.h`. Its
record stores the wrapped typed expression, optional inline semantic atom,
and `SourceLoc`. Add:

```c
expr *NewGeometryArgument(SourceLoc *loc, expr *value, int semantic);
int IsGeometryArgument(const expr *value);
expr *GetGeometryArgumentValue(expr *value);
int GetGeometryArgumentSemantic(const expr *value);
```

Add `%type <sc_expr> actual_argument` and change only the call-argument
grammar:

```yacc
actual_argument:
          expression
              { $$ = NewGeometryArgument(Cg->tokenLoc, $1, 0); }
        | expression ':' semantics_identifier
              { $$ = NewGeometryArgument(Cg->tokenLoc, $1, $3); }
;

non_empty_argument_list:
          actual_argument
        | non_empty_argument_list ',' actual_argument
              { $$ = NewBinopNode(FUN_ARG_OP, $1, $3); }
;
```

Ordinary call resolution unwraps arguments with semantic zero. It diagnoses a
nonzero annotation unless the selected intrinsic has the geometry-special
flag. This keeps annotations illegal on arbitrary calls.

- [ ] **Step 4: Define resolved frontend operation records**

Add to `cg_geometry.h`:

```c
typedef enum CgGeometryOperationKind_Rec {
    CG_GEOMETRY_OPERATION_EMIT,
    CG_GEOMETRY_OPERATION_FLAT,
    CG_GEOMETRY_OPERATION_RESTART
} CgGeometryOperationKind;

typedef struct CgGeometryValue_Rec {
    struct CgGeometryValue_Rec *next;
    int canonicalSemantic;
    int sourceSemantic;
    Type *type;
    expr *value;
    SourceLoc loc;
} CgGeometryValue;

typedef struct CgGeometryOperation_Rec {
    struct CgGeometryOperation_Rec *next;
    CgGeometryOperationKind kind;
    stmt *statement;
    CgGeometryValue *values;
    SourceLoc loc;
} CgGeometryOperation;

typedef struct CgGeometryTypeView_Rec {
    struct CgGeometryTypeView_Rec *next;
    Type *sourceType;
    Type *resolvedType;
} CgGeometryTypeView;

typedef void *(*CgGeometryAllocFn)(void *arg, size_t size);

typedef struct CgGeometryProgram_Rec {
    CgGeometryAllocFn alloc;
    void *allocArg;
    CgGeometryConfig config;
    Symbol *entry;
    CgGeometryTypeView *typeViews;
    CgGeometryOperation *operations;
    int failed;
} CgGeometryProgram;
```

Initialize the record through this Cg-owned allocator API:

```c
void CgGeometryInitProgram(CgGeometryProgram *program,
                           CgGeometryAllocFn alloc, void *allocArg);
CgGeometryOperation *CgGeometryFindOperation(
                           const CgGeometryProgram *program,
                           const stmt *statement);
Type *CgGeometryFindResolvedType(const CgGeometryProgram *program,
                                 const Type *sourceType);
```

Allocate all records from the selected program/module pool. Add lookup helpers
by source `Type *` and `stmt *`; never store them in the four backend-temporary
AST pointer slots.

- [ ] **Step 5: Resolve bundles recursively and deterministically**

Declare and implement:

```c
int CgGeometryResolveBundle(CgGeometryProgram *program,
                            CgGeometryOperationKind kind,
                            expr *arguments,
                            CgGeometryValue **values,
                            CgGeometryDiagnostic *diagnostic);
```

For each argument, select a semantic in
this exact order: inline annotation, directly referenced declaration,
selected aggregate member, then direct indexing of an attribute-array
parameter. Arithmetic and constructors do not inherit semantics.

Recursively flatten aggregates in source declaration order. Each leaf becomes
one `CgGeometryValue`. Canonicalize semantic root case and numeric suffix for
equality while preserving the source atom. Reject an unresolved leaf, empty
bundle, illegal value type, or duplicate canonical semantic. For
`flatAttrib`, additionally reject canonical `POSITION`.

- [ ] **Step 6: Recognize complete-statement special calls**

Add a post-typecheck call classifier that identifies the selected intrinsic
identity rather than its spelling. Require `emitVertex` and `flatAttrib` to
have at least one argument, `restartStrip` to have none, and all three calls to
be the complete expression of an expression statement. Calls nested inside an
assignment, constructor, conditional, return, argument, or arithmetic node
receive `ERROR_S_GEOMETRY_OPERATION_CONTEXT`.

Given an initialized `CgGeometryProgram`, the helper constructs
`CgGeometryOperation` records; Task 5 connects that helper to selected-entry
analysis and reachable stage legality. Global classification still checks
arity, wrapper syntax, value types, and expression context in unreachable code
so malformed dead code is diagnosed consistently.

```c
int CgGeometryClassifyOperationStatement(CgGeometryProgram *program,
                                         stmt *statement,
                                         CgGeometryDiagnostic *diagnostic);
```

- [ ] **Step 7: Add exact operation fixtures**

Create `operations.cg` with all three operations in a triangle entry and a
reachable helper. Create `aggregate_bundles.cg` with nested struct leaves and
`semantic_sources.cg` covering inline, declaration, member, and direct
attribute-index inheritance.

Create one negative fixture for empty emit/flat, restart arguments,
unresolved expression, duplicate case-insensitive semantics, `POSITION` in
flat, annotation on an ordinary call, and every prohibited expression
context. Add explicit Cg 1.1 fixtures for a special operation and an annotated
geometry argument. Each file asserts its exact diagnostic code and line.
Register the three positive compiler fixtures as `DISABLED TRUE` until Task 5
connects selected-program analysis; unit bundle tests and all negative global
classification fixtures are enabled in this task.

```c
TRIANGLE void operation_main(AttribArray<float4> p : POSITION,
                             uniform float4 flatColor)
{
    flatAttrib(flatColor : COLOR0);
    emitVertex(p[0] : POSITION);
    restartStrip();
}
```

- [ ] **Step 8: Regenerate and run focused/full tests**

```powershell
cmake --build build-cg20-geometry --config Release --target regenerate_parser regenerate_stdlib
cmake --build build-cg20-geometry --config Release
ctest --test-dir build-cg20-geometry -C Release -R "cg_geometry_unit|cg_stdlib_unit|cg20_.*geometry_operation|stdlib_regeneration|parser_regeneration" --output-on-failure
ctest --test-dir build-cg20-geometry -C Release --output-on-failure
```

Expected: bundle order and enabled diagnostics are stable, generated files
reproduce, and the complete enabled suite passes.

- [ ] **Step 9: Commit operation parsing and resolution**

```powershell
git add parser.y parser.c parser.h support.h support.c cg_stdlib.def cg_stdlib.h cg_stdlib.c stdlib.cg stdlib.c cg_geometry.h cg_geometry.c semantic.c errors.h tests/cg_geometry_test.c tests/cg_stdlib_test.c tests/CMakeLists.txt tests/cg20
git commit -m "Resolve Cg geometry operations"
```

## Task 5: Resolve Geometry Programs, Reachability, and Semantics

**Files:**

- Modify: `cg_geometry.h`
- Modify: `cg_geometry.c`
- Modify: `cg_reach.h`
- Modify: `cg_reach.c`
- Modify: `semantic.c`
- Modify: `compile.c`
- Modify: `hal.h`
- Modify: `errors.h`
- Modify: `tests/cg_geometry_test.c`
- Modify: `tests/check_cg20.cmake`
- Modify: `tests/check_cg20_failure.cmake`
- Create: `tests/cg20/geometry/semantics.cg`
- Create: `tests/cg20/geometry/reachable_helper.cg`
- Create: `tests/cg20/geometry/unreachable_helper.cg`
- Create: `tests/cg20/geometry/profile_options.cg`
- Create: `tests/cg20/diagnostics/geometry_semantic_instanceid_type.cg`
- Create: `tests/cg20/diagnostics/geometry_semantic_instanceid_output.cg`
- Create: `tests/cg20/diagnostics/geometry_semantic_vertexid_shape.cg`
- Create: `tests/cg20/diagnostics/geometry_semantic_vertexid_output.cg`
- Create: `tests/cg20/diagnostics/geometry_semantic_primitiveid_type.cg`
- Create: `tests/cg20/diagnostics/geometry_semantic_layer_type.cg`
- Create: `tests/cg20/diagnostics/geometry_stage_vertex.cg`
- Create: `tests/cg20/diagnostics/geometry_stage_fragment.cg`
- Create: `tests/cg20/diagnostics/geometry_entry_call.cg`
- Modify: `tests/CMakeLists.txt`
- Modify: `tests/cg20/conformance.csv`

- [ ] **Step 1: Add failing semantic-classification tests**

First add these public classifications to `cg_geometry.h`:

```c
typedef enum CgGeometrySemanticClass_Rec {
    CG_GEOMETRY_SEMANTIC_ORDINARY = 0,
    CG_GEOMETRY_SEMANTIC_PRIMITIVE_INPUT,
    CG_GEOMETRY_SEMANTIC_VERTEX_INPUT,
    CG_GEOMETRY_SEMANTIC_PRIMITIVE_ID,
    CG_GEOMETRY_SEMANTIC_OUTPUT
} CgGeometrySemanticClass;

#define CG_GEOMETRY_DIRECTION_INPUT  1
#define CG_GEOMETRY_DIRECTION_OUTPUT 2
#define CG_GEOMETRY_DIRECTION_BOTH   3

CgGeometrySemanticClass CgGeometryClassifySemantic(int semantic);
int CgGeometryValidateSemantic(int semantic, Type *type, int direction,
                               CgGeometryDiagnostic *diagnostic);
```

Then add table-driven cases to `tests/cg_geometry_test.c`:

```c
typedef struct SemanticCase_Rec {
    const char *name;
    CgGeometrySemanticClass expected;
    int direction;
    CgScalarKind scalar;
    int isAttribArray;
} SemanticCase;

static const SemanticCase cases[] = {
    { "INSTANCEID", CG_GEOMETRY_SEMANTIC_PRIMITIVE_INPUT,
      CG_GEOMETRY_DIRECTION_INPUT, CG_SCALAR_INT, 0 },
    { "VERTEXID", CG_GEOMETRY_SEMANTIC_VERTEX_INPUT,
      CG_GEOMETRY_DIRECTION_INPUT, CG_SCALAR_INT, 1 },
    { "PRIMITIVEID", CG_GEOMETRY_SEMANTIC_PRIMITIVE_ID,
      CG_GEOMETRY_DIRECTION_BOTH, CG_SCALAR_INT, 0 },
    { "LAYER", CG_GEOMETRY_SEMANTIC_OUTPUT,
      CG_GEOMETRY_DIRECTION_OUTPUT, CG_SCALAR_INT, 0 }
};
```

Assert aliases/case normalization and rejection of wrong direction, scalar
kind, and shape.

- [ ] **Step 2: Implement the selected-program analysis entry point**

Declare:

```c
int CgGeometryAnalyzeProgram(CgGeometryProgram *program,
                             Scope *globalScope,
                             Symbol *entry,
                             const CgReachGraph *reach,
                             const CgProfileOption *profileOptions,
                             CgIRStage profileStage,
                             CgGeometryDiagnostic *diagnostic);
```

Require the caller to initialize `program` with `CgGeometryInitProgram`.
Parse raw options once, merge the entry's stored
source modifiers, determine stage/configuration, build resolved attribute
array views, fold `.length`, classify entry interfaces, and resolve operation
records. Allocation failure sets `program->failed` and returns zero without a
partially usable program.

For option-supplied locations, synthesize `SourceLoc` with file atom
`<command-line>` and line `optionOrdinal + 1`. This gives known
`Vertices=N` metadata and diagnostics a valid stable location without
pretending the option came from shader source.

- [ ] **Step 3: Enforce geometry entry and reachability rules**

Reject calls to any topology-qualified function as an ordinary helper. A
geometry operation reached from the selected entry requires geometry stage.
An unreachable geometry-only helper remains valid under a vertex or fragment
entry. When one helper is reachable from multiple separately compiled stages,
validate it against the current selected stage only.

Use `CgReachWitness` to emit notes from the illegal operation back to the
selected entry, preserving the diagnostic ordering introduced by the base
plan.

```c
if (operation != NULL && program->config.stage != CGIR_STAGE_GEOMETRY) {
    diagnostic->reason = CG_GEOMETRY_DIAGNOSTIC_REACHABLE_STAGE;
    diagnostic->loc = operation->loc;
    return 0;
}
```

- [ ] **Step 4: Implement complete geometry semantic validation**

Require:

- `INSTANCEID`: scalar `int` input.
- `VERTEXID`: `AttribArray<int>` input.
- input `PRIMITIVEID`: scalar `int`.
- output `PRIMITIVEID`: `int` value in emit/flat bundles.
- output `LAYER`: `int` value in emit/flat bundles.
- ordinary per-vertex input semantics: `AttribArray<T>`.
- `INSTANCEID` and `VERTEXID`: never output.
- every attribute-array parameter: selected resolved extent.

Do not require `POSITION` in every emit and do not compare dynamic emit count
to `Vertices=N`.

```c
if (!CgGeometryValidateSemantic(value->canonicalSemantic, value->type,
                                CG_GEOMETRY_DIRECTION_OUTPUT,
                                diagnostic))
    return 0;
```

- [ ] **Step 5: Integrate analysis after entry selection and reachability**

In `CompileProgram`, after `CgReachBuild` and before `CgIRLowerProgram`, create
one `CgGeometryProgram` and call `CgGeometryAnalyzeProgram`. Map structured
option and source diagnostics to the geometry error block. Pass the analyzed
program to lowering in Task 6/7; until then, retain it only for tests and keep
the old lowering call.

Remove `DISABLED TRUE` from the Task 4 positive operation fixtures once this
integration is active.

```c
static void *CgGeometryPoolAlloc(void *arg, size_t size)
{
    return mem_Calloc((MemoryPool *) arg, size, 1);
}

CgGeometryInitProgram(&geometry, CgGeometryPoolAlloc, CurrentScope->pool);
if (!CgGeometryAnalyzeProgram(&geometry, globalScope, entry, &reach,
                              Cg->options.profileOptions, profileStage,
                              &geometryDiagnostic))
    return 0;
```

`-nocode` follows the same analysis path. It must fail for invalid geometry
source and succeed for a valid generic geometry program without publishing
output.

- [ ] **Step 6: Extend the Cg 2.0 harness for selected entries and repeated options**

Keep the base plan's two-argument `add_cg20_success` helper unchanged for
existing tests. Add `add_cg20_geometry_success(name profile source entry
expected profile_options)` and the corresponding failure helper in
`tests/CMakeLists.txt`; each passes all six values explicitly to the fixture
script and appends `name` to the existing global `CG20_TEST_NAMES` property.
Teach both harness scripts to accept `ENTRY`, `EXPECTED`, and
`PROFILE_OPTIONS`; treat `PROFILE_OPTIONS` as a semicolon list and append each
item as a separate pair:

```cmake
set(profile_args)
foreach(option IN LISTS PROFILE_OPTIONS)
    list(APPEND profile_args -po "${option}")
endforeach()
execute_process(
    COMMAND "${CGC}" -quiet -profile "${PROFILE}" ${profile_args}
        -entry "${ENTRY}" -o "${ACTUAL}" "${SOURCE}"
    RESULT_VARIABLE result
    OUTPUT_VARIABLE stdout
    ERROR_VARIABLE stderr)
```

Add tests for source-only, options-only, matching mixed, conflicting mixed,
duplicate equal/unequal `Vertices`, malformed values, and missing input.

- [ ] **Step 7: Add reachability and semantic fixtures**

`semantics.cg` uses every geometry-only semantic in a valid direction.
`reachable_helper.cg` calls operations through two helper levels.
`unreachable_helper.cg` compiles a vertex entry while an unused helper contains
a valid geometry operation. Add one negative fixture for each wrong
direction/type/shape and a reachable stage mismatch.

```c
TRIANGLE void bad_layer(AttribArray<float4> p : POSITION)
{
    emitVertex(p[0] : POSITION, float4(1.0) : LAYER);
}
```

- [ ] **Step 8: Run semantic, nocode, transaction, and full tests**

```powershell
cmake --build build-cg20-geometry --config Release
ctest --test-dir build-cg20-geometry -C Release -R "cg_geometry_unit|cg20_.*geometry_(semantic|stage|profile_option)|cg20_output" --output-on-failure
ctest --test-dir build-cg20-geometry -C Release --output-on-failure
```

Expected: invalid programs preserve sentinel output, valid `-nocode` creates
no shader, unreachable helpers do not poison other stages, and the full suite
passes.

- [ ] **Step 9: Commit selected-program analysis**

```powershell
git add cg_geometry.h cg_geometry.c cg_reach.h cg_reach.c semantic.c compile.c hal.h errors.h tests/cg_geometry_test.c tests/check_cg20.cmake tests/check_cg20_failure.cmake tests/CMakeLists.txt tests/cg20
git commit -m "Validate Cg geometry programs"
```

## Task 6: Extend Cg IR with Geometry Metadata, Types, and Operations

**Files:**

- Modify: `cg_ir.h`
- Modify: `cg_ir.c`
- Modify: `cg_ir_verify.c`
- Modify: `cg_geometry.h`
- Modify: `tests/cg_ir_test.c`
- Modify: `CMakeLists.txt`
- Modify: `tests/CMakeLists.txt`

- [ ] **Step 1: Add failing builder and verifier tests**

Extend `tests/cg_ir_test.c` to construct a geometry module with triangle input,
triangle-strip output, maximum six, one resolved `AttribArray<float4,3>`, an
emit bundle containing `POSITION` and `TEXCOORD0`, one flat `COLOR0`, and one
restart. Assert ownership, order, types, source locations, and operation kinds.

Add one malformed module for every new invariant: unknown stage, missing or
extra geometry metadata, bad topology enum/count, zero known maximum,
attribute array in a non-geometry module, wrong extent, geometry operation in
an unreachable/non-geometry function, empty bundle, duplicate semantic,
invalid special-semantic type/direction, flat position, restart operands, and
missing location. Run `cg_ir_unit`; expect missing enum/builders.

```c
assert(CgIRSetStage(&module, CGIR_STAGE_GEOMETRY));
assert(CgIRSetGeometryInfo(&module, &geometry));
assert(CgIRNewGeometryEmit(&module, values, loc) != NULL);
assert(CgIRVerifyModule(&module, &diagnostic));
```

- [ ] **Step 2: Extend stable Cg IR structures**

Include `cg_geometry.h` from `cg_ir.h`. Add `CgIRStage stage` to `CgIRModule`
and this optional record:

```c
typedef struct CgIRGeometryInfo_Rec {
    CgGeometryInput inputTopology;
    CgGeometryOutput outputTopology;
    unsigned int inputVertexCount;
    unsigned int maxOutputVertices;
    int hasMaxOutputVertices;
    SourceLoc inputLoc;
    SourceLoc outputLoc;
    SourceLoc maxVerticesLoc;
} CgIRGeometryInfo;
```

Add `CgIRGeometryInfo *geometry` to the module. Neutral, vertex, and fragment
modules keep it null.

- [ ] **Step 3: Add explicit statement and bundle nodes**

Append without renumbering prior stable statement kinds:

```c
CGIR_STMT_GEOMETRY_EMIT,
CGIR_STMT_GEOMETRY_FLAT,
CGIR_STMT_GEOMETRY_RESTART
```

Define:

```c
typedef struct CgIRGeometryValue_Rec {
    struct CgIRGeometryValue_Rec *next;
    int canonicalSemantic;
    int sourceSemantic;
    CgIRExpr *value;
    Type *type;
    SourceLoc loc;
} CgIRGeometryValue;
```

The emit/flat statement union member owns a nonempty ordered list; restart has
no operands. Add module-owned builders that deep-copy list links but retain
verified canonical type/semantic/expression references.

- [ ] **Step 4: Add geometry builder APIs**

Declare and implement:

```c
int CgIRSetStage(CgIRModule *module, CgIRStage stage);
int CgIRSetGeometryInfo(CgIRModule *module,
                        const CgIRGeometryInfo *geometry);
CgIRGeometryValue *CgIRNewGeometryValue(CgIRModule *module,
                        int canonicalSemantic, int sourceSemantic,
                        Type *type, CgIRExpr *value, SourceLoc loc);
CgIRStmt *CgIRNewGeometryEmit(CgIRModule *module,
                              CgIRGeometryValue *values, SourceLoc loc);
CgIRStmt *CgIRNewGeometryFlat(CgIRModule *module,
                              CgIRGeometryValue *values, SourceLoc loc);
CgIRStmt *CgIRNewGeometryRestart(CgIRModule *module, SourceLoc loc);
```

Allocation failure follows the base module-failed contract and returns null.

- [ ] **Step 5: Add one stable geometry verifier reason**

Append `CGIR_VERIFY_GEOMETRY` to `CgIRVerifyReason`. Extend the verifier with
all design invariants, including `CGIR_STAGE_UNKNOWN` rejection and exact
topology-to-count mapping. `CGIR_STAGE_NEUTRAL` is valid and contains no
geometry metadata, resolved attribute arrays, or operations.

The verifier does not count control-flow emissions or prove a relationship to
the declared maximum.

```c
if (module->stage == CGIR_STAGE_GEOMETRY && module->geometry == NULL) {
    diagnostic->reason = CGIR_VERIFY_GEOMETRY;
    diagnostic->node = module;
    diagnostic->loc = module->entry->loc;
    return 0;
}
```

- [ ] **Step 6: Run clean/dirty allocator and malformed-module tests**

```powershell
cmake --build build-cg20-geometry --config Release --target cg_ir_unit
ctest --test-dir build-cg20-geometry -C Release -R "cg_ir_(unit|assertions_active)" --output-on-failure
```

Expected: valid geometry passes under clean and dirty allocators, every
malformed module reports `CGIR_VERIFY_GEOMETRY` with the offending node and
location, allocation failure leaves the module failed, and assertions remain
active in the unit binary.

- [ ] **Step 7: Commit the Cg IR geometry model**

```powershell
git add cg_ir.h cg_ir.c cg_ir_verify.c cg_geometry.h tests/cg_ir_test.c CMakeLists.txt tests/CMakeLists.txt
git commit -m "Represent geometry in Cg IR"
```

## Task 7: Lower Geometry to Cg IR and Print Normalized Generic Output

**Files:**

- Modify: `cg_ir_lower.h`
- Modify: `cg_ir_lower.c`
- Modify: `cg_ir_print.c`
- Modify: `compile.c`
- Modify: `generic_hal.c`
- Modify: `tests/cg_ir_test.c`
- Create: `tests/cg20/geometry/pass_through.cg`
- Create: `tests/cg20/geometry/amplify.cg`
- Create: `tests/cg20/geometry/restart.cg`
- Create: `tests/cg20/geometry/flat.cg`
- Create: `tests/cg20/geometry/adjacency.cg`
- Create: `tests/cg20/geometry/layered.cg`
- Create: `tests/cg20/geometry/pass_through.expected`
- Create: `tests/cg20/geometry/amplify.expected`
- Create: `tests/cg20/geometry/restart.expected`
- Create: `tests/cg20/geometry/flat.expected`
- Create: `tests/cg20/geometry/adjacency.expected`
- Create: `tests/cg20/geometry/layered.expected`
- Modify: `tests/CMakeLists.txt`
- Modify: `tests/cg20/conformance.csv`

- [ ] **Step 1: Add a failing normalized-printer unit golden**

Extend the Cg IR unit's literal expected output with this exact geometry
prefix and operations:

```text
module main stage geometry
geometry input triangle vertices 3
geometry output triangle_strip max_vertices 6
```

The statement body must include:

```text
emit_vertex {
  POSITION = %position
  TEXCOORD0 = %uv
}
flat_attribute {
  COLOR0 = %color
}
restart_strip
```

Run `cg_ir_unit`; expect a golden mismatch because the printer does not yet
handle geometry.

- [ ] **Step 2: Pass selected geometry facts into lowering**

Extend `CgIRLowerContext` with `const CgGeometryProgram *geometry` and change
the public entry point to:

```c
int CgIRLowerProgram(CgIRLowerContext *context, Scope *globalScope,
                     Symbol *entry, const CgGeometryProgram *geometry);
```

Update its single compile caller and all unit callers. Set the module stage
from `geometry->config`. For geometry stage, copy one `CgIRGeometryInfo` before
lowering declarations.

- [ ] **Step 3: Lower resolved type views and operations one-for-one**

When a reachable declaration uses an unresolved attribute-array source type,
look it up in `geometry->typeViews` and store the resolved canonical type in
Cg IR. For each frontend statement, query
`CgGeometryFindOperation(geometry, sourceStmt)` before ordinary lowering.

Lower every `CgGeometryValue.value` through the normal typed expression path,
copy its already resolved semantic/type/location, then construct exactly one
emit, flat, or restart Cg IR statement. Do not leave the special intrinsic
call in Cg IR and do not re-infer semantics.

```c
operation = CgGeometryFindOperation(context->geometry, sourceStmt);
if (operation != NULL)
    return CgIRLowerGeometryStatement(context, operation);
```

Declare `CgIRLowerGeometryStatement` as a private static helper in
`cg_ir_lower.c`; it returns the newly built statement or null on module
failure.

- [ ] **Step 4: Verify before every HAL callback**

After lowering, call `CgIRVerifyModule`. On failure, emit the base internal IR
diagnostic and stop before profile validation. Both normal generation and
`-nocode` execute this verification; `-nocode` stops after selected-profile
validation.

```c
static int ReportCgIRFailure(const CgIRVerifyDiagnostic *diagnostic);

if (!CgIRVerifyModule(&module, &verifyDiagnostic))
    return ReportCgIRFailure(&verifyDiagnostic);
```

Implement the static helper in `compile.c` by emitting the base plan's one
internal IR diagnostic at `diagnostic->loc` and returning zero.

- [ ] **Step 5: Print deterministic geometry forms**

Extend `CgIRPrintModule` to print the three header lines only for geometry.
Use canonical input tokens `point`, `line`, `line_adjacency`, `triangle`,
`triangle_adjacency`; output tokens `points`, `line_strip`,
`triangle_strip`; and `unknown` for an absent maximum.

Print `AttribArray<canonical-element,extent>` and operation bundle leaves in
their stored order. Never print source pointers, hash traversal order, or an
unresolved semantic.

```c
fprintf(out, "geometry input %s vertices %u\n",
        CgGeometryInputName(info->inputTopology), info->inputVertexCount);
fprintf(out, "geometry output %s max_vertices ",
        CgGeometryOutputName(info->outputTopology));
```

- [ ] **Step 6: Add compiler-level generic goldens**

Create the six fixtures named above. Cover pass-through, amplification inside
a loop, multiple strip restarts, flat state, adjacency indexing, and layer plus
primitive ID. Register each under `generic` with explicit entry and profile
options. Use an explicit `ENTRY` and literal `PROFILE_OPTIONS` list in every test
registration; no test inherits options from another fixture.

Generate each expectation once through the harness's existing controlled
`UPDATE_EXPECTED` mode, inspect the complete file, then run with updates off.
Every expectation must contain resolved array extents and explicit geometry
nodes.

```cmake
add_cg20_geometry_success(cg20_geometry_pass_through generic
    tests/cg20/geometry/pass_through.cg main
    tests/cg20/geometry/pass_through.expected
    "TRIANGLE;Vertices=3")
```

- [ ] **Step 7: Prove non-geometry normalized output did not change**

Run:

```powershell
cmake --build build-cg20-geometry --config Release
ctest --test-dir build-cg20-geometry -C Release -R "cg_ir_unit|cg20_.*geometry_(pass|amplify|restart|flat|adjacency|layered)|cg20_.*normalized|generic_" --output-on-failure
ctest --test-dir build-cg20-geometry -C Release --output-on-failure
```

Expected: geometry goldens pass, every base non-geometry normalized golden is
byte-identical, and the complete suite passes.

- [ ] **Step 8: Commit Cg IR lowering and generic emission**

```powershell
git add cg_ir_lower.h cg_ir_lower.c cg_ir_print.c compile.c generic_hal.c tests/cg_ir_test.c tests/CMakeLists.txt tests/cg20/geometry tests/cg20/conformance.csv
git commit -m "Lower geometry to normalized Cg IR"
```

## Task 8: Upgrade Shared Vertex and Fragment GLSL Output to Core 1.50

**Files:**

- Modify: `glsl_hal.h`
- Modify: `glsl_hal.c`
- Modify: `glslv_hal.c`
- Modify: `glslf_hal.c`
- Modify: `glsl_ir.h`
- Modify: `glsl_ir.c`
- Modify: `glsl_lower.c`
- Modify: `glsl_codegen.c`
- Modify: `errors.h`
- Modify: `tests/glsl_ir_test.c`
- Modify: `tests/glsl_semantics_test.c`
- Create: `tests/check_glsl150.cmake`
- Modify: `tests/check_glsl.cmake`
- Modify: `tests/validate_glsl.cmake`
- Modify: `tests/check_glsl_interface.cmake`
- Modify: `tests/check_glsl_link.cmake`
- Modify: every tracked `tests/glsl/**/*.expected`
- Modify: `tests/CMakeLists.txt`

- [ ] **Step 1: Add failing core-1.50 contract tests**

Extend `tests/glsl_ir_test.c` to assert stage-correct input/output storage and
these spellings:

```c
assert(strcmp(GlslStorageSpelling(GLSL_STAGE_VERTEX,
              GLSL_STORAGE_INPUT), "in") == 0);
assert(strcmp(GlslStorageSpelling(GLSL_STAGE_VERTEX,
              GLSL_STORAGE_OUTPUT), "out") == 0);
assert(strcmp(GlslBuiltinSpelling(GLSL_BUILTIN_TEX2D), "texture") == 0);
assert(strcmp(GlslBuiltinSpelling(GLSL_BUILTIN_TEX2D_PROJ),
              "textureProj") == 0);
assert(strcmp(GlslBuiltinSpelling(GLSL_BUILTIN_TEX2D_LOD),
              "textureLod") == 0);
```

Declare and implement `GlslStorageSpelling(GlslStage, GlslStorage)` in
`glsl_ir.h`/`glsl_ir.c`; it is the single spelling table used by codegen and
the unit test.

Create `tests/check_glsl150.cmake`. It reads one generated shader and fails
unless it starts with exactly one `#version 150` and contains none of:

```cmake
set(forbidden
    "(^|[^A-Za-z0-9_])attribute([^A-Za-z0-9_]|$)"
    "(^|[^A-Za-z0-9_])varying([^A-Za-z0-9_]|$)"
    "gl_FragColor"
    "texture1D\\("
    "texture2D\\("
    "texture3D\\("
    "textureCube\\("
    "(^|\\n)[ \\t]*#extension")
```

Register it for representative vertex and fragment goldens. Run and expect
failures on `#version 110`, `attribute`, `varying`, and `gl_FragColor`.

- [ ] **Step 2: Make the shared GLSL IR stage-directional**

Replace `GLSL_STORAGE_ATTRIBUTE` and `GLSL_STORAGE_VARYING` with
`GLSL_STORAGE_INPUT` and `GLSL_STORAGE_OUTPUT`. Preserve uniform, sampler,
const, builtin, and private storage. Add:

```c
typedef enum GlslInterpolation_Rec {
    GLSL_INTERPOLATION_DEFAULT = 0,
    GLSL_INTERPOLATION_FLAT,
    GLSL_INTERPOLATION_NOPERSPECTIVE,
    GLSL_INTERPOLATION_SMOOTH
} GlslInterpolation;
```

Store interpolation on `GlslDecl` and `GlslBinding`. Integer interstage
interfaces use `flat`; matching producer and consumer declarations must carry
the same qualifier.

- [ ] **Step 3: Emit exactly core GLSL 1.50 syntax**

Set `VERSION_STRING_GLSL` to `"1.50"`. Make `PrintCodeHeader_glsl` return
success without writing; both the Cg 1.1 adapter and Cg 2.0 IR path end in
`GlslWriteModule`, which owns the only version directive. After the existing
module structural check succeeds, make the writer's first output be:

```c
fprintf(out, "#version 150\n");
```

Task 9 replaces that structural check with `GlslVerifyModule` without moving
version ownership.

Update global declaration generation to print optional interpolation followed
by `in` or `out`. Do not emit a profile suffix or extension line. Keep
`gl_Position`, `gl_PointSize`, `gl_ClipDistance`, `gl_FragCoord`,
`gl_FrontFacing`, and `gl_FragDepth` only where core 1.50 defines them.

- [ ] **Step 4: Replace compatibility fragment color and generic interfaces**

Map fragment `COLOR0` to a deterministic user output such as
`out vec4 cg_COLOR0;` and lower the entry result assignment to that declaration
instead of `gl_FragColor`. Continue rejecting additional color outputs if the
focused profile already rejects them.

Map compatibility-only semantic storage to ordinary deterministic user
interfaces. Preserve true core built-ins, especially vertex `POSITION` output
to `gl_Position` and fragment depth to `gl_FragDepth`.

- [ ] **Step 5: Modernize supported texture intrinsic spellings**

Extend the base catalog-to-GLSL mapping by intrinsic identity:

| Cg intrinsic family | Core 1.50 spelling |
| --- | --- |
| base `tex1D`/`tex2D`/`tex3D`/`texCUBE` | `texture` |
| projected variants | `textureProj` |
| explicit LOD variants | `textureLod` |

Keep stage, sampler, coordinate, and focused-capability checks in profile
validation. Do not infer the target call from a source function name.

- [ ] **Step 6: Update portable vertex/fragment limits to the core contract**

Set shared descriptors to the selected core-1.50 portable minima used by the
focused subset: 16 vertex attributes, 64 vertex output components, 128
fragment input components, 1,024 numeric uniform components per stage, 16
texture units per stage, and the existing focused limit of one fragment color
output. Tests at the old GLSL 1.10
boundaries must be reclassified as success or moved to the new exact/one-past
boundary.

- [ ] **Step 7: Intentionally regenerate every GLSL golden**

Add a cache option used only by the fixture registration:

```cmake
set(CGC_UPDATE_GLSL_EXPECTED OFF CACHE BOOL
    "Regenerate tracked GLSL expectations; inspect before committing")
```

Pass it as `UPDATE_EXPECTED` to `check_glsl.cmake`. Then run:

```powershell
cmake -S . -B build-cg20-geometry -DBUILD_TESTING=ON -DCGC_UPDATE_GLSL_EXPECTED=ON
cmake --build build-cg20-geometry --config Release
ctest --test-dir build-cg20-geometry -C Release -R "^(glslv_|glslf_)" --output-on-failure
git diff -- tests/glsl
cmake -S . -B build-cg20-geometry -DBUILD_TESTING=ON -DCGC_UPDATE_GLSL_EXPECTED=OFF
```

Inspect all changes. Expected semantic changes are `#version 150`, `in`/`out`,
explicit fragment output, modern texture names, and 1.50 limit outcomes. Any
unrelated expression/control-flow change is a defect to fix before proceeding.

- [ ] **Step 8: Validate every vertex/fragment shader and link pairs**

Extend `validate_glsl.cmake` to require `#version 150`, allow stages `vert` and
`frag` for now, invoke `check_glsl150.cmake`, then run the validator. Update
interface/link scripts to extract `in` and `out` declarations rather than
`varying` and to ignore built-ins.

Run:

```powershell
cmake --build build-cg20-geometry --config Release --target glsl_ir_unit glsl_semantics_unit cgc
ctest --test-dir build-cg20-geometry -C Release -R "glsl_ir_unit|glsl_semantics_unit|^(glslv_|glslf_|glsl_link|validate_glsl)" --output-on-failure
ctest --test-dir build-cg20-geometry -C Release --output-on-failure
```

Expected: every generated vertex/fragment shader validates, representative
pairs link, all forbidden-token checks pass, and the complete suite passes.

- [ ] **Step 9: Commit the in-place GLSL 1.50 upgrade**

```powershell
git add glsl_hal.h glsl_hal.c glslv_hal.c glslf_hal.c glsl_ir.h glsl_ir.c glsl_lower.c glsl_codegen.c errors.h tests/glsl_ir_test.c tests/glsl_semantics_test.c tests/check_glsl150.cmake tests/check_glsl.cmake tests/validate_glsl.cmake tests/check_glsl_interface.cmake tests/check_glsl_link.cmake tests/glsl tests/CMakeLists.txt
git commit -m "Upgrade GLSL profiles to core 1.50"
```

## Task 9: Add Geometry Structures and Verification to Shared GLSL IR

**Files:**

- Modify: `glsl_ir.h`
- Modify: `glsl_ir.c`
- Create: `glsl_verify.c`
- Modify: `glsl_codegen.c`
- Modify: `tests/glsl_ir_test.c`
- Modify: `CMakeLists.txt`
- Modify: `tests/CMakeLists.txt`

- [ ] **Step 1: Write failing geometry target-IR tests**

Extend `tests/glsl_ir_test.c` to build a geometry module with triangle-adjacent
input, triangle-strip output, maximum 12, arrayed input, private flat shadow
and flag, assignments attached to one emit, and one restart. Write it to a
temporary stream and compare these required lines:

```glsl
#version 150
layout(triangles_adjacency) in;
layout(triangle_strip, max_vertices = 12) out;
```

```glsl
if (cg_flat_COLOR0_defined) {
    cg_COLOR0 = cg_flat_COLOR0;
}
EmitVertex();
EndPrimitive();
```

Add malformed cases for every verifier reason listed in Step 4. Run the unit
and expect missing structures/builders.

- [ ] **Step 2: Add geometry stage, layout, and replay types**

Append `GLSL_STAGE_GEOMETRY` without renumbering existing stages. Define:

```c
typedef enum GlslGeometryInput_Rec {
    GLSL_GEOMETRY_INPUT_POINTS,
    GLSL_GEOMETRY_INPUT_LINES,
    GLSL_GEOMETRY_INPUT_LINES_ADJACENCY,
    GLSL_GEOMETRY_INPUT_TRIANGLES,
    GLSL_GEOMETRY_INPUT_TRIANGLES_ADJACENCY
} GlslGeometryInput;

typedef enum GlslGeometryOutput_Rec {
    GLSL_GEOMETRY_OUTPUT_POINTS,
    GLSL_GEOMETRY_OUTPUT_LINE_STRIP,
    GLSL_GEOMETRY_OUTPUT_TRIANGLE_STRIP
} GlslGeometryOutput;

typedef struct GlslGeometryInfo_Rec {
    GlslGeometryInput inputTopology;
    GlslGeometryOutput outputTopology;
    int inputVertexCount;
    int maxOutputVertices;
    GlslLoc inputLoc;
    GlslLoc outputLoc;
    GlslLoc maxVerticesLoc;
} GlslGeometryInfo;

typedef struct GlslFlatReplay_Rec {
    struct GlslFlatReplay_Rec *next;
    GlslDecl *target;
    GlslDecl *shadow;
    GlslDecl *defined;
} GlslFlatReplay;
```

Add `GlslGeometryInfo *geometry` to `GlslModule`.
Move the existing `GlslLoc` typedef above `GlslGeometryInfo` in `glsl_ir.h` so
the C90 declaration order is valid; do not duplicate the typedef.

- [ ] **Step 3: Add explicit emit and restart statements**

Append `GLSL_STMT_GEOMETRY_EMIT` and `GLSL_STMT_GEOMETRY_RESTART`.
The emit union member owns an ordered assignment statement list plus ordered
`GlslFlatReplay` list. Restart has no operands. Add module-owned builders:

```c
GlslStmt *GlslNewGeometryEmit(GlslModule *module,
                              GlslStmt *assignments,
                              GlslFlatReplay *replay);
GlslStmt *GlslNewGeometryRestart(GlslModule *module);
GlslFlatReplay *GlslNewFlatReplay(GlslModule *module,
                                  GlslDecl *target,
                                  GlslDecl *shadow,
                                  GlslDecl *defined);
```

- [ ] **Step 4: Implement a standalone GLSL IR verifier**

Declare in `glsl_ir.h`:

```c
typedef enum GlslVerifyReason_Rec {
    GLSL_VERIFY_OK = 0,
    GLSL_VERIFY_STAGE,
    GLSL_VERIFY_TYPE,
    GLSL_VERIFY_DECLARATION,
    GLSL_VERIFY_INTERFACE,
    GLSL_VERIFY_CONTROL,
    GLSL_VERIFY_GEOMETRY,
    GLSL_VERIFY_LOCATION
} GlslVerifyReason;

typedef struct GlslVerifyDiagnostic_Rec {
    GlslVerifyReason reason;
    GlslLoc loc;
    const void *node;
} GlslVerifyDiagnostic;

int GlslVerifyModule(const GlslModule *module,
                     GlslVerifyDiagnostic *diagnostic);
```

In `glsl_verify.c`, retain all base codegen validation and add: metadata
exactly for geometry; valid layouts/count/positive maximum; geometry inputs
are arrays of the exact topology length except scalar primitive built-ins;
emit/restart only in geometry; emit assignment list well formed; each replay
triple has matching type, private shadow/flag ownership, Boolean flag, and
output target; and no geometry node in vertex/fragment.

- [ ] **Step 5: Generate layouts and explicit side effects**

After the version header and before declarations, generate exactly one input
and one output layout line using fixed enum-to-token switches. For emit, write
its output assignments, then for each replay write the guarded assignment,
then `EmitVertex();`. For restart, write `EndPrimitive();`.

Call `GlslVerifyModule` before `GlslWriteModule` writes `#version 150`, so a
broken target module writes no bytes at all. Remove duplicate private
structural checks from codegen once verifier coverage exists.

- [ ] **Step 6: Run target-IR unit and assertion tests**

```powershell
cmake --build build-cg20-geometry --config Release --target glsl_ir_unit
ctest --test-dir build-cg20-geometry -C Release -R "glsl_ir_(unit|assertions_active)" --output-on-failure
```

Expected: the valid module prints the exact layouts and operations, every
malformed module returns a stable reason/node/location, and assertion seams
remain active.

- [ ] **Step 7: Commit shared GLSL geometry IR**

```powershell
git add glsl_ir.h glsl_ir.c glsl_verify.c glsl_codegen.c tests/glsl_ir_test.c CMakeLists.txt tests/CMakeLists.txt
git commit -m "Add geometry to shared GLSL IR"
```

## Task 10: Lower Cg Geometry Interfaces and Operations to GLSL IR

**Files:**

- Create: `glslg_hal.c`
- Modify: `glsl_hal.h`
- Modify: `glsl_hal.c`
- Modify: `glslv_hal.c`
- Modify: `glslf_hal.c`
- Modify: `glsl_lower.c`
- Modify: `glsl_ir.h`
- Modify: `glsl_ir.c`
- Modify: `errors.h`
- Modify: `tests/glsl_ir_test.c`
- Modify: `tests/glsl_semantics_test.c`
- Create: `tests/glsl/geometry/topologies.cg`
- Create: `tests/glsl/geometry/topology_points.expected`
- Create: `tests/glsl/geometry/topology_lines.expected`
- Create: `tests/glsl/geometry/topology_lines_adjacency.expected`
- Create: `tests/glsl/geometry/topology_triangles.expected`
- Create: `tests/glsl/geometry/topology_triangles_adjacency.expected`
- Create: `tests/glsl/geometry/output_points.expected`
- Create: `tests/glsl/geometry/output_line_strip.expected`
- Create: `tests/glsl/geometry/output_triangle_strip.expected`
- Create: `tests/glsl/geometry/semantics.cg`
- Create: `tests/glsl/geometry/semantics.expected`
- Create: `tests/glsl/geometry/operations.cg`
- Create: `tests/glsl/geometry/operations.expected`
- Create: `tests/glsl/geometry/helpers.cg`
- Create: `tests/glsl/geometry/helpers.expected`
- Create: `tests/glsl/geometry/control_flow.cg`
- Create: `tests/glsl/geometry/control_flow.expected`
- Create: `tests/glsl/geometry/uniform_texture.cg`
- Create: `tests/glsl/geometry/uniform_texture.expected`
- Modify: `CMakeLists.txt`
- Modify: `tests/CMakeLists.txt`

- [ ] **Step 1: Add failing Cg-IR-to-GLSL-IR lowering tests**

In `tests/glsl_ir_test.c`, build verified Cg IR for one triangle pass-through
program and call `GlslLowerCgIR` with `GlslGeometryProfileDesc()`. Assert
layouts, array extent, built-in declarations, explicit emit/restart nodes,
and verification success.

In `tests/glsl_semantics_test.c`, table-test this exact map:

```c
{ "INSTANCEID", 1, GLSL_INTERFACE_PRIMITIVE_ID_IN },
{ "PRIMITIVEID", 1, GLSL_INTERFACE_PRIMITIVE_ID_IN },
{ "VERTEXID", 1, GLSL_INTERFACE_USER },
{ "POSITION", 1, GLSL_INTERFACE_GEOMETRY_POSITION_IN },
{ "POSITION", 0, GLSL_INTERFACE_POSITION },
{ "PRIMITIVEID", 0, GLSL_INTERFACE_PRIMITIVE_ID_OUT },
{ "LAYER", 0, GLSL_INTERFACE_LAYER }
```

Run both units and expect the geometry descriptor and mappings to be absent.

- [ ] **Step 2: Define the `glslg` descriptor**

In `glsl_hal.h`, append these values to `GlslInterface` so existing enum values
retain their numbers:

```c
GLSL_INTERFACE_USER,
GLSL_INTERFACE_GEOMETRY_POSITION_IN,
GLSL_INTERFACE_PRIMITIVE_ID_IN,
GLSL_INTERFACE_PRIMITIVE_ID_OUT,
GLSL_INTERFACE_LAYER
```

Then add profile/connector constants without registering them yet:

```c
#define PROFILE_GLSLG_NAME "glslg"
#define PROFILE_GLSLG_ID 14
#define CID_GLSLG_IN_ID 18
#define CID_GLSLG_OUT_ID 19

const GlslProfileDesc *GlslGeometryProfileDesc(void);
int InitHAL_glslg(slHAL *hal);
```

Create `glslg_hal.c` with connector and semantic tables for all focused Cg
geometry inputs/outputs: conventional/`ATTRn` vertex semantics,
`INSTANCEID`, `VERTEXID`, input/output `PRIMITIVEID`, `POSITION`, `PSIZE`,
`FOG`, `COLOR0/1`, `BCOL0/1`, `TEXCOORD0-7`, `CLP0-5`, and `LAYER`.
Return its immutable descriptor for units. Registration happens only after the
lowering and writer tests pass in Step 8.

- [ ] **Step 3: Lower geometry layout and interfaces**

Map Cg topology metadata one-for-one to `GlslGeometryInfo`. Require the
already profile-validated known maximum; record an error instead of inventing
one.

Flatten aggregate inputs/outputs recursively by verified semantic leaf using
the same deterministic naming function as vertex/fragment. Geometry
per-vertex user inputs become arrays of the exact topology length. Scalar
primitive inputs remain scalar. Matching semantic leaf type, interpolation,
and generated name must be identical in producer/consumer stages.

- [ ] **Step 4: Implement built-in geometry semantics and the vertex-ID bridge**

Lower:

- input `POSITION` to `gl_in[index].gl_Position`;
- output `POSITION` to `gl_Position`;
- input `INSTANCEID` and scalar input `PRIMITIVEID` to
  `gl_PrimitiveIDIn`;
- output `PRIMITIVEID` to `gl_PrimitiveID`;
- output `LAYER` to `gl_Layer`;
- geometry input `VERTEXID` to a deterministic flat integer user-varying
  array.

Extend `glslf` so fragment input `PRIMITIVEID` maps to its core
`gl_PrimitiveID` input. It has no generated user declaration and links to the
geometry built-in output according to the core pipeline rule.

For `glslv`, map a vertex input `VERTEXID` to `gl_VertexID`. If the vertex
entry outputs `VERTEXID`, assign it to the matching flat integer output so a
separately compiled geometry entry links. Never reference `gl_VertexID` from
geometry code.

- [ ] **Step 5: Lower emit and restart explicitly**

For every Cg IR emit, lower its resolved bundle into output assignments in
stored order and create one `GlslNewGeometryEmit`. Lower restart to one
`GlslNewGeometryRestart`. These operations remain legal inside reachable
helpers and preserve their original structured control-flow location.

- [ ] **Step 6: Implement path-correct flat shadow state**

For every semantic ever written by `flatAttrib`, create two unique private
module globals accessible from helpers:

```glsl
vec4 cg_flat_COLOR0;
bool cg_flat_COLOR0_defined = false;
```

A flat operation assigns the shadow then sets its flag true. Attach one replay
triple to every emit for each flat semantic in the program. Codegen's guarded
replay makes conditional/loop behavior exact: paths that have not executed
`flatAttrib` do not overwrite the ordinary emit value, while paths that have
executed it use the most recent shadow value. Preserve shadow state across
`EndPrimitive()` until the source updates it.

- [ ] **Step 7: Lower the focused uniform/helper/control/texture subset**

Reuse the shared Cg-IR lowering for uniforms, reachable helpers, structured
control flow, and supported texture intrinsics. Geometry texture calls use the
core 1.50 `texture`, `textureProj`, or `textureLod` spelling selected by
intrinsic identity. Unsupported sampler or intrinsic families retain profile
diagnostics.

- [ ] **Step 8: Register `glslg` and add geometry compiler goldens**

Add `glslg_hal.c` to `CMakeLists.txt` and register it in
`RegisterProfiles_glsl`:

```c
RegisterProfile(InitHAL_glslg, PROFILE_GLSLG_NAME, PROFILE_GLSLG_ID);
```

Register compiler tests with `-profile glslg -po Vertices=N`. Cover every topology,
semantic map, pass-through/amplification/restart, repeated and conditional
flat state, helper-contained operations, control flow, nested aggregate
inputs, uniforms, and supported texture access.
Include `restartStrip` under point output and require `EndPrimitive()` rather
than optimizing it away; neutral legality and target spelling stay uniform
across all three output topologies.

Every golden begins `#version 150`, declares exact layouts, contains no
extension/compatibility tokens, and passes `glslangValidator -S geom`.

- [ ] **Step 9: Run focused lowering and full regressions**

```powershell
cmake --build build-cg20-geometry --config Release
ctest --test-dir build-cg20-geometry -C Release -R "glsl_ir_unit|glsl_semantics_unit|glsl_geometry_lower|validate_.*geometry" --output-on-failure
ctest --test-dir build-cg20-geometry -C Release --output-on-failure
```

Expected: all geometry outputs validate, vertex/fragment goldens stay
unchanged from Task 8, and the complete suite passes.

- [ ] **Step 10: Commit geometry lowering**

```powershell
git add glslg_hal.c glsl_hal.h glsl_hal.c glslv_hal.c glslf_hal.c glsl_lower.c glsl_ir.h glsl_ir.c errors.h tests/glsl_ir_test.c tests/glsl_semantics_test.c tests/glsl/geometry CMakeLists.txt tests/CMakeLists.txt
git commit -m "Lower Cg geometry to GLSL IR"
```

## Task 11: Enforce `glslg` Stage, Maximum, Capability, and Resource Rules

**Files:**

- Modify: `glsl_hal.h`
- Modify: `glsl_hal.c`
- Modify: `glslg_hal.c`
- Modify: `glsl_lower.c`
- Modify: `glsl_ir.h`
- Modify: `glsl_ir.c`
- Modify: `errors.h`
- Modify: `tests/glsl_ir_test.c`
- Modify: `tests/check_glsl_failure.cmake`
- Modify: `tests/check_output_transaction.cmake`
- Create: `tests/glsl/geometry/limits/input_components_64.cg`
- Create: `tests/glsl/geometry/limits/input_components_65.cg`
- Create: `tests/glsl/geometry/limits/output_components_128.cg`
- Create: `tests/glsl/geometry/limits/output_components_129.cg`
- Create: `tests/glsl/geometry/limits/total_components_1024.cg`
- Create: `tests/glsl/geometry/limits/total_components_1025.cg`
- Create: `tests/glsl/geometry/limits/uniform_components_1024.cg`
- Create: `tests/glsl/geometry/limits/uniform_components_1025.cg`
- Create: `tests/glsl/geometry/limits/texture_units_16.cg`
- Create: `tests/glsl/geometry/limits/texture_units_17.cg`
- Create: `tests/glsl/geometry/limits/output_vertices.cg`
- Create: `tests/glsl/geometry/diagnostics/missing_vertices.cg`
- Create: `tests/glsl/geometry/diagnostics/wrong_stage_vertex.cg`
- Create: `tests/glsl/geometry/diagnostics/wrong_stage_fragment.cg`
- Create: `tests/glsl/geometry/diagnostics/unsupported_type.cg`
- Create: `tests/glsl/geometry/diagnostics/unsupported_intrinsic.cg`
- Create: `tests/glsl/geometry/diagnostics/unsupported_sampler.cg`
- Modify: `tests/CMakeLists.txt`
- Modify: `tests/cg20/conformance.csv`

- [ ] **Step 1: Add failing stage and maximum tests**

Register these exact cases:

- geometry source under `glslg` without `Vertices=N`: maximum-required
  diagnostic and preserved sentinel output;
- geometry source under `glslg -po Vertices=3`: success;
- vertex source under `glslg`: stage mismatch;
- geometry source under `glslv` and `glslf`: stage mismatch;
- vertex/fragment source under the wrong existing GLSL profile: stage
  mismatch;
- generic geometry source without `Vertices=N`: success with unknown maximum
  metadata.

Run them and expect at least the missing-maximum and stage cases to fail.

- [ ] **Step 2: Validate profile capability before target lowering**

Factor one shared entry point in `glsl_hal.c`:

```c
typedef struct GlslProfileDiagnostic_Rec {
    GlslErrorKind kind;
    SourceLoc loc;
    const char *reason;
    const char *resourceName;
    int resourceUsed;
    int resourceAvailable;
} GlslProfileDiagnostic;

int GlslValidateCgIR(const GlslProfileDesc *profile,
                     const CgIRModule *source,
                     GlslProfileDiagnostic *diagnostic);
```

Require exact stage equality for `glslv`, `glslg`, and `glslf`. For geometry,
require nonnull metadata and a positive known maximum before allocating
`GlslModule`. Keep unsupported Cg types, samplers, intrinsics, interfaces, and
operations in the existing structured GLSL diagnostic family.

`ValidateIR_glsl` runs this preflight and a scratch lower/verify pass;
`GenerateIR_glsl` repeats it in the compilation pool and writes only after
`GlslVerifyModule` succeeds. Neither callback caches pointers between calls.

- [ ] **Step 3: Extend explicit portable limit descriptors**

Replace ambiguous shared fields with stage-relevant component limits:

```c
typedef struct GlslLimits_Rec {
    int attributes;
    int inputComponents;
    int outputComponents;
    int maxOutputVertices;
    int totalOutputComponents;
    int uniformComponents;
    int textureUnits;
    int colorOutputs;
} GlslLimits;
```

Set `glslg` to input 64, output 128, output vertices 256, total output 1024,
uniform 1024, and texture units 16. Use zero for fields irrelevant to a stage
and test the field's applicability explicitly; never interpret zero as an
unlimited resource.

- [ ] **Step 4: Count the exact lowered resource interface once**

After interface flattening but before code generation, count:

- geometry input components per input vertex; do not multiply by topology
  length;
- output components per emitted vertex, including represented built-ins;
- declared maximum output vertices;
- `outputComponents * maxOutputVertices` total output components;
- numeric uniform scalar components, recursively through arrays/structs;
- distinct sampler declarations/units.

Scalars count one, vectors their length, matrices rows times columns, arrays
element count times element components, and structs the recursive sum. Count
each canonical interface declaration once even if referenced by multiple
operations. Use checked integer multiplication for the total. Report the
source declaration that crosses a component/unit boundary and the profile
option location for an output-vertex boundary.

- [ ] **Step 5: Add exact boundary fixtures**

Build compact boundaries from valid types:

- input 64: four `AttribArray<float4x4>` leaves; input 65 adds
  `AttribArray<int> : VERTEXID`;
- output 128: eight `float4x4` semantic leaves; output 129 adds `LAYER`;
- output vertices: the same small point-output entry with `Vertices=256`
  succeeds and `Vertices=257` fails;
- total 1024: `POSITION` only with `Vertices=256`; total 1025:
  `POSITION` plus `LAYER` with `Vertices=205`;
- uniforms 1024: 256 `float4` array elements; 1025: 255 `float4` elements
  plus `float3` and `float2`;
- texture units: sixteen and seventeen distinct used sampler parameters.

Use `ATTR0`-`ATTR3` for the four matrix input leaves and `TEXCOORD0`-
`TEXCOORD7` for the eight matrix output leaves. Compile component-boundary
fixtures with `Vertices=1` so only the intended per-vertex limit is under test.

- [ ] **Step 6: Verify transactional and `-nocode` behavior**

Extend failure scripts to pass `PROFILE_OPTIONS`. For missing maximum,
one-past limits, and stage mismatch, seed the destination with `sentinel`, run
normal generation and `-nocode`, and assert:

- nonzero exit;
- exact diagnostic code and source/option location;
- destination still equals `sentinel` for normal generation;
- no `.cgc-tmp-*` file remains;
- `-nocode` publishes nothing.

- [ ] **Step 7: Run all GLSL limits and regressions**

```powershell
cmake --build build-cg20-geometry --config Release
ctest --test-dir build-cg20-geometry -C Release -R "glsl.*(stage|maximum|limit)|glslg_|cg20_output" --output-on-failure
ctest --test-dir build-cg20-geometry -C Release --output-on-failure
```

Expected: every exact boundary succeeds, every one-past case reports the used
and available counts, no failure publishes partial source, and the full suite
passes.

- [ ] **Step 8: Commit profile validation and limits**

```powershell
git add glsl_hal.h glsl_hal.c glslg_hal.c glsl_lower.c glsl_ir.h glsl_ir.c errors.h tests/glsl_ir_test.c tests/check_glsl_failure.cmake tests/check_output_transaction.cmake tests/glsl/geometry tests/CMakeLists.txt tests/cg20/conformance.csv
git commit -m "Validate GLSL geometry profile limits"
```

## Task 12: Validate and Link Complete GLSL 1.50 Pipelines

**Files:**

- Create: `tests/check_glsl_pipeline.cmake`
- Modify: `tests/validate_glsl.cmake`
- Modify: `tests/check_glsl150.cmake`
- Modify: `tests/check_glsl_interface.cmake`
- Modify: `tests/check_glsl_link.cmake`
- Create: `tests/glsl/link/vp_geometry_link.cg`
- Create: `tests/glsl/link/gp_geometry_link.cg`
- Create: `tests/glsl/link/fp_geometry_link.cg`
- Create: `tests/glsl/link/vp_vertex_id.cg`
- Create: `tests/glsl/link/gp_vertex_id.cg`
- Create: `tests/glsl/link/fp_vertex_id.cg`
- Create: `tests/glsl/link/gp_type_mismatch.cg`
- Create: `tests/glsl/link/fp_type_mismatch.cg`
- Modify: `arb_hal.h`
- Modify: `arb_hal.c`
- Modify: `arbvp1_hal.c`
- Modify: `arbfp1_hal.c`
- Modify: `tests/cg20/conformance.csv`
- Modify: `tests/check_cg20_manifest.cmake`
- Modify: `tests/CMakeLists.txt`
- Modify: `README.md`

- [ ] **Step 1: Register failing geometry validation and pipeline tests**

In `tests/CMakeLists.txt`, register the existing
`tests/glsl/geometry/topologies.cg` fixture through `validate_glsl.cmake` with
`PROFILE=glslg`, `STAGE=geom`, and `PROFILE_OPTIONS` equal to
`TRIANGLE;Vertices=3`. Register `glsl_pipeline_geometry` with the three exact
`vp_geometry_link.cg`, `gp_geometry_link.cg`, and `fp_geometry_link.cg` paths
and `tests/check_glsl_pipeline.cmake` as its runner.

Run:

```powershell
cmake --build build-cg20-geometry --config Release
ctest --test-dir build-cg20-geometry -C Release -R "validate_.*geometry|glsl_pipeline_geometry" --output-on-failure
```

Expected: geometry validation fails because the current validator harness does
not recognize the geometry stage, and the pipeline test fails because its
runner has not been created.

- [ ] **Step 2: Make validation accept all three exact stages and add the pipeline runner**

Extend `validate_glsl.cmake` with the exact profile-stage pairs:

```cmake
if(PROFILE STREQUAL "glslv")
    set(expected_stage vert)
elseif(PROFILE STREQUAL "glslg")
    set(expected_stage geom)
elseif(PROFILE STREQUAL "glslf")
    set(expected_stage frag)
else()
    message(FATAL_ERROR "unsupported GLSL profile ${PROFILE}")
endif()
if(NOT "${STAGE}" STREQUAL "${expected_stage}")
    message(FATAL_ERROR "${PROFILE} must be validated as ${expected_stage}")
endif()
```

Accept `PROFILE_OPTIONS` and pass every item as `-po`. Run
`check_glsl150.cmake` before `glslangValidator -S`. Register every successful
tracked vertex, geometry, and fragment fixture, not merely representative
ones.

Create `tests/check_glsl_pipeline.cmake`. Require compiler, validator, three
sources, three outputs ending in `.vert`, `.geom`, and `.frag`, and geometry
options. Define `run_cgc` to expand a
semicolon option list into repeated `-po` pairs, require zero exit and silent
stdout/stderr, then call it:

```cmake
run_cgc(glslv "${VERTEX_SOURCE}" "${VERTEX_OUTPUT}" "")
run_cgc(glslg "${GEOMETRY_SOURCE}" "${GEOMETRY_OUTPUT}"
        "TRIANGLE;Vertices=3")
run_cgc(glslf "${FRAGMENT_SOURCE}" "${FRAGMENT_OUTPUT}" "")
```

After individual source-contract and stage validation, link:

```cmake
execute_process(
    COMMAND "${VALIDATOR}" -l "${VERTEX_OUTPUT}" "${GEOMETRY_OUTPUT}"
        "${FRAGMENT_OUTPUT}"
    RESULT_VARIABLE link_result
    OUTPUT_VARIABLE link_stdout
    ERROR_VARIABLE link_stderr)
if(NOT link_result EQUAL 0)
    message(FATAL_ERROR "GLSL pipeline link failed:\n${link_stdout}${link_stderr}")
endif()
```

Add `EXPECT_LINK_FAILURE` mode that requires nonzero link result and a supplied
stable mismatch fragment.

- [ ] **Step 3: Add the representative semantic pipeline**

Create a vertex program that outputs `POSITION`, `COLOR0`, and `TEXCOORD0`.
Create `gp_geometry_link.cg`:

```c
TRIANGLE TRIANGLE_OUT void main(
    AttribArray<float4> position : POSITION,
    AttribArray<float4> color : COLOR0,
    AttribArray<float2> texcoord : TEXCOORD0,
    int primitive : PRIMITIVEID)
{
    int i;
    for (i = 0; i < position.length; ++i) {
        emitVertex(position[i] : POSITION,
                   color[i] : COLOR0,
                   texcoord[i] : TEXCOORD0,
                   primitive : PRIMITIVEID);
    }
}
```

The fragment program consumes matching `COLOR0`, `TEXCOORD0`, and
`PRIMITIVEID`. Register independent validation plus full link.

- [ ] **Step 4: Add the `VERTEXID` bridge pipeline**

Create a vertex entry that reads `int vertexId : VERTEXID`, exports it under
`VERTEXID`, and exports position/color. Geometry consumes
`AttribArray<int> : VERTEXID` and uses it in an emitted color; fragment
consumes that color. Assert generated vertex source reads `gl_VertexID`, both
interstage declarations use the same `flat` integer name, and geometry source
does not mention `gl_VertexID`.

- [ ] **Step 5: Add negative cross-stage link fixtures**

Create one geometry output/fragment input type mismatch for the same semantic
and one interpolation mismatch test seam. Each shader validates separately;
the three-stage link must fail with the expected interface-mismatch fragment.
This proves the positive link is meaningful rather than only a syntax check.

- [ ] **Step 6: Add controlled ARB geometry rejection**

Extend ARB IR/profile validation to inspect `source->stage` after successful
Cg IR verification. If geometry, emit one controlled unsupported-stage
profile diagnostic at the entry/modifier location and return failure before
legacy lowering or output. Register the same valid generic geometry fixture
under `arbvp1` and `arbfp1`; both fail transactionally while its `generic` and
`glslg` cases succeed.

- [ ] **Step 7: Close the geometry conformance manifest**

Add separate rows for every topology modifier/default, `Vertices` parse/merge
rule, attribute-array placement/type/length rule, operation arity/context,
semantic source/duplicate/direction/type rule, reachability rule, Cg IR
invariant, GLSL mapping, resource boundary, stage mismatch, legacy-language
rejection, and ARB rejection. Each non-out-of-scope row names one registered
CTest. Run the manifest checker and reject duplicate requirement IDs or
missing test names.

- [ ] **Step 8: Update profile documentation**

Document in `README.md`:

- `glslv`, `glslg`, and `glslf` always emit core `#version 150`;
- there is no GLSL version selector or retained 1.10 mode;
- `glslg` requires `-po Vertices=N` plus source or option input topology;
- topology and output option spellings;
- focused geometry semantics and vertex-ID bridge;
- portable resource limits and profile diagnostics;
- `generic` permits unknown maximum metadata;
- example three-stage commands and validator invocation.

- [ ] **Step 9: Run generated-source and complete Debug/Release qualification**

```powershell
cmake --build build-cg20-geometry --config Release --target regenerate_parser regenerate_stdlib
git diff --check
ctest --test-dir build-cg20-geometry -C Release -R "parser_regeneration|stdlib_regeneration|cg20_manifest|validate_|glsl_pipeline|arb.*geometry" --output-on-failure

cmake -S . -B build-cg20-geometry-release -DBUILD_TESTING=ON -DCMAKE_BUILD_TYPE=Release
cmake --build build-cg20-geometry-release --config Release
ctest --test-dir build-cg20-geometry-release -C Release --output-on-failure

cmake -S . -B build-cg20-geometry-debug -DBUILD_TESTING=ON -DCMAKE_BUILD_TYPE=Debug
cmake --build build-cg20-geometry-debug --config Debug
ctest --test-dir build-cg20-geometry-debug -C Debug --output-on-failure
```

Expected: regeneration leaves no unintended diff, manifest passes, every
generated shader validates, positive pipelines link, negative pipelines fail
as intended, and both complete suites report zero failures.

- [ ] **Step 10: Manually qualify bundled shaders and one geometry pipeline**

```powershell
$cgc = (Resolve-Path '.\build-cg20-geometry-release\Release\cgc.exe').Path
foreach ($shader in 'position.cg','reflection.cg','vertexlight.cg','vertexlight4.cg') {
    $name = [IO.Path]::GetFileNameWithoutExtension($shader)
    & $cgc -quiet -profile glslv -o "build-cg20-geometry-release\$name.vert" $shader
    if ($LASTEXITCODE -ne 0) { throw "glslv failed: $shader" }
    & glslangValidator -S vert "build-cg20-geometry-release\$name.vert"
    if ($LASTEXITCODE -ne 0) { throw "validation failed: $shader" }
}

& $cgc -quiet -profile glslg -po TRIANGLE -po Vertices=3 `
    -o build-cg20-geometry-release\pipeline.geom `
    tests\glsl\link\gp_geometry_link.cg
if ($LASTEXITCODE -ne 0) { throw 'glslg failed' }
& glslangValidator -S geom build-cg20-geometry-release\pipeline.geom
if ($LASTEXITCODE -ne 0) { throw 'geometry validation failed' }
```

Expected: all five outputs begin with exactly `#version 150`, contain readable
`main` functions, and validate without extensions.

- [ ] **Step 11: Commit conformance, validation, and documentation**

```powershell
git add tests/check_glsl_pipeline.cmake tests/validate_glsl.cmake tests/check_glsl150.cmake tests/check_glsl_interface.cmake tests/check_glsl_link.cmake tests/glsl/link arb_hal.h arb_hal.c arbvp1_hal.c arbfp1_hal.c tests/cg20/conformance.csv tests/check_cg20_manifest.cmake tests/CMakeLists.txt README.md
git commit -m "Qualify Cg geometry GLSL pipelines"
```

## Final Verification Checklist

- [ ] `git diff --check` reports no whitespace errors.
- [ ] No incomplete-work markers remain in the geometry implementation or its
  fixtures.
- [ ] Every geometry manifest row names a registered passing test.
- [ ] All five input topologies and three output topologies reach verified Cg
  IR and valid GLSL 1.50.
- [ ] Every resolved attribute-array extent is exactly `1`, `2`, `3`, `4`, or
  `6` as dictated by topology.
- [ ] Every `emitVertex`, `flatAttrib`, and `restartStrip` becomes one explicit
  verified Cg IR operation; emit/restart become explicit verified GLSL IR
  operations.
- [ ] Conditional and repeated flat-state tests prove defined-flag replay and
  post-`EmitVertex` restoration.
- [ ] `glslv`, `glslg`, and `glslf` output starts with exactly one
  `#version 150` and contains no forbidden compatibility construct or
  unnecessary extension.
- [ ] Every tracked generated shader passes `glslangValidator` with its exact
  stage.
- [ ] Representative vertex-geometry-fragment pipelines link; negative
  interface pipelines fail linking.
- [ ] All six geometry resource families pass at their portable boundary and
  fail one past it with exact used/available diagnostics.
- [ ] `glslg` rejects missing `Vertices=N`; `generic` preserves unknown maximum
  metadata.
- [ ] ARB profiles reject verified geometry modules transactionally.
- [ ] Explicit Cg 1.1 rejects every new source form while its other fixtures
  remain unchanged.
- [ ] Existing non-geometry normalized generic goldens remain byte-identical.
- [ ] Parser and standard-library regeneration leave no diff.
- [ ] Complete Release and Debug CTest suites report zero failed tests.
- [ ] `git status --short` contains no build artifacts or unintended files.

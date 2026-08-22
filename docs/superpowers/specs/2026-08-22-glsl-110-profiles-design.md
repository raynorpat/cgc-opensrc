# GLSL 1.10 Profiles Design

## Objective

Add `glslv` and `glslf` profiles that translate the Cg subset accepted by this
compiler front end into deterministic, readable, strict desktop GLSL 1.10.
The vertex and fragment outputs must begin with `#version 110`, require no
extensions, and validate with `glslangValidator` when that tool is available.

The implementation must leave the existing `generic` profile unchanged. It
must also avoid the profile and connector IDs reserved by the approved ARB
profile design.

## Standards Baseline

The language contract is the Khronos OpenGL Shading Language 1.10
specification:

- <https://registry.khronos.org/OpenGL/specs/gl/GLSLangSpec.1.10.pdf>

Portable resource checks use the minimum OpenGL 2.0 limits:

- <https://registry.khronos.org/OpenGL/specs/gl/glspec20.pdf>

The profiles emit only base GLSL 1.10. They do not emit `#extension`
directives or silently select newer language behavior.

## Scope

The first release supports a practical Cg subset:

- vertex and fragment entry points;
- scalar, vector, and square-matrix numeric types accepted by this front end;
- fixed-size arrays and structs;
- global and entry-parameter uniforms;
- named vertex attributes and semantic-linked varyings;
- readable helper functions and overloads;
- constructors, casts, swizzles, indexing, and matrix selectors;
- scalar, vector, and square-matrix expressions;
- structured conditionals and loops;
- common numeric, geometric, and texture intrinsics;
- the listed non-shadow base GLSL sampler types and fragment texture access;
- source-located diagnostics for unsupported constructs.

The first release does not support:

- GLSL extensions or language versions newer than 1.10;
- non-square matrices;
- multiple fragment color outputs;
- rectangle, buffer, multisample, or shadow samplers;
- vertex texture access, because the OpenGL 2.0 portable minimum is zero
  vertex texture units;
- recursion;
- unsupported packed arrays;
- runtime rendering comparisons or driver-specific workarounds; or
- preservation of source comments, which are discarded before HAL code
  generation.

## Architecture

The profiles use a shared structured source backend with isolated stage
descriptors.

### Profile and HAL Modules

- `glsl_hal.h` declares shared descriptors, profile IDs, connector IDs,
  limits, intrinsic IDs, and `RegisterProfiles_glsl`.
- `glsl_hal.c` implements registration, common HAL callbacks, type policy,
  binding helpers, intrinsic recognition, and shared initialization.
- `glslv_hal.c` owns vertex connector registers, semantic aliases, stage
  built-ins, limits, and vertex-profile initialization.
- `glslf_hal.c` owns fragment connector registers, semantic aliases, sampler
  policy, limits, and fragment-profile initialization.

Use profile IDs 12 and 13 for `glslv` and `glslf`. Use connector IDs 14
through 17 for their input and output connectors. These values avoid generic
profile ID 5, generic connector IDs 8 and 9, and IDs 10 through 13 reserved by
the ARB profile design.

The common capability callback reports:

- structured `if` statements are retained;
- matrices are retained;
- indexed arrays are supported;
- helper functions are not forcibly inlined;
- ordinary helper-function returns are allowed; and
- bindings are late so emitted declarations and binding metadata use the
  backend's final sanitized names.

### Structured GLSL Representation

- `glsl_ir.h` declares `GlslModule`, declarations, types, functions,
  statements, expressions, interface bindings, and source-location fields.
- `glsl_ir.c` implements node construction, lists, and type comparison. Nodes
  use the compiler's existing arena lifetime, so lowering has one allocation
  policy and requires no independent per-node cleanup path.
- `glsl_lower.c` translates the checked and normalized compiler AST into the
  structured GLSL module.
- `glsl_codegen.c` validates final ordering assumptions and emits formatted
  GLSL without consulting or mutating the compiler AST.

The GLSL representation is structured source IR, not a register or assembly
IR. It retains declarations, functions, statements, expressions, and types so
the emitted shader remains recognizable. Nodes may retain immutable pointers
to compiler symbols and types for identity and diagnostics, but output
decisions are stored in GLSL-owned nodes.

### Compiler Flow

The existing compiler continues to perform parsing, semantic analysis,
binding construction, standard normalization, and constant folding. The GLSL
profiles request preservation of structured conditionals and matrices.

`GenerateCode_glsl` performs these steps:

1. Collect reachable user structs, globals, helper functions, and the selected
   entry point.
2. Assign deterministic GLSL names.
3. Build attributes, varyings, uniforms, outputs, and binding metadata from
   HAL bindings.
4. Lower typed statements and expressions into `GlslModule`.
5. Validate stage rules, GLSL 1.10 restrictions, and portable resources.
6. Emit metadata, declarations, structs, helpers, and `void main()` in stable
   dependency order.

Lowering and validation finish before declarations or function bodies are
written. The driver writes the version and compiler-comment header before
semantic compilation, so a failed translation may leave that header in the
output file, but it must not leave a partial shader body.

## Type Mapping

Map compiler types as follows:

| Cg/compiler type | GLSL 1.10 type |
| --- | --- |
| `float`, `float1` | `float` |
| `float2`, `float3`, `float4` | `vec2`, `vec3`, `vec4` |
| `int`, `int1` | `int` |
| `int2`, `int3`, `int4` | `ivec2`, `ivec3`, `ivec4` |
| `bool`, `bool1` | `bool` |
| `bool2`, `bool3`, `bool4` | `bvec2`, `bvec3`, `bvec4` |
| `float2x2`, `float3x3`, `float4x4` | `mat2`, `mat3`, `mat4` |
| fixed-size array | equivalent sized GLSL array |
| struct | equivalent GLSL struct |

The compiler represents Cg matrices as packed arrays of row vectors.
Lowering must recognize the standard-library matrix typedefs rather than
treating them as ordinary arrays. Non-square standard-library matrix typedefs
produce a GLSL-specific unsupported-type diagnostic.

Cg matrix selectors use row-then-column notation. GLSL matrix indexing uses
column-then-row indexing. Lowering therefore maps `_mRC` to `[C][R]` and maps
multi-component selectors such as `_m00_m01_m02` to an explicit vector
constructor when they cannot be represented by one GLSL selector. Matrix
multiplication is lowered dimensionally so its mathematical result matches
Cg. Binding metadata states that uniforms use GLSL's matrix upload convention;
the application is responsible for adapting data that was previously uploaded
with Cg-specific layout assumptions.

Only types already recognized by this front end are in scope. Adding distinct
`half` or `fixed` base types to the parser is not part of the GLSL profile.

## Names and Declarations

Preserve a source name when it is a valid GLSL identifier, is not reserved by
GLSL, does not start with `gl_`, and does not collide in its emitted scope.
Otherwise, prefix it with `cg_` and append a stable numeric suffix only when
needed. All references use the resolved symbol identity, not spelling-based
lookup.

GLSL overloads are preserved when their mapped parameter types remain
distinct. If Cg overloads collapse to the same GLSL signature, such as a
`float1` overload and a `float` overload, mangle the helper names
deterministically and rewrite resolved call sites.

Emit content in this order:

1. `#version 110` from `PrintCodeHeader`;
2. compiler and binding comments;
3. struct declarations in dependency order;
4. attributes, varyings, uniforms, and other global declarations;
5. helper prototypes when a call cycle in declaration order requires them;
6. helper definitions; and
7. `void main()`.

Recursion remains an error in the existing front end. Prototypes serve only
forward references, not recursive programs.

## Entry-Point Lowering

The selected Cg entry point becomes GLSL's parameterless `void main()`.
Compiler-generated `$vin` and `$vout` connector references are never emitted
as GLSL structs.

- Non-uniform input parameters become locals initialized from attributes,
  varyings, or fragment built-ins.
- Uniform entry parameters become global uniforms, and their references
  resolve directly to those globals.
- Output parameters and returned output structs remain readable locals while
  the compiler-generated final assignments become assignments to varyings or
  GLSL built-ins.
- The original entry-point name is not emitted as a second callable function.
- Ordinary user helper functions and structs remain separate definitions.

This uses the connector assignments already inserted by
`BuildSemanticStructs` and the return lowering performed by
`CheckFunctionDefinitions`. It avoids reconstructing the original return at
code-generation time.

## Interface and Semantics

All user interface values must be floating-point scalars or vectors. Integer
and Boolean values remain valid for locals, uniforms, expressions, and helper
parameters, but are rejected as attributes or varyings in this profile.

### Vertex Inputs

Vertex inputs use generated named attributes. Their emitted names are based on
the canonical semantic, for example:

```glsl
attribute vec4 cg_ATTRIB0;
attribute vec3 cg_NORMAL0;
```

Recognized input semantics are `ATTRIB0` through `ATTRIB15`, `POSITION`,
`NORMAL`, `COLOR0`, `COLOR1`, `TEXCOORD0` through `TEXCOORD7`, `TANGENT`,
`BINORMAL`, `BLENDWEIGHT`, and `BLENDINDICES`. An omitted numeric suffix is
canonicalized to zero where the semantic family is indexed. Conventional
aliases such as `DIFFUSE` for `COLOR0` and `SPECULAR` for `COLOR1` resolve to
the canonical name.

Every used vertex semantic consumes one attribute slot. Duplicate canonical
semantics or more than 16 attributes are errors. An entry input without a
semantic is rejected rather than receiving an order-dependent implicit
attribute.

### Vertex Outputs and Fragment Inputs

Vertex `POSITION` maps to `gl_Position` and is a required output. `PSIZE` maps
to `gl_PointSize`.

`COLOR0`, `COLOR1`, `TEXCOORD0` through `TEXCOORD7`, and `FOG`/`FOGCOORD`
become user varyings with deterministic names such as `cg_COLOR0` and
`cg_TEXCOORD0`. The fragment profile uses the same canonical spelling and type
rules, allowing separately translated stages to link when their Cg semantics
match.

Fragment `POSITION` or `WPOS` maps to read-only `gl_FragCoord`. `FACE`, when
used with a Boolean scalar, maps to read-only `gl_FrontFacing`.

A varying semantic may appear only once per stage and direction. The profile
rejects conflicting types or directions during binding. The portable varying
budget is 32 floating-point components.

### Fragment Outputs

`COLOR` and `COLOR0` map to `gl_FragColor`. `DEPTH` maps to `gl_FragDepth`.
`COLOR1` and higher indices are errors because the profile does not enable a
multiple-render-target extension.

### Uniforms and Samplers

Global uniforms and uniform entry parameters become global GLSL uniforms.
Names are preserved and sanitized through the common name allocator.
Equivalent references to the same source symbol share one declaration.

The profile registers `sampler1D`, `sampler2D`, `sampler3D`, and
`samplerCUBE` Cg types and maps them to `sampler1D`, `sampler2D`, `sampler3D`,
and `samplerCube`. Samplers are valid only as uniforms. Shadow, rectangle, and
extension-only sampler types are rejected.

The portable resource envelope is:

- 512 vertex uniform scalar components;
- 64 fragment uniform scalar components;
- 32 varying floating-point components;
- 16 vertex attributes;
- zero vertex texture units;
- two fragment texture units; and
- one fragment color output.

Arrays and structs count the scalar components of their leaves. Matrix
uniforms count all matrix components. Sampler uniforms count texture units and
not numeric uniform components.

## Expression and Statement Lowering

The structured lowerer supports:

- scalar and vector unary and binary arithmetic;
- scalar and vector assignment;
- constructors and explicit conversions;
- scalar and vector comparisons;
- scalar logical operators;
- swizzles and write masks;
- array, vector, and matrix indexing;
- struct selection;
- scalar conditional expressions;
- component-wise Cg conditional expressions, lowered to explicit component
  constructors when GLSL 1.10 has no equivalent vector conditional operator;
- function calls with `in`, `out`, and `inout` parameters;
- `if`/`else`, `for`, `while`, and `do` statements;
- `break` and `continue`;
- returns in helper functions; and
- fragment-only `discard`.

Compound assignments, increment/decrement forms, comma statements, chained
assignments, and struct assignments may arrive already normalized by the
existing compiler. The lowerer nevertheless validates every received node and
reports an unsupported-node diagnostic rather than printing a placeholder.

Recursion is rejected by the existing call-graph checks. `discard` in a
vertex shader is a stage error.

## Intrinsics

`stdlib.cg` will derive a shared GLSL preprocessor guard from the profile
macros `PROFILE_GLSLV` and `PROFILE_GLSLF`. Under that guard, GLSL-supported
intrinsics are declared as `__internal` overloads. Outside that guard, the
existing generic standard-library definitions remain byte-for-byte
equivalent, so the generic profile keeps its current semantics. The common
HAL callback assigns a stable intrinsic group and index after checking the
full resolved signature. Calls are lowered by that identity, so a user
function with the same spelling is not accidentally rewritten.

The initial intrinsic table includes:

| Cg spelling | GLSL lowering |
| --- | --- |
| `mul(a, b)` | dimension-checked `a * b` |
| `dot`, `cross`, `normalize`, `reflect`, `refract` | same GLSL built-in |
| `length`, `distance` | same GLSL built-in |
| `min`, `max`, `clamp`, `abs`, `sign` | same GLSL built-in |
| `floor`, `ceil`, `sqrt`, `exp`, `exp2`, `log`, `log2` | same GLSL built-in |
| `sin`, `cos`, `tan`, `asin`, `acos`, `atan` | same GLSL built-in |
| `rsqrt` | `inversesqrt` |
| `lerp` | `mix` |
| `frac` | `fract` |
| `saturate(x)` | `clamp(x, 0.0, 1.0)` with matching shape |
| `tex1D`, `tex2D`, `tex3D`, `texCUBE` | `texture1D`, `texture2D`, `texture3D`, `textureCube` |

Texture intrinsics are valid only in `glslf`. Coordinate and result types must
match the sampler target. Projective, bias, gradient, and explicit-level
variants are outside the first release unless they already have an exact base
GLSL 1.10 fragment-shader equivalent and receive a dedicated signature and
test.

## Output and Binding Metadata

`PrintCodeHeader_glsl` emits exactly:

```glsl
#version 110
```

The existing driver then emits its compiler version and command-line text with
the profile's `//` comment prefix.

Before GLSL declarations, the backend emits stable, parseable mappings:

```glsl
// cgc-bind attribute cg_ATTRIB0 ATTRIB0
// cgc-bind varying cg_TEXCOORD0 TEXCOORD0
// cgc-bind uniform objviewproj_matrix
```

Each line identifies the GLSL storage class, emitted name, and canonical Cg
semantic when a semantic exists. Declarations remain the source of truth.

GLSL 1.10 uniforms cannot express Cg uniform defaults. Preserve each default
as a `// cgc-default` line containing the emitted uniform name and normalized
constant values. The application must apply those defaults after linking.

The shader body and binding metadata are deterministic across builds and
runs: declaration ordering follows stable source or semantic order, generated
suffixes follow first use in that order, and floating-point constants use one
locale-independent format. The existing driver-owned compiler-version and
command-line comments intentionally record build and invocation provenance;
they are valid GLSL comments but are excluded from deterministic golden
comparisons. Tests also normalize line endings.

## Diagnostics

Reserve error numbers 6200 through 6299 for GLSL profiles so they do not
conflict with existing diagnostics or the ARB design's 6000 range. Add precise
diagnostics for:

- unsupported GLSL profile type;
- unsupported expression or statement;
- invalid stage operation;
- unsupported, duplicate, or out-of-range semantic;
- interface direction or type mismatch;
- unresolved GLSL name conflict;
- invalid intrinsic signature;
- unavailable strict-1.10 intrinsic or sampler feature; and
- portable resource-limit overflow.

Every diagnostic uses the nearest original `SourceLoc` retained on the AST or
GLSL node. The backend never silently approximates unsupported behavior and
never emits an extension to make an otherwise invalid program compile.

## Build and Generated Sources

Add all GLSL backend sources to `CMakeLists.txt` and `Makefile`. Add
`RegisterProfiles_glsl` to the startup registration table in `cgcmain.c`.

Add profile-guarded sampler and intrinsic declarations to `stdlib.cg`, then
regenerate and check in synchronized `stdlib.c` using the repository's
`tokenize` target.

The scanner already tokenizes `break` and `continue`, but the checked-in grammar
does not currently accept them. Add the two leaf statement kinds and grammar
productions needed by the approved control-flow subset, update the existing AST
walkers for those leaf nodes, and regenerate synchronized `parser.c` and
`parser.h` from `parser.y`. Record the Bison version used.

`glslangValidator` is an optional test dependency, not a compiler build or
runtime dependency. CMake detects it with `find_program` and registers shader
validation tests only when present.

Update `README.txt` with:

- `glslv` and `glslf` command examples;
- the strict GLSL 1.10 contract;
- supported and unsupported features;
- generated attribute and varying naming;
- binding and default metadata;
- matrix upload expectations; and
- optional validator testing.

## Verification

### Generic Regression

Compile `position.cg`, `reflection.cg`, `vertexlight.cg`, and
`vertexlight4.cg` through `generic`. Compare normalized output, diagnostics,
and exit status to frozen baselines so GLSL work cannot change generic-profile
behavior.

### Vertex Golden Tests

Compile all four bundled shaders with `glslv` and compare exact normalized
GLSL output. These cover input structs, generated attributes, output varyings,
uniform matrices, arrays, helper calls, matrix selectors, and entry lowering.

### Fragment and Focused Fixtures

Add small fixtures grouped by responsibility:

- profile registration and headers;
- scalar, vector, matrix, struct, and array types;
- vertex inputs, varyings, built-in outputs, aliases, and collisions;
- arithmetic, constructors, comparisons, conditionals, and control flow;
- uniforms, defaults, arrays, and matrix behavior;
- helper functions, overload resolution, and name sanitization;
- numeric and geometric intrinsic mappings;
- `tex1D`, `tex2D`, `tex3D`, `texCUBE`, and `discard` in fragments;
- cross-stage matching of varying declarations;
- portable resource boundaries; and
- every GLSL-specific diagnostic category.

Successful fixtures use exact golden source comparisons. Negative fixtures
check exit status and exact diagnostic code plus stable message text. A failed
fixture also verifies that no declaration or function body was emitted.

### Unit Tests

Add a small C test target for deterministic name allocation, type mapping,
structured IR constructors, semantic canonicalization, resource counting, and
intrinsic selection. Keep it C90-compatible like the compiler target.

### GLSL Validation

When `glslangValidator` is available, validate every successful golden output
with `-S vert` or `-S frag`. The file's `#version 110` selects the required
dialect. Golden, unit, diagnostic, and generic regression tests remain active
when the validator is absent.

The work is complete when:

1. both profiles register and select correctly;
2. all four bundled shaders produce deterministic `glslv` output;
3. fragment fixtures cover arithmetic, control flow, uniforms, 2D and cube
   textures, and discard;
4. every successful shader passes `glslangValidator` when available;
5. unsupported cases fail with the intended GLSL diagnostic; and
6. all generic regressions remain unchanged.

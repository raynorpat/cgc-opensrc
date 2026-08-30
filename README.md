# Release Information

This is the Cg 2.0 compiler: `cgc` compiles standalone Cg 2.0 shaders and
its `tokenize` helper regenerates the standard library. Both build with
CMake:

```sh
cmake -S . -B build
cmake --build build --config Release
```

## Language versions

The default language is Cg 2.0. The historical Cg 1.1 behavior stays
available explicitly through `-version 1.1`; `-version 2.0` selects the
default language explicitly, and any other version string is a
command-line error:

```sh
./build/cgc -quiet -profile generic shader.cg              # default: Cg 2.0
./build/cgc -quiet -version 2.0 -profile generic shader.cg # explicit 2.0
./build/cgc -quiet -version 1.1 -profile generic shader.cg # legacy mode
```

Cg 2.0 adds the complete scalar family (`char`, `unsigned char`, `short`,
`unsigned short`, `int`, `unsigned int`, `long`, `unsigned long`, `fixed`,
`half`, `float`, `double`, plus the compile-time `cint`/`cfloat`), their
literal suffixes (`u`/`ul`, `h`, `x`, `f`, `d`), typed implicit/explicit
conversions with lossy-conversion warnings, first-class sized and unsized
arrays (including `.length` and dynamic assignment), the full sampler
family as language types, interfaces with single inheritance and dynamic
dispatch, struct methods, profile-qualified overloads with wildcard
precedence, default arguments, and a backend-neutral normalized Cg IR.
Reserved words of Cg 2.0 that have no grammar productions (CgFX words,
C++-isms) are rejected with a reserved-word diagnostic; `technique`,
`pass`, and `compile` are recognized case-insensitively. The reserved-word
grid shipped in this release is best-effort pending direct verification
against the published specification (notably whether `vertexshader` is
reserved like its sibling `pixelshader`).

Scope: this compiler implements the standalone shader language and
standard library only. CgFX constructs — techniques, passes, annotations,
state assignments, and runtime effect selection — parse nowhere and stay
reserved. Legacy mode exists to preserve intentional historical behavior;
it accepts a superset of the old scanner keywords.

Under the default language the neutral `generic` profile lowers every
reachable construct to verified, typed Cg IR and prints it in a
deterministic normalized form (diagnostics-friendly; not a promised
interchange format). With explicit `-version 1.1 -profile generic` the
compiler prints the historical tree dump instead, byte-compatible with
earlier releases for legacy sources that clear the shared internal
gates: every language mode builds and verifies the Cg IR before any
emission, so a source triggering an internal-invariant rejection
(`C9011`/`C9012`) fails with an internal error in legacy mode too,
where an earlier release may have emitted output.

The release contains a pre-built parser (`parser.c` and `parser.h`) generated
from `parser.y` with GNU Bison. Normal builds use the checked-in generated
sources and do not require Bison. To regenerate the generated parser and
standard library, run:

```sh
cmake --build build --target regenerate_stdlib
cmake --build build --target regenerate_parser
```

The parser target requires GNU Bison. The standard-library target uses the
locally built `tokenize` executable. Both must leave the checked-in
generated sources unchanged.

## Profiles

The compiler provides these profiles:

- `generic` is the complete neutral backend: under Cg 2.0 it accepts every
  valid program and emits the normalized IR described above.
- `glslv`, `glslg`, and `glslf` translate Cg vertex, geometry, and fragment
  entry points to core GLSL 1.50.
- `hlslv` and `hlslf` translate Cg vertex and fragment entry points to
  DirectX 9 HLSL Shader Model 3.
- `arbvp1` emits base `!!ARBvp1.0` vertex assembly.
- `arbfp1` emits base `!!ARBfp1.0` fragment assembly.

For a single-config build, use:

```sh
./build/cgc -quiet -profile generic position.cg
./build/cgc -quiet -profile glslv -entry main -o shader.vert shader.cg
./build/cgc -quiet -profile glslg -entry main -po TRIANGLE \
  -po TRIANGLE_OUT -po Vertices=3 -o shader.geom geometry.cg
./build/cgc -quiet -profile glslf -entry main -o shader.frag shader.cg
glslangValidator -l shader.vert shader.geom shader.frag
```

For a multi-config Release build on Windows, use:

```powershell
.\build\Release\cgc.exe -quiet -profile generic position.cg
.\build\Release\cgc.exe -quiet -profile glslv -entry main -o shader.vert shader.cg
.\build\Release\cgc.exe -quiet -profile glslg -entry main -po TRIANGLE -po TRIANGLE_OUT -po Vertices=3 -o shader.geom geometry.cg
.\build\Release\cgc.exe -quiet -profile glslf -entry main -o shader.frag shader.cg
glslangValidator -l shader.vert shader.geom shader.frag
```

GLSL output begins with exactly `#version 150`, uses no extensions, and keeps
source structures and helper functions readable where core GLSL 1.50 permits.
There is no GLSL version selector and no retained GLSL 1.10 output mode.
Vertex inputs are generated named attributes such as `cg_ATTRIB0`. Vertex
outputs and fragment inputs use matching canonical semantic names such as
`cg_COLOR0` and `cg_TEXCOORD0`. Vertex `POSITION`, fragment `COLOR0`, and
fragment `DEPTH` map to `gl_Position`, a generated core output such as
`cg_COLOR0`, and `gl_FragDepth`.

Geometry entries require an input topology and `-po Vertices=N`. The input
topology may be a source modifier or one of the exact profile options
`POINT`, `LINE`, `LINE_ADJ`, `TRIANGLE`, and `TRIANGLE_ADJ`. Output modifiers
and options are `POINT_OUT`, `LINE_OUT`, and `TRIANGLE_OUT`; when omitted,
the output defaults to the point, line-strip, or triangle-strip family of the
input. Source and option settings may agree but conflicting settings are
diagnosed. The neutral `generic` profile retains an unknown maximum in its IR,
whereas `glslg` requires a positive known maximum.

Geometry inputs use `AttribArray<T>` for per-vertex values and its resolved
length is fixed by topology (1, 2, 4, 3, or 6 vertices respectively).
`POSITION` maps through `gl_in[].gl_Position` and `gl_Position`;
`PRIMITIVEID` and `LAYER` use the core geometry built-ins. A vertex-stage
`VERTEXID` read comes from `gl_VertexID` and is exported as a flat integer
varying so geometry can consume the matching `AttribArray<int>`—geometry does
not read `gl_VertexID` directly. `emitVertex`, `flatAttrib`, and
`restartStrip` provide explicit emission, persistent flat-output state, and
strip restart operations.

The generated `// cgc-bind` comments record attributes, varyings, built-ins,
uniforms, and canonical Cg semantics. A `// cgc-default` comment records a Cg
uniform default that GLSL cannot declare; the application must apply these
defaults after linking. Matrix uniforms follow GLSL's column-major upload
convention. Applications that previously supplied Cg row-oriented data must
adapt or transpose it when uploading the GLSL uniform.

The practical supported subset includes vertex, geometry, and fragment entry
points, scalars and vectors, square matrices, fixed arrays, structs, uniforms,
readable helpers and overloads, structured conditionals and loops,
`break`/`continue`, common numeric and geometric intrinsics, fragment
`discard`, and base 1D, 2D, 3D, and cube texture sampling in geometry and
fragment shaders.

Valid Cg 2.0 constructs outside that subset are never frontend errors: the
language accepts them (the neutral `generic` profile compiles every one of
them), and a GLSL profile instead reports a profile diagnostic naming the
unsupported operation — for example non-square matrices, interface
dispatch, dynamic unsized arrays, bitwise operators on vectors, vertex
texture sampling, recursion, rectangle samplers, or operations without an
exact core-1.50 translation. The enforced portable limits are 16 attributes,
64 vertex output components, 1,024 vertex uniform components, and 16 vertex
texture units; geometry allows 64 input components per input vertex, 128
output components per emitted vertex, 256 output vertices, 1,024 total output
components, 1,024 uniform components, and 16 texture units; fragment allows
128 input components, 1,024 uniform components, 16 texture units, and one
focused color output. Limit and capability failures are profile diagnostics
that report the rejected resource or operation before publishing output.

`glslangValidator` is an optional test dependency. CMake discovers it once
when tests are configured and adds stage-correct validation for every
successful GLSL fixture; golden, unit, diagnostic, interface-text, and generic
regression tests remain enabled when it is absent. Run the suites with:

```sh
cmake -S . -B build -DBUILD_TESTING=ON
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

## DirectX 9 HLSL Shader Model 3 profiles

`hlslv` emits standalone `vs_3_0` source and `hlslf` emits standalone
`ps_3_0` source. A Windows Release build can translate and optionally compile
the result with the legacy DirectX compiler as follows:

```powershell
.\build\Release\cgc.exe -quiet -profile hlslv -entry main -o shader.vs.hlsl shader.cg
fxc /nologo /Gec /WX /T vs_3_0 /E main /Fo shader.vso shader.vs.hlsl
.\build\Release\cgc.exe -quiet -profile hlslf -entry main -o shader.ps.hlsl shader.cg
fxc /nologo /Gec /WX /T ps_3_0 /E main /Fo shader.pso shader.ps.hlsl
```

Generated files contain exactly one public HLSL `main`; source helpers and the
selected Cg entry remain behind that wrapper. Uniforms receive explicit
physical bindings in the DirectX 9 banks: floating-point data uses `c`, integer
data uses `i`, boolean data uses `b`, and samplers use `s`. A source `C#`,
`I#`, `B#`, `S#`, or `TEXUNIT#` semantic and the supported `#pragma bind`
forms reserve an explicit location. Remaining uniforms are allocated in
source order by deterministic first fit, with arrays, matrices, and
homogeneous structures reserving their complete register spans. Mixed-bank
structures are split into named leaves.

Cg defaults are preserved both as generated HLSL initializers and as
`// cgc-default` metadata for hosts that inspect the binding contract.
Matrices are declared `row_major`, matching the Cg source data convention.
Interface comments retain the source name while generated signatures use the
canonical DirectX semantic. Accepted aliases include vertex `HPOS`, `COL#`,
`TEX#`, `ATTR#`, and `ATTRIB#`, and pixel `WPOS`, `FACE`, `COL#`, and `TEX#`;
alias and case normalization still participate in duplicate detection.

The test suite performs exact source comparison, stage validation, and
cross-stage semantic, direction, shape, and type link checks. When `fxc` is
available it is an optional validation dependency. The automated validation
uses `/Gec /WX` so backward-compatible DirectX 9 syntax is accepted while all
warnings remain errors; `cgc` itself never invokes `fxc`. The exhaustive
[HLSL SM3 compatibility matrix](docs/hlsl-sm3-compatibility.md) classifies
every exposed type, qualifier, statement, operator, intrinsic, texture form,
semantic, binding, aggregate, and resource limit with a registered test.

These profiles intentionally do not emit DirectX assembly or bytecode and do
not invoke `fxc`, D3DX, `D3DCompile`, or another compiler. They do not target
Shader Models 1, 2, 4, 5, or 6; geometry, hull, domain, or compute stages; or
the DirectX effects framework, techniques, passes, and state blocks. DirectX
10+ `SV_*` semantics, DXIL compatibility, runtime rendering or image tests,
and updates to the parser or standard library for a newer Cg release are also
out of scope. Output is not promised to be byte-identical to NVIDIA Cg, and
the profiles do not predict optimization-dependent instruction or temporary
register usage without an external HLSL compiler.

## OpenGL ARB Profiles

`arbvp1` and `arbfp1` target the base `!!ARBvp1.0` and `!!ARBfp1.0`
languages from the Khronos ARB program specifications:

```sh
cgc -profile arbvp1 -entry main -o program.arb shader.cg
cgc -profile arbfp1 -entry main -o program.arb shader.cg
```

The backend preserves the existing Cg front end and rejects NVIDIA profile
options. Loops with compile-time-resolvable bounds are statically unrolled.
Relative uniform indexing is available only in vertex programs through an
address register; fragment uniform indices must resolve at compile time.
Fragment discard lowers to `KIL`. The profiles enforce the portable minimum
resource limits guaranteed by the base specifications: 128 vertex
instructions, 12 vertex temporaries, 96 vertex parameter vectors, and 16
vertex attributes; and on the fragment side, 72 total instructions (48 ALU,
24 texture), 4 texture indirections, 16 temporaries, 24 parameter vectors,
10 attributes, and 2 texture units.

Cg 2.0 geometry modules are verified by the shared front end and then rejected
by both ARB profiles with controlled diagnostic C6011 before legacy lowering;
the base ARB program languages have no geometry stage.

Windows test builds additionally compile a hidden-window WGL smoke test that
loads generated assembly with `glProgramStringARB`; CTest skips it when the
required driver extension is unavailable.

An optional compatibility oracle compares acceptance and public binding
metadata against a locally installed NVIDIA Cg compiler. Configure it
explicitly; the normal build and test suite neither requires nor
redistributes the proprietary executable:

```sh
cmake -S . -B build -DBUILD_TESTING=ON \
  -DCGC_REFERENCE_EXECUTABLE="path/to/NVIDIA/cgc"
ctest --test-dir build -C Debug -L oracle --output-on-failure
```

Run the self-contained suite with:

```sh
cmake -S . -B build -DBUILD_TESTING=ON
cmake --build build --config Debug
ctest --test-dir build -C Debug --output-on-failure
```

The Cg 2.0 conformance manifest lives at `tests/cg20/conformance.csv`: one
row per normative requirement, each naming the single registered CTest that
answers it (`valid-generic`, `invalid-language`, `backend-reject`, or
`out-of-scope`). The `cg20_manifest` test enforces that every row names a
registered test, that requirement ids are unique, and that every intrinsic
opcode in `cg_stdlib.def` is unique. To run just the conformance and
regeneration checks:

```sh
ctest --test-dir build -C Release -R "cg20|stdlib_regeneration|parser" --output-on-failure
```

## Compiler Internals

Under the default Cg 2.0 language every profile first builds and verifies the
backend-neutral typed IR in `cg_ir.c` and `cg_ir_verify.c` before any target
code runs. `generic` prints it through `cg_ir_print.c`, the GLSL profiles
lower it to their structured source IR, and the ARB profiles use it for stage
validation before retaining their historical tree lowering. The
historical tree-printing back end remains in `generic_hal.[ch]` for explicit
`-version 1.1` compiles. The GLSL profiles share a structured source backend
in `glsl_ir.[ch]`, `glsl_lower.c`, and `glsl_codegen.c`, with stage-specific
HAL descriptors; their Cg-IR lowering lives in `glsl_lower.c`
(`GlslLowerCgIR`).

`hal.[ch]` describes the hardware abstraction layer by which profiles
communicate with the front-end. To add a new profile, you can use
`generic_hal.c` as a framework/example. A profile must register itself
with the HAL by calling `RegisterProfile` as part of compiler startup.
Then, if selected via the appropriate command line argument, the
profile will be called to verify various constructs in the source code,
deal with connector semantics, and finally, generate code. See
`generic_hal.c` for more details.

The directory contains four vertex examples: `vertexlight.cg`,
`vertexlight4.cg`, `position.cg`, and `reflection.cg`. They can be inspected
with `generic` or translated with `glslv`.

Developers can download all the latest Cg-related content from the
[NVIDIA Cg website](http://www.nvidia.com/Cg).

## License

Copyright (c) 2002, NVIDIA Corporation.

NVIDIA Corporation("NVIDIA") supplies this software to you in
consideration of your agreement to the following terms, and your use,
installation, modification or redistribution of this NVIDIA software
constitutes acceptance of these terms. If you do not agree with these
terms, please do not use, install, modify or redistribute this NVIDIA
software.

In consideration of your agreement to abide by the following terms, and
subject to these terms, NVIDIA grants you a personal, non-exclusive
license, under NVIDIA's copyrights in this original NVIDIA software (the
"NVIDIA Software"), to use, reproduce, modify and redistribute the
NVIDIA Software, with or without modifications, in source and/or binary
forms; provided that if you redistribute the NVIDIA Software, you must
retain the copyright notice of NVIDIA, this notice and the following
text and disclaimers in all such redistributions of the NVIDIA Software.
Neither the name, trademarks, service marks nor logos of NVIDIA
Corporation may be used to endorse or promote products derived from the
NVIDIA Software without specific prior written permission from NVIDIA.
Except as expressly stated in this notice, no other rights or licenses
express or implied, are granted by NVIDIA herein, including but not
limited to any patent rights that may be infringed by your derivative
works or by other works in which the NVIDIA Software may be
incorporated. No hardware is licensed hereunder.

THE NVIDIA SOFTWARE IS BEING PROVIDED ON AN "AS IS" BASIS, WITHOUT
WARRANTIES OR CONDITIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED,
INCLUDING WITHOUT LIMITATION, WARRANTIES OR CONDITIONS OF TITLE,
NON-INFRINGEMENT, MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, OR
ITS USE AND OPERATION EITHER ALONE OR IN COMBINATION WITH OTHER
PRODUCTS.

IN NO EVENT SHALL NVIDIA BE LIABLE FOR ANY SPECIAL, INDIRECT,
INCIDENTAL, EXEMPLARY, CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, LOST PROFITS; PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) OR ARISING IN
ANY WAY OUT OF THE USE, REPRODUCTION, MODIFICATION AND/OR DISTRIBUTION
OF THE NVIDIA SOFTWARE, HOWEVER CAUSED AND WHETHER UNDER THEORY OF
CONTRACT, TORT (INCLUDING NEGLIGENCE), STRICT LIABILITY OR OTHERWISE,
EVEN IF NVIDIA HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

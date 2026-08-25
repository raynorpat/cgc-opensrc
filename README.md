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
Reserved words of Cg 2.0 that are not implemented syntax (CgFX words,
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
earlier releases for legacy sources.

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
- `glslv` translates a Cg vertex entry point to strict GLSL 1.10.
- `glslf` translates a Cg fragment entry point to strict GLSL 1.10.

For a single-config build, use:

```sh
./build/cgc -quiet -profile generic position.cg
./build/cgc -quiet -profile glslv -entry main -o shader.vert shader.cg
./build/cgc -quiet -profile glslf -entry main -o shader.frag shader.cg
```

For a multi-config Release build on Windows, use:

```powershell
.\build\Release\cgc.exe -quiet -profile generic position.cg
.\build\Release\cgc.exe -quiet -profile glslv -entry main -o shader.vert shader.cg
.\build\Release\cgc.exe -quiet -profile glslf -entry main -o shader.frag shader.cg
```

GLSL output begins with exactly `#version 110`, uses no extensions, and keeps
source structures and helper functions readable where base GLSL 1.10 permits.
Vertex inputs are generated named attributes such as `cg_ATTRIB0`. Vertex
outputs and fragment inputs use matching canonical semantic names such as
`cg_COLOR0` and `cg_TEXCOORD0`. Vertex `POSITION`, fragment `COLOR0`, and
fragment `DEPTH` map to `gl_Position`, `gl_FragColor`, and `gl_FragDepth`.

The generated `// cgc-bind` comments record attributes, varyings, built-ins,
uniforms, and canonical Cg semantics. A `// cgc-default` comment records a Cg
uniform default that GLSL cannot declare; the application must apply these
defaults after linking. Matrix uniforms follow GLSL's column-major upload
convention. Applications that previously supplied Cg row-oriented data must
adapt or transpose it when uploading the GLSL uniform.

The practical supported subset includes vertex and fragment entry points,
scalars and vectors, square matrices, fixed arrays, structs, uniforms,
readable helpers and overloads, structured conditionals and loops,
`break`/`continue`, common numeric and geometric intrinsics, fragment
`discard`, and base 1D, 2D, 3D, and cube texture sampling in fragment shaders.

Valid Cg 2.0 constructs outside that subset are never frontend errors: the
language accepts them (the neutral `generic` profile compiles every one of
them), and a GLSL profile instead reports a profile diagnostic naming the
unsupported operation — for example non-square matrices, interface
dispatch, dynamic unsized arrays, bitwise operators on vectors, vertex
texture sampling, recursion, rectangle samplers, or operations without an
exact base-1.10 translation. Their portable OpenGL 2.0
limits are 16 vertex attributes, 32 varying floating-point components, 512
vertex and 64 fragment uniform components, zero vertex texture units, two
fragment texture units, and one fragment color output.

`glslangValidator` is an optional test dependency. CMake discovers it once
when tests are configured and adds stage-correct validation for every
successful GLSL fixture; golden, unit, diagnostic, interface-text, and generic
regression tests remain enabled when it is absent. Run the suites with:

```sh
cmake -S . -B build -DBUILD_TESTING=ON
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
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

Under the default Cg 2.0 language every profile consumes the backend-neutral
typed IR built by `cg_ir.c` and verified by `cg_ir_verify.c` before any
target code runs; `generic` prints it through `cg_ir_print.c`. The
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

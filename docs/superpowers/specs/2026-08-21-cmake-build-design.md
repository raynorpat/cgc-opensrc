# CMake Build Design

## Goal

Replace the legacy Microsoft Visual C++ project files with a small, portable CMake build that produces both the `cgc` compiler and its `tokenize` helper.

## Build Structure

The repository root will contain one `CMakeLists.txt`. It will declare a C project and list each target's sources explicitly:

- `cgc` will compile the compiler front end, generic profile, and checked-in generated sources.
- `tokenize` will compile the helper used to generate `stdlib.c` from `stdlib.cg`.

Normal builds will use the checked-in `stdlib.c`, `parser.c`, and `parser.h`. This keeps Bison and source regeneration out of the default build path. Debug configurations will define `CGC_DEBUG_THE_COMPILER`, preserving the legacy project behavior.

## Source Regeneration

CMake will expose opt-in regeneration targets:

- The standard-library regeneration target will run the newly built `tokenize` executable on `stdlib.cg` and update `stdlib.c`.
- The parser regeneration target will use Bison to update `parser.c` and `parser.h` from `parser.y`. Configuration will not require Bison, but requesting this target without Bison will produce a clear error.

Regeneration will not run as a dependency of `cgc`, avoiding generated-file churn and tool-version differences during ordinary builds.

## Legacy File Removal and Documentation

The obsolete Visual Studio 6 and Visual Studio 2008 files will be removed:

- `cgc.sln`
- `cgc.vcproj`
- `tokenize.vcproj`
- `cgc.dsw`
- `cgc.dsp`
- `tokenize.dsp`

The portable Makefile will remain. `README.txt` will describe the CMake configure, build, and regeneration commands instead of advertising the removed Visual Studio projects.

## Verification

Verification will cover:

1. Configuring CMake in a separate build directory.
2. Building both `cgc` and `tokenize`.
3. Running `cgc -profile generic` on a checked-in example shader and checking for successful output.
4. Exercising standard-library regeneration and parser regeneration when Bison is available.
5. Confirming that no legacy Visual Studio project or workspace files remain tracked.

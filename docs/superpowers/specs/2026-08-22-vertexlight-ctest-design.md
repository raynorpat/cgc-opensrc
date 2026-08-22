# Vertexlight CTest Smoke Tests Design

## Goal

Add CTest coverage that runs the `cgc` compiler against `vertexlight.cg` and
`vertexlight4.cg` using the generic profile. Each test succeeds when the
compiler exits with status zero and fails for any nonzero exit status.

## CMake Integration

The root `CMakeLists.txt` will include CTest so consumers can control test
registration through the standard `BUILD_TESTING` option. When testing is
enabled, it will add the `tests` subdirectory.

The test definitions will live in `tests/CMakeLists.txt`, keeping test-suite
configuration separate from the compiler targets. That file will register two
explicit tests:

- `cgc_vertexlight`
- `cgc_vertexlight4`

Each test will invoke `$<TARGET_FILE:cgc>` with `-profile generic` and the
absolute source-tree path to its shader. Using the target-file generator
expression supports single- and multi-configuration generators, while an
absolute shader path makes the tests independent of CTest's working directory.

Two direct `add_test()` calls are preferred over a loop or wrapper script. The
small amount of duplication keeps each registered test visible and avoids an
abstraction that is not needed for two exit-status-only checks.

## Test Contract

CTest's default process-result handling supplies the complete assertion:

- Exit status zero passes.
- Any nonzero exit status, crash, or failure to launch fails.

Compiler stdout and stderr remain visible in verbose or failing CTest output,
but the tests will not compare either stream with golden files. This keeps the
suite focused on successful generic-profile compilation and avoids brittle
tree-output comparisons.

## 64-bit Allocator Compatibility

The new smoke tests expose an existing access violation in 64-bit builds. The
memory-pool allocator stores its alignment mask as an `unsigned`, then applies
the complemented mask to `uintptr_t` pointer values and `size_t` allocation
sizes. On 64-bit Windows, complementing the 32-bit value zero-extends it during
the wider expression and clears the upper pointer bits.

`MemoryPool_rec.alignmask` will change from `unsigned` to `uintptr_t`. The mask
is used in pointer-width allocation arithmetic, so storing it at pointer width
fixes all affected expressions at their common source without repetitive casts
or a new alignment abstraction. The public `mem_CreatePool` interface remains
unchanged, and 32-bit behavior is preserved because `uintptr_t` remains 32 bits
there.

## Scope

The change enables CTest in the root build, adds test definitions under the
`tests` folder, and corrects the memory-pool alignment-mask width in `memory.c`.
It will not modify either shader, generated parser sources, standard library
generation, or public compiler interfaces. The existing `cgc` executable
remains a normal build target and is the executable exercised by both tests.

## Verification

Verification will configure and build 64-bit and Win32 Debug variants with
testing enabled, then run the two tests through CTest in both build trees. The
acceptance criteria are that each build discovers exactly the two requested
vertexlight tests and reports both as passed. Because allocator behavior
changes, `position.cg` and `reflection.cg` will also be compiled manually with
the generic profile in both architectures so all four bundled shader inputs
are covered.

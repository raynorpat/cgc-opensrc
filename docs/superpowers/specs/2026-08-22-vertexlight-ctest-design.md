# Vertexlight CTest Smoke Tests Design

## Goal

Add CTest coverage that runs the `cgc` compiler against `vertexlight.cg` and
`vertexlight4.cg` using the generic profile. Each test succeeds when the
compiler exits with status zero and fails for any nonzero exit status.

## CMake Integration

The root `CMakeLists.txt` will include CTest so consumers can control test
registration through the standard `BUILD_TESTING` option. When testing is
enabled, it will register two explicit tests:

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

## Scope

The change is limited to CTest registration in the root build. It will not
modify either shader, compiler behavior, generated parser sources, or standard
library generation. The existing `cgc` executable remains a normal build
target and is the executable exercised by both tests.

## Verification

Verification will configure and build the project with testing enabled, then
run the two tests through CTest. The acceptance criteria are that CTest
discovers exactly the two requested vertexlight tests and reports both as
passed.

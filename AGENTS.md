# Repository Guidelines

## Project Structure & Module Organization

This is a C codebase for the NVIDIA Cg compiler front end and its generic profile. Compiler stages live in paired files under `src/`: scanning in `src/scanner.c`/`src/scanner.h`, parsing in `src/parser.y` plus generated `src/parser.c`/`src/parser.h`, semantic analysis in `src/semantic.c`, and compilation in `src/compile.c`. Hardware abstraction code is in `src/hal.*`; use `src/generic_hal.*` as the reference when changing profile behavior. `src/cgcmain.c` contains the compiler entry point, while `src/tokenize.c` provides the standard-library tokenizer. The four example shaders—`tests/cg/position.cg`, `tests/cg/reflection.cg`, `tests/cg/vertexlight.cg`, and `tests/cg/vertexlight4.cg`—are manual-validation inputs; `src/stdlib.cg` supplies the built-in standard library.

## Coding Style & Naming Conventions

Match the local file first and avoid unrelated reformatting. The mixed legacy style predominantly uses four spaces for indentation, braces on the next line for function bodies, PascalCase for public functions and types (`RegisterProfile`, `AtomTable`), and concise lowercase names for locals and fields. Preserve existing pointer spacing such as `const char *name`, keep interfaces in matching headers, and retain the `__NAME_H` include-guard pattern. New source files must retain the NVIDIA license notice where required.

## Testing Guidelines

There is no automated test suite or stated coverage threshold. Validate compiler changes against all four bundled inputs: `tests/cg/position.cg`, `tests/cg/reflection.cg`, `tests/cg/vertexlight.cg`, and `tests/cg/vertexlight4.cg`. Exercise them with the generic profile and compare stdout, diagnostics, and exit status with the behavior before your change. For parser, scanner, semantic, or constant-folding work, add a minimal `.cg` regression input when it captures a distinct case; name it descriptively, for example `invalid-array-index.cg`, and document the expected result in the pull request.

## Commit & Pull Request Guidelines

History is minimal, so follow the existing concise, imperative subject style: `Add generic profile validation`. Keep each commit focused on one compiler concern. Pull requests should explain the affected compiler stage, behavioral impact, and manual checks performed. Link relevant issues and include representative before/after output when diagnostics or generated trees change. Avoid committing editor artifacts or compiled binaries.

## Generated Sources & Security

Treat `src/parser.y` as the grammar source of truth. When grammar changes require regeneration, keep `src/parser.c` and `src/parser.h` synchronized and mention the generator version in the pull request. Do not add proprietary shader sources, credentials, or machine-specific paths. Preserve the redistribution notices in `LICENSE` and existing source headers.

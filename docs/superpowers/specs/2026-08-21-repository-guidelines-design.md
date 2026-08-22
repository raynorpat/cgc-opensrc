# Repository Guidelines Design

## Purpose

Create a concise `AGENTS.md` contributor guide for this repository. The guide will help contributors navigate and modify the legacy Cg compiler source without claiming support for workflows that the repository does not document.

## Scope

The document will be titled **Repository Guidelines** and remain between 200 and 400 words. It will cover:

- the repository's flat C source layout and the roles of key files;
- coding and naming conventions visible in the existing C and header files;
- manual validation using the bundled `.cg` shader examples;
- commit guidance derived from the repository's single historical commit;
- pull-request expectations appropriate for compiler changes;
- parser-generation and licensing cautions.

Build-system instructions will be omitted at the user's request. The guide will not invent automated tests, coverage requirements, formatters, or linters that are not present in the repository.

## Content Approach

Use short Markdown sections with direct, actionable instructions and repository-specific examples such as `parser.y`, `generic_hal.c`, and `vertexlight.cg`. Clearly identify manual example compilation as validation rather than describing it as an automated test suite. Note that generated `parser.c` and `parser.h` should stay synchronized with `parser.y` when grammar changes are intentionally regenerated.

Commit advice will acknowledge the limited history and recommend concise, imperative subjects consistent with the existing commit. Pull requests should summarize affected compiler stages, report manual validation, and include representative output changes when behavior changes.

## Acceptance Criteria

- `AGENTS.md` exists at the repository root.
- The title is `Repository Guidelines`.
- The guide is 200–400 words.
- No build commands or build-system section is included.
- Every stated convention is supported by repository evidence or clearly framed as contributor guidance.

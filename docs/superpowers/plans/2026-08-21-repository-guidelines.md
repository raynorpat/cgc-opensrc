# Repository Guidelines Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Create a concise, repository-specific contributor guide at the project root.

**Architecture:** Add one standalone Markdown file describing the flat source layout, established C conventions, manual validation, contribution expectations, and generated-file cautions. Omit all build-system guidance as requested.

**Tech Stack:** Markdown, Git, PowerShell validation commands

---

### Task 1: Create and verify the contributor guide

**Files:**
- Create: `AGENTS.md`

- [ ] **Step 1: Create `AGENTS.md` with the approved content**

```markdown
# Repository Guidelines

## Project Structure & Module Organization

This is a flat C codebase for the NVIDIA Cg compiler front end and its generic profile. Compiler stages live in paired root-level files: scanning in `scanner.c`/`scanner.h`, parsing in `parser.y` plus generated `parser.c`/`parser.h`, semantic analysis in `semantic.c`, and compilation in `compile.c`. Hardware abstraction code is in `hal.*`; use `generic_hal.*` as the reference when changing profile behavior. `cgcmain.c` contains the compiler entry point, while `tokenize.c` provides the standard-library tokenizer. The root-level `.cg` files are sample shader inputs used for manual validation.

## Coding Style & Naming Conventions

Match the surrounding legacy C style; avoid unrelated reformatting. Use four spaces for indentation, braces on the next line for function bodies, and existing pointer spacing such as `const char *name`. Public functions and types generally use PascalCase (`RegisterProfile`, `AtomTable`); local variables and fields use concise lowercase names. Keep interfaces in matching headers and preserve the existing `__NAME_H` include-guard pattern. New source files must retain the NVIDIA license notice where required.

## Testing Guidelines

There is no automated test suite or stated coverage threshold. Validate compiler changes against all four bundled inputs: `position.cg`, `reflection.cg`, `vertexlight.cg`, and `vertexlight4.cg`. Exercise them with the generic profile and compare stdout, diagnostics, and exit status with the behavior before your change. For parser, scanner, semantic, or constant-folding work, add a minimal `.cg` regression input when it captures a distinct case; name it descriptively, for example `invalid-array-index.cg`, and document the expected result in the pull request.

## Commit & Pull Request Guidelines

History is minimal, so follow the existing concise, imperative subject style: `Add generic profile validation`. Keep each commit focused on one compiler concern. Pull requests should explain the affected compiler stage, behavioral impact, and manual checks performed. Link relevant issues and include representative before/after output when diagnostics or generated trees change. Avoid committing editor artifacts or compiled binaries.

## Generated Sources & Security

Treat `parser.y` as the grammar source of truth. When grammar changes require regeneration, keep `parser.c` and `parser.h` synchronized and mention the generator version in the pull request. Do not add proprietary shader sources, credentials, or machine-specific paths. Preserve the redistribution notices in `LICENSE` and existing source headers.
```

- [ ] **Step 2: Verify title, word count, and omitted build guidance**

Run:

```powershell
$text = Get-Content -Raw AGENTS.md
if (-not $text.StartsWith('# Repository Guidelines')) { throw 'Incorrect title' }
$count = ([regex]::Matches($text, '\b[\w.-]+\b')).Count
if ($count -lt 200 -or $count -gt 400) { throw "Word count: $count" }
if ($text -match '(?im)^## .*Build|\bmake\b|Visual Studio') { throw 'Build guidance found' }
"AGENTS.md verified: $count words"
```

Expected: `AGENTS.md verified:` followed by a count between 200 and 400.

- [ ] **Step 3: Check Markdown and repository status**

Run:

```powershell
git diff --check
git status --short
```

Expected: no whitespace errors; status lists only `?? AGENTS.md` in addition to any pre-existing user changes.

- [ ] **Step 4: Commit the contributor guide if requested**

```powershell
git add -- AGENTS.md
git commit -m "Add repository contributor guidelines"
```

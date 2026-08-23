# Cg 2.0 Language and Backend-Neutral IR Design

Date: 2026-08-23

Status: Approved design

## Purpose

Extend this compiler from its historical Cg language subset to the complete
standalone Cg 2.0 language and Cg 2.0 standard library. Cg 2.0 becomes the
default language mode. The compiler will validate and represent every valid
standalone Cg 2.0 shader in a typed, backend-neutral IR before any target
profile applies its restrictions.

This project establishes a complete frontend and a neutral backend boundary.
It preserves the currently supported GLSL output subset, but it does not add a
new production shader-language or assembly target. Production backend
expansion is a separate follow-on project.

The normative authority is NVIDIA's published Cg 2.0 Language Specification
and the Cg 2.0 standard-library documentation. Historical NVIDIA compiler
quirks are not conformance requirements.

Reference:

- [NVIDIA Cg 2.0 Language Specification](https://developer.download.nvidia.com/cg/Cg_2.0/2.0.0010/Cg-2.0_Dec2007_LanguageSpecification.pdf)

## Scope Decisions

| Area | Decision |
| --- | --- |
| Compatibility target | Complete Cg 2.0 language and standard library |
| Conformance authority | Published specification, not reference-compiler quirks |
| Source form | Standalone shaders compiled from a named entry point |
| CgFX | Excluded, including techniques, passes, and state execution |
| Default language | Cg 2.0 |
| Legacy behavior | Available explicitly with `-version 1.1` |
| First backend milestone | Typed, backend-neutral Cg IR |
| Standard library | Typed intrinsic IR plus constant-foldable or portable definitions |
| Existing GLSL profiles | Preserve their currently supported subset and output |
| New production targets | Deferred to separately designed follow-on projects |

## Current State and Principal Gaps

The repository already has a useful profile boundary. `scanner.c`, `parser.y`,
`symbols.c`, `semantic.c`, and `compile.c` implement the shared frontend and
middle-end. `hal.[ch]` supplies profile callbacks. `generic_hal.[ch]` prints the
historical compiler tree, while `glsl_lower.c`, `glsl_ir.[ch]`, and
`glsl_codegen.c` implement a structured GLSL 1.10 path.

The current frontend represents only the core scalar bases `cfloat`, `cint`,
`float`, `int`, and `bool`. Scalar identity is encoded in a four-bit field that
also feeds legacy operator subcodes, and half of the available values are
reserved for HAL-specific bases. That representation cannot faithfully model
the complete Cg 2.0 scalar set.

The parser and semantic analyzer already cover many useful Cg constructs,
including structs, packed vectors and matrices, first-class aggregate
assignments, overloads, binding semantics, structured control flow, and parts
of the standard library. The main missing or incomplete areas include:

- Distinct `char`, `short`, `long`, unsigned, `half`, `fixed`, and `double`
  types and their literal suffixes.
- The complete promotion and explicit/implicit conversion matrix.
- Interface declarations, struct methods, single-interface inheritance, and
  interface assignment and dispatch.
- Unsized arrays and their dynamic assignment behavior.
- Profile-qualified functions, wildcard profiles, and profile-specific
  overload ranking.
- Complete default-parameter rules.
- The complete Cg 2.0 sampler family and sampler compatibility rules.
- Complete standard-library signatures and explicit backend-neutral intrinsic
  identities.
- A target-independent typed IR boundary before GLSL-specific lowering.

## Goals

1. Parse and semantically validate the complete standalone Cg 2.0 language.
2. Make Cg 2.0 the default without removing an explicit legacy mode.
3. Separate language validity from target-profile capability.
4. Represent every valid reachable Cg 2.0 construct in verified Cg IR.
5. Represent every Cg 2.0 standard-library call as either a selected portable
   definition or a typed intrinsic operation.
6. Preserve existing GLSL output for the overlapping supported subset.
7. Give unsupported backends precise profile diagnostics rather than frontend
   errors, crashes, or partial output.
8. Make conformance measurable through a requirement-to-test manifest.

## Non-Goals

- Parsing or executing CgFX techniques, passes, annotations used only by CgFX,
  state assignments, or runtime effect selection.
- Bug-for-bug matching of an NVIDIA Cg Toolkit binary.
- Adding Direct3D bytecode, historical GPU assembly, SPIR-V, or a newer GLSL
  production backend in this project.
- Implementing a software shader interpreter or texture runtime.
- Serializing Cg IR as a stable public interchange format.
- Refactoring unrelated legacy code.

## Architecture

The completed pipeline is:

```text
preprocessor
    -> scanner and parser
    -> typed AST and symbol graph
    -> Cg 2.0 semantic analysis
    -> entry selection and reachability
    -> verified Cg IR
    -> HAL profile validation
    -> target lowering and emission
```

Language version and target profile are independent choices. Cg 2.0 semantic
analysis accepts constructs that a particular backend cannot generate. After
the selected entry point is known, the compiler computes its reachable
functions and globals, builds Cg IR for that program, and lets the HAL enforce
target capabilities.

This separation follows the specification's rule that profile restrictions
apply to the selected top-level program and its reachable dependencies rather
than to unrelated declarations in the source file.

### Language policy

Add a `CgLanguageVersion` enum to compiler options with values for 1.1 and 2.0.
The command line accepts `-version 1.1` and `-version 2.0`; omission selects
2.0. Unknown version strings are command-line errors.

Both versions use one scanner, grammar, AST, and diagnostic engine. Version
policy helpers answer questions such as whether a syntax form is available or
which conversion behavior applies. Version checks must be centralized in
these helpers rather than spread across parser actions and semantic walkers.

The legacy mode exists to preserve intentional historical behavior. It is not
a second compiler and it does not prevent language-neutral correctness fixes.

## Frontend Components

### Scanner and parser

`parser.y` remains the grammar source of truth. Grammar changes must regenerate
and commit synchronized `parser.c` and `parser.h` using the repository's CMake
target, with the generator version recorded in the implementation work.

The scanner and grammar will cover:

- All normative Cg 2.0 scalar type spellings and sampler spellings.
- Cg 2.0 integer and floating literal suffixes, including signed-width,
  unsigned-width, `half`, `fixed`, `float`, and `double` forms.
- Interface declarations and interface member function declarations.
- Struct member functions and single-interface inheritance.
- Profile-qualified function declarations and definitions.
- Default arguments under the normative placement and constness rules.
- Sized and unsized array declarations.
- All specified vector and matrix swizzle forms.

Words reserved by Cg 2.0 but not part of standalone shader syntax remain
unavailable as identifiers. When encountered where an identifier is expected,
the compiler reports a reserved-word diagnostic instead of treating the word
as an implemented construct. This includes CgFX-only words. Reserved words
marked case-insensitive by the specification are recognized case-insensitively;
all other keyword matching retains its specified case behavior.

### Canonical type system

Scalar identity moves out of the four-bit `Type.properties` base field into an
explicit `CgScalarKind`. Required scalar identities include compile-time
`cint` and `cfloat`, Boolean, signed and unsigned integer widths, `fixed`,
`half`, `float`, and `double`. `void`, samplers, arrays, structs, interfaces,
and functions remain distinct type categories rather than scalar identities.

Qualifiers, domain, packedness, array sizing, and type category remain
orthogonal properties. Canonical types are interned so equivalent types have a
stable identity. Profile-specific extension types, if later needed, use a
registry and do not consume a fixed range inside the language enum.

Expression and declaration nodes retain canonical `Type *` results. Legacy
operator subcodes may cache lowering information during migration, but they
are no longer authoritative for expression type. All consumers that currently
decode scalar identity from four-bit operator fields must migrate to canonical
types before additional scalar types are enabled for that consumer.

The semantic type module owns:

- Type equivalence and qualifier comparison.
- Numeric rank, signedness, precision, range, and category queries.
- Implicit conversion classification.
- Explicit conversion classification.
- Warning requirements for lossy implicit conversions.
- Scalar smearing and vector/matrix shape compatibility.
- Structure, array, sampler, and interface compatibility.

### Constants and arithmetic

Constants retain both their explicit or compile-time source kind and their
converted canonical type. Constant evaluation implements the Cg 2.0 usual
arithmetic conversions before applying an operation.

The evaluator must provide deterministic behavior for required integer widths,
unsigned arithmetic, `half`, `fixed`, `float`, and `double`. `fixed` folding
uses the specified minimum range and clamping behavior. Approximate floating
results need not be bit-identical to NVIDIA's compiler, but folding must occur
at a precision and range permitted by the specification.

Undefined constant operations, overflow cases for which the language requires
a diagnostic, and non-finite target literals use controlled diagnostics rather
than host-C undefined behavior.

### Declarations, scopes, and interfaces

The existing regular and tag namespaces remain. Symbols gain explicit data
for:

- Function default arguments.
- Exact and wildcard profile selectors.
- Interface methods.
- Struct method implementations.
- A struct's optional single inherited interface.

An interface may contain function declarations but no data fields or default
implementations. A struct inheriting an interface must provide a compatible
implementation of every interface method. Interface values are dynamic typed
objects: assignment checks compatibility, while their current concrete type is
preserved in the typed AST and Cg IR.

### Arrays and aggregates

Arrays remain first-class values. The frontend distinguishes packed and
unpacked arrays, preserves nested shape, supports arrays inside structures,
and implements full-array copy semantics.

An unsized array declared without an initializer has a dynamic array type. An
initializer gives an otherwise unsized declarator a concrete size. Assignment
to a dynamic array validates element compatibility and updates its dynamic
shape in Cg IR. `.length` is a typed array operation, not a field lookup.

Profile limits on indexing, copying, lvalue subscripting, or maximum sizes are
not language errors. They are checked after reachability by the selected HAL.

### Overload resolution

Overload selection implements the specification's ordered algorithm:

1. Collect visible functions with the requested name.
2. Filter incompatible exact or wildcard profile selectors.
3. Filter arity, retaining candidates whose trailing parameters have defaults.
4. Compare actual parameters from left to right using exact match, compatible
   dynamic type, promotion, and implicit conversion in that order.
5. Prefer an exact profile selector, then the most-specific matching wildcard,
   then an open-profile function.
6. Require one remaining candidate.

The HAL supplies immutable profile identity metadata: exact profile name,
vertex or fragment family, supported wildcard families, and a total
specificity ordering for matching wildcards. It must not reimplement language
overload resolution.

### Standard library

Add `cg_stdlib.def` as the declarative source of truth for Cg 2.0 standard
library operations. Each entry records:

- Source name and intrinsic identity.
- Parameter and result type families.
- Direction qualifiers.
- Allowed scalar precisions and vector or matrix shapes.
- Purity and side-effect classification.
- Whether compile-time folding is available.
- Whether a portable `stdlib.cg` implementation is preferred.

Initialization expands these descriptions into ordinary overload symbols so
standard calls use the same resolver as user functions. After selection, a
runtime-only standard call becomes a typed `CgIRIntrinsic` operation. Portable
definitions in `stdlib.cg` remain ordinary functions and may themselves call
intrinsics.

The table and generated tokenized standard library must have reproducibility
tests. Every intrinsic entry must have a unique stable opcode and at least one
signature coverage test.

## Cg IR

`cg_ir.c` and `cg_ir.h` define a backend-neutral, structured, typed IR. A
`CgIRModule` represents one selected entry point under one selected compilation
profile. It owns canonical references to reachable globals, functions, binding
semantics, and source locations.

### Type coverage

Cg IR can represent:

- All Cg 2.0 scalar and compile-time scalar kinds.
- Vectors and matrices without erasing element precision or shape.
- Sized, unsized, packed, and unpacked arrays.
- Struct and interface types.
- Sampler types.
- Function signatures and parameter direction.

### Expression coverage

Expression nodes include typed constants, symbol references, field and method
selection, indexing, swizzles and write masks, construction, casts, unary and
binary operations, assignment, conditional selection, user calls, interface
method calls, and intrinsic calls.

Selected user calls store symbol identity rather than only a source name.
Intrinsic calls store a `CgIRIntrinsic` plus the selected typed signature.
Interface assignment and dispatch remain explicit high-level operations; the
neutral IR does not force a tag-and-branch implementation.

### Statement coverage

Statements preserve blocks, declarations, expression statements,
conditionals, loops, returns, `break`, `continue`, and `discard`. Structured
control flow remains structured. Target-specific flattening or unrolling is a
backend responsibility.

### Ownership and source locations

Each module owns its nodes through a module arena or an equivalent single-owner
allocator consistent with the repository's C allocation style. IR nodes do not
own frontend symbols or type objects; they hold stable references whose
lifetime covers module validation and generation.

Every user-derived node retains a `SourceLoc`. Compiler-synthesized nodes retain
the nearest responsible source location and a synthesized flag.

### IR verification

A standalone verifier runs immediately after construction and before any HAL
callback. It checks:

- Canonical and compatible result types.
- Symbol and declaration ownership.
- Operand counts and operand types.
- Lvalue requirements and write-mask uniqueness.
- Call arity, parameter directions, and selected signature identity.
- Intrinsic signature consistency.
- Legal placement of return, loop jumps, and discard.
- Interface assignment and dispatch compatibility.
- Required source locations.

Verifier failures are compiler defects, not user language errors.

## Profile Integration and Data Flow

Compilation proceeds in this order:

1. Parse the entire translation unit using the selected language version.
2. Perform profile-independent declaration and semantic checking, retaining
   candidate sets for calls whose winner depends on the compilation profile.
3. Select the entry point and resolve those profile-qualified calls using
   immutable HAL profile identity metadata, then complete call-dependent type
   checks.
4. Compute reachable functions and globals.
5. Build Cg IR for the reachable program.
6. Verify Cg IR.
7. Invoke profile capability validation.
8. If code generation is enabled, perform target-internal lowering and emit
   output.

The HAL exposes IR-oriented validation and generation hooks. `ValidateIR`
checks target capabilities without modifying the module. `GenerateIR` owns any
target-specific lowering and emission. A backend may internally split
generation into `LowerIR` and emission, but no opaque cross-profile lowered-IR
ABI is introduced in the shared HAL. The existing AST-oriented `GenerateCode`
hook remains temporarily available until every current profile has migrated.

This two-hook public interface is the concrete form of the approved logical
validate/lower/generate lifecycle and avoids adding an unnecessary shared
opaque backend-object protocol.

### Generic profile

In Cg 2.0 mode, `generic` is the complete neutral backend. It accepts every
verified core Cg IR module and emits a deterministic normalized textual form.
The normalized form is intended for diagnostics, golden tests, and compiler
development; it is not a promised external serialization format.

During migration, `-version 1.1 -profile generic` preserves the historical tree
output. Once all legacy behavior tests have an explicit mode, compatibility
changes to that historical output require a separately approved design.

### GLSL profiles

`glsl_ir` remains a target-specific representation. `glsl_lower.c` migrates
from consuming the legacy frontend tree to consuming Cg IR. The migration must
preserve existing generated GLSL text for the already supported subset.

Valid Cg 2.0 operations outside strict GLSL 1.10 capabilities fail in HAL
validation with a profile diagnostic. They must not be rejected by the parser,
language type checker, or Cg IR verifier.

## Diagnostics and Failure Handling

Diagnostics are classified by their responsible layer:

- Scanner or parser: malformed tokens, reserved words, or invalid syntax.
- Language semantics: invalid declarations, types, conversions, overloads,
  interfaces, arrays, sampler use, or control flow.
- IR verifier: internal compiler invariant failure.
- Profile validation: a valid Cg program that the selected target cannot
  implement.

Existing codes and wording remain stable when the underlying behavior is
unchanged. New codes are grouped by subsystem. A primary diagnostic identifies
the most specific source location. Notes identify related declarations,
interface requirements, close overload candidates, or a reachable call path.

Invalid expressions receive a poison type. Later checks recognize poison and
avoid derivative messages. Overload failures report actual types and concise
reasons for the closest candidates. Interface errors identify both the
incompatible implementation and its required declaration. Profile errors name
the profile and, for helper code, show a reachability chain from the entry
point.

No target output is committed until parsing, semantic analysis, Cg IR
construction, verification, and profile validation succeed. Emission uses a
temporary destination and atomically replaces the requested output only after
success. A failed compile removes its temporary file and preserves any
pre-existing destination.

`-nocode` performs every step through profile validation and skips only target
lowering and emission.

In release builds, an IR verifier failure produces one controlled internal
compiler diagnostic. Assertion-enabled test builds additionally assert at the
failure site.

## Testing Strategy

Add a checked-in conformance manifest that maps every normative standalone Cg
2.0 requirement to tests. Each row has exactly one status:

- Supported and positively tested.
- Invalid by a required language rule and negatively tested.
- Valid in Cg IR but rejected by an explicitly named backend.
- Outside scope because it belongs to CgFX.

There is no unresolved or generic "unsupported" status at completion.

Fixtures live under `tests/cg20/` and are grouped by literals, types,
conversions, arrays, structs, interfaces, overloads, control flow, semantics,
intrinsics, diagnostics, and profile isolation.

Test layers include:

- Unit tests for canonical types, type equivalence, conversions, overload
  ranking, constant evaluation, intrinsic lookup, reachability, and IR
  verification.
- Positive and negative shader fixtures for grammar and semantic rules.
- Stable normalized Cg IR golden fixtures under the `generic` profile.
- Generated signature coverage for every standard-library overload family.
- Reachability cases showing that an unsupported construct fails only when it
  is reachable from the selected entry point.
- Default Cg 2.0 runs of existing shaders in the overlapping language subset.
- Explicit `-version 1.1` tests for intentionally different legacy behavior.
- Existing GLSL golden, validation, interface, diagnostic, and limit tests.
- Parser and standard-library regeneration reproducibility tests.

Reference-compiler comparison may be used as non-gating investigation data,
but it cannot override the published specification or become a required test
dependency.

## Milestone Boundaries

The implementation plan should divide work into independently releasable
milestones in this order:

1. Conformance manifest, language-version plumbing, and locked legacy baseline.
2. Canonical scalar type representation and literal support.
3. Normative conversions, arithmetic, and constant evaluation.
4. Complete arrays, aggregates, and samplers.
5. Interfaces, struct methods, and inheritance.
6. Default arguments and profile-qualified overload resolution.
7. Declarative standard library and typed intrinsic identities.
8. Cg IR construction, verification, and normalized generic emission.
9. Migration of current GLSL lowering to Cg IR with identical supported output.
10. Full conformance audit, documentation, and release qualification.

Each milestone begins with failing focused tests and ends with the complete
existing CTest suite passing. A milestone must not enable a partially modeled
type or construct in default Cg 2.0 mode before its semantic and IR invariants
are enforced.

## Completion Criteria

The project is complete only when:

1. Every conformance-manifest row has a passing test and no unresolved status.
2. Every Cg 2.0 standard-library signature resolves to a typed intrinsic or a
   portable implementation.
3. Valid standalone shaders covering every language feature reach verified Cg
   IR through the default Cg 2.0 mode.
4. The neutral `generic` backend emits deterministic IR for those shaders.
5. Existing supported GLSL fixtures retain their expected generated output.
6. Unsupported GLSL features produce profile diagnostics rather than frontend
   errors, internal failures, or partial files.
7. Explicit legacy-mode compatibility tests pass.
8. Parser and standard-library generated sources reproduce exactly.
9. The full CTest suite passes in release and assertion-enabled builds.

## Risks and Mitigations

### Pervasive four-bit type assumptions

Risk: operator encodings, folding, printing, semantic helpers, and HAL checks
may silently truncate new scalar identities.

Mitigation: make canonical `Type *` data authoritative first, add audit tests
for every old bit-mask use, and enable new types one family at a time only
after their consumers migrate.

### Grammar ambiguity

Risk: profile specifiers, type names, interface members, and struct methods may
introduce conflicts in the legacy grammar.

Mitigation: add minimal grammar fixtures for each form, regenerate with one
documented Bison version, inspect conflicts explicitly, and avoid accepting
CgFX productions.

### Standard-library scale

Risk: handwritten overload declarations become incomplete or inconsistent.

Mitigation: use one declarative table, generate ordinary overload symbols, and
derive exhaustive signature tests from that table.

### Legacy regressions after changing the default

Risk: existing scripts observe new conversions or diagnostics without opting
in.

Mitigation: run existing compatible shaders under default 2.0, add explicit
1.1 tests for intentional differences, preserve old diagnostic behavior where
the rules agree, and document the default change prominently.

### Backend leakage into language semantics

Risk: current HAL checks may reject valid Cg 2.0 before Cg IR construction.

Mitigation: classify every check as language, IR invariant, or profile
capability; move profile capability checks after reachability; and test valid
but backend-unsupported programs through the neutral `generic` profile.

## Follow-On Backend Work

After this project passes its conformance and neutral-IR gates, each production
backend expansion receives its own design and implementation plan. That design
must choose concrete targets, capability mappings, resource limits, interface
semantics, intrinsic lowerings, and validation tools. Likely options include a
newer GLSL family, SPIR-V, or historical Cg assembly profiles, but this design
does not select among them.

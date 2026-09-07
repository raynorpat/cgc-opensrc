# GLSL Lowering Modularization Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [x]`) syntax for tracking.

**Goal:** Mechanically split glsl_lower.c into private modules while preserving compiler behavior.

**Architecture:** Keep the existing public facade and stack-owned context. Install the complete cross-module interface before moving functions, then extract one responsibility per commit. Use one canonical CMake source list and backend-specific structural tests.

**Tech Stack:** C90 with extensions, CMake 3.16+, MSVC x64, CTest, glslangValidator, Windows SDK FXC, Windows OpenGL smoke validation.

## Approved design and starting state

Design: `docs/superpowers/specs/2026-09-07-glsl-lowering-modularization-design.md`.
Inventory was derived from commit `7774767b115522c4d9c21c9ee49c6471bd1f1a3f`.
Execute GLSL first; ARB has its own plan and fresh subsequent baseline.
Reconcile the exact inventory against the execution baseline before editing if source changes land meanwhile.

Planning makes no compiler changes and does not claim new test results. The previously
verified inventory is 1,370 tests. Each plan adds one registered structural test:
GLSL expects 1,371 and subsequent ARB expects 1,372, provided the baseline inventory
has not changed independently. Compare names, not only totals.

## File structure

| File | Responsibility / original definition count |
|---|---|
| glsl_lower.c | public facade (2) |
| glsl_lower_support.c | support (17) |
| glsl_lower_decl.c | decl (40) |
| glsl_lower_interface.c | interface (26) |
| glsl_lower_aggregate.c | aggregate (41) |
| glsl_lower_legacy_expr.c | legacy expr (17) |
| glsl_lower_legacy_stmt.c | legacy stmt (4) |
| glsl_lower_ir_expr.c | ir expr (18) |
| glsl_lower_ir_stmt.c | ir stmt (4) |
| glsl_lower_geometry.c | geometry (15) |
| glsl_lower_function.c | function (21) |
| glsl_lower_internal.h | existing types and 89 cross-module declarations |
| CMakeLists.txt | canonical CGC_GLSL_LOWER_SOURCES and cgc consumption |
| tests/CMakeLists.txt | real GLSL unit consumption and structural registration |
| tests/check_glsl_lower_structure.cmake | module/build/public and private ownership guard |
| tests/check_glsl_lower_structure_selftest.cmake | positive/negative checker fixtures |

Do not modify code generation, public headers, profile algorithms, expected shader
files, parser sources, or other backends. Existing warnings are baseline evidence,
not cleanup tasks.

### Task 1: Isolate and capture a pristine baseline

**Files:** Read source and tests only; create ignored build artifacts.

- [x] **Step 1: Pin baseline and create independent worktrees.** Run from the main checkout.
Choose these exact unused paths; if a path already exists, inspect it instead of overwriting it.

```powershell
$baseCommit = (git rev-parse HEAD).Trim()
git check-ignore .worktrees
if ($LASTEXITCODE -ne 0) { throw 'worktree directory is not ignored' }
git worktree add --detach .worktrees/glsl-lower-pristine $baseCommit
if ($LASTEXITCODE -ne 0) { throw 'baseline worktree failed' }
git worktree add .worktrees/glsl-lower-modularization -b codex/glsl-lower-modularization $baseCommit
if ($LASTEXITCODE -ne 0) { throw 'candidate worktree failed' }
```

Record the full baseline SHA. Preserve user changes in the main checkout.

- [x] **Step 2: Build the pristine baseline in both configurations.**
Run inside `.worktrees/glsl-lower-pristine`.

```powershell
cmake -S . -B build-cg20-glsl-baseline -A x64 -DBUILD_TESTING=ON -DCGC_REQUIRE_FXC=ON
if ($LASTEXITCODE -ne 0) { throw 'configure failed' }
Select-String -Path build-cg20-glsl-baseline/CMakeCache.txt -Pattern 'GLSLANG_VALIDATOR:FILEPATH|FXC_EXECUTABLE:FILEPATH'
cmake --build build-cg20-glsl-baseline --config Release
if ($LASTEXITCODE -ne 0) { throw 'Release build failed' }
ctest --test-dir build-cg20-glsl-baseline -C Release --output-on-failure
if ($LASTEXITCODE -ne 0) { throw 'Release baseline failed' }
cmake --build build-cg20-glsl-baseline --config Debug
if ($LASTEXITCODE -ne 0) { throw 'Debug build failed' }
ctest --test-dir build-cg20-glsl-baseline -C Debug --output-on-failure
if ($LASTEXITCODE -ne 0) { throw 'Debug baseline failed' }
```

Expected: all registered tests pass. Require actual glslangValidator and FXC paths,
not NOTFOUND. Record any ARB WGL skip (return 77) separately from a pass. Do not
claim driver validation for a skipped smoke test.

- [x] **Step 3: Snapshot test inventory and artifacts.**
Keep the baseline build untouched afterward. Read CTest JSON with
`ctest --test-dir build-cg20-glsl-baseline -C Release --show-only=json-v1`.
Record sorted test names, commands, validator locations and complete artifact paths.
GLSL output includes both tests/Release/**/*.glsl and tests/validated/Release/**/*.glsl.
Do not copy compiler binaries or generated outputs into tracked source.

### Task 2: Install complete private interface and initial canonical wiring

**Files:** Modify `glsl_lower.c`, `CMakeLists.txt`, `tests/CMakeLists.txt`; create `glsl_lower_internal.h`.

- [x] **Step 1: Move the original private types into the following header.**
Copy the complete original NVIDIA notice verbatim above the include guard; the
comment below is an instruction, not replacement license text. Standard library
includes remain before the private header in every C translation unit.

```c
/* Copy the complete original NVIDIA notice before this guard. */
#ifndef __GLSL_LOWER_INTERNAL_H
#define __GLSL_LOWER_INTERNAL_H

#include "slglobals.h"
#include "glsl_hal.h"
#include "cg_stdlib.h"
#include "cg_ir.h"

#define GLSL_MATRIX_MAX_ARGUMENTS 16

typedef struct GlslMatrixHelper_Rec {
    struct GlslMatrixHelper_Rec *next;
    GlslFunction *function;
    GlslType result;
    GlslType parameters[GLSL_MATRIX_MAX_ARGUMENTS];
    int parameterCount;
} GlslMatrixHelper;

typedef enum GlslMatrixSelectorHelperKind_Enum {
    GLSL_MATRIX_SELECTOR_GET,
    GLSL_MATRIX_SELECTOR_SET
} GlslMatrixSelectorHelperKind;

typedef struct GlslMatrixSelectorHelper_Rec {
    struct GlslMatrixSelectorHelper_Rec *next;
    GlslFunction *function;
    GlslMatrixSelectorHelperKind kind;
    GlslType matrixType;
    GlslType valueType;
    int count;
    int mask;
} GlslMatrixSelectorHelper;

typedef struct GlslInterfaceSource_Rec {
    struct GlslInterfaceSource_Rec *next;
    const Symbol *source;
    const char *interfaceKey;
    const char *reservedName;
    int isOutput;
} GlslInterfaceSource;

typedef struct GlslGeometryFlat_Rec {
    struct GlslGeometryFlat_Rec *next;
    int semantic;
    GlslDecl *target;
    GlslDecl *shadow;
    GlslDecl *defined;
} GlslGeometryFlat;

typedef struct GlslGeometryInputBinding_Rec {
    struct GlslGeometryInputBinding_Rec *next;
    const Symbol *source;
    GlslDecl *declaration;
} GlslGeometryInputBinding;

typedef struct GlslGeometryOutputBinding_Rec {
    struct GlslGeometryOutputBinding_Rec *next;
    const char *interfaceKey;
    GlslDecl *declaration;
} GlslGeometryOutputBinding;

typedef struct GlslLowerContext_Rec {
    GlslModule *module;
    const GlslProfileDesc *profile;
    /* Legacy -version 1.1 tree path only; the Cg IR lowering never
     * reads frontend scopes (see GlslLowerCgIR below). */
    Scope *scope;
    /* Verified Cg IR module consumed by the Cg 2.0 lowering path. */
    const CgIRModule *source;
    const CgIRFunction *entry;
    GlslFunction *function;
    GlslMatrixHelper *matrixHelpers;
    GlslMatrixHelper *lastMatrixHelper;
    GlslMatrixSelectorHelper *selectorHelpers;
    GlslMatrixSelectorHelper *lastSelectorHelper;
    GlslInterfaceSource *interfaceSources;
    GlslGeometryFlat *geometryFlat;
    GlslGeometryFlat *lastGeometryFlat;
    GlslGeometryInputBinding *geometryInputs;
    GlslGeometryOutputBinding *geometryOutputs;
    SourceLoc statementLoc;
    int loopDepth;
} GlslLowerContext;

void GlslSetLoc(GlslLoc *target, const SourceLoc *source);
const char *GlslAllocateSymbolNameForSource(
    GlslLowerContext *context, const void *identity, const char *source,
    const SourceLoc *loc);
const char *GlslAllocateNameForSource(GlslLowerContext *context,
    const char *source, const SourceLoc *loc);
const char *GlslAllocateDistinctNameForSource(
    GlslLowerContext *context, const char *source, const SourceLoc *loc);
const char *GlslAllocateScopedSymbolNameForSource(
    GlslLowerContext *context, const void *nameSpace, const void *identity,
    const char *source, const SourceLoc *loc);
int GlslLowerError(GlslLowerContext *context);
void GlslRecordFailure(GlslLowerContext *context, const char *reason);
void GlslRecordFailureKind(GlslLowerContext *context,
                                  GlslErrorKind kind,
                                  const char *reason);
void GlslRecordFailureKindAt(GlslLowerContext *context,
                                    GlslErrorKind kind,
                                    const char *reason,
                                    const SourceLoc *loc);
const char *GlslUnsupportedExprReason(const expr *source);
char *GlslCopyText(GlslModule *module, const char *text);
int GlslTypesEqual(const GlslType *left, const GlslType *right);
int GlslIsSamplerType(const GlslType *type);
GlslDecl *GlslFindDecl(GlslLowerContext *context,
                              const void *identity);
GlslFunction *GlslFindFunction(GlslModule *module,
                                      const void *identity);
int GlslLowerType(GlslLowerContext *context, Type *source,
                         GlslType *target, const SourceLoc *loc);
void GlslInsertDecl(GlslDecl **list, GlslDecl *decl);
int GlslEnsureTypeAt(GlslLowerContext *context, Type *type,
                            const SourceLoc *loc);
int GlslEnsureSymbolTypes(GlslLowerContext *context, Symbol *symbol);
int GlslEnsureParameterTypes(GlslLowerContext *context,
                                    Symbol *formal);
int GlslSortStructs(GlslLowerContext *context);
int GlslCollectParameters(GlslLowerContext *context, Symbol *formal,
                                 int entry);
GlslBinding *GlslFindUniformBinding(GlslModule *module,
                                           const Symbol *symbol);
int GlslCollectUniforms(GlslLowerContext *context, Symbol *program);
int GlslValidateUniformLimit(GlslLowerContext *context);
int GlslValidateInterfaceLimits(GlslLowerContext *context);
int GlslAllocateTextureUnits(GlslLowerContext *context);
int GlslFiniteDefaultFloat(float value);
int GlslCollectDefaults(GlslLowerContext *context);
int GlslCollectLocals(GlslLowerContext *context, Symbol *symbol,
                             int entry);
void GlslInsertBinding(GlslBinding **list, GlslBinding *binding);
GlslDecl *GlslLowerInterface(GlslLowerContext *context,
                                    Symbol *member);
int GlslCollectCallsInStatements(GlslLowerContext *context,
                                        stmt *source);
int GlslAssignHelperNames(GlslLowerContext *context);
void GlslMarkForwardCalls(GlslLowerContext *context);
void GlslAppendExpr(GlslExpr **list, GlslExpr *expression);
GlslExpr *GlslLowerExprChain(GlslLowerContext *context, expr *source,
                                    opcode listOp);
GlslExpr *GlslNewLiteral(GlslLowerContext *context, GlslBase base,
    int intValue, float floatValue);
GlslExpr *GlslNewSwizzle(GlslLowerContext *context, GlslExpr *object,
    const GlslType *type, const char *mask);
int GlslMatrixSelectorCount(const expr *source);
GlslExpr *GlslMatrixSelectorComponent(GlslLowerContext *context,
    GlslExpr *matrix, const expr *selectorSource, int component);
GlslExpr *GlslLowerMatrixSwizzle(GlslLowerContext *context,
    expr *source, const GlslType *type);
int GlslMatrixNumericParameterType(const GlslType *type);
GlslMatrixSelectorHelper *GlslGetMatrixSelectorHelper(
    GlslLowerContext *context, GlslMatrixSelectorHelperKind kind,
    const GlslType *matrixType, const GlslType *valueType, int count,
    int mask);
GlslExpr *GlslLowerImpureMatrixConstructor(
    GlslLowerContext *context, GlslExpr *arguments, const GlslType *type);
GlslExpr *GlslLowerMatrixConstructor(GlslLowerContext *context,
    expr *source, const GlslType *type);
int GlslValidateTextureCall(GlslLowerContext *context,
    GlslBuiltin builtin, const GlslType *result, GlslExpr *arguments);
GlslBuiltin GlslIntrinsicBuiltin(CgIntrinsic intrinsic);
GlslExpr *GlslLowerExpr(GlslLowerContext *context, expr *source);
int GlslLowerStatementList(GlslLowerContext *context, stmt *source,
                                  GlslStmt **list);
int GlslLowerHelper(GlslLowerContext *context,
                           GlslFunction *function);
void GlslPrependMatrixHelpers(GlslLowerContext *context);
void GlslPrependMatrixSelectorHelpers(GlslLowerContext *context);
int GlslValidateEntryInterfaces(GlslLowerContext *context,
                                       Symbol *program);
int GlslIRInstallGeometryInfo(GlslLowerContext *context);
GlslInterpolation GlslInterpolationForType(const GlslType *type);
int GlslIRRegisterGeometryInput(GlslLowerContext *context,
    const CgIRDecl *param);
GlslStmt *GlslIRGeometryAssignments(
    GlslLowerContext *context, const CgIRGeometryValue *value);
GlslStmt *GlslIRGeometryFlatAssignments(
    GlslLowerContext *context, const CgIRGeometryValue *value);
GlslFlatReplay *GlslIRGeometryFlatReplay(
    GlslLowerContext *context);
int GlslLowerTypeAuto(GlslLowerContext *context, Type *source,
                             GlslType *target, const SourceLoc *loc);
int GlslEnsureTypeAuto(GlslLowerContext *context, Type *type,
                              const SourceLoc *loc);
int GlslIRScalarBase(GlslLowerContext *context, CgScalarKind kind,
                            GlslBase *base, const SourceLoc *loc);
int GlslIRIsSamplerValue(const Type *type);
int GlslIRType(GlslLowerContext *context, Type *source,
                      GlslType *target, const SourceLoc *loc);
int GlslIREnsureTypeAt(GlslLowerContext *context, Type *type,
                              const SourceLoc *loc);
int GlslIRSamplerPlacementCheck(GlslLowerContext *context,
                                       const CgIRDecl *decl);
const CgIRFunction *GlslIRFindIRFunction(const CgIRModule *source,
                                                const Symbol *symbol);
int GlslIRCollectCallsInStmt(GlslLowerContext *context,
                                    const CgIRStmt *stmt);
int GlslIRCollectGeometryFlatState(GlslLowerContext *context);
void GlslIRMarkForwardCallsInStmt(GlslLowerContext *context,
                                         GlslFunction *caller,
                                         const CgIRStmt *stmt);
int GlslIRValidateEntryInterfaces(GlslLowerContext *context,
                                         const CgIRFunction *entry);
int GlslIRNeedsMaterialization(const CgIRExpr *expr);
GlslExpr *GlslCloneExpr(GlslModule *module, const GlslExpr *expr);
GlslDecl *GlslIRAddLocal(GlslLowerContext *context, Symbol *symbol,
                                Type *type, const SourceLoc *loc);
int GlslIRLowerAggregateAssign(GlslLowerContext *context,
                                      const CgIRExpr *assign,
                                      GlslStmt **list);
int GlslIRSharedSelection(const CgIRExpr *expr,
                                 const CgIRExpr **objectOut,
                                 int *countOut, int *maskOut);
const CgIRDecl *GlslIRGeometryEntryParameter(
    const GlslLowerContext *context, const Symbol *symbol);
GlslExpr *GlslIRGeometryBuiltinElement(
    GlslLowerContext *context, const CgIRDecl *param,
    GlslExpr *indexExpr, const GlslType *resultType,
    const SourceLoc *loc);
GlslExpr *GlslIRGeometryBuiltinArray(
    GlslLowerContext *context, const CgIRExpr *expr,
    const GlslType *type);
GlslExpr *GlslIRLowerExpr(GlslLowerContext *context,
                                 const CgIRExpr *expr);
const char *GlslIRDeclNameText(const CgIRDecl *decl);
GlslExpr *GlslIRMatrixElement(GlslLowerContext *context,
                                     GlslExpr *matrix, int row,
                                     int column);
int GlslIRTryGroupWrite(GlslLowerContext *context,
                               const CgIRStmt *head,
                               const CgIRStmt **nextOut, GlslStmt **list);
int GlslIRLocalDeclaration(GlslLowerContext *context,
                                  const CgIRStmt *stmt,
                                  const CgIRStmt **nextOut, GlslStmt **out);
int GlslIRBlockBody(GlslLowerContext *context,
                           const CgIRStmt *stmt, GlslStmt **out);
int GlslIRLowerFunctionBody(GlslLowerContext *context,
                                   const CgIRFunction *irFunction);
int GlslIRCollectUniforms(GlslLowerContext *context,
                                 const CgIRFunction *entry);
int GlslIREnsureEntryLocals(GlslLowerContext *context,
                                   const CgIRFunction *entry);

#endif
```

The moved block includes GLSL_MATRIX_MAX_ARGUMENTS and the matrix/interface/geometry/context types. Keep GlslDefaultValue and GlslDefaultBaseClass local to declaration lowering.

- [x] **Step 2: Establish linkage before extraction.**
For precisely the declarations above, remove `static` from definitions and any
retained matching forward declarations, even while their bodies still reside in
the facade. Remove redundant cross-module forwards now supplied by the header.
Preserve needed static forwards until their owning module is moved.
Do not rename GLSL helpers.

This preparation allows calls in either direction during all intermediate
extractions. The final API is based on the caller manifest in Appendix A.

Replace the existing project header includes in the facade with:
```c
#include "glsl_lower_internal.h"
```
Preserve the existing standard-library includes and their order. Remove only the
private type block moved to the header.

- [x] **Step 3: Wire the initial one-file canonical source list.**
Before `add_subdirectory(tests)` in root CMake:
```cmake
set(CGC_GLSL_LOWER_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/glsl_lower.c
)
```
Replace the direct `glsl_lower.c` source in `cgc` with:
```cmake
    ${CGC_GLSL_LOWER_SOURCES}
```
In `glsl_lower_ir_unit`, replace `${PROJECT_SOURCE_DIR}/glsl_lower.c` with `${CGC_GLSL_LOWER_SOURCES}`.

- [x] **Step 4: Configure/build/test the candidate.**
Run from `.worktrees/glsl-lower-modularization`.
```powershell
cmake -S . -B build-cg20-glsl-candidate -A x64 -DBUILD_TESTING=ON -DCGC_REQUIRE_FXC=ON
if ($LASTEXITCODE -ne 0) { throw 'configure failed' }
cmake --build build-cg20-glsl-candidate --config Debug --target cgc glsl_lower_ir_unit
if ($LASTEXITCODE -ne 0) { throw 'Debug failed' }
cmake --build build-cg20-glsl-candidate --config Release --target cgc glsl_lower_ir_unit
if ($LASTEXITCODE -ne 0) { throw 'Release failed' }
ctest --test-dir build-cg20-glsl-candidate -C Release -R "(glsl|geometry|^generic_)" --output-on-failure
if ($LASTEXITCODE -ne 0) { throw 'backend tests failed' }
git diff --check
```
Expected: build/link success and all selected tests pass, with nonzero selected
inventory. Audit that every promoted name is unique across repository definitions.

- [x] **Step 5: Commit only the preparation files.**
```powershell
git add glsl_lower.c glsl_lower_internal.h CMakeLists.txt tests/CMakeLists.txt
git commit -m "Prepare GLSL lowering module interfaces"
```

### Task 3: Extract support lowering

**Files:** Create `glsl_lower_support.c`; modify `glsl_lower.c` and `CMakeLists.txt`.

- [x] **Step 1: Move this exact ordered inventory.**
Move complete definitions and attached comments from the original source, preserving
relative order. The line numbers refer to the pinned planning baseline and are
navigation hints; function names are authoritative.

```text
GlslSetLoc — original line 140
GlslAllocateSymbolNameForSource — original line 148
GlslAllocateNameForSource — original line 160
GlslAllocateDistinctNameForSource — original line 171
GlslAllocateScopedSymbolNameForSource — original line 182
GlslLowerError — original line 195
GlslRecordFailure — original line 201
GlslRecordFailureKind — original line 214
GlslRecordFailureKindAt — original line 224
GlslUnsupportedExprReason — original line 241
GlslCopyText — original line 262
GlslTypesEqual — original line 274
GlslIsSamplerType — original line 293
GlslFindDeclList — original line 308
GlslFindDecl — original line 319
GlslFindFunction — original line 361
GlslAppendExpr — original line 2406
```

Create the module with its complete original license and this include block:
```c
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <float.h>
#include <limits.h>

#include "glsl_lower_internal.h"
```
Keep the private API linkage established in Task 2. Move any required static
forward declarations with their owning module.



- [x] **Step 2: Append the new file to the canonical list.**
The list after this task must be exactly:
```cmake
set(CGC_GLSL_LOWER_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/glsl_lower.c
    ${CMAKE_CURRENT_SOURCE_DIR}/glsl_lower_support.c
)
```

- [x] **Step 3: Audit extraction before building.**
Compare the moved bodies against the pinned baseline and the prior commit. Allow
only the recorded linkage changes.
Require each listed definition exactly once in its new owner, zero remaining
copies, and no changes to remaining bodies. Preserve all file/function-static
state and comments. A link failure is a missing dependency to reconcile against
Appendix A, not permission to change behavior.

- [x] **Step 4: Build both configurations and run backend coverage.**
```powershell
cmake -S . -B build-cg20-glsl-candidate -A x64 -DBUILD_TESTING=ON -DCGC_REQUIRE_FXC=ON
if ($LASTEXITCODE -ne 0) { throw 'configure failed' }
cmake --build build-cg20-glsl-candidate --config Debug --target cgc glsl_lower_ir_unit
if ($LASTEXITCODE -ne 0) { throw 'Debug failed' }
cmake --build build-cg20-glsl-candidate --config Release --target cgc glsl_lower_ir_unit
if ($LASTEXITCODE -ne 0) { throw 'Release failed' }
ctest --test-dir build-cg20-glsl-candidate -C Release -R "(glsl|geometry|^generic_)" --output-on-failure
if ($LASTEXITCODE -ne 0) { throw 'backend tests failed' }
git diff --check
```
Expected: all selected tests pass. Compare generated backend outputs with the
pristine baseline using Task 14's complete-set procedure. Run only
one configuration's ARB tests at a time because assembly artifact paths are shared.

- [x] **Step 5: Commit this extraction.**
```powershell
git add glsl_lower.c glsl_lower_support.c CMakeLists.txt
git commit -m "Extract GLSL support lowering"
```

### Task 4: Extract decl lowering

**Files:** Create `glsl_lower_decl.c`; modify `glsl_lower.c` and `CMakeLists.txt`.

- [x] **Step 1: Move this exact ordered inventory.**
Move complete definitions and attached comments from the original source, preserving
relative order. The line numbers refer to the pinned planning baseline and are
navigation hints; function names are authoritative.

```text
GlslCanonicalStructType — original line 375
GlslFindTag — original line 393
GlslFindStruct — original line 411
GlslLowerType — original line 426
GlslDeclComesBefore — original line 576
GlslInsertDecl — original line 585
GlslNewSourceDecl — original line 596
GlslCollectMembers — original line 625
GlslEnsureTypeAt — original line 645
GlslEnsureSymbolTypes — original line 660
GlslEnsureParameterTypes — original line 685
GlslEnsureType — original line 703
GlslTypeUsesStruct — original line 773
GlslStructReady — original line 781
GlslSortStructs — original line 796
GlslCollectParameters — original line 861
GlslAppendDefaultValue — original line 1406
GlslDefaultBaseClassOf — original line 1435
GlslFiniteDefaultFloat — original line 1453
GlslAppendTypedDefaultValue — original line 1458
GlslConvertDefaultValue — original line 1488
GlslFlattenDefaultExpr — original line 1559
GlslDefaultTargetBase — original line 1667
GlslStoreDefaultLeaf — original line 1683
GlslStoreDefaultType — original line 1715
GlslCollectDefaults — original line 1759
GlslCollectLocals — original line 1842
GlslLowerTypeAuto — original line 4881
GlslEnsureTypeAuto — original line 4889
GlslIRScalarBase — original line 4903
GlslIRIsSamplerValue — original line 4933
GlslIRFindStruct — original line 4955
GlslIRRegisterStruct — original line 4984
GlslIRType — original line 5033
GlslIREnsureType — original line 5188
GlslIREnsureTypeAt — original line 5268
GlslIRSamplerPlacementCheck — original line 5291
GlslIRNewSourceDecl — original line 5314
GlslIRLocalDeclaration — original line 8799
GlslIREnsureEntryLocals — original line 9045
```

Create the module with its complete original license and this include block:
```c
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <float.h>
#include <limits.h>

#include "glsl_lower_internal.h"
```
Keep the private API linkage established in Task 2. Move any required static
forward declarations with their owning module. Move the exact GlslDefaultValue typedef and GlslDefaultBaseClass enum immediately before GlslDefaultBaseClassOf.



- [x] **Step 2: Append the new file to the canonical list.**
The list after this task must be exactly:
```cmake
set(CGC_GLSL_LOWER_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/glsl_lower.c
    ${CMAKE_CURRENT_SOURCE_DIR}/glsl_lower_support.c
    ${CMAKE_CURRENT_SOURCE_DIR}/glsl_lower_decl.c
)
```

- [x] **Step 3: Audit extraction before building.**
Compare the moved bodies against the pinned baseline and the prior commit. Allow
only the recorded linkage changes.
Require each listed definition exactly once in its new owner, zero remaining
copies, and no changes to remaining bodies. Preserve all file/function-static
state and comments. A link failure is a missing dependency to reconcile against
Appendix A, not permission to change behavior.

- [x] **Step 4: Build both configurations and run backend coverage.**
```powershell
cmake -S . -B build-cg20-glsl-candidate -A x64 -DBUILD_TESTING=ON -DCGC_REQUIRE_FXC=ON
if ($LASTEXITCODE -ne 0) { throw 'configure failed' }
cmake --build build-cg20-glsl-candidate --config Debug --target cgc glsl_lower_ir_unit
if ($LASTEXITCODE -ne 0) { throw 'Debug failed' }
cmake --build build-cg20-glsl-candidate --config Release --target cgc glsl_lower_ir_unit
if ($LASTEXITCODE -ne 0) { throw 'Release failed' }
ctest --test-dir build-cg20-glsl-candidate -C Release -R "(glsl|geometry|^generic_)" --output-on-failure
if ($LASTEXITCODE -ne 0) { throw 'backend tests failed' }
git diff --check
```
Expected: all selected tests pass. Compare generated backend outputs with the
pristine baseline using Task 14's complete-set procedure. Run only
one configuration's ARB tests at a time because assembly artifact paths are shared.

- [x] **Step 5: Commit this extraction.**
```powershell
git add glsl_lower.c glsl_lower_decl.c CMakeLists.txt
git commit -m "Extract GLSL decl lowering"
```

### Task 5: Extract interface lowering

**Files:** Create `glsl_lower_interface.c`; modify `glsl_lower.c` and `CMakeLists.txt`.

- [x] **Step 1: Move this exact ordered inventory.**
Move complete definitions and attached comments from the original source, preserving
relative order. The line numbers refer to the pinned planning baseline and are
navigation hints; function names are authoritative.

```text
GlslSamplerDeclComesBefore — original line 834
GlslInsertSamplerDecl — original line 846
GlslFindUniformBinding — original line 894
GlslCollectUniformSymbol — original line 910
GlslCollectUniformList — original line 965
GlslCollectUniformsInExpr — original line 975
GlslCollectUniformsInStatements — original line 1002
GlslCollectUniforms — original line 1060
GlslValidateUniformLimit — original line 1083
GlslRecordResourceLimit — original line 1132
GlslRecordResourceLimitAt — original line 1145
GlslValidateGeometryInterfaceLimits — original line 1159
GlslValidateInterfaceLimits — original line 1247
GlslAllocateTextureUnits — original line 1352
GlslBindingComesBefore — original line 1866
GlslInsertBinding — original line 1882
GlslHasInterfaceBinding — original line 1893
GlslReservedInterfaceName — original line 1909
GlslLowerInterface — original line 1927
GlslValidateInterfaceSource — original line 4255
GlslValidateInterfaceType — original line 4311
GlslValidateEntryInterfaces — original line 4330
GlslIRCollectUniformsInExpr — original line 5359
GlslIRCollectUniformsInStmt — original line 5441
GlslIRValidateEntryInterfaces — original line 6070
GlslIRCollectUniforms — original line 9013
```

Create the module with its complete original license and this include block:
```c
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <float.h>
#include <limits.h>

#include "glsl_lower_internal.h"
```
Keep the private API linkage established in Task 2. Move any required static
forward declarations with their owning module.



- [x] **Step 2: Append the new file to the canonical list.**
The list after this task must be exactly:
```cmake
set(CGC_GLSL_LOWER_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/glsl_lower.c
    ${CMAKE_CURRENT_SOURCE_DIR}/glsl_lower_support.c
    ${CMAKE_CURRENT_SOURCE_DIR}/glsl_lower_decl.c
    ${CMAKE_CURRENT_SOURCE_DIR}/glsl_lower_interface.c
)
```

- [x] **Step 3: Audit extraction before building.**
Compare the moved bodies against the pinned baseline and the prior commit. Allow
only the recorded linkage changes.
Require each listed definition exactly once in its new owner, zero remaining
copies, and no changes to remaining bodies. Preserve all file/function-static
state and comments. A link failure is a missing dependency to reconcile against
Appendix A, not permission to change behavior.

- [x] **Step 4: Build both configurations and run backend coverage.**
```powershell
cmake -S . -B build-cg20-glsl-candidate -A x64 -DBUILD_TESTING=ON -DCGC_REQUIRE_FXC=ON
if ($LASTEXITCODE -ne 0) { throw 'configure failed' }
cmake --build build-cg20-glsl-candidate --config Debug --target cgc glsl_lower_ir_unit
if ($LASTEXITCODE -ne 0) { throw 'Debug failed' }
cmake --build build-cg20-glsl-candidate --config Release --target cgc glsl_lower_ir_unit
if ($LASTEXITCODE -ne 0) { throw 'Release failed' }
ctest --test-dir build-cg20-glsl-candidate -C Release -R "(glsl|geometry|^generic_)" --output-on-failure
if ($LASTEXITCODE -ne 0) { throw 'backend tests failed' }
git diff --check
```
Expected: all selected tests pass. Compare generated backend outputs with the
pristine baseline using Task 14's complete-set procedure. Run only
one configuration's ARB tests at a time because assembly artifact paths are shared.

- [x] **Step 5: Commit this extraction.**
```powershell
git add glsl_lower.c glsl_lower_interface.c CMakeLists.txt
git commit -m "Extract GLSL interface lowering"
```

### Task 6: Extract aggregate lowering

**Files:** Create `glsl_lower_aggregate.c`; modify `glsl_lower.c` and `CMakeLists.txt`.

- [x] **Step 1: Move this exact ordered inventory.**
Move complete definitions and attached comments from the original source, preserving
relative order. The line numbers refer to the pinned planning baseline and are
navigation hints; function names are authoritative.

```text
GlslNewIndexLiteral — original line 2601
GlslMatrixComponent — original line 2620
GlslMatrixSelectorCount — original line 2641
GlslMatrixMaskComponent — original line 2651
GlslMatrixSelectorComponent — original line 2664
GlslLowerMatrixSwizzle — original line 2678
GlslMatrixNumericParameterType — original line 2734
GlslFindMatrixHelper — original line 2745
GlslMatrixHelperName — original line 2766
GlslNewParameterComponent — original line 2798
GlslFindMatrixSelectorHelper — original line 2820
GlslMatrixSelectorHelperName — original line 2838
GlslCreateMatrixSelectorHelper — original line 2869
GlslGetMatrixSelectorHelper — original line 2987
GlslCreateMatrixHelper — original line 3007
GlslLowerImpureMatrixConstructor — original line 3091
GlslLowerMatrixConstructor — original line 3136
GlslPrependMatrixHelpers — original line 4209
GlslPrependMatrixSelectorHelpers — original line 4232
GlslIRNeedsMaterialization — original line 6197
GlslIRContainsArray — original line 6250
GlslIRPostNormalizeNeedsMaterialization — original line 6282
GlslCloneExpr — original line 6341
GlslIRDeclRef — original line 6435
GlslIRAppendMemberStep — original line 6446
GlslIRAppendIndexStep — original line 6464
GlslIRAppendDynamicIndex — original line 6485
GlslIRChooseFlattenName — original line 6509
GlslIRNewTempSymbol — original line 6540
GlslIRAddLocal — original line 6562
GlslIRNewIndexTemp — original line 6605
GlslIREmitAggregateLeaves — original line 6655
GlslIRLowerMaterialized — original line 6717
GlslIRNewAggregateTemp — original line 6792
GlslIRLowerAggregateAssign — original line 6849
GlslIRDeclNameText — original line 8103
GlslIRMatrixStoreShape — original line 8114
GlslIRStoreTargetMarked — original line 8159
GlslIRIsTempMove — original line 8170
GlslIRMatrixElement — original line 8184
GlslIRTryGroupWrite — original line 8221
```

Create the module with its complete original license and this include block:
```c
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <float.h>
#include <limits.h>

#include "glsl_lower_internal.h"
```
Keep the private API linkage established in Task 2. Move any required static
forward declarations with their owning module.

Preserve materialization, matrix-helper append order, selector identities and grouped-write producer-marker checks exactly.

- [x] **Step 2: Append the new file to the canonical list.**
The list after this task must be exactly:
```cmake
set(CGC_GLSL_LOWER_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/glsl_lower.c
    ${CMAKE_CURRENT_SOURCE_DIR}/glsl_lower_support.c
    ${CMAKE_CURRENT_SOURCE_DIR}/glsl_lower_decl.c
    ${CMAKE_CURRENT_SOURCE_DIR}/glsl_lower_interface.c
    ${CMAKE_CURRENT_SOURCE_DIR}/glsl_lower_aggregate.c
)
```

- [x] **Step 3: Audit extraction before building.**
Compare the moved bodies against the pinned baseline and the prior commit. Allow
only the recorded linkage changes.
Require each listed definition exactly once in its new owner, zero remaining
copies, and no changes to remaining bodies. Preserve all file/function-static
state and comments. A link failure is a missing dependency to reconcile against
Appendix A, not permission to change behavior.

- [x] **Step 4: Build both configurations and run backend coverage.**
```powershell
cmake -S . -B build-cg20-glsl-candidate -A x64 -DBUILD_TESTING=ON -DCGC_REQUIRE_FXC=ON
if ($LASTEXITCODE -ne 0) { throw 'configure failed' }
cmake --build build-cg20-glsl-candidate --config Debug --target cgc glsl_lower_ir_unit
if ($LASTEXITCODE -ne 0) { throw 'Debug failed' }
cmake --build build-cg20-glsl-candidate --config Release --target cgc glsl_lower_ir_unit
if ($LASTEXITCODE -ne 0) { throw 'Release failed' }
ctest --test-dir build-cg20-glsl-candidate -C Release -R "(glsl|geometry|^generic_)" --output-on-failure
if ($LASTEXITCODE -ne 0) { throw 'backend tests failed' }
git diff --check
```
Expected: all selected tests pass. Compare generated backend outputs with the
pristine baseline using Task 14's complete-set procedure. Run only
one configuration's ARB tests at a time because assembly artifact paths are shared.

- [x] **Step 5: Commit this extraction.**
```powershell
git add glsl_lower.c glsl_lower_aggregate.c CMakeLists.txt
git commit -m "Extract GLSL aggregate lowering"
```

### Task 7: Extract legacy expr lowering

**Files:** Create `glsl_lower_legacy_expr.c`; modify `glsl_lower.c` and `CMakeLists.txt`.

- [x] **Step 1: Move this exact ordered inventory.**
Move complete definitions and attached comments from the original source, preserving
relative order. The line numbers refer to the pinned planning baseline and are
navigation hints; function names are authoritative.

```text
GlslLowerExprChain — original line 2421
GlslLowerTextureArguments — original line 2441
GlslNewLiteral — original line 2487
GlslLowerConstant — original line 2513
GlslNewSwizzle — original line 2553
GlslLowerSwizzle — original line 2568
GlslUnaryOperator — original line 3230
GlslBinaryOperator — original line 3243
GlslVectorComparisonName — original line 3270
GlslValidateTextureCall — original line 3283
GlslIntrinsicBuiltin — original line 3401
GlslLowerCall — original line 3447
GlslLowerVectorComparison — original line 3566
GlslLowerMaskedAssignment — original line 3606
GlslLowerComponent — original line 3644
GlslLowerConditional — original line 3663
GlslLowerExpr — original line 3719
```

Create the module with its complete original license and this include block:
```c
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <float.h>
#include <limits.h>

#include "glsl_lower_internal.h"
```
Keep the private API linkage established in Task 2. Move any required static
forward declarations with their owning module.



- [x] **Step 2: Append the new file to the canonical list.**
The list after this task must be exactly:
```cmake
set(CGC_GLSL_LOWER_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/glsl_lower.c
    ${CMAKE_CURRENT_SOURCE_DIR}/glsl_lower_support.c
    ${CMAKE_CURRENT_SOURCE_DIR}/glsl_lower_decl.c
    ${CMAKE_CURRENT_SOURCE_DIR}/glsl_lower_interface.c
    ${CMAKE_CURRENT_SOURCE_DIR}/glsl_lower_aggregate.c
    ${CMAKE_CURRENT_SOURCE_DIR}/glsl_lower_legacy_expr.c
)
```

- [x] **Step 3: Audit extraction before building.**
Compare the moved bodies against the pinned baseline and the prior commit. Allow
only the recorded linkage changes.
Require each listed definition exactly once in its new owner, zero remaining
copies, and no changes to remaining bodies. Preserve all file/function-static
state and comments. A link failure is a missing dependency to reconcile against
Appendix A, not permission to change behavior.

- [x] **Step 4: Build both configurations and run backend coverage.**
```powershell
cmake -S . -B build-cg20-glsl-candidate -A x64 -DBUILD_TESTING=ON -DCGC_REQUIRE_FXC=ON
if ($LASTEXITCODE -ne 0) { throw 'configure failed' }
cmake --build build-cg20-glsl-candidate --config Debug --target cgc glsl_lower_ir_unit
if ($LASTEXITCODE -ne 0) { throw 'Debug failed' }
cmake --build build-cg20-glsl-candidate --config Release --target cgc glsl_lower_ir_unit
if ($LASTEXITCODE -ne 0) { throw 'Release failed' }
ctest --test-dir build-cg20-glsl-candidate -C Release -R "(glsl|geometry|^generic_)" --output-on-failure
if ($LASTEXITCODE -ne 0) { throw 'backend tests failed' }
git diff --check
```
Expected: all selected tests pass. Compare generated backend outputs with the
pristine baseline using Task 14's complete-set procedure. Run only
one configuration's ARB tests at a time because assembly artifact paths are shared.

- [x] **Step 5: Commit this extraction.**
```powershell
git add glsl_lower.c glsl_lower_legacy_expr.c CMakeLists.txt
git commit -m "Extract GLSL legacy expr lowering"
```

### Task 8: Extract legacy stmt lowering

**Files:** Create `glsl_lower_legacy_stmt.c`; modify `glsl_lower.c` and `CMakeLists.txt`.

- [x] **Step 1: Move this exact ordered inventory.**
Move complete definitions and attached comments from the original source, preserving
relative order. The line numbers refer to the pinned planning baseline and are
navigation hints; function names are authoritative.

```text
GlslLowerMatrixAssignment — original line 3894
GlslLowerBranch — original line 3978
GlslLowerForPart — original line 3989
GlslLowerStatementList — original line 4012
```

Create the module with its complete original license and this include block:
```c
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <float.h>
#include <limits.h>

#include "glsl_lower_internal.h"
```
Keep the private API linkage established in Task 2. Move any required static
forward declarations with their owning module.



- [x] **Step 2: Append the new file to the canonical list.**
The list after this task must be exactly:
```cmake
set(CGC_GLSL_LOWER_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/glsl_lower.c
    ${CMAKE_CURRENT_SOURCE_DIR}/glsl_lower_support.c
    ${CMAKE_CURRENT_SOURCE_DIR}/glsl_lower_decl.c
    ${CMAKE_CURRENT_SOURCE_DIR}/glsl_lower_interface.c
    ${CMAKE_CURRENT_SOURCE_DIR}/glsl_lower_aggregate.c
    ${CMAKE_CURRENT_SOURCE_DIR}/glsl_lower_legacy_expr.c
    ${CMAKE_CURRENT_SOURCE_DIR}/glsl_lower_legacy_stmt.c
)
```

- [x] **Step 3: Audit extraction before building.**
Compare the moved bodies against the pinned baseline and the prior commit. Allow
only the recorded linkage changes.
Require each listed definition exactly once in its new owner, zero remaining
copies, and no changes to remaining bodies. Preserve all file/function-static
state and comments. A link failure is a missing dependency to reconcile against
Appendix A, not permission to change behavior.

- [x] **Step 4: Build both configurations and run backend coverage.**
```powershell
cmake -S . -B build-cg20-glsl-candidate -A x64 -DBUILD_TESTING=ON -DCGC_REQUIRE_FXC=ON
if ($LASTEXITCODE -ne 0) { throw 'configure failed' }
cmake --build build-cg20-glsl-candidate --config Debug --target cgc glsl_lower_ir_unit
if ($LASTEXITCODE -ne 0) { throw 'Debug failed' }
cmake --build build-cg20-glsl-candidate --config Release --target cgc glsl_lower_ir_unit
if ($LASTEXITCODE -ne 0) { throw 'Release failed' }
ctest --test-dir build-cg20-glsl-candidate -C Release -R "(glsl|geometry|^generic_)" --output-on-failure
if ($LASTEXITCODE -ne 0) { throw 'backend tests failed' }
git diff --check
```
Expected: all selected tests pass. Compare generated backend outputs with the
pristine baseline using Task 14's complete-set procedure. Run only
one configuration's ARB tests at a time because assembly artifact paths are shared.

- [x] **Step 5: Commit this extraction.**
```powershell
git add glsl_lower.c glsl_lower_legacy_stmt.c CMakeLists.txt
git commit -m "Extract GLSL legacy stmt lowering"
```

### Task 9: Extract ir expr lowering

**Files:** Create `glsl_lower_ir_expr.c`; modify `glsl_lower.c` and `CMakeLists.txt`.

- [x] **Step 1: Move this exact ordered inventory.**
Move complete definitions and attached comments from the original source, preserving
relative order. The line numbers refer to the pinned planning baseline and are
navigation hints; function names are authoritative.

```text
GlslIRUnaryOperator — original line 6898
GlslIRBinaryOperator — original line 6908
GlslIRComparisonName — original line 6927
GlslIRUnsupportedReason — original line 6945
GlslIRCompoundOperator — original line 6964
GlslIRSharedSelection — original line 6982
GlslIRConstantComponent — original line 7043
GlslIRLowerConstant — original line 7062
GlslIRLowerSwizzle — original line 7072
GlslIRLowerMatrixConstructor — original line 7110
GlslIRLowerExprList — original line 7196
GlslIRLowerTextureArguments — original line 7218
GlslIRLowerCallCore — original line 7263
GlslIRLowerVectorComparison — original line 7375
GlslIRLowerComponent — original line 7417
GlslIRLowerConditional — original line 7438
GlslIRLowerIncrement — original line 7503
GlslIRLowerExpr — original line 7649
```

Create the module with its complete original license and this include block:
```c
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <float.h>
#include <limits.h>

#include "glsl_lower_internal.h"
```
Keep the private API linkage established in Task 2. Move any required static
forward declarations with their owning module.
Preserve the explanatory Cg IR lowering comment with this module; it documents normalized shape assumptions.


- [x] **Step 2: Append the new file to the canonical list.**
The list after this task must be exactly:
```cmake
set(CGC_GLSL_LOWER_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/glsl_lower.c
    ${CMAKE_CURRENT_SOURCE_DIR}/glsl_lower_support.c
    ${CMAKE_CURRENT_SOURCE_DIR}/glsl_lower_decl.c
    ${CMAKE_CURRENT_SOURCE_DIR}/glsl_lower_interface.c
    ${CMAKE_CURRENT_SOURCE_DIR}/glsl_lower_aggregate.c
    ${CMAKE_CURRENT_SOURCE_DIR}/glsl_lower_legacy_expr.c
    ${CMAKE_CURRENT_SOURCE_DIR}/glsl_lower_legacy_stmt.c
    ${CMAKE_CURRENT_SOURCE_DIR}/glsl_lower_ir_expr.c
)
```

- [x] **Step 3: Audit extraction before building.**
Compare the moved bodies against the pinned baseline and the prior commit. Allow
only the recorded linkage changes.
Require each listed definition exactly once in its new owner, zero remaining
copies, and no changes to remaining bodies. Preserve all file/function-static
state and comments. A link failure is a missing dependency to reconcile against
Appendix A, not permission to change behavior.

- [x] **Step 4: Build both configurations and run backend coverage.**
```powershell
cmake -S . -B build-cg20-glsl-candidate -A x64 -DBUILD_TESTING=ON -DCGC_REQUIRE_FXC=ON
if ($LASTEXITCODE -ne 0) { throw 'configure failed' }
cmake --build build-cg20-glsl-candidate --config Debug --target cgc glsl_lower_ir_unit
if ($LASTEXITCODE -ne 0) { throw 'Debug failed' }
cmake --build build-cg20-glsl-candidate --config Release --target cgc glsl_lower_ir_unit
if ($LASTEXITCODE -ne 0) { throw 'Release failed' }
ctest --test-dir build-cg20-glsl-candidate -C Release -R "(glsl|geometry|^generic_)" --output-on-failure
if ($LASTEXITCODE -ne 0) { throw 'backend tests failed' }
git diff --check
```
Expected: all selected tests pass. Compare generated backend outputs with the
pristine baseline using Task 14's complete-set procedure. Run only
one configuration's ARB tests at a time because assembly artifact paths are shared.

- [x] **Step 5: Commit this extraction.**
```powershell
git add glsl_lower.c glsl_lower_ir_expr.c CMakeLists.txt
git commit -m "Extract GLSL ir expr lowering"
```

### Task 10: Extract ir stmt lowering

**Files:** Create `glsl_lower_ir_stmt.c`; modify `glsl_lower.c` and `CMakeLists.txt`.

- [x] **Step 1: Move this exact ordered inventory.**
Move complete definitions and attached comments from the original source, preserving
relative order. The line numbers refer to the pinned planning baseline and are
navigation hints; function names are authoritative.

```text
GlslIRLowerStatement — original line 8553
GlslIRBlockBody — original line 8835
GlslIRBranch — original line 8867
GlslIRForPart — original line 8882
```

Create the module with its complete original license and this include block:
```c
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <float.h>
#include <limits.h>

#include "glsl_lower_internal.h"
```
Keep the private API linkage established in Task 2. Move any required static
forward declarations with their owning module.



- [x] **Step 2: Append the new file to the canonical list.**
The list after this task must be exactly:
```cmake
set(CGC_GLSL_LOWER_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/glsl_lower.c
    ${CMAKE_CURRENT_SOURCE_DIR}/glsl_lower_support.c
    ${CMAKE_CURRENT_SOURCE_DIR}/glsl_lower_decl.c
    ${CMAKE_CURRENT_SOURCE_DIR}/glsl_lower_interface.c
    ${CMAKE_CURRENT_SOURCE_DIR}/glsl_lower_aggregate.c
    ${CMAKE_CURRENT_SOURCE_DIR}/glsl_lower_legacy_expr.c
    ${CMAKE_CURRENT_SOURCE_DIR}/glsl_lower_legacy_stmt.c
    ${CMAKE_CURRENT_SOURCE_DIR}/glsl_lower_ir_expr.c
    ${CMAKE_CURRENT_SOURCE_DIR}/glsl_lower_ir_stmt.c
)
```

- [x] **Step 3: Audit extraction before building.**
Compare the moved bodies against the pinned baseline and the prior commit. Allow
only the recorded linkage changes.
Require each listed definition exactly once in its new owner, zero remaining
copies, and no changes to remaining bodies. Preserve all file/function-static
state and comments. A link failure is a missing dependency to reconcile against
Appendix A, not permission to change behavior.

- [x] **Step 4: Build both configurations and run backend coverage.**
```powershell
cmake -S . -B build-cg20-glsl-candidate -A x64 -DBUILD_TESTING=ON -DCGC_REQUIRE_FXC=ON
if ($LASTEXITCODE -ne 0) { throw 'configure failed' }
cmake --build build-cg20-glsl-candidate --config Debug --target cgc glsl_lower_ir_unit
if ($LASTEXITCODE -ne 0) { throw 'Debug failed' }
cmake --build build-cg20-glsl-candidate --config Release --target cgc glsl_lower_ir_unit
if ($LASTEXITCODE -ne 0) { throw 'Release failed' }
ctest --test-dir build-cg20-glsl-candidate -C Release -R "(glsl|geometry|^generic_)" --output-on-failure
if ($LASTEXITCODE -ne 0) { throw 'backend tests failed' }
git diff --check
```
Expected: all selected tests pass. Compare generated backend outputs with the
pristine baseline using Task 14's complete-set procedure. Run only
one configuration's ARB tests at a time because assembly artifact paths are shared.

- [x] **Step 5: Commit this extraction.**
```powershell
git add glsl_lower.c glsl_lower_ir_stmt.c CMakeLists.txt
git commit -m "Extract GLSL ir stmt lowering"
```

### Task 11: Extract geometry lowering

**Files:** Create `glsl_lower_geometry.c`; modify `glsl_lower.c` and `CMakeLists.txt`.

- [x] **Step 1: Move this exact ordered inventory.**
Move complete definitions and attached comments from the original source, preserving
relative order. The line numbers refer to the pinned planning baseline and are
navigation hints; function names are authoritative.

```text
GlslIRInstallGeometryInfo — original line 4487
GlslInterpolationForType — original line 4549
GlslIRRegisterGeometryInput — original line 4561
GlslIRGeometryOutputDecl — original line 4658
GlslIRGeometryAssignments — original line 4726
GlslIRFindGeometryFlat — original line 4766
GlslIRAssignDecl — original line 4778
GlslIRGeometryFlatAssignments — original line 4799
GlslIRGeometryFlatReplay — original line 4837
GlslIRRegisterGeometryFlatValue — original line 5659
GlslIRCollectGeometryFlatInStmt — original line 5714
GlslIRCollectGeometryFlatState — original line 5755
GlslIRGeometryEntryParameter — original line 7540
GlslIRGeometryBuiltinElement — original line 7559
GlslIRGeometryBuiltinArray — original line 7608
```

Create the module with its complete original license and this include block:
```c
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <float.h>
#include <limits.h>

#include "glsl_lower_internal.h"
```
Keep the private API linkage established in Task 2. Move any required static
forward declarations with their owning module.



- [x] **Step 2: Append the new file to the canonical list.**
The list after this task must be exactly:
```cmake
set(CGC_GLSL_LOWER_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/glsl_lower.c
    ${CMAKE_CURRENT_SOURCE_DIR}/glsl_lower_support.c
    ${CMAKE_CURRENT_SOURCE_DIR}/glsl_lower_decl.c
    ${CMAKE_CURRENT_SOURCE_DIR}/glsl_lower_interface.c
    ${CMAKE_CURRENT_SOURCE_DIR}/glsl_lower_aggregate.c
    ${CMAKE_CURRENT_SOURCE_DIR}/glsl_lower_legacy_expr.c
    ${CMAKE_CURRENT_SOURCE_DIR}/glsl_lower_legacy_stmt.c
    ${CMAKE_CURRENT_SOURCE_DIR}/glsl_lower_ir_expr.c
    ${CMAKE_CURRENT_SOURCE_DIR}/glsl_lower_ir_stmt.c
    ${CMAKE_CURRENT_SOURCE_DIR}/glsl_lower_geometry.c
)
```

- [x] **Step 3: Audit extraction before building.**
Compare the moved bodies against the pinned baseline and the prior commit. Allow
only the recorded linkage changes.
Require each listed definition exactly once in its new owner, zero remaining
copies, and no changes to remaining bodies. Preserve all file/function-static
state and comments. A link failure is a missing dependency to reconcile against
Appendix A, not permission to change behavior.

- [x] **Step 4: Build both configurations and run backend coverage.**
```powershell
cmake -S . -B build-cg20-glsl-candidate -A x64 -DBUILD_TESTING=ON -DCGC_REQUIRE_FXC=ON
if ($LASTEXITCODE -ne 0) { throw 'configure failed' }
cmake --build build-cg20-glsl-candidate --config Debug --target cgc glsl_lower_ir_unit
if ($LASTEXITCODE -ne 0) { throw 'Debug failed' }
cmake --build build-cg20-glsl-candidate --config Release --target cgc glsl_lower_ir_unit
if ($LASTEXITCODE -ne 0) { throw 'Release failed' }
ctest --test-dir build-cg20-glsl-candidate -C Release -R "(glsl|geometry|^generic_)" --output-on-failure
if ($LASTEXITCODE -ne 0) { throw 'backend tests failed' }
git diff --check
```
Expected: all selected tests pass. Compare generated backend outputs with the
pristine baseline using Task 14's complete-set procedure. Run only
one configuration's ARB tests at a time because assembly artifact paths are shared.

- [x] **Step 5: Commit this extraction.**
```powershell
git add glsl_lower.c glsl_lower_geometry.c CMakeLists.txt
git commit -m "Extract GLSL geometry lowering"
```

### Task 12: Extract function lowering

**Files:** Create `glsl_lower_function.c`; modify `glsl_lower.c` and `CMakeLists.txt`.

- [x] **Step 1: Move this exact ordered inventory.**
Move complete definitions and attached comments from the original source, preserving
relative order. The line numbers refer to the pinned planning baseline and are
navigation hints; function names are authoritative.

```text
GlslFunctionComesBefore — original line 2011
GlslInsertFunction — original line 2030
GlslCollectCallsInStatements — original line 2043
GlslCollectHelper — original line 2088
GlslCollectCallsInExpr — original line 2151
GlslMappedSignatureEqual — original line 2183
GlslBuildSignature — original line 2205
GlslAssignHelperNames — original line 2234
GlslFunctionIsAfter — original line 2291
GlslMarkForwardCallsInExpr — original line 2309
GlslMarkForwardCallsInStatements — original line 2345
GlslMarkForwardCalls — original line 2392
GlslLowerHelper — original line 4194
GlslIRFindIRFunction — original line 5510
GlslIRCollectCallsInExpr — original line 5524
GlslIRCollectCallsInStmt — original line 5597
GlslIRFirstBodyLoc — original line 5788
GlslIRCollectHelper — original line 5810
GlslIRMarkForwardCallsInExpr — original line 5901
GlslIRMarkForwardCallsInStmt — original line 5995
GlslIRLowerFunctionBody — original line 8921
```

Create the module with its complete original license and this include block:
```c
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <float.h>
#include <limits.h>

#include "glsl_lower_internal.h"
```
Keep the private API linkage established in Task 2. Move any required static
forward declarations with their owning module.



- [x] **Step 2: Append the new file to the canonical list.**
The list after this task must be exactly:
```cmake
set(CGC_GLSL_LOWER_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/glsl_lower.c
    ${CMAKE_CURRENT_SOURCE_DIR}/glsl_lower_support.c
    ${CMAKE_CURRENT_SOURCE_DIR}/glsl_lower_decl.c
    ${CMAKE_CURRENT_SOURCE_DIR}/glsl_lower_interface.c
    ${CMAKE_CURRENT_SOURCE_DIR}/glsl_lower_aggregate.c
    ${CMAKE_CURRENT_SOURCE_DIR}/glsl_lower_legacy_expr.c
    ${CMAKE_CURRENT_SOURCE_DIR}/glsl_lower_legacy_stmt.c
    ${CMAKE_CURRENT_SOURCE_DIR}/glsl_lower_ir_expr.c
    ${CMAKE_CURRENT_SOURCE_DIR}/glsl_lower_ir_stmt.c
    ${CMAKE_CURRENT_SOURCE_DIR}/glsl_lower_geometry.c
    ${CMAKE_CURRENT_SOURCE_DIR}/glsl_lower_function.c
)
```

- [x] **Step 3: Audit extraction before building.**
Compare the moved bodies against the pinned baseline and the prior commit. Allow
only the recorded linkage changes.
Require each listed definition exactly once in its new owner, zero remaining
copies, and no changes to remaining bodies. Preserve all file/function-static
state and comments. A link failure is a missing dependency to reconcile against
Appendix A, not permission to change behavior.

- [x] **Step 4: Build both configurations and run backend coverage.**
```powershell
cmake -S . -B build-cg20-glsl-candidate -A x64 -DBUILD_TESTING=ON -DCGC_REQUIRE_FXC=ON
if ($LASTEXITCODE -ne 0) { throw 'configure failed' }
cmake --build build-cg20-glsl-candidate --config Debug --target cgc glsl_lower_ir_unit
if ($LASTEXITCODE -ne 0) { throw 'Debug failed' }
cmake --build build-cg20-glsl-candidate --config Release --target cgc glsl_lower_ir_unit
if ($LASTEXITCODE -ne 0) { throw 'Release failed' }
ctest --test-dir build-cg20-glsl-candidate -C Release -R "(glsl|geometry|^generic_)" --output-on-failure
if ($LASTEXITCODE -ne 0) { throw 'backend tests failed' }
git diff --check
```
Expected: all selected tests pass. Compare generated backend outputs with the
pristine baseline using Task 14's complete-set procedure. Run only
one configuration's ARB tests at a time because assembly artifact paths are shared.

- [x] **Step 5: Commit this extraction.**
```powershell
git add glsl_lower.c glsl_lower_function.c CMakeLists.txt
git commit -m "Extract GLSL function lowering"
```

### Task 13: Enforce the final facade and structural contract

**Files:** Modify `glsl_lower.c` and `tests/CMakeLists.txt`;
create `tests/check_glsl_lower_structure.cmake` and
`tests/check_glsl_lower_structure_selftest.cmake`.

- [x] **Step 1: Audit the facade.**
Keep only the complete original license, standard includes, private include,
and `GlslLowerLegacyProgram` and `GlslLowerCgIR`.
Remove obsolete private forwards and section banners left by extraction.
Public function bodies retain their exact original contents.
The fixed ceiling is 350 lines; existing entry bodies and the license fit within it.

- [x] **Step 2: Add this complete structural checker.**
It follows the HLSL checker without changing the HLSL test. Test-double exceptions
apply only to public-definition checks; production private-header leaks still fail.

```cmake
cmake_minimum_required(VERSION 3.16)

if(NOT DEFINED SOURCE_ROOT OR SOURCE_ROOT STREQUAL "")
    message(FATAL_ERROR "SOURCE_ROOT is required")
endif()

set(lower_header
    glsl_lower_internal.h)
set(lower_sources
    glsl_lower.c
    glsl_lower_support.c
    glsl_lower_decl.c
    glsl_lower_interface.c
    glsl_lower_aggregate.c
    glsl_lower_legacy_expr.c
    glsl_lower_legacy_stmt.c
    glsl_lower_ir_expr.c
    glsl_lower_ir_stmt.c
    glsl_lower_geometry.c
    glsl_lower_function.c)

foreach(relative IN LISTS lower_header lower_sources)
    if(NOT EXISTS "${SOURCE_ROOT}/${relative}")
        message(FATAL_ERROR "missing GLSL lowering file ${relative}")
    endif()
endforeach()

function(RemoveCmakeComments input output)
    string(REGEX REPLACE "#[^\r\n]*" "" uncommented "${input}")
    set(${output} "${uncommented}" PARENT_SCOPE)
endfunction()

function(NormalizeLowerSourceEntry entry output)
    string(REPLACE [=[${CMAKE_CURRENT_SOURCE_DIR}]=] "${SOURCE_ROOT}"
        normalized_entry "${entry}")
    get_filename_component(normalized_entry "${normalized_entry}" ABSOLUTE)
    file(TO_CMAKE_PATH "${normalized_entry}" normalized_entry)
    set(${output} "${normalized_entry}" PARENT_SCOPE)
endfunction()

function(HasInternalHeaderInclude relative output)
    file(STRINGS "${SOURCE_ROOT}/${relative}" source_lines)
    set(found FALSE)
    foreach(line IN LISTS source_lines)
        string(REGEX MATCH
            "^[ \t]*#[ \t]*include[ \t]*\"glsl_lower_internal\\.h\""
            include_line "${line}")
        if(NOT include_line STREQUAL "")
            set(found TRUE)
            break()
        endif()
    endforeach()
    set(${output} ${found} PARENT_SCOPE)
endfunction()

foreach(relative IN LISTS lower_sources)
    HasInternalHeaderInclude("${relative}" has_internal_header)
    if(NOT has_internal_header)
        message(FATAL_ERROR
            "${relative} must include glsl_lower_internal.h")
    endif()
endforeach()

file(STRINGS "${SOURCE_ROOT}/glsl_lower.c" facade_lines)
list(LENGTH facade_lines facade_line_count)
if(facade_line_count GREATER 350)
    message(FATAL_ERROR
        "glsl_lower.c remains a monolith: ${facade_line_count} lines")
endif()

file(READ "${SOURCE_ROOT}/CMakeLists.txt" root_cmake)
set(source_list_pattern
    "set[ \t\r\n]*\\([ \t\r\n]*CGC_GLSL_LOWER_SOURCES[ \t\r\n]+([^)]*)\\)")
string(REGEX MATCH "${source_list_pattern}" source_list_match "${root_cmake}")
if(source_list_match STREQUAL "")
    message(FATAL_ERROR "CGC_GLSL_LOWER_SOURCES is not defined")
endif()
set(source_list_body "${CMAKE_MATCH_1}")
RemoveCmakeComments("${source_list_body}" source_list_body)
string(REGEX MATCHALL "[^ \t\r\n]+" source_list_entries
    "${source_list_body}")

set(canonical_entries)
foreach(relative IN LISTS lower_sources)
    NormalizeLowerSourceEntry("${SOURCE_ROOT}/${relative}" canonical_entry)
    list(APPEND canonical_entries "${canonical_entry}")
endforeach()
list(LENGTH lower_sources lower_source_count)
list(LENGTH source_list_entries source_list_entry_count)
if(NOT source_list_entry_count EQUAL lower_source_count)
    message(FATAL_ERROR
        "CGC_GLSL_LOWER_SOURCES must contain exactly the canonical lowering sources")
endif()
foreach(relative IN LISTS lower_sources)
    NormalizeLowerSourceEntry("${SOURCE_ROOT}/${relative}" canonical_entry)
    set(match_count 0)
    foreach(entry IN LISTS source_list_entries)
        NormalizeLowerSourceEntry("${entry}" normalized_entry)
        if(normalized_entry STREQUAL canonical_entry)
            math(EXPR match_count "${match_count} + 1")
        endif()
    endforeach()
    if(NOT match_count EQUAL 1)
        message(FATAL_ERROR
            "CGC_GLSL_LOWER_SOURCES must contain ${relative} exactly once")
    endif()
endforeach()
foreach(entry IN LISTS source_list_entries)
    NormalizeLowerSourceEntry("${entry}" normalized_entry)
    list(FIND canonical_entries "${normalized_entry}" lower_source_position)
    if(lower_source_position EQUAL -1)
        message(FATAL_ERROR
            "CGC_GLSL_LOWER_SOURCES contains non-canonical entry ${entry}")
    endif()
endforeach()

set(cgc_target_pattern
    "add_executable[ \t\r\n]*\\([ \t\r\n]*cgc[ \t\r\n]+([^)]*)\\)")
string(REGEX MATCH "${cgc_target_pattern}" cgc_target_match "${root_cmake}")
if(cgc_target_match STREQUAL "")
    message(FATAL_ERROR "add_executable(cgc ...) is not defined")
endif()
set(cgc_target_body "${CMAKE_MATCH_1}")
RemoveCmakeComments("${cgc_target_body}" cgc_target_body)
string(FIND "${cgc_target_body}" [=[${CGC_GLSL_LOWER_SOURCES}]=]
    cgc_list_position)
if(cgc_list_position EQUAL -1)
    message(FATAL_ERROR
        "cgc does not use CGC_GLSL_LOWER_SOURCES")
endif()
string(REGEX MATCH "glsl_lower[^ \t\r\n)]*\\.c" direct_cgc_lower_source
    "${cgc_target_body}")
if(NOT direct_cgc_lower_source STREQUAL "")
    message(FATAL_ERROR
        "cgc directly lists ${direct_cgc_lower_source} instead of CGC_GLSL_LOWER_SOURCES")
endif()

file(READ "${SOURCE_ROOT}/tests/CMakeLists.txt" test_cmake)
set(geometry_target_pattern
    "add_executable[ \t\r\n]*\\([ \t\r\n]*glsl_lower_ir_unit[ \t\r\n]+([^)]*)\\)")
string(REGEX MATCH "${geometry_target_pattern}" geometry_target_match
    "${test_cmake}")
if(geometry_target_match STREQUAL "")
    message(FATAL_ERROR
        "add_executable(glsl_lower_ir_unit ...) is not defined")
endif()
set(geometry_target_body "${CMAKE_MATCH_1}")
RemoveCmakeComments("${geometry_target_body}" geometry_target_body)
string(FIND "${geometry_target_body}" [=[${CGC_GLSL_LOWER_SOURCES}]=]
    geometry_list_position)
if(geometry_list_position EQUAL -1)
    message(FATAL_ERROR
        "glsl_lower_ir_unit does not use CGC_GLSL_LOWER_SOURCES")
endif()
string(REGEX MATCHALL "[^ \t\r\n]+" geometry_target_entries
    "${geometry_target_body}")
foreach(entry IN LISTS geometry_target_entries)
    if(entry STREQUAL "glsl_lower_ir_test.c")
        continue()
    endif()
    string(REGEX MATCH "glsl_lower[^ \t\r\n)]*\\.c"
        direct_geometry_lower_source "${entry}")
    if(NOT direct_geometry_lower_source STREQUAL "")
        message(FATAL_ERROR
            "glsl_lower_ir_unit directly lists ${direct_geometry_lower_source} instead of CGC_GLSL_LOWER_SOURCES")
    endif()
endforeach()

file(GLOB_RECURSE repository_files RELATIVE "${SOURCE_ROOT}"
    "${SOURCE_ROOT}/*.c"
    "${SOURCE_ROOT}/*.h")
foreach(relative IN LISTS repository_files)
    if(relative MATCHES "(^|/)([.]worktrees|build[^/]*|cmake-build[^/]*|generated[^/]*)(/|$)")
        continue()
    endif()

    HasInternalHeaderInclude("${relative}" has_internal_header)
    if(has_internal_header)
        list(FIND lower_sources "${relative}" lower_source_position)
        if(lower_source_position EQUAL -1)
            message(FATAL_ERROR
                "${relative} includes glsl_lower_internal.h outside the lowering implementation")
        endif()
    endif()
endforeach()

set(public_entry_points
    GlslLowerLegacyProgram
    GlslLowerCgIR)
foreach(entry_point IN LISTS public_entry_points)
    set("${entry_point}_definition_owners")
endforeach()
foreach(relative IN LISTS repository_files)
    if(NOT relative MATCHES "\\.c$")
        continue()
    endif()
    if(relative MATCHES "(^|/)([.]worktrees|build[^/]*|cmake-build[^/]*|generated[^/]*)(/|$)")
        continue()
    endif()
    if(relative MATCHES "^tests/")
        continue()
    endif()

    file(READ "${SOURCE_ROOT}/${relative}" source_content)
    string(REGEX REPLACE "//[^\r\n]*" "" source_content
        "${source_content}")
    string(REGEX REPLACE "/\\*([^*]|\\*+[^*/])*\\*+/" "" source_content
        "${source_content}")
    foreach(entry_point IN LISTS public_entry_points)
        set(definition_pattern
            "int[ \t\r\n]+${entry_point}[ \t\r\n]*\\([^;{}]*\\)[ \t\r\n]*\\{")
        string(REGEX MATCH "${definition_pattern}" entry_point_definition
            "${source_content}")
        if(NOT entry_point_definition STREQUAL "")
            if(NOT relative STREQUAL "glsl_lower.c")
                message(FATAL_ERROR
                    "${relative} defines ${entry_point}; only glsl_lower.c may define it")
            endif()
            list(APPEND "${entry_point}_definition_owners" "${relative}")
        endif()
    endforeach()
endforeach()
foreach(entry_point IN LISTS public_entry_points)
    list(LENGTH "${entry_point}_definition_owners" definition_owner_count)
    if(NOT definition_owner_count EQUAL 1)
        message(FATAL_ERROR
            "glsl_lower.c must be the only definition owner of ${entry_point}")
    endif()
endforeach()
```

- [x] **Step 3: Add this complete checker self-test.**
```cmake
cmake_minimum_required(VERSION 3.16)
if(NOT DEFINED SOURCE_ROOT OR NOT DEFINED STRUCTURE_SCRIPT)
    message(FATAL_ERROR "SOURCE_ROOT and STRUCTURE_SCRIPT required")
endif()
set(fixture "${SOURCE_ROOT}/build-cg20-glsl-structure-fixture")
file(MAKE_DIRECTORY "${fixture}/tests" "${fixture}/.worktrees/other")
file(WRITE "${fixture}/.worktrees/other/copy.c" "")
file(WRITE "${fixture}/unrelated.c" "")
file(WRITE "${fixture}/tests/stub.c" "")
file(WRITE "${fixture}/glsl_lower_internal.h" "")
set(root_cmake [=[set(CGC_GLSL_LOWER_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/glsl_lower.c
    ${CMAKE_CURRENT_SOURCE_DIR}/glsl_lower_support.c
    ${CMAKE_CURRENT_SOURCE_DIR}/glsl_lower_decl.c
    ${CMAKE_CURRENT_SOURCE_DIR}/glsl_lower_interface.c
    ${CMAKE_CURRENT_SOURCE_DIR}/glsl_lower_aggregate.c
    ${CMAKE_CURRENT_SOURCE_DIR}/glsl_lower_legacy_expr.c
    ${CMAKE_CURRENT_SOURCE_DIR}/glsl_lower_legacy_stmt.c
    ${CMAKE_CURRENT_SOURCE_DIR}/glsl_lower_ir_expr.c
    ${CMAKE_CURRENT_SOURCE_DIR}/glsl_lower_ir_stmt.c
    ${CMAKE_CURRENT_SOURCE_DIR}/glsl_lower_geometry.c
    ${CMAKE_CURRENT_SOURCE_DIR}/glsl_lower_function.c
)
add_executable(cgc ${CGC_GLSL_LOWER_SOURCES})
]=])
file(WRITE "${fixture}/CMakeLists.txt" "${root_cmake}")
file(WRITE "${fixture}/tests/CMakeLists.txt" [=[add_executable(glsl_lower_ir_unit glsl_lower_ir_test.c ${CGC_GLSL_LOWER_SOURCES})
]=])
file(WRITE "${fixture}/glsl_lower.c" [=[#include "glsl_lower_internal.h"
int GlslLowerLegacyProgram(void) { return 0; }
int GlslLowerCgIR(void) { return 0; }
]=])
file(WRITE "${fixture}/glsl_lower_support.c" "#include \"glsl_lower_internal.h\"\n")
file(WRITE "${fixture}/glsl_lower_decl.c" "#include \"glsl_lower_internal.h\"\n")
file(WRITE "${fixture}/glsl_lower_interface.c" "#include \"glsl_lower_internal.h\"\n")
file(WRITE "${fixture}/glsl_lower_aggregate.c" "#include \"glsl_lower_internal.h\"\n")
file(WRITE "${fixture}/glsl_lower_legacy_expr.c" "#include \"glsl_lower_internal.h\"\n")
file(WRITE "${fixture}/glsl_lower_legacy_stmt.c" "#include \"glsl_lower_internal.h\"\n")
file(WRITE "${fixture}/glsl_lower_ir_expr.c" "#include \"glsl_lower_internal.h\"\n")
file(WRITE "${fixture}/glsl_lower_ir_stmt.c" "#include \"glsl_lower_internal.h\"\n")
file(WRITE "${fixture}/glsl_lower_geometry.c" "#include \"glsl_lower_internal.h\"\n")
file(WRITE "${fixture}/glsl_lower_function.c" "#include \"glsl_lower_internal.h\"\n")
function(Check expected)
    execute_process(COMMAND "${CMAKE_COMMAND}" "-DSOURCE_ROOT=${fixture}" -P "${STRUCTURE_SCRIPT}" RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE error)
    if(expected STREQUAL "PASS")
        if(NOT result EQUAL 0)
            message(FATAL_ERROR "${output}${error}")
        endif()
    else()
        string(FIND "${output}${error}" "${expected}" found)
        if(result EQUAL 0 OR found LESS 0)
            message(FATAL_ERROR "wrong failure: ${output}${error}")
        endif()
    endif()
endfunction()
Check("PASS")
foreach(direct_source glsl_lower.c glsl_lower_support.c)
    file(WRITE "${fixture}/tests/CMakeLists.txt"
        "add_executable(glsl_lower_ir_unit glsl_lower_ir_test.c ${direct_source} \${CGC_GLSL_LOWER_SOURCES})\n")
    Check("directly lists ${direct_source}")
endforeach()
file(WRITE "${fixture}/tests/CMakeLists.txt" [=[add_executable(glsl_lower_ir_unit glsl_lower_ir_test.c ${CGC_GLSL_LOWER_SOURCES})
]=])
Check("PASS")
file(WRITE "${fixture}/unrelated.c" "#include \"glsl_lower_internal.h\"\n")
Check("outside the lowering")
file(WRITE "${fixture}/unrelated.c" "int GlslLowerLegacyProgram(void) { return 0; }\n")
Check("only glsl_lower.c may define it")
file(WRITE "${fixture}/unrelated.c" "")
file(WRITE "${fixture}/tests/stub.c" "int GlslLowerLegacyProgram(void) { return 0; }\n")
Check("PASS")
file(WRITE "${fixture}/.worktrees/other/copy.c" [=[#include "glsl_lower_internal.h"
int GlslLowerLegacyProgram(void) { return 0; }
int GlslLowerCgIR(void) { return 0; }
]=])
Check("PASS")
string(REPLACE "glsl_lower_support.c" "glsl_lower.c" duplicate "${root_cmake}")
file(WRITE "${fixture}/CMakeLists.txt" "${duplicate}")
Check("exactly once")
string(REPLACE "\${CMAKE_CURRENT_SOURCE_DIR}/" "\${CMAKE_CURRENT_SOURCE_DIR}/wrong/" wrong "${root_cmake}")
file(WRITE "${fixture}/CMakeLists.txt" "${wrong}")
Check("exactly once")
string(REPLACE "add_executable(cgc " "add_executable(cgc glsl_lower.c " bypass "${root_cmake}")
file(WRITE "${fixture}/CMakeLists.txt" "${bypass}")
Check("directly lists")
file(WRITE "${fixture}/CMakeLists.txt" "${root_cmake}")
file(WRITE "${fixture}/glsl_lower_support.c" "")
Check("must include")
file(WRITE "${fixture}/glsl_lower_support.c" "#include \"glsl_lower_internal.h\"\n")
Check("PASS")
```

- [x] **Step 4: Run the checker and its negative fixtures.**
```powershell
$root = (Resolve-Path '.').Path
$guard = (Resolve-Path 'tests/check_glsl_lower_structure.cmake').Path
cmake "-DSOURCE_ROOT=$root" "-DSTRUCTURE_SCRIPT=$guard" -P tests/check_glsl_lower_structure_selftest.cmake
if ($LASTEXITCODE -ne 0) { throw 'checker self-test failed' }
cmake "-DSOURCE_ROOT=$root" -P tests/check_glsl_lower_structure.cmake
if ($LASTEXITCODE -ne 0) { throw 'real structure failed' }
```
Expected: both exit zero; negative fixtures internally prove rejection. Do not
weaken guards to accommodate a failed extraction.

- [x] **Step 5: Register exactly one test in tests/CMakeLists.txt.**
Place near glsl_lower_ir_unit:
```cmake
add_test(
    NAME glsl_lower_structure
    COMMAND ${CMAKE_COMMAND}
        -DSOURCE_ROOT=${PROJECT_SOURCE_DIR}
        -P ${CMAKE_CURRENT_SOURCE_DIR}/check_glsl_lower_structure.cmake
)
```
```powershell
cmake -S . -B build-cg20-glsl-candidate -A x64 -DBUILD_TESTING=ON -DCGC_REQUIRE_FXC=ON
if ($LASTEXITCODE -ne 0) { throw 'configure failed' }
ctest --test-dir build-cg20-glsl-candidate -C Release -R "^glsl_lower_structure$" --output-on-failure
if ($LASTEXITCODE -ne 0) { throw 'registered structure failed' }
git diff --check
git add glsl_lower.c tests/CMakeLists.txt tests/check_glsl_lower_structure.cmake tests/check_glsl_lower_structure_selftest.cmake
git commit -m "Enforce GLSL lowering boundaries"
```
Expected: 1/1 structural test passes. Audit context/type single ownership,
no new mutable globals, and unchanged public headers separately from the lexical guard.

### Task 14: Final equivalence qualification

**Files:** Verification artifacts only. Do not edit expected outputs.

- [x] **Step 1: Fresh candidate configuration and full Release qualification.**
```powershell
cmake -S . -B build-cg20-glsl-final -A x64 -DBUILD_TESTING=ON -DCGC_REQUIRE_FXC=ON
if ($LASTEXITCODE -ne 0) { throw 'configure failed' }
cmake --build build-cg20-glsl-final --config Release
if ($LASTEXITCODE -ne 0) { throw 'Release build failed' }
ctest --test-dir build-cg20-glsl-final -C Release --output-on-failure
if ($LASTEXITCODE -ne 0) { throw 'Release suite failed' }
```
Expected: baseline test names retained, exactly one additional structural test.
Do not count skipped OpenGL checks as successful driver validation.

- [x] **Step 2: Compare complete Release artifact sets before Debug writes them.**
Run this PowerShell from the candidate root. Baseline is the untouched sibling
created by Task 1.

```powershell
$baselineRoot = (Resolve-Path '../glsl-lower-pristine/build-cg20-glsl-baseline/tests').Path
$candidateRoot = (Resolve-Path 'build-cg20-glsl-final/tests').Path
function ArtifactManifest([string]$artifactRoot) {
    Get-ChildItem -LiteralPath $artifactRoot -Recurse -File -Filter '*.glsl' |
        Where-Object { $_.FullName -match '[\\/]Release[\\/]' } |
        ForEach-Object {
            [pscustomobject]@{
                Path = $_.FullName.Substring($artifactRoot.Length).Replace('\','/')
                Hash = (Get-FileHash -LiteralPath $_.FullName -Algorithm SHA256).Hash
            }
        } | Sort-Object Path
}
$baseline = @(ArtifactManifest $baselineRoot)
$candidate = @(ArtifactManifest $candidateRoot)
if ($baseline.Count -eq 0 -or $candidate.Count -eq 0) { throw 'empty artifact set' }
$differences = @(Compare-Object $baseline $candidate -Property Path,Hash)
$pathDifferences = @(Compare-Object $baseline $candidate -Property Path)
if ($pathDifferences.Count) { $pathDifferences; throw 'missing or extra artifacts' }
if ($differences.Count) { $differences; throw 'raw mismatch: classify every differing line before continuing' }
"All $($baseline.Count) raw artifacts match"
```

Raw differences require exhaustive line classification. Only known legacy compiler
timestamp values and absolute source/build roots may differ. Document exact values,
paths and line numbers before normalizing; compare complete normalized files and
require zero remaining differences. Never remove arbitrary headers, instruction
lines, version directives, bindings, or diagnostic content.

- [x] **Step 3: Verify diagnostic and status equivalence.**
Compare the unchanged negative fixtures against both binaries using the existing
CTest commands recorded in the JSON inventory. Retain full stdout/stderr and exit
results in the build evidence. Existing expected-diagnostic tests must pass on
both sides; additionally compare their captured failure diagnostics exactly after
replacing only the recorded baseline/candidate absolute roots. Any changed text,
line location, error order or exit status blocks qualification. Do not accept
matching message fragments alone as evidence of full diagnostic equivalence.
The exact-body audit is an independent additional check.

```powershell
# Run with PowerShell 7 so ProcessStartInfo.ArgumentList preserves arguments.
$baselineExe = (Resolve-Path '../glsl-lower-pristine/build-cg20-glsl-baseline/Release/cgc.exe').Path
$candidateExe = (Resolve-Path 'build-cg20-glsl-final/Release/cgc.exe').Path
$registry = ctest --test-dir build-cg20-glsl-final -C Release --show-only=json-v1 | ConvertFrom-Json
if ($LASTEXITCODE -ne 0) { throw "test inventory failed" }
$outputPath = Join-Path (Resolve-Path 'build-cg20-glsl-final').Path 'diagnostic-replay.out'
function RunCompiler([string]$compiler, [string[]]$arguments) {
    $info = [System.Diagnostics.ProcessStartInfo]::new()
    $info.FileName = $compiler
    $info.UseShellExecute = $false
    $info.RedirectStandardOutput = $true
    $info.RedirectStandardError = $true
    foreach ($arg in $arguments) { $info.ArgumentList.Add($arg) }
    $process = [System.Diagnostics.Process]::Start($info)
    $stdout = $process.StandardOutput.ReadToEndAsync()
    $stderr = $process.StandardError.ReadToEndAsync()
    $process.WaitForExit()
    $result = [pscustomobject]@{ Exit=$process.ExitCode; Out=$stdout.Result; Err=$stderr.Result }
    $process.Dispose()
    return $result
}
$checked = 0
foreach ($test in $registry.tests) {
    $defs = @{}
    foreach ($arg in $test.command) {
        if ($arg -match "^-D([^=]+)=(.*)$") { $defs[$Matches[1]] = $Matches[2] }
    }
    if ($defs.PROFILE -notmatch "^glsl" -or -not $defs.ContainsKey("CODE") -or -not $defs.SOURCE) { continue }
    $arguments = @("-quiet", "-profile", $defs.PROFILE)
    if ($defs.ENTRY) { $arguments += @("-entry", $defs.ENTRY) }
    if ($defs.PROFILE_OPTIONS) {
        foreach ($option in ($defs.PROFILE_OPTIONS -split ";")) { $arguments += @("-po", $option) }
    }
    if ($defs.EXTRA_ARGS) { $arguments += $defs.EXTRA_ARGS -split ";" }
    $arguments += @("-o", $outputPath, $defs.SOURCE)
    if (Test-Path -LiteralPath $outputPath) { Remove-Item -LiteralPath $outputPath }
    $before = RunCompiler $baselineExe $arguments
    if (Test-Path -LiteralPath $outputPath) { Remove-Item -LiteralPath $outputPath }
    $after = RunCompiler $candidateExe $arguments
    if ($before.Exit -eq 0 -or $before.Exit -ne $after.Exit -or $before.Out -cne $after.Out -or $before.Err -cne $after.Err) {
        throw "diagnostic/exit mismatch: $($test.name)"
    }
    $checked++
}
if ($checked -eq 0) { throw "no negative fixtures replayed" }
"Exact diagnostic and exit matches: $checked"
```

Both binaries receive the same absolute source/output paths, so this replay needs
no path normalization. The full suites also cover specialized transaction/pipeline
drivers beyond the single-source failures selected above.

- [x] **Step 4: Full Debug build and suite.**
```powershell
cmake --build build-cg20-glsl-final --config Debug
if ($LASTEXITCODE -ne 0) { throw 'Debug build failed' }
ctest --test-dir build-cg20-glsl-final -C Debug --output-on-failure
if ($LASTEXITCODE -ne 0) { throw 'Debug suite failed' }
ctest --test-dir build-cg20-glsl-final -C Release -R "^(hlsl_compatibility_matrix|hlsl_sm4_sm5_compatibility_matrix|hlsl_modern_fxc_coverage|hlsl_validation_exact_target_contract)$" --output-on-failure
if ($LASTEXITCODE -ne 0) { throw 'HLSL compatibility regression' }
```
Expected: all tests pass and four HLSL contracts remain green, including the
197-combination modern FXC contract unless independently expanded.

- [ ] **Step 5: Final review and integration checks.**
Verify all 205 baseline definitions have exactly one owner; function bodies,
private type fields, static storage and phase order match the baseline.
Public glsl_hal.h and glsl_ir.h must be byte-identical to baseline.
Run:
```powershell
git diff --check
git status --short
git log -12 --oneline
```
Expected: only focused committed work and ignored builds; user changes remain
untouched. Perform the final review against the approved design before integration.
On the merged main checkout, rerun the structural self-test, registered structural
test and full suite. Keep nested worktrees present during the structural check to
exercise the integration environment. Do not push or merge merely because this
planning document exists; execution/integration follow the user's active request.

## Appendix A: Caller-derived cross-module API

This manifest excludes mere forward declarations. It lists actual callers in
different final modules. Task 2 prepares this complete interface before any body
moves, which avoids temporary reverse-dependency link failures.

| Private helper | Owner | Callers in other modules |
|---|---|---|
| GlslSetLoc | support | GlslNewSourceDecl (decl), GlslEnsureType (decl), GlslCollectUniformSymbol (interface), GlslLowerInterface (interface), GlslCollectHelper (function), GlslLowerMatrixAssignment (legacy_stmt), GlslLowerForPart (legacy_stmt), GlslLowerStatementList (legacy_stmt), GlslLowerLegacyProgram (facade), GlslIRInstallGeometryInfo (geometry), GlslIRRegisterGeometryInput (geometry), GlslIRGeometryOutputDecl (geometry), GlslIRGeometryAssignments (geometry), GlslIRAssignDecl (geometry), GlslIRRegisterStruct (decl), GlslIRNewSourceDecl (decl), GlslIRRegisterGeometryFlatValue (geometry), GlslIRCollectHelper (function), GlslIRAppendMemberStep (aggregate), GlslIRAppendIndexStep (aggregate), GlslIRAppendDynamicIndex (aggregate), GlslIRAddLocal (aggregate), GlslIRNewIndexTemp (aggregate), GlslIREmitAggregateLeaves (aggregate), GlslIRNewAggregateTemp (aggregate), GlslIRGeometryBuiltinElement (geometry), GlslIRTryGroupWrite (aggregate), GlslIRLowerStatement (ir_stmt), GlslIRForPart (ir_stmt), GlslIRLowerFunctionBody (function), GlslLowerCgIR (facade) |
| GlslAllocateSymbolNameForSource | support | GlslNewSourceDecl (decl), GlslEnsureType (decl), GlslCollectUniformSymbol (interface), GlslLowerInterface (interface), GlslAssignHelperNames (function), GlslLowerLegacyProgram (facade), GlslIRRegisterStruct (decl), GlslIRNewSourceDecl (decl), GlslIRAddLocal (aggregate), GlslIRLowerFunctionBody (function), GlslLowerCgIR (facade) |
| GlslAllocateNameForSource | support | GlslValidateInterfaceSource (interface), GlslIRRegisterGeometryInput (geometry), GlslIRGeometryOutputDecl (geometry), GlslIRRegisterGeometryFlatValue (geometry) |
| GlslAllocateDistinctNameForSource | support | GlslMatrixHelperName (aggregate), GlslMatrixSelectorHelperName (aggregate) |
| GlslAllocateScopedSymbolNameForSource | support | GlslNewSourceDecl (decl), GlslCreateMatrixSelectorHelper (aggregate), GlslCreateMatrixHelper (aggregate), GlslIRNewSourceDecl (decl), GlslIRAddLocal (aggregate), GlslIRLowerFunctionBody (function) |
| GlslLowerError | support | GlslLowerLegacyProgram (facade), GlslLowerCgIR (facade) |
| GlslRecordFailure | support | GlslValidateUniformLimit (interface), GlslValidateGeometryInterfaceLimits (interface), GlslAppendDefaultValue (decl), GlslAppendTypedDefaultValue (decl), GlslConvertDefaultValue (decl), GlslFlattenDefaultExpr (decl), GlslStoreDefaultLeaf (decl), GlslCollectDefaults (decl), GlslCollectHelper (function), GlslLowerExprChain (legacy_expr), GlslLowerConstant (legacy_expr), GlslLowerExpr (legacy_expr), GlslLowerForPart (legacy_stmt), GlslLowerStatementList (legacy_stmt), GlslIRInstallGeometryInfo (geometry), GlslIRRegisterGeometryInput (geometry), GlslIRGeometryOutputDecl (geometry), GlslIRGeometryFlatAssignments (geometry), GlslIRCollectHelper (function), GlslIRConstantComponent (ir_expr), GlslIRLowerExpr (ir_expr), GlslIRLowerStatement (ir_stmt), GlslIRForPart (ir_stmt), GlslIRLowerFunctionBody (function), GlslLowerCgIR (facade) |
| GlslRecordFailureKind | support | GlslEnsureType (decl), GlslSortStructs (decl), GlslCollectUniformSymbol (interface), GlslValidateUniformLimit (interface), GlslValidateInterfaceLimits (interface), GlslAllocateTextureUnits (interface), GlslAppendTypedDefaultValue (decl), GlslConvertDefaultValue (decl), GlslDefaultTargetBase (decl), GlslStoreDefaultType (decl), GlslLowerInterface (interface), GlslCollectHelper (function), GlslLowerTextureArguments (legacy_expr), GlslLowerImpureMatrixConstructor (aggregate), GlslLowerMatrixConstructor (aggregate), GlslValidateTextureCall (legacy_expr), GlslLowerCall (legacy_expr), GlslLowerExpr (legacy_expr), GlslLowerStatementList (legacy_stmt), GlslValidateInterfaceSource (interface), GlslIRCollectHelper (function), GlslIRLowerMatrixConstructor (ir_expr), GlslIRLowerTextureArguments (ir_expr), GlslIRLowerCallCore (ir_expr), GlslIRLowerExpr (ir_expr), GlslIRLowerStatement (ir_stmt) |
| GlslRecordFailureKindAt | support | GlslLowerType (decl), GlslEnsureSymbolTypes (decl), GlslEnsureParameterTypes (decl), GlslValidateGeometryInterfaceLimits (interface), GlslIRScalarBase (decl), GlslIRType (decl), GlslIREnsureType (decl), GlslIRSamplerPlacementCheck (decl), GlslIRLowerAggregateAssign (aggregate), GlslIRLocalDeclaration (decl) |
| GlslUnsupportedExprReason | support | GlslLowerExpr (legacy_expr) |
| GlslCopyText | support | GlslLowerType (decl), GlslAllocateTextureUnits (interface), GlslNewSwizzle (legacy_expr), GlslIRType (decl) |
| GlslTypesEqual | support | GlslMappedSignatureEqual (function), GlslFindMatrixHelper (aggregate), GlslFindMatrixSelectorHelper (aggregate), GlslValidateTextureCall (legacy_expr), GlslLowerCall (legacy_expr), GlslLowerExpr (legacy_expr), GlslIRLowerCallCore (ir_expr), GlslIRLowerExpr (ir_expr) |
| GlslIsSamplerType | support | GlslLowerTextureArguments (legacy_expr), GlslLowerExpr (legacy_expr), GlslIRLowerTextureArguments (ir_expr), GlslIRLowerExpr (ir_expr) |
| GlslFindDecl | support | GlslCollectLocals (decl), GlslLowerInterface (interface), GlslLowerTextureArguments (legacy_expr), GlslLowerExpr (legacy_expr), GlslIRLowerMaterialized (aggregate), GlslIRLowerTextureArguments (ir_expr), GlslIRLowerExpr (ir_expr) |
| GlslFindFunction | support | GlslCollectHelper (function), GlslMarkForwardCallsInExpr (function), GlslLowerCall (legacy_expr), GlslIRCollectHelper (function), GlslIRMarkForwardCallsInExpr (function), GlslIRLowerCallCore (ir_expr) |
| GlslLowerType | decl | GlslCollectHelper (function), GlslLowerExpr (legacy_expr) |
| GlslInsertDecl | decl | GlslIRAddLocal (aggregate) |
| GlslEnsureTypeAt | decl | GlslCollectHelper (function), GlslLowerLegacyProgram (facade) |
| GlslEnsureSymbolTypes | decl | GlslCollectHelper (function), GlslLowerLegacyProgram (facade) |
| GlslEnsureParameterTypes | decl | GlslCollectHelper (function), GlslLowerLegacyProgram (facade) |
| GlslSortStructs | decl | GlslLowerLegacyProgram (facade), GlslLowerCgIR (facade) |
| GlslCollectParameters | decl | GlslLowerHelper (function), GlslLowerLegacyProgram (facade) |
| GlslFindUniformBinding | interface | GlslValidateTextureCall (legacy_expr) |
| GlslCollectUniforms | interface | GlslLowerLegacyProgram (facade) |
| GlslValidateUniformLimit | interface | GlslLowerLegacyProgram (facade), GlslLowerCgIR (facade) |
| GlslValidateInterfaceLimits | interface | GlslLowerLegacyProgram (facade), GlslLowerCgIR (facade) |
| GlslAllocateTextureUnits | interface | GlslLowerLegacyProgram (facade), GlslLowerCgIR (facade) |
| GlslFiniteDefaultFloat | decl | GlslIRConstantComponent (ir_expr) |
| GlslCollectDefaults | decl | GlslLowerLegacyProgram (facade), GlslLowerCgIR (facade) |
| GlslCollectLocals | decl | GlslLowerHelper (function), GlslLowerLegacyProgram (facade) |
| GlslInsertBinding | interface | GlslIRRegisterGeometryInput (geometry), GlslIRGeometryOutputDecl (geometry) |
| GlslLowerInterface | interface | GlslLowerExpr (legacy_expr), GlslIRLowerExpr (ir_expr) |
| GlslCollectCallsInStatements | function | GlslLowerLegacyProgram (facade) |
| GlslAssignHelperNames | function | GlslLowerLegacyProgram (facade), GlslLowerCgIR (facade) |
| GlslMarkForwardCalls | function | GlslLowerLegacyProgram (facade) |
| GlslAppendExpr | support | GlslLowerExprChain (legacy_expr), GlslLowerConstant (legacy_expr), GlslLowerMatrixSwizzle (aggregate), GlslCreateMatrixSelectorHelper (aggregate), GlslCreateMatrixHelper (aggregate), GlslLowerMatrixConstructor (aggregate), GlslLowerConditional (legacy_expr), GlslIRLowerMatrixConstructor (ir_expr), GlslIRLowerExprList (ir_expr), GlslIRLowerConditional (ir_expr), GlslIRGeometryBuiltinArray (geometry), GlslIRLowerExpr (ir_expr) |
| GlslLowerExprChain | legacy_expr | GlslLowerMatrixConstructor (aggregate) |
| GlslNewLiteral | legacy_expr | GlslIRConstantComponent (ir_expr), GlslIRLowerCallCore (ir_expr), GlslIRLowerIncrement (ir_expr), GlslIRGeometryBuiltinArray (geometry) |
| GlslNewSwizzle | legacy_expr | GlslNewParameterComponent (aggregate), GlslLowerMatrixConstructor (aggregate), GlslIRLowerMaterialized (aggregate), GlslIRLowerSwizzle (ir_expr), GlslIRLowerMatrixConstructor (ir_expr), GlslIRLowerComponent (ir_expr) |
| GlslMatrixSelectorCount | aggregate | GlslLowerExpr (legacy_expr), GlslLowerMatrixAssignment (legacy_stmt), GlslLowerStatementList (legacy_stmt) |
| GlslMatrixSelectorComponent | aggregate | GlslLowerMatrixAssignment (legacy_stmt) |
| GlslLowerMatrixSwizzle | aggregate | GlslLowerExpr (legacy_expr) |
| GlslMatrixNumericParameterType | aggregate | GlslIRLowerMatrixConstructor (ir_expr) |
| GlslGetMatrixSelectorHelper | aggregate | GlslLowerMatrixAssignment (legacy_stmt), GlslIRLowerExpr (ir_expr) |
| GlslLowerImpureMatrixConstructor | aggregate | GlslIRLowerMatrixConstructor (ir_expr) |
| GlslLowerMatrixConstructor | aggregate | GlslLowerExpr (legacy_expr) |
| GlslValidateTextureCall | legacy_expr | GlslIRLowerCallCore (ir_expr) |
| GlslIntrinsicBuiltin | legacy_expr | GlslIRLowerCallCore (ir_expr) |
| GlslLowerExpr | legacy_expr | GlslLowerMatrixSwizzle (aggregate), GlslLowerMatrixAssignment (legacy_stmt), GlslLowerForPart (legacy_stmt), GlslLowerStatementList (legacy_stmt) |
| GlslLowerStatementList | legacy_stmt | GlslLowerHelper (function), GlslLowerLegacyProgram (facade) |
| GlslLowerHelper | function | GlslLowerLegacyProgram (facade) |
| GlslPrependMatrixHelpers | aggregate | GlslLowerLegacyProgram (facade), GlslLowerCgIR (facade) |
| GlslPrependMatrixSelectorHelpers | aggregate | GlslLowerLegacyProgram (facade), GlslLowerCgIR (facade) |
| GlslValidateEntryInterfaces | interface | GlslLowerLegacyProgram (facade) |
| GlslIRInstallGeometryInfo | geometry | GlslLowerCgIR (facade) |
| GlslInterpolationForType | geometry | GlslLowerInterface (interface) |
| GlslIRRegisterGeometryInput | geometry | GlslIRLowerFunctionBody (function) |
| GlslIRGeometryAssignments | geometry | GlslIRLowerStatement (ir_stmt) |
| GlslIRGeometryFlatAssignments | geometry | GlslIRLowerStatement (ir_stmt) |
| GlslIRGeometryFlatReplay | geometry | GlslIRLowerStatement (ir_stmt) |
| GlslLowerTypeAuto | decl | GlslCollectUniformSymbol (interface), GlslLowerInterface (interface), GlslMappedSignatureEqual (function), GlslBuildSignature (function) |
| GlslEnsureTypeAuto | decl | GlslCollectUniformSymbol (interface) |
| GlslIRScalarBase | decl | GlslIRLowerExpr (ir_expr) |
| GlslIRIsSamplerValue | decl | GlslIRCollectHelper (function) |
| GlslIRType | decl | GlslIRRegisterGeometryInput (geometry), GlslIRGeometryOutputDecl (geometry), GlslIRGeometryAssignments (geometry), GlslIRRegisterGeometryFlatValue (geometry), GlslIRCollectHelper (function), GlslIRAddLocal (aggregate), GlslIRLowerMaterialized (aggregate), GlslIRNewAggregateTemp (aggregate), GlslIRLowerAggregateAssign (aggregate), GlslIRLowerConstant (ir_expr), GlslIRLowerSwizzle (ir_expr), GlslIRLowerExpr (ir_expr), GlslIRLowerFunctionBody (function) |
| GlslIREnsureTypeAt | decl | GlslIRCollectHelper (function), GlslIRLowerFunctionBody (function) |
| GlslIRSamplerPlacementCheck | decl | GlslIRCollectHelper (function), GlslIRLowerFunctionBody (function) |
| GlslIRFindIRFunction | function | GlslIRCollectGeometryFlatState (geometry), GlslIRCollectUniforms (interface), GlslLowerCgIR (facade) |
| GlslIRCollectCallsInStmt | function | GlslLowerCgIR (facade) |
| GlslIRCollectGeometryFlatState | geometry | GlslLowerCgIR (facade) |
| GlslIRMarkForwardCallsInStmt | function | GlslLowerCgIR (facade) |
| GlslIRValidateEntryInterfaces | interface | GlslLowerCgIR (facade) |
| GlslIRNeedsMaterialization | aggregate | GlslIRLowerExpr (ir_expr) |
| GlslCloneExpr | aggregate | GlslIRLowerIncrement (ir_expr), GlslIRLowerExpr (ir_expr) |
| GlslIRAddLocal | aggregate | GlslIRLocalDeclaration (decl) |
| GlslIRLowerAggregateAssign | aggregate | GlslIRLowerStatement (ir_stmt) |
| GlslIRSharedSelection | ir_expr | GlslIRTryGroupWrite (aggregate) |
| GlslIRGeometryEntryParameter | geometry | GlslIRLowerExpr (ir_expr) |
| GlslIRGeometryBuiltinElement | geometry | GlslIRLowerExpr (ir_expr) |
| GlslIRGeometryBuiltinArray | geometry | GlslIRLowerExpr (ir_expr) |
| GlslIRLowerExpr | ir_expr | GlslIRGeometryAssignments (geometry), GlslIRGeometryFlatAssignments (geometry), GlslIRNewIndexTemp (aggregate), GlslIRLowerMaterialized (aggregate), GlslIRTryGroupWrite (aggregate), GlslIRLowerStatement (ir_stmt), GlslIRForPart (ir_stmt) |
| GlslIRDeclNameText | aggregate | GlslIRLocalDeclaration (decl) |
| GlslIRMatrixElement | aggregate | GlslIRLowerExpr (ir_expr) |
| GlslIRTryGroupWrite | aggregate | GlslIRBlockBody (ir_stmt) |
| GlslIRLocalDeclaration | decl | GlslIRBlockBody (ir_stmt) |
| GlslIRBlockBody | ir_stmt | GlslIRLowerFunctionBody (function) |
| GlslIRLowerFunctionBody | function | GlslLowerCgIR (facade) |
| GlslIRCollectUniforms | interface | GlslLowerCgIR (facade) |
| GlslIREnsureEntryLocals | decl | GlslLowerCgIR (facade) |

## Execution status: isolated qualification complete

Implementation through Task 13 and isolated Task 14 qualification are complete.
Qualified source commit: `a36bce36366e4520039670597afa963359382e4e`.
Pristine source: `7774767b115522c4d9c21c9ee49c6471bd1f1a3f`.
Every implementation task passed independent specification and quality reviews;
the complete branch also passed final review.

- The facade is 269 lines; all 205 original bodies and approved owner/signature
  records match. All 89 private declarations, private types, state, and public
  headers are preserved.
- Fresh full Release and Debug builds and suites passed: 1,371 tests each,
  no skips. The sole inventory addition is `glsl_lower_structure`.
- Both ARB WGL smoke tests passed in both configurations.
- All 100 negative GLSL replays match complete stdout, stderr, and nonzero exit.
- Four HLSL contracts pass, including 197 exact modern FXC combinations.
- Final recursive Release comparison covers 205 GLSL files: 3 raw-identical,
  202 identical after 404 precisely classified timestamp/root-comment differences.
  The original frozen manifest contains 204 files. The additional validator-failure
  probe has a separately recorded later pristine hash; that provenance is explicit
  and the original manifest was not rewritten.
- Incremental Tasks 3–12 compared the 204 files under `tests/Release` and
  `tests/validated/Release`. Final review corrected that narrower inventory to
  include `tests/validator-failure/Release/probe.glsl`.
- The planned guard's broad source pattern falsely matched the existing
  `glsl_lower_ir_test.c` harness. The corrected unit scan exempts only that exact
  token; new fixtures prove direct facade/support entries remain rejected.
  The implementation snippets above include this reviewed correction.
- The selected incremental test filter needs five additional unit executables on
  a fresh build: `glsl_ir_unit`, `hlsl_ir_unit`, `hlsl_geometry_lower_unit`,
  `glsl_semantics_unit`, and `cg_geometry_unit`. Four additional output producers
  are `cg20_profile_overload_vertex`, `cg20_profile_overload_fragment`,
  `cg20_profile_overload_open`, and `cg20_unreachable_profile_feature`.
- Evidence is retained in ignored `build-cg20-glsl-final/EVIDENCE.md` and
  `build-cg20-glsl-preflight/`; pristine evidence remains frozen in the sibling
  baseline worktree. Task 6 build/selected-test logs were captured by an independent
  rerun after the first successful run did not retain full logs.

The user approved local integration into master, then ARB continuation. Main-checkout
verification is pending; nothing has been pushed.

## Final review checklist

- [ ] Approved responsibilities all have explicit function ownership.
- [ ] Every shared declaration matches its definition and callers.
- [ ] All private types/forward declarations travel with the correct owner.
- [ ] No public API, profile, IR, code-generation or expected-output change.
- [ ] Pristine baseline remained separate throughout execution.
- [ ] All old test names retained; new structural test counted exactly once.
- [ ] Debug/Release, external validation, raw or narrowly normalized differential
  evidence and main-checkout structure checks pass.
- [ ] Any unsupported-driver skips are reported explicitly.

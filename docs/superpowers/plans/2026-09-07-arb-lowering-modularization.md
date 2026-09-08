# ARB Lowering Modularization Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [x]`) syntax for tracking.

**Goal:** Mechanically split arb_lower.c into private modules while preserving compiler behavior.

**Architecture:** Keep the existing public facade and stack-owned context. Install the complete cross-module interface before moving functions, then extract one responsibility per commit. Use one canonical CMake source list and backend-specific structural tests.

**Tech Stack:** C90 with extensions, CMake 3.16+, MSVC x64, CTest, glslangValidator, Windows SDK FXC, Windows OpenGL smoke validation.

## Execution status — 2026-09-07

Implementation and isolated qualification are complete at `97395d5`; both final independent specification/architecture and quality reviews approved with no actionable findings. GLSL was already integrated into master before ARB began. With explicit user approval, ARB was fast-forwarded into `master` at `2596fd3`; nothing was pushed.

Merged-main verification is complete: a fresh x64 Release build and all **1,372/1,372 tests** passed with zero skips, including both actual WGL program loads and the 197-combination FXC contract. The structural self-test and guard passed with nested worktrees present. Evidence is retained in `build-cg20-arb-merged/EVIDENCE.md`. Existing user changes were preserved; worktrees and ignored evidence remain available.

- Frozen execution baseline: `ea5a11414d1615c02eeedb9295d1f150c2dbe58d`.
- Fresh Debug and Release: **1,372/1,372 each**, zero skipped; both WGL tests actually loaded programs in both configurations.
- Test names: all 1,371 baseline names retained, plus only `arb_lower_structure`.
- Release outputs: all **32 recursive assembly artifacts** compared; 64 complete changed lines classified only as timestamps and exact command-line roots. All other bytes and frozen baseline hashes match.
- Negative replay: **15 fixtures** with identical full stdout, stderr and nonzero exit status.
- Four HLSL contracts passed, including **197 exact modern FXC combinations**.
- All **58 function bodies/signatures**, 24 private declarations, original type bytes, public headers, comments and state ownership preserved; facade **121 lines** against the fixed 160 ceiling.
- Evidence index: `.worktrees/arb-lower-modularization/build-cg20-arb-final/EVIDENCE.md`. Baseline and all per-task/full-run logs are retained in ignored build directories.

Tasks 1–6 received separate specification and quality reviews. The agent limit temporarily prevented delegation for Tasks 7–8; they were executed locally with the same red/green and exact-preservation checks, then independently reviewed after the next continuation restored agent availability. Final whole-branch specification and quality reviews approved both steps. The only correction was an extra EOF blank line after Task 3, fixed and re-reviewed in `02ae294`.

Execution used Debug then Release for the pristine baseline, leaving shared assembly paths frozen as Release. Final candidate comparison ran after Release and before Debug. The final Debug run subsequently overwrote shared assembly files; the preserved Release comparison log contains every original/replacement metadata line.

## Approved design and starting state

Design: `docs/superpowers/specs/2026-09-07-arb-lowering-modularization-design.md`.
Inventory was derived from commit `7774767b115522c4d9c21c9ee49c6471bd1f1a3f`.
Execute after GLSL is integrated. Pin a fresh baseline at that point; GLSL changes must not be silently attributed to this plan.
Reconcile the exact inventory against the execution baseline before editing if source changes land meanwhile.

Planning makes no compiler changes and does not claim new test results. The previously
verified inventory is 1,370 tests. Each plan adds one registered structural test:
GLSL expects 1,371 and subsequent ARB expects 1,372, provided the baseline inventory
has not changed independently. Compare names, not only totals.

## File structure

| File | Responsibility / original definition count |
|---|---|
| arb_lower.c | public facade (1) |
| arb_lower_support.c | support (15) |
| arb_lower_operand.c | operand (5) |
| arb_lower_expr.c | expr (16) |
| arb_lower_loop.c | loop (17) |
| arb_lower_stmt.c | stmt (4) |
| arb_lower_internal.h | existing types and 24 cross-module declarations |
| CMakeLists.txt | canonical CGC_ARB_LOWER_SOURCES and cgc consumption |
| tests/CMakeLists.txt | structural registration; keep IR-only and smoke targets independent |
| tests/check_arb_lower_structure.cmake | module/build/public and private ownership guard |
| tests/check_arb_lower_structure_selftest.cmake | positive/negative checker fixtures |

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
git worktree add --detach .worktrees/arb-lower-pristine $baseCommit
if ($LASTEXITCODE -ne 0) { throw 'baseline worktree failed' }
git worktree add .worktrees/arb-lower-modularization -b codex/arb-lower-modularization $baseCommit
if ($LASTEXITCODE -ne 0) { throw 'candidate worktree failed' }
```

Record the full baseline SHA. Preserve user changes in the main checkout.

- [x] **Step 2: Build the pristine baseline in both configurations.**
Run inside `.worktrees/arb-lower-pristine`.

```powershell
cmake -S . -B build-cg20-arb-baseline -A x64 -DBUILD_TESTING=ON -DCGC_REQUIRE_FXC=ON
if ($LASTEXITCODE -ne 0) { throw 'configure failed' }
Select-String -Path build-cg20-arb-baseline/CMakeCache.txt -Pattern 'GLSLANG_VALIDATOR:FILEPATH|FXC_EXECUTABLE:FILEPATH'
cmake --build build-cg20-arb-baseline --config Release
if ($LASTEXITCODE -ne 0) { throw 'Release build failed' }
ctest --test-dir build-cg20-arb-baseline -C Release --output-on-failure
if ($LASTEXITCODE -ne 0) { throw 'Release baseline failed' }
cmake --build build-cg20-arb-baseline --config Debug
if ($LASTEXITCODE -ne 0) { throw 'Debug build failed' }
ctest --test-dir build-cg20-arb-baseline -C Debug --output-on-failure
if ($LASTEXITCODE -ne 0) { throw 'Debug baseline failed' }
```

Expected: all registered tests pass. Require actual glslangValidator and FXC paths,
not NOTFOUND. Record any ARB WGL skip (return 77) separately from a pass. Do not
claim driver validation for a skipped smoke test.

- [x] **Step 3: Snapshot test inventory and artifacts.**
Keep the baseline build untouched afterward. Read CTest JSON with
`ctest --test-dir build-cg20-arb-baseline -C Release --show-only=json-v1`.
Record sorted test names, commands, validator locations and complete artifact paths.
ARB fixtures write tests/*.arb, shared between configurations; capture Release artifacts before Debug can overwrite them, or rerun pristine Release after Debug and before comparison.
Do not copy compiler binaries or generated outputs into tracked source.

### Task 2: Install complete private interface and initial canonical wiring

**Files:** Modify `arb_lower.c`, `CMakeLists.txt`; create `arb_lower_internal.h`.

- [x] **Step 1: Move the original private types into the following header.**
Copy the complete original NVIDIA notice verbatim above the include guard; the
comment below is an instruction, not replacement license text. Standard library
includes remain before the private header in every C translation unit.

```c
/* Copy the complete original NVIDIA notice before this guard. */
#ifndef __ARB_LOWER_INTERNAL_H
#define __ARB_LOWER_INTERNAL_H

#include "slglobals.h"
#include "cg_stdlib.h"
#include "arb_ir.h"

typedef struct ConsumedStmt_Rec {
    struct ConsumedStmt_Rec *next;
    stmt *stmt;
} ConsumedStmt;

typedef struct LoopInit_Rec {
    struct LoopInit_Rec *next;
    stmt *loop;
    Symbol *symbol;
    int value;
} LoopInit;

typedef struct ArbLowerContext_Rec {
    ArbProgram *ir;
    const ArbProfileDesc *profile;
    Symbol *program;
    struct ArbStaticValue_Rec *staticValues;
    int dstSelValid;        // Target selection order recorded by LowerLValue
    int dstSelWidth;
    int dstSel[4];
    ConsumedStmt *consumedHead; // Statements claimed by loop analysis
    int attrRegno[16];          // Seen vertex attribute registers
    int attrName[16];           // Binding spelling atom per register
    int attrCount;
    LoopInit *loopInitHead;     // Paired while/do initializers
} ArbLowerContext;

#define ARB_MAX_UNROLL 256

typedef struct ArbStaticValue_Rec {
    struct ArbStaticValue_Rec *next;
    Symbol *symbol;
    int value;
} ArbStaticValue;

int ArbLowerMaskFromType(Type *fType);
int ArbLowerGetSymbolTemp(ArbLowerContext *ctx, Symbol *symbol);
int ArbLowerGetSymbolTempBlock(ArbLowerContext *ctx, Symbol *symbol, int *count);
void ArbLowerClearSymbolTemps(ArbLowerContext *ctx);
void ArbLowerTrackSymbolTemp(ArbLowerContext *ctx, Symbol *symbol);
int ArbLowerStaticFind(const ArbLowerContext *ctx, Symbol *symbol, int *value);
void ArbLowerMarkConsumedStmt(ArbLowerContext *ctx, stmt *fStmt);
int ArbLowerIsConsumedStmt(ArbLowerContext *ctx, stmt *fStmt);
int ArbLowerUniformQuadBase(Symbol *symb);
int ArbLowerConnectorMember(ArbLowerContext *ctx, expr *expression,
                                ArbOperand *operand);
int ArbLowerLValue(ArbLowerContext *ctx, expr *expression,
                       ArbOperand *operand, int *mask);
void ArbLowerPairWhileInitializers(ArbLowerContext *ctx, stmt *list);
int ArbLowerCanonicalFor(ArbLowerContext *ctx, stmt *fStmt);
int ArbLowerCanonicalWhileDo(ArbLowerContext *ctx, stmt *fStmt,
                                 int testFirst);
int ArbLowerStatement(ArbLowerContext *ctx, stmt *statement);
ArbOperand ArbLowerSmearOperand(ArbOperand operand);
int ArbLowerEmitBinary(ArbLowerContext *ctx, ArbOpcode opcode, int mask,
                      const SourceLoc *loc, ArbOperand a, ArbOperand b,
                      ArbOperand *result);
int ArbLowerEmitUnary(ArbLowerContext *ctx, ArbOpcode opcode, int mask,
                     const SourceLoc *loc, ArbOperand a, ArbOperand *result);
int ArbLowerBuildSelect(ArbLowerContext *ctx, const SourceLoc *loc,
                       ArbOperand cond, ArbOperand tv, ArbOperand fv,
                       int width, int mask, ArbOperand *result);
int ArbLowerTypeWidth(Type *fType);
int ArbLowerWidthMask(int width);
void ArbLowerComposeSwizzledSource(ArbOperand *operand, const int *selection,
                                  int width);
int ArbLowerSwizzleNode(ArbLowerContext *ctx, expr *expression,
                            ArbOperand *operand);
int ArbLowerExpression(ArbLowerContext *ctx, expr *expression,
                           ArbOperand *operand);

#endif
```

The moved block includes ConsumedStmt, LoopInit, ArbLowerContext, ARB_MAX_UNROLL and ArbStaticValue. Keep DotTerm local to expression lowering.

- [x] **Step 2: Establish linkage before extraction.**
For precisely the declarations above, remove `static` from definitions and any
retained matching forward declarations, even while their bodies still reside in
the facade. Remove redundant cross-module forwards now supplied by the header.
Preserve needed static forwards until their owning module is moved.
Apply the exact token mapping below to declarations, definitions and every call; do not rename fields, strings or unrelated compiler functions.

```text
MaskFromType -> ArbLowerMaskFromType
GetSymbolTemp -> ArbLowerGetSymbolTemp
GetSymbolTempBlock -> ArbLowerGetSymbolTempBlock
ClearSymbolTemps -> ArbLowerClearSymbolTemps
TrackSymbolTemp -> ArbLowerTrackSymbolTemp
StaticFind -> ArbLowerStaticFind
MarkConsumedStmt -> ArbLowerMarkConsumedStmt
IsConsumedStmt -> ArbLowerIsConsumedStmt
UniformQuadBase -> ArbLowerUniformQuadBase
LowerConnectorMember -> ArbLowerConnectorMember
LowerLValue -> ArbLowerLValue
PairWhileInitializers -> ArbLowerPairWhileInitializers
LowerCanonicalFor -> ArbLowerCanonicalFor
LowerCanonicalWhileDo -> ArbLowerCanonicalWhileDo
LowerStatement -> ArbLowerStatement
SmearOperand -> ArbLowerSmearOperand
EmitBinary -> ArbLowerEmitBinary
EmitUnary -> ArbLowerEmitUnary
BuildSelect -> ArbLowerBuildSelect
TypeWidth -> ArbLowerTypeWidth
WidthMask -> ArbLowerWidthMask
ComposeSwizzledSource -> ArbLowerComposeSwizzledSource
LowerSwizzleNode -> ArbLowerSwizzleNode
LowerExpression -> ArbLowerExpression
```

This preparation allows calls in either direction during all intermediate
extractions. The final API is based on the caller manifest in Appendix A.

Replace the existing project header includes in the facade with:
```c
#include "arb_lower_internal.h"
```
Preserve the existing standard-library includes and their order. Remove only the
private type block moved to the header.

- [x] **Step 3: Wire the initial one-file canonical source list.**
Before `add_subdirectory(tests)` in root CMake:
```cmake
set(CGC_ARB_LOWER_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/arb_lower.c
)
```
Replace the direct `arb_lower.c` source in `cgc` with:
```cmake
    ${CGC_ARB_LOWER_SOURCES}
```
Keep `arb_ir_tests` and `arb_smoke` unchanged: neither compiles the lowerer.

- [x] **Step 4: Configure/build/test the candidate.**
Run from `.worktrees/arb-lower-modularization`.
```powershell
cmake -S . -B build-cg20-arb-candidate -A x64 -DBUILD_TESTING=ON -DCGC_REQUIRE_FXC=ON
if ($LASTEXITCODE -ne 0) { throw 'configure failed' }
cmake --build build-cg20-arb-candidate --config Debug --target cgc arb_ir_tests arb_smoke
if ($LASTEXITCODE -ne 0) { throw 'Debug failed' }
cmake --build build-cg20-arb-candidate --config Release --target cgc arb_ir_tests arb_smoke
if ($LASTEXITCODE -ne 0) { throw 'Release failed' }
ctest --test-dir build-cg20-arb-candidate -C Release -R "(^arb|^generic_)" --output-on-failure
if ($LASTEXITCODE -ne 0) { throw 'backend tests failed' }
git diff --check
```
Expected: build/link success and all selected tests pass, with nonzero selected
inventory. Audit that every promoted name is unique across repository definitions.

- [x] **Step 5: Commit only the preparation files.**
```powershell
git add arb_lower.c arb_lower_internal.h CMakeLists.txt
git commit -m "Prepare ARB lowering module interfaces"
```

### Task 3: Extract support lowering

**Files:** Create `arb_lower_support.c`; modify `arb_lower.c` and `CMakeLists.txt`.

- [x] **Step 1: Move this exact ordered inventory.**
Move complete definitions and attached comments from the original source, preserving
relative order. The line numbers refer to the pinned planning baseline and are
navigation hints; function names are authoritative.

```text
MaskFromType (now ArbLowerMaskFromType) — original line 111
GetSymbolTemp (now ArbLowerGetSymbolTemp) — original line 131
GetSymbolTempBlock (now ArbLowerGetSymbolTempBlock) — original line 144
ClearSymbolTemps (now ArbLowerClearSymbolTemps) — original line 179
TrackSymbolTemp (now ArbLowerTrackSymbolTemp) — original line 199
MarkConsumedStmt (now ArbLowerMarkConsumedStmt) — original line 367
IsConsumedStmt (now ArbLowerIsConsumedStmt) — original line 377
SmearOperand (now ArbLowerSmearOperand) — original line 1591
EmitBinary (now ArbLowerEmitBinary) — original line 1605
EmitUnary (now ArbLowerEmitUnary) — original line 1628
BuildSelect (now ArbLowerBuildSelect) — original line 1653
TypeWidth (now ArbLowerTypeWidth) — original line 1783
WidthMask (now ArbLowerWidthMask) — original line 1792
IsIdentitySwizzleLower — original line 1801
ComposeSwizzledSource (now ArbLowerComposeSwizzledSource) — original line 1812
```

Create the module with its complete original license and this include block:
```c
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "arb_lower_internal.h"
```
Keep the private API linkage established in Task 2. Move any required static
forward declarations with their owning module.



- [x] **Step 2: Append the new file to the canonical list.**
The list after this task must be exactly:
```cmake
set(CGC_ARB_LOWER_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/arb_lower.c
    ${CMAKE_CURRENT_SOURCE_DIR}/arb_lower_support.c
)
```

- [x] **Step 3: Audit extraction before building.**
Compare the moved bodies against the pinned baseline and the prior commit. Allow
only the recorded ARB identifier mapping and linkage changes.
Require each listed definition exactly once in its new owner, zero remaining
copies, and no changes to remaining bodies. Preserve all file/function-static
state and comments. A link failure is a missing dependency to reconcile against
Appendix A, not permission to change behavior.

- [x] **Step 4: Build both configurations and run backend coverage.**
```powershell
cmake -S . -B build-cg20-arb-candidate -A x64 -DBUILD_TESTING=ON -DCGC_REQUIRE_FXC=ON
if ($LASTEXITCODE -ne 0) { throw 'configure failed' }
cmake --build build-cg20-arb-candidate --config Debug --target cgc arb_ir_tests arb_smoke
if ($LASTEXITCODE -ne 0) { throw 'Debug failed' }
cmake --build build-cg20-arb-candidate --config Release --target cgc arb_ir_tests arb_smoke
if ($LASTEXITCODE -ne 0) { throw 'Release failed' }
ctest --test-dir build-cg20-arb-candidate -C Release -R "(^arb|^generic_)" --output-on-failure
if ($LASTEXITCODE -ne 0) { throw 'backend tests failed' }
git diff --check
```
Expected: all selected tests pass. Compare generated backend outputs with the
pristine baseline using Task 9's complete-set procedure. Run only
one configuration's ARB tests at a time because assembly artifact paths are shared.

- [x] **Step 5: Commit this extraction.**
```powershell
git add arb_lower.c arb_lower_support.c CMakeLists.txt
git commit -m "Extract ARB support lowering"
```

### Task 4: Extract operand lowering

**Files:** Create `arb_lower_operand.c`; modify `arb_lower.c` and `CMakeLists.txt`.

- [x] **Step 1: Move this exact ordered inventory.**
Move complete definitions and attached comments from the original source, preserving
relative order. The line numbers refer to the pinned planning baseline and are
navigation hints; function names are authoritative.

```text
ResolveMemberBinding — original line 691
UniformQuadBase (now ArbLowerUniformQuadBase) — original line 714
LowerConnectorMember (now ArbLowerConnectorMember) — original line 899
LowerLValue (now ArbLowerLValue) — original line 1012
LowerSwizzleNode (now ArbLowerSwizzleNode) — original line 1831
```

Create the module with its complete original license and this include block:
```c
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "arb_lower_internal.h"
```
Keep the private API linkage established in Task 2. Move any required static
forward declarations with their owning module.



- [x] **Step 2: Append the new file to the canonical list.**
The list after this task must be exactly:
```cmake
set(CGC_ARB_LOWER_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/arb_lower.c
    ${CMAKE_CURRENT_SOURCE_DIR}/arb_lower_support.c
    ${CMAKE_CURRENT_SOURCE_DIR}/arb_lower_operand.c
)
```

- [x] **Step 3: Audit extraction before building.**
Compare the moved bodies against the pinned baseline and the prior commit. Allow
only the recorded ARB identifier mapping and linkage changes.
Require each listed definition exactly once in its new owner, zero remaining
copies, and no changes to remaining bodies. Preserve all file/function-static
state and comments. A link failure is a missing dependency to reconcile against
Appendix A, not permission to change behavior.

- [x] **Step 4: Build both configurations and run backend coverage.**
```powershell
cmake -S . -B build-cg20-arb-candidate -A x64 -DBUILD_TESTING=ON -DCGC_REQUIRE_FXC=ON
if ($LASTEXITCODE -ne 0) { throw 'configure failed' }
cmake --build build-cg20-arb-candidate --config Debug --target cgc arb_ir_tests arb_smoke
if ($LASTEXITCODE -ne 0) { throw 'Debug failed' }
cmake --build build-cg20-arb-candidate --config Release --target cgc arb_ir_tests arb_smoke
if ($LASTEXITCODE -ne 0) { throw 'Release failed' }
ctest --test-dir build-cg20-arb-candidate -C Release -R "(^arb|^generic_)" --output-on-failure
if ($LASTEXITCODE -ne 0) { throw 'backend tests failed' }
git diff --check
```
Expected: all selected tests pass. Compare generated backend outputs with the
pristine baseline using Task 9's complete-set procedure. Run only
one configuration's ARB tests at a time because assembly artifact paths are shared.

- [x] **Step 5: Commit this extraction.**
```powershell
git add arb_lower.c arb_lower_operand.c CMakeLists.txt
git commit -m "Extract ARB operand lowering"
```

### Task 5: Extract expr lowering

**Files:** Create `arb_lower_expr.c`; modify `arb_lower.c` and `CMakeLists.txt`.

- [x] **Step 1: Move this exact ordered inventory.**
Move complete definitions and attached comments from the original source, preserving
relative order. The line numbers refer to the pinned planning baseline and are
navigation hints; function names are authoritative.

```text
ExprSame — original line 736
SwizzleComponentLeaf — original line 765
IsMulFamily — original line 785
IsAddFamily — original line 795
DetectDotTerm — original line 815
DetectDotTermList — original line 837
DetectDotChain — original line 862
LowerConstantNode — original line 1732
LowerVectorConstructor — original line 1863
LowerBuiltinCall — original line 1903
LowerBooleanOrComparison — original line 2114
LowerArithmeticBinary — original line 2240
SethiUllmanCost — original line 2339
IsCommutativeOpcode — original line 2363
RejectUnsupportedBinary — original line 2377
LowerExpression (now ArbLowerExpression) — original line 2401
```

Create the module with its complete original license and this include block:
```c
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "arb_lower_internal.h"
```
Keep the private API linkage established in Task 2. Move any required static
forward declarations with their owning module. Move the exact DotTerm typedef before DetectDotTerm; preserve its associated comments.



- [x] **Step 2: Append the new file to the canonical list.**
The list after this task must be exactly:
```cmake
set(CGC_ARB_LOWER_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/arb_lower.c
    ${CMAKE_CURRENT_SOURCE_DIR}/arb_lower_support.c
    ${CMAKE_CURRENT_SOURCE_DIR}/arb_lower_operand.c
    ${CMAKE_CURRENT_SOURCE_DIR}/arb_lower_expr.c
)
```

- [x] **Step 3: Audit extraction before building.**
Compare the moved bodies against the pinned baseline and the prior commit. Allow
only the recorded ARB identifier mapping and linkage changes.
Require each listed definition exactly once in its new owner, zero remaining
copies, and no changes to remaining bodies. Preserve all file/function-static
state and comments. A link failure is a missing dependency to reconcile against
Appendix A, not permission to change behavior.

- [x] **Step 4: Build both configurations and run backend coverage.**
```powershell
cmake -S . -B build-cg20-arb-candidate -A x64 -DBUILD_TESTING=ON -DCGC_REQUIRE_FXC=ON
if ($LASTEXITCODE -ne 0) { throw 'configure failed' }
cmake --build build-cg20-arb-candidate --config Debug --target cgc arb_ir_tests arb_smoke
if ($LASTEXITCODE -ne 0) { throw 'Debug failed' }
cmake --build build-cg20-arb-candidate --config Release --target cgc arb_ir_tests arb_smoke
if ($LASTEXITCODE -ne 0) { throw 'Release failed' }
ctest --test-dir build-cg20-arb-candidate -C Release -R "(^arb|^generic_)" --output-on-failure
if ($LASTEXITCODE -ne 0) { throw 'backend tests failed' }
git diff --check
```
Expected: all selected tests pass. Compare generated backend outputs with the
pristine baseline using Task 9's complete-set procedure. Run only
one configuration's ARB tests at a time because assembly artifact paths are shared.

- [x] **Step 5: Commit this extraction.**
```powershell
git add arb_lower.c arb_lower_expr.c CMakeLists.txt
git commit -m "Extract ARB expr lowering"
```

### Task 6: Extract loop lowering

**Files:** Create `arb_lower_loop.c`; modify `arb_lower.c` and `CMakeLists.txt`.

- [x] **Step 1: Move this exact ordered inventory.**
Move complete definitions and attached comments from the original source, preserving
relative order. The line numbers refer to the pinned planning baseline and are
navigation hints; function names are authoritative.

```text
StaticPush — original line 219
StaticPop — original line 230
StaticFind (now ArbLowerStaticFind) — original line 239
EvalConstInt — original line 259
TrailingUpdateScan — original line 401
ExtractSimpleDelta — original line 443
ExtractStepDelta — original line 496
AssignedLocalInt — original line 536
CanonicalLoopCondition — original line 554
ConditionHolds — original line 620
StepProvesTermination — original line 638
SimulateLoopIterations — original line 663
PairWhileInitializers (now ArbLowerPairWhileInitializers) — original line 1298
FindLoopInit — original line 1347
LowerLoopBody — original line 1363
LowerCanonicalFor (now ArbLowerCanonicalFor) — original line 1384
LowerCanonicalWhileDo (now ArbLowerCanonicalWhileDo) — original line 1453
```

Create the module with its complete original license and this include block:
```c
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "arb_lower_internal.h"
```
Keep the private API linkage established in Task 2. Move any required static
forward declarations with their owning module.



- [x] **Step 2: Append the new file to the canonical list.**
The list after this task must be exactly:
```cmake
set(CGC_ARB_LOWER_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/arb_lower.c
    ${CMAKE_CURRENT_SOURCE_DIR}/arb_lower_support.c
    ${CMAKE_CURRENT_SOURCE_DIR}/arb_lower_operand.c
    ${CMAKE_CURRENT_SOURCE_DIR}/arb_lower_expr.c
    ${CMAKE_CURRENT_SOURCE_DIR}/arb_lower_loop.c
)
```

- [x] **Step 3: Audit extraction before building.**
Compare the moved bodies against the pinned baseline and the prior commit. Allow
only the recorded ARB identifier mapping and linkage changes.
Require each listed definition exactly once in its new owner, zero remaining
copies, and no changes to remaining bodies. Preserve all file/function-static
state and comments. A link failure is a missing dependency to reconcile against
Appendix A, not permission to change behavior.

- [x] **Step 4: Build both configurations and run backend coverage.**
```powershell
cmake -S . -B build-cg20-arb-candidate -A x64 -DBUILD_TESTING=ON -DCGC_REQUIRE_FXC=ON
if ($LASTEXITCODE -ne 0) { throw 'configure failed' }
cmake --build build-cg20-arb-candidate --config Debug --target cgc arb_ir_tests arb_smoke
if ($LASTEXITCODE -ne 0) { throw 'Debug failed' }
cmake --build build-cg20-arb-candidate --config Release --target cgc arb_ir_tests arb_smoke
if ($LASTEXITCODE -ne 0) { throw 'Release failed' }
ctest --test-dir build-cg20-arb-candidate -C Release -R "(^arb|^generic_)" --output-on-failure
if ($LASTEXITCODE -ne 0) { throw 'backend tests failed' }
git diff --check
```
Expected: all selected tests pass. Compare generated backend outputs with the
pristine baseline using Task 9's complete-set procedure. Run only
one configuration's ARB tests at a time because assembly artifact paths are shared.

- [x] **Step 5: Commit this extraction.**
```powershell
git add arb_lower.c arb_lower_loop.c CMakeLists.txt
git commit -m "Extract ARB loop lowering"
```

### Task 7: Extract stmt lowering

**Files:** Create `arb_lower_stmt.c`; modify `arb_lower.c` and `CMakeLists.txt`.

- [x] **Step 1: Move this exact ordered inventory.**
Move complete definitions and attached comments from the original source, preserving
relative order. The line numbers refer to the pinned planning baseline and are
navigation hints; function names are authoritative.

```text
SameOperand — original line 1090
LowerAssignment — original line 1111
LowerExpressionStmt — original line 1209
LowerStatement (now ArbLowerStatement) — original line 1520
```

Create the module with its complete original license and this include block:
```c
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "arb_lower_internal.h"
```
Keep the private API linkage established in Task 2. Move any required static
forward declarations with their owning module.



- [x] **Step 2: Append the new file to the canonical list.**
The list after this task must be exactly:
```cmake
set(CGC_ARB_LOWER_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/arb_lower.c
    ${CMAKE_CURRENT_SOURCE_DIR}/arb_lower_support.c
    ${CMAKE_CURRENT_SOURCE_DIR}/arb_lower_operand.c
    ${CMAKE_CURRENT_SOURCE_DIR}/arb_lower_expr.c
    ${CMAKE_CURRENT_SOURCE_DIR}/arb_lower_loop.c
    ${CMAKE_CURRENT_SOURCE_DIR}/arb_lower_stmt.c
)
```

- [x] **Step 3: Audit extraction before building.**
Compare the moved bodies against the pinned baseline and the prior commit. Allow
only the recorded ARB identifier mapping and linkage changes.
Require each listed definition exactly once in its new owner, zero remaining
copies, and no changes to remaining bodies. Preserve all file/function-static
state and comments. A link failure is a missing dependency to reconcile against
Appendix A, not permission to change behavior.

- [x] **Step 4: Build both configurations and run backend coverage.**
```powershell
cmake -S . -B build-cg20-arb-candidate -A x64 -DBUILD_TESTING=ON -DCGC_REQUIRE_FXC=ON
if ($LASTEXITCODE -ne 0) { throw 'configure failed' }
cmake --build build-cg20-arb-candidate --config Debug --target cgc arb_ir_tests arb_smoke
if ($LASTEXITCODE -ne 0) { throw 'Debug failed' }
cmake --build build-cg20-arb-candidate --config Release --target cgc arb_ir_tests arb_smoke
if ($LASTEXITCODE -ne 0) { throw 'Release failed' }
ctest --test-dir build-cg20-arb-candidate -C Release -R "(^arb|^generic_)" --output-on-failure
if ($LASTEXITCODE -ne 0) { throw 'backend tests failed' }
git diff --check
```
Expected: all selected tests pass. Compare generated backend outputs with the
pristine baseline using Task 9's complete-set procedure. Run only
one configuration's ARB tests at a time because assembly artifact paths are shared.

- [x] **Step 5: Commit this extraction.**
```powershell
git add arb_lower.c arb_lower_stmt.c CMakeLists.txt
git commit -m "Extract ARB stmt lowering"
```

### Task 8: Enforce the final facade and structural contract

**Files:** Modify `arb_lower.c` and `tests/CMakeLists.txt`;
create `tests/check_arb_lower_structure.cmake` and
`tests/check_arb_lower_structure_selftest.cmake`.

- [x] **Step 1: Audit the facade.**
Keep only the complete original license, standard includes, private include,
and `ArbLowerProgram`.
Remove obsolete private forwards and section banners left by extraction.
Public function bodies retain their exact original contents, except the recorded private call-name substitutions.
The fixed ceiling is 160 lines; existing entry bodies and the license fit within it.

- [x] **Step 2: Add this complete structural checker.**
It follows the HLSL checker without changing the HLSL test. Test-double exceptions
apply only to public-definition checks; production private-header leaks still fail.

```cmake
cmake_minimum_required(VERSION 3.16)

if(NOT DEFINED SOURCE_ROOT OR SOURCE_ROOT STREQUAL "")
    message(FATAL_ERROR "SOURCE_ROOT is required")
endif()

set(lower_header
    arb_lower_internal.h)
set(lower_sources
    arb_lower.c
    arb_lower_support.c
    arb_lower_operand.c
    arb_lower_expr.c
    arb_lower_loop.c
    arb_lower_stmt.c)

foreach(relative IN LISTS lower_header lower_sources)
    if(NOT EXISTS "${SOURCE_ROOT}/${relative}")
        message(FATAL_ERROR "missing ARB lowering file ${relative}")
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
            "^[ \t]*#[ \t]*include[ \t]*\"arb_lower_internal\\.h\""
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
            "${relative} must include arb_lower_internal.h")
    endif()
endforeach()

file(STRINGS "${SOURCE_ROOT}/arb_lower.c" facade_lines)
list(LENGTH facade_lines facade_line_count)
if(facade_line_count GREATER 160)
    message(FATAL_ERROR
        "arb_lower.c remains a monolith: ${facade_line_count} lines")
endif()

file(READ "${SOURCE_ROOT}/CMakeLists.txt" root_cmake)
set(source_list_pattern
    "set[ \t\r\n]*\\([ \t\r\n]*CGC_ARB_LOWER_SOURCES[ \t\r\n]+([^)]*)\\)")
string(REGEX MATCH "${source_list_pattern}" source_list_match "${root_cmake}")
if(source_list_match STREQUAL "")
    message(FATAL_ERROR "CGC_ARB_LOWER_SOURCES is not defined")
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
        "CGC_ARB_LOWER_SOURCES must contain exactly the canonical lowering sources")
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
            "CGC_ARB_LOWER_SOURCES must contain ${relative} exactly once")
    endif()
endforeach()
foreach(entry IN LISTS source_list_entries)
    NormalizeLowerSourceEntry("${entry}" normalized_entry)
    list(FIND canonical_entries "${normalized_entry}" lower_source_position)
    if(lower_source_position EQUAL -1)
        message(FATAL_ERROR
            "CGC_ARB_LOWER_SOURCES contains non-canonical entry ${entry}")
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
string(FIND "${cgc_target_body}" [=[${CGC_ARB_LOWER_SOURCES}]=]
    cgc_list_position)
if(cgc_list_position EQUAL -1)
    message(FATAL_ERROR
        "cgc does not use CGC_ARB_LOWER_SOURCES")
endif()
string(REGEX MATCH "arb_lower[^ \t\r\n)]*\\.c" direct_cgc_lower_source
    "${cgc_target_body}")
if(NOT direct_cgc_lower_source STREQUAL "")
    message(FATAL_ERROR
        "cgc directly lists ${direct_cgc_lower_source} instead of CGC_ARB_LOWER_SOURCES")
endif()

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
                "${relative} includes arb_lower_internal.h outside the lowering implementation")
        endif()
    endif()
endforeach()

set(public_entry_points
    ArbLowerProgram)
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
            if(NOT relative STREQUAL "arb_lower.c")
                message(FATAL_ERROR
                    "${relative} defines ${entry_point}; only arb_lower.c may define it")
            endif()
            list(APPEND "${entry_point}_definition_owners" "${relative}")
        endif()
    endforeach()
endforeach()
foreach(entry_point IN LISTS public_entry_points)
    list(LENGTH "${entry_point}_definition_owners" definition_owner_count)
    if(NOT definition_owner_count EQUAL 1)
        message(FATAL_ERROR
            "arb_lower.c must be the only definition owner of ${entry_point}")
    endif()
endforeach()
```

- [x] **Step 3: Add this complete checker self-test.**
```cmake
cmake_minimum_required(VERSION 3.16)
if(NOT DEFINED SOURCE_ROOT OR NOT DEFINED STRUCTURE_SCRIPT)
    message(FATAL_ERROR "SOURCE_ROOT and STRUCTURE_SCRIPT required")
endif()
set(fixture "${SOURCE_ROOT}/build-cg20-arb-structure-fixture")
file(MAKE_DIRECTORY "${fixture}/tests" "${fixture}/.worktrees/other")
file(WRITE "${fixture}/.worktrees/other/copy.c" "")
file(WRITE "${fixture}/unrelated.c" "")
file(WRITE "${fixture}/tests/stub.c" "")
file(WRITE "${fixture}/arb_lower_internal.h" "")
set(root_cmake [=[set(CGC_ARB_LOWER_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/arb_lower.c
    ${CMAKE_CURRENT_SOURCE_DIR}/arb_lower_support.c
    ${CMAKE_CURRENT_SOURCE_DIR}/arb_lower_operand.c
    ${CMAKE_CURRENT_SOURCE_DIR}/arb_lower_expr.c
    ${CMAKE_CURRENT_SOURCE_DIR}/arb_lower_loop.c
    ${CMAKE_CURRENT_SOURCE_DIR}/arb_lower_stmt.c
)
add_executable(cgc ${CGC_ARB_LOWER_SOURCES})
]=])
file(WRITE "${fixture}/CMakeLists.txt" "${root_cmake}")
file(WRITE "${fixture}/tests/CMakeLists.txt" [=[]=])
file(WRITE "${fixture}/arb_lower.c" [=[#include "arb_lower_internal.h"
int ArbLowerProgram(void) { return 0; }
]=])
file(WRITE "${fixture}/arb_lower_support.c" "#include \"arb_lower_internal.h\"\n")
file(WRITE "${fixture}/arb_lower_operand.c" "#include \"arb_lower_internal.h\"\n")
file(WRITE "${fixture}/arb_lower_expr.c" "#include \"arb_lower_internal.h\"\n")
file(WRITE "${fixture}/arb_lower_loop.c" "#include \"arb_lower_internal.h\"\n")
file(WRITE "${fixture}/arb_lower_stmt.c" "#include \"arb_lower_internal.h\"\n")
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
file(WRITE "${fixture}/unrelated.c" "#include \"arb_lower_internal.h\"\n")
Check("outside the lowering")
file(WRITE "${fixture}/unrelated.c" "int ArbLowerProgram(void) { return 0; }\n")
Check("only arb_lower.c may define it")
file(WRITE "${fixture}/unrelated.c" "")
file(WRITE "${fixture}/tests/stub.c" "#include \"arb_lower_internal.h\"\n")
Check("outside the lowering")
file(WRITE "${fixture}/tests/stub.c" "int ArbLowerProgram(void) { return 0; }\n")
Check("PASS")
file(WRITE "${fixture}/.worktrees/other/copy.c" [=[#include "arb_lower_internal.h"
int ArbLowerProgram(void) { return 0; }
]=])
Check("PASS")
string(REPLACE "arb_lower_support.c" "arb_lower.c" duplicate "${root_cmake}")
file(WRITE "${fixture}/CMakeLists.txt" "${duplicate}")
Check("exactly once")
string(REPLACE "\${CMAKE_CURRENT_SOURCE_DIR}/" "\${CMAKE_CURRENT_SOURCE_DIR}/wrong/" wrong "${root_cmake}")
file(WRITE "${fixture}/CMakeLists.txt" "${wrong}")
Check("exactly once")
string(REPLACE "add_executable(cgc " "add_executable(cgc arb_lower.c " bypass "${root_cmake}")
file(WRITE "${fixture}/CMakeLists.txt" "${bypass}")
Check("directly lists")
file(WRITE "${fixture}/CMakeLists.txt" "${root_cmake}")
file(WRITE "${fixture}/arb_lower_support.c" "")
Check("must include")
file(WRITE "${fixture}/arb_lower_support.c" "#include \"arb_lower_internal.h\"\n")
Check("PASS")
```

- [x] **Step 4: Run the checker and its negative fixtures.**
```powershell
$root = (Resolve-Path '.').Path
$guard = (Resolve-Path 'tests/check_arb_lower_structure.cmake').Path
cmake "-DSOURCE_ROOT=$root" "-DSTRUCTURE_SCRIPT=$guard" -P tests/check_arb_lower_structure_selftest.cmake
if ($LASTEXITCODE -ne 0) { throw 'checker self-test failed' }
cmake "-DSOURCE_ROOT=$root" -P tests/check_arb_lower_structure.cmake
if ($LASTEXITCODE -ne 0) { throw 'real structure failed' }
```
Expected: both exit zero; negative fixtures internally prove rejection. Do not
weaken guards to accommodate a failed extraction.

- [x] **Step 5: Register exactly one test in tests/CMakeLists.txt.**
Place near arb_ir_unit:
```cmake
add_test(
    NAME arb_lower_structure
    COMMAND ${CMAKE_COMMAND}
        -DSOURCE_ROOT=${PROJECT_SOURCE_DIR}
        -P ${CMAKE_CURRENT_SOURCE_DIR}/check_arb_lower_structure.cmake
)
```
```powershell
cmake -S . -B build-cg20-arb-candidate -A x64 -DBUILD_TESTING=ON -DCGC_REQUIRE_FXC=ON
if ($LASTEXITCODE -ne 0) { throw 'configure failed' }
ctest --test-dir build-cg20-arb-candidate -C Release -R "^arb_lower_structure$" --output-on-failure
if ($LASTEXITCODE -ne 0) { throw 'registered structure failed' }
git diff --check
git add arb_lower.c tests/CMakeLists.txt tests/check_arb_lower_structure.cmake tests/check_arb_lower_structure_selftest.cmake
git commit -m "Enforce ARB lowering boundaries"
```
Expected: 1/1 structural test passes. Audit context/type single ownership,
no new mutable globals, and unchanged public headers separately from the lexical guard.

### Task 9: Final equivalence qualification

**Files:** Verification artifacts only. Do not edit expected outputs.

- [x] **Step 1: Fresh candidate configuration and full Release qualification.**
```powershell
cmake -S . -B build-cg20-arb-final -A x64 -DBUILD_TESTING=ON -DCGC_REQUIRE_FXC=ON
if ($LASTEXITCODE -ne 0) { throw 'configure failed' }
cmake --build build-cg20-arb-final --config Release
if ($LASTEXITCODE -ne 0) { throw 'Release build failed' }
ctest --test-dir build-cg20-arb-final -C Release --output-on-failure
if ($LASTEXITCODE -ne 0) { throw 'Release suite failed' }
```
Expected: baseline test names retained, exactly one additional structural test.
Do not count skipped OpenGL checks as successful driver validation.

- [x] **Step 2: Compare complete Release artifact sets before Debug writes them.**
Ensure pristine Release ran last: ARB test artifacts in tests/*.arb are shared between configurations and Debug may overwrite them. Run this PowerShell from the candidate root. Baseline is the untouched sibling
created by Task 1.

```powershell
$baselineRoot = (Resolve-Path '../arb-lower-pristine/build-cg20-arb-baseline/tests').Path
$candidateRoot = (Resolve-Path 'build-cg20-arb-final/tests').Path
function ArtifactManifest([string]$artifactRoot) {
    Get-ChildItem -LiteralPath $artifactRoot -Recurse -File -Filter '*.arb' |
        Where-Object { $_.DirectoryName -eq $artifactRoot } |
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
$baselineExe = (Resolve-Path '../arb-lower-pristine/build-cg20-arb-baseline/Release/cgc.exe').Path
$candidateExe = (Resolve-Path 'build-cg20-arb-final/Release/cgc.exe').Path
$registry = ctest --test-dir build-cg20-arb-final -C Release --show-only=json-v1 | ConvertFrom-Json
if ($LASTEXITCODE -ne 0) { throw "test inventory failed" }
$outputPath = Join-Path (Resolve-Path 'build-cg20-arb-final').Path 'diagnostic-replay.out'
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
    if ($defs.PROFILE -notmatch "^arb" -or $defs.EXPECT_SUCCESS -ne "FALSE" -or -not $defs.SOURCE) { continue }
    $arguments = @("-quiet", "-profile", $defs.PROFILE)
    if ($defs.ENTRY) { $arguments += @("-entry", $defs.ENTRY) }
    else { $arguments += @("-entry", "main") }
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
cmake --build build-cg20-arb-final --config Debug
if ($LASTEXITCODE -ne 0) { throw 'Debug build failed' }
ctest --test-dir build-cg20-arb-final -C Debug --output-on-failure
if ($LASTEXITCODE -ne 0) { throw 'Debug suite failed' }
ctest --test-dir build-cg20-arb-final -C Release -R "^(hlsl_compatibility_matrix|hlsl_sm4_sm5_compatibility_matrix|hlsl_modern_fxc_coverage|hlsl_validation_exact_target_contract)$" --output-on-failure
if ($LASTEXITCODE -ne 0) { throw 'HLSL compatibility regression' }
```
Expected: all tests pass and four HLSL contracts remain green, including the
197-combination modern FXC contract unless independently expanded.

- [x] **Step 5: Final review.**
Verify all 58 baseline definitions have exactly one owner; function bodies,
private type fields, static storage and phase order match the baseline.
Public arb_ir.h and arb_hal.h must be byte-identical to baseline.
Run:
```powershell
git diff --check
git status --short
git log -12 --oneline
```
Expected: only focused committed work and ignored builds; user changes remain
untouched. Perform the final review against the approved design before integration.
- [x] **Step 6: Local integration, only after explicit user approval.**

On the merged main checkout, rerun the structural self-test, registered structural
test and full suite. Keep nested worktrees present during the structural check to
exercise the integration environment. Do not push or merge merely because this
planning document exists; execution/integration follow the user's active request.

## Appendix A: Caller-derived cross-module API

Exclude forward declarations when counting callers. The table uses final names;
Task 2 contains the complete old/new token map.

| Private helper | Owner | Callers in other modules |
|---|---|---|
| ArbLowerMaskFromType | support | ArbLowerLValue (operand), LowerExpressionStmt (stmt) |
| ArbLowerGetSymbolTemp | support | ArbLowerConnectorMember (operand), ArbLowerLValue (operand) |
| ArbLowerGetSymbolTempBlock | support | ArbLowerConnectorMember (operand) |
| ArbLowerClearSymbolTemps | support | ArbLowerProgram (facade) |
| ArbLowerTrackSymbolTemp | support | ArbLowerConnectorMember (operand), ArbLowerLValue (operand) |
| ArbLowerStaticFind | loop | ArbLowerExpression (expr) |
| ArbLowerMarkConsumedStmt | support | ArbLowerPairWhileInitializers (loop) |
| ArbLowerIsConsumedStmt | support | ArbLowerStatement (stmt) |
| ArbLowerUniformQuadBase | operand | ArbLowerExpression (expr) |
| ArbLowerConnectorMember | operand | ArbLowerExpression (expr) |
| ArbLowerLValue | operand | LowerAssignment (stmt), LowerExpressionStmt (stmt) |
| ArbLowerPairWhileInitializers | loop | ArbLowerStatement (stmt) |
| ArbLowerCanonicalFor | loop | ArbLowerStatement (stmt) |
| ArbLowerCanonicalWhileDo | loop | ArbLowerStatement (stmt) |
| ArbLowerStatement | stmt | LowerLoopBody (loop), ArbLowerProgram (facade) |
| ArbLowerSmearOperand | support | LowerVectorConstructor (expr), LowerBuiltinCall (expr), LowerBooleanOrComparison (expr), LowerArithmeticBinary (expr), ArbLowerExpression (expr) |
| ArbLowerEmitBinary | support | LowerBooleanOrComparison (expr), LowerArithmeticBinary (expr), ArbLowerExpression (expr) |
| ArbLowerEmitUnary | support | LowerBuiltinCall (expr), LowerArithmeticBinary (expr) |
| ArbLowerBuildSelect | support | LowerExpressionStmt (stmt), ArbLowerExpression (expr) |
| ArbLowerTypeWidth | support | LowerExpressionStmt (stmt), LowerBooleanOrComparison (expr), LowerArithmeticBinary (expr), ArbLowerExpression (expr) |
| ArbLowerWidthMask | support | LowerBooleanOrComparison (expr), LowerArithmeticBinary (expr), ArbLowerExpression (expr) |
| ArbLowerComposeSwizzledSource | support | ArbLowerSwizzleNode (operand), LowerArithmeticBinary (expr), ArbLowerExpression (expr) |
| ArbLowerSwizzleNode | operand | ArbLowerExpression (expr) |
| ArbLowerExpression | expr | LowerAssignment (stmt), LowerExpressionStmt (stmt), ArbLowerStatement (stmt), ArbLowerSwizzleNode (operand) |

## Review checklist

- [x] Every definition has one owner and every shared call has a private declaration.
- [x] Exact-body audit allows only the recorded identifier mapping and linkage changes.
- [x] Public headers, profiles, IR and expected outputs remain unchanged.
- [x] Pristine baseline, both full suites, assembly comparisons and diagnostic comparisons pass.
- [x] Structural checks pass in both isolated and main checkouts.
- [x] ARB WGL skips are recorded separately from passes.

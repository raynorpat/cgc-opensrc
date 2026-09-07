# HLSL Lowering Modularization Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (- [ ]) syntax for tracking.

**Goal:** Split the 6,000-line HLSL lowerer into private responsibility-focused C modules while preserving generated output, diagnostics, ordering, exit status, public APIs, and downstream behavior.

**Architecture:** Keep HlslLowerProgram and HlslLowerProgramWithIR in hlsl_lower.c as the only public facade. Move existing functions without logic changes into support, declaration, expression, statement, geometry, and function modules that share one stack-owned HlslLowerContext through hlsl_lower_internal.h.

**Tech Stack:** C90-compatible C, CMake 3.16+, CTest, Visual Studio/MSVC multi-configuration builds, Microsoft FXC validation, existing Cg IR and typed HLSL IR.

---

## Required Baseline

The design source of truth is:

    docs/superpowers/specs/2026-09-07-hlsl-lowering-modularization-design.md

The plan was written in:

    D:/raynorpat/cgc/.worktrees/hlsl-lowering-modularization

on branch:

    codex/hlsl-lowering-modularization

The clean Release baseline at commit efe9f3a passed 1,369 of 1,369 tests with CGC_REQUIRE_FXC=ON. Preserve the existing warnings in compile.c and arb_lower.c as baseline noise; do not repair them in this refactor.

Before the first extraction, also establish the Debug baseline:

~~~powershell
cmake -S . -B build-cg20-hlsl-lower-baseline -A x64 -DBUILD_TESTING=ON -DCGC_REQUIRE_FXC=ON
cmake --build build-cg20-hlsl-lower-baseline --config Debug
ctest --test-dir build-cg20-hlsl-lower-baseline -C Debug --output-on-failure
~~~

Expected: the build exits 0 and CTest reports 100% tests passed, 0 tests failed out of 1369.

Do not update expected HLSL files during this work. Never configure with CGC_UPDATE_HLSL_EXPECTED=ON.

## File Responsibility Map

### New production files

- hlsl_lower_internal.h: private context, value mode, and cross-module declarations.
- hlsl_lower_support.c: allocation, locations, failure recording, shared lookup, text copying, and source-node construction.
- hlsl_lower_decl.c: source types, declarations, structures, uniforms, defaults, parameters, locals, and semantics.
- hlsl_lower_expr.c: source-AST and Cg-IR expressions, calls, intrinsics, ordered evaluation, aggregate copies, and swizzles.
- hlsl_lower_stmt.c: source-AST and Cg-IR statements, initializers, loops, and break rewriting.
- hlsl_lower_geometry.c: geometry operations, output state, flat replay, effect analysis, and geometry-function preparation.
- hlsl_lower_function.c: call discovery, helper reachability, function lowering, returned-structure initialization, and entry inspection.

Every new C and header file must begin with the exact NVIDIA license notice currently at hlsl_lower.c:1-42. Copy it unchanged.

### Modified files

- hlsl_lower.c: remove extracted definitions; retain includes, public entry points, and top-level orchestration.
- CMakeLists.txt: define and consume the canonical CGC_HLSL_LOWER_SOURCES list.
- tests/CMakeLists.txt: consume that list in hlsl_geometry_lower_unit and register the final structural check.

### New test file

- tests/check_hlsl_lower_structure.cmake: enforce file presence, private-header ownership, facade size, public-entry ownership, and canonical build wiring.

## Extraction Rules

These rules apply to every move in Tasks 2-7:

1. Use apply_patch for all source edits.
2. Identify each function by its complete definition, from its return-type line through its existing closing comment. This existing function shows the exact boundary style:

~~~c
static void HlslSetLoc(HlslLoc *target, const SourceLoc *source)
{
    if (target != NULL && source != NULL) {
        target->file = source->file;
        target->line = source->line;
    }
} // HlslSetLoc
~~~

3. Copy the exact existing body into the destination file and remove it from hlsl_lower.c in the same patch.
4. Remove static only for functions declared in hlsl_lower_internal.h. Keep every destination-private helper static.
5. Do not rename functions, variables, parameters, diagnostics, or closing comments.
6. Do not reorder checks, declarations, list insertion, allocations, or short-circuit expressions.
7. Do not reformat a moved body.
8. Add only the prototypes required by an already-created module. Do not declare future functions early because their current static definitions would conflict.
9. Build both cgc and hlsl_geometry_lower_unit after each extraction before committing.

---

### Task 1: Add the structural guard and canonical source list

**Files:**
- Create: tests/check_hlsl_lower_structure.cmake
- Modify: CMakeLists.txt
- Modify: tests/CMakeLists.txt

- [ ] **Step 1: Write the final structural check before the new files exist**

Create tests/check_hlsl_lower_structure.cmake with this content:

~~~cmake
cmake_minimum_required(VERSION 3.16)

if(NOT DEFINED SOURCE_ROOT OR SOURCE_ROOT STREQUAL "")
    message(FATAL_ERROR "SOURCE_ROOT is required")
endif()

set(lower_header
    hlsl_lower_internal.h)
set(lower_sources
    hlsl_lower.c
    hlsl_lower_support.c
    hlsl_lower_decl.c
    hlsl_lower_expr.c
    hlsl_lower_stmt.c
    hlsl_lower_geometry.c
    hlsl_lower_function.c)

foreach(relative IN LISTS lower_header lower_sources)
    if(NOT EXISTS "${SOURCE_ROOT}/${relative}")
        message(FATAL_ERROR "missing HLSL lowering file ${relative}")
    endif()
endforeach()

foreach(relative IN LISTS lower_sources)
    file(READ "${SOURCE_ROOT}/${relative}" content)
    string(FIND "${content}" "#include \"hlsl_lower_internal.h\""
           include_position)
    if(include_position EQUAL -1)
        message(FATAL_ERROR
            "${relative} must include hlsl_lower_internal.h")
    endif()
endforeach()

file(READ "${SOURCE_ROOT}/hlsl_lower.c" facade)
string(FIND "${facade}" "int HlslLowerProgramWithIR(" with_ir_position)
string(FIND "${facade}" "int HlslLowerProgram(" public_position)
if(with_ir_position EQUAL -1 OR public_position EQUAL -1)
    message(FATAL_ERROR
        "hlsl_lower.c must own both public lowering entry points")
endif()
file(STRINGS "${SOURCE_ROOT}/hlsl_lower.c" facade_lines)
list(LENGTH facade_lines facade_line_count)
if(facade_line_count GREATER 450)
    message(FATAL_ERROR
        "hlsl_lower.c remains a monolith: ${facade_line_count} lines")
endif()

set(private_sources
    hlsl_lower_support.c
    hlsl_lower_decl.c
    hlsl_lower_expr.c
    hlsl_lower_stmt.c
    hlsl_lower_geometry.c
    hlsl_lower_function.c)
foreach(relative IN LISTS private_sources)
    file(READ "${SOURCE_ROOT}/${relative}" content)
    string(FIND "${content}" "int HlslLowerProgramWithIR("
           with_ir_position)
    string(FIND "${content}" "int HlslLowerProgram("
           public_position)
    if(NOT with_ir_position EQUAL -1 OR NOT public_position EQUAL -1)
        message(FATAL_ERROR
            "${relative} defines a public lowering entry point")
    endif()
endforeach()

file(READ "${SOURCE_ROOT}/CMakeLists.txt" root_cmake)
string(FIND "${root_cmake}" "set(CGC_HLSL_LOWER_SOURCES"
       list_position)
if(list_position EQUAL -1)
    message(FATAL_ERROR "CGC_HLSL_LOWER_SOURCES is not defined")
endif()
foreach(relative IN LISTS lower_sources)
    string(FIND "${root_cmake}" "${relative}" source_position)
    if(source_position EQUAL -1)
        message(FATAL_ERROR
            "CGC_HLSL_LOWER_SOURCES omits ${relative}")
    endif()
endforeach()

file(READ "${SOURCE_ROOT}/tests/CMakeLists.txt" test_cmake)
string(FIND "${test_cmake}" [=[${CGC_HLSL_LOWER_SOURCES}]=]
       test_list_position)
if(test_list_position EQUAL -1)
    message(FATAL_ERROR
        "hlsl_geometry_lower_unit does not use CGC_HLSL_LOWER_SOURCES")
endif()
~~~

- [ ] **Step 2: Run the structural check and verify it fails for the intended reason**

~~~powershell
cmake "-DSOURCE_ROOT=$((Resolve-Path '.').Path)" -P tests/check_hlsl_lower_structure.cmake
~~~

Expected: nonzero exit with missing HLSL lowering file hlsl_lower_internal.h. No syntax or argument error is acceptable.

- [ ] **Step 3: Introduce the canonical source variable without changing compiled sources**

Insert this before if(BUILD_TESTING) in the root CMakeLists.txt:

~~~cmake
set(CGC_HLSL_LOWER_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/hlsl_lower.c
)
~~~

Replace the standalone hlsl_lower.c entry in the cgc target source list with:

~~~cmake
    ${CGC_HLSL_LOWER_SOURCES}
~~~

Replace only ${PROJECT_SOURCE_DIR}/hlsl_lower.c in the hlsl_geometry_lower_unit source list with:

~~~cmake
    ${CGC_HLSL_LOWER_SOURCES}
~~~

Do not register the structural script with CTest yet; it must remain an explicit red test until Task 8.

- [ ] **Step 4: Reconfigure, build, and run lowering-focused tests**

~~~powershell
cmake -S . -B build-cg20-hlsl-lower-baseline -A x64 -DBUILD_TESTING=ON -DCGC_REQUIRE_FXC=ON
cmake --build build-cg20-hlsl-lower-baseline --config Release --target cgc hlsl_geometry_lower_unit
ctest --test-dir build-cg20-hlsl-lower-baseline -C Release -R "^(hlsl_geometry_lower_unit|hlsl_geometry_lower_assertions_active|hlsl_ir_unit|hlsl_semantics_unit|hlsl_modern_raw_output_determinism)$" --output-on-failure
~~~

Expected: configuration and build exit 0; all selected tests pass.

- [ ] **Step 5: Commit the guard and build-list preparation**

~~~powershell
git add CMakeLists.txt tests/CMakeLists.txt tests/check_hlsl_lower_structure.cmake
git commit -m "Prepare HLSL lowering modularization"
~~~

---

### Task 2: Extract the private context and shared support

**Files:**
- Create: hlsl_lower_internal.h
- Create: hlsl_lower_support.c
- Modify: hlsl_lower.c
- Modify: CMakeLists.txt

- [ ] **Step 1: Add the private header with only the context and support API**

After the unchanged NVIDIA notice, hlsl_lower_internal.h must contain:

~~~c
#ifndef __HLSL_LOWER_INTERNAL_H
#define __HLSL_LOWER_INTERNAL_H

#include <stddef.h>

#include "slglobals.h"
#include "cg_stdlib.h"
#include "cg_ir.h"
#include "hlsl_hal.h"

typedef struct HlslLowerContext_Rec {
    HlslModule *module;
    const HlslProfileDesc *profile;
    Scope *scope;
    HlslFunction *function;
    Symbol *collectingHelper;
    SourceLoc statementLoc;
    int entryFile;
    int loopDepth;
    int geometryInputExtent;
    const CgIRModule *sourceIR;
} HlslLowerContext;

typedef enum HlslValueMode_Enum {
    HLSL_VALUE_DISCARD,
    HLSL_VALUE_RVALUE,
    HLSL_VALUE_LVALUE
} HlslValueMode;

void HlslSetLoc(HlslLoc *target, const SourceLoc *source);
HlslExpr *HlslNewSourceExpr(HlslLowerContext *context,
                            HlslExprKind kind, HlslType type);
int HlslLowerFailure(HlslLowerContext *context, HlslErrorKind kind,
                     const char *reason, const SourceLoc *loc);
void *HlslLowerAlloc(HlslLowerContext *context, size_t size);
char *HlslGeneratedSource(HlslLowerContext *context, const char *source);
HlslDecl *HlslFindDeclList(HlslDecl *list, const void *identity);
HlslDecl *HlslFindDecl(HlslLowerContext *context, const void *identity);
HlslFunction *HlslFindFunction(HlslModule *module, const void *identity);
char *HlslCopyText(HlslLowerContext *context, const char *text);

#endif // __HLSL_LOWER_INTERNAL_H
~~~

- [ ] **Step 2: Move the support definitions without changing their bodies**

Create hlsl_lower_support.c with the unchanged license, these includes, and the exact existing definitions:

~~~c
#include <float.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>

#include "hlsl_lower_internal.h"
~~~

Move this exact inventory from hlsl_lower.c:

~~~text
HlslSetLoc
HlslNewSourceExpr
HlslLowerFailure
HlslLowerAlloc
HlslGeneratedSource
HlslFindDeclList
HlslFindDecl
HlslFindFunction
HlslCopyText
~~~

Remove static from all nine definitions because they are in the private header. Retain their existing closing comments.

- [ ] **Step 3: Make the facade consume the private header**

In hlsl_lower.c:

- add #include "hlsl_lower_internal.h";
- remove the local HlslLowerContext and HlslValueMode definitions;
- remove the nine moved definitions and their old forward declaration;
- keep the not-yet-moved static forward declarations exactly where needed.

The include block must still provide float.h, limits.h, stdio.h, and string.h until later extraction proves which translation unit owns them.

- [ ] **Step 4: Add support to the canonical CMake list**

The list must now be:

~~~cmake
set(CGC_HLSL_LOWER_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/hlsl_lower.c
    ${CMAKE_CURRENT_SOURCE_DIR}/hlsl_lower_support.c
)
~~~

- [ ] **Step 5: Build and run support-sensitive tests**

~~~powershell
cmake -S . -B build-cg20-hlsl-lower-baseline -A x64 -DBUILD_TESTING=ON -DCGC_REQUIRE_FXC=ON
cmake --build build-cg20-hlsl-lower-baseline --config Debug --target cgc hlsl_geometry_lower_unit
cmake --build build-cg20-hlsl-lower-baseline --config Release --target cgc hlsl_geometry_lower_unit
ctest --test-dir build-cg20-hlsl-lower-baseline -C Release -R "^(hlsl_geometry_lower_unit|hlsl_geometry_lower_assertions_active|hlsl_ir_unit|hlsl_semantics_unit|hlsl_modern_raw_output_determinism)$" --output-on-failure
~~~

Expected: both builds exit 0 and all selected tests pass with unchanged output.

- [ ] **Step 6: Commit the shared lowering context**

~~~powershell
git add CMakeLists.txt hlsl_lower.c hlsl_lower_internal.h hlsl_lower_support.c
git commit -m "Extract HLSL lowering support"
~~~

---

### Task 3: Extract types, declarations, uniforms, and defaults

**Files:**
- Create: hlsl_lower_decl.c
- Modify: hlsl_lower_internal.h
- Modify: hlsl_lower.c
- Modify: CMakeLists.txt

- [ ] **Step 1: Extend the private header with the declaration API**

Insert these declarations before the closing include guard:

~~~c
int HlslLowerType(HlslLowerContext *context, Type *source,
                  HlslType *target, const SourceLoc *loc);
int HlslEnsureType(HlslLowerContext *context, Type *type);
const char *HlslFunctionSemantic(HlslLowerContext *context,
                                 Symbol *symbol);
int HlslRejectStorage(HlslLowerContext *context, Symbol *symbol);
int HlslSortStructs(HlslLowerContext *context);
int HlslCollectUniformList(HlslLowerContext *context, SymbolList *list);
int HlslCollectUniformTree(HlslLowerContext *context, Symbol *symbol);
int HlslCollectDefaults(HlslLowerContext *context);
int HlslCollectParameters(HlslLowerContext *context,
                          Symbol *formal, int isEntry);
int HlslCollectLocals(HlslLowerContext *context, Symbol *symbol);
~~~

- [ ] **Step 2: Move the declaration implementation**

Create hlsl_lower_decl.c with the unchanged license and:

~~~c
#include <float.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>

#include "hlsl_lower_internal.h"
~~~

Move these definitions from hlsl_lower.c, preserving their current relative order:

~~~text
HlslLogicalTypeName
HlslCanonicalStructType
HlslFindTag
HlslFindStruct
HlslLowerType
HlslDeclComesBefore
HlslInsertDecl
HlslSourceSemantic
HlslFunctionSemantic
HlslRejectStorage
HlslNewSourceDecl
HlslCollectMembers
HlslEnsureSymbolTypes
HlslEnsureType
HlslTypeUsesStruct
HlslStructReady
HlslSortStructs
HlslParseRegisterSemantic
HlslFindUniformBinding
HlslSymbolListContains
HlslCollectUniform
HlslCollectUniformList
HlslCollectUniformTree
HlslDefaultClassOf
HlslFiniteDefaultFloat
HlslAppendTypedDefault
HlslConvertDefault
HlslFlattenDefaultExpr
HlslDefaultComponentCount
HlslDefaultTargetBase
HlslStoreDefaultType
HlslCollectDefaults
HlslSemanticAtomMatches
HlslSemanticAtomIsPrimitiveIdentity
HlslCollectParameters
HlslCollectLocals
~~~

Move the existing HlslCanonicalStructType forward declaration into
hlsl_lower_decl.c immediately before HlslLogicalTypeName because that first
function calls the later definition.

Also move HlslDefaultClass and HlslDefaultValue from their current local declarations into hlsl_lower_decl.c immediately before the first function that uses each type.

Remove static only from the ten functions declared in the private header. Keep every other function static. Leave HlslResolveBuiltinSymbol and HlslResolveBuiltinSignature in hlsl_lower.c for Task 4.

- [ ] **Step 3: Add the declaration module to CMake**

Add this after hlsl_lower_support.c:

~~~cmake
    ${CMAKE_CURRENT_SOURCE_DIR}/hlsl_lower_decl.c
~~~

- [ ] **Step 4: Build and run declaration-focused coverage**

~~~powershell
cmake -S . -B build-cg20-hlsl-lower-baseline -A x64 -DBUILD_TESTING=ON -DCGC_REQUIRE_FXC=ON
cmake --build build-cg20-hlsl-lower-baseline --config Debug --target cgc hlsl_geometry_lower_unit
cmake --build build-cg20-hlsl-lower-baseline --config Release --target cgc hlsl_geometry_lower_unit
ctest --test-dir build-cg20-hlsl-lower-baseline -C Release -R "^(hlsl_semantics_unit|hlslv_registration|hlslf_registration|hlslv40_registration|hlslf40_registration|hlslv50_registration|hlslf50_registration|modern_defaults_v40|hlslv_vp_register_banks|hlsl_geometry_lower_unit)$" --output-on-failure
~~~

Expected: both targets build in both configurations and every selected test passes.

- [ ] **Step 5: Commit declaration extraction**

~~~powershell
git add CMakeLists.txt hlsl_lower.c hlsl_lower_internal.h hlsl_lower_decl.c
git commit -m "Extract HLSL declaration lowering"
~~~

---

### Task 4: Extract source-AST and Cg-IR expression lowering

**Files:**
- Create: hlsl_lower_expr.c
- Modify: hlsl_lower_internal.h
- Modify: hlsl_lower.c
- Modify: CMakeLists.txt

- [ ] **Step 1: Add the cross-module expression API**

Insert these declarations before the closing include guard:

~~~c
HlslExpr *HlslNewLiteral(HlslLowerContext *context, HlslBase base,
                         int intValue, float floatValue);
HlslExpr *HlslNewSymbolExpr(HlslLowerContext *context, HlslDecl *decl);
HlslExpr *HlslNewAssignment(HlslLowerContext *context,
                            HlslExpr *left, HlslExpr *right);
int HlslAppendExpression(HlslLowerContext *context, HlslStmt **list,
                         HlslExpr *expression);
HlslExpr *HlslCaptureValue(HlslLowerContext *context,
                           HlslStmt **list, HlslExpr *value);
int HlslAppendRecursiveCopy(HlslLowerContext *context,
                            const HlslType *type,
                            HlslExpr *target, HlslExpr *source,
                            HlslStmt **statements);
HlslExpr *HlslScalarizeVectorCondition(HlslLowerContext *context,
                                       HlslStmt **prefix,
                                       HlslExpr *condition);
HlslExpr *HlslLowerExpr(HlslLowerContext *context, expr *source,
                        HlslStmt **prefix, HlslValueMode valueMode);
HlslExpr *HlslLowerIRExpr(HlslLowerContext *context,
                          const CgIRExpr *source,
                          HlslStmt **prefix,
                          HlslValueMode valueMode);
~~~

- [ ] **Step 2: Move all expression definitions**

Create hlsl_lower_expr.c with the unchanged license and:

~~~c
#include <float.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>

#include "hlsl_lower_internal.h"
~~~

Move HlslResolveBuiltinSymbol and HlslResolveBuiltinSignature first. Then move every definition from HlslNewLiteral through HlslLowerExpr, excluding the already-extracted HlslCopyText. Finally move every definition from HlslIROperator through HlslLowerIRExpr.

The complete expression inventory is:

~~~text
HlslResolveBuiltinSymbol
HlslResolveBuiltinSignature
HlslNewLiteral
HlslLowerConstant
HlslBinaryOperator
HlslIsAssignmentOperator
HlslUnaryOperator
HlslIsComparison
HlslNewSymbolExpr
HlslNewTemporary
HlslNewExpressionStmt
HlslNewAssignment
HlslBuiltinHelperTypeEqual
HlslFindBuiltinHelper
HlslNewBuiltinConstant
HlslCreateBuiltinHelper
HlslAppendExpression
HlslCaptureValue
HlslStabilizeLvalueAddress
HlslTypeNeedsRecursiveCopy
HlslIsStableAggregateSource
HlslIsNativeAggregateTempAssignment
HlslCopyMember
HlslCopyIndex
HlslAppendRecursiveCopy
HlslDetachLastExpression
HlslLowerOrderedValue
HlslNewSwizzle
HlslComponent
HlslScalarizeVectorCondition
HlslLowerSwizzle
HlslLowerMatrixSwizzle
HlslLowerExprList
HlslLowerCall
HlslLowerConditional
HlslLowerVectorComparison
HlslLowerExpr
HlslIROperator
HlslLowerIRExprList
HlslLowerIRConstant
HlslIRTypeIsIdentical
HlslLowerIRCall
HlslLowerIRIntrinsic
HlslLowerIRExpr
~~~

Move the function-local HlslCopyOut type with HlslLowerCall. Remove static only from the nine functions declared in hlsl_lower_internal.h.

- [ ] **Step 3: Add the expression module to CMake**

Add:

~~~cmake
    ${CMAKE_CURRENT_SOURCE_DIR}/hlsl_lower_expr.c
~~~

after hlsl_lower_decl.c.

- [ ] **Step 4: Build and run expression and evaluation-order tests**

~~~powershell
cmake -S . -B build-cg20-hlsl-lower-baseline -A x64 -DBUILD_TESTING=ON -DCGC_REQUIRE_FXC=ON
cmake --build build-cg20-hlsl-lower-baseline --config Debug --target cgc hlsl_geometry_lower_unit
cmake --build build-cg20-hlsl-lower-baseline --config Release --target cgc hlsl_geometry_lower_unit
ctest --test-dir build-cg20-hlsl-lower-baseline -C Release -R "^(hlsl_geometry_lower_unit|hlslv_operator_surface|hlsl_validate_hlslv_operator_surface|hlslv_evaluation_order|hlsl_validate_hlslv_evaluation_order|hlslv_nested_postinc|hlsl_validate_hlslv_nested_postinc|modern_intrinsic_user_same_name_v40)$" --output-on-failure
~~~

Expected: both builds exit 0; source and FXC validation tests pass.

- [ ] **Step 5: Commit expression extraction**

~~~powershell
git add CMakeLists.txt hlsl_lower.c hlsl_lower_internal.h hlsl_lower_expr.c
git commit -m "Extract HLSL expression lowering"
~~~

---

### Task 5: Extract statement and loop lowering

**Files:**
- Create: hlsl_lower_stmt.c
- Modify: hlsl_lower_internal.h
- Modify: hlsl_lower.c
- Modify: CMakeLists.txt

- [ ] **Step 1: Add the statement API**

Insert:

~~~c
int HlslLowerStatements(HlslLowerContext *context, stmt *source,
                        HlslStmt **list);
int HlslLowerIRStatements(HlslLowerContext *context,
                          const CgIRStmt *source,
                          HlslStmt **list);
HlslStmt *HlslLowerGeometryOperation(HlslLowerContext *context,
                                     const CgIRStmt *operation);
~~~

before the closing include guard.

- [ ] **Step 2: Move source and Cg-IR statement lowering**

Create hlsl_lower_stmt.c with the unchanged license and:

~~~c
#include <float.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>

#include "hlsl_lower_internal.h"
~~~

Move this exact inventory in the listed order:

~~~text
HlslIsSyntheticInputAssignment
HlslNewUnaryExpr
HlslNewBreakGuard
HlslNewBoolAssignment
HlslStatementsAreForParts
HlslRewriteLoopBreaks
HlslAppendIRInitializerCopy
HlslLowerIRDeclarationInitializer
HlslLowerStatements
HlslLowerIRStatements
~~~

Remove static from HlslLowerStatements and HlslLowerIRStatements. Also remove
static from the existing HlslLowerGeometryOperation definition but leave its
complete body in hlsl_lower.c until Task 6. The new statement module calls that
private API and must not duplicate either expression or geometry logic.

- [ ] **Step 3: Add the statement module to CMake**

Add:

~~~cmake
    ${CMAKE_CURRENT_SOURCE_DIR}/hlsl_lower_stmt.c
~~~

after hlsl_lower_expr.c.

- [ ] **Step 4: Build and run control-flow coverage**

~~~powershell
cmake -S . -B build-cg20-hlsl-lower-baseline -A x64 -DBUILD_TESTING=ON -DCGC_REQUIRE_FXC=ON
cmake --build build-cg20-hlsl-lower-baseline --config Debug --target cgc hlsl_geometry_lower_unit
cmake --build build-cg20-hlsl-lower-baseline --config Release --target cgc hlsl_geometry_lower_unit
ctest --test-dir build-cg20-hlsl-lower-baseline -C Release -R "^(hlsl_geometry_lower_unit|hlslv_control|hlsl_validate_hlslv_control|hlslv_loop_prefixes|hlsl_validate_hlslv_loop_prefixes|hlslv_empty_control|hlsl_validate_hlslv_empty_control|hlslf_discard|hlsl_validate_hlslf_discard|hlslf_vector_discard|hlsl_validate_hlslf_vector_discard)$" --output-on-failure
~~~

Expected: both builds exit 0 and every selected control-flow test passes.

- [ ] **Step 5: Commit statement extraction**

~~~powershell
git add CMakeLists.txt hlsl_lower.c hlsl_lower_internal.h hlsl_lower_stmt.c
git commit -m "Extract HLSL statement lowering"
~~~

---

### Task 6: Extract geometry lowering and effect analysis

**Files:**
- Create: hlsl_lower_geometry.c
- Modify: hlsl_lower_internal.h
- Modify: hlsl_lower.c
- Modify: CMakeLists.txt

- [ ] **Step 1: Add the geometry API**

Insert:

~~~c
int HlslCollectGeometryOutput(HlslLowerContext *context);
int HlslPrepareGeometryFunctions(HlslLowerContext *context);
~~~

before the closing include guard.

- [ ] **Step 2: Move geometry operations and analysis**

Create hlsl_lower_geometry.c with the unchanged license and:

~~~c
#include <float.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>

#include "hlsl_lower_internal.h"
~~~

Move this exact inventory, preserving the order within each existing block:

~~~text
HlslGeometryOutputMember
HlslGeometryFlatState
HlslGeometryMemberExpr
HlslGeometryConvert
HlslGeometryValueRoot
HlslGeometryCapturedMember
HlslLowerGeometryOperation
HlslGeometryIRHasDirectEffect
HlslGeometryIRExprCallsEffect
HlslGeometryIRCallsEffect
HlslMarkGeometryEffects
HlslGeometrySemanticInfo
HlslGeometryMemberName
HlslCollectGeometryOutputStatements
HlslCollectGeometryOutput
HlslPrepareGeometryFunctionState
HlslPrepareGeometryFunctions
~~~

HlslLowerGeometryOperation already has private external linkage from Task 5;
move it without changing its declaration. Remove static only from
HlslCollectGeometryOutput and HlslPrepareGeometryFunctions. Preserve the
function-static outputIdentity and placeholderIdentity objects in
HlslCollectGeometryOutput exactly; they provide stable identities within the
process.

- [ ] **Step 3: Add the geometry module to CMake**

Add:

~~~cmake
    ${CMAKE_CURRENT_SOURCE_DIR}/hlsl_lower_geometry.c
~~~

after hlsl_lower_stmt.c.

- [ ] **Step 4: Build and run the complete focused geometry set**

~~~powershell
cmake -S . -B build-cg20-hlsl-lower-baseline -A x64 -DBUILD_TESTING=ON -DCGC_REQUIRE_FXC=ON
cmake --build build-cg20-hlsl-lower-baseline --config Debug --target cgc hlsl_geometry_lower_unit
cmake --build build-cg20-hlsl-lower-baseline --config Release --target cgc hlsl_geometry_lower_unit
ctest --test-dir build-cg20-hlsl-lower-baseline -C Release -R "^(hlsl_geometry_lower_unit|hlsl_geometry_lower_assertions_active|hlslg(40|50)_(pass_through|amplify|restart|flat|conditional_flat|reachable_helper|loop_order)|hlsl_validate_hlslg(40|50)_(pass_through|amplify|restart|flat|conditional_flat|reachable_helper|loop_order))$" --output-on-failure
~~~

Expected: the unit test and every selected SM4/SM5 geometry source and FXC validation test pass.

- [ ] **Step 5: Commit geometry extraction**

~~~powershell
git add CMakeLists.txt hlsl_lower.c hlsl_lower_internal.h hlsl_lower_geometry.c
git commit -m "Extract HLSL geometry lowering"
~~~

---

### Task 7: Extract reachability and function lowering

**Files:**
- Create: hlsl_lower_function.c
- Modify: hlsl_lower_internal.h
- Modify: hlsl_lower.c
- Modify: CMakeLists.txt

- [ ] **Step 1: Add the function-level private API**

Insert:

~~~c
int HlslCollectCallsInStatements(HlslLowerContext *context, stmt *source);
int HlslCollectIRCallsInStatements(HlslLowerContext *context,
                                   const CgIRStmt *source);
int HlslLowerFunction(HlslLowerContext *context,
                      HlslFunction *function);
Type *HlslOriginalEntryResult(Symbol *program);
int HlslIsEmptyEntry(Symbol *program);
~~~

before the closing include guard.

- [ ] **Step 2: Move reachability, helper, and function definitions**

Create hlsl_lower_function.c with the unchanged license and:

~~~c
#include <float.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>

#include "hlsl_lower_internal.h"
~~~

Move this exact inventory:

~~~text
HlslFindSourceIRFunction
HlslCollectCallsInStatements
HlslCollectHelper
HlslCollectCallsInExpr
HlslCollectIRCallsInExpr
HlslCollectIRCallsInStatements
HlslDeclListContains
HlslInitializeReturnedStructs
HlslLowerFunction
HlslOriginalEntryResult
HlslIsEmptyEntry
~~~

Move the forward declarations for HlslCollectCallsInExpr and HlslCollectIRCallsInStatements into hlsl_lower_function.c immediately before HlslFindSourceIRFunction. Remove static only from the five functions declared in the private header.

- [ ] **Step 3: Reduce hlsl_lower.c to the facade**

After extraction, hlsl_lower.c must contain:

- the unchanged license and include block;
- #include "hlsl_lower_internal.h";
- HlslLowerProgramWithIR with its current body unchanged;
- HlslLowerProgram with its current body unchanged.

It must contain no private function definition, local lowering type, or obsolete forward declaration. Do not reorder any expression within the two public functions.

- [ ] **Step 4: Add the function module to CMake**

The final canonical list must be:

~~~cmake
set(CGC_HLSL_LOWER_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/hlsl_lower.c
    ${CMAKE_CURRENT_SOURCE_DIR}/hlsl_lower_support.c
    ${CMAKE_CURRENT_SOURCE_DIR}/hlsl_lower_decl.c
    ${CMAKE_CURRENT_SOURCE_DIR}/hlsl_lower_expr.c
    ${CMAKE_CURRENT_SOURCE_DIR}/hlsl_lower_stmt.c
    ${CMAKE_CURRENT_SOURCE_DIR}/hlsl_lower_geometry.c
    ${CMAKE_CURRENT_SOURCE_DIR}/hlsl_lower_function.c
)
~~~

- [ ] **Step 5: Build and run helper/reachability coverage**

~~~powershell
cmake -S . -B build-cg20-hlsl-lower-baseline -A x64 -DBUILD_TESTING=ON -DCGC_REQUIRE_FXC=ON
cmake --build build-cg20-hlsl-lower-baseline --config Debug --target cgc hlsl_geometry_lower_unit
cmake --build build-cg20-hlsl-lower-baseline --config Release --target cgc hlsl_geometry_lower_unit
ctest --test-dir build-cg20-hlsl-lower-baseline -C Release -R "^(hlsl_geometry_lower_unit|hlslv_helpers|hlsl_validate_hlslv_helpers|modern_helpers_v40|hlsl_validate_modern_helpers_v40|hlslg40_reachable_helper|hlsl_validate_hlslg40_reachable_helper|hlslg50_reachable_helper|hlsl_validate_hlslg50_reachable_helper)$" --output-on-failure
~~~

Expected: both builds exit 0 and every selected helper/reachability test passes.

- [ ] **Step 6: Commit function extraction and facade reduction**

~~~powershell
git add CMakeLists.txt hlsl_lower.c hlsl_lower_internal.h hlsl_lower_function.c
git commit -m "Extract HLSL function lowering"
~~~

---

### Task 8: Register and pass the structural contract

**Files:**
- Modify: tests/CMakeLists.txt
- Test: tests/check_hlsl_lower_structure.cmake

- [ ] **Step 1: Run the previously failing structural script directly**

~~~powershell
cmake "-DSOURCE_ROOT=$((Resolve-Path '.').Path)" -P tests/check_hlsl_lower_structure.cmake
~~~

Expected: exit 0 with no fatal message. If it fails, fix only the reported module boundary or build wiring; do not weaken the assertions or raise the 450-line facade limit.

- [ ] **Step 2: Register the structural script with CTest**

Add this near hlsl_geometry_lower_unit in tests/CMakeLists.txt:

~~~cmake
add_test(
    NAME hlsl_lower_structure
    COMMAND ${CMAKE_COMMAND}
        -DSOURCE_ROOT=${PROJECT_SOURCE_DIR}
        -P ${CMAKE_CURRENT_SOURCE_DIR}/check_hlsl_lower_structure.cmake
)
~~~

- [ ] **Step 3: Reconfigure and verify the registered contract**

~~~powershell
cmake -S . -B build-cg20-hlsl-lower-baseline -A x64 -DBUILD_TESTING=ON -DCGC_REQUIRE_FXC=ON
ctest --test-dir build-cg20-hlsl-lower-baseline -C Release -R "^hlsl_lower_structure$" --output-on-failure
git diff --check
~~~

Expected: one of one structural test passes and git diff --check prints nothing.

- [ ] **Step 4: Audit private and public ownership**

~~~powershell
rg -n "^int HlslLowerProgram(WithIR)?\(" hlsl_lower*.c
rg -n '#include "hlsl_lower_internal.h"' hlsl_lower*.c
rg -n "HlslLowerContext|HlslValueMode" hlsl_lower_internal.h hlsl_lower*.c
~~~

Expected:

- both public definitions appear only in hlsl_lower.c;
- all seven lowering C files include the private header;
- the context and enum definitions appear only in hlsl_lower_internal.h;
- remaining matches are legitimate uses, not duplicate definitions.

- [ ] **Step 5: Commit the structural contract**

~~~powershell
git add tests/CMakeLists.txt tests/check_hlsl_lower_structure.cmake
git commit -m "Enforce HLSL lowering boundaries"
~~~

---

### Task 9: Perform full equivalence qualification

**Files:**
- Verify only; do not modify production or expected-output files.

- [ ] **Step 1: Configure a fresh final build with required FXC**

~~~powershell
cmake -S . -B build-cg20-hlsl-lower-final -A x64 -DBUILD_TESTING=ON -DCGC_REQUIRE_FXC=ON
~~~

Expected: configuration exits 0, discovers FXC, and generates the build without missing-source errors.

- [ ] **Step 2: Build and test Release**

~~~powershell
cmake --build build-cg20-hlsl-lower-final --config Release
ctest --test-dir build-cg20-hlsl-lower-final -C Release --output-on-failure
~~~

Expected: build exits 0 and CTest reports 100% tests passed. The structural test increases the inventory from the 1,369-test baseline to 1,370 tests; no pre-existing test may disappear.

- [ ] **Step 3: Compare every generated Release HLSL artifact with baseline**

Run after both baseline and final Release suites have populated their output directories:

~~~powershell
$baselineRoot = (Resolve-Path 'build-cg20-hlsl-lower-baseline/tests/Release').Path
$candidateRoot = (Resolve-Path 'build-cg20-hlsl-lower-final/tests/Release').Path
$baseline = Get-ChildItem -LiteralPath $baselineRoot -Filter '*.hlsl' -Recurse |
    ForEach-Object {
        [pscustomobject]@{
            Path = $_.FullName.Substring($baselineRoot.Length)
            Hash = (Get-FileHash -LiteralPath $_.FullName -Algorithm SHA256).Hash
        }
    } | Sort-Object Path
$candidate = Get-ChildItem -LiteralPath $candidateRoot -Filter '*.hlsl' -Recurse |
    ForEach-Object {
        [pscustomobject]@{
            Path = $_.FullName.Substring($candidateRoot.Length)
            Hash = (Get-FileHash -LiteralPath $_.FullName -Algorithm SHA256).Hash
        }
    } | Sort-Object Path
$difference = Compare-Object $baseline $candidate -Property Path, Hash
if ($difference) {
    $difference | Format-Table -AutoSize
    throw 'generated HLSL differs from baseline'
}
Write-Output "All $($baseline.Count) generated HLSL files match the baseline"
~~~

Expected: the command reports that every generated HLSL file matches and baseline and exits 0.

- [ ] **Step 4: Build and test Debug**

~~~powershell
cmake --build build-cg20-hlsl-lower-final --config Debug
ctest --test-dir build-cg20-hlsl-lower-final -C Debug --output-on-failure
~~~

Expected: build exits 0 and all 1,370 tests pass.

- [ ] **Step 5: Verify compatibility and external-validation contracts explicitly**

~~~powershell
ctest --test-dir build-cg20-hlsl-lower-final -C Release -R "^(hlsl_compatibility_matrix|hlsl_sm4_sm5_compatibility_matrix|hlsl_modern_fxc_coverage|hlsl_validation_exact_target_contract)$" --output-on-failure
~~~

Expected: all four tests pass; modern FXC coverage continues to report 197 exact combinations.

- [ ] **Step 6: Perform the final repository audit**

~~~powershell
git diff --check
git diff --exit-code efe9f3a -- hlsl_hal.h hlsl_ir.h hlsl_modern.h
git status --short
git log --oneline --decorate -8
~~~

Expected:

- git diff --check prints nothing;
- the public HLSL headers are byte-identical to commit efe9f3a;
- status contains no source or documentation changes beyond intentionally committed work;
- the history contains one focused commit for each extraction boundary;
- build-cg20-hlsl-lower-baseline and build-cg20-hlsl-lower-final remain ignored and do not appear in status.

Do not amend extraction commits during this audit. If a qualification defect requires code changes, make a focused repair commit and rerun the affected focused tests plus both complete Debug and Release suites.

---

## Definition of Done

- hlsl_lower.c contains only the unchanged public facade and top-level orchestration and has at most 450 lines.
- hlsl_lower_internal.h is the only private shared interface for the lowering subsystem.
- The six private C modules own exactly the responsibilities defined in the design.
- No new mutable global state exists.
- Public headers, HLSL IR layout, and exported API declarations are unchanged.
- cgc and hlsl_geometry_lower_unit consume the same canonical source list.
- Every baseline Release HLSL artifact has the same SHA-256 hash after refactoring.
- All 1,370 post-refactor tests pass in Debug and Release with required FXC validation.
- The compatibility matrices and 197-combination modern FXC contract remain green.
- No unrelated warning cleanup, backend redesign, GLSL work, or behavior fix enters the branch.

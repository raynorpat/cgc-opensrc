cmake_minimum_required(VERSION 3.16)

if(NOT DEFINED STRUCTURE_SCRIPT OR STRUCTURE_SCRIPT STREQUAL "")
    message(FATAL_ERROR "STRUCTURE_SCRIPT is required")
endif()
if(NOT DEFINED SOURCE_ROOT OR SOURCE_ROOT STREQUAL "")
    get_filename_component(SOURCE_ROOT "${CMAKE_CURRENT_LIST_DIR}/.."
        ABSOLUTE)
endif()

set(fixture_root
    "${SOURCE_ROOT}/build-cg20-hlsl-lower-structure-fixture")
file(MAKE_DIRECTORY "${fixture_root}")

file(WRITE "${fixture_root}/hlsl_lower_internal.h" [=[
#ifndef HLSL_LOWER_INTERNAL_H
#define HLSL_LOWER_INTERNAL_H
#endif
]=])
file(WRITE "${fixture_root}/hlsl_lower.c" [=[
#include "hlsl_lower_internal.h"
int HlslLowerProgramWithIR(void)
{
    return 0;
}
int HlslLowerProgram(void)
{
    return 0;
}
]=])
foreach(source IN ITEMS
        hlsl_lower_support.c
        hlsl_lower_decl.c
        hlsl_lower_expr.c
        hlsl_lower_stmt.c
        hlsl_lower_geometry.c
        hlsl_lower_function.c)
    file(WRITE "${fixture_root}/${source}" [=[
#include "hlsl_lower_internal.h"
]=])
endforeach()

file(WRITE "${fixture_root}/CMakeLists.txt" [=[
set(CGC_HLSL_LOWER_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/hlsl_lower.c
    ${CMAKE_CURRENT_SOURCE_DIR}/hlsl_lower_support.c
    ${CMAKE_CURRENT_SOURCE_DIR}/hlsl_lower_decl.c
    ${CMAKE_CURRENT_SOURCE_DIR}/hlsl_lower_expr.c
    ${CMAKE_CURRENT_SOURCE_DIR}/hlsl_lower_stmt.c
    ${CMAKE_CURRENT_SOURCE_DIR}/hlsl_lower_geometry.c
    ${CMAKE_CURRENT_SOURCE_DIR}/hlsl_lower_function.c
)
add_executable(cgc
    ${CGC_HLSL_LOWER_SOURCES}
    hlsl_lower.c
)
]=])
file(MAKE_DIRECTORY "${fixture_root}/tests")
file(WRITE "${fixture_root}/tests/CMakeLists.txt" [=[
add_executable(hlsl_geometry_lower_unit
    ${CGC_HLSL_LOWER_SOURCES}
)
]=])

function(ExpectStructureFailure expected_diagnostic)
    execute_process(
        COMMAND "${CMAKE_COMMAND}"
            "-DSOURCE_ROOT=${fixture_root}"
            -P "${STRUCTURE_SCRIPT}"
        RESULT_VARIABLE result
        OUTPUT_VARIABLE output
        ERROR_VARIABLE error)
    if(result EQUAL 0)
        message(FATAL_ERROR
            "expected structural check to reject ${expected_diagnostic}")
    endif()
    string(CONCAT diagnostics "${output}" "${error}")
    string(FIND "${diagnostics}" "${expected_diagnostic}"
        diagnostic_position)
    if(diagnostic_position EQUAL -1)
        message(FATAL_ERROR
            "unexpected structural-check failure: ${diagnostics}")
    endif()
endfunction()

function(ExpectStructureSuccess)
    execute_process(
        COMMAND "${CMAKE_COMMAND}"
            "-DSOURCE_ROOT=${fixture_root}"
            -P "${STRUCTURE_SCRIPT}"
        RESULT_VARIABLE result
        OUTPUT_VARIABLE output
        ERROR_VARIABLE error)
    if(NOT result EQUAL 0)
        message(FATAL_ERROR
            "expected structural check to pass: ${output}${error}")
    endif()
endfunction()

ExpectStructureFailure(
    "cgc directly lists hlsl_lower.c instead of CGC_HLSL_LOWER_SOURCES")

file(WRITE "${fixture_root}/CMakeLists.txt" [=[
set(CGC_HLSL_LOWER_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/hlsl_lower.c
    ${CMAKE_CURRENT_SOURCE_DIR}/hlsl_lower_support.c
    ${CMAKE_CURRENT_SOURCE_DIR}/hlsl_lower_decl.c
    ${CMAKE_CURRENT_SOURCE_DIR}/hlsl_lower_expr.c
    ${CMAKE_CURRENT_SOURCE_DIR}/hlsl_lower_stmt.c
    ${CMAKE_CURRENT_SOURCE_DIR}/hlsl_lower_geometry.c
    ${CMAKE_CURRENT_SOURCE_DIR}/hlsl_lower_function.c
)
add_executable(cgc
    ${CGC_HLSL_LOWER_SOURCES}
)
]=])
file(WRITE "${fixture_root}/tests/CMakeLists.txt" [=[
add_executable(hlsl_geometry_lower_unit
    ${CGC_HLSL_LOWER_SOURCES}
    hlsl_lower_geometry.c
)
]=])
ExpectStructureFailure(
    "hlsl_geometry_lower_unit directly lists hlsl_lower_geometry.c")

file(WRITE "${fixture_root}/tests/CMakeLists.txt" [=[
add_executable(hlsl_geometry_lower_unit
    ${CGC_HLSL_LOWER_SOURCES}
)
]=])
file(WRITE "${fixture_root}/CMakeLists.txt" [=[
set(CGC_HLSL_LOWER_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/hlsl_lower.c
    ${CMAKE_CURRENT_SOURCE_DIR}/hlsl_lower.c
    ${CMAKE_CURRENT_SOURCE_DIR}/hlsl_lower_support.c
    ${CMAKE_CURRENT_SOURCE_DIR}/hlsl_lower_decl.c
    ${CMAKE_CURRENT_SOURCE_DIR}/hlsl_lower_expr.c
    ${CMAKE_CURRENT_SOURCE_DIR}/hlsl_lower_stmt.c
    ${CMAKE_CURRENT_SOURCE_DIR}/hlsl_lower_geometry.c
    ${CMAKE_CURRENT_SOURCE_DIR}/hlsl_lower_function.c
)
add_executable(cgc
    ${CGC_HLSL_LOWER_SOURCES}
)
]=])
ExpectStructureFailure(
    "CGC_HLSL_LOWER_SOURCES must contain exactly the canonical lowering sources")

file(WRITE "${fixture_root}/CMakeLists.txt" [=[
set(CGC_HLSL_LOWER_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/hlsl_lower.c
    ${CMAKE_CURRENT_SOURCE_DIR}/hlsl_lower_support.c
    ${CMAKE_CURRENT_SOURCE_DIR}/hlsl_lower_decl.c
    ${CMAKE_CURRENT_SOURCE_DIR}/hlsl_lower_expr.c
    ${CMAKE_CURRENT_SOURCE_DIR}/hlsl_lower_stmt.c
    ${CMAKE_CURRENT_SOURCE_DIR}/hlsl_lower_geometry.c
    ${CMAKE_CURRENT_SOURCE_DIR}/hlsl_lower_function.c
)
add_executable(cgc
    ${CGC_HLSL_LOWER_SOURCES}
)
]=])
file(WRITE "${fixture_root}/unrelated.c" [=[
#include "hlsl_lower_internal.h"
]=])
ExpectStructureFailure(
    "unrelated.c includes hlsl_lower_internal.h")

file(WRITE "${fixture_root}/unrelated.c" [=[
int HlslLowerProgram(void);
// int HlslLowerProgramWithIR(void) { return 0; }
/*
int HlslLowerProgram(void)
{
    return 0;
}
*/
]=])
file(WRITE "${fixture_root}/hlsl_lower_support.c" [=[
#include "hlsl_lower_internal.h"
int HlslLowerProgram(void)
{
    return 0;
}
]=])
ExpectStructureFailure(
    "hlsl_lower_support.c defines HlslLowerProgram")

file(WRITE "${fixture_root}/hlsl_lower_support.c" [=[
#include "hlsl_lower_internal.h"
]=])
ExpectStructureSuccess()

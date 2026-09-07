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

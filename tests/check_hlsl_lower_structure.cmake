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

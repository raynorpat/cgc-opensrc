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
            "^[ \t]*#[ \t]*include[ \t]*\"hlsl_lower_internal\\.h\""
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
            "${relative} must include hlsl_lower_internal.h")
    endif()
endforeach()

file(STRINGS "${SOURCE_ROOT}/hlsl_lower.c" facade_lines)
list(LENGTH facade_lines facade_line_count)
if(facade_line_count GREATER 450)
    message(FATAL_ERROR
        "hlsl_lower.c remains a monolith: ${facade_line_count} lines")
endif()

file(READ "${SOURCE_ROOT}/CMakeLists.txt" root_cmake)
set(source_list_pattern
    "set[ \t\r\n]*\\([ \t\r\n]*CGC_HLSL_LOWER_SOURCES[ \t\r\n]+([^)]*)\\)")
string(REGEX MATCH "${source_list_pattern}" source_list_match "${root_cmake}")
if(source_list_match STREQUAL "")
    message(FATAL_ERROR "CGC_HLSL_LOWER_SOURCES is not defined")
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
        "CGC_HLSL_LOWER_SOURCES must contain exactly the canonical lowering sources")
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
            "CGC_HLSL_LOWER_SOURCES must contain ${relative} exactly once")
    endif()
endforeach()
foreach(entry IN LISTS source_list_entries)
    NormalizeLowerSourceEntry("${entry}" normalized_entry)
    list(FIND canonical_entries "${normalized_entry}" lower_source_position)
    if(lower_source_position EQUAL -1)
        message(FATAL_ERROR
            "CGC_HLSL_LOWER_SOURCES contains non-canonical entry ${entry}")
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
string(FIND "${cgc_target_body}" [=[${CGC_HLSL_LOWER_SOURCES}]=]
    cgc_list_position)
if(cgc_list_position EQUAL -1)
    message(FATAL_ERROR
        "cgc does not use CGC_HLSL_LOWER_SOURCES")
endif()
string(REGEX MATCH "hlsl_lower[^ \t\r\n)]*\\.c" direct_cgc_lower_source
    "${cgc_target_body}")
if(NOT direct_cgc_lower_source STREQUAL "")
    message(FATAL_ERROR
        "cgc directly lists ${direct_cgc_lower_source} instead of CGC_HLSL_LOWER_SOURCES")
endif()

file(READ "${SOURCE_ROOT}/tests/CMakeLists.txt" test_cmake)
set(geometry_target_pattern
    "add_executable[ \t\r\n]*\\([ \t\r\n]*hlsl_geometry_lower_unit[ \t\r\n]+([^)]*)\\)")
string(REGEX MATCH "${geometry_target_pattern}" geometry_target_match
    "${test_cmake}")
if(geometry_target_match STREQUAL "")
    message(FATAL_ERROR
        "add_executable(hlsl_geometry_lower_unit ...) is not defined")
endif()
set(geometry_target_body "${CMAKE_MATCH_1}")
RemoveCmakeComments("${geometry_target_body}" geometry_target_body)
string(FIND "${geometry_target_body}" [=[${CGC_HLSL_LOWER_SOURCES}]=]
    geometry_list_position)
if(geometry_list_position EQUAL -1)
    message(FATAL_ERROR
        "hlsl_geometry_lower_unit does not use CGC_HLSL_LOWER_SOURCES")
endif()
string(REGEX MATCH "hlsl_lower[^ \t\r\n)]*\\.c"
    direct_geometry_lower_source "${geometry_target_body}")
if(NOT direct_geometry_lower_source STREQUAL "")
    message(FATAL_ERROR
        "hlsl_geometry_lower_unit directly lists ${direct_geometry_lower_source} instead of CGC_HLSL_LOWER_SOURCES")
endif()

file(GLOB_RECURSE repository_files RELATIVE "${SOURCE_ROOT}"
    "${SOURCE_ROOT}/*.c"
    "${SOURCE_ROOT}/*.h")
foreach(relative IN LISTS repository_files)
    if(relative MATCHES "(^|/)(build[^/]*|cmake-build[^/]*|generated[^/]*)(/|$)")
        continue()
    endif()

    HasInternalHeaderInclude("${relative}" has_internal_header)
    if(has_internal_header)
        list(FIND lower_sources "${relative}" lower_source_position)
        if(lower_source_position EQUAL -1)
            message(FATAL_ERROR
                "${relative} includes hlsl_lower_internal.h outside the lowering implementation")
        endif()
    endif()
endforeach()

set(public_entry_points
    HlslLowerProgramWithIR
    HlslLowerProgram)
foreach(entry_point IN LISTS public_entry_points)
    set("${entry_point}_definition_owners")
endforeach()
foreach(relative IN LISTS repository_files)
    if(NOT relative MATCHES "\\.c$")
        continue()
    endif()
    if(relative MATCHES "(^|/)(build[^/]*|cmake-build[^/]*|generated[^/]*)(/|$)")
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
            if(NOT relative STREQUAL "hlsl_lower.c")
                message(FATAL_ERROR
                    "${relative} defines ${entry_point}; only hlsl_lower.c may define it")
            endif()
            list(APPEND "${entry_point}_definition_owners" "${relative}")
        endif()
    endforeach()
endforeach()
foreach(entry_point IN LISTS public_entry_points)
    list(LENGTH "${entry_point}_definition_owners" definition_owner_count)
    if(NOT definition_owner_count EQUAL 1)
        message(FATAL_ERROR
            "hlsl_lower.c must be the only definition owner of ${entry_point}")
    endif()
endforeach()

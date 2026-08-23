foreach(required CGC VERTEX_SOURCE FRAGMENT_SOURCE VERTEX_OUTPUT
        FRAGMENT_OUTPUT)
    if(NOT DEFINED ${required})
        message(FATAL_ERROR "${required} must be defined")
    endif()
endforeach()

include("${CMAKE_CURRENT_LIST_DIR}/check_config_output.cmake")
prepare_config_output("${VERTEX_OUTPUT}")
prepare_config_output("${FRAGMENT_OUTPUT}")
file(REMOVE "${VERTEX_OUTPUT}" "${FRAGMENT_OUTPUT}")

execute_process(
    COMMAND "${CGC}" -quiet -profile glslv -o "${VERTEX_OUTPUT}"
        "${VERTEX_SOURCE}"
    RESULT_VARIABLE vertex_result
    OUTPUT_VARIABLE vertex_stdout
    ERROR_VARIABLE vertex_stderr
)
if(NOT vertex_result EQUAL 0 OR NOT vertex_stdout STREQUAL "" OR
        NOT vertex_stderr STREQUAL "")
    message(FATAL_ERROR
        "vertex compile failed (${vertex_result}):\n${vertex_stdout}${vertex_stderr}")
endif()

execute_process(
    COMMAND "${CGC}" -quiet -profile glslf -o "${FRAGMENT_OUTPUT}"
        "${FRAGMENT_SOURCE}"
    RESULT_VARIABLE fragment_result
    OUTPUT_VARIABLE fragment_stdout
    ERROR_VARIABLE fragment_stderr
)
if(NOT fragment_result EQUAL 0 OR NOT fragment_stdout STREQUAL "" OR
        NOT fragment_stderr STREQUAL "")
    message(FATAL_ERROR
        "fragment compile failed (${fragment_result}):\n${fragment_stdout}${fragment_stderr}")
endif()

file(READ "${VERTEX_OUTPUT}" vertex_text)
file(READ "${FRAGMENT_OUTPUT}" fragment_text)
string(REPLACE "\r\n" "\n" vertex_text "${vertex_text}")
string(REPLACE "\r" "\n" vertex_text "${vertex_text}")
string(REPLACE "\r\n" "\n" fragment_text "${fragment_text}")
string(REPLACE "\r" "\n" fragment_text "${fragment_text}")
string(REGEX REPLACE "(^|\n)//[^\n]*" "" vertex_interface_text
    "${vertex_text}")
string(REGEX REPLACE "(^|\n)//[^\n]*" "" fragment_interface_text
    "${fragment_text}")
string(REGEX MATCHALL "varying [^;\n]+" vertex_varyings
    "${vertex_interface_text}")
string(REGEX MATCHALL "varying [^;\n]+" fragment_varyings
    "${fragment_interface_text}")
list(SORT vertex_varyings)
list(SORT fragment_varyings)
if(NOT vertex_varyings STREQUAL fragment_varyings)
    message(FATAL_ERROR
        "varying mismatch\nvertex: ${vertex_varyings}\n"
        "fragment: ${fragment_varyings}")
endif()
set(expected_varyings
    "varying vec2 cg_TEXCOORD0"
    "varying vec4 cg_COLOR0")
if(NOT vertex_varyings STREQUAL expected_varyings)
    message(FATAL_ERROR
        "unexpected linked interface: ${vertex_varyings}")
endif()
foreach(expected_decl "varying vec4 cg_COLOR0;"
                      "varying vec2 cg_TEXCOORD0;")
    string(FIND "${vertex_text}" "${expected_decl}" vertex_decl_offset)
    string(FIND "${fragment_text}" "${expected_decl}" fragment_decl_offset)
    if(vertex_decl_offset EQUAL -1 OR fragment_decl_offset EQUAL -1)
        message(FATAL_ERROR
            "linked interface is missing ${expected_decl}")
    endif()
endforeach()
foreach(shader_var vertex_text fragment_text)
    if(NOT "${${shader_var}}" MATCHES "^#version 110\n")
        message(FATAL_ERROR "${shader_var} does not begin with #version 110")
    endif()
    string(REGEX MATCHALL "(^|\n)#version[ \t]+110([ \t]*\n|$)"
        version_lines "${${shader_var}}")
    list(LENGTH version_lines version_count)
    string(FIND "${${shader_var}}" "\nvoid main()\n" main_offset)
    string(FIND "${${shader_var}}" " cg_output;" output_offset)
    if("${${shader_var}}" MATCHES "\\$vin|\\$vout" OR
            "${${shader_var}}" MATCHES
                "(^|[^A-Za-z0-9_])float[1-4]([^A-Za-z0-9_]|$)" OR
            NOT version_count EQUAL 1 OR main_offset EQUAL -1 OR
            output_offset EQUAL -1)
        message(FATAL_ERROR
            "${shader_var} is not readable, fully lowered GLSL")
    endif()
endforeach()

if(GLSLANG_VALIDATOR)
    execute_process(
        COMMAND "${GLSLANG_VALIDATOR}" -l "${VERTEX_OUTPUT}"
            "${FRAGMENT_OUTPUT}"
        RESULT_VARIABLE validator_result
        OUTPUT_VARIABLE validator_stdout
        ERROR_VARIABLE validator_stderr
    )
    if(NOT validator_result EQUAL 0)
        message(FATAL_ERROR
            "glslang link failed:\n${validator_stdout}${validator_stderr}")
    endif()
endif()

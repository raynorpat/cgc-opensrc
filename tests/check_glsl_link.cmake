foreach(required CGC VERTEX_SOURCE FRAGMENT_SOURCE VERTEX_ACTUAL
        FRAGMENT_ACTUAL)
    if(NOT DEFINED ${required})
        message(FATAL_ERROR "${required} must be defined")
    endif()
endforeach()

include("${CMAKE_CURRENT_LIST_DIR}/check_config_output.cmake")
prepare_config_output("${VERTEX_ACTUAL}")
prepare_config_output("${FRAGMENT_ACTUAL}")

file(REMOVE "${VERTEX_ACTUAL}" "${FRAGMENT_ACTUAL}")
execute_process(
    COMMAND "${CGC}" -quiet -profile glslv -o "${VERTEX_ACTUAL}"
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
    COMMAND "${CGC}" -quiet -profile glslf -o "${FRAGMENT_ACTUAL}"
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

file(READ "${VERTEX_ACTUAL}" vertex_text)
file(READ "${FRAGMENT_ACTUAL}" fragment_text)
string(REPLACE "\r\n" "\n" vertex_text "${vertex_text}")
string(REPLACE "\r" "\n" vertex_text "${vertex_text}")
string(REPLACE "\r\n" "\n" fragment_text "${fragment_text}")
string(REPLACE "\r" "\n" fragment_text "${fragment_text}")

# Core 1.50 interstage rule: every shared interface keeps one canonical
# name across the pair -- the producer declares it "out <type> name;"
# and the consumer declares it "in <type> name;" (a stage that also
# reads and writes the same semantic renames only its own copy).
foreach(interface cg_COLOR0 cg_TEXCOORD0 cg_TEXCOORD1)
    string(FIND "${vertex_text}" "out vec4 ${interface};" vertex_found)
    string(FIND "${fragment_text}" "in vec4 ${interface};" fragment_found)
    if(vertex_found EQUAL -1 OR fragment_found EQUAL -1)
        message(FATAL_ERROR
            "linked interface ${interface} does not have the same canonical name")
    endif()
endforeach()
# A vertex input carrying COLOR0 keeps its collision-suffixed name;
# the unsuffixed spelling belongs to the vertex output alone.
string(FIND "${vertex_text}" "in vec4 cg_COLOR0;" unsuffixed_input)
if(NOT unsuffixed_input EQUAL -1)
    message(FATAL_ERROR "COLOR0 vertex input stole the canonical output name")
endif()
foreach(shader_var vertex_text fragment_text)
    # Judge the version directive on banner-stripped text; the writer
    # emits #version 150 first, but the output transaction wraps the
    # shader in compiler banner comments.
    string(REGEX REPLACE "(^|\n)// cgc version [^\n]*\n" "\\1"
        "${shader_var}_normalized" "${${shader_var}}")
    string(REGEX REPLACE "(^|\n)// command line args:[^\n]*\n" "\\1"
        "${shader_var}_normalized" "${${shader_var}_normalized}")
    if(NOT "${${shader_var}_normalized}" MATCHES "^#version 150\n")
        message(FATAL_ERROR "${shader_var} does not begin with #version 150")
    endif()
endforeach()

if(GLSLANG_VALIDATOR)
    execute_process(
        COMMAND "${GLSLANG_VALIDATOR}" -l "${VERTEX_ACTUAL}"
            "${FRAGMENT_ACTUAL}"
        RESULT_VARIABLE validator_result
        OUTPUT_VARIABLE validator_stdout
        ERROR_VARIABLE validator_stderr
    )
    if(NOT validator_result EQUAL 0)
        message(FATAL_ERROR
            "glslang link failed:\n${validator_stdout}${validator_stderr}")
    endif()
endif()

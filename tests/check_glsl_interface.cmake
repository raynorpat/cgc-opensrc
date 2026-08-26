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

# Core 1.50 interstage interfaces are directional: the pair links
# through the producer's "out" declarations and the consumer's "in"
# declarations, so extract each direction separately and ignore core
# built-ins (gl_*), which core 1.50 defines per stage.  A same-stage
# read+write of one semantic renames only that stage's own copy (the
# fragment output becomes cg_COLOR0_1 while the consumed input keeps
# cg_COLOR0), so cross-stage pairing compares vertex outs against
# fragment ins, never same-direction sets.
string(REGEX MATCHALL "(^|\n)out [^;{\n]+" raw_vertex_outs
    "${vertex_interface_text}")
string(REGEX MATCHALL "(^|\n)in [^;{\n]+" raw_fragment_ins
    "${fragment_interface_text}")
set(vertex_outs "")
set(fragment_ins "")
foreach(declaration ${raw_vertex_outs})
    string(REPLACE "\n" "" declaration "${declaration}")
    if(NOT declaration MATCHES " gl_")
        list(APPEND vertex_outs "${declaration}")
    endif()
endforeach()
foreach(declaration ${raw_fragment_ins})
    string(REPLACE "\n" "" declaration "${declaration}")
    if(NOT declaration MATCHES " gl_")
        list(APPEND fragment_ins "${declaration}")
    endif()
endforeach()
list(SORT vertex_outs)
list(SORT fragment_ins)
# Pair on type and name: strip each declaration's own direction
# keyword, then require the vertex producer set to equal the fragment
# consumer set.
set(vertex_pairs "")
set(fragment_pairs "")
foreach(declaration ${vertex_outs})
    string(REGEX REPLACE "^out " "" interface "${declaration}")
    list(APPEND vertex_pairs "${interface}")
endforeach()
foreach(declaration ${fragment_ins})
    string(REGEX REPLACE "^in " "" interface "${declaration}")
    list(APPEND fragment_pairs "${interface}")
endforeach()
list(SORT vertex_pairs)
list(SORT fragment_pairs)
if(NOT vertex_pairs STREQUAL fragment_pairs)
    message(FATAL_ERROR
        "interface mismatch\nvertex out: ${vertex_outs}\n"
        "fragment in: ${fragment_ins}")
endif()
set(expected_vertex_outs
    "out vec2 cg_TEXCOORD0"
    "out vec4 cg_COLOR0")
set(expected_fragment_ins
    "in vec2 cg_TEXCOORD0"
    "in vec4 cg_COLOR0")
if(NOT vertex_outs STREQUAL expected_vertex_outs)
    message(FATAL_ERROR
        "unexpected linked vertex interface: ${vertex_outs}")
endif()
if(NOT fragment_ins STREQUAL expected_fragment_ins)
    message(FATAL_ERROR
        "unexpected linked fragment interface: ${fragment_ins}")
endif()

# The fragment reads COLOR0 and writes COLOR0: its output copy must be
# renamed cg_COLOR0_1 so the two global declarations stay distinct,
# while the input keeps the canonical name shared with the vertex out.
string(FIND "${fragment_text}" "out vec4 cg_COLOR0_1;"
    fragment_renamed_output)
string(FIND "${fragment_text}" "out vec4 cg_COLOR0;"
    fragment_plain_output)
if(fragment_renamed_output EQUAL -1 OR NOT fragment_plain_output EQUAL -1)
    message(FATAL_ERROR
        "fragment read+write of COLOR0 did not rename its output copy")
endif()
foreach(expected_decl "out vec4 cg_COLOR0;"
                      "out vec2 cg_TEXCOORD0;")
    string(FIND "${vertex_text}" "${expected_decl}" vertex_decl_offset)
    if(vertex_decl_offset EQUAL -1)
        message(FATAL_ERROR
            "linked interface is missing ${expected_decl}")
    endif()
endforeach()
foreach(expected_decl "in vec4 cg_COLOR0;"
                      "in vec2 cg_TEXCOORD0;")
    string(FIND "${fragment_text}" "${expected_decl}" fragment_decl_offset)
    if(fragment_decl_offset EQUAL -1)
        message(FATAL_ERROR
            "linked interface is missing ${expected_decl}")
    endif()
endforeach()
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
    string(REGEX MATCHALL "(^|\n)#version([ \t]|\n)"
        version_lines "${${shader_var}_normalized}")
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

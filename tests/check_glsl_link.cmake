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
foreach(varying cg_COLOR0 cg_TEXCOORD0 cg_TEXCOORD1)
    string(FIND "${vertex_text}" "varying vec4 ${varying};" vertex_found)
    string(FIND "${fragment_text}" "varying vec4 ${varying};" fragment_found)
    if(vertex_found EQUAL -1 OR fragment_found EQUAL -1)
        message(FATAL_ERROR
            "linked varying ${varying} does not have the same canonical name")
    endif()
endforeach()
string(FIND "${vertex_text}" "attribute vec4 cg_COLOR0;" unsuffixed_attribute)
if(NOT unsuffixed_attribute EQUAL -1)
    message(FATAL_ERROR "COLOR0 attribute stole the canonical varying name")
endif()

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

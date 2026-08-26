foreach(required CGC VALIDATOR PROFILE STAGE SOURCE OUTPUT)
    if(NOT DEFINED ${required})
        message(FATAL_ERROR "${required} must be defined")
    endif()
endforeach()
if(NOT PROFILE STREQUAL "glslv" AND NOT PROFILE STREQUAL "glslf")
    message(FATAL_ERROR "unsupported GLSL profile ${PROFILE}")
endif()
if(NOT STAGE STREQUAL "vert" AND NOT STAGE STREQUAL "frag")
    message(FATAL_ERROR "unsupported validator stage ${STAGE}")
endif()
if(PROFILE STREQUAL "glslv" AND NOT STAGE STREQUAL "vert")
    message(FATAL_ERROR "glslv must be validated as vert")
endif()
if(PROFILE STREQUAL "glslf" AND NOT STAGE STREQUAL "frag")
    message(FATAL_ERROR "glslf must be validated as frag")
endif()

include("${CMAKE_CURRENT_LIST_DIR}/check_config_output.cmake")
prepare_config_output("${OUTPUT}")
get_filename_component(output_dir "${OUTPUT}" DIRECTORY)
file(MAKE_DIRECTORY "${output_dir}")
file(REMOVE "${OUTPUT}" "${OUTPUT}.normalized")
execute_process(
    COMMAND "${CGC}" -quiet -profile "${PROFILE}" -o "${OUTPUT}"
        "${SOURCE}"
    RESULT_VARIABLE cgc_result
    OUTPUT_VARIABLE cgc_stdout
    ERROR_VARIABLE cgc_stderr
)
if(NOT cgc_result EQUAL 0 OR NOT cgc_stdout STREQUAL "" OR
        NOT cgc_stderr STREQUAL "")
    message(FATAL_ERROR
        "cgc failed (${cgc_result}):\n${cgc_stdout}${cgc_stderr}")
endif()
if(NOT EXISTS "${OUTPUT}")
    message(FATAL_ERROR "cgc did not create ${OUTPUT}")
endif()

file(READ "${OUTPUT}" shader)
string(REPLACE "\r\n" "\n" shader "${shader}")
string(REPLACE "\r" "\n" shader "${shader}")
# Strip the compiler banner/trailer comments so the shader itself is
# judged; this mirrors the normalization check_glsl.cmake applies.
string(REGEX REPLACE "(^|\n)// cgc version [^\n]*\n" "\\1"
    shader "${shader}")
string(REGEX REPLACE "(^|\n)// command line args:[^\n]*\n" "\\1"
    shader "${shader}")
string(REGEX REPLACE "(^|\n)// End of program\n?$" "\\1"
    shader "${shader}")
# Core GLSL 1.50: GlslWriteModule owns the only version directive and
# emits it first, before any declaration or bind comment.
if(NOT shader MATCHES "^#version 150\n")
    message(FATAL_ERROR "generated shader does not begin with #version 150")
endif()
string(REGEX MATCHALL "(^|\n)#version([ \t]|\n)" version_lines "${shader}")
list(LENGTH version_lines version_count)
if(NOT version_count EQUAL 1)
    message(FATAL_ERROR
        "generated shader contains ${version_count} #version directives")
endif()
if(shader MATCHES "(^|\n)[ \t]*#extension" OR
        shader MATCHES "\\$vin|\\$vout")
    message(FATAL_ERROR "generated shader contains non-portable pseudo syntax")
endif()
string(FIND "${shader}" "\nvoid main()\n" main_offset)
if(main_offset EQUAL -1)
    message(FATAL_ERROR "generated shader does not contain readable void main()")
endif()

# Shared core-1.50 token contract: exactly one "#version 150" plus none
# of the compatibility spellings (attribute/varying storage,
# gl_FragColor, texture1D/2D/3D/Cube calls, #extension lines).  The
# contract script recompiles into the same path; the writer is
# deterministic, so it judges exactly the shader read above.
set(ACTUAL "${OUTPUT}")
include("${CMAKE_CURRENT_LIST_DIR}/check_glsl150.cmake")

execute_process(
    COMMAND "${VALIDATOR}" -S "${STAGE}" "${OUTPUT}"
    RESULT_VARIABLE validator_result
    OUTPUT_VARIABLE validator_stdout
    ERROR_VARIABLE validator_stderr
)
if(NOT validator_result EQUAL 0)
    message(FATAL_ERROR
        "glslangValidator failed:\n${validator_stdout}${validator_stderr}")
endif()
file(REMOVE "${OUTPUT}.normalized")

# Core GLSL 1.50 output contract: one generated shader must start with
# exactly one "#version 150" directive and must not contain any
# compatibility-era token: attribute/varying storage, gl_FragColor,
# old texture1D/2D/3D/Cube intrinsic spellings, or an #extension line.
# The profile suffix check lives in validate_glsl.cmake; this script
# owns only the core-1.50 syntax contract.

foreach(required CGC PROFILE SOURCE ACTUAL)
    if(NOT DEFINED ${required})
        message(FATAL_ERROR "${required} must be defined")
    endif()
endforeach()

include("${CMAKE_CURRENT_LIST_DIR}/check_config_output.cmake")
prepare_config_output("${ACTUAL}")

file(REMOVE "${ACTUAL}")
execute_process(
    COMMAND "${CGC}" -quiet -profile "${PROFILE}" -o "${ACTUAL}" "${SOURCE}"
    RESULT_VARIABLE result
    OUTPUT_VARIABLE stdout
    ERROR_VARIABLE stderr
)
if(NOT result EQUAL 0)
    message(FATAL_ERROR
        "${PROFILE} compile failed (${result}):\n${stdout}${stderr}")
endif()
if(NOT EXISTS "${ACTUAL}")
    message(FATAL_ERROR "cgc did not create ${ACTUAL}")
endif()

file(READ "${ACTUAL}" shader)
string(REPLACE "\r\n" "\n" shader "${shader}")
string(REPLACE "\r" "\n" shader "${shader}")
# Strip the compiler banner comments so the shader itself is judged;
# this mirrors the normalization check_glsl.cmake applies.
string(REGEX REPLACE "(^|\n)// cgc version [^\n]*\n" "\\1"
    shader "${shader}")
string(REGEX REPLACE "(^|\n)// command line args:[^\n]*\n" "\\1"
    shader "${shader}")
string(REGEX REPLACE "(^|\n)// End of program\n?$" "\\1"
    shader "${shader}")

if(NOT shader MATCHES "^#version 150\n")
    message(FATAL_ERROR "${ACTUAL} does not begin with #version 150")
endif()
string(REGEX MATCHALL "(^|\n)#version([ \t]|\n)" version_lines "${shader}")
list(LENGTH version_lines version_count)
if(NOT version_count EQUAL 1)
    message(FATAL_ERROR
        "${ACTUAL} contains ${version_count} #version directives")
endif()

set(forbidden
    "(^|[^A-Za-z0-9_])attribute([^A-Za-z0-9_]|$)"
    "(^|[^A-Za-z0-9_])varying([^A-Za-z0-9_]|$)"
    "gl_FragColor"
    "texture1D\\("
    "texture2D\\("
    "texture3D\\("
    "textureCube\\("
    "(^|\\n)[ \\t]*#extension")
foreach(pattern ${forbidden})
    if(shader MATCHES "${pattern}")
        message(FATAL_ERROR
            "${ACTUAL} contains forbidden token /${pattern}/:\n${shader}")
    endif()
endforeach()

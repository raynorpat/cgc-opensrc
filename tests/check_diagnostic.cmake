foreach(required CGC PROFILE SOURCE CODE ACTUAL)
    if(NOT DEFINED ${required})
        message(FATAL_ERROR "${required} must be defined")
    endif()
endforeach()

include("${CMAKE_CURRENT_LIST_DIR}/check_config_output.cmake")
prepare_config_output("${ACTUAL}")

file(REMOVE "${ACTUAL}" "${ACTUAL}.normalized")
execute_process(
    COMMAND "${CGC}" -quiet -profile "${PROFILE}" -o "${ACTUAL}" "${SOURCE}"
    RESULT_VARIABLE result
    OUTPUT_VARIABLE stdout
    ERROR_VARIABLE stderr
)
if(result EQUAL 0)
    message(FATAL_ERROR "expected ${PROFILE} compile to fail for ${SOURCE}")
endif()
set(diagnostics "${stdout}${stderr}")
string(REGEX MATCHALL "error C${CODE}:" requested_matches "${diagnostics}")
list(LENGTH requested_matches requested_count)
if(NOT requested_count EQUAL 1)
    message(FATAL_ERROR
        "expected error C${CODE} exactly once for ${SOURCE}:\n${diagnostics}")
endif()
string(REGEX MATCHALL "(error|warning) C[0-9][0-9][0-9][0-9]:"
    all_matches "${diagnostics}")
list(LENGTH all_matches diagnostic_count)
if(NOT diagnostic_count EQUAL 1)
    message(FATAL_ERROR
        "expected one diagnostic for ${SOURCE}:\n${diagnostics}")
endif()
if(DEFINED EXPECTED_LINE)
    get_filename_component(source_name "${SOURCE}" NAME)
    if(NOT diagnostics MATCHES "${source_name}\\(${EXPECTED_LINE}\\)")
        message(FATAL_ERROR
            "expected ${source_name} line ${EXPECTED_LINE}:\n${diagnostics}")
    endif()
endif()
if(DEFINED MESSAGE AND NOT diagnostics MATCHES "${MESSAGE}")
    message(FATAL_ERROR
        "expected diagnostic text ${MESSAGE}:\n${diagnostics}")
endif()

if(NOT EXISTS "${ACTUAL}")
    message(FATAL_ERROR "cgc did not create ${ACTUAL}")
endif()
file(READ "${ACTUAL}" shader)
string(REPLACE "\r\n" "\n" shader "${shader}")
string(REPLACE "\r" "\n" shader "${shader}")
if(NOT shader MATCHES "(^|\n)// End of program\n?$")
    message(FATAL_ERROR "failed translation did not terminate normally")
endif()
string(REGEX REPLACE "(^|\n)// cgc version [^\n]*\n" "\\1" shader "${shader}")
string(REGEX REPLACE "(^|\n)// command line args:[^\n]*\n" "\\1" shader "${shader}")
string(REGEX REPLACE "(^|\n)// End of program\n?$" "\\1" shader "${shader}")
if(NOT shader STREQUAL "#version 110\n")
    file(WRITE "${ACTUAL}.normalized" "${shader}")
    message(FATAL_ERROR "failed translation emitted a partial shader body")
endif()
file(REMOVE "${ACTUAL}.normalized")

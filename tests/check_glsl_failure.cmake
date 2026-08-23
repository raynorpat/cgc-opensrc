foreach(required CGC PROFILE SOURCE ACTUAL CODE EXPECTED_LINE REASON)
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
if(result EQUAL 0)
    message(FATAL_ERROR
        "${PROFILE} unexpectedly lowered ${SOURCE} without a diagnostic")
endif()
set(diagnostics "${stdout}${stderr}")
string(REGEX MATCHALL "error C[0-9][0-9][0-9][0-9]:" matches
    "${diagnostics}")
list(LENGTH matches match_count)
if(NOT match_count EQUAL 1)
    message(FATAL_ERROR
        "${PROFILE} reported ${match_count} compiler errors:\n${diagnostics}")
endif()
if(NOT diagnostics MATCHES "error C${CODE}:")
    message(FATAL_ERROR
        "${PROFILE} did not report C${CODE}:\n${diagnostics}")
endif()
if(CODE EQUAL 5508 AND
   NOT diagnostics MATCHES "not supported by this profile")
    message(FATAL_ERROR
        "${PROFILE} did not explain the lowering failure:\n${diagnostics}")
endif()
get_filename_component(source_name "${SOURCE}" NAME)
if(NOT diagnostics MATCHES "${source_name}\\(${EXPECTED_LINE}\\)")
    message(FATAL_ERROR
        "${PROFILE} reported the wrong source line:\n${diagnostics}")
endif()
if(NOT diagnostics MATCHES "${REASON}")
    message(FATAL_ERROR
        "${PROFILE} did not identify ${REASON}:\n${diagnostics}")
endif()
if(NOT EXISTS "${ACTUAL}")
    message(FATAL_ERROR "cgc did not create ${ACTUAL}")
endif()
file(READ "${ACTUAL}" actual_text)
string(REPLACE "\r\n" "\n" actual_text "${actual_text}")
string(REPLACE "\r" "\n" actual_text "${actual_text}")
if(NOT actual_text MATCHES "(^|\n)// End of program\n?$")
    message(FATAL_ERROR
        "${PROFILE} did not terminate normally:\n${actual_text}")
endif()
string(REGEX REPLACE "(^|\n)// cgc version [^\n]*\n" "\\1"
    actual_text "${actual_text}")
string(REGEX REPLACE "(^|\n)// command line args:[^\n]*\n" "\\1"
    actual_text "${actual_text}")
string(REGEX REPLACE "(^|\n)// End of program\n?$" "\\1"
    actual_text "${actual_text}")
if(NOT actual_text STREQUAL "#version 110\n")
    file(WRITE "${ACTUAL}.normalized" "${actual_text}")
    message(FATAL_ERROR
        "${PROFILE} wrote a partial shader; actual: ${ACTUAL}.normalized")
endif()

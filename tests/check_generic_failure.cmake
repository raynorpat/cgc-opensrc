foreach(required CGC SOURCE ACTUAL CODE EXPECTED_LINE REASON)
    if(NOT DEFINED ${required})
        message(FATAL_ERROR "${required} must be defined")
    endif()
endforeach()

file(REMOVE "${ACTUAL}")
execute_process(
    COMMAND "${CGC}" -quiet -profile generic -o "${ACTUAL}" "${SOURCE}"
    RESULT_VARIABLE result
    OUTPUT_VARIABLE stdout
    ERROR_VARIABLE stderr
)
if(result EQUAL 0)
    message(FATAL_ERROR
        "generic unexpectedly accepted ${SOURCE}")
endif()
set(diagnostics "${stdout}${stderr}")
string(REGEX MATCHALL "error C[0-9][0-9][0-9][0-9]:" matches
    "${diagnostics}")
list(LENGTH matches match_count)
if(NOT match_count EQUAL 1)
    message(FATAL_ERROR
        "generic reported ${match_count} compiler errors:\n${diagnostics}")
endif()
if(NOT diagnostics MATCHES "error C${CODE}:")
    message(FATAL_ERROR
        "generic did not report C${CODE}:\n${diagnostics}")
endif()
get_filename_component(source_name "${SOURCE}" NAME)
if(NOT diagnostics MATCHES "${source_name}\\(${EXPECTED_LINE}\\)")
    message(FATAL_ERROR
        "generic reported the wrong source line:\n${diagnostics}")
endif()
if(NOT diagnostics MATCHES "${REASON}")
    message(FATAL_ERROR
        "generic did not identify ${REASON}:\n${diagnostics}")
endif()
if(NOT EXISTS "${ACTUAL}")
    message(FATAL_ERROR "cgc did not create ${ACTUAL}")
endif()
file(READ "${ACTUAL}" actual_text)
string(REPLACE "\r\n" "\n" actual_text "${actual_text}")
string(REPLACE "\r" "\n" actual_text "${actual_text}")
if(NOT actual_text MATCHES "(^|\n)// End of program\n?$")
    message(FATAL_ERROR
        "generic did not terminate normally:\n${actual_text}")
endif()
string(REGEX REPLACE "(^|\n)// cgc version [^\n]*\n" "\\1"
    actual_text "${actual_text}")
string(REGEX REPLACE "(^|\n)// command line args:[^\n]*\n" "\\1"
    actual_text "${actual_text}")
string(REGEX REPLACE "(^|\n)// End of program\n?$" "\\1"
    actual_text "${actual_text}")
if(NOT actual_text STREQUAL "# Generic output by Cg compiler\n")
    file(WRITE "${ACTUAL}.normalized" "${actual_text}")
    message(FATAL_ERROR
        "generic wrote successful output; actual: ${ACTUAL}.normalized")
endif()

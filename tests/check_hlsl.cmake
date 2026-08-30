foreach(required CGC PROFILE SOURCE EXPECTED)
    if(NOT DEFINED ${required})
        message(FATAL_ERROR "${required} must be defined")
    endif()
endforeach()

include("${CMAKE_CURRENT_LIST_DIR}/check_config_output.cmake")
prepare_config_output("${ACTUAL}")
get_filename_component(source_directory "${SOURCE}" DIRECTORY)

file(REMOVE "${ACTUAL}" "${ACTUAL}.normalized")
execute_process(
    COMMAND "${CGC}" -quiet -profile "${PROFILE}" -o "${ACTUAL}" "${SOURCE}"
    WORKING_DIRECTORY "${source_directory}"
    RESULT_VARIABLE result
    OUTPUT_VARIABLE stdout
    ERROR_VARIABLE stderr
)
if(NOT result EQUAL 0)
    message(FATAL_ERROR
        "${PROFILE} compile failed (${result}):\n${stdout}${stderr}")
endif()
if(NOT stdout STREQUAL "")
    message(FATAL_ERROR
        "${PROFILE} compile wrote to stdout:\n${stdout}")
endif()
if(NOT stderr STREQUAL "")
    message(FATAL_ERROR
        "${PROFILE} compile wrote to stderr:\n${stderr}")
endif()
if(NOT EXISTS "${ACTUAL}")
    message(FATAL_ERROR "cgc did not create ${ACTUAL}")
endif()

file(READ "${ACTUAL}" actual_text)
string(REPLACE "\r\n" "\n" actual_text "${actual_text}")
string(REPLACE "\r" "\n" actual_text "${actual_text}")
string(REGEX REPLACE "(^|\n)// cgc version [^\n]*\n" "\\1"
    actual_text "${actual_text}")
string(REGEX REPLACE "(^|\n)// command line args:[^\n]*\n" "\\1"
    actual_text "${actual_text}")
string(REGEX REPLACE "(^|\n)// End of program\n?$" "\\1"
    actual_text "${actual_text}")

if(DEFINED UPDATE_EXPECTED AND UPDATE_EXPECTED)
    file(WRITE "${EXPECTED}" "${actual_text}")
    return()
endif()

file(READ "${EXPECTED}" expected_text)
string(REPLACE "\r\n" "\n" expected_text "${expected_text}")
string(REPLACE "\r" "\n" expected_text "${expected_text}")
if(NOT actual_text STREQUAL expected_text)
    file(WRITE "${ACTUAL}.normalized" "${actual_text}")
    message(FATAL_ERROR
        "HLSL output differs from ${EXPECTED}; actual: ${ACTUAL}.normalized")
endif()

file(REMOVE "${ACTUAL}.normalized")

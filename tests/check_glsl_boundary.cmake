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
if(NOT result EQUAL 0 OR NOT stdout STREQUAL "" OR NOT stderr STREQUAL "")
    message(FATAL_ERROR
        "boundary shader failed (${result}):\nstdout=${stdout}\nstderr=${stderr}")
endif()
if(NOT EXISTS "${ACTUAL}")
    message(FATAL_ERROR "cgc did not create ${ACTUAL}")
endif()
file(READ "${ACTUAL}" shader)
if(NOT shader MATCHES "#version 110" OR
   NOT shader MATCHES "void main\\(\\)")
    message(FATAL_ERROR "boundary shader output is incomplete")
endif()

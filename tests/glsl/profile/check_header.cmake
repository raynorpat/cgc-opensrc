foreach(required CGC PROFILE SOURCE)
    if(NOT DEFINED ${required})
        message(FATAL_ERROR "${required} must be defined")
    endif()
endforeach()

execute_process(
    COMMAND "${CGC}" -quiet -nocode -profile "${PROFILE}" "${SOURCE}"
    RESULT_VARIABLE result
    OUTPUT_VARIABLE output
    ERROR_VARIABLE error
)
if(NOT result EQUAL 0)
    message(FATAL_ERROR "${PROFILE} registration failed: ${output}${error}")
endif()
string(REPLACE "\r\n" "\n" output "${output}")
if(NOT output MATCHES "^#version 110\n")
    message(FATAL_ERROR "${PROFILE} did not emit #version 110 first:\n${output}")
endif()

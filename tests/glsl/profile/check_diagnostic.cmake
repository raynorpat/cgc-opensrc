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
if(result EQUAL 0)
    message(FATAL_ERROR "${PROFILE} unexpectedly accepted unsupported connector")
endif()
set(diagnostics "${output}${error}")
if(NOT diagnostics MATCHES "error C5001:")
    message(FATAL_ERROR "${PROFILE} did not report C5001:\n${diagnostics}")
endif()

foreach(required CGC PROFILE SOURCE CODE)
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
    message(FATAL_ERROR "${PROFILE} unexpectedly accepted ${SOURCE}")
endif()
set(diagnostics "${output}${error}")
if(NOT diagnostics MATCHES "error C${CODE}:")
    message(FATAL_ERROR "${PROFILE} did not report C${CODE}:\n${diagnostics}")
endif()
if(NOT diagnostics MATCHES "// End of program")
    message(FATAL_ERROR "${PROFILE} did not complete normally:\n${diagnostics}")
endif()

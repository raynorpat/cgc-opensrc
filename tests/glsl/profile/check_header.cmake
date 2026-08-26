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
# Core 1.50: the module writer owns the only version directive, so a
# -nocode compile emits no header at all.
if(output MATCHES "#version")
    message(FATAL_ERROR
        "${PROFILE} emitted a version directive without codegen:\n${output}")
endif()

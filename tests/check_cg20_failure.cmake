execute_process(
    COMMAND "${CGC}" -quiet -nocode -profile "${PROFILE}" ${EXTRA_ARGS} "${SOURCE}"
    RESULT_VARIABLE result
    OUTPUT_VARIABLE output
    ERROR_VARIABLE error
)
if(result EQUAL 0)
    message(FATAL_ERROR "Cg 2.0 failure fixture unexpectedly succeeded")
endif()
set(diagnostic "${output}${error}")
if(NOT diagnostic MATCHES "${CODE}")
    message(FATAL_ERROR "missing diagnostic code ${CODE}:\n${diagnostic}")
endif()
if(NOT diagnostic MATCHES "${MESSAGE}")
    message(FATAL_ERROR "missing diagnostic '${MESSAGE}':\n${diagnostic}")
endif()

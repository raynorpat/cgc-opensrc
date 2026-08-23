execute_process(
    COMMAND "${CGC}" -quiet -nocode -profile "${PROFILE}" ${EXTRA_ARGS} "${SOURCE}"
    RESULT_VARIABLE result
    OUTPUT_VARIABLE output
    ERROR_VARIABLE error
)
if(NOT result EQUAL 0)
    message(FATAL_ERROR
        "Cg 2.0 success fixture failed (${result})\nstdout:\n${output}\nstderr:\n${error}")
endif()
if(DEFINED MESSAGE AND NOT "${output}${error}" MATCHES "${MESSAGE}")
    message(FATAL_ERROR
        "Cg 2.0 fixture did not match '${MESSAGE}'\nstdout:\n${output}\nstderr:\n${error}")
endif()

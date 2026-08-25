# Checks that a helper-structure fixture is rejected by the selected
# profile family: cgc must fail and report the unknown profile
# specifier for the undeclared predefined structure name.  Parse
# recovery legitimately cascades afterwards, so unlike
# check_generic_failure this script asserts on the presence of the
# leading diagnostic rather than an exact error count.

foreach(required CGC PROFILE SOURCE CODE REASON)
    if(NOT DEFINED ${required})
        message(FATAL_ERROR "${required} must be defined")
    endif()
endforeach()

execute_process(
    COMMAND "${CGC}" -quiet -nocode -profile "${PROFILE}" "${SOURCE}"
    RESULT_VARIABLE result
    OUTPUT_VARIABLE stdout
    ERROR_VARIABLE stderr
)
if(result EQUAL 0)
    message(FATAL_ERROR
        "${PROFILE} unexpectedly accepted ${SOURCE}")
endif()
set(diagnostics "${stdout}${stderr}")
if(NOT diagnostics MATCHES "error C${CODE}:")
    message(FATAL_ERROR
        "${PROFILE} did not report C${CODE}:\n${diagnostics}")
endif()
if(NOT diagnostics MATCHES "${REASON}")
    message(FATAL_ERROR
        "${PROFILE} did not identify ${REASON}:\n${diagnostics}")
endif()

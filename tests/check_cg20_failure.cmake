if(NOT DEFINED CODE)
    message(FATAL_ERROR "check_cg20_failure.cmake requires CODE")
endif()
if(NOT DEFINED MESSAGE)
    message(FATAL_ERROR "check_cg20_failure.cmake requires MESSAGE")
endif()
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

# NOTES pins layered notes in document order: each entry must appear
# literally (quotes stripped from the diagnostic first) after the
# previous one, so the call-path hop order is part of the contract.

if(DEFINED NOTES)
    string(REPLACE "\"" "" clean_diagnostic "${diagnostic}")
    set(remaining_notes "${clean_diagnostic}")
    foreach(note IN LISTS NOTES)
        string(FIND "${remaining_notes}" "${note}" note_found)
        if(note_found EQUAL -1)
            message(FATAL_ERROR
                "missing ordered note '${note}':\n${diagnostic}")
        endif()
        string(SUBSTRING "${remaining_notes}" ${note_found} -1
            remaining_notes)
    endforeach()
endif()

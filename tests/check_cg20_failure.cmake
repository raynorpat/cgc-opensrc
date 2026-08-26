if(NOT DEFINED CODE)
    message(FATAL_ERROR "check_cg20_failure.cmake requires CODE")
endif()
if(NOT DEFINED MESSAGE)
    message(FATAL_ERROR "check_cg20_failure.cmake requires MESSAGE")
endif()

# Geometry mode (Task 5): ENTRY selects one program entry and
# PROFILE_OPTIONS is a semicolon list where every item becomes a
# separate "-po <option>" pair.  EXPECTED is accepted for symmetry with
# the success runner; failure fixtures pin diagnostics, not output.
# The run keeps -nocode so a failed compilation never publishes.

set(cg20_profile_args)
if(DEFINED PROFILE_OPTIONS)
    foreach(cg20_option IN LISTS PROFILE_OPTIONS)
        list(APPEND cg20_profile_args -po "${cg20_option}")
    endforeach()
endif()
set(cg20_entry_args)
if(DEFINED ENTRY)
    list(APPEND cg20_entry_args -entry "${ENTRY}")
endif()
execute_process(
    COMMAND "${CGC}" -quiet -nocode -profile "${PROFILE}" ${cg20_profile_args}
        ${cg20_entry_args} ${EXTRA_ARGS} "${SOURCE}"
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

# EXPECTED_LINE optionally pins the diagnostic's source location.  Most
# diagnostics cite "<source>(<line>)"; profile-binder rejections may
# carry a bare "(0)" with no file component, so anchor on the
# "(<line>) : error" location token itself.  Undefined means no line
# check, preserving existing consumers.

if(DEFINED EXPECTED_LINE)
    if(NOT diagnostic MATCHES "\\(${EXPECTED_LINE}\\) : error")
        message(FATAL_ERROR
            "diagnostic does not cite line ${EXPECTED_LINE}:\n${diagnostic}")
    endif()
endif()

# NOTES pins layered notes in document order: each entry must appear
# literally (quotes stripped from the diagnostic first) after the
# previous one, so the call-path hop order is part of the contract.

if(DEFINED NOTES)
    # Registrations escape the separators so the note list survives
    # the add_test COMMAND expansion as one argument; undo that
    # escaping here so every note becomes one ordered entry.
    string(REPLACE "\\;" ";" NOTES "${NOTES}")
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

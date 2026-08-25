foreach(required CGC PROFILE SOURCE CODE ACTUAL)
    if(NOT DEFINED ${required})
        message(FATAL_ERROR "${required} must be defined")
    endif()
endforeach()

include("${CMAKE_CURRENT_LIST_DIR}/check_config_output.cmake")
prepare_config_output("${ACTUAL}")

file(REMOVE "${ACTUAL}" "${ACTUAL}.normalized")
execute_process(
    COMMAND "${CGC}" -quiet -profile "${PROFILE}" -o "${ACTUAL}" "${SOURCE}"
    RESULT_VARIABLE result
    OUTPUT_VARIABLE stdout
    ERROR_VARIABLE stderr
)
if(result EQUAL 0)
    message(FATAL_ERROR "expected ${PROFILE} compile to fail for ${SOURCE}")
endif()
set(diagnostics "${stdout}${stderr}")
string(REGEX MATCHALL "error C${CODE}:" requested_matches "${diagnostics}")
list(LENGTH requested_matches requested_count)
if(NOT requested_count EQUAL 1)
    message(FATAL_ERROR
        "expected error C${CODE} exactly once for ${SOURCE}:\n${diagnostics}")
endif()
string(REGEX MATCHALL "(error|warning) C[0-9][0-9][0-9][0-9]:"
    all_matches "${diagnostics}")
list(LENGTH all_matches diagnostic_count)
if(NOT diagnostic_count EQUAL 1)
    message(FATAL_ERROR
        "expected one diagnostic for ${SOURCE}:\n${diagnostics}")
endif()
if(DEFINED EXPECTED_LINE)
    get_filename_component(source_name "${SOURCE}" NAME)
    if(NOT diagnostics MATCHES "${source_name}\\(${EXPECTED_LINE}\\)")
        message(FATAL_ERROR
            "expected ${source_name} line ${EXPECTED_LINE}:\n${diagnostics}")
    endif()
endif()
if(DEFINED MESSAGE AND NOT diagnostics MATCHES "${MESSAGE}")
    message(FATAL_ERROR
        "expected diagnostic text ${MESSAGE}:\n${diagnostics}")
endif()

# NOTES pins layered notes in document order: each entry must appear
# literally (quotes stripped from the diagnostics first) after the
# previous one, so the call-path hop order is part of the contract.

if(DEFINED NOTES)
    string(REPLACE "\"" "" clean_diagnostic "${diagnostics}")
    set(remaining_notes "${clean_diagnostic}")
    foreach(note IN LISTS NOTES)
        string(FIND "${remaining_notes}" "${note}" note_found)
        if(note_found EQUAL -1)
            message(FATAL_ERROR
                "missing ordered note '${note}':\n${diagnostics}")
        endif()
        string(SUBSTRING "${remaining_notes}" ${note_found} -1
            remaining_notes)
    endforeach()
endif()

# Transactional output (Task 18): a failed translation publishes
# nothing.  The compiler generates into a same-directory temporary and
# aborts on failure, so the -o destination must not exist afterwards --
# in particular no partial shader body can replace a previous program.

if(EXISTS "${ACTUAL}")
    file(READ "${ACTUAL}" published)
    message(FATAL_ERROR
        "failed compilation published an output file:\n${published}")
endif()

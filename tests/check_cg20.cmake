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

# FORBID pins an absence: the regex may not appear in the combined
# output.  Used for exclusion rules such as a static global staying out
# of the program-interface listing.

if(DEFINED FORBID AND "${output}${error}" MATCHES "${FORBID}")
    message(FATAL_ERROR
        "Cg 2.0 fixture output contains forbidden '${FORBID}':\nstdout:\n${output}\nstderr:\n${error}")
endif()

# EXPECTED golden mode: generate the program to ACTUAL via -o, strip the
# volatile banner lines the compiler writes into every output stream, and
# compare the normalized text byte-for-byte with the checked-in file.

if(DEFINED EXPECTED)
    foreach(required CGC PROFILE SOURCE EXPECTED ACTUAL)
        if(NOT DEFINED ${required})
            message(FATAL_ERROR "${required} must be defined for EXPECTED")
        endif()
    endforeach()
    get_filename_component(cg20_expected_dir "${ACTUAL}" DIRECTORY)
    file(MAKE_DIRECTORY "${cg20_expected_dir}")
    file(REMOVE "${ACTUAL}" "${ACTUAL}.normalized")
    execute_process(
        COMMAND "${CGC}" -quiet -profile "${PROFILE}" ${EXTRA_ARGS}
            -o "${ACTUAL}" "${SOURCE}"
        RESULT_VARIABLE result
        OUTPUT_VARIABLE output
        ERROR_VARIABLE error
    )
    if(NOT result EQUAL 0)
        message(FATAL_ERROR
            "Cg 2.0 normalized fixture failed (${result})\nstdout:\n${output}\nstderr:\n${error}")
    endif()
    if(NOT output STREQUAL "")
        message(FATAL_ERROR
            "Cg 2.0 normalized fixture wrote to stdout:\n${output}")
    endif()
    file(READ "${ACTUAL}" actual_text)
    string(REPLACE "\r\n" "\n" actual_text "${actual_text}")
    string(REPLACE "\r" "\n" actual_text "${actual_text}")
    string(REGEX REPLACE "(^|\n)// cgc version [^\n]*\n" "\\1"
        actual_text "${actual_text}")
    string(REGEX REPLACE "(^|\n)// command line args:[^\n]*\n" "\\1"
        actual_text "${actual_text}")
    string(REGEX REPLACE "(^|\n)// End of program\n?$" "\\1"
        actual_text "${actual_text}")

    # file(WRITE) emits CRLF on Windows, so round-trip both sides
    # through it to make the byte comparison ending-agnostic while the
    # checked-in expectation stays LF.
    file(READ "${EXPECTED}" expected_text)
    string(REPLACE "\r\n" "\n" expected_text "${expected_text}")
    string(REPLACE "\r" "\n" expected_text "${expected_text}")
    set(cg20_expected_actual "${ACTUAL}.expected")
    file(WRITE "${ACTUAL}.normalized" "${actual_text}")
    file(WRITE "${cg20_expected_actual}" "${expected_text}")
    execute_process(
        COMMAND "${CMAKE_COMMAND}" -E compare_files
            "${ACTUAL}.normalized" "${cg20_expected_actual}"
        RESULT_VARIABLE result
    )
    if(NOT result EQUAL 0)
        message(FATAL_ERROR
            "normalized Cg IR differs from ${EXPECTED}; actual: ${ACTUAL}.normalized")
    endif()
    file(REMOVE "${ACTUAL}.normalized" "${cg20_expected_actual}")
endif()

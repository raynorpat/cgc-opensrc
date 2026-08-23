foreach(required TEST_NAME CGC PROFILE SOURCE EXPECT_SUCCESS WORK_DIR)
    if(NOT DEFINED ${required})
        message(FATAL_ERROR "${required} is required")
    endif()
endforeach()

file(MAKE_DIRECTORY "${WORK_DIR}")
set(output_file "${WORK_DIR}/${TEST_NAME}.arb")
file(REMOVE "${output_file}")

execute_process(
    COMMAND "${CGC}" -quiet -profile "${PROFILE}" -entry main
            -o "${output_file}" ${EXTRA_ARGS} "${SOURCE}"
    RESULT_VARIABLE result
    OUTPUT_VARIABLE stdout
    ERROR_VARIABLE stderr
)

function(normalize_arb input output_name)
    string(REPLACE "\r\n" "\n" normalized "${input}")
    string(REGEX REPLACE "(^|\n)# cgc version[^\n]*\n" "\\1"
        normalized "${normalized}")
    string(REGEX REPLACE "(^|\n)# command line args:[^\n]*\n" "\\1"
        normalized "${normalized}")
    string(REGEX REPLACE "(^|\n)# End of program\n?$" "\\1"
        normalized "${normalized}")
    set(${output_name} "${normalized}" PARENT_SCOPE)
endfunction()

if(EXPECT_SUCCESS)
    if(NOT result EQUAL 0)
        message(FATAL_ERROR "${TEST_NAME} failed (${result}):\n${stdout}${stderr}")
    endif()
    if(NOT EXISTS "${output_file}")
        message(FATAL_ERROR "${TEST_NAME} did not create ${output_file}")
    endif()
    file(READ "${output_file}" actual)
    normalize_arb("${actual}" actual)
    if(DEFINED EXPECTED_OUTPUT)
        file(READ "${EXPECTED_OUTPUT}" expected)
        normalize_arb("${expected}" expected)
        if(NOT actual STREQUAL expected)
            file(WRITE "${WORK_DIR}/${TEST_NAME}.actual" "${actual}")
            message(FATAL_ERROR
                "${TEST_NAME} output mismatch; actual saved to "
                "${WORK_DIR}/${TEST_NAME}.actual"
            )
        endif()
    endif()
    if(DEFINED SMOKE AND DEFINED STAGE)
        execute_process(
            COMMAND "${SMOKE}" "${STAGE}" "${output_file}"
            RESULT_VARIABLE smoke_result
            OUTPUT_VARIABLE smoke_stdout
            ERROR_VARIABLE smoke_stderr
        )
        if(smoke_result EQUAL 77)
            message("SKIP: ${smoke_stdout}${smoke_stderr}")
        elseif(NOT smoke_result EQUAL 0)
            message(FATAL_ERROR "ARB driver load failed:\n${smoke_stdout}${smoke_stderr}")
        endif()
    endif()
else()
    if(result EQUAL 0)
        message(FATAL_ERROR "${TEST_NAME} unexpectedly succeeded")
    endif()
    if(EXISTS "${output_file}")
        file(READ "${output_file}" failed_output)
        foreach(forbidden "#var " "#const " "#default " "TEMP " "PARAM "
                          "ATTRIB " "OUTPUT " "ADDRESS " "END")
            string(FIND "${failed_output}" "${forbidden}" found)
            if(NOT found EQUAL -1)
                message(FATAL_ERROR
                    "${TEST_NAME} left partial backend output containing ${forbidden}"
                )
            endif()
        endforeach()
    endif()
    set(diagnostics "${stdout}${stderr}")
    if(DEFINED EXPECTED_DIAGNOSTICS)
        file(STRINGS "${EXPECTED_DIAGNOSTICS}" fragments)
        foreach(fragment IN LISTS fragments)
            if(NOT fragment STREQUAL "")
                string(FIND "${diagnostics}" "${fragment}" found)
                if(found EQUAL -1)
                    message(FATAL_ERROR
                        "missing diagnostic fragment '${fragment}':\n${diagnostics}"
                    )
                endif()
            endif()
        endforeach()
    endif()
endif()

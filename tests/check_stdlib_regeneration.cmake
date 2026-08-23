foreach(required_variable TOKENIZE INPUT EXPECTED SCRIPT SOURCE_DIR WORK_DIR)
    if(NOT DEFINED ${required_variable})
        message(FATAL_ERROR "${required_variable} must be defined")
    endif()
endforeach()

file(MAKE_DIRECTORY "${WORK_DIR}")
set(first_output "${WORK_DIR}/stdlib-first.c")
set(second_output "${WORK_DIR}/stdlib-second.c")
set(first_temp "${WORK_DIR}/stdlib-generic-first.cg")
set(second_temp "${WORK_DIR}/stdlib-generic-second.cg")
file(REMOVE "${first_output}" "${second_output}"
            "${first_temp}" "${second_temp}")

execute_process(
    COMMAND "${CMAKE_COMMAND}"
        -DTOKENIZE=${TOKENIZE}
        -DINPUT=${INPUT}
        -DOUTPUT=${first_output}
        -DTEMP=${first_temp}
        -P ${SCRIPT}
    WORKING_DIRECTORY "${SOURCE_DIR}"
    RESULT_VARIABLE first_result
    OUTPUT_VARIABLE first_stdout
    ERROR_VARIABLE first_stderr
)
if(NOT first_result EQUAL 0)
    message(FATAL_ERROR
        "First stdlib regeneration failed: ${first_stdout}${first_stderr}")
endif()

execute_process(
    COMMAND "${CMAKE_COMMAND}"
        -DTOKENIZE=${TOKENIZE}
        -DINPUT=${INPUT}
        -DOUTPUT=${second_output}
        -DTEMP=${second_temp}
        -P ${SCRIPT}
    WORKING_DIRECTORY "${SOURCE_DIR}"
    RESULT_VARIABLE second_result
    OUTPUT_VARIABLE second_stdout
    ERROR_VARIABLE second_stderr
)
if(NOT second_result EQUAL 0)
    message(FATAL_ERROR
        "Second stdlib regeneration failed: ${second_stdout}${second_stderr}")
endif()

file(SHA256 "${EXPECTED}" expected_hash)
file(SHA256 "${first_output}" first_hash)
file(SHA256 "${second_output}" second_hash)
if(NOT first_hash STREQUAL expected_hash)
    message(FATAL_ERROR
        "Regenerated stdlib.c changed: ${first_hash} != ${expected_hash}")
endif()
if(NOT second_hash STREQUAL first_hash)
    message(FATAL_ERROR
        "Repeated stdlib regeneration changed: ${second_hash} != ${first_hash}")
endif()

if(NOT DEFINED UNIT)
    message(FATAL_ERROR "UNIT must be defined")
endif()

execute_process(
    COMMAND "${UNIT}" --verify-assertions-active
    RESULT_VARIABLE result
    OUTPUT_VARIABLE stdout
    ERROR_VARIABLE stderr
)
if(NOT result EQUAL 0)
    message(FATAL_ERROR "assertion probe failed (${result}):\n${stdout}${stderr}")
endif()
if(NOT stdout STREQUAL "glsl-ir-assertions-active\n" OR
        NOT stderr STREQUAL "")
    message(FATAL_ERROR
        "assertion probe did not prove active assertions:\n${stdout}${stderr}")
endif()

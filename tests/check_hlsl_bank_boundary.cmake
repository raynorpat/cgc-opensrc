foreach(required CGC PROFILE SOURCE EXPECTED ACTUAL BANK TYPE COUNT)
    if(NOT DEFINED ${required})
        message(FATAL_ERROR "${required} must be defined")
    endif()
endforeach()

include("${CMAKE_CURRENT_LIST_DIR}/check_config_output.cmake")
prepare_config_output("${ACTUAL}")
get_filename_component(source_directory "${SOURCE}" DIRECTORY)

file(REMOVE "${ACTUAL}" "${ACTUAL}.fxc")
execute_process(
    COMMAND "${CGC}" -quiet -nowarn -profile "${PROFILE}"
        -o "${ACTUAL}" "${SOURCE}"
    WORKING_DIRECTORY "${source_directory}"
    RESULT_VARIABLE result
    OUTPUT_VARIABLE stdout
    ERROR_VARIABLE stderr
)
if(NOT result EQUAL 0)
    message(FATAL_ERROR
        "${PROFILE} compile failed (${result}):\n${stdout}${stderr}")
endif()
if(NOT stdout STREQUAL "" OR NOT stderr STREQUAL "")
    message(FATAL_ERROR
        "${PROFILE} compile produced diagnostics:\n${stdout}${stderr}")
endif()
if(NOT EXISTS "${ACTUAL}")
    message(FATAL_ERROR "cgc did not create ${ACTUAL}")
endif()

file(READ "${ACTUAL}" actual_text)
string(REGEX MATCHALL
    "uniform ${TYPE} cg_${BANK}[0-9]+ : register\\(${BANK}[0-9]+\\)"
    actual_declarations "${actual_text}")
list(LENGTH actual_declarations actual_count)
if(NOT actual_count EQUAL COUNT)
    message(FATAL_ERROR
        "expected ${COUNT} ${BANK}-bank declarations, found ${actual_count}")
endif()
string(REPLACE ";" ";\n" actual_declarations "${actual_declarations}")
string(APPEND actual_declarations ";\n")

file(READ "${EXPECTED}" expected_declarations)
string(REPLACE "\r\n" "\n" expected_declarations "${expected_declarations}")
string(REPLACE "\r" "\n" expected_declarations "${expected_declarations}")
if(NOT actual_declarations STREQUAL expected_declarations)
    message(FATAL_ERROR
        "${BANK}-bank declarations differ from ${EXPECTED}:\n${actual_declarations}")
endif()

if(DEFINED FXC AND NOT FXC STREQUAL "" AND EXISTS "${FXC}")
    execute_process(
        COMMAND "${FXC}" /nologo /Gec /WX /T vs_3_0 /E main
            /Fo "${ACTUAL}.fxc" "${ACTUAL}"
        RESULT_VARIABLE fxc_result
        OUTPUT_VARIABLE fxc_stdout
        ERROR_VARIABLE fxc_stderr
    )
    if(NOT fxc_result EQUAL 0)
        message(FATAL_ERROR
            "FXC rejected ${ACTUAL} (${fxc_result}):\n${fxc_stdout}${fxc_stderr}")
    endif()
    file(REMOVE "${ACTUAL}.fxc")
endif()

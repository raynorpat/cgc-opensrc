foreach(required CGC PROFILE SOURCE OUTPUT)
    if(NOT DEFINED ${required})
        message(FATAL_ERROR "${required} must be defined")
    endif()
endforeach()

include("${CMAKE_CURRENT_LIST_DIR}/check_config_output.cmake")
prepare_config_output("${OUTPUT}")

set(hlsl_profile_args)
if(DEFINED PROFILE_OPTIONS)
    foreach(hlsl_option IN LISTS PROFILE_OPTIONS)
        list(APPEND hlsl_profile_args -po "${hlsl_option}")
    endforeach()
endif()

execute_process(
    COMMAND "${CGC}" -quiet -nocode -profile "${PROFILE}"
        ${hlsl_profile_args} -o "${OUTPUT}" "${SOURCE}"
    RESULT_VARIABLE result
    OUTPUT_VARIABLE stdout
    ERROR_VARIABLE stderr
)
if(NOT result EQUAL 0)
    message(FATAL_ERROR
        "${PROFILE} no-code compile failed (${result}):\n${stdout}${stderr}")
endif()
if(NOT stdout STREQUAL "")
    message(FATAL_ERROR
        "${PROFILE} no-code compile wrote to stdout:\n${stdout}")
endif()
if(NOT stderr STREQUAL "")
    message(FATAL_ERROR
        "${PROFILE} no-code compile wrote to stderr:\n${stderr}")
endif()

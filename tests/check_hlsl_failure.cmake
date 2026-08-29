foreach(required CGC PROFILE SOURCE CODE EXPECTED_LINE MESSAGE ACTUAL)
    if(NOT DEFINED ${required} OR "${${required}}" STREQUAL "")
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
    message(FATAL_ERROR
        "${PROFILE} unexpectedly lowered ${SOURCE} without a diagnostic")
endif()
set(diagnostics "${stdout}${stderr}")
string(REGEX MATCHALL "error C[0-9][0-9][0-9][0-9]:" matches
    "${diagnostics}")
list(LENGTH matches match_count)
if(NOT match_count EQUAL 1)
    message(FATAL_ERROR
        "${PROFILE} reported ${match_count} compiler errors:\n${diagnostics}")
endif()
if(NOT diagnostics MATCHES "error C${CODE}:")
    message(FATAL_ERROR
        "${PROFILE} did not report C${CODE}:\n${diagnostics}")
endif()
get_filename_component(source_name "${SOURCE}" NAME)
string(FIND "${diagnostics}" "${source_name}(${EXPECTED_LINE})"
    location_index)
if(location_index EQUAL -1)
    message(FATAL_ERROR
        "${PROFILE} reported the wrong source line:\n${diagnostics}")
endif()
if(NOT diagnostics MATCHES "${MESSAGE}")
    message(FATAL_ERROR
        "${PROFILE} did not match message ${MESSAGE}:\n${diagnostics}")
endif()
# Transactional output: HLSL validation happens before publication, so a
# failed compile may leave no content at all.  Comments can carry binding or
# default metadata and are therefore content too; never discard them here.
if(EXISTS "${ACTUAL}")
    file(READ "${ACTUAL}" published)
    string(REPLACE "\r\n" "\n" published "${published}")
    string(REPLACE "\r" "\n" published "${published}")
    string(REGEX REPLACE "[ \t\n]" "" payload "${published}")
    if(NOT payload STREQUAL "")
        message(FATAL_ERROR
            "failed compilation published HLSL content:\n${published}")
    endif()
endif()

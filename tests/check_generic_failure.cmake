foreach(required CGC SOURCE ACTUAL CODE EXPECTED_LINE REASON)
    if(NOT DEFINED ${required})
        message(FATAL_ERROR "${required} must be defined")
    endif()
endforeach()

include("${CMAKE_CURRENT_LIST_DIR}/check_config_output.cmake")
prepare_config_output("${ACTUAL}")

file(REMOVE "${ACTUAL}" "${ACTUAL}.normalized")
execute_process(
    COMMAND "${CGC}" -quiet -profile generic -o "${ACTUAL}" "${SOURCE}"
    RESULT_VARIABLE result
    OUTPUT_VARIABLE stdout
    ERROR_VARIABLE stderr
)
if(result EQUAL 0)
    message(FATAL_ERROR
        "generic unexpectedly accepted ${SOURCE}")
endif()
set(diagnostics "${stdout}${stderr}")
string(REGEX MATCHALL "error C[0-9][0-9][0-9][0-9]:" matches
    "${diagnostics}")
list(LENGTH matches match_count)
if(NOT match_count EQUAL 1)
    message(FATAL_ERROR
        "generic reported ${match_count} compiler errors:\n${diagnostics}")
endif()
if(NOT diagnostics MATCHES "error C${CODE}:")
    message(FATAL_ERROR
        "generic did not report C${CODE}:\n${diagnostics}")
endif()
get_filename_component(source_name "${SOURCE}" NAME)
if(NOT diagnostics MATCHES "${source_name}\\(${EXPECTED_LINE}\\)")
    message(FATAL_ERROR
        "generic reported the wrong source line:\n${diagnostics}")
endif()
if(NOT diagnostics MATCHES "${REASON}")
    message(FATAL_ERROR
        "generic did not identify ${REASON}:\n${diagnostics}")
endif()
# Transactional output (Task 18): a failed compilation publishes
# nothing -- the -o destination is withheld entirely, so no partial
# program can ever replace a previous file.

if(EXISTS "${ACTUAL}")
    file(READ "${ACTUAL}" published)
    message(FATAL_ERROR
        "failed compilation published an output file:\n${published}")
endif()

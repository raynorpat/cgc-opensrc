foreach(required_variable BISON SOURCE EXPECTED_PARSER EXPECTED_DEFINES WORK_DIR)
    if(NOT DEFINED ${required_variable})
        message(FATAL_ERROR "${required_variable} must be defined")
    endif()
endforeach()

if(BISON STREQUAL "BISON-NOTFOUND")
    # Same convention as the WGL smoke tests: without the generator the
    # check cannot run, but the suite must not fail for its absence.
    message(STATUS "bison not found; parser regeneration check skipped")
    return()
endif()

# Bison embeds its input and output paths in "#line" directives and the
# parser.h include guard, so the only faithful reproduction is running
# the generator exactly like the checked-in artifacts were made: a
# directory holding "parser.y", with plain "parser.c"/"parser.h"
# outputs, from that working directory.

file(MAKE_DIRECTORY "${WORK_DIR}")
set(staged_source "${WORK_DIR}/parser.y")
set(regen_parser "${WORK_DIR}/parser.c")
set(regen_defines "${WORK_DIR}/parser.h")
file(REMOVE "${regen_parser}" "${regen_defines}")
configure_file("${SOURCE}" "${staged_source}" COPYONLY)

execute_process(
    COMMAND "${BISON}" parser.y
        --defines=parser.h
        --output=parser.c
    WORKING_DIRECTORY "${WORK_DIR}"
    RESULT_VARIABLE result
    OUTPUT_VARIABLE stdout
    ERROR_VARIABLE stderr
)
if(NOT result EQUAL 0)
    message(FATAL_ERROR
        "Parser regeneration failed: ${stdout}${stderr}")
endif()
if(NOT "${stderr}" STREQUAL "")
    message(FATAL_ERROR
        "Parser regeneration reported warnings/conflicts:\n${stderr}")
endif()

execute_process(
    COMMAND "${CMAKE_COMMAND}" -E compare_files
        "${regen_parser}" "${EXPECTED_PARSER}"
    RESULT_VARIABLE parser_differs
)
if(parser_differs)
    message(FATAL_ERROR
        "Regenerated parser.c differs from the checked-in file")
endif()

execute_process(
    COMMAND "${CMAKE_COMMAND}" -E compare_files
        "${regen_defines}" "${EXPECTED_DEFINES}"
    RESULT_VARIABLE defines_differs
)
if(defines_differs)
    message(FATAL_ERROR
        "Regenerated parser.h differs from the checked-in file")
endif()

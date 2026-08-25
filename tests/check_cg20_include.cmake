# check_cg20_include.cmake - The Cg preprocessor resolves #include names
# relative to the compiler's working directory (the repository's
# documented include search behavior: the named file is opened as given,
# with no built-in search path).  This driver copies a main fixture and
# its included header into one scratch directory, compiles the main
# fixture by bare name from that directory, and asserts success.
#
# WORK_DIR arrives from the registering CMakeLists so the scratch
# location can carry the $<CONFIG> generator expression.

foreach(required CGC MAIN_SOURCE INCLUDED_SOURCE WORK_DIR)
    if(NOT DEFINED ${required})
        message(FATAL_ERROR "check_cg20_include.cmake requires ${required}")
    endif()
endforeach()

get_filename_component(cg20_include_name "${MAIN_SOURCE}" NAME)
get_filename_component(cg20_include_header "${INCLUDED_SOURCE}" NAME)
file(MAKE_DIRECTORY "${WORK_DIR}")
file(COPY "${MAIN_SOURCE}" "${INCLUDED_SOURCE}" DESTINATION "${WORK_DIR}")

execute_process(
    COMMAND "${CGC}" -quiet -nocode -profile generic "${cg20_include_name}"
    WORKING_DIRECTORY "${WORK_DIR}"
    RESULT_VARIABLE result
    OUTPUT_VARIABLE output
    ERROR_VARIABLE error
)
if(NOT result EQUAL 0)
    message(FATAL_ERROR
        "#include fixture failed (${result}):\nstdout:\n${output}\nstderr:\n${error}")
endif()

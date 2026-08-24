# check_diagnostic_count.cmake - Count-pinning failure driver: the
# compilation must fail, must report code CODE, and must report it
# EXACTLY COUNT times.  When FORBID is defined its regex may not appear
# anywhere in the combined output.  Used to pin layered-diagnostics
# behavior: a poisoned operand reports once, never twice.

if(NOT DEFINED CGC)
    message(FATAL_ERROR "check_diagnostic_count.cmake requires CGC")
endif()
if(NOT DEFINED SOURCE)
    message(FATAL_ERROR "check_diagnostic_count.cmake requires SOURCE")
endif()
if(NOT DEFINED PROFILE)
    set(PROFILE generic)
endif()
if(NOT DEFINED CODE)
    message(FATAL_ERROR "check_diagnostic_count.cmake requires CODE")
endif()
if(NOT DEFINED COUNT)
    message(FATAL_ERROR "check_diagnostic_count.cmake requires COUNT")
endif()

execute_process(
    COMMAND "${CGC}" -quiet -nocode -profile "${PROFILE}" ${EXTRA_ARGS} "${SOURCE}"
    RESULT_VARIABLE result
    OUTPUT_VARIABLE output
    ERROR_VARIABLE error
)
if(result EQUAL 0)
    message(FATAL_ERROR
        "count fixture unexpectedly compiled:\nstdout:\n${output}\nstderr:\n${error}")
endif()
set(diagnostic "${output}${error}")

string(REGEX MATCHALL "C${CODE}" hits "${diagnostic}")
list(LENGTH hits hits_length)
if(NOT hits_length EQUAL COUNT)
    message(FATAL_ERROR
        "expected code C${CODE} exactly ${COUNT} time(s), found ${hits_length}:\n${diagnostic}")
endif()

if(DEFINED FORBID AND diagnostic MATCHES "${FORBID}")
    message(FATAL_ERROR
        "forbidden secondary diagnostic '${FORBID}' appeared:\n${diagnostic}")
endif()

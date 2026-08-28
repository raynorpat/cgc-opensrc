# check_output_transaction.cmake - A failed compilation must leave an
# existing destination file untouched: the compiler generates into a
# same-directory temporary and commits only after the whole compilation
# succeeds.  This driver writes a sentinel into the requested output,
# runs cgc on a known-invalid shader with -o, and asserts that the
# command fails, the destination still holds exactly the sentinel, and
# no ".cgc-tmp-*" temporary remains next to it.

if(NOT DEFINED CGC)
    message(FATAL_ERROR "check_output_transaction.cmake requires CGC")
endif()
if(NOT DEFINED SOURCE)
    message(FATAL_ERROR "check_output_transaction.cmake requires SOURCE")
endif()
if(NOT DEFINED DESTINATION)
    message(FATAL_ERROR "check_output_transaction.cmake requires DESTINATION")
endif()

set(sentinel "// sentinel: this destination must survive a failed compilation\n")
if(NOT DEFINED PROFILE)
    set(PROFILE generic)
endif()
set(profile_args)
if(DEFINED PROFILE_OPTIONS)
    foreach(option IN LISTS PROFILE_OPTIONS)
        list(APPEND profile_args -po "${option}")
    endforeach()
endif()
set(entry_args)
if(DEFINED ENTRY)
    list(APPEND entry_args -entry "${ENTRY}")
endif()

foreach(mode normal nocode)
    file(WRITE "${DESTINATION}" "${sentinel}")
    set(mode_args)
    if(mode STREQUAL nocode)
        list(APPEND mode_args -nocode)
    endif()
    execute_process(
        COMMAND "${CGC}" -quiet ${mode_args} -profile "${PROFILE}"
            ${profile_args} ${entry_args} ${EXTRA_ARGS}
            -o "${DESTINATION}" "${SOURCE}"
        RESULT_VARIABLE result
        OUTPUT_VARIABLE output
        ERROR_VARIABLE error
    )
    if(result EQUAL 0)
        message(FATAL_ERROR
            "invalid fixture unexpectedly compiled in ${mode} mode:\nstdout:\n${output}\nstderr:\n${error}")
    endif()
    set(diagnostic "${output}${error}")
    if(DEFINED CODE AND NOT diagnostic MATCHES "error C${CODE}:")
        message(FATAL_ERROR
            "${mode} mode did not report C${CODE}:\n${diagnostic}")
    endif()
    file(READ "${DESTINATION}" actual)
    if(NOT actual STREQUAL sentinel)
        message(FATAL_ERROR
            "destination was not preserved in ${mode} mode\nexpected sentinel:\n${sentinel}\nactual:\n${actual}")
    endif()
    file(GLOB leftovers "${DESTINATION}.cgc-tmp-*")
    if(leftovers)
        message(FATAL_ERROR
            "temporary output files remain after ${mode} abort: ${leftovers}")
    endif()
endforeach()

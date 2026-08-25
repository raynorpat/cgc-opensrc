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
file(WRITE "${DESTINATION}" "${sentinel}")

execute_process(
    COMMAND "${CGC}" -quiet -nocode -profile generic ${EXTRA_ARGS}
        -o "${DESTINATION}" "${SOURCE}"
    RESULT_VARIABLE result
    OUTPUT_VARIABLE output
    ERROR_VARIABLE error
)
if(result EQUAL 0)
    message(FATAL_ERROR
        "invalid fixture unexpectedly compiled:\nstdout:\n${output}\nstderr:\n${error}")
endif()

file(READ "${DESTINATION}" actual)
if(NOT actual STREQUAL sentinel)
    message(FATAL_ERROR
        "destination was not preserved across a failed compilation\nexpected sentinel:\n${sentinel}\nactual:\n${actual}")
endif()

file(GLOB leftovers "${DESTINATION}.cgc-tmp-*")
if(leftovers)
    message(FATAL_ERROR
        "temporary output files remain after abort: ${leftovers}")
endif()

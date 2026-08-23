# compare_arb_oracle.cmake - Compare acceptance class and public binding
# metadata between the compiler under test and a reference NVIDIA cgc.

foreach(required CGC_UNDER_TEST REFERENCE_CGC PROFILE SOURCE ENTRY WORK_DIR)
    if(NOT DEFINED ${required})
        message(FATAL_ERROR "${required} is required")
    endif()
endforeach()

file(MAKE_DIRECTORY "${WORK_DIR}")
get_filename_component(srcName "${SOURCE}" NAME_WE)
set(underOut "${WORK_DIR}/${srcName}_under.arb")
set(refOut "${WORK_DIR}/${srcName}_ref.arb")
file(REMOVE "${underOut}" "${refOut}")

execute_process(
    COMMAND "${CGC_UNDER_TEST}" -quiet -profile "${PROFILE}"
            -entry "${ENTRY}" -o "${underOut}" "${SOURCE}"
    RESULT_VARIABLE underResult
    OUTPUT_VARIABLE underStdout
    ERROR_VARIABLE underStderr
)
execute_process(
    COMMAND "${REFERENCE_CGC}" -quiet -profile "${PROFILE}"
            -entry "${ENTRY}" -o "${refOut}" "${SOURCE}"
    RESULT_VARIABLE refResult
    OUTPUT_VARIABLE refStdout
    ERROR_VARIABLE refStderr
)

if(NOT underResult EQUAL 0 AND NOT refResult EQUAL 0)
    # Both reject: acceptance class agrees; detailed diagnostics are
    # owned by the self-contained failure fixtures.
    return()
endif()
if(underResult EQUAL 0 AND NOT refResult EQUAL 0)
    message(FATAL_ERROR
        "oracle mismatch: under-test accepted, reference rejected\n"
        "reference diagnostics:\n${refStderr}")
endif()
if(refResult EQUAL 0 AND NOT underResult EQUAL 0)
    message(FATAL_ERROR
        "oracle mismatch: reference accepted, under-test rejected\n"
        "under-test diagnostics:\n${underStderr}${underStdout}")
endif()

function(readPublicBindings file outVar)
    file(READ "${file}" raw)
    string(REPLACE "\r\n" "\n" raw "${raw}")
    string(REGEX MATCHALL "(^|\n)#(var|const|default) [^\n]*"
           lines "${raw}")
    set(cleaned "")
    foreach(line ${lines})
        string(STRIP "${line}" line)
        string(REGEX REPLACE " +" " " line "${line}")
        list(APPEND cleaned "${line}")
    endforeach()
    list(SORT cleaned)
    set(${outVar} "${cleaned}" PARENT_SCOPE)
endfunction()

readPublicBindings("${underOut}" underLines)
readPublicBindings("${refOut}" refLines)

foreach(pair "var:underLines:refLines")
    # placeholder loop for clarity
endforeach()

list(LENGTH underLines nUnder)
list(LENGTH refLines nRef)
if(NOT nUnder EQUAL nRef)
    message(FATAL_ERROR
        "oracle binding count mismatch: ${nUnder} vs ${nRef}\n"
        "under:\n${underLines}\nreference:\n${refLines}")
endif()
set(ii 0)
while(ii LESS nUnder)
    list(GET underLines ${ii} a)
    list(GET refLines ${ii} b)
    if(NOT a STREQUAL b)
        message(FATAL_ERROR
            "oracle binding mismatch at entry ${ii}:\n"
            "  under:     ${a}\n"
            "  reference: ${b}")
    endif()
    math(EXPR ii "${ii} + 1")
endwhile()

message(STATUS "oracle agreement on ${nUnder} public bindings")

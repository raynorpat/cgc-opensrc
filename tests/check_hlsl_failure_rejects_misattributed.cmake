foreach(required UNIT SOURCE CHECKER ACTUAL)
    if(NOT DEFINED ${required} OR "${${required}}" STREQUAL "")
        message(FATAL_ERROR "${required} must be defined")
    endif()
endforeach()

execute_process(
    COMMAND "${CMAKE_COMMAND}"
        "-DCGC=${UNIT}"
        -DPROFILE=hlsl-misattributed-error
        "-DSOURCE=${SOURCE}"
        -DCODE=6411
        -DEXPECTED_LINE=3
        "-DMESSAGE=expected diagnostic"
        "-DACTUAL=${ACTUAL}"
        -P "${CHECKER}"
    RESULT_VARIABLE result
    OUTPUT_VARIABLE stdout
    ERROR_VARIABLE stderr
)
file(REMOVE "${ACTUAL}" "${ACTUAL}.normalized")
if(result EQUAL 0)
    message(FATAL_ERROR
        "HLSL failure runner accepted note location/message as primary")
endif()
set(diagnostics "${stdout}${stderr}")
if(NOT diagnostics MATCHES "wrong source line|did not match message")
    message(FATAL_ERROR
        "HLSL failure runner rejected the primary for the wrong reason:\n${diagnostics}")
endif()

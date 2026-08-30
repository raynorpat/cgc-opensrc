foreach(required UNIT SOURCE CHECKER ACTUAL)
    if(NOT DEFINED ${required} OR "${${required}}" STREQUAL "")
        message(FATAL_ERROR "${required} must be defined")
    endif()
endforeach()

execute_process(
    COMMAND "${CMAKE_COMMAND}"
        "-DCGC=${UNIT}"
        -DPROFILE=hlsl-metadata-injection
        "-DSOURCE=${SOURCE}"
        -DCODE=6411
        -DEXPECTED_LINE=3
        "-DMESSAGE=HLSL name cannot be resolved for.*injected"
        "-DACTUAL=${ACTUAL}"
        -P "${CHECKER}"
    RESULT_VARIABLE result
    OUTPUT_VARIABLE stdout
    ERROR_VARIABLE stderr
)
file(REMOVE "${ACTUAL}" "${ACTUAL}.normalized")
if(result EQUAL 0)
    message(FATAL_ERROR
        "HLSL failure runner accepted injected binding metadata")
endif()
set(diagnostics "${stdout}${stderr}")
if(NOT diagnostics MATCHES "published HLSL content")
    message(FATAL_ERROR
        "HLSL failure runner failed for the wrong reason:\n${diagnostics}")
endif()

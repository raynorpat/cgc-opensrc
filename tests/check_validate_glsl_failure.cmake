foreach(required CMAKE_COMMAND CGC SOURCE OUTPUT VALIDATE_SCRIPT)
    if(NOT DEFINED ${required})
        message(FATAL_ERROR "${required} must be defined")
    endif()
endforeach()

execute_process(
    COMMAND "${CMAKE_COMMAND}"
        -DCGC=${CGC}
        -DVALIDATOR=${CMAKE_COMMAND}
        -DPROFILE=glslv
        -DSTAGE=vert
        -DSOURCE=${SOURCE}
        -DCONFIG=${CONFIG}
        -DOUTPUT=${OUTPUT}
        -P "${VALIDATE_SCRIPT}"
    RESULT_VARIABLE result
    OUTPUT_VARIABLE stdout
    ERROR_VARIABLE stderr
)
if(result EQUAL 0)
    message(FATAL_ERROR "validator failure did not propagate")
endif()
set(diagnostics "${stdout}${stderr}")
if(NOT diagnostics MATCHES "glslangValidator failed")
    message(FATAL_ERROR
        "validation failed for the wrong reason:\n${diagnostics}")
endif()

foreach(required CMAKE_COMMAND CGC SOURCE OUTPUT DIAGNOSTIC_SCRIPT)
    if(NOT DEFINED ${required})
        message(FATAL_ERROR "${required} must be defined")
    endif()
endforeach()

get_filename_component(output_dir "${OUTPUT}" DIRECTORY)
file(MAKE_DIRECTORY "${output_dir}")
file(WRITE "${OUTPUT}.normalized" "stale diagnostic output")
execute_process(
    COMMAND "${CMAKE_COMMAND}"
        -DCGC=${CGC}
        -DPROFILE=glslv
        -DSOURCE=${SOURCE}
        -DCODE=6201
        -DEXPECTED_LINE=6
        -DMESSAGE=bitwise
        -DACTUAL=${OUTPUT}
        -P "${DIAGNOSTIC_SCRIPT}"
    RESULT_VARIABLE result
    OUTPUT_VARIABLE stdout
    ERROR_VARIABLE stderr
)
if(NOT result EQUAL 0)
    message(FATAL_ERROR
        "diagnostic runner failed (${result}):\n${stdout}${stderr}")
endif()
if(EXISTS "${OUTPUT}.normalized")
    message(FATAL_ERROR "diagnostic runner left a stale normalized artifact")
endif()

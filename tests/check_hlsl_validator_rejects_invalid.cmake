foreach(required VALIDATOR CGC FXC SOURCE WORK_DIR CONFIG)
    if(NOT DEFINED ${required})
        message(FATAL_ERROR "${required} must be defined")
    endif()
endforeach()

file(MAKE_DIRECTORY "${WORK_DIR}")
set(output "${WORK_DIR}/intentionally-invalid.hlsl")
set(bytecode "${WORK_DIR}/intentionally-invalid.fxc")
file(REMOVE "${output}" "${bytecode}")
execute_process(
    COMMAND "${CMAKE_COMMAND}"
        -DCGC=${CGC}
        -DFXC=${FXC}
        -DPROFILE=hlslf40
        -DTARGET=ps_4_0
        -DLEGACY_SYNTAX=FALSE
        -DSOURCE=${SOURCE}
        -DOUTPUT=${output}
        -DBYTECODE=${bytecode}
        -DCONFIG=${CONFIG}
        -P ${VALIDATOR}
    RESULT_VARIABLE validator_result
    OUTPUT_VARIABLE validator_stdout
    ERROR_VARIABLE validator_stderr)

if(validator_result EQUAL 0)
    message(FATAL_ERROR
        "validate_hlsl.cmake accepted intentionally invalid generated HLSL")
endif()
set(validator_output "${validator_stdout}${validator_stderr}")
if(NOT validator_output MATCHES "FXC rejected")
    message(FATAL_ERROR
        "validator failed for the wrong reason:\n${validator_output}")
endif()
if(EXISTS "${bytecode}")
    file(SIZE "${bytecode}" bytecode_size)
    if(NOT bytecode_size EQUAL 0)
        message(FATAL_ERROR
            "failed validation published nonempty bytecode: ${bytecode}")
    endif()
endif()

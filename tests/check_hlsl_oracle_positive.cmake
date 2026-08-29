foreach(required CGC WORK_DIR VALID_EMPTY INVALID_SOURCE MISSING_SOURCE)
    if(NOT DEFINED ${required})
        message(FATAL_ERROR "${required} must be defined")
    endif()
endforeach()

function(expect_oracle_failure source expected name)
    execute_process(
        COMMAND "${CMAKE_COMMAND}"
            -DCGC_UNDER_TEST=${CGC}
            -DREFERENCE_CGC=${CGC}
            -DPROFILE=hlslv
            -DSOURCE=${source}
            -DENTRY=main
            -DWORK_DIR=${WORK_DIR}/${name}
            -P "${CMAKE_CURRENT_LIST_DIR}/check_hlsl_oracle.cmake"
        RESULT_VARIABLE result
        OUTPUT_VARIABLE stdout
        ERROR_VARIABLE stderr)
    set(output "${stdout}${stderr}")
    if(result EQUAL 0)
        message(FATAL_ERROR
            "positive oracle contract unexpectedly accepted ${name}")
    endif()
    if(NOT output MATCHES "${expected}")
        message(FATAL_ERROR
            "positive oracle contract emitted the wrong ${name} failure:\n"
            "${output}")
    endif()
endfunction()

expect_oracle_failure("${MISSING_SOURCE}" "source does not exist"
                      missing_source)
expect_oracle_failure("${INVALID_SOURCE}" "rejected by both compilers"
                      double_rejection)
expect_oracle_failure("${VALID_EMPTY}" "without normalized public metadata"
                      empty_metadata)

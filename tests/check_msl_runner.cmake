file(MAKE_DIRECTORY "${WORK_DIR}")
file(WRITE "${WORK_DIR}/wrong.expected" "WRONG SOURCE")
foreach(control snapshot status)
    if(control STREQUAL "snapshot")
        set(extra "-DEXPECTED=${WORK_DIR}/wrong.expected")
        set(message "Generated Metal differs")
    else()
        set(extra -DREJECT=C6601)
        set(message "Expected rejection")
    endif()
    execute_process(COMMAND "${CMAKE_COMMAND}" "-DCGC=${CGC}" -DPROFILE=mslv
        "-DSOURCE=${SOURCE}" "-DWORK_DIR=${WORK_DIR}/${control}" "${extra}"
        -P "${CMAKE_CURRENT_LIST_DIR}/run_msl_test.cmake"
        RESULT_VARIABLE status OUTPUT_VARIABLE out ERROR_VARIABLE err)
    if("${status}" STREQUAL "0" OR NOT "${out}${err}" MATCHES "${message}")
        message(FATAL_ERROR "Runner did not reject wrong ${control}: ${out}${err}")
    endif()
endforeach()

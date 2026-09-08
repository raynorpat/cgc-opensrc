file(MAKE_DIRECTORY "${WORK_DIR}/directory.metal")
execute_process(COMMAND "${CGC}" -quiet -version 2.0 -profile mslf
    -o "${WORK_DIR}/directory.metal" "${SOURCE}"
    RESULT_VARIABLE status OUTPUT_VARIABLE out ERROR_VARIABLE err)
if("${status}" STREQUAL "0" OR NOT IS_DIRECTORY "${WORK_DIR}/directory.metal")
    message(FATAL_ERROR "Directory destination incorrectly succeeded: ${status}\n${out}${err}")
endif()
file(GLOB leaked "${WORK_DIR}/*.cgc-tmp-*")
if(leaked)
    message(FATAL_ERROR "Bad output destination leaked ${leaked}")
endif()
execute_process(COMMAND "${CMAKE_COMMAND}" -E env CGC_TEST_MSL_WRITER_FAIL=1 ASAN_OPTIONS=detect_leaks=0
    "${FAULT_CGC}" -quiet -version 2.0 -profile mslf "${SOURCE}"
    RESULT_VARIABLE status OUTPUT_VARIABLE out ERROR_VARIABLE err)
if("${status}" STREQUAL "0" OR NOT out MATCHES "deliberate partial writer output")
    message(FATAL_ERROR "Partial stdout writer failure did not report failure")
endif()

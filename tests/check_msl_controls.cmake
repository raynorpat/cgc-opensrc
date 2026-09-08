file(MAKE_DIRECTORY "${WORK_DIR}")
# Both an invalid compiler input and stale artifacts must fail validation.
file(WRITE "${WORK_DIR}/invalid.metal" "#error MSL_INVALID_CONTROL\n")
file(WRITE "${WORK_DIR}/shader.air" "STALE_AIR")
file(WRITE "${WORK_DIR}/shader.metallib" "STALE_LIBRARY")
execute_process(COMMAND "${CMAKE_COMMAND}" "-DSOURCE=${WORK_DIR}/invalid.metal"
    "-DWORK_DIR=${WORK_DIR}" -P "${CMAKE_CURRENT_LIST_DIR}/validate_msl.cmake"
    RESULT_VARIABLE result OUTPUT_VARIABLE out ERROR_VARIABLE err)
if("${result}" STREQUAL "0" OR EXISTS "${WORK_DIR}/shader.metallib" OR
   NOT "${out}${err}" MATCHES "MSL_INVALID_CONTROL")
    message(FATAL_ERROR "Apple validation accepted invalid or stale output: ${out}${err}")
endif()

execute_process(COMMAND "${RUNTIME}" "${VERTEX_DIR}/shader.metallib"
    "${FRAGMENT_DIR}/shader.metallib" cg_mslv_main cg_mslf_main arithmetic 1
    RESULT_VARIABLE status OUTPUT_VARIABLE out ERROR_VARIABLE err)
if("${status}" STREQUAL "0" OR NOT "${err}" MATCHES "got.*expected")
    message(FATAL_ERROR "GPU comparator did not reject wrong pixels: ${status}\n${out}${err}")
endif()

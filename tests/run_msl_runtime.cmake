foreach(arg RUNTIME VERTEX_DIR FRAGMENT_DIR CASE)
    if(NOT DEFINED ${arg})
        message(FATAL_ERROR "${arg} is required")
    endif()
endforeach()
execute_process(COMMAND "${RUNTIME}" "${VERTEX_DIR}/shader.metallib"
    "${FRAGMENT_DIR}/shader.metallib" cg_mslv_main cg_mslf_main "${CASE}" 1
    RESULT_VARIABLE status OUTPUT_VARIABLE out ERROR_VARIABLE err)
if(NOT "${status}" STREQUAL "0")
    message(FATAL_ERROR "Metal runtime failed: ${status}\n${out}${err}")
endif()
message(STATUS "${out}")

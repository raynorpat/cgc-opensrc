# check_hlsl_oracle.cmake - Compare HLSL acceptance and public metadata
# with an explicitly supplied NVIDIA Cg compiler.

foreach(required CGC_UNDER_TEST REFERENCE_CGC PROFILE SOURCE ENTRY WORK_DIR)
    if(NOT DEFINED ${required})
        message(FATAL_ERROR "${required} is required")
    endif()
endforeach()

if(NOT EXISTS "${SOURCE}")
    message(FATAL_ERROR "HLSL oracle source does not exist: ${SOURCE}")
endif()

file(MAKE_DIRECTORY "${WORK_DIR}")
get_filename_component(source_name "${SOURCE}" NAME_WE)
set(under_output "${WORK_DIR}/${source_name}_${PROFILE}_under.hlsl")
set(reference_output "${WORK_DIR}/${source_name}_${PROFILE}_reference.hlsl")
file(REMOVE "${under_output}" "${reference_output}")

execute_process(
    COMMAND "${CGC_UNDER_TEST}" -quiet -profile "${PROFILE}"
            -entry "${ENTRY}" -o "${under_output}" "${SOURCE}"
    RESULT_VARIABLE under_result
    OUTPUT_VARIABLE under_stdout
    ERROR_VARIABLE under_stderr)
execute_process(
    COMMAND "${REFERENCE_CGC}" -quiet -profile "${PROFILE}"
            -entry "${ENTRY}" -o "${reference_output}" "${SOURCE}"
    RESULT_VARIABLE reference_result
    OUTPUT_VARIABLE reference_stdout
    ERROR_VARIABLE reference_stderr)

if(NOT under_result EQUAL 0 AND NOT reference_result EQUAL 0)
    message(FATAL_ERROR
        "HLSL positive oracle fixture was rejected by both compilers\n"
        "under-test diagnostics:\n${under_stderr}${under_stdout}\n"
        "reference diagnostics:\n${reference_stderr}${reference_stdout}")
endif()
if(under_result EQUAL 0 AND NOT reference_result EQUAL 0)
    message(FATAL_ERROR
        "HLSL oracle mismatch: under-test accepted, reference rejected\n"
        "reference diagnostics:\n${reference_stderr}${reference_stdout}")
endif()
if(reference_result EQUAL 0 AND NOT under_result EQUAL 0)
    message(FATAL_ERROR
        "HLSL oracle mismatch: reference accepted, under-test rejected\n"
        "under-test diagnostics:\n${under_stderr}${under_stdout}")
endif()
if(NOT EXISTS "${under_output}" OR NOT EXISTS "${reference_output}")
    message(FATAL_ERROR "HLSL oracle accepted without producing both outputs")
endif()
file(SIZE "${under_output}" under_size)
file(SIZE "${reference_output}" reference_size)
if(under_size EQUAL 0 OR reference_size EQUAL 0)
    message(FATAL_ERROR "HLSL oracle accepted with an empty output")
endif()

include("${CMAKE_CURRENT_LIST_DIR}/hlsl_oracle_records.cmake")
hlsl_read_public_records("${under_output}" under_records)
hlsl_read_public_records("${reference_output}" reference_records)

list(LENGTH under_records under_record_count)
list(LENGTH reference_records reference_record_count)
if(under_record_count EQUAL 0 OR reference_record_count EQUAL 0)
    message(FATAL_ERROR
        "HLSL oracle accepted without normalized public metadata")
endif()

if(NOT under_records STREQUAL reference_records)
    string(JOIN "\n  " under_text ${under_records})
    string(JOIN "\n  " reference_text ${reference_records})
    message(FATAL_ERROR
        "HLSL oracle public metadata mismatch\n"
        "under-test:\n  ${under_text}\n"
        "reference:\n  ${reference_text}")
endif()

message(STATUS
    "HLSL oracle agreement on ${under_record_count} public records")

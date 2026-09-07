foreach(required CGC SOURCE LEGACY_SOURCE WORK_DIR TEST_BINARY_ROOT)
    if(NOT DEFINED ${required})
        message(FATAL_ERROR "${required} must be defined")
    endif()
endforeach()

get_filename_component(test_root "${TEST_BINARY_ROOT}" REALPATH)
get_filename_component(work_absolute "${WORK_DIR}" ABSOLUTE)
get_filename_component(work_parent "${work_absolute}" DIRECTORY)
get_filename_component(work_name "${work_absolute}" NAME)
if(NOT work_parent STREQUAL test_root OR
   NOT work_name STREQUAL "raw-determinism")
    message(FATAL_ERROR
        "refusing unexpected raw-determinism path: ${work_absolute}")
endif()

file(REMOVE_RECURSE "${work_absolute}")
set(source_a_dir "${work_absolute}/from-a")
set(source_b_dir "${work_absolute}/from-b")
set(output_a_dir "${work_absolute}/to-a")
set(output_b_dir "${work_absolute}/to-b")
file(MAKE_DIRECTORY "${source_a_dir}" "${source_b_dir}"
    "${output_a_dir}" "${output_b_dir}")
get_filename_component(source_name "${SOURCE}" NAME)
file(COPY "${SOURCE}" DESTINATION "${source_a_dir}")
file(COPY "${SOURCE}" DESTINATION "${source_b_dir}")
set(source_a "${source_a_dir}/${source_name}")
set(source_b "${source_b_dir}/${source_name}")
set(output_a "${output_a_dir}/first.hlsl")
set(output_b "${output_b_dir}/second.hlsl")

function(compile_modern source output working_directory)
    execute_process(
        COMMAND "${CGC}" -quiet -profile hlslf40
            -entry texture_pixel -o "${output}" "${source}"
        WORKING_DIRECTORY "${working_directory}"
        RESULT_VARIABLE result
        OUTPUT_VARIABLE stdout
        ERROR_VARIABLE stderr)
    if(NOT result EQUAL 0 OR NOT stdout STREQUAL "" OR
       NOT stderr STREQUAL "")
        message(FATAL_ERROR
            "modern raw compile failed (${result}):\n${stdout}${stderr}")
    endif()
endfunction()

compile_modern("${source_a}" "${output_a}" "${source_a_dir}")
compile_modern("${source_b}" "${output_b}" "${source_b_dir}")
execute_process(
    COMMAND "${CMAKE_COMMAND}" -E compare_files "${output_a}" "${output_b}"
    RESULT_VARIABLE compare_result)
if(NOT compare_result EQUAL 0)
    message(FATAL_ERROR
        "modern raw output changes across absolute source/output directories")
endif()

file(READ "${output_a}" raw_output)
string(REPLACE "\r\n" "\n" normalized_output "${raw_output}")
string(REPLACE "\r" "\n" normalized_output "${normalized_output}")
set(expected_header
    "// compiler cgc\n// profile hlslf40\n// target ps_4_0\n// entry texture_pixel\n")
string(FIND "${normalized_output}" "${expected_header}" header_index)
if(NOT header_index EQUAL 0)
    message(FATAL_ERROR
        "modern raw output lacks the ordered deterministic header")
endif()
foreach(forbidden
        "build date"
        "command line args:"
        "${source_a_dir}"
        "${source_b_dir}"
        "${output_a_dir}"
        "${output_b_dir}")
    string(FIND "${normalized_output}" "${forbidden}" forbidden_index)
    if(NOT forbidden_index EQUAL -1)
        message(FATAL_ERROR
            "modern raw output contains forbidden volatile text: ${forbidden}")
    endif()
endforeach()
if(normalized_output MATCHES "[A-Za-z]:[/\\\\]")
    message(FATAL_ERROR "modern raw output contains a machine-specific path")
endif()

execute_process(
    COMMAND "${CGC}" -quiet -profile hlslf40
        -entry texture_pixel "${source_a}"
    WORKING_DIRECTORY "${source_b_dir}"
    RESULT_VARIABLE stdout_result
    OUTPUT_VARIABLE stdout_output
    ERROR_VARIABLE stdout_stderr)
if(NOT stdout_result EQUAL 0 OR NOT stdout_stderr STREQUAL "")
    message(FATAL_ERROR
        "modern stdout compile failed (${stdout_result}):\n${stdout_stderr}")
endif()
string(REPLACE "\r\n" "\n" stdout_normalized "${stdout_output}")
string(REPLACE "\r" "\n" stdout_normalized "${stdout_normalized}")
if(NOT stdout_normalized STREQUAL normalized_output)
    message(FATAL_ERROR
        "modern stdout output differs from deterministic file output")
endif()

set(legacy_output "${work_absolute}/legacy.hlsl")
execute_process(
    COMMAND "${CGC}" -quiet -profile hlslf
        -o "${legacy_output}" "${LEGACY_SOURCE}"
    RESULT_VARIABLE legacy_result
    OUTPUT_VARIABLE legacy_stdout
    ERROR_VARIABLE legacy_stderr)
if(NOT legacy_result EQUAL 0 OR NOT legacy_stdout STREQUAL "" OR
   NOT legacy_stderr STREQUAL "")
    message(FATAL_ERROR
        "legacy raw compile failed (${legacy_result}):\n${legacy_stdout}${legacy_stderr}")
endif()
file(READ "${legacy_output}" legacy_text)
foreach(required_legacy "cgc version" "build date" "command line args:")
    string(FIND "${legacy_text}" "${required_legacy}" legacy_index)
    if(legacy_index EQUAL -1)
        message(FATAL_ERROR
            "legacy SM3 raw header changed: missing ${required_legacy}")
    endif()
endforeach()

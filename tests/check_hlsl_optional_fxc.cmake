foreach(required SOURCE_DIR BUILD_DIR)
    if(NOT DEFINED ${required})
        message(FATAL_ERROR "${required} must be defined")
    endif()
endforeach()

file(REMOVE_RECURSE "${BUILD_DIR}")
set(saved_program_files_x86 "$ENV{ProgramFiles\(x86\)}")
set(ENV{ProgramFiles\(x86\)} "${BUILD_DIR}/missing-windows-sdk")
execute_process(
    COMMAND "${CMAKE_COMMAND}"
        -S "${SOURCE_DIR}"
        -B "${BUILD_DIR}"
        -DBUILD_TESTING=ON
        -DCGC_REQUIRE_FXC=OFF
        -DFXC_EXECUTABLE=FXC_EXECUTABLE-NOTFOUND
        -DCMAKE_FIND_USE_SYSTEM_ENVIRONMENT_PATH=FALSE
        -DCMAKE_FIND_USE_CMAKE_SYSTEM_PATH=FALSE
    RESULT_VARIABLE configure_result
    OUTPUT_VARIABLE configure_stdout
    ERROR_VARIABLE configure_stderr)
set(ENV{ProgramFiles\(x86\)} "${saved_program_files_x86}")

if(NOT configure_result EQUAL 0)
    message(FATAL_ERROR
        "CGC_REQUIRE_FXC=OFF failed configuration without fxc.exe:\n${configure_stdout}${configure_stderr}")
endif()
set(configure_output "${configure_stdout}${configure_stderr}")
if(NOT configure_output MATCHES
   "fxc not found; external HLSL validation tests are disabled")
    message(FATAL_ERROR
        "optional missing-fxc configuration omitted the disabled-test status:\n${configure_output}")
endif()

execute_process(
    COMMAND "${CMAKE_CTEST_COMMAND}" --test-dir "${BUILD_DIR}" -N
    RESULT_VARIABLE list_result
    OUTPUT_VARIABLE list_stdout
    ERROR_VARIABLE list_stderr)
if(NOT list_result EQUAL 0)
    message(FATAL_ERROR
        "could not list optional missing-fxc tests:\n${list_stdout}${list_stderr}")
endif()
if(list_stdout MATCHES "Test +#[0-9]+: hlsl_validate_")
    message(FATAL_ERROR
        "external validation test was registered without fxc.exe:\n${list_stdout}")
endif()
if(list_stdout MATCHES "hlsl_validator_rejects_invalid_generated_source")
    message(FATAL_ERROR
        "invalid-HLSL fxc harness test was registered without fxc.exe:\n${list_stdout}")
endif()

execute_process(
    COMMAND "${CMAKE_CTEST_COMMAND}"
        --test-dir "${BUILD_DIR}"
        -C Release
        -R "^hlsl_sm4_sm5_compatibility_matrix$"
        --output-on-failure
    RESULT_VARIABLE matrix_result
    OUTPUT_VARIABLE matrix_stdout
    ERROR_VARIABLE matrix_stderr)
if(NOT matrix_result EQUAL 0)
    message(FATAL_ERROR
        "modern HLSL matrix failed without fxc.exe:\n${matrix_stdout}${matrix_stderr}")
endif()

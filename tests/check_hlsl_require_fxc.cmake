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
        -DCGC_REQUIRE_FXC=ON
        -DFXC_EXECUTABLE=FXC_EXECUTABLE-NOTFOUND
        -DCMAKE_FIND_USE_SYSTEM_ENVIRONMENT_PATH=FALSE
        -DCMAKE_FIND_USE_CMAKE_SYSTEM_PATH=FALSE
    RESULT_VARIABLE configure_result
    OUTPUT_VARIABLE configure_stdout
    ERROR_VARIABLE configure_stderr)
set(ENV{ProgramFiles\(x86\)} "${saved_program_files_x86}")

if(configure_result EQUAL 0)
    message(FATAL_ERROR
        "CGC_REQUIRE_FXC=ON configured successfully without fxc.exe")
endif()
set(configure_output "${configure_stdout}${configure_stderr}")
if(NOT configure_output MATCHES
   "CGC_REQUIRE_FXC requires fxc.exe from the Windows SDK")
    message(FATAL_ERROR
        "missing-fxc configuration did not report the Windows SDK requirement:\n${configure_output}")
endif()

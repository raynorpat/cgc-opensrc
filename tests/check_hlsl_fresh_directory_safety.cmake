foreach(required TEST_BINARY_ROOT SCRIPT)
    if(NOT DEFINED ${required})
        message(FATAL_ERROR "${required} must be defined")
    endif()
endforeach()

set(expected_root "${TEST_BINARY_ROOT}/expected-root")
set(unsafe_work "${TEST_BINARY_ROOT}/fresh-link")
set(sentinel "${unsafe_work}/sentinel.txt")
file(MAKE_DIRECTORY "${expected_root}" "${unsafe_work}")
file(WRITE "${sentinel}" "must survive rejected cleanup\n")

execute_process(
    COMMAND "${CMAKE_COMMAND}"
        -DMODE=link
        -DCGC=unused
        -DCONFIG=Debug
        -DWORK_DIR=${unsafe_work}
        -DTEST_BINARY_ROOT=${expected_root}
        -DVERTEX_SOURCE=unused
        -DFRAGMENT_SOURCE=unused
        -P "${SCRIPT}"
    RESULT_VARIABLE result
    OUTPUT_VARIABLE stdout
    ERROR_VARIABLE stderr)
if(result EQUAL 0)
    message(FATAL_ERROR "unsafe fresh-directory cleanup was accepted")
endif()
if(NOT "${stdout}${stderr}" MATCHES
       "refusing to clear fresh-directory probe outside test binary root")
    message(FATAL_ERROR
        "unsafe cleanup failed for the wrong reason:\n${stdout}${stderr}")
endif()
if(NOT EXISTS "${sentinel}")
    message(FATAL_ERROR "unsafe fresh-directory cleanup removed sentinel")
endif()
file(REMOVE "${sentinel}")

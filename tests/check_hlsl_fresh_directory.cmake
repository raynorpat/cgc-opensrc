foreach(required MODE CGC CONFIG WORK_DIR TEST_BINARY_ROOT)
    if(NOT DEFINED ${required})
        message(FATAL_ERROR "${required} must be defined")
    endif()
endforeach()

get_filename_component(test_root "${TEST_BINARY_ROOT}" REALPATH)
get_filename_component(work_absolute "${WORK_DIR}" ABSOLUTE)
get_filename_component(work_name "${work_absolute}" NAME)
if(NOT work_name STREQUAL "fresh-link" AND
   NOT work_name STREQUAL "fresh-validate")
    message(FATAL_ERROR
        "refusing to clear unexpected fresh-directory probe ${WORK_DIR}")
endif()
if(EXISTS "${work_absolute}")
    get_filename_component(work_canonical "${work_absolute}" REALPATH)
    get_filename_component(work_parent "${work_canonical}" DIRECTORY)
else()
    get_filename_component(work_parent "${work_absolute}" DIRECTORY)
    get_filename_component(work_parent "${work_parent}" REALPATH)
endif()
if(WIN32)
    string(TOLOWER "${test_root}" test_root_compare)
    string(TOLOWER "${work_parent}" work_parent_compare)
else()
    set(test_root_compare "${test_root}")
    set(work_parent_compare "${work_parent}")
endif()
if(test_root STREQUAL "" OR
   NOT work_parent_compare STREQUAL test_root_compare)
    message(FATAL_ERROR
        "refusing to clear fresh-directory probe outside test binary root: "
        "${WORK_DIR} (root ${TEST_BINARY_ROOT})")
endif()
file(REMOVE_RECURSE "${work_absolute}")
if(EXISTS "${work_absolute}")
    message(FATAL_ERROR
        "failed to remove fresh-directory probe ${work_absolute}")
endif()

if(MODE STREQUAL "validate")
    foreach(required FXC PROFILE TARGET SOURCE)
        if(NOT DEFINED ${required})
            message(FATAL_ERROR "${required} must be defined")
        endif()
    endforeach()
    set(output "${work_absolute}/${CONFIG}/shader.hlsl")
    set(bytecode "${work_absolute}/${CONFIG}/shader.fxc")
    execute_process(
        COMMAND "${CMAKE_COMMAND}"
            -DCGC=${CGC}
            -DFXC=${FXC}
            -DPROFILE=${PROFILE}
            -DTARGET=${TARGET}
            -DSOURCE=${SOURCE}
            -DCONFIG=${CONFIG}
            -DOUTPUT=${output}
            -DBYTECODE=${bytecode}
            -P "${CMAKE_CURRENT_LIST_DIR}/validate_hlsl.cmake"
        RESULT_VARIABLE result
        OUTPUT_VARIABLE stdout
        ERROR_VARIABLE stderr)
    if(NOT result EQUAL 0)
        message(FATAL_ERROR
            "fresh validation directory failed:\n${stdout}${stderr}")
    endif()
    if(NOT EXISTS "${output}" OR NOT EXISTS "${bytecode}")
        message(FATAL_ERROR "fresh validation outputs were not created")
    endif()
elseif(MODE STREQUAL "link")
    foreach(required VERTEX_SOURCE FRAGMENT_SOURCE)
        if(NOT DEFINED ${required})
            message(FATAL_ERROR "${required} must be defined")
        endif()
    endforeach()
    set(vertex_output "${work_absolute}/${CONFIG}/shader.vs.hlsl")
    set(fragment_output "${work_absolute}/${CONFIG}/shader.ps.hlsl")
    execute_process(
        COMMAND "${CMAKE_COMMAND}"
            -DCGC=${CGC}
            -DVERTEX_SOURCE=${VERTEX_SOURCE}
            -DFRAGMENT_SOURCE=${FRAGMENT_SOURCE}
            -DCONFIG=${CONFIG}
            -DVERTEX_OUTPUT=${vertex_output}
            -DFRAGMENT_OUTPUT=${fragment_output}
            -P "${CMAKE_CURRENT_LIST_DIR}/check_hlsl_link.cmake"
        RESULT_VARIABLE result
        OUTPUT_VARIABLE stdout
        ERROR_VARIABLE stderr)
    if(NOT result EQUAL 0)
        message(FATAL_ERROR
            "fresh link directory failed:\n${stdout}${stderr}")
    endif()
    if(NOT EXISTS "${vertex_output}" OR NOT EXISTS "${fragment_output}")
        message(FATAL_ERROR "fresh link outputs were not created")
    endif()
else()
    message(FATAL_ERROR "unknown fresh-directory mode ${MODE}")
endif()

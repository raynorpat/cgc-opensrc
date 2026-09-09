option(CGC_REQUIRE_METAL "Require the qualified Apple Metal compiler" OFF)
option(CGC_REQUIRE_METAL_RUNTIME "Require Metal compiler and GPU execution" OFF)
set(CGC_METAL_AVAILABLE FALSE)
if(APPLE)
    find_program(CGC_XCRUN xcrun)
    if(CGC_XCRUN)
        execute_process(COMMAND "${CMAKE_COMMAND}"
            "-DOUTPUT_DIR=${CMAKE_CURRENT_BINARY_DIR}/msl-toolchain-probe"
            -P "${CMAKE_CURRENT_SOURCE_DIR}/run_msl_probes.cmake"
            RESULT_VARIABLE probe_result OUTPUT_VARIABLE probe_out ERROR_VARIABLE probe_err)
        if("${probe_result}" STREQUAL "0")
            set(CGC_METAL_AVAILABLE TRUE)
        endif()
    endif()
endif()
if((CGC_REQUIRE_METAL OR CGC_REQUIRE_METAL_RUNTIME) AND NOT CGC_METAL_AVAILABLE)
    message(FATAL_ERROR "Metal qualification requires a usable MSL 2.0 Apple compiler: ${probe_out}${probe_err}")
endif()
if(CGC_METAL_AVAILABLE)
    enable_language(OBJC)
    add_executable(msl_runtime msl_runtime.m)
    target_compile_options(msl_runtime PRIVATE -fobjc-arc -mmacosx-version-min=13.0)
    target_link_options(msl_runtime PRIVATE -mmacosx-version-min=13.0)
    target_link_libraries(msl_runtime PRIVATE "-framework Foundation" "-framework Metal")
    add_test(NAME msl_apple_invalid_and_stale_control COMMAND "${CMAKE_COMMAND}"
        "-DWORK_DIR=${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/msl/invalid-control"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/check_msl_controls.cmake")
else()
    message(STATUS "Metal compiler unavailable; portable MSL tests remain enabled")
endif()
function(add_msl_fixture name profile)
    set(work "${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/msl/${name}")
    set(source "${CMAKE_CURRENT_SOURCE_DIR}/msl/profile/${name}.cg")
    if(NOT EXISTS "${source}")
        set(source "${PROJECT_SOURCE_DIR}/tests/cg/${name}.cg")
    endif()
    add_test(NAME msl_profile_${name} COMMAND "${CMAKE_COMMAND}"
        "-DCGC=$<TARGET_FILE:cgc>" "-DPROFILE=${profile}"
        "-DSOURCE=${source}"
        "-DEXPECTED=${CMAKE_CURRENT_SOURCE_DIR}/msl/profile/${name}.expected"
        "-DWORK_DIR=${work}" "-DAPPLE_VALIDATE=${CGC_METAL_AVAILABLE}" ${ARGN}
        -P "${CMAKE_CURRENT_SOURCE_DIR}/run_msl_test.cmake")
    set_tests_properties(msl_profile_${name} PROPERTIES FIXTURES_SETUP msl_${name})
endfunction()
add_msl_fixture(vertex mslv)
add_msl_fixture(fragment mslf)
add_msl_fixture(arithmetic mslf)
add_msl_fixture(uniform mslf)
add_msl_fixture(discard mslf)
add_msl_fixture(alternate mslf -DENTRY=shade)
add_test(NAME msl_transaction_preserves_output COMMAND "${CMAKE_COMMAND}"
    "-DCGC=$<TARGET_FILE:cgc>"
    "-DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/msl/profile/unsupported.cg"
    "-DWORK_DIR=${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/msl/transaction"
    -P "${CMAKE_CURRENT_SOURCE_DIR}/check_msl_transaction.cmake")
foreach(version 1.1)
    add_test(NAME msl_profile_reject_${version} COMMAND "${CMAKE_COMMAND}"
        "-DCGC=$<TARGET_FILE:cgc>" -DPROFILE=mslv "-DVERSION=${version}" -DREJECT=C6600
        "-DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/msl/profile/vertex.cg"
        "-DWORK_DIR=${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/msl/reject-${version}"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/run_msl_test.cmake")
endforeach()
if(CGC_METAL_AVAILABLE)
    foreach(case solid arithmetic uniform discard)
        if(case STREQUAL "solid")
            set(fragment fragment)
        else()
            set(fragment "${case}")
        endif()
        add_test(NAME msl_runtime_${case} COMMAND msl_runtime
            "${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/msl/vertex/shader.metallib"
            "${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/msl/${fragment}/shader.metallib"
            cg_mslv_main cg_mslf_main "${case}" "$<BOOL:${CGC_REQUIRE_METAL_RUNTIME}>")
        set_tests_properties(msl_runtime_${case} PROPERTIES
            FIXTURES_REQUIRED "msl_vertex;msl_${fragment}")
        if(NOT CGC_REQUIRE_METAL_RUNTIME)
            set_tests_properties(msl_runtime_${case} PROPERTIES SKIP_RETURN_CODE 77)
        endif()
    endforeach()
endif()

add_executable(msl_ir_test msl_ir_test.c ${PROJECT_SOURCE_DIR}/src/msl_ir.c
    ${PROJECT_SOURCE_DIR}/src/msl_verify.c)
target_include_directories(msl_ir_test PRIVATE ${PROJECT_SOURCE_DIR}/src)
set_target_properties(msl_ir_test PROPERTIES C_STANDARD 90 C_STANDARD_REQUIRED YES C_EXTENSIONS YES)
add_test(NAME msl_ir_invariants_and_allocation COMMAND msl_ir_test)
if(CGC_METAL_AVAILABLE)
    add_test(NAME msl_runtime_wrong_expected_control COMMAND "${CMAKE_COMMAND}"
        "-DRUNTIME=$<TARGET_FILE:msl_runtime>"
        "-DVERTEX_DIR=${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/msl/vertex"
        "-DFRAGMENT_DIR=${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/msl/fragment"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/check_msl_runtime_control.cmake")
    set_tests_properties(msl_runtime_wrong_expected_control PROPERTIES FIXTURES_REQUIRED "msl_vertex;msl_fragment")
endif()
foreach(mode code nocode)
    if(mode STREQUAL "nocode")
        set(msl_extra -DEXTRA_ARGS=-nocode)
    else()
        set(msl_extra)
    endif()
    add_test(NAME msl_function_recursion_${mode} COMMAND "${CMAKE_COMMAND}"
        "-DCGC=$<TARGET_FILE:cgc>" -DPROFILE=mslf -DREJECT=C6607
        "-DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/msl/profile/recursion.cg"
        "-DWORK_DIR=${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/msl/recursion-${mode}"
        ${msl_extra} -P "${CMAKE_CURRENT_SOURCE_DIR}/run_msl_test.cmake")
endforeach()

foreach(name position reflection vertexlight vertexlight4 varying_vertex lod_vertex)
    add_msl_fixture(${name} mslv)
endforeach()
foreach(name matrix layout array depth varying_fragment mismatch swap texture cube lod cube_lod color_fragment global)
    add_msl_fixture(${name} mslf)
endforeach()
if(CGC_METAL_AVAILABLE)
    foreach(case matrix layout array depth swap texture lod cube_lod global varying mismatch cube0 cube1 cube2 cube3 cube4 cube5 lod_vertex pipeline_position pipeline_reflection pipeline_vertexlight pipeline_vertexlight4)
        set(vertex vertex)
        set(fragment "${case}")
        if(case STREQUAL "varying")
            set(vertex varying_vertex)
            set(fragment varying_fragment)
        elseif(case STREQUAL "mismatch")
            set(vertex varying_vertex)
        elseif(case MATCHES "^cube[0-5]$")
            set(fragment cube)
        elseif(case STREQUAL "lod_vertex")
            set(vertex lod_vertex)
            set(fragment color_fragment)
        elseif(case MATCHES "^pipeline_(.*)$")
            set(vertex "${CMAKE_MATCH_1}")
            set(fragment color_fragment)
        endif()
        add_test(NAME msl_runtime_${case} COMMAND msl_runtime
            "${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/msl/${vertex}/shader.metallib"
            "${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/msl/${fragment}/shader.metallib"
            cg_mslv_main cg_mslf_main "${case}" "$<BOOL:${CGC_REQUIRE_METAL_RUNTIME}>")
        set_tests_properties(msl_runtime_${case} PROPERTIES FIXTURES_REQUIRED "msl_${vertex};msl_${fragment}")
        if(NOT CGC_REQUIRE_METAL_RUNTIME)
            set_tests_properties(msl_runtime_${case} PROPERTIES SKIP_RETURN_CODE 77)
        endif()
    endforeach()
endif()
foreach(spec "read_only|mslf|6608" "vertex_sampling|mslv|6602" "resource_slot16|mslf|6604" "uniform_overflow|mslf|6605")
    string(REPLACE "|" ";" fields "${spec}")
    list(GET fields 0 name)
    list(GET fields 1 profile)
    list(GET fields 2 code)
    add_test(NAME msl_diagnostic_${name} COMMAND "${CMAKE_COMMAND}"
        "-DCGC=$<TARGET_FILE:cgc>" "-DPROFILE=${profile}" "-DREJECT=C${code}"
        "-DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/msl/profile/${name}.cg"
        "-DWORK_DIR=${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/msl/reject-${name}"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/run_msl_test.cmake")
endforeach()
add_test(NAME msl_runner_wrong_source_and_status COMMAND "${CMAKE_COMMAND}"
    "-DCGC=$<TARGET_FILE:cgc>"
    "-DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/msl/profile/vertex.cg"
    "-DWORK_DIR=${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/msl/runner-control"
    -P "${CMAKE_CURRENT_SOURCE_DIR}/check_msl_runner.cmake")

add_msl_fixture(default mslf)
add_msl_fixture(row_write mslf)
if(CGC_METAL_AVAILABLE)
    foreach(case default row_write)
        add_test(NAME msl_runtime_${case} COMMAND msl_runtime
            "${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/msl/vertex/shader.metallib"
            "${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/msl/${case}/shader.metallib"
            cg_mslv_main cg_mslf_main "${case}" "$<BOOL:${CGC_REQUIRE_METAL_RUNTIME}>")
        set_tests_properties(msl_runtime_${case} PROPERTIES FIXTURES_REQUIRED "msl_vertex;msl_${case}")
        if(NOT CGC_REQUIRE_METAL_RUNTIME)
            set_tests_properties(msl_runtime_${case} PROPERTIES SKIP_RETURN_CODE 77)
        endif()
    endforeach()
endif()

add_msl_fixture(half mslf)
if(CGC_METAL_AVAILABLE)
    add_test(NAME msl_runtime_half COMMAND msl_runtime
        "${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/msl/vertex/shader.metallib"
        "${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/msl/half/shader.metallib"
        cg_mslv_main cg_mslf_main half "$<BOOL:${CGC_REQUIRE_METAL_RUNTIME}>")
    set_tests_properties(msl_runtime_half PROPERTIES FIXTURES_REQUIRED "msl_vertex;msl_half")
    if(NOT CGC_REQUIRE_METAL_RUNTIME)
        set_tests_properties(msl_runtime_half PROPERTIES SKIP_RETURN_CODE 77)
    endif()
endif()

# The extended corpus uses independent numerical expectations and repeat-output
# checks. Existing hand-reviewed goldens remain the source serialization oracle.
function(add_msl_coverage name profile)
    set(work "${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/msl/coverage-${name}")
    add_test(NAME msl_coverage_${name} COMMAND "${CMAKE_COMMAND}"
        "-DCGC=$<TARGET_FILE:cgc>" "-DPROFILE=${profile}"
        "-DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/msl/coverage/${name}.cg"
        "-DWORK_DIR=${work}" "-DAPPLE_VALIDATE=${CGC_METAL_AVAILABLE}"
        -DREPEAT_OUTPUT=ON -P "${CMAKE_CURRENT_SOURCE_DIR}/run_msl_test.cmake")
    set_tests_properties(msl_coverage_${name} PROPERTIES FIXTURES_SETUP msl_coverage_${name})
    if(CGC_METAL_AVAILABLE AND ARGC GREATER 2)
        add_test(NAME msl_runtime_coverage_${name} COMMAND msl_runtime
            "${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/msl/vertex/shader.metallib"
            "${work}/shader.metallib" cg_mslv_main cg_mslf_main "coverage_${name}"
            "$<BOOL:${CGC_REQUIRE_METAL_RUNTIME}>" "${ARGV2}")
        set_tests_properties(msl_runtime_coverage_${name} PROPERTIES FIXTURES_REQUIRED "msl_vertex;msl_coverage_${name}")
        if(NOT CGC_REQUIRE_METAL_RUNTIME)
            set_tests_properties(msl_runtime_coverage_${name} PROPERTIES SKIP_RETURN_CODE 77)
        endif()
    endif()
endfunction()
function(add_msl_coverage_rejection name profile code)
    set(reject_line 1)
    if(name STREQUAL "reject_logical_effects")
        set(reject_line 2)
    endif()
    add_test(NAME msl_coverage_${name} COMMAND "${CMAKE_COMMAND}"
        "-DCGC=$<TARGET_FILE:cgc>" "-DPROFILE=${profile}" "-DREJECT=${code}" "-DREJECT_LINE=${reject_line}"
        "-DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/msl/coverage/${name}.cg"
        "-DWORK_DIR=${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/msl/coverage-${name}"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/check_msl_rejection.cmake")
endfunction()
include("${CMAKE_CURRENT_SOURCE_DIR}/msl/coverage/cases.cmake")
foreach(name layout texture default)
    add_test(NAME msl_allocation_${name} COMMAND "${CMAKE_COMMAND}"
        "-DCGC=$<TARGET_FILE:cgc_msl_fault>" -DPROFILE=mslf
        "-DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/msl/profile/${name}.cg"
        "-DWORK_DIR=${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/msl/fault-${name}"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/check_msl_faults.cmake")
endforeach()
foreach(name matrix_constructor_effects copy_indices aggregate_copy)
    add_test(NAME msl_allocation_${name} COMMAND "${CMAKE_COMMAND}"
        "-DCGC=$<TARGET_FILE:cgc_msl_fault>" -DPROFILE=mslf
        "-DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/msl/coverage/${name}.cg"
        "-DWORK_DIR=${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/msl/fault-${name}"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/check_msl_faults.cmake")
endforeach()
add_test(NAME msl_structure_and_enum_inventory COMMAND "${CMAKE_COMMAND}"
    "-DSOURCE_ROOT=${PROJECT_SOURCE_DIR}"
    -P "${CMAKE_CURRENT_SOURCE_DIR}/check_msl_structure.cmake")
add_test(NAME msl_metadata_binding_invariants COMMAND "${CMAKE_COMMAND}"
    "-DBASE_DIR=${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/msl"
    -P "${CMAKE_CURRENT_SOURCE_DIR}/check_msl_metadata.cmake")
set_tests_properties(msl_metadata_binding_invariants PROPERTIES FIXTURES_REQUIRED
    "msl_coverage_metadata_order_a;msl_coverage_metadata_order_b;msl_coverage_metadata_resources_a;msl_coverage_metadata_resources_b;msl_coverage_metadata_attributes_a;msl_coverage_metadata_attributes_b;msl_coverage_metadata_nested;msl_coverage_metadata_defaults;msl_varying_fragment")
foreach(mode exact wildcard open)
    if(mode STREQUAL "exact")
        set(expected "4,0,0,1")
    elseif(mode STREQUAL "wildcard")
        set(expected "2,0,0,1")
    else()
        set(expected "1,0,0,1")
    endif()
    if(CGC_METAL_AVAILABLE)
        add_test(NAME msl_runtime_overload_vertex_${mode} COMMAND msl_runtime
            "${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/msl/coverage-overload_vertex_${mode}/shader.metallib"
            "${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/msl/color_fragment/shader.metallib"
            cg_mslv_main cg_mslf_main "coverage_overload_vertex_${mode}"
            "$<BOOL:${CGC_REQUIRE_METAL_RUNTIME}>" "${expected}")
        set_tests_properties(msl_runtime_overload_vertex_${mode} PROPERTIES
            FIXTURES_REQUIRED "msl_coverage_overload_vertex_${mode};msl_color_fragment")
        if(NOT CGC_REQUIRE_METAL_RUNTIME)
            set_tests_properties(msl_runtime_overload_vertex_${mode} PROPERTIES SKIP_RETURN_CODE 77)
        endif()
    endif()
endforeach()
foreach(name metadata_defaults metadata_nested matrix_alias_selector matrix_row_store_effects)
    add_test(NAME msl_allocation_${name} COMMAND "${CMAKE_COMMAND}"
        "-DCGC=$<TARGET_FILE:cgc_msl_fault>" -DPROFILE=mslf
        "-DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/msl/coverage/${name}.cg"
        "-DWORK_DIR=${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/msl/fault-${name}"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/check_msl_faults.cmake")
endforeach()
add_test(NAME msl_transaction_io_failure COMMAND "${CMAKE_COMMAND}"
    "-DCGC=$<TARGET_FILE:cgc>" "-DFAULT_CGC=$<TARGET_FILE:cgc_msl_fault>"
    "-DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/msl/profile/fragment.cg"
    "-DWORK_DIR=${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/msl/io-failure"
    -P "${CMAKE_CURRENT_SOURCE_DIR}/check_msl_io_failure.cmake")
add_test(NAME msl_profile_reject_1.1_nocode COMMAND "${CMAKE_COMMAND}"
    "-DCGC=$<TARGET_FILE:cgc>" -DPROFILE=mslf -DVERSION=1.1 -DREJECT=C6600 -DEXTRA_ARGS=-nocode
    "-DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/msl/profile/fragment.cg"
    "-DWORK_DIR=${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/msl/version-nocode"
    -P "${CMAKE_CURRENT_SOURCE_DIR}/run_msl_test.cmake")
add_test(NAME msl_allocation_matrix_row_inout COMMAND "${CMAKE_COMMAND}"
    "-DCGC=$<TARGET_FILE:cgc_msl_fault>" -DPROFILE=mslf
    "-DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/msl/coverage/matrix_row_inout.cg"
    "-DWORK_DIR=${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/msl/fault-matrix_row_inout"
    -P "${CMAKE_CURRENT_SOURCE_DIR}/check_msl_faults.cmake")
foreach(name aggregate_defaults global_constants local_initializers matrix_compound minimal_dependencies dependent_constants)
    add_test(NAME msl_allocation_${name} COMMAND "${CMAKE_COMMAND}"
        "-DCGC=$<TARGET_FILE:cgc_msl_fault>" -DPROFILE=mslf
        "-DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/msl/coverage/${name}.cg"
        "-DWORK_DIR=${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/msl/fault-${name}"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/check_msl_faults.cmake")
endforeach()
add_test(NAME msl_plan_contract COMMAND "${CMAKE_COMMAND}"
    "-DCGC=$<TARGET_FILE:cgc>"
    "-DBASE_DIR=${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/msl"
    "-DWORK_DIR=${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/msl/plan-contract"
    -P "${CMAKE_CURRENT_SOURCE_DIR}/check_msl_plan_contract.cmake")
set_tests_properties(msl_plan_contract PROPERTIES FIXTURES_REQUIRED
    "msl_coverage_minimal_dependencies;msl_coverage_global_constants;msl_coverage_aggregate_defaults")

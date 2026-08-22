function(add_generic_regression name source expected_sha256)
    add_test(
        NAME generic_${name}
        COMMAND ${CMAKE_COMMAND}
            -DCGC=$<TARGET_FILE:cgc>
            -DSOURCE=${PROJECT_SOURCE_DIR}/${source}
            -DEXPECTED_SHA256=${expected_sha256}
            -P ${CMAKE_CURRENT_SOURCE_DIR}/check_generic.cmake
    )
endfunction()

function(add_glsl_fixture name profile source expected)
    add_test(
        NAME ${name}
        COMMAND ${CMAKE_COMMAND}
            -DCGC=$<TARGET_FILE:cgc>
            -DPROFILE=${profile}
            -DSOURCE=${PROJECT_SOURCE_DIR}/${source}
            -DEXPECTED=${PROJECT_SOURCE_DIR}/${expected}
            -DACTUAL=${CMAKE_CURRENT_BINARY_DIR}/${name}.glsl
            -P ${CMAKE_CURRENT_SOURCE_DIR}/check_glsl.cmake
    )
endfunction()

function(add_glsl_failure name source expected_line reason)
    add_test(
        NAME ${name}
        COMMAND ${CMAKE_COMMAND}
            -DCGC=$<TARGET_FILE:cgc>
            -DPROFILE=glslv
            -DSOURCE=${PROJECT_SOURCE_DIR}/${source}
            -DACTUAL=${CMAKE_CURRENT_BINARY_DIR}/${name}.glsl
            -DCODE=5508
            -DEXPECTED_LINE=${expected_line}
            -DREASON=${reason}
            -P ${CMAKE_CURRENT_SOURCE_DIR}/check_glsl_failure.cmake
    )
endfunction()

function(add_generic_failure name source expected_line reason)
    add_test(
        NAME ${name}
        COMMAND ${CMAKE_COMMAND}
            -DCGC=$<TARGET_FILE:cgc>
            -DSOURCE=${PROJECT_SOURCE_DIR}/${source}
            -DACTUAL=${CMAKE_CURRENT_BINARY_DIR}/${name}.txt
            -DCODE=1116
            -DEXPECTED_LINE=${expected_line}
            -DREASON=${reason}
            -P ${CMAKE_CURRENT_SOURCE_DIR}/check_generic_failure.cmake
    )
endfunction()

function(add_glsl_common_failure name source expected_line reason)
    add_test(
        NAME ${name}
        COMMAND ${CMAKE_COMMAND}
            -DCGC=$<TARGET_FILE:cgc>
            -DPROFILE=glslv
            -DSOURCE=${PROJECT_SOURCE_DIR}/${source}
            -DACTUAL=${CMAKE_CURRENT_BINARY_DIR}/${name}.glsl
            -DCODE=1116
            -DEXPECTED_LINE=${expected_line}
            -DREASON=${reason}
            -P ${CMAKE_CURRENT_SOURCE_DIR}/check_glsl_failure.cmake
    )
endfunction()

add_executable(glsl_ir_unit
    ${PROJECT_SOURCE_DIR}/glsl_ir.c
    ${PROJECT_SOURCE_DIR}/glsl_codegen.c
    glsl_ir_test.c
)

target_include_directories(glsl_ir_unit PRIVATE
    ${PROJECT_SOURCE_DIR}
)

set_target_properties(glsl_ir_unit PROPERTIES
    C_STANDARD 90
    C_STANDARD_REQUIRED YES
    C_EXTENSIONS YES
)

add_test(
    NAME glsl_ir_unit
    COMMAND glsl_ir_unit
)

add_executable(glsl_semantics_unit
    ${PROJECT_SOURCE_DIR}/atom.c
    ${PROJECT_SOURCE_DIR}/glsl_ir.c
    ${PROJECT_SOURCE_DIR}/glsl_hal.c
    ${PROJECT_SOURCE_DIR}/glslv_hal.c
    ${PROJECT_SOURCE_DIR}/glslf_hal.c
    ${PROJECT_SOURCE_DIR}/memory.c
    glsl_semantics_test.c
)

target_include_directories(glsl_semantics_unit PRIVATE
    ${PROJECT_SOURCE_DIR}
)

set_target_properties(glsl_semantics_unit PROPERTIES
    C_STANDARD 90
    C_STANDARD_REQUIRED YES
    C_EXTENSIONS YES
)

add_test(
    NAME glsl_semantics_unit
    COMMAND glsl_semantics_unit
)

add_generic_regression(position position.cg
    fbd55194fb43822d993f60915df94f7a0627fd9bea27013a8c2d1d0ccdfe7b8e)
add_generic_regression(reflection reflection.cg
    aa2c396563920d50a7b45a6a06c359ec865faa338de4ad42294143a1243b6d8b)
add_generic_regression(vertexlight vertexlight.cg
    a973a0f2820dd1777d0e2cdb593a93f1e0c6d3f5f8640b2017724e37f8fd3c81)
add_generic_regression(vertexlight4 vertexlight4.cg
    5454876aedd830f1d07b65e26cd51e7570035ee01d7d4b784ba4769632e31961)
add_generic_regression(loop_jumps tests/glsl/expressions/vp_jumps.cg
    07c60ea6ceaeba6c3cd85020e9088d23293179983fc7cd12ca51b306025ce5f4)
add_generic_failure(generic_break_outside_loop
    tests/glsl/diagnostics/vp_break_outside_loop.cg 5 break)
add_generic_failure(generic_continue_outside_loop
    tests/glsl/diagnostics/vp_continue_outside_loop.cg 5 continue)

function(add_glsl_registration name)
    add_test(
        NAME ${name}_registration
        COMMAND ${CMAKE_COMMAND}
            -DCGC=$<TARGET_FILE:cgc>
            -DPROFILE=${name}
            -DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/glsl/profile/empty.cg
            -P ${CMAKE_CURRENT_SOURCE_DIR}/glsl/profile/check_header.cmake
    )
endfunction()

add_glsl_registration(glslv)
add_glsl_registration(glslf)

add_glsl_fixture(glslv_passthrough glslv
    tests/glsl/profile/vp_passthrough.cg
    tests/glsl/profile/vp_passthrough.expected)

add_glsl_fixture(glslv_defaults glslv
    tests/glsl/uniforms/vp_defaults.cg
    tests/glsl/uniforms/vp_defaults.expected)

add_glsl_fixture(glslv_uniform_collision glslv
    tests/glsl/uniforms/vp_uniform_collision.cg
    tests/glsl/uniforms/vp_uniform_collision.expected)

add_glsl_fixture(glslv_position glslv
    position.cg
    tests/glsl/vertex/position.expected)

add_glsl_fixture(glslv_reflection glslv
    reflection.cg
    tests/glsl/vertex/reflection.expected)

add_glsl_fixture(glslv_vertexlight glslv
    vertexlight.cg
    tests/glsl/vertex/vertexlight.expected)

add_glsl_fixture(glslv_vertexlight4 glslv
    vertexlight4.cg
    tests/glsl/vertex/vertexlight4.expected)

add_glsl_fixture(glslv_control glslv
    tests/glsl/expressions/vp_control.cg
    tests/glsl/expressions/vp_control.expected)

add_glsl_fixture(glslv_jumps glslv
    tests/glsl/expressions/vp_jumps.cg
    tests/glsl/expressions/vp_jumps.expected)

add_glsl_fixture(glslv_qualifiers glslv
    tests/glsl/expressions/vp_qualifiers.cg
    tests/glsl/expressions/vp_qualifiers.expected)

add_glsl_fixture(glslv_vector_compare glslv
    tests/glsl/expressions/vp_vector_compare.cg
    tests/glsl/expressions/vp_vector_compare.expected)

add_glsl_fixture(glslv_expression_edges glslv
    tests/glsl/expressions/vp_expression_edges.cg
    tests/glsl/expressions/vp_expression_edges.expected)

add_glsl_fixture(glslv_grouping glslv
    tests/glsl/expressions/vp_grouping.cg
    tests/glsl/expressions/vp_grouping.expected)

add_glsl_fixture(glslv_builtin_name_helper glslv
    tests/glsl/expressions/vp_builtin_name_helper.cg
    tests/glsl/expressions/vp_builtin_name_helper.expected)

add_glsl_fixture(glslv_numeric_intrinsics glslv
    tests/glsl/expressions/vp_numeric_intrinsics.cg
    tests/glsl/expressions/vp_numeric_intrinsics.expected)

add_glsl_fixture(glslv_float1_swizzle glslv
    tests/glsl/expressions/vp_float1_swizzle.cg
    tests/glsl/expressions/vp_float1_swizzle.expected)

add_glsl_fixture(glslv_helper_struct glslv
    tests/glsl/expressions/vp_helper_struct.cg
    tests/glsl/expressions/vp_helper_struct.expected)

add_glsl_fixture(glslv_forward_helper glslv
    tests/glsl/expressions/vp_forward_helper.cg
    tests/glsl/expressions/vp_forward_helper.expected)

add_glsl_fixture(glslv_formal_interface_collision glslv
    tests/glsl/profile/vp_formal_interface_collision.cg
    tests/glsl/profile/vp_formal_interface_collision.expected)

add_glsl_fixture(glslv_type_local_collision glslv
    tests/glsl/profile/vp_type_local_collision.cg
    tests/glsl/profile/vp_type_local_collision.expected)

add_glsl_fixture(glslv_function_local_collision glslv
    tests/glsl/profile/vp_function_local_collision.cg
    tests/glsl/profile/vp_function_local_collision.expected)

add_glsl_failure(glslv_unsupported_lowering
    tests/glsl/profile/vp_unsupported_operator.cg 9 remainder)
add_glsl_failure(glslv_constant_remainder
    tests/glsl/diagnostics/vp_constant_remainder.cg 6 remainder)
add_glsl_failure(glslv_constant_shift_left
    tests/glsl/diagnostics/vp_constant_shift_left.cg 6 "left shift")
add_glsl_failure(glslv_constant_shift_right
    tests/glsl/diagnostics/vp_constant_shift_right.cg 6 "right shift")
add_glsl_failure(glslv_constant_bitwise_and
    tests/glsl/diagnostics/vp_constant_bitwise_and.cg 6 "bitwise and")
add_glsl_failure(glslv_constant_bitwise_xor
    tests/glsl/diagnostics/vp_constant_bitwise_xor.cg 6 "bitwise xor")
add_glsl_failure(glslv_constant_bitwise_or
    tests/glsl/diagnostics/vp_constant_bitwise_or.cg 6 "bitwise or")
add_glsl_failure(glslv_constant_bitwise_not
    tests/glsl/diagnostics/vp_constant_bitwise_not.cg 6 "bitwise not")
add_glsl_failure(glslv_nonfinite_infinity
    tests/glsl/diagnostics/vp_nonfinite_infinity.cg 6 non-finite)
add_glsl_failure(glslv_nonfinite_nan
    tests/glsl/diagnostics/vp_nonfinite_nan.cg 6 non-finite)
add_glsl_failure(glslv_nonsquare_matrix
    tests/glsl/diagnostics/vp_nonsquare_matrix.cg 5 "non-square GLSL matrix")
add_glsl_common_failure(glslv_break_outside_loop
    tests/glsl/diagnostics/vp_break_outside_loop.cg 5 break)
add_glsl_common_failure(glslv_continue_outside_loop
    tests/glsl/diagnostics/vp_continue_outside_loop.cg 5 continue)

add_test(
    NAME glslv_semantics
    COMMAND cgc -quiet -nocode -profile glslv
        ${CMAKE_CURRENT_SOURCE_DIR}/glsl/semantics/vp_semantics.cg
)

add_test(
    NAME glslf_semantics
    COMMAND cgc -quiet -nocode -profile glslf
        ${CMAKE_CURRENT_SOURCE_DIR}/glsl/semantics/fp_semantics.cg
)

add_test(
    NAME glslv_unsupported_connector
    COMMAND ${CMAKE_COMMAND}
        -DCGC=$<TARGET_FILE:cgc>
        -DPROFILE=glslv
        -DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/glsl/profile/unsupported_connector.cg
        -DCODE=5001
        -P ${CMAKE_CURRENT_SOURCE_DIR}/glsl/profile/check_diagnostic.cmake
)

add_test(
    NAME unbound_input
    COMMAND ${CMAKE_COMMAND}
        -DCGC=$<TARGET_FILE:cgc>
        -DPROFILE=glslv
        -DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/glsl/diagnostics/unbound_input.cg
        -DCODE=5116
        -P ${CMAKE_CURRENT_SOURCE_DIR}/glsl/profile/check_diagnostic.cmake
)

add_test(
    NAME glslf_face_vector
    COMMAND ${CMAKE_COMMAND}
        -DCGC=$<TARGET_FILE:cgc>
        -DPROFILE=glslf
        -DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/glsl/diagnostics/face_vector.cg
        -DCODE=5108
        -P ${CMAKE_CURRENT_SOURCE_DIR}/glsl/profile/check_diagnostic.cmake
)

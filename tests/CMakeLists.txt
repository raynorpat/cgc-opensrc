find_program(GLSLANG_VALIDATOR NAMES glslangValidator)
if(NOT GLSLANG_VALIDATOR)
    message(STATUS
        "glslangValidator not found; GLSL source validation tests are disabled")
endif()

function(add_glsl_validation name profile stage source)
    if(GLSLANG_VALIDATOR)
        add_test(
            NAME validate_${name}
            COMMAND ${CMAKE_COMMAND}
                -DCGC=$<TARGET_FILE:cgc>
                "-DVALIDATOR=${GLSLANG_VALIDATOR}"
                -DPROFILE=${profile}
                -DSTAGE=${stage}
                -DSOURCE=${PROJECT_SOURCE_DIR}/${source}
                "-DCONFIG=$<CONFIG>"
                "-DOUTPUT=${CMAKE_CURRENT_BINARY_DIR}/validated/$<CONFIG>/${name}.glsl"
                -P ${CMAKE_CURRENT_SOURCE_DIR}/validate_glsl.cmake
        )
    endif()
endfunction()

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
            "-DCONFIG=$<CONFIG>"
            "-DACTUAL=${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/${name}.glsl"
            -P ${CMAKE_CURRENT_SOURCE_DIR}/check_glsl.cmake
    )
    if(profile STREQUAL "glslv")
        set(stage vert)
    else()
        set(stage frag)
    endif()
    add_glsl_validation(${name} ${profile} ${stage} ${source})
endfunction()

function(add_glsl_failure name source expected_line reason)
    add_test(
        NAME ${name}
        COMMAND ${CMAKE_COMMAND}
            -DCGC=$<TARGET_FILE:cgc>
            -DPROFILE=glslv
            -DSOURCE=${PROJECT_SOURCE_DIR}/${source}
            "-DCONFIG=$<CONFIG>"
            "-DACTUAL=${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/${name}.glsl"
            -DCODE=6201
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
            "-DCONFIG=$<CONFIG>"
            "-DACTUAL=${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/${name}.txt"
            -DCODE=1116
            -DEXPECTED_LINE=${expected_line}
            -DREASON=${reason}
            -P ${CMAKE_CURRENT_SOURCE_DIR}/check_generic_failure.cmake
    )
endfunction()

function(add_generic_diagnostic name source code expected_line reason)
    add_test(
        NAME ${name}
        COMMAND ${CMAKE_COMMAND}
            -DCGC=$<TARGET_FILE:cgc>
            -DSOURCE=${PROJECT_SOURCE_DIR}/${source}
            "-DCONFIG=$<CONFIG>"
            "-DACTUAL=${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/${name}.txt"
            -DCODE=${code}
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
            "-DCONFIG=$<CONFIG>"
            "-DACTUAL=${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/${name}.glsl"
            -DCODE=1116
            -DEXPECTED_LINE=${expected_line}
            -DREASON=${reason}
            -P ${CMAKE_CURRENT_SOURCE_DIR}/check_glsl_failure.cmake
    )
endfunction()

function(add_glsl_profile_failure name profile source code expected_line reason)
    add_test(
        NAME ${name}
        COMMAND ${CMAKE_COMMAND}
            -DCGC=$<TARGET_FILE:cgc>
            -DPROFILE=${profile}
            -DSOURCE=${PROJECT_SOURCE_DIR}/${source}
            "-DCONFIG=$<CONFIG>"
            "-DACTUAL=${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/${name}.glsl"
            -DCODE=${code}
            -DEXPECTED_LINE=${expected_line}
            -DREASON=${reason}
            -P ${CMAKE_CURRENT_SOURCE_DIR}/check_glsl_failure.cmake
    )
endfunction()

function(add_glsl_diagnostic name profile source code expected_line message)
    add_test(
        NAME ${name}
        COMMAND ${CMAKE_COMMAND}
            -DCGC=$<TARGET_FILE:cgc>
            -DPROFILE=${profile}
            -DSOURCE=${PROJECT_SOURCE_DIR}/${source}
            "-DCONFIG=$<CONFIG>"
            "-DACTUAL=${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/${name}.glsl"
            -DCODE=${code}
            -DEXPECTED_LINE=${expected_line}
            -DMESSAGE=${message}
            -P ${CMAKE_CURRENT_SOURCE_DIR}/check_diagnostic.cmake
    )
endfunction()

function(add_glsl_boundary name profile source stage)
    add_test(
        NAME ${name}
        COMMAND ${CMAKE_COMMAND}
            -DCGC=$<TARGET_FILE:cgc>
            -DPROFILE=${profile}
            -DSOURCE=${PROJECT_SOURCE_DIR}/${source}
            "-DCONFIG=$<CONFIG>"
            "-DACTUAL=${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/${name}.glsl"
            -P ${CMAKE_CURRENT_SOURCE_DIR}/check_glsl_boundary.cmake
    )
    add_glsl_validation(${name} ${profile} ${stage} ${source})
endfunction()

# Every Cg 2.0 conformance test registers through one of these two
# helpers so the global CG20_TEST_NAMES property stays complete; the
# cg20_manifest cross-check requires every non-out-of-scope manifest
# row to name exactly one name recorded here.  Everything after the
# test name is the add_test COMMAND tail (cmake driver script plus its
# -D definitions).

function(add_cg20_success name)
    set_property(GLOBAL APPEND PROPERTY CG20_TEST_NAMES "${name}")
    add_test(NAME "${name}" COMMAND ${CMAKE_COMMAND} ${ARGN})
endfunction()

function(add_cg20_failure name)
    set_property(GLOBAL APPEND PROPERTY CG20_TEST_NAMES "${name}")
    add_test(NAME "${name}" COMMAND ${CMAKE_COMMAND} ${ARGN})
endfunction()

# Output transaction (Task 18): a failed compilation must preserve the
# pre-existing destination and leave no ".cgc-tmp-*" temporary behind.

add_cg20_failure(cg20_output_preserved_on_failure
    -DCGC=$<TARGET_FILE:cgc>
    -DSOURCE=${PROJECT_SOURCE_DIR}/tests/cg20/diagnostics/reserved_word.cg
    "-DCONFIG=$<CONFIG>"
    "-DDESTINATION=${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/cg20_output_preserved_on_failure.out"
    -P ${CMAKE_CURRENT_SOURCE_DIR}/check_output_transaction.cmake
)

# Layered diagnostics: count-pinned single reports for poisoned
# operands and dynamic-array assignment intercepts.

add_cg20_failure(cg20_poison_return_single_diagnostic
    -DCGC=$<TARGET_FILE:cgc>
    -DSOURCE=${PROJECT_SOURCE_DIR}/tests/cg20/diagnostics/poison_return.cg
    -DCODE=1008
    -DCOUNT=1
    -DFORBID=C1016
    -P ${CMAKE_CURRENT_SOURCE_DIR}/check_diagnostic_count.cmake
)
add_cg20_failure(cg20_dynamic_array_single_diagnostic
    -DCGC=$<TARGET_FILE:cgc>
    -DSOURCE=${PROJECT_SOURCE_DIR}/tests/cg20/diagnostics/dynamic_array_element_mismatch.cg
    -DCODE=1035
    -DCOUNT=1
    -P ${CMAKE_CURRENT_SOURCE_DIR}/check_diagnostic_count.cmake
)

add_cg20_success(cg20_smoke
    -DCGC=$<TARGET_FILE:cgc> -DPROFILE=generic
    -DSOURCE=${PROJECT_SOURCE_DIR}/tests/cg20/profile/smoke.cg
    -P ${CMAKE_CURRENT_SOURCE_DIR}/check_cg20.cmake)
add_cg20_success(cg20_scalar_types
    -DCGC=$<TARGET_FILE:cgc> -DPROFILE=generic
    -DSOURCE=${PROJECT_SOURCE_DIR}/tests/cg20/types/scalars.cg
    -P ${CMAKE_CURRENT_SOURCE_DIR}/check_cg20.cmake)
add_cg20_success(cg20_literal_suffixes
    -DCGC=$<TARGET_FILE:cgc> -DPROFILE=generic
    -DSOURCE=${PROJECT_SOURCE_DIR}/tests/cg20/literals/suffixes.cg
    -P ${CMAKE_CURRENT_SOURCE_DIR}/check_cg20.cmake)
add_cg20_success(cg20_literal_int_forms
    -DCGC=$<TARGET_FILE:cgc> -DPROFILE=generic
    -DSOURCE=${PROJECT_SOURCE_DIR}/tests/cg20/literals/int_forms.cg
    -P ${CMAKE_CURRENT_SOURCE_DIR}/check_cg20.cmake)
add_cg20_success(cg20_literal_float_forms
    -DCGC=$<TARGET_FILE:cgc> -DPROFILE=generic
    -DSOURCE=${PROJECT_SOURCE_DIR}/tests/cg20/literals/float_forms.cg
    -P ${CMAKE_CURRENT_SOURCE_DIR}/check_cg20.cmake)
add_cg20_success(cg20_literal_bool
    -DCGC=$<TARGET_FILE:cgc> -DPROFILE=generic
    -DSOURCE=${PROJECT_SOURCE_DIR}/tests/cg20/literals/bool_literals.cg
    -P ${CMAKE_CURRENT_SOURCE_DIR}/check_cg20.cmake)
add_cg20_success(cg20_type_compile_time
    -DCGC=$<TARGET_FILE:cgc> -DPROFILE=generic
    -DSOURCE=${PROJECT_SOURCE_DIR}/tests/cg20/types/compile_time.cg
    -P ${CMAKE_CURRENT_SOURCE_DIR}/check_cg20.cmake)
add_cg20_success(cg20_type_vectors_matrices
    -DCGC=$<TARGET_FILE:cgc> -DPROFILE=generic
    -DSOURCE=${PROJECT_SOURCE_DIR}/tests/cg20/types/vectors_matrices.cg
    -P ${CMAKE_CURRENT_SOURCE_DIR}/check_cg20.cmake)
add_cg20_success(cg20_nonfloat_matrix_typedef
    -DCGC=$<TARGET_FILE:cgc> -DPROFILE=generic
    -DSOURCE=${PROJECT_SOURCE_DIR}/tests/cg20/types/nonfloat_matrix_typedef.cg
    -P ${CMAKE_CURRENT_SOURCE_DIR}/check_cg20.cmake)
add_cg20_success(cg20_conversions_numeric
    -DCGC=$<TARGET_FILE:cgc> -DPROFILE=generic
    -DSOURCE=${PROJECT_SOURCE_DIR}/tests/cg20/conversions/numeric.cg
    -P ${CMAKE_CURRENT_SOURCE_DIR}/check_cg20.cmake)
add_cg20_success(cg20_conversions_aggregate
    -DCGC=$<TARGET_FILE:cgc> -DPROFILE=generic
    -DSOURCE=${PROJECT_SOURCE_DIR}/tests/cg20/conversions/aggregate_casts.cg
    -P ${CMAKE_CURRENT_SOURCE_DIR}/check_cg20.cmake)
add_cg20_success(cg20_conversion_struct_scalar
    -DCGC=$<TARGET_FILE:cgc> -DPROFILE=generic
    -DSOURCE=${PROJECT_SOURCE_DIR}/tests/cg20/conversions/struct_scalar.cg
    -P ${CMAKE_CURRENT_SOURCE_DIR}/check_cg20.cmake)
add_cg20_success(cg20_conversion_bool_int
    -DCGC=$<TARGET_FILE:cgc> -DPROFILE=generic
    -DSOURCE=${PROJECT_SOURCE_DIR}/tests/cg20/conversions/bool_int.cg
    -P ${CMAKE_CURRENT_SOURCE_DIR}/check_cg20.cmake)
add_cg20_success(cg20_conversion_reshape
    -DCGC=$<TARGET_FILE:cgc> -DPROFILE=generic
    -DSOURCE=${PROJECT_SOURCE_DIR}/tests/cg20/conversions/reshape.cg
    -P ${CMAKE_CURRENT_SOURCE_DIR}/check_cg20.cmake)
add_cg20_success(cg20_conversion_narrow_warn
    -DCGC=$<TARGET_FILE:cgc> -DPROFILE=generic
    -DSOURCE=${PROJECT_SOURCE_DIR}/tests/cg20/conversions/narrow_warn.cg
    "-DMESSAGE=implicit.conversion.may.lose.information"
    -P ${CMAKE_CURRENT_SOURCE_DIR}/check_cg20.cmake)
add_cg20_failure(cg20_conversion_reshape_mismatch
    -DCGC=$<TARGET_FILE:cgc> -DPROFILE=generic
    -DSOURCE=${PROJECT_SOURCE_DIR}/tests/cg20/diagnostics/reshape_mismatch.cg
    -DCODE=1033 -DEXPECTED_LINE=13 -DMESSAGE=cast.not.allowed
    -P ${CMAKE_CURRENT_SOURCE_DIR}/check_cg20_failure.cmake)
add_cg20_failure(cg20_remainder_float_diag
    -DCGC=$<TARGET_FILE:cgc> -DPROFILE=generic
    -DSOURCE=${PROJECT_SOURCE_DIR}/tests/cg20/diagnostics/remainder_float.cg
    -DCODE=1021 -DEXPECTED_LINE=9 "-DMESSAGE=operands.to.\"%\".must.be.integral"
    -P ${CMAKE_CURRENT_SOURCE_DIR}/check_cg20_failure.cmake)
add_cg20_success(cg20_unsized_array
    -DCGC=$<TARGET_FILE:cgc> -DPROFILE=generic
    -DSOURCE=${PROJECT_SOURCE_DIR}/tests/cg20/arrays/unsized.cg
    -P ${CMAKE_CURRENT_SOURCE_DIR}/check_cg20.cmake)
add_cg20_success(cg20_arrays_nested_struct
    -DCGC=$<TARGET_FILE:cgc> -DPROFILE=generic
    -DSOURCE=${PROJECT_SOURCE_DIR}/tests/cg20/arrays/nested_struct.cg
    -P ${CMAKE_CURRENT_SOURCE_DIR}/check_cg20.cmake)
add_cg20_success(cg20_array_length
    -DCGC=$<TARGET_FILE:cgc> -DPROFILE=generic
    -DSOURCE=${PROJECT_SOURCE_DIR}/tests/cg20/arrays/length.cg
    -P ${CMAKE_CURRENT_SOURCE_DIR}/check_cg20.cmake)
add_cg20_success(cg20_control_flow
    -DCGC=$<TARGET_FILE:cgc> -DPROFILE=generic
    -DSOURCE=${PROJECT_SOURCE_DIR}/tests/cg20/statements/control_flow.cg
    -P ${CMAKE_CURRENT_SOURCE_DIR}/check_cg20.cmake)
add_cg20_success(cg20_discard_generic
    -DCGC=$<TARGET_FILE:cgc> -DPROFILE=generic
    -DSOURCE=${PROJECT_SOURCE_DIR}/tests/cg20/statements/discard.cg
    -P ${CMAKE_CURRENT_SOURCE_DIR}/check_cg20.cmake)
add_cg20_success(cg20_swizzles
    -DCGC=$<TARGET_FILE:cgc> -DPROFILE=generic
    -DSOURCE=${PROJECT_SOURCE_DIR}/tests/cg20/expressions/swizzles.cg
    -P ${CMAKE_CURRENT_SOURCE_DIR}/check_cg20.cmake)
add_cg20_failure(cg20_unsized_initializer
    -DCGC=$<TARGET_FILE:cgc>
    -DSOURCE=${PROJECT_SOURCE_DIR}/tests/cg20/diagnostics/unsized_initializer.cg
    -DCODE=1117 -DEXPECTED_LINE=3
    "-DREASON=cannot.infer.array.size"
    "-DCONFIG=$<CONFIG>"
    "-DACTUAL=${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/cg20_unsized_initializer.txt"
    -P ${CMAKE_CURRENT_SOURCE_DIR}/check_generic_failure.cmake
)
add_cg20_success(cg20_static_global
    -DCGC=$<TARGET_FILE:cgc> -DPROFILE=generic
    -DSOURCE=${PROJECT_SOURCE_DIR}/tests/cg20/profile/static_global.cg
    "-DFORBID=hidden"
    -P ${CMAKE_CURRENT_SOURCE_DIR}/check_cg20.cmake)
add_cg20_success(cg20_sampler_base_compat
    -DCGC=$<TARGET_FILE:cgc> -DPROFILE=generic
    -DSOURCE=${PROJECT_SOURCE_DIR}/tests/cg20/samplers/compatible_base.cg
    -P ${CMAKE_CURRENT_SOURCE_DIR}/check_cg20.cmake)
add_cg20_success(cg20_sampler_rect
    -DCGC=$<TARGET_FILE:cgc> -DPROFILE=generic
    -DSOURCE=${PROJECT_SOURCE_DIR}/tests/cg20/samplers/rect.cg
    -P ${CMAKE_CURRENT_SOURCE_DIR}/check_cg20.cmake)
add_cg20_failure(cg20_sampler_assignment
    -DCGC=$<TARGET_FILE:cgc> -DPROFILE=generic
    -DSOURCE=${PROJECT_SOURCE_DIR}/tests/cg20/diagnostics/sampler_assignment.cg
    -DCODE=1150 -DMESSAGE=sampler.values
    -P ${CMAKE_CURRENT_SOURCE_DIR}/check_cg20_failure.cmake)
add_cg20_failure(cg20_sampler_incompatible
    -DCGC=$<TARGET_FILE:cgc> -DPROFILE=generic
    -DSOURCE=${PROJECT_SOURCE_DIR}/tests/cg20/diagnostics/sampler_incompatible.cg
    -DCODE=1102 "-DMESSAGE=incompatible.type.for.parameter"
    -P ${CMAKE_CURRENT_SOURCE_DIR}/check_cg20_failure.cmake)
add_cg20_failure(cg20_sampler_varying_global
    -DCGC=$<TARGET_FILE:cgc> -DPROFILE=generic
    -DSOURCE=${PROJECT_SOURCE_DIR}/tests/cg20/diagnostics/sampler_varying_global.cg
    -DCODE=1151 "-DMESSAGE=sampler.variables.must.be.global.uniforms"
    -P ${CMAKE_CURRENT_SOURCE_DIR}/check_cg20_failure.cmake)
add_cg20_success(cg20_sampler_helper_parameter
    -DCGC=$<TARGET_FILE:cgc> -DPROFILE=generic
    -DSOURCE=${PROJECT_SOURCE_DIR}/tests/cg20/samplers/helper_parameter.cg
    -P ${CMAKE_CURRENT_SOURCE_DIR}/check_cg20.cmake)

# The sampler-to-numeric cast rejection is a language conversion rule:
# it fires identically under the neutral generic profile, which is the
# strongest witness that no profile gate is involved.

add_cg20_failure(cg20_invalid_conversion
    -DCGC=$<TARGET_FILE:cgc> -DPROFILE=generic
    -DSOURCE=${PROJECT_SOURCE_DIR}/tests/cg20/diagnostics/invalid_conversion.cg
    -DCODE=1033 "-DMESSAGE=cast.not.allowed"
    -P ${CMAKE_CURRENT_SOURCE_DIR}/check_cg20_failure.cmake)
add_cg20_failure(cg20_reserved_word
    -DCGC=$<TARGET_FILE:cgc> -DPROFILE=generic
    -DSOURCE=${PROJECT_SOURCE_DIR}/tests/cg20/diagnostics/reserved_word.cg
    -DCODE=1300 -DMESSAGE=reserved.word
    -P ${CMAKE_CURRENT_SOURCE_DIR}/check_cg20_failure.cmake)
add_cg20_failure(cg20_reserved_word_case
    -DCGC=$<TARGET_FILE:cgc> -DPROFILE=generic
    -DSOURCE=${PROJECT_SOURCE_DIR}/tests/cg20/diagnostics/reserved_word_case.cg
    -DCODE=1300 -DEXPECTED_LINE=7 -DMESSAGE=reserved.word
    -P ${CMAKE_CURRENT_SOURCE_DIR}/check_cg20_failure.cmake)
add_cg20_failure(cg20_literal_overflow
    -DCGC=$<TARGET_FILE:cgc> -DPROFILE=generic
    -DSOURCE=${PROJECT_SOURCE_DIR}/tests/cg20/diagnostics/literal_overflow.cg
    -DCODE=130
    "-DMESSAGE=literal_overflow\\.cg\\(3\\).*literal_overflow\\.cg\\(4\\)"
    -P ${CMAKE_CURRENT_SOURCE_DIR}/check_cg20_failure.cmake)
add_cg20_failure(cg20_return_needs_semantic
    -DCGC=$<TARGET_FILE:cgc> -DPROFILE=generic
    -DSOURCE=${PROJECT_SOURCE_DIR}/tests/cg20/diagnostics/return_needs_semantic.cg
    -DCODE=5121 -DEXPECTED_LINE=5 "-DMESSAGE=must.declare.a.semantic.on.its.return.value"
    -P ${CMAKE_CURRENT_SOURCE_DIR}/check_cg20_failure.cmake)

# Entry return semantics are scalar/vector only (Task 1b ruling): a
# matrix entry return carries its semantic to the profile binder, which
# rejects it exactly like a hand-written struct matrix member.  Valid Cg
# IR, rejected backend -- the generic profile is itself the rejecting
# backend here, so no generic success twin can exist for this row.

add_cg20_failure(cg20_matrix_return_reject
    -DCGC=$<TARGET_FILE:cgc> -DPROFILE=generic
    -DSOURCE=${PROJECT_SOURCE_DIR}/tests/cg20/diagnostics/matrix_return.cg
    -DCODE=5107 -DEXPECTED_LINE=0 "-DMESSAGE=not.a.scalar.or.vector"
    -P ${CMAKE_CURRENT_SOURCE_DIR}/check_cg20_failure.cmake)
add_cg20_success(cg20_legacy_keywords
    -DCGC=$<TARGET_FILE:cgc> -DPROFILE=generic
    "-DEXTRA_ARGS=-version\;1.1"
    -DSOURCE=${PROJECT_SOURCE_DIR}/tests/cg20/profile/legacy_keywords.cg
    -P ${CMAKE_CURRENT_SOURCE_DIR}/check_cg20.cmake)
add_cg20_success(cg20_pp_object_macros
    -DCGC=$<TARGET_FILE:cgc> -DPROFILE=generic
    -DSOURCE=${PROJECT_SOURCE_DIR}/tests/cg20/pp/object_macros.cg
    -P ${CMAKE_CURRENT_SOURCE_DIR}/check_cg20.cmake)
add_cg20_success(cg20_pp_function_macros
    -DCGC=$<TARGET_FILE:cgc> -DPROFILE=generic
    -DSOURCE=${PROJECT_SOURCE_DIR}/tests/cg20/pp/function_macros.cg
    -P ${CMAKE_CURRENT_SOURCE_DIR}/check_cg20.cmake)
add_cg20_success(cg20_pp_conditionals
    -DCGC=$<TARGET_FILE:cgc> -DPROFILE=generic
    -DSOURCE=${PROJECT_SOURCE_DIR}/tests/cg20/pp/conditionals.cg
    -P ${CMAKE_CURRENT_SOURCE_DIR}/check_cg20.cmake)
add_cg20_success(cg20_pp_include
    -DCGC=$<TARGET_FILE:cgc>
    -DMAIN_SOURCE=${PROJECT_SOURCE_DIR}/tests/cg20/pp/include_main.cg
    -DINCLUDED_SOURCE=${PROJECT_SOURCE_DIR}/tests/cg20/pp/included_header.cg
    "-DWORK_DIR=${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/cg20-include-work"
    -P ${CMAKE_CURRENT_SOURCE_DIR}/check_cg20_include.cmake)
add_cg20_success(cg20_default_domains
    -DCGC=$<TARGET_FILE:cgc> -DPROFILE=generic
    -DSOURCE=${PROJECT_SOURCE_DIR}/tests/cg20/profile/default_domains.cg
    "-DMESSAGE=//semantic globalTint.*//var float4 globalTint.*-1 : 1.*//var float4 position : [$]vin[.]POSITION"
    -P ${CMAKE_CURRENT_SOURCE_DIR}/check_cg20.cmake)
add_cg20_success(cg20_input_alias
    -DCGC=$<TARGET_FILE:cgc> -DPROFILE=generic
    -DSOURCE=${PROJECT_SOURCE_DIR}/tests/cg20/profile/input_alias.cg
    -P ${CMAKE_CURRENT_SOURCE_DIR}/check_cg20.cmake)
add_cg20_success(cg20_global_varying
    -DCGC=$<TARGET_FILE:cgc> -DPROFILE=generic
    -DSOURCE=${PROJECT_SOURCE_DIR}/tests/cg20/profile/global_varying.cg
    -P ${CMAKE_CURRENT_SOURCE_DIR}/check_cg20.cmake)
add_cg20_failure(cg20_output_alias
    -DCGC=$<TARGET_FILE:cgc> -DPROFILE=generic
    -DSOURCE=${PROJECT_SOURCE_DIR}/tests/cg20/diagnostics/output_alias.cg
    -DCODE=5122 "-DMESSAGE=outputs.conflict.at.semantic"
    -P ${CMAKE_CURRENT_SOURCE_DIR}/check_cg20_failure.cmake)
add_cg20_failure(cg20_semantic_case_collision
    -DCGC=$<TARGET_FILE:cgc> -DPROFILE=generic
    -DSOURCE=${PROJECT_SOURCE_DIR}/tests/cg20/diagnostics/semantic_case_collision.cg
    -DCODE=5122 "-DMESSAGE=outputs.conflict.at.semantic"
    -P ${CMAKE_CURRENT_SOURCE_DIR}/check_cg20_failure.cmake)
add_cg20_success(cg20_interface_conformance
    -DCGC=$<TARGET_FILE:cgc> -DPROFILE=generic
    -DSOURCE=${PROJECT_SOURCE_DIR}/tests/cg20/interfaces/conformance.cg
    -P ${CMAKE_CURRENT_SOURCE_DIR}/check_cg20.cmake)
add_cg20_success(cg20_interface_dispatch
    -DCGC=$<TARGET_FILE:cgc> -DPROFILE=generic
    -DSOURCE=${PROJECT_SOURCE_DIR}/tests/cg20/interfaces/dispatch.cg
    -P ${CMAKE_CURRENT_SOURCE_DIR}/check_cg20.cmake)
add_cg20_success(cg20_overload_defaults
    -DCGC=$<TARGET_FILE:cgc> -DPROFILE=generic
    -DSOURCE=${PROJECT_SOURCE_DIR}/tests/cg20/overloads/defaults.cg
    -P ${CMAKE_CURRENT_SOURCE_DIR}/check_cg20.cmake)

# Task 19 profile-reject fixtures: valid Cg 2.0 sources (generic must
# compile each) whose GLSL lowering is rejected at profile validation
# with a specific unsupported reason instead of reaching codegen.

add_cg20_success(cg20_glsl_interface_dispatch_generic
    -DCGC=$<TARGET_FILE:cgc> -DPROFILE=generic
    -DSOURCE=${PROJECT_SOURCE_DIR}/tests/cg20/diagnostics/glsl_interface_dispatch.cg
    -P ${CMAKE_CURRENT_SOURCE_DIR}/check_cg20.cmake)
add_cg20_failure(glslv_glsl_interface_dispatch
    -DCGC=$<TARGET_FILE:cgc> -DPROFILE=glslv
    -DSOURCE=${PROJECT_SOURCE_DIR}/tests/cg20/diagnostics/glsl_interface_dispatch.cg
    "-DCONFIG=$<CONFIG>"
    "-DACTUAL=${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/glslv_glsl_interface_dispatch.glsl"
    -DCODE=6200 -DEXPECTED_LINE=19 -DREASON=interface
    -P ${CMAKE_CURRENT_SOURCE_DIR}/check_glsl_failure.cmake)

add_cg20_success(cg20_glsl_unsized_array_generic
    -DCGC=$<TARGET_FILE:cgc> -DPROFILE=generic
    -DSOURCE=${PROJECT_SOURCE_DIR}/tests/cg20/diagnostics/glsl_unsized_array.cg
    -P ${CMAKE_CURRENT_SOURCE_DIR}/check_cg20.cmake)
add_cg20_failure(glslv_glsl_unsized_array
    -DCGC=$<TARGET_FILE:cgc> -DPROFILE=glslv
    -DSOURCE=${PROJECT_SOURCE_DIR}/tests/cg20/diagnostics/glsl_unsized_array.cg
    "-DCONFIG=$<CONFIG>"
    "-DACTUAL=${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/glslv_glsl_unsized_array.glsl"
    -DCODE=6200 -DEXPECTED_LINE=6 "-DREASON=dynamic.unsized.array"
    -P ${CMAKE_CURRENT_SOURCE_DIR}/check_glsl_failure.cmake)

# Task 9 carry-in pin: samplerRECT has no GLSL spelling; rejection is
# anchored in the language sampler rules (post pid-gate retirement).
add_cg20_failure(glslf_sampler_rect
    -DCGC=$<TARGET_FILE:cgc> -DPROFILE=glslf
    -DSOURCE=${PROJECT_SOURCE_DIR}/tests/cg20/samplers/rect.cg
    "-DCONFIG=$<CONFIG>"
    "-DACTUAL=${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/glslf_sampler_rect.glsl"
    -DCODE=6200 -DEXPECTED_LINE=4 -DREASON=samplerRECT
    -P ${CMAKE_CURRENT_SOURCE_DIR}/check_glsl_failure.cmake)

# Cg 2.0 reachability fixtures (Task 16): reachable.cg compiles through
# generic; unreachable_profile_feature keeps a glslv-rejected operation
# inside an uncalled helper and must still generate code under glslv.

add_cg20_success(cg20_reachable
    -DCGC=$<TARGET_FILE:cgc> -DPROFILE=generic
    -DSOURCE=${PROJECT_SOURCE_DIR}/tests/cg20/ir/reachable.cg
    -P ${CMAKE_CURRENT_SOURCE_DIR}/check_cg20.cmake)

# Compiler-level normalized golden (Task 17): under the default Cg 2.0
# language the generic profile emits the verified module as normalized
# Cg IR; reachable.cg pins its complete normalized output byte-for-byte.

add_cg20_success(cg20_ir_normalized
    -DCGC=$<TARGET_FILE:cgc> -DPROFILE=generic
    -DSOURCE=${PROJECT_SOURCE_DIR}/tests/cg20/ir/reachable.cg
    -DEXPECTED=${PROJECT_SOURCE_DIR}/tests/cg20/ir/normalized.expected
    "-DACTUAL=${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/cg20_ir_normalized.txt"
    -P ${CMAKE_CURRENT_SOURCE_DIR}/check_cg20.cmake)
if(GLSLANG_VALIDATOR)
    set(cg20_unreachable_validator "-DVALIDATOR=${GLSLANG_VALIDATOR}")
else()
    set(cg20_unreachable_validator "")
endif()
add_cg20_success(cg20_unreachable_profile_feature
    -DCGC=$<TARGET_FILE:cgc>
    -DPROFILE=glslv
    -DSTAGE=vert
    -DENTRY=main
    -DSOURCE=${PROJECT_SOURCE_DIR}/tests/cg20/ir/unreachable_profile_feature.cg
    "-DCONFIG=$<CONFIG>"
    "-DOUTPUT=${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/cg20_unreachable_profile_feature.glsl"
    "-DMESSAGE=#version 110"
    ${cg20_unreachable_validator}
    -P ${CMAKE_CURRENT_SOURCE_DIR}/check_cg20_output.cmake)

# Cg 2.0 standard-library fixtures (Task 13): the numeric and texture
# families compile through generic only; predefined_outputs exercises
# the fragment-family helper structures under glslf and rejects them
# under glslv.

add_cg20_success(cg20_intrinsic_numeric
    -DCGC=$<TARGET_FILE:cgc> -DPROFILE=generic
    -DSOURCE=${PROJECT_SOURCE_DIR}/tests/cg20/intrinsics/numeric.cg
    -P ${CMAKE_CURRENT_SOURCE_DIR}/check_cg20.cmake)
add_cg20_success(cg20_intrinsic_out_params
    -DCGC=$<TARGET_FILE:cgc> -DPROFILE=generic
    -DSOURCE=${PROJECT_SOURCE_DIR}/tests/cg20/intrinsics/out_params.cg
    -P ${CMAKE_CURRENT_SOURCE_DIR}/check_cg20.cmake)
add_cg20_success(cg20_intrinsic_texture_families
    -DCGC=$<TARGET_FILE:cgc> -DPROFILE=generic
    -DSOURCE=${PROJECT_SOURCE_DIR}/tests/cg20/intrinsics/texture_families.cg
    -P ${CMAKE_CURRENT_SOURCE_DIR}/check_cg20.cmake)
add_cg20_success(cg20_intrinsic_debug
    -DCGC=$<TARGET_FILE:cgc> -DPROFILE=generic
    -DSOURCE=${PROJECT_SOURCE_DIR}/tests/cg20/intrinsics/debug.cg
    -P ${CMAKE_CURRENT_SOURCE_DIR}/check_cg20.cmake)
# The fragment-family helper structures answer only under glslf; both
# rejecting profiles report the unknown-profile-specifier diagnostic at
# the first use of the undeclared structure name.

add_cg20_success(cg20_intrinsic_predefined_outputs_glslf
    -DCGC=$<TARGET_FILE:cgc> -DPROFILE=glslf
    -DSOURCE=${PROJECT_SOURCE_DIR}/tests/cg20/intrinsics/predefined_outputs.cg
    -P ${CMAKE_CURRENT_SOURCE_DIR}/check_cg20.cmake)

set(cg20_predefined_outputs_source
    ${PROJECT_SOURCE_DIR}/tests/cg20/intrinsics/predefined_outputs.cg)
foreach(reject_profile glslv generic)
    add_cg20_failure(cg20_intrinsic_predefined_outputs_reject_${reject_profile}
        -DCGC=$<TARGET_FILE:cgc>
        -DPROFILE=${reject_profile}
        -DSOURCE=${cg20_predefined_outputs_source}
        -DCODE=1165
        -DREASON=does.not.name.a.known.profile
        -P ${CMAKE_CURRENT_SOURCE_DIR}/check_helper_reject.cmake
    )
endforeach()

# Profile-qualified overload resolution: each stage must select its own
# exact overload and pass glslang validation on the generated program.
# The same fixture run under the neutral generic profile must resolve to
# the open-profile overload, pinning the last tier of the precedence
# rule from the language side.

function(add_cg20_profile_selection name profile stage entry marker)
    if(GLSLANG_VALIDATOR)
        set(cg20_profile_validator "-DVALIDATOR=${GLSLANG_VALIDATOR}")
    else()
        set(cg20_profile_validator "")
    endif()
    add_cg20_success(${name}
        -DCGC=$<TARGET_FILE:cgc>
        -DPROFILE=${profile}
        -DSTAGE=${stage}
        -DENTRY=${entry}
        -DSOURCE=${PROJECT_SOURCE_DIR}/tests/cg20/overloads/profile.cg
        "-DCONFIG=$<CONFIG>"
        "-DOUTPUT=${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/${name}.glsl"
        "-DMESSAGE=${marker}"
        ${cg20_profile_validator}
        -P ${CMAKE_CURRENT_SOURCE_DIR}/check_cg20_output.cmake
    )
endfunction()
add_cg20_profile_selection(cg20_profile_overload_vertex glslv vert mainV
    "0\\.25, *0\\.5, *0\\.5, *1\\.0")
add_cg20_profile_selection(cg20_profile_overload_fragment glslf frag mainF
    "0\\.875, *0\\.5, *0\\.5, *1\\.0")
add_cg20_success(cg20_profile_overload_open
    -DCGC=$<TARGET_FILE:cgc> -DPROFILE=generic
    -DENTRY=mainV
    -DSOURCE=${PROJECT_SOURCE_DIR}/tests/cg20/overloads/profile.cg
    "-DCONFIG=$<CONFIG>"
    "-DOUTPUT=${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/cg20_profile_overload_open.glsl"
    "-DMESSAGE=0\\.625000"
    -P ${CMAKE_CURRENT_SOURCE_DIR}/check_cg20_output.cmake)

add_cg20_failure(cg20_default_after_optional
    -DCGC=$<TARGET_FILE:cgc> -DPROFILE=generic
    -DSOURCE=${PROJECT_SOURCE_DIR}/tests/cg20/diagnostics/default_after_optional.cg
    -DCODE=1160 -DMESSAGE=default.argument.for
    -P ${CMAKE_CURRENT_SOURCE_DIR}/check_cg20_failure.cmake)
add_cg20_failure(cg20_ambiguous_overload
    -DCGC=$<TARGET_FILE:cgc> -DPROFILE=generic
    -DSOURCE=${PROJECT_SOURCE_DIR}/tests/cg20/diagnostics/ambiguous_overload.cg
    -DCODE=1101
    "-DMESSAGE=ambiguous.overloaded.function.reference"
    "-DNOTES=candidate #1: float probe(float);candidate #2: float probe(int)"
    -P ${CMAKE_CURRENT_SOURCE_DIR}/check_cg20_failure.cmake)

# Geometry topology modifiers (Task 2): positives compile under the
# neutral generic profile; negatives pin exact code and line.  The
# modifiers fixture also uses AttribArray<T> parameters whose syntax
# arrives with the attribute-array task, so its registration stays
# DISABLED until that task enables it.

add_cg20_success(cg20_geometry_modifiers
    -DCGC=$<TARGET_FILE:cgc> -DPROFILE=generic
    -DSOURCE=${PROJECT_SOURCE_DIR}/tests/cg20/geometry/modifiers.cg
    -P ${CMAKE_CURRENT_SOURCE_DIR}/check_cg20.cmake)
set_tests_properties(cg20_geometry_modifiers PROPERTIES DISABLED TRUE)

add_cg20_success(cg20_geometry_options_only
    -DCGC=$<TARGET_FILE:cgc> -DPROFILE=generic
    "-DEXTRA_ARGS=-po\;TRIANGLE\;-po\;Vertices=3"
    -DSOURCE=${PROJECT_SOURCE_DIR}/tests/cg20/geometry/options_only.cg
    -P ${CMAKE_CURRENT_SOURCE_DIR}/check_cg20.cmake)

function(add_cg20_geometry_default_output name entry)
    add_cg20_success(${name}
        -DCGC=$<TARGET_FILE:cgc> -DPROFILE=generic
        "-DEXTRA_ARGS=-entry\;${entry}"
        -DSOURCE=${PROJECT_SOURCE_DIR}/tests/cg20/geometry/default_outputs.cg
        -DEXPECTED=${PROJECT_SOURCE_DIR}/tests/cg20/geometry/${name}.expected
        "-DACTUAL=${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/${name}.txt"
        -P ${CMAKE_CURRENT_SOURCE_DIR}/check_cg20.cmake)
endfunction()
add_cg20_geometry_default_output(cg20_geometry_default_point point_default)
add_cg20_geometry_default_output(cg20_geometry_default_line line_default)
add_cg20_geometry_default_output(cg20_geometry_default_line_adj line_adj_default)
add_cg20_geometry_default_output(cg20_geometry_default_triangle triangle_default)
add_cg20_geometry_default_output(cg20_geometry_default_triangle_adj triangle_adj_default)

add_cg20_failure(cg20_geometry_duplicate_modifier
    -DCGC=$<TARGET_FILE:cgc> -DPROFILE=generic
    -DSOURCE=${PROJECT_SOURCE_DIR}/tests/cg20/diagnostics/geometry_duplicate_modifier.cg
    -DCODE=6303 -DEXPECTED_LINE=4
    "-DMESSAGE=repeated.geometry.modifier"
    -P ${CMAKE_CURRENT_SOURCE_DIR}/check_cg20_failure.cmake)
add_cg20_failure(cg20_geometry_conflicting_modifier
    -DCGC=$<TARGET_FILE:cgc> -DPROFILE=generic
    -DSOURCE=${PROJECT_SOURCE_DIR}/tests/cg20/diagnostics/geometry_conflicting_modifier.cg
    -DCODE=6304 -DEXPECTED_LINE=4
    "-DMESSAGE=conflicts.with"
    -P ${CMAKE_CURRENT_SOURCE_DIR}/check_cg20_failure.cmake)
add_cg20_failure(cg20_geometry_modifier_on_variable
    -DCGC=$<TARGET_FILE:cgc> -DPROFILE=generic
    -DSOURCE=${PROJECT_SOURCE_DIR}/tests/cg20/diagnostics/geometry_modifier_on_variable.cg
    -DCODE=6305 -DEXPECTED_LINE=4
    "-DMESSAGE=geometry.modifier.requires.a.function"
    -P ${CMAKE_CURRENT_SOURCE_DIR}/check_cg20_failure.cmake)
add_cg20_failure(cg20_geometry_cg11
    -DCGC=$<TARGET_FILE:cgc> -DPROFILE=generic
    "-DEXTRA_ARGS=-version\;1.1"
    -DSOURCE=${PROJECT_SOURCE_DIR}/tests/cg20/diagnostics/geometry_cg11.cg
    -DCODE=1400 -DEXPECTED_LINE=5
    "-DMESSAGE=requires.the.Cg.2.0.language"
    -P ${CMAKE_CURRENT_SOURCE_DIR}/check_cg20_failure.cmake)

# Attribute arrays (Task 3): canonical read-only AttribArray<T> types.
# The four positive fixtures need selected-program analysis (resolved
# type views plus .length folding), which Task 5 wires into
# CompileProgram -- exactly like Task 4's operation positives they stay
# registered DISABLED TRUE until then.  Every negative fixture fires at
# declaration or expression checking and is enabled now.

add_cg20_success(cg20_geometry_attrib_array
    -DCGC=$<TARGET_FILE:cgc> -DPROFILE=generic
    -DSOURCE=${PROJECT_SOURCE_DIR}/tests/cg20/geometry/attrib_array.cg
    -P ${CMAKE_CURRENT_SOURCE_DIR}/check_cg20.cmake)
set_tests_properties(cg20_geometry_attrib_array PROPERTIES DISABLED TRUE)

add_cg20_success(cg20_geometry_attrib_array_aggregate
    -DCGC=$<TARGET_FILE:cgc> -DPROFILE=generic
    -DSOURCE=${PROJECT_SOURCE_DIR}/tests/cg20/geometry/attrib_array_aggregate.cg
    -P ${CMAKE_CURRENT_SOURCE_DIR}/check_cg20.cmake)
set_tests_properties(cg20_geometry_attrib_array_aggregate PROPERTIES DISABLED TRUE)

add_cg20_success(cg20_geometry_attrib_array_helper
    -DCGC=$<TARGET_FILE:cgc> -DPROFILE=generic
    -DSOURCE=${PROJECT_SOURCE_DIR}/tests/cg20/geometry/attrib_array_helper.cg
    -P ${CMAKE_CURRENT_SOURCE_DIR}/check_cg20.cmake)
set_tests_properties(cg20_geometry_attrib_array_helper PROPERTIES DISABLED TRUE)

function(add_cg20_geometry_attrib_array_length name entry)
    add_cg20_success(${name}
        -DCGC=$<TARGET_FILE:cgc> -DPROFILE=generic
        "-DEXTRA_ARGS=-entry\;${entry}"
        -DSOURCE=${PROJECT_SOURCE_DIR}/tests/cg20/geometry/attrib_array_length.cg
        -P ${CMAKE_CURRENT_SOURCE_DIR}/check_cg20.cmake)
    set_tests_properties(${name} PROPERTIES DISABLED TRUE)
endfunction()
add_cg20_geometry_attrib_array_length(cg20_geometry_attrib_array_length_point point_length)
add_cg20_geometry_attrib_array_length(cg20_geometry_attrib_array_length_line line_length)
add_cg20_geometry_attrib_array_length(cg20_geometry_attrib_array_length_line_adj line_adj_length)
add_cg20_geometry_attrib_array_length(cg20_geometry_attrib_array_length_triangle triangle_length)
add_cg20_geometry_attrib_array_length(cg20_geometry_attrib_array_length_triangle_adj triangle_adj_length)

add_cg20_failure(cg20_geometry_attrib_array_global
    -DCGC=$<TARGET_FILE:cgc> -DPROFILE=generic
    -DSOURCE=${PROJECT_SOURCE_DIR}/tests/cg20/diagnostics/geometry_attrib_array_global.cg
    -DCODE=6306 -DEXPECTED_LINE=4
    "-DMESSAGE=not.permitted.on.a.global.variable"
    -P ${CMAKE_CURRENT_SOURCE_DIR}/check_cg20_failure.cmake)
add_cg20_failure(cg20_geometry_attrib_array_uniform
    -DCGC=$<TARGET_FILE:cgc> -DPROFILE=generic
    -DSOURCE=${PROJECT_SOURCE_DIR}/tests/cg20/diagnostics/geometry_attrib_array_uniform.cg
    -DCODE=6306 -DEXPECTED_LINE=4
    "-DMESSAGE=not.permitted.on.a.uniform.variable"
    -P ${CMAKE_CURRENT_SOURCE_DIR}/check_cg20_failure.cmake)
add_cg20_failure(cg20_geometry_attrib_array_output
    -DCGC=$<TARGET_FILE:cgc> -DPROFILE=generic
    -DSOURCE=${PROJECT_SOURCE_DIR}/tests/cg20/diagnostics/geometry_attrib_array_output.cg
    -DCODE=6306 -DEXPECTED_LINE=5
    "-DMESSAGE=not.permitted.on.an.output.parameter"
    -P ${CMAKE_CURRENT_SOURCE_DIR}/check_cg20_failure.cmake)
add_cg20_failure(cg20_geometry_attrib_array_return
    -DCGC=$<TARGET_FILE:cgc> -DPROFILE=generic
    -DSOURCE=${PROJECT_SOURCE_DIR}/tests/cg20/diagnostics/geometry_attrib_array_return.cg
    -DCODE=6306 -DEXPECTED_LINE=4
    "-DMESSAGE=not.permitted.on.a.function.return.value"
    -P ${CMAKE_CURRENT_SOURCE_DIR}/check_cg20_failure.cmake)
add_cg20_failure(cg20_geometry_attrib_array_local
    -DCGC=$<TARGET_FILE:cgc> -DPROFILE=generic
    -DSOURCE=${PROJECT_SOURCE_DIR}/tests/cg20/diagnostics/geometry_attrib_array_local.cg
    -DCODE=6306 -DEXPECTED_LINE=6
    "-DMESSAGE=not.permitted.on.a.local.variable"
    -P ${CMAKE_CURRENT_SOURCE_DIR}/check_cg20_failure.cmake)
add_cg20_failure(cg20_geometry_attrib_array_member
    -DCGC=$<TARGET_FILE:cgc> -DPROFILE=generic
    -DSOURCE=${PROJECT_SOURCE_DIR}/tests/cg20/diagnostics/geometry_attrib_array_member.cg
    -DCODE=6306 -DEXPECTED_LINE=6
    "-DMESSAGE=not.permitted.on.a.struct.member"
    -P ${CMAKE_CURRENT_SOURCE_DIR}/check_cg20_failure.cmake)
add_cg20_failure(cg20_geometry_attrib_array_non_geometry
    -DCGC=$<TARGET_FILE:cgc> -DPROFILE=generic
    -DSOURCE=${PROJECT_SOURCE_DIR}/tests/cg20/diagnostics/geometry_attrib_array_non_geometry.cg
    -DCODE=6309 -DEXPECTED_LINE=6
    "-DMESSAGE=requires.a.geometry.program"
    -P ${CMAKE_CURRENT_SOURCE_DIR}/check_cg20_failure.cmake)
add_cg20_failure(cg20_geometry_attrib_array_void
    -DCGC=$<TARGET_FILE:cgc> -DPROFILE=generic
    -DSOURCE=${PROJECT_SOURCE_DIR}/tests/cg20/diagnostics/geometry_attrib_array_void.cg
    -DCODE=6307 -DEXPECTED_LINE=4
    "-DMESSAGE=invalid.AttribArray.element.type..void"
    -P ${CMAKE_CURRENT_SOURCE_DIR}/check_cg20_failure.cmake)
add_cg20_failure(cg20_geometry_attrib_array_sampler
    -DCGC=$<TARGET_FILE:cgc> -DPROFILE=generic
    -DSOURCE=${PROJECT_SOURCE_DIR}/tests/cg20/diagnostics/geometry_attrib_array_sampler.cg
    -DCODE=6307 -DEXPECTED_LINE=4
    "-DMESSAGE=invalid.AttribArray.element.type"
    -P ${CMAKE_CURRENT_SOURCE_DIR}/check_cg20_failure.cmake)
add_cg20_failure(cg20_geometry_attrib_array_function
    -DCGC=$<TARGET_FILE:cgc> -DPROFILE=generic
    -DSOURCE=${PROJECT_SOURCE_DIR}/tests/cg20/diagnostics/geometry_attrib_array_function.cg
    -DCODE=501 -DEXPECTED_LINE=9
    "-DMESSAGE=type.name.expected"
    -P ${CMAKE_CURRENT_SOURCE_DIR}/check_cg20_failure.cmake)
add_cg20_failure(cg20_geometry_attrib_array_interface
    -DCGC=$<TARGET_FILE:cgc> -DPROFILE=generic
    -DSOURCE=${PROJECT_SOURCE_DIR}/tests/cg20/diagnostics/geometry_attrib_array_interface.cg
    -DCODE=6307 -DEXPECTED_LINE=9
    "-DMESSAGE=invalid.AttribArray.element.type"
    -P ${CMAKE_CURRENT_SOURCE_DIR}/check_cg20_failure.cmake)
add_cg20_failure(cg20_geometry_attrib_array_write_whole
    -DCGC=$<TARGET_FILE:cgc> -DPROFILE=generic
    -DSOURCE=${PROJECT_SOURCE_DIR}/tests/cg20/diagnostics/geometry_attrib_array_write_whole.cg
    -DCODE=6308 -DEXPECTED_LINE=6
    "-DMESSAGE=values.are.read-only"
    -P ${CMAKE_CURRENT_SOURCE_DIR}/check_cg20_failure.cmake)
add_cg20_failure(cg20_geometry_attrib_array_write_element
    -DCGC=$<TARGET_FILE:cgc> -DPROFILE=generic
    -DSOURCE=${PROJECT_SOURCE_DIR}/tests/cg20/diagnostics/geometry_attrib_array_write_element.cg
    -DCODE=6308 -DEXPECTED_LINE=7
    "-DMESSAGE=values.are.read-only"
    -P ${CMAKE_CURRENT_SOURCE_DIR}/check_cg20_failure.cmake)
add_cg20_failure(cg20_geometry_attrib_array_cg11
    -DCGC=$<TARGET_FILE:cgc> -DPROFILE=generic
    "-DEXTRA_ARGS=-version\;1.1"
    -DSOURCE=${PROJECT_SOURCE_DIR}/tests/cg20/diagnostics/geometry_attrib_array_cg11.cg
    -DCODE=1400 -DEXPECTED_LINE=5
    "-DMESSAGE=requires.the.Cg.2.0.language"
    -P ${CMAKE_CURRENT_SOURCE_DIR}/check_cg20_failure.cmake)

# Parser regeneration determinism: regenerating from parser.y must
# reproduce both checked-in artifacts byte-for-byte without warnings or
# grammar conflicts.  Skipped (return code 77) where bison is absent.

find_package(BISON QUIET)
if(BISON_FOUND)
    set(cg20_parser_regen_bison "${BISON_EXECUTABLE}")
else()
    set(cg20_parser_regen_bison "BISON-NOTFOUND")
endif()
add_test(
    NAME parser_regeneration
    COMMAND ${CMAKE_COMMAND}
        -DBISON=${cg20_parser_regen_bison}
        -DSOURCE=${PROJECT_SOURCE_DIR}/parser.y
        -DEXPECTED_PARSER=${PROJECT_SOURCE_DIR}/parser.c
        -DEXPECTED_DEFINES=${PROJECT_SOURCE_DIR}/parser.h
        "-DWORK_DIR=${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/parser-regeneration"
        -P ${CMAKE_CURRENT_SOURCE_DIR}/check_parser_regeneration.cmake
)
if(NOT BISON_FOUND)
    set_tests_properties(parser_regeneration PROPERTIES SKIP_RETURN_CODE 77)
endif()

# Profile-failure call-path notes (Task 18): the primary diagnostic at
# the rejected operation is followed by one note per hop back to the
# entry, naming second, first, then main in order.  The rejection is a
# generation-time decision, so this runner must not pass -nocode.

add_cg20_failure(cg20_profile_call_path
    -DCGC=$<TARGET_FILE:cgc>
    -DPROFILE=glslv
    -DSOURCE=${PROJECT_SOURCE_DIR}/tests/cg20/diagnostics/profile_call_path.cg
    "-DCONFIG=$<CONFIG>"
    "-DACTUAL=${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/cg20_profile_call_path.glsl"
    -DCODE=6202
    -DEXPECTED_LINE=10
    -DMESSAGE=discard
    "-DNOTES=profile failure in function second;called from function first;reached from entry function main"
    -P ${CMAKE_CURRENT_SOURCE_DIR}/check_diagnostic.cmake
)
add_cg20_failure(cg20_interface_data_member
    -DCGC=$<TARGET_FILE:cgc> -DPROFILE=generic
    -DSOURCE=${PROJECT_SOURCE_DIR}/tests/cg20/diagnostics/interface_data.cg
    -DCODE=1155 "-DMESSAGE=data.members.not.allowed.in.interface"
    -P ${CMAKE_CURRENT_SOURCE_DIR}/check_cg20_failure.cmake)
add_cg20_failure(cg20_interface_missing_method
    -DCGC=$<TARGET_FILE:cgc> -DPROFILE=generic
    -DSOURCE=${PROJECT_SOURCE_DIR}/tests/cg20/diagnostics/interface_missing_method.cg
    -DCODE=1156 "-DMESSAGE=does.not.implement.interface.method"
    -P ${CMAKE_CURRENT_SOURCE_DIR}/check_cg20_failure.cmake)
add_cg20_failure(cg20_interface_bad_signature
    -DCGC=$<TARGET_FILE:cgc> -DPROFILE=generic
    -DSOURCE=${PROJECT_SOURCE_DIR}/tests/cg20/diagnostics/interface_bad_signature.cg
    -DCODE=1157 "-DMESSAGE=does.not.match.interface.method"
    -P ${CMAKE_CURRENT_SOURCE_DIR}/check_cg20_failure.cmake)

add_executable(glsl_ir_unit
    ${PROJECT_SOURCE_DIR}/glsl_ir.c
    ${PROJECT_SOURCE_DIR}/glsl_codegen.c
    glsl_ir_test.c
)

target_include_directories(glsl_ir_unit PRIVATE
    ${PROJECT_SOURCE_DIR}
)

target_compile_definitions(glsl_ir_unit PRIVATE
    GLSL_IR_TEST_SEAMS=1
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

add_test(
    NAME glsl_ir_assertions_active
    COMMAND ${CMAKE_COMMAND}
        -DUNIT=$<TARGET_FILE:glsl_ir_unit>
        -P ${CMAKE_CURRENT_SOURCE_DIR}/check_assertions_active.cmake
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

# Real GlslLowerCgIR driven by direct CgIRModule construction: the
# matrix group-write recognizer branches (distinct-value coordinate
# validation, producer-marked count cap, elementwise assignment shape)
# have no shader-level source that shares one base node across sibling
# stores, so they are covered at the unit layer.

add_executable(glsl_lower_ir_unit
    ${PROJECT_SOURCE_DIR}/atom.c
    ${PROJECT_SOURCE_DIR}/cg_ir.c
    ${PROJECT_SOURCE_DIR}/cg_numeric.c
    ${PROJECT_SOURCE_DIR}/cg_stdlib.c
    ${PROJECT_SOURCE_DIR}/cg_types.c
    ${PROJECT_SOURCE_DIR}/glsl_codegen.c
    ${PROJECT_SOURCE_DIR}/glsl_hal.c
    ${PROJECT_SOURCE_DIR}/glsl_ir.c
    ${PROJECT_SOURCE_DIR}/glsl_lower.c
    ${PROJECT_SOURCE_DIR}/glslf_hal.c
    ${PROJECT_SOURCE_DIR}/glslv_hal.c
    ${PROJECT_SOURCE_DIR}/memory.c
    ${PROJECT_SOURCE_DIR}/symbols.c
    glsl_lower_ir_test.c
)

target_include_directories(glsl_lower_ir_unit PRIVATE
    ${PROJECT_SOURCE_DIR}
)

set_target_properties(glsl_lower_ir_unit PROPERTIES
    C_STANDARD 90
    C_STANDARD_REQUIRED YES
    C_EXTENSIONS YES
)

add_test(
    NAME glsl_lower_ir_unit
    COMMAND glsl_lower_ir_unit
)

add_test(
    NAME glsl_lower_ir_assertions_active
    COMMAND ${CMAKE_COMMAND}
        -DUNIT=$<TARGET_FILE:glsl_lower_ir_unit>
        -P ${CMAKE_CURRENT_SOURCE_DIR}/check_assertions_active.cmake
)

add_executable(cg_language_unit
    ${PROJECT_SOURCE_DIR}/language.c
    cg_language_test.c
)
target_include_directories(cg_language_unit PRIVATE ${PROJECT_SOURCE_DIR})
set_target_properties(cg_language_unit PROPERTIES
    C_STANDARD 90 C_STANDARD_REQUIRED YES C_EXTENSIONS YES)
add_test(NAME cg_language_unit COMMAND cg_language_unit)

add_executable(cg_types_unit
    ${PROJECT_SOURCE_DIR}/atom.c
    ${PROJECT_SOURCE_DIR}/cg_numeric.c
    ${PROJECT_SOURCE_DIR}/cg_types.c
    ${PROJECT_SOURCE_DIR}/cg_stdlib.c
    ${PROJECT_SOURCE_DIR}/memory.c
    ${PROJECT_SOURCE_DIR}/symbols.c
    cg_types_test.c
)
target_include_directories(cg_types_unit PRIVATE
    ${PROJECT_SOURCE_DIR}
)
set_target_properties(cg_types_unit PROPERTIES
    C_STANDARD 90
    C_STANDARD_REQUIRED YES
    C_EXTENSIONS YES
)
add_test(
    NAME cg_types_unit
    COMMAND cg_types_unit
)

add_executable(cg_overload_unit
    ${PROJECT_SOURCE_DIR}/atom.c
    ${PROJECT_SOURCE_DIR}/cg_numeric.c
    ${PROJECT_SOURCE_DIR}/cg_types.c
    ${PROJECT_SOURCE_DIR}/cg_overload.c
    ${PROJECT_SOURCE_DIR}/cg_stdlib.c
    ${PROJECT_SOURCE_DIR}/memory.c
    ${PROJECT_SOURCE_DIR}/symbols.c
    cg_overload_test.c
)
target_include_directories(cg_overload_unit PRIVATE
    ${PROJECT_SOURCE_DIR}
)
set_target_properties(cg_overload_unit PROPERTIES
    C_STANDARD 90
    C_STANDARD_REQUIRED YES
    C_EXTENSIONS YES
)
add_test(
    NAME cg_overload_unit
    COMMAND cg_overload_unit
)

add_executable(cg_stdlib_unit
    ${PROJECT_SOURCE_DIR}/atom.c
    ${PROJECT_SOURCE_DIR}/cg_numeric.c
    ${PROJECT_SOURCE_DIR}/cg_types.c
    ${PROJECT_SOURCE_DIR}/cg_stdlib.c
    ${PROJECT_SOURCE_DIR}/memory.c
    ${PROJECT_SOURCE_DIR}/symbols.c
    cg_stdlib_test.c
)
target_include_directories(cg_stdlib_unit PRIVATE
    ${PROJECT_SOURCE_DIR}
)
set_target_properties(cg_stdlib_unit PROPERTIES
    C_STANDARD 90
    C_STANDARD_REQUIRED YES
    C_EXTENSIONS YES
)
add_test(
    NAME cg_stdlib_unit
    COMMAND cg_stdlib_unit
)

add_executable(cg_geometry_unit
    ${PROJECT_SOURCE_DIR}/atom.c
    ${PROJECT_SOURCE_DIR}/cg_geometry.c
    ${PROJECT_SOURCE_DIR}/cg_numeric.c
    ${PROJECT_SOURCE_DIR}/cg_types.c
    ${PROJECT_SOURCE_DIR}/cg_stdlib.c
    ${PROJECT_SOURCE_DIR}/memory.c
    ${PROJECT_SOURCE_DIR}/symbols.c
    cg_geometry_test.c
)
target_include_directories(cg_geometry_unit PRIVATE
    ${PROJECT_SOURCE_DIR}
)
set_target_properties(cg_geometry_unit PROPERTIES
    C_STANDARD 90
    C_STANDARD_REQUIRED YES
    C_EXTENSIONS YES
)
add_test(
    NAME cg_geometry_unit
    COMMAND cg_geometry_unit
)

add_executable(cg_ir_unit
    ${PROJECT_SOURCE_DIR}/atom.c
    ${PROJECT_SOURCE_DIR}/cg_ir.c
    ${PROJECT_SOURCE_DIR}/cg_ir_lower.c
    ${PROJECT_SOURCE_DIR}/cg_ir_print.c
    ${PROJECT_SOURCE_DIR}/cg_ir_verify.c
    ${PROJECT_SOURCE_DIR}/cg_numeric.c
    ${PROJECT_SOURCE_DIR}/cg_reach.c
    ${PROJECT_SOURCE_DIR}/cg_types.c
    ${PROJECT_SOURCE_DIR}/cg_stdlib.c
    ${PROJECT_SOURCE_DIR}/memory.c
    ${PROJECT_SOURCE_DIR}/symbols.c
    cg_ir_test.c
)
target_include_directories(cg_ir_unit PRIVATE
    ${PROJECT_SOURCE_DIR}
)
# The unit suite hand-builds invalid modules to exercise the verifier's
# controlled failure paths; CG_IR_TEST_SEAMS lets CgIRFail record its
# diagnostic in assertion-enabled builds instead of aborting.  Compiler
# binaries keep the loud abort for lowered-module invariant breaks.
target_compile_definitions(cg_ir_unit PRIVATE
    CG_IR_TEST_SEAMS=1
)
set_target_properties(cg_ir_unit PROPERTIES
    C_STANDARD 90
    C_STANDARD_REQUIRED YES
    C_EXTENSIONS YES
)
add_test(
    NAME cg_ir_unit
    COMMAND cg_ir_unit
)

add_test(
    NAME cg_ir_assertions_active
    COMMAND ${CMAKE_COMMAND}
        -DUNIT=$<TARGET_FILE:cg_ir_unit>
        -P ${CMAKE_CURRENT_SOURCE_DIR}/check_assertions_active.cmake
)

add_generic_regression(position position.cg
    fbd55194fb43822d993f60915df94f7a0627fd9bea27013a8c2d1d0ccdfe7b8e)
add_generic_regression(reflection reflection.cg
    54c9eafeef857ed7d520e5e487293e0ff623e6463363edced7bde9877c5b159e)
add_generic_regression(vertexlight vertexlight.cg
    a973a0f2820dd1777d0e2cdb593a93f1e0c6d3f5f8640b2017724e37f8fd3c81)
# Hashes updated by "Parse Cg 2.0 type keywords": the new fixed keyword
# atoms shift dynamic atom IDs, which reorders top-level declarations in
# generic output.  Content is unchanged (verified equal modulo declaration
# order against pre-change builds).
# Hash updated by "Model Cg 2.0 sampler types": interning the six sampler
# type-name atoms during symbol-table initialization shifts dynamic atom
# IDs, which reorders top-level declarations in generic output.  Content
# is unchanged (verified byte-equal modulo declaration order against the
# pre-change build).
# Hash updated by "Add the Cg 2.0 intrinsic catalog": the FUN_BUILTIN_OP
# opcode was renamed FUN_INTRINSIC_OP, so the node dump for the one
# intrinsic call in reflection.cg now prints "intrinsic" instead of
# "builtin".  No other output content changed.
# These regressions run with explicit -version 1.1 (see check_generic.cmake):
# their hashes pin the historical generic tree dump, byte-identical between
# default-language and 1.1 compiles for these legacy inputs, while Cg 2.0
# emission switched to the normalized IR printer ("Emit normalized Cg IR
# from generic").
# Hashes updated by "Parse geometry topology modifiers": interning the
# eight topology-modifier keyword atoms shifts dynamic atom IDs, which
# reorders top-level declarations in generic output.  Verified against
# pre-change builds: outputs are identical as line multisets modulo the
# stripped banner, i.e. declaration order moved and no content changed.
add_generic_regression(vertexlight4 vertexlight4.cg
    5454876aedd830f1d07b65e26cd51e7570035ee01d7d4b784ba4769632e31961)
add_generic_regression(loop_jumps tests/glsl/expressions/vp_jumps.cg
    07c60ea6ceaeba6c3cd85020e9088d23293179983fc7cd12ca51b306025ce5f4)
add_generic_regression(direct_discard tests/glsl/diagnostics/vp_discard.cg
    0a03690bd59c1ed3ed7f8799845b4468157ff57743c72bb2bf0cd6d54345bea1)
add_generic_regression(helper_discard
    tests/glsl/diagnostics/generic_helper_discard.cg
    4afadd12ac3af8acdb484c0e26a854dfa0fb7f9e3d8d31d64bf6ebf37e4cf058)
add_generic_diagnostic(generic_matrix_scalar2_baseline
    tests/glsl/diagnostics/generic_matrix_scalar2.cg 1035 9
    "assignment.of.incompatible.types")
add_generic_diagnostic(generic_matrix_rows_baseline
    tests/glsl/diagnostics/generic_matrix_rows.cg 5503 9
    "non-scalar.not.allowed")
add_generic_diagnostic(generic_matrix_scalar3_baseline
    tests/glsl/diagnostics/generic_matrix_scalar3.cg 5018 11
    "length.of.constructed.vectors")
add_generic_diagnostic(generic_matrix_bool_baseline
    tests/glsl/diagnostics/vp_matrix_bool_constructor.cg 1035 9
    "assignment.of.incompatible.types")
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

add_glsl_fixture(glslf_passthrough glslf
    tests/glsl/profile/fp_passthrough.cg
    tests/glsl/profile/fp_passthrough.expected)

add_glsl_fixture(glslv_scalar_return glslv
    tests/glsl/profile/vp_scalar_return.cg
    tests/glsl/profile/vp_scalar_return.expected)

add_glsl_fixture(glslf_scalar_return glslf
    tests/glsl/profile/fp_scalar_return.cg
    tests/glsl/profile/fp_scalar_return.expected)

add_glsl_fixture(glslf_control glslf
    tests/glsl/expressions/fp_control.cg
    tests/glsl/expressions/fp_control.expected)

add_glsl_fixture(glslf_builtins glslf
    tests/glsl/semantics/fp_builtins.cg
    tests/glsl/semantics/fp_builtins.expected)

add_glsl_fixture(glslf_interfaces glslf
    tests/glsl/semantics/fp_interfaces.cg
    tests/glsl/semantics/fp_interfaces.expected)

add_glsl_fixture(glslf_tex2d glslf
    tests/glsl/textures/fp_tex2d.cg
    tests/glsl/textures/fp_tex2d.expected)

add_glsl_fixture(glslf_texcube glslf
    tests/glsl/textures/fp_texcube.cg
    tests/glsl/textures/fp_texcube.expected)

add_glsl_fixture(glslf_tex1d glslf
    tests/glsl/textures/fp_tex1d.cg
    tests/glsl/textures/fp_tex1d.expected)

add_glsl_fixture(glslf_tex3d glslf
    tests/glsl/textures/fp_tex3d.cg
    tests/glsl/textures/fp_tex3d.expected)

add_glsl_fixture(glslf_two_samplers glslf
    tests/glsl/textures/fp_two_samplers.cg
    tests/glsl/textures/fp_two_samplers.expected)

add_glsl_fixture(glslf_same_line_sampler_params glslf
    tests/glsl/textures/fp_same_line_sampler_params.cg
    tests/glsl/textures/fp_same_line_sampler_params.expected)

add_glsl_fixture(glslf_same_line_global_samplers glslf
    tests/glsl/textures/fp_same_line_global_samplers.cg
    tests/glsl/textures/fp_same_line_global_samplers.expected)

add_glsl_fixture(glslf_helper_sampler glslf
    tests/glsl/textures/fp_helper_sampler.cg
    tests/glsl/textures/fp_helper_sampler.expected)

add_glsl_fixture(glslf_user_tex_helper glslf
    tests/glsl/textures/fp_user_tex_helper.cg
    tests/glsl/textures/fp_user_tex_helper.expected)

add_glsl_fixture(glslf_sampler_numeric_limit glslf
    tests/glsl/textures/fp_sampler_numeric_limit.cg
    tests/glsl/textures/fp_sampler_numeric_limit.expected)

add_glsl_profile_failure(glslf_third_sampler glslf
    tests/glsl/textures/fp_third_sampler.cg 6207 5
    "3.used,.2.available")

add_glsl_profile_failure(glslv_sampler glslv
    tests/glsl/textures/vp_sampler.cg 6202 7
    "texture.sampling")

# Sampler pins updated by "Lower Cg IR to GLSL 1.10" (Task 19): the GLSL
# pid-gate around the Cg 2.0 language sampler rules was retired, so these
# rejections now surface at the language layer with the shared 1150-1154
# families instead of the lowering-only 6208 family.  Lines shift where
# the language rule anchors at the declaration/return rather than the
# first use.  Content each test pins is unchanged: one diagnostic, same
# rejected program.  These registrations run through add_cg20_failure so
# the conformance-manifest cross-check can cite them by name.

add_cg20_failure(glslf_local_sampler
    -DCGC=$<TARGET_FILE:cgc> -DPROFILE=glslf
    -DSOURCE=${PROJECT_SOURCE_DIR}/tests/glsl/textures/fp_local_sampler.cg
    "-DCONFIG=$<CONFIG>"
    "-DACTUAL=${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/glslf_local_sampler.glsl"
    -DCODE=1151 -DEXPECTED_LINE=6
    "-DREASON=global.uniforms.or.function.parameters"
    -P ${CMAKE_CURRENT_SOURCE_DIR}/check_glsl_failure.cmake)

add_cg20_failure(glslf_sampler_array
    -DCGC=$<TARGET_FILE:cgc> -DPROFILE=glslf
    -DSOURCE=${PROJECT_SOURCE_DIR}/tests/glsl/textures/fp_sampler_array.cg
    "-DCONFIG=$<CONFIG>"
    "-DACTUAL=${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/glslf_sampler_array.glsl"
    -DCODE=1151 -DEXPECTED_LINE=3
    "-DREASON=global.uniforms.or.function.parameters"
    -P ${CMAKE_CURRENT_SOURCE_DIR}/check_glsl_failure.cmake)

add_cg20_failure(glslf_sampler_default
    -DCGC=$<TARGET_FILE:cgc> -DPROFILE=glslf
    -DSOURCE=${PROJECT_SOURCE_DIR}/tests/glsl/textures/fp_sampler_default.cg
    "-DCONFIG=$<CONFIG>"
    "-DACTUAL=${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/glslf_sampler_default.glsl"
    -DCODE=1056 -DEXPECTED_LINE=3
    "-DREASON=invalid.initialization"
    -P ${CMAKE_CURRENT_SOURCE_DIR}/check_glsl_failure.cmake)

add_cg20_failure(glslf_sampler_helper_return
    -DCGC=$<TARGET_FILE:cgc> -DPROFILE=glslf
    -DSOURCE=${PROJECT_SOURCE_DIR}/tests/glsl/textures/fp_sampler_helper_return.cg
    "-DCONFIG=$<CONFIG>"
    "-DACTUAL=${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/glslf_sampler_helper_return.glsl"
    -DCODE=1152 -DEXPECTED_LINE=5
    "-DREASON=cannot.return.a.sampler.value"
    -P ${CMAKE_CURRENT_SOURCE_DIR}/check_glsl_failure.cmake)

add_cg20_failure(glslf_sampler_conditional
    -DCGC=$<TARGET_FILE:cgc> -DPROFILE=glslf
    -DSOURCE=${PROJECT_SOURCE_DIR}/tests/glsl/textures/fp_sampler_conditional.cg
    "-DCONFIG=$<CONFIG>"
    "-DACTUAL=${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/glslf_sampler_conditional.glsl"
    -DCODE=1154 -DEXPECTED_LINE=7
    "-DREASON=samplers.are.not.allowed.in.conditional.selection"
    -P ${CMAKE_CURRENT_SOURCE_DIR}/check_glsl_failure.cmake)

add_cg20_failure(glslf_sampler_assignment
    -DCGC=$<TARGET_FILE:cgc> -DPROFILE=glslf
    -DSOURCE=${PROJECT_SOURCE_DIR}/tests/glsl/textures/fp_sampler_assignment.cg
    "-DCONFIG=$<CONFIG>"
    "-DACTUAL=${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/glslf_sampler_assignment.glsl"
    -DCODE=1150 -DEXPECTED_LINE=7
    "-DREASON=assignment.of.sampler.values.is.not.allowed"
    -P ${CMAKE_CURRENT_SOURCE_DIR}/check_glsl_failure.cmake)

add_cg20_failure(glslf_sampler_cast
    -DCGC=$<TARGET_FILE:cgc> -DPROFILE=glslf
    -DSOURCE=${PROJECT_SOURCE_DIR}/tests/glsl/textures/fp_sampler_cast.cg
    "-DCONFIG=$<CONFIG>"
    "-DACTUAL=${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/glslf_sampler_cast.glsl"
    -DCODE=1033 -DEXPECTED_LINE=5
    "-DREASON=cast.not.allowed"
    -P ${CMAKE_CURRENT_SOURCE_DIR}/check_glsl_failure.cmake)

add_cg20_failure(glslf_sampler_constructor
    -DCGC=$<TARGET_FILE:cgc> -DPROFILE=glslf
    -DSOURCE=${PROJECT_SOURCE_DIR}/tests/glsl/textures/fp_sampler_constructor.cg
    "-DCONFIG=$<CONFIG>"
    "-DACTUAL=${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/glslf_sampler_constructor.glsl"
    -DCODE=5502 -DEXPECTED_LINE=5
    "-DREASON=requires.numeric.or.bool.elements"
    -P ${CMAKE_CURRENT_SOURCE_DIR}/check_glsl_failure.cmake)

add_cg20_failure(glslf_sampler_comparison
    -DCGC=$<TARGET_FILE:cgc> -DPROFILE=glslf
    -DSOURCE=${PROJECT_SOURCE_DIR}/tests/glsl/textures/fp_sampler_comparison.cg
    "-DCONFIG=$<CONFIG>"
    "-DACTUAL=${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/glslf_sampler_comparison.glsl"
    -DCODE=1022 -DEXPECTED_LINE=8
    "-DREASON=must.be.numeric"
    -P ${CMAKE_CURRENT_SOURCE_DIR}/check_glsl_failure.cmake)

# SAMPLER-HELPER-PARAM-CAP: the glslf backend caps sampler-typed
# helper formals to plain uniform declarations; the language layer
# itself permits them, witnessed by the generic success twin
# cg20_sampler_helper_parameter.  This legacy GLSL fixture cannot be
# its own twin because generic's output-only TEXCOORD binding rejects
# its entry shape first (C5108), for reasons unrelated to samplers.

add_cg20_failure(glslf_sampler_helper_parameter
    -DCGC=$<TARGET_FILE:cgc> -DPROFILE=glslf
    -DSOURCE=${PROJECT_SOURCE_DIR}/tests/glsl/textures/fp_sampler_helper_parameter.cg
    "-DCONFIG=$<CONFIG>"
    "-DACTUAL=${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/glslf_sampler_helper_parameter.glsl"
    -DCODE=6208 -DEXPECTED_LINE=1
    "-DREASON=samplers.must.be.uniforms"
    -P ${CMAKE_CURRENT_SOURCE_DIR}/check_glsl_failure.cmake)

add_cg20_failure(glslf_sampler_struct
    -DCGC=$<TARGET_FILE:cgc> -DPROFILE=glslf
    -DSOURCE=${PROJECT_SOURCE_DIR}/tests/glsl/textures/fp_sampler_struct.cg
    "-DCONFIG=$<CONFIG>"
    "-DACTUAL=${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/glslf_sampler_struct.glsl"
    -DCODE=1151 -DEXPECTED_LINE=1
    "-DREASON=global.uniforms.or.function.parameters"
    -P ${CMAKE_CURRENT_SOURCE_DIR}/check_glsl_failure.cmake)

# Conformance-manifest cross-check (Task 20): every non-out-of-scope
# row of conformance.csv must name one CTest registered through
# add_cg20_success/add_cg20_failure (or one unit binary), requirement
# ids must be unique, and cg_stdlib.def must keep assigning unique
# intrinsic opcodes.  Registered after the LAST add_cg20_* call above;
# any future add_cg20_* registration must stay above this point or be
# folded into one of the helpers so the snapshot stays complete.

# A few manifest rows answer their rule with a pre-existing suite test
# that predates the add_cg20_* helpers: the language, type, overload,
# and stdlib unit binaries, the historical generic golden for
# position.cg (the -version 1.1 opt-in row), and the two GLSL golden
# passthrough fixtures pinning mandatory vertex/fragment outputs.
# Record them so the cross-check sees one witness per cited row.

foreach(cg20_external_test
        cg_language_unit cg_types_unit cg_overload_unit cg_stdlib_unit
        cg_geometry_unit generic_position glslv_passthrough glslf_passthrough)
    set_property(GLOBAL APPEND PROPERTY CG20_TEST_NAMES "${cg20_external_test}")
endforeach()

get_property(cg20_test_names GLOBAL PROPERTY CG20_TEST_NAMES)
add_test(
    NAME cg20_manifest
    COMMAND ${CMAKE_COMMAND}
        -DMANIFEST=${CMAKE_CURRENT_SOURCE_DIR}/cg20/conformance.csv
        "-DTEST_NAMES=${cg20_test_names}"
        -DPIN_CATALOG=${PROJECT_SOURCE_DIR}/cg_stdlib.def
        -P ${CMAKE_CURRENT_SOURCE_DIR}/check_cg20_manifest.cmake
)

add_glsl_profile_failure(glslf_invalid_texture_coord glslf
    tests/glsl/textures/fp_invalid_coord.cg 1102 6
    "incompatible.type.for.parameter.#2")

add_glsl_profile_failure(glslf_texture_bias glslf
    tests/glsl/textures/fp_texture_bias.cg 1104 6
    "too.many.parameters")

add_glsl_fixture(glslv_reused_interface glslv
    tests/glsl/semantics/vp_reused_interface.cg
    tests/glsl/semantics/vp_reused_interface.expected)

add_glsl_fixture(glslf_helper_discard glslf
    tests/glsl/expressions/fp_helper_discard.cg
    tests/glsl/expressions/fp_helper_discard.expected)

add_glsl_fixture(glslf_vector_discard glslf
    tests/glsl/expressions/fp_vector_discard.cg
    tests/glsl/expressions/fp_vector_discard.expected)

add_glsl_fixture(glslv_early_return glslv
    tests/glsl/expressions/vp_early_return.cg
    tests/glsl/expressions/vp_early_return.expected)

add_glsl_fixture(glslf_early_return glslf
    tests/glsl/expressions/fp_early_return.cg
    tests/glsl/expressions/fp_early_return.expected)

add_glsl_fixture(glslv_out_returns glslv
    tests/glsl/expressions/vp_out_returns.cg
    tests/glsl/expressions/vp_out_returns.expected)

add_glsl_fixture(glslf_out_returns glslf
    tests/glsl/expressions/fp_out_returns.cg
    tests/glsl/expressions/fp_out_returns.expected)

add_glsl_fixture(glslv_return_once glslv
    tests/glsl/expressions/vp_return_once.cg
    tests/glsl/expressions/vp_return_once.expected)

add_glsl_fixture(glslf_return_once glslf
    tests/glsl/expressions/fp_return_once.cg
    tests/glsl/expressions/fp_return_once.expected)

add_glsl_fixture(glslf_return_pure_once glslf
    tests/glsl/expressions/fp_return_pure_once.cg
    tests/glsl/expressions/fp_return_pure_once.expected)

add_glsl_fixture(glslv_struct_array_assignment glslv
    tests/glsl/expressions/vp_struct_array_assignment.cg
    tests/glsl/expressions/vp_struct_array_assignment.expected)

add_glsl_fixture(glslv_nested_struct_array_assignment glslv
    tests/glsl/expressions/vp_nested_struct_array_assignment.cg
    tests/glsl/expressions/vp_nested_struct_array_assignment.expected)

add_glsl_fixture(glslv_struct_rhs_once glslv
    tests/glsl/expressions/vp_struct_rhs_once.cg
    tests/glsl/expressions/vp_struct_rhs_once.expected)

add_glsl_fixture(glslv_struct_indices_once glslv
    tests/glsl/expressions/vp_struct_indices_once.cg
    tests/glsl/expressions/vp_struct_indices_once.expected)

add_glsl_fixture(glslv_linked_varyings glslv
    tests/glsl/semantics/vp_linked_varyings.cg
    tests/glsl/semantics/vp_linked_varyings.expected)

add_glsl_fixture(glslf_linked_varyings glslf
    tests/glsl/semantics/fp_linked_varyings.cg
    tests/glsl/semantics/fp_linked_varyings.expected)

add_test(
    NAME glsl_linked_varyings
    COMMAND ${CMAKE_COMMAND}
        -DCGC=$<TARGET_FILE:cgc>
        "-DCONFIG=$<CONFIG>"
        "-DGLSLANG_VALIDATOR=${GLSLANG_VALIDATOR}"
        -DVERTEX_SOURCE=${PROJECT_SOURCE_DIR}/tests/glsl/semantics/vp_linked_varyings.cg
        -DFRAGMENT_SOURCE=${PROJECT_SOURCE_DIR}/tests/glsl/semantics/fp_linked_varyings.cg
        "-DVERTEX_ACTUAL=${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/glsl_linked_varyings.vert"
        "-DFRAGMENT_ACTUAL=${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/glsl_linked_varyings.frag"
        -P ${CMAKE_CURRENT_SOURCE_DIR}/check_glsl_link.cmake
)

add_test(
    NAME glsl_linked_varyings_without_validator
    COMMAND ${CMAKE_COMMAND}
        -DCGC=$<TARGET_FILE:cgc>
        "-DCONFIG=$<CONFIG>"
        -DGLSLANG_VALIDATOR=GLSLANG_VALIDATOR-NOTFOUND
        -DVERTEX_SOURCE=${PROJECT_SOURCE_DIR}/tests/glsl/semantics/vp_linked_varyings.cg
        -DFRAGMENT_SOURCE=${PROJECT_SOURCE_DIR}/tests/glsl/semantics/fp_linked_varyings.cg
        "-DVERTEX_ACTUAL=${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/glsl_linked_varyings_without_validator.vert"
        "-DFRAGMENT_ACTUAL=${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/glsl_linked_varyings_without_validator.frag"
        -P ${CMAKE_CURRENT_SOURCE_DIR}/check_glsl_link.cmake
)

add_test(
    NAME glsl_link_interface
    COMMAND ${CMAKE_COMMAND}
        -DCGC=$<TARGET_FILE:cgc>
        "-DGLSLANG_VALIDATOR=${GLSLANG_VALIDATOR}"
        "-DCONFIG=$<CONFIG>"
        -DVERTEX_SOURCE=${PROJECT_SOURCE_DIR}/tests/glsl/link/vp_link.cg
        -DFRAGMENT_SOURCE=${PROJECT_SOURCE_DIR}/tests/glsl/link/fp_link.cg
        "-DVERTEX_OUTPUT=${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/glsl_link_interface.vert"
        "-DFRAGMENT_OUTPUT=${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/glsl_link_interface.frag"
        -P ${CMAKE_CURRENT_SOURCE_DIR}/check_glsl_interface.cmake
)
add_glsl_validation(glsl_link_vertex glslv vert
    tests/glsl/link/vp_link.cg)
add_glsl_validation(glsl_link_fragment glslf frag
    tests/glsl/link/fp_link.cg)

add_test(
    NAME glsl_validator_failure_propagates
    COMMAND ${CMAKE_COMMAND}
        -DCMAKE_COMMAND=${CMAKE_COMMAND}
        -DCGC=$<TARGET_FILE:cgc>
        -DSOURCE=${PROJECT_SOURCE_DIR}/tests/glsl/profile/vp_passthrough.cg
        "-DCONFIG=$<CONFIG>"
        "-DOUTPUT=${CMAKE_CURRENT_BINARY_DIR}/validator-failure/$<CONFIG>/probe.glsl"
        -DVALIDATE_SCRIPT=${CMAKE_CURRENT_SOURCE_DIR}/validate_glsl.cmake
        -P ${CMAKE_CURRENT_SOURCE_DIR}/check_validate_glsl_failure.cmake
)

add_glsl_fixture(glslf_uniform_components_64 glslf
    tests/glsl/limits/fp_uniform_components_64.cg
    tests/glsl/limits/fp_uniform_components_64.expected)

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

add_glsl_fixture(glslv_matrix_constructors glslv
    tests/glsl/expressions/vp_matrix_constructors.cg
    tests/glsl/expressions/vp_matrix_constructors.expected)

add_glsl_fixture(glslv_matrix_side_effects glslv
    tests/glsl/expressions/vp_matrix_side_effects.cg
    tests/glsl/expressions/vp_matrix_side_effects.expected)

add_glsl_fixture(glslv_matrix_numeric_args glslv
    tests/glsl/expressions/vp_matrix_numeric_args.cg
    tests/glsl/expressions/vp_matrix_numeric_args.expected)

add_glsl_fixture(glslv_matrix_selectors_once glslv
    tests/glsl/expressions/vp_matrix_selectors_once.cg
    tests/glsl/expressions/vp_matrix_selectors_once.expected)

# Explicit elementwise matrix stores (fix round 1): mixed-coordinate
# adjacent stores lower independently with their own coordinates, and a
# sixteen-store fill passes the four-component producer group cap.
add_glsl_fixture(glslv_matrix_element_stores glslv
    tests/glsl/expressions/vp_matrix_element_stores.cg
    tests/glsl/expressions/vp_matrix_element_stores.expected)

add_glsl_fixture(glslf_matrix_element_stores glslf
    tests/glsl/expressions/fp_matrix_element_stores.cg
    tests/glsl/expressions/fp_matrix_element_stores.expected)

add_glsl_fixture(glslv_aggregate_defaults glslv
    tests/glsl/uniforms/vp_aggregate_defaults.cg
    tests/glsl/uniforms/vp_aggregate_defaults.expected)

add_glsl_fixture(glslv_aggregate_default_collision glslv
    tests/glsl/uniforms/vp_aggregate_default_collision.cg
    tests/glsl/uniforms/vp_aggregate_default_collision.expected)

add_glsl_fixture(glslv_aggregate_default_conversions glslv
    tests/glsl/uniforms/vp_aggregate_default_conversions.cg
    tests/glsl/uniforms/vp_aggregate_default_conversions.expected)

add_glsl_fixture(glslv_uniform_components_512 glslv
    tests/glsl/limits/vp_uniform_components_512.cg
    tests/glsl/limits/vp_uniform_components_512.expected)

add_glsl_fixture(glslv_uniform_struct_global glslv
    tests/glsl/uniforms/vp_uniform_struct_global.cg
    tests/glsl/uniforms/vp_uniform_struct_global.expected)

add_glsl_fixture(glslv_uniform_struct_param glslv
    tests/glsl/uniforms/vp_uniform_struct_param.cg
    tests/glsl/uniforms/vp_uniform_struct_param.expected)

add_glsl_fixture(glslv_uniform_struct_nested glslv
    tests/glsl/uniforms/vp_uniform_struct_nested.cg
    tests/glsl/uniforms/vp_uniform_struct_nested.expected)

# Expected output updated by "Add the Cg 2.0 intrinsic catalog": interning
# the standard-library catalog atoms during symbol-table initialization
# shifts dynamic atom IDs, and symbol trees order by bit-reversed atom ID,
# so the two colliding cg_gl_Data members swap positions in the golden.
# Content is unchanged: the emitted declaration/usage multisets are equal
# to the pre-change golden modulo that ordering (verified against an
# af70094 baseline build).
# Updated again by "Gate matrix write fallback on producer markers" (merge
# of opengl-arb-profiles): ARB profile registration interns further atoms
# at startup, shifting the same tie-break once more.  Re-verified as
# order-only: member declarations swap and the commutative addition swaps
# operands with it; token multiset equality proven against db3d1a6.
# Updated again by "Parse geometry topology modifiers": the eight new
# keyword atoms shift the same tie-break; re-verified as order-only
# against the pre-change build with the identical member/addition swap.
add_glsl_fixture(glslv_uniform_struct_collision glslv
    tests/glsl/uniforms/vp_uniform_struct_collision.cg
    tests/glsl/uniforms/vp_uniform_struct_collision.expected)

add_glsl_fixture(glslv_uniform_struct_components_512 glslv
    tests/glsl/limits/vp_uniform_struct_components_512.cg
    tests/glsl/limits/vp_uniform_struct_components_512.expected)

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

add_glsl_diagnostic(glslv_missing_return_semantic glslv
    tests/glsl/diagnostics/vp_missing_return_semantic.cg 5121 1
    "must.declare.a.semantic.on.its.return.value")
add_glsl_diagnostic(glslv_non_square_diagnostic glslv
    tests/glsl/diagnostics/vp_non_square.cg 6209 2
    "square.matrix,.found.*float4x3")
add_glsl_diagnostic(glslv_texture_diagnostic glslv
    tests/glsl/diagnostics/vp_texture.cg 6202 7
    "glslv.profile.does.not.support.operation.*texture.sampling")
add_glsl_diagnostic(glslv_bitwise_diagnostic glslv
    tests/glsl/diagnostics/vp_bitwise.cg 6201 6
    "does.not.support.operation.*bitwise.and")
add_test(
    NAME glsl_diagnostic_cleans_normalized_output
    COMMAND ${CMAKE_COMMAND}
        -DCMAKE_COMMAND=${CMAKE_COMMAND}
        -DCGC=$<TARGET_FILE:cgc>
        -DSOURCE=${PROJECT_SOURCE_DIR}/tests/glsl/diagnostics/vp_bitwise.cg
        "-DOUTPUT=${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/diagnostic-cleanup.glsl"
        -DDIAGNOSTIC_SCRIPT=${CMAKE_CURRENT_SOURCE_DIR}/check_diagnostic.cmake
        -P ${CMAKE_CURRENT_SOURCE_DIR}/check_diagnostic_cleanup.cmake
)
add_glsl_diagnostic(glslf_color1_diagnostic glslf
    tests/glsl/diagnostics/fp_color1.cg 6203 1
    "cannot.bind.semantic.*COLOR1")
add_glsl_diagnostic(glslv_duplicate_semantic_diagnostic glslv
    tests/glsl/diagnostics/vp_duplicate_semantic.cg 6204 3
    "interface.conflicts.at.semantic.*ATTRIB0")
add_glsl_diagnostic(glslv_attributes_limit glslv
    tests/glsl/limits/vp_attributes17.cg 6207 18
    "attribute.*17.used,.16.available")
add_glsl_diagnostic(glslv_varyings_limit glslv
    tests/glsl/limits/vp_varyings36.cg 6207 11
    "varying.*36.used,.32.available")
add_glsl_diagnostic(glslf_uniforms_limit glslf
    tests/glsl/limits/fp_uniforms68.cg 6207 2
    "fragment.uniform.components.*68.used,.64.available")
add_glsl_diagnostic(glslf_samplers_limit glslf
    tests/glsl/limits/fp_samplers3.cg 6207 5
    "fragment.texture.units.*3.used,.2.available")
add_glsl_diagnostic(glslv_intrinsic_diagnostic glslv
    tests/glsl/diagnostics/vp_bad_intrinsic.cg 6206 1
    "no.exact.intrinsic.*rsqrt")
add_glsl_diagnostic(glslv_type_loc_local glslv
    tests/glsl/diagnostics/vp_type_loc_local.cg 6209 5
    "square.matrix,.found.*float2x3")
add_glsl_diagnostic(glslv_type_loc_local_unsupported glslv
    tests/glsl/diagnostics/vp_type_loc_local_unsupported.cg 6200 5
    "does.not.support.type.*matrix")
add_glsl_diagnostic(glslv_type_loc_entry_uniform glslv
    tests/glsl/diagnostics/vp_type_loc_entry_uniform.cg 6209 4
    "square.matrix,.found.*float2x3")
add_glsl_diagnostic(glslv_type_loc_global_uniform glslv
    tests/glsl/diagnostics/vp_type_loc_global_uniform.cg 6209 1
    "square.matrix,.found.*float2x3")
add_glsl_diagnostic(glslv_type_loc_helper_formal glslv
    tests/glsl/diagnostics/vp_type_loc_helper_formal.cg 6209 3
    "square.matrix,.found.*float2x3")
add_glsl_diagnostic(glslv_type_loc_helper_result glslv
    tests/glsl/diagnostics/vp_type_loc_helper_result.cg 6209 1
    "square.matrix,.found.*float2x3")
add_glsl_diagnostic(glslv_type_loc_helper_local glslv
    tests/glsl/diagnostics/vp_type_loc_helper_local.cg 6209 3
    "square.matrix,.found.*float2x3")
add_glsl_diagnostic(glslv_type_loc_struct_member glslv
    tests/glsl/diagnostics/vp_type_loc_struct_member.cg 6209 2
    "square.matrix,.found.*float2x3")
add_glsl_boundary(glslv_attributes_boundary glslv
    tests/glsl/limits/vp_attributes16.cg vert)
add_glsl_boundary(glslv_varyings_boundary glslv
    tests/glsl/limits/vp_varyings32.cg vert)
add_glsl_boundary(glslv_uniforms_boundary glslv
    tests/glsl/limits/vp_uniform_components_512.cg vert)
add_glsl_boundary(glslf_uniforms_boundary glslf
    tests/glsl/limits/fp_uniform_components_64.cg frag)
add_glsl_boundary(glslf_samplers_boundary glslf
    tests/glsl/textures/fp_two_samplers.cg frag)
add_glsl_boundary(glslf_color_boundary glslf
    tests/glsl/profile/fp_passthrough.cg frag)

add_glsl_failure(glslv_unsupported_lowering
    tests/glsl/profile/vp_unsupported_operator.cg 9 remainder)
add_glsl_profile_failure(glslv_discard glslv
    tests/glsl/diagnostics/vp_discard.cg 6202 8 discard)
add_glsl_profile_failure(glslv_impure_array_aggregate glslv
    tests/glsl/diagnostics/vp_impure_array_aggregate.cg 6201 28
    "side-effecting.aggregate.with.arrays")
# Fix round 2: a producer group write (`_m` selector stores with
# selectorRead-marked targets) followed by any fifth consecutive
# matching-shape element store cannot fall back to elementwise
# lowering -- each whole-vector value would print into one scalar
# component target -- so the count cap keeps the loud failure.
add_glsl_profile_failure(glslv_matrix_group_write_cap glslv
    tests/glsl/diagnostics/vp_matrix_group_write_cap.cg 6201 13
    "GLSL.1.10.program")
add_glsl_profile_failure(glslv_builtin_position_width glslv
    tests/glsl/diagnostics/vp_builtin_position_width.cg 6203 2 POSITION)
add_glsl_profile_failure(glslf_builtin_position_width glslf
    tests/glsl/diagnostics/fp_builtin_position_width.cg 6203 5 WPOS)
add_glsl_profile_failure(glslf_builtin_color_width glslf
    tests/glsl/diagnostics/fp_builtin_color_width.cg 6203 2 COLOR)
add_glsl_profile_failure(glslf_builtin_depth_vector glslf
    tests/glsl/diagnostics/fp_builtin_depth_vector.cg 6203 2 DEPTH)
add_glsl_profile_failure(glslv_duplicate_input glslv
    tests/glsl/diagnostics/vp_duplicate_input.cg 6204 6
    "interface.conflicts")
add_glsl_profile_failure(glslv_duplicate_output glslv
    tests/glsl/diagnostics/vp_duplicate_output.cg 6204 3
    "interface.conflicts")
add_glsl_profile_failure(glslf_duplicate_input glslf
    tests/glsl/diagnostics/fp_duplicate_input.cg 6204 6
    "interface.conflicts")
add_glsl_profile_failure(glslf_duplicate_builtin_input glslf
    tests/glsl/diagnostics/fp_duplicate_builtin_input.cg 6204 6
    "interface.conflicts")
add_glsl_profile_failure(glslf_duplicate_output glslf
    tests/glsl/diagnostics/fp_duplicate_output.cg 6204 3
    "interface.conflicts")
add_glsl_profile_failure(glslf_numeric_discard glslf
    tests/glsl/diagnostics/fp_numeric_discard.cg 1018 8
    "Boolean.expression.expected")
add_glsl_profile_failure(glslv_duplicate_exact_input_same glslv
    tests/glsl/diagnostics/vp_duplicate_exact_input_same.cg 6204 6
    "interface.conflicts")
add_glsl_profile_failure(glslv_duplicate_exact_input_type glslv
    tests/glsl/diagnostics/vp_duplicate_exact_input_type.cg 6204 6
    "interface.conflicts")
# Exact-duplicate outputs moved from the GLSL interface-conflict check to
# the language-level output-alias rule ("Enforce Cg program interface
# semantics"): same canonical semantic, so the second declaration is
# rejected before any profile lowering.
add_glsl_profile_failure(glslv_duplicate_exact_output glslv
    tests/glsl/diagnostics/vp_duplicate_exact_output.cg 5122 4
    "outputs.conflict.at.semantic")
add_glsl_profile_failure(glslf_duplicate_exact_input_same glslf
    tests/glsl/diagnostics/fp_duplicate_exact_input_same.cg 6204 6
    "interface.conflicts")
add_glsl_profile_failure(glslf_duplicate_exact_input_type glslf
    tests/glsl/diagnostics/fp_duplicate_exact_input_type.cg 6204 6
    "interface.conflicts")
add_glsl_profile_failure(glslf_duplicate_exact_output glslf
    tests/glsl/diagnostics/fp_duplicate_exact_output.cg 5122 3
    "outputs.conflict.at.semantic")
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
add_glsl_profile_failure(glslv_nonsquare_matrix glslv
    tests/glsl/diagnostics/vp_nonsquare_matrix.cg 6209 6 "square.matrix")
add_glsl_profile_failure(glslv_matrix_bool_constructor glslv
    tests/glsl/diagnostics/vp_matrix_bool_constructor.cg 6200 9
    "matrix constructor argument type")
add_glsl_failure(glslv_aggregate_default_int_range
    tests/glsl/diagnostics/vp_aggregate_default_int_range.cg 6
    "uniform default conversion range")
add_test(
    NAME glslv_uniform_struct_default
    COMMAND ${CMAKE_COMMAND}
        -DCGC=$<TARGET_FILE:cgc>
        -DPROFILE=glslv
        -DSOURCE=${PROJECT_SOURCE_DIR}/tests/glsl/diagnostics/vp_uniform_struct_default.cg
        "-DCONFIG=$<CONFIG>"
        "-DACTUAL=${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/glslv_uniform_struct_default.glsl"
        -DCODE=1056
        -DEXPECTED_LINE=11
        -DREASON=invalid.initialization
        -P ${CMAKE_CURRENT_SOURCE_DIR}/check_glsl_failure.cmake
)
add_test(
    NAME glslv_uniform_components_513
    COMMAND ${CMAKE_COMMAND}
        -DCGC=$<TARGET_FILE:cgc>
        -DPROFILE=glslv
        -DSOURCE=${PROJECT_SOURCE_DIR}/tests/glsl/limits/vp_uniform_components_513.cg
        "-DCONFIG=$<CONFIG>"
        "-DACTUAL=${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/glslv_uniform_components_513.glsl"
        -DCODE=6207
        -DEXPECTED_LINE=14
        -DREASON=513.used,.512.available
        -P ${CMAKE_CURRENT_SOURCE_DIR}/check_glsl_failure.cmake
)
add_glsl_profile_failure(glslf_uniform_components_65 glslf
    tests/glsl/limits/fp_uniform_components_65.cg 6207 7
    "65.used,.64.available")
add_test(
    NAME glslv_uniform_components_516
    COMMAND ${CMAKE_COMMAND}
        -DCGC=$<TARGET_FILE:cgc>
        -DPROFILE=glslv
        -DSOURCE=${PROJECT_SOURCE_DIR}/tests/glsl/limits/vp_uniform_components_516.cg
        "-DCONFIG=$<CONFIG>"
        "-DACTUAL=${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/glslv_uniform_components_516.glsl"
        -DCODE=6207
        -DEXPECTED_LINE=6
        -DREASON=516.used,.512.available
        -P ${CMAKE_CURRENT_SOURCE_DIR}/check_glsl_failure.cmake
)
add_test(
    NAME glslv_uniform_struct_components_513
    COMMAND ${CMAKE_COMMAND}
        -DCGC=$<TARGET_FILE:cgc>
        -DPROFILE=glslv
        -DSOURCE=${PROJECT_SOURCE_DIR}/tests/glsl/limits/vp_uniform_struct_components_513.cg
        "-DCONFIG=$<CONFIG>"
        "-DACTUAL=${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/glslv_uniform_struct_components_513.glsl"
        -DCODE=6207
        -DEXPECTED_LINE=14
        -DREASON=513.used,.512.available
        -P ${CMAKE_CURRENT_SOURCE_DIR}/check_glsl_failure.cmake
)
add_test(
    NAME glslv_matrix_constructor_underfilled
    COMMAND ${CMAKE_COMMAND}
        -DCGC=$<TARGET_FILE:cgc>
        -DPROFILE=glslv
        -DSOURCE=${PROJECT_SOURCE_DIR}/tests/glsl/diagnostics/vp_matrix_constructor_underfilled.cg
        "-DCONFIG=$<CONFIG>"
        "-DACTUAL=${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/glslv_matrix_constructor_underfilled.glsl"
        -DCODE=1067
        -DEXPECTED_LINE=9
        -DREASON=too.little.data
        -P ${CMAKE_CURRENT_SOURCE_DIR}/check_glsl_failure.cmake
)
add_test(
    NAME glslv_matrix_constructor_overfilled
    COMMAND ${CMAKE_COMMAND}
        -DCGC=$<TARGET_FILE:cgc>
        -DPROFILE=glslv
        -DSOURCE=${PROJECT_SOURCE_DIR}/tests/glsl/diagnostics/vp_matrix_constructor_overfilled.cg
        "-DCONFIG=$<CONFIG>"
        "-DACTUAL=${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/glslv_matrix_constructor_overfilled.glsl"
        -DCODE=1068
        -DEXPECTED_LINE=9
        -DREASON=too.much.data
        -P ${CMAKE_CURRENT_SOURCE_DIR}/check_glsl_failure.cmake
)
add_test(
    NAME glslv_vector_constructor_overfilled
    COMMAND ${CMAKE_COMMAND}
        -DCGC=$<TARGET_FILE:cgc>
        -DPROFILE=glslv
        -DSOURCE=${PROJECT_SOURCE_DIR}/tests/glsl/diagnostics/vp_vector_constructor_overfilled.cg
        "-DCONFIG=$<CONFIG>"
        "-DACTUAL=${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/glslv_vector_constructor_overfilled.glsl"
        -DCODE=5018
        -DEXPECTED_LINE=9
        -DREASON=length.of.constructed.vectors
        -P ${CMAKE_CURRENT_SOURCE_DIR}/check_glsl_failure.cmake
)
add_test(
    NAME glslv_matrix_constructor_row_overflow
    COMMAND ${CMAKE_COMMAND}
        -DCGC=$<TARGET_FILE:cgc>
        -DPROFILE=glslv
        -DSOURCE=${PROJECT_SOURCE_DIR}/tests/glsl/diagnostics/vp_matrix_constructor_row_overflow.cg
        "-DCONFIG=$<CONFIG>"
        "-DACTUAL=${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/glslv_matrix_constructor_row_overflow.glsl"
        -DCODE=1068
        -DEXPECTED_LINE=9
        -DREASON=too.much.data
        -P ${CMAKE_CURRENT_SOURCE_DIR}/check_glsl_failure.cmake
)
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
        -DCODE=6203
        -P ${CMAKE_CURRENT_SOURCE_DIR}/glsl/profile/check_diagnostic.cmake
)

foreach(marker_case missing duplicated reversed nested prefix suffix
                    trailing_space cr_only lf crlf eof)
    add_test(
        NAME stdlib_marker_${marker_case}
        COMMAND ${CMAKE_COMMAND}
            -DTOKENIZE=$<TARGET_FILE:tokenize>
            -DINPUT=${PROJECT_SOURCE_DIR}/stdlib.cg
            -DSCRIPT=${PROJECT_SOURCE_DIR}/cmake/RegenerateStdlib.cmake
            -DCASE=${marker_case}
            "-DWORK_DIR=${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/stdlib-marker-tests"
            -P ${CMAKE_CURRENT_SOURCE_DIR}/check_stdlib_markers.cmake
    )
endforeach()

add_test(
    NAME stdlib_regeneration_reproducible
    COMMAND ${CMAKE_COMMAND}
        -DTOKENIZE=$<TARGET_FILE:tokenize>
        -DINPUT=stdlib.cg
        -DEXPECTED=${PROJECT_SOURCE_DIR}/stdlib.c
        -DSCRIPT=${PROJECT_SOURCE_DIR}/cmake/RegenerateStdlib.cmake
        -DSOURCE_DIR=${PROJECT_SOURCE_DIR}
        "-DWORK_DIR=${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/stdlib-reproducibility"
        -P ${CMAKE_CURRENT_SOURCE_DIR}/check_stdlib_regeneration.cmake
)

foreach(stream_case default valid valid_underscore valid_leading_underscore
                    leading_digit quote hyphen whitespace empty overlong)
    add_test(
        NAME tokenize_stream_name_${stream_case}
        COMMAND ${CMAKE_COMMAND}
            -DTOKENIZE=$<TARGET_FILE:tokenize>
            -DINPUT=tests/glsl/profile/empty.cg
            -DSOURCE_DIR=${PROJECT_SOURCE_DIR}
            -DCASE=${stream_case}
            -P ${CMAKE_CURRENT_SOURCE_DIR}/check_tokenize_stream_name.cmake
    )
endforeach()

function(add_arb_fixture name profile source expected)
    add_test(
        NAME ${name}
        COMMAND ${CMAKE_COMMAND}
            -DTEST_NAME=${name}
            -DCGC=$<TARGET_FILE:cgc>
            -DPROFILE=${profile}
            -DSOURCE=${PROJECT_SOURCE_DIR}/${source}
            -DEXPECT_SUCCESS=TRUE
            -DEXPECTED_OUTPUT=${PROJECT_SOURCE_DIR}/${expected}
            "-DEXTRA_ARGS=${ARGN}"
            -DWORK_DIR=${CMAKE_CURRENT_BINARY_DIR}
            -P ${CMAKE_CURRENT_SOURCE_DIR}/run_arb_test.cmake
    )
endfunction()

function(add_arb_failure name profile source diagnostics)
    add_test(
        NAME ${name}
        COMMAND ${CMAKE_COMMAND}
            -DTEST_NAME=${name}
            -DCGC=$<TARGET_FILE:cgc>
            -DPROFILE=${profile}
            -DSOURCE=${PROJECT_SOURCE_DIR}/${source}
            -DEXPECT_SUCCESS=FALSE
            -DEXPECTED_DIAGNOSTICS=${PROJECT_SOURCE_DIR}/${diagnostics}
            "-DEXTRA_ARGS=${ARGN}"
            -DWORK_DIR=${CMAKE_CURRENT_BINARY_DIR}
            -P ${CMAKE_CURRENT_SOURCE_DIR}/run_arb_test.cmake
    )
endfunction()

add_test(NAME arbvp1_registration
    COMMAND $<TARGET_FILE:cgc> -quiet -nocode -profile arbvp1
            ${PROJECT_SOURCE_DIR}/tests/arb/profile/vp_passthrough.cg)
add_test(NAME arbfp1_registration
    COMMAND $<TARGET_FILE:cgc> -quiet -nocode -profile arbfp1
            ${PROJECT_SOURCE_DIR}/tests/arb/profile/fp_passthrough.cg)

add_executable(arb_ir_tests arb_ir_tests.c ${PROJECT_SOURCE_DIR}/arb_ir.c)
target_include_directories(arb_ir_tests PRIVATE ${PROJECT_SOURCE_DIR})
set_target_properties(arb_ir_tests PROPERTIES
    C_STANDARD 90
    C_STANDARD_REQUIRED YES
    C_EXTENSIONS YES
)
add_test(NAME arb_ir_unit COMMAND arb_ir_tests)

add_arb_fixture(arbvp1_passthrough arbvp1
    tests/arb/profile/vp_passthrough.cg
    tests/arb/profile/vp_passthrough.expected)
add_test(NAME arbvp1_wgl_smoke
    COMMAND $<TARGET_FILE:arb_smoke> vp
        ${CMAKE_CURRENT_BINARY_DIR}/arbvp1_passthrough.arb)
set_tests_properties(arbvp1_passthrough PROPERTIES
    FIXTURES_SETUP arbvp1_smoke_program)
set_tests_properties(arbvp1_wgl_smoke PROPERTIES
    DEPENDS arbvp1_passthrough
    SKIP_RETURN_CODE 77)

add_arb_fixture(arbvp1_arithmetic arbvp1
    tests/arb/arithmetic/vp_arithmetic.cg
    tests/arb/arithmetic/vp_arithmetic.expected)
add_arb_fixture(arbvp1_swizzle arbvp1
    tests/arb/arithmetic/vp_swizzle.cg
    tests/arb/arithmetic/vp_swizzle.expected)

add_arb_fixture(arbvp1_matrix arbvp1
    tests/arb/uniforms/vp_matrix.cg
    tests/arb/uniforms/vp_matrix.expected)
add_arb_fixture(arbvp1_bindings arbvp1
    tests/arb/uniforms/vp_bindings.cg
    tests/arb/uniforms/vp_bindings.expected)

add_arb_fixture(arbvp1_conditional arbvp1
    tests/arb/control-flow/vp_conditional.cg
    tests/arb/control-flow/vp_conditional.expected)
add_arb_fixture(arbvp1_static_loop arbvp1
    tests/arb/control-flow/vp_static_loop.cg
    tests/arb/control-flow/vp_static_loop.expected)
add_arb_fixture(arbvp1_static_while arbvp1
    tests/arb/control-flow/vp_static_while.cg
    tests/arb/control-flow/vp_static_while.expected)
add_arb_fixture(arbvp1_static_do arbvp1
    tests/arb/control-flow/vp_static_do.cg
    tests/arb/control-flow/vp_static_do.expected)
add_arb_failure(arb_dynamic_loop arbvp1
    tests/arb/diagnostics/dynamic_loop.cg
    tests/arb/diagnostics/dynamic_loop.txt)

add_arb_fixture(arbvp1_relative_index arbvp1
    tests/arb/uniforms/vp_relative_index.cg
    tests/arb/uniforms/vp_relative_index.expected)
add_arb_fixture(arbvp1_params_96 arbvp1
    tests/arb/limits/vp_params_96.cg
    tests/arb/limits/vp_params_96.expected)
add_arb_failure(arbvp1_params_97 arbvp1
    tests/arb/limits/vp_params_97.cg
    tests/arb/limits/vp_params_97.txt)
add_arb_fixture(arbvp1_instructions_128 arbvp1
    tests/arb/limits/vp_instructions_128.cg
    tests/arb/limits/vp_instructions_128.expected)
add_arb_failure(arbvp1_instructions_129 arbvp1
    tests/arb/limits/vp_instructions_129.cg
    tests/arb/limits/vp_instructions_129.txt)

add_test(NAME arbfp1_sampler_types
    COMMAND $<TARGET_FILE:cgc> -quiet -nocode -profile arbfp1
            ${PROJECT_SOURCE_DIR}/tests/arb/profile/fp_sampler_nocode.cg)

add_arb_fixture(arbfp1_passthrough arbfp1
    tests/arb/profile/fp_passthrough.cg
    tests/arb/profile/fp_passthrough.expected)
add_arb_fixture(arbfp1_conditional arbfp1
    tests/arb/arithmetic/fp_conditional.cg
    tests/arb/arithmetic/fp_conditional.expected)

add_arb_fixture(arbfp1_tex2d arbfp1
    tests/arb/textures/fp_tex2d.cg
    tests/arb/textures/fp_tex2d.expected)
add_test(NAME arbfp1_wgl_smoke
    COMMAND $<TARGET_FILE:arb_smoke> fp
        ${CMAKE_CURRENT_BINARY_DIR}/arbfp1_tex2d.arb)
set_tests_properties(arbfp1_tex2d PROPERTIES
    FIXTURES_SETUP arbfp1_smoke_program)
set_tests_properties(arbfp1_wgl_smoke PROPERTIES
    DEPENDS arbfp1_tex2d
    SKIP_RETURN_CODE 77)
add_arb_fixture(arbfp1_target_1d arbfp1
    tests/arb/textures/fp_targets.cg
    tests/arb/textures/fp_target_1d.expected -DTEST_1D)
add_arb_fixture(arbfp1_target_3d arbfp1
    tests/arb/textures/fp_targets.cg
    tests/arb/textures/fp_target_3d.expected -DTEST_3D)
add_arb_fixture(arbfp1_target_cube arbfp1
    tests/arb/textures/fp_targets.cg
    tests/arb/textures/fp_target_cube.expected -DTEST_CUBE)
add_arb_fixture(arbfp1_target_rect arbfp1
    tests/arb/textures/fp_targets.cg
    tests/arb/textures/fp_target_rect.expected -DTEST_RECT)
add_arb_fixture(arbfp1_target_projected_biased arbfp1
    tests/arb/textures/fp_targets.cg
    tests/arb/textures/fp_target_projected_biased.expected -DTEST_PROJECTED_BIASED)
add_arb_fixture(arbfp1_dependent arbfp1
    tests/arb/textures/fp_dependent.cg
    tests/arb/textures/fp_dependent.expected)
add_arb_fixture(arbfp1_alu_dependent arbfp1
    tests/arb/textures/fp_alu_dependent.cg
    tests/arb/textures/fp_alu_dependent.expected)
add_arb_fixture(arbfp1_discard arbfp1
    tests/arb/control-flow/fp_discard.cg
    tests/arb/control-flow/fp_discard.expected)
add_arb_failure(arb_texture_conflict arbfp1
    tests/arb/diagnostics/fp_texture_conflict.cg
    tests/arb/diagnostics/fp_texture_conflict.txt)

add_arb_fixture(arbvp1_all_semantics arbvp1
    tests/arb/vp-semantics/vp_all_semantics.cg
    tests/arb/vp-semantics/vp_all_semantics_conventional.expected)
add_arb_fixture(arbvp1_all_semantics_generic arbvp1
    tests/arb/vp-semantics/vp_all_semantics.cg
    tests/arb/vp-semantics/vp_all_semantics_generic.expected -DTEST_GENERIC)
add_arb_fixture(arbfp1_all_semantics arbfp1
    tests/arb/fp-semantics/fp_all_semantics.cg
    tests/arb/fp-semantics/fp_all_semantics_primary.expected)
add_arb_fixture(arbfp1_all_semantics_secondary arbfp1
    tests/arb/fp-semantics/fp_all_semantics.cg
    tests/arb/fp-semantics/fp_all_semantics_secondary.expected -DTEST_SECONDARY)
add_arb_failure(arbvp1_missing_position arbvp1
    tests/arb/diagnostics/vp_missing_position.cg
    tests/arb/diagnostics/vp_missing_position.txt)
add_arb_failure(arbvp1_attribute_alias arbvp1
    tests/arb/diagnostics/vp_attribute_alias.cg
    tests/arb/diagnostics/vp_attribute_alias.txt)
add_arb_failure(arbvp1_discard arbvp1
    tests/arb/diagnostics/vp_discard.cg
    tests/arb/diagnostics/vp_discard.txt)
add_arb_failure(arbvp1_clip_output arbvp1
    tests/arb/diagnostics/vp_clip_output.cg
    tests/arb/diagnostics/vp_clip_output.txt)
add_arb_failure(arbfp1_dynamic_index arbfp1
    tests/arb/diagnostics/fp_dynamic_index.cg
    tests/arb/diagnostics/fp_dynamic_index.txt)
add_arb_fixture(arbfp1_params_24 arbfp1
    tests/arb/limits/fp_params_24.cg
    tests/arb/limits/fp_params_24.expected)
add_arb_failure(arbfp1_params_25 arbfp1
    tests/arb/limits/fp_params_25.cg
    tests/arb/limits/fp_params_25.txt)
add_arb_fixture(arbfp1_attributes_10 arbfp1
    tests/arb/limits/fp_attributes_10.cg
    tests/arb/limits/fp_attributes_10.expected)
add_arb_failure(arbfp1_attributes_11 arbfp1
    tests/arb/limits/fp_attributes_11.cg
    tests/arb/limits/fp_attributes_11.txt)
add_arb_fixture(arbfp1_alu_48 arbfp1
    tests/arb/limits/fp_alu_48.cg
    tests/arb/limits/fp_alu_48.expected)
add_arb_failure(arbfp1_alu_49 arbfp1
    tests/arb/limits/fp_alu_49.cg
    tests/arb/limits/fp_alu_49.txt)
add_arb_fixture(arbfp1_tex_24 arbfp1
    tests/arb/limits/fp_tex_24.cg
    tests/arb/limits/fp_tex_24.expected)
add_arb_failure(arbfp1_tex_25 arbfp1
    tests/arb/limits/fp_tex_25.cg
    tests/arb/limits/fp_tex_25.txt)
add_arb_failure(arbfp1_indirections_5 arbfp1
    tests/arb/textures/fp_indirections_5.cg
    tests/arb/limits/fp_indirections_5.txt)
add_arb_fixture(arbfp1_texture_units_2 arbfp1
    tests/arb/limits/fp_texture_units_2.cg
    tests/arb/limits/fp_texture_units_2.expected)
add_arb_failure(arbfp1_texture_units_3 arbfp1
    tests/arb/limits/fp_texture_units_3.cg
    tests/arb/limits/fp_texture_units_3.txt)

set(CGC_REFERENCE_EXECUTABLE "" CACHE FILEPATH
    "Optional NVIDIA Cg cgc executable used for ARB compatibility tests")

if(CGC_REFERENCE_EXECUTABLE AND NOT EXISTS "${CGC_REFERENCE_EXECUTABLE}")
    message(FATAL_ERROR
        "CGC_REFERENCE_EXECUTABLE does not exist: ${CGC_REFERENCE_EXECUTABLE}")
endif()

if(CGC_REFERENCE_EXECUTABLE)
    function(add_arb_oracle name profile source)
        add_test(NAME oracle_${name}
            COMMAND ${CMAKE_COMMAND}
                -DCGC_UNDER_TEST=$<TARGET_FILE:cgc>
                -DREFERENCE_CGC=${CGC_REFERENCE_EXECUTABLE}
                -DPROFILE=${profile}
                -DSOURCE=${PROJECT_SOURCE_DIR}/${source}
                -DENTRY=main
                -DWORK_DIR=${CMAKE_CURRENT_BINARY_DIR}/oracle
                -P ${CMAKE_CURRENT_SOURCE_DIR}/compare_arb_oracle.cmake)
        set_tests_properties(oracle_${name} PROPERTIES LABELS oracle)
    endfunction()

    add_arb_oracle(vp_passthrough arbvp1 tests/arb/oracle/vp_passthrough.cg)
    add_arb_oracle(vp_uniform arbvp1 tests/arb/oracle/vp_uniform.cg)
    add_arb_oracle(fp_passthrough arbfp1 tests/arb/oracle/fp_passthrough.cg)
    add_arb_oracle(fp_texture arbfp1 tests/arb/oracle/fp_texture.cg)
endif()

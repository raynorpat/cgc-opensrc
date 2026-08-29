find_program(GLSLANG_VALIDATOR NAMES glslangValidator)
if(NOT GLSLANG_VALIDATOR)
    message(STATUS
        "glslangValidator not found; GLSL source validation tests are disabled")
endif()

find_program(FXC_EXECUTABLE NAMES fxc fxc.exe)
if(WIN32 AND NOT FXC_EXECUTABLE)
    file(GLOB FXC_EXECUTABLE_CANDIDATES
        "$ENV{ProgramFiles\(x86\)}/Windows Kits/10/bin/*/x64/fxc.exe")
    list(SORT FXC_EXECUTABLE_CANDIDATES COMPARE NATURAL ORDER DESCENDING)
    if(FXC_EXECUTABLE_CANDIDATES)
        list(GET FXC_EXECUTABLE_CANDIDATES 0 FXC_EXECUTABLE)
        set(FXC_EXECUTABLE "${FXC_EXECUTABLE}" CACHE FILEPATH
            "DirectX HLSL compiler used for SM3 validation" FORCE)
    endif()
endif()
if(NOT FXC_EXECUTABLE)
    message(STATUS "fxc not found; HLSL SM3 validation tests are disabled")
endif()

# Golden regeneration switch (Task 8 Step 7): used only by the fixture
# registration below; flip to ON to rewrite the tracked GLSL
# expectations, inspect every diff before committing, then turn it off.
set(CGC_UPDATE_GLSL_EXPECTED OFF CACHE BOOL
    "Regenerate tracked GLSL expectations; inspect before committing")
set(CGC_UPDATE_HLSL_EXPECTED OFF CACHE BOOL
    "Regenerate tracked HLSL expectations; inspect before committing")

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

function(add_glsl_geometry_validation name source entry profile_options)
    if(GLSLANG_VALIDATOR)
        set(glsl_geometry_args
            -DCGC=$<TARGET_FILE:cgc>
            "-DVALIDATOR=${GLSLANG_VALIDATOR}"
            -DPROFILE=glslg
            -DSTAGE=geom
            -DSOURCE=${PROJECT_SOURCE_DIR}/${source}
            -DENTRY=${entry}
            "-DCONFIG=$<CONFIG>"
            "-DOUTPUT=${CMAKE_CURRENT_BINARY_DIR}/validated/$<CONFIG>/${name}.glsl")
        string(REPLACE ";" "\\;" glsl_geometry_options
            "${profile_options}")
        list(APPEND glsl_geometry_args
            "-DPROFILE_OPTIONS=${glsl_geometry_options}")
        add_test(
            NAME validate_${name}
            COMMAND ${CMAKE_COMMAND} ${glsl_geometry_args}
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
            -DUPDATE_EXPECTED=${CGC_UPDATE_GLSL_EXPECTED}
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

# Core GLSL 1.50 output contract (Task 8 Step 1): compiles a fixture
# and fails unless the generated shader starts with exactly one
# #version 150 and contains none of the forbidden compatibility tokens
# (attribute/varying/gl_FragColor/texture1D-3D,Cube/#extension).
function(add_glsl150_contract name profile source)
    add_test(
        NAME ${name}
        COMMAND ${CMAKE_COMMAND}
            -DCGC=$<TARGET_FILE:cgc>
            -DPROFILE=${profile}
            -DSOURCE=${PROJECT_SOURCE_DIR}/${source}
            "-DCONFIG=$<CONFIG>"
            "-DACTUAL=${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/${name}.glsl"
            -P ${CMAKE_CURRENT_SOURCE_DIR}/check_glsl150.cmake
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
    "-DMESSAGE=#version 150"
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
# neutral generic profile; negatives pin exact code and line.

add_cg20_success(cg20_geometry_modifiers
    -DCGC=$<TARGET_FILE:cgc> -DPROFILE=generic
    "-DEXTRA_ARGS=-entry\;point_main"
    -DSOURCE=${PROJECT_SOURCE_DIR}/tests/cg20/geometry/modifiers.cg
    -P ${CMAKE_CURRENT_SOURCE_DIR}/check_cg20.cmake)

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
# The entry-input positives compile once Task 5's selected-program
# analysis supplies the resolved type views and folds .length.  Every
# negative fixture fires at declaration or analysis time.

add_cg20_success(cg20_geometry_attrib_array
    -DCGC=$<TARGET_FILE:cgc> -DPROFILE=generic
    "-DEXTRA_ARGS=-entry\;triangle_main"
    -DSOURCE=${PROJECT_SOURCE_DIR}/tests/cg20/geometry/attrib_array.cg
    -P ${CMAKE_CURRENT_SOURCE_DIR}/check_cg20.cmake)

add_cg20_success(cg20_geometry_attrib_array_aggregate
    -DCGC=$<TARGET_FILE:cgc> -DPROFILE=generic
    "-DEXTRA_ARGS=-entry\;triangle_agg"
    -DSOURCE=${PROJECT_SOURCE_DIR}/tests/cg20/geometry/attrib_array_aggregate.cg
    -P ${CMAKE_CURRENT_SOURCE_DIR}/check_cg20.cmake)

add_cg20_success(cg20_geometry_attrib_array_helper
    -DCGC=$<TARGET_FILE:cgc> -DPROFILE=generic
    "-DEXTRA_ARGS=-entry\;triangle_helper\;-po\;TRIANGLE\;-po\;Vertices=3"
    -DSOURCE=${PROJECT_SOURCE_DIR}/tests/cg20/geometry/attrib_array_helper.cg
    -P ${CMAKE_CURRENT_SOURCE_DIR}/check_cg20.cmake)
add_glsl_geometry_validation(glsl_geometry_attrib_array_helper
    tests/cg20/geometry/attrib_array_helper.cg triangle_helper
    "TRIANGLE;Vertices=3")

function(add_cg20_geometry_attrib_array_length name entry)
    add_cg20_success(${name}
        -DCGC=$<TARGET_FILE:cgc> -DPROFILE=generic
        "-DEXTRA_ARGS=-entry\;${entry}"
        -DSOURCE=${PROJECT_SOURCE_DIR}/tests/cg20/geometry/attrib_array_length.cg
        -P ${CMAKE_CURRENT_SOURCE_DIR}/check_cg20.cmake)
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

# Geometry operations (Task 4): the three positive fixtures compile
# end-to-end now that Task 5 wires selected-program analysis into
# CompileProgram.  Every negative fixture is diagnosed by global
# operation classification and was already enabled.

add_cg20_success(cg20_geometry_operations
    -DCGC=$<TARGET_FILE:cgc> -DPROFILE=generic
    "-DEXTRA_ARGS=-entry\;operation_main"
    -DSOURCE=${PROJECT_SOURCE_DIR}/tests/cg20/geometry/operations.cg
    -P ${CMAKE_CURRENT_SOURCE_DIR}/check_cg20.cmake)

add_cg20_success(cg20_geometry_aggregate_bundles
    -DCGC=$<TARGET_FILE:cgc> -DPROFILE=generic
    "-DEXTRA_ARGS=-entry\;aggregate_main"
    -DSOURCE=${PROJECT_SOURCE_DIR}/tests/cg20/geometry/aggregate_bundles.cg
    -P ${CMAKE_CURRENT_SOURCE_DIR}/check_cg20.cmake)

add_cg20_success(cg20_geometry_semantic_sources
    -DCGC=$<TARGET_FILE:cgc> -DPROFILE=generic
    "-DEXTRA_ARGS=-entry\;sources_main"
    -DSOURCE=${PROJECT_SOURCE_DIR}/tests/cg20/geometry/semantic_sources.cg
    -P ${CMAKE_CURRENT_SOURCE_DIR}/check_cg20.cmake)

function(add_cg20_geometry_operation_negative name code line message)
    string(REPLACE "cg20_" "" source_name "${name}")
    add_cg20_failure(${name}
        -DCGC=$<TARGET_FILE:cgc> -DPROFILE=generic
        -DSOURCE=${PROJECT_SOURCE_DIR}/tests/cg20/diagnostics/${source_name}.cg
        -DCODE=${code} -DEXPECTED_LINE=${line}
        "-DMESSAGE=${message}"
        -P ${CMAKE_CURRENT_SOURCE_DIR}/check_cg20_failure.cmake)
endfunction()

add_cg20_geometry_operation_negative(cg20_geometry_operation_empty_emit
    6310 5 invalid.arguments.to.geometry.operation)
add_cg20_geometry_operation_negative(cg20_geometry_operation_empty_flat
    6310 5 invalid.arguments.to.geometry.operation)
add_cg20_geometry_operation_negative(cg20_geometry_operation_restart_args
    6310 5 invalid.arguments.to.geometry.operation)
add_cg20_geometry_operation_negative(cg20_geometry_operation_unresolved_semantic
    6312 7 has.no.binding.semantic)
add_cg20_geometry_operation_negative(cg20_geometry_operation_duplicate_semantic
    6313 5 duplicate.geometry.output.semantic)
add_cg20_geometry_operation_negative(cg20_geometry_operation_flat_position
    6315 4 POSITION.cannot.be.passed.to.flatAttrib)
add_cg20_geometry_operation_negative(cg20_geometry_operation_regular_annotation
    6320 6 binding.semantic)
foreach(context assignment constructor conditional return argument arithmetic)
    add_cg20_geometry_operation_negative(
        cg20_geometry_operation_nested_${context}
        6311 7 must.be.a.complete.statement)
endforeach()

add_cg20_failure(cg20_geometry_operation_cg11
    -DCGC=$<TARGET_FILE:cgc> -DPROFILE=generic
    "-DEXTRA_ARGS=-version\;1.1"
    -DSOURCE=${PROJECT_SOURCE_DIR}/tests/cg20/diagnostics/geometry_operation_cg11.cg
    -DCODE=1400 -DEXPECTED_LINE=4
    "-DMESSAGE=requires.the.Cg.2.0.language"
    -P ${CMAKE_CURRENT_SOURCE_DIR}/check_cg20_failure.cmake)
add_cg20_failure(cg20_geometry_operation_annotation_cg11
    -DCGC=$<TARGET_FILE:cgc> -DPROFILE=generic
    "-DEXTRA_ARGS=-version\;1.1"
    -DSOURCE=${PROJECT_SOURCE_DIR}/tests/cg20/diagnostics/geometry_operation_annotation_cg11.cg
    -DCODE=1400 -DEXPECTED_LINE=4
    "-DMESSAGE=requires.the.Cg.2.0.language"
    -P ${CMAKE_CURRENT_SOURCE_DIR}/check_cg20_failure.cmake)

# Selected-program analysis (Task 5): the geometry runners pass an
# explicit entry and a semicolon list of raw "-po" values to the fixture
# scripts, which expand each item into its own option pair.  Both
# helpers append to CG20_TEST_NAMES like the base pair, so the manifest
# cross-check sees them; existing two-argument registrations are
# unchanged.

function(add_cg20_geometry_success name profile source entry expected profile_options)
    set_property(GLOBAL APPEND PROPERTY CG20_TEST_NAMES "${name}")
    set(cg20_geometry_args
        -DCGC=$<TARGET_FILE:cgc>
        -DPROFILE=${profile}
        -DSOURCE=${PROJECT_SOURCE_DIR}/${source}
        -DENTRY=${entry}
        "-DCONFIG=$<CONFIG>"
        "-DACTUAL=${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/${name}.out")
    if(expected)
        list(APPEND cg20_geometry_args "-DEXPECTED=${expected}")
    endif()
    if(profile_options)
        # Escape the separators so the option list survives the
        # add_test COMMAND expansion as one argument.
        string(REPLACE ";" "\\;" cg20_escaped_options "${profile_options}")
        list(APPEND cg20_geometry_args "-DPROFILE_OPTIONS=${cg20_escaped_options}")
    endif()
    add_test(NAME "${name}"
        COMMAND ${CMAKE_COMMAND} ${cg20_geometry_args}
            -P ${CMAKE_CURRENT_SOURCE_DIR}/check_cg20.cmake)
endfunction()

function(add_cg20_geometry_failure name profile source entry code line message profile_options)
    set_property(GLOBAL APPEND PROPERTY CG20_TEST_NAMES "${name}")
    set(cg20_geometry_args
        -DCGC=$<TARGET_FILE:cgc>
        -DPROFILE=${profile}
        -DSOURCE=${PROJECT_SOURCE_DIR}/${source}
        -DENTRY=${entry}
        -DCODE=${code}
        -DEXPECTED_LINE=${line}
        "-DCONFIG=$<CONFIG>"
        "-DACTUAL=${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/${name}.txt")
    if(message)
        list(APPEND cg20_geometry_args "-DMESSAGE=${message}")
    endif()
    if(profile_options)
        # Escape the separators so the option list survives the
        # add_test COMMAND expansion as one argument.
        string(REPLACE ";" "\\;" cg20_escaped_options "${profile_options}")
        list(APPEND cg20_geometry_args "-DPROFILE_OPTIONS=${cg20_escaped_options}")
    endif()
    add_test(NAME "${name}"
        COMMAND ${CMAKE_COMMAND} ${cg20_geometry_args}
            -P ${CMAKE_CURRENT_SOURCE_DIR}/check_cg20_failure.cmake)
endfunction()

# Reachability and semantic positives (Step 7): every geometry-only
# semantic in its valid direction, operations through two helper
# levels, and an unreachable geometry helper that must not poison a
# vertex entry.

add_cg20_geometry_success(cg20_geometry_semantics generic
    tests/cg20/geometry/semantics.cg semantics_main "" "")
add_cg20_geometry_success(cg20_geometry_reachable_helper generic
    tests/cg20/geometry/reachable_helper.cg reachable_main "" "")
add_cg20_geometry_success(cg20_geometry_unreachable_helper generic
    tests/cg20/geometry/unreachable_helper.cg main "" "")

# Profile-option matrix (Step 6): source-only, options-only, matching
# mixed, conflicting mixed, duplicate equal/unequal Vertices, malformed
# values, and missing input.  Option ordinals start at zero, so a
# command-line anchor cites ordinal + 1 as its line.

add_cg20_geometry_success(cg20_geometry_profile_option_source_only generic
    tests/cg20/geometry/profile_options.cg profile_main "" "")
add_cg20_geometry_success(cg20_geometry_profile_option_options_only generic
    tests/cg20/geometry/options_only.cg main "" "TRIANGLE;Vertices=3")
add_cg20_geometry_success(cg20_geometry_profile_option_mixed_match generic
    tests/cg20/geometry/profile_options.cg profile_main "" "TRIANGLE;Vertices=3")
add_cg20_geometry_success(cg20_geometry_profile_option_duplicate_equal generic
    tests/cg20/geometry/options_only.cg main "" "TRIANGLE;Vertices=3;Vertices=3")
add_cg20_geometry_failure(cg20_geometry_profile_option_mixed_conflict generic
    tests/cg20/geometry/profile_options.cg profile_main
    6301 1 conflicting.geometry.profile.option "LINE_ADJ")
add_cg20_geometry_failure(cg20_geometry_profile_option_duplicate_unequal generic
    tests/cg20/geometry/options_only.cg main
    6301 3 conflicting.geometry.profile.option "TRIANGLE;Vertices=2;Vertices=5")
add_cg20_geometry_failure(cg20_geometry_profile_option_malformed generic
    tests/cg20/geometry/options_only.cg main
    6300 2 invalid.geometry.profile.option "TRIANGLE;Vertices=x")
add_cg20_geometry_failure(cg20_geometry_profile_option_missing_input generic
    tests/cg20/geometry/options_only.cg main
    6302 1 input.topology.is.required "Vertices=5")

# Lowered geometry goldens (Task 7): each fixture compiles under the
# generic profile with an explicit entry and its own literal -po list,
# then pins the complete normalized output -- geometry header, resolved
# attribute-array extents, and explicit emit/flat/restart operations.
# Every registration states its own ENTRY and PROFILE_OPTIONS.

add_cg20_geometry_success(cg20_geometry_pass_through generic
    tests/cg20/geometry/pass_through.cg main
    ${PROJECT_SOURCE_DIR}/tests/cg20/geometry/pass_through.expected
    "TRIANGLE;Vertices=3")
add_cg20_geometry_success(cg20_geometry_amplify generic
    tests/cg20/geometry/amplify.cg main
    ${PROJECT_SOURCE_DIR}/tests/cg20/geometry/amplify.expected
    "POINT;Vertices=8")
add_cg20_geometry_success(cg20_geometry_restart generic
    tests/cg20/geometry/restart.cg main
    ${PROJECT_SOURCE_DIR}/tests/cg20/geometry/restart.expected
    "TRIANGLE;Vertices=12")
add_cg20_geometry_success(cg20_geometry_flat generic
    tests/cg20/geometry/flat.cg main
    ${PROJECT_SOURCE_DIR}/tests/cg20/geometry/flat.expected
    "TRIANGLE;Vertices=6")
add_cg20_geometry_success(cg20_geometry_adjacency generic
    tests/cg20/geometry/adjacency.cg main
    ${PROJECT_SOURCE_DIR}/tests/cg20/geometry/adjacency.expected
    "LINE_ADJ;Vertices=4")
add_cg20_geometry_success(cg20_geometry_layered generic
    tests/cg20/geometry/layered.cg main
    ${PROJECT_SOURCE_DIR}/tests/cg20/geometry/layered.expected
    "POINT;Vertices=4")

add_cg20_geometry_success(glsl_geometry_lower_pass glslg
    tests/glsl/geometry/topologies.cg main ""
    "TRIANGLE;Vertices=3")
add_glsl_geometry_validation(glsl_geometry_lower_pass
    tests/glsl/geometry/topologies.cg main "TRIANGLE;Vertices=3")
add_cg20_geometry_success(glsl_geometry_flat_lower glslg
    tests/cg20/geometry/flat.cg main
    ${PROJECT_SOURCE_DIR}/tests/glsl/geometry/operations.expected
    "TRIANGLE;Vertices=6")
add_cg20_geometry_success(glsl_geometry_semantics_lower glslg
    tests/glsl/geometry/semantics.cg semantics_main
    ${PROJECT_SOURCE_DIR}/tests/glsl/geometry/semantics.expected
    "Vertices=3")
add_glsl_geometry_validation(glsl_geometry_semantics_lower
    tests/glsl/geometry/semantics.cg semantics_main "Vertices=3")
add_glsl_geometry_validation(glsl_geometry_pass_through_lower
    tests/cg20/geometry/pass_through.cg main "TRIANGLE;Vertices=3")
add_glsl_geometry_validation(glsl_geometry_amplify_lower
    tests/cg20/geometry/amplify.cg main "POINT;POINT_OUT;Vertices=4")
add_glsl_geometry_validation(glsl_geometry_adjacency_lower
    tests/cg20/geometry/adjacency.cg main
    "LINE_ADJ;LINE_OUT;Vertices=2")
add_glsl_geometry_validation(glsl_geometry_flat_lower
    tests/cg20/geometry/flat.cg main "TRIANGLE;Vertices=6")
add_glsl_geometry_validation(glsl_geometry_reachable_helper_lower
    tests/cg20/geometry/reachable_helper.cg reachable_main "Vertices=3")
add_cg20_geometry_success(glsl_geometry_uniform_texture_lower glslg
    tests/glsl/geometry/uniform_texture.cg main
    ${PROJECT_SOURCE_DIR}/tests/glsl/geometry/uniform_texture.expected
    "Vertices=1")
add_glsl_geometry_validation(glsl_geometry_uniform_texture_lower
    tests/glsl/geometry/uniform_texture.cg main "Vertices=1")
add_cg20_geometry_success(glsl_geometry_conditional_flat_lower glslg
    tests/glsl/geometry/conditional_flat.cg main
    ${PROJECT_SOURCE_DIR}/tests/glsl/geometry/control_flow.expected
    "Vertices=1")
add_glsl_geometry_validation(glsl_geometry_conditional_flat_lower
    tests/glsl/geometry/conditional_flat.cg main "Vertices=1")
add_cg20_geometry_success(glsl_geometry_reachable_helper_lower glslg
    tests/cg20/geometry/reachable_helper.cg reachable_main
    ${PROJECT_SOURCE_DIR}/tests/glsl/geometry/helpers.expected
    "Vertices=3")

add_cg20_geometry_success(glsl_geometry_topology_points glslg
    tests/cg20/geometry/pass_through.cg main
    ${PROJECT_SOURCE_DIR}/tests/glsl/geometry/topology_points.expected
    "POINT;Vertices=1")
add_glsl_geometry_validation(glsl_geometry_topology_points
    tests/cg20/geometry/pass_through.cg main "POINT;Vertices=1")
add_cg20_geometry_success(glsl_geometry_topology_lines glslg
    tests/cg20/geometry/pass_through.cg main
    ${PROJECT_SOURCE_DIR}/tests/glsl/geometry/topology_lines.expected
    "LINE;Vertices=1")
add_glsl_geometry_validation(glsl_geometry_topology_lines
    tests/cg20/geometry/pass_through.cg main "LINE;Vertices=1")
add_cg20_geometry_success(glsl_geometry_topology_lines_adjacency glslg
    tests/cg20/geometry/pass_through.cg main
    ${PROJECT_SOURCE_DIR}/tests/glsl/geometry/topology_lines_adjacency.expected
    "LINE_ADJ;Vertices=1")
add_glsl_geometry_validation(glsl_geometry_topology_lines_adjacency
    tests/cg20/geometry/pass_through.cg main "LINE_ADJ;Vertices=1")
add_cg20_geometry_success(glsl_geometry_topology_triangles glslg
    tests/cg20/geometry/pass_through.cg main
    ${PROJECT_SOURCE_DIR}/tests/glsl/geometry/topology_triangles.expected
    "TRIANGLE;Vertices=1")
add_glsl_geometry_validation(glsl_geometry_topology_triangles
    tests/cg20/geometry/pass_through.cg main "TRIANGLE;Vertices=1")
add_cg20_geometry_success(glsl_geometry_topology_triangles_adjacency glslg
    tests/cg20/geometry/pass_through.cg main
    ${PROJECT_SOURCE_DIR}/tests/glsl/geometry/topology_triangles_adjacency.expected
    "TRIANGLE_ADJ;Vertices=1")
add_glsl_geometry_validation(glsl_geometry_topology_triangles_adjacency
    tests/cg20/geometry/pass_through.cg main "TRIANGLE_ADJ;Vertices=1")
add_cg20_geometry_success(glsl_geometry_output_points glslg
    tests/cg20/geometry/pass_through.cg main
    ${PROJECT_SOURCE_DIR}/tests/glsl/geometry/output_points.expected
    "POINT;POINT_OUT;Vertices=1")
add_glsl_geometry_validation(glsl_geometry_output_points
    tests/cg20/geometry/pass_through.cg main
    "POINT;POINT_OUT;Vertices=1")
add_cg20_geometry_success(glsl_geometry_output_line_strip glslg
    tests/cg20/geometry/pass_through.cg main
    ${PROJECT_SOURCE_DIR}/tests/glsl/geometry/output_line_strip.expected
    "POINT;LINE_OUT;Vertices=1")
add_glsl_geometry_validation(glsl_geometry_output_line_strip
    tests/cg20/geometry/pass_through.cg main
    "POINT;LINE_OUT;Vertices=1")
add_cg20_geometry_success(glsl_geometry_output_triangle_strip glslg
    tests/cg20/geometry/pass_through.cg main
    ${PROJECT_SOURCE_DIR}/tests/glsl/geometry/output_triangle_strip.expected
    "POINT;TRIANGLE_OUT;Vertices=1")
add_glsl_geometry_validation(glsl_geometry_output_triangle_strip
    tests/cg20/geometry/pass_through.cg main
    "POINT;TRIANGLE_OUT;Vertices=1")
add_glsl150_contract(glslv_vertex_id_bridge glslv
    tests/glsl/geometry/vertex_id_bridge.cg)
add_glsl_validation(glslv_vertex_id_bridge glslv vert
    tests/glsl/geometry/vertex_id_bridge.cg)
add_glsl150_contract(glslf_primitive_id_input glslf
    tests/glsl/geometry/fragment_primitive_id.cg)
add_glsl_validation(glslf_primitive_id_input glslf frag
    tests/glsl/geometry/fragment_primitive_id.cg)

# Geometry profile preflight and exact portable resource boundaries.
add_cg20_geometry_failure(glslg_geometry_missing_vertices glslg
    tests/glsl/geometry/diagnostics/missing_vertices.cg main
    6319 0 "requires.-po.Vertices" "")
add_cg20_geometry_failure(glslv_geometry_stage_mismatch glslv
    tests/glsl/geometry/diagnostics/wrong_stage_vertex.cg main
    6318 1 "cannot.compile.a.geometry.program" "Vertices=1")
add_cg20_geometry_failure(glslf_geometry_stage_mismatch glslf
    tests/glsl/geometry/diagnostics/wrong_stage_fragment.cg main
    6318 1 "cannot.compile.a.geometry.program" "Vertices=1")
add_cg20_geometry_failure(arbvp1_geometry_stage_reject arbvp1
    tests/cg20/geometry/pass_through.cg main
    6011 6 "does.not.support.geometry.programs"
    "TRIANGLE;Vertices=1")
add_cg20_geometry_failure(arbfp1_geometry_stage_reject arbfp1
    tests/cg20/geometry/pass_through.cg main
    6011 6 "does.not.support.geometry.programs"
    "TRIANGLE;Vertices=1")
add_cg20_geometry_failure(glslg_unsupported_type glslg
    tests/glsl/geometry/diagnostics/unsupported_type.cg main
    6200 5 "does.not.support.type.*matrix" "Vertices=1")
add_cg20_geometry_failure(glslg_unsupported_intrinsic glslg
    tests/glsl/geometry/diagnostics/unsupported_intrinsic.cg main
    6206 1 "no.exact.intrinsic.*rsqrt" "Vertices=1")
add_cg20_geometry_failure(glslg_unsupported_sampler glslg
    tests/glsl/geometry/diagnostics/unsupported_sampler.cg main
    6200 1 "does.not.support.type.*samplerRECT" "Vertices=1")

add_cg20_geometry_success(glslg_input_components_64 glslg
    tests/glsl/geometry/limits/input_components_64.cg main "" "Vertices=1")
add_glsl_geometry_validation(glslg_input_components_64
    tests/glsl/geometry/limits/input_components_64.cg main "Vertices=1")
add_cg20_geometry_failure(glslg_input_components_65 glslg
    tests/glsl/geometry/limits/input_components_65.cg main
    6207 1 "65.used,.64.available" "Vertices=1")
add_cg20_geometry_success(glslg_output_components_128 glslg
    tests/glsl/geometry/limits/output_components_128.cg main "" "Vertices=1")
add_glsl_geometry_validation(glslg_output_components_128
    tests/glsl/geometry/limits/output_components_128.cg main "Vertices=1")
add_cg20_geometry_failure(glslg_output_components_129 glslg
    tests/glsl/geometry/limits/output_components_129.cg main
    6207 5 "129.used,.128.available" "Vertices=1")
add_cg20_geometry_success(glslg_output_vertices_256 glslg
    tests/glsl/geometry/limits/output_vertices.cg main "" "Vertices=256")
add_glsl_geometry_validation(glslg_output_vertices_256
    tests/glsl/geometry/limits/output_vertices.cg main "Vertices=256")
add_cg20_geometry_failure(glslg_output_vertices_257 glslg
    tests/glsl/geometry/limits/output_vertices.cg main
    6207 1 "257.used,.256.available" "Vertices=257")
add_cg20_geometry_success(glslg_total_components_1024 glslg
    tests/glsl/geometry/limits/total_components_1024.cg main "" "Vertices=256")
add_glsl_geometry_validation(glslg_total_components_1024
    tests/glsl/geometry/limits/total_components_1024.cg main "Vertices=256")
add_cg20_geometry_failure(glslg_total_components_1025 glslg
    tests/glsl/geometry/limits/total_components_1025.cg main
    6207 1 "1025.used,.1024.available" "Vertices=205")
add_cg20_geometry_success(glslg_uniform_components_1024 glslg
    tests/glsl/geometry/limits/uniform_components_1024.cg main "" "Vertices=1")
add_glsl_geometry_validation(glslg_uniform_components_1024
    tests/glsl/geometry/limits/uniform_components_1024.cg main "Vertices=1")
add_cg20_geometry_failure(glslg_uniform_components_1025 glslg
    tests/glsl/geometry/limits/uniform_components_1025.cg main
    6207 3 "1025.used,.1024.available" "Vertices=1")
add_cg20_geometry_success(glslg_texture_units_16 glslg
    tests/glsl/geometry/limits/texture_units_16.cg main "" "Vertices=1")
add_glsl_geometry_validation(glslg_texture_units_16
    tests/glsl/geometry/limits/texture_units_16.cg main "Vertices=1")
add_cg20_geometry_failure(glslg_texture_units_17 glslg
    tests/glsl/geometry/limits/texture_units_17.cg main
    6207 17 "17.used,.16.available" "Vertices=1")

foreach(transaction missing_vertices input_components_65 output_vertices_257)
    if(transaction STREQUAL missing_vertices)
        set(transaction_source tests/glsl/geometry/diagnostics/missing_vertices.cg)
        set(transaction_options "")
        set(transaction_code 6319)
    elseif(transaction STREQUAL input_components_65)
        set(transaction_source tests/glsl/geometry/limits/input_components_65.cg)
        set(transaction_options "Vertices=1")
        set(transaction_code 6207)
    else()
        set(transaction_source tests/glsl/geometry/limits/output_vertices.cg)
        set(transaction_options "Vertices=257")
        set(transaction_code 6207)
    endif()
    string(REPLACE ";" "\\;" transaction_options_escaped
        "${transaction_options}")
    add_test(NAME glslg_${transaction}_transaction
        COMMAND ${CMAKE_COMMAND}
            -DCGC=$<TARGET_FILE:cgc>
            -DPROFILE=glslg
            -DSOURCE=${PROJECT_SOURCE_DIR}/${transaction_source}
            -DENTRY=main
            "-DPROFILE_OPTIONS=${transaction_options_escaped}"
            -DCODE=${transaction_code}
            "-DDESTINATION=${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/glslg_${transaction}_transaction.glsl"
            -P ${CMAKE_CURRENT_SOURCE_DIR}/check_output_transaction.cmake)
endforeach()

foreach(arb_profile arbvp1 arbfp1)
    add_test(NAME ${arb_profile}_geometry_transaction
        COMMAND ${CMAKE_COMMAND}
            -DCGC=$<TARGET_FILE:cgc>
            -DPROFILE=${arb_profile}
            -DSOURCE=${PROJECT_SOURCE_DIR}/tests/glsl/link/gp_geometry_link.cg
            -DENTRY=main
            -DPROFILE_OPTIONS=Vertices=1
            -DCODE=6011
            "-DDESTINATION=${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/${arb_profile}_geometry_transaction.out"
            -P ${CMAKE_CURRENT_SOURCE_DIR}/check_output_transaction.cmake)
endforeach()

# Geometry semantic negatives (Step 7): one fixture per wrong direction,
# type, or shape, each pinning exact code and line.

function(add_cg20_geometry_semantic_negative name entry code line message)
    string(REPLACE "cg20_" "" source_name "${name}")
    add_cg20_geometry_failure(${name} generic
        tests/cg20/diagnostics/${source_name}.cg ${entry}
        ${code} ${line} ${message} "")
endfunction()

add_cg20_geometry_semantic_negative(cg20_geometry_semantic_instanceid_type
    bad_instance_type 6321 5 invalid.value.type)
add_cg20_geometry_semantic_negative(cg20_geometry_semantic_instanceid_output
    bad_instance_output 6314 6 invalid.geometry.semantic)
add_cg20_geometry_semantic_negative(cg20_geometry_semantic_vertexid_shape
    bad_vertexid_shape 6321 5 invalid.value.type)
add_cg20_geometry_semantic_negative(cg20_geometry_semantic_vertexid_output
    bad_vertexid_output 6314 6 invalid.geometry.semantic)
add_cg20_geometry_semantic_negative(cg20_geometry_semantic_primitiveid_type
    bad_primitiveid_type 6321 5 invalid.value.type)
add_cg20_geometry_semantic_negative(cg20_geometry_semantic_layer_type
    bad_layer 6321 7 invalid.value.type)

# Reachability negatives (Steps 3 and 7): a geometry operation reached
# from a vertex or fragment entry anchors C6317 at the operation with
# call-path notes back to the entry, and calling a topology-qualified
# function as an ordinary helper anchors C6316 at its declaration.

add_cg20_failure(cg20_geometry_stage_vertex
    -DCGC=$<TARGET_FILE:cgc> -DPROFILE=glslv -DENTRY=vertex_main
    -DSOURCE=${PROJECT_SOURCE_DIR}/tests/cg20/diagnostics/geometry_stage_vertex.cg
    "-DCONFIG=$<CONFIG>"
    "-DACTUAL=${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/cg20_geometry_stage_vertex.txt"
    -DCODE=6317 -DEXPECTED_LINE=7
    "-DMESSAGE=reachable.from.a.non-geometry.entry"
    "-DNOTES=profile failure in function strip_helper\\;reached from entry function vertex_main"
    -P ${CMAKE_CURRENT_SOURCE_DIR}/check_cg20_failure.cmake)
add_cg20_failure(cg20_geometry_stage_fragment
    -DCGC=$<TARGET_FILE:cgc> -DPROFILE=glslf -DENTRY=fragment_main
    -DSOURCE=${PROJECT_SOURCE_DIR}/tests/cg20/diagnostics/geometry_stage_fragment.cg
    "-DCONFIG=$<CONFIG>"
    "-DACTUAL=${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/cg20_geometry_stage_fragment.txt"
    -DCODE=6317 -DEXPECTED_LINE=7
    "-DMESSAGE=reachable.from.a.non-geometry.entry"
    "-DNOTES=profile failure in function strip_helper\\;reached from entry function fragment_main"
    -P ${CMAKE_CURRENT_SOURCE_DIR}/check_cg20_failure.cmake)
add_cg20_failure(cg20_geometry_entry_call
    -DCGC=$<TARGET_FILE:cgc> -DPROFILE=generic -DENTRY=main
    -DSOURCE=${PROJECT_SOURCE_DIR}/tests/cg20/diagnostics/geometry_entry_call.cg
    "-DCONFIG=$<CONFIG>"
    "-DACTUAL=${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/cg20_geometry_entry_call.txt"
    -DCODE=6316 -DEXPECTED_LINE=5
    "-DMESSAGE=cannot.be.called"
    "-DNOTES=profile failure in function geometry_entry\\;reached from entry function main"
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
    ${PROJECT_SOURCE_DIR}/glsl_verify.c
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

add_executable(hlsl_ir_unit
    ${PROJECT_SOURCE_DIR}/hlsl_bind.c
    ${PROJECT_SOURCE_DIR}/hlsl_hal.c
    ${PROJECT_SOURCE_DIR}/hlsl_ir.c
    ${PROJECT_SOURCE_DIR}/hlsl_legalize.c
    ${PROJECT_SOURCE_DIR}/hlsl_validate.c
    ${PROJECT_SOURCE_DIR}/hlsl_codegen.c
    ${PROJECT_SOURCE_DIR}/hlslv_hal.c
    ${PROJECT_SOURCE_DIR}/hlslf_hal.c
    hlsl_ir_test.c
)

target_compile_definitions(hlsl_ir_unit PRIVATE
    HLSL_CANONICALIZATION_ONLY
)

target_include_directories(hlsl_ir_unit PRIVATE
    ${PROJECT_SOURCE_DIR}
)

set_target_properties(hlsl_ir_unit PROPERTIES
    C_STANDARD 90
    C_STANDARD_REQUIRED YES
    C_EXTENSIONS YES
)

add_test(
    NAME hlsl_ir_unit
    COMMAND hlsl_ir_unit
)

add_test(
    NAME hlsl_pipeline_phase_order
    COMMAND ${CMAKE_COMMAND}
        -DSOURCE=${PROJECT_SOURCE_DIR}
        -P ${CMAKE_CURRENT_SOURCE_DIR}/check_hlsl_phase_order.cmake
)

add_executable(hlsl_invariants_unit
    ${PROJECT_SOURCE_DIR}/hlsl_hal.c
    ${PROJECT_SOURCE_DIR}/hlsl_ir.c
    ${PROJECT_SOURCE_DIR}/hlsl_legalize.c
    ${PROJECT_SOURCE_DIR}/hlsl_validate.c
    hlsl_invariants_test.c
)

target_compile_definitions(hlsl_invariants_unit PRIVATE
    HLSL_CANONICALIZATION_ONLY
)

target_include_directories(hlsl_invariants_unit PRIVATE
    ${PROJECT_SOURCE_DIR}
)

add_test(
    NAME hlsl_invariants_unit
    COMMAND hlsl_invariants_unit
)

add_test(
    NAME hlsl_binding_register_sampler_unit
    COMMAND hlsl_ir_unit
)

add_test(
    NAME hlsl_semantic_canonicalization
    COMMAND hlsl_ir_unit
)

add_test(
    NAME hlsl_ir_assertions_active
    COMMAND ${CMAKE_COMMAND}
        -DUNIT=$<TARGET_FILE:hlsl_ir_unit>
        -P ${CMAKE_CURRENT_SOURCE_DIR}/check_assertions_active.cmake
)

add_executable(hlsl_semantics_unit
    ${PROJECT_SOURCE_DIR}/atom.c
    ${PROJECT_SOURCE_DIR}/hlsl_bind.c
    ${PROJECT_SOURCE_DIR}/hlsl_ir.c
    ${PROJECT_SOURCE_DIR}/hlsl_hal.c
    ${PROJECT_SOURCE_DIR}/hlslv_hal.c
    ${PROJECT_SOURCE_DIR}/hlslf_hal.c
    hlsl_semantics_test.c
)

target_compile_definitions(hlsl_semantics_unit PRIVATE
    HLSL_DIAGNOSTIC_TESTING
)

target_include_directories(hlsl_semantics_unit PRIVATE
    ${PROJECT_SOURCE_DIR}
)

set_target_properties(hlsl_semantics_unit PROPERTIES
    C_STANDARD 90
    C_STANDARD_REQUIRED YES
    C_EXTENSIONS YES
)

add_test(NAME hlsl_semantics_unit COMMAND hlsl_semantics_unit)
add_test(
    NAME hlsl_name_collision_reporting
    COMMAND ${CMAKE_COMMAND}
        -DCGC=$<TARGET_FILE:hlsl_semantics_unit>
        -DPROFILE=hlsl-report-name
        -DSOURCE=${PROJECT_SOURCE_DIR}/tests/hlsl/diagnostics/vp_name_collision_report.cg
        -DCODE=6411
        -DEXPECTED_LINE=3
        "-DMESSAGE=HLSL name cannot be resolved for.*exhaustedName"
        "-DACTUAL=${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/hlsl_name_collision_reporting.hlsl"
        -P ${CMAKE_CURRENT_SOURCE_DIR}/check_hlsl_failure.cmake
)
add_test(
    NAME hlsl_failure_runner_rejects_metadata
    COMMAND ${CMAKE_COMMAND}
        -DUNIT=$<TARGET_FILE:hlsl_semantics_unit>
        -DSOURCE=${PROJECT_SOURCE_DIR}/tests/hlsl/diagnostics/vp_name_collision_report.cg
        -DCHECKER=${CMAKE_CURRENT_SOURCE_DIR}/check_hlsl_failure.cmake
        "-DACTUAL=${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/hlsl_failure_runner_metadata.hlsl"
        -P ${CMAKE_CURRENT_SOURCE_DIR}/check_hlsl_failure_rejects_metadata.cmake
)
add_test(
    NAME hlsl_semantics_assertions_active
    COMMAND ${CMAKE_COMMAND}
        -DUNIT=$<TARGET_FILE:hlsl_semantics_unit>
        -P ${CMAKE_CURRENT_SOURCE_DIR}/check_assertions_active.cmake
)
add_test(NAME hlsl_interface_same_owner_same_slot
    COMMAND hlsl_semantics_unit same-slot)
add_test(NAME hlsl_interface_same_owner_different_slot
    COMMAND hlsl_semantics_unit different-slot)
add_test(NAME hlsl_interface_same_owner_different_direction
    COMMAND hlsl_semantics_unit different-direction)
add_test(NAME hlsl_interface_different_owner_collision
    COMMAND hlsl_semantics_unit different-owner)
add_test(NAME hlsl_interface_fresh_hal_isolation
    COMMAND hlsl_semantics_unit fresh-hal)

add_executable(glsl_semantics_unit
    ${PROJECT_SOURCE_DIR}/atom.c
    ${PROJECT_SOURCE_DIR}/glsl_ir.c
    ${PROJECT_SOURCE_DIR}/glsl_hal.c
    ${PROJECT_SOURCE_DIR}/glsl_verify.c
    ${PROJECT_SOURCE_DIR}/glslv_hal.c
    ${PROJECT_SOURCE_DIR}/glslf_hal.c
    ${PROJECT_SOURCE_DIR}/glslg_hal.c
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
    ${PROJECT_SOURCE_DIR}/cg_geometry.c
    ${PROJECT_SOURCE_DIR}/cg_ir.c
    ${PROJECT_SOURCE_DIR}/cg_ir_verify.c
    ${PROJECT_SOURCE_DIR}/cg_numeric.c
    ${PROJECT_SOURCE_DIR}/cg_stdlib.c
    ${PROJECT_SOURCE_DIR}/language.c
    ${PROJECT_SOURCE_DIR}/cg_types.c
    ${PROJECT_SOURCE_DIR}/glsl_codegen.c
    ${PROJECT_SOURCE_DIR}/glsl_hal.c
    ${PROJECT_SOURCE_DIR}/glsl_ir.c
    ${PROJECT_SOURCE_DIR}/glsl_lower.c
    ${PROJECT_SOURCE_DIR}/glsl_verify.c
    ${PROJECT_SOURCE_DIR}/glslf_hal.c
    ${PROJECT_SOURCE_DIR}/glslg_hal.c
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
    ${PROJECT_SOURCE_DIR}/language.c
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
    ${PROJECT_SOURCE_DIR}/language.c
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
    ${PROJECT_SOURCE_DIR}/language.c
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
    ${PROJECT_SOURCE_DIR}/language.c
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
    ${PROJECT_SOURCE_DIR}/cg_geometry.c
    ${PROJECT_SOURCE_DIR}/cg_ir.c
    ${PROJECT_SOURCE_DIR}/cg_ir_lower.c
    ${PROJECT_SOURCE_DIR}/cg_ir_print.c
    ${PROJECT_SOURCE_DIR}/cg_ir_verify.c
    ${PROJECT_SOURCE_DIR}/cg_numeric.c
    ${PROJECT_SOURCE_DIR}/cg_reach.c
    ${PROJECT_SOURCE_DIR}/cg_types.c
    ${PROJECT_SOURCE_DIR}/cg_stdlib.c
    # Lowering maps every module's program stage through the shared
    # profile-stage conversion helper.
    ${PROJECT_SOURCE_DIR}/hal.c
    ${PROJECT_SOURCE_DIR}/language.c
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
# Hashes updated by geometry profile support: interning the topology and
# GLSL geometry-profile atoms shifts dynamic atom IDs, which
# reorders top-level declarations in generic output.  Verified against
# pre-change builds: outputs are identical as line multisets modulo the
# stripped banner, i.e. declaration order moved and no content changed.
add_generic_regression(vertexlight4 vertexlight4.cg
    8659adcf27d5047461607902942f0ab14161d5fcba4f491c6302ca9676b1a02c)
add_generic_regression(loop_jumps tests/glsl/expressions/vp_jumps.cg
    07c60ea6ceaeba6c3cd85020e9088d23293179983fc7cd12ca51b306025ce5f4)
add_generic_regression(direct_discard tests/glsl/diagnostics/vp_discard.cg
    0a03690bd59c1ed3ed7f8799845b4468157ff57743c72bb2bf0cd6d54345bea1)
add_generic_regression(helper_discard
    tests/glsl/diagnostics/generic_helper_discard.cg
    bd6b290f26f99e5aba011e12059163aa3648a872710b0f04f40b654fe8b73afc)
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

function(add_hlsl_validation_case name profile source packing)
    if(FXC_EXECUTABLE)
        if(profile STREQUAL "hlslv")
            set(target vs_3_0)
        else()
            set(target ps_3_0)
        endif()
        # These vertex limit fixtures intentionally do not write POSITION;
        # cgc reports only the known C7005 boundary warning for them.
        set(suppress_warnings FALSE)
        if(name MATCHES
           "^hlslv_limit_(inputs16|constants256|int16|bool16|samplers4)$")
            set(suppress_warnings TRUE)
        endif()
        add_test(
            NAME hlsl_validate_${name}
            COMMAND ${CMAKE_COMMAND}
                -DCGC=$<TARGET_FILE:cgc>
                -DFXC=${FXC_EXECUTABLE}
                -DPROFILE=${profile}
                -DTARGET=${target}
                -DSOURCE=${PROJECT_SOURCE_DIR}/${source}
                "-DCONFIG=$<CONFIG>"
                -DPACKING=${packing}
                -DSUPPRESS_WARNINGS=${suppress_warnings}
                "-DOUTPUT=${CMAKE_CURRENT_BINARY_DIR}/validated/$<CONFIG>/${name}.hlsl"
                "-DBYTECODE=${CMAKE_CURRENT_BINARY_DIR}/validated/$<CONFIG>/${name}.fxc"
                -P ${CMAKE_CURRENT_SOURCE_DIR}/validate_hlsl.cmake)
    endif()
endfunction()

function(add_hlsl_validation name profile source)
    if(name MATCHES "matrix|rectangular")
        add_hlsl_validation_case(${name}_zpr ${profile} ${source} /Zpr)
        add_hlsl_validation_case(${name}_zpc ${profile} ${source} /Zpc)
    else()
        add_hlsl_validation_case(${name} ${profile} ${source} "")
    endif()
endfunction()

function(add_hlsl_fixture name profile source expected)
    add_test(
        NAME ${name}
        COMMAND ${CMAKE_COMMAND}
            -DCGC=$<TARGET_FILE:cgc>
            -DPROFILE=${profile}
            -DSOURCE=${PROJECT_SOURCE_DIR}/${source}
            -DEXPECTED=${PROJECT_SOURCE_DIR}/${expected}
            -DUPDATE_EXPECTED=${CGC_UPDATE_HLSL_EXPECTED}
            "-DCONFIG=$<CONFIG>"
            "-DACTUAL=${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/${name}.hlsl"
            -P ${CMAKE_CURRENT_SOURCE_DIR}/check_hlsl.cmake)
    # The two registration goldens intentionally exercise the Task 1 void
    # profile skeleton.  SM3 bytecode requires a POSITION/COLOR write, so
    # these are the only successful HLSL fixtures that cannot run via FXC.
    if(NOT name STREQUAL "hlslv_registration" AND
       NOT name STREQUAL "hlslf_registration")
        add_hlsl_validation(${name} ${profile} ${source})
    endif()
endfunction()

add_hlsl_fixture(hlslv_registration hlslv
    tests/hlsl/profile/empty.cg tests/hlsl/profile/empty-vs.expected)
add_hlsl_fixture(hlslf_registration hlslf
    tests/hlsl/profile/empty.cg tests/hlsl/profile/empty-ps.expected)
add_hlsl_fixture(hlslv_vp_passthrough hlslv
    tests/hlsl/profile/vp_passthrough.cg
    tests/hlsl/profile/vp_passthrough.expected)
add_hlsl_fixture(hlslf_fp_passthrough hlslf
    tests/hlsl/profile/fp_passthrough.cg
    tests/hlsl/profile/fp_passthrough.expected)
add_hlsl_fixture(hlslv_vp_out_inout hlslv
    tests/hlsl/profile/vp_out_inout.cg
    tests/hlsl/profile/vp_out_inout.expected)
# These historical "all semantics" fixtures retain every semantic family but
# use a legal simultaneous SM3 boundary subset.  Exhaustive descriptor, alias,
# and one-past coverage lives in hlsl_semantics_unit; the complete tables
# cannot all occupy one 16-input VS or 10-input PS interface at once.
add_hlsl_fixture(hlslv_vp_all_semantics hlslv
    tests/hlsl/interfaces/vp_all_semantics.cg
    tests/hlsl/interfaces/vp_all_semantics.expected)
add_hlsl_fixture(hlslf_fp_all_semantics hlslf
    tests/hlsl/interfaces/fp_all_semantics.cg
    tests/hlsl/interfaces/fp_all_semantics.expected)
add_hlsl_fixture(hlslv_vp_register_banks hlslv
    tests/hlsl/bindings/vp_register_banks.cg
    tests/hlsl/bindings/vp_register_banks.expected)
add_hlsl_fixture(hlslv_vp_int_bank_shapes hlslv
    tests/hlsl/bindings/vp_int_bank_shapes.cg
    tests/hlsl/bindings/vp_int_bank_shapes.expected)
add_hlsl_fixture(hlslv_vp_int_array_helpers hlslv
    tests/hlsl/bindings/vp_int_array_helpers.cg
    tests/hlsl/bindings/vp_int_array_helpers.expected)
add_hlsl_fixture(hlslv_vp_bool_bank_shapes hlslv
    tests/hlsl/bindings/vp_bool_bank_shapes.cg
    tests/hlsl/bindings/vp_bool_bank_shapes.expected)
add_hlsl_fixture(hlslv_vp_mixed_struct hlslv
    tests/hlsl/bindings/vp_mixed_struct.cg
    tests/hlsl/bindings/vp_mixed_struct.expected)
add_hlsl_fixture(hlslf_fp_samplers hlslf
    tests/hlsl/bindings/fp_samplers.cg
    tests/hlsl/bindings/fp_samplers.expected)
add_hlsl_fixture(hlslv_global_entry_name_collision hlslv
    tests/hlsl/bindings/vp_global_entry_name_collision.cg
    tests/hlsl/bindings/vp_global_entry_name_collision.expected)
add_hlsl_fixture(hlslv_output_member_name_collision hlslv
    tests/hlsl/interfaces/vp_output_member_name_collision.cg
    tests/hlsl/interfaces/vp_output_member_name_collision.expected)
add_hlsl_fixture(hlslv_reachable_helper_parameters hlslv
    tests/hlsl/profile/vp_reachable_helper_parameters.cg
    tests/hlsl/profile/vp_reachable_helper_parameters.expected)
add_hlsl_fixture(hlslv_arithmetic hlslv
    tests/hlsl/expressions/vp_arithmetic.cg
    tests/hlsl/expressions/vp_arithmetic.expected)
# The legacy source grammar does not admit a comma expression directly.
# Standalone post-increment reaches the same common COMMA_OP expansion and
# keeps the once-only ordering contract observable in the golden.
add_hlsl_fixture(hlslv_side_effects hlslv
    tests/hlsl/expressions/vp_side_effects.cg
    tests/hlsl/expressions/vp_side_effects.expected)
add_hlsl_fixture(hlslv_scalar_conditional_order hlslv
    tests/hlsl/expressions/vp_scalar_conditional_order.cg
    tests/hlsl/expressions/vp_scalar_conditional_order.expected)
add_hlsl_fixture(hlslv_evaluation_order hlslv
    tests/hlsl/expressions/vp_evaluation_order.cg
    tests/hlsl/expressions/vp_evaluation_order.expected)
add_hlsl_fixture(hlslv_nested_postinc hlslv
    tests/hlsl/expressions/vp_nested_postinc.cg
    tests/hlsl/expressions/vp_nested_postinc.expected)
add_hlsl_fixture(hlslv_control hlslv
    tests/hlsl/control/vp_control.cg
    tests/hlsl/control/vp_control.expected)
add_hlsl_fixture(hlslv_loop_prefixes hlslv
    tests/hlsl/control/vp_loop_prefixes.cg
    tests/hlsl/control/vp_loop_prefixes.expected)
add_hlsl_fixture(hlslv_empty_control hlslv
    tests/hlsl/control/vp_empty_control.cg
    tests/hlsl/control/vp_empty_control.expected)
add_hlsl_fixture(hlslf_discard hlslf
    tests/hlsl/control/fp_discard.cg
    tests/hlsl/control/fp_discard.expected)
add_hlsl_fixture(hlslf_vector_discard hlslf
    tests/hlsl/control/fp_vector_discard.cg
    tests/hlsl/control/fp_vector_discard.expected)
add_hlsl_fixture(hlslv_helpers hlslv
    tests/hlsl/functions/vp_helpers.cg
    tests/hlsl/functions/vp_helpers.expected)
add_hlsl_fixture(hlslv_inline_helper hlslv
    tests/hlsl/functions/vp_inline_helper.cg
    tests/hlsl/functions/vp_inline_helper.expected)
add_hlsl_fixture(hlslv_struct_array hlslv
    tests/hlsl/aggregates/vp_struct_array.cg
    tests/hlsl/aggregates/vp_struct_array.expected)
add_hlsl_fixture(hlslv_nested_out hlslv
    tests/hlsl/aggregates/vp_nested_out.cg
    tests/hlsl/aggregates/vp_nested_out.expected)
add_hlsl_fixture(hlslv_aliased_out hlslv
    tests/hlsl/aggregates/vp_aliased_out.cg
    tests/hlsl/aggregates/vp_aliased_out.expected)
add_hlsl_fixture(hlslv_rectangular hlslv
    tests/hlsl/matrices/vp_rectangular.cg
    tests/hlsl/matrices/vp_rectangular.expected)
add_hlsl_fixture(hlslv_matrix_arithmetic hlslv
    tests/hlsl/matrices/vp_matrix_arithmetic.cg
    tests/hlsl/matrices/vp_matrix_arithmetic.expected)
add_hlsl_fixture(hlslv_matrix_order hlslv
    tests/hlsl/matrices/vp_matrix_order.cg
    tests/hlsl/matrices/vp_matrix_order.expected)
add_hlsl_fixture(hlslv_defaults hlslv
    tests/hlsl/bindings/vp_defaults.cg
    tests/hlsl/bindings/vp_defaults.expected)
add_hlsl_fixture(hlslv_mixed_struct_helpers hlslv
    tests/hlsl/bindings/vp_mixed_struct_helpers.cg
    tests/hlsl/bindings/vp_mixed_struct_helpers.expected)
add_hlsl_fixture(hlslv_position hlslv
    position.cg tests/hlsl/vertex/position.expected)
add_hlsl_fixture(hlslv_reflection hlslv
    reflection.cg tests/hlsl/vertex/reflection.expected)
add_hlsl_fixture(hlslv_vertexlight hlslv
    vertexlight.cg tests/hlsl/vertex/vertexlight.expected)
add_hlsl_fixture(hlslv_vertexlight4 hlslv
    vertexlight4.cg tests/hlsl/vertex/vertexlight4.expected)
add_hlsl_fixture(hlslv_intrinsic_numeric hlslv
    tests/hlsl/intrinsics/vp_numeric.cg
    tests/hlsl/intrinsics/vp_numeric.expected)
add_hlsl_fixture(hlslv_intrinsic_geometric hlslv
    tests/hlsl/intrinsics/vp_geometric.cg
    tests/hlsl/intrinsics/vp_geometric.expected)
add_hlsl_fixture(hlslf_intrinsic_derivatives hlslf
    tests/hlsl/intrinsics/fp_derivatives.cg
    tests/hlsl/intrinsics/fp_derivatives.expected)
add_hlsl_fixture(hlslv_intrinsic_user_same_name hlslv
    tests/hlsl/intrinsics/vp_user_same_name.cg
    tests/hlsl/intrinsics/vp_user_same_name.expected)
add_hlsl_fixture(hlslf_texture_1d hlslf
    tests/hlsl/textures/fp_tex1d.cg
    tests/hlsl/textures/fp_tex1d.expected)
add_hlsl_fixture(hlslf_texture_2d hlslf
    tests/hlsl/textures/fp_tex2d.cg
    tests/hlsl/textures/fp_tex2d.expected)
add_hlsl_fixture(hlslf_texture_3d hlslf
    tests/hlsl/textures/fp_tex3d.cg
    tests/hlsl/textures/fp_tex3d.expected)
add_hlsl_fixture(hlslf_texture_cube hlslf
    tests/hlsl/textures/fp_texcube.cg
    tests/hlsl/textures/fp_texcube.expected)
add_hlsl_fixture(hlslf_texture_forms hlslf
    tests/hlsl/textures/fp_project_bias_lod_grad.cg
    tests/hlsl/textures/fp_project_bias_lod_grad.expected)
add_hlsl_fixture(hlslv_texture_lod hlslv
    tests/hlsl/textures/vp_tex2dlod.cg
    tests/hlsl/textures/vp_tex2dlod.expected)
add_hlsl_fixture(hlslf_texture_sampler_helper_parameter hlslf
    tests/hlsl/textures/fp_sampler_helper_parameter.cg
    tests/hlsl/textures/fp_sampler_helper_parameter.expected)

function(add_hlsl_link_test name vertex_source fragment_source)
    add_test(
        NAME ${name}
        COMMAND ${CMAKE_COMMAND}
            -DCGC=$<TARGET_FILE:cgc>
            -DVERTEX_SOURCE=${PROJECT_SOURCE_DIR}/${vertex_source}
            -DFRAGMENT_SOURCE=${PROJECT_SOURCE_DIR}/${fragment_source}
            "-DCONFIG=$<CONFIG>"
            "-DVERTEX_OUTPUT=${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/${name}.vs.hlsl"
            "-DFRAGMENT_OUTPUT=${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/${name}.ps.hlsl"
            -P ${CMAKE_CURRENT_SOURCE_DIR}/check_hlsl_link.cmake)
endfunction()

add_hlsl_link_test(hlsl_link_interface
    tests/hlsl/link/vp_link.cg tests/hlsl/link/fp_link.cg)
add_hlsl_link_test(hlsl_link_interface_mismatch
    tests/hlsl/link/vp_link_mismatch.cg
    tests/hlsl/link/fp_link_mismatch.cg)
set_tests_properties(hlsl_link_interface_mismatch PROPERTIES WILL_FAIL TRUE)
set_property(TEST hlsl_link_interface_mismatch APPEND PROPERTY
    ENVIRONMENT "HLSL_EXPECT_LINK_MISMATCH=COLOR0 type mismatch")

add_test(NAME hlsl_link_fresh_directory
    COMMAND ${CMAKE_COMMAND}
        -DMODE=link
        -DCGC=$<TARGET_FILE:cgc>
        "-DCONFIG=$<CONFIG>"
        "-DWORK_DIR=${CMAKE_CURRENT_BINARY_DIR}/fresh-link"
        "-DTEST_BINARY_ROOT=${CMAKE_CURRENT_BINARY_DIR}"
        -DVERTEX_SOURCE=${PROJECT_SOURCE_DIR}/tests/hlsl/link/vp_link.cg
        -DFRAGMENT_SOURCE=${PROJECT_SOURCE_DIR}/tests/hlsl/link/fp_link.cg
        -P ${CMAKE_CURRENT_SOURCE_DIR}/check_hlsl_fresh_directory.cmake)

if(FXC_EXECUTABLE)
    add_test(NAME hlsl_validate_fresh_directory
        COMMAND ${CMAKE_COMMAND}
            -DMODE=validate
            -DCGC=$<TARGET_FILE:cgc>
            -DFXC=${FXC_EXECUTABLE}
            -DPROFILE=hlslv
            -DTARGET=vs_3_0
            "-DCONFIG=$<CONFIG>"
            "-DWORK_DIR=${CMAKE_CURRENT_BINARY_DIR}/fresh-validate"
            "-DTEST_BINARY_ROOT=${CMAKE_CURRENT_BINARY_DIR}"
            -DSOURCE=${PROJECT_SOURCE_DIR}/tests/hlsl/profile/vp_passthrough.cg
            -P ${CMAKE_CURRENT_SOURCE_DIR}/check_hlsl_fresh_directory.cmake)
endif()

add_test(NAME hlsl_fresh_directory_cleanup_safety
    COMMAND ${CMAKE_COMMAND}
        "-DTEST_BINARY_ROOT=${CMAKE_CURRENT_BINARY_DIR}/fresh-safety/$<CONFIG>"
        "-DSCRIPT=${CMAKE_CURRENT_SOURCE_DIR}/check_hlsl_fresh_directory.cmake"
        -P ${CMAKE_CURRENT_SOURCE_DIR}/check_hlsl_fresh_directory_safety.cmake)

add_test(NAME hlsl_metadata_direction_unit
    COMMAND ${CMAKE_COMMAND}
        "-DWORK_DIR=${CMAKE_CURRENT_BINARY_DIR}/metadata-direction/$<CONFIG>"
        -P ${CMAKE_CURRENT_SOURCE_DIR}/check_hlsl_oracle_normalization.cmake)

add_test(NAME hlsl_positive_oracle_contract_unit
    COMMAND ${CMAKE_COMMAND}
        -DCGC=$<TARGET_FILE:cgc>
        "-DWORK_DIR=${CMAKE_CURRENT_BINARY_DIR}/positive-oracle/$<CONFIG>"
        -DVALID_EMPTY=${PROJECT_SOURCE_DIR}/tests/hlsl/profile/empty.cg
        -DINVALID_SOURCE=${PROJECT_SOURCE_DIR}/tests/hlsl/diagnostics/vp_missing_position.cg
        -DMISSING_SOURCE=${PROJECT_SOURCE_DIR}/tests/hlsl/oracle/missing.cg
        -P ${CMAKE_CURRENT_SOURCE_DIR}/check_hlsl_oracle_positive.cmake)

function(add_hlsl_failure name profile source code line reason)
    add_test(
        NAME ${name}
        COMMAND ${CMAKE_COMMAND}
            -DCGC=$<TARGET_FILE:cgc>
            -DPROFILE=${profile}
            -DSOURCE=${PROJECT_SOURCE_DIR}/${source}
            "-DCONFIG=$<CONFIG>"
            "-DACTUAL=${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/${name}.hlsl"
            -DCODE=${code}
            -DEXPECTED_LINE=${line}
            -DMESSAGE=${reason}
            -P ${CMAKE_CURRENT_SOURCE_DIR}/check_hlsl_failure.cmake)
endfunction()

function(add_hlsl_limit_success name profile source)
    add_test(
        NAME ${name}
        COMMAND $<TARGET_FILE:cgc>
            -quiet
            -profile ${profile}
            -o ${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/${name}.hlsl
            ${PROJECT_SOURCE_DIR}/${source}
    )
    add_hlsl_validation(${name} ${profile} ${source})
endfunction()

function(add_hlsl_bank_boundary name source expected bank type count)
    add_test(
        NAME ${name}
        COMMAND ${CMAKE_COMMAND}
            -DCGC=$<TARGET_FILE:cgc>
            -DPROFILE=hlslv
            -DSOURCE=${PROJECT_SOURCE_DIR}/${source}
            -DEXPECTED=${PROJECT_SOURCE_DIR}/${expected}
            "-DACTUAL=${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/${name}.hlsl"
            "-DFXC=${FXC_EXECUTABLE}"
            -DBANK=${bank}
            -DTYPE=${type}
            -DCOUNT=${count}
            -P ${CMAKE_CURRENT_SOURCE_DIR}/check_hlsl_bank_boundary.cmake)
    add_hlsl_validation(${name} hlslv ${source})
endfunction()

add_hlsl_limit_success(hlslv_limit_inputs16 hlslv
    tests/hlsl/limits/vp_inputs16.cg)
add_hlsl_failure(hlslv_limit_inputs17 hlslv
    tests/hlsl/limits/vp_inputs17.cg 6408 17
    "HLSL inputs limit exceeded: 17 used, 16 available")
add_hlsl_limit_success(hlslv_limit_outputs12 hlslv
    tests/hlsl/limits/vp_outputs12.cg)
add_hlsl_failure(hlslv_limit_outputs13 hlslv
    tests/hlsl/limits/vp_outputs13.cg 6408 14
    "HLSL outputs limit exceeded: 13 used, 12 available")
add_hlsl_limit_success(hlslf_limit_inputs10 hlslf
    tests/hlsl/limits/fp_inputs10.cg)
add_hlsl_failure(hlslf_limit_inputs11 hlslf
    tests/hlsl/limits/fp_inputs11.cg 6408 11
    "HLSL inputs limit exceeded: 11 used, 10 available")
add_hlsl_limit_success(hlslv_limit_constants256 hlslv
    tests/hlsl/limits/vp_constants256.cg)
add_hlsl_failure(hlslv_limit_constants257 hlslv
    tests/hlsl/limits/vp_constants257.cg 6408 2
    "HLSL c limit exceeded: 257 used, 256 available")
add_hlsl_limit_success(hlslf_limit_constants224 hlslf
    tests/hlsl/limits/fp_constants224.cg)
add_hlsl_failure(hlslf_limit_constants225 hlslf
    tests/hlsl/limits/fp_constants225.cg 6408 2
    "HLSL c limit exceeded: 225 used, 224 available")
add_hlsl_bank_boundary(hlslv_limit_int16
    tests/hlsl/limits/vp_int16.cg
    tests/hlsl/limits/vp_int16.expected i int4 16)
add_hlsl_failure(hlslv_limit_int17 hlslv
    tests/hlsl/limits/vp_int17.cg 6408 10
    "HLSL i limit exceeded: 17 used, 16 available")
add_hlsl_bank_boundary(hlslv_limit_bool16
    tests/hlsl/limits/vp_bool16.cg
    tests/hlsl/limits/vp_bool16.expected b bool 16)
add_hlsl_failure(hlslv_limit_bool17 hlslv
    tests/hlsl/limits/vp_bool17.cg 6408 9
    "HLSL b limit exceeded: 17 used, 16 available")
add_hlsl_limit_success(hlslv_limit_samplers4 hlslv
    tests/hlsl/limits/vp_samplers4.cg)
add_hlsl_failure(hlslv_limit_samplers5 hlslv
    tests/hlsl/limits/vp_samplers5.cg 6408 6
    "HLSL s limit exceeded: 5 used, 4 available")
add_hlsl_limit_success(hlslf_limit_samplers16 hlslf
    tests/hlsl/limits/fp_samplers16.cg)
add_hlsl_failure(hlslf_limit_samplers17 hlslf
    tests/hlsl/limits/fp_samplers17.cg 6408 18
    "HLSL s limit exceeded: 17 used, 16 available")
add_hlsl_limit_success(hlslf_limit_colors4 hlslf
    tests/hlsl/limits/fp_colors4.cg)
add_hlsl_failure(hlslf_limit_colors5 hlslf
    tests/hlsl/limits/fp_colors5.cg 6408 6
    "HLSL color outputs limit exceeded: 5 used, 4 available")
add_hlsl_failure(hlslf_limit_colors5_alias_lowercase hlslf
    tests/hlsl/limits/fp_colors5_alias_lowercase.cg 6408 6
    "HLSL color outputs limit exceeded: 5 used, 4 available")
add_hlsl_failure(hlslf_limit_sparse_color4 hlslf
    tests/hlsl/limits/fp_sparse_color4.cg 6408 1
    "HLSL color outputs limit exceeded: 5 used, 4 available")

add_hlsl_failure(hlslv_diagnostic_unsupported_type hlslv
    tests/hlsl/diagnostics/vp_unsupported_type.cg 6400 2
    "HLSL Shader Model 3 does not support type")
add_hlsl_failure(hlslv_diagnostic_missing_position hlslv
    tests/hlsl/diagnostics/vp_missing_position.cg 6405 1
    "HLSL vertex entry must write POSITION0")
add_hlsl_failure(hlslv_diagnostic_entry_abi hlslv
    tests/hlsl/diagnostics/vp_empty_input_abi.cg 6406 1
    "HLSL entry interface cannot represent")
add_hlsl_failure(hlslv_diagnostic_position_wrong_width hlslv
    tests/hlsl/diagnostics/vp_position_wrong_width.cg 6403 2 POSITION)
add_hlsl_failure(hlslv_diagnostic_position_wrong_base hlslv
    tests/hlsl/diagnostics/vp_position_wrong_base.cg 6403 2 POSITION)

add_hlsl_failure(hlslv_texture_implicit_stage hlslv
    tests/hlsl/diagnostics/vp_implicit_texture.cg 6409 5 tex2D)
add_hlsl_failure(hlslf_sampler_unit_type_conflict hlslf
    tests/hlsl/diagnostics/fp_sampler_unit_type_conflict.cg 6409 2 TEXUNIT0)
add_hlsl_failure(hlslf_texture_sampler_coord_mismatch hlslf
    tests/hlsl/diagnostics/fp_sampler_coord_mismatch.cg 6409 4 tex2D)
add_hlsl_failure(hlslf_texture_bias_coord_mismatch hlslf
    tests/hlsl/diagnostics/fp_tex2dbias_coord_mismatch.cg 6409 4 tex2Dbias)
add_hlsl_failure(hlslf_texture_lod_coord_mismatch hlslf
    tests/hlsl/diagnostics/fp_tex2dlod_coord_mismatch.cg 6409 4 tex2Dlod)
add_hlsl_failure(hlslv_texture_lod_coord_mismatch hlslv
    tests/hlsl/diagnostics/vp_tex2dlod_coord_mismatch.cg 6409 5 tex2Dlod)
add_hlsl_failure(glslf_texture_sampler_coord_mismatch_unchanged glslf
    tests/hlsl/diagnostics/fp_sampler_coord_mismatch.cg 1102 4
    incompatible.type.for.parameter)
add_hlsl_failure(hlslf_texture_local_sampler hlslf
    tests/hlsl/diagnostics/fp_local_sampler.cg 1151 4 localImage)
add_hlsl_failure(hlslf_texture_sampler_array hlslf
    tests/hlsl/diagnostics/fp_sampler_array.cg 6409 1 sampler.array)
add_hlsl_failure(hlslf_texture_sampler_rect hlslf
    tests/cg20/samplers/rect.cg 6409 9 samplerRECT)

add_test(
    NAME generic_entry_inout_still_rejected
    COMMAND ${CMAKE_COMMAND}
        -DCGC=$<TARGET_FILE:cgc>
        -DPROFILE=generic
        -DSOURCE=${PROJECT_SOURCE_DIR}/tests/hlsl/profile/vp_out_inout.cg
        -DCODE=5046
        -DMESSAGE=cannot.be.in.out
        -P ${CMAKE_CURRENT_SOURCE_DIR}/check_cg20_failure.cmake)

function(add_hlsl_interface_failure name source code line reason)
    add_test(
        NAME ${name}
        COMMAND ${CMAKE_COMMAND}
            -DCGC=$<TARGET_FILE:cgc>
            -DPROFILE=hlslv
            -DSOURCE=${PROJECT_SOURCE_DIR}/${source}
            "-DCONFIG=$<CONFIG>"
            "-DACTUAL=${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/${name}.hlsl"
            -DCODE=${code}
            -DEXPECTED_LINE=${line}
            -DMESSAGE=${reason}
            -P ${CMAKE_CURRENT_SOURCE_DIR}/check_hlsl_failure.cmake
    )
endfunction()

add_hlsl_interface_failure(hlslv_interface_exact_output_conflict
    tests/hlsl/diagnostics/vp_exact_output_conflict.cg 6404 4 COLOR0)
add_hlsl_interface_failure(hlslv_interface_case_output_conflict
    tests/hlsl/diagnostics/vp_case_output_conflict.cg 6404 4 COLOR0)
add_hlsl_interface_failure(hlslv_interface_alias_output_conflict
    tests/hlsl/diagnostics/vp_alias_output_conflict.cg 6404 4 COLOR0)
add_hlsl_interface_failure(hlslv_interface_invalid_width
    tests/hlsl/diagnostics/vp_invalid_width.cg 6403 2 POSITION)
add_hlsl_interface_failure(hlslv_interface_unbound_input
    tests/hlsl/diagnostics/vp_unbound_input.cg 6403 3 missingSemantic)
add_hlsl_interface_failure(hlslv_binding_register_collision
    tests/hlsl/diagnostics/vp_register_collision.cg 6407 2 second)
add_hlsl_interface_failure(hlslv_intrinsic_derivative_stage
    tests/hlsl/diagnostics/vp_derivative.cg 6402 3 ddx)
add_hlsl_interface_failure(hlslv_intrinsic_bad_overload
    tests/hlsl/diagnostics/vp_bad_intrinsic.cg 6410 1
    "cross.*float2.*float2")
add_hlsl_interface_failure(hlslv_intrinsic_scalar_vector1_mismatch
    tests/hlsl/diagnostics/vp_bad_intrinsic_shape.cg 6410 1
    "dot.*float.*float1")
add_hlsl_interface_failure(hlslv_intrinsic_uint_kind
    tests/hlsl/diagnostics/vp_bad_intrinsic_uint.cg 6410 1 abs)
add_hlsl_interface_failure(hlslv_intrinsic_double_kind
    tests/hlsl/diagnostics/vp_bad_intrinsic_double.cg 6410 1 sqrt)
add_hlsl_interface_failure(hlslv_intrinsic_char_kind
    tests/hlsl/diagnostics/vp_bad_intrinsic_char.cg 6410 1 abs)
add_hlsl_interface_failure(hlslv_intrinsic_short_kind
    tests/hlsl/diagnostics/vp_bad_intrinsic_short.cg 6410 1 abs)
add_hlsl_interface_failure(hlslv_intrinsic_long_kind
    tests/hlsl/diagnostics/vp_bad_intrinsic_long.cg 6410 1 abs)
add_hlsl_interface_failure(hlslv_recursion
    tests/hlsl/diagnostics/vp_recursion.cg 6401 3 recursive.HLSL.helper)
add_hlsl_interface_failure(hlslv_nested_recursion
    tests/hlsl/diagnostics/vp_nested_recursion.cg 6401 10 recursive.HLSL.helper)
add_hlsl_interface_failure(hlslv_recursion_call_location
    tests/hlsl/diagnostics/vp_recursion_call_location.cg 6401 11 recursive.HLSL.helper)
add_hlsl_interface_failure(hlslv_inline_recursion
    tests/hlsl/diagnostics/vp_inline_recursion.cg 6401 4 recursive.HLSL.helper)
add_hlsl_interface_failure(hlslv_inline_mutual_recursion
    tests/hlsl/diagnostics/vp_inline_mutual_recursion.cg 6401 6 recursive.HLSL.helper)

add_generic_diagnostic(generic_hlsl_recursion_unchanged
    tests/hlsl/diagnostics/vp_recursion.cg 5005 3 recurse)
add_generic_regression(hlsl_inline_helper_unchanged
    tests/hlsl/functions/vp_inline_helper.cg
    49d0d0f7ee9e40c652333d2cce56438bf3f89cb9928d93a14bc6e548f4c97c76)

add_test(
    NAME generic_hlsl_conditional_side_effects_unchanged
    COMMAND ${CMAKE_COMMAND}
        -DCGC=$<TARGET_FILE:cgc>
        -DPROFILE=generic
        -DSOURCE=${PROJECT_SOURCE_DIR}/tests/hlsl/expressions/vp_side_effects.cg
        -DCODE=1052
        -DEXPECTED_LINE=31
        -DMESSAGE=side.effects
        -P ${CMAKE_CURRENT_SOURCE_DIR}/check_cg20_failure.cmake
)

file(GENERATE
    OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/hlslv_discard.cg"
    CONTENT [=[float4 main(float4 position : POSITION) : POSITION
{
    discard;
    return position;
}
]=]
)

add_test(
    NAME hlslv_discard
    COMMAND ${CMAKE_COMMAND}
        -DCGC=$<TARGET_FILE:cgc>
        -DPROFILE=hlslv
        "-DSOURCE=${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/hlslv_discard.cg"
        "-DCONFIG=$<CONFIG>"
        "-DACTUAL=${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/hlslv_discard.hlsl"
        -DCODE=6402
        -DEXPECTED_LINE=3
        -DMESSAGE=discard
        -P ${CMAKE_CURRENT_SOURCE_DIR}/check_hlsl_failure.cmake
)

add_test(
    NAME hlslv_interface_attr_texcoord_conflict
    COMMAND ${CMAKE_COMMAND}
        -DCGC=$<TARGET_FILE:cgc>
        -DPROFILE=hlslv
        -DSOURCE=${PROJECT_SOURCE_DIR}/tests/hlsl/diagnostics/vp_attr_texcoord_conflict.cg
        "-DCONFIG=$<CONFIG>"
        "-DACTUAL=${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/hlslv_interface_attr_texcoord_conflict.hlsl"
        -DCODE=6404
        -DEXPECTED_LINE=3
        -DMESSAGE=TEXCOORD0
        -P ${CMAKE_CURRENT_SOURCE_DIR}/check_hlsl_failure.cmake
)

file(GENERATE
    OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/hlslv_interface_global_input.cg"
    CONTENT [=[varying float4 globalTexcoord : TEXCOORD0;
float4 main(float4 position : POSITION) : POSITION
{
    return position + globalTexcoord;
}
]=]
)

add_test(
    NAME hlslv_interface_global_input
    COMMAND $<TARGET_FILE:cgc> -quiet -nocode -profile hlslv
        "${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/hlslv_interface_global_input.cg"
)

file(GENERATE
    OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/hlslv_interface_global_output.cg"
    CONTENT [=[varying float4 globalPosition : HPOS;
float4 main(float4 position : POSITION) : COLOR0
{
    globalPosition = position;
    return position;
}
]=]
)

add_test(
    NAME hlslv_interface_global_output
    COMMAND $<TARGET_FILE:cgc> -quiet -nocode -profile hlslv
        "${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/hlslv_interface_global_output.cg"
)

file(GENERATE
    OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/hlslv_interface_invalid_semantic.cg"
    CONTENT [=[struct VertexIn { float4 invalid : CLP0; };
struct VertexOut { float4 position : POSITION; };
VertexOut main(VertexIn input)
{
    VertexOut output;
    output.position = input.invalid;
    return output;
}
]=]
)

add_test(
    NAME hlslv_interface_invalid_semantic
    COMMAND ${CMAKE_COMMAND}
        -DCGC=$<TARGET_FILE:cgc>
        -DPROFILE=hlslv
        "-DSOURCE=${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/hlslv_interface_invalid_semantic.cg"
        "-DCONFIG=$<CONFIG>"
        "-DACTUAL=${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/hlslv_interface_invalid_semantic.hlsl"
        -DCODE=6403
        -DEXPECTED_LINE=1
        -DMESSAGE=CLP0
        -P ${CMAKE_CURRENT_SOURCE_DIR}/check_hlsl_failure.cmake
)

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

# Core 1.50 portable limits: three samplers sit below the 16 texture
# units per stage, so this former GLSL 1.10 one-past fixture now
# compiles cleanly; it doubles as a core-1.50 syntax contract check.
add_glsl150_contract(glslf_third_sampler glslf
    tests/glsl/textures/fp_third_sampler.cg)

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

# The three-stage pipeline runners are declared with the GLSL link
# fixtures below; expose their stable names to this earlier manifest
# snapshot just like the other externally registered witnesses.
foreach(cg20_pipeline_test glsl_pipeline_geometry
        glsl_pipeline_vertex_id glsl_pipeline_type_mismatch)
    set_property(GLOBAL APPEND PROPERTY CG20_TEST_NAMES
        "${cg20_pipeline_test}")
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

function(add_glsl_geometry_pipeline name vertex geometry fragment)
    set(pipeline_args
        -DCGC=$<TARGET_FILE:cgc>
        "-DVALIDATOR=${GLSLANG_VALIDATOR}"
        -DVERTEX_SOURCE=${PROJECT_SOURCE_DIR}/${vertex}
        -DGEOMETRY_SOURCE=${PROJECT_SOURCE_DIR}/${geometry}
        -DFRAGMENT_SOURCE=${PROJECT_SOURCE_DIR}/${fragment}
        "-DGEOMETRY_OPTIONS=Vertices=3"
        "-DVERTEX_OUTPUT=${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/${name}.vert"
        "-DGEOMETRY_OUTPUT=${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/${name}.geom"
        "-DFRAGMENT_OUTPUT=${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/${name}.frag")
    add_cg20_success(${name} ${pipeline_args} ${ARGN}
        -P ${CMAKE_CURRENT_SOURCE_DIR}/check_glsl_pipeline.cmake)
    if(NOT GLSLANG_VALIDATOR)
        set_tests_properties(${name} PROPERTIES DISABLED TRUE)
    endif()
endfunction()

add_glsl_geometry_pipeline(glsl_pipeline_geometry
    tests/glsl/link/vp_geometry_link.cg
    tests/glsl/link/gp_geometry_link.cg
    tests/glsl/link/fp_geometry_link.cg
    -DCHECK_POINT_SIZE_BRIDGE=ON)
add_glsl_geometry_pipeline(glsl_pipeline_vertex_id
    tests/glsl/link/vp_vertex_id.cg
    tests/glsl/link/gp_vertex_id.cg
    tests/glsl/link/fp_vertex_id.cg
    -DCHECK_VERTEX_ID_BRIDGE=ON)
add_glsl_geometry_pipeline(glsl_pipeline_type_mismatch
    tests/glsl/link/vp_geometry_link.cg
    tests/glsl/link/gp_type_mismatch.cg
    tests/glsl/link/fp_type_mismatch.cg
    -DEXPECT_LINK_FAILURE=ON
    "-DLINK_FAILURE_PATTERN=Types.must.match")
add_glsl_geometry_pipeline(glsl_pipeline_interpolation_mismatch
    tests/glsl/link/vp_vertex_id.cg
    tests/glsl/link/gp_vertex_id.cg
    tests/glsl/link/fp_vertex_id.cg
    -DINJECT_INTERPOLATION_MISMATCH=ON
    -DEXPECT_LINK_FAILURE=ON
    "-DLINK_FAILURE_PATTERN=interpolation.qualifiers.do.not.match")
add_glsl_validation(glsl_pipeline_geometry_vertex glslv vert
    tests/glsl/link/vp_geometry_link.cg)
add_glsl_geometry_validation(glsl_pipeline_geometry_geometry
    tests/glsl/link/gp_geometry_link.cg main "Vertices=3")
add_glsl_validation(glsl_pipeline_geometry_fragment glslf frag
    tests/glsl/link/fp_geometry_link.cg)
add_glsl_validation(glsl_pipeline_vertex_id_vertex glslv vert
    tests/glsl/link/vp_vertex_id.cg)
add_glsl_geometry_validation(glsl_pipeline_vertex_id_geometry
    tests/glsl/link/gp_vertex_id.cg main "Vertices=3")
add_glsl_validation(glsl_pipeline_vertex_id_fragment glslf frag
    tests/glsl/link/fp_vertex_id.cg)
add_glsl_geometry_validation(glsl_pipeline_type_mismatch_geometry
    tests/glsl/link/gp_type_mismatch.cg main "Vertices=3")
add_glsl_validation(glsl_pipeline_type_mismatch_fragment glslf frag
    tests/glsl/link/fp_type_mismatch.cg)

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
# Updated again by "Resolve Cg geometry operations": the three new
# intrinsic-name atoms (emitVertex/flatAttrib/restartStrip) shift the
# same tie-break once more; re-verified as order-only with the identical
# member/addition swap against the pre-change build.
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
# Core 1.50 portable limits: 36 vertex output components sit below the
# 64-component minimum, so this former GLSL 1.10 one-past fixture now
# compiles cleanly; it doubles as a core-1.50 syntax contract check.
add_glsl150_contract(glslv_varyings_limit glslv
    tests/glsl/limits/vp_varyings36.cg)
add_glsl_diagnostic(glslf_uniforms_limit glslf
    tests/glsl/limits/fp_uniforms1025.cg 6207 3
    "fragment.uniform.components.*1025.used,.1024.available")
# Core 1.50 portable limits: fp_samplers3 uses three of the sixteen
# texture units per stage, so this former GLSL 1.10 one-past fixture
# now compiles cleanly; it doubles as a core-1.50 syntax contract
# check.  The new one-past boundary lives in glslf_seventeen_samplers.
add_glsl150_contract(glslf_samplers_limit glslf
    tests/glsl/limits/fp_samplers3.cg)
add_glsl_profile_failure(glslf_seventeen_samplers glslf
    tests/glsl/textures/fp_samplers17.cg 6207 19
    "17.used,.16.available")
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
    "GLSL.profile.program")
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
    NAME glslv_uniform_components_1025
    COMMAND ${CMAKE_COMMAND}
        -DCGC=$<TARGET_FILE:cgc>
        -DPROFILE=glslv
        -DSOURCE=${PROJECT_SOURCE_DIR}/tests/glsl/limits/vp_uniform_components_1025.cg
        "-DCONFIG=$<CONFIG>"
        "-DACTUAL=${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/glslv_uniform_components_1025.glsl"
        -DCODE=6207
        -DEXPECTED_LINE=14
        -DREASON=1025.used,.1024.available
        -P ${CMAKE_CURRENT_SOURCE_DIR}/check_glsl_failure.cmake
)
add_glsl_profile_failure(glslf_uniform_components_1025 glslf
    tests/glsl/limits/fp_uniform_components_1025.cg 6207 7
    "1025.used,.1024.available")
# Core 1.50 portable limits: 516 uniform components sit below the
# 1,024-component minimum, so this former GLSL 1.10 one-past fixture
# now compiles cleanly; it doubles as a core-1.50 syntax contract check.
add_glsl150_contract(glslv_uniform_components_516 glslv
    tests/glsl/limits/vp_uniform_components_516.cg)
add_test(
    NAME glslv_uniform_struct_components_1025
    COMMAND ${CMAKE_COMMAND}
        -DCGC=$<TARGET_FILE:cgc>
        -DPROFILE=glslv
        -DSOURCE=${PROJECT_SOURCE_DIR}/tests/glsl/limits/vp_uniform_struct_components_1025.cg
        "-DCONFIG=$<CONFIG>"
        "-DACTUAL=${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/glslv_uniform_struct_components_1025.glsl"
        -DCODE=6207
        -DEXPECTED_LINE=14
        -DREASON=1025.used,.1024.available
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

    function(add_hlsl_oracle name profile source)
        add_test(NAME hlsl_oracle_${name}
            COMMAND ${CMAKE_COMMAND}
                -DCGC_UNDER_TEST=$<TARGET_FILE:cgc>
                -DREFERENCE_CGC=${CGC_REFERENCE_EXECUTABLE}
                -DPROFILE=${profile}
                -DSOURCE=${PROJECT_SOURCE_DIR}/${source}
                -DENTRY=main
                -DWORK_DIR=${CMAKE_CURRENT_BINARY_DIR}/hlsl-oracle/$<CONFIG>
                -P ${CMAKE_CURRENT_SOURCE_DIR}/check_hlsl_oracle.cmake)
        set_tests_properties(hlsl_oracle_${name} PROPERTIES LABELS oracle)
    endfunction()

    add_hlsl_oracle(vp_uniform hlslv tests/hlsl/oracle/vp_uniform.cg)
    add_hlsl_oracle(fp_texture hlslf tests/hlsl/oracle/fp_texture.cg)
endif()

# Core GLSL 1.50 contract on representative vertex and fragment
# fixtures (Task 8 Step 1 registration; the checks read only fresh cgc
# output, so they pass once the compiler emits core 1.50 and fail while
# it still emits #version 110/attribute/varying/gl_FragColor).

add_glsl150_contract(glsl150_glslv_passthrough glslv
    tests/glsl/profile/vp_passthrough.cg)
add_glsl150_contract(glsl150_glslf_passthrough glslf
    tests/glsl/profile/fp_passthrough.cg)
add_glsl150_contract(glsl150_glslv_vertexlight glslv
    vertexlight.cg)
add_glsl150_contract(glsl150_glslf_tex2d glslf
    tests/glsl/textures/fp_tex2d.cg)
add_glsl150_contract(glsl150_glslv_position glslv
    position.cg)

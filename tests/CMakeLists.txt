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

function(add_cg20_success name source)
    add_test(
        NAME ${name}
        COMMAND ${CMAKE_COMMAND}
            -DCGC=$<TARGET_FILE:cgc>
            -DPROFILE=generic
            -DSOURCE=${PROJECT_SOURCE_DIR}/${source}
            -P ${CMAKE_CURRENT_SOURCE_DIR}/check_cg20.cmake
    )
endfunction()

add_cg20_success(cg20_smoke tests/cg20/profile/smoke.cg)
add_cg20_success(cg20_scalar_types tests/cg20/types/scalars.cg)
add_cg20_success(cg20_literal_suffixes tests/cg20/literals/suffixes.cg)
add_cg20_success(cg20_conversions_numeric tests/cg20/conversions/numeric.cg)
add_cg20_success(cg20_conversions_aggregate
    tests/cg20/conversions/aggregate_casts.cg)
add_cg20_success(cg20_unsized_array tests/cg20/arrays/unsized.cg)
add_cg20_success(cg20_arrays_nested_struct tests/cg20/arrays/nested_struct.cg)
add_cg20_success(cg20_array_length tests/cg20/arrays/length.cg)
add_cg20_success(cg20_sampler_base_compat
    tests/cg20/samplers/compatible_base.cg)
add_cg20_success(cg20_sampler_rect tests/cg20/samplers/rect.cg)
add_test(
    NAME cg20_sampler_assignment
    COMMAND ${CMAKE_COMMAND}
        -DCGC=$<TARGET_FILE:cgc>
        -DPROFILE=generic
        -DSOURCE=${PROJECT_SOURCE_DIR}/tests/cg20/diagnostics/sampler_assignment.cg
        -DCODE=1150
        -DMESSAGE=sampler.values
        -P ${CMAKE_CURRENT_SOURCE_DIR}/check_cg20_failure.cmake
)
add_test(
    NAME cg20_sampler_incompatible
    COMMAND ${CMAKE_COMMAND}
        -DCGC=$<TARGET_FILE:cgc>
        -DPROFILE=generic
        -DSOURCE=${PROJECT_SOURCE_DIR}/tests/cg20/diagnostics/sampler_incompatible.cg
        -DCODE=1102
        -DMESSAGE=incompatible.type.for.parameter
        -P ${CMAKE_CURRENT_SOURCE_DIR}/check_cg20_failure.cmake
)
add_test(
    NAME cg20_sampler_varying_global
    COMMAND ${CMAKE_COMMAND}
        -DCGC=$<TARGET_FILE:cgc>
        -DPROFILE=generic
        -DSOURCE=${PROJECT_SOURCE_DIR}/tests/cg20/diagnostics/sampler_varying_global.cg
        -DCODE=1151
        -DMESSAGE=sampler.variables.must.be.global.uniforms
        -P ${CMAKE_CURRENT_SOURCE_DIR}/check_cg20_failure.cmake
)
add_test(
    NAME cg20_invalid_conversion
    COMMAND ${CMAKE_COMMAND}
        -DCGC=$<TARGET_FILE:cgc>
        -DPROFILE=glslf
        -DSOURCE=${PROJECT_SOURCE_DIR}/tests/cg20/diagnostics/invalid_conversion.cg
        -DCODE=1033
        -DMESSAGE=cast.not.allowed
        -P ${CMAKE_CURRENT_SOURCE_DIR}/check_cg20_failure.cmake
)
add_generic_diagnostic(cg20_unsized_initializer
    tests/cg20/diagnostics/unsized_initializer.cg 1117 3
    "cannot.infer.array.size")
add_test(
    NAME cg20_reserved_word
    COMMAND ${CMAKE_COMMAND}
        -DCGC=$<TARGET_FILE:cgc>
        -DPROFILE=generic
        -DSOURCE=${PROJECT_SOURCE_DIR}/tests/cg20/diagnostics/reserved_word.cg
        -DCODE=1300
        -DMESSAGE=reserved.word
        -P ${CMAKE_CURRENT_SOURCE_DIR}/check_cg20_failure.cmake
)
add_test(
    NAME cg20_literal_overflow
    COMMAND ${CMAKE_COMMAND}
        -DCGC=$<TARGET_FILE:cgc>
        -DPROFILE=generic
        -DSOURCE=${PROJECT_SOURCE_DIR}/tests/cg20/diagnostics/literal_overflow.cg
        -DCODE=130
        -DMESSAGE="literal_overflow\\.cg\\(3\\).*literal_overflow\\.cg\\(4\\)"
        -P ${CMAKE_CURRENT_SOURCE_DIR}/check_cg20_failure.cmake
)
add_test(
    NAME cg20_default_domains
    COMMAND ${CMAKE_COMMAND}
        -DCGC=$<TARGET_FILE:cgc>
        -DPROFILE=generic
        -DSOURCE=${PROJECT_SOURCE_DIR}/tests/cg20/profile/default_domains.cg
        "-DMESSAGE=//semantic globalTint.*//var float4 globalTint.*-1 : 1.*//var float4 position : [$]vin[.]POSITION"
        -P ${CMAKE_CURRENT_SOURCE_DIR}/check_cg20.cmake
)
add_cg20_success(cg20_input_alias tests/cg20/profile/input_alias.cg)
add_cg20_success(cg20_global_varying tests/cg20/profile/global_varying.cg)
add_test(
    NAME cg20_output_alias
    COMMAND ${CMAKE_COMMAND}
        -DCGC=$<TARGET_FILE:cgc>
        -DPROFILE=generic
        -DSOURCE=${PROJECT_SOURCE_DIR}/tests/cg20/diagnostics/output_alias.cg
        -DCODE=5122
        -DMESSAGE=outputs.conflict.at.semantic
        -P ${CMAKE_CURRENT_SOURCE_DIR}/check_cg20_failure.cmake
)
add_test(
    NAME cg20_semantic_case_collision
    COMMAND ${CMAKE_COMMAND}
        -DCGC=$<TARGET_FILE:cgc>
        -DPROFILE=generic
        -DSOURCE=${PROJECT_SOURCE_DIR}/tests/cg20/diagnostics/semantic_case_collision.cg
        -DCODE=5122
        -DMESSAGE=outputs.conflict.at.semantic
        -P ${CMAKE_CURRENT_SOURCE_DIR}/check_cg20_failure.cmake
)
add_cg20_success(cg20_interface_conformance tests/cg20/interfaces/conformance.cg)
add_cg20_success(cg20_interface_dispatch tests/cg20/interfaces/dispatch.cg)
add_cg20_success(cg20_overload_defaults tests/cg20/overloads/defaults.cg)

# Cg 2.0 reachability fixtures (Task 16): reachable.cg compiles through
# generic; unreachable_profile_feature keeps a glslv-rejected operation
# inside an uncalled helper and must still generate code under glslv.

add_cg20_success(cg20_reachable tests/cg20/ir/reachable.cg)

# Compiler-level normalized golden (Task 17): under the default Cg 2.0
# language the generic profile emits the verified module as normalized
# Cg IR; reachable.cg pins its complete normalized output byte-for-byte.

add_test(
    NAME cg20_ir_normalized
    COMMAND ${CMAKE_COMMAND}
        -DCGC=$<TARGET_FILE:cgc>
        -DPROFILE=generic
        -DSOURCE=${PROJECT_SOURCE_DIR}/tests/cg20/ir/reachable.cg
        -DEXPECTED=${PROJECT_SOURCE_DIR}/tests/cg20/ir/normalized.expected
        "-DACTUAL=${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/cg20_ir_normalized.txt"
        -P ${CMAKE_CURRENT_SOURCE_DIR}/check_cg20.cmake
)
if(GLSLANG_VALIDATOR)
    set(cg20_unreachable_validator "-DVALIDATOR=${GLSLANG_VALIDATOR}")
else()
    set(cg20_unreachable_validator "")
endif()
add_test(
    NAME cg20_unreachable_profile_feature
    COMMAND ${CMAKE_COMMAND}
        -DCGC=$<TARGET_FILE:cgc>
        -DPROFILE=glslv
        -DSTAGE=vert
        -DENTRY=main
        -DSOURCE=${PROJECT_SOURCE_DIR}/tests/cg20/ir/unreachable_profile_feature.cg
        "-DCONFIG=$<CONFIG>"
        "-DOUTPUT=${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/cg20_unreachable_profile_feature.glsl"
        "-DMESSAGE=#version 110"
        ${cg20_unreachable_validator}
        -P ${CMAKE_CURRENT_SOURCE_DIR}/check_cg20_output.cmake
)

# Cg 2.0 standard-library fixtures (Task 13): the numeric and texture
# families compile through generic only; predefined_outputs exercises
# the fragment-family helper structures under glslf and rejects them
# under glslv.

add_cg20_success(cg20_intrinsic_numeric
    tests/cg20/intrinsics/numeric.cg)
add_cg20_success(cg20_intrinsic_out_params
    tests/cg20/intrinsics/out_params.cg)
add_cg20_success(cg20_intrinsic_texture_families
    tests/cg20/intrinsics/texture_families.cg)
add_cg20_success(cg20_intrinsic_debug
    tests/cg20/intrinsics/debug.cg)
# The fragment-family helper structures answer only under glslf; both
# rejecting profiles report the unknown-profile-specifier diagnostic at
# the first use of the undeclared structure name.

add_test(
    NAME cg20_intrinsic_predefined_outputs_glslf
    COMMAND ${CMAKE_COMMAND}
        -DCGC=$<TARGET_FILE:cgc>
        -DPROFILE=glslf
        -DSOURCE=${PROJECT_SOURCE_DIR}/tests/cg20/intrinsics/predefined_outputs.cg
        -P ${CMAKE_CURRENT_SOURCE_DIR}/check_cg20.cmake
)

set(cg20_predefined_outputs_source
    ${PROJECT_SOURCE_DIR}/tests/cg20/intrinsics/predefined_outputs.cg)
foreach(reject_profile glslv generic)
    add_test(
        NAME cg20_intrinsic_predefined_outputs_reject_${reject_profile}
        COMMAND ${CMAKE_COMMAND}
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
function(add_cg20_profile_selection name profile stage entry marker)
    if(GLSLANG_VALIDATOR)
        set(cg20_profile_validator "-DVALIDATOR=${GLSLANG_VALIDATOR}")
    else()
        set(cg20_profile_validator "")
    endif()
    add_test(
        NAME ${name}
        COMMAND ${CMAKE_COMMAND}
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

add_test(
    NAME cg20_default_after_optional
    COMMAND ${CMAKE_COMMAND}
        -DCGC=$<TARGET_FILE:cgc>
        -DPROFILE=generic
        -DSOURCE=${PROJECT_SOURCE_DIR}/tests/cg20/diagnostics/default_after_optional.cg
        -DCODE=1160
        -DMESSAGE=default.argument.for
        -P ${CMAKE_CURRENT_SOURCE_DIR}/check_cg20_failure.cmake
)
add_test(
    NAME cg20_ambiguous_overload
    COMMAND ${CMAKE_COMMAND}
        -DCGC=$<TARGET_FILE:cgc>
        -DPROFILE=generic
        -DSOURCE=${PROJECT_SOURCE_DIR}/tests/cg20/diagnostics/ambiguous_overload.cg
        -DCODE=1101
        -DMESSAGE=ambiguous.overloaded.function.reference
        -P ${CMAKE_CURRENT_SOURCE_DIR}/check_cg20_failure.cmake
)
add_test(
    NAME cg20_interface_data_member
    COMMAND ${CMAKE_COMMAND}
        -DCGC=$<TARGET_FILE:cgc>
        -DPROFILE=generic
        -DSOURCE=${PROJECT_SOURCE_DIR}/tests/cg20/diagnostics/interface_data.cg
        -DCODE=1155
        -DMESSAGE=data.members.not.allowed.in.interface
        -P ${CMAKE_CURRENT_SOURCE_DIR}/check_cg20_failure.cmake
)
add_test(
    NAME cg20_interface_missing_method
    COMMAND ${CMAKE_COMMAND}
        -DCGC=$<TARGET_FILE:cgc>
        -DPROFILE=generic
        -DSOURCE=${PROJECT_SOURCE_DIR}/tests/cg20/diagnostics/interface_missing_method.cg
        -DCODE=1156
        -DMESSAGE=does.not.implement.interface.method
        -P ${CMAKE_CURRENT_SOURCE_DIR}/check_cg20_failure.cmake
)
add_test(
    NAME cg20_interface_bad_signature
    COMMAND ${CMAKE_COMMAND}
        -DCGC=$<TARGET_FILE:cgc>
        -DPROFILE=generic
        -DSOURCE=${PROJECT_SOURCE_DIR}/tests/cg20/diagnostics/interface_bad_signature.cg
        -DCODE=1157
        -DMESSAGE=does.not.match.interface.method
        -P ${CMAKE_CURRENT_SOURCE_DIR}/check_cg20_failure.cmake
)
add_test(
    NAME cg20_manifest
    COMMAND ${CMAKE_COMMAND}
        -DMANIFEST=${CMAKE_CURRENT_SOURCE_DIR}/cg20/conformance.csv
        -P ${CMAKE_CURRENT_SOURCE_DIR}/check_cg20_manifest.cmake
)

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

add_glsl_profile_failure(glslf_local_sampler glslf
    tests/glsl/textures/fp_local_sampler.cg 6208 6
    "samplers.must.be.uniforms")

add_glsl_profile_failure(glslf_sampler_array glslf
    tests/glsl/textures/fp_sampler_array.cg 6208 3
    "sampler.arrays.or.aggregates")

add_glsl_profile_failure(glslf_sampler_default glslf
    tests/glsl/textures/fp_sampler_default.cg 1056 3
    "invalid.initialization")

add_glsl_profile_failure(glslf_sampler_helper_return glslf
    tests/glsl/textures/fp_sampler_helper_return.cg 6208 3
    "sampler.helper.result")

add_glsl_profile_failure(glslf_sampler_conditional glslf
    tests/glsl/textures/fp_sampler_conditional.cg 6208 7
    "texture.sampler.argument.must.be.a.direct.bound.uniform")

add_glsl_profile_failure(glslf_sampler_assignment glslf
    tests/glsl/textures/fp_sampler_assignment.cg 6208 7
    "opaque.sampler.expression")

add_glsl_profile_failure(glslf_sampler_cast glslf
    tests/glsl/textures/fp_sampler_cast.cg 1033 5
    "cast.not.allowed")

add_glsl_profile_failure(glslf_sampler_constructor glslf
    tests/glsl/textures/fp_sampler_constructor.cg 5502 5
    "requires.numeric.or.bool.elements")

add_glsl_profile_failure(glslf_sampler_comparison glslf
    tests/glsl/textures/fp_sampler_comparison.cg 1022 8
    "must.be.numeric")

add_glsl_profile_failure(glslf_sampler_helper_parameter glslf
    tests/glsl/textures/fp_sampler_helper_parameter.cg 6208 1
    "samplers.must.be.uniforms")

add_glsl_profile_failure(glslf_sampler_struct glslf
    tests/glsl/textures/fp_sampler_struct.cg 6208 1
    "samplers.must.be.uniforms")

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

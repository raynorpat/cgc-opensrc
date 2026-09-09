cmake_minimum_required(VERSION 3.16)

project(cgc LANGUAGES C)

include(CTest)
include(CheckIncludeFile)

check_include_file(stdint.h CGC_HAVE_STDINT_H)

set(CGC_HLSL_LOWER_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/hlsl_lower.c
    ${CMAKE_CURRENT_SOURCE_DIR}/hlsl_lower_support.c
    ${CMAKE_CURRENT_SOURCE_DIR}/hlsl_lower_decl.c
    ${CMAKE_CURRENT_SOURCE_DIR}/hlsl_lower_expr.c
    ${CMAKE_CURRENT_SOURCE_DIR}/hlsl_lower_stmt.c
    ${CMAKE_CURRENT_SOURCE_DIR}/hlsl_lower_geometry.c
    ${CMAKE_CURRENT_SOURCE_DIR}/hlsl_lower_function.c
)

set(CGC_GLSL_LOWER_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/glsl_lower.c
    ${CMAKE_CURRENT_SOURCE_DIR}/glsl_lower_support.c
    ${CMAKE_CURRENT_SOURCE_DIR}/glsl_lower_decl.c
    ${CMAKE_CURRENT_SOURCE_DIR}/glsl_lower_interface.c
    ${CMAKE_CURRENT_SOURCE_DIR}/glsl_lower_aggregate.c
    ${CMAKE_CURRENT_SOURCE_DIR}/glsl_lower_legacy_expr.c
    ${CMAKE_CURRENT_SOURCE_DIR}/glsl_lower_legacy_stmt.c
    ${CMAKE_CURRENT_SOURCE_DIR}/glsl_lower_ir_expr.c
    ${CMAKE_CURRENT_SOURCE_DIR}/glsl_lower_ir_stmt.c
    ${CMAKE_CURRENT_SOURCE_DIR}/glsl_lower_geometry.c
    ${CMAKE_CURRENT_SOURCE_DIR}/glsl_lower_function.c
)

set(CGC_ARB_LOWER_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/arb_lower.c
    ${CMAKE_CURRENT_SOURCE_DIR}/arb_lower_support.c
    ${CMAKE_CURRENT_SOURCE_DIR}/arb_lower_operand.c
    ${CMAKE_CURRENT_SOURCE_DIR}/arb_lower_expr.c
    ${CMAKE_CURRENT_SOURCE_DIR}/arb_lower_loop.c
    ${CMAKE_CURRENT_SOURCE_DIR}/arb_lower_stmt.c
)

set(CGC_MSL_LOWER_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/msl_lower.c
    ${CMAKE_CURRENT_SOURCE_DIR}/msl_lower_support.c
    ${CMAKE_CURRENT_SOURCE_DIR}/msl_lower_aggregate.c
    ${CMAKE_CURRENT_SOURCE_DIR}/msl_lower_texture.c
    ${CMAKE_CURRENT_SOURCE_DIR}/msl_lower_decl.c
    ${CMAKE_CURRENT_SOURCE_DIR}/msl_lower_expr.c
    ${CMAKE_CURRENT_SOURCE_DIR}/msl_lower_stmt.c
    ${CMAKE_CURRENT_SOURCE_DIR}/msl_lower_function.c
    ${CMAKE_CURRENT_SOURCE_DIR}/msl_bind.c
)

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()

add_executable(cgc
    msl_ir.c msl_verify.c msl_validate.c msl_codegen.c msl_hal.c mslv_hal.c mslf_hal.c
    ${CGC_MSL_LOWER_SOURCES}
    arb_hal.c
    arbfp1_hal.c
    arbvp1_hal.c
    atom.c
    binding.c
    cgcmain.c
    cg_geometry.c
    cg_ir.c
    cg_ir_lower.c
    cg_ir_default.c
    cg_ir_print.c
    cg_ir_verify.c
    cg_numeric.c
    cg_overload.c
    cg_reach.c
    cg_stdlib.c
    cg_types.c
    cgstruct.c
    check.c
    compile.c
    constfold.c
    cpp.c
    generic_hal.c
    glsl_ir.c
    glsl_verify.c
    ${CGC_GLSL_LOWER_SOURCES}
    glsl_codegen.c
    glsl_hal.c
    glslv_hal.c
    glslf_hal.c
    glslg_hal.c
    hal.c
    hlsl_bind.c
    hlsl_ir.c
    hlsl_modern.c
    ${CGC_HLSL_LOWER_SOURCES}
    hlsl_legalize.c
    hlsl_validate.c
    hlsl_codegen.c
    hlsl_hal.c
    hlslv_hal.c
    hlslf_hal.c
    hlslv40_hal.c
    hlslg40_hal.c
    hlslf40_hal.c
    hlslv50_hal.c
    hlslg50_hal.c
    hlslf50_hal.c
    inline.c
    language.c
    memory.c
    output.c
    parser.c
    printutils.c
    scanner.c
    semantic.c
    stdlib.c
    support.c
    support_iter.c
    symbols.c
    tokens.c
    arb_ir.c
    ${CGC_ARB_LOWER_SOURCES}
    arb_codegen.c
)

add_executable(tokenize
    atom.c
    cg_numeric.c
    cgstruct.c
    language.c
    scanner.c
    tokenize.c
    tokens.c
)

set_target_properties(cgc tokenize PROPERTIES
    C_STANDARD 90
    C_STANDARD_REQUIRED YES
    C_EXTENSIONS YES
)

if(MSVC AND CMAKE_SIZEOF_VOID_P EQUAL 8)
    set_source_files_properties(scanner.c PROPERTIES
        COMPILE_OPTIONS /U_WIN32
    )
endif()

target_compile_definitions(cgc PRIVATE
    $<$<BOOL:${CGC_HAVE_STDINT_H}>:CGC_HAVE_STDINT_H>
    $<$<CONFIG:Debug>:CGC_DEBUG_THE_COMPILER>
)

if(BUILD_TESTING AND WIN32)
    add_executable(arb_smoke tests/arb_smoke.c)
    target_link_libraries(arb_smoke PRIVATE opengl32)
    set_target_properties(arb_smoke PROPERTIES
        C_STANDARD 90
        C_STANDARD_REQUIRED YES
        C_EXTENSIONS YES)
endif()
add_custom_target(regenerate_stdlib
    COMMAND ${CMAKE_COMMAND}
        -DTOKENIZE=$<TARGET_FILE:tokenize>
        -DINPUT=stdlib.cg
        -DOUTPUT=stdlib.c
        -DTEMP=${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/stdlib_generic.cg
        -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/RegenerateStdlib.cmake
    DEPENDS
        tokenize
        ${CMAKE_CURRENT_SOURCE_DIR}/stdlib.cg
        ${CMAKE_CURRENT_SOURCE_DIR}/cmake/RegenerateStdlib.cmake
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    VERBATIM
)

find_package(BISON QUIET)

if(BISON_FOUND)
    add_custom_target(regenerate_parser
        COMMAND ${BISON_EXECUTABLE}
            parser.y
            --defines=parser.h
            --output=parser.c
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/parser.y
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        VERBATIM
    )
else()
    add_custom_target(regenerate_parser
        COMMAND ${CMAKE_COMMAND} -E echo
            "Bison is required to regenerate parser.c and parser.h"
        COMMAND ${CMAKE_COMMAND} -E false
        VERBATIM
    )
endif()

if(BUILD_TESTING)
    # Compile the actual compiler/lowerers with a test-only fault seam. No
    # environment-controlled fault behavior is present in the production cgc.
    get_target_property(CGC_MSL_FAULT_SOURCES cgc SOURCES)
    add_executable(cgc_msl_fault ${CGC_MSL_FAULT_SOURCES})
    if(APPLE AND CMAKE_C_COMPILER_ID MATCHES "Clang")
        target_compile_options(cgc_msl_fault PRIVATE -g -fsanitize=address -fno-omit-frame-pointer)
        target_link_options(cgc_msl_fault PRIVATE -fsanitize=address)
    endif()
    target_compile_definitions(cgc_msl_fault PRIVATE CGC_MSL_FAULT_TEST
        $<$<BOOL:${CGC_HAVE_STDINT_H}>:CGC_HAVE_STDINT_H>)
    set_target_properties(cgc_msl_fault PROPERTIES
        C_STANDARD 90 C_STANDARD_REQUIRED YES C_EXTENSIONS YES)
endif()

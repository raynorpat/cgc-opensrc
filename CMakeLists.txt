cmake_minimum_required(VERSION 3.16)

project(cgc LANGUAGES C)

include(CTest)
include(CheckIncludeFile)

check_include_file(stdint.h CGC_HAVE_STDINT_H)

set(CGC_HLSL_LOWER_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/hlsl_lower.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/hlsl_lower_support.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/hlsl_lower_decl.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/hlsl_lower_expr.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/hlsl_lower_stmt.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/hlsl_lower_geometry.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/hlsl_lower_function.c
)

set(CGC_GLSL_LOWER_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/glsl_lower.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/glsl_lower_support.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/glsl_lower_decl.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/glsl_lower_interface.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/glsl_lower_aggregate.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/glsl_lower_legacy_expr.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/glsl_lower_legacy_stmt.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/glsl_lower_ir_expr.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/glsl_lower_ir_stmt.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/glsl_lower_geometry.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/glsl_lower_function.c
)

set(CGC_ARB_LOWER_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/arb_lower.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/arb_lower_support.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/arb_lower_operand.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/arb_lower_expr.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/arb_lower_loop.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/arb_lower_stmt.c
)

set(CGC_MSL_LOWER_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/msl_lower.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/msl_lower_support.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/msl_lower_aggregate.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/msl_lower_texture.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/msl_lower_decl.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/msl_lower_expr.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/msl_lower_stmt.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/msl_lower_function.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/msl_bind.c
)

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()

add_executable(cgc
    src/msl_ir.c src/msl_verify.c src/msl_validate.c src/msl_codegen.c src/msl_hal.c src/mslv_hal.c src/mslf_hal.c
    ${CGC_MSL_LOWER_SOURCES}
    src/arb_hal.c
    src/arbfp1_hal.c
    src/arbvp1_hal.c
    src/atom.c
    src/binding.c
    src/cgcmain.c
    src/cg_geometry.c
    src/cg_ir.c
    src/cg_ir_lower.c
    src/cg_ir_default.c
    src/cg_ir_print.c
    src/cg_ir_verify.c
    src/cg_numeric.c
    src/cg_overload.c
    src/cg_reach.c
    src/cg_stdlib.c
    src/cg_types.c
    src/cgstruct.c
    src/check.c
    src/compile.c
    src/constfold.c
    src/cpp.c
    src/generic_hal.c
    src/glsl_ir.c
    src/glsl_verify.c
    ${CGC_GLSL_LOWER_SOURCES}
    src/glsl_codegen.c
    src/glsl_hal.c
    src/glslv_hal.c
    src/glslf_hal.c
    src/glslg_hal.c
    src/hal.c
    src/hlsl_bind.c
    src/hlsl_ir.c
    src/hlsl_modern.c
    ${CGC_HLSL_LOWER_SOURCES}
    src/hlsl_legalize.c
    src/hlsl_validate.c
    src/hlsl_codegen.c
    src/hlsl_hal.c
    src/hlslv_hal.c
    src/hlslf_hal.c
    src/hlslv40_hal.c
    src/hlslg40_hal.c
    src/hlslf40_hal.c
    src/hlslv50_hal.c
    src/hlslg50_hal.c
    src/hlslf50_hal.c
    src/inline.c
    src/language.c
    src/memory.c
    src/output.c
    src/parser.c
    src/printutils.c
    src/scanner.c
    src/semantic.c
    src/stdlib.c
    src/support.c
    src/support_iter.c
    src/symbols.c
    src/tokens.c
    src/arb_ir.c
    ${CGC_ARB_LOWER_SOURCES}
    src/arb_codegen.c
)

add_executable(tokenize
    src/atom.c
    src/cg_numeric.c
    src/cgstruct.c
    src/language.c
    src/scanner.c
    src/tokenize.c
    src/tokens.c
)

set_target_properties(cgc tokenize PROPERTIES
    C_STANDARD 90
    C_STANDARD_REQUIRED YES
    C_EXTENSIONS YES
)

if(MSVC AND CMAKE_SIZEOF_VOID_P EQUAL 8)
    set_source_files_properties(src/scanner.c PROPERTIES
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
        -DOUTPUT=${CMAKE_CURRENT_SOURCE_DIR}/src/stdlib.c
        -DTEMP=${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/stdlib_generic.cg
        -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/RegenerateStdlib.cmake
    DEPENDS
        tokenize
        ${CMAKE_CURRENT_SOURCE_DIR}/src/stdlib.cg
        ${CMAKE_CURRENT_SOURCE_DIR}/cmake/RegenerateStdlib.cmake
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/src
    VERBATIM
)

find_package(BISON QUIET)

if(BISON_FOUND)
    add_custom_target(regenerate_parser
        COMMAND ${BISON_EXECUTABLE}
            parser.y
            --defines=parser.h
            --output=parser.c
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/parser.y
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/src
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

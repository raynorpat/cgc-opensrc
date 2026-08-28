cmake_minimum_required(VERSION 3.16)

project(cgc LANGUAGES C)

include(CTest)
include(CheckIncludeFile)

check_include_file(stdint.h CGC_HAVE_STDINT_H)

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()

add_executable(cgc
    arb_hal.c
    arbfp1_hal.c
    arbvp1_hal.c
    atom.c
    binding.c
    cgcmain.c
    cg_geometry.c
    cg_ir.c
    cg_ir_lower.c
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
    glsl_lower.c
    glsl_codegen.c
    glsl_hal.c
    glslv_hal.c
    glslf_hal.c
    glslg_hal.c
    hal.c
    hlsl_ir.c
    hlsl_codegen.c
    hlsl_hal.c
    hlslv_hal.c
    hlslf_hal.c
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
    arb_lower.c
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

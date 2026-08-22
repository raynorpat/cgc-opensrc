cmake_minimum_required(VERSION 3.16)

project(cgc LANGUAGES C)

include(CTest)
include(CheckIncludeFile)

check_include_file(stdint.h CGC_HAVE_STDINT_H)

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()

add_executable(cgc
    atom.c
    binding.c
    cgcmain.c
    cgstruct.c
    check.c
    compile.c
    constfold.c
    cpp.c
    generic_hal.c
    glsl_ir.c
    glsl_hal.c
    glslv_hal.c
    glslf_hal.c
    hal.c
    inline.c
    memory.c
    parser.c
    printutils.c
    scanner.c
    semantic.c
    stdlib.c
    support.c
    support_iter.c
    symbols.c
    tokens.c
)

add_executable(tokenize
    atom.c
    cgstruct.c
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

add_custom_target(regenerate_stdlib
    COMMAND ${CMAKE_COMMAND}
        -DTOKENIZE=$<TARGET_FILE:tokenize>
        -DINPUT=stdlib.cg
        -DOUTPUT=stdlib.c
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
            ${CMAKE_CURRENT_SOURCE_DIR}/parser.y
            --defines=${CMAKE_CURRENT_SOURCE_DIR}/parser.h
            --output=${CMAKE_CURRENT_SOURCE_DIR}/parser.c
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/parser.y
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

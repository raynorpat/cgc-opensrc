foreach(required_variable TOKENIZE INPUT SCRIPT CASE WORK_DIR)
    if(NOT DEFINED ${required_variable})
        message(FATAL_ERROR "${required_variable} must be defined")
    endif()
endforeach()

file(MAKE_DIRECTORY "${WORK_DIR}")
set(begin_marker "// CGC_GLSL_TEXTURES_BEGIN")
set(end_marker "// CGC_GLSL_TEXTURES_END")
file(READ "${INPUT}" malformed_source)
set(expect_success FALSE)

if(CASE STREQUAL "missing")
    string(REPLACE "${begin_marker}" "" malformed_source
        "${malformed_source}")
elseif(CASE STREQUAL "duplicated")
    set(malformed_source "${malformed_source}\n${malformed_source}")
elseif(CASE STREQUAL "reversed")
    string(REPLACE "${begin_marker}" "CGC_MARKER_PLACEHOLDER"
        malformed_source "${malformed_source}")
    string(REPLACE "${end_marker}" "${begin_marker}"
        malformed_source "${malformed_source}")
    string(REPLACE "CGC_MARKER_PLACEHOLDER" "${end_marker}"
        malformed_source "${malformed_source}")
elseif(CASE STREQUAL "nested")
    string(REPLACE "${begin_marker}"
        "${begin_marker}\n${begin_marker}" malformed_source
        "${malformed_source}")
    string(REPLACE "${end_marker}" "${end_marker}\n${end_marker}"
        malformed_source "${malformed_source}")
elseif(CASE STREQUAL "prefix")
    string(REPLACE "${begin_marker}" "prefix${begin_marker}"
        malformed_source "${malformed_source}")
elseif(CASE STREQUAL "suffix")
    string(REPLACE "${end_marker}" "${end_marker}_typo"
        malformed_source "${malformed_source}")
elseif(CASE STREQUAL "trailing_space")
    string(REPLACE "${begin_marker}" "${begin_marker} "
        malformed_source "${malformed_source}")
elseif(CASE STREQUAL "cr_only")
    string(REPLACE "\r\n" "\n" malformed_source "${malformed_source}")
    string(REPLACE "\n" "\r" malformed_source "${malformed_source}")
elseif(CASE STREQUAL "lf")
    set(expect_success TRUE)
    string(REPLACE "\r\n" "\n" malformed_source "${malformed_source}")
elseif(CASE STREQUAL "crlf")
    set(expect_success TRUE)
    string(REPLACE "\r\n" "\n" malformed_source "${malformed_source}")
    string(REPLACE "\n" "\r\n" malformed_source "${malformed_source}")
elseif(CASE STREQUAL "eof")
    set(expect_success TRUE)
    string(REPLACE "${end_marker}" "" malformed_source
        "${malformed_source}")
    string(REGEX REPLACE "[\r\n]+$" "" malformed_source
        "${malformed_source}")
    set(malformed_source "${malformed_source}\n${end_marker}")
else()
    message(FATAL_ERROR "Unknown marker case: ${CASE}")
endif()

set(malformed_input "${WORK_DIR}/${CASE}.cg")
set(generated_output "${WORK_DIR}/${CASE}.c")
set(generic_temp "${WORK_DIR}/${CASE}-generic.cg")
set(output_sentinel "existing output must remain unchanged\n")
file(WRITE "${malformed_input}" "${malformed_source}")
file(REMOVE "${generated_output}" "${generic_temp}")
file(WRITE "${generated_output}" "${output_sentinel}")

execute_process(
    COMMAND "${CMAKE_COMMAND}"
        -DTOKENIZE=${TOKENIZE}
        -DINPUT=${malformed_input}
        -DOUTPUT=${generated_output}
        -DTEMP=${generic_temp}
        -P ${SCRIPT}
    RESULT_VARIABLE regenerate_result
    OUTPUT_VARIABLE regenerate_output
    ERROR_VARIABLE regenerate_error
)

if(expect_success)
    if(NOT regenerate_result EQUAL 0)
        message(FATAL_ERROR
            "Valid ${CASE} marker boundaries failed regeneration:\n"
            "${regenerate_output}${regenerate_error}")
    endif()
    file(READ "${generated_output}" generated_contents)
    if(generated_contents STREQUAL output_sentinel OR
       NOT generated_contents MATCHES "stdlib_generic.cg")
        message(FATAL_ERROR
            "Valid ${CASE} marker boundaries did not generate both streams")
    endif()
else()
    if(regenerate_result EQUAL 0)
        message(FATAL_ERROR
            "Malformed ${CASE} markers unexpectedly regenerated stdlib.c")
    endif()
    file(READ "${generated_output}" preserved_output)
    if(NOT preserved_output STREQUAL output_sentinel)
        message(FATAL_ERROR
            "Malformed ${CASE} markers changed the existing output file")
    endif()
    if(EXISTS "${generic_temp}")
        message(FATAL_ERROR
            "Malformed ${CASE} markers wrote the generic temporary file")
    endif()
    if(NOT regenerate_error MATCHES "marker")
        message(FATAL_ERROR
            "Malformed ${CASE} markers produced the wrong diagnostic:\n${regenerate_error}")
    endif()
endif()

file(REMOVE "${malformed_input}" "${generic_temp}" "${generated_output}")

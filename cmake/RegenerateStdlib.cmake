foreach(required_variable TOKENIZE INPUT OUTPUT TEMP)
    if(NOT DEFINED ${required_variable})
        message(FATAL_ERROR "${required_variable} must be defined")
    endif()
endforeach()

execute_process(
    COMMAND "${TOKENIZE}" "${INPUT}"
    RESULT_VARIABLE tokenize_result
    OUTPUT_VARIABLE generated_source
    ERROR_VARIABLE tokenize_error
)

if(NOT tokenize_result EQUAL 0)
    message(FATAL_ERROR
        "Tokenizer failed with exit code ${tokenize_result}:\n${tokenize_error}"
    )
endif()

file(READ "${INPUT}" generic_source)
set(begin_marker "// CGC_GLSL_TEXTURES_BEGIN")
set(end_marker "// CGC_GLSL_TEXTURES_END")
string(FIND "${generic_source}" "${begin_marker}" begin_offset)
string(FIND "${generic_source}" "${end_marker}" end_offset)
if(begin_offset EQUAL -1 OR end_offset EQUAL -1 OR end_offset LESS begin_offset)
    message(FATAL_ERROR "GLSL texture prototype markers are missing or malformed")
endif()

string(LENGTH "${end_marker}" end_marker_length)
math(EXPR suffix_offset "${end_offset} + ${end_marker_length}")
string(SUBSTRING "${generic_source}" ${suffix_offset} 2 line_ending)
if(line_ending STREQUAL "\r\n")
    math(EXPR suffix_offset "${suffix_offset} + 2")
else()
    math(EXPR suffix_offset "${suffix_offset} + 1")
endif()
string(SUBSTRING "${generic_source}" 0 ${begin_offset} generic_prefix)
string(SUBSTRING "${generic_source}" ${suffix_offset} -1 generic_suffix)
file(WRITE "${TEMP}" "${generic_prefix}${generic_suffix}")

execute_process(
    COMMAND "${TOKENIZE}" "${TEMP}" "stdlib_generic.cg"
    RESULT_VARIABLE generic_result
    OUTPUT_VARIABLE generic_generated_source
    ERROR_VARIABLE generic_error
)
file(REMOVE "${TEMP}")

if(NOT generic_result EQUAL 0)
    message(FATAL_ERROR
        "Generic tokenizer failed with exit code ${generic_result}:\n${generic_error}"
    )
endif()

string(FIND "${generic_generated_source}" "unsigned char" generic_data_offset)
if(generic_data_offset EQUAL -1)
    message(FATAL_ERROR "Generic tokenizer output did not contain token data")
endif()
string(SUBSTRING "${generic_generated_source}" ${generic_data_offset} -1 generic_data)
file(WRITE "${OUTPUT}" "${generated_source}\n${generic_data}")

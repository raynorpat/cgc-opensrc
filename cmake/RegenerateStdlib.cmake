foreach(required_variable TOKENIZE INPUT OUTPUT TEMP)
    if(NOT DEFINED ${required_variable})
        message(FATAL_ERROR "${required_variable} must be defined")
    endif()
endforeach()

file(READ "${INPUT}" generic_source)
set(begin_marker "// CGC_GLSL_TEXTURES_BEGIN")
set(end_marker "// CGC_GLSL_TEXTURES_END")

function(validate_marker_line source marker label offset_result after_result)
    string(REGEX MATCHALL "${marker}" marker_matches "${source}")
    list(LENGTH marker_matches marker_count)
    if(NOT marker_count EQUAL 1)
        message(FATAL_ERROR
            "GLSL texture ${label} marker must occur exactly once as a standalone line")
    endif()

    string(FIND "${source}" "${marker}" marker_offset)
    string(LENGTH "${source}" source_length)
    string(LENGTH "${marker}" marker_length)
    if(marker_offset GREATER 0)
        math(EXPR previous_offset "${marker_offset} - 1")
        string(SUBSTRING "${source}" ${previous_offset} 1 previous_char)
        if(NOT previous_char STREQUAL "\n")
            message(FATAL_ERROR
                "GLSL texture ${label} marker must be a standalone line")
        endif()
    endif()

    math(EXPR marker_after "${marker_offset} + ${marker_length}")
    if(marker_after LESS source_length)
        string(SUBSTRING "${source}" ${marker_after} 1 next_char)
        if(next_char STREQUAL "\n")
            math(EXPR marker_after "${marker_after} + 1")
        elseif(next_char STREQUAL "\r")
            math(EXPR newline_offset "${marker_after} + 1")
            if(NOT newline_offset LESS source_length)
                message(FATAL_ERROR
                    "GLSL texture ${label} marker has a malformed line ending")
            endif()
            string(SUBSTRING "${source}" ${newline_offset} 1 newline_char)
            if(NOT newline_char STREQUAL "\n")
                message(FATAL_ERROR
                    "GLSL texture ${label} marker has a malformed line ending")
            endif()
            math(EXPR marker_after "${marker_after} + 2")
        else()
            message(FATAL_ERROR
                "GLSL texture ${label} marker must be a standalone line")
        endif()
    elseif(NOT marker_after EQUAL source_length)
        message(FATAL_ERROR "GLSL texture ${label} marker is malformed")
    endif()

    set(${offset_result} ${marker_offset} PARENT_SCOPE)
    set(${after_result} ${marker_after} PARENT_SCOPE)
endfunction()

validate_marker_line("${generic_source}" "${begin_marker}" "begin"
                     begin_offset begin_after)
validate_marker_line("${generic_source}" "${end_marker}" "end"
                     end_offset suffix_offset)
if(end_offset LESS begin_after)
    message(FATAL_ERROR
        "GLSL texture prototype markers must contain exactly one ordered begin/end pair")
endif()

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

string(LENGTH "${generic_source}" generic_source_length)
if(begin_offset EQUAL 0)
    set(generic_prefix "")
else()
    string(SUBSTRING "${generic_source}" 0 ${begin_offset} generic_prefix)
endif()
if(suffix_offset EQUAL generic_source_length)
    set(generic_suffix "")
else()
    string(SUBSTRING "${generic_source}" ${suffix_offset} -1 generic_suffix)
endif()
file(WRITE "${TEMP}" "${generic_prefix}${generic_suffix}")

execute_process(
    COMMAND "${TOKENIZE}" "${TEMP}" "stdlib_generic_cg"
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
string(REPLACE "stdlibgenericcg" "stdlibgeneric_cg"
               generic_data "${generic_data}")
string(REPLACE "\"stdlib_generic_cg\", // name"
               "\"stdlib_generic.cg\", // name"
               generic_data "${generic_data}")
file(WRITE "${OUTPUT}" "${generated_source}\n${generic_data}")

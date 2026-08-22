foreach(required_variable TOKENIZE INPUT OUTPUT)
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

file(WRITE "${OUTPUT}" "${generated_source}")

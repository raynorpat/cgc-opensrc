foreach(required_variable TOKENIZE INPUT SOURCE_DIR CASE)
    if(NOT DEFINED ${required_variable})
        message(FATAL_ERROR "${required_variable} must be defined")
    endif()
endforeach()

set(expect_success FALSE)
set(use_default FALSE)
if(CASE STREQUAL "default")
    set(expect_success TRUE)
    set(use_default TRUE)
    set(expected_name "tests_glsl_profile_empty_cg")
elseif(CASE STREQUAL "valid")
    set(expect_success TRUE)
    set(logical_name "validStream1")
    set(expected_name "validStream1")
elseif(CASE STREQUAL "valid_underscore")
    set(expect_success TRUE)
    set(logical_name "valid_stream")
    set(expected_name "validstream")
elseif(CASE STREQUAL "valid_leading_underscore")
    set(expect_success TRUE)
    set(logical_name "_stream")
    set(expected_name "stream")
elseif(CASE STREQUAL "leading_digit")
    set(logical_name "1stream")
elseif(CASE STREQUAL "quote")
    set(logical_name "bad\"name")
elseif(CASE STREQUAL "hyphen")
    set(logical_name "bad-name")
elseif(CASE STREQUAL "whitespace")
    set(logical_name "bad name")
elseif(CASE STREQUAL "empty")
    set(logical_name "")
elseif(CASE STREQUAL "overlong")
    string(REPEAT "a" 256 logical_name)
else()
    message(FATAL_ERROR "Unknown stream-name case: ${CASE}")
endif()

if(use_default)
    execute_process(
        COMMAND "${TOKENIZE}" "${INPUT}"
        WORKING_DIRECTORY "${SOURCE_DIR}"
        RESULT_VARIABLE tokenize_result
        OUTPUT_VARIABLE tokenize_output
        ERROR_VARIABLE tokenize_error
    )
else()
    execute_process(
        COMMAND "${TOKENIZE}" "${INPUT}" "${logical_name}"
        WORKING_DIRECTORY "${SOURCE_DIR}"
        RESULT_VARIABLE tokenize_result
        OUTPUT_VARIABLE tokenize_output
        ERROR_VARIABLE tokenize_error
    )
endif()

if(expect_success)
    if(NOT tokenize_result EQUAL 0)
        message(FATAL_ERROR
            "Valid ${CASE} stream name failed: ${tokenize_error}")
    endif()
    if(NOT tokenize_error STREQUAL "")
        message(FATAL_ERROR
            "Valid ${CASE} stream name wrote stderr: ${tokenize_error}")
    endif()
    if(NOT tokenize_output MATCHES
       "unsigned char ${expected_name}_tokendata\\[\\]")
        message(FATAL_ERROR
            "Valid ${CASE} token data used the wrong identifier")
    endif()
    if(NOT tokenize_output MATCHES "TokenStream ${expected_name}_stream")
        message(FATAL_ERROR
            "Valid ${CASE} token stream used the wrong identifier")
    endif()
else()
    if(tokenize_result EQUAL 0)
        message(FATAL_ERROR
            "Invalid ${CASE} stream name unexpectedly succeeded")
    endif()
    if(NOT tokenize_output STREQUAL "")
        message(FATAL_ERROR
            "Invalid ${CASE} stream name wrote generated source")
    endif()
    if(NOT tokenize_error MATCHES "stream.name")
        message(FATAL_ERROR
            "Invalid ${CASE} stream name produced the wrong diagnostic: ${tokenize_error}")
    endif()
endif()

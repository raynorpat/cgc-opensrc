foreach(required CGC SOURCE EXPECTED_SHA256)
    if(NOT DEFINED ${required})
        message(FATAL_ERROR "${required} must be defined")
    endif()
endforeach()

# The pinned hashes guard the historical tree-dump output, which stays
# the emission path for explicit -version 1.1 compiles; default-language
# (Cg 2.0) compiles emit normalized Cg IR instead.

execute_process(
    COMMAND "${CGC}" -quiet -version 1.1 -profile generic "${SOURCE}"
    RESULT_VARIABLE result
    OUTPUT_VARIABLE output
    ERROR_VARIABLE error
)

if(NOT result EQUAL 0)
    message(FATAL_ERROR "generic compile failed (${result}):\n${output}${error}")
endif()

if(NOT error STREQUAL "")
    message(FATAL_ERROR "generic compile emitted diagnostics:\n${error}")
endif()

string(REPLACE "\r\n" "\n" output "${output}")
string(REPLACE "\r" "\n" output "${output}")
string(REGEX REPLACE "(^|\n)// cgc version [^\n]*\n" "\\1" output "${output}")
string(REGEX REPLACE "(^|\n)// command line args:[^\n]*\n" "\\1" output "${output}")
string(SHA256 actual_sha256 "${output}")

if(NOT actual_sha256 STREQUAL EXPECTED_SHA256)
    message(FATAL_ERROR
        "generic output changed for ${SOURCE}\n"
        "expected: ${EXPECTED_SHA256}\n"
        "actual:   ${actual_sha256}"
    )
endif()

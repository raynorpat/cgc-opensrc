# Compiles one entry point of a success fixture with code generation
# and asserts a selection marker appears in the generated program.
# Optional VALIDATOR/STAGE additionally run glslangValidator on the
# output file.

foreach(required CGC PROFILE ENTRY SOURCE MESSAGE OUTPUT)
    if(NOT DEFINED ${required})
        message(FATAL_ERROR "${required} must be defined")
    endif()
endforeach()

get_filename_component(output_dir "${OUTPUT}" DIRECTORY)
file(MAKE_DIRECTORY "${output_dir}")
file(REMOVE "${OUTPUT}")

execute_process(
    COMMAND "${CGC}" -quiet -profile "${PROFILE}" -entry "${ENTRY}"
        -o "${OUTPUT}" "${SOURCE}"
    RESULT_VARIABLE result
    OUTPUT_VARIABLE cgc_stdout
    ERROR_VARIABLE cgc_stderr
)
if(NOT result EQUAL 0)
    message(FATAL_ERROR
        "profile selection fixture failed (${result})\nstdout:\n${cgc_stdout}\nstderr:\n${cgc_stderr}")
endif()
file(READ "${OUTPUT}" shader)
string(REPLACE "\r\n" "\n" shader "${shader}")
if(NOT shader MATCHES "${MESSAGE}")
    message(FATAL_ERROR
        "generated program did not select expected overload (missing '${MESSAGE}'):\n${shader}")
endif()

if(DEFINED VALIDATOR AND NOT VALIDATOR STREQUAL "")
    if(NOT DEFINED STAGE)
        message(FATAL_ERROR "VALIDATOR requires STAGE")
    endif()
    execute_process(
        COMMAND "${VALIDATOR}" -S "${STAGE}" "${OUTPUT}"
        RESULT_VARIABLE validator_result
        OUTPUT_VARIABLE validator_stdout
        ERROR_VARIABLE validator_stderr
    )
    if(NOT validator_result EQUAL 0)
        message(FATAL_ERROR
            "glslangValidator failed:\n${validator_stdout}${validator_stderr}")
    endif()
endif()

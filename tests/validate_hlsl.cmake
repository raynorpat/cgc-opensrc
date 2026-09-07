foreach(required CGC FXC PROFILE TARGET SOURCE OUTPUT BYTECODE CONFIG LEGACY_SYNTAX)
    if(NOT DEFINED ${required})
        message(FATAL_ERROR "${required} must be defined")
    endif()
endforeach()

include("${CMAKE_CURRENT_LIST_DIR}/check_config_output.cmake")
prepare_config_output("${OUTPUT}")
prepare_config_output("${BYTECODE}")
get_filename_component(source_directory "${SOURCE}" DIRECTORY)

file(REMOVE "${OUTPUT}" "${BYTECODE}")
set(cgc_arguments -quiet)
if(DEFINED DEFINES AND NOT DEFINES STREQUAL "")
    foreach(source_define IN LISTS DEFINES)
        list(APPEND cgc_arguments "-D${source_define}")
    endforeach()
endif()
if(DEFINED SUPPRESS_WARNINGS AND SUPPRESS_WARNINGS)
    list(APPEND cgc_arguments -nowarn)
endif()
if(DEFINED ENTRY AND NOT ENTRY STREQUAL "")
    list(APPEND cgc_arguments -entry "${ENTRY}")
endif()
if(DEFINED PROFILE_OPTIONS AND NOT PROFILE_OPTIONS STREQUAL "")
    foreach(profile_option IN LISTS PROFILE_OPTIONS)
        list(APPEND cgc_arguments -po "${profile_option}")
    endforeach()
endif()
list(APPEND cgc_arguments -profile "${PROFILE}" -o "${OUTPUT}" "${SOURCE}")
execute_process(
    COMMAND "${CGC}" ${cgc_arguments}
    WORKING_DIRECTORY "${source_directory}"
    RESULT_VARIABLE cgc_result
    OUTPUT_VARIABLE cgc_stdout
    ERROR_VARIABLE cgc_stderr)
if(NOT cgc_result EQUAL 0)
    message(FATAL_ERROR
        "${PROFILE} compile failed (${cgc_result}):\n"
        "${cgc_stdout}${cgc_stderr}")
endif()
if(NOT cgc_stdout STREQUAL "" OR NOT cgc_stderr STREQUAL "")
    message(FATAL_ERROR
        "${PROFILE} compile produced diagnostics:\n"
        "${cgc_stdout}${cgc_stderr}")
endif()
if(NOT EXISTS "${OUTPUT}")
    message(FATAL_ERROR "cgc did not create ${OUTPUT}")
endif()

# /Ges rejects the legal legacy sampler1D/sampler2D/sampler3D/samplerCUBE
# syntax required by DirectX 9 (X3086).  /Gec keeps that DX9-compatible
# syntax; modern profiles use strict /Ges.  /WX keeps every warning fatal.
if(LEGACY_SYNTAX)
    set(syntax_mode /Gec)
else()
    set(syntax_mode /Ges)
endif()
set(fxc_arguments /nologo /WX ${syntax_mode} /E main /T "${TARGET}")
if(DEFINED PACKING AND NOT PACKING STREQUAL "")
    list(APPEND fxc_arguments "${PACKING}")
endif()
list(APPEND fxc_arguments /Fo "${BYTECODE}" "${OUTPUT}")
execute_process(
    COMMAND "${FXC}" ${fxc_arguments}
    RESULT_VARIABLE fxc_result
    OUTPUT_VARIABLE fxc_stdout
    ERROR_VARIABLE fxc_stderr)
if(NOT fxc_result EQUAL 0)
    message(FATAL_ERROR
        "FXC rejected ${OUTPUT} (${fxc_result}):\n"
        "${fxc_stdout}${fxc_stderr}")
endif()
if(NOT EXISTS "${BYTECODE}")
    message(FATAL_ERROR "FXC did not create ${BYTECODE}")
endif()
file(SIZE "${BYTECODE}" bytecode_size)
if(bytecode_size EQUAL 0)
    message(FATAL_ERROR "FXC created empty bytecode ${BYTECODE}")
endif()

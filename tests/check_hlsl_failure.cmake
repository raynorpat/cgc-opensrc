foreach(required CGC PROFILE SOURCE CODE EXPECTED_LINE MESSAGE ACTUAL)
    if(NOT DEFINED ${required} OR "${${required}}" STREQUAL "")
        message(FATAL_ERROR "${required} must be defined")
    endif()
endforeach()

include("${CMAKE_CURRENT_LIST_DIR}/check_config_output.cmake")
prepare_config_output("${ACTUAL}")

file(REMOVE "${ACTUAL}" "${ACTUAL}.normalized")
set(cgc_entry_args)
if(DEFINED ENTRY AND NOT "${ENTRY}" STREQUAL "")
    list(APPEND cgc_entry_args -entry "${ENTRY}")
endif()
set(cgc_profile_args)
if(DEFINED PROFILE_OPTIONS)
    string(REPLACE "\\;" ";" hlsl_profile_options "${PROFILE_OPTIONS}")
    foreach(hlsl_option IN LISTS hlsl_profile_options)
        list(APPEND cgc_profile_args -po "${hlsl_option}")
    endforeach()
endif()
set(cgc_define_args)
if(DEFINED DEFINES)
    string(REPLACE "\\;" ";" hlsl_defines "${DEFINES}")
    foreach(hlsl_define IN LISTS hlsl_defines)
        list(APPEND cgc_define_args "-D${hlsl_define}")
    endforeach()
endif()
execute_process(
    COMMAND "${CGC}" -quiet -profile "${PROFILE}" ${cgc_entry_args}
        ${cgc_profile_args} ${cgc_define_args}
        -o "${ACTUAL}" "${SOURCE}"
    RESULT_VARIABLE result
    OUTPUT_VARIABLE stdout
    ERROR_VARIABLE stderr
)
if(result EQUAL 0)
    message(FATAL_ERROR
        "${PROFILE} unexpectedly lowered ${SOURCE} without a diagnostic")
endif()
set(diagnostics "${stdout}${stderr}")
string(REGEX MATCHALL "error C[0-9][0-9][0-9][0-9]:" matches
    "${diagnostics}")
list(LENGTH matches match_count)
if(NOT match_count EQUAL 1)
    message(FATAL_ERROR
        "${PROFILE} reported ${match_count} compiler errors:\n${diagnostics}")
endif()
string(REGEX MATCH "[^\r\n]*error C[0-9][0-9][0-9][0-9]:[^\r\n]*"
    primary_diagnostic "${diagnostics}")
if(NOT primary_diagnostic MATCHES "error C${CODE}:")
    message(FATAL_ERROR
        "${PROFILE} did not report C${CODE}:\n${diagnostics}")
endif()
if(DEFINED EXPECTED_LOCATION)
    set(expected_location "${EXPECTED_LOCATION}")
else()
    get_filename_component(source_name "${SOURCE}" NAME)
    set(expected_location "${source_name}(${EXPECTED_LINE})")
endif()
string(FIND "${primary_diagnostic}" "${expected_location}"
    location_index)
if(location_index EQUAL -1)
    message(FATAL_ERROR
        "${PROFILE} reported the wrong source line:\n${diagnostics}")
endif()
if(NOT primary_diagnostic MATCHES "${MESSAGE}")
    message(FATAL_ERROR
        "${PROFILE} did not match message ${MESSAGE}:\n${diagnostics}")
endif()
if(DEFINED NOTES)
    string(REPLACE "\\;" ";" NOTES "${NOTES}")
    string(REPLACE "\"" "" clean_diagnostics "${diagnostics}")
    set(remaining_notes "${clean_diagnostics}")
    foreach(note IN LISTS NOTES)
        string(FIND "${remaining_notes}" "${note}" note_index)
        if(note_index EQUAL -1)
            message(FATAL_ERROR
                "${PROFILE} did not report ordered note ${note}:\n${diagnostics}")
        endif()
        string(SUBSTRING "${remaining_notes}" ${note_index} -1
            remaining_notes)
    endforeach()
endif()
if(DEFINED NOTE_CODE)
    string(REGEX MATCHALL "notice C${NOTE_CODE}:" note_matches "${diagnostics}")
    list(LENGTH note_matches note_count)
    if(NOT note_count EQUAL 1)
        message(FATAL_ERROR
            "${PROFILE} did not report note C${NOTE_CODE} exactly once:\n${diagnostics}")
    endif()
endif()
if(DEFINED NOTE_LINE)
    get_filename_component(source_name "${SOURCE}" NAME)
    string(FIND "${diagnostics}" "${source_name}(${NOTE_LINE})" note_location)
    if(note_location EQUAL -1)
        message(FATAL_ERROR
            "${PROFILE} reported the note at the wrong source line:\n${diagnostics}")
    endif()
endif()
if(DEFINED NOTE_MESSAGE AND NOT diagnostics MATCHES "${NOTE_MESSAGE}")
    message(FATAL_ERROR
        "${PROFILE} did not match note message ${NOTE_MESSAGE}:\n${diagnostics}")
endif()
# Transactional output: HLSL validation happens before publication, so a
# failed compile may leave no content at all.  Comments can carry binding or
# default metadata and are therefore content too; never discard them here.
if(EXISTS "${ACTUAL}")
    file(SIZE "${ACTUAL}" published_size)
    file(READ "${ACTUAL}" published)
    string(REPLACE "\r\n" "\n" published "${published}")
    string(REPLACE "\r" "\n" published "${published}")
    if(published MATCHES "cgc-bind|cbuffer|Texture[A-Za-z0-9]*[ \t]|struct[ \t]+|[A-Za-z_][A-Za-z0-9_]*[ \t]*\\([^;]*\\)[ \t\r\n]*\\{")
        message(FATAL_ERROR
            "failed compilation published HLSL content:\n${published}")
    endif()
    if(NOT published_size EQUAL 0)
        message(FATAL_ERROR
            "failed compilation published a nonempty output file:\n${published}")
    endif()
endif()

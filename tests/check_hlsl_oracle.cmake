# check_hlsl_oracle.cmake - Compare HLSL acceptance and public metadata
# with an explicitly supplied NVIDIA Cg compiler.

foreach(required CGC_UNDER_TEST REFERENCE_CGC PROFILE SOURCE ENTRY WORK_DIR)
    if(NOT DEFINED ${required})
        message(FATAL_ERROR "${required} is required")
    endif()
endforeach()

file(MAKE_DIRECTORY "${WORK_DIR}")
get_filename_component(source_name "${SOURCE}" NAME_WE)
set(under_output "${WORK_DIR}/${source_name}_${PROFILE}_under.hlsl")
set(reference_output "${WORK_DIR}/${source_name}_${PROFILE}_reference.hlsl")
file(REMOVE "${under_output}" "${reference_output}")

execute_process(
    COMMAND "${CGC_UNDER_TEST}" -quiet -profile "${PROFILE}"
            -entry "${ENTRY}" -o "${under_output}" "${SOURCE}"
    RESULT_VARIABLE under_result
    OUTPUT_VARIABLE under_stdout
    ERROR_VARIABLE under_stderr)
execute_process(
    COMMAND "${REFERENCE_CGC}" -quiet -profile "${PROFILE}"
            -entry "${ENTRY}" -o "${reference_output}" "${SOURCE}"
    RESULT_VARIABLE reference_result
    OUTPUT_VARIABLE reference_stdout
    ERROR_VARIABLE reference_stderr)

if(NOT under_result EQUAL 0 AND NOT reference_result EQUAL 0)
    return()
endif()
if(under_result EQUAL 0 AND NOT reference_result EQUAL 0)
    message(FATAL_ERROR
        "HLSL oracle mismatch: under-test accepted, reference rejected\n"
        "reference diagnostics:\n${reference_stderr}${reference_stdout}")
endif()
if(reference_result EQUAL 0 AND NOT under_result EQUAL 0)
    message(FATAL_ERROR
        "HLSL oracle mismatch: reference accepted, under-test rejected\n"
        "under-test diagnostics:\n${under_stderr}${under_stdout}")
endif()
if(NOT EXISTS "${under_output}" OR NOT EXISTS "${reference_output}")
    message(FATAL_ERROR "HLSL oracle accepted without producing both outputs")
endif()

function(normalize_semantic input output)
    set(value "${input}")
    string(STRIP "${value}" value)
    string(REGEX REPLACE "^\\$(vin|vout)\\." "" value "${value}")
    string(TOUPPER "${value}" value)
    if(value STREQUAL "POSITION" OR value STREQUAL "HPOS")
        set(value POSITION0)
    elseif(value STREQUAL "COLOR" OR value STREQUAL "COL")
        set(value COLOR0)
    elseif(value MATCHES "^COL([0-9]+)$")
        set(value "COLOR${CMAKE_MATCH_1}")
    elseif(value STREQUAL "TEXCOORD")
        set(value TEXCOORD0)
    elseif(value MATCHES "^TEX([0-9]+)$")
        set(value "TEXCOORD${CMAKE_MATCH_1}")
    endif()
    set(${output} "${value}" PARENT_SCOPE)
endfunction()

function(normalize_defaults input output)
    set(value "${input}")
    string(REGEX REPLACE "[=,{}()]" " " value "${value}")
    string(REGEX REPLACE "[ \t]+" ";" parts "${value}")
    set(cleaned "")
    foreach(part ${parts})
        if(part STREQUAL "")
            continue()
        endif()
        if(part MATCHES "^-?[0-9]+\\.[0-9]+$")
            string(REGEX REPLACE "0+$" "" part "${part}")
            if(part MATCHES "\\.$")
                string(REGEX REPLACE "\\.$" "" part "${part}")
            endif()
        endif()
        list(APPEND cleaned "${part}")
    endforeach()
    string(JOIN " " value ${cleaned})
    set(${output} "${value}" PARENT_SCOPE)
endfunction()

function(read_public_records path output)
    file(READ "${path}" raw)
    string(REPLACE "\r\n" "\n" raw "${raw}")
    string(REGEX MATCHALL "(^|\n)//[ ]*(var|default|cgc-default|cgc-bind interface)[^\n]*"
           lines "${raw}")
    set(records "")
    foreach(line ${lines})
        string(STRIP "${line}" line)
        if(line MATCHES "^//[ ]*cgc-bind interface +(in|out) +([^ ]+) +(.+) +([^ ]+)$")
            set(public_name "${CMAKE_MATCH_2}")
            set(type_name "${CMAKE_MATCH_3}")
            normalize_semantic("${CMAKE_MATCH_4}" semantic)
            if(public_name STREQUAL "result")
                set(public_name "$result")
            endif()
            string(STRIP "${type_name}" type_name)
            string(REGEX REPLACE "[ \t]+" " " type_name "${type_name}")
            list(APPEND records
                "semantic|${public_name}|${type_name}|${semantic}")
        elseif(line MATCHES "^//[ ]*var +([^:]+):([^:]*):([^:]*):")
            set(type_and_public "${CMAKE_MATCH_1}")
            set(public_semantic "${CMAKE_MATCH_2}")
            set(register_name "${CMAKE_MATCH_3}")
            string(STRIP "${type_and_public}" type_and_public)
            string(REGEX MATCH "^(.+) +([^ ]+)$" split "${type_and_public}")
            set(type_name "${CMAKE_MATCH_1}")
            set(public_name "${CMAKE_MATCH_2}")
            string(REGEX REPLACE "^main\\." "" public_name
                   "${public_name}")
            string(STRIP "${type_name}" type_name)
            string(REGEX REPLACE "[ \t]+" " " type_name "${type_name}")
            string(STRIP "${register_name}" register_name)
            if(register_name MATCHES "^([cibs])\\[([0-9]+)\\]$")
                list(APPEND records
                    "register|${public_name}|${type_name}|${CMAKE_MATCH_1}${CMAKE_MATCH_2}")
            endif()
            string(STRIP "${public_semantic}" public_semantic)
            string(TOUPPER "${public_semantic}" binding_semantic)
            if(binding_semantic MATCHES "^([CIBS])([0-9]+)$")
                string(TOLOWER "${CMAKE_MATCH_1}" register_bank)
                list(APPEND records
                    "register|${public_name}|${type_name}|${register_bank}${CMAKE_MATCH_2}")
            elseif(binding_semantic MATCHES "^TEXUNIT([0-9]+)$")
                list(APPEND records
                    "register|${public_name}|${type_name}|s${CMAKE_MATCH_1}")
            elseif(NOT public_semantic STREQUAL "")
                normalize_semantic("${public_semantic}" semantic)
                if(public_name STREQUAL "main")
                    set(public_name "$result")
                endif()
                list(APPEND records
                    "semantic|${public_name}|${type_name}|${semantic}")
            endif()
        elseif(line MATCHES "^//[ ]*(cgc-default|default) +(.+)$")
            normalize_defaults("${CMAKE_MATCH_2}" default_record)
            list(APPEND records "default|${default_record}")
        endif()
    endforeach()
    list(REMOVE_DUPLICATES records)
    list(SORT records)
    set(${output} "${records}" PARENT_SCOPE)
endfunction()

read_public_records("${under_output}" under_records)
read_public_records("${reference_output}" reference_records)

if(NOT under_records STREQUAL reference_records)
    string(JOIN "\n  " under_text ${under_records})
    string(JOIN "\n  " reference_text ${reference_records})
    message(FATAL_ERROR
        "HLSL oracle public metadata mismatch\n"
        "under-test:\n  ${under_text}\n"
        "reference:\n  ${reference_text}")
endif()

list(LENGTH under_records record_count)
message(STATUS "HLSL oracle agreement on ${record_count} public records")

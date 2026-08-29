foreach(required CGC VERTEX_SOURCE FRAGMENT_SOURCE
                 VERTEX_OUTPUT FRAGMENT_OUTPUT)
    if(NOT DEFINED ${required})
        message(FATAL_ERROR "${required} must be defined")
    endif()
endforeach()

# The mismatch CTest remains WILL_FAIL as required, but this outer invocation
# turns an unrelated setup/compiler failure into success so WILL_FAIL exposes
# it.  Only the intended diagnostic is re-emitted as a failing result.
if(DEFINED ENV{HLSL_EXPECT_LINK_MISMATCH} AND NOT HLSL_LINK_CHILD)
    execute_process(
        COMMAND "${CMAKE_COMMAND}"
            -DCGC=${CGC}
            -DVERTEX_SOURCE=${VERTEX_SOURCE}
            -DFRAGMENT_SOURCE=${FRAGMENT_SOURCE}
            -DVERTEX_OUTPUT=${VERTEX_OUTPUT}
            -DFRAGMENT_OUTPUT=${FRAGMENT_OUTPUT}
            -DHLSL_LINK_CHILD=TRUE
            -P "${CMAKE_CURRENT_LIST_FILE}"
        RESULT_VARIABLE child_result
        OUTPUT_VARIABLE child_stdout
        ERROR_VARIABLE child_stderr)
    set(child_output "${child_stdout}${child_stderr}")
    if(child_result EQUAL 0)
        message(STATUS "mismatched HLSL interfaces unexpectedly linked")
        return()
    endif()
    if(NOT child_output MATCHES "$ENV{HLSL_EXPECT_LINK_MISMATCH}")
        message(STATUS "unexpected HLSL link failure:\n${child_output}")
        return()
    endif()
    string(REGEX MATCH
        "$ENV{HLSL_EXPECT_LINK_MISMATCH}[^\n]*" mismatch_diagnostic
        "${child_output}")
    message(FATAL_ERROR "${mismatch_diagnostic}")
endif()

include("${CMAKE_CURRENT_LIST_DIR}/check_config_output.cmake")
prepare_config_output("${VERTEX_OUTPUT}")
prepare_config_output("${FRAGMENT_OUTPUT}")

function(compile_hlsl profile source output)
    get_filename_component(source_directory "${source}" DIRECTORY)
    file(REMOVE "${output}")
    execute_process(
        COMMAND "${CGC}" -quiet -profile "${profile}"
            -o "${output}" "${source}"
        WORKING_DIRECTORY "${source_directory}"
        RESULT_VARIABLE result
        OUTPUT_VARIABLE stdout
        ERROR_VARIABLE stderr)
    if(NOT result EQUAL 0)
        message(FATAL_ERROR
            "${profile} compile failed (${result}):\n${stdout}${stderr}")
    endif()
    if(NOT stdout STREQUAL "" OR NOT stderr STREQUAL "")
        message(FATAL_ERROR
            "${profile} compile produced diagnostics:\n${stdout}${stderr}")
    endif()
    if(NOT EXISTS "${output}")
        message(FATAL_ERROR "${profile} did not create ${output}")
    endif()
endfunction()

function(read_interface file direction drop_position output_variable)
    file(STRINGS "${file}" interface_lines
        REGEX "^// cgc-bind interface (in|out) ")
    set(records "")
    foreach(line IN LISTS interface_lines)
        if(NOT line MATCHES
           "^// cgc-bind interface (in|out) ([^ ]+) ([^ ]+) ([^ ]+)$")
            message(FATAL_ERROR "malformed HLSL interface record: ${line}")
        endif()
        set(record_direction "${CMAKE_MATCH_1}")
        set(public_name "${CMAKE_MATCH_2}")
        set(type_name "${CMAKE_MATCH_3}")
        set(semantic "${CMAKE_MATCH_4}")
        if(NOT record_direction STREQUAL direction)
            continue()
        endif()
        if(drop_position AND semantic STREQUAL "POSITION0")
            continue()
        endif()
        if(NOT type_name MATCHES
           "^(float|half|fixed|int|bool)([1-4]?)$")
            message(FATAL_ERROR
                "unsupported HLSL link interface type ${type_name} at ${semantic}")
        endif()
        set(width "${CMAKE_MATCH_2}")
        if(width STREQUAL "")
            set(width 1)
        endif()
        list(APPEND records
            "${semantic}|${public_name}|${type_name}|${width}")
    endforeach()
    list(SORT records)
    set(${output_variable} "${records}" PARENT_SCOPE)
endfunction()

function(split_record record semantic public_name type_name width)
    if(NOT record MATCHES "^([^|]+)[|]([^|]+)[|]([^|]+)[|]([1-4])$")
        message(FATAL_ERROR "invalid normalized interface record: ${record}")
    endif()
    set(${semantic} "${CMAKE_MATCH_1}" PARENT_SCOPE)
    set(${public_name} "${CMAKE_MATCH_2}" PARENT_SCOPE)
    set(${type_name} "${CMAKE_MATCH_3}" PARENT_SCOPE)
    set(${width} "${CMAKE_MATCH_4}" PARENT_SCOPE)
endfunction()

compile_hlsl(hlslv "${VERTEX_SOURCE}" "${VERTEX_OUTPUT}")
compile_hlsl(hlslf "${FRAGMENT_SOURCE}" "${FRAGMENT_OUTPUT}")
read_interface("${VERTEX_OUTPUT}" out TRUE vertex_records)
read_interface("${FRAGMENT_OUTPUT}" in FALSE fragment_records)

list(LENGTH vertex_records vertex_count)
list(LENGTH fragment_records fragment_count)
if(vertex_count EQUAL 0)
    message(FATAL_ERROR "vertex output contains no linkable interface records")
endif()
if(fragment_count EQUAL 0)
    message(FATAL_ERROR "pixel input contains no linkable interface records")
endif()

foreach(vertex_record IN LISTS vertex_records)
    split_record("${vertex_record}"
        vertex_semantic vertex_public vertex_type vertex_width)
    set(found FALSE)
    foreach(fragment_record IN LISTS fragment_records)
        split_record("${fragment_record}"
            fragment_semantic fragment_public fragment_type fragment_width)
        if(vertex_semantic STREQUAL fragment_semantic)
            set(found TRUE)
            if(NOT vertex_public STREQUAL fragment_public)
                message(FATAL_ERROR
                    "${vertex_semantic} public-name mismatch: vertex "
                    "${vertex_public}, pixel ${fragment_public}")
            endif()
            if(NOT vertex_type STREQUAL fragment_type OR
               NOT vertex_width STREQUAL fragment_width)
                message(FATAL_ERROR
                    "${vertex_public} type mismatch at ${vertex_semantic}: "
                    "vertex ${vertex_type} width ${vertex_width}, pixel "
                    "${fragment_type} width ${fragment_width}")
            endif()
        endif()
    endforeach()
    if(NOT found)
        message(FATAL_ERROR
            "${vertex_semantic} missing from pixel input interface")
    endif()
endforeach()

foreach(fragment_record IN LISTS fragment_records)
    split_record("${fragment_record}"
        fragment_semantic fragment_public fragment_type fragment_width)
    set(found FALSE)
    foreach(vertex_record IN LISTS vertex_records)
        split_record("${vertex_record}"
            vertex_semantic vertex_public vertex_type vertex_width)
        if(fragment_semantic STREQUAL vertex_semantic)
            set(found TRUE)
        endif()
    endforeach()
    if(NOT found)
        message(FATAL_ERROR
            "${fragment_semantic} missing from vertex output interface")
    endif()
endforeach()

if(NOT vertex_count EQUAL fragment_count)
    message(FATAL_ERROR
        "HLSL interface count mismatch: vertex ${vertex_count}, "
        "pixel ${fragment_count}")
endif()

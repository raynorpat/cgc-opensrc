foreach(required CGC VERTEX_SOURCE FRAGMENT_SOURCE
                 VERTEX_OUTPUT FRAGMENT_OUTPUT CONFIG)
    if(NOT DEFINED ${required})
        message(FATAL_ERROR "${required} must be defined")
    endif()
endforeach()

if(NOT DEFINED VERTEX_PROFILE)
    set(VERTEX_PROFILE hlslv)
endif()
if(NOT DEFINED PIXEL_PROFILE)
    set(PIXEL_PROFILE hlslf)
endif()

set(has_geometry FALSE)
foreach(geometry_input GEOMETRY_PROFILE GEOMETRY_SOURCE GEOMETRY_OUTPUT)
    if(DEFINED ${geometry_input} AND NOT "${${geometry_input}}" STREQUAL "")
        set(has_geometry TRUE)
    endif()
endforeach()
if(has_geometry)
    foreach(required GEOMETRY_PROFILE GEOMETRY_SOURCE GEOMETRY_OUTPUT)
        if(NOT DEFINED ${required} OR "${${required}}" STREQUAL "")
            message(FATAL_ERROR
                "${required} must be defined for a geometry pipeline")
        endif()
    endforeach()
endif()

# The mismatch CTest remains WILL_FAIL as required, but this outer invocation
# turns an unrelated setup/compiler failure into success so WILL_FAIL exposes
# it.  Only the intended diagnostic is re-emitted as a failing result.
if(DEFINED ENV{HLSL_EXPECT_LINK_MISMATCH} AND NOT HLSL_LINK_CHILD)
    set(child_arguments
        -DCGC=${CGC}
        -DVERTEX_SOURCE=${VERTEX_SOURCE}
        -DFRAGMENT_SOURCE=${FRAGMENT_SOURCE}
        -DCONFIG=${CONFIG}
        -DVERTEX_OUTPUT=${VERTEX_OUTPUT}
        -DFRAGMENT_OUTPUT=${FRAGMENT_OUTPUT}
        -DVERTEX_PROFILE=${VERTEX_PROFILE}
        -DPIXEL_PROFILE=${PIXEL_PROFILE}
        -DHLSL_LINK_CHILD=TRUE)
    foreach(optional VERTEX_OPTIONS GEOMETRY_PROFILE GEOMETRY_SOURCE
                     GEOMETRY_OUTPUT GEOMETRY_OPTIONS PIXEL_OPTIONS
                     ASSERT_VERTEX_ID_BRIDGE)
        if(DEFINED ${optional})
            string(REPLACE ";" "\\;" optional_value "${${optional}}")
            list(APPEND child_arguments "-D${optional}=${optional_value}")
        endif()
    endforeach()
    execute_process(
        COMMAND "${CMAKE_COMMAND}" ${child_arguments}
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
if(has_geometry)
    prepare_config_output("${GEOMETRY_OUTPUT}")
endif()
prepare_config_output("${FRAGMENT_OUTPUT}")

function(compile_hlsl profile source output options)
    get_filename_component(source_directory "${source}" DIRECTORY)
    file(REMOVE "${output}")
    set(profile_arguments)
    if(NOT "${options}" STREQUAL "")
        string(REPLACE "\\;" ";" profile_options "${options}")
        foreach(option IN LISTS profile_options)
            list(APPEND profile_arguments -po "${option}")
        endforeach()
    endif()
    execute_process(
        COMMAND "${CGC}" -quiet -profile "${profile}"
            ${profile_arguments} -o "${output}" "${source}"
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

function(read_legacy_interface file direction drop_position output_variable)
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

function(split_legacy_record record semantic public_name type_name width)
    if(NOT record MATCHES "^([^|]+)[|]([^|]+)[|]([^|]+)[|]([1-4])$")
        message(FATAL_ERROR "invalid normalized interface record: ${record}")
    endif()
    set(${semantic} "${CMAKE_MATCH_1}" PARENT_SCOPE)
    set(${public_name} "${CMAKE_MATCH_2}" PARENT_SCOPE)
    set(${type_name} "${CMAKE_MATCH_3}" PARENT_SCOPE)
    set(${width} "${CMAKE_MATCH_4}" PARENT_SCOPE)
endfunction()

function(read_modern_interface file direction output_variable)
    file(STRINGS "${file}" interface_lines
        REGEX "^// cgc-bind interface (in|out) ")
    set(records "")
    foreach(line IN LISTS interface_lines)
        if(NOT line MATCHES
           "^// cgc-bind interface (in|out) ([^ ]+) ([^ ]+) ([^ ]+) ([^ ]+) (system|user) (default|linear|centroid|noperspective|nointerpolation)$")
            message(FATAL_ERROR
                "malformed modern HLSL interface record: ${line}")
        endif()
        if(NOT CMAKE_MATCH_1 STREQUAL direction)
            continue()
        endif()
        string(TOUPPER "${CMAKE_MATCH_5}" canonical_key)
        # Canonical identity is deliberately the first normalized field.
        # Public/source names and emitted SV spellings remain diagnostic data.
        list(APPEND records
            "${canonical_key}|${CMAKE_MATCH_2}|${CMAKE_MATCH_3}|${CMAKE_MATCH_4}|${CMAKE_MATCH_6}|${CMAKE_MATCH_7}")
    endforeach()
    list(SORT records)
    set(${output_variable} "${records}" PARENT_SCOPE)
endfunction()

function(split_modern_record record canonical public_name type_name
        emitted_semantic semantic_class interpolation)
    if(NOT record MATCHES
       "^([^|]+)[|]([^|]+)[|]([^|]+)[|]([^|]+)[|](system|user)[|](default|linear|centroid|noperspective|nointerpolation)$")
        message(FATAL_ERROR
            "invalid normalized modern interface record: ${record}")
    endif()
    set(${canonical} "${CMAKE_MATCH_1}" PARENT_SCOPE)
    set(${public_name} "${CMAKE_MATCH_2}" PARENT_SCOPE)
    set(${type_name} "${CMAKE_MATCH_3}" PARENT_SCOPE)
    set(${emitted_semantic} "${CMAKE_MATCH_4}" PARENT_SCOPE)
    set(${semantic_class} "${CMAKE_MATCH_5}" PARENT_SCOPE)
    set(${interpolation} "${CMAKE_MATCH_6}" PARENT_SCOPE)
endfunction()

function(format_modern_declaration record output_variable)
    split_modern_record("${record}" canonical public_name type_name
        emitted_semantic semantic_class interpolation)
    set(${output_variable}
        "${public_name} ${type_name} ${emitted_semantic} ${canonical} ${semantic_class} ${interpolation}"
        PARENT_SCOPE)
endfunction()

function(compare_modern_interfaces producer_records consumer_records boundary)
    foreach(consumer_record IN LISTS consumer_records)
        split_modern_record("${consumer_record}" consumer_key consumer_public
            consumer_type consumer_emitted consumer_class consumer_interpolation)
        set(found FALSE)
        foreach(producer_record IN LISTS producer_records)
            split_modern_record("${producer_record}" producer_key producer_public
                producer_type producer_emitted producer_class
                producer_interpolation)
            if(NOT producer_key STREQUAL consumer_key)
                continue()
            endif()
            set(found TRUE)
            format_modern_declaration("${producer_record}" producer_declaration)
            format_modern_declaration("${consumer_record}" consumer_declaration)
            if(NOT producer_class STREQUAL consumer_class)
                message(FATAL_ERROR
                    "${boundary} ${consumer_key} class mismatch: producer ${producer_declaration}, consumer ${consumer_declaration}")
            endif()
            # Check interpolation before type so a qualifier conflict remains
            # visible even when integral nointerpolation changes base type too.
            if(NOT producer_interpolation STREQUAL consumer_interpolation)
                message(FATAL_ERROR
                    "${boundary} ${consumer_key} interpolation mismatch: producer ${producer_declaration}, consumer ${consumer_declaration}")
            endif()
            if(NOT producer_type STREQUAL consumer_type)
                message(FATAL_ERROR
                    "${boundary} ${consumer_key} type mismatch: producer ${producer_declaration}, consumer ${consumer_declaration}")
            endif()
        endforeach()
        # System inputs may be generated by the fixed-function pipeline. User
        # inputs, including CG_VERTEXID0, require an explicit prior producer.
        if(NOT found AND consumer_class STREQUAL "user")
            format_modern_declaration("${consumer_record}" consumer_declaration)
            message(FATAL_ERROR
                "${boundary} ${consumer_key} missing producer for consumer ${consumer_declaration}")
        endif()
    endforeach()
endfunction()

function(classify_modern_profile profile stage_prefix modern_output model_output)
    set(is_modern FALSE)
    set(shader_model "")
    if(profile MATCHES "^hlsl${stage_prefix}(40|50)$")
        set(is_modern TRUE)
        set(shader_model "${CMAKE_MATCH_1}")
    endif()
    set(${modern_output} "${is_modern}" PARENT_SCOPE)
    set(${model_output} "${shader_model}" PARENT_SCOPE)
endfunction()

classify_modern_profile("${VERTEX_PROFILE}" v vertex_modern vertex_model)
set(geometry_modern FALSE)
set(geometry_model "")
if(has_geometry)
    classify_modern_profile("${GEOMETRY_PROFILE}" g geometry_modern
        geometry_model)
endif()
classify_modern_profile("${PIXEL_PROFILE}" f pixel_modern pixel_model)

set(modern FALSE)
if(vertex_modern OR geometry_modern OR pixel_modern)
    set(modern TRUE)
    if(NOT vertex_modern OR NOT pixel_modern OR
       (has_geometry AND NOT geometry_modern))
        message(FATAL_ERROR "cannot mix legacy and modern HLSL link profiles")
    endif()
    if(NOT vertex_model STREQUAL pixel_model OR
       (has_geometry AND NOT vertex_model STREQUAL geometry_model))
        if(has_geometry)
            message(FATAL_ERROR
                "HLSL shader model mismatch: vertex ${VERTEX_PROFILE}, geometry ${GEOMETRY_PROFILE}, pixel ${PIXEL_PROFILE}")
        else()
            message(FATAL_ERROR
                "HLSL shader model mismatch: vertex ${VERTEX_PROFILE}, pixel ${PIXEL_PROFILE}")
        endif()
    endif()
endif()

compile_hlsl("${VERTEX_PROFILE}" "${VERTEX_SOURCE}" "${VERTEX_OUTPUT}"
    "${VERTEX_OPTIONS}")
if(has_geometry)
    compile_hlsl("${GEOMETRY_PROFILE}" "${GEOMETRY_SOURCE}"
        "${GEOMETRY_OUTPUT}" "${GEOMETRY_OPTIONS}")
endif()
compile_hlsl("${PIXEL_PROFILE}" "${FRAGMENT_SOURCE}" "${FRAGMENT_OUTPUT}"
    "${PIXEL_OPTIONS}")

if(modern)
    read_modern_interface("${VERTEX_OUTPUT}" out vertex_outputs)
    read_modern_interface("${FRAGMENT_OUTPUT}" in pixel_inputs)
    if(has_geometry)
        read_modern_interface("${GEOMETRY_OUTPUT}" in geometry_inputs)
        read_modern_interface("${GEOMETRY_OUTPUT}" out geometry_outputs)
        compare_modern_interfaces("${vertex_outputs}" "${geometry_inputs}"
            "VS->GS")
        compare_modern_interfaces("${geometry_outputs}" "${pixel_inputs}"
            "GS->PS")
    else()
        compare_modern_interfaces("${vertex_outputs}" "${pixel_inputs}"
            "VS->PS")
    endif()

    if(ASSERT_VERTEX_ID_BRIDGE)
        file(READ "${VERTEX_OUTPUT}" vertex_text)
        file(READ "${GEOMETRY_OUTPUT}" geometry_text)
        file(READ "${FRAGMENT_OUTPUT}" pixel_text)
        string(REGEX MATCHALL
            "// cgc-bind interface in [^ \n]+ uint SV_VertexID SV_VertexID system nointerpolation"
            vertex_id_inputs "${vertex_text}")
        list(LENGTH vertex_id_inputs vertex_id_input_count)
        if(NOT vertex_id_input_count EQUAL 1)
            message(FATAL_ERROR
                "vertex-ID pipeline requires exactly one public SV_VertexID input")
        endif()
        string(REGEX MATCHALL
            "// cgc-bind interface out [^ \n]+ int CG_VERTEXID0 CG_VERTEXID0 user nointerpolation"
            vertex_bridges "${vertex_text}")
        list(LENGTH vertex_bridges vertex_bridge_count)
        string(REGEX MATCHALL
            "// cgc-bind interface in [^ \n]+ int CG_VERTEXID0 CG_VERTEXID0 user nointerpolation"
            geometry_bridges "${geometry_text}")
        list(LENGTH geometry_bridges geometry_bridge_count)
        if(NOT vertex_bridge_count EQUAL 1 OR
           NOT geometry_bridge_count EQUAL 1)
            message(FATAL_ERROR
                "vertex-ID pipeline requires one matching nointerpolation CG_VERTEXID0 bridge")
        endif()
        if(pixel_text MATCHES
           "// cgc-bind interface (in|out) [^\n]* CG_VERTEXID0 ")
            message(FATAL_ERROR
                "pixel interface unexpectedly exposes CG_VERTEXID0")
        endif()
    endif()
    return()
endif()

read_legacy_interface("${VERTEX_OUTPUT}" out TRUE vertex_records)
read_legacy_interface("${FRAGMENT_OUTPUT}" in FALSE fragment_records)

list(LENGTH vertex_records vertex_count)
list(LENGTH fragment_records fragment_count)
if(vertex_count EQUAL 0)
    message(FATAL_ERROR "vertex output contains no linkable interface records")
endif()
if(fragment_count EQUAL 0)
    message(FATAL_ERROR "pixel input contains no linkable interface records")
endif()

foreach(vertex_record IN LISTS vertex_records)
    split_legacy_record("${vertex_record}"
        vertex_semantic vertex_public vertex_type vertex_width)
    set(found FALSE)
    foreach(fragment_record IN LISTS fragment_records)
        split_legacy_record("${fragment_record}"
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
    split_legacy_record("${fragment_record}"
        fragment_semantic fragment_public fragment_type fragment_width)
    set(found FALSE)
    foreach(vertex_record IN LISTS vertex_records)
        split_legacy_record("${vertex_record}"
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

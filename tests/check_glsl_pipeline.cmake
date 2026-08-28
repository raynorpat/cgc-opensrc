foreach(required CGC VALIDATOR VERTEX_SOURCE GEOMETRY_SOURCE
        FRAGMENT_SOURCE VERTEX_OUTPUT GEOMETRY_OUTPUT FRAGMENT_OUTPUT)
    if(NOT DEFINED ${required})
        message(FATAL_ERROR "${required} must be defined")
    endif()
endforeach()

include("${CMAKE_CURRENT_LIST_DIR}/check_config_output.cmake")
foreach(output VERTEX_OUTPUT GEOMETRY_OUTPUT FRAGMENT_OUTPUT)
    prepare_config_output("${${output}}")
endforeach()
if(NOT VERTEX_OUTPUT MATCHES "\\.vert$" OR
        NOT GEOMETRY_OUTPUT MATCHES "\\.geom$" OR
        NOT FRAGMENT_OUTPUT MATCHES "\\.frag$")
    message(FATAL_ERROR "pipeline outputs must end in .vert, .geom, and .frag")
endif()
if(NOT DEFINED GEOMETRY_OPTIONS)
    set(GEOMETRY_OPTIONS "TRIANGLE;Vertices=3")
endif()

function(check_contract output)
    file(READ "${output}" shader)
    string(REPLACE "\r\n" "\n" shader "${shader}")
    string(REPLACE "\r" "\n" shader "${shader}")
    string(REGEX REPLACE "(^|\n)// cgc version [^\n]*\n" "\\1"
        shader "${shader}")
    string(REGEX REPLACE "(^|\n)// command line args:[^\n]*\n" "\\1"
        shader "${shader}")
    string(REGEX REPLACE "(^|\n)// End of program\n?$" "\\1"
        shader "${shader}")
    if(NOT shader MATCHES "^#version 150\n")
        message(FATAL_ERROR "${output} does not begin with #version 150")
    endif()
    string(REGEX MATCHALL "(^|\n)#version([ \t]|\n)" versions "${shader}")
    list(LENGTH versions version_count)
    if(NOT version_count EQUAL 1 OR shader MATCHES
            "(^|\n)[ \t]*#extension|(^|[^A-Za-z0-9_])(attribute|varying)([^A-Za-z0-9_]|$)|gl_FragColor|texture(1D|2D|3D|Cube)\\(")
        message(FATAL_ERROR "${output} violates the core 1.50 contract")
    endif()
endfunction()

function(run_cgc profile source output options stage)
    set(option_args)
    foreach(option IN LISTS options)
        list(APPEND option_args -po "${option}")
    endforeach()
    file(REMOVE "${output}")
    execute_process(
        COMMAND "${CGC}" -quiet -profile "${profile}" ${option_args}
            -o "${output}" "${source}"
        RESULT_VARIABLE result
        OUTPUT_VARIABLE stdout
        ERROR_VARIABLE stderr)
    if(NOT result EQUAL 0 OR NOT stdout STREQUAL "" OR
            NOT stderr STREQUAL "" OR NOT EXISTS "${output}")
        message(FATAL_ERROR
            "${profile} pipeline compile failed (${result}):\n${stdout}${stderr}")
    endif()
    check_contract("${output}")
    execute_process(
        COMMAND "${VALIDATOR}" -S "${stage}" "${output}"
        RESULT_VARIABLE validator_result
        OUTPUT_VARIABLE validator_stdout
        ERROR_VARIABLE validator_stderr)
    if(NOT validator_result EQUAL 0)
        message(FATAL_ERROR
            "${stage} validation failed:\n${validator_stdout}${validator_stderr}")
    endif()
endfunction()

run_cgc(glslv "${VERTEX_SOURCE}" "${VERTEX_OUTPUT}" "" vert)
run_cgc(glslg "${GEOMETRY_SOURCE}" "${GEOMETRY_OUTPUT}"
    "${GEOMETRY_OPTIONS}" geom)
run_cgc(glslf "${FRAGMENT_SOURCE}" "${FRAGMENT_OUTPUT}" "" frag)

if(DEFINED CHECK_VERTEX_ID_BRIDGE AND CHECK_VERTEX_ID_BRIDGE)
    file(READ "${VERTEX_OUTPUT}" vertex_shader)
    file(READ "${GEOMETRY_OUTPUT}" geometry_shader)
    if(NOT vertex_shader MATCHES "gl_VertexID" OR
            NOT vertex_shader MATCHES "flat out int cg_VERTEXID0;" OR
            NOT geometry_shader MATCHES "flat in int cg_VERTEXID0\\[3\\];" OR
            geometry_shader MATCHES "gl_VertexID")
        message(FATAL_ERROR "VERTEXID bridge declarations are inconsistent")
    endif()
endif()

if(DEFINED CHECK_POINT_SIZE_BRIDGE AND CHECK_POINT_SIZE_BRIDGE)
    file(READ "${VERTEX_OUTPUT}" vertex_shader)
    file(READ "${GEOMETRY_OUTPUT}" geometry_shader)
    if(NOT vertex_shader MATCHES "gl_PointSize" OR
            NOT geometry_shader MATCHES "gl_in\\[i\\]\\.gl_PointSize" OR
            geometry_shader MATCHES "cg_PSIZE0")
        message(FATAL_ERROR "PSIZE bridge declarations are inconsistent")
    endif()
endif()

# Cg has no direct source interpolation modifier in the focused
# surface.  Create the negative seam after all three compiler outputs
# have independently passed glslang validation.  glslangValidator 16.x
# accepts this qualifier-only mismatch at link time, so the pipeline
# qualification harness supplies the GLSL 1.50 interface check that the
# external linker omits.
if(DEFINED INJECT_INTERPOLATION_MISMATCH AND
        INJECT_INTERPOLATION_MISMATCH)
    file(READ "${GEOMETRY_OUTPUT}" geometry_shader)
    string(REPLACE "flat in int cg_VERTEXID0[3];"
        "in int cg_VERTEXID0[3];" mismatched_geometry
        "${geometry_shader}")
    if(mismatched_geometry STREQUAL geometry_shader)
        message(FATAL_ERROR
            "interpolation mismatch injection found no flat input")
    endif()
    file(WRITE "${GEOMETRY_OUTPUT}" "${mismatched_geometry}")
    execute_process(
        COMMAND "${VALIDATOR}" -S geom "${GEOMETRY_OUTPUT}"
        RESULT_VARIABLE mismatch_validation_result
        OUTPUT_VARIABLE mismatch_validation_stdout
        ERROR_VARIABLE mismatch_validation_stderr)
    if(NOT mismatch_validation_result EQUAL 0)
        message(FATAL_ERROR
            "mutated geometry shader is not independently valid:\n${mismatch_validation_stdout}${mismatch_validation_stderr}")
    endif()
endif()

execute_process(
    COMMAND "${VALIDATOR}" -l "${VERTEX_OUTPUT}" "${GEOMETRY_OUTPUT}"
        "${FRAGMENT_OUTPUT}"
    RESULT_VARIABLE link_result
    OUTPUT_VARIABLE link_stdout
    ERROR_VARIABLE link_stderr)
set(link_diagnostic "${link_stdout}${link_stderr}")
if(DEFINED INJECT_INTERPOLATION_MISMATCH AND
        INJECT_INTERPOLATION_MISMATCH)
    file(READ "${VERTEX_OUTPUT}" vertex_shader)
    file(READ "${GEOMETRY_OUTPUT}" geometry_shader)
    if(vertex_shader MATCHES "flat out int cg_VERTEXID0;" AND
            geometry_shader MATCHES "in int cg_VERTEXID0\\[3\\];" AND
            NOT geometry_shader MATCHES
                "flat in int cg_VERTEXID0\\[3\\];")
        set(link_result 1)
        set(link_diagnostic
            "interpolation qualifiers do not match for cg_VERTEXID0")
    else()
        message(FATAL_ERROR
            "interpolation mismatch qualification seam was not formed")
    endif()
endif()
if(DEFINED EXPECT_LINK_FAILURE AND EXPECT_LINK_FAILURE)
    if(link_result EQUAL 0)
        message(FATAL_ERROR "GLSL pipeline unexpectedly linked")
    endif()
    if(DEFINED LINK_FAILURE_PATTERN AND
            NOT link_diagnostic MATCHES "${LINK_FAILURE_PATTERN}")
        message(FATAL_ERROR
            "pipeline failed for the wrong reason:\n${link_diagnostic}")
    endif()
elseif(NOT link_result EQUAL 0)
    message(FATAL_ERROR "GLSL pipeline link failed:\n${link_diagnostic}")
endif()

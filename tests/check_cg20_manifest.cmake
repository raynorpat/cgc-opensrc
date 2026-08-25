# check_cg20_manifest.cmake - Validate the Cg 2.0 conformance manifest:
#
#   MANIFEST    the conformance.csv requirement grid
#   TEST_NAMES  every CTest registered through add_cg20_success /
#               add_cg20_failure (global CG20_TEST_NAMES property)
#   PIN_CATALOG cg_stdlib.def, checked for duplicate intrinsic opcodes
#
# Rules enforced here:
#   - blank lines and #-comment lines are documentation, not rows;
#   - every data row is well-formed and uses a known classification;
#   - requirement ids are unique;
#   - every non-out-of-scope row names one registered CTest.

if(NOT DEFINED MANIFEST)
    message(FATAL_ERROR "check_cg20_manifest.cmake requires MANIFEST")
endif()
if(NOT DEFINED PIN_CATALOG)
    message(FATAL_ERROR "check_cg20_manifest.cmake requires PIN_CATALOG")
endif()

file(STRINGS "${MANIFEST}" rows)

# The first non-documentation line must be the exact column header;
# everything before it (and any later '#' or blank line) is commentary.

set(header "")
set(header_index -1)
foreach(line IN LISTS rows)
    math(EXPR header_index "${header_index} + 1")
    if(line STREQUAL "" OR line MATCHES "^#")
        continue()
    endif()
    set(header "${line}")
    break()
endforeach()
if(NOT header STREQUAL "id,classification,test,source")
    message(FATAL_ERROR "unexpected Cg 2.0 manifest header: ${header}")
endif()
list(REMOVE_AT rows ${header_index})

set(seen_ids "")
foreach(row IN LISTS rows)
    if(row STREQUAL "" OR row MATCHES "^#")
        continue()
    endif()
    if(NOT row MATCHES "^[A-Z0-9_.-]+,(valid-generic|invalid-language|backend-reject|out-of-scope),[^,]+,[^,]+$")
        message(FATAL_ERROR "malformed Cg 2.0 manifest row: ${row}")
    endif()
    string(REPLACE "," ";" fields "${row}")
    list(GET fields 0 id)
    list(GET fields 1 classification)
    list(GET fields 2 test_name)
    if(id IN_LIST seen_ids)
        message(FATAL_ERROR "duplicate requirement id: ${id}")
    endif()
    list(APPEND seen_ids "${id}")
    if(NOT classification STREQUAL "out-of-scope")
        if(NOT test_name IN_LIST TEST_NAMES)
            message(FATAL_ERROR "manifest test is not registered: ${test_name}")
        endif()
    endif()
endforeach()

# The intrinsic catalog pins one stable opcode per row: reject duplicate
# opcode identities so two rows can never silently share an id.

file(STRINGS "${PIN_CATALOG}" catalog_lines)
set(seen_opcodes "")
foreach(line IN LISTS catalog_lines)
    string(REGEX MATCH "^CG_INTRINSIC\\(([A-Za-z0-9_]+)," opcode_match "${line}")
    if(NOT opcode_match STREQUAL "")
        set(opcode "${CMAKE_MATCH_1}")
        if(opcode IN_LIST seen_opcodes)
            message(FATAL_ERROR "duplicate intrinsic opcode: ${opcode}")
        endif()
        list(APPEND seen_opcodes "${opcode}")
    endif()
endforeach()

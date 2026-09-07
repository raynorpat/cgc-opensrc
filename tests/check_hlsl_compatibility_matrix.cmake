if(NOT DEFINED MATRIX OR NOT EXISTS "${MATRIX}")
    message(FATAL_ERROR "HLSL compatibility matrix is missing: ${MATRIX}")
endif()
if(NOT DEFINED REGISTERED_TESTS OR NOT EXISTS "${REGISTERED_TESTS}")
    message(FATAL_ERROR "registered-test inventory is missing: ${REGISTERED_TESTS}")
endif()
if(NOT DEFINED REGISTERED_CONTRACTS OR NOT EXISTS "${REGISTERED_CONTRACTS}")
    message(FATAL_ERROR
        "registered-test contract inventory is missing: ${REGISTERED_CONTRACTS}")
endif()
if(DEFINED MODE AND MODE STREQUAL "modern" AND
   (NOT DEFINED MODERN_WITNESSES OR NOT EXISTS "${MODERN_WITNESSES}"))
    message(FATAL_ERROR
        "registered modern-test inventory is missing: ${MODERN_WITNESSES}")
endif()

file(STRINGS "${REGISTERED_TESTS}" registered_tests)
file(STRINGS "${REGISTERED_CONTRACTS}" registered_contracts)
file(STRINGS "${MATRIX}" matrix_lines)
if(DEFINED MODE AND MODE STREQUAL "modern")
    file(STRINGS "${MODERN_WITNESSES}" modern_witnesses)
endif()

if(DEFINED MODE AND MODE STREQUAL "modern")
    if(NOT DEFINED SURFACE_INVENTORY OR
       NOT EXISTS "${SURFACE_INVENTORY}")
        message(FATAL_ERROR
            "modern HLSL surface inventory is missing: ${SURFACE_INVENTORY}")
    endif()
    file(STRINGS "${SURFACE_INVENTORY}" surface_inventory_lines)
    set(required_features)
    foreach(surface_line IN LISTS surface_inventory_lines)
        if(surface_line STREQUAL "" OR surface_line MATCHES "^#")
            continue()
        endif()
        if(NOT surface_line MATCHES
           "^(Target|Type|Aggregate|Qualifier|Default|Operator|Statement|Function|Semantic|Binding|Intrinsic|Texture|Resource|Geometry|Validation)[|][^|]+$")
            message(FATAL_ERROR
                "invalid modern HLSL surface key: ${surface_line}")
        endif()
        list(FIND required_features "${surface_line}" surface_index)
        if(NOT surface_index EQUAL -1)
            message(FATAL_ERROR
                "duplicate modern HLSL surface key: ${surface_line}")
        endif()
        list(APPEND required_features "${surface_line}")
    endforeach()
    set(required_categories
        Target Type Aggregate Qualifier Default Operator Statement Function
        Semantic Binding Intrinsic Texture Resource Geometry Validation)
    set(status_pattern
        "^(native|legalized|stage/model-specific|rejected C[0-9][0-9][0-9][0-9]|not exposed)$")
else()
    set(required_categories
        Target Type Aggregate Qualifier Default Operator Statement Function
        Semantic Binding Intrinsic Texture Resource Validation)
    set(status_pattern
        "^(native|legalized|vertex|pixel|rejected C[0-9][0-9][0-9][0-9])$")
endif()
set(seen_categories)
set(seen_features)
set(row_count 0)

foreach(line IN LISTS matrix_lines)
    if(NOT line MATCHES "^\\|")
        continue()
    endif()
    if(line MATCHES "^\\|[ ]*Category[ ]*\\|" OR
       line MATCHES "^\\|[-| ]+\\|$")
        continue()
    endif()

    string(REPLACE "|" ";" cells "${line}")
    list(LENGTH cells cell_count)
    if(NOT cell_count EQUAL 6)
        message(FATAL_ERROR "matrix row must contain exactly four columns: ${line}")
    endif()
    list(GET cells 1 category)
    list(GET cells 2 feature)
    list(GET cells 3 status)
    list(GET cells 4 test_name)
    string(STRIP "${category}" category)
    string(STRIP "${feature}" feature)
    string(STRIP "${status}" status)
    string(STRIP "${test_name}" test_name)
    string(REPLACE "`" "" test_name "${test_name}")

    if(category STREQUAL "" OR feature STREQUAL "" OR
       status STREQUAL "" OR test_name STREQUAL "")
        message(FATAL_ERROR "matrix row has an unclassified cell: ${line}")
    endif()
    list(FIND required_categories "${category}" category_index)
    if(category_index EQUAL -1)
        message(FATAL_ERROR
            "matrix row has invalid category '${category}': ${line}")
    endif()
    if(NOT status MATCHES "${status_pattern}")
        message(FATAL_ERROR
            "matrix row has invalid status '${status}': ${line}")
    endif()
    if(DEFINED MODE AND MODE STREQUAL "modern" AND
       NOT status STREQUAL "not exposed" AND
       NOT status MATCHES "^rejected C")
        if(test_name MATCHES "^cg20_" OR
           test_name MATCHES "^hlslv_" OR
           test_name MATCHES "^hlslf_" OR
           test_name STREQUAL "hlsl_validate_fresh_directory" OR
           test_name MATCHES "^hlslg(40|50)_registration$")
            message(FATAL_ERROR
                "modern HLSL claim uses a legacy, generic, or no-code witness '${test_name}': ${line}")
        endif()
        list(FIND modern_witnesses "${test_name}" modern_witness_index)
        if(modern_witness_index EQUAL -1)
            message(FATAL_ERROR
                "modern HLSL claim is not backed by a registered modern execution '${test_name}': ${line}")
        endif()
    endif()
    list(FIND registered_tests "${test_name}" test_index)
    if(test_index EQUAL -1)
        message(FATAL_ERROR
            "matrix row names an unregistered test '${test_name}': ${line}")
    endif()
    set(feature_key "${category}|${feature}")
    list(FIND seen_features "${feature_key}" feature_index)
    if(NOT feature_index EQUAL -1)
        message(FATAL_ERROR "matrix contains duplicate feature row: ${line}")
    endif()
    list(APPEND seen_features "${feature_key}")
    if(status MATCHES "^rejected C([0-9][0-9][0-9][0-9])$")
        set(expected_code "${CMAKE_MATCH_1}")
        list(FIND registered_contracts
            "${test_name}|${expected_code}" contract_index)
        if(contract_index EQUAL -1)
            message(FATAL_ERROR
                "matrix rejection ${status} is not backed by ${test_name} with that code: ${line}")
        endif()
    endif()

    list(APPEND seen_categories "${category}")
    math(EXPR row_count "${row_count} + 1")
endforeach()

if(DEFINED MODE AND MODE STREQUAL "modern")
    foreach(required_feature IN LISTS required_features)
        list(FIND seen_features "${required_feature}" feature_index)
        if(feature_index EQUAL -1)
            message(FATAL_ERROR
                "modern HLSL matrix is missing canonical surface key: ${required_feature}")
        endif()
    endforeach()
    foreach(seen_feature IN LISTS seen_features)
        list(FIND required_features "${seen_feature}" feature_index)
        if(feature_index EQUAL -1)
            message(FATAL_ERROR
                "modern HLSL matrix has noncanonical surface key: ${seen_feature}")
        endif()
    endforeach()
    list(LENGTH required_features expected_rows)
elseif(DEFINED EXPECTED_ROWS)
    set(expected_rows ${EXPECTED_ROWS})
else()
    set(expected_rows 286)
endif()
if(NOT row_count EQUAL expected_rows)
    message(FATAL_ERROR
        "HLSL compatibility matrix is incomplete: ${row_count} rows, expected ${expected_rows}")
endif()

foreach(category IN LISTS required_categories)
    list(FIND seen_categories "${category}" category_index)
    if(category_index EQUAL -1)
        message(FATAL_ERROR
            "HLSL compatibility matrix is missing category '${category}'")
    endif()
endforeach()

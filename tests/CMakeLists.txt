function(add_generic_regression name source expected_sha256)
    add_test(
        NAME generic_${name}
        COMMAND ${CMAKE_COMMAND}
            -DCGC=$<TARGET_FILE:cgc>
            -DSOURCE=${PROJECT_SOURCE_DIR}/${source}
            -DEXPECTED_SHA256=${expected_sha256}
            -P ${CMAKE_CURRENT_SOURCE_DIR}/check_generic.cmake
    )
endfunction()

add_generic_regression(position position.cg
    fbd55194fb43822d993f60915df94f7a0627fd9bea27013a8c2d1d0ccdfe7b8e)
add_generic_regression(reflection reflection.cg
    aa2c396563920d50a7b45a6a06c359ec865faa338de4ad42294143a1243b6d8b)
add_generic_regression(vertexlight vertexlight.cg
    a973a0f2820dd1777d0e2cdb593a93f1e0c6d3f5f8640b2017724e37f8fd3c81)
add_generic_regression(vertexlight4 vertexlight4.cg
    5454876aedd830f1d07b65e26cd51e7570035ee01d7d4b784ba4769632e31961)

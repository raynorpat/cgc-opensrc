add_test(
    NAME cgc_vertexlight
    COMMAND $<TARGET_FILE:cgc>
        -profile generic
        ${PROJECT_SOURCE_DIR}/vertexlight.cg
)

add_test(
    NAME cgc_vertexlight4
    COMMAND $<TARGET_FILE:cgc>
        -profile generic
        ${PROJECT_SOURCE_DIR}/vertexlight4.cg
)

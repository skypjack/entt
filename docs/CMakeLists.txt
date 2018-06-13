#
# Doxygen configuration (documentation)
#

set(DOXY_SOURCE_DIRECTORY ${entt_SOURCE_DIR}/src)
set(DOXY_DOCS_DIRECTORY ${entt_SOURCE_DIR}/docs)
set(DOXY_OUTPUT_DIRECTORY ${entt_BINARY_DIR})

configure_file(doxy.in ${entt_BINARY_DIR}/doxy.cfg @ONLY)

add_custom_target(
    docs ALL
    COMMAND ${DOXYGEN_EXECUTABLE} ${entt_BINARY_DIR}/doxy.cfg
    WORKING_DIRECTORY ${entt_SOURCE_DIR}
    VERBATIM
    SOURCES doxy.in
)

install(
    DIRECTORY ${DOXY_OUTPUT_DIRECTORY}/html
    DESTINATION share/${PROJECT_NAME}-${PROJECT_VERSION}/
)

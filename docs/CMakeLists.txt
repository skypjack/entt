# Doxygen configuration (documentation)

include(FetchContent)

FetchContent_Declare(
    doxygen-awesome-css
    GIT_REPOSITORY https://github.com/jothepro/doxygen-awesome-css
    GIT_TAG main
    GIT_SHALLOW 1
)

FetchContent_GetProperties(doxygen-awesome-css)

if(NOT doxygen-awesome-css_POPULATED)
    FetchContent_Populate(doxygen-awesome-css)
    set(doxygen-awesome-css_INCLUDE_DIR ${doxygen-awesome-css_SOURCE_DIR})
endif()

set(DOXY_SOURCE_DIRECTORY ${EnTT_SOURCE_DIR}/src)
set(DOXY_CSS_DIRECTORY ${doxygen-awesome-css_INCLUDE_DIR})
set(DOXY_DOCS_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
set(DOXY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})

configure_file(doxy.in doxy.cfg @ONLY)

add_custom_target(
    docs ALL
    COMMAND ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/doxy.cfg
    WORKING_DIRECTORY ${EnTT_SOURCE_DIR}
    VERBATIM
    SOURCES
        dox/extra.dox
        md/config.md
        md/container.md
        md/core.md
        md/entity.md
        md/faq.md
        md/lib.md
        md/links.md
        md/locator.md
        md/meta.md
        md/poly.md
        md/process.md
        md/reference.md
        md/resource.md
        md/signal.md
        md/unreal.md
        doxy.in
)

install(
    DIRECTORY ${DOXY_OUTPUT_DIRECTORY}/html
    DESTINATION share/${PROJECT_NAME}-${PROJECT_VERSION}/
)

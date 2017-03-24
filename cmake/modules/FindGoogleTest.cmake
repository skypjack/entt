# FindGoogleTest
# ---------
#
# Locate Google Test Framework
#
# This module defines:
#
# ::
#
#   GOOGLETEST_INCLUDE_DIRS, where to find the headers
#   GOOGLETEST_LIBRARIES, the libraries against which to link
#   GOOGLETEST_FOUND, if false, do not try to use the above mentioned vars
#

set(BUILD_DEPS_DIR ${CMAKE_SOURCE_DIR}/${PROJECT_DEPS_DIR})
set(GOOGLETEST_DEPS_DIR googletest)

find_path(
    GOOGLETEST_INCLUDE_DIR NAMES gtest/gtest.h
    PATHS ${BUILD_DEPS_DIR}/${GOOGLETEST_DEPS_DIR}/googletest/include/
    NO_DEFAULT_PATH
)

find_path(
    GOOGLEMOCK_INCLUDE_DIR NAMES gmock/gmock.h
    PATHS ${BUILD_DEPS_DIR}/${GOOGLETEST_DEPS_DIR}/googlemock/include/
    NO_DEFAULT_PATH
)

find_library(
    GOOGLETEST_LIBRARY NAMES gtest
    PATHS ${BUILD_DEPS_DIR}/${GOOGLETEST_DEPS_DIR}/build/googlemock/gtest/
    PATH_SUFFIXES Release
    NO_DEFAULT_PATH
)

find_library(
    GOOGLETEST_MAIN_LIBRARY NAMES gtest_main
    PATHS ${BUILD_DEPS_DIR}/${GOOGLETEST_DEPS_DIR}/build/googlemock/gtest/
    PATH_SUFFIXES Release
    NO_DEFAULT_PATH
)

find_library(
    GOOGLEMOCK_LIBRARY NAMES gmock
    PATHS ${BUILD_DEPS_DIR}/${GOOGLETEST_DEPS_DIR}/build/googlemock/
    PATH_SUFFIXES Release
    NO_DEFAULT_PATH
)

find_library(
    GOOGLEMOCK_MAIN_LIBRARY NAMES gmock_main
    PATHS ${BUILD_DEPS_DIR}/${GOOGLETEST_DEPS_DIR}/build/googlemock/
    PATH_SUFFIXES Release
    NO_DEFAULT_PATH
)

include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(
    GOOGLETEST
    FOUND_VAR GOOGLETEST_FOUND
    REQUIRED_VARS
        GOOGLETEST_LIBRARY
        GOOGLETEST_MAIN_LIBRARY
        GOOGLEMOCK_LIBRARY
        GOOGLEMOCK_MAIN_LIBRARY
        GOOGLETEST_INCLUDE_DIR
        GOOGLEMOCK_INCLUDE_DIR
)

if(GOOGLETEST_FOUND)
    set(
        GOOGLETEST_LIBRARIES
        ${GOOGLETEST_LIBRARY}
        ${GOOGLETEST_MAIN_LIBRARY}
        ${GOOGLEMOCK_LIBRARY}
        ${GOOGLEMOCK_MAIN_LIBRARY}
    )

    set(
        GOOGLETEST_INCLUDE_DIRS
        ${GOOGLETEST_INCLUDE_DIR}
        ${GOOGLEMOCK_INCLUDE_DIR}
    )
endif(GOOGLETEST_FOUND)


mark_as_advanced(
    GOOGLETEST_INCLUDE_DIR
    GOOGLEMOCK_INCLUDE_DIR
    GOOGLETEST_LIBRARY
    GOOGLETEST_MAIN_LIBRARY
    GOOGLEMOCK_LIBRARY
    GOOGLEMOCK_MAIN_LIBRARY
)

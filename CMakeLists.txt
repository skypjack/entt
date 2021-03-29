#
# EnTT
#

cmake_minimum_required(VERSION 3.12.4)

#
# Building in-tree is not allowed (we take care of your craziness).
#

if(${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR})
    message(FATAL_ERROR "Prevented in-tree built. Please create a build directory outside of the source code and call cmake from there. Thank you.")
endif()

#
# Read project version
#

set(ENTT_VERSION_REGEX "#define ENTT_VERSION_.*[ \t]+(.+)")
file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/src/entt/config/version.h" ENTT_VERSION REGEX ${ENTT_VERSION_REGEX})
list(TRANSFORM ENTT_VERSION REPLACE ${ENTT_VERSION_REGEX} "\\1")
string(JOIN "." ENTT_VERSION ${ENTT_VERSION})

#
# Project configuration
#

project(
    EnTT
    VERSION ${ENTT_VERSION}
    DESCRIPTION "Gaming meets modern C++ - a fast and reliable entity-component system (ECS) and much more"
    HOMEPAGE_URL "https://github.com/skypjack/entt"
    LANGUAGES CXX
)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

message(VERBOSE "*")
message(VERBOSE "* ${PROJECT_NAME} v${PROJECT_VERSION} (${CMAKE_BUILD_TYPE})")
message(VERBOSE "* Copyright (c) 2017-2021 Michele Caini <michele.caini@gmail.com>")
message(VERBOSE "*")

option(ENTT_USE_LIBCPP "Use libc++ by adding -stdlib=libc++ flag if availbale." ON)
option(ENTT_USE_SANITIZER "Enable sanitizers by adding -fsanitize=address -fno-omit-frame-pointer -fsanitize=undefined flags" OFF)

#
# Compiler stuff
#

if(NOT WIN32 AND ENTT_USE_LIBCPP)
    include(CheckCXXSourceCompiles)
    include(CMakePushCheckState)

    cmake_push_check_state()

    set(CMAKE_REQUIRED_FLAGS "${CMAKE_REQUIRED_FLAGS} -stdlib=libc++")

    check_cxx_source_compiles("
        #include<type_traits>
        int main() { return std::is_same_v<int, char>; }
    " ENTT_HAS_LIBCPP)

    if(NOT ENTT_HAS_LIBCPP)
        message(VERBOSE "The option ENTT_USE_LIBCPP is set (by default) but libc++ is not available. The flag will not be added to the target.")
    endif()

    cmake_pop_check_state()
endif()

#
# Add EnTT target
#

include(GNUInstallDirs)

add_library(EnTT INTERFACE)
add_library(EnTT::EnTT ALIAS EnTT)

target_include_directories(
    EnTT
    INTERFACE
        $<BUILD_INTERFACE:${EnTT_SOURCE_DIR}/src>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

if(ENTT_USE_SANITIZER)
    target_compile_options(EnTT INTERFACE $<$<CONFIG:Debug>:-fsanitize=address -fno-omit-frame-pointer -fsanitize=undefined>)
    target_link_libraries(EnTT INTERFACE $<$<CONFIG:Debug>:-fsanitize=address -fno-omit-frame-pointer -fsanitize=undefined>)
endif()

if(ENTT_HAS_LIBCPP)
    target_compile_options(EnTT BEFORE INTERFACE -stdlib=libc++)
endif()

target_compile_features(EnTT INTERFACE cxx_std_17)

#
# Install pkg-config file
#

set(EnTT_PKGCONFIG ${CMAKE_CURRENT_BINARY_DIR}/entt.pc)

configure_file(
    ${EnTT_SOURCE_DIR}/cmake/in/entt.pc.in
    ${EnTT_PKGCONFIG}
    @ONLY
)

install(
    FILES ${EnTT_PKGCONFIG}
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig
)

#
# Install EnTT
#

include(CMakePackageConfigHelpers)

install(
    TARGETS EnTT
    EXPORT EnTTTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

write_basic_package_version_file(
    EnTTConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY AnyNewerVersion
)

configure_package_config_file(
    ${EnTT_SOURCE_DIR}/cmake/in/EnTTConfig.cmake.in
    EnTTConfig.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/EnTT/cmake
)

export(
    EXPORT EnTTTargets
    FILE ${CMAKE_CURRENT_BINARY_DIR}/EnTTTargets.cmake
    NAMESPACE EnTT::
)

install(
    EXPORT EnTTTargets
    FILE EnTTTargets.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/EnTT/cmake
    NAMESPACE EnTT::
)

install(
    FILES
        ${PROJECT_BINARY_DIR}/EnTTConfig.cmake
        ${PROJECT_BINARY_DIR}/EnTTConfigVersion.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/EnTT/cmake
)

install(DIRECTORY src/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

export(PACKAGE EnTT)

#
# Tests
#

option(ENTT_BUILD_TESTING "Enable building tests." OFF)

if(ENTT_BUILD_TESTING)
    option(ENTT_FIND_GTEST_PACKAGE "Enable finding gtest package." OFF)
    option(ENTT_BUILD_BENCHMARK "Build benchmark." OFF)
    option(ENTT_BUILD_EXAMPLE "Build examples." OFF)
    option(ENTT_BUILD_LIB "Build lib tests." OFF)
    option(ENTT_BUILD_SNAPSHOT "Build snapshot test with Cereal." OFF)

    include(CTest)
    enable_testing()
    add_subdirectory(test)
endif()

#
# Documentation
#

option(ENTT_BUILD_DOCS "Enable building with documentation." OFF)

if(ENTT_BUILD_DOCS)
    find_package(Doxygen 1.8)

    if(DOXYGEN_FOUND)
        add_subdirectory(docs)
    endif()
endif()

#
# AOB
#

add_custom_target(
    aob
    SOURCES
        .github/workflows/build.yml
        .github/workflows/coverage.yml
        .github/workflows/deploy.yml
        .github/workflows/sanitizer.yml
        .github/FUNDING.yml
        AUTHORS
        CONTRIBUTING.md
        LICENSE
        README.md
        TODO
)

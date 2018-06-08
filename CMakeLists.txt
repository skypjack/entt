#
# EnTT
#

cmake_minimum_required(VERSION 3.2)

include(GNUInstallDirs)

#
# Building in-tree is not allowed (we take care of your craziness).
#

if(${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR})
    message(FATAL_ERROR "Prevented in-tree built. Please create a build directory outside of the source code and call cmake from there. Thank you.")
endif()

#
# Project configuration
#

project(entt VERSION 2.6.1)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

set(SETTINGS_ORGANIZATION "Michele Caini")
set(SETTINGS_APPLICATION ${PROJECT_NAME})
set(PROJECT_AUTHOR "Michele Caini")
set(PROJECT_AUTHOR_EMAIL "michele.caini@gmail.com")

message("*")
message("* ${PROJECT_NAME} v${PROJECT_VERSION} (${CMAKE_BUILD_TYPE})")
message("* Copyright (c) 2018 ${PROJECT_AUTHOR} <${PROJECT_AUTHOR_EMAIL}>")
message("*")

option(ENTT_COMPILE_OPTIONS "Use compile options from EnTT." ON)
option(USE_LIBCPP "Use libc++ by adding -stdlib=libc++ flag if availbale." ON)

#
# Compiler stuff
#

if(NOT MSVC AND USE_LIBCPP)
    include(CheckCXXSourceCompiles)
    include(CMakePushCheckState)
    
    cmake_push_check_state()

    set(CMAKE_REQUIRED_FLAGS "${CMAKE_REQUIRED_FLAGS} -stdlib=libc++")
    
    check_cxx_source_compiles("
        #include<type_traits>
        int main() { return std::is_same<int, int>::value ? 0 : 1; }
    " HAS_LIBCPP)

    if(USE_LIBCPP AND NOT HAS_LIBCPP)
        message(WARNING "The option USE_LIBCPP is set (by default) but libc++ is not available. The flag will not be added to the target")
    endif()

    cmake_pop_check_state()
endif()

#
# Add EnTT target
#

add_library(EnTT INTERFACE)

target_include_directories(EnTT INTERFACE
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/src>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_compile_definitions(EnTT
    INTERFACE $<$<AND:$<CONFIG:Debug>,$<NOT:$<CXX_COMPILER_ID:MSVC>>>:DEBUG>
    INTERFACE $<$<AND:$<CONFIG:Release>,$<NOT:$<CXX_COMPILER_ID:MSVC>>>:RELEASE>
)

if(ENTT_COMPILE_OPTIONS)
    target_compile_options(EnTT
        INTERFACE $<$<AND:$<CONFIG:Debug>,$<NOT:$<CXX_COMPILER_ID:MSVC>>>:-O0 -g>
        INTERFACE $<$<AND:$<CONFIG:Release>,$<CXX_COMPILER_ID:Clang>>:-O2>
        INTERFACE $<$<AND:$<CONFIG:Release>,$<CXX_COMPILER_ID:GNU>>:-O3>
    )
endif()

if(USE_LIBCPP AND HAS_LIBCPP)
    target_compile_options(EnTT BEFORE
        INTERFACE -stdlib=libc++
    )
endif()

target_compile_features(EnTT
    INTERFACE cxx_std_14
)

#
# Install EnTT
#

if(${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
    set(ENTT_INSTALL_CONFIGDIR cmake)
else()
    set(ENTT_INSTALL_CONFIGDIR ${CMAKE_INSTALL_LIBDIR}/cmake/entt)
endif()

install(DIRECTORY src/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR} 
)

install(TARGETS EnTT EXPORT EnTTTargets)

export(EXPORT EnTTTargets
  FILE ${CMAKE_CURRENT_BINARY_DIR}/EnTTTargets.cmake
)

install(EXPORT EnTTTargets
    FILE EnTTTargets.cmake
    DESTINATION ${ENTT_INSTALL_CONFIGDIR} 
)

# build tree package config file
configure_file(cmake/in/EnTTBuildConfig.cmake.in EnTTConfig.cmake @ONLY)

include(CMakePackageConfigHelpers)

# install tree package config file
configure_package_config_file(cmake/in/EnTTConfig.cmake.in
    ${ENTT_INSTALL_CONFIGDIR}/EnTTConfig.cmake
    INSTALL_DESTINATION ${ENTT_INSTALL_CONFIGDIR} 
    PATH_VARS CMAKE_INSTALL_INCLUDEDIR
    NO_CHECK_REQUIRED_COMPONENTS_MACRO
)

write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/EnTTConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY AnyNewerVersion
)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${ENTT_INSTALL_CONFIGDIR}/EnTTConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/EnTTConfigVersion.cmake
    DESTINATION ${ENTT_INSTALL_CONFIGDIR}
)

export(PACKAGE EnTT)

#
# Tests
#

option(BUILD_TESTING "Enable testing with ctest." ON)

if(BUILD_TESTING)
    set(THREADS_PREFER_PTHREAD_FLAG ON)
    find_package(Threads REQUIRED)

    option(BUILD_BENCHMARK "Build benchmark." OFF)
    option(BUILD_MOD "Build mod example." OFF)
    option(BUILD_SNAPSHOT "Build snapshot example." OFF)

    # gtest, gtest_main, gmock and gmock_main targets are available from now on
    set(GOOGLETEST_DEPS_DIR ${entt_SOURCE_DIR}/deps/googletest)
    configure_file(${entt_SOURCE_DIR}/cmake/in/googletest.in ${GOOGLETEST_DEPS_DIR}/CMakeLists.txt)
    execute_process(COMMAND ${CMAKE_COMMAND} -G "${CMAKE_GENERATOR}" . WORKING_DIRECTORY ${GOOGLETEST_DEPS_DIR})
    execute_process(COMMAND ${CMAKE_COMMAND} --build . WORKING_DIRECTORY ${GOOGLETEST_DEPS_DIR})
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    add_subdirectory(${GOOGLETEST_DEPS_DIR}/src ${GOOGLETEST_DEPS_DIR}/build)

    enable_testing()
    add_subdirectory(test)
endif()

#
# Documentation
#

option(BUILD_DOCS "Enable building with documentation." OFF)

if(BUILD_DOCS)
    find_package(Doxygen 1.8)

    if(DOXYGEN_FOUND)
        add_subdirectory(docs)
    endif()
endif()

#
# AOB
#

add_custom_target(
    entt_aob
    SOURCES
        appveyor.yml
        AUTHORS
        CONTRIBUTING
        LICENSE
        README.md
        TODO
        .travis.yml
)

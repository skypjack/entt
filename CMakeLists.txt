#
# EnTT
#

cmake_minimum_required(VERSION 3.2)

#
# Building in-tree is not allowed (we take care of your craziness).
#

if(${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR})
    message(FATAL_ERROR "Prevented in-tree built. Please create a build directory outside of the source code and call cmake from there. Thank you.")
endif()

#
# Project configuration
#

project(entt VERSION 2.5.0)

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

#
# Compiler stuff
#

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT MSVC)
    include(CheckCXXSourceCompiles)

    set(OLD_CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS})
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")

    check_cxx_source_compiles("
        #include<type_traits>
        int main() { return std::is_same<int, int>::value ? 0 : 1; }
    " HAS_LIBCPP)

    if(NOT HAS_LIBCPP)
        set(CMAKE_CXX_FLAGS "${OLD_CMAKE_CXX_FLAGS}")
    endif()

    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pedantic -Wall")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -DRELEASE")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0 -g -DDEBUG")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,--no-undefined")

    if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        # it seems that -O3 ruins the performance when using clang ...
        set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O2")
    else()
        # ... on the other side, GCC is incredibly comfortable with it.
        set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3")
    endif()
endif()

#
# Include EnTT
#

include_directories(${entt_SOURCE_DIR}/src)

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

find_package(Doxygen 1.8)

if(DOXYGEN_FOUND)
    add_subdirectory(docs)
endif()

#
# AOB
#

add_custom_target(
    entt_aob
    SOURCES
        appveyor.yml
        AUTHORS
        LICENSE
        README.md
        TODO
        .travis.yml
)

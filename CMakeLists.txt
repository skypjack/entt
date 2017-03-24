#
# EnTT
#

#
# Building in-tree is not allowed (we take care of your craziness).
#

if(${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR})
    message(FATAL_ERROR "Prevented in-tree built. Please create a build directory outside of the source code and call cmake from there. Thank you.")
endif()

#
# Project configuration
#

project(entt)
cmake_minimum_required(VERSION 3.4)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif(NOT CMAKE_BUILD_TYPE)

set(PROJECT_NAME "entt")
set(PROJECT_VERSION_MAJOR 1)
set(PROJECT_VERSION_MINOR 0)
set(PROJECT_VERSION_PATCH 0)
set(PROJECT_VERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH})
set(SETTINGS_ORGANIZATION "Michele Caini")
set(SETTINGS_APPLICATION ${PROJECT_NAME})
set(PROJECT_AUTHOR "Michele Caini")
set(PROJECT_YEAR_COPYRIGHT "2017")
set(PROJECT_AUTHOR_EMAIL "michele.caini@gmail.com")

set(PROJECT_BUILD_MESSAGE ${PROJECT_NAME} " v" ${PROJECT_VERSION} " (" ${CMAKE_BUILD_TYPE} ")")
set(COPYRIGHT_BUILD_MESSAGE "Copyright (c) " ${PROJECT_YEAR_COPYRIGHT} " " ${PROJECT_AUTHOR}  " <" ${PROJECT_AUTHOR_EMAIL} ">")

message("*")
message("* " ${PROJECT_BUILD_MESSAGE})
message("* " ${COPYRIGHT_BUILD_MESSAGE})
message("*")

#
# Compile stuff
#

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,--no-undefined")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pedantic -Wall")
# set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} "-Wextra -Weffc++")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0 -g -DDEBUG")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -DRELEASE")

add_definitions(
    -DPROJECT_NAME=${PROJECT_NAME}
    -DPROJECT_VERSION=${PROJECT_VERSION}
)

#
# CMake configuration
#

set(PROJECT_CMAKE_MODULES cmake/modules)
set(PROJECT_BUILD_DIR build)
set(PROJECT_DEPS_DIR deps)
set(PROJECT_SRC_DIR src)
set(PROJECT_TEST_DIR test)

set(PROJECT_RUNTIME_OUTPUT_DIRECTORY bin)

set(
    CMAKE_MODULE_PATH
    ${CMAKE_MODULE_PATH}
    ${CMAKE_SOURCE_DIR}/${PROJECT_CMAKE_MODULES}
)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_RUNTIME_OUTPUT_DIRECTORY})

#
# Enable test support using ctest
#

include(CTest)

#
# Referenced packages
#

set(THREADS_PREFER_PTHREAD_FLAG ON)

find_package(Threads REQUIRED)
find_package(GoogleTest)

#
# Referenced directories and targets
#

if(${GOOGLETEST_FOUND})
    add_subdirectory(${PROJECT_TEST_DIR})
endif(${GOOGLETEST_FOUND})

#
# I use QtCreator and I need the lines below, so do not ask. :-)
#

file(
    GLOB_RECURSE PROJECT_FILES FOLLOW_SYMLINKS
    *.txt *.c *.cpp *.hpp *.h *.in *.cmake *.sh *.md AUTHORS LICENSE
)

add_custom_target(
    QTCREATOR_FALLBACK ALL
    COMMENT "Feel free to ignore this target, but please do not remove it."
    SOURCES ${PROJECT_FILES}
)

# EnTT

cmake_minimum_required(VERSION 3.15.7)

# Read project version

set(ENTT_VERSION_REGEX "#define ENTT_VERSION_.*[ \t]+(.+)")
file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/src/entt/config/version.h" ENTT_VERSION REGEX ${ENTT_VERSION_REGEX})
list(TRANSFORM ENTT_VERSION REPLACE ${ENTT_VERSION_REGEX} "\\1")
string(JOIN "." ENTT_VERSION ${ENTT_VERSION})

# Project configuration

project(
    EnTT
    VERSION ${ENTT_VERSION}
    DESCRIPTION "Gaming meets modern C++ - a fast and reliable entity-component system (ECS) and much more"
    HOMEPAGE_URL "https://github.com/skypjack/entt"
    LANGUAGES C CXX
)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()
    
message(VERBOSE "*")
message(VERBOSE "* ${PROJECT_NAME} v${PROJECT_VERSION} (${CMAKE_BUILD_TYPE})")
message(VERBOSE "* Copyright (c) 2017-2025 Michele Caini <michele.caini@gmail.com>")
message(VERBOSE "*")

# CMake stuff

list(INSERT CMAKE_MODULE_PATH 0 ${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules)

# Compiler stuff

option(ENTT_USE_LIBCPP "Use libc++ by adding -stdlib=libc++ flag if available." OFF)
option(ENTT_USE_SANITIZER "Enable sanitizers by adding -fsanitize=address -fno-omit-frame-pointer -fsanitize=undefined flags if available." OFF)
option(ENTT_USE_CLANG_TIDY "Enable static analysis with clang-tidy" OFF)

if(ENTT_USE_LIBCPP)
    if(NOT WIN32)
        include(CheckCXXSourceCompiles)
        include(CMakePushCheckState)

        cmake_push_check_state()

        set(CMAKE_REQUIRED_FLAGS "${CMAKE_REQUIRED_FLAGS} -stdlib=libc++")

        check_cxx_source_compiles("
            #include<type_traits>
            int main() { return std::is_same_v<int, char>; }
        " ENTT_HAS_LIBCPP)

        cmake_pop_check_state()
    endif()

    if(NOT ENTT_HAS_LIBCPP)
        message(VERBOSE "The option ENTT_USE_LIBCPP is set but libc++ is not available.")
    endif()
endif()

if(ENTT_USE_SANITIZER)
    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
        set(ENTT_HAS_SANITIZER TRUE CACHE BOOL "" FORCE)
        mark_as_advanced(ENTT_HAS_SANITIZER)
    endif()

    if(NOT ENTT_HAS_SANITIZER)
        message(VERBOSE "The option ENTT_USE_SANITIZER is set but sanitizer support is not available.")
    endif()
endif()

if(ENTT_USE_CLANG_TIDY)
    find_program(ENTT_CLANG_TIDY_EXECUTABLE "clang-tidy")

    if(NOT ENTT_CLANG_TIDY_EXECUTABLE)
        message(VERBOSE "The option ENTT_USE_CLANG_TIDY is set but clang-tidy executable is not available.")
    endif()
endif()

# Add EnTT target

include(GNUInstallDirs)

add_library(EnTT INTERFACE)
add_library(EnTT::EnTT ALIAS EnTT)

target_include_directories(
    EnTT
    INTERFACE
        $<BUILD_INTERFACE:${EnTT_SOURCE_DIR}/src>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_compile_features(EnTT INTERFACE cxx_std_17)

if(ENTT_HAS_LIBCPP)
    target_compile_options(EnTT BEFORE INTERFACE -stdlib=libc++)
endif()

if(ENTT_HAS_SANITIZER)
    target_compile_options(EnTT INTERFACE $<$<CONFIG:Debug>:-fsanitize=address -fno-omit-frame-pointer -fsanitize=undefined>)
    target_link_libraries(EnTT INTERFACE $<$<CONFIG:Debug>:-fsanitize=address -fno-omit-frame-pointer -fsanitize=undefined>)
endif()

if(ENTT_CLANG_TIDY_EXECUTABLE)
    set(CMAKE_CXX_CLANG_TIDY "${ENTT_CLANG_TIDY_EXECUTABLE};--config-file=${EnTT_SOURCE_DIR}/.clang-tidy;--header-filter=${EnTT_SOURCE_DIR}/src/entt/.*")
endif()

# Add EnTT goodies

option(ENTT_INCLUDE_HEADERS "Add all EnTT headers to the EnTT target." OFF)
option(ENTT_INCLUDE_NATVIS "Add EnTT natvis files to the EnTT target." OFF)

if(ENTT_INCLUDE_HEADERS)
    set(
        HEADERS_FILES
        config/config.h
        config/macro.h
        config/version.h
        container/dense_map.hpp
        container/dense_set.hpp
        container/table.hpp
        container/fwd.hpp
        core/algorithm.hpp
        core/any.hpp
        core/attribute.h
        core/bit.hpp
        core/compressed_pair.hpp
        core/enum.hpp
        core/family.hpp
        core/fwd.hpp
        core/hashed_string.hpp
        core/ident.hpp
        core/iterator.hpp
        core/memory.hpp
        core/monostate.hpp
        core/ranges.hpp
        core/tuple.hpp
        core/type_info.hpp
        core/type_traits.hpp
        core/utility.hpp
        entity/component.hpp
        entity/entity.hpp
        entity/fwd.hpp
        entity/group.hpp
        entity/handle.hpp
        entity/mixin.hpp
        entity/helper.hpp
        entity/organizer.hpp
        entity/ranges.hpp
        entity/registry.hpp
        entity/runtime_view.hpp
        entity/snapshot.hpp
        entity/sparse_set.hpp
        entity/storage.hpp
        entity/view.hpp
        graph/adjacency_matrix.hpp
        graph/dot.hpp
        graph/flow.hpp
        graph/fwd.hpp
        locator/locator.hpp
        meta/adl_pointer.hpp
        meta/container.hpp
        meta/context.hpp
        meta/factory.hpp
        meta/fwd.hpp
        meta/meta.hpp
        meta/node.hpp
        meta/pointer.hpp
        meta/policy.hpp
        meta/range.hpp
        meta/resolve.hpp
        meta/template.hpp
        meta/type_traits.hpp
        meta/utility.hpp
        poly/fwd.hpp
        poly/poly.hpp
        process/fwd.hpp
        process/process.hpp
        process/scheduler.hpp
        resource/cache.hpp
        resource/fwd.hpp
        resource/loader.hpp
        resource/resource.hpp
        signal/delegate.hpp
        signal/dispatcher.hpp
        signal/emitter.hpp
        signal/fwd.hpp
        signal/sigh.hpp
        tools/davey.hpp
        entt.hpp
        fwd.hpp
        tools.hpp
    )

    list(TRANSFORM HEADERS_FILES APPEND ">" OUTPUT_VARIABLE HEADERS_BUILD_INTERFACE)
    list(TRANSFORM HEADERS_BUILD_INTERFACE PREPEND "$<BUILD_INTERFACE:${EnTT_SOURCE_DIR}/src/entt/")

    list(TRANSFORM HEADERS_FILES APPEND ">" OUTPUT_VARIABLE HEADERS_INSTALL_INTERFACE)
    list(TRANSFORM HEADERS_INSTALL_INTERFACE PREPEND "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/entt/")

    target_sources(EnTT INTERFACE ${HEADERS_BUILD_INTERFACE} ${HEADERS_INSTALL_INTERFACE})
endif()

if(ENTT_INCLUDE_NATVIS)
    if(MSVC)
        set(ENTT_HAS_NATVIS TRUE CACHE BOOL "" FORCE)
        mark_as_advanced(ENTT_HAS_NATVIS)
    endif()

    if(NOT ENTT_HAS_NATVIS)
        message(VERBOSE "The option ENTT_INCLUDE_NATVIS is set but natvis files are not supported. They will not be added to the target.")
    endif()
endif()

if(ENTT_HAS_NATVIS)
    set(
        NATVIS_FILES
        config.natvis
        container.natvis
        core.natvis
        entity.natvis
        graph.natvis
        locator.natvis
        meta.natvis
        poly.natvis
        process.natvis
        resource.natvis
        signal.natvis
    )

    list(TRANSFORM NATVIS_FILES APPEND ">" OUTPUT_VARIABLE NATVIS_BUILD_INTERFACE)
    list(TRANSFORM NATVIS_BUILD_INTERFACE PREPEND "$<BUILD_INTERFACE:${EnTT_SOURCE_DIR}/src/entt/natvis/")

    list(TRANSFORM NATVIS_FILES APPEND ">" OUTPUT_VARIABLE NATVIS_INSTALL_INTERFACE)
    list(TRANSFORM NATVIS_INSTALL_INTERFACE PREPEND "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/entt/natvis/")

    target_sources(EnTT INTERFACE ${NATVIS_BUILD_INTERFACE} ${NATVIS_INSTALL_INTERFACE})
endif()

# Install EnTT and all related files

option(ENTT_INSTALL "Install EnTT and all related files." OFF)

if(ENTT_INSTALL)
    # Install pkg-config file

    include(JoinPaths)

    set(EnTT_PKGCONFIG ${CMAKE_CURRENT_BINARY_DIR}/entt.pc)

    join_paths(EnTT_PKGCONFIG_INCLUDEDIR "\${prefix}" "${CMAKE_INSTALL_INCLUDEDIR}")

    configure_file(
        ${EnTT_SOURCE_DIR}/cmake/in/entt.pc.in
        ${EnTT_PKGCONFIG}
        @ONLY
    )

    install(
        FILES ${EnTT_PKGCONFIG}
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig
    )

    # Install EnTT

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

    install(
        DIRECTORY src/
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
        FILES_MATCHING
        PATTERN "*.h"
        PATTERN "*.hpp"
        PATTERN "*.natvis"
    )

    export(PACKAGE EnTT)
endif()

# Tests and testbed

option(ENTT_BUILD_TESTING "Enable building tests." OFF)
option(ENTT_BUILD_TESTBED "Enable building testbed." OFF)

if(ENTT_BUILD_TESTING OR ENTT_BUILD_TESTBED)
    set(ENTT_ID_TYPE std::uint32_t CACHE STRING "Type of identifiers to use for tests and testbed")
    set(ENTT_CXX_STD cxx_std_17 CACHE STRING "C++ standard revision to use for tests and testbed")

    # Tests and tesetbed do not work together because SDL gets confused with EnTT tests
    if(ENTT_BUILD_TESTING)
        option(ENTT_FIND_GTEST_PACKAGE "Enable finding gtest package." OFF)
    
        option(ENTT_BUILD_BENCHMARK "Build benchmark." OFF)
        option(ENTT_BUILD_EXAMPLE "Build examples." OFF)
        option(ENTT_BUILD_LIB "Build lib tests." OFF)
        option(ENTT_BUILD_SNAPSHOT "Build snapshot test with Cereal." OFF)
    
        include(CTest)
        enable_testing()
        add_subdirectory(test)
    elseif(ENTT_BUILD_TESTBED)
        add_subdirectory(testbed)
    endif()
endif()

# Documentation

option(ENTT_BUILD_DOCS "Enable building with documentation." OFF)

if(ENTT_BUILD_DOCS)
    add_subdirectory(docs)
endif()

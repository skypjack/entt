# Tests configuration

include(FetchContent)
include(CheckCXXSourceCompiles)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

if(ENTT_FIND_GTEST_PACKAGE)
    find_package(GTest REQUIRED)
else()
    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG main
        GIT_SHALLOW 1
    )

    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

    FetchContent_MakeAvailable(googletest)

    add_library(GTest::Main ALIAS gtest_main)

    target_compile_features(gtest PUBLIC cxx_std_20)
    set_target_properties(gtest PROPERTIES CXX_CLANG_TIDY "")

    target_compile_features(gtest_main PUBLIC cxx_std_20)
    set_target_properties(gtest_main PROPERTIES CXX_CLANG_TIDY "")

    target_compile_features(gmock PUBLIC cxx_std_20)
    set_target_properties(gmock PROPERTIES CXX_CLANG_TIDY "")

    target_compile_features(gmock_main PUBLIC cxx_std_20)
    set_target_properties(gmock_main PROPERTIES CXX_CLANG_TIDY "")
endif()

include_directories($<TARGET_PROPERTY:EnTT,INTERFACE_INCLUDE_DIRECTORIES>)
add_compile_options($<TARGET_PROPERTY:EnTT,INTERFACE_COMPILE_OPTIONS>)

function(SETUP_TARGET TARGET_NAME)
    set_target_properties(${TARGET_NAME} PROPERTIES CXX_EXTENSIONS OFF)
    target_compile_features(${TARGET_NAME} PRIVATE ${ENTT_CXX_STD})
    target_link_libraries(${TARGET_NAME} PRIVATE EnTT)

    if(MSVC)
        set_target_properties(${TARGET_NAME} PROPERTIES LINK_FLAGS_RELWITHDEBINFO "/PROFILE")

        target_compile_options(
            ${TARGET_NAME}
            PRIVATE
                $<$<STREQUAL:"${CMAKE_CXX_COMPILER_ID}","Clang">:
                    -Wdocumentation
                    -Wno-deprecated-declarations
                    -Wno-exceptions
                    -Wconversion
                >
                /EHsc /wd4324 /wd4996
                # disabling INCREMENTAL is required by SizeBench
                $<$<CONFIG:Debug>:/Od /INCREMENTAL:NO>
                $<$<CONFIG:Release>:/O2>
        )

        target_link_options(
            ${TARGET_NAME}
            PRIVATE
                # disabling INCREMENTAL is required by SizeBench
                $<$<CONFIG:Debug>:/INCREMENTAL:NO>
                $<$<CONFIG:Release>:/OPT:NOICF>
        )
    else()
        target_compile_options(
            ${TARGET_NAME}
            PRIVATE
                -fvisibility=hidden
                -pedantic
                -Wall
                -Wconversion
                -Wno-deprecated-declarations
                -Wshadow
                $<$<CONFIG:Debug>:-O0 -g>
                $<$<CONFIG:Release>:-O2>
        )
    endif()

    target_compile_definitions(
        ${TARGET_NAME}
        PRIVATE
            ENTT_ID_TYPE=${ENTT_ID_TYPE}
            _ENABLE_EXTENDED_ALIGNED_STORAGE
            NOMINMAX
            ${ARGN}
    )
endfunction()

add_library(odr OBJECT odr.cpp)
set_target_properties(odr PROPERTIES POSITION_INDEPENDENT_CODE ON)
SETUP_TARGET(odr)

function(SETUP_BASIC_TEST)
    cmake_parse_arguments(BASIC_TEST "DEFS" "NAME" "SOURCES" ${ARGN})
    add_executable(${BASIC_TEST_NAME} $<TARGET_OBJECTS:odr> ${BASIC_TEST_SOURCES})
    target_link_libraries(${BASIC_TEST_NAME} PRIVATE GTest::Main Threads::Threads)
    SETUP_TARGET(${BASIC_TEST_NAME} ${BASIC_TEST_UNPARSED_ARGUMENTS})
    add_test(NAME ${BASIC_TEST_NAME} COMMAND ${BASIC_TEST_NAME})
    set_tests_properties(${BASIC_TEST_NAME} PROPERTIES TIMEOUT 60)
endfunction()

function(SETUP_BASIC_TEST_DEPRECATED TEST_NAME TEST_SOURCES)
    add_executable(${TEST_NAME} $<TARGET_OBJECTS:odr> ${TEST_SOURCES})
    target_link_libraries(${TEST_NAME} PRIVATE GTest::Main Threads::Threads)
    SETUP_TARGET(${TEST_NAME} ${ARGN})
    add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})
    set_tests_properties(${TEST_NAME} PROPERTIES TIMEOUT 60)
endfunction()

function(SETUP_LIB_SHARED_TEST TEST_NAME SUB_PATH)
    set(TARGET_NAME ${TEST_NAME}_${SUB_PATH})
    add_library(_${TARGET_NAME} SHARED $<TARGET_OBJECTS:odr> lib/${TEST_NAME}/${SUB_PATH}/lib.cpp)
    SETUP_TARGET(_${TARGET_NAME} ENTT_API_EXPORT)
    SETUP_BASIC_TEST_DEPRECATED(lib_${TARGET_NAME} lib/${TEST_NAME}/${SUB_PATH}/main.cpp ENTT_API_IMPORT)
    set_target_properties(lib_${TARGET_NAME} PROPERTIES CXX_CLANG_TIDY "")
    target_link_libraries(lib_${TARGET_NAME} PRIVATE _${TARGET_NAME})
endfunction()

function(SETUP_LIB_PLUGIN_TEST TEST_NAME SUB_PATH)
    set(TARGET_NAME ${TEST_NAME}_${SUB_PATH})
    add_library(_${TARGET_NAME} MODULE $<TARGET_OBJECTS:odr> lib/${TEST_NAME}/${SUB_PATH}/plugin.cpp)
    SETUP_TARGET(_${TARGET_NAME} ${ARGVN})
    SETUP_BASIC_TEST_DEPRECATED(lib_${TARGET_NAME} lib/${TEST_NAME}/${SUB_PATH}/main.cpp PLUGIN="$<TARGET_FILE:_${TARGET_NAME}>" ${ARGVN})
    target_link_libraries(_${TARGET_NAME} PRIVATE cr::cr)
    target_link_libraries(lib_${TARGET_NAME} PRIVATE cr::cr ${CMAKE_DL_LIBS})
    set_target_properties(_${TARGET_NAME} PROPERTIES CXX_CLANG_TIDY "")
    set_target_properties(lib_${TARGET_NAME} PROPERTIES CXX_CLANG_TIDY "")
    target_compile_options(_${TARGET_NAME} PRIVATE $<$<NOT:$<STREQUAL:"${CMAKE_CXX_COMPILER_ID}","MSVC">>:-Wno-conversion>)
    target_compile_options(lib_${TARGET_NAME} PRIVATE $<$<NOT:$<STREQUAL:"${CMAKE_CXX_COMPILER_ID}","MSVC">>:-Wno-conversion>)
    add_dependencies(lib_${TARGET_NAME} _${TARGET_NAME})
endfunction()

# Test benchmark

if(ENTT_BUILD_BENCHMARK)
    SETUP_BASIC_TEST(
        NAME benchmark
        SOURCES benchmark/benchmark.cpp
    )

    set_target_properties(benchmark PROPERTIES CXX_CLANG_TIDY "")
endif()

# Test example

if(ENTT_BUILD_EXAMPLE)
    SETUP_BASIC_TEST_DEPRECATED(custom_identifier example/custom_identifier.cpp)
    SETUP_BASIC_TEST_DEPRECATED(entity_copy example/entity_copy.cpp)
    SETUP_BASIC_TEST_DEPRECATED(reserved_bits example/reserved_bits.cpp)
    SETUP_BASIC_TEST_DEPRECATED(signal_less example/signal_less.cpp)
endif()

# Test lib

if(ENTT_BUILD_LIB)
    FetchContent_Declare(
        cr
        GIT_REPOSITORY https://github.com/fungos/cr.git
        GIT_TAG master
        GIT_SHALLOW 1
    )

    FetchContent_MakeAvailable(cr)

    add_library(cr::cr ALIAS cr)

    SETUP_LIB_SHARED_TEST(dispatcher shared)
    SETUP_LIB_PLUGIN_TEST(dispatcher plugin)

    SETUP_LIB_SHARED_TEST(emitter shared)
    SETUP_LIB_PLUGIN_TEST(emitter plugin)

    SETUP_LIB_SHARED_TEST(locator shared)
    SETUP_LIB_PLUGIN_TEST(locator plugin)

    SETUP_LIB_SHARED_TEST(meta shared)
    SETUP_LIB_PLUGIN_TEST(meta plugin)
    SETUP_LIB_PLUGIN_TEST(meta plugin_std ENTT_STANDARD_CPP)

    SETUP_LIB_SHARED_TEST(registry shared)
    SETUP_LIB_PLUGIN_TEST(registry plugin)

    SETUP_LIB_SHARED_TEST(view shared)
    SETUP_LIB_PLUGIN_TEST(view plugin)
endif()

# Test snapshot

if(ENTT_BUILD_SNAPSHOT)
    FetchContent_Declare(
        cereal
        GIT_REPOSITORY https://github.com/USCiLab/cereal.git
        GIT_TAG v1.3.2
        GIT_SHALLOW 1
    )
       
    set(BUILD_DOC OFF CACHE BOOL "" FORCE)
    set(BUILD_SANDBOX OFF CACHE BOOL "" FORCE)
    set(SKIP_PORTABILITY_TEST ON CACHE BOOL "" FORCE)
    set(SKIP_PERFORMANCE_COMPARISON ON CACHE BOOL "" FORCE)

    FetchContent_MakeAvailable(cereal)

    SETUP_BASIC_TEST_DEPRECATED(cereal_snapshot snapshot/snapshot.cpp)

    target_link_libraries(cereal_snapshot PRIVATE cereal)
    target_compile_options(cereal_snapshot PRIVATE $<$<NOT:$<STREQUAL:"${CMAKE_CXX_COMPILER_ID}","MSVC">>:-Wno-conversion>)
    set_target_properties(cereal_snapshot PROPERTIES CXX_CLANG_TIDY "")
endif()

# Test config

SETUP_BASIC_TEST_DEPRECATED(version entt/config/version.cpp)

# Test container

SETUP_BASIC_TEST_DEPRECATED(dense_map entt/container/dense_map.cpp)
SETUP_BASIC_TEST_DEPRECATED(dense_set entt/container/dense_set.cpp)
SETUP_BASIC_TEST_DEPRECATED(table entt/container/table.cpp)

# Test core

SETUP_BASIC_TEST_DEPRECATED(algorithm entt/core/algorithm.cpp)
SETUP_BASIC_TEST_DEPRECATED(any entt/core/any.cpp)
SETUP_BASIC_TEST_DEPRECATED(bit entt/core/bit.cpp)
SETUP_BASIC_TEST_DEPRECATED(compressed_pair entt/core/compressed_pair.cpp)
SETUP_BASIC_TEST_DEPRECATED(enum entt/core/enum.cpp)
SETUP_BASIC_TEST_DEPRECATED(family entt/core/family.cpp)
SETUP_BASIC_TEST_DEPRECATED(hashed_string entt/core/hashed_string.cpp)
SETUP_BASIC_TEST_DEPRECATED(ident entt/core/ident.cpp)
SETUP_BASIC_TEST_DEPRECATED(iterator entt/core/iterator.cpp)
SETUP_BASIC_TEST_DEPRECATED(memory entt/core/memory.cpp)
SETUP_BASIC_TEST_DEPRECATED(monostate entt/core/monostate.cpp)
SETUP_BASIC_TEST_DEPRECATED(tuple entt/core/tuple.cpp)
SETUP_BASIC_TEST_DEPRECATED(type_info entt/core/type_info.cpp)
SETUP_BASIC_TEST_DEPRECATED(type_traits entt/core/type_traits.cpp)
SETUP_BASIC_TEST_DEPRECATED(utility entt/core/utility.cpp)

# Test entity

SETUP_BASIC_TEST_DEPRECATED(component entt/entity/component.cpp)
SETUP_BASIC_TEST_DEPRECATED(entity entt/entity/entity.cpp)
SETUP_BASIC_TEST_DEPRECATED(group entt/entity/group.cpp)
SETUP_BASIC_TEST_DEPRECATED(handle entt/entity/handle.cpp)
SETUP_BASIC_TEST_DEPRECATED(helper entt/entity/helper.cpp)
SETUP_BASIC_TEST_DEPRECATED(organizer entt/entity/organizer.cpp)
SETUP_BASIC_TEST_DEPRECATED(reactive_mixin entt/entity/reactive_mixin.cpp)
SETUP_BASIC_TEST_DEPRECATED(registry entt/entity/registry.cpp)
SETUP_BASIC_TEST_DEPRECATED(runtime_view entt/entity/runtime_view.cpp)
SETUP_BASIC_TEST_DEPRECATED(sigh_mixin entt/entity/sigh_mixin.cpp)
SETUP_BASIC_TEST_DEPRECATED(snapshot entt/entity/snapshot.cpp)
SETUP_BASIC_TEST_DEPRECATED(sparse_set entt/entity/sparse_set.cpp)
SETUP_BASIC_TEST_DEPRECATED(storage entt/entity/storage.cpp)
SETUP_BASIC_TEST_DEPRECATED(storage_entity entt/entity/storage_entity.cpp)
SETUP_BASIC_TEST_DEPRECATED(storage_no_instance entt/entity/storage_no_instance.cpp)
SETUP_BASIC_TEST_DEPRECATED(storage_utility entt/entity/storage_utility.cpp)
SETUP_BASIC_TEST_DEPRECATED(storage_utility_no_mixin entt/entity/storage_utility.cpp ENTT_NO_MIXIN)
SETUP_BASIC_TEST_DEPRECATED(view entt/entity/view.cpp)

# Test graph

SETUP_BASIC_TEST_DEPRECATED(adjacency_matrix entt/graph/adjacency_matrix.cpp)
SETUP_BASIC_TEST_DEPRECATED(dot entt/graph/dot.cpp)
SETUP_BASIC_TEST_DEPRECATED(flow entt/graph/flow.cpp)

# Test locator

SETUP_BASIC_TEST(
    NAME locator
    SOURCES entt/locator/locator.cpp
)

# Test meta

SETUP_BASIC_TEST_DEPRECATED(meta_any entt/meta/meta_any.cpp)
SETUP_BASIC_TEST_DEPRECATED(meta_base entt/meta/meta_base.cpp)
SETUP_BASIC_TEST_DEPRECATED(meta_container entt/meta/meta_container.cpp)
SETUP_BASIC_TEST_DEPRECATED(meta_context entt/meta/meta_context.cpp)
SETUP_BASIC_TEST_DEPRECATED(meta_conv entt/meta/meta_conv.cpp)
SETUP_BASIC_TEST_DEPRECATED(meta_ctor entt/meta/meta_ctor.cpp)
SETUP_BASIC_TEST_DEPRECATED(meta_custom entt/meta/meta_custom.cpp)
SETUP_BASIC_TEST_DEPRECATED(meta_data entt/meta/meta_data.cpp)
SETUP_BASIC_TEST_DEPRECATED(meta_factory entt/meta/meta_factory.cpp)
SETUP_BASIC_TEST_DEPRECATED(meta_func entt/meta/meta_func.cpp)
SETUP_BASIC_TEST_DEPRECATED(meta_handle entt/meta/meta_handle.cpp)
SETUP_BASIC_TEST_DEPRECATED(meta_pointer entt/meta/meta_pointer.cpp)
SETUP_BASIC_TEST_DEPRECATED(meta_range entt/meta/meta_range.cpp)
SETUP_BASIC_TEST_DEPRECATED(meta_template entt/meta/meta_template.cpp)
SETUP_BASIC_TEST_DEPRECATED(meta_type entt/meta/meta_type.cpp)
SETUP_BASIC_TEST_DEPRECATED(meta_utility entt/meta/meta_utility.cpp)

# Test poly

SETUP_BASIC_TEST(
    NAME poly
    SOURCES entt/poly/poly.cpp
)

# Test process

SETUP_BASIC_TEST(
    NAME process
    SOURCES
        entt/process/process.cpp
        entt/process/scheduler.cpp
)

# Test resource

SETUP_BASIC_TEST(
    NAME resource
    SOURCES
        entt/resource/resource.cpp
        entt/resource/resource_cache.cpp
        entt/resource/resource_loader.cpp
)

# Test signal

SETUP_BASIC_TEST_DEPRECATED(delegate entt/signal/delegate.cpp)
SETUP_BASIC_TEST_DEPRECATED(dispatcher entt/signal/dispatcher.cpp)
SETUP_BASIC_TEST_DEPRECATED(emitter entt/signal/emitter.cpp)
SETUP_BASIC_TEST_DEPRECATED(sigh entt/signal/sigh.cpp)

# Test stl

SETUP_BASIC_TEST(
    NAME stl
    SOURCES
        entt/stl/functional.cpp
        entt/stl/memory.cpp
    DEFS ENTT_USE_STL
)

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

    target_compile_features(gtest PUBLIC cxx_std_17)
    set_target_properties(gtest PROPERTIES CXX_CLANG_TIDY "")

    target_compile_features(gtest_main PUBLIC cxx_std_17)
    set_target_properties(gtest_main PROPERTIES CXX_CLANG_TIDY "")

    target_compile_features(gmock PUBLIC cxx_std_17)
    set_target_properties(gmock PROPERTIES CXX_CLANG_TIDY "")

    target_compile_features(gmock_main PUBLIC cxx_std_17)
    set_target_properties(gmock_main PROPERTIES CXX_CLANG_TIDY "")
endif()

include_directories($<TARGET_PROPERTY:EnTT,INTERFACE_INCLUDE_DIRECTORIES>)
add_compile_options($<TARGET_PROPERTY:EnTT,INTERFACE_COMPILE_OPTIONS>)

function(SETUP_TARGET TARGET_NAME)
    set_target_properties(${TARGET_NAME} PROPERTIES CXX_EXTENSIONS OFF)
    target_compile_features(${TARGET_NAME} PRIVATE ${ENTT_CXX_STD})
    target_link_libraries(${TARGET_NAME} PRIVATE EnTT)

    if(MSVC)
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

function(SETUP_BASIC_TEST TEST_NAME TEST_SOURCES)
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
    SETUP_BASIC_TEST(lib_${TARGET_NAME} lib/${TEST_NAME}/${SUB_PATH}/main.cpp ENTT_API_IMPORT)
    set_target_properties(lib_${TARGET_NAME} PROPERTIES CXX_CLANG_TIDY "")
    target_link_libraries(lib_${TARGET_NAME} PRIVATE _${TARGET_NAME})
endfunction()

function(SETUP_LIB_PLUGIN_TEST TEST_NAME SUB_PATH)
    set(TARGET_NAME ${TEST_NAME}_${SUB_PATH})
    add_library(_${TARGET_NAME} MODULE $<TARGET_OBJECTS:odr> lib/${TEST_NAME}/${SUB_PATH}/plugin.cpp)
    SETUP_TARGET(_${TARGET_NAME} ${ARGVN})
    SETUP_BASIC_TEST(lib_${TARGET_NAME} lib/${TEST_NAME}/${SUB_PATH}/main.cpp PLUGIN="$<TARGET_FILE:_${TARGET_NAME}>" ${ARGVN})
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
    SETUP_BASIC_TEST(benchmark benchmark/benchmark.cpp)
    set_target_properties(benchmark PROPERTIES CXX_CLANG_TIDY "")
endif()

# Test example

if(ENTT_BUILD_EXAMPLE)
    SETUP_BASIC_TEST(custom_identifier example/custom_identifier.cpp)
    SETUP_BASIC_TEST(entity_copy example/entity_copy.cpp)
    SETUP_BASIC_TEST(reserved_bits example/reserved_bits.cpp)
    SETUP_BASIC_TEST(signal_less example/signal_less.cpp)
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

    FetchContent_GetProperties(cereal)

    if(NOT cereal_POPULATED)
        FetchContent_Populate(cereal)
        set(cereal_INCLUDE_DIR ${cereal_SOURCE_DIR}/include)
    endif()

    SETUP_BASIC_TEST(cereal snapshot/snapshot.cpp)

    set_target_properties(cereal PROPERTIES CXX_CLANG_TIDY "")
    target_include_directories(cereal PRIVATE ${cereal_INCLUDE_DIR})
    target_compile_options(cereal PRIVATE $<$<NOT:$<STREQUAL:"${CMAKE_CXX_COMPILER_ID}","MSVC">>:-Wno-conversion>)
endif()

# Test config

SETUP_BASIC_TEST(version entt/config/version.cpp)

# Test container

SETUP_BASIC_TEST(dense_map entt/container/dense_map.cpp)
SETUP_BASIC_TEST(dense_set entt/container/dense_set.cpp)
SETUP_BASIC_TEST(table entt/container/table.cpp)

# Test core

SETUP_BASIC_TEST(algorithm entt/core/algorithm.cpp)
SETUP_BASIC_TEST(any entt/core/any.cpp)
SETUP_BASIC_TEST(bit entt/core/bit.cpp)
SETUP_BASIC_TEST(compressed_pair entt/core/compressed_pair.cpp)
SETUP_BASIC_TEST(enum entt/core/enum.cpp)
SETUP_BASIC_TEST(family entt/core/family.cpp)
SETUP_BASIC_TEST(hashed_string entt/core/hashed_string.cpp)
SETUP_BASIC_TEST(ident entt/core/ident.cpp)
SETUP_BASIC_TEST(iterator entt/core/iterator.cpp)
SETUP_BASIC_TEST(memory entt/core/memory.cpp)
SETUP_BASIC_TEST(monostate entt/core/monostate.cpp)
SETUP_BASIC_TEST(tuple entt/core/tuple.cpp)
SETUP_BASIC_TEST(type_info entt/core/type_info.cpp)
SETUP_BASIC_TEST(type_traits entt/core/type_traits.cpp)
SETUP_BASIC_TEST(utility entt/core/utility.cpp)

# Test entity

SETUP_BASIC_TEST(component entt/entity/component.cpp)
SETUP_BASIC_TEST(entity entt/entity/entity.cpp)
SETUP_BASIC_TEST(group entt/entity/group.cpp)
SETUP_BASIC_TEST(handle entt/entity/handle.cpp)
SETUP_BASIC_TEST(helper entt/entity/helper.cpp)
SETUP_BASIC_TEST(organizer entt/entity/organizer.cpp)
SETUP_BASIC_TEST(reactive_mixin entt/entity/reactive_mixin.cpp)
SETUP_BASIC_TEST(registry entt/entity/registry.cpp)
SETUP_BASIC_TEST(runtime_view entt/entity/runtime_view.cpp)
SETUP_BASIC_TEST(sigh_mixin entt/entity/sigh_mixin.cpp)
SETUP_BASIC_TEST(snapshot entt/entity/snapshot.cpp)
SETUP_BASIC_TEST(sparse_set entt/entity/sparse_set.cpp)
SETUP_BASIC_TEST(storage entt/entity/storage.cpp)
SETUP_BASIC_TEST(storage_entity entt/entity/storage_entity.cpp)
SETUP_BASIC_TEST(storage_no_instance entt/entity/storage_no_instance.cpp)
SETUP_BASIC_TEST(storage_utility entt/entity/storage_utility.cpp)
SETUP_BASIC_TEST(storage_utility_no_mixin entt/entity/storage_utility.cpp ENTT_NO_MIXIN)
SETUP_BASIC_TEST(view entt/entity/view.cpp)

# Test graph

SETUP_BASIC_TEST(adjacency_matrix entt/graph/adjacency_matrix.cpp)
SETUP_BASIC_TEST(dot entt/graph/dot.cpp)
SETUP_BASIC_TEST(flow entt/graph/flow.cpp)

# Test locator

SETUP_BASIC_TEST(locator entt/locator/locator.cpp)

# Test meta

SETUP_BASIC_TEST(meta_any entt/meta/meta_any.cpp)
SETUP_BASIC_TEST(meta_base entt/meta/meta_base.cpp)
SETUP_BASIC_TEST(meta_container entt/meta/meta_container.cpp)
SETUP_BASIC_TEST(meta_context entt/meta/meta_context.cpp)
SETUP_BASIC_TEST(meta_conv entt/meta/meta_conv.cpp)
SETUP_BASIC_TEST(meta_ctor entt/meta/meta_ctor.cpp)
SETUP_BASIC_TEST(meta_custom entt/meta/meta_custom.cpp)
SETUP_BASIC_TEST(meta_data entt/meta/meta_data.cpp)
SETUP_BASIC_TEST(meta_dtor entt/meta/meta_dtor.cpp)
SETUP_BASIC_TEST(meta_factory entt/meta/meta_factory.cpp)
SETUP_BASIC_TEST(meta_func entt/meta/meta_func.cpp)
SETUP_BASIC_TEST(meta_handle entt/meta/meta_handle.cpp)
SETUP_BASIC_TEST(meta_pointer entt/meta/meta_pointer.cpp)
SETUP_BASIC_TEST(meta_range entt/meta/meta_range.cpp)
SETUP_BASIC_TEST(meta_template entt/meta/meta_template.cpp)
SETUP_BASIC_TEST(meta_type entt/meta/meta_type.cpp)
SETUP_BASIC_TEST(meta_utility entt/meta/meta_utility.cpp)

# Test poly

SETUP_BASIC_TEST(poly entt/poly/poly.cpp)

# Test process

SETUP_BASIC_TEST(process entt/process/process.cpp)
SETUP_BASIC_TEST(scheduler entt/process/scheduler.cpp)

# Test resource

SETUP_BASIC_TEST(resource entt/resource/resource.cpp)
SETUP_BASIC_TEST(resource_cache entt/resource/resource_cache.cpp)
SETUP_BASIC_TEST(resource_loader entt/resource/resource_loader.cpp)

# Test signal

SETUP_BASIC_TEST(delegate entt/signal/delegate.cpp)
SETUP_BASIC_TEST(dispatcher entt/signal/dispatcher.cpp)
SETUP_BASIC_TEST(emitter entt/signal/emitter.cpp)
SETUP_BASIC_TEST(sigh entt/signal/sigh.cpp)

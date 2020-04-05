#
# Tests configuration
#

include(FetchContent)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

if(FIND_GTEST_PACKAGE)
    find_package(GTest REQUIRED)
else()
    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG master
        GIT_SHALLOW 1
    )

    FetchContent_GetProperties(googletest)

    if(NOT googletest_POPULATED)
        FetchContent_Populate(googletest)
        set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
        add_subdirectory(${googletest_SOURCE_DIR} ${googletest_BINARY_DIR})
    endif()

    add_library(GTest::Main ALIAS gtest_main)

    target_compile_features(gtest PUBLIC cxx_std_17)
    target_compile_features(gtest_main PUBLIC cxx_std_17)
    target_compile_features(gmock PUBLIC cxx_std_17)
    target_compile_features(gmock_main PUBLIC cxx_std_17)
endif()

include_directories($<TARGET_PROPERTY:EnTT,INTERFACE_INCLUDE_DIRECTORIES>)
add_compile_options($<TARGET_PROPERTY:EnTT,INTERFACE_COMPILE_OPTIONS>)

function(SETUP_TARGET TARGET_NAME)
    set_target_properties(${TARGET_NAME} PROPERTIES CXX_EXTENSIONS OFF)
    target_link_libraries(${TARGET_NAME} PRIVATE EnTT)

    target_compile_options(
        ${TARGET_NAME}
        PRIVATE
            $<$<NOT:$<PLATFORM_ID:Windows>>:-pedantic -fvisibility=hidden -Wall -Wshadow -Wno-deprecated-declarations>
            $<$<PLATFORM_ID:Windows>:/EHsc /W1 /wd4996 /w14800>
    )

    target_compile_options(
        ${TARGET_NAME}
        PRIVATE
            $<$<AND:$<CONFIG:Debug>,$<NOT:$<PLATFORM_ID:Windows>>>:-O0 -g>
            $<$<AND:$<CONFIG:Release>,$<NOT:$<PLATFORM_ID:Windows>>>:-O2>
            $<$<AND:$<CONFIG:Debug>,$<PLATFORM_ID:Windows>>:/Od>
            $<$<AND:$<CONFIG:Release>,$<PLATFORM_ID:Windows>>:/O2>
    )
endfunction()

add_library(odr OBJECT odr.cpp)
SETUP_TARGET(odr)

function(SETUP_BASIC_TEST TEST_NAME TEST_SOURCES)
    add_executable(${TEST_NAME} $<TARGET_OBJECTS:odr> ${TEST_SOURCES})
    target_link_libraries(${TEST_NAME} PRIVATE GTest::Main Threads::Threads)
    SETUP_TARGET(${TEST_NAME})
    add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})
endfunction()

function(SETUP_LIB_TEST TEST_NAME)
    add_library(_${TEST_NAME} SHARED lib/${TEST_NAME}/lib.cpp)
    SETUP_TARGET(_${TEST_NAME})
    SETUP_BASIC_TEST(lib_${TEST_NAME} lib/${TEST_NAME}/main.cpp)
    target_compile_definitions(_${TEST_NAME} PRIVATE ENTT_API_EXPORT ${ARGV1})
    target_compile_definitions(lib_${TEST_NAME} PRIVATE ENTT_API_IMPORT ${ARGV1})
    target_link_libraries(lib_${TEST_NAME} PRIVATE _${TEST_NAME})
endfunction()

function(SETUP_PLUGIN_TEST TEST_NAME)
    add_library(_${TEST_NAME} MODULE lib/${TEST_NAME}/plugin.cpp)
    SETUP_TARGET(_${TEST_NAME})
    SETUP_BASIC_TEST(lib_${TEST_NAME} lib/${TEST_NAME}/main.cpp)
    target_include_directories(_${TEST_NAME} PRIVATE ${cr_INCLUDE_DIR})
    target_include_directories(lib_${TEST_NAME} PRIVATE ${cr_INCLUDE_DIR})
    target_compile_definitions(lib_${TEST_NAME} PRIVATE NOMINMAX PLUGIN="$<TARGET_FILE:_${TEST_NAME}>" ${ARGV1})
    target_compile_definitions(_${TEST_NAME} PRIVATE NOMINMAX ${ARGV1})
    target_link_libraries(lib_${TEST_NAME} PRIVATE ${CMAKE_DL_LIBS})
endfunction()

# Test benchmark

if(BUILD_BENCHMARK)
    SETUP_BASIC_TEST(benchmark benchmark/benchmark.cpp)
endif()

# Test lib

if(BUILD_LIB)
    FetchContent_Declare(
        cr
        GIT_REPOSITORY https://github.com/fungos/cr.git
        GIT_TAG master
        GIT_SHALLOW 1
    )

    FetchContent_GetProperties(cr)

    if(NOT cr_POPULATED)
        FetchContent_Populate(cr)
        set(cr_INCLUDE_DIR ${cr_SOURCE_DIR})
    endif()

    SETUP_LIB_TEST(dispatcher)
    SETUP_LIB_TEST(emitter)
    SETUP_LIB_TEST(meta)
    SETUP_LIB_TEST(registry)

    SETUP_PLUGIN_TEST(dispatcher_plugin)
    SETUP_PLUGIN_TEST(emitter_plugin)
    SETUP_PLUGIN_TEST(meta_plugin)
    SETUP_PLUGIN_TEST(registry_plugin)

    SETUP_PLUGIN_TEST(meta_plugin_std ENTT_STANDARD_CPP)
endif()

# Test snapshot

if(BUILD_SNAPSHOT)
    FetchContent_Declare(
        cereal
        GIT_REPOSITORY https://github.com/USCiLab/cereal.git
        GIT_TAG v1.2.2
        GIT_SHALLOW 1
    )

    FetchContent_GetProperties(cereal)

    if(NOT cereal_POPULATED)
        FetchContent_Populate(cereal)
        set(cereal_INCLUDE_DIR ${cereal_SOURCE_DIR}/include)
    endif()

    SETUP_BASIC_TEST(cereal snapshot/snapshot.cpp)
    target_include_directories(cereal PRIVATE ${cereal_INCLUDE_DIR})
endif()

# Test core

SETUP_BASIC_TEST(algorithm entt/core/algorithm.cpp)
SETUP_BASIC_TEST(family entt/core/family.cpp)
SETUP_BASIC_TEST(hashed_string entt/core/hashed_string.cpp)
SETUP_BASIC_TEST(ident entt/core/ident.cpp)
SETUP_BASIC_TEST(monostate entt/core/monostate.cpp)
SETUP_BASIC_TEST(type_info entt/core/type_info.cpp)
SETUP_BASIC_TEST(type_traits entt/core/type_traits.cpp)
SETUP_BASIC_TEST(utility entt/core/utility.cpp)

# Test entity

SETUP_BASIC_TEST(actor entt/entity/actor.cpp)
SETUP_BASIC_TEST(entity entt/entity/entity.cpp)
SETUP_BASIC_TEST(group entt/entity/group.cpp)
SETUP_BASIC_TEST(helper entt/entity/helper.cpp)
SETUP_BASIC_TEST(observer entt/entity/observer.cpp)
SETUP_BASIC_TEST(registry entt/entity/registry.cpp)
SETUP_BASIC_TEST(runtime_view entt/entity/runtime_view.cpp)
SETUP_BASIC_TEST(snapshot entt/entity/snapshot.cpp)
SETUP_BASIC_TEST(sparse_set entt/entity/sparse_set.cpp)
SETUP_BASIC_TEST(storage entt/entity/storage.cpp)
SETUP_BASIC_TEST(view entt/entity/view.cpp)

# Test locator

SETUP_BASIC_TEST(locator entt/locator/locator.cpp)

# Test meta

SETUP_BASIC_TEST(meta entt/meta/meta.cpp)

# Test process

SETUP_BASIC_TEST(process entt/process/process.cpp)
SETUP_BASIC_TEST(scheduler entt/process/scheduler.cpp)

# Test resource

SETUP_BASIC_TEST(resource entt/resource/resource.cpp)

# Test signal

SETUP_BASIC_TEST(delegate entt/signal/delegate.cpp)
SETUP_BASIC_TEST(dispatcher entt/signal/dispatcher.cpp)
SETUP_BASIC_TEST(emitter entt/signal/emitter.cpp)
SETUP_BASIC_TEST(sigh entt/signal/sigh.cpp)

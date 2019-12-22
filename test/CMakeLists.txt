#
# Tests configuration
#

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

if(FIND_GTEST_PACKAGE)
    find_package(GTest REQUIRED)
else()
    # gtest, gtest_main, gmock and gmock_main targets are available from now on
    set(GOOGLETEST_DEPS_DIR ${EnTT_SOURCE_DIR}/deps/googletest)
    configure_file(${EnTT_SOURCE_DIR}/cmake/in/googletest.in ${GOOGLETEST_DEPS_DIR}/CMakeLists.txt)
    execute_process(COMMAND ${CMAKE_COMMAND} -G "${CMAKE_GENERATOR}" . WORKING_DIRECTORY ${GOOGLETEST_DEPS_DIR})
    execute_process(COMMAND ${CMAKE_COMMAND} --build . WORKING_DIRECTORY ${GOOGLETEST_DEPS_DIR})
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    add_subdirectory(${GOOGLETEST_DEPS_DIR}/src ${GOOGLETEST_DEPS_DIR}/build)
    target_compile_features(gmock_main PRIVATE $<TARGET_PROPERTY:EnTT,INTERFACE_COMPILE_FEATURES>)
    target_compile_features(gmock PRIVATE $<TARGET_PROPERTY:EnTT,INTERFACE_COMPILE_FEATURES>)
    add_library(GTest::Main ALIAS gtest_main)
endif()

include_directories($<TARGET_PROPERTY:EnTT,INTERFACE_INCLUDE_DIRECTORIES>)
add_compile_options($<TARGET_PROPERTY:EnTT,INTERFACE_COMPILE_OPTIONS>)

macro(SETUP_TARGET TARGET_NAME)
    set_target_properties(${TARGET_NAME} PROPERTIES CXX_EXTENSIONS OFF)
    target_link_libraries(${TARGET_NAME} PRIVATE EnTT)

    target_compile_options(
        ${TARGET_NAME}
        PRIVATE $<$<AND:$<CONFIG:Debug>,$<NOT:$<PLATFORM_ID:Windows>>>:-O0 -g -pedantic -Wall -Wshadow -fvisibility=hidden>
        PRIVATE $<$<AND:$<CONFIG:Release>,$<NOT:$<PLATFORM_ID:Windows>>>:-O2 -pedantic -Wall -Wshadow -fvisibility=hidden>
        PRIVATE $<$<AND:$<CONFIG:Debug>,$<PLATFORM_ID:Windows>>:/EHsc /W1>
        PRIVATE $<$<AND:$<CONFIG:Release>,$<PLATFORM_ID:Windows>>:/EHsc /W1 /O2>
    )
endmacro()

add_library(odr OBJECT odr.cpp)
SETUP_TARGET(odr)

macro(SETUP_BASIC_TEST TEST_NAME TEST_SOURCE)
    add_executable(${TEST_NAME} $<TARGET_OBJECTS:odr> ${TEST_SOURCE})
    target_link_libraries(${TEST_NAME} PRIVATE GTest::Main Threads::Threads)
    SETUP_TARGET(${TEST_NAME})
    add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})
endmacro()

macro(SETUP_LIB_TEST TEST_NAME)
    add_library(_${TEST_NAME} SHARED lib/${TEST_NAME}/lib.cpp)
    SETUP_TARGET(_${TEST_NAME})
    SETUP_BASIC_TEST(lib_${TEST_NAME} lib/${TEST_NAME}/main.cpp)
    target_compile_definitions(_${TEST_NAME} PRIVATE ENTT_API_EXPORT)
    target_compile_definitions(lib_${TEST_NAME} PRIVATE ENTT_API_IMPORT)
    target_link_libraries(lib_${TEST_NAME} PRIVATE _${TEST_NAME})
endmacro()

macro(SETUP_PLUGIN_TEST TEST_NAME)
    add_library(_${TEST_NAME} MODULE lib/${TEST_NAME}/plugin.cpp)
    SETUP_TARGET(_${TEST_NAME})
    SETUP_BASIC_TEST(lib_${TEST_NAME} lib/${TEST_NAME}/main.cpp)
    target_link_libraries(lib_${TEST_NAME} PRIVATE ${CMAKE_DL_LIBS})
    target_include_directories(lib_${TEST_NAME} PRIVATE ${CR_SRC_DIR})
    target_compile_definitions(lib_${TEST_NAME} PRIVATE NOMINMAX PLUGIN="$<TARGET_FILE:_${TEST_NAME}>")
    target_include_directories(_${TEST_NAME} PRIVATE ${CR_SRC_DIR})
    target_compile_definitions(_${TEST_NAME} PRIVATE NOMINMAX)
endmacro()

# Test benchmark

if(BUILD_BENCHMARK)
    SETUP_BASIC_TEST(benchmark benchmark/benchmark.cpp)
endif()

# Test lib

if(BUILD_LIB)
    set(CR_DEPS_DIR ${EnTT_SOURCE_DIR}/deps/cr)
    configure_file(${EnTT_SOURCE_DIR}/cmake/in/cr.in ${CR_DEPS_DIR}/CMakeLists.txt)
    execute_process(COMMAND ${CMAKE_COMMAND} -G "${CMAKE_GENERATOR}" . WORKING_DIRECTORY ${CR_DEPS_DIR})
    execute_process(COMMAND ${CMAKE_COMMAND} --build . WORKING_DIRECTORY ${CR_DEPS_DIR})
    set(CR_SRC_DIR ${CR_DEPS_DIR}/src)

    SETUP_LIB_TEST(dispatcher)
    SETUP_LIB_TEST(emitter)
    SETUP_LIB_TEST(meta)
    SETUP_LIB_TEST(registry)

    SETUP_PLUGIN_TEST(dispatcher_plugin)
    SETUP_PLUGIN_TEST(emitter_plugin)
    SETUP_PLUGIN_TEST(meta_plugin)
    SETUP_PLUGIN_TEST(registry_plugin)
endif()

# Test mod

if(BUILD_MOD)
    enable_language(C)

    set(DUKTAPE_DEPS_DIR ${EnTT_SOURCE_DIR}/deps/duktape)
    configure_file(${EnTT_SOURCE_DIR}/cmake/in/duktape.in ${DUKTAPE_DEPS_DIR}/CMakeLists.txt)
    execute_process(COMMAND ${CMAKE_COMMAND} -G "${CMAKE_GENERATOR}" . WORKING_DIRECTORY ${DUKTAPE_DEPS_DIR})
    execute_process(COMMAND ${CMAKE_COMMAND} --build . WORKING_DIRECTORY ${DUKTAPE_DEPS_DIR})
    set(DUKTAPE_SRC_DIR ${DUKTAPE_DEPS_DIR}/src/src)

    set(MOD_TEST_SOURCE ${DUKTAPE_SRC_DIR}/duktape.c mod/mod.cpp)
    SETUP_BASIC_TEST(mod "${MOD_TEST_SOURCE}")
    target_include_directories(mod PRIVATE ${DUKTAPE_SRC_DIR})
endif()

# Test snapshot

if(BUILD_SNAPSHOT)
    set(CEREAL_DEPS_DIR ${EnTT_SOURCE_DIR}/deps/cereal)
    configure_file(${EnTT_SOURCE_DIR}/cmake/in/cereal.in ${CEREAL_DEPS_DIR}/CMakeLists.txt)
    execute_process(COMMAND ${CMAKE_COMMAND} -G "${CMAKE_GENERATOR}" . WORKING_DIRECTORY ${CEREAL_DEPS_DIR})
    execute_process(COMMAND ${CMAKE_COMMAND} --build . WORKING_DIRECTORY ${CEREAL_DEPS_DIR})
    set(CEREAL_SRC_DIR ${CEREAL_DEPS_DIR}/src/include)

    SETUP_BASIC_TEST(cereal snapshot/snapshot.cpp)
    target_include_directories(cereal PRIVATE ${CEREAL_SRC_DIR})
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

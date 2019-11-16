#
# Tests configuration
#

include_directories($<TARGET_PROPERTY:EnTT,INTERFACE_INCLUDE_DIRECTORIES>)
add_compile_options($<TARGET_PROPERTY:EnTT,INTERFACE_COMPILE_OPTIONS>)

macro(SETUP_TARGET TARGET_NAME)
    set_target_properties(${TARGET_NAME} PROPERTIES CXX_EXTENSIONS OFF)
    target_link_libraries(${TARGET_NAME} PRIVATE EnTT)

    target_compile_options(
        ${TARGET_NAME}
        PRIVATE $<$<AND:$<CONFIG:Debug>,$<NOT:$<PLATFORM_ID:Windows>>>:-O0 -g -pedantic -Wall -Wshadow>
        PRIVATE $<$<AND:$<CONFIG:Release>,$<NOT:$<PLATFORM_ID:Windows>>>:-O2 -pedantic -Wall -Wshadow>
        PRIVATE $<$<AND:$<CONFIG:Debug>,$<PLATFORM_ID:Windows>>:/EHsc /W1>
        PRIVATE $<$<AND:$<CONFIG:Release>,$<PLATFORM_ID:Windows>>:/EHsc /W1 /O2>
    )
endmacro()

add_library(odr OBJECT odr.cpp)
SETUP_TARGET(odr)

macro(SETUP_AND_ADD_TEST TEST_NAME TEST_SOURCE)
    add_executable(${TEST_NAME} $<TARGET_OBJECTS:odr> ${TEST_SOURCE})
    target_link_libraries(${TEST_NAME} PRIVATE GTest::Main Threads::Threads)
    SETUP_TARGET(${TEST_NAME})
    add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})
endmacro()

# Test benchmark

if(BUILD_BENCHMARK)
    SETUP_AND_ADD_TEST(benchmark benchmark/benchmark.cpp)
endif()

# Test lib

if(BUILD_LIB)
    add_library(a_module_shared SHARED lib/a_module.cpp)
    add_library(a_module_static STATIC lib/a_module.cpp)
    add_library(another_module_shared SHARED lib/another_module.cpp)
    add_library(another_module_static STATIC lib/another_module.cpp)

    SETUP_TARGET(a_module_shared)
    SETUP_TARGET(a_module_static)
    SETUP_TARGET(another_module_shared)
    SETUP_TARGET(another_module_static)

    SETUP_AND_ADD_TEST(lib_shared lib/lib.cpp)
    target_link_libraries(lib_shared PRIVATE a_module_shared another_module_shared)

    SETUP_AND_ADD_TEST(lib_static lib/lib.cpp)
    target_link_libraries(lib_static PRIVATE a_module_static another_module_static)
endif()

# Test mod

if(BUILD_MOD)
    set(DUKTAPE_DEPS_DIR ${EnTT_SOURCE_DIR}/deps/duktape)
    configure_file(${EnTT_SOURCE_DIR}/cmake/in/duktape.in ${DUKTAPE_DEPS_DIR}/CMakeLists.txt)
    execute_process(COMMAND ${CMAKE_COMMAND} -G "${CMAKE_GENERATOR}" . WORKING_DIRECTORY ${DUKTAPE_DEPS_DIR})
    execute_process(COMMAND ${CMAKE_COMMAND} --build . WORKING_DIRECTORY ${DUKTAPE_DEPS_DIR})
    set(DUKTAPE_SRC_DIR ${DUKTAPE_DEPS_DIR}/src/src)

    set(MOD_TEST_SOURCE ${DUKTAPE_SRC_DIR}/duktape.c mod/mod.cpp)
    SETUP_AND_ADD_TEST(mod "${MOD_TEST_SOURCE}")
    target_include_directories(mod PRIVATE ${DUKTAPE_SRC_DIR})
endif()

# Test snapshot

if(BUILD_SNAPSHOT)
    set(CEREAL_DEPS_DIR ${EnTT_SOURCE_DIR}/deps/cereal)
    configure_file(${EnTT_SOURCE_DIR}/cmake/in/cereal.in ${CEREAL_DEPS_DIR}/CMakeLists.txt)
    execute_process(COMMAND ${CMAKE_COMMAND} -G "${CMAKE_GENERATOR}" . WORKING_DIRECTORY ${CEREAL_DEPS_DIR})
    execute_process(COMMAND ${CMAKE_COMMAND} --build . WORKING_DIRECTORY ${CEREAL_DEPS_DIR})
    set(CEREAL_SRC_DIR ${CEREAL_DEPS_DIR}/src/include)

    SETUP_AND_ADD_TEST(cereal snapshot/snapshot.cpp)
    target_include_directories(cereal PRIVATE ${CEREAL_SRC_DIR})
endif()

# Test core

SETUP_AND_ADD_TEST(algorithm entt/core/algorithm.cpp)
SETUP_AND_ADD_TEST(family entt/core/family.cpp)
SETUP_AND_ADD_TEST(hashed_string entt/core/hashed_string.cpp)
SETUP_AND_ADD_TEST(ident entt/core/ident.cpp)
SETUP_AND_ADD_TEST(monostate entt/core/monostate.cpp)
SETUP_AND_ADD_TEST(type_traits entt/core/type_traits.cpp)
SETUP_AND_ADD_TEST(utility entt/core/utility.cpp)

# Test entity

SETUP_AND_ADD_TEST(actor entt/entity/actor.cpp)
SETUP_AND_ADD_TEST(entity entt/entity/entity.cpp)
SETUP_AND_ADD_TEST(group entt/entity/group.cpp)
SETUP_AND_ADD_TEST(helper entt/entity/helper.cpp)
SETUP_AND_ADD_TEST(observer entt/entity/observer.cpp)
SETUP_AND_ADD_TEST(registry entt/entity/registry.cpp)
SETUP_AND_ADD_TEST(runtime_view entt/entity/runtime_view.cpp)
SETUP_AND_ADD_TEST(snapshot entt/entity/snapshot.cpp)
SETUP_AND_ADD_TEST(sparse_set entt/entity/sparse_set.cpp)
SETUP_AND_ADD_TEST(storage entt/entity/storage.cpp)
SETUP_AND_ADD_TEST(view entt/entity/view.cpp)

# Test locator

SETUP_AND_ADD_TEST(locator entt/locator/locator.cpp)

# Test meta

SETUP_AND_ADD_TEST(meta entt/meta/meta.cpp)

# Test process

SETUP_AND_ADD_TEST(process entt/process/process.cpp)
SETUP_AND_ADD_TEST(scheduler entt/process/scheduler.cpp)

# Test resource

SETUP_AND_ADD_TEST(resource entt/resource/resource.cpp)

# Test signal

SETUP_AND_ADD_TEST(delegate entt/signal/delegate.cpp)
SETUP_AND_ADD_TEST(dispatcher entt/signal/dispatcher.cpp)
SETUP_AND_ADD_TEST(emitter entt/signal/emitter.cpp)
SETUP_AND_ADD_TEST(sigh entt/signal/sigh.cpp)

#
# Tests configuration
#

include_directories($<TARGET_PROPERTY:EnTT,INTERFACE_INCLUDE_DIRECTORIES>)
add_compile_options($<TARGET_PROPERTY:EnTT,INTERFACE_COMPILE_OPTIONS>)

add_library(odr OBJECT odr.cpp)
set_target_properties(odr PROPERTIES CXX_EXTENSIONS OFF)
target_compile_definitions(odr PRIVATE $<TARGET_PROPERTY:EnTT,INTERFACE_COMPILE_DEFINITIONS>)
target_compile_features(odr PRIVATE $<TARGET_PROPERTY:EnTT,INTERFACE_COMPILE_FEATURES>)
target_compile_options(odr PRIVATE $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-pedantic -Wall>)
target_compile_options(odr PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/EHsc>)

macro(SETUP_AND_ADD_TEST TEST_NAME TEST_SOURCE)
    add_executable(${TEST_NAME} $<TARGET_OBJECTS:odr> ${TEST_SOURCE})
    set_target_properties(${TEST_NAME} PROPERTIES CXX_EXTENSIONS OFF)
    target_link_libraries(${TEST_NAME} PRIVATE EnTT GTest::Main Threads::Threads)
    target_compile_definitions(${TEST_NAME} PRIVATE $<TARGET_PROPERTY:EnTT,INTERFACE_COMPILE_DEFINITIONS>)
    target_compile_features(${TEST_NAME} PRIVATE $<TARGET_PROPERTY:EnTT,INTERFACE_COMPILE_FEATURES>)
    target_compile_options(${TEST_NAME} PRIVATE $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-pedantic -Wall>)
    target_compile_options(${TEST_NAME} PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/EHsc>)
    add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})
endmacro()

# Test benchmark

if(BUILD_BENCHMARK)
    SETUP_AND_ADD_TEST(benchmark benchmark/benchmark.cpp)
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

# Test entity

SETUP_AND_ADD_TEST(actor entt/entity/actor.cpp)
SETUP_AND_ADD_TEST(attachee entt/entity/attachee.cpp)
SETUP_AND_ADD_TEST(entity entt/entity/entity.cpp)
SETUP_AND_ADD_TEST(helper entt/entity/helper.cpp)
SETUP_AND_ADD_TEST(prototype entt/entity/prototype.cpp)
SETUP_AND_ADD_TEST(registry entt/entity/registry.cpp)
SETUP_AND_ADD_TEST(snapshot entt/entity/snapshot.cpp)
SETUP_AND_ADD_TEST(sparse_set entt/entity/sparse_set.cpp)
SETUP_AND_ADD_TEST(view entt/entity/view.cpp)

# Test locator

SETUP_AND_ADD_TEST(locator entt/locator/locator.cpp)

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

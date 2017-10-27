#
# Tests configuration
#

include_directories(${PROJECT_SRC_DIR})

add_library(odr OBJECT odr.cpp)

# Test benchmark

if(CMAKE_BUILD_TYPE MATCHES Release)
    add_executable(
        benchmark
        $<TARGET_OBJECTS:odr>
        entt/entity/benchmark.cpp
    )
    target_link_libraries(benchmark PRIVATE gtest_main Threads::Threads)
    add_test(NAME benchmark COMMAND benchmark)
endif()

# Test core

add_executable(
    core
    $<TARGET_OBJECTS:odr>
    entt/core/family.cpp
    entt/core/ident.cpp
)
target_link_libraries(core PRIVATE gtest_main Threads::Threads)
add_test(NAME core COMMAND core)

# Test entity

add_executable(
    entity
    $<TARGET_OBJECTS:odr>
    entt/entity/registry.cpp
    entt/entity/sparse_set.cpp
    entt/entity/view.cpp
)
target_link_libraries(entity PRIVATE gtest_main Threads::Threads)
add_test(NAME entity COMMAND entity)

# Test locator

add_executable(
    locator
    $<TARGET_OBJECTS:odr>
    entt/locator/locator.cpp
)
target_link_libraries(locator PRIVATE gtest_main Threads::Threads)
add_test(NAME locator COMMAND locator)

# Test locator

add_executable(
    signal
    $<TARGET_OBJECTS:odr>
    entt/signal/bus.cpp
    entt/signal/delegate.cpp
    entt/signal/dispatcher.cpp
    entt/signal/emitter.cpp
    entt/signal/sigh.cpp
    entt/signal/signal.cpp
)
target_link_libraries(signal PRIVATE gtest_main Threads::Threads)
add_test(NAME signal COMMAND signal)

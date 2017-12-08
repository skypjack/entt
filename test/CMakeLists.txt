#
# Tests configuration
#

add_library(odr OBJECT odr.cpp)

# Test benchmark

if(BUILD_BENCHMARK)
    add_executable(
        benchmark
        $<TARGET_OBJECTS:odr>
        benchmark/benchmark.cpp
    )
    target_link_libraries(benchmark PRIVATE gtest_main Threads::Threads)
    add_test(NAME benchmark COMMAND benchmark)
endif()

# Test mod

if(BUILD_MOD)
    add_executable(
        mod
        $<TARGET_OBJECTS:odr>
        mod/duktape.c
        mod/mod.cpp
    )
    target_link_libraries(mod PRIVATE gtest_main Threads::Threads m)
    add_test(NAME mod COMMAND mod)
endif()

# Test core

add_executable(
    core
    $<TARGET_OBJECTS:odr>
    entt/core/family.cpp
    entt/core/hashed_string.cpp
    entt/core/ident.cpp
)
target_link_libraries(core PRIVATE gtest_main Threads::Threads)
add_test(NAME core COMMAND core)

# Test entity

add_executable(
    entity
    $<TARGET_OBJECTS:odr>
    entt/entity/actor.cpp
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

# Test process

add_executable(
    process
    $<TARGET_OBJECTS:odr>
    entt/process/process.cpp
    entt/process/scheduler.cpp
)
target_link_libraries(process PRIVATE gtest_main Threads::Threads)
add_test(NAME process COMMAND process)

# Test resource

add_executable(
    resource
    $<TARGET_OBJECTS:odr>
    entt/resource/resource.cpp
)
target_link_libraries(resource PRIVATE gtest_main Threads::Threads)
add_test(NAME resource COMMAND resource)

# Test signal

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

#
# Tests configuration
#

set(COMMON_LINK_LIBS gtest_main Threads::Threads)
include_directories(${PROJECT_SRC_DIR})

# Test core

add_executable(core entt/core/ident.cpp odr.cpp)
target_link_libraries(core PRIVATE ${COMMON_LINK_LIBS})
add_test(NAME core COMMAND core)

# Test entt

add_executable(entity entt/entity/registry.cpp entt/entity/sparse_set.cpp entt/entity/view.cpp odr.cpp)
target_link_libraries(entity PRIVATE ${COMMON_LINK_LIBS})
add_test(NAME entity COMMAND entity)

# Test benchmark

IF(CMAKE_BUILD_TYPE MATCHES Release)
    add_executable(benchmark entt/entity/benchmark.cpp odr.cpp)
    target_link_libraries(benchmark PRIVATE ${COMMON_LINK_LIBS})
    add_test(NAME benchmark COMMAND benchmark)
ENDIF()

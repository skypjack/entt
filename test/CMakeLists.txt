#
# Tests configuration
#

set(COMMON_LINK_LIBS gtest_main Threads::Threads)

# List of available targets

set(TARGET_ENTT entt)
set(TARGET_BENCHMARK benchmark)

# Test TARGET_ENTT

add_executable(${TARGET_ENTT} ident.cpp registry.cpp sparse_set.cpp)
target_include_directories(${TARGET_ENTT} PRIVATE ${PROJECT_SRC_DIR})
target_link_libraries(${TARGET_ENTT} PRIVATE ${COMMON_LINK_LIBS})
add_test(NAME ${TARGET_ENTT} COMMAND ${TARGET_ENTT})

# Test TARGET_BENCHMARK

IF(CMAKE_BUILD_TYPE MATCHES Release)
    add_executable(${TARGET_BENCHMARK} benchmark.cpp)
    target_include_directories(${TARGET_BENCHMARK} PRIVATE ${PROJECT_SRC_DIR})
    target_link_libraries(${TARGET_BENCHMARK} PRIVATE ${COMMON_LINK_LIBS})
    add_test(NAME ${TARGET_BENCHMARK} COMMAND ${TARGET_BENCHMARK})
ENDIF()

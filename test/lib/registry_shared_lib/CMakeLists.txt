
add_library(shared_registry SHARED lib_registry.cpp)
target_link_libraries(shared_registry PUBLIC EnTT::EnTT)
target_compile_definitions(shared_registry
        PRIVATE ENTT_API_EXPORT
        INTERFACE ENTT_API_IMPORT
        )

add_library(importing_plugin SHARED plugin.cpp)
target_link_libraries(importing_plugin shared_registry)

add_executable(importing_core main.cpp)
target_link_libraries(importing_core shared_registry GTest::gtest)
target_compile_definitions(importing_core PRIVATE PLUGIN_PATH="$<TARGET_FILE:importing_plugin>")

if (UNIX)
    target_link_libraries(importing_core dl)
endif()

add_dependencies(importing_core importing_plugin)

add_test(NAME shared_registry_lib
        COMMAND importing_core)
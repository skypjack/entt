#include <entt/entity/registry.hpp>

#ifndef LIB_EXPORT
#if defined _WIN32 || defined __CYGWIN__
#define LIB_EXPORT __declspec(dllexport)
#elif defined __GNUC__
#define LIB_EXPORT __attribute__((visibility("default")))
#else
#define LIB_EXPORT
#endif
#endif

LIB_EXPORT typename entt::registry<>::component_type a_module_int_type() {
    entt::registry<> registry;

    (void)registry.type<double>();
    (void)registry.type<float>();

    return registry.type<int>();
}

LIB_EXPORT typename entt::registry<>::component_type a_module_char_type() {
    entt::registry<> registry;

    (void)registry.type<double>();
    (void)registry.type<float>();

    return registry.type<char>();
}

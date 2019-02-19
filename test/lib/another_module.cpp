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

ENTT_SHARED_TYPE(int)
ENTT_SHARED_TYPE(char)
ENTT_SHARED_TYPE(double)
ENTT_SHARED_TYPE(float)

LIB_EXPORT typename entt::registry<>::component_type another_module_int_type() {
    entt::registry<> registry;

    (void)registry.type<char>();
    (void)registry.type<const int>();
    (void)registry.type<double>();
    (void)registry.type<const char>();
    (void)registry.type<float>();

    return registry.type<int>();
}

LIB_EXPORT typename entt::registry<>::component_type another_module_char_type() {
    entt::registry<> registry;

    (void)registry.type<int>();
    (void)registry.type<const char>();
    (void)registry.type<float>();
    (void)registry.type<const int>();
    (void)registry.type<double>();

    return registry.type<char>();
}

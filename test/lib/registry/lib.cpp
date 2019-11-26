#include <entt/entity/registry.hpp>
#include "types.h"

#ifndef LIB_EXPORT
#if defined _WIN32 || defined __CYGWIN__
#define LIB_EXPORT __declspec(dllexport)
#elif defined __GNUC__
#define LIB_EXPORT __attribute__((visibility("default")))
#else
#define LIB_EXPORT
#endif
#endif

LIB_EXPORT typename entt::component int_type() {
    entt::registry registry;

    (void)registry.type<double>();
    (void)registry.type<float>();

    return registry.type<int>();
}

LIB_EXPORT typename entt::component char_type() {
    entt::registry registry;

    (void)registry.type<double>();
    (void)registry.type<float>();

    return registry.type<char>();
}

LIB_EXPORT void update_position(int delta, entt::registry &registry) {
    registry.view<position, velocity>().each([delta](auto &pos, auto &vel) {
        pos.x += delta * vel.dx;
        pos.y += delta * vel.dy;
    });
}

LIB_EXPORT void assign_velocity(int vel, entt::registry &registry) {
    for(auto entity: registry.view<position>()) {
        registry.assign<velocity>(entity, vel, vel);
    }
}

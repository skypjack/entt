#define ENTT_API_EXPORT

#include <entt/entity/registry.hpp>
#include <entt/lib/attribute.h>
#include "types.h"

ENTT_API entt::component int_type() {
    entt::registry registry;

    (void)registry.type<double>();
    (void)registry.type<float>();

    return registry.type<int>();
}

ENTT_API entt::component char_type() {
    entt::registry registry;

    (void)registry.type<double>();
    (void)registry.type<float>();

    return registry.type<char>();
}

ENTT_API void update_position(int delta, entt::registry &registry) {
    registry.view<position, velocity>().each([delta](auto &pos, auto &vel) {
        pos.x += delta * vel.dx;
        pos.y += delta * vel.dy;
    });
}

ENTT_API void assign_velocity(int vel, entt::registry &registry) {
    for(auto entity: registry.view<position>()) {
        registry.assign<velocity>(entity, vel, vel);
    }
}

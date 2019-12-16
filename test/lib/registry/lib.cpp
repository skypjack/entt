#include <entt/core/attribute.h>
#include <entt/entity/registry.hpp>
#include "types.h"

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

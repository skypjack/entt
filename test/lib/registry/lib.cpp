#include <entt/core/attribute.h>
#include <entt/entity/registry.hpp>
#include "types.h"

ENTT_API void update_position(entt::registry &registry) {
    registry.view<position, velocity>().each([](auto &pos, auto &vel) {
        pos.x += 16 * vel.dx;
        pos.y += 16 * vel.dy;
    });
}

ENTT_API void assign_velocity(entt::registry &registry) {
    for(auto entity: registry.view<position>()) {
        registry.assign<velocity>(entity, 1., 1.);
    }
}

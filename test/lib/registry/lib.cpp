#include <entt/core/attribute.h>
#include <entt/entity/registry.hpp>
#include "types.h"

ENTT_API void update_position(entt::registry &registry) {
    registry.view<position, velocity>().each([](auto &pos, auto &vel) {
        pos.x += static_cast<int>(16 * vel.dx);
        pos.y += static_cast<int>(16 * vel.dy);
    });
}

ENTT_API void emplace_velocity(entt::registry &registry) {
    // forces the creation of the pool for the velocity component
    static_cast<void>(registry.storage<velocity>());

    for(auto entity: registry.view<position>()) {
        registry.emplace<velocity>(entity, 1., 1.);
    }
}

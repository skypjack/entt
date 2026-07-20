#include <application/context.h>
#include <component/position_component.h>
#include <component/rect_component.h>
#include <component/velocity_component.h>
#include <entt/entity/registry.hpp>
#include <system/movement_system.h>

namespace testbed {

void movement_system(entt::registry &registry, const context &ctx, const double delta) {
    int width{};
    int height{};

    for(auto [entity, position, velocity, rect]: registry.view<position_component, velocity_component, rect_component>().each()) {
        position.x += (velocity.dx * delta) * (ctx.logical_width() / 10.);
        position.y += velocity.dy * delta * (ctx.logical_height() / 10.);

        if(position.x < 0) {
            position.x = 0.;
            velocity.dx = 1.;
        } else if(auto pos = position.x + rect.w; pos > ctx.logical_width()) {
            position.x = ctx.logical_width() - rect.w;
            velocity.dx = -1;
        }

        if(position.y < 0) {
            position.y = 0.;
            velocity.dy = 1.;
        } else if(auto pos = position.y + rect.h; pos > ctx.logical_height()) {
            position.y = ctx.logical_height() - rect.h;
            velocity.dy = -1;
        }
    }
}

} // namespace testbed

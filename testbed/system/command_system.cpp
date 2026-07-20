#include <component/input_listener_component.h>
#include <component/rect_component.h>
#include <component/velocity_component.h>
#include <entt/entity/registry.hpp>
#include <system/command_system.h>

namespace testbed {

void command_system(entt::registry &registry) {
    for([[maybe_unused]] auto [entt, input]: registry.view<input_listener_component>().each()) {
        switch(input.command) {
        case input_listener_component::type::UP:
            registry.get<velocity_component>(entt).dy = -1.0f;
            break;
        case input_listener_component::type::DOWN:
            registry.get<velocity_component>(entt).dy = 1.0f;
            break;
        case input_listener_component::type::LEFT:
            registry.get<velocity_component>(entt).dx = -1.0f;
            break;
        case input_listener_component::type::RIGHT:
            registry.get<velocity_component>(entt).dx = 1.0f;
            break;
        case input_listener_component::type::STOP: {
            auto &vel = registry.get<velocity_component>(entt);
            vel.dx = vel.dy = 0.0f;
        } break;
        case input_listener_component::type::PLUS: {
            registry.get<rect_component>(entt).w *= 1.1;
            registry.get<rect_component>(entt).h *= 1.1;
        } break;
        case input_listener_component::type::MINUS: {
            registry.get<rect_component>(entt).w *= 0.9;
            registry.get<rect_component>(entt).h *= 0.9;
        } break;
        default:
            break;
        }

        input.command = input_listener_component::type::NONE;
    }
}

} // namespace testbed

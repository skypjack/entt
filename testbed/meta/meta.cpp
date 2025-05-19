#include <component/input_listener_component.h>
#include <component/position_component.h>
#include <component/rect_component.h>
#include <component/renderable_component.h>
#include <entt/meta/factory.hpp>
#include <meta/meta.h>

namespace testbed {

void meta_setup() {
    entt::meta_factory<input_listener_component::type>()
        .type("command type")
        .data<input_listener_component::type::UP>("up")
        .data<input_listener_component::type::DOWN>("down")
        .data<input_listener_component::type::LEFT>("left")
        .data<input_listener_component::type::RIGHT>("right");

    entt::meta_factory<input_listener_component>()
        .type("input listener")
        .data<&input_listener_component::command>("command");

    entt::meta_factory<position_component>()
        .type("position")
        .data<&SDL_FPoint::x>("x")
        .data<&SDL_FPoint::y>("y");

    entt::meta_factory<rect_component>()
        .type("rect")
        .data<&SDL_FRect::x>("x")
        .data<&SDL_FRect::y>("y")
        .data<&SDL_FRect::w>("w")
        .data<&SDL_FRect::h>("h");

    entt::meta_factory<renderable_component>()
        .type("renderable");
}

} // namespace testbed

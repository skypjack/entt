#include <component/input_listener_component.h>
#include <component/position_component.h>
#include <component/rect_component.h>
#include <component/renderable_component.h>
#include <entt/meta/factory.hpp>
#include <meta/meta.h>

namespace testbed {

void meta_setup() {
    entt::meta_factory<input_listener_component>()
        .type("input listener")
        .data<&input_listener_component::command>("command");

    entt::meta_factory<position_component>()
        .type("position")
        .data<&SDL_FPoint::x>("x")
        .data<&SDL_FPoint::y>("y");

    // bind components...
}

} // namespace testbed

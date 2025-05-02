#include <component/input_listener_component.h>
#include <component/position_component.h>
#include <component/rect_component.h>
#include <component/renderable_component.h>
#include <entt/core/hashed_string.hpp>
#include <entt/davey/meta.hpp>
#include <entt/meta/factory.hpp>
#include <meta/meta.h>

namespace testbed {

void meta_setup() {
    using namespace entt::literals;

    entt::meta_factory<input_listener_component>()
        .custom<entt::davey_data>("input listener")
        .data<&input_listener_component::command>("command"_hs)
        .custom<entt::davey_data>("command");

    entt::meta_factory<position_component>()
        .custom<entt::davey_data>("position")
        .data<&SDL_FPoint::x>("x"_hs)
        .custom<entt::davey_data>("x")
        .data<&SDL_FPoint::y>("y"_hs)
        .custom<entt::davey_data>("y");

    // bind components...
}

} // namespace testbed

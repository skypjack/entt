#include <SDL3/SDL_rect.h>
#include <component/collectible_component.h>
#include <component/color_component.h>
#include <component/game_state_component.h>
#include <component/input_listener_component.h>
#include <component/player_component.h>
#include <component/position_component.h>
#include <component/rect_component.h>
#include <component/renderable_component.h>
#include <entt/meta/factory.hpp>
#include <meta/meta.h>

namespace testbed {

void meta_setup() {
    entt::meta_factory<SDL_FPoint>()
        .type("sdl_fpoint")
        .data<&SDL_FPoint::x>("x")
        .data<&SDL_FPoint::y>("y");

    entt::meta_factory<SDL_FRect>()
        .type("sdl_frect")
        .data<&SDL_FRect::x>("x")
        .data<&SDL_FRect::y>("y")
        .data<&SDL_FRect::w>("w")
        .data<&SDL_FRect::h>("h");

    entt::meta_factory<input_listener_component>()
        .type("input listener")
        .data<&input_listener_component::up>("up")
        .data<&input_listener_component::down>("down")
        .data<&input_listener_component::left>("left")
        .data<&input_listener_component::right>("right");

    entt::meta_factory<SDL_FPoint>()
        .type("point");

    entt::meta_factory<position_component>()
        .type("position")
        .base<SDL_FPoint>();

    entt::meta_factory<rect_component>()
        .type("rect")
        .base<SDL_FRect>();

    entt::meta_factory<renderable_component>()
        .type("renderable");

    entt::meta_factory<player_component>()
        .type("player")
        .data<&player_component::speed>("speed");

    entt::meta_factory<collectible_component>()
        .type("collectible")
        .data<&collectible_component::value>("value");

    entt::meta_factory<color_component>()
        .type("color")
        .data<&color_component::red>("red")
        .data<&color_component::green>("green")
        .data<&color_component::blue>("blue");

    entt::meta_factory<game_state_component>()
        .type("game state")
        .data<&game_state_component::score>("score")
        .data<&game_state_component::best_score>("best_score")
        .data<&game_state_component::remaining_time>("remaining_time")
        .data<&game_state_component::running>("running");
}

} // namespace testbed

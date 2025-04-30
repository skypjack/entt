#include <application/context.h>
#include <component/input_listener_component.h>
#include <entt/entity/registry.hpp>
#include <system/input_system.h>

namespace testbed {

namespace internal {

static void update_listeners(entt::registry &registry, input_listener_component::type command) {
    for([[maybe_unused]] auto [entt, elem]: registry.view<input_listener_component>().each()) {
        elem.command = command;
    }
}

} // namespace internal

void input_system(entt::registry &registry, const SDL_Event &event, bool &quit) {
    switch(event.type) {
    case SDL_EVENT_QUIT:
        quit = true;
        break;
    case SDL_EVENT_KEY_DOWN:
        switch(event.key.key) {
        case SDLK_ESCAPE:
            quit = true;
            break;
        case SDLK_UP:
            internal::update_listeners(registry, input_listener_component::type::UP);
            break;
        case SDLK_DOWN:
            internal::update_listeners(registry, input_listener_component::type::DOWN);
            break;
        case SDLK_LEFT:
            internal::update_listeners(registry, input_listener_component::type::LEFT);
            break;
        case SDLK_RIGHT:
            internal::update_listeners(registry, input_listener_component::type::RIGHT);
            break;
        }
        break;
    }
}

} // namespace testbed

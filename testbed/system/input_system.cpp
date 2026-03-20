#include <application/context.h>
#include <component/input_listener_component.h>
#include <entt/entity/registry.hpp>
#include <system/input_system.h>

namespace testbed {

namespace internal {

static void update_listeners(entt::registry &registry, SDL_Keycode key, bool pressed) {
    for([[maybe_unused]] auto [entt, elem]: registry.view<input_listener_component>().each()) {
        switch(key) {
        case SDLK_UP:
        case SDLK_W:
            elem.up = pressed;
            break;
        case SDLK_DOWN:
        case SDLK_S:
            elem.down = pressed;
            break;
        case SDLK_LEFT:
        case SDLK_A:
            elem.left = pressed;
            break;
        case SDLK_RIGHT:
        case SDLK_D:
            elem.right = pressed;
            break;
        }
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
        default:
            internal::update_listeners(registry, event.key.key, true);
            break;
        }
        break;
    case SDL_EVENT_KEY_UP:
        internal::update_listeners(registry, event.key.key, false);
        break;
    }
}

} // namespace testbed

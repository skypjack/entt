#include <application/context.h>
#include <entt/entity/registry.hpp>
#include <system/input_system.h>

namespace testbed {

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
        }
        break;
    }
}

} // namespace testbed

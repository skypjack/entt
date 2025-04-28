#pragma once

#include <SDL3/SDL_events.h>
#include <entt/entity/fwd.hpp>

namespace testbed {

void input_system(entt::registry &, const SDL_Event &, bool &);

} // namespace testbed

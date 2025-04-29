#pragma once

#include <entt/entity/fwd.hpp>

namespace testbed {

struct context;

void hud_system(entt::registry &, const context &);

} // namespace testbed

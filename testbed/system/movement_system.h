#pragma once

#include <entt/entity/fwd.hpp>

namespace testbed {

struct context;

void movement_system(entt::registry &, const context &, const double);

} // namespace testbed

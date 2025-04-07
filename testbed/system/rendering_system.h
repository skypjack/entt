#pragma once

#include <entt/entity/fwd.hpp>

namespace testbed {

struct context;

void rendering_system(entt::registry &, const context &);

} // namespace testbed

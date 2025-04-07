#include <application/context.h>
#include <entt/entity/registry.hpp>
#include <system/rendering_system.h>

namespace testbed {

void rendering_system(entt::registry &registry, const context &ctx) {
    // render...
    static_cast<void>(registry);
    static_cast<void>(ctx);
}

} // namespace testbed

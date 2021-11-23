#include <cr.h>
#include <entt/entity/registry.hpp>
#include "types.h"

CR_EXPORT int cr_main(cr_plugin *ctx, cr_op operation) {
    switch(operation) {
    case CR_STEP: {
        // forces things to break
        auto &registry = *static_cast<entt::registry *>(ctx->userdata);

        // forces the creation of the pool for the velocity component
        static_cast<void>(registry.storage<velocity>());

        const auto view = registry.view<position>();
        registry.insert(view.begin(), view.end(), velocity{1., 1.});

        registry.view<position, velocity>().each([](position &pos, velocity &vel) {
            pos.x += static_cast<int>(16 * vel.dx);
            pos.y += static_cast<int>(16 * vel.dy);
        });
    } break;
    case CR_CLOSE:
    case CR_LOAD:
    case CR_UNLOAD:
        // nothing to do here, this is only a test.
        break;
    }

    return 0;
}

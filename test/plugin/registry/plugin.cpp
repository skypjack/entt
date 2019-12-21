#include <cr.h>
#include <entt/entity/registry.hpp>
#include "types.h"

CR_EXPORT int cr_main(cr_plugin *ctx, cr_op operation) {
    switch (operation) {
    case CR_STEP:
        [ctx]() {
            auto *registry = static_cast<entt::registry *>(ctx->userdata);

            for(auto entity: registry->view<position>()) {
                registry->assign<velocity>(entity, 1., 1.);
            }

            registry->view<position, velocity>().each([](auto &pos, auto &vel) {
                pos.x += 16 * vel.dx;
                pos.y += 16 * vel.dy;
            });
        }();
        break;
    case CR_CLOSE:
        static_cast<entt::registry *>(ctx->userdata)->discard<velocity>();
        break;
    case CR_LOAD:
    case CR_UNLOAD:
        // nothing to do here, this is only a test.
        break;
    }

    return 0;
}

#include <cr.h>
#include <entt/core/type_info.hpp>
#include <entt/entity/registry.hpp>
#include "types.h"

template<typename Type>
struct entt::type_index<Type> {};

CR_EXPORT int cr_main(cr_plugin *ctx, cr_op operation) {
    switch (operation) {
    case CR_STEP:
        [ctx]() {
            auto &registry = *static_cast<entt::registry *>(ctx->userdata);

            const auto position_view = registry.view<position>(entt::exclude<tag>);
            registry.assign(position_view.begin(), position_view.end(), velocity{1., 1.});

            registry.view<position, velocity>().each([](auto &pos, auto &vel) {
                pos.x += static_cast<int>(16 * vel.dx);
                pos.y += static_cast<int>(16 * vel.dy);
            });

            const auto tag_view = registry.view<tag>();
            registry.destroy(tag_view.begin(), tag_view.end());
        }();
        break;
    case CR_CLOSE:
    case CR_LOAD:
    case CR_UNLOAD:
        // nothing to do here, this is only a test.
        break;
    }

    return 0;
}

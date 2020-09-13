#include <cr.h>
#include <entt/core/type_info.hpp>
#include <entt/entity/registry.hpp>
#include "type_context.h"
#include "types.h"

inline static type_context *context;

template<typename Type>
struct entt::type_index<Type> {
    [[nodiscard]] static id_type value() ENTT_NOEXCEPT {
        static const entt::id_type value = context->value(type_info<Type>::id());
        return value;
    }
};

CR_EXPORT int cr_main(cr_plugin *ctx, cr_op operation) {
    switch (operation) {
    case CR_STEP:
        if(!context) {
            context = static_cast<type_context *>(ctx->userdata);
        } else {
            // forces things to break
            static_cast<entt::registry *>(ctx->userdata)->prepare<velocity>();
            
            const auto view = static_cast<entt::registry *>(ctx->userdata)->view<position>();
            static_cast<entt::registry *>(ctx->userdata)->insert(view.begin(), view.end(), velocity{1., 1.});
            
            static_cast<entt::registry *>(ctx->userdata)->view<position, velocity>().each([](auto &pos, auto &vel) {
                pos.x += static_cast<int>(16 * vel.dx);
                pos.y += static_cast<int>(16 * vel.dy);
            });
        }
        break;
    case CR_CLOSE:
    case CR_LOAD:
    case CR_UNLOAD:
        // nothing to do here, this is only a test.
        break;
    }

    return 0;
}

#include <cr.h>
#include <entt/core/type_info.hpp>
#include <entt/signal/emitter.hpp>
#include "type_context.h"
#include "types.h"

struct ctx {
    inline static type_context *ref;
};

template<typename Type>
struct entt::type_seq<Type> {
    [[nodiscard]] static id_type value() ENTT_NOEXCEPT {
        static const entt::id_type value = ctx::ref->value(entt::type_hash<Type>::value());
        return value;
    }
};

CR_EXPORT int cr_main(cr_plugin *ctx, cr_op operation) {
    switch (operation) {
    case CR_STEP:
        if(!ctx::ref) {
            ctx::ref = static_cast<type_context *>(ctx->userdata);
        } else {
            static_cast<test_emitter *>(ctx->userdata)->publish<event>();
            static_cast<test_emitter *>(ctx->userdata)->publish<message>(42);
            static_cast<test_emitter *>(ctx->userdata)->publish<message>(3);
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

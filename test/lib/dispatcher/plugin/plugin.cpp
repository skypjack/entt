#include <cr.h>
#include <entt/signal/dispatcher.hpp>
#include "types.h"

CR_EXPORT int cr_main(cr_plugin *ctx, cr_op operation) {
    switch(operation) {
    case CR_STEP:
        static_cast<entt::dispatcher *>(ctx->userdata)->trigger<event>();
        static_cast<entt::dispatcher *>(ctx->userdata)->trigger(message{42});
        break;
    case CR_CLOSE:
    case CR_LOAD:
    case CR_UNLOAD:
        // nothing to do here, this is only a test.
        break;
    }

    return 0;
}

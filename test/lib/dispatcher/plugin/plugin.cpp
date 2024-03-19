#include <cr.h>
#include <entt/signal/dispatcher.hpp>
#include "../../../common/boxed_type.h"
#include "../../../common/empty.h"

CR_EXPORT int cr_main(cr_plugin *ctx, cr_op operation) {
    switch(operation) {
    case CR_STEP:
        static_cast<entt::dispatcher *>(ctx->userdata)->trigger<test::empty>();
        static_cast<entt::dispatcher *>(ctx->userdata)->trigger(test::boxed_int{4});
        break;
    case CR_CLOSE:
    case CR_LOAD:
    case CR_UNLOAD:
        // nothing to do here, this is only a test.
        break;
    }

    return 0;
}

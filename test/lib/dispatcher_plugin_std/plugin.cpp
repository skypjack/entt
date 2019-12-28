#include <cr.h>
#include "types.h"

CR_EXPORT int cr_main(cr_plugin *ctx, cr_op operation) {
    switch (operation) {
    case CR_STEP:
        static_cast<dispatcher_proxy *>(ctx->userdata)->trigger(event{});
        static_cast<dispatcher_proxy *>(ctx->userdata)->trigger(message{42});
        break;
    case CR_CLOSE:
    case CR_LOAD:
    case CR_UNLOAD:
        // nothing to do here, this is only a test.
        break;
    }

    return 0;
}

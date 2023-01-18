#include <cr.h>
#include "types.h"

CR_EXPORT int cr_main(cr_plugin *ctx, cr_op operation) {
    switch(operation) {
    case CR_STEP:
        static_cast<test_emitter *>(ctx->userdata)->publish(event{});
        static_cast<test_emitter *>(ctx->userdata)->publish(message{42});
        static_cast<test_emitter *>(ctx->userdata)->publish(message{3});
        break;
    case CR_CLOSE:
    case CR_LOAD:
    case CR_UNLOAD:
        // nothing to do here, this is only a test.
        break;
    }

    return 0;
}

#include <cr.h>
#include "../../../common/boxed_type.h"
#include "../../../common/emitter.h"
#include "../../../common/empty.h"

CR_EXPORT int cr_main(cr_plugin *ctx, cr_op operation) {
    switch(operation) {
    case CR_STEP:
        static_cast<test::emitter *>(ctx->userdata)->publish(test::empty{});
        static_cast<test::emitter *>(ctx->userdata)->publish(test::boxed_int{4});
        static_cast<test::emitter *>(ctx->userdata)->publish(test::boxed_int{3});
        break;
    case CR_CLOSE:
    case CR_LOAD:
    case CR_UNLOAD:
        // nothing to do here, this is only a test.
        break;
    }

    return 0;
}

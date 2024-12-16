#include <cr.h>
#include "../types.h"

CR_EXPORT int cr_main(cr_plugin *ctx, cr_op operation) {
    switch(operation) {
    case CR_STEP: {
        // unset filter fallback should not be accessible across boundaries
        auto &view = *static_cast<view_type *>(ctx->userdata);
        ctx->userdata = view.storage<1u>();
    } break;
    case CR_CLOSE:
    case CR_LOAD:
    case CR_UNLOAD:
        // nothing to do here, this is only a test.
        break;
    }

    return 0;
}

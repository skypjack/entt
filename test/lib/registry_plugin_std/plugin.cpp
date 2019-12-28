#include <cr.h>
#include "types.h"

CR_EXPORT int cr_main(cr_plugin *ctx, cr_op operation) {
    switch (operation) {
    case CR_STEP:
        [ctx]() {
            auto *proxy = static_cast<registry_proxy *>(ctx->userdata);

            proxy->assign({1., 1.});

            proxy->for_each([](auto &pos, auto &vel) {
                pos.x += static_cast<int>(16 * vel.dx);
                pos.y += static_cast<int>(16 * vel.dy);
            });
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

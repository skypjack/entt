#include <cr.h>
#include <entt/locator/locator.hpp>
#include "types.h"

CR_EXPORT int cr_main(cr_plugin *ctx, cr_op operation) {
    switch(operation) {
    case CR_LOAD:
        entt::locator<service>::reset(static_cast<userdata *>(ctx->userdata)->handle);
        break;
    case CR_STEP:
        entt::locator<service>::value().value = static_cast<userdata *>(ctx->userdata)->value;
        break;
    case CR_UNLOAD:
    case CR_CLOSE:
        break;
    }

    return 0;
}

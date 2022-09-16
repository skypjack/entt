#include <cr.h>
#include <entt/core/hashed_string.hpp>
#include <entt/meta/context.hpp>
#include <entt/meta/factory.hpp>
#include <entt/meta/meta.hpp>
#include "types.h"

position create_position(int x, int y) {
    return position{x, y};
}

void set_up() {
    using namespace entt::literals;

    entt::meta<position>()
        .type("position"_hs)
        .ctor<&create_position>()
        .data<&position::x>("x"_hs)
        .data<&position::y>("y"_hs);

    entt::meta<velocity>()
        .type("velocity"_hs)
        .ctor<>()
        .data<&velocity::dx>("dx"_hs)
        .data<&velocity::dy>("dy"_hs);
}

void tear_down() {
    entt::meta_reset<position>();
    entt::meta_reset<velocity>();
}

CR_EXPORT int cr_main(cr_plugin *ctx, cr_op operation) {
    switch(operation) {
    case CR_LOAD:
        entt::locator<entt::meta_ctx>::reset(static_cast<userdata *>(ctx->userdata)->ctx);
        set_up();
        break;
    case CR_STEP:
        static_cast<userdata *>(ctx->userdata)->any = 42;
        break;
    case CR_UNLOAD:
    case CR_CLOSE:
        tear_down();
        break;
    }

    return 0;
}

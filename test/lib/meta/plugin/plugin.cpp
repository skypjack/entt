#include <cr.h>
#include <entt/core/hashed_string.hpp>
#include <entt/locator/locator.hpp>
#include <entt/meta/context.hpp>
#include <entt/meta/factory.hpp>
#include <entt/meta/meta.hpp>
#include "../../../common/boxed_type.h"
#include "../../../common/empty.h"
#include "userdata.h"

test::boxed_int create_boxed_int(int value) {
    return test::boxed_int{value};
}

void set_up() {
    using namespace entt::literals;

    entt::meta_factory<test::boxed_int>{}
        .type("boxed_int"_hs)
        .ctor<&create_boxed_int>()
        .data<&test::boxed_int::value>("value"_hs);

    entt::meta_factory<test::empty>{}
        .type("empty"_hs)
        .ctor<>();
}

void tear_down() {
    entt::meta_reset<test::boxed_int>();
    entt::meta_reset<test::empty>();
}

CR_EXPORT int cr_main(cr_plugin *ctx, cr_op operation) {
    switch(operation) {
    case CR_LOAD:
        entt::locator<entt::meta_ctx>::reset(static_cast<userdata *>(ctx->userdata)->ctx);
        set_up();
        break;
    case CR_STEP:
        static_cast<userdata *>(ctx->userdata)->any = 4;
        break;
    case CR_UNLOAD:
    case CR_CLOSE:
        tear_down();
        break;
    }

    return 0;
}

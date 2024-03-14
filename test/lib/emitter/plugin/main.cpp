#define CR_HOST

#include <gtest/gtest.h>
#include "common/boxed_type.h"
#include "common/emitter.h"
#include <cr.h>

TEST(Lib, Emitter) {
    test::emitter emitter;
    int value{};

    ASSERT_EQ(value, 0);

    emitter.on<test::boxed_int>([&](test::boxed_int msg, test::emitter &owner) {
        value = msg.value;
        owner.erase<test::boxed_int>();
    });

    cr_plugin ctx;
    cr_plugin_load(ctx, PLUGIN);

    ctx.userdata = &emitter;
    cr_plugin_update(ctx);

    ASSERT_EQ(value, 4);

    emitter = {};
    cr_plugin_close(ctx);
}

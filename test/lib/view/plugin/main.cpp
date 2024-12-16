#define CR_HOST

#include <gtest/gtest.h>
#include <cr.h>
#include "../types.h"

TEST(Lib, View) {
    view_type view{};

    cr_plugin ctx;
    cr_plugin_load(ctx, PLUGIN);

    ctx.userdata = &view;
    cr_plugin_update(ctx);

    ASSERT_EQ(ctx.userdata, nullptr);

    cr_plugin_close(ctx);
}

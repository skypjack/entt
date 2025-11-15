#include <gtest/gtest.h>
#include <entt/config/config.h>
#include "../types.h"
#include "lib.h"

TEST(Lib, View) {
    view_type view{};
    const void *storage = filter(view);

    ASSERT_EQ(storage, nullptr);
}

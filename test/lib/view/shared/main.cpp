#include <gtest/gtest.h>
#include <entt/core/attribute.h>
#include "../types.h"

ENTT_API const void *filter(const view_type &);

TEST(Lib, View) {
    view_type view{};
    const void *storage = filter(view);

    ASSERT_EQ(storage, nullptr);
}

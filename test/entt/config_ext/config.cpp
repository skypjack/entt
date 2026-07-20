#include <gtest/gtest.h>
#include <entt/config/config.h>

TEST(Config, All) {
    ASSERT_TRUE(ENTT_EXT_CONFIG);
    ASSERT_EQ(ENTT_SPARSE_PAGE, 512);
    ASSERT_EQ(ENTT_PACKED_PAGE, 128);
}

#include <memory>
#include <gtest/gtest.h>
#include <entt/core/memory.hpp>

TEST(Memory, Unfancy) {
    std::shared_ptr<int> shared = std::make_shared<int>();
    auto *plain = std::addressof(*shared);

    ASSERT_EQ(entt::unfancy(shared), plain);
    ASSERT_EQ(entt::unfancy(plain), plain);
}

#include <memory>
#include <gtest/gtest.h>
#include <entt/core/memory.hpp>

TEST(Memory, ToAddress) {
    std::shared_ptr<int> shared = std::make_shared<int>();
    auto *plain = std::addressof(*shared);

    ASSERT_EQ(entt::to_address(shared), plain);
    ASSERT_EQ(entt::to_address(plain), plain);
}

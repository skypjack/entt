#include <memory>
#include <gtest/gtest.h>
#include <entt/stl/memory.hpp>

TEST(Memory, ToAddress) {
    const std::shared_ptr<int> shared = std::make_shared<int>();
    auto *plain = &*shared;

    ASSERT_EQ(entt::stl::to_address(shared), plain);
    ASSERT_EQ(entt::stl::to_address(plain), plain);
}

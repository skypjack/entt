#include <memory>
#include <gtest/gtest.h>
#include <entt/stl/memory.hpp>

TEST(ToAddress, Functionalities) {
    const std::shared_ptr<int> shared = std::make_shared<int>();
    auto *plain = &*shared;

    ASSERT_EQ(entt::stl::internal::to_address(shared), plain);
    ASSERT_EQ(entt::stl::internal::to_address(plain), plain);
}

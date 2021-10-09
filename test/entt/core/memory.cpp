#include <cmath>
#include <limits>
#include <memory>
#include <gtest/gtest.h>
#include <entt/core/memory.hpp>

struct test_allocator: std::allocator<int> {
    using base = std::allocator<int>;
    using propagate_on_container_copy_assignment = std::true_type;
    using propagate_on_container_swap = std::true_type;

    using std::allocator<int>::allocator;

    test_allocator &operator=(const test_allocator &other) {
        // necessary to avoid call suppression
        base::operator=(other);
        return *this;
    }
};

TEST(Memory, ToAddress) {
    std::shared_ptr<int> shared = std::make_shared<int>();
    auto *plain = std::addressof(*shared);

    ASSERT_EQ(entt::to_address(shared), plain);
    ASSERT_EQ(entt::to_address(plain), plain);
}

TEST(Memory, PoccaPocmaAndPocs) {
    test_allocator lhs, rhs;
    // honestly, I don't even know how one is supposed to test such a thing :)
    entt::propagate_on_container_copy_assignment(lhs, rhs);
    entt::propagate_on_container_move_assignment(lhs, rhs);
    entt::propagate_on_container_swap(lhs, rhs);
}

TEST(Memory, IsPowerOfTwo) {
    // constexpr-ness guaranteed
    constexpr auto zero_is_power_of_two = entt::is_power_of_two(0u);

    ASSERT_FALSE(zero_is_power_of_two);
    ASSERT_TRUE(entt::is_power_of_two(1u));
    ASSERT_TRUE(entt::is_power_of_two(2u));
    ASSERT_TRUE(entt::is_power_of_two(4u));
    ASSERT_FALSE(entt::is_power_of_two(7u));
    ASSERT_TRUE(entt::is_power_of_two(128u));
    ASSERT_FALSE(entt::is_power_of_two(200u));
}

TEST(Memory, NextPowerOfTwo) {
    // constexpr-ness guaranteed
    constexpr auto next_power_of_two_of_zero = entt::next_power_of_two(0u);

    ASSERT_EQ(next_power_of_two_of_zero, 1u);
    ASSERT_EQ(entt::next_power_of_two(1u), 1u);
    ASSERT_EQ(entt::next_power_of_two(2u), 2u);
    ASSERT_EQ(entt::next_power_of_two(3u), 4u);
    ASSERT_EQ(entt::next_power_of_two(17u), 32u);
    ASSERT_EQ(entt::next_power_of_two(32u), 32u);
    ASSERT_EQ(entt::next_power_of_two(33u), 64u);
    ASSERT_EQ(entt::next_power_of_two(std::pow(2, 16)), std::pow(2, 16));
    ASSERT_EQ(entt::next_power_of_two(std::pow(2, 16) + 1u), std::pow(2, 17));
    ASSERT_DEATH(static_cast<void>(entt::next_power_of_two((std::size_t{1u} << (std::numeric_limits<std::size_t>::digits - 1)) + 1)), "");
}

TEST(Memory, FastMod) {
    // constexpr-ness guaranteed
    constexpr auto fast_mod_of_zero = entt::fast_mod(0u, 8u);

    ASSERT_EQ(fast_mod_of_zero, 0u);
    ASSERT_EQ(entt::fast_mod(7u, 8u), 7u);
    ASSERT_EQ(entt::fast_mod(8u, 8u), 0u);
}

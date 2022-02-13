#include <cmath>
#include <cstddef>
#include <limits>
#include <memory>
#include <type_traits>
#include <utility>
#include <gtest/gtest.h>
#include <entt/core/memory.hpp>
#include "../common/basic_test_allocator.hpp"
#include "../common/throwing_allocator.hpp"
#include "../common/tracked_memory_resource.hpp"

TEST(Memory, ToAddress) {
    std::shared_ptr<int> shared = std::make_shared<int>();
    auto *plain = std::addressof(*shared);

    ASSERT_EQ(entt::to_address(shared), plain);
    ASSERT_EQ(entt::to_address(plain), plain);
}

TEST(Memory, PoccaPocmaAndPocs) {
    test::basic_test_allocator<int> lhs, rhs;
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
}

TEST(MemoryDeathTest, NextPowerOfTwo) {
    ASSERT_DEATH(static_cast<void>(entt::next_power_of_two((std::size_t{1u} << (std::numeric_limits<std::size_t>::digits - 1)) + 1)), "");
}

TEST(Memory, FastMod) {
    // constexpr-ness guaranteed
    constexpr auto fast_mod_of_zero = entt::fast_mod(0u, 8u);

    ASSERT_EQ(fast_mod_of_zero, 0u);
    ASSERT_EQ(entt::fast_mod(7u, 8u), 7u);
    ASSERT_EQ(entt::fast_mod(8u, 8u), 0u);
}

TEST(Memory, AllocateUnique) {
    test::throwing_allocator<double> allocator{};
    test::throwing_allocator<int>::trigger_on_allocate = true;

    ASSERT_THROW((entt::allocate_unique<int>(allocator, 0)), test::throwing_allocator<int>::exception_type);

    std::unique_ptr<int, entt::allocation_deleter<test::throwing_allocator<int>>> ptr = entt::allocate_unique<int>(allocator, 42);

    ASSERT_TRUE(ptr);
    ASSERT_EQ(*ptr, 42);

    ptr.reset();

    ASSERT_FALSE(ptr);
}

#if defined(ENTT_HAS_TRACKED_MEMORY_RESOURCE)

TEST(Memory, UsesAllocatorConstruction) {
    using string_type = typename test::tracked_memory_resource::string_type;

    test::tracked_memory_resource memory_resource{};
    std::pmr::polymorphic_allocator<string_type> allocator{&memory_resource};
    const char *str = "a string long enough to force an allocation (hopefully)";

    std::unique_ptr<string_type, entt::allocation_deleter<std::pmr::polymorphic_allocator<string_type>>> ptr = entt::allocate_unique<string_type>(allocator, str);

    ASSERT_GT(memory_resource.do_allocate_counter(), 1u);
    ASSERT_EQ(memory_resource.do_deallocate_counter(), 0u);
}

#endif

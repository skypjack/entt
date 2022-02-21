#include <cmath>
#include <cstddef>
#include <limits>
#include <memory>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>
#include <gtest/gtest.h>
#include <entt/core/memory.hpp>
#include "../common/basic_test_allocator.hpp"
#include "../common/throwing_allocator.hpp"
#include "../common/throwing_type.hpp"
#include "../common/tracked_memory_resource.hpp"

TEST(ToAddress, Functionalities) {
    std::shared_ptr<int> shared = std::make_shared<int>();
    auto *plain = std::addressof(*shared);

    ASSERT_EQ(entt::to_address(shared), plain);
    ASSERT_EQ(entt::to_address(plain), plain);
}

TEST(PoccaPocmaAndPocs, Functionalities) {
    test::basic_test_allocator<int> lhs, rhs;
    // honestly, I don't even know how one is supposed to test such a thing :)
    entt::propagate_on_container_copy_assignment(lhs, rhs);
    entt::propagate_on_container_move_assignment(lhs, rhs);
    entt::propagate_on_container_swap(lhs, rhs);
}

TEST(IsPowerOfTwo, Functionalities) {
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

TEST(NextPowerOfTwo, Functionalities) {
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

TEST(NextPowerOfTwoDeathTest, Functionalities) {
    ASSERT_DEATH(static_cast<void>(entt::next_power_of_two((std::size_t{1u} << (std::numeric_limits<std::size_t>::digits - 1)) + 1)), "");
}

TEST(FastMod, Functionalities) {
    // constexpr-ness guaranteed
    constexpr auto fast_mod_of_zero = entt::fast_mod(0u, 8u);

    ASSERT_EQ(fast_mod_of_zero, 0u);
    ASSERT_EQ(entt::fast_mod(7u, 8u), 7u);
    ASSERT_EQ(entt::fast_mod(8u, 8u), 0u);
}

TEST(AllocateUnique, Functionalities) {
    test::throwing_allocator<test::throwing_type> allocator{};
    test::throwing_allocator<test::throwing_type>::trigger_on_allocate = true;
    test::throwing_type::trigger_on_value = 0;

    ASSERT_THROW((entt::allocate_unique<test::throwing_type>(allocator, 0)), test::throwing_allocator<test::throwing_type>::exception_type);
    ASSERT_THROW((entt::allocate_unique<test::throwing_type>(allocator, test::throwing_type{0})), test::throwing_type::exception_type);

    std::unique_ptr<test::throwing_type, entt::allocation_deleter<test::throwing_allocator<test::throwing_type>>> ptr = entt::allocate_unique<test::throwing_type>(allocator, 42);

    ASSERT_TRUE(ptr);
    ASSERT_EQ(*ptr, 42);

    ptr.reset();

    ASSERT_FALSE(ptr);
}

#if defined(ENTT_HAS_TRACKED_MEMORY_RESOURCE)

TEST(AllocateUnique, NoUsesAllocatorConstruction) {
    test::tracked_memory_resource memory_resource{};
    std::pmr::polymorphic_allocator<int> allocator{&memory_resource};

    using type = std::unique_ptr<int, entt::allocation_deleter<std::pmr::polymorphic_allocator<int>>>;
    type ptr = entt::allocate_unique<int>(allocator, 0);

    ASSERT_TRUE(memory_resource.is_equal(memory_resource));
    ASSERT_EQ(memory_resource.do_allocate_counter(), 1u);
    ASSERT_EQ(memory_resource.do_deallocate_counter(), 0u);
}

TEST(AllocateUnique, UsesAllocatorConstruction) {
    using string_type = typename test::tracked_memory_resource::string_type;

    test::tracked_memory_resource memory_resource{};
    std::pmr::polymorphic_allocator<string_type> allocator{&memory_resource};

    using type = std::unique_ptr<string_type, entt::allocation_deleter<std::pmr::polymorphic_allocator<string_type>>>;
    type ptr = entt::allocate_unique<string_type>(allocator, test::tracked_memory_resource::default_value);

    ASSERT_TRUE(memory_resource.is_equal(memory_resource));
    ASSERT_GT(memory_resource.do_allocate_counter(), 1u);
    ASSERT_EQ(memory_resource.do_deallocate_counter(), 0u);
}

#endif

TEST(UsesAllocatorConstructionArgs, NoUsesAllocatorConstruction) {
    const auto args = entt::uses_allocator_construction_args<int>(std::allocator<int>{}, 42);

    static_assert(std::tuple_size_v<decltype(args)> == 1u);
    static_assert(std::is_same_v<decltype(args), const std::tuple<int &&>>);

    ASSERT_EQ(std::get<0>(args), 42);
}

TEST(UsesAllocatorConstructionArgs, LeadingAllocatorConvetion) {
    const auto args = entt::uses_allocator_construction_args<std::tuple<int, char>>(std::allocator<int>{}, 42, 'c');

    static_assert(std::tuple_size_v<decltype(args)> == 4u);
    static_assert(std::is_same_v<decltype(args), const std::tuple<std::allocator_arg_t, const std::allocator<int> &, int &&, char &&>>);

    ASSERT_EQ(std::get<2>(args), 42);
    ASSERT_EQ(std::get<3>(args), 'c');
}

TEST(UsesAllocatorConstructionArgs, TrailingAllocatorConvetion) {
    const auto args = entt::uses_allocator_construction_args<std::vector<int>>(std::allocator<int>{}, 42u);

    static_assert(std::tuple_size_v<decltype(args)> == 2u);
    static_assert(std::is_same_v<decltype(args), const std::tuple<unsigned int &&, const std::allocator<int> &>>);

    ASSERT_EQ(std::get<0>(args), 42);
}

TEST(MakeObjUsingAllocator, Functionalities) {
    test::throwing_allocator<int>::trigger_on_allocate = true;

    ASSERT_THROW((entt::make_obj_using_allocator<std::vector<int, test::throwing_allocator<int>>>(test::throwing_allocator<int>{}, 42u)), test::throwing_allocator<int>::exception_type);

    const auto vec = entt::make_obj_using_allocator<std::vector<int>>(std::allocator<int>{}, 42u);

    ASSERT_FALSE(vec.empty());
    ASSERT_EQ(vec.size(), 42u);
}

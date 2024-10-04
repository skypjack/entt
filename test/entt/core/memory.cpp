#include <array>
#include <cstddef>
#include <memory>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>
#include <gtest/gtest.h>
#include <entt/core/memory.hpp>
#include "../../common/basic_test_allocator.hpp"
#include "../../common/config.h"
#include "../../common/throwing_allocator.hpp"
#include "../../common/throwing_type.hpp"
#include "../../common/tracked_memory_resource.hpp"

TEST(ToAddress, Functionalities) {
    const std::shared_ptr<int> shared = std::make_shared<int>();
    auto *plain = &*shared;

    ASSERT_EQ(entt::to_address(shared), plain);
    ASSERT_EQ(entt::to_address(plain), plain);
}

TEST(PoccaPocmaAndPocs, Functionalities) {
    test::basic_test_allocator<int> lhs;
    test::basic_test_allocator<int> rhs;
    test::basic_test_allocator<int, std::false_type> no_pocs;

    // code coverage purposes
    ASSERT_FALSE(lhs == rhs);
    ASSERT_NO_THROW(entt::propagate_on_container_swap(no_pocs, no_pocs));

    // honestly, I don't even know how one is supposed to test such a thing :)
    entt::propagate_on_container_copy_assignment(lhs, rhs);
    entt::propagate_on_container_move_assignment(lhs, rhs);
    entt::propagate_on_container_swap(lhs, rhs);
}

ENTT_DEBUG_TEST(PoccaPocmaAndPocsDeathTest, Functionalities) {
    test::basic_test_allocator<int, std::false_type> lhs;
    test::basic_test_allocator<int, std::false_type> rhs;

    ASSERT_DEATH(entt::propagate_on_container_swap(lhs, rhs), "");
}

TEST(AllocateUnique, Functionalities) {
    test::throwing_allocator<test::throwing_type> allocator{};

    allocator.throw_counter<test::throwing_type>(0u);

    ASSERT_THROW((entt::allocate_unique<test::throwing_type>(allocator, false)), test::throwing_allocator_exception);
    ASSERT_THROW((entt::allocate_unique<test::throwing_type>(allocator, test::throwing_type{true})), test::throwing_type_exception);

    std::unique_ptr<test::throwing_type, entt::allocation_deleter<test::throwing_allocator<test::throwing_type>>> ptr = entt::allocate_unique<test::throwing_type>(allocator, false);

    ASSERT_TRUE(ptr);
    ASSERT_EQ(*ptr, false);

    ptr.reset();

    ASSERT_FALSE(ptr);
}

#if defined(ENTT_HAS_TRACKED_MEMORY_RESOURCE)

TEST(AllocateUnique, NoUsesAllocatorConstruction) {
    test::tracked_memory_resource memory_resource{};
    std::pmr::polymorphic_allocator<int> allocator{&memory_resource};

    using type = std::unique_ptr<int, entt::allocation_deleter<std::pmr::polymorphic_allocator<int>>>;
    [[maybe_unused]] const type ptr = entt::allocate_unique<int>(allocator, 0);

    ASSERT_EQ(memory_resource.do_allocate_counter(), 1u);
    ASSERT_EQ(memory_resource.do_deallocate_counter(), 0u);
}

TEST(AllocateUnique, UsesAllocatorConstruction) {
    using string_type = typename test::tracked_memory_resource::string_type;

    test::tracked_memory_resource memory_resource{};
    std::pmr::polymorphic_allocator<string_type> allocator{&memory_resource};

    using type = std::unique_ptr<string_type, entt::allocation_deleter<std::pmr::polymorphic_allocator<string_type>>>;
    [[maybe_unused]] const type ptr = entt::allocate_unique<string_type>(allocator, test::tracked_memory_resource::default_value);

    ASSERT_GT(memory_resource.do_allocate_counter(), 1u);
    ASSERT_EQ(memory_resource.do_deallocate_counter(), 0u);
}

#endif

TEST(UsesAllocatorConstructionArgs, NoUsesAllocatorConstruction) {
    const auto value = 4;
    const auto args = entt::uses_allocator_construction_args<int>(std::allocator<int>{}, value);

    ASSERT_EQ(std::tuple_size_v<decltype(args)>, 1u);
    testing::StaticAssertTypeEq<decltype(args), const std::tuple<const int &>>();
    ASSERT_EQ(std::get<0>(args), value);
}

TEST(UsesAllocatorConstructionArgs, LeadingAllocatorConvention) {
    const auto value = 4;
    const auto args = entt::uses_allocator_construction_args<std::tuple<int, char>>(std::allocator<int>{}, value, 'c');

    ASSERT_EQ(std::tuple_size_v<decltype(args)>, 4u);
    testing::StaticAssertTypeEq<decltype(args), const std::tuple<std::allocator_arg_t, const std::allocator<int> &, const int &, char &&>>();
    ASSERT_EQ(std::get<2>(args), value);
}

TEST(UsesAllocatorConstructionArgs, TrailingAllocatorConvention) {
    const auto size = 4u;
    const auto args = entt::uses_allocator_construction_args<std::vector<int>>(std::allocator<int>{}, size);

    ASSERT_EQ(std::tuple_size_v<decltype(args)>, 2u);
    testing::StaticAssertTypeEq<decltype(args), const std::tuple<const unsigned int &, const std::allocator<int> &>>();
    ASSERT_EQ(std::get<0>(args), size);
}

TEST(UsesAllocatorConstructionArgs, PairPiecewiseConstruct) {
    const auto size = 4u;
    const auto tup = std::make_tuple(size);
    const auto args = entt::uses_allocator_construction_args<std::pair<int, std::vector<int>>>(std::allocator<int>{}, std::piecewise_construct, std::make_tuple(3), tup);

    ASSERT_EQ(std::tuple_size_v<decltype(args)>, 3u);
    testing::StaticAssertTypeEq<decltype(args), const std::tuple<std::piecewise_construct_t, std::tuple<int &&>, std::tuple<const unsigned int &, const std::allocator<int> &>>>();
    ASSERT_EQ(std::get<0>(std::get<2>(args)), size);
}

TEST(UsesAllocatorConstructionArgs, PairNoArgs) {
    [[maybe_unused]] const auto args = entt::uses_allocator_construction_args<std::pair<int, std::vector<int>>>(std::allocator<int>{});

    ASSERT_EQ(std::tuple_size_v<decltype(args)>, 3u);
    testing::StaticAssertTypeEq<decltype(args), const std::tuple<std::piecewise_construct_t, std::tuple<>, std::tuple<const std::allocator<int> &>>>();
}

TEST(UsesAllocatorConstructionArgs, PairValues) {
    const auto size = 4u;
    const auto args = entt::uses_allocator_construction_args<std::pair<int, std::vector<int>>>(std::allocator<int>{}, 3, size);

    ASSERT_EQ(std::tuple_size_v<decltype(args)>, 3u);
    testing::StaticAssertTypeEq<decltype(args), const std::tuple<std::piecewise_construct_t, std::tuple<int &&>, std::tuple<const unsigned int &, const std::allocator<int> &>>>();
    ASSERT_EQ(std::get<0>(std::get<2>(args)), size);
}

TEST(UsesAllocatorConstructionArgs, PairConstLValueReference) {
    const auto value = std::make_pair(3, 4u);
    const auto args = entt::uses_allocator_construction_args<std::pair<int, std::vector<int>>>(std::allocator<int>{}, value);

    ASSERT_EQ(std::tuple_size_v<decltype(args)>, 3u);
    testing::StaticAssertTypeEq<decltype(args), const std::tuple<std::piecewise_construct_t, std::tuple<const int &>, std::tuple<const unsigned int &, const std::allocator<int> &>>>();
    ASSERT_EQ(std::get<0>(std::get<1>(args)), 3);
    ASSERT_EQ(std::get<0>(std::get<2>(args)), 4u);
}

TEST(UsesAllocatorConstructionArgs, PairRValueReference) {
    [[maybe_unused]] const auto args = entt::uses_allocator_construction_args<std::pair<int, std::vector<int>>>(std::allocator<int>{}, std::make_pair(3, 4u));

    ASSERT_EQ(std::tuple_size_v<decltype(args)>, 3u);
    testing::StaticAssertTypeEq<decltype(args), const std::tuple<std::piecewise_construct_t, std::tuple<int &&>, std::tuple<unsigned int &&, const std::allocator<int> &>>>();
}

TEST(MakeObjUsingAllocator, Functionalities) {
    const auto size = 4u;
    test::throwing_allocator<int> allocator{};

    allocator.throw_counter<int>(0u);

    ASSERT_THROW((entt::make_obj_using_allocator<std::vector<int, test::throwing_allocator<int>>>(allocator, size)), test::throwing_allocator_exception);

    const auto vec = entt::make_obj_using_allocator<std::vector<int>>(std::allocator<int>{}, size);

    ASSERT_FALSE(vec.empty());
    ASSERT_EQ(vec.size(), size);
}

TEST(UninitializedConstructUsingAllocator, NoUsesAllocatorConstruction) {
    alignas(int) std::array<std::byte, sizeof(int)> storage{};
    const std::allocator<int> allocator{};

    // NOLINTNEXTLINE(*-reinterpret-cast)
    int *value = entt::uninitialized_construct_using_allocator(reinterpret_cast<int *>(storage.data()), allocator, 1);

    ASSERT_EQ(*value, 1);
}

#if defined(ENTT_HAS_TRACKED_MEMORY_RESOURCE)
#    include <memory_resource>

TEST(UninitializedConstructUsingAllocator, UsesAllocatorConstruction) {
    using string_type = typename test::tracked_memory_resource::string_type;

    test::tracked_memory_resource memory_resource{};
    const std::pmr::polymorphic_allocator<string_type> allocator{&memory_resource};
    alignas(string_type) std::array<std::byte, sizeof(string_type)> storage{};

    // NOLINTNEXTLINE(*-reinterpret-cast)
    string_type *value = entt::uninitialized_construct_using_allocator(reinterpret_cast<string_type *>(storage.data()), allocator, test::tracked_memory_resource::default_value);

    ASSERT_GT(memory_resource.do_allocate_counter(), 0u);
    ASSERT_EQ(memory_resource.do_deallocate_counter(), 0u);
    ASSERT_EQ(*value, test::tracked_memory_resource::default_value);

    value->~string_type();
}

#endif

#include <cstddef>
#include <memory>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>
#include <gtest/gtest.h>
#include <entt/core/compressed_pair.hpp>
#include "../../common/empty.h"
#include "../../common/non_default_constructible.h"

TEST(CompressedPair, Size) {
    struct local {
        int value;
        test::empty empty;
    };

    ASSERT_EQ(sizeof(entt::compressed_pair<int, int>), sizeof(int) * 2u);
    ASSERT_EQ(sizeof(entt::compressed_pair<test::empty, int>), sizeof(int));
    ASSERT_EQ(sizeof(entt::compressed_pair<int, test::empty>), sizeof(int));
    ASSERT_LT(sizeof(entt::compressed_pair<int, test::empty>), sizeof(local));
    ASSERT_LT(sizeof(entt::compressed_pair<int, test::empty>), sizeof(std::pair<int, test::empty>));
}

TEST(CompressedPair, ConstructCopyMove) {
    static_assert(!std::is_default_constructible_v<entt::compressed_pair<test::non_default_constructible, test::empty>>, "Default constructible type not allowed");
    static_assert(std::is_default_constructible_v<entt::compressed_pair<std::unique_ptr<int>, test::empty>>, "Default constructible type required");

    static_assert(std::is_copy_constructible_v<entt::compressed_pair<test::non_default_constructible, test::empty>>, "Copy constructible type required");
    static_assert(!std::is_copy_constructible_v<entt::compressed_pair<std::unique_ptr<int>, test::empty>>, "Copy constructible type not allowed");
    static_assert(std::is_copy_assignable_v<entt::compressed_pair<test::non_default_constructible, test::empty>>, "Copy assignable type required");
    static_assert(!std::is_copy_assignable_v<entt::compressed_pair<std::unique_ptr<int>, test::empty>>, "Copy assignable type not allowed");

    static_assert(std::is_move_constructible_v<entt::compressed_pair<std::unique_ptr<int>, test::empty>>, "Move constructible type required");
    static_assert(std::is_move_assignable_v<entt::compressed_pair<std::unique_ptr<int>, test::empty>>, "Move assignable type required");

    entt::compressed_pair copyable{test::non_default_constructible{2}, test::empty{}};
    auto by_copy{copyable};

    ASSERT_EQ(by_copy.first().value, 2);

    by_copy.first().value = 3;
    copyable = by_copy;

    ASSERT_EQ(copyable.first().value, 3);

    entt::compressed_pair<test::empty, std::unique_ptr<int>> movable{test::empty{}, std::make_unique<int>(1)};
    auto by_move{std::move(movable)};

    ASSERT_TRUE(by_move.second());
    ASSERT_EQ(*by_move.second(), 1);

    *by_move.second() = 3;
    movable = std::move(by_move);

    ASSERT_TRUE(movable.second());
    ASSERT_EQ(*movable.second(), 3);
}

TEST(CompressedPair, PiecewiseConstruct) {
    const entt::compressed_pair<test::empty, test::empty> empty{std::piecewise_construct, std::make_tuple(), std::make_tuple()};
    const entt::compressed_pair<std::vector<int>, std::size_t> pair{std::piecewise_construct, std::forward_as_tuple(std::vector<int>{2}), std::make_tuple(sizeof(empty))};

    ASSERT_EQ(pair.first().size(), 1u);
    ASSERT_EQ(pair.second(), sizeof(empty));
}

TEST(CompressedPair, DeductionGuide) {
    const int value = 2;
    const test::empty empty{};
    entt::compressed_pair pair{value, 3};

    testing::StaticAssertTypeEq<decltype(entt::compressed_pair{test::empty{}, empty}), entt::compressed_pair<test::empty, test::empty>>();
    testing::StaticAssertTypeEq<decltype(pair), entt::compressed_pair<int, int>>();

    ASSERT_EQ(pair.first(), 2);
    ASSERT_EQ(pair.second(), 3);
}

TEST(CompressedPair, Getters) {
    entt::compressed_pair pair{3, test::empty{}};
    const auto &cpair = pair;

    testing::StaticAssertTypeEq<decltype(pair.first()), int &>();
    testing::StaticAssertTypeEq<decltype(pair.second()), test::empty &>();

    testing::StaticAssertTypeEq<decltype(cpair.first()), const int &>();
    testing::StaticAssertTypeEq<decltype(cpair.second()), const test::empty &>();

    ASSERT_EQ(pair.first(), cpair.first());
    ASSERT_EQ(&pair.second(), &cpair.second());
}

TEST(CompressedPair, Swap) {
    entt::compressed_pair pair{1, 2};
    entt::compressed_pair other{3, 4};

    swap(pair, other);

    ASSERT_EQ(pair.first(), 3);
    ASSERT_EQ(pair.second(), 4);
    ASSERT_EQ(other.first(), 1);
    ASSERT_EQ(other.second(), 2);

    pair.swap(other);

    ASSERT_EQ(pair.first(), 1);
    ASSERT_EQ(pair.second(), 2);
    ASSERT_EQ(other.first(), 3);
    ASSERT_EQ(other.second(), 4);
}

TEST(CompressedPair, Get) {
    entt::compressed_pair pair{1, 2};

    ASSERT_EQ(pair.get<0>(), 1);
    ASSERT_EQ(pair.get<1>(), 2);

    ASSERT_EQ(&pair.get<0>(), &pair.first());
    ASSERT_EQ(&pair.get<1>(), &pair.second());

    auto &&[first, second] = pair;

    ASSERT_EQ(first, 1);
    ASSERT_EQ(second, 2);

    first = 3;
    second = 4;

    ASSERT_EQ(pair.first(), 3);
    ASSERT_EQ(pair.second(), 4);

    // NOLINTNEXTLINE(readability-qualified-auto)
    auto &[cfirst, csecond] = std::as_const(pair);

    ASSERT_EQ(cfirst, 3);
    ASSERT_EQ(csecond, 4);

    testing::StaticAssertTypeEq<decltype(cfirst), const int>();
    testing::StaticAssertTypeEq<decltype(csecond), const int>();

    auto [tfirst, tsecond] = entt::compressed_pair{32, 64};

    ASSERT_EQ(tfirst, 32);
    ASSERT_EQ(tsecond, 64);

    testing::StaticAssertTypeEq<decltype(cfirst), const int>();
    testing::StaticAssertTypeEq<decltype(csecond), const int>();
}

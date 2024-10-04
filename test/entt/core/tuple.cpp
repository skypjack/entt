#include <tuple>
#include <utility>
#include <gtest/gtest.h>
#include <entt/core/tuple.hpp>

TEST(Tuple, IsTuple) {
    ASSERT_FALSE(entt::is_tuple_v<int>);
    ASSERT_TRUE(entt::is_tuple_v<std::tuple<>>);
    ASSERT_TRUE(entt::is_tuple_v<std::tuple<int>>);
    ASSERT_TRUE((entt::is_tuple_v<std::tuple<int, char>>));
}

TEST(Tuple, UnwrapTuple) {
    auto single = std::make_tuple(2);
    auto multi = std::make_tuple(2, 'c');
    auto ref = std::forward_as_tuple(std::get<0>(single));

    testing::StaticAssertTypeEq<decltype(entt::unwrap_tuple(single)), int &>();
    testing::StaticAssertTypeEq<decltype(entt::unwrap_tuple(multi)), std::tuple<int, char> &>();
    testing::StaticAssertTypeEq<decltype(entt::unwrap_tuple(ref)), int &>();

    testing::StaticAssertTypeEq<decltype(entt::unwrap_tuple(std::move(single))), int &&>();
    testing::StaticAssertTypeEq<decltype(entt::unwrap_tuple(std::move(multi))), std::tuple<int, char> &&>();
    testing::StaticAssertTypeEq<decltype(entt::unwrap_tuple(std::move(ref))), int &>();

    testing::StaticAssertTypeEq<decltype(entt::unwrap_tuple(std::as_const(single))), const int &>();
    testing::StaticAssertTypeEq<decltype(entt::unwrap_tuple(std::as_const(multi))), const std::tuple<int, char> &>();
    testing::StaticAssertTypeEq<decltype(entt::unwrap_tuple(std::as_const(ref))), int &>();

    ASSERT_EQ(entt::unwrap_tuple(single), 2);
    ASSERT_EQ(entt::unwrap_tuple(multi), multi);
    ASSERT_EQ(entt::unwrap_tuple(std::move(ref)), 2);
}

TEST(Tuple, ForwardApply) {
    entt::forward_apply first{[](auto &&...args) { return sizeof...(args); }};
    entt::forward_apply second{[](int value) { return value; }};
    entt::forward_apply third{[](auto... args) { return (args + ...); }};

    ASSERT_EQ(first(std::make_tuple()), 0u);
    ASSERT_EQ(std::as_const(first)(std::make_tuple()), 0u);

    ASSERT_EQ(second(std::make_tuple(2)), 2);
    ASSERT_EQ(std::as_const(second)(std::make_tuple(2)), 2);

    ASSERT_EQ(third(std::make_tuple('a', 1)), 'b');
    ASSERT_EQ(std::as_const(third)(std::make_tuple('a', 1)), 'b');
}

#include <tuple>
#include <gtest/gtest.h>
#include <entt/core/tuple.hpp>

TEST(Tuple, UnwrapTuple) {
    auto single = std::make_tuple(42);
    auto multi = std::make_tuple(42, 'c');
    auto ref = std::forward_as_tuple(std::get<0>(single));

    ASSERT_TRUE((std::is_same_v<decltype(entt::unwrap_tuple(single)), int &>));
    ASSERT_TRUE((std::is_same_v<decltype(entt::unwrap_tuple(multi)), std::tuple<int, char> &>));
    ASSERT_TRUE((std::is_same_v<decltype(entt::unwrap_tuple(ref)), int &>));

    ASSERT_TRUE((std::is_same_v<decltype(entt::unwrap_tuple(std::move(single))), int &&>));
    ASSERT_TRUE((std::is_same_v<decltype(entt::unwrap_tuple(std::move(multi))), std::tuple<int, char> &&>));
    ASSERT_TRUE((std::is_same_v<decltype(entt::unwrap_tuple(std::move(ref))), int &>));

    ASSERT_TRUE((std::is_same_v<decltype(entt::unwrap_tuple(std::as_const(single))), const int &>));
    ASSERT_TRUE((std::is_same_v<decltype(entt::unwrap_tuple(std::as_const(multi))), const std::tuple<int, char> &>));
    ASSERT_TRUE((std::is_same_v<decltype(entt::unwrap_tuple(std::as_const(ref))), int &>));

    ASSERT_EQ(entt::unwrap_tuple(single), 42);
    ASSERT_EQ(entt::unwrap_tuple(multi), multi);
    ASSERT_EQ(entt::unwrap_tuple(std::move(ref)), 42);
}

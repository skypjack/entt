#include <tuple>
#include <gtest/gtest.h>
#include <entt/core/tuple.hpp>

TEST(Tuple, IsTuple) {
    static_assert(!entt::is_tuple_v<int>);
    static_assert(entt::is_tuple_v<std::tuple<>>);
    static_assert(entt::is_tuple_v<std::tuple<int>>);
    static_assert(entt::is_tuple_v<std::tuple<int, char>>);
}

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

TEST(Tuple, ForwardApply) {
    ASSERT_EQ(entt::forward_apply{[](auto &&...args) { return sizeof...(args); }}(std::make_tuple()), 0u);
    ASSERT_EQ(entt::forward_apply{[](int i) { return i; }}(std::make_tuple(42)), 42);
    ASSERT_EQ(entt::forward_apply{[](auto... args) { return (args + ...); }}(std::make_tuple('a', 1)), 'b');
}

TEST(TypeList, TupleSize) {
    ASSERT_EQ(std::tuple_size_v<entt::type_list<>>, 0u);
    ASSERT_EQ(std::tuple_size_v<entt::type_list<int>>, 1u);
    ASSERT_EQ((std::tuple_size_v<entt::type_list<int, float>>), 2u);
}

TEST(TypeList, TupleElement) {
    ASSERT_TRUE((std::is_same_v<int, std::tuple_element_t<0, entt::type_list<int>>>));
    ASSERT_TRUE((std::is_same_v<int, std::tuple_element_t<0, entt::type_list<int, float>>>));
    ASSERT_TRUE((std::is_same_v<float, std::tuple_element_t<1, entt::type_list<int, float>>>));
}

TEST(ValueList, TupleSize) {
    ASSERT_EQ(std::tuple_size_v<entt::value_list<>>, 0u);
    ASSERT_EQ(std::tuple_size_v<entt::value_list<42>>, 1u);
    ASSERT_EQ((std::tuple_size_v<entt::value_list<42, 'a'>>), 2u);
}

TEST(ValueList, TupleElement) {
    ASSERT_TRUE((std::is_same_v<int, std::tuple_element_t<0, entt::value_list<42>>>));
    ASSERT_TRUE((std::is_same_v<int, std::tuple_element_t<0, entt::value_list<42, 'a'>>>));
    ASSERT_TRUE((std::is_same_v<char, std::tuple_element_t<1, entt::value_list<42, 'a'>>>));
}

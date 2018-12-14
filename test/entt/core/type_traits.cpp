#include <type_traits>
#include <gtest/gtest.h>
#include <entt/core/type_traits.hpp>

TEST(TypeTraits, TypeList) {
    static_assert(std::is_same_v<
        decltype(entt::type_list_cat()),
        entt::type_list<>
    >);

    static_assert(std::is_same_v<
        decltype(entt::type_list_cat(entt::type_list<int>{})),
        entt::type_list<int>
    >);

    static_assert(std::is_same_v<
        decltype(entt::type_list_cat(entt::type_list<int, char>{}, entt::type_list<>{}, entt::type_list<double, void>{})),
        entt::type_list<int, char, double, void>
    >);

    SUCCEED();
}

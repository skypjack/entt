#include <type_traits>
#include <gtest/gtest.h>
#include <entt/core/ident.hpp>

struct a_type {};
struct another_type {};

TEST(Ident, Uniqueness) {
    using id = entt::ident<a_type, another_type>;
    constexpr a_type an_instance;
    constexpr another_type another_instance;

    ASSERT_NE(id::value<a_type>, id::value<another_type>);
    ASSERT_EQ(id::value<a_type>, id::value<decltype(an_instance)>);
    ASSERT_NE(id::value<a_type>, id::value<decltype(another_instance)>);
    ASSERT_EQ(id::value<a_type>, id::value<a_type>);
    ASSERT_EQ(id::value<another_type>, id::value<another_type>);

    // test uses in constant expressions
    switch(id::value<another_type>) {
    case id::value<a_type>:
        FAIL();
    case id::value<another_type>:
        SUCCEED();
    }
}

TEST(Identifier, SingleType) {
    using id = entt::ident<a_type>;
    [[maybe_unused]] std::integral_constant<id::value_type, id::value<a_type>> ic;
}

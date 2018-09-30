#include <type_traits>
#include <gtest/gtest.h>
#include <entt/core/ident.hpp>

struct a_type {};
struct another_type {};

TEST(Identifier, Uniqueness) {
    using id = entt::identifier<a_type, another_type>;
    constexpr a_type an_instance;
    constexpr another_type another_instance;

    ASSERT_NE(id::type<a_type>, id::type<another_type>);
    ASSERT_EQ(id::type<a_type>, id::type<decltype(an_instance)>);
    ASSERT_NE(id::type<a_type>, id::type<decltype(another_instance)>);
    ASSERT_EQ(id::type<a_type>, id::type<a_type>);
    ASSERT_EQ(id::type<another_type>, id::type<another_type>);

    // test uses in constant expressions
    switch(id::type<another_type>) {
    case id::type<a_type>:
        FAIL();
    case id::type<another_type>:
        SUCCEED();
    }
}

TEST(Identifier, SingleType) {
    using id = entt::identifier<a_type>;
    std::integral_constant<id::identifier_type, id::type<a_type>> ic;
    (void)ic;
}

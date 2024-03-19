#include <type_traits>
#include <gtest/gtest.h>
#include <entt/core/ident.hpp>
#include "../../common/boxed_type.h"
#include "../../common/empty.h"

TEST(Ident, Uniqueness) {
    using id = entt::ident<test::empty, test::boxed_int>;
    constexpr test::empty instance;
    constexpr test::boxed_int other;

    ASSERT_NE(id::value<test::empty>, id::value<test::boxed_int>);
    ASSERT_EQ(id::value<test::empty>, id::value<decltype(instance)>);
    ASSERT_NE(id::value<test::empty>, id::value<decltype(other)>);
    ASSERT_EQ(id::value<test::empty>, id::value<test::empty>);
    ASSERT_EQ(id::value<test::boxed_int>, id::value<test::boxed_int>);

    // test uses in constant expressions
    switch(id::value<test::boxed_int>) {
    case id::value<test::boxed_int>:
        SUCCEED();
        break;
    case id::value<test::empty>:
    default:
        FAIL();
        break;
    }
}

TEST(Identifier, SingleType) {
    using id = entt::ident<test::empty>;
    [[maybe_unused]] const std::integral_constant<id::value_type, id::value<test::empty>> ic;
}

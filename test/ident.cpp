#include <gtest/gtest.h>
#include <ident.hpp>

struct A {};
struct B {};

TEST(Identifier, Uniqueness) {
    constexpr auto ID = entt::ident<A, B>;
    constexpr A a;
    constexpr B b;

    ASSERT_NE(ID.get<A>(), ID.get<B>());
    ASSERT_EQ(ID.get<A>(), ID.get<decltype(a)>());
    ASSERT_NE(ID.get<A>(), ID.get<decltype(b)>());
    ASSERT_EQ(ID.get<A>(), ID.get<A>());
    ASSERT_EQ(ID.get<B>(), ID.get<B>());

    // test uses in constant expressions
    switch(ID.get<B>()) {
    case ID.get<A>():
        FAIL();
        break;
    case ID.get<B>():
        SUCCEED();
    }
}

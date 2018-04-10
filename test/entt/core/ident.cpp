#include <type_traits>
#include <gtest/gtest.h>
#include <entt/core/ident.hpp>

struct AType {};
struct AnotherType {};

TEST(Identifier, Uniqueness) {
    constexpr auto ID = entt::ident<AType, AnotherType>;
    constexpr AType anInstance;
    constexpr AnotherType anotherInstance;

    ASSERT_NE(ID.get<AType>(), ID.get<AnotherType>());
    ASSERT_EQ(ID.get<AType>(), ID.get<decltype(anInstance)>());
    ASSERT_NE(ID.get<AType>(), ID.get<decltype(anotherInstance)>());
    ASSERT_EQ(ID.get<AType>(), ID.get<AType>());
    ASSERT_EQ(ID.get<AnotherType>(), ID.get<AnotherType>());

    // test uses in constant expressions
    switch(ID.get<AnotherType>()) {
    case ID.get<AType>():
        FAIL();
        break;
    case ID.get<AnotherType>():
        SUCCEED();
    }
}

TEST(Identifier, SingleType) {
    constexpr auto ID = entt::ident<AType>;
    std::integral_constant<decltype(ID)::identifier_type, ID.get()> ic;
    (void)ic;
}

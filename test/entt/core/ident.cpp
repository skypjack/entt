#include <type_traits>
#include <gtest/gtest.h>
#include <entt/core/ident.hpp>

struct AType {};
struct AnotherType {};

TEST(Identifier, Uniqueness) {
    using ID = entt::Identifier<AType, AnotherType>;
    constexpr AType anInstance;
    constexpr AnotherType anotherInstance;

    ASSERT_NE(ID::get<AType>(), ID::get<AnotherType>());
    ASSERT_EQ(ID::get<AType>(), ID::get<decltype(anInstance)>());
    ASSERT_NE(ID::get<AType>(), ID::get<decltype(anotherInstance)>());
    ASSERT_EQ(ID::get<AType>(), ID::get<AType>());
    ASSERT_EQ(ID::get<AnotherType>(), ID::get<AnotherType>());

    // test uses in constant expressions
    switch(ID::get<AnotherType>()) {
    case ID::get<AType>():
        FAIL();
    case ID::get<AnotherType>():
        SUCCEED();
    }
}

TEST(Identifier, SingleType) {
    using ID = entt::Identifier<AType>;
    std::integral_constant<ID::identifier_type, ID::get<AType>()> ic;
    (void)ic;
}

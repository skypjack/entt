#include <gtest/gtest.h>
#include <entt/core/enum.hpp>
#include "../../common/bitmask.h"

template<typename Type>
struct Enum: testing::Test {
    using type = Type;
};

using EnumTypes = ::testing::Types<test::enum_is_bitmask, test::enum_as_bitmask>;

TYPED_TEST_SUITE(Enum, EnumTypes, );

TYPED_TEST(Enum, Functionalities) {
    using enum_type = typename TestFixture::type;

    ASSERT_TRUE(!!((enum_type::foo | enum_type::bar) & enum_type::foo));
    ASSERT_TRUE(!!((enum_type::foo | enum_type::bar) & enum_type::bar));
    ASSERT_TRUE(!((enum_type::foo | enum_type::bar) & enum_type::quux));

    ASSERT_TRUE(!!((enum_type::foo ^ enum_type::bar) & enum_type::foo));
    ASSERT_TRUE(!((enum_type::foo ^ enum_type::foo) & enum_type::foo));

    ASSERT_TRUE(!(~enum_type::foo & enum_type::foo));
    ASSERT_TRUE(!!(~enum_type::foo & enum_type::bar));

    ASSERT_TRUE(enum_type::foo == enum_type::foo);
    ASSERT_TRUE(enum_type::foo != enum_type::bar);

    enum_type value = enum_type::foo;

    ASSERT_TRUE(!!(value & enum_type::foo));
    ASSERT_TRUE(!(value & enum_type::bar));
    ASSERT_TRUE(!(value & enum_type::quux));

    value |= (enum_type::bar | enum_type::quux);

    ASSERT_TRUE(!!(value & enum_type::foo));
    ASSERT_TRUE(!!(value & enum_type::bar));
    ASSERT_TRUE(!!(value & enum_type::quux));

    value &= (enum_type::bar | enum_type::quux);

    ASSERT_TRUE(!(value & enum_type::foo));
    ASSERT_TRUE(!!(value & enum_type::bar));
    ASSERT_TRUE(!!(value & enum_type::quux));

    value ^= enum_type::bar;

    ASSERT_TRUE(!(value & enum_type::foo));
    ASSERT_TRUE(!(value & enum_type::bar));
    ASSERT_TRUE(!!(value & enum_type::quux));
}

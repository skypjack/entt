#include <cstdint>
#include <type_traits>
#include <gtest/gtest.h>
#include <entt/core/enum.hpp>

enum class detected {
    foo = 0x01,
    bar = 0x02,
    quux = 0x04,
    _entt_enum_as_bitmask
};

// small type on purpose
enum class registered : std::uint8_t {
    foo = 0x01,
    bar = 0x02,
    quux = 0x04
};

template<>
struct entt::enum_as_bitmask<registered>
    : std::true_type {};

template<typename Type>
struct Enum: testing::Test {
    using type = Type;
};

using EnumTypes = ::testing::Types<detected, registered>;

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

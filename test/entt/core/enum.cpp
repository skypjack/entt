#include <gtest/gtest.h>
#include <entt/core/enum.hpp>
#include <entt/core/type_traits.hpp>

enum class detected {
    foo = 0x01,
    bar = 0x02,
    quux = 0x04,
    _entt_enum_as_bitmask
};

enum class registered {
    foo = 0x01,
    bar = 0x02,
    quux = 0x04
};

template<>
struct entt::enum_as_bitmask<registered>
    : std::true_type {};

TEST(Enum, Functionalities) {
    auto test = [](auto identity) {
        using enum_type = typename decltype(identity)::type;

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
    };

    test(entt::type_identity<detected>{});
    test(entt::type_identity<registered>{});
}

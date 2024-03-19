#include <gtest/gtest.h>
#include <entt/config/config.h>
#include <entt/entity/component.hpp>
#include "../../common/boxed_type.h"
#include "../../common/empty.h"
#include "../../common/non_movable.h"

struct self_contained {
    static constexpr auto in_place_delete = true;
    static constexpr auto page_size = 4u;
};

struct traits_based {};

template<>
struct entt::component_traits<traits_based> {
    using type = traits_based;
    static constexpr auto in_place_delete = false;
    static constexpr auto page_size = 8u;
};

TEST(Component, VoidType) {
    using traits_type = entt::component_traits<void>;

    ASSERT_FALSE(traits_type::in_place_delete);
    ASSERT_EQ(traits_type::page_size, 0u);
}

TEST(Component, Empty) {
    using traits_type = entt::component_traits<test::empty>;

    ASSERT_FALSE(traits_type::in_place_delete);
    ASSERT_EQ(traits_type::page_size, 0u);
}

TEST(Component, NonEmpty) {
    using traits_type = entt::component_traits<test::boxed_int>;

    ASSERT_FALSE(traits_type::in_place_delete);
    ASSERT_EQ(traits_type::page_size, ENTT_PACKED_PAGE);
}

TEST(Component, NonMovable) {
    using traits_type = entt::component_traits<test::non_movable>;

    ASSERT_TRUE(traits_type::in_place_delete);
    ASSERT_EQ(traits_type::page_size, ENTT_PACKED_PAGE);
}

TEST(Component, SelfContained) {
    using traits_type = entt::component_traits<self_contained>;

    ASSERT_TRUE(traits_type::in_place_delete);
    ASSERT_EQ(traits_type::page_size, 4u);
}

TEST(Component, TraitsBased) {
    using traits_type = entt::component_traits<traits_based>;

    ASSERT_TRUE(!traits_type::in_place_delete);
    ASSERT_EQ(traits_type::page_size, 8u);
}

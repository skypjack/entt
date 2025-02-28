#include <type_traits>
#include <gtest/gtest.h>
#include <entt/config/config.h>
#include <entt/entity/component.hpp>
#include "../../common/boxed_type.h"
#include "../../common/empty.h"
#include "../../common/entity.h"
#include "../../common/non_movable.h"

struct self_contained {
    static constexpr auto in_place_delete = true;
    static constexpr auto page_size = 4u;
};

struct traits_based {};

template<>
struct entt::component_traits<traits_based /*, entt::entity */> {
    using entity_type = entt::entity;
    using element_type = traits_based;

    static constexpr auto in_place_delete = true;
    static constexpr auto page_size = 8u;
};

template<>
struct entt::component_traits<traits_based, test::entity> {
    using entity_type = test::entity;
    using element_type = traits_based;

    static constexpr auto in_place_delete = false;
    static constexpr auto page_size = 16u;
};

template<typename Entity>
struct entt::component_traits<traits_based, Entity> {
    using entity_type = Entity;
    using element_type = traits_based;

    static constexpr auto in_place_delete = true;
    static constexpr auto page_size = 32u;
};

template<typename Type>
struct Component: testing::Test {
    using entity_type = Type;
};

using EntityTypes = ::testing::Types<entt::entity, test::entity, test::other_entity>;

TYPED_TEST_SUITE(Component, EntityTypes, );

TYPED_TEST(Component, VoidType) {
    using entity_type = typename TestFixture::entity_type;
    using traits_type = entt::component_traits<void, entity_type>;

    ASSERT_FALSE(traits_type::in_place_delete);
    ASSERT_EQ(traits_type::page_size, 0u);
}

TYPED_TEST(Component, Empty) {
    using entity_type = typename TestFixture::entity_type;
    using traits_type = entt::component_traits<test::empty, entity_type>;

    ASSERT_FALSE(traits_type::in_place_delete);
    ASSERT_EQ(traits_type::page_size, 0u);
}

TYPED_TEST(Component, NonEmpty) {
    using entity_type = typename TestFixture::entity_type;
    using traits_type = entt::component_traits<test::boxed_int, entity_type>;

    ASSERT_FALSE(traits_type::in_place_delete);
    ASSERT_EQ(traits_type::page_size, ENTT_PACKED_PAGE);
}

TYPED_TEST(Component, NonMovable) {
    using entity_type = typename TestFixture::entity_type;
    using traits_type = entt::component_traits<test::non_movable, entity_type>;

    ASSERT_TRUE(traits_type::in_place_delete);
    ASSERT_EQ(traits_type::page_size, ENTT_PACKED_PAGE);
}

TYPED_TEST(Component, SelfContained) {
    using entity_type = typename TestFixture::entity_type;
    using traits_type = entt::component_traits<self_contained, entity_type>;

    ASSERT_TRUE(traits_type::in_place_delete);
    ASSERT_EQ(traits_type::page_size, 4u);
}

TYPED_TEST(Component, TraitsBased) {
    using entity_type = typename TestFixture::entity_type;
    using traits_type = entt::component_traits<traits_based, entity_type>;

    if constexpr(std::is_same_v<entity_type, entt::entity>) {
        ASSERT_TRUE(traits_type::in_place_delete);
        ASSERT_EQ(traits_type::page_size, 8u);
    } else if constexpr(std::is_same_v<entity_type, test::entity>) {
        ASSERT_FALSE(traits_type::in_place_delete);
        ASSERT_EQ(traits_type::page_size, 16u);
    } else {
        ASSERT_TRUE(traits_type::in_place_delete);
        ASSERT_EQ(traits_type::page_size, 32u);
    }
}

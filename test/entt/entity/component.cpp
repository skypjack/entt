#include <type_traits>
#include <gtest/gtest.h>
#include <entt/config/config.h>
#include <entt/entity/component.hpp>
#include "../../common/boxed_type.h"
#include "../../common/empty.h"
#include "../../common/non_movable.h"

struct ComponentBase: testing::Test {
    enum class my_entity : std::uint32_t {};
    enum class other_entity : std::uint32_t {};

    struct self_contained {
        static constexpr auto in_place_delete = true;
        static constexpr auto page_size = 4u;
    };

    struct traits_based {};
};

template<>
struct entt::component_traits<ComponentBase::traits_based, ComponentBase::my_entity> {
    using entity_type = ComponentBase::my_entity;
    using element_type = ComponentBase::traits_based;

    static constexpr auto in_place_delete = true;
    static constexpr auto page_size = 8u;
};

template<>
struct entt::component_traits<ComponentBase::traits_based, ComponentBase::other_entity> {
    using entity_type = ComponentBase::other_entity;
    using element_type = ComponentBase::traits_based;

    static constexpr auto in_place_delete = false;
    static constexpr auto page_size = 16u;
};

template<entt::entity_like Entity>
struct entt::component_traits<ComponentBase::traits_based, Entity> {
    using entity_type = Entity;
    using element_type = ComponentBase::traits_based;

    static constexpr auto in_place_delete = true;
    static constexpr auto page_size = 32u;
};

template<typename Type>
struct Component: ComponentBase {
    using entity_type = Type;
};

using EntityTypes = ::testing::Types<entt::entity, ComponentBase::my_entity, ComponentBase::other_entity>;

TYPED_TEST_SUITE(Component, EntityTypes, );

TYPED_TEST(Component, VoidType) {
    using traits_type = entt::component_traits<void, typename TestFixture::entity_type>;

    ASSERT_FALSE(traits_type::in_place_delete);
    ASSERT_EQ(traits_type::page_size, 0u);
}

TYPED_TEST(Component, Empty) {
    using traits_type = entt::component_traits<test::empty, typename TestFixture::entity_type>;

    ASSERT_FALSE(traits_type::in_place_delete);
    ASSERT_EQ(traits_type::page_size, 0u);
}

TYPED_TEST(Component, NonEmpty) {
    using traits_type = entt::component_traits<test::boxed_int, typename TestFixture::entity_type>;

    ASSERT_FALSE(traits_type::in_place_delete);
    ASSERT_EQ(traits_type::page_size, ENTT_PACKED_PAGE);
}

TYPED_TEST(Component, NonMovable) {
    using traits_type = entt::component_traits<test::non_movable, typename TestFixture::entity_type>;

    ASSERT_TRUE(traits_type::in_place_delete);
    ASSERT_EQ(traits_type::page_size, ENTT_PACKED_PAGE);
}

TYPED_TEST(Component, SelfContained) {
    using traits_type = entt::component_traits<ComponentBase::self_contained, typename TestFixture::entity_type>;

    ASSERT_TRUE(traits_type::in_place_delete);
    ASSERT_EQ(traits_type::page_size, 4u);
}

TYPED_TEST(Component, TraitsBased) {
    using traits_type = entt::component_traits<ComponentBase::traits_based, typename TestFixture::entity_type>;

    if constexpr(std::is_same_v<typename traits_type::entity_type, ComponentBase::my_entity>) {
        ASSERT_TRUE(traits_type::in_place_delete);
        ASSERT_EQ(traits_type::page_size, 8u);
    } else if constexpr(std::is_same_v<typename traits_type::entity_type, ComponentBase::other_entity>) {
        ASSERT_FALSE(traits_type::in_place_delete);
        ASSERT_EQ(traits_type::page_size, 16u);
    } else {
        ASSERT_TRUE(traits_type::in_place_delete);
        ASSERT_EQ(traits_type::page_size, 32u);
    }
}

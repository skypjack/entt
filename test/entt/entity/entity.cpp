#include <cstddef>
#include <cstdint>
#include <gtest/gtest.h>
#include <entt/config/config.h>
#include <entt/entity/entity.hpp>
#include "../../common/entity.h"

struct entity_traits {
    using value_type = test::entity;
    using entity_type = std::uint32_t;
    using version_type = std::uint16_t;
    static constexpr entity_type entity_mask = 0x3FFFF; // 18b
    static constexpr entity_type version_mask = 0x0FFF; // 12b
};

struct other_entity_traits {
    using value_type = test::other_entity;
    using entity_type = std::uint32_t;
    using version_type = std::uint16_t;
    static constexpr entity_type entity_mask = 0xFFFFFFFF; // 32b
    static constexpr entity_type version_mask = 0x00;      // 0b
};

template<>
struct entt::entt_traits<test::entity>: entt::basic_entt_traits<entity_traits> {
    static constexpr std::size_t page_size = ENTT_SPARSE_PAGE;
};

template<>
struct entt::entt_traits<test::other_entity>: entt::basic_entt_traits<other_entity_traits> {
    static constexpr std::size_t page_size = ENTT_SPARSE_PAGE;
};

template<typename Type>
struct Entity: testing::Test {
    using type = Type;
};

using EntityTypes = ::testing::Types<entt::entity, test::entity, test::other_entity>;

TYPED_TEST_SUITE(Entity, EntityTypes, );

TYPED_TEST(Entity, Traits) {
    using entity_type = typename TestFixture::type;
    using traits_type = entt::entt_traits<entity_type>;

    constexpr entity_type tombstone{entt::tombstone};
    constexpr entity_type null{entt::null};

    const entity_type entity = traits_type::construct(4u, 1u);
    const entity_type other = traits_type::construct(3u, 0u);

    ASSERT_EQ(entt::to_integral(entity), entt::to_integral(entity));
    ASSERT_NE(entt::to_integral(entity), entt::to_integral<entity_type>(entt::null));
    ASSERT_NE(entt::to_integral(entity), entt::to_integral(entity_type{}));

    ASSERT_EQ(entt::to_entity(entity), 4u);
    ASSERT_EQ(entt::to_version(entity), !!traits_type::version_mask);

    ASSERT_EQ(entt::to_entity(other), 3u);
    ASSERT_EQ(entt::to_version(other), 0u);

    ASSERT_EQ(traits_type::construct(entt::to_entity(entity), entt::to_version(entity)), entity);
    ASSERT_EQ(traits_type::construct(entt::to_entity(other), entt::to_version(other)), other);

    if constexpr(traits_type::version_mask == 0u) {
        ASSERT_EQ(traits_type::construct(entt::to_entity(entity), entt::to_version(other)), entity);
    } else {
        ASSERT_NE(traits_type::construct(entt::to_entity(entity), entt::to_version(other)), entity);
    }

    ASSERT_EQ(traits_type::construct(entt::to_entity(other), entt::to_version(entity)), traits_type::combine(entt::to_integral(other), entt::to_integral(entity)));

    ASSERT_EQ(traits_type::combine(entt::tombstone, entt::null), tombstone);
    ASSERT_EQ(traits_type::combine(entt::null, entt::tombstone), null);

    ASSERT_EQ(traits_type::next(entity), traits_type::construct(entt::to_integral(entity), entt::to_version(entity) + 1u));
    ASSERT_EQ(traits_type::next(other), traits_type::construct(entt::to_integral(other), entt::to_version(other) + 1u));

    ASSERT_EQ(traits_type::next(entt::tombstone), traits_type::construct(entt::null, {}));
    ASSERT_EQ(traits_type::next(entt::null), traits_type::construct(entt::null, {}));

    if constexpr(traits_type::to_integral(tombstone) != ~typename traits_type::entity_type{}) {
        // test reserved bits, if any
        constexpr entity_type reserved{traits_type::to_integral(entity) | (traits_type::to_integral(tombstone) + 1u)};

        ASSERT_NE(reserved, entity);

        ASSERT_NE(traits_type::to_integral(null), ~typename traits_type::entity_type{});
        ASSERT_NE(traits_type::to_integral(tombstone), ~typename traits_type::entity_type{});

        ASSERT_EQ(traits_type::to_entity(reserved), traits_type::to_entity(entity));
        ASSERT_EQ(traits_type::to_version(reserved), traits_type::to_version(entity));

        ASSERT_EQ(traits_type::to_version(null), traits_type::version_mask);
        ASSERT_EQ(traits_type::to_version(tombstone), traits_type::version_mask);

        ASSERT_EQ(traits_type::to_version(traits_type::next(null)), 0u);
        ASSERT_EQ(traits_type::to_version(traits_type::next(tombstone)), 0u);

        ASSERT_EQ(traits_type::construct(traits_type::to_integral(entity), traits_type::version_mask + 1u), entity_type{traits_type::to_entity(entity)});

        ASSERT_EQ(traits_type::construct(traits_type::to_integral(null), traits_type::to_version(null) + 1u), entity_type{traits_type::to_entity(null)});
        ASSERT_EQ(traits_type::construct(traits_type::to_integral(tombstone), traits_type::to_version(tombstone) + 1u), entity_type{traits_type::to_entity(tombstone)});

        ASSERT_EQ(traits_type::next(reserved), traits_type::next(entity));

        ASSERT_EQ(traits_type::next(null), traits_type::combine(null, entity_type{}));
        ASSERT_EQ(traits_type::next(tombstone), traits_type::combine(tombstone, entity_type{}));

        ASSERT_EQ(traits_type::combine(entity, reserved), entity);
        ASSERT_NE(traits_type::combine(entity, reserved), reserved);

        ASSERT_EQ(traits_type::combine(reserved, entity), entity);
        ASSERT_NE(traits_type::combine(reserved, entity), reserved);
    }
}

TYPED_TEST(Entity, Null) {
    using entity_type = typename TestFixture::type;
    using traits_type = entt::entt_traits<entity_type>;

    constexpr entity_type null{entt::null};

    ASSERT_FALSE(entity_type{} == entt::null);
    ASSERT_TRUE(entt::null == entt::null);
    ASSERT_FALSE(entt::null != entt::null);

    const entity_type entity{4u};

    ASSERT_EQ(traits_type::combine(entt::null, entt::to_integral(entity)), (traits_type::construct(entt::to_entity(null), entt::to_version(entity))));
    ASSERT_EQ(traits_type::combine(entt::null, entt::to_integral(null)), null);
    ASSERT_EQ(traits_type::combine(entt::null, entt::tombstone), null);

    ASSERT_FALSE(entity == entt::null);
    ASSERT_FALSE(entt::null == entity);

    ASSERT_TRUE(entity != entt::null);
    ASSERT_TRUE(entt::null != entity);
}

TYPED_TEST(Entity, Tombstone) {
    using entity_type = typename TestFixture::type;
    using traits_type = entt::entt_traits<entity_type>;

    constexpr entity_type tombstone{entt::tombstone};

    ASSERT_FALSE(entity_type{} == entt::tombstone);
    ASSERT_TRUE(entt::tombstone == entt::tombstone);
    ASSERT_FALSE(entt::tombstone != entt::tombstone);

    const entity_type entity{4u};

    ASSERT_EQ(traits_type::combine(entt::to_integral(entity), entt::tombstone), (traits_type::construct(entt::to_entity(entity), entt::to_version(tombstone))));
    ASSERT_EQ(traits_type::combine(entt::tombstone, entt::to_integral(tombstone)), tombstone);
    ASSERT_EQ(traits_type::combine(entt::tombstone, entt::null), tombstone);

    ASSERT_FALSE(entity == entt::tombstone);
    ASSERT_FALSE(entt::tombstone == entity);

    ASSERT_TRUE(entity != entt::tombstone);
    ASSERT_TRUE(entt::tombstone != entity);
}

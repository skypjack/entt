#include <cstddef>
#include <cstdint>
#include <iterator>
#include <gtest/gtest.h>
#include <entt/config/config.h>
#include <entt/core/enum.hpp>
#include <entt/core/fwd.hpp>
#include <entt/entity/entity.hpp>
#include <entt/entity/mixin.hpp>
#include <entt/entity/registry.hpp>
#include <entt/entity/view.hpp>

enum class my_entity : entt::id_type {
    disabled = 0x10000000,
    _entt_enum_as_bitmask
};

struct entity_traits {
    using value_type = my_entity;
    using entity_type = std::uint32_t;
    using version_type = std::uint16_t;
    static constexpr entity_type entity_mask = 0xFFFF;  // 16b
    static constexpr entity_type version_mask = 0x0FFF; // 12b
};

template<>
struct entt::entt_traits<my_entity>: entt::basic_entt_traits<entity_traits> {
    static constexpr std::size_t page_size = ENTT_SPARSE_PAGE;
};

namespace {

[[nodiscard]] bool is_disabled(const my_entity entity) {
    return ((entity & my_entity::disabled) == my_entity::disabled);
}

} // namespace

TEST(Example, DisabledEntity) {
    entt::basic_registry<my_entity> registry{};
    auto view = registry.view<my_entity, int>();

    const my_entity entity = registry.create(entt::entt_traits<my_entity>::construct(4u, 1u));
    const my_entity other = registry.create(entt::entt_traits<my_entity>::construct(3u, 0u));

    registry.emplace<int>(entity);
    registry.emplace<int>(other);

    ASSERT_FALSE(is_disabled(*registry.storage<my_entity>().find(entity)));
    ASSERT_FALSE(is_disabled(*registry.storage<my_entity>().find(other)));

    ASSERT_FALSE(is_disabled(*registry.storage<int>().find(entity)));
    ASSERT_FALSE(is_disabled(*registry.storage<int>().find(other)));

    registry.storage<my_entity>().bump(entity | my_entity::disabled);

    ASSERT_TRUE(is_disabled(*registry.storage<my_entity>().find(entity)));
    ASSERT_FALSE(is_disabled(*registry.storage<my_entity>().find(other)));

    ASSERT_FALSE(is_disabled(*registry.storage<int>().find(entity)));
    ASSERT_FALSE(is_disabled(*registry.storage<int>().find(other)));

    view.use<my_entity>();

    ASSERT_EQ(std::distance(view.begin(), view.end()), 2);

    for(auto entt: view) {
        if(entt::to_entity(entt) == entt::to_entity(entity)) {
            ASSERT_NE(entt, entity);
            ASSERT_EQ(entt::to_version(entt), entt::to_version(entity));
            ASSERT_TRUE(is_disabled(entt));
        } else {
            ASSERT_EQ(entt, other);
            ASSERT_EQ(entt::to_version(entt), entt::to_version(other));
            ASSERT_FALSE(is_disabled(entt));
        }
    }

    view.use<int>();

    ASSERT_EQ(std::distance(view.begin(), view.end()), 2);

    for(auto entt: view) {
        ASSERT_FALSE(is_disabled(entt));
    }

    registry.storage<my_entity>().bump(entity);
    registry.storage<int>().bump(other | my_entity::disabled);

    ASSERT_FALSE(is_disabled(*registry.storage<my_entity>().find(entity)));
    ASSERT_FALSE(is_disabled(*registry.storage<my_entity>().find(other)));

    ASSERT_FALSE(is_disabled(*registry.storage<int>().find(entity)));
    ASSERT_TRUE(is_disabled(*registry.storage<int>().find(other)));

    view.use<my_entity>();

    ASSERT_EQ(std::distance(view.begin(), view.end()), 2);

    for(auto entt: view) {
        ASSERT_FALSE(is_disabled(entt));
    }

    view.use<int>();

    ASSERT_EQ(std::distance(view.begin(), view.end()), 2);

    for(auto entt: view) {
        if(entt::to_entity(entt) == entt::to_entity(other)) {
            ASSERT_NE(entt, other);
            ASSERT_EQ(entt::to_version(entt), entt::to_version(other));
            ASSERT_TRUE(is_disabled(entt));
        } else {
            ASSERT_EQ(entt, entity);
            ASSERT_EQ(entt::to_version(entt), entt::to_version(entity));
            ASSERT_FALSE(is_disabled(entt));
        }
    }

    registry.storage<int>().bump(other);

    ASSERT_FALSE(is_disabled(*registry.storage<my_entity>().find(entity)));
    ASSERT_FALSE(is_disabled(*registry.storage<my_entity>().find(other)));

    ASSERT_FALSE(is_disabled(*registry.storage<int>().find(entity)));
    ASSERT_FALSE(is_disabled(*registry.storage<int>().find(other)));

    view.use<my_entity>();

    ASSERT_EQ(std::distance(view.begin(), view.end()), 2);

    for(auto entt: view) {
        ASSERT_FALSE(is_disabled(entt));
    }

    view.use<int>();

    ASSERT_EQ(std::distance(view.begin(), view.end()), 2);

    for(auto entt: view) {
        ASSERT_FALSE(is_disabled(entt));
    }
}

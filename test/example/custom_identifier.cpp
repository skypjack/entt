#include <type_traits>
#include <gtest/gtest.h>
#include <entt/entity/entity.hpp>
#include <entt/entity/registry.hpp>

struct entity_id {
    using entity_type = typename entt::entt_traits<entt::entity>::entity_type;

    entity_id(entity_type value = entt::null)
        : entt{value}
    {}

    entity_id(const entity_id &other)
        : entt{other.entt}
    {}

    operator entity_type() const {
        return entt;
    }

private:
    entity_type entt;
};

template<>
struct entt::entt_traits<entity_id>: entt::entt_traits<entt::entity> {};

TEST(Example, CustomIdentifier) {
    entt::basic_registry<entity_id> registry{};
    entity_id entity{};

    ASSERT_FALSE(registry.valid(entity));
    ASSERT_TRUE(entity == entt::null);

    entity = registry.create();

    ASSERT_TRUE(registry.valid(entity));
    ASSERT_TRUE(entity != entt::null);

    ASSERT_FALSE((registry.all_of<int, char>(entity)));
    ASSERT_EQ(registry.try_get<int>(entity), nullptr);

    registry.emplace<int>(entity, 42);

    ASSERT_TRUE((registry.any_of<int, char>(entity)));
    ASSERT_EQ(registry.get<int>(entity), 42);

    registry.destroy(entity);

    ASSERT_FALSE(registry.valid(entity));
    ASSERT_TRUE(entity != entt::null);

    entity = registry.create();

    ASSERT_TRUE(registry.valid(entity));
    ASSERT_TRUE(entity != entt::null);
}

#include <cstdint>
#include <gtest/gtest.h>
#include <entt/entity/entity.hpp>
#include <entt/entity/registry.hpp>

struct entity_id final {
    using entity_type = std::uint32_t;
    static constexpr auto null = entt::null;

    constexpr entity_id(entity_type value = null) noexcept
        : entt{value} {}

    ~entity_id() = default;

    constexpr entity_id(const entity_id &other) = default;
    constexpr entity_id(entity_id &&other) noexcept = default;

    constexpr entity_id &operator=(const entity_id &other) = default;
    constexpr entity_id &operator=(entity_id &&other) noexcept = default;

    constexpr operator entity_type() const noexcept {
        return entt;
    }

private:
    entity_type entt;
};

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

    registry.emplace<int>(entity, 2);

    ASSERT_TRUE((registry.any_of<int, char>(entity)));
    ASSERT_EQ(registry.get<int>(entity), 2);

    registry.destroy(entity);

    ASSERT_FALSE(registry.valid(entity));
    ASSERT_TRUE(entity != entt::null);

    entity = registry.create();

    ASSERT_TRUE(registry.valid(entity));
    ASSERT_TRUE(entity != entt::null);
}

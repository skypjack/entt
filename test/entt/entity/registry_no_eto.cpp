#include <gtest/gtest.h>
#include <entt/entity/registry.hpp>

struct empty_type {};

bool operator==(const empty_type &lhs, const empty_type &rhs) {
    return &lhs == &rhs;
}

TEST(Registry, NoEto) {
    entt::registry registry;
    const auto entity = registry.create();

    registry.emplace<empty_type>(entity);
    registry.emplace<int>(entity, 42);

    ASSERT_NE(registry.storage<empty_type>().raw(), nullptr);
    ASSERT_NE(registry.try_get<empty_type>(entity), nullptr);
    ASSERT_EQ(registry.view<empty_type>().get(entity), std::as_const(registry).view<const empty_type>().get(entity));

    auto view = registry.view<empty_type, int>();
    auto cview = std::as_const(registry).view<const empty_type, const int>();

    ASSERT_EQ((std::get<0>(view.get<empty_type, int>(entity))), (std::get<0>(cview.get<const empty_type, const int>(entity))));
}

#include <gtest/gtest.h>
#include <entt/core/hashed_string.hpp>
#include <entt/entity/helper.hpp>
#include <entt/entity/registry.hpp>

TEST(Helper, AsView) {
    using entity_type = typename entt::registry<>::entity_type;

    entt::registry<> registry;
    const entt::registry<> cregistry;

    ([](entt::view<entity_type, int, char>) {})(entt::as_view{registry});
    ([](entt::view<entity_type, const int, char>) {})(entt::as_view{registry});
    ([](entt::view<entity_type, const double>) {})(entt::as_view{cregistry});
}

TEST(Helper, AsGroup) {
    using entity_type = typename entt::registry<>::entity_type;

    entt::registry<> registry;
    const entt::registry<> cregistry;

    ([](entt::group<entity_type, entt::get_t<>, double, float>) {})(entt::as_group{registry});
    ([](entt::group<entity_type, entt::get_t<>, const double, float>) {})(entt::as_group{registry});
    ([](entt::group<entity_type, entt::get_t<>, const double, const float>) {})(entt::as_group{cregistry});
}

TEST(Helper, Dependency) {
    entt::registry<> registry;
    const auto entity = registry.create();
    entt::connect<double, float>(registry.construction<int>());

    ASSERT_FALSE(registry.has<double>(entity));
    ASSERT_FALSE(registry.has<float>(entity));

    registry.assign<char>(entity);

    ASSERT_FALSE(registry.has<double>(entity));
    ASSERT_FALSE(registry.has<float>(entity));

    registry.assign<int>(entity);

    ASSERT_TRUE(registry.has<double>(entity));
    ASSERT_TRUE(registry.has<float>(entity));
    ASSERT_EQ(registry.get<double>(entity), .0);
    ASSERT_EQ(registry.get<float>(entity), .0f);

    registry.get<double>(entity) = .3;
    registry.get<float>(entity) = .1f;
    registry.remove<int>(entity);
    registry.assign<int>(entity);

    ASSERT_EQ(registry.get<double>(entity), .3);
    ASSERT_EQ(registry.get<float>(entity), .1f);

    registry.remove<int>(entity);
    registry.remove<float>(entity);
    registry.assign<int>(entity);

    ASSERT_TRUE(registry.has<float>(entity));
    ASSERT_EQ(registry.get<double>(entity), .3);
    ASSERT_EQ(registry.get<float>(entity), .0f);

    registry.remove<int>(entity);
    registry.remove<double>(entity);
    registry.remove<float>(entity);
    entt::disconnect<double, float>(registry.construction<int>());
    registry.assign<int>(entity);

    ASSERT_FALSE(registry.has<double>(entity));
    ASSERT_FALSE(registry.has<float>(entity));
}

TEST(Helper, Label) {
    entt::registry<> registry;
    const auto entity = registry.create();
    registry.assign<entt::label<"foobar"_hs>>(entity);
    registry.assign<int>(entity, 42);
    int counter{};

    ASSERT_FALSE(registry.has<entt::label<"barfoo"_hs>>(entity));
    ASSERT_TRUE(registry.has<entt::label<"foobar"_hs>>(entity));

    for(auto entity: registry.view<int, entt::label<"foobar"_hs>>()) {
        (void)entity;
        ++counter;
    }

    ASSERT_NE(counter, 0);

    for(auto entity: registry.view<entt::label<"foobar"_hs>>()) {
        (void)entity;
        --counter;
    }

    ASSERT_EQ(counter, 0);
}

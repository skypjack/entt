#include <type_traits>
#include <utility>
#include <gtest/gtest.h>
#include <entt/entity/handle.hpp>
#include <entt/entity/registry.hpp>

TEST(BasicHandle, Assumptions) {
    static_assert(std::is_trivially_copyable_v<entt::handle>);
    static_assert(std::is_trivially_assignable_v<entt::handle, entt::handle>);
    static_assert(std::is_trivially_destructible_v<entt::handle>);

    static_assert(std::is_trivially_copyable_v<entt::const_handle>);
    static_assert(std::is_trivially_assignable_v<entt::const_handle, entt::const_handle>);
    static_assert(std::is_trivially_destructible_v<entt::const_handle>);
}

TEST(BasicHandle, DeductionGuide) {
    static_assert(std::is_same_v<decltype(entt::basic_handle{std::declval<entt::registry &>(), {}}), entt::basic_handle<entt::entity>>);
    static_assert(std::is_same_v<decltype(entt::basic_handle{std::declval<const entt::registry &>(), {}}), entt::basic_handle<const entt::entity>>);
}

TEST(BasicHandle, Construction) {
    entt::registry registry;
    const auto entity = registry.create();

    entt::handle handle{registry, entity};
    entt::const_handle chandle{std::as_const(registry), entity};

    ASSERT_FALSE(entt::null == handle.entity());
    ASSERT_EQ(entity, handle);
    ASSERT_TRUE(handle);

    ASSERT_FALSE(entt::null == chandle.entity());
    ASSERT_EQ(entity, chandle);
    ASSERT_TRUE(chandle);

    ASSERT_EQ(handle, chandle);

    static_assert(std::is_same_v<entt::registry &, decltype(handle.registry())>);
    static_assert(std::is_same_v<const entt::registry &, decltype(chandle.registry())>);

    handle = entt::null;

    ASSERT_TRUE(entt::null == handle.entity());
    ASSERT_NE(entity, handle);
    ASSERT_FALSE(handle);

    ASSERT_NE(handle, chandle);
}

TEST(BasicHandle, Component) {
    entt::registry registry;
    const auto entity = registry.create();
    entt::handle handle{registry, entity};

    ASSERT_EQ(3, handle.emplace<int>(3));
    ASSERT_EQ('c', handle.emplace_or_replace<char>('c'));

    const auto &patched = handle.patch<int>([](auto &comp) { comp = 42; });

    ASSERT_EQ(42, patched);
    ASSERT_EQ('a', handle.replace<char>('a'));
    ASSERT_TRUE((handle.has<int, char>()));

    handle.remove<char>();

    ASSERT_TRUE(registry.empty<char>());
    ASSERT_EQ(0u, handle.remove_if_exists<char>());

    handle.visit([](auto id) { ASSERT_EQ(entt::type_info<int>::id(), id); });

    ASSERT_TRUE((handle.any<int, char>()));
    ASSERT_FALSE((handle.has<int, char>()));
    ASSERT_FALSE(handle.orphan());

    handle.remove_all();

    ASSERT_TRUE(registry.empty<int>());
    ASSERT_TRUE(handle.orphan());

    ASSERT_EQ(42, handle.get_or_emplace<int>(42));
    ASSERT_EQ(42, handle.get_or_emplace<int>(1));
    ASSERT_EQ(42, handle.get<int>());

    ASSERT_EQ(42, *handle.try_get<int>());
    ASSERT_EQ(nullptr, handle.try_get<char>());
}

TEST(BasicHandle, FromEntity) {
    entt::registry registry;
    const auto entity = registry.create();

    registry.emplace<int>(entity, 42);
    registry.emplace<char>(entity, 'c');

    entt::handle handle{registry, entity};

    ASSERT_TRUE(handle);
    ASSERT_EQ(entity, handle.entity());
    ASSERT_TRUE((handle.has<int, char>()));
    ASSERT_EQ(handle.get<int>(), 42);
    ASSERT_EQ(handle.get<char>(), 'c');
}

TEST(BasicHandle, Lifetime) {
    entt::registry registry;
    const auto entity = registry.create();
    auto *handle = new entt::handle{registry, entity};
    handle->emplace<int>();

    ASSERT_FALSE(registry.empty<int>());
    ASSERT_FALSE(registry.empty());

    registry.each([handle](const auto e) {
        ASSERT_EQ(handle->entity(), e);
    });

    delete handle;

    ASSERT_FALSE(registry.empty<int>());
    ASSERT_FALSE(registry.empty());
}

TEST(BasicHandle, ImplicitConversions) {
    entt::registry registry;
    const entt::handle handle{registry, registry.create()};
    const entt::const_handle chandle = handle;

    handle.emplace<int>(42);

    ASSERT_EQ(handle.get<int>(), chandle.get<int>());
    ASSERT_EQ(chandle.get<int>(), 42);
}

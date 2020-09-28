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

    static_assert(std::is_same_v<entt::registry *, decltype(handle.registry())>);
    static_assert(std::is_same_v<const entt::registry *, decltype(chandle.registry())>);
}


TEST(BasicHandle, Invalidation) {
    entt::handle handle;

    ASSERT_TRUE(nullptr == handle.registry());
    ASSERT_TRUE(entt::null == handle.entity());
    ASSERT_FALSE(handle);

    entt::registry registry;
    const auto entity = registry.create();

    handle = {registry, entity};

    ASSERT_FALSE(nullptr == handle.registry());
    ASSERT_FALSE(entt::null == handle.entity());
    ASSERT_TRUE(handle);

    handle = {};

    ASSERT_TRUE(nullptr == handle.registry());
    ASSERT_TRUE(entt::null == handle.entity());
    ASSERT_FALSE(handle);
}


TEST(BasicHandle, Comparison) {
    entt::registry registry;
    const auto entity1 = registry.create();
    const auto entity2 = registry.create();

    entt::handle handle1{registry, entity1};
    entt::handle handle2{registry, entity2};
    entt::const_handle chandle1 = handle1;
    entt::const_handle chandle2 = handle2;

    ASSERT_NE(handle1, handle2);
    ASSERT_FALSE(handle1 == handle2);
    ASSERT_TRUE(handle1 != handle2);

    ASSERT_NE(chandle1, chandle2);
    ASSERT_FALSE(chandle1 == chandle2);
    ASSERT_TRUE(chandle1 != chandle2);

    ASSERT_EQ(handle1, chandle1);
    ASSERT_TRUE(handle1 == chandle1);
    ASSERT_FALSE(handle1 != chandle1);

    ASSERT_EQ(handle2, chandle2);
    ASSERT_TRUE(handle2 == chandle2);
    ASSERT_FALSE(handle2 != chandle2);

    ASSERT_NE(handle1, chandle2);
    ASSERT_FALSE(handle1 == chandle2);
    ASSERT_TRUE(handle1 != chandle2);

    handle1 = {};
    chandle2 = {};

    ASSERT_NE(handle1, handle2);
    ASSERT_FALSE(handle1 == handle2);
    ASSERT_TRUE(handle1 != handle2);

    ASSERT_NE(chandle1, chandle2);
    ASSERT_FALSE(chandle1 == chandle2);
    ASSERT_TRUE(chandle1 != chandle2);

    ASSERT_NE(handle1, chandle1);
    ASSERT_FALSE(handle1 == chandle1);
    ASSERT_TRUE(handle1 != chandle1);

    ASSERT_NE(handle2, chandle2);
    ASSERT_FALSE(handle2 == chandle2);
    ASSERT_TRUE(handle2 != chandle2);

    ASSERT_EQ(handle1, chandle2);
    ASSERT_TRUE(handle1 == chandle2);
    ASSERT_FALSE(handle1 != chandle2);

    handle2 = {};
    chandle1 = {};

    ASSERT_EQ(handle1, handle2);
    ASSERT_TRUE(handle1 == handle2);
    ASSERT_FALSE(handle1 != handle2);

    ASSERT_EQ(chandle1, chandle2);
    ASSERT_TRUE(chandle1 == chandle2);
    ASSERT_FALSE(chandle1 != chandle2);

    ASSERT_EQ(handle1, chandle1);
    ASSERT_TRUE(handle1 == chandle1);
    ASSERT_FALSE(handle1 != chandle1);

    ASSERT_EQ(handle2, chandle2);
    ASSERT_TRUE(handle2 == chandle2);
    ASSERT_FALSE(handle2 != chandle2);

    ASSERT_EQ(handle1, chandle2);
    ASSERT_TRUE(handle1 == chandle2);
    ASSERT_FALSE(handle1 != chandle2);

    entt::registry registry_b;
    const auto entity_b1 = registry.create();

    handle1 = {registry_b, entity_b1};
    handle2 = {registry, entity1};
    chandle1 = handle1;
    chandle2 = handle2;

    ASSERT_NE(handle1, handle2);
    ASSERT_FALSE(handle1 == handle2);
    ASSERT_TRUE(handle1 != handle2);

    ASSERT_NE(chandle1, chandle2);
    ASSERT_FALSE(chandle1 == chandle2);
    ASSERT_TRUE(chandle1 != chandle2);

    ASSERT_EQ(handle1, chandle1);
    ASSERT_TRUE(handle1 == chandle1);
    ASSERT_FALSE(handle1 != chandle1);

    ASSERT_EQ(handle2, chandle2);
    ASSERT_TRUE(handle2 == chandle2);
    ASSERT_FALSE(handle2 != chandle2);

    ASSERT_NE(handle1, chandle2);
    ASSERT_FALSE(handle1 == chandle2);
    ASSERT_TRUE(handle1 != chandle2);
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

    handle.visit([](auto info) { ASSERT_EQ(entt::type_hash<int>::value(), info.hash()); });

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

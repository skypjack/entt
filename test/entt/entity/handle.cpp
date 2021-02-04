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

    ASSERT_FALSE(handle);
    ASSERT_EQ(handle.registry(), nullptr);
    ASSERT_EQ(handle.entity(), entt::entity{entt::null});

    entt::registry registry;
    const auto entity = registry.create();

    handle = { registry, entity };

    ASSERT_TRUE(handle);
    ASSERT_NE(handle.registry(), nullptr);
    ASSERT_NE(handle.entity(), entt::entity{entt::null});

    handle = {};

    ASSERT_FALSE(handle);
    ASSERT_EQ(handle.registry(), nullptr);
    ASSERT_EQ(handle.entity(), entt::entity{entt::null});
}

TEST(BasicHandle, Destruction) {
    entt::registry registry;
    const auto entity = registry.create();
    entt::handle handle{registry, entity};

    ASSERT_TRUE(handle);
    ASSERT_TRUE(handle.valid());
    ASSERT_NE(handle.registry(), nullptr);
    ASSERT_EQ(handle.entity(), entity);

    handle.destroy(registry.version(entity));

    ASSERT_FALSE(handle);
    ASSERT_FALSE(handle.valid());
    ASSERT_NE(handle.registry(), nullptr);
    ASSERT_EQ(handle.entity(), entity);
    ASSERT_EQ(registry.current(entity), typename entt::registry::version_type{});

    handle = entt::handle{registry, registry.create()};

    ASSERT_TRUE(handle);
    ASSERT_TRUE(handle.valid());
    ASSERT_NE(handle.registry(), nullptr);
    ASSERT_EQ(handle.entity(), entity);

    handle.destroy();

    ASSERT_FALSE(handle);
    ASSERT_FALSE(handle.valid());
    ASSERT_NE(handle.registry(), nullptr);
    ASSERT_EQ(handle.entity(), entity);
    ASSERT_NE(registry.current(entity), typename entt::registry::version_type{});
}

TEST(BasicHandle, Comparison) {
    entt::registry registry;
    const auto entity = registry.create();

    entt::handle handle{registry, entity};
    entt::const_handle chandle = handle;

    ASSERT_NE(handle, entt::handle{});
    ASSERT_FALSE(handle == entt::handle{});
    ASSERT_TRUE(handle != entt::handle{});

    ASSERT_NE(chandle, entt::const_handle{});
    ASSERT_FALSE(chandle == entt::const_handle{});
    ASSERT_TRUE(chandle != entt::const_handle{});

    ASSERT_EQ(handle, chandle);
    ASSERT_TRUE(handle == chandle);
    ASSERT_FALSE(handle != chandle);

    ASSERT_EQ(entt::handle{}, entt::const_handle{});
    ASSERT_TRUE(entt::handle{} == entt::const_handle{});
    ASSERT_FALSE(entt::handle{} != entt::const_handle{});

    handle = {};
    chandle = {};

    ASSERT_EQ(handle, entt::handle{});
    ASSERT_TRUE(handle == entt::handle{});
    ASSERT_FALSE(handle != entt::handle{});

    ASSERT_EQ(chandle, entt::const_handle{});
    ASSERT_TRUE(chandle == entt::const_handle{});
    ASSERT_FALSE(chandle != entt::const_handle{});

    entt::registry other;
    const auto entt = other.create();

    handle = { registry, entity };
    chandle = { other, entt };

    ASSERT_NE(handle, chandle);
    ASSERT_FALSE(chandle == handle);
    ASSERT_TRUE(chandle != handle);
    ASSERT_EQ(handle.entity(), chandle.entity());
    ASSERT_NE(handle.registry(), chandle.registry());
}

TEST(BasicHandle, Component) {
    entt::registry registry;
    const auto entity = registry.create();
    entt::handle_view<int, char, double> handle{registry, entity};

    ASSERT_EQ(3, handle.emplace<int>(3));
    ASSERT_EQ('c', handle.emplace_or_replace<char>('c'));
    ASSERT_EQ(.3, handle.emplace_or_replace<double>(.3));

    const auto &patched = handle.patch<int>([](auto &comp) { comp = 42; });

    ASSERT_EQ(42, patched);
    ASSERT_EQ('a', handle.replace<char>('a'));
    ASSERT_TRUE((handle.all_of<int, char, double>()));
    ASSERT_EQ((std::make_tuple(42, 'a', .3)), (handle.get<int, char, double>()));

    handle.remove<char, double>();

    ASSERT_TRUE((registry.empty<char, double>()));
    ASSERT_EQ(0u, (handle.remove_if_exists<char, double>()));

    handle.visit([](auto info) { ASSERT_EQ(entt::type_id<int>(), info); });

    ASSERT_TRUE((handle.any_of<int, char, double>()));
    ASSERT_FALSE((handle.all_of<int, char, double>()));
    ASSERT_FALSE(handle.orphan());

    handle.remove<int>();

    ASSERT_TRUE(registry.empty<int>());
    ASSERT_TRUE(handle.orphan());

    ASSERT_EQ(42, handle.get_or_emplace<int>(42));
    ASSERT_EQ(42, handle.get_or_emplace<int>(1));
    ASSERT_EQ(42, handle.get<int>());

    ASSERT_EQ(42, *handle.try_get<int>());
    ASSERT_EQ(nullptr, handle.try_get<char>());
    ASSERT_EQ(nullptr, std::get<1>(handle.try_get<int, char, double>()));
}

TEST(BasicHandle, RemoveAll) {
    entt::registry registry;
    const auto entity = registry.create();
    entt::handle handle{registry, entity};

    ASSERT_EQ(3, handle.emplace<int>(3));
    ASSERT_EQ('c', handle.emplace_or_replace<char>('c'));
    ASSERT_TRUE((handle.all_of<int, char>()));

    handle.remove_all();

    ASSERT_FALSE((handle.any_of<int, char>()));
}

TEST(BasicHandle, FromEntity) {
    entt::registry registry;
    const auto entity = registry.create();

    registry.emplace<int>(entity, 42);
    registry.emplace<char>(entity, 'c');

    entt::handle handle{registry, entity};

    ASSERT_TRUE(handle);
    ASSERT_EQ(entity, handle.entity());
    ASSERT_TRUE((handle.all_of<int, char>()));
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
    const entt::handle_view<int, char> vhandle = handle;
    const entt::const_handle_view<int> cvhandle = vhandle;

    handle.emplace<int>(42);

    ASSERT_EQ(handle.get<int>(), chandle.get<int>());
    ASSERT_EQ(chandle.get<int>(), vhandle.get<int>());
    ASSERT_EQ(vhandle.get<int>(), cvhandle.get<int>());
    ASSERT_EQ(cvhandle.get<int>(), 42);
}

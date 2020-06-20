#include <functional>
#include <gtest/gtest.h>
#include <entt/entity/proxy.hpp>
#include <entt/entity/registry.hpp>

// A proxy is very lightweight
static_assert(std::is_trivially_copyable_v<entt::proxy>);
static_assert(std::is_trivially_assignable_v<entt::proxy, entt::proxy>);
static_assert(std::is_trivially_destructible_v<entt::proxy>);

static_assert(std::is_trivially_copyable_v<entt::const_proxy>);
static_assert(std::is_trivially_assignable_v<entt::const_proxy, entt::const_proxy>);
static_assert(std::is_trivially_destructible_v<entt::const_proxy>);

TEST(Proxy, Construction) {
    entt::registry registry;
    const auto &cregistry = registry;
    const auto entity = registry.create();
    
    entt::proxy proxy1;
    entt::proxy proxy2{entity, registry};
    entt::const_proxy proxy3;
    entt::const_proxy proxy4{entity, cregistry};
    
    // ASSERT_EQ(entt::null, proxy1.entity());
    ASSERT_TRUE(entt::null == proxy1.entity());
    ASSERT_FALSE(proxy1);
    
    ASSERT_EQ(&registry, &proxy2.backend());
    ASSERT_EQ(entity, proxy2.entity());
    ASSERT_TRUE(proxy2);
    
    // ASSERT_EQ(entt::null, proxy3.entity());
    ASSERT_TRUE(entt::null == proxy3.entity());
    ASSERT_FALSE(proxy3);
    
    ASSERT_EQ(&cregistry, &proxy4.backend());
    ASSERT_EQ(entity, proxy4.entity());
    ASSERT_TRUE(proxy4);
}

TEST(Proxy, Component) {
    entt::registry registry;
    const auto entity = registry.create();
    entt::proxy proxy{entity, registry};

    ASSERT_TRUE(registry.empty<int>());
    ASSERT_FALSE(registry.empty());
    ASSERT_FALSE(proxy.has<int>());

    auto &cint = proxy.assign<int>();
    auto &cchar = proxy.assign<char>();

    ASSERT_EQ(&cint, &proxy.get<int>());
    ASSERT_EQ(&cchar, &std::as_const(proxy).get<char>());
    ASSERT_EQ(&cint, &std::get<0>(proxy.get<int, char>()));
    ASSERT_EQ(&cchar, &std::get<1>(proxy.get<int, char>()));
    ASSERT_EQ(&cint, std::get<0>(proxy.try_get<int, char, double>()));
    ASSERT_EQ(&cchar, std::get<1>(proxy.try_get<int, char, double>()));
    ASSERT_EQ(nullptr, std::get<2>(proxy.try_get<int, char, double>()));
    ASSERT_EQ(nullptr, proxy.try_get<double>());
    ASSERT_EQ(&cchar, proxy.try_get<char>());
    ASSERT_EQ(&cint, proxy.try_get<int>());

    ASSERT_FALSE(registry.empty<int>());
    ASSERT_FALSE(registry.empty());
    ASSERT_TRUE((proxy.has<int, char>()));
    ASSERT_FALSE(proxy.has<double>());

    proxy.remove<int>();

    ASSERT_TRUE(registry.empty<int>());
    ASSERT_FALSE(registry.empty());
    ASSERT_FALSE(proxy.has<int>());
}

TEST(Proxy, FromEntity) {
    entt::registry registry;
    const auto entity = registry.create();

    registry.emplace<int>(entity, 42);
    registry.emplace<char>(entity, 'c');

    entt::proxy proxy{entity, registry};

    ASSERT_TRUE(proxy);
    ASSERT_EQ(entity, proxy.entity());
    ASSERT_TRUE((proxy.has<int, char>()));
    ASSERT_EQ(proxy.get<int>(), 42);
    ASSERT_EQ(proxy.get<char>(), 'c');
}

TEST(Proxy, Lifetime) {
    entt::registry registry;
    const auto entity = registry.create();
    auto *proxy = new entt::proxy{entity, registry};
    proxy->assign<int>();

    ASSERT_FALSE(registry.empty<int>());
    ASSERT_FALSE(registry.empty());

    registry.each([proxy](const auto entity) {
        ASSERT_EQ(proxy->entity(), entity);
    });

    delete proxy;

    ASSERT_FALSE(registry.empty<int>());
    ASSERT_FALSE(registry.empty());
}

ENTT_OPAQUE_TYPE(my_entity, entt::id_type);

TEST(Proxy, DeductionGuides) {
    entt::basic_registry<my_entity> registry;
    const auto &cregistry = registry;
    const my_entity entity = registry.create();

    entt::basic_proxy proxy1{entity, registry};
    entt::basic_proxy proxy2{entity, cregistry};
    
    static_assert(std::is_same_v<decltype(proxy1), entt::basic_proxy<my_entity>>);
    static_assert(std::is_same_v<decltype(proxy2), entt::basic_proxy<const my_entity>>);
}

void add_int(entt::proxy proxy, int i) {
    proxy.assign<int>(i);
}

int get_int(entt::const_proxy proxy) {
    return proxy.get<int>();
}

TEST(Proxy, ImplicitConversions) {
    entt::registry registry;
    const auto entity = registry.create();
    const entt::proxy proxy{entity, registry};

    add_int(proxy, 42);
    ASSERT_EQ(42, get_int(proxy));
}

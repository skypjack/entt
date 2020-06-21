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
    entt::proxy proxy2{registry, entity};
    entt::const_proxy proxy3;
    entt::const_proxy proxy4{cregistry, entity};
    
    // ASSERT_EQ(entt::null, proxy1.entity());
    ASSERT_TRUE(entt::null == proxy1.entity());
    ASSERT_FALSE(proxy1);
    
    ASSERT_EQ(&registry, &proxy2.registry());
    ASSERT_EQ(entity, proxy2.entity());
    ASSERT_TRUE(proxy2);
    
    // ASSERT_EQ(entt::null, proxy3.entity());
    ASSERT_TRUE(entt::null == proxy3.entity());
    ASSERT_FALSE(proxy3);
    
    ASSERT_EQ(&cregistry, &proxy4.registry());
    ASSERT_EQ(entity, proxy4.entity());
    ASSERT_TRUE(proxy4);
    
    static_assert(std::is_same_v<entt::registry &, decltype(proxy2.registry())>);
    static_assert(std::is_same_v<const entt::registry &, decltype(proxy4.registry())>);
}

TEST(Proxy, Component) {
    // There's no need to test these functions thoroughly as the registry tests
    // already do that. We merely need to check that the functions exist.

    entt::registry registry;
    const auto entity = registry.create();
    entt::proxy proxy{registry, entity};
    
    ASSERT_EQ(1, proxy.emplace<int>(1));
    ASSERT_EQ(2, proxy.emplace_or_replace<int>(2));
    ASSERT_EQ(3, proxy.emplace_or_replace<long>(3));
    
    const auto patched = proxy.patch<int>([](auto &comp) {
      comp = 4;
    });
    ASSERT_EQ(4, patched);
    
    ASSERT_EQ(5, proxy.replace<long>(5));
    ASSERT_TRUE((proxy.has<int, long>()));
    
    ASSERT_FALSE(registry.empty<long>());
    proxy.remove<long>();
    ASSERT_TRUE(registry.empty<long>());
    ASSERT_EQ(0, proxy.remove_if_exists<long>());
    
    proxy.visit([](auto id) {
      ASSERT_EQ(entt::type_info<int>::id(), id);
    });
    ASSERT_TRUE((proxy.any<int, long>()));
    ASSERT_FALSE((proxy.has<int, long>()));
    
    ASSERT_FALSE(registry.empty<int>());
    ASSERT_FALSE(proxy.orphan());
    proxy.remove_all();
    ASSERT_TRUE(registry.empty<int>());
    ASSERT_TRUE(proxy.orphan());
    
    ASSERT_EQ(6, proxy.get_or_emplace<int>(6));
    ASSERT_EQ(6, proxy.get_or_emplace<int>(7));
    ASSERT_EQ(6, proxy.get<int>());
    
    ASSERT_EQ(6, *proxy.try_get<int>());
    ASSERT_EQ(nullptr, proxy.try_get<long>());
}

TEST(Proxy, FromEntity) {
    entt::registry registry;
    const auto entity = registry.create();

    registry.emplace<int>(entity, 42);
    registry.emplace<char>(entity, 'c');

    entt::proxy proxy{registry, entity};

    ASSERT_TRUE(proxy);
    ASSERT_EQ(entity, proxy.entity());
    ASSERT_TRUE((proxy.has<int, char>()));
    ASSERT_EQ(proxy.get<int>(), 42);
    ASSERT_EQ(proxy.get<char>(), 'c');
}

TEST(Proxy, Lifetime) {
    entt::registry registry;
    const auto entity = registry.create();
    auto *proxy = new entt::proxy{registry, entity};
    proxy->emplace<int>();

    ASSERT_FALSE(registry.empty<int>());
    ASSERT_FALSE(registry.empty());

    registry.each([proxy](const auto e) {
        ASSERT_EQ(proxy->entity(), e);
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

    entt::basic_proxy proxy1{registry, entity};
    entt::basic_proxy proxy2{cregistry, entity};
    
    static_assert(std::is_same_v<decltype(proxy1), entt::basic_proxy<my_entity>>);
    static_assert(std::is_same_v<decltype(proxy2), entt::basic_proxy<const my_entity>>);
}

void add_int(entt::proxy proxy, int i) {
    proxy.emplace<int>(i);
}

int get_int(entt::const_proxy proxy) {
    return proxy.get<int>();
}

TEST(Proxy, ImplicitConversions) {
    entt::registry registry;
    const auto entity = registry.create();
    const entt::proxy proxy{registry, entity};

    add_int(proxy, 42);
    ASSERT_EQ(42, get_int(proxy));
}

#include <gtest/gtest.h>
#include <entt/core/hashed_string.hpp>
#include <entt/entity/helper.hpp>
#include <entt/entity/registry.hpp>
#include <entt/core/type_traits.hpp>

TEST(Helper, AsView) {
    entt::registry registry;
    const entt::registry cregistry;

    ([](entt::view<entt::exclude_t<>, int>) {})(entt::as_view{registry});
    ([](entt::view<entt::exclude_t<int>, char, double>) {})(entt::as_view{registry});
    ([](entt::view<entt::exclude_t<int>, const char, double>) {})(entt::as_view{registry});
    ([](entt::view<entt::exclude_t<int>, const char, const double>) {})(entt::as_view{registry});
}

TEST(Helper, AsGroup) {
    entt::registry registry;
    const entt::registry cregistry;

    ([](entt::group<entt::exclude_t<int>, entt::get_t<char>, double>) {})(entt::as_group{registry});
    ([](entt::group<entt::exclude_t<int>, entt::get_t<const char>, double>) {})(entt::as_group{registry});
    ([](entt::group<entt::exclude_t<int>, entt::get_t<const char>, const double>) {})(entt::as_group{registry});
}

TEST(Helper, Tag) {
    entt::registry registry;
    const auto entity = registry.create();
    registry.assign<entt::tag<"foobar"_hs>>(entity);
    registry.assign<int>(entity, 42);
    int counter{};

    ASSERT_FALSE(registry.has<entt::tag<"barfoo"_hs>>(entity));
    ASSERT_TRUE(registry.has<entt::tag<"foobar"_hs>>(entity));

    for(auto entt: registry.view<int, entt::tag<"foobar"_hs>>()) {
        (void)entt;
        ++counter;
    }

    ASSERT_NE(counter, 0);

    for(auto entt: registry.view<entt::tag<"foobar"_hs>>()) {
        (void)entt;
        --counter;
    }

    ASSERT_EQ(counter, 0);
}

struct my_component {
    mutable int *foo;

    static void create_s(my_component &c)                   { *(c.foo) |=  0b00000001; }
    void create()                                           { *(  foo) |=  0b00000010; }
    void create_e(entt::entity)                             { *(  foo) |=  0b00000100; }
    void create_er(entt::entity, entt::registry &)          { *(  foo) |=  0b00001000; }
    static void create_s_c(const my_component &c)           { *(c.foo) |=  0b00010000; }
    void create_c() const                                   { *(  foo) |=  0b00100000; }
    void create_e_c(entt::entity) const                     { *(  foo) |=  0b01000000; }
    void create_er_c(entt::entity, entt::registry &) const  { *(  foo) |=  0b10000000; }

    static void destroy_s(my_component &c)                  { *(c.foo) &= ~0b00000001; }
    void destroy()                                          { *(  foo) &= ~0b00000010; }
    void destroy_e(entt::entity)                            { *(  foo) &= ~0b00000100; }
    void destroy_er(entt::entity, entt::registry &)         { *(  foo) &= ~0b00001000; }
    static void destroy_s_c(const my_component &c)          { *(c.foo) &= ~0b00010000; }
    void destroy_c() const                                  { *(  foo) &= ~0b00100000; }
    void destroy_e_c(entt::entity) const                    { *(  foo) &= ~0b01000000; }
    void destroy_er_c(entt::entity, entt::registry &) const { *(  foo) &= ~0b10000000; }
};

TEST(Helper, InvokeOnComponent) {
    entt::registry registry;
    const auto entity = registry.create();

    registry.on_construct<my_component>().connect<entt::invoke_on_component<&my_component::create_s>>();
    registry.on_construct<my_component>().connect<entt::invoke_on_component<&my_component::create>>();
    registry.on_construct<my_component>().connect<entt::invoke_on_component<&my_component::create_e>>();
    registry.on_construct<my_component>().connect<entt::invoke_on_component<&my_component::create_er>>();
    registry.on_construct<my_component>().connect<entt::invoke_on_component<&my_component::create_s_c>>();
    registry.on_construct<my_component>().connect<entt::invoke_on_component<&my_component::create_c>>();
    registry.on_construct<my_component>().connect<entt::invoke_on_component<&my_component::create_e_c>>();
    registry.on_construct<my_component>().connect<entt::invoke_on_component<&my_component::create_er_c>>();

    registry.on_destroy<my_component>().connect<entt::invoke_on_component<&my_component::destroy_s>>();
    registry.on_destroy<my_component>().connect<entt::invoke_on_component<&my_component::destroy>>();
    registry.on_destroy<my_component>().connect<entt::invoke_on_component<&my_component::destroy_e>>();
    registry.on_destroy<my_component>().connect<entt::invoke_on_component<&my_component::destroy_er>>();
    registry.on_destroy<my_component>().connect<entt::invoke_on_component<&my_component::destroy_s_c>>();
    registry.on_destroy<my_component>().connect<entt::invoke_on_component<&my_component::destroy_c>>();
    registry.on_destroy<my_component>().connect<entt::invoke_on_component<&my_component::destroy_e_c>>();
    registry.on_destroy<my_component>().connect<entt::invoke_on_component<&my_component::destroy_er_c>>();

    int result = 0;
    registry.assign<my_component>(entity, &result);
    ASSERT_EQ(result, 0b11111111);
    registry.remove<my_component>(entity);
    ASSERT_EQ(result, 0);
}

#include <gtest/gtest.h>
#include <entt/entity/entity.hpp>
#include <entt/entity/registry.hpp>
#include <entt/meta/factory.hpp>
#include <entt/signal/dispatcher.hpp>
#include <entt/signal/emitter.hpp>
#include "types.h"

extern typename entt::component a_module_int_type();
extern typename entt::component a_module_char_type();
extern typename entt::component another_module_int_type();
extern typename entt::component another_module_char_type();

extern void update_position(int delta, entt::registry &);
extern void assign_velocity(int, entt::registry &);

extern void trigger_an_event(int, entt::dispatcher &);
extern void trigger_another_event(entt::dispatcher &);

extern void emit_an_event(int, test_emitter &);
extern void emit_another_event(test_emitter &);

extern void a_module_bind_ctx(entt::meta_ctx);
extern void another_module_bind_ctx(entt::meta_ctx);

extern void a_module_meta_init();
extern void another_module_meta_init();

extern void a_module_meta_deinit();
extern void another_module_meta_deinit();

struct listener {
    void on_an_event(an_event event) { value = event.payload; }
    void on_another_event(another_event) {}

    int value;
};

TEST(Lib, Types) {
    entt::registry registry;

    ASSERT_EQ(registry.type<int>(), registry.type<const int>());
    ASSERT_EQ(registry.type<char>(), registry.type<const char>());

    ASSERT_EQ(registry.type<int>(), a_module_int_type());
    ASSERT_EQ(registry.type<char>(), a_module_char_type());
    ASSERT_EQ(registry.type<const int>(), a_module_int_type());
    ASSERT_EQ(registry.type<const char>(), a_module_char_type());

    ASSERT_EQ(registry.type<const char>(), another_module_char_type());
    ASSERT_EQ(registry.type<const int>(), another_module_int_type());
    ASSERT_EQ(registry.type<char>(), another_module_char_type());
    ASSERT_EQ(registry.type<int>(), another_module_int_type());
}

TEST(Lib, Registry) {
    entt::registry registry;

    for(auto i = 0; i < 3; ++i) {
        const auto entity = registry.create();
        registry.assign<position>(entity, i, i+1);
    }

    assign_velocity(2, registry);

    ASSERT_EQ(registry.size<position>(), entt::registry::size_type{3});
    ASSERT_EQ(registry.size<velocity>(), entt::registry::size_type{3});

    update_position(1, registry);

    registry.view<position>().each([](auto entity, auto &position) {
        ASSERT_EQ(position.x, entt::to_integer(entity) + 2);
        ASSERT_EQ(position.y, entt::to_integer(entity) + 3);
    });
}

TEST(Lib, Dispatcher) {
    entt::dispatcher dispatcher;
    listener listener;

    dispatcher.sink<an_event>().connect<&listener::on_an_event>(listener);
    dispatcher.sink<another_event>().connect<&listener::on_another_event>(listener);

    listener.value = 0;

    trigger_an_event(3, dispatcher);
    trigger_another_event(dispatcher);

    ASSERT_EQ(listener.value, 3);
}

TEST(Lib, Emitter) {
    test_emitter emitter;

    emitter.once<another_event>([](another_event, test_emitter &) {});
    emitter.once<an_event>([](an_event event, test_emitter &) {
        ASSERT_EQ(event.payload, 3);
    });

    emit_an_event(3, emitter);
    emit_another_event(emitter);

    emitter.once<an_event>([](an_event event, test_emitter &) {
        ASSERT_EQ(event.payload, 42);
    });

    emit_an_event(42, emitter);
    emit_another_event(emitter);
}

TEST(Lib, Meta) {
    ASSERT_FALSE(entt::resolve("double"_hs));
    ASSERT_FALSE(entt::resolve("char"_hs));
    ASSERT_FALSE(entt::resolve("int"_hs));
    ASSERT_FALSE(entt::resolve<int>().data("0"_hs));
    ASSERT_FALSE(entt::resolve<char>().data("c"_hs));

    a_module_bind_ctx(entt::meta_ctx{});
    another_module_bind_ctx(entt::meta_ctx{});

    entt::meta<double>().type("double"_hs).conv<int>();
    another_module_meta_init();
    a_module_meta_init();

    ASSERT_TRUE(entt::resolve("double"_hs));
    ASSERT_TRUE(entt::resolve("char"_hs));
    ASSERT_TRUE(entt::resolve("int"_hs));
    ASSERT_TRUE(entt::resolve<int>().data("0"_hs));
    ASSERT_TRUE(entt::resolve<char>().data("c"_hs));

    a_module_meta_deinit();
    entt::meta().reset();
    another_module_meta_deinit();

    ASSERT_FALSE(entt::resolve("double"_hs));
    ASSERT_FALSE(entt::resolve("char"_hs));
    ASSERT_FALSE(entt::resolve("int"_hs));
    ASSERT_FALSE(entt::resolve<int>().data("0"_hs));
    ASSERT_FALSE(entt::resolve<char>().data("c"_hs));
}

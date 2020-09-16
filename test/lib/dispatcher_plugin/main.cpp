#define CR_HOST

#include <cr.h>
#include <gtest/gtest.h>
#include <entt/core/type_info.hpp>
#include <entt/signal/dispatcher.hpp>
#include "type_context.h"
#include "types.h"

template<typename Type>
struct entt::type_seq<Type> {
    [[nodiscard]] static id_type value() ENTT_NOEXCEPT {
        static const entt::id_type value = type_context::instance()->value(entt::type_hash<Type>::value());
        return value;
    }
};

struct listener {
    void on(message msg) { value = msg.payload; }
    int value{};
};

TEST(Lib, Dispatcher) {
    entt::dispatcher dispatcher;
    listener listener;

    ASSERT_EQ(listener.value, 0);

    dispatcher.sink<message>().connect<&listener::on>(listener);

    cr_plugin ctx;
    cr_plugin_load(ctx, PLUGIN);

    ctx.userdata = type_context::instance();
    cr_plugin_update(ctx);

    ctx.userdata = &dispatcher;
    cr_plugin_update(ctx);

    ASSERT_EQ(listener.value, 42);

    dispatcher = {};
    cr_plugin_close(ctx);
}

#define CR_HOST

#include <cr.h>
#include <gtest/gtest.h>
#include <entt/core/type_info.hpp>
#include <entt/signal/emitter.hpp>
#include "type_context.h"
#include "types.h"

template<typename Type>
struct entt::type_seq<Type> {
    [[nodiscard]] static id_type value() ENTT_NOEXCEPT {
        static const entt::id_type value = type_context::instance()->value(entt::type_hash<Type>::value());
        return value;
    }
};

TEST(Lib, Emitter) {
    test_emitter emitter;
    int value{};

    ASSERT_EQ(value, 0);

    emitter.once<message>([&](message msg, test_emitter &) { value = msg.payload; });

    cr_plugin ctx;
    cr_plugin_load(ctx, PLUGIN);

    ctx.userdata = type_context::instance();
    cr_plugin_update(ctx);

    ctx.userdata = &emitter;
    cr_plugin_update(ctx);

    ASSERT_EQ(value, 42);

    emitter = {};
    cr_plugin_close(ctx);
}

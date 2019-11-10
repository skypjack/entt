#include <entt/entity/registry.hpp>
#include <entt/meta/factory.hpp>
#include <entt/signal/dispatcher.hpp>
#include <entt/signal/emitter.hpp>
#include "types.h"

#ifndef LIB_EXPORT
#if defined _WIN32 || defined __CYGWIN__
#define LIB_EXPORT __declspec(dllexport)
#elif defined __GNUC__
#define LIB_EXPORT __attribute__((visibility("default")))
#else
#define LIB_EXPORT
#endif
#endif

LIB_EXPORT typename entt::component a_module_int_type() {
    entt::registry registry;

    (void)registry.type<double>();
    (void)registry.type<float>();

    return registry.type<int>();
}

LIB_EXPORT typename entt::component a_module_char_type() {
    entt::registry registry;

    (void)registry.type<double>();
    (void)registry.type<float>();

    return registry.type<char>();
}

LIB_EXPORT void update_position(int delta, entt::registry &registry) {
    registry.view<position, velocity>().each([delta](auto &pos, auto &vel) {
        pos.x += delta * vel.dx;
        pos.y += delta * vel.dy;
    });
}

LIB_EXPORT void trigger_another_event(entt::dispatcher &dispatcher) {
    dispatcher.trigger<another_event>();
}

LIB_EXPORT void emit_another_event(test_emitter &emitter) {
    emitter.publish<another_event>();
}

LIB_EXPORT void a_module_bind_ctx(entt::meta_ctx context) {
    entt::meta_ctx::bind(context);
}

LIB_EXPORT void a_module_meta_init() {
    entt::meta<char>().type().data<'c'>("c"_hs);
}

LIB_EXPORT void a_module_meta_deinit() {
    entt::meta().reset();
}

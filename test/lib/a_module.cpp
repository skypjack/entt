#include <entt/entity/registry.hpp>
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

ENTT_SHARED_TYPE(int)
ENTT_SHARED_TYPE(char)
ENTT_SHARED_TYPE(double)
ENTT_SHARED_TYPE(float)

LIB_EXPORT typename entt::registry<>::component_type a_module_int_type() {
    entt::registry<> registry;

    (void)registry.type<double>();
    (void)registry.type<float>();

    return registry.type<int>();
}

LIB_EXPORT typename entt::registry<>::component_type a_module_char_type() {
    entt::registry<> registry;

    (void)registry.type<double>();
    (void)registry.type<float>();

    return registry.type<char>();
}

LIB_EXPORT void update_position(int delta, entt::registry<> &registry) {
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

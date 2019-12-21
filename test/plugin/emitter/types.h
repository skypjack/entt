#ifndef ENTT_PLUGIN_EMITTER_TYPES_H
#define ENTT_PLUGIN_EMITTER_TYPES_H

#include <entt/signal/emitter.hpp>

struct test_emitter
        : entt::emitter<test_emitter>
{};

struct message {
    int payload;
};

struct event {};

#endif // ENTT_PLUGIN_EMITTER_TYPES_H

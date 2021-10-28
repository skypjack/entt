#ifndef ENTT_LIB_EMITTER_PLUGIN_TYPES_H
#define ENTT_LIB_EMITTER_PLUGIN_TYPES_H

#include <entt/signal/emitter.hpp>

struct test_emitter
    : entt::emitter<test_emitter> {};

struct message {
    int payload;
};

struct event {};

#endif

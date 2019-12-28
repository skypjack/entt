#ifndef ENTT_LIB_EMITTER_PLUGIN_STD_TYPES_H
#define ENTT_LIB_EMITTER_PLUGIN_STD_TYPES_H

#include <entt/signal/emitter.hpp>

struct test_emitter
        : entt::emitter<test_emitter>
{};

struct message {
    int payload;
};

struct event {};

struct emitter_proxy {
    virtual ~emitter_proxy() = default;
    virtual void publish(message) = 0;
    virtual void publish(event) = 0;
};

#endif

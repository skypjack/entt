#ifndef ENTT_LIB_DISPATCHER_PLUGIN_TYPES_H
#define ENTT_LIB_DISPATCHER_PLUGIN_TYPES_H

struct message {
    int payload;
};

struct event {};

struct dispatcher_proxy {
    virtual ~dispatcher_proxy() = default;
    virtual void trigger(message) = 0;
    virtual void trigger(event) = 0;
};

#endif

#ifndef ENTT_LIB_DISPATCHER_PLUGIN_PROXY_H
#define ENTT_LIB_DISPATCHER_PLUGIN_PROXY_H

#include <entt/signal/fwd.hpp>
#include "types.h"

struct proxy: dispatcher_proxy {
    proxy(entt::dispatcher &);
    void trigger(message) override;
    void trigger(event) override;

private:
    entt::dispatcher *dispatcher;
};

#endif

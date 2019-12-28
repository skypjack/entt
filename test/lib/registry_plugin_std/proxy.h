#ifndef ENTT_LIB_REGISTRY_PLUGIN_STD_PROXY_H
#define ENTT_LIB_REGISTRY_PLUGIN_STD_PROXY_H

#include <entt/entity/fwd.hpp>
#include "types.h"

struct proxy: registry_proxy {
    proxy(entt::registry &);
    void for_each(void(*)(position &, velocity &)) override;
    void assign(velocity) override;

private:
    entt::registry *registry;
};

#endif

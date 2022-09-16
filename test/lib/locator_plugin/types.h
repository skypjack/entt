#ifndef ENTT_LIB_LOCATOR_PLUGIN_TYPES_H
#define ENTT_LIB_LOCATOR_PLUGIN_TYPES_H

#include <entt/locator/locator.hpp>

struct service {
    int value;
};

struct userdata {
    using node_type = typename entt::locator<service>::node_type;
    node_type handle;
    int value;
};

#endif

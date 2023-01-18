#ifndef ENTT_LIB_LOCATOR_PLUGIN_TYPES_H
#define ENTT_LIB_LOCATOR_PLUGIN_TYPES_H

#include <entt/locator/locator.hpp>

struct service;

struct userdata {
    typename entt::locator<service>::node_type handle;
    int value;
};

#endif

#ifndef ENTT_LIB_META_PLUGIN_TYPES_H
#define ENTT_LIB_META_PLUGIN_TYPES_H

#include <entt/locator/locator.hpp>

class meta_ctx;

struct userdata {
    entt::locator<entt::meta_ctx>::node_type ctx;
    entt::meta_any any;
};

#endif

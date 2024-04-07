#ifndef ENTT_LIB_META_PLUGIN_USERDATA_H
#define ENTT_LIB_META_PLUGIN_USERDATA_H

#include <entt/locator/locator.hpp>
#include <entt/meta/context.hpp>
#include <entt/meta/meta.hpp>

struct userdata {
    entt::locator<entt::meta_ctx>::node_type ctx{};
    entt::meta_any any{};
};

#endif

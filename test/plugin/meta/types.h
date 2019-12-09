#ifndef ENTT_PLUGIN_META_TYPES_H
#define ENTT_PLUGIN_META_TYPES_H

#include <entt/meta/meta.hpp>

struct position {
    int x{};
    int y{};
};

struct velocity {
    double dx{};
    double dy{};
};

struct userdata {
    entt::meta_ctx ctx;
    entt::meta_any any;
};

#endif // ENTT_PLUGIN_META_TYPES_H

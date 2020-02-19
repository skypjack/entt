#ifndef ENTT_LIB_META_PLUGIN_TYPES_H
#define ENTT_LIB_META_PLUGIN_TYPES_H

#include <entt/meta/meta.hpp>

struct position {
    int x;
    int y;
};

struct velocity {
    double dx;
    double dy;
};

struct userdata {
    entt::meta_ctx ctx;
    entt::meta_any any;
};

#endif

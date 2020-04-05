#ifndef ENTT_LIB_REGISTRY_TYPES_H
#define ENTT_LIB_REGISTRY_TYPES_H

#include <entt/core/attribute.h>

struct ENTT_API position {
    int x;
    int y;
};

struct ENTT_API velocity {
    double dx;
    double dy;
};

#endif

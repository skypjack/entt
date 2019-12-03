#ifndef ENTT_LIB_META_TYPES_H
#define ENTT_LIB_META_TYPES_H

#include <entt/lib/attribute.h>

struct ENTT_API position {
    int x{};
    int y{};
};

struct ENTT_API velocity {
    double dx{};
    double dy{};
};

#endif // ENTT_LIB_META_TYPES_H

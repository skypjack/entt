#include <entt/core/type_traits.hpp>

struct position {
    int x;
    int y;
};

struct velocity {
    int dx;
    int dy;
};

ENTT_SHARED_TYPE(position)
ENTT_SHARED_TYPE(velocity)

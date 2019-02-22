#include <entt/core/type_traits.hpp>

ENTT_SHARED_STRUCT(position, {
    int x;
    int y;
})

ENTT_SHARED_STRUCT(velocity, {
    int dx;
    int dy;
})

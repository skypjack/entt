#include <entt/core/type_traits.hpp>

ENTT_NAMED_STRUCT(position, {
    int x;
    int y;
});

ENTT_NAMED_STRUCT(velocity, {
    int dx;
    int dy;
});

ENTT_NAMED_TYPE(int);
ENTT_NAMED_TYPE(char);

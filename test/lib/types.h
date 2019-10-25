#include <entt/core/type_traits.hpp>
#include <entt/signal/emitter.hpp>

struct test_emitter
        : entt::emitter<test_emitter>
{};

struct position {
    int x;
    int y;
};

struct velocity {
    int dx;
    int dy;
};

struct an_event {
    int payload;
};

struct another_event {};

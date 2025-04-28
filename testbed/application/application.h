#pragma once

#include <SDL3/SDL_events.h>
#include <entt/entity/fwd.hpp>

struct SDL_Renderer;

namespace testbed {

struct config;
struct context;

class application {
    void update(entt::registry &);
    void draw(entt::registry &, const context &) const;
    void input(entt::registry &);

public:
    application();
    ~application();

    int run();

private:
    bool quit;
};

} // namespace testbed

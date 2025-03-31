#pragma once

#include <SDL3/SDL_events.h>

struct SDL_Renderer;

namespace testbed {

struct config;
struct context;

class application {
    void update();
    void draw(const context &) const;
    void input();

public:
    application();
    ~application();

    int run();

private:
    bool quit;
};

} // namespace testbed

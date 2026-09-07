#pragma once

struct SDL_Renderer;
struct SDL_Window;

namespace testbed {

struct context {
    context();
    ~context();

    context(const context &) = delete;
    context(context &&) = delete;

    context &operator=(const context &) = delete;
    context &operator=(context &&) = delete;

    static constexpr auto logical_width() noexcept {
        return 1920;
    }

    static constexpr auto logical_height() noexcept {
        return 1080;
    }

    SDL_Window *window() const noexcept;
    SDL_Renderer *renderer() const noexcept;

    operator SDL_Window *() const noexcept;
    operator SDL_Renderer *() const noexcept;

private:
    SDL_Window *sdl_window;
    SDL_Renderer *sdl_renderer;
};

} // namespace testbed

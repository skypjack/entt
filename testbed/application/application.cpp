#include <cstdint>
#include <SDL3/SDL.h>
#include <SDL3/SDL_stdinc.h>
#include <application/application.h>
#include <application/context.h>
#include <backends/imgui_impl_sdl3.h>
#include <backends/imgui_impl_sdlrenderer3.h>
#include <component/input_listener_component.h>
#include <component/position_component.h>
#include <component/rect_component.h>
#include <component/renderable_component.h>
#include <component/velocity_component.h>
#include <entt/entity/registry.hpp>
#include <imgui.h>
#include <meta/meta.h>
#include <system/command_system.h>
#include <system/imgui_system.h>
#include <system/input_system.h>
#include <system/movement_system.h>
#include <system/rendering_system.h>

namespace testbed {

void application::update(entt::registry &registry, const context &ctx, const double delta) {
    ImGui_ImplSDLRenderer3_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();

    command_system(registry);
    movement_system(registry, ctx, delta);
}

void application::draw(entt::registry &registry, const context &context) const {
    SDL_SetRenderDrawColor(context, 0u, 0u, 0u, SDL_ALPHA_OPAQUE);
    SDL_RenderClear(context);

    rendering_system(registry, context);
    imgui_system(registry);

    ImGui::Render();
    ImGuiIO &io = ImGui::GetIO();
    SDL_SetRenderScale(context, io.DisplayFramebufferScale.x, io.DisplayFramebufferScale.y);
    ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), context);

    SDL_RenderPresent(context);
}

void application::input(entt::registry &registry) {
    ImGuiIO &io = ImGui::GetIO();
    SDL_Event event{};

    while(SDL_PollEvent(&event)) {
        ImGui_ImplSDL3_ProcessEvent(&event);
        input_system(registry, event, quit);
    }
}

application::application()
    : quit{} {
    SDL_Init(SDL_INIT_EVENTS | SDL_INIT_VIDEO);
}

application::~application() {
    SDL_Quit();
}

static void static_setup_for_dev_purposes(entt::registry &registry) {
    const auto entt = registry.create();

    // makes the entity a vessel for input
    registry.emplace<input_listener_component>(entt);

    // makes the entity able to move around
    registry.emplace<position_component>(entt, SDL_FPoint{400.f, 400.f});
    registry.emplace<velocity_component>(entt);

    // makes the entity visible on the screen
    registry.emplace<rect_component>(entt, SDL_FRect{0.f, 0.f, 20.f, 20.f});
    registry.emplace<renderable_component>(entt);
}

int application::run() {
    entt::registry registry{};
    context context{};

    meta_setup();
    static_setup_for_dev_purposes(registry);

    Uint64 last{SDL_GetPerformanceCounter()};
    Uint64 current{};
    double delta{};

    quit = false;

    while(!quit) {
        current = SDL_GetPerformanceCounter();
        delta = (current - last) / static_cast<double>(SDL_GetPerformanceFrequency());
        last = current;

        update(registry, context, delta);
        draw(registry, context);
        input(registry);
    }

    return 0;
}

} // namespace testbed

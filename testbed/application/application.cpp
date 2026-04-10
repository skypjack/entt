#include <cstdint>
#include <SDL3/SDL_timer.h>
#include <SDL3/SDL.h>
#include <application/application.h>
#include <application/context.h>
#include <backends/imgui_impl_sdl3.h>
#include <backends/imgui_impl_sdlrenderer3.h>
#include <component/collectible_component.h>
#include <component/color_component.h>
#include <component/game_state_component.h>
#include <component/input_listener_component.h>
#include <component/player_component.h>
#include <component/position_component.h>
#include <component/rect_component.h>
#include <component/renderable_component.h>
#include <entt/entity/registry.hpp>
#include <imgui.h>
#include <meta/meta.h>
#include <system/gameplay_system.h>
#include <system/hud_system.h>
#include <system/imgui_system.h>
#include <system/input_system.h>
#include <system/rendering_system.h>

namespace testbed {

void application::update(entt::registry &registry) {
    ImGui_ImplSDLRenderer3_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();

    static Uint64 last_tick = SDL_GetTicks();
    const Uint64 current_tick = SDL_GetTicks();
    const float delta_time = static_cast<float>(current_tick - last_tick) / 1000.f;
    last_tick = current_tick;

    gameplay_system(registry, delta_time);
}

void application::draw(entt::registry &registry, const context &context) const {
    SDL_SetRenderDrawColor(context, 0u, 0u, 0u, SDL_ALPHA_OPAQUE);
    SDL_RenderClear(context);

    rendering_system(registry, context);
    hud_system(registry, context);
    imgui_system(registry);

    ImGui::Render();
    ImGuiIO &io = ImGui::GetIO();
    SDL_SetRenderScale(context, io.DisplayFramebufferScale.x, io.DisplayFramebufferScale.y);
    ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), context);

    SDL_RenderPresent(context);
}

void application::input(entt::registry &registry) {
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

void application::setup(entt::registry &registry) const {
    registry.ctx().emplace<game_state_component>();

    const auto player = registry.create();
    registry.emplace<input_listener_component>(player);
    registry.emplace<player_component>(player, 420.f);
    registry.emplace<position_component>(player, SDL_FPoint{240.f, 240.f});
    registry.emplace<rect_component>(player, SDL_FRect{0.f, 0.f, 72.f, 72.f});
    registry.emplace<color_component>(player, color_component{99u, 179u, 237u});
    registry.emplace<renderable_component>(player);

    const auto collectible = registry.create();
    registry.emplace<collectible_component>(collectible, 1);
    registry.emplace<position_component>(collectible, SDL_FPoint{920.f, 420.f});
    registry.emplace<rect_component>(collectible, SDL_FRect{0.f, 0.f, 48.f, 48.f});
    registry.emplace<color_component>(collectible, color_component{255u, 211u, 92u});
    registry.emplace<renderable_component>(collectible);
}

int application::run() {
    entt::registry registry{};
    context context{};

    meta_setup();
    setup(registry);

    quit = false;

    while(!quit) {
        update(registry);
        draw(registry, context);
        input(registry);
    }

    return 0;
}

} // namespace testbed

#include <cstdint>
#include <SDL3/SDL.h>
#include <application/application.h>
#include <application/context.h>
#include <backends/imgui_impl_sdl3.h>
#include <backends/imgui_impl_sdlrenderer3.h>
#include <imgui.h>

namespace testbed {

void application::update() {
    ImGui_ImplSDLRenderer3_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();

    // update...
}

void application::draw(const context &context) const {
    SDL_SetRenderDrawColor(context, 0u, 0u, 0u, SDL_ALPHA_OPAQUE);
    SDL_RenderClear(context);

    // draw...

    ImGui::Render();
    ImGuiIO &io = ImGui::GetIO();
    SDL_SetRenderScale(context, io.DisplayFramebufferScale.x, io.DisplayFramebufferScale.y);
    ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), context);

    SDL_RenderPresent(context);
}

void application::input() {
    ImGuiIO &io = ImGui::GetIO();
    SDL_Event event{};

    while(SDL_PollEvent(&event)) {
        ImGui_ImplSDL3_ProcessEvent(&event);

        switch(event.type) {
        case SDL_EVENT_KEY_DOWN:
            switch(event.key.key) {
            case SDLK_ESCAPE:
                quit = true;
                break;
            }
            break;
        }
    }
}

application::application()
    : quit{} {
    SDL_Init(SDL_INIT_EVENTS | SDL_INIT_VIDEO);
}

application::~application() {
    SDL_Quit();
}

int application::run() {
    context context{};
    quit = false;

    while(!quit) {
        update();
        draw(context);
        input();
    }

    return 0;
}

} // namespace testbed

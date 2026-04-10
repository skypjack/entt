#include <application/context.h>
#include <component/game_state_component.h>
#include <entt/entity/registry.hpp>
#include <imgui.h>
#include <system/hud_system.h>

namespace testbed {

void hud_system(entt::registry &registry, const context &ctx) {
    const auto &state = registry.ctx().get<game_state_component>();
    static_cast<void>(ctx);

    ImGui::SetNextWindowPos(ImVec2{20.f, 20.f}, ImGuiCond_Always);
    ImGui::SetNextWindowBgAlpha(0.8f);

    if(ImGui::Begin("Collector HUD", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove)) {
        ImGui::Text("Arrow keys / WASD to move");
        ImGui::Text("Collect the gold square before time runs out");
        ImGui::Separator();
        ImGui::Text("Score: %d", state.score);
        ImGui::Text("Best score: %d", state.best_score);
        ImGui::Text("Time left: %.1f s", state.remaining_time);

        if(!state.running) {
            ImGui::Separator();
            ImGui::TextUnformatted("Round complete. Restart the app to play again.");
        }
    }

    ImGui::End();
}

} // namespace testbed

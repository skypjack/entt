#include <entt/davey/davey.hpp>
#include <entt/entity/registry.hpp>
#include <imgui.h>
#include <system/imgui_system.h>

namespace testbed {

void imgui_system(const entt::registry &registry) {
    ImGui::Begin("Davey - registry");
    entt::davey(registry);
    ImGui::End();

    ImGui::Begin("Davey - view");
    entt::davey(registry.view<int, double>());
    ImGui::End();

    ImGui::Begin("Davey - storage");
    entt::davey(*registry.storage<int>());
    ImGui::End();
}

} // namespace testbed

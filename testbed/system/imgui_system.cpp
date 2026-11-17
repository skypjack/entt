#include <utility>
#include <component/input_listener_component.h>
#include <component/rect_component.h>
#include <component/renderable_component.h>
#include <entt/entity/registry.hpp>
#include <entt/tools/davey.hpp>
#include <imgui.h>
#include <system/imgui_system.h>

namespace testbed {

void imgui_system(entt::registry &registry) {
    ImGui::Begin("Davey - Registry Introspection - Demo");

    if(ImGui::CollapsingHeader("Registry")) {
        ImGui::PushID("Registry");
        entt::davey(registry);
        ImGui::PopID();
    }

    if(ImGui::CollapsingHeader("Const Registry")) {
        ImGui::PushID("Const Registry");
        entt::davey(std::as_const(registry));
        ImGui::PopID();
    }

    if(ImGui::CollapsingHeader("View - Renderable & Rect")) {
        ImGui::PushID("View");
        entt::davey(registry.view<renderable_component, rect_component>());
        ImGui::PopID();
    }

    if(ImGui::CollapsingHeader("Storage - Input Listener")) {
        ImGui::PushID("Storage");
        entt::davey(registry.storage<input_listener_component>());
        ImGui::PopID();
    }

    ImGui::End();
}

} // namespace testbed

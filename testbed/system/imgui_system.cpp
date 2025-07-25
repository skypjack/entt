#include <component/input_listener_component.h>
#include <component/rect_component.h>
#include <component/renderable_component.h>
#include <entt/entity/registry.hpp>
#include <entt/tools/davey.hpp>
#include <imgui.h>
#include <system/imgui_system.h>

namespace testbed {

void imgui_system(const entt::registry &registry) {
    ImGui::Begin("Davey - registry");
    entt::davey(registry);
    ImGui::End();

    ImGui::Begin("Davey - view");
    entt::davey(registry.view<renderable_component, rect_component>());
    ImGui::End();

    ImGui::Begin("Davey - storage");
    entt::davey(*registry.storage<input_listener_component>());
    ImGui::End();
}

} // namespace testbed

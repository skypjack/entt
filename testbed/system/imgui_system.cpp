#include <entt/entity/registry.hpp>
#include <imgui.h>
#include <system/imgui_system.h>

namespace testbed {

void imgui_system(entt::registry &registry) {
    ImGui::Begin("testbed");

    // ...
    static_cast<void>(registry);

    ImGui::End();
}

} // namespace testbed

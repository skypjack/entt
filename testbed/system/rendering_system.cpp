#include <SDL3/SDL_render.h>
#include <application/context.h>
#include <component/position_component.h>
#include <component/rect_component.h>
#include <component/renderable_component.h>
#include <entt/entity/registry.hpp>
#include <system/rendering_system.h>

namespace testbed {

void rendering_system(entt::registry &registry, const context &ctx) {
    constexpr int logical_width = 1920;
    constexpr int logical_height = 1080;

    SDL_SetRenderLogicalPresentation(ctx, logical_width, logical_height, SDL_LOGICAL_PRESENTATION_LETTERBOX);
    SDL_SetRenderDrawColor(ctx, 0u, 0u, 0u, SDL_ALPHA_OPAQUE);
    SDL_RenderClear(ctx);

    for(auto [entt, pos, rect]: registry.view<renderable_component, position_component, rect_component>().each()) {
        SDL_FRect elem{rect.x + pos.x, rect.y + pos.y, rect.w, rect.h};
        SDL_SetRenderDrawColor(ctx, 255u, 255u, 255u, SDL_ALPHA_OPAQUE);
        SDL_RenderRect(ctx, &elem);
    }

    SDL_SetRenderLogicalPresentation(ctx, 0, 0, SDL_LOGICAL_PRESENTATION_DISABLED);
}

} // namespace testbed

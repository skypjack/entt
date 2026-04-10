#include <algorithm>
#include <cmath>
#include <cstdint>
#include <SDL3/SDL_rect.h>
#include <component/collectible_component.h>
#include <component/game_state_component.h>
#include <component/input_listener_component.h>
#include <component/player_component.h>
#include <component/position_component.h>
#include <component/rect_component.h>
#include <entt/entity/registry.hpp>
#include <system/gameplay_system.h>

namespace testbed {

namespace {

[[nodiscard]] bool intersects(const SDL_FRect &lhs, const SDL_FRect &rhs) {
    return lhs.x < (rhs.x + rhs.w) && (lhs.x + lhs.w) > rhs.x && lhs.y < (rhs.y + rhs.h) && (lhs.y + lhs.h) > rhs.y;
}

[[nodiscard]] SDL_FRect world_rect(const position_component &pos, const rect_component &rect) {
    return SDL_FRect{pos.x + rect.x, pos.y + rect.y, rect.w, rect.h};
}

void clamp_player(position_component &position, const rect_component &rect) {
    constexpr float world_width = 1920.f;
    constexpr float world_height = 1080.f;

    position.x = std::clamp(position.x, 0.f, world_width - rect.w);
    position.y = std::clamp(position.y, 0.f, world_height - rect.h);
}

void respawn_collectible(position_component &position, int score) {
    constexpr float min_x = 80.f;
    constexpr float min_y = 120.f;
    constexpr float x_span = 1680.f;
    constexpr float y_span = 780.f;

    const auto seed = static_cast<float>((score * 131) % 997);
    position.x = min_x + std::fmod(seed * 73.f, x_span);
    position.y = min_y + std::fmod(seed * 37.f, y_span);
}

} // namespace

void gameplay_system(entt::registry &registry, float delta_time) {
    auto &state = registry.ctx().get<game_state_component>();

    if(state.running) {
        state.remaining_time = std::max(0.f, state.remaining_time - delta_time);
        state.running = state.remaining_time > 0.f;
    }

    auto player_view = registry.view<player_component, input_listener_component, position_component, rect_component>();

    for(auto [entity, player, input, position, rect]: player_view.each()) {
        static_cast<void>(entity);

        if(state.running) {
            float x_axis = 0.f;
            float y_axis = 0.f;

            if(input.left) {
                x_axis -= 1.f;
            }

            if(input.right) {
                x_axis += 1.f;
            }

            if(input.up) {
                y_axis -= 1.f;
            }

            if(input.down) {
                y_axis += 1.f;
            }

            position.x += x_axis * player.speed * delta_time;
            position.y += y_axis * player.speed * delta_time;
            clamp_player(position, rect);
        }

        const auto player_rect = world_rect(position, rect);
        auto collectible_view = registry.view<collectible_component, position_component, rect_component>();

        for(auto [collectible_entity, collectible, collectible_position, collectible_rect]: collectible_view.each()) {
            if(state.running && intersects(player_rect, world_rect(collectible_position, collectible_rect))) {
                state.score += collectible.value;
                state.best_score = std::max(state.best_score, state.score);
                respawn_collectible(collectible_position, state.score);

                collectible.value = 1 + (state.score / 5);
                break;
            }

            static_cast<void>(collectible_entity);
        }
    }
}

} // namespace testbed

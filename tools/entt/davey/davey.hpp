#ifndef ENTT_DAVEY_DAVEY_HPP
#define ENTT_DAVEY_DAVEY_HPP

#include <string>
#include <entt/entity/mixin.hpp>
#include <entt/entity/registry.hpp>
#include <entt/entity/sparse_set.hpp>
#include <entt/entity/storage.hpp>
#include <entt/locator/locator.hpp>
#include <entt/meta/context.hpp>
#include <entt/meta/meta.hpp>
#include <entt/meta/resolve.hpp>
#include <imgui.h>
#include "meta.hpp"

namespace entt {

namespace internal {

template<typename Entity, typename Allocator>
static void present_entity(const meta_ctx &ctx, const entt::basic_registry<Entity, Allocator> &registry, const Entity entt) {
    for([[maybe_unused]] auto [id, storage]: registry.storage()) {
        if(storage.contains(entt)) {
            if(auto type = entt::resolve(ctx, storage.type()); type) {
                if(const davey_data *info = type.custom(); ImGui::TreeNode(&storage.type(), "%s", info ? info->name : std::string{storage.type().name()}.c_str())) {
                    if(const auto obj = type.from_void(storage.value(entt)); obj) {
                        // TODO present element
                    }

                    ImGui::TreePop();
                }
            } else {
                const std::string name{storage.type().name()};
                ImGui::Text("%s", name.c_str());
            }
        }
    }
}

template<typename Entity, typename Allocator>
void storage_tab(const meta_ctx &ctx, const entt::basic_registry<Entity, Allocator> &registry) {
    for([[maybe_unused]] auto [id, storage]: registry.storage()) {
        if(auto type = entt::resolve(ctx, storage.type()); type) {
            if(const davey_data *info = type.custom(); ImGui::TreeNode(&storage.type(), "%s (%d)", info ? info->name : std::string{storage.type().name()}.c_str(), storage.size())) {
                if(const auto type = entt::resolve(ctx, storage.type()); type) {
                    for(auto entt: storage) {
                        ImGui::PushID(static_cast<int>(entt::to_entity(entt)));

                        if(ImGui::TreeNode(&storage.type(), "%d [%d/%d]", entt::to_integral(entt), entt::to_entity(entt), entt::to_version(entt))) {
                            if(const auto obj = type.from_void(storage.value(entt)); obj) {
                                // TODO present element
                            }

                            ImGui::TreePop();
                        }

                        ImGui::PopID();
                    }
                } else {
                    for(auto entt: storage) {
                        ImGui::Text("%d [%d/%d]", entt::to_integral(entt), entt::to_entity(entt), entt::to_version(entt));
                    }
                }

                ImGui::TreePop();
            }
        } else {
            const std::string name{storage.type().name()};
            ImGui::Text("%s (%d)", name.c_str(), storage.size());
        }
    }
}

template<typename Entity, typename Allocator>
void entity_tab(const meta_ctx &ctx, const entt::basic_registry<Entity, Allocator> &registry) {
    for(const auto [entt]: registry.storage<Entity>()->each()) {
        ImGui::PushID(static_cast<int>(entt::to_entity(entt)));

        if(ImGui::TreeNode(&entt::type_id<entt::entity>(), "%d [%d/%d]", entt::to_integral(entt), entt::to_entity(entt), entt::to_version(entt))) {
            present_entity(ctx, registry, entt);
            ImGui::TreePop();
        }

        ImGui::PopID();
    }
}

} // namespace internal

template<typename Entity, typename Allocator>
void davey(const meta_ctx &ctx, const entt::basic_sparse_set<Entity, Allocator> &set) {
    // TODO
}

template<typename Entity, typename Allocator>
void davey(const entt::basic_sparse_set<Entity, Allocator> &set) {
    // TODO
}

template<typename Entity, typename Allocator>
void davey(const meta_ctx &ctx, const entt::basic_registry<Entity, Allocator> &registry) {
    ImGui::Begin("Davey");
    ImGui::BeginTabBar("#tabs");

    if(ImGui::BeginTabItem("Storage")) {
        internal::storage_tab(ctx, registry);
        ImGui::EndTabItem();
    }

    if(ImGui::BeginTabItem("Entity")) {
        internal::entity_tab(ctx, registry);
        ImGui::EndTabItem();
    }

    ImGui::EndTabBar();
    ImGui::End();
}

template<typename Entity, typename Allocator>
void davey(const entt::basic_registry<Entity, Allocator> &registry) {
    davey(locator<meta_ctx>::value_or(), registry);
}

} // namespace entt

#endif

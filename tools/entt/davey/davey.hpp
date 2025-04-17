#ifndef ENTT_DAVEY_DAVEY_HPP
#define ENTT_DAVEY_DAVEY_HPP

#include <cstdint>
#include <ios>
#include <sstream>
#include <string>
#include <entt/entity/mixin.hpp>
#include <entt/entity/registry.hpp>
#include <entt/entity/sparse_set.hpp>
#include <entt/entity/storage.hpp>
#include <entt/locator/locator.hpp>
#include <entt/meta/container.hpp>
#include <entt/meta/context.hpp>
#include <entt/meta/meta.hpp>
#include <entt/meta/pointer.hpp>
#include <entt/meta/resolve.hpp>
#include <imgui.h>
#include "meta.hpp"

namespace entt {

namespace internal {

#define DAVEY_OR(elem) info ? info->name : std::string{elem.info().name()}.c_str()

template<typename Entity, typename OnEntity>
static void present_element(const entt::meta_any &obj, OnEntity on_entity) {
    for([[maybe_unused]] const auto [id, data]: obj.type().data()) {
        const auto elem = data.get(obj);
        const davey_data *info = data.custom();

        if(auto type = data.type(); type.info() == entt::type_id<const char *>()) {
            ImGui::Text("%s: %s", DAVEY_OR(type), elem.template cast<const char *>());
        } else if(type.info() == entt::type_id<std::string>()) {
            ImGui::Text("%s: %s", DAVEY_OR(type), elem.template cast<const std::string &>().c_str());
        } else if(type.info() == entt::type_id<Entity>()) {
            if(const auto entt = elem.template cast<Entity>(); entt == entt::null) {
                ImGui::Text("%s: %s", DAVEY_OR(type), "null");
            } else {
                on_entity(DAVEY_OR(type), elem.template cast<Entity>());
            }
        } else if(type.is_arithmetic()) {
            if(type.info() == entt::type_id<bool>()) {
                std::stringstream buffer{};
                buffer << std::boolalpha << elem.template cast<bool>();
                ImGui::Text("%s: %s", DAVEY_OR(type), buffer.str().c_str());
            } else if(type.info() == entt::type_id<char>()) {
                ImGui::Text("%s: %c", DAVEY_OR(type), elem.template cast<char>());
            } else if(type.is_integral()) {
                ImGui::Text("%s: %zu", DAVEY_OR(type), elem.template allow_cast<std::uint64_t>().template cast<std::uint64_t>());
            } else {
                ImGui::Text("%s: %f", DAVEY_OR(type), elem.template allow_cast<double>().template cast<double>());
            }
        } else if(type.is_pointer_like()) {
            if(ImGui::TreeNode(DAVEY_OR(type))) {
                present_element<Entity>(*obj, on_entity);
                ImGui::TreePop();
            }
        } else if(type.is_class() && entt::resolve(type.info())) {
            if(ImGui::TreeNode(DAVEY_OR(type))) {
                present_element<Entity>(elem, on_entity);
                ImGui::TreePop();
            }
        } else if(type.is_sequence_container()) {
            if(ImGui::TreeNode(DAVEY_OR(type))) {
                entt::meta_sequence_container view = elem.as_sequence_container();

                for(std::size_t pos{}, last = view.size(); pos < last; ++pos) {
                    ImGui::PushID(static_cast<int>(pos));

                    if(ImGui::TreeNode(DAVEY_OR(type), "%zu", pos)) {
                        present_element<Entity>(view[pos], on_entity);
                        ImGui::TreePop();
                    }

                    ImGui::PopID();
                }

                ImGui::TreePop();
            }
        } else if(type.is_associative_container()) {
            if(ImGui::TreeNode(DAVEY_OR(type))) {
                entt::meta_associative_container view = elem.as_associative_container();
                auto it = view.begin();

                for(std::size_t pos{}, last = view.size(); pos < last; ++pos, ++it) {
                    ImGui::PushID(static_cast<int>(pos));

                    if(ImGui::TreeNode(DAVEY_OR(type), "%zu", pos)) {
                        const auto [key, value] = *it;

                        if(ImGui::TreeNode("key")) {
                            present_element<Entity>(key, on_entity);
                            ImGui::TreePop();
                        }

                        if(ImGui::TreeNode("value")) {
                            present_element<Entity>(value, on_entity);
                            ImGui::TreePop();
                        }

                        ImGui::TreePop();
                    }

                    ImGui::PopID();
                }

                ImGui::TreePop();
            }
        } else {
            const std::string underlying_type{data.type().info().name()};
            ImGui::Text("%s: %s", DAVEY_OR(type), underlying_type.c_str());
        }
    }
}

template<typename Entity, typename Allocator>
static void present_entity(const meta_ctx &ctx, const entt::basic_registry<Entity, Allocator> &registry, const Entity entt) {
    for([[maybe_unused]] auto [id, storage]: registry.storage()) {
        if(storage.contains(entt)) {
            if(auto type = entt::resolve(ctx, storage.info()); type) {
                if(const davey_data *info = type.custom(); ImGui::TreeNode(&storage.info(), "%s", DAVEY_OR(storage))) {
                    if(const auto obj = type.from_void(storage.value(entt)); obj) {
                        present_element<Entity>(obj, [&ctx, &registry](const char *name, const entt::entity other) {
                            if(ImGui::TreeNode(name, "%s: %d [%d/%d]", name, entt::to_integral(other), entt::to_entity(other), entt::to_version(other))) {
                                present_entity<Entity>(ctx, registry, other);
                                ImGui::TreePop();
                            }
                        });
                    }

                    ImGui::TreePop();
                }
            } else {
                const std::string name{storage.info().name()};
                ImGui::Text("%s", name.c_str());
            }
        }
    }
}

template<typename Entity, typename Allocator>
void storage_tab(const meta_ctx &ctx, const entt::basic_registry<Entity, Allocator> &registry) {
    for([[maybe_unused]] auto [id, storage]: registry.storage()) {
        if(auto type = entt::resolve(ctx, storage.info()); type) {
            if(const davey_data *info = type.custom(); ImGui::TreeNode(&storage.info(), "%s (%zu)", DAVEY_OR(storage), storage.size())) {
                if(const auto type = entt::resolve(ctx, storage.info()); type) {
                    for(auto entt: storage) {
                        ImGui::PushID(static_cast<int>(entt::to_entity(entt)));

                        if(ImGui::TreeNode(&storage.info(), "%d [%d/%d]", entt::to_integral(entt), entt::to_entity(entt), entt::to_version(entt))) {
                            if(const auto obj = type.from_void(storage.value(entt)); obj) {
                                present_element<Entity>(obj, [](const char *name, const entt::entity entt) {
                                    ImGui::Text("%s: %d [%d/%d]", name, entt::to_integral(entt), entt::to_entity(entt), entt::to_version(entt));
                                });
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
            const std::string name{storage.info().name()};
            ImGui::Text("%s (%zu)", name.c_str(), storage.size());
        }
    }
}

template<typename Entity, typename Allocator>
void entity_tab(const meta_ctx &ctx, const entt::basic_registry<Entity, Allocator> &registry) {
    for(const auto [entt]: registry.template storage<Entity>()->each()) {
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

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

namespace entt {

namespace internal {

#define LABEL_OR(elem) label ? label : std::string{elem.info().name()}.data()

template<typename Entity, typename OnEntity>
static void present_element(const entt::meta_any &obj, OnEntity on_entity) {
    for([[maybe_unused]] const auto [id, data]: obj.type().data()) {
        const auto elem = data.get(obj);
        const char *label = data.name();

        if(auto type = data.type(); type.info() == entt::type_id<const char *>()) {
            ImGui::Text("%s: %s", LABEL_OR(type), elem.template cast<const char *>());
        } else if(type.info() == entt::type_id<std::string>()) {
            ImGui::Text("%s: %s", LABEL_OR(type), elem.template cast<const std::string &>().data());
        } else if(type.info() == entt::type_id<Entity>()) {
            if(const auto entt = elem.template cast<Entity>(); entt == entt::null) {
                ImGui::Text("%s: %s", LABEL_OR(type), "null");
            } else {
                on_entity(LABEL_OR(type), elem.template cast<Entity>());
            }
        } else if(type.is_enum()) {
            const char *as_string = nullptr;

            for(auto [id, curr]: type.data()) {
                if(curr.get({}) == elem) {
                    as_string = curr.name();
                    break;
                }
            }

            if(as_string) {
                ImGui::Text("%s: %s", LABEL_OR(type), as_string);
            } else {
                ImGui::Text("%s: %zu", LABEL_OR(type), elem.template allow_cast<std::uint64_t>().template cast<std::uint64_t>());
            }
        } else if(type.is_arithmetic()) {
            if(type.info() == entt::type_id<bool>()) {
                std::stringstream buffer{};
                buffer << std::boolalpha << elem.template cast<bool>();
                ImGui::Text("%s: %s", LABEL_OR(type), buffer.str().data());
            } else if(type.info() == entt::type_id<char>()) {
                ImGui::Text("%s: %c", LABEL_OR(type), elem.template cast<char>());
            } else if(type.is_integral()) {
                ImGui::Text("%s: %zu", LABEL_OR(type), elem.template allow_cast<std::uint64_t>().template cast<std::uint64_t>());
            } else {
                ImGui::Text("%s: %f", LABEL_OR(type), elem.template allow_cast<double>().template cast<double>());
            }
        } else if(type.is_pointer_like()) {
            if(auto deref = *obj; deref) {
                if(ImGui::TreeNode(LABEL_OR(type))) {
                    present_element<Entity>(*obj, on_entity);
                    ImGui::TreePop();
                }
            } else {
                ImGui::Text("%s: %s", LABEL_OR(type), "null");
            }
        } else if(type.is_sequence_container()) {
            if(ImGui::TreeNode(LABEL_OR(type))) {
                entt::meta_sequence_container view = elem.as_sequence_container();

                for(std::size_t pos{}, last = view.size(); pos < last; ++pos) {
                    ImGui::PushID(static_cast<int>(pos));

                    if(ImGui::TreeNode(LABEL_OR(type), "%zu", pos)) {
                        present_element<Entity>(view[pos], on_entity);
                        ImGui::TreePop();
                    }

                    ImGui::PopID();
                }

                ImGui::TreePop();
            }
        } else if(type.is_associative_container()) {
            if(ImGui::TreeNode(LABEL_OR(type))) {
                entt::meta_associative_container view = elem.as_associative_container();
                auto it = view.begin();

                for(std::size_t pos{}, last = view.size(); pos < last; ++pos, ++it) {
                    ImGui::PushID(static_cast<int>(pos));

                    if(ImGui::TreeNode(LABEL_OR(type), "%zu", pos)) {
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
        } else if(type.is_class()) {
            if(ImGui::TreeNode(LABEL_OR(type))) {
                present_element<Entity>(elem, on_entity);
                ImGui::TreePop();
            }
        } else {
            const std::string underlying_type{data.type().info().name()};
            ImGui::Text("%s: %s", LABEL_OR(type), underlying_type.data());
        }
    }
}

template<typename Entity, typename Allocator>
static void present_storage(const meta_ctx &ctx, const entt::basic_sparse_set<Entity, Allocator> &storage) {
    if(auto type = entt::resolve(ctx, storage.info()); type) {
        for(auto entt: storage) {
            ImGui::PushID(static_cast<int>(entt::to_entity(entt)));

            if(ImGui::TreeNode(&storage.info(), "%d [%d/%d]", entt::to_integral(entt), entt::to_entity(entt), entt::to_version(entt))) {
                if(const auto obj = type.from_void(storage.value(entt)); obj) {
                    present_element<typename std::decay_t<decltype(storage)>::entity_type>(obj, [](const char *name, const Entity entt) {
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
}

template<typename Entity, typename It>
static void present_entity(const meta_ctx &ctx, const Entity entt, const It from, const It to) {
    for(auto it = from; it != to; ++it) {
        if(const auto &storage = it->second; storage.contains(entt)) {
            if(auto type = entt::resolve(ctx, storage.info()); type) {
                if(const char *label = type.name(); ImGui::TreeNode(&storage.info(), "%s", LABEL_OR(storage))) {
                    if(const auto obj = type.from_void(storage.value(entt)); obj) {
                        present_element<Entity>(obj, [&ctx, from, to](const char *name, const Entity other) {
                            if(ImGui::TreeNode(name, "%s: %d [%d/%d]", name, entt::to_integral(other), entt::to_entity(other), entt::to_version(other))) {
                                present_entity<Entity>(ctx, other, from, to);
                                ImGui::TreePop();
                            }
                        });
                    }

                    ImGui::TreePop();
                }
            } else {
                const std::string name{storage.info().name()};
                ImGui::Text("%s", name.data());
            }
        }
    }
}

template<typename... Get, typename... Exclude, std::size_t... Index>
static void present_view(const meta_ctx &ctx, const entt::basic_view<get_t<Get...>, exclude_t<Exclude...>> &view, std::index_sequence<Index...>) {
    using view_type = entt::basic_view<get_t<Get...>, exclude_t<Exclude...>>;
    const std::array<const typename view_type::common_type *, sizeof...(Index)> range{view.template storage<Index>()...};

    for(auto tup: view.each()) {
        const auto entt = std::get<0>(tup);
        ImGui::PushID(static_cast<int>(entt::to_entity(entt)));

        if(ImGui::TreeNode(&entt::type_id<typename view_type::entity_type>(), "%d [%d/%d]", entt::to_integral(entt), entt::to_entity(entt), entt::to_version(entt))) {
            for(const auto *storage: range) {
                if(auto type = entt::resolve(ctx, storage->info()); type) {
                    if(const char *label = type.name(); ImGui::TreeNode(&storage->info(), "%s", LABEL_OR((*storage)))) {
                        if(const auto obj = type.from_void(storage->value(entt)); obj) {
                            present_element<typename view_type::entity_type>(obj, [](const char *name, const typename view_type::entity_type entt) {
                                ImGui::Text("%s: %d [%d/%d]", name, entt::to_integral(entt), entt::to_entity(entt), entt::to_version(entt));
                            });
                        }

                        ImGui::TreePop();
                    }
                } else {
                    const std::string name{storage->info().name()};
                    ImGui::Text("%s", name.data());
                }
            }

            ImGui::TreePop();
        }

        ImGui::PopID();
    }
}

} // namespace internal

template<typename Entity, typename Type, typename Allocator>
void davey(const meta_ctx &ctx, const entt::basic_storage<Entity, Type, Allocator> &storage) {
    internal::present_storage(ctx, storage);
}

template<typename Entity, typename Type, typename Allocator>
void davey(const entt::basic_storage<Entity, Type, Allocator> &storage) {
    davey(locator<meta_ctx>::value_or(), storage);
}

template<typename... Get, typename... Exclude>
void davey(const meta_ctx &ctx, const entt::basic_view<get_t<Get...>, exclude_t<Exclude...>> &view) {
    internal::present_view(ctx, view, std::index_sequence_for<Get...>{});
}

template<typename... Get, typename... Exclude>
void davey(const entt::basic_view<get_t<Get...>, exclude_t<Exclude...>> &view) {
    davey(locator<meta_ctx>::value_or(), view);
}

template<typename Entity, typename Allocator>
void davey(const meta_ctx &ctx, const entt::basic_registry<Entity, Allocator> &registry) {
    ImGui::BeginTabBar("#tabs");

    if(ImGui::BeginTabItem("Entity")) {
        for(const auto [entt]: registry.template storage<Entity>()->each()) {
            ImGui::PushID(static_cast<int>(entt::to_entity(entt)));

            if(ImGui::TreeNode(&entt::type_id<Entity>(), "%d [%d/%d]", entt::to_integral(entt), entt::to_entity(entt), entt::to_version(entt))) {
                const auto range = registry.storage();
                internal::present_entity(ctx, entt, range.begin(), range.end());
                ImGui::TreePop();
            }

            ImGui::PopID();
        }

        ImGui::EndTabItem();
    }

    if(ImGui::BeginTabItem("Storage")) {
        for([[maybe_unused]] auto [id, storage]: registry.storage()) {
            if(const char *label = entt::resolve(ctx, storage.info()).name(); ImGui::TreeNode(&storage.info(), "%s (%zu)", LABEL_OR(storage), storage.size())) {
                internal::present_storage(ctx, storage);
                ImGui::TreePop();
            }
        }

        ImGui::EndTabItem();
    }

    ImGui::EndTabBar();
}

template<typename Entity, typename Allocator>
void davey(const entt::basic_registry<Entity, Allocator> &registry) {
    davey(locator<meta_ctx>::value_or(), registry);
}

} // namespace entt

#endif

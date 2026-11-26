#ifndef ENTT_TOOLS_DAVEY_HPP
#define ENTT_TOOLS_DAVEY_HPP

#include "../config/config.h"
#include "../core/type_traits.hpp"
#include "../entity/mixin.hpp"
#include "../entity/registry.hpp"
#include "../entity/sparse_set.hpp"
#include "../entity/storage.hpp"
#include "../locator/locator.hpp"
#include "../meta/container.hpp"
#include "../meta/context.hpp"
#include "../meta/meta.hpp"
#include "../meta/pointer.hpp"
#include "../meta/resolve.hpp"
#include "../stl/cstdint.hpp"
#include "../stl/string.hpp"
#include "../stl/type_traits.hpp"
#include "../stl/utility.hpp"

#if __has_include(<imgui.h>)
#    include <imgui.h>
#endif

namespace entt {

/*! @cond ENTT_INTERNAL */
namespace internal {

constexpr const char *unknown = "??";

template<typename Entity>
static void present_element(meta_any obj, auto on_entity) {
    for([[maybe_unused]] const auto [id, data]: obj.type().data()) {
        auto elem = data.get(obj);
        const stl::string name = data.name().empty() ? stl::string{data.type().info().name()} : stl::string{data.name()};
        const char *const label = name.c_str();

        if(auto type = data.type(); type.info() == type_id<const char *>()) {
            ImGui::Text("%s: %s", label, elem.template cast<const char *>());
        } else if(type.info() == type_id<stl::string>()) {
            ImGui::Text("%s: %s", label, elem.template cast<const stl::string &>().data());
        } else if(type.info() == type_id<Entity>()) {
            if(const auto entt = elem.template cast<Entity>(); entt == null) {
                ImGui::Text("%s: %s", label, "null");
            } else {
                on_entity(label, entt);
            }
        } else if(type.is_enum()) {
            const char *as_string = nullptr;

            for(auto [id, curr]: type.data()) {
                if(curr.get({}) == elem) {
                    as_string = curr.name().data();
                    break;
                }
            }

            if(as_string) {
                ImGui::Text("%s: %s", label, as_string);
            } else {
                if(elem.template allow_cast<stl::uint64_t>()) {
                    ImGui::Text("%s: %zu", label, elem.template cast<stl::uint64_t>());
                } else {
                    ImGui::Text("%s: %s", label, unknown);
                }
            }
        } else if(type.is_arithmetic()) {
            if(type.info() == type_id<bool>()) {
                const auto value = elem.template cast<bool>();
                ImGui::Text("%s: %s", label, value ? "true" : "false");
            } else if(type.info() == type_id<char>()) {
                ImGui::Text("%s: %c", label, elem.template cast<char>());
            } else if(type.is_integral()) {
                if(elem.template allow_cast<stl::uint64_t>()) {
                    ImGui::Text("%s: %zu", label, elem.template cast<stl::uint64_t>());
                } else {
                    ImGui::Text("%s: %s", label, unknown);
                }
            } else {
                if(elem.template allow_cast<double>()) {
                    ImGui::Text("%s: %f", label, elem.template cast<double>());
                } else {
                    ImGui::Text("%s: %s", label, unknown);
                }
            }
        } else if(type.is_pointer_like()) {
            if(auto deref = *obj; deref) {
                if(ImGui::TreeNode(label)) {
                    present_element<Entity>(obj.as_ref(), on_entity);
                    ImGui::TreePop();
                }
            } else {
                ImGui::Text("%s: %s", label, "null");
            }
        } else if(type.is_sequence_container()) {
            if(ImGui::TreeNode(label)) {
                meta_sequence_container view = elem.as_sequence_container();

                for(stl::size_t pos{}, last = view.size(); pos < last; ++pos) {
                    ImGui::PushID(static_cast<int>(pos));

                    if(ImGui::TreeNode(label, "%zu", pos)) {
                        present_element<Entity>(view[pos], on_entity);
                        ImGui::TreePop();
                    }

                    ImGui::PopID();
                }

                ImGui::TreePop();
            }
        } else if(type.is_associative_container()) {
            if(ImGui::TreeNode(label)) {
                meta_associative_container view = elem.as_associative_container();
                auto it = view.begin();

                for(stl::size_t pos{}, last = view.size(); pos < last; ++pos, ++it) {
                    ImGui::PushID(static_cast<int>(pos));

                    if(ImGui::TreeNode(label, "%zu", pos)) {
                        const auto [key, value] = *it;

                        if(ImGui::TreeNode("key")) {
                            present_element<Entity>(key.as_ref(), on_entity);
                            ImGui::TreePop();
                        }

                        if(ImGui::TreeNode("value")) {
                            present_element<Entity>(value.as_ref(), on_entity);
                            ImGui::TreePop();
                        }

                        ImGui::TreePop();
                    }

                    ImGui::PopID();
                }

                ImGui::TreePop();
            }
        } else if(type.is_class()) {
            if(ImGui::TreeNode(label)) {
                present_element<Entity>(elem.as_ref(), on_entity);
                ImGui::TreePop();
            }
        } else {
            const stl::string underlying_type{data.type().info().name()};
            ImGui::Text("%s: %s", label, underlying_type.data());
        }
    }

    for([[maybe_unused]] const auto [id, base]: obj.type().base()) {
        auto other = stl::as_const(obj).allow_cast(base.type());
        present_element<Entity>(other.as_ref(), on_entity);
    }
}

static void present_storage(const meta_ctx &ctx, auto &storage) {
    using entity_type = typename stl::remove_cvref_t<decltype(storage)>::entity_type;

    if(auto type = resolve(ctx, storage.info()); type) {
        for(auto entt: storage) {
            ImGui::PushID(static_cast<int>(to_entity(entt)));

            if(ImGui::TreeNode(&storage.info(), "%d [%d/%d]", to_integral(entt), to_entity(entt), to_version(entt))) {
                if(auto obj = type.from_void(storage.value(entt)); obj) {
                    present_element<entity_type>(obj.as_ref(), [](const char *name, const entity_type entt) {
                        ImGui::Text("%s: %d [%d/%d]", name, to_integral(entt), to_entity(entt), to_version(entt));
                    });
                }

                ImGui::TreePop();
            }

            ImGui::PopID();
        }
    } else {
        for(auto entt: storage) {
            ImGui::Text("%d [%d/%d]", to_integral(entt), to_entity(entt), to_version(entt));
        }
    }
}

static void present_entity(const meta_ctx &ctx, const auto entt, auto from, auto to) {
    using entity_type = decltype(entt);

    for(auto it = from; it != to; ++it) {
        if(auto &storage = it->second; storage.contains(entt)) {
            if(auto type = resolve(ctx, storage.info()); type) {
                const stl::string name = type.name().empty() ? stl::string{storage.info().name()} : stl::string{type.name()};
                const char *const label = name.c_str();

                if(ImGui::TreeNode(&storage.info(), "%s", label)) {
                    if(auto obj = type.from_void(storage.value(entt)); obj) {
                        present_element<entity_type>(obj.as_ref(), [&ctx, from, to](const char *name, const entity_type other) {
                            if(ImGui::TreeNode(name, "%s: %d [%d/%d]", name, to_integral(other), to_entity(other), to_version(other))) {
                                present_entity<entity_type>(ctx, other, from, to);
                                ImGui::TreePop();
                            }
                        });
                    }

                    ImGui::TreePop();
                }
            } else {
                const stl::string name{storage.info().name()};
                ImGui::Text("%s", name.data());
            }
        }
    }
}

template<stl::size_t... Index>
static void present_view(const meta_ctx &ctx, auto view, stl::index_sequence<Index...>) {
    using view_type = decltype(view);

    const auto present_storage_element = [&ctx](auto *storage, const auto entt) {
        if(auto type = resolve(ctx, storage->info()); type) {
            const stl::string name = type.name().empty() ? stl::string{storage->info().name()} : stl::string{type.name()};
            const char *const label = name.c_str();

            if(ImGui::TreeNode(&storage->info(), "%s", label)) {
                if(const auto obj = type.from_void(storage->value(entt)); obj) {
                    present_element<typename view_type::entity_type>(obj.as_ref(), [](const char *name, const view_type::entity_type entt) {
                        ImGui::Text("%s: %d [%d/%d]", name, to_integral(entt), to_entity(entt), to_version(entt));
                    });
                }

                ImGui::TreePop();
            }
        } else {
            const stl::string name{storage->info().name()};
            ImGui::Text("%s", name.data());
        }
    };

    for(auto tup: view.each()) {
        const auto entt = stl::get<0>(tup);
        ImGui::PushID(static_cast<int>(to_entity(entt)));

        if(ImGui::TreeNode(&type_id<typename view_type::entity_type>(), "%d [%d/%d]", to_integral(entt), to_entity(entt), to_version(entt))) {
            (present_storage_element(view.template storage<Index>(), entt), ...);
            ImGui::TreePop();
        }

        ImGui::PopID();
    }
}

} // namespace internal
/*! @endcond */

/**
 * @brief ImGui-based introspection tool for view types.
 * @tparam Get Types of storage iterated by the view.
 * @tparam Exclude Types of storage used to filter the view.
 * @param ctx The context from which to search for meta types.
 * @param view An instance of the view type.
 */
template<typename... Get, typename... Exclude>
void davey(const meta_ctx &ctx, basic_view<get_t<Get...>, exclude_t<Exclude...>> view) {
    internal::present_view(ctx, view, stl::index_sequence_for<Get...>{});
}

/**
 * @brief ImGui-based introspection tool for storage types.
 * @param ctx The context from which to search for meta types.
 * @param storage An instance of the storage type.
 */
void davey(const meta_ctx &ctx, storage_like auto &storage) {
    using base_type = typename stl::remove_cvref_t<decltype(storage)>::base_type;
    internal::present_storage(ctx, static_cast<constness_as_t<base_type, stl::remove_reference_t<decltype(storage)>> &>(storage));
}

/**
 * @brief ImGui-based introspection tool for storage types.
 * @param storage An instance of the storage type.
 */
void davey(storage_like auto &storage) {
    davey(locator<meta_ctx>::value_or(), storage);
}

/**
 * @brief ImGui-based introspection tool for view types.
 * @tparam Get Types of storage iterated by the view.
 * @tparam Exclude Types of storage used to filter the view.
 * @param view An instance of the view type.
 */
template<typename... Get, typename... Exclude>
void davey(basic_view<get_t<Get...>, exclude_t<Exclude...>> view) {
    davey(locator<meta_ctx>::value_or(), view);
}

/**
 * @brief ImGui-based introspection tool for registry types.
 * @tparam Entity Registry entity type.
 * @tparam Allocator Registry allocator type.
 * @param ctx The context from which to search for meta types.
 * @param registry An instance of the registry type.
 */
void davey(const meta_ctx &ctx, registry_like auto &registry) {
    constexpr bool is_const_registry = stl::is_const_v<stl::remove_reference_t<decltype(registry)>>;
    using registry_type = stl::remove_cvref_t<decltype(registry)>;

    if constexpr(!is_const_registry) {
        if(ImGui::Button("Add Entity")) {
            [[maybe_unused]] auto entt = registry.create();
        }

        ImGui::Separator();
    }

    if(ImGui::BeginTabBar("#tabs")) {
        if(ImGui::BeginTabItem("Entity")) {
            using storage_type = typename registry_type::template storage_for_type<typename registry_type::entity_type>;
            using iterable_type = decltype(stl::declval<storage_type>().each());

            iterable_type iterable{};

            if constexpr(is_const_registry) {
                iterable = registry.template storage<typename registry_type::entity_type>()->each();
            } else {
                iterable = registry.template storage<typename registry_type::entity_type>().each();
            }

            for(const auto [entt]: iterable) {
                ImGui::PushID(static_cast<int>(to_entity(entt)));

                if(ImGui::TreeNode(&type_id<typename registry_type::entity_type>(), "%d [%d/%d]", to_integral(entt), to_entity(entt), to_version(entt))) {
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
                const auto type = resolve(ctx, storage.info());
                const stl::string name = type.name().empty() ? stl::string{storage.info().name()} : stl::string{type.name()};
                const char *const label = name.c_str();

                if(ImGui::TreeNode(&storage.info(), "%s (%zu)", label, storage.size())) {
                    internal::present_storage(ctx, storage);
                    ImGui::TreePop();
                }
            }

            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }
}

/**
 * @brief ImGui-based introspection tool for registry types.
 * @tparam Entity Registry entity type.
 * @tparam Allocator Registry allocator type.
 * @param registry An instance of the registry type.
 */
void davey(registry_like auto &registry) {
    davey(locator<meta_ctx>::value_or(), registry);
}

} // namespace entt

#endif

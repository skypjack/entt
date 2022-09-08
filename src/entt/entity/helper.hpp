#ifndef ENTT_ENTITY_HELPER_HPP
#define ENTT_ENTITY_HELPER_HPP

#include <memory>
#include <type_traits>
#include "../core/fwd.hpp"
#include "../core/type_traits.hpp"
#include "../signal/delegate.hpp"
#include "component.hpp"
#include "fwd.hpp"
#include "group.hpp"
#include "view.hpp"

namespace entt {

/**
 * @brief Converts a registry to a view.
 * @tparam Registry Basic registry type.
 */
template<typename Registry>
class as_view {
    template<typename... Get, typename... Exclude>
    auto dispatch(get_t<Get...>, exclude_t<Exclude...>) const {
        return reg.template view<constness_as_t<typename Get::value_type, Get>...>(exclude_t<constness_as_t<typename Exclude::value_type, Exclude>...>{});
    }

public:
    /*! @brief Type of registry to convert. */
    using registry_type = Registry;
    /*! @brief Underlying entity identifier. */
    using entity_type = std::remove_const_t<typename registry_type::entity_type>;

    /**
     * @brief Constructs a converter for a given registry.
     * @param source A valid reference to a registry.
     */
    as_view(registry_type &source) noexcept
        : reg{source} {}

    /**
     * @brief Conversion function from a registry to a view.
     * @tparam Get Type of storage used to construct the view.
     * @tparam Exclude Types of storage used to filter the view.
     * @return A newly created view.
     */
    template<typename Get, typename Exclude>
    operator basic_view<Get, Exclude>() const {
        return dispatch(Get{}, Exclude{});
    }

private:
    registry_type &reg;
};

/**
 * @brief Converts a registry to a group.
 * @tparam Registry Basic registry type.
 */
template<typename Registry>
class as_group {
    template<typename... Owned, typename... Get, typename... Exclude>
    auto dispatch(owned_t<Owned...>, get_t<Get...>, exclude_t<Exclude...>) const {
        if constexpr(std::is_const_v<registry_type>) {
            return reg.template group_if_exists<typename Owned::value_type...>(get_t<typename Get::value_type...>{}, exclude_t<typename Exclude::value_type...>{});
        } else {
            return reg.template group<constness_as_t<typename Owned::value_type, Owned>...>(get_t<constness_as_t<typename Get::value_type, Get>...>{}, exclude_t<constness_as_t<typename Exclude::value_type, Exclude>...>{});
        }
    }

public:
    /*! @brief Type of registry to convert. */
    using registry_type = Registry;
    /*! @brief Underlying entity identifier. */
    using entity_type = std::remove_const_t<typename registry_type::entity_type>;

    /**
     * @brief Constructs a converter for a given registry.
     * @param source A valid reference to a registry.
     */
    as_group(registry_type &source) noexcept
        : reg{source} {}

    /**
     * @brief Conversion function from a registry to a group.
     * @tparam Owned Types of _owned_ by the group.
     * @tparam Get Types of storage _observed_ by the group.
     * @tparam Exclude Types of storage used to filter the group.
     * @return A newly created group.
     */
    template<typename Owned, typename Get, typename Exclude>
    operator basic_group<Owned, Get, Exclude>() const {
        return dispatch(Owned{}, Get{}, Exclude{});
    }

private:
    registry_type &reg;
};

/**
 * @brief Helper to create a listener that directly invokes a member function.
 * @tparam Member Member function to invoke on a component of the given type.
 * @tparam Registry Basic registry type.
 * @param reg A registry that contains the given entity and its components.
 * @param entt Entity from which to get the component.
 */
template<auto Member, typename Registry = std::decay_t<nth_argument_t<0u, Member>>>
void invoke(Registry &reg, const typename Registry::entity_type entt) {
    static_assert(std::is_member_function_pointer_v<decltype(Member)>, "Invalid pointer to non-static member function");
    delegate<void(Registry &, const typename Registry::entity_type)> func;
    func.template connect<Member>(reg.template get<member_class_t<decltype(Member)>>(entt));
    func(reg, entt);
}

/**
 * @brief Returns the entity associated with a given component.
 *
 * @warning
 * Currently, this function only works correctly with the default pool as it
 * makes assumptions about how the components are laid out.
 *
 * @tparam Registry Basic registry type.
 * @tparam Component Type of component.
 * @param reg A registry that contains the given entity and its components.
 * @param instance A valid component instance.
 * @return The entity associated with the given component.
 */
template<typename Registry, typename Component>
typename Registry::entity_type to_entity(const Registry &reg, const Component &instance) {
    const auto &storage = reg.template storage<Component>();
    const typename Registry::base_type &base = storage;
    const auto *addr = std::addressof(instance);

    for(auto it = base.rbegin(), last = base.rend(); it < last; it += component_traits<Component>::page_size) {
        if(const auto dist = (addr - std::addressof(storage.get(*it))); dist >= 0 && dist < static_cast<decltype(dist)>(component_traits<Component>::page_size)) {
            return *(it + dist);
        }
    }

    return null;
}

} // namespace entt

#endif

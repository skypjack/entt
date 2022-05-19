#ifndef ENTT_ENTITY_HELPER_HPP
#define ENTT_ENTITY_HELPER_HPP

#include <type_traits>
#include "../core/fwd.hpp"
#include "../core/type_traits.hpp"
#include "../signal/delegate.hpp"
#include "component.hpp"
#include "fwd.hpp"
#include "registry.hpp"

namespace entt {

/**
 * @brief Converts a registry to a view.
 * @tparam Registry Basic registry type.
 */
template<typename Registry>
struct as_view {
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
     * @tparam Exclude Types of components used to filter the view.
     * @tparam Component Type of components used to construct the view.
     * @return A newly created view.
     */
    template<typename Exclude, typename... Component>
    operator basic_view<entity_type, get_t<Component...>, Exclude>() const {
        return reg.template view<Component...>(Exclude{});
    }

private:
    registry_type &reg;
};

/**
 * @brief Converts a registry to a group.
 * @tparam Registry Basic registry type.
 */
template<typename Registry>
struct as_group {
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
     * @tparam Get Types of components observed by the group.
     * @tparam Exclude Types of components used to filter the group.
     * @tparam Owned Types of components owned by the group.
     * @return A newly created group.
     */
    template<typename Get, typename Exclude, typename... Owned>
    operator basic_group<entity_type, owned_t<Owned...>, Get, Exclude>() const {
        if constexpr(std::is_const_v<registry_type>) {
            return reg.template group_if_exists<Owned...>(Get{}, Exclude{});
        } else {
            return reg.template group<Owned...>(Get{}, Exclude{});
        }
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
template<auto Member, typename Registry = registry>
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

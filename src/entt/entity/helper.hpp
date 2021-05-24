#ifndef ENTT_ENTITY_HELPER_HPP
#define ENTT_ENTITY_HELPER_HPP


#include <type_traits>
#include "../config/config.h"
#include "../core/fwd.hpp"
#include "../core/type_traits.hpp"
#include "../signal/delegate.hpp"
#include "registry.hpp"
#include "fwd.hpp"


namespace entt {


/**
 * @brief Converts a registry to a view.
 * @tparam Entity A valid entity type (see entt_traits for more details).
 */
template<typename Entity>
struct as_view {
    /*! @brief Underlying entity identifier. */
    using entity_type = std::remove_const_t<Entity>;
    /*! @brief Type of registry to convert. */
    using registry_type = constness_as_t<basic_registry<entity_type>, Entity>;

    /**
     * @brief Constructs a converter for a given registry.
     * @param source A valid reference to a registry.
     */
    as_view(registry_type &source) ENTT_NOEXCEPT: reg{source} {}

    /**
     * @brief Conversion function from a registry to a view.
     * @tparam Exclude Types of components used to filter the view.
     * @tparam Component Type of components used to construct the view.
     * @return A newly created view.
     */
    template<typename Exclude, typename... Component>
    operator basic_view<entity_type, Exclude, Component...>() const {
        return reg.template view<Component...>(Exclude{});
    }

private:
    registry_type &reg;
};


/**
 * @brief Deduction guide.
 * @tparam Entity A valid entity type (see entt_traits for more details).
 */
template<typename Entity>
as_view(basic_registry<Entity> &) -> as_view<Entity>;


/**
 * @brief Deduction guide.
 * @tparam Entity A valid entity type (see entt_traits for more details).
 */
template<typename Entity>
as_view(const basic_registry<Entity> &) -> as_view<const Entity>;


/**
 * @brief Converts a registry to a group.
 * @tparam Entity A valid entity type (see entt_traits for more details).
 */
template<typename Entity>
struct as_group {
    /*! @brief Underlying entity identifier. */
    using entity_type = std::remove_const_t<Entity>;
    /*! @brief Type of registry to convert. */
    using registry_type = constness_as_t<basic_registry<entity_type>, Entity>;

    /**
     * @brief Constructs a converter for a given registry.
     * @param source A valid reference to a registry.
     */
    as_group(registry_type &source) ENTT_NOEXCEPT: reg{source} {}

    /**
     * @brief Conversion function from a registry to a group.
     * @tparam Exclude Types of components used to filter the group.
     * @tparam Get Types of components observed by the group.
     * @tparam Owned Types of components owned by the group.
     * @return A newly created group.
     */
    template<typename Exclude, typename Get, typename... Owned>
    operator basic_group<entity_type, Exclude, Get, Owned...>() const {
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
 * @brief Deduction guide.
 * @tparam Entity A valid entity type (see entt_traits for more details).
 */
template<typename Entity>
as_group(basic_registry<Entity> &) -> as_group<Entity>;


/**
 * @brief Deduction guide.
 * @tparam Entity A valid entity type (see entt_traits for more details).
 */
template<typename Entity>
as_group(const basic_registry<Entity> &) -> as_group<const Entity>;



/**
 * @brief Helper to create a listener that directly invokes a member function.
 * @tparam Member Member function to invoke on a component of the given type.
 * @tparam Entity A valid entity type (see entt_traits for more details).
 * @param reg A registry that contains the given entity and its components.
 * @param entt Entity from which to get the component.
 */
template<auto Member, typename Entity = entity>
void invoke(basic_registry<Entity> &reg, const Entity entt) {
    static_assert(std::is_member_function_pointer_v<decltype(Member)>, "Invalid pointer to non-static member function");
    delegate<void(basic_registry<Entity> &, const Entity)> func;
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
 * @tparam Entity A valid entity type (see entt_traits for more details).
 * @tparam Component Type of component.
 * @param reg A registry that contains the given entity and its components.
 * @param instance A valid component instance.
 * @return The entity associated with the given component.
 */
template<typename Entity, typename Component>
Entity to_entity(const basic_registry<Entity> &reg, const Component &instance) {
    const auto view = reg.template view<const Component>();
    const auto *addr = std::addressof(instance);

    for(auto it = view.rbegin(), last = view.rend(); it < last; it += ENTT_PACKED_PAGE) {
        if(const auto dist = (addr - std::addressof(view.template get<const Component>(*it))); dist >= 0 && dist < ENTT_PACKED_PAGE) {
            return *(it + dist);
        }
    }

    return entt::null;
}


}


#endif

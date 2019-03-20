#ifndef ENTT_ENTITY_HELPER_HPP
#define ENTT_ENTITY_HELPER_HPP


#include <type_traits>
#include "../config/config.h"
#include "../core/hashed_string.hpp"
#include "../signal/sigh.hpp"
#include "registry.hpp"


namespace entt {


/**
 * @brief Converts a registry to a view.
 * @tparam Const Constness of the accepted registry.
 * @tparam Entity A valid entity type (see entt_traits for more details).
 */
template<bool Const, typename Entity>
struct as_view {
    /*! @brief Type of registry to convert. */
    using registry_type = std::conditional_t<Const, const entt::basic_registry<Entity>, entt::basic_registry<Entity>>;

    /**
     * @brief Constructs a converter for a given registry.
     * @param source A valid reference to a registry.
     */
    as_view(registry_type &source) ENTT_NOEXCEPT: reg{source} {}

    /**
     * @brief Conversion function from a registry to a view.
     * @tparam Component Type of components used to construct the view.
     * @return A newly created view.
     */
    template<typename... Component>
    inline operator entt::basic_view<Entity, Component...>() const {
        return reg.template view<Component...>();
    }

private:
    registry_type &reg;
};


/**
 * @brief Deduction guideline.
 *
 * It allows to deduce the constness of a registry directly from the instance
 * provided to the constructor.
 *
 * @tparam Entity A valid entity type (see entt_traits for more details).
 */
template<typename Entity>
as_view(basic_registry<Entity> &) ENTT_NOEXCEPT -> as_view<false, Entity>;


/*! @copydoc as_view */
template<typename Entity>
as_view(const basic_registry<Entity> &) ENTT_NOEXCEPT -> as_view<true, Entity>;


/**
 * @brief Converts a registry to a group.
 * @tparam Const Constness of the accepted registry.
 * @tparam Entity A valid entity type (see entt_traits for more details).
 */
template<bool Const, typename Entity>
struct as_group {
    /*! @brief Type of registry to convert. */
    using registry_type = std::conditional_t<Const, const entt::basic_registry<Entity>, entt::basic_registry<Entity>>;

    /**
     * @brief Constructs a converter for a given registry.
     * @param source A valid reference to a registry.
     */
    as_group(registry_type &source) ENTT_NOEXCEPT: reg{source} {}

    /**
     * @brief Conversion function from a registry to a group.
     *
     * @note
     * Unfortunately, only full owning groups are supported because of an issue
     * with msvc that doesn't manage to correctly deduce types.
     *
     * @tparam Owned Types of components owned by the group.
     * @return A newly created group.
     */
    template<typename... Owned>
    inline operator entt::basic_group<Entity, get_t<>, Owned...>() const {
        return reg.template group<Owned...>();
    }

private:
    registry_type &reg;
};


/**
 * @brief Deduction guideline.
 *
 * It allows to deduce the constness of a registry directly from the instance
 * provided to the constructor.
 *
 * @tparam Entity A valid entity type (see entt_traits for more details).
 */
template<typename Entity>
as_group(basic_registry<Entity> &) ENTT_NOEXCEPT -> as_group<false, Entity>;


/*! @copydoc as_group */
template<typename Entity>
as_group(const basic_registry<Entity> &) ENTT_NOEXCEPT -> as_group<true, Entity>;


/**
 * @brief Dependency function prototype.
 *
 * A _dependency function_ is a built-in listener to use to automatically assign
 * components to an entity when a type has a dependency on some other types.
 *
 * This is a prototype function to use to create dependencies.<br/>
 * It isn't intended for direct use, although nothing forbids using it freely.
 *
 * @tparam Entity A valid entity type (see entt_traits for more details).
 * @tparam Component Types of components to assign to an entity if triggered.
 * @param reg A valid reference to a registry.
 * @param entt A valid entity identifier.
 */
template<typename Entity, typename... Component>
void dependency(basic_registry<Entity> &reg, const Entity entt) {
    ((reg.template has<Component>(entt) ? void() : (reg.template assign<Component>(entt), void())), ...);
}


/**
 * @brief Connects a dependency function to the given sink.
 *
 * A _dependency function_ is a built-in listener to use to automatically assign
 * components to an entity when a type has a dependency on some other types.
 *
 * The following adds components `a_type` and `another_type` whenever `my_type`
 * is assigned to an entity:
 * @code{.cpp}
 * entt::registry registry;
 * entt::connect<a_type, another_type>(registry.construction<my_type>());
 * @endcode
 *
 * @tparam Dependency Types of components to assign to an entity if triggered.
 * @tparam Entity A valid entity type (see entt_traits for more details).
 * @param sink A sink object properly initialized.
 */
template<typename... Dependency, typename Entity>
inline void connect(sink<void(basic_registry<Entity> &, const Entity)> sink) {
    sink.template connect<dependency<Entity, Dependency...>>();
}


/**
 * @brief Disconnects a dependency function from the given sink.
 *
 * A _dependency function_ is a built-in listener to use to automatically assign
 * components to an entity when a type has a dependency on some other types.
 *
 * The following breaks the dependency between the component `my_type` and the
 * components `a_type` and `another_type`:
 * @code{.cpp}
 * entt::registry registry;
 * entt::disconnect<a_type, another_type>(registry.construction<my_type>());
 * @endcode
 *
 * @tparam Dependency Types of components used to create the dependency.
 * @tparam Entity A valid entity type (see entt_traits for more details).
 * @param sink A sink object properly initialized.
 */
template<typename... Dependency, typename Entity>
inline void disconnect(sink<void(basic_registry<Entity> &, const Entity)> sink) {
    sink.template disconnect<dependency<Entity, Dependency...>>();
}


/**
 * @brief Alias template to ease the assignment of tags to entities.
 *
 * If used in combination with hashed strings, it simplifies the assignment of
 * tags to entities and the use of tags in general where a type would be
 * required otherwise.<br/>
 * As an example and where the user defined literal for hashed strings hasn't
 * been changed:
 * @code{.cpp}
 * entt::registry registry;
 * registry.assign<entt::tag<"enemy"_hs>>(entity);
 * @endcode
 *
 * @note
 * Tags are empty components and therefore candidates for the empty component
 * optimization.
 *
 * @tparam Value The numeric representation of an instance of hashed string.
 */
template<typename hashed_string::hash_type Value>
using tag = std::integral_constant<typename hashed_string::hash_type, Value>;


}


#endif // ENTT_ENTITY_HELPER_HPP

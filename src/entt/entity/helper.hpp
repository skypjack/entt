#ifndef ENTT_ENTITY_HELPER_HPP
#define ENTT_ENTITY_HELPER_HPP


#include <type_traits>
#include "../core/hashed_string.hpp"
#include "../signal/sigh.hpp"
#include "registry.hpp"
#include "utility.hpp"


namespace entt {


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
 * @param registry A valid reference to a registry.
 * @param entity A valid entity identifier.
 */
template<typename Entity, typename... Component>
void dependency(Registry<Entity> &registry, const Entity entity) {
    using accumulator_type = int[];
    accumulator_type accumulator = { ((registry.template has<Component>(entity) ? void() : (registry.template assign<Component>(entity), void())), 0)... };
    (void)accumulator;
}


/**
 * @brief Connects a dependency function to the given sink.
 *
 * A _dependency function_ is a built-in listener to use to automatically assign
 * components to an entity when a type has a dependency on some other types.
 *
 * The following adds components `AType` and `AnotherType` whenever `MyType` is
 * assigned to an entity:
 * @code{.cpp}
 * entt::DefaultRegistry registry;
 * entt::connect<AType, AnotherType>(registry.construction<MyType>());
 * @endcode
 *
 * @tparam Dependency Types of components to assign to an entity if triggered.
 * @tparam Entity A valid entity type (see entt_traits for more details).
 * @param sink A sink object properly initialized.
 */
template<typename... Dependency, typename Entity>
inline void connect(Sink<void(Registry<Entity> &, const Entity)> sink) {
    sink.template connect<dependency<Entity, Dependency...>>();
}


/**
 * @brief Disconnects a dependency function from the given sink.
 *
 * A _dependency function_ is a built-in listener to use to automatically assign
 * components to an entity when a type has a dependency on some other types.
 *
 * The following breaks the dependency between the component `MyType` and the
 * components `AType` and `AnotherType`:
 * @code{.cpp}
 * entt::DefaultRegistry registry;
 * entt::disconnect<AType, AnotherType>(registry.construction<MyType>());
 * @endcode
 *
 * @tparam Dependency Types of components used to create the dependency.
 * @tparam Entity A valid entity type (see entt_traits for more details).
 * @param sink A sink object properly initialized.
 */
template<typename... Dependency, typename Entity>
inline void disconnect(Sink<void(Registry<Entity> &, const Entity)> sink) {
    sink.template disconnect<dependency<Entity, Dependency...>>();
}


/**
 * @brief Alias template to ease the assignment of labels to entities.
 *
 * If used in combination with hashed strings, it simplifies the assignment of
 * labels to entities and the use of labels in general where a type would be
 * required otherwise.<br/>
 * As an example and where the user defined literal for hashed strings hasn't
 * been changed:
 * @code{.cpp}
 * entt::DefaultRegistry registry;
 * registry.assign<entt::label<"enemy"_hs>>(entity);
 * @endcode
 *
 * @tparam Value The numeric representation of an instance of hashed string.
 */
template<typename HashedString::hash_type Value>
using label = std::integral_constant<typename HashedString::hash_type, Value>;


}


#endif // ENTT_ENTITY_HELPER_HPP

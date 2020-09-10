#ifndef ENTT_ENTITY_POOL_HPP
#define ENTT_ENTITY_POOL_HPP


#include <iterator>
#include <type_traits>
#include <utility>
#include "../config/config.h"
#include "../core/type_traits.hpp"
#include "../signal/sigh.hpp"
#include "storage.hpp"


namespace entt {


/**
 * @brief Default pool implementation.
 * @tparam Entity A valid entity type (see entt_traits for more details).
 * @tparam Type Type of objects assigned to the entities.
 */
template<typename Entity, typename Type>
struct default_pool final: storage<Entity, Type> {
    static_assert(std::is_same_v<Type, std::decay_t<Type>>, "Invalid object type");

    /*! @brief Type of the objects associated with the entities. */
    using object_type = Type;
    /*! @brief Underlying entity identifier. */
    using entity_type = Entity;

    /**
    * @brief Returns a sink object.
    *
    * The sink returned by this function can be used to receive notifications
    * whenever a new instance is created and assigned to an entity.<br/>
    * The function type for a listener is equivalent to:
    *
    * @code{.cpp}
    * void(Entity);
    * @endcode
    *
    * Listeners are invoked **after** the object has been assigned to the
    * entity.
    *
    * @sa sink
    *
    * @return A temporary sink object.
    */
    [[nodiscard]] auto on_construct() ENTT_NOEXCEPT {
        return sink{construction};
    }

    /**
    * @brief Returns a sink object for the given type.
    *
    * The sink returned by this function can be used to receive notifications
    * whenever an instance is explicitly updated.<br/>
    * The function type for a listener is equivalent to:
    *
    * @code{.cpp}
    * void(Entity);
    * @endcode
    *
    * Listeners are invoked **after** the object has been updated.
    *
    * @sa sink
    *
    * @return A temporary sink object.
    */
    [[nodiscard]] auto on_update() ENTT_NOEXCEPT {
        return sink{update};
    }

    /**
    * @brief Returns a sink object for the given type.
    *
    * The sink returned by this function can be used to receive notifications
    * whenever an instance is removed from an entity and thus destroyed.<br/>
    * The function type for a listener is equivalent to:
    *
    * @code{.cpp}
    * void(Entity);
    * @endcode
    *
    * Listeners are invoked **before** the object has been removed from the
    * entity.
    *
    * @sa sink
    *
    * @return A temporary sink object.
    */
    [[nodiscard]] auto on_destroy() ENTT_NOEXCEPT {
        return sink{destruction};
    }

    /**
    * @brief Assigns an entity to a pool.
    *
    * A new object is created and initialized with the arguments provided (the
    * object type must have a proper constructor or be of aggregate type). Then
    * the instance is assigned to the given entity.
    *
    * @warning
    * Attempting to use an invalid entity or to assign an entity that already
    * belongs to the pool results in undefined behavior.<br/>
    * An assertion will abort the execution at runtime in debug mode in case of
    * invalid entity or if the entity already belongs to the pool.
    *
    * @tparam Args Types of arguments to use to construct the object.
    * @param entity A valid entity identifier.
    * @param args Parameters to use to initialize the object.
    * @return A reference to the newly created object.
    */
    template<typename... Args>
    decltype(auto) emplace(const entity_type entt, Args &&... args) {
        storage<entity_type, Type>::emplace(entt, std::forward<Args>(args)...);
        construction.publish(entt);

        if constexpr(!is_eto_eligible_v<object_type>) {
            return this->get(entt);
        }
    }

    /**
    * @brief Assigns multiple entities to a pool.
    *
    * @sa emplace
    *
    * @tparam It Type of input iterator.
    * @param first An iterator to the first element of the range of entities.
    * @param last An iterator past the last element of the range of entities.
    * @param value An instance of the type to assign.
    */
    template<typename It, typename... Args>
    void insert(It first, It last, Args &&... args) {
        storage<entity_type, object_type>::insert(first, last, std::forward<Args>(args)...);

        if(!construction.empty()) {
            for(; first != last; ++first) {
                construction.publish(*first);
            }
        }
    }

    /**
    * @brief Removes an entity from a pool.
    *
    * @warning
    * Attempting to use an invalid entity or to remove an entity that doesn't
    * belong to the pool results in undefined behavior.<br/>
    * An assertion will abort the execution at runtime in debug mode in case of
    * invalid entity or if the entity doesn't belong to the pool.
    *
    * @param entity A valid entity identifier.
    */
    void erase(const entity_type entt) override {
        destruction.publish(entt);
        storage<entity_type, object_type>::erase(entt);
    }

    /**
    * @brief Removes multiple entities from a pool.
    *
    * @see remove
    *
    * @tparam It Type of input iterator.
    * @param first An iterator to the first element of the range of entities.
    * @param last An iterator past the last element of the range of entities.
    */
    template<typename It>
    void erase(It first, It last) {
        if(std::distance(first, last) == std::distance(this->begin(), this->end())) {
            if(!destruction.empty()) {
                for(; first != last; ++first) {
                    destruction.publish(*first);
                }
            }

            this->clear();
        } else {
            for(; first != last; ++first) {
                this->erase(*first);
            }
        }
    }

    /**
    * @brief Patches the given instance for an entity.
    *
    * The signature of the functions should be equivalent to the following:
    *
    * @code{.cpp}
    * void(Type &);
    * @endcode
    *
    * @note
    * Empty types aren't explicitly instantiated and therefore they are never
    * returned. However, this function can be used to trigger an update signal
    * for them.
    *
    * @warning
    * Attempting to use an invalid entity or to patch an object of an entity
    * that doesn't belong to the pool results in undefined behavior.<br/>
    * An assertion will abort the execution at runtime in debug mode in case of
    * invalid entity or if the entity doesn't belong to the pool.
    *
    * @tparam Func Types of the function objects to invoke.
    * @param entity A valid entity identifier.
    * @param func Valid function objects.
    * @return A reference to the patched instance.
    */
    template<typename... Func>
    decltype(auto) patch(const entity_type entt, [[maybe_unused]] Func &&... func) {
        if constexpr(is_eto_eligible_v<object_type>) {
            update.publish(entt);
        } else {
            (std::forward<Func>(func)(this->get(entt)), ...);
            update.publish(entt);
            return this->get(entt);
        }
    }

    /**
    * @brief Replaces the object associated with an entity in a pool.
    *
    * A new object is created and initialized with the arguments provided (the
    * object type must have a proper constructor or be of aggregate type). Then
    * the instance is assigned to the given entity.
    *
    * @warning
    * Attempting to use an invalid entity or to replace an object of an entity
    * that doesn't belong to the pool results in undefined behavior.<br/>
    * An assertion will abort the execution at runtime in debug mode in case of
    * invalid entity or if the entity doesn't belong to the pool.
    *
    * @tparam Args Types of arguments to use to construct the object.
    * @param entity A valid entity identifier.
    * @param args Parameters to use to initialize the object.
    * @return A reference to the object being replaced.
    */
    decltype(auto) replace(const entity_type entt, object_type instance) {
        return patch(entt, [&instance](auto &&curr) { curr = std::move(instance); });
    }

private:
    sigh<void(const entity_type)> construction{};
    sigh<void(const entity_type)> destruction{};
    sigh<void(const entity_type)> update{};
};


/**
 * @brief Applies component-to-pool conversion and defines the resulting type as
 * the member typedef type.
 *
 * Formally:
 *
 * * If the component type is a non-const one, the member typedef type is the
 *   declared storage type.
 * * If the component type is a const one, the member typedef type is the
 *   declared storage type, except it has a const-qualifier added.
 *
 * @tparam Entity A valid entity type (see entt_traits for more details).
 * @tparam Type Type of objects assigned to the entities.
 */
template<typename Entity, typename Type, typename = void>
struct pool {
    /*! @brief Resulting type after component-to-pool conversion. */
    using type = default_pool<Entity, Type>;
};


/*! @copydoc pool */
template<typename Entity, typename Type>
struct pool<Entity, const Type> {
    /*! @brief Resulting type after component-to-pool conversion. */
    using type = std::add_const_t<typename pool<Entity, std::remove_const_t<Type>>::type>;
};


/**
 * @brief Alias declaration to use to make component-to-pool conversions.
 * @tparam Entity A valid entity type (see entt_traits for more details).
 * @tparam Type Type of objects assigned to the entities.
 */
template<typename Entity, typename Type>
using pool_t = typename pool<Entity, Type>::type;


}


#endif

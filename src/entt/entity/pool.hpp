#ifndef ENTT_ENTITY_POOL_HPP
#define ENTT_ENTITY_POOL_HPP


#include <iterator>
#include <type_traits>
#include <utility>
#include "../config/config.h"
#include "../core/type_traits.hpp"
#include "../signal/sigh.hpp"
#include "fwd.hpp"
#include "storage.hpp"


namespace entt {


/**
 * @brief Mixin type to use to wrap basic storage classes.
 * @tparam Type The type of the underlying storage.
 */
template<typename Type>
struct storage_adapter_mixin: Type {
    static_assert(std::is_same_v<typename Type::value_type, std::decay_t<typename Type::value_type>>, "Invalid object type");

    /*! @brief Type of the objects associated with the entities. */
    using value_type = typename Type::value_type;
    /*! @brief Underlying entity identifier. */
    using entity_type = typename Type::entity_type;

    /**
     * @brief Assigns entities to a storage.
     * @tparam Args Types of arguments to use to construct the object.
     * @param entity A valid entity identifier.
     * @param args Parameters to use to initialize the object.
     * @return A reference to the newly created object.
     */
    template<typename... Args>
    decltype(auto) emplace(basic_registry<entity_type> &, const entity_type entity, Args &&... args) {
        return Type::emplace(entity, std::forward<Args>(args)...);
    }

    /**
     * @brief Assigns entities to a storage.
     * @tparam It Type of input iterator.
     * @tparam Args Types of arguments to use to construct the objects
     * associated with the entities.
     * @param first An iterator to the first element of the range of entities.
     * @param last An iterator past the last element of the range of entities.
     * @param args Parameters to use to initialize the objects associated with
     * the entities.
     */
    template<typename It, typename... Args>
    void insert(basic_registry<entity_type> &, It first, It last, Args &&... args) {
        Type::insert(first, last, std::forward<Args>(args)...);
    }

    /**
     * @brief Removes entities from a storage.
     * @param entity A valid entity identifier.
     */
    void remove(basic_registry<entity_type> &, const entity_type entity) {
        Type::remove(entity);
    }

    /**
     * @copybrief remove
     * @tparam It Type of input iterator.
     * @param first An iterator to the first element of the range of entities.
     * @param last An iterator past the last element of the range of entities.
     */
    template<typename It>
    void remove(basic_registry<entity_type> &, It first, It last) {
        Type::remove(first, last);
    }

    /**
     * @brief Patches the given instance for an entity.
     * @tparam Func Types of the function objects to invoke.
     * @param entity A valid entity identifier.
     * @param func Valid function objects.
     * @return A reference to the patched instance.
     */
    template<typename... Func>
    decltype(auto) patch(basic_registry<entity_type> &, const entity_type entity, [[maybe_unused]] Func &&... func) {
        auto &instance = this->get(entity);
        (std::forward<Func>(func)(instance), ...);
        return instance;
    }
};


/**
 * @brief Mixin type to use to add signal support to storage types.
 * @tparam Type The type of the underlying storage.
 */
template<typename Type>
struct sigh_storage_mixin: Type {
    /*! @brief Underlying value type. */
    using value_type = typename Type::value_type;
    /*! @brief Underlying entity identifier. */
    using entity_type = typename Type::entity_type;

    /**
     * @brief Returns a sink object.
     *
     * The sink returned by this function can be used to receive notifications
     * whenever a new instance is created and assigned to an entity.<br/>
     * The function type for a listener is equivalent to:
     *
     * @code{.cpp}
     * void(basic_registry<entity_type> &, entity_type);
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
     * @brief Returns a sink object.
     *
     * The sink returned by this function can be used to receive notifications
     * whenever an instance is explicitly updated.<br/>
     * The function type for a listener is equivalent to:
     *
     * @code{.cpp}
     * void(basic_registry<entity_type> &, entity_type);
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
     * @brief Returns a sink object.
     *
     * The sink returned by this function can be used to receive notifications
     * whenever an instance is removed from an entity and thus destroyed.<br/>
     * The function type for a listener is equivalent to:
     *
     * @code{.cpp}
     * void(basic_registry<entity_type> &, entity_type);
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
     * @copybrief storage_adapter_mixin::emplace
     * @tparam Args Types of arguments to use to construct the object.
     * @param owner The registry that issued the request.
     * @param entity A valid entity identifier.
     * @param args Parameters to use to initialize the object.
     * @return A reference to the newly created object.
     */
    template<typename... Args>
    decltype(auto) emplace(basic_registry<entity_type> &owner, const entity_type entity, Args &&... args) {
        Type::emplace(owner, entity, std::forward<Args>(args)...);
        construction.publish(owner, entity);

        if constexpr(std::tuple_size_v<decltype(Type::get_as_tuple({}))> != 0) {
            return this->get(entity);
        }
    }

    /**
     * @copybrief storage_adapter_mixin::insert
     * @tparam It Type of input iterator.
     * @tparam Args Types of arguments to use to construct the objects
     * associated with the entities.
     * @param owner The registry that issued the request.
     * @param first An iterator to the first element of the range of entities.
     * @param last An iterator past the last element of the range of entities.
     * @param args Parameters to use to initialize the objects associated with
     * the entities.
     */
    template<typename It, typename... Args>
    void insert(basic_registry<entity_type> &owner, It first, It last, Args &&... args) {
        Type::insert(owner, first, last, std::forward<Args>(args)...);

        if(!construction.empty()) {
            for(; first != last; ++first) {
                construction.publish(owner, *first);
            }
        }
    }

    /**
     * @copybrief storage_adapter_mixin::remove
     * @param owner The registry that issued the request.
     * @param entity A valid entity identifier.
     */
    void remove(basic_registry<entity_type> &owner, const entity_type entity) {
        destruction.publish(owner, entity);
        Type::remove(owner, entity);
    }

    /**
     * @copybrief storage_adapter_mixin::remove
     * @tparam It Type of input iterator.
     * @param owner The registry that issued the request.
     * @param first An iterator to the first element of the range of entities.
     * @param last An iterator past the last element of the range of entities.
     */
    template<typename It>
    void remove(basic_registry<entity_type> &owner, It first, It last) {
        if(!destruction.empty()) {
            for(auto it = first; it != last; ++it) {
                destruction.publish(owner, *it);
            }
        }

        Type::remove(owner, first, last);
    }

    /**
     * @copybrief storage_adapter_mixin::patch
     * @tparam Func Types of the function objects to invoke.
     * @param owner The registry that issued the request.
     * @param entity A valid entity identifier.
     * @param func Valid function objects.
     * @return A reference to the patched instance.
     */
    template<typename... Func>
    decltype(auto) patch(basic_registry<entity_type> &owner, const entity_type entity, [[maybe_unused]] Func &&... func) {
        if constexpr(std::tuple_size_v<decltype(Type::get_as_tuple({}))> == 0) {
            update.publish(owner, entity);
        } else {
            Type::patch(owner, entity, std::forward<Func>(func)...);
            update.publish(owner, entity);
            return this->get(entity);
        }
    }

private:
    sigh<void(basic_registry<entity_type> &, const entity_type)> construction{};
    sigh<void(basic_registry<entity_type> &, const entity_type)> destruction{};
    sigh<void(basic_registry<entity_type> &, const entity_type)> update{};
};


/**
 * @brief Applies component-to-storage conversion and defines the resulting type
 * as the member typedef type.
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
struct pool_traits {
    /*! @brief Resulting type after component-to-pool conversion. */
    using value_type = sigh_storage_mixin<storage_adapter_mixin<basic_storage<Entity, Type>>>;
};


/*! @copydoc pool_traits */
template<typename Entity, typename Type>
struct pool_traits<Entity, const Type> {
    /*! @brief Resulting type after component-to-pool conversion. */
    using value_type = std::add_const_t<typename pool_traits<Entity, std::remove_const_t<Type>>::value_type>;
};


/**
 * @brief Alias declaration to use to make component-to-pool conversions.
 * @tparam Entity A valid entity type (see entt_traits for more details).
 * @tparam Type Type of objects assigned to the entities.
 */
template<typename Entity, typename Type>
using pool_t = typename pool_traits<Entity, Type>::value_type;


}


#endif

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


template<typename Entity, typename Component>
struct default_pool final: storage<Entity, Component> {
    static_assert(std::is_same_v<Component, std::decay_t<Component>>, "Invalid component type");

    [[nodiscard]] auto on_construct() ENTT_NOEXCEPT {
        return sink{construction};
    }

    [[nodiscard]] auto on_update() ENTT_NOEXCEPT {
        return sink{update};
    }

    [[nodiscard]] auto on_destroy() ENTT_NOEXCEPT {
        return sink{destruction};
    }

    template<typename... Args>
    decltype(auto) emplace(const Entity entt, Args &&... args) {
        storage<entity_type, Component>::emplace(entt, std::forward<Args>(args)...);
        construction.publish(entt);

        if constexpr(!is_eto_eligible_v<Component>) {
            return this->get(entt);
        }
    }

    template<typename It, typename... Args>
    void insert(It first, It last, Args &&... args) {
        storage<entity_type, Component>::insert(first, last, std::forward<Args>(args)...);

        if(!construction.empty()) {
            while(first != last) { construction.publish(*(first++)); }
        }
    }

    void erase(const Entity entt) override {
        destruction.publish(entt);
        storage<entity_type, Component>::erase(entt);
    }

    template<typename It>
    void erase(It first, It last) {
        if(std::distance(first, last) == std::distance(this->begin(), this->end())) {
            if(!destruction.empty()) {
                while(first != last) { destruction.publish(*(first++)); }
            }

            this->clear();
        } else {
            while(first != last) { this->erase(*(first++)); }
        }
    }

    template<typename... Func>
    decltype(auto) patch(const Entity entt, [[maybe_unused]] Func &&... func) {
        if constexpr(is_eto_eligible_v<Component>) {
            update.publish(entt);
        } else {
            (std::forward<Func>(func)(this->get(entt)), ...);
            update.publish(entt);
            return this->get(entt);
        }
    }

    decltype(auto) replace(const Entity entt, Component component) {
        return patch(entt, [&component](auto &&curr) { curr = std::move(component); });
    }

private:
    sigh<void(const Entity)> construction{};
    sigh<void(const Entity)> destruction{};
    sigh<void(const Entity)> update{};
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

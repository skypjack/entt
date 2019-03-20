#ifndef ENTT_ENTITY_ACTOR_HPP
#define ENTT_ENTITY_ACTOR_HPP


#include <cassert>
#include <utility>
#include <type_traits>
#include "../config/config.h"
#include "registry.hpp"
#include "entity.hpp"
#include "fwd.hpp"


namespace entt {


/**
 * @brief Dedicated to those who aren't confident with entity-component systems.
 *
 * Tiny wrapper around a registry, for all those users that aren't confident
 * with entity-component systems and prefer to iterate objects directly.
 *
 * @tparam Entity A valid entity type (see entt_traits for more details).
 */
template<typename Entity>
struct basic_actor {
    /*! @brief Type of registry used internally. */
    using registry_type = basic_registry<Entity>;
    /*! @brief Underlying entity identifier. */
    using entity_type = Entity;

    /**
     * @brief Constructs an actor by using the given registry.
     * @param ref An entity-component system properly initialized.
     */
    basic_actor(registry_type &ref)
        : reg{&ref}, entt{ref.create()}
    {}

    /*! @brief Default destructor. */
    virtual ~basic_actor() {
        reg->destroy(entt);
    }

    /**
     * @brief Move constructor.
     *
     * After actor move construction, instances that have been moved from are
     * placed in a valid but unspecified state. It's highly discouraged to
     * continue using them.
     *
     * @param other The instance to move from.
     */
    basic_actor(basic_actor &&other)
        : reg{other.reg}, entt{other.entt}
    {
        other.entt = null;
    }

    /**
     * @brief Move assignment operator.
     *
     * After actor move assignment, instances that have been moved from are
     * placed in a valid but unspecified state. It's highly discouraged to
     * continue using them.
     *
     * @param other The instance to move from.
     * @return This actor.
     */
    basic_actor & operator=(basic_actor &&other) {
        if(this != &other) {
            auto tmp{std::move(other)};
            std::swap(reg, tmp.reg);
            std::swap(entt, tmp.entt);
        }

        return *this;
    }

    /**
     * @brief Assigns the given component to an actor.
     *
     * A new instance of the given component is created and initialized with the
     * arguments provided (the component must have a proper constructor or be of
     * aggregate type). Then the component is assigned to the actor.<br/>
     * In case the actor already has a component of the given type, it's
     * replaced with the new one.
     *
     * @tparam Component Type of the component to create.
     * @tparam Args Types of arguments to use to construct the component.
     * @param args Parameters to use to initialize the component.
     * @return A reference to the newly created component.
     */
    template<typename Component, typename... Args>
    Component & assign(Args &&... args) {
        return reg->template assign_or_replace<Component>(entt, std::forward<Args>(args)...);
    }

    /**
     * @brief Removes the given component from an actor.
     * @tparam Component Type of the component to remove.
     */
    template<typename Component>
    void remove() {
        reg->template remove<Component>(entt);
    }

    /**
     * @brief Checks if an actor has the given component.
     * @tparam Component Type of the component for which to perform the check.
     * @return True if the actor has the component, false otherwise.
     */
    template<typename Component>
    bool has() const ENTT_NOEXCEPT {
        return reg->template has<Component>(entt);
    }

    /**
     * @brief Returns references to the given components for an actor.
     * @tparam Component Types of components to get.
     * @return References to the components owned by the actor.
     */
    template<typename... Component>
    decltype(auto) get() const ENTT_NOEXCEPT {
        return std::as_const(*reg).template get<Component...>(entt);
    }

    /*! @copydoc get */
    template<typename... Component>
    decltype(auto) get() ENTT_NOEXCEPT {
        return reg->template get<Component...>(entt);
    }

    /**
     * @brief Returns pointers to the given components for an actor.
     * @tparam Component Types of components to get.
     * @return Pointers to the components owned by the actor.
     */
    template<typename... Component>
    auto try_get() const ENTT_NOEXCEPT {
        return std::as_const(*reg).template try_get<Component...>(entt);
    }

    /*! @copydoc try_get */
    template<typename... Component>
    auto try_get() ENTT_NOEXCEPT {
        return reg->template try_get<Component...>(entt);
    }

    /**
     * @brief Returns a reference to the underlying registry.
     * @return A reference to the underlying registry.
     */
    inline const registry_type & backend() const ENTT_NOEXCEPT {
        return *reg;
    }

    /*! @copydoc backend */
    inline registry_type & backend() ENTT_NOEXCEPT {
        return const_cast<registry_type &>(std::as_const(*this).backend());
    }

    /**
     * @brief Returns the entity associated with an actor.
     * @return The entity associated with the actor.
     */
    inline entity_type entity() const ENTT_NOEXCEPT {
        return entt;
    }

private:
    registry_type *reg;
    Entity entt;
};


}


#endif // ENTT_ENTITY_ACTOR_HPP

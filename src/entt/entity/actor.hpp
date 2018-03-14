#ifndef ENTT_ENTITY_ACTOR_HPP
#define ENTT_ENTITY_ACTOR_HPP


#include <utility>
#include "registry.hpp"


namespace entt {


/**
 * @brief Dedicated to those who aren't confident with entity-component systems.
 *
 * Tiny wrapper around a registry, for all those users that aren't confident
 * with entity-component systems and prefer to iterate objects directly.
 *
 * @tparam Entity A valid entity type (see entt_traits for more details).
 * @tparam Delta Type to use to provide elapsed time.
 */
template<typename Entity, typename Delta>
struct Actor {
    /*! @brief Type of registry used internally. */
    using registry_type = Registry<Entity>;
    /*! @brief Underlying entity identifier. */
    using entity_type = Entity;
    /*! @brief Type used to provide elapsed time. */
    using delta_type = Delta;

    /**
     * @brief Constructs an actor by using the given registry.
     * @param reg An entity-component system properly initialized.
     */
    Actor(Registry<Entity> &reg)
        : reg{reg}, entity{reg.create()}
    {}

    /*! @brief Default destructor. */
    virtual ~Actor() {
        reg.destroy(entity);
    }

    /*! @brief Default copy constructor. */
    Actor(const Actor &) = default;
    /*! @brief Default move constructor. */
    Actor(Actor &&) = default;

    /*! @brief Default copy assignment operator. @return This actor. */
    Actor & operator=(const Actor &) = default;
    /*! @brief Default move assignment operator. @return This actor. */
    Actor & operator=(Actor &&) = default;

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
    Component & set(Args &&... args) {
        return reg.template accommodate<Component>(entity, std::forward<Args>(args)...);
    }

    /**
     * @brief Removes the given component from an actor.
     * @tparam Component Type of the component to remove.
     */
    template<typename Component>
    void unset() {
        reg.template remove<Component>(entity);
    }

    /**
     * @brief Checks if an actor has the given component.
     * @tparam Component Type of the component for which to perform the check.
     * @return True if the actor has the component, false otherwise.
     */
    template<typename Component>
    bool has() const noexcept {
        return reg.template has<Component>(entity);
    }

    /**
     * @brief Returns a reference to the given component for an actor.
     * @tparam Component Type of the component to get.
     * @return A reference to the instance of the component owned by the entity.
     */
    template<typename Component>
    const Component & get() const noexcept {
        return reg.template get<Component>(entity);
    }

    /**
     * @brief Returns a reference to the given component for an actor.
     * @tparam Component Type of the component to get.
     * @return A reference to the instance of the component owned by the entity.
     */
    template<typename Component>
    Component & get() noexcept {
        return const_cast<Component &>(const_cast<const Actor *>(this)->get<Component>());
    }

    /**
     * @brief Returns a reference to the underlying registry.
     * @return A reference to the underlying registry
     */
    const registry_type & registry() const noexcept {
        return reg;
    }

    /**
     * @brief Returns a reference to the underlying registry.
     * @return A reference to the underlying registry
     */
    registry_type & registry() noexcept {
        return const_cast<registry_type &>(const_cast<const Actor *>(this)->registry());
    }

    /**
     * @brief Updates an actor, whatever it means to update it.
     * @param delta Elapsed time.
     */
    virtual void update(delta_type delta) = 0;

private:
    registry_type &reg;
    Entity entity;
};


/**
 * @brief Default actor class.
 *
 * The default actor is the best choice for almost all the applications.<br/>
 * Users should have a really good reason to choose something different.
 *
 * @tparam Delta Type to use to provide elapsed time.
 */
template<typename Delta>
using DefaultActor = Actor<DefaultRegistry::entity_type, Delta>;


}


#endif // ENTT_ENTITY_ACTOR_HPP

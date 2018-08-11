#ifndef ENTT_ENTITY_ACTOR_HPP
#define ENTT_ENTITY_ACTOR_HPP


#include <cassert>
#include <utility>
#include "../config/config.h"
#include "registry.hpp"
#include "entity.hpp"


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
struct Actor {
    /*! @brief Type of registry used internally. */
    using registry_type = Registry<Entity>;
    /*! @brief Underlying entity identifier. */
    using entity_type = Entity;

    /**
     * @brief Constructs an actor by using the given registry.
     * @param reg An entity-component system properly initialized.
     */
    Actor(Registry<Entity> &reg)
        : reg{&reg}, entt{reg.create()}
    {}

    /*! @brief Default destructor. */
    virtual ~Actor() {
        reg->destroy(entt);
    }

    /*! @brief Copying an actor isn't allowed. */
    Actor(const Actor &) = delete;

    /**
     * @brief Move constructor.
     *
     * After actor move construction, instances that have been moved from are
     * placed in a valid but unspecified state. It's highly discouraged to
     * continue using them.
     *
     * @param other The instance to move from.
     */
    Actor(Actor &&other)
        : reg{other.reg}, entt{other.entt}
    {
        other.entt = entt::null;
    }

    /*! @brief Default copy assignment operator. @return This actor. */
    Actor & operator=(const Actor &) = delete;

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
    Actor & operator=(Actor &&other) {
        if(this != &other) {
            auto tmp{std::move(other)};
            std::swap(reg, tmp.reg);
            std::swap(entt, tmp.entt);
        }

        return *this;
    }

    /**
     * @brief Assigns the given tag to an actor.
     *
     * A new instance of the given tag is created and initialized with the
     * arguments provided (the tag must have a proper constructor or be of
     * aggregate type). Then the tag is removed from its previous owner (if any)
     * and assigned to the actor.
     *
     * @tparam Tag Type of the tag to create.
     * @tparam Args Types of arguments to use to construct the tag.
     * @param args Parameters to use to initialize the tag.
     * @return A reference to the newly created tag.
     */
    template<typename Tag, typename... Args>
    Tag & assign(tag_t, Args &&... args) {
        return (reg->template remove<Tag>(), reg->template assign<Tag>(tag_t{}, entt, std::forward<Args>(args)...));
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
        return reg->template accommodate<Component>(entt, std::forward<Args>(args)...);
    }

    /**
     * @brief Removes the given tag from an actor.
     * @tparam Tag Type of the tag to remove.
     */
    template<typename Tag>
    void remove(tag_t) {
        assert(has<Tag>(tag_t{}));
        reg->template remove<Tag>();
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
     * @brief Checks if an actor owns the given tag.
     * @tparam Tag Type of the tag for which to perform the check.
     * @return True if the actor owns the tag, false otherwise.
     */
    template<typename Tag>
    bool has(tag_t) const ENTT_NOEXCEPT {
        return (reg->template has<Tag>() && (reg->template attachee<Tag>() == entt));
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
     * @brief Returns a reference to the given tag for an actor.
     * @tparam Tag Type of the tag to get.
     * @return A reference to the instance of the tag owned by the actor.
     */
    template<typename Tag>
    const Tag & get(tag_t) const ENTT_NOEXCEPT {
        assert(has<Tag>(tag_t{}));
        return reg->template get<Tag>();
    }

    /**
     * @brief Returns a reference to the given tag for an actor.
     * @tparam Tag Type of the tag to get.
     * @return A reference to the instance of the tag owned by the actor.
     */
    template<typename Tag>
    inline Tag & get(tag_t) ENTT_NOEXCEPT {
        return const_cast<Tag &>(const_cast<const Actor *>(this)->get<Tag>(tag_t{}));
    }

    /**
     * @brief Returns a reference to the given component for an actor.
     * @tparam Component Type of the component to get.
     * @return A reference to the instance of the component owned by the actor.
     */
    template<typename Component>
    const Component & get() const ENTT_NOEXCEPT {
        return reg->template get<Component>(entt);
    }

    /**
     * @brief Returns a reference to the given component for an actor.
     * @tparam Component Type of the component to get.
     * @return A reference to the instance of the component owned by the actor.
     */
    template<typename Component>
    inline Component & get() ENTT_NOEXCEPT {
        return const_cast<Component &>(const_cast<const Actor *>(this)->get<Component>());
    }

    /**
     * @brief Returns a reference to the underlying registry.
     * @return A reference to the underlying registry.
     */
    inline const registry_type & registry() const ENTT_NOEXCEPT {
        return *reg;
    }

    /**
     * @brief Returns a reference to the underlying registry.
     * @return A reference to the underlying registry.
     */
    inline registry_type & registry() ENTT_NOEXCEPT {
        return const_cast<registry_type &>(const_cast<const Actor *>(this)->registry());
    }

    /**
     * @brief Returns the entity associated with an actor.
     * @return The entity associated with the actor.
     */
    inline entity_type entity() const ENTT_NOEXCEPT {
        return entt;
    }

private:
    registry_type * reg;
    Entity entt;
};


/**
 * @brief Default actor class.
 *
 * The default actor is the best choice for almost all the applications.<br/>
 * Users should have a really good reason to choose something different.
 */
using DefaultActor = Actor<DefaultRegistry::entity_type>;


}


#endif // ENTT_ENTITY_ACTOR_HPP

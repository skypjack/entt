#ifndef ENTT_ENTITY_ACTOR_HPP
#define ENTT_ENTITY_ACTOR_HPP


#include <utility>
#include <type_traits>
#include "../config/config.h"
#include "registry.hpp"
#include "entity.hpp"
#include "fwd.hpp"


namespace entt {


/**
 * @brief Dedicated to those who aren't confident with the
 * entity-component-system architecture.
 *
 * Tiny wrapper around a registry, for all those users that aren't confident
 * with entity-component-system architecture and prefer to iterate objects
 * directly.
 *
 * @tparam Entity A valid entity type (see entt_traits for more details).
 */
template<typename Entity>
class basic_actor {
public:
    /*! @brief Type of registry used internally. */
    using registry_type = basic_registry<Entity>;
    /*! @brief Underlying entity identifier. */
    using entity_type = Entity;

    basic_actor() ENTT_NOEXCEPT
        : entt{entt::null}, reg{nullptr}
    {}

    /**
     * @brief Move constructor.
     *
     * After actor move construction, instances that have been moved from are
     * placed in a valid but unspecified state. It's highly discouraged to
     * continue using them.
     *
     * @param other The instance to move from.
     */
    basic_actor(basic_actor &&other) ENTT_NOEXCEPT
        : entt{other.entt}, reg{other.reg}
    {
        other.entt = null;
    }

    /**
     * @brief Constructs an actor from a given registry.
     * @param ref An instance of the registry class.
     */
    explicit basic_actor(registry_type &ref)
        : entt{ref.create()}, reg{&ref}
    {}

    /**
     * @brief Constructs an actor from a given entity.
     * @param entity A valid entity identifier.
     * @param ref An instance of the registry class.
     */
    explicit basic_actor(entity_type entity, registry_type &ref) ENTT_NOEXCEPT
        : entt{entity}, reg{&ref}
    {
        ENTT_ASSERT(ref.valid(entity));
    }

    /*! @brief Default destructor. */
    virtual ~basic_actor() {
        if(*this) {
            reg->destroy(entt);
        }
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
    basic_actor & operator=(basic_actor &&other) ENTT_NOEXCEPT {
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
    decltype(auto) assign(Args &&... args) {
        return reg->template emplace_or_replace<Component>(entt, std::forward<Args>(args)...);
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
     * @brief Checks if an actor has the given components.
     * @tparam Component Components for which to perform the check.
     * @return True if the actor has all the components, false otherwise.
     */
    template<typename... Component>
    [[nodiscard]] bool has() const {
        return reg->template has<Component...>(entt);
    }

    /**
     * @brief Returns references to the given components for an actor.
     * @tparam Component Types of components to get.
     * @return References to the components owned by the actor.
     */
    template<typename... Component>
    [[nodiscard]] decltype(auto) get() const {
        return std::as_const(*reg).template get<Component...>(entt);
    }

    /*! @copydoc get */
    template<typename... Component>
    [[nodiscard]] decltype(auto) get() {
        return reg->template get<Component...>(entt);
    }

    /**
     * @brief Returns pointers to the given components for an actor.
     * @tparam Component Types of components to get.
     * @return Pointers to the components owned by the actor.
     */
    template<typename... Component>
    [[nodiscard]] auto try_get() const {
        return std::as_const(*reg).template try_get<Component...>(entt);
    }

    /*! @copydoc try_get */
    template<typename... Component>
    [[nodiscard]] auto try_get() {
        return reg->template try_get<Component...>(entt);
    }

    /**
     * @brief Returns a reference to the underlying registry.
     * @return A reference to the underlying registry.
     */
    [[nodiscard]] const registry_type & backend() const ENTT_NOEXCEPT {
        return *reg;
    }

    /*! @copydoc backend */
    [[nodiscard]] registry_type & backend() ENTT_NOEXCEPT {
        return const_cast<registry_type &>(std::as_const(*this).backend());
    }

    /**
     * @brief Returns the entity associated with an actor.
     * @return The entity associated with the actor.
     */
    [[nodiscard]] entity_type entity() const ENTT_NOEXCEPT {
        return entt;
    }

    /**
     * @brief Checks if an actor refers to a valid entity or not.
     * @return True if the actor refers to a valid entity, false otherwise.
     */
    [[nodiscard]] explicit operator bool() const {
        return reg && reg->valid(entt);
    }

private:
    entity_type entt;
    registry_type *reg;
};


template<typename Entity>
basic_actor(basic_registry<Entity> &) -> basic_actor<Entity>;

template<typename Entity>
basic_actor(Entity, basic_registry<Entity> &) -> basic_actor<Entity>;


/**
 * @brief Non-owning handle to an entity.
 *
 * Tiny wrapper around an entity and a registry.
 *
 * @tparam Entity A valid entity type (see entt_traits for more details).
 */
template<typename Entity>
class basic_handle {
public:
    /*! @brief Type of registry used internally. */
    using registry_type = std::conditional_t<
        std::is_const_v<Entity>,
        const basic_registry<std::remove_const_t<Entity>>,
        basic_registry<Entity>
    >;
    /*! @brief Underlying entity identifier. */
    using entity_type = std::remove_const_t<Entity>;

    basic_handle() ENTT_NOEXCEPT
        : entt{entt::null}, reg{nullptr}
    {}

    /**
     * @brief Constructs a handle from a given entity.
     * @param entity A valid entity identifier.
     * @param ref An instance of the registry class.
     */
    explicit basic_handle(entity_type entity, registry_type &ref) ENTT_NOEXCEPT
        : entt{entity}, reg{&ref}
    {
        // Does this assertion really make sense?
        ENTT_ASSERT(ref.valid(entity));
    }
    
    using actor_type = std::conditional_t<
        std::is_const_v<Entity>,
        const basic_actor<std::remove_const_t<Entity>>,
        basic_actor<Entity>
    >;
    
    /**
     * @brief Constructs a handle from an actor.
     * @param actor An actor to construct from.
     */
    basic_handle(actor_type &actor) ENTT_NOEXCEPT
        : entt{actor.entity()}, reg{&actor.backend()} {}

    /**
     * @brief Constructs a const handle from a non-const handle.
     * @param handle A handle to construct from.
     */
    basic_handle(basic_handle<entity_type> handle) ENTT_NOEXCEPT
        : entt{handle.entity()}, reg{&handle.backend()} {}

    /**
     * @brief Assigns the given component to a handle.
     *
     * A new instance of the given component is created and initialized with the
     * arguments provided (the component must have a proper constructor or be of
     * aggregate type). Then the component is assigned to the handle.<br/>
     * In case the handle already has a component of the given type, it's
     * replaced with the new one.
     *
     * @tparam Component Type of the component to create.
     * @tparam Args Types of arguments to use to construct the component.
     * @param args Parameters to use to initialize the component.
     * @return A reference to the newly created component.
     */
    template<typename Component, typename... Args>
    decltype(auto) assign(Args &&... args) const {
        static_assert(!std::is_const_v<Entity>);
        return reg->template emplace_or_replace<Component>(entt, std::forward<Args>(args)...);
    }

    /**
     * @brief Removes the given component from a handle.
     * @tparam Component Type of the component to remove.
     */
    template<typename Component>
    void remove() const {
        static_assert(!std::is_const_v<Entity>);
        reg->template remove<Component>(entt);
    }

    /**
     * @brief Checks if a handle has the given components.
     * @tparam Component Components for which to perform the check.
     * @return True if the handle has all the components, false otherwise.
     */
    template<typename... Component>
    [[nodiscard]] bool has() const {
        return reg->template has<Component...>(entt);
    }

    /**
     * @brief Returns references to the given components for a handle.
     * @tparam Component Types of components to get.
     * @return References to the components owned by the handle.
     */
    template<typename... Component>
    [[nodiscard]] decltype(auto) get() const {
        return reg->template get<Component...>(entt);
    }

    /**
     * @brief Returns pointers to the given components for a handle.
     * @tparam Component Types of components to get.
     * @return Pointers to the components owned by the handle.
     */
    template<typename... Component>
    [[nodiscard]] auto try_get() const {
        return reg->template try_get<Component...>(entt);
    }

    /**
     * @brief Returns a reference to the underlying registry.
     * @return A reference to the underlying registry.
     */
    [[nodiscard]] registry_type & backend() const ENTT_NOEXCEPT {
        return *reg;
    }

    /**
     * @brief Returns the entity associated with a handle.
     * @return The entity associated with the handle.
     */
    [[nodiscard]] entity_type entity() const ENTT_NOEXCEPT {
        return entt;
    }

    /**
     * @brief Checks if a handle refers to a valid entity or not.
     * @return True if the handle refers to a valid entity, false otherwise.
     */
    [[nodiscard]] explicit operator bool() const {
        return reg && reg->valid(entt);
    }

private:
    entity_type entt;
    registry_type *reg;
};


template<typename Entity>
basic_handle(Entity, basic_registry<Entity> &) -> basic_handle<Entity>;

template<typename Entity>
basic_handle(Entity, const basic_registry<Entity> &) -> basic_handle<const Entity>;

template<typename Entity>
basic_handle(basic_actor<Entity> &) -> basic_handle<Entity>;

template<typename Entity>
basic_handle(const basic_actor<Entity> &) -> basic_handle<const Entity>;


}


#endif

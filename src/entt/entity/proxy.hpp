#ifndef ENTT_ENTITY_PROXY_HPP
#define ENTT_ENTITY_PROXY_HPP


#include <utility>
#include <type_traits>
#include "../config/config.h"
#include "registry.hpp"
#include "entity.hpp"
#include "fwd.hpp"


namespace entt {


/**
 * @brief Non-owning proxy to an entity.
 *
 * Tiny wrapper around an entity and a registry.
 *
 * @tparam Entity A valid entity type (see entt_traits for more details).
 */
template<typename Entity>
class basic_proxy {
public:
    /*! @brief Underlying entity identifier. */
    using entity_type = std::remove_const_t<Entity>;
    /*! @brief Type of registry used internally. */
    using registry_type = std::conditional_t<
        std::is_const_v<Entity>,
        const basic_registry<entity_type>,
        basic_registry<entity_type>
    >;

    basic_proxy() ENTT_NOEXCEPT
        : entt{null}, reg{nullptr}
    {}

    /**
     * @brief Constructs a proxy from a given entity.
     * @param entity A valid entity identifier.
     * @param ref An instance of the registry class.
     */
    explicit basic_proxy(entity_type entity, registry_type &ref) ENTT_NOEXCEPT
        : entt{entity}, reg{&ref}
    {
        // Does this assertion really make sense?
        ENTT_ASSERT(ref.valid(entity));
    }
    
    /**
     * @brief Constructs a const proxy from a non-const proxy.
     * @param proxy A proxy to construct from.
     */
    basic_proxy(basic_proxy<entity_type> proxy) ENTT_NOEXCEPT
        : entt{proxy.entity()}, reg{&proxy.backend()} {}

    /**
     * @brief Assigns the given component to a proxy.
     *
     * A new instance of the given component is created and initialized with the
     * arguments provided (the component must have a proper constructor or be of
     * aggregate type). Then the component is assigned to the proxy.<br/>
     * In case the proxy already has a component of the given type, it's
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
     * @brief Removes the given component from a proxy.
     * @tparam Component Type of the component to remove.
     */
    template<typename Component>
    void remove() const {
        static_assert(!std::is_const_v<Entity>);
        reg->template remove<Component>(entt);
    }

    /**
     * @brief Checks if a proxy has the given components.
     * @tparam Component Components for which to perform the check.
     * @return True if the proxy has all the components, false otherwise.
     */
    template<typename... Component>
    [[nodiscard]] bool has() const {
        return reg->template has<Component...>(entt);
    }

    /**
     * @brief Returns references to the given components for a proxy.
     * @tparam Component Types of components to get.
     * @return References to the components owned by the proxy.
     */
    template<typename... Component>
    [[nodiscard]] decltype(auto) get() const {
        return reg->template get<Component...>(entt);
    }

    /**
     * @brief Returns pointers to the given components for a proxy.
     * @tparam Component Types of components to get.
     * @return Pointers to the components owned by the proxy.
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
     * @brief Returns the entity associated with a proxy.
     * @return The entity associated with the proxy.
     */
    [[nodiscard]] entity_type entity() const ENTT_NOEXCEPT {
        return entt;
    }

    /**
     * @brief Checks if a proxy refers to a valid entity or not.
     * @return True if the proxy refers to a valid entity, false otherwise.
     */
    [[nodiscard]] explicit operator bool() const {
        return reg && reg->valid(entt);
    }

private:
    entity_type entt;
    registry_type *reg;
};


template<typename Entity>
basic_proxy(Entity, basic_registry<Entity> &) -> basic_proxy<Entity>;

template<typename Entity>
basic_proxy(Entity, const basic_registry<Entity> &) -> basic_proxy<const Entity>;


}


#endif

#ifndef ENTT_ENTITY_PROTOTYPE_HPP
#define ENTT_ENTITY_PROTOTYPE_HPP


#include <tuple>
#include <utility>
#include <cstddef>
#include <type_traits>
#include <unordered_map>
#include "../config/config.h"
#include "registry.hpp"


namespace entt {


/**
 * @brief Prototype container for _concepts_.
 *
 * A prototype is used to define a _concept_ in terms of components.<br/>
 * Prototypes act as templates for those specific types of an application which
 * users would otherwise define through a series of component assignments to
 * entities. In other words, prototypes can be used to assign components to
 * entities of a registry at once.
 *
 * @note
 * Components used along with prototypes must be copy constructible. Prototypes
 * wrap component types with custom types, so they do not interfere with other
 * users of the registry they were built with.
 *
 * @warning
 * Prototypes directly use their underlying registries to store entities and
 * components for their purposes. Users must ensure that the lifetime of a
 * registry and its contents exceed that of the prototypes that use it.
 *
 * @tparam Entity A valid entity type (see entt_traits for more details).
 */
template<typename Entity>
class Prototype final {
    using basic_fn_type = void(const Prototype &, Registry<Entity> &, const Entity);
    using component_type = typename Registry<Entity>::component_type;

    template<typename Component>
    struct Wrapper { Component component; };

    struct Handler {
        basic_fn_type *accommodate;
        basic_fn_type *assign;
    };

    void release() {
        if(registry->valid(entity)) {
            registry->destroy(entity);
        }
    }

public:
    /*! @brief Registry type. */
    using registry_type = Registry<Entity>;
    /*! @brief Underlying entity identifier. */
    using entity_type = Entity;
    /*! @brief Unsigned integer type. */
    using size_type = std::size_t;

    /**
     * @brief Constructs a prototype that is bound to a given registry.
     * @param registry A valid reference to a registry.
     */
    Prototype(Registry<Entity> &registry)
        : registry{&registry},
          entity{registry.create()}
    {}

    /**
     * @brief Releases all its resources.
     */
    ~Prototype() {
        release();
    }

    /*! @brief Copying a prototype isn't allowed. */
    Prototype(const Prototype &) = delete;

    /**
     * @brief Move constructor.
     *
     * After prototype move construction, instances that have been moved from
     * are placed in a valid but unspecified state. It's highly discouraged to
     * continue using them.
     *
     * @param other The instance to move from.
     */
    Prototype(Prototype &&other)
        : handlers{std::move(other.handlers)},
          registry{other.registry},
          entity{other.entity}
    {
        other.entity = ~entity_type{};
    }

    /*! @brief Copying a prototype isn't allowed. @return This Prototype. */
    Prototype & operator=(const Prototype &) = delete;

    /**
     * @brief Move assignment operator.
     *
     * After prototype move assignment, instances that have been moved from are
     * placed in a valid but unspecified state. It's highly discouraged to
     * continue using them.
     *
     * @param other The instance to move from.
     * @return This Prototype.
     */
    Prototype & operator=(Prototype &&other) {
        if(this != &other) {
            auto tmp{std::move(other)};
            handlers.swap(tmp.handlers);
            std::swap(registry, tmp.registry);
            std::swap(entity, tmp.entity);
        }

        return *this;
    }

    /**
     * @brief Assigns to or replaces the given component of a prototype.
     * @tparam Component Type of component to assign or replace.
     * @tparam Args Types of arguments to use to construct the component.
     * @param args Parameters to use to initialize the component.
     * @return A reference to the newly created component.
     */
    template<typename Component, typename... Args>
    Component & set(Args &&... args) {
        basic_fn_type *accommodate = [](const Prototype &prototype, Registry<Entity> &other, const Entity dst) {
            const auto &wrapper = prototype.registry->template get<Wrapper<Component>>(prototype.entity);
            other.template accommodate<Component>(dst, wrapper.component);
        };

        basic_fn_type *assign = [](const Prototype &prototype, Registry<Entity> &other, const Entity dst) {
            if(!other.template has<Component>(dst)) {
                const auto &wrapper = prototype.registry->template get<Wrapper<Component>>(prototype.entity);
                other.template accommodate<Component>(dst, wrapper.component);
            }
        };

        handlers[registry->template type<Component>()] = Handler{accommodate, assign};
        auto &wrapper = registry->template accommodate<Wrapper<Component>>(entity, Component{std::forward<Args>(args)...});
        return wrapper.component;
    }

    /**
     * @brief Removes the given component from a prototype.
     * @tparam Component Type of component to remove.
     */
    template<typename Component>
    void unset() ENTT_NOEXCEPT {
        registry->template reset<Wrapper<Component>>(entity);
        handlers.erase(registry->template type<Component>());
    }

    /**
     * @brief Checks if a prototype owns all the given components.
     * @tparam Component Components for which to perform the check.
     * @return True if the prototype owns all the components, false otherwise.
     */
    template<typename... Component>
    bool has() const ENTT_NOEXCEPT {
        return registry->template has<Wrapper<Component>...>(entity);
    }

    /**
     * @brief Returns a reference to the given component.
     *
     * @warning
     * Attempting to get a component from a prototype that doesn't own it
     * results in undefined behavior.<br/>
     * An assertion will abort the execution at runtime in debug mode if the
     * prototype doesn't own an instance of the given component.
     *
     * @tparam Component Type of component to get.
     * @return A reference to the component owned by the prototype.
     */
    template<typename Component>
    const Component & get() const ENTT_NOEXCEPT {
        return registry->template get<Wrapper<Component>>(entity).component;
    }

    /**
     * @brief Returns a reference to the given component.
     *
     * @warning
     * Attempting to get a component from a prototype that doesn't own it
     * results in undefined behavior.<br/>
     * An assertion will abort the execution at runtime in debug mode if the
     * prototype doesn't own an instance of the given component.
     *
     * @tparam Component Type of component to get.
     * @return A reference to the component owned by the prototype.
     */
    template<typename Component>
    inline Component & get() ENTT_NOEXCEPT {
        return const_cast<Component &>(const_cast<const Prototype *>(this)->get<Component>());
    }

    /**
     * @brief Returns a reference to the given components.
     *
     * @warning
     * Attempting to get components from a prototype that doesn't own them
     * results in undefined behavior.<br/>
     * An assertion will abort the execution at runtime in debug mode if the
     * prototype doesn't own instances of the given components.
     *
     * @tparam Component Type of components to get.
     * @return References to the components owned by the prototype.
     */
    template<typename... Component>
    inline std::enable_if_t<(sizeof...(Component) > 1), std::tuple<const Component &...>>
    get() const ENTT_NOEXCEPT {
        return std::tuple<const Component &...>{get<Component>()...};
    }

    /**
     * @brief Returns a reference to the given components.
     *
     * @warning
     * Attempting to get components from a prototype that doesn't own them
     * results in undefined behavior.<br/>
     * An assertion will abort the execution at runtime in debug mode if the
     * prototype doesn't own instances of the given components.
     *
     * @tparam Component Type of components to get.
     * @return References to the components owned by the prototype.
     */
    template<typename... Component>
    inline std::enable_if_t<(sizeof...(Component) > 1), std::tuple<Component &...>>
    get() ENTT_NOEXCEPT {
        return std::tuple<Component &...>{get<Component>()...};
    }

    /**
     * @brief Creates a new entity using a given prototype.
     *
     * Utility shortcut, equivalent to the following snippet:
     *
     * @code{.cpp}
     * const auto entity = registry.create();
     * prototype(registry, entity);
     * @endcode
     *
     * @note
     * The registry may or may not be different from the one already used by
     * the prototype. There is also an overload that directly uses the
     * underlying registry.
     *
     * @param other A valid reference to a registry.
     * @return A valid entity identifier.
     */
    entity_type create(registry_type &other) const {
        const auto entity = other.create();
        assign(other, entity);
        return entity;
    }

    /**
     * @brief Creates a new entity using a given prototype.
     *
     * Utility shortcut, equivalent to the following snippet:
     *
     * @code{.cpp}
     * const auto entity = registry.create();
     * prototype(entity);
     * @endcode
     *
     * @note
     * This overload directly uses the underlying registry as a working space.
     * Therefore, the components of the prototype and of the entity will share
     * the same registry.
     *
     * @return A valid entity identifier.
     */
    inline entity_type create() const {
        return create(*registry);
    }

    /**
     * @brief Assigns the components of a prototype to a given entity.
     *
     * Assigning a prototype to an entity won't overwrite existing components
     * under any circumstances.<br/>
     * In other words, only those components that the entity doesn't own yet are
     * copied over. All the other components remain unchanged.
     *
     * @note
     * The registry may or may not be different from the one already used by
     * the prototype. There is also an overload that directly uses the
     * underlying registry.
     *
     * @warning
     * Attempting to use an invalid entity results in undefined behavior.<br/>
     * An assertion will abort the execution at runtime in debug mode in case of
     * invalid entity.
     *
     * @param other A valid reference to a registry.
     * @param dst A valid entity identifier.
     */
    void assign(registry_type &other, const entity_type dst) const {
        for(auto &handler: handlers) {
            handler.second.assign(*this, other, dst);
        }
    }

    /**
     * @brief Assigns the components of a prototype to a given entity.
     *
     * Assigning a prototype to an entity won't overwrite existing components
     * under any circumstances.<br/>
     * In other words, only those components that the entity doesn't own yet are
     * copied over. All the other components remain unchanged.
     *
     * @note
     * This overload directly uses the underlying registry as a working space.
     * Therefore, the components of the prototype and of the entity will share
     * the same registry.
     *
     * @warning
     * Attempting to use an invalid entity results in undefined behavior.<br/>
     * An assertion will abort the execution at runtime in debug mode in case of
     * invalid entity.
     *
     * @param dst A valid entity identifier.
     */
    inline void assign(const entity_type dst) const {
        assign(*registry, dst);
    }

    /**
     * @brief Assigns or replaces the components of a prototype for an entity.
     *
     * Existing components are overwritten, if any. All the other components
     * will be copied over to the target entity.
     *
     * @note
     * The registry may or may not be different from the one already used by
     * the prototype. There is also an overload that directly uses the
     * underlying registry.
     *
     * @warning
     * Attempting to use an invalid entity results in undefined behavior.<br/>
     * An assertion will abort the execution at runtime in debug mode in case of
     * invalid entity.
     *
     * @param other A valid reference to a registry.
     * @param dst A valid entity identifier.
     */
    void accommodate(registry_type &other, const entity_type dst) const {
        for(auto &handler: handlers) {
            handler.second.accommodate(*this, other, dst);
        }
    }

    /**
     * @brief Assigns or replaces the components of a prototype for an entity.
     *
     * Existing components are overwritten, if any. All the other components
     * will be copied over to the target entity.
     *
     * @note
     * This overload directly uses the underlying registry as a working space.
     * Therefore, the components of the prototype and of the entity will share
     * the same registry.
     *
     * @warning
     * Attempting to use an invalid entity results in undefined behavior.<br/>
     * An assertion will abort the execution at runtime in debug mode in case of
     * invalid entity.
     *
     * @param dst A valid entity identifier.
     */
    inline void accommodate(const entity_type dst) const {
        accommodate(*registry, dst);
    }

    /**
     * @brief Assigns the components of a prototype to an entity.
     *
     * Assigning a prototype to an entity won't overwrite existing components
     * under any circumstances.<br/>
     * In other words, only the components that the entity doesn't own yet are
     * copied over. All the other components remain unchanged.
     *
     * @note
     * The registry may or may not be different from the one already used by
     * the prototype. There is also an overload that directly uses the
     * underlying registry.
     *
     * @warning
     * Attempting to use an invalid entity results in undefined behavior.<br/>
     * An assertion will abort the execution at runtime in debug mode in case of
     * invalid entity.
     *
     * @param other A valid reference to a registry.
     * @param dst A valid entity identifier.
     */
    inline void operator()(registry_type &other, const entity_type dst) const ENTT_NOEXCEPT {
        assign(other, dst);
    }

    /**
     * @brief Assigns the components of a prototype to an entity.
     *
     * Assigning a prototype to an entity won't overwrite existing components
     * under any circumstances.<br/>
     * In other words, only the components that the entity doesn't own yet are
     * copied over. All the other components remain unchanged.
     *
     * @note
     * This overload directly uses the underlying registry as a working space.
     * Therefore, the components of the prototype and of the entity will share
     * the same registry.
     *
     * @warning
     * Attempting to use an invalid entity results in undefined behavior.<br/>
     * An assertion will abort the execution at runtime in debug mode in case of
     * invalid entity.
     *
     * @param dst A valid entity identifier.
     */
    inline void operator()(const entity_type dst) const ENTT_NOEXCEPT {
        assign(*registry, dst);
    }

    /**
     * @brief Creates a new entity using a given prototype.
     *
     * Utility shortcut, equivalent to the following snippet:
     *
     * @code{.cpp}
     * const auto entity = registry.create();
     * prototype(registry, entity);
     * @endcode
     *
     * @note
     * The registry may or may not be different from the one already used by
     * the prototype. There is also an overload that directly uses the
     * underlying registry.
     *
     * @param other A valid reference to a registry.
     * @return A valid entity identifier.
     */
    inline entity_type operator()(registry_type &other) const ENTT_NOEXCEPT {
        return create(other);
    }

    /**
     * @brief Creates a new entity using a given prototype.
     *
     * Utility shortcut, equivalent to the following snippet:
     *
     * @code{.cpp}
     * const auto entity = registry.create();
     * prototype(entity);
     * @endcode
     *
     * @note
     * This overload directly uses the underlying registry as a working space.
     * Therefore, the components of the prototype and of the entity will share
     * the same registry.
     *
     * @return A valid entity identifier.
     */
    inline entity_type operator()() const ENTT_NOEXCEPT {
        return create(*registry);
    }

private:
    std::unordered_map<component_type, Handler> handlers;
    Registry<Entity> *registry;
    entity_type entity;
};


/**
 * @brief Default prototype
 *
 * The default prototype is the best choice for almost all the
 * applications.<br/>
 * Users should have a really good reason to choose something different.
 */
using DefaultPrototype = Prototype<DefaultRegistry::entity_type>;


}


#endif // ENTT_ENTITY_PROTOTYPE_HPP

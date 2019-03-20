#ifndef ENTT_ENTITY_PROTOTYPE_HPP
#define ENTT_ENTITY_PROTOTYPE_HPP


#include <tuple>
#include <utility>
#include <cstddef>
#include <type_traits>
#include <unordered_map>
#include "../config/config.h"
#include "registry.hpp"
#include "entity.hpp"
#include "fwd.hpp"


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
class basic_prototype {
    using basic_fn_type = void(const basic_prototype &, basic_registry<Entity> &, const Entity);
    using component_type = typename basic_registry<Entity>::component_type;

    template<typename Component>
    struct component_wrapper { Component component; };

    struct component_handler {
        basic_fn_type *assign_or_replace;
        basic_fn_type *assign;
    };

    void release() {
        if(reg->valid(entity)) {
            reg->destroy(entity);
        }
    }

public:
    /*! @brief Registry type. */
    using registry_type = basic_registry<Entity>;
    /*! @brief Underlying entity identifier. */
    using entity_type = Entity;
    /*! @brief Unsigned integer type. */
    using size_type = std::size_t;

    /**
     * @brief Constructs a prototype that is bound to a given registry.
     * @param ref A valid reference to a registry.
     */
    basic_prototype(registry_type &ref)
        : reg{&ref},
          entity{ref.create()}
    {}

    /**
     * @brief Releases all its resources.
     */
    ~basic_prototype() {
        release();
    }

    /**
     * @brief Move constructor.
     *
     * After prototype move construction, instances that have been moved from
     * are placed in a valid but unspecified state. It's highly discouraged to
     * continue using them.
     *
     * @param other The instance to move from.
     */
    basic_prototype(basic_prototype &&other)
        : handlers{std::move(other.handlers)},
          reg{other.reg},
          entity{other.entity}
    {
        other.entity = null;
    }

    /**
     * @brief Move assignment operator.
     *
     * After prototype move assignment, instances that have been moved from are
     * placed in a valid but unspecified state. It's highly discouraged to
     * continue using them.
     *
     * @param other The instance to move from.
     * @return This prototype.
     */
    basic_prototype & operator=(basic_prototype &&other) {
        if(this != &other) {
            auto tmp{std::move(other)};
            handlers.swap(tmp.handlers);
            std::swap(reg, tmp.reg);
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
        component_handler handler;

        handler.assign_or_replace = [](const basic_prototype &proto, registry_type &other, const Entity dst) {
            const auto &wrapper = proto.reg->template get<component_wrapper<Component>>(proto.entity);
            other.template assign_or_replace<Component>(dst, wrapper.component);
        };

        handler.assign = [](const basic_prototype &proto, registry_type &other, const Entity dst) {
            if(!other.template has<Component>(dst)) {
                const auto &wrapper = proto.reg->template get<component_wrapper<Component>>(proto.entity);
                other.template assign<Component>(dst, wrapper.component);
            }
        };

        handlers[reg->template type<Component>()] = handler;
        auto &wrapper = reg->template assign_or_replace<component_wrapper<Component>>(entity, Component{std::forward<Args>(args)...});
        return wrapper.component;
    }

    /**
     * @brief Removes the given component from a prototype.
     * @tparam Component Type of component to remove.
     */
    template<typename Component>
    void unset() ENTT_NOEXCEPT {
        reg->template reset<component_wrapper<Component>>(entity);
        handlers.erase(reg->template type<Component>());
    }

    /**
     * @brief Checks if a prototype owns all the given components.
     * @tparam Component Components for which to perform the check.
     * @return True if the prototype owns all the components, false otherwise.
     */
    template<typename... Component>
    bool has() const ENTT_NOEXCEPT {
        return reg->template has<component_wrapper<Component>...>(entity);
    }

    /**
     * @brief Returns references to the given components.
     *
     * @warning
     * Attempting to get a component from a prototype that doesn't own it
     * results in undefined behavior.<br/>
     * An assertion will abort the execution at runtime in debug mode if the
     * prototype doesn't own an instance of the given component.
     *
     * @tparam Component Types of components to get.
     * @return References to the components owned by the prototype.
     */
    template<typename... Component>
    decltype(auto) get() const ENTT_NOEXCEPT {
        if constexpr(sizeof...(Component) == 1) {
            return (std::as_const(*reg).template get<component_wrapper<Component...>>(entity).component);
        } else {
            return std::tuple<std::add_const_t<Component> &...>{get<Component>()...};
        }
    }

    /*! @copydoc get */
    template<typename... Component>
    inline decltype(auto) get() ENTT_NOEXCEPT {
        if constexpr(sizeof...(Component) == 1) {
            return (const_cast<Component &>(std::as_const(*this).template get<Component>()), ...);
        } else {
            return std::tuple<Component &...>{get<Component>()...};
        }
    }

    /**
     * @brief Returns pointers to the given components.
     * @tparam Component Types of components to get.
     * @return Pointers to the components owned by the prototype.
     */
    template<typename... Component>
    auto try_get() const ENTT_NOEXCEPT {
        if constexpr(sizeof...(Component) == 1) {
            const auto *wrapper = reg->template try_get<component_wrapper<Component...>>(entity);
            return wrapper ? &wrapper->component : nullptr;
        } else {
            return std::tuple<std::add_const_t<Component> *...>{try_get<Component>()...};
        }
    }

    /*! @copydoc try_get */
    template<typename... Component>
    inline auto try_get() ENTT_NOEXCEPT {
        if constexpr(sizeof...(Component) == 1) {
            return (const_cast<Component *>(std::as_const(*this).template try_get<Component>()), ...);
        } else {
            return std::tuple<Component *...>{try_get<Component>()...};
        }
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
        const auto entt = other.create();
        assign(other, entt);
        return entt;
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
        return create(*reg);
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
        assign(*reg, dst);
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
    void assign_or_replace(registry_type &other, const entity_type dst) const {
        for(auto &handler: handlers) {
            handler.second.assign_or_replace(*this, other, dst);
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
    inline void assign_or_replace(const entity_type dst) const {
        assign_or_replace(*reg, dst);
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
        assign(*reg, dst);
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
        return create(*reg);
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

private:
    std::unordered_map<component_type, component_handler> handlers;
    registry_type *reg;
    entity_type entity;
};


}


#endif // ENTT_ENTITY_PROTOTYPE_HPP

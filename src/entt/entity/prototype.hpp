#ifndef ENTT_ENTITY_PROTOTYPE_HPP
#define ENTT_ENTITY_PROTOTYPE_HPP


#include <tuple>
#include <memory>
#include <vector>
#include <utility>
#include <cstddef>
#include <algorithm>
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
 * Components used along with prototypes must be copy constructible.
 *
 * @tparam Entity A valid entity type (see entt_traits for more details).
 */
template<typename Entity>
class Prototype {
    using component_type = typename Registry<Entity>::component_type;
    using fn_type = void(*)(const void *, Registry<Entity> &, Entity);
    using deleter_type = void(*)(void *);
    using ptr_type = std::unique_ptr<void, deleter_type>;

    template<typename Component>
    static void accommodate(const void *component, Registry<Entity> &registry, Entity entity) {
        const auto &ref = *static_cast<const Component *>(component);
        registry.template accommodate<Component>(entity, ref);
    }

    template<typename Component>
    static void assign(const void *component, Registry<Entity> &registry, Entity entity) {
        if(!registry.template has<Component>(entity)) {
            const auto &ref = *static_cast<const Component *>(component);
            registry.template assign<Component>(entity, ref);
        }
    }

    struct Handler final {
        Handler(ptr_type component, fn_type accommodate, fn_type assign, component_type type)
            : component{std::move(component)},
              accommodate{accommodate},
              assign{assign},
              type{type}
        {}

        ptr_type component{nullptr, +[](void *) {}};
        fn_type accommodate{nullptr};
        fn_type assign{nullptr};
        component_type type;
    };

public:
    /*! @brief Registry type. */
    using registry_type = Registry<Entity>;
    /*! @brief Underlying entity identifier. */
    using entity_type = Entity;
    /*! @brief Unsigned integer type. */
    using size_type = std::size_t;

    /**
     * @brief Assigns to or replaces the given component of a prototype.
     * @tparam Component Type of component to assign or replace.
     * @tparam Args Types of arguments to use to construct the component.
     * @param args Parameters to use to initialize the component.
     * @return A reference to the newly created component.
     */
    template<typename Component, typename... Args>
    Component & set(Args &&... args) {
        const auto ctype = registry_type::template type<Component>();

        auto it = std::find_if(handlers.begin(), handlers.end(), [ctype](const auto &handler) {
            return handler.type == ctype;
        });

        const auto deleter = +[](void *component) { delete static_cast<Component *>(component); };
        ptr_type component{new Component{std::forward<Args>(args)...}, deleter};

        if(it == handlers.cend()) {
            handlers.emplace_back(std::move(component), &Prototype::accommodate<Component>, &Prototype::assign<Component>, ctype);
        } else {
            it->component = std::move(component);
        }

        return *static_cast<Component *>(component.get());
    }

    /**
     * @brief Removes the given component from a prototype.
     * @tparam Component Type of component to remove.
     */
    template<typename Component>
    void unset() ENTT_NOEXCEPT {
        handlers.erase(std::remove_if(handlers.begin(), handlers.end(), [](const auto &handler) {
            return handler.type == registry_type::template type<Component>();
        }), handlers.end());
    }

    /**
     * @brief Checks if a prototype owns all the given components.
     * @tparam Component Components for which to perform the check.
     * @return True if the prototype owns all the components, false otherwise.
     */
    template<typename... Component>
    bool has() const ENTT_NOEXCEPT {
        auto found = [this](const auto ctype) {
            return std::find_if(handlers.cbegin(), handlers.cend(), [ctype](const auto &handler) {
                return handler.type == ctype;
            }) != handlers.cend();
        };

        bool all = true;
        using accumulator_type = bool[];
        accumulator_type accumulator = { all, (all = all && found(registry_type::template type<Component>()))... };
        (void)accumulator;
        return all;
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
        assert(has<Component>());

        auto it = std::find_if(handlers.cbegin(), handlers.cend(), [](const auto &handler) {
            return handler.type == registry_type::template type<Component>();
        });

        return *static_cast<Component *>(it->component.get());
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
    std::enable_if_t<(sizeof...(Component) > 1), std::tuple<const Component &...>>
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
    std::enable_if_t<(sizeof...(Component) > 1), std::tuple<Component &...>>
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
     * @warning
     * Attempting to use an invalid entity results in undefined behavior.<br/>
     * An assertion will abort the execution at runtime in debug mode in case of
     * invalid entity.
     *
     * @param registry A valid reference to a registry.
     * @return A valid entity identifier.
     */
    entity_type create(registry_type &registry) {
        const auto entity = registry.create();
        assign(registry, entity);
        return entity;
    }

    /**
     * @brief Assigns the components of a prototype to a given entity.
     *
     * Assigning a prototype to an entity won't overwrite existing components
     * under any circumstances.<br/>
     * In other words, only those components that the entity doesn't own yet are
     * copied over. All the other components remain unchanged.
     *
     * @warning
     * Attempting to use an invalid entity results in undefined behavior.<br/>
     * An assertion will abort the execution at runtime in debug mode in case of
     * invalid entity.
     *
     * @param registry A valid reference to a registry.
     * @param entity A valid entity identifier.
     */
    void assign(registry_type &registry, entity_type entity) {
        std::for_each(handlers.begin(), handlers.end(), [&registry, entity](auto &&handler) {
            handler.assign(handler.component.get(), registry, entity);
        });
    }

    /**
     * @brief Assigns or replaces the components of a prototype for an entity.
     *
     * Existing components are overwritten, if any. All the other components
     * will be copied over to the target entity.
     *
     * @warning
     * Attempting to use an invalid entity results in undefined behavior.<br/>
     * An assertion will abort the execution at runtime in debug mode in case of
     * invalid entity.
     *
     * @param registry A valid reference to a registry.
     * @param entity A valid entity identifier.
     */
    void accommodate(registry_type &registry, entity_type entity) {
        std::for_each(handlers.begin(), handlers.end(), [&registry, entity](auto &&handler) {
            handler.accommodate(handler.component.get(), registry, entity);
        });
    }

    /**
     * @brief Assigns the components of a prototype to an entity.
     *
     * Assigning a prototype to an entity won't overwrite existing components
     * under any circumstances.<br/>
     * In other words, only the components that the entity doesn't own yet are
     * copied over. All the other components remain unchanged.
     *
     * @warning
     * Attempting to use an invalid entity results in undefined behavior.<br/>
     * An assertion will abort the execution at runtime in debug mode in case of
     * invalid entity.
     *
     * @param registry A valid reference to a registry.
     * @param entity A valid entity identifier.
     */
    inline void operator()(registry_type &registry, entity_type entity) ENTT_NOEXCEPT {
        assign(registry, entity);
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
     * @warning
     * Attempting to use an invalid entity results in undefined behavior.<br/>
     * An assertion will abort the execution at runtime in debug mode in case of
     * invalid entity.
     *
     * @param registry A valid reference to a registry.
     * @return A valid entity identifier.
     */
    inline entity_type operator()(registry_type &registry) ENTT_NOEXCEPT {
        return create(registry);
    }

private:
    std::vector<Handler> handlers;
};


/**
 * @brief Default prototype
 */
using DefaultPrototype = Prototype<uint32_t>;


}


#endif // ENTT_ENTITY_PROTOTYPE_HPP

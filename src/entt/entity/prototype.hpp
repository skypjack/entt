#ifndef ENTT_ENTITY_PROTOTYPE_HPP
#define ENTT_ENTITY_PROTOTYPE_HPP

#include "registry.hpp"

namespace entt {

/**
 * @brief A prototype entity for creating new entities
 *
 * Prototype provides a similar interface to the registry except that Prototype
 * stores a single entity. This entity is not apart of the registry so it is not
 * seen by views. The Prototype can be used to accommodate components to an
 * entity on the registry.
 *
 * Note that components stored in the Prototype must have copy constructors to
 * initialize an entity.
 *
 * @tparam Entity A valid entity type (see entt_traits for more details).
 */
template <typename Entity>
class Prototype {
public:
    using entity_t = Entity;
    using registry_t = entt::Registry<Entity>;
    using family_t = entt::Family<struct PrototypeFamily>;

private:
    struct StorageBase {
        virtual ~StorageBase() = default;
        virtual void accommodate(registry_t &, entity_t) const ENTT_NOEXCEPT = 0;
    };
    
    template <typename Component>
    struct Storage : StorageBase {
        void accommodate(registry_t &reg, const entity_t entity) const ENTT_NOEXCEPT override {
            reg.template accommodate<Component>(entity, comp);
        }
        
        Component comp;
    };

public:
    Prototype() = default;
    Prototype(Prototype &&) = default;
    Prototype &operator=(Prototype &&) = default;

    /**
     * @brief Checks if there exists at least one component assigned
     *
     * @return True if no components are assigned
     */
    bool empty() const ENTT_NOEXCEPT {
        return std::all_of(comps.cbegin(), comps.cend(), [] (auto comp) {
            return comp == nullptr;
        });
    }

    /**
     * @brief Accommodate copies of the components to the given entity
     */
    void operator()(registry_t &reg, const entity_t entity) const ENTT_NOEXCEPT {
        for (const std::unique_ptr<StorageBase> &component : comps) {
            if (component) {
                component->accommodate(reg, entity);
            }
        }
    }
    /**
     * @brief Create an new entity assign copies of the components to it
     *
     * @return Newly created entity
     */
    entity_t operator()(registry_t &reg) const ENTT_NOEXCEPT {
        const entity_t entity = reg.create();
        (*this)(reg, entity);
        return entity;
    }

    /**
     * @brief Returns the numeric identifier of a type of component at runtime
     *
     * The component doesn't need to be assigned to the prototype for this
     * function to return a valid ID. The IDs are not synced with the registry.
     *
     * @tparam Component Type of component to query.
     * @return Runtime numeric identifier of the given type of component
     */
    template <typename Component>
    size_t type() const ENTT_NOEXCEPT {
        return family_t::template type<Component>();
    }

    /**
     * @brief Assigns the given component to the prototype
     *
     * @warning
     * An assertion will abort the execution at runtime in debug mode in case
     * the prototype already owns the given component
     *
     * @tparam Component Type of component to create
     * @tparam Args Types of arguments to use to construct the component.
     * @param args Parameters to use to initialize the component.
     * @return A reference to the newly created component.
     */
    template <typename Component, typename... Args>
    Component &assign(Args &&... args) {
        assert(!has<Component>());
        auto component = std::make_unique<Storage<Component>>();
        component->comp = Component {std::forward<Args>(args)...};
        const size_t index = type<Component>();
        while (comps.size() <= index) {
            comps.emplace_back();
        }
        Component &comp = component->comp;
        comps[index] = std::move(component);
        return comp;
    }
    
    /**
     * @brief Removes the given component from the prototype
     *
     * @warning
     * An assertion will abort the execution at runtime in debug mode in case
     * the prototype doesn't own the given component
     *
     * @tparam Component Type of component to remove
     */
    template <typename Component>
    void remove() ENTT_NOEXCEPT {
        assert(has<Component>());
        comps[type<Component>()] = nullptr;
    }
    
    /**
     * @brief Checks if the prototype has all the given components
     *
     * @tparam Components Components that the prototype must own
     * @return True if the prototype owns all of the components, false otherwise.
     */
    template <typename... Components>
    bool has() const ENTT_NOEXCEPT {
        bool all = true;
        [[maybe_unused]]
        bool acc[] = {(all = all && type<Components>() < comps.size() && comps[type<Components>()] != nullptr)...};
        return all;
    }
    
    /**
     * @brief Returns a const reference to the given component
     *
     * @warning
     * An assertion will abort the execution at runtime in debug mode in case
     * the prototype doesn't own the given component
     *
     * @tparam Component Type of component to get
     * @return A reference to the component
     */
    template <typename Component>
    const Component &get() const ENTT_NOEXCEPT {
        assert(has<Component>());
        return static_cast<Storage<Component> *>(comps[type<Component>()].get())->comp;
    }
    
    /**
     * @brief Returns a reference to the given component
     *
     * @warning
     * An assertion will abort the execution at runtime in debug mode in case
     * the prototype doesn't own the given component
     *
     * @tparam Component Type of component to get
     * @return A reference to the component
     */
    template <typename Component>
    Component &get() ENTT_NOEXCEPT {
        assert(has<Component>());
        return static_cast<Storage<Component> *>(comps[type<Component>()].get())->comp;
    }
    
    /**
     * @brief Replaces the given component
     *
     * @warning
     * An assertion will abort the execution at runtime in debug mode in case
     * the prototype doesn't own the given component
     *
     * @tparam Component Type of component to replace
     * @tparam Args Types of arguments to use to construct the component
     * @param args Parameters to use to initialize the component
     * @return A refernce to the newly created component
     */
    template <typename Component, typename... Args>
    Component &replace(Args &&... args) ENTT_NOEXCEPT {
        return (get<Component>() = Component{std::forward<Args>(args)...});
    }
    
    /**
     * @brief Assigns of replaces the given component
     *
     * @tparam Component Type of component to replace
     * @tparam Args Types of arguments to use to construct the component
     * @param args Parameters to use to initialize the component
     * @return A refernce to the newly created component
     */
    template <typename Component, typename... Args>
    Component &accommodate(Args &&... args) ENTT_NOEXCEPT {
        if (has<Component>()) {
            return replace<Component>(std::forward<Args>(args)...);
        } else {
            return assign<Component>(std::forward<Args>(args)...);
        }
    }
    
private:
    std::vector<std::unique_ptr<StorageBase>> comps;
};

/**
 * @brief Default prototype
 */
using DefaultPrototype = Prototype<uint32_t>;

}

#endif

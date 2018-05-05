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
  class ComponentBase {
  public:
    virtual ~ComponentBase() = default;
    virtual void accommodate(registry_t &, entity_t) const ENTT_NOEXCEPT = 0;
  };
  
  template <typename Comp>
  class Component : public ComponentBase {
  public:
    void accommodate(registry_t &reg, const entity_t entity) const ENTT_NOEXCEPT override {
      reg.template accommodate<Comp>(entity, comp);
    }
    
    Comp comp;
  };

public:
  Prototype() = default;
  Prototype(Prototype &&) = default;
  Prototype &operator=(Prototype &&) = default;

  /**
   * @brief Checks if there exists at least one component assigned
   *
   * @return True if at least one component is assigned
   */
  bool empty() const ENTT_NOEXCEPT {
    return comps.empty();
  }

  /**
   * @brief Accommodate copies of the components to the given entity
   */
  void operator()(registry_t &reg, const entity_t entity) const ENTT_NOEXCEPT {
    for (const std::unique_ptr<ComponentBase> &component : comps) {
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
   * @tparam Comp Type of component to query.
   * @return Runtime numeric identifier of the given type of component
   */
  template <typename Comp>
  size_t type() const ENTT_NOEXCEPT {
    return family_t::template type<Comp>();
  }

  /**
   * @brief Assigns the given component to the prototype
   *
   * @warning
   * An assertion will abort the execution at runtime in debug mode in case
   * the prototype already owns the given component
   *
   * @tparam Comp Type of component to create
   * @tparam Args Types of arguments to use to construct the component.
   * @return A reference to the newly created component.
   */
  template <typename Comp, typename ...Args>
  Comp &assign(Args &&... args) {
    assert(!has<Comp>());
    auto component = std::make_unique<Component<Comp>>();
    component->comp = Comp {std::forward<Args>(args)...};
    const size_t index = type<Comp>();
    while (comps.size() <= index) {
      comps.emplace_back();
    }
    Comp &comp = component->comp;
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
   * @tparam Comp Type of component to remove
   */
  template <typename Comp>
  void remove() ENTT_NOEXCEPT {
    assert(has<Comp>());
    comps[type<Comp>()] = nullptr;
  }
  
  /**
   * @brief Checks if the prototype has all the given components
   *
   * @tparam Comp Components that the prototype must own
   * @return True if the prototype owns all of the components, false otherwise.
   */
  template <typename... Comp>
  bool has() const ENTT_NOEXCEPT {
    bool all = true;
    [[maybe_unused]]
    bool acc[] = {(all = all && type<Comp>() < comps.size() && comps[type<Comp>()] != nullptr)...};
    return all;
  }
  
  /**
   * @brief Returns a const reference to the given component
   *
   * @warning
   * An assertion will abort the execution at runtime in debug mode in case
   * the prototype doesn't own the given component
   *
   * @tparam Comp Type of component to get
   * @return A reference to the component
   */
  template <typename Comp>
  const Comp &get() const ENTT_NOEXCEPT {
    assert(has<Comp>());
    return static_cast<Component<Comp> *>(comps[type<Comp>()].get())->comp;
  }
  
  /**
   * @brief Returns a reference to the given component
   *
   * @warning
   * An assertion will abort the execution at runtime in debug mode in case
   * the prototype doesn't own the given component
   *
   * @tparam Comp Type of component to get
   * @return A reference to the component
   */
  template <typename Comp>
  Comp &get() ENTT_NOEXCEPT {
    assert(has<Comp>());
    return static_cast<Component<Comp> *>(comps[type<Comp>()].get())->comp;
  }
  
private:
  std::vector<std::unique_ptr<ComponentBase>> comps;
};

}

#endif

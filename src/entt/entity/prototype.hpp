#ifndef ENTT_ENTITY_PROTOTYPE_HPP
#define ENTT_ENTITY_PROTOTYPE_HPP

#include "registry.hpp"

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
    virtual void assign(registry_t &, entity_t) const = 0;
  };
  
  template <typename Comp>
  class Component : public ComponentBase {
  public:
    void assign(registry_t &registry, const entity_t entity) const override {
      registry.template accommodate<Comp>(entity, comp);
    }
    
    Comp comp;
  };

public:
  Prototype() = default;
  Prototype(Prototype &&) = default;
  Prototype &operator=(Prototype &&) = default;

  bool empty() const ENTT_NOEXCEPT {
    return comps.empty();
  }

  void operator()(registry_t &reg, const entity_t entity) const ENTT_NOEXCEPT {
    for (const std::unique_ptr<ComponentBase> &component : comps) {
      if (component) {
        component->assign(reg, entity);
      }
    }
  }
  entity_t operator()(registry_t &reg) const ENTT_NOEXCEPT {
    const entity_t entity = reg.create();
    (*this)(reg, entity);
    return entity;
  }

  template <typename Comp>
  size_t type() const ENTT_NOEXCEPT {
    return family_t::template type<Comp>();
  }

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
  
  template <typename Comp>
  void remove() ENTT_NOEXCEPT {
    assert(has<Comp>());
    comps[type<Comp>()] = nullptr;
  }
  
  template <typename Comp>
  bool has() const ENTT_NOEXCEPT {
    const size_t index = type<Comp>();
    return index < comps.size() && comps[index] != nullptr;
  }
  
  template <typename Comp>
  const Comp &get() const ENTT_NOEXCEPT {
    assert(has<Comp>());
    return static_cast<Component<Comp> *>(comps[type<Comp>()].get())->comp;
  }
  
  template <typename Comp>
  Comp &get() ENTT_NOEXCEPT {
    assert(has<Comp>());
    return static_cast<Component<Comp> *>(comps[type<Comp>()].get())->comp;
  }
  
private:
  std::vector<std::unique_ptr<ComponentBase>> comps;
};

#endif

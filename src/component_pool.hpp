#ifndef ENTT_COMPONENT_POOL_HPP
#define ENTT_COMPONENT_POOL_HPP


#include <utility>
#include <vector>
#include <cassert>


namespace entt {


template<typename, typename, typename...>
class ComponentPool;


template<typename Entity, typename Component>
class ComponentPool<Entity, Component> {
public:
    using component_type = Component;
    using entity_type = Entity;
    using pos_type = entity_type;
    using size_type = typename std::vector<component_type>::size_type;

private:
    bool valid(entity_type entity) const noexcept {
        return entity < reverse.size() && reverse[entity] < direct.size() && direct[reverse[entity]] == entity;
    }

public:
    explicit ComponentPool(size_type dim = 4098) noexcept {
        assert(!(dim < 0));
        data.reserve(dim);
    }

    ComponentPool(ComponentPool &&) = default;

    ~ComponentPool() noexcept {
        assert(empty());
    }

    ComponentPool & operator=(ComponentPool &&) = default;

    bool empty() const noexcept {
        return data.empty();
    }

    size_type capacity() const noexcept {
        return data.capacity();
    }

    size_type size() const noexcept {
        return data.size();
    }

    const entity_type * entities() const noexcept {
        return direct.data();
    }

    bool has(entity_type entity) const noexcept {
        return valid(entity);
    }

    const component_type & get(entity_type entity) const noexcept {
        assert(valid(entity));
        return data[reverse[entity]];
    }

    component_type & get(entity_type entity) noexcept {
        return const_cast<component_type &>(const_cast<const ComponentPool *>(this)->get(entity));
    }

    template<typename... Args>
    component_type & construct(entity_type entity, Args... args) {
        assert(!valid(entity));

        if(!(entity < reverse.size())) {
            reverse.resize(entity+1);
        }

        reverse[entity] = direct.size();
        direct.emplace_back(entity);
        data.push_back({ args... });

        return data.back();
    }

    void destroy(entity_type entity) {
        assert(valid(entity));

        auto last = direct.size() - 1;

        reverse[direct[last]] = reverse[entity];
        direct[reverse[entity]] = direct[last];
        data[reverse[entity]] = std::move(data[last]);

        direct.pop_back();
        data.pop_back();
    }

    void reset() {
        data.clear();
        reverse.resize(0);
        direct.clear();
    }

private:
    std::vector<component_type> data;
    std::vector<pos_type> reverse;
    std::vector<entity_type> direct;
};


template<typename Entity, typename Component, typename... Components>
class ComponentPool
        : ComponentPool<Entity, Component>, ComponentPool<Entity, Components>...
{
    template<typename Comp>
    using Pool = ComponentPool<Entity, Comp>;

public:
    using entity_type = typename Pool<Component>::entity_type;
    using pos_type = typename Pool<Component>::pos_type;
    using size_type = typename Pool<Component>::size_type;

    explicit ComponentPool(size_type dim = 4098) noexcept
        : Pool<Component>{dim}, Pool<Components>{dim}...
    {
        assert(!(dim < 0));
    }

    ComponentPool(const ComponentPool &) = delete;
    ComponentPool(ComponentPool &&) = delete;

    ComponentPool & operator=(const ComponentPool &) = delete;
    ComponentPool & operator=(ComponentPool &&) = delete;

    template<typename Comp>
    bool empty() const noexcept {
        return Pool<Comp>::empty();
    }

    template<typename Comp>
    size_type capacity() const noexcept {
        return Pool<Comp>::capacity();
    }

    template<typename Comp>
    size_type size() const noexcept {
        return Pool<Comp>::size();
    }

    template<typename Comp>
    const entity_type * entities() const noexcept {
        return Pool<Comp>::entities();
    }

    template<typename Comp>
    bool has(entity_type entity) const noexcept {
        return Pool<Comp>::has(entity);
    }

    template<typename Comp>
    const Comp & get(entity_type entity) const noexcept {
        return Pool<Comp>::get(entity);
    }

    template<typename Comp>
    Comp & get(entity_type entity) noexcept {
        return const_cast<Comp &>(const_cast<const ComponentPool *>(this)->get<Comp>(entity));
    }

    template<typename Comp, typename... Args>
    Comp & construct(entity_type entity, Args... args) {
        return Pool<Comp>::construct(entity, args...);
    }

    template<typename Comp>
    void destroy(entity_type entity) {
        Pool<Comp>::destroy(entity);
    }

    template<typename Comp>
    void reset() {
        Pool<Comp>::reset();
    }

    void reset() {
        using accumulator_type = int[];
        Pool<Component>::reset();
        accumulator_type accumulator = { (Pool<Components>::reset(), 0)... };
        (void)accumulator;
    }
};


}


#endif // ENTT_COMPONENT_POOL_HPP

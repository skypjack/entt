#ifndef ENTT_COMPONENT_POOL_HPP
#define ENTT_COMPONENT_POOL_HPP


#include <type_traits>
#include <utility>
#include <vector>
#include <tuple>
#include <memory>
#include <cstddef>
#include <cassert>
#include <algorithm>


namespace entt {


template<typename, typename, typename...>
class ComponentPool;


template<typename Entity, typename Component>
class ComponentPool<Entity, Component> {
public:
    using component_type = Component;
    using size_type = std::uint32_t;
    using entity_type = Entity;

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
        return static_cast<size_type>(data.capacity());
    }

    size_type size() const noexcept {
        return static_cast<size_type>(data.size());
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

        reverse[entity] = static_cast<size_type>(direct.size());
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
    std::vector<size_type> reverse;
    std::vector<entity_type> direct;
};


template<typename Entity, typename Component, typename... Components>
class ComponentPool {
    template<typename Comp>
    using Pool = ComponentPool<Entity, Comp>;

public:
    using size_type = typename Pool<Component>::size_type;
    using entity_type = typename Pool<Component>::entity_type;

    explicit ComponentPool(size_type dim = 4098) noexcept
#ifdef _MSC_VER
        : pools(Pool<Component>{dim}, Pool<Components>{dim}...)
#else
        : pools{Pool<Component>{dim}, Pool<Components>{dim}...}
#endif
    {
        assert(!(dim < 0));
    }

    ComponentPool(const ComponentPool &) = delete;
    ComponentPool(ComponentPool &&) = delete;

    ComponentPool & operator=(const ComponentPool &) = delete;
    ComponentPool & operator=(ComponentPool &&) = delete;

    template<typename Comp>
    bool empty() const noexcept {
        return std::get<Pool<Comp>>(pools).empty();
    }

    template<typename Comp>
    size_type capacity() const noexcept {
        return std::get<Pool<Comp>>(pools).capacity();
    }

    template<typename Comp>
    size_type size() const noexcept {
        return std::get<Pool<Comp>>(pools).size();
    }

    template<typename Comp>
    const entity_type * entities() const noexcept {
        return std::get<Pool<Comp>>(pools).entities();
    }

    template<typename Comp>
    bool has(entity_type entity) const noexcept {
        return std::get<Pool<Comp>>(pools).has(entity);
    }

    template<typename Comp>
    const Comp & get(entity_type entity) const noexcept {
        return std::get<Pool<Comp>>(pools).get(entity);
    }

    template<typename Comp>
    Comp & get(entity_type entity) noexcept {
        return const_cast<Comp &>(const_cast<const ComponentPool *>(this)->get<Comp>(entity));
    }

    template<typename Comp, typename... Args>
    Comp & construct(entity_type entity, Args... args) {
        return std::get<Pool<Comp>>(pools).construct(entity, args...);
    }

    template<typename Comp>
    void destroy(entity_type entity) {
        std::get<Pool<Comp>>(pools).destroy(entity);
    }

    template<typename Comp>
    void reset() {
        std::get<Pool<Comp>>(pools).reset();
    }

    void reset() {
        using accumulator_type = int[];
        std::get<Pool<Component>>(pools).reset();
        accumulator_type accumulator = { (std::get<Pool<Components>>(pools).reset(), 0)... };
        (void)accumulator;
    }

private:
    std::tuple<Pool<Component>, Pool<Components>...> pools;
};


}


#endif // ENTT_COMPONENT_POOL_HPP

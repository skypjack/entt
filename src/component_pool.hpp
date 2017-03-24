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


template<typename, typename...>
struct ComponentPool;


template<typename Component>
struct ComponentPool<Component> final {
    using component_type = Component;
    using size_type = std::uint32_t;
    using entity_type = std::uint32_t;

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
    component_type & construct(entity_type entity, Args&&... args) {
        assert(!valid(entity));

        if(!(entity < reverse.size())) {
            reverse.resize(entity+1);
        }

        reverse[entity] = direct.size();
        direct.emplace_back(entity);
        data.push_back({ std::forward<Args>(args)... });

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


template<typename Component, typename... Components>
struct  ComponentPool final {
    using size_type = typename ComponentPool<Component>::size_type;
    using entity_type = typename ComponentPool<Component>::entity_type;

    explicit ComponentPool(size_type dim = 4098) noexcept
        : pools{ComponentPool<Component>{dim}, ComponentPool<Components>{dim}...}
    {
        assert(!(dim < 0));
    }

    ComponentPool(const ComponentPool &) = delete;
    ComponentPool(ComponentPool &&) = delete;

    ComponentPool & operator=(const ComponentPool &) = delete;
    ComponentPool & operator=(ComponentPool &&) = delete;

    template<typename Comp>
    bool empty() const noexcept {
        return std::get<ComponentPool<Comp>>(pools).empty();
    }

    template<typename Comp>
    size_type capacity() const noexcept {
        return std::get<ComponentPool<Comp>>(pools).capacity();
    }

    template<typename Comp>
    size_type size() const noexcept {
        return std::get<ComponentPool<Comp>>(pools).size();
    }

    template<typename Comp>
    const entity_type * entities() const noexcept {
        return std::get<ComponentPool<Comp>>(pools).entities();
    }

    template<typename Comp>
    bool has(entity_type entity) const noexcept {
        return std::get<ComponentPool<Comp>>(pools).has(entity);
    }

    template<typename Comp>
    const Comp & get(entity_type entity) const noexcept {
        return std::get<ComponentPool<Comp>>(pools).get(entity);
    }

    template<typename Comp>
    Comp & get(entity_type entity) noexcept {
        return const_cast<Comp &>(const_cast<const ComponentPool *>(this)->get<Comp>(entity));
    }

    template<typename Comp, typename... Args>
    Comp & construct(entity_type entity, Args&&... args) {
        return std::get<ComponentPool<Comp>>(pools).construct(entity, std::forward<Args>(args)...);
    }

    template<typename Comp>
    void destroy(entity_type entity) {
        std::get<ComponentPool<Comp>>(pools).destroy(entity);
    }

    void reset() {
        using accumulator_type = int[];
        std::get<ComponentPool<Component>>(pools).reset();
        accumulator_type accumulator = { (std::get<ComponentPool<Components>>(pools).reset(), 0)... };
        (void)accumulator;
    }

private:
    std::tuple<ComponentPool<Component>, ComponentPool<Components>...> pools;
};


}


#endif // ENTT_COMPONENT_POOL_HPP

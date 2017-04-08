#ifndef ENTT_REGISTRY_HPP
#define ENTT_REGISTRY_HPP


#include <cassert>
#include <memory>
#include <vector>
#include <utility>
#include <cstddef>
#include <iterator>
#include <algorithm>
#include "component_pool.hpp"


namespace entt {


template<typename...>
class View;


template<template<typename...> class Pool, typename... Components, typename Type, typename... Types>
class View<Pool<Components...>, Type, Types...> final {
    using pool_type = Pool<Components...>;
    using entity_type = typename pool_type::entity_type;

    class ViewIterator {
        bool valid() const noexcept {
            using accumulator_type = bool[];
            bool check = pool.template has<Type>(entities[pos-1]);
            accumulator_type accumulator = { true, (check = check && pool.template has<Types>(entities[pos-1]))... };
            (void)accumulator;
            return check;
        }

    public:
        using value_type = entity_type;
        using difference_type = std::ptrdiff_t;
        using reference = entity_type &;
        using pointer = entity_type *;
        using iterator_category = std::input_iterator_tag;

        ViewIterator(pool_type &pool, const entity_type *entities, typename pool_type::size_type pos) noexcept
            : pool{pool}, entities{entities}, pos{pos}
        {
            if(this->pos) { while(!valid() && --this->pos); }
        }

        ViewIterator & operator++() noexcept {
            if(pos) { while(--pos && !valid()); }
            return *this;
        }

        ViewIterator operator++(int) noexcept {
            ViewIterator orig = *this;
            return this->operator++(), orig;
        }

        bool operator==(const ViewIterator &other) const noexcept {
            return other.entities == entities && other.pos == pos;
        }

        bool operator!=(const ViewIterator &other) const noexcept {
            return !(*this == other);
        }

        value_type operator*() const noexcept {
            return *(entities+pos-1);
        }

    private:
        pool_type &pool;
        const entity_type *entities;
        std::uint32_t pos;
    };

    template<typename Comp>
    void prefer() noexcept {
        auto sz = pool.template size<Comp>();

        if(sz < size) {
            entities = pool.template entities<Comp>();
            size = sz;
        }
    }

public:
    using iterator_type = ViewIterator;
    using size_type = typename pool_type::size_type;

    explicit View(pool_type &pool) noexcept
        : entities{pool.template entities<Type>()},
          size{pool.template size<Type>()},
          pool{pool}
    {
        using accumulator_type = int[];
        accumulator_type accumulator = { 0, (prefer<Types>(), 0)... };
        (void)accumulator;
    }

    iterator_type begin() const noexcept {
        return ViewIterator{pool, entities, size};
    }

    iterator_type end() const noexcept {
        return ViewIterator{pool, entities, 0};
    }

    void reset() noexcept {
        using accumulator_type = int[];
        entities = pool.template entities<Type>();
        size = pool.template size<Type>();
        accumulator_type accumulator = { 0, (prefer<Types>(), 0)... };
        (void)accumulator;
    }

private:
    const entity_type *entities;
    size_type size;
    pool_type &pool;
};


template<template<typename...> class Pool, typename... Components, typename Type>
class View<Pool<Components...>, Type> final {
    using pool_type = Pool<Components...>;
    using entity_type = typename pool_type::entity_type;

    struct ViewIterator {
        using value_type = entity_type;
        using difference_type = std::ptrdiff_t;
        using reference = entity_type &;
        using pointer = entity_type *;
        using iterator_category = std::input_iterator_tag;

        ViewIterator(const entity_type *entities, typename pool_type::size_type pos) noexcept
            : entities{entities}, pos{pos}
        {}

        ViewIterator & operator++() noexcept {
            --pos;
            return *this;
        }

        ViewIterator operator++(int) noexcept {
            ViewIterator orig = *this;
            return this->operator++(), orig;
        }

        bool operator==(const ViewIterator &other) const noexcept {
            return other.entities == entities && other.pos == pos;
        }

        bool operator!=(const ViewIterator &other) const noexcept {
            return !(*this == other);
        }

        value_type operator*() const noexcept {
            return *(entities+pos-1);
        }

    private:
        const entity_type *entities;
        typename pool_type::size_type pos;
    };

public:
    using iterator_type = ViewIterator;
    using size_type = typename pool_type::size_type;

    explicit View(pool_type &pool) noexcept: pool{pool} {}

    iterator_type begin() const noexcept {
        return ViewIterator{pool.template entities<Type>(), pool.template size<Type>()};
    }

    iterator_type end() const noexcept {
        return ViewIterator{pool.template entities<Type>(), 0};
    }

    size_type size() const noexcept {
        return pool.template size<Type>();
    }

private:
    pool_type &pool;
};


template<typename>
struct Registry;


template<template<typename...> class Pool, typename... Components>
struct Registry<Pool<Components...>> final {
    static_assert(sizeof...(Components) > 1, "!");

    using entity_type = typename Pool<Components...>::entity_type;
    using size_type = std::size_t;

private:
    using pool_type = Pool<Components...>;

    template<typename Comp>
    void destroy(entity_type entity) {
        if(pool.template has<Comp>(entity)) {
            pool.template destroy<Comp>(entity);
        }
    }

    template<typename Comp>
    void clone(entity_type from, entity_type to) {
        if(pool.template has<Comp>(from)) {
            pool.template construct<Comp>(to, pool.template get<Comp>(from));
        }
    }

    template<typename Comp>
    void sync(entity_type from, entity_type to) {
        bool src = pool.template has<Comp>(from);
        bool dst = pool.template has<Comp>(to);

        if(src && dst) {
            copy<Comp>(from, to);
        } else if(src) {
            clone<Comp>(from, to);
        } else if(dst) {
            destroy(to);
        }
    }

public:
    template<typename... Comp>
    using view_type = View<pool_type, Comp...>;

    template<typename... Args>
    Registry(Args&&... args)
        : count{0}, pool{std::forward<Args>(args)...}
    {}

    Registry(const Registry &) = delete;
    Registry(Registry &&) = delete;

    Registry & operator=(const Registry &) = delete;
    Registry & operator=(Registry &&) = delete;

    size_type size() const noexcept {
        return count - available.size();
    }

    size_type capacity() const noexcept {
        return count;
    }

    template<typename Comp>
    bool empty() const noexcept {
        return pool.template empty<Comp>();
    }

    bool empty() const noexcept {
        return available.size() == count;
    }

    template<typename... Comp>
    entity_type create() noexcept {
        using accumulator_type = int[];
        auto entity = create();
        accumulator_type accumulator = { 0, (assign<Comp>(entity), 0)... };
        (void)accumulator;
        return entity;
    }

    entity_type create() noexcept {
        entity_type entity;

        if(available.empty()) {
            entity = count++;
        } else {
            entity = available.back();
            available.pop_back();
        }

        return entity;
    }

    void destroy(entity_type entity) {
        using accumulator_type = int[];
        accumulator_type accumulator = { 0, (destroy<Components>(entity), 0)... };
        (void)accumulator;
        available.push_back(entity);
    }

    template<typename Comp, typename... Args>
    Comp & assign(entity_type entity, Args... args) {
        return pool.template construct<Comp>(entity, args...);
    }

    template<typename Comp>
    void remove(entity_type entity) {
        pool.template destroy<Comp>(entity);
    }

    template<typename Comp>
    bool has(entity_type entity) const noexcept {
        return pool.template has<Comp>(entity);
    }

    template<typename Comp>
    const Comp & get(entity_type entity) const noexcept {
        return pool.template get<Comp>(entity);
    }

    template<typename Comp>
    Comp & get(entity_type entity) noexcept {
        return pool.template get<Comp>(entity);
    }

    template<typename Comp, typename... Args>
    void replace(entity_type entity, Args... args) {
        pool.template get<Comp>(entity) = Comp{args...};
    }

    entity_type clone(entity_type from) {
        auto to = create();
        using accumulator_type = int[];
        accumulator_type accumulator = { 0, (clone<Components>(from, to), 0)... };
        (void)accumulator;
        return to;
    }

    template<typename Comp>
    void copy(entity_type from, entity_type to) {
        pool.template get<Comp>(to) = pool.template get<Comp>(from);
    }

    void copy(entity_type from, entity_type to) {
        using accumulator_type = int[];
        accumulator_type accumulator = { 0, (sync<Components>(from, to), 0)... };
        (void)accumulator;
    }

    template<typename Comp>
    void reset() {
        pool.template reset<Comp>();
    }

    void reset() {
        available.clear();
        count = 0;
        pool.reset();
    }

    template<typename... Comp>
    view_type<Comp...> view() {
        return view_type<Comp...>{pool};
    }

private:
    std::vector<entity_type> available;
    entity_type count;
    pool_type pool;
};


template<typename... Components>
using DefaultRegistry = Registry<ComponentPool<Components...>>;


}


#endif // ENTT_REGISTRY_HPP

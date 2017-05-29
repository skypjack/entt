#ifndef ENTT_REGISTRY_HPP
#define ENTT_REGISTRY_HPP


#include <tuple>
#include <vector>
#include <utility>
#include <cstddef>
#include <iterator>
#include "component_pool.hpp"


namespace entt {


template<typename...>
class View;


template<template<typename...> class Pool, typename Entity, typename... Components, typename Type, typename... Types, typename... Filters>
class View<Pool<Entity, Components...>, std::tuple<Type, Types...>, std::tuple<Filters...>> final {
    using pool_type = Pool<Entity, Components...>;
    using entity_type = typename pool_type::entity_type;

    class ViewIterator {
        bool valid() const noexcept {
            using accumulator_type = bool[];
            bool check = pool.template has<Type>(entities[pos-1]);
            accumulator_type types = { true, (check = check && pool.template has<Types>(entities[pos-1]))... };
            accumulator_type filters = { true, (check = check && not pool.template has<Filters>(entities[pos-1]))... };
            return void(types), void(filters), check;
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
        typename pool_type::size_type pos;
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

    template<typename... Comp>
    using view_type = View<pool_type, std::tuple<Type, Types...>, std::tuple<Comp...>>;

    explicit View(pool_type &pool) noexcept
        : entities{pool.template entities<Type>()},
          size{pool.template size<Type>()},
          pool{pool}
    {
        using accumulator_type = int[];
        accumulator_type accumulator = { 0, (prefer<Types>(), 0)... };
        (void)accumulator;
    }

    template<typename... Comp>
    view_type<Comp...> exclude() noexcept {
        return view_type<Comp...>{pool};
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


template<template<typename...> class Pool, typename Entity, typename... Components, typename Type>
class View<Pool<Entity, Components...>, std::tuple<Type>, std::tuple<>> final {
    using pool_type = Pool<Entity, Components...>;
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

    template<typename... Comp>
    using view_type = View<pool_type, std::tuple<Type>, std::tuple<Comp...>>;

    explicit View(pool_type &pool) noexcept: pool{pool} {}

    template<typename... Comp>
    view_type<Comp...> exclude() noexcept {
        return view_type<Comp...>{pool};
    }

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
class Registry;


template<template<typename...> class Pool, typename Entity, typename... Components>
class Registry<Pool<Entity, Components...>> {
    using pool_type = Pool<Entity, Components...>;

public:
    static_assert(sizeof...(Components) > 1, "!");

    using entity_type = typename pool_type::entity_type;
    using size_type = typename std::vector<entity_type>::size_type;

private:
    template<typename Comp>
    void destroy(entity_type entity) {
        if(pool.template has<Comp>(entity)) {
            pool.template destroy<Comp>(entity);
        }
    }

    template<typename Comp>
    void clone(entity_type to, entity_type from) {
        if(pool.template has<Comp>(from)) {
            pool.template construct<Comp>(to, pool.template get<Comp>(from));
        }
    }

    template<typename Comp>
    void sync(entity_type to, entity_type from) {
        bool src = pool.template has<Comp>(from);
        bool dst = pool.template has<Comp>(to);

        if(src && dst) {
            copy<Comp>(to, from);
        } else if(src) {
            clone<Comp>(to, from);
        } else if(dst) {
            destroy(to);
        }
    }

public:
    template<typename... Comp>
    using view_type = View<pool_type, std::tuple<Comp...>, std::tuple<>>;

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

    template<typename... Comp>
    bool has(entity_type entity) const noexcept {
        using accumulator_type = bool[];
        bool all = true;
        accumulator_type accumulator = { true, (all = all && pool.template has<Comp>(entity))... };
        (void)accumulator;
        return all;
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
    Comp & replace(entity_type entity, Args... args) {
        return (pool.template get<Comp>(entity) = Comp{args...});
    }

    template<typename Comp, typename... Args>
    Comp & accomodate(entity_type entity, Args... args) {
        return (pool.template has<Comp>(entity)
                ? this->template replace<Comp>(entity, std::forward<Args>(args)...)
                : this->template assign<Comp>(entity, std::forward<Args>(args)...));
    }

    entity_type clone(entity_type from) {
        auto to = create();
        using accumulator_type = int[];
        accumulator_type accumulator = { 0, (clone<Components>(to, from), 0)... };
        (void)accumulator;
        return to;
    }

    template<typename Comp>
    Comp & copy(entity_type to, entity_type from) {
        return (pool.template get<Comp>(to) = pool.template get<Comp>(from));
    }

    void copy(entity_type to, entity_type from) {
        using accumulator_type = int[];
        accumulator_type accumulator = { 0, (sync<Components>(to, from), 0)... };
        (void)accumulator;
    }

    template<typename Comp>
    void reset(entity_type entity) {
        if(pool.template has<Comp>(entity)) {
            pool.template destroy<Comp>(entity);
        }
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
    view_type<Comp...> view() noexcept {
        return view_type<Comp...>{pool};
    }

private:
    std::vector<entity_type> available;
    entity_type count;
    pool_type pool;
};


template<typename Entity, typename... Components>
using StandardRegistry = Registry<ComponentPool<Entity, Components...>>;


template<typename... Components>
using DefaultRegistry = Registry<ComponentPool<std::uint32_t, Components...>>;


}


#endif // ENTT_REGISTRY_HPP

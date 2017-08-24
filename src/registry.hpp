#ifndef ENTT_REGISTRY_HPP
#define ENTT_REGISTRY_HPP


#include <vector>
#include <bitset>
#include <utility>
#include <cstddef>
#include <iterator>
#include <cassert>
#include <type_traits>
#include "component_pool.hpp"
#include "ident.hpp"


namespace entt {


template<typename...>
class View;


template<template<typename...> class Pool, typename Entity, typename... Components, typename Type, typename... Types>
class View<Pool<Entity, Components...>, Type, Types...> final {
    using pool_type = Pool<Entity, Components...>;
    using entity_type = typename pool_type::entity_type;
    using mask_type = std::bitset<sizeof...(Components)+1>;
    using underlying_iterator_type = typename pool_type::const_iterator_type;

    class ViewIterator;

public:
    using iterator_type = ViewIterator;
    using const_iterator_type = iterator_type;
    using size_type = typename pool_type::size_type;

private:
    class ViewIterator {
        inline bool valid() const noexcept {
            return ((mask[*begin] & bitmask) == bitmask);
        }

    public:
        using value_type = entity_type;
        using difference_type = std::ptrdiff_t;
        using reference = entity_type &;
        using pointer = entity_type *;
        using iterator_category = std::input_iterator_tag;

        ViewIterator(underlying_iterator_type begin, underlying_iterator_type end, const mask_type &bitmask, const mask_type *mask) noexcept
            : begin{begin}, end{end}, bitmask{bitmask}, mask{mask}
        {
            if(begin != end && !valid()) {
                ++(*this);
            }
        }

        ViewIterator & operator++() noexcept {
            ++begin;
            while(begin != end && !valid()) { ++begin; }
            return *this;
        }

        ViewIterator operator++(int) noexcept {
            ViewIterator orig = *this;
            return ++(*this), orig;
        }

        bool operator==(const ViewIterator &other) const noexcept {
            return other.begin == begin;
        }

        bool operator!=(const ViewIterator &other) const noexcept {
            return !(*this == other);
        }

        value_type operator*() const noexcept {
            return *begin;
        }

    private:
        underlying_iterator_type begin;
        underlying_iterator_type end;
        const mask_type bitmask;
        const mask_type *mask;
    };

    template<typename Comp>
    void prefer(size_type &size) noexcept {
        auto sz = pool.template size<Comp>();

        if(sz < size) {
            from = pool.template begin<Type>();
            to = pool.template end<Type>();
            size = sz;
        }
    }

public:
    explicit View(pool_type &pool, const mask_type *mask) noexcept
        : from{pool.template begin<Type>()},
          to{pool.template end<Type>()},
          pool{pool},
          mask{mask}
    {
        using accumulator_type = int[];
        size_type size = pool.template size<Type>();
        bitmask.set(ident<Components...>.template get<Type>());
        accumulator_type types = { 0, (bitmask.set(ident<Components...>.template get<Types>()), 0)... };
        accumulator_type pref = { 0, (prefer<Types>(size), 0)... };
        (void)types, (void)pref;
    }

    const_iterator_type begin() const noexcept {
        return ViewIterator{from, to, bitmask, mask};
    }

    iterator_type begin() noexcept {
        return const_cast<const View *>(this)->begin();
    }

    const_iterator_type end() const noexcept {
        return ViewIterator{to, to, bitmask, mask};
    }

    iterator_type end() noexcept {
        return const_cast<const View *>(this)->end();
    }

    void reset() noexcept {
        using accumulator_type = int[];
        from = pool.template begin<Type>();
        to = pool.template end<Type>();
        size_type size = pool.template size<Type>();
        accumulator_type accumulator = { 0, (prefer<Types>(size), 0)... };
        (void)accumulator;
    }

private:
    underlying_iterator_type from;
    underlying_iterator_type to;
    pool_type &pool;
    const mask_type *mask;
    mask_type bitmask;
};


template<template<typename...> class Pool, typename Entity, typename... Components, typename Type>
class View<Pool<Entity, Components...>, Type> final {
    using pool_type = Pool<Entity, Components...>;

public:
    using size_type = typename pool_type::size_type;
    using iterator_type = typename pool_type::const_iterator_type;
    using const_iterator_type = iterator_type;

    explicit View(pool_type &pool) noexcept
        : pool{pool}
    {}

    const_iterator_type cbegin() const noexcept {
        return pool.template cbegin<Type>();
    }

    iterator_type begin() noexcept {
        return pool.template begin<Type>();
    }

    const_iterator_type cend() const noexcept {
        return pool.template cend<Type>();
    }

    iterator_type end() noexcept {
        return pool.template end<Type>();
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
    static_assert(sizeof...(Components) > 1, "!");

    using pool_type = Pool<Entity, Components...>;
    using mask_type = std::bitset<sizeof...(Components)+1>;

    static constexpr auto validity_bit = sizeof...(Components);

public:
    using entity_type = typename pool_type::entity_type;
    using size_type = typename std::vector<mask_type>::size_type;

private:
    template<typename Comp>
    void clone(entity_type to, entity_type from) {
        if(entities[from].test(ident<Components...>.template get<Comp>())) {
            assign<Comp>(to, pool.template get<Comp>(from));
        }
    }

    template<typename Comp>
    void sync(entity_type to, entity_type from) {
        bool src = entities[from].test(ident<Components...>.template get<Comp>());
        bool dst = entities[to].test(ident<Components...>.template get<Comp>());

        if(src && dst) {
            copy<Comp>(to, from);
        } else if(src) {
            clone<Comp>(to, from);
        } else if(dst) {
            remove<Comp>(to);
        }
    }

public:
    template<typename... Comp>
    using view_type = View<pool_type, Comp...>;

    template<typename... Args>
    Registry(Args&&... args)
        : pool{std::forward<Args>(args)...}
    {}

    Registry(const Registry &) = delete;
    Registry(Registry &&) = delete;

    Registry & operator=(const Registry &) = delete;
    Registry & operator=(Registry &&) = delete;

    size_type size() const noexcept {
        return entities.size() - available.size();
    }

    size_type capacity() const noexcept {
        return entities.size();
    }

    template<typename Comp>
    bool empty() const noexcept {
        return pool.template empty<Comp>();
    }

    bool empty() const noexcept {
        return entities.empty();
    }

    bool valid(entity_type entity) const noexcept {
        return (entity < entities.size() && entities[entity].test(validity_bit));
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
            entity = entity_type(entities.size());
            entities.emplace_back();
        } else {
            entity = available.back();
            available.pop_back();
        }

        entities[entity].set(validity_bit);

        return entity;
    }

    void destroy(entity_type entity) {
        assert(valid(entity));
        using accumulator_type = int[];
        accumulator_type accumulator = { 0, (reset<Components>(entity), 0)... };
        available.push_back(entity);
        entities[entity].reset();
        (void)accumulator;
    }

    template<typename Comp, typename... Args>
    Comp & assign(entity_type entity, Args... args) {
        assert(valid(entity));
        entities[entity].set(ident<Components...>.template get<Comp>());
        return pool.template construct<Comp>(entity, args...);
    }

    template<typename Comp>
    void remove(entity_type entity) {
        assert(valid(entity));
        entities[entity].reset(ident<Components...>.template get<Comp>());
        pool.template destroy<Comp>(entity);
    }

    template<typename... Comp>
    bool has(entity_type entity) const noexcept {
        assert(valid(entity));
        using accumulator_type = bool[];
        bool all = true;
        auto &mask = entities[entity];
        accumulator_type accumulator = { true, (all = all && mask.test(ident<Components...>.template get<Comp>()))... };
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
        assert(valid(entity));

        return (entities[entity].test(ident<Components...>.template get<Comp>())
                ? this->template replace<Comp>(entity, std::forward<Args>(args)...)
                : this->template assign<Comp>(entity, std::forward<Args>(args)...));
    }

    entity_type clone(entity_type from) {
        assert(valid(from));
        using accumulator_type = int[];
        auto to = create();
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
        assert(valid(entity));

        if(entities[entity].test(ident<Components...>.template get<Comp>())) {
            remove<Comp>(entity);
        }
    }

    template<typename Comp>
    void reset() {
        for(entity_type entity = 0, last = entity_type(entities.size()); entity < last; ++entity) {
            if(entities[entity].test(ident<Components...>.template get<Comp>())) {
                remove<Comp>(entity);
            }
        }
    }

    void reset() {
        entities.clear();
        available.clear();
        pool.reset();
    }

    template<typename... Comp>
    std::enable_if_t<(sizeof...(Comp) == 1), view_type<Comp...>>
    view() noexcept { return view_type<Comp...>{pool}; }

    template<typename... Comp>
    std::enable_if_t<(sizeof...(Comp) > 1), view_type<Comp...>>
    view() noexcept { return view_type<Comp...>{pool, entities.data()}; }

private:
    std::vector<mask_type> entities;
    std::vector<entity_type> available;
    pool_type pool;
};


template<typename Entity, typename... Components>
using StandardRegistry = Registry<ComponentPool<Entity, Components...>>;


template<typename... Components>
using DefaultRegistry = Registry<ComponentPool<std::uint32_t, Components...>>;


}


#endif // ENTT_REGISTRY_HPP

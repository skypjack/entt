#ifndef ENTT_REGISTRY_HPP
#define ENTT_REGISTRY_HPP


#include <tuple>
#include <vector>
#include <bitset>
#include <utility>
#include <cstddef>
#include <cassert>
#include <type_traits>
#include "sparse_set.hpp"
#include "ident.hpp"


namespace entt {


template<typename, std::size_t...>
class View;


template<typename Pool, std::size_t Ident, std::size_t... Other>
class View<Pool, Ident, Other...> final {
    using pool_type = Pool;
    using mask_type = std::bitset<std::tuple_size<Pool>::value + 1>;
    using underlying_iterator_type = typename std::tuple_element_t<Ident, Pool>::iterator_type;

    class ViewIterator;

public:
    using iterator_type = ViewIterator;
    using entity_type = typename std::tuple_element_t<Ident, Pool>::index_type;
    using size_type = typename std::tuple_element_t<Ident, Pool>::size_type;

private:
    class ViewIterator {
        inline bool valid() const noexcept {
            return ((mask[*begin] & bitmask) == bitmask);
        }

    public:
        using value_type = entity_type;

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

    template<std::size_t Idx>
    void prefer(size_type &size) noexcept {
        auto &&cpool = std::get<Idx>(*pool);
        auto sz = cpool.size();

        if(sz < size) {
            from = cpool.begin();
            to = cpool.end();
            size = sz;
        }
    }

public:
    explicit View(const pool_type *pool, const mask_type *mask) noexcept
        : from{std::get<Ident>(*pool).begin()},
          to{std::get<Ident>(*pool).end()},
          pool{pool},
          mask{mask}
    {
        using accumulator_type = int[];
        size_type size = std::get<Ident>(*pool).size();
        bitmask.set(Ident);
        accumulator_type types = { 0, (bitmask.set(Other), 0)... };
        accumulator_type pref = { 0, (prefer<Other>(size), 0)... };
        (void)types, (void)pref;
    }

    iterator_type begin() const noexcept {
        return ViewIterator{from, to, bitmask, mask};
    }

    iterator_type end() const noexcept {
        return ViewIterator{to, to, bitmask, mask};
    }

    void reset() noexcept {
        using accumulator_type = int[];
        auto &&cpool = std::get<Ident>(*pool);
        from = cpool.begin();
        to = cpool.end();
        size_type size = cpool.size();
        accumulator_type accumulator = { 0, (prefer<Other>(size), 0)... };
        (void)accumulator;
    }

private:
    underlying_iterator_type from;
    underlying_iterator_type to;
    const pool_type *pool;
    const mask_type *mask;
    mask_type bitmask;
};


template<typename Pool, std::size_t Ident>
class View<Pool, Ident> final {
    using pool_type = std::tuple_element_t<Ident, Pool>;

public:
    using iterator_type = typename pool_type::iterator_type;
    using entity_type = typename pool_type::index_type;
    using size_type = typename pool_type::size_type;
    using raw_type = typename pool_type::type;

    explicit View(const Pool *pool) noexcept
        : pool{&std::get<Ident>(*pool)}
    {}

    raw_type * raw() noexcept {
        return pool->raw();
    }

    const raw_type * raw() const noexcept {
        return pool->raw();
    }

    const entity_type * data() const noexcept {
        return pool->data();
    }

    size_type size() const noexcept {
        return pool->size();
    }

    iterator_type begin() const noexcept {
        return pool->begin();
    }

    iterator_type end() const noexcept {
        return pool->end();
    }

private:
    const pool_type *pool;
};


template<typename Entity, typename... Component>
class Registry {
    using pool_type = std::tuple<SparseSet<Entity, Component>...>;
    using mask_type = std::bitset<sizeof...(Component)+1>;

    static constexpr auto validity_bit = sizeof...(Component);

    // variable templates are fine as well, but for the fact that MSVC goes crazy
    template<typename Comp>
    struct identifier {
        static constexpr auto value = ident<Component...>.template get<Comp>();
    };

public:
    using entity_type = Entity;
    using size_type = typename std::vector<mask_type>::size_type;

    template<typename... Comp>
    using view_type = View<pool_type, identifier<Comp>::value...>;

private:
    template<typename Comp>
    void clone(entity_type to, entity_type from) {
        if(entities[from].test(identifier<Comp>::value)) {
            assign<Comp>(to, std::get<identifier<Comp>::value>(pool).get(from));
        }
    }

    template<typename Comp>
    void sync(entity_type to, entity_type from) {
        bool src = entities[from].test(identifier<Comp>::value);
        bool dst = entities[to].test(identifier<Comp>::value);

        if(src && dst) {
            copy<Comp>(to, from);
        } else if(src) {
            clone<Comp>(to, from);
        } else if(dst) {
            remove<Comp>(to);
        }
    }

public:
    explicit Registry() = default;
    ~Registry() = default;

    Registry(const Registry &) = delete;
    Registry(Registry &&) = delete;

    Registry & operator=(const Registry &) = delete;
    Registry & operator=(Registry &&) = delete;

    template<typename Comp>
    size_type size() const noexcept {
        return std::get<identifier<Comp>::value>(pool).size();
    }

    size_type size() const noexcept {
        return entities.size() - available.size();
    }

    template<typename Comp>
    size_type capacity() const noexcept {
        return std::get<identifier<Comp>::value>(pool).capacity();
    }

    size_type capacity() const noexcept {
        return entities.size();
    }

    template<typename Comp>
    bool empty() const noexcept {
        return std::get<identifier<Comp>::value>(pool).empty();
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
        accumulator_type accumulator = { 0, (reset<Component>(entity), 0)... };
        available.push_back(entity);
        entities[entity].reset();
        (void)accumulator;
    }

    template<typename Comp, typename... Args>
    Comp & assign(entity_type entity, Args... args) {
        assert(valid(entity));
        entities[entity].set(identifier<Comp>::value);
        return std::get<identifier<Comp>::value>(pool).construct(entity, args...);
    }

    template<typename Comp>
    void remove(entity_type entity) {
        assert(valid(entity));
        entities[entity].reset(identifier<Comp>::value);
        std::get<identifier<Comp>::value>(pool).destroy(entity);
    }

    template<typename... Comp>
    bool has(entity_type entity) const noexcept {
        assert(valid(entity));
        using accumulator_type = bool[];
        bool all = true;
        auto &mask = entities[entity];
        accumulator_type accumulator = { true, (all = all && mask.test(identifier<Comp>::value))... };
        (void)accumulator;
        return all;
    }

    template<typename Comp>
    const Comp & get(entity_type entity) const noexcept {
        assert(valid(entity));
        return std::get<identifier<Comp>::value>(pool).get(entity);
    }

    template<typename Comp>
    Comp & get(entity_type entity) noexcept {
        assert(valid(entity));
        return std::get<identifier<Comp>::value>(pool).get(entity);
    }

    template<typename Comp, typename... Args>
    Comp & replace(entity_type entity, Args... args) {
        assert(valid(entity));
        return (std::get<identifier<Comp>::value>(pool).get(entity) = Comp{args...});
    }

    template<typename Comp, typename... Args>
    Comp & accomodate(entity_type entity, Args... args) {
        assert(valid(entity));

        return (entities[entity].test(identifier<Comp>::value)
                ? this->template replace<Comp>(entity, std::forward<Args>(args)...)
                : this->template assign<Comp>(entity, std::forward<Args>(args)...));
    }

    entity_type clone(entity_type from) {
        assert(valid(from));
        using accumulator_type = int[];
        auto to = create();
        accumulator_type accumulator = { 0, (clone<Component>(to, from), 0)... };
        (void)accumulator;
        return to;
    }

    template<typename Comp>
    Comp & copy(entity_type to, entity_type from) {
        assert(valid(to));
        assert(valid(from));
        auto &&cpool = std::get<identifier<Comp>::value>(pool);
        return (cpool.get(to) = cpool.get(from));
    }

    void copy(entity_type to, entity_type from) {
        assert(valid(to));
        assert(valid(from));
        using accumulator_type = int[];
        accumulator_type accumulator = { 0, (sync<Component>(to, from), 0)... };
        (void)accumulator;
    }

    template<typename Comp>
    void swap(entity_type lhs, entity_type rhs) {
        assert(valid(lhs));
        assert(valid(rhs));
        std::get<identifier<Comp>::value>(pool).swap(lhs, rhs);
    }

    template<typename Comp, typename Compare>
    void sort(Compare compare) {
        std::get<identifier<Comp>::value>(pool).sort(std::move(compare));
    }

    template<typename To, typename From>
    void sort() {
        auto &&to = std::get<identifier<To>::value>(pool);
        auto &&from = std::get<identifier<From>::value>(pool);
        to.respect(from);
    }

    template<typename Comp>
    void reset(entity_type entity) {
        assert(valid(entity));

        if(entities[entity].test(identifier<Comp>::value)) {
            remove<Comp>(entity);
        }
    }

    template<typename Comp>
    void reset() {
        for(entity_type entity = 0, last = entity_type(entities.size()); entity < last; ++entity) {
            if(entities[entity].test(identifier<Comp>::value)) {
                remove<Comp>(entity);
            }
        }
    }

    void reset() {
        using accumulator_type = int[];
        accumulator_type acc = { 0, (std::get<identifier<Component>::value>(pool).reset(), 0)... };
        entities.clear();
        available.clear();
        (void)acc;
    }

    template<typename... Comp>
    // view_type<Comp...> is fine as well, but for the fact that MSVC dislikes it
    std::enable_if_t<(sizeof...(Comp) == 1), View<pool_type, identifier<Comp>::value...>>
    view() noexcept { return view_type<Comp...>{&pool}; }

    template<typename... Comp>
    // view_type<Comp...> is fine as well, but for the fact that MSVC dislikes it
    std::enable_if_t<(sizeof...(Comp) > 1), View<pool_type, identifier<Comp>::value...>>
    view() noexcept { return view_type<Comp...>{&pool, entities.data()}; }

private:
    std::vector<mask_type> entities;
    std::vector<entity_type> available;
    pool_type pool;
};


template<typename... Component>
using DefaultRegistry = Registry<std::uint32_t, Component...>;


}


#endif // ENTT_REGISTRY_HPP

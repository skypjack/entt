#ifndef ENTT_REGISTRY_HPP
#define ENTT_REGISTRY_HPP


#include <tuple>
#include <vector>
#include <memory>
#include <utility>
#include <cstddef>
#include <cassert>
#include <algorithm>
#include "sparse_set.hpp"


namespace entt {


template<typename, typename...>
class View;


template<typename Entity, typename Component, typename... Other>
class View<Entity, Component, Other...> final {
    template<typename Comp>
    using pool_type = SparseSet<Entity, Comp>;

    using underlying_iterator_type = typename pool_type<Component>::iterator_type;

    class ViewIterator;

public:
    using iterator_type = ViewIterator;
    using entity_type = typename pool_type<Component>::index_type;
    using size_type = typename pool_type<Component>::size_type;

private:
    class ViewIterator {
        inline bool valid() const noexcept {
            using accumulator_type = bool[];
            auto entity = *begin;
            bool all = std::get<pool_type<Component> &>(pools).has(entity);
            accumulator_type accumulator =  { all, (all = all && std::get<pool_type<Other> &>(pools).has(entity))... };
            (void)accumulator;
            return all;
        }

    public:
        using value_type = entity_type;

        ViewIterator(const std::tuple<pool_type<Component> &, pool_type<Other> &...> &pools, underlying_iterator_type begin, underlying_iterator_type end) noexcept
            : pools{pools}, begin{begin}, end{end}
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
        const std::tuple<pool_type<Component> &, pool_type<Other> &...> &pools;
        underlying_iterator_type begin;
        underlying_iterator_type end;
    };

    template<typename Comp>
    void prefer(size_type &size) noexcept {
        auto &cpool = std::get<pool_type<Comp> &>(pools);
        auto sz = cpool.size();

        if(sz < size) {
            from = cpool.begin();
            to = cpool.end();
            size = sz;
        }
    }

public:
    explicit View(pool_type<Component> &pool, pool_type<Other>&... other) noexcept
        : from{pool.begin()},
          to{pool.end()},
          pools{pool, other...}
    {
        using accumulator_type = int[];
        size_type size = pool.size();
        accumulator_type accumulator = { 0, (prefer<Other>(size), 0)... };
        (void)accumulator;
    }

    iterator_type begin() const noexcept {
        return ViewIterator{pools, from, to};
    }

    iterator_type end() const noexcept {
        return ViewIterator{pools, to, to};
    }

    template<typename Comp>
    const Comp & get(entity_type entity) const noexcept {
        return std::get<pool_type<Comp> &>(pools).get(entity);
    }

    template<typename Comp>
    Comp & get(entity_type entity) noexcept {
        return const_cast<Comp &>(const_cast<const View *>(this)->get<Comp>(entity));
    }

    void reset() noexcept {
        using accumulator_type = int[];
        auto &cpool = std::get<pool_type<Component> &>(pools);
        from = cpool.begin();
        to = cpool.end();
        size_type size = cpool.size();
        accumulator_type accumulator = { 0, (prefer<Other>(size), 0)... };
        (void)accumulator;
    }

private:
    underlying_iterator_type from;
    underlying_iterator_type to;
    std::tuple<pool_type<Component> &, pool_type<Other> &...> pools;
};


template<typename Entity, typename Component>
class View<Entity, Component> final {
    using pool_type = SparseSet<Entity, Component>;

public:
    using iterator_type = typename pool_type::iterator_type;
    using entity_type = typename pool_type::index_type;
    using size_type = typename pool_type::size_type;
    using raw_type = typename pool_type::type;

    explicit View(pool_type &pool) noexcept
        : pool{pool}
    {}

    size_type size() const noexcept {
        return pool.size();
    }

    raw_type * raw() noexcept {
        return pool.raw();
    }

    const raw_type * raw() const noexcept {
        return pool.raw();
    }

    const entity_type * data() const noexcept {
        return pool.data();
    }

    iterator_type begin() const noexcept {
        return pool.begin();
    }

    iterator_type end() const noexcept {
        return pool.end();
    }

    const Component & get(entity_type entity) const noexcept {
        return pool.get(entity);
    }

    Component & get(entity_type entity) noexcept {
        return const_cast<Component &>(const_cast<const View *>(this)->get(entity));
    }

private:
    pool_type &pool;
};


template<typename Entity>
struct Registry {
    using entity_type = Entity;
    using size_type = std::size_t;

private:
    using base_pool_type = SparseSet<entity_type>;

    template<typename Component>
    using pool_type = SparseSet<entity_type, Component>;

    static std::size_t identifier() noexcept {
        static std::size_t value = 0;
        return value++;
    }

    template<typename>
    static std::size_t type() noexcept {
        static const std::size_t value = identifier();
        return value;
    }

    template<typename Component>
    bool managed() const noexcept {
        auto ctype = type<Component>();
        return ctype < pools.size() && pools[ctype];
    }

    template<typename Component>
    const pool_type<Component> & pool() const noexcept {
        assert(managed<Component>());
        return static_cast<pool_type<Component> &>(*pools[type<Component>()]);
    }

    template<typename Component>
    pool_type<Component> & pool() noexcept {
        assert(managed<Component>());
        return const_cast<pool_type<Component> &>(const_cast<const Registry *>(this)->pool<Component>());
    }

    template<typename Component>
    pool_type<Component> & ensure() {
        auto ctype = type<Component>();

        if(!(ctype < pools.size())) {
            pools.resize(ctype + 1);
        }

        if(!pools[ctype]) {
            pools[ctype] = std::make_unique<pool_type<Component>>();
        }

        return pool<Component>();
    }

public:
    explicit Registry() = default;
    ~Registry() = default;

    Registry(const Registry &) = delete;
    Registry(Registry &&) = delete;

    Registry & operator=(const Registry &) = delete;
    Registry & operator=(Registry &&) = delete;

    template<typename Component>
    size_type size() const noexcept {
        return managed<Component>() ? pool<Component>().size() : size_type{};
    }

    size_type size() const noexcept {
        return next - available.size();
    }

    template<typename Component>
    size_type capacity() const noexcept {
        return managed<Component>() ? pool<Component>().capacity() : size_type{};
    }

    size_type capacity() const noexcept {
        return next;
    }

    template<typename Component>
    bool empty() const noexcept {
        return managed<Component>() ? pool<Component>().empty() : true;
    }

    bool empty() const noexcept {
        return next == available.size();
    }

    bool valid(entity_type entity) const noexcept {
        return (entity < next && std::find(available.cbegin(), available.cend(), entity) == available.cend());
    }

    template<typename... Component>
    entity_type create() noexcept {
        using accumulator_type = int[];
        auto entity = create();
        accumulator_type accumulator = { 0, (assign<Component>(entity), 0)... };
        (void)accumulator;
        return entity;
    }

    entity_type create() noexcept {
        entity_type entity;

        if(available.empty()) {
            entity = next++;
        } else {
            entity = available.back();
            available.pop_back();
        }

        return entity;
    }

    void destroy(entity_type entity) {
        assert(valid(entity));

        for(auto &&cpool: pools) {
            if(cpool && cpool->has(entity)) {
                cpool->destroy(entity);
            }
        }

        available.push_back(entity);
    }

    template<typename Component, typename... Args>
    Component & assign(entity_type entity, Args&&... args) {
        assert(valid(entity));
        return ensure<Component>().construct(entity, std::forward<Args>(args)...);
    }

    template<typename Component>
    void remove(entity_type entity) {
        assert(valid(entity));
        assert(managed<Component>());
        return pool<Component>().destroy(entity);
    }

    template<typename... Component>
    bool has(entity_type entity) const noexcept {
        assert(valid(entity));
        using accumulator_type = bool[];
        bool all = true;
        accumulator_type accumulator = { all, (all = all && managed<Component>() && pool<Component>().has(entity))... };
        (void)accumulator;
        return all;
    }

    template<typename Component>
    const Component & get(entity_type entity) const noexcept {
        assert(valid(entity));
        assert(managed<Component>());
        return pool<Component>().get(entity);
    }

    template<typename Component>
    Component & get(entity_type entity) noexcept {
        return const_cast<Component &>(const_cast<const Registry *>(this)->get<Component>(entity));
    }

    template<typename Component, typename... Args>
    Component & replace(entity_type entity, Args&&... args) {
        assert(valid(entity));
        assert(managed<Component>());
        return (pool<Component>().get(entity) = Component{std::forward<Args>(args)...});
    }

    template<typename Component, typename... Args>
    Component & accomodate(entity_type entity, Args&&... args) {
        assert(valid(entity));
        auto &cpool = ensure<Component>();

        return (cpool.has(entity)
                ? (cpool.get(entity) = Component{std::forward<Args>(args)...})
                : cpool.construct(entity, std::forward<Args>(args)...));
    }

    template<typename Component>
    void swap(entity_type lhs, entity_type rhs) {
        assert(valid(lhs));
        assert(valid(rhs));
        assert(managed<Component>());
        pool<Component>().swap(lhs, rhs);
    }

    template<typename Component, typename Compare>
    void sort(Compare &&compare) {
        ensure<Component>().sort(std::forward<Compare>(compare));
    }

    template<typename To, typename From>
    void sort() {
        ensure<To>().respect(ensure<From>());
    }

    template<typename Component>
    void reset(entity_type entity) {
        assert(valid(entity));

        if(managed<Component>()) {
            auto &cpool = pool<Component>();

            if(cpool.has(entity)) {
                cpool.destroy(entity);
            }
        }
    }

    template<typename Component>
    void reset() {
        if(managed<Component>()) {
            auto &cpool = pool<Component>();

            for(entity_type entity = 0; entity < next; ++entity) {
                if(cpool.has(entity)) {
                    cpool.destroy(entity);
                }
            }
        }
    }

    void reset() {
        for(auto &&cpool: pools) {
            if(cpool) {
                cpool->reset();
            }
        }

        next = entity_type{};
        available.clear();
        pools.clear();
    }

    template<typename... Component>
    View<entity_type, Component...> view() noexcept {
        return View<entity_type, Component...>{ensure<Component>()...};
    }

private:
    std::vector<std::unique_ptr<base_pool_type>> pools;
    std::vector<entity_type> available;
    entity_type next{};
};


using DefaultRegistry = Registry<std::uint32_t>;


}


#endif // ENTT_REGISTRY_HPP

#ifndef ENTT_VIEW_HPP
#define ENTT_VIEW_HPP


#include <tuple>
#include "sparse_set.hpp"


namespace entt {


template<typename Entity, typename... Component>
class PersistentView final {
    static_assert(sizeof...(Component) > 0, "!");

    template<typename Comp>
    using pool_type = SparseSet<Entity, Comp>;

    using view_type = SparseSet<Entity>;

public:
    using iterator_type = typename view_type::iterator_type;
    using entity_type = typename view_type::entity_type;
    using size_type = typename view_type::size_type;

    explicit PersistentView(view_type &view, pool_type<Component>&... pools) noexcept
        : view{view}, pools{pools...}
    {}

    size_type size() const noexcept {
        return view.size();
    }

    const entity_type * data() const noexcept {
        return view.data();
    }

    iterator_type begin() const noexcept {
        return view.begin();
    }

    iterator_type end() const noexcept {
        return view.end();
    }

    template<typename Comp>
    const Comp & get(entity_type entity) const noexcept {
        return std::get<pool_type<Comp> &>(pools).get(entity);
    }

    template<typename Comp>
    Comp & get(entity_type entity) noexcept {
        return const_cast<Comp &>(const_cast<const PersistentView *>(this)->get<Comp>(entity));
    }

private:
    view_type &view;
    std::tuple<pool_type<Component> &...> pools;
};


template<typename Entity, typename Component>
class PersistentView<Entity, Component> final {
    using pool_type = SparseSet<Entity, Component>;

public:
    using iterator_type = typename pool_type::iterator_type;
    using entity_type = typename pool_type::entity_type;
    using size_type = typename pool_type::size_type;
    using raw_type = typename pool_type::type;

    explicit PersistentView(pool_type &pool) noexcept
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
        return const_cast<Component &>(const_cast<const PersistentView *>(this)->get(entity));
    }

private:
    pool_type &pool;
};


template<typename Entity, typename First, typename... Other>
class DynamicView final {
    template<typename Component>
    using pool_type = SparseSet<Entity, Component> &;

    using base_pool_type = SparseSet<Entity>;
    using underlying_iterator_type = typename base_pool_type::iterator_type;
    using repo_type = std::tuple<pool_type<First>, pool_type<Other>...>;

    class ViewIterator {
        inline bool valid() const noexcept {
            using accumulator_type = bool[];
            auto entity = *begin;
            bool all = std::get<pool_type<First>>(pools).has(entity);
            accumulator_type accumulator =  { all, (all = all && std::get<pool_type<Other>>(pools).has(entity))... };
            (void)accumulator;
            return all;
        }

    public:
        using value_type = typename base_pool_type::entity_type;

        ViewIterator(const repo_type &pools, underlying_iterator_type begin, underlying_iterator_type end) noexcept
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
        const repo_type &pools;
        underlying_iterator_type begin;
        underlying_iterator_type end;
    };

public:
    using iterator_type = ViewIterator;
    using entity_type = typename base_pool_type::entity_type;
    using size_type = typename base_pool_type::size_type;

    explicit DynamicView(pool_type<First> pool, pool_type<Other>... other) noexcept
        : pools{pool, other...}, view{nullptr}
    {
        reset();
    }

    iterator_type begin() const noexcept {
        return ViewIterator{pools, view->begin(), view->end()};
    }

    iterator_type end() const noexcept {
        return ViewIterator{pools, view->end(), view->end()};
    }

    template<typename Component>
    const Component & get(entity_type entity) const noexcept {
        return std::get<pool_type<Component>>(pools).get(entity);
    }

    template<typename Component>
    Component & get(entity_type entity) noexcept {
        return const_cast<Component &>(const_cast<const DynamicView *>(this)->get<Component>(entity));
    }

    void reset() {
        using accumulator_type = void *[];
        view = &std::get<pool_type<First>>(pools);
        accumulator_type accumulator = { nullptr, (std::get<pool_type<Other>>(pools).size() < view->size() ? (view = &std::get<pool_type<Other>>(pools)) : nullptr)... };
        (void)accumulator;
    }

private:
    repo_type pools;
    base_pool_type *view;
};


template<typename Entity, typename Component>
class DynamicView<Entity, Component> final {
    using pool_type = SparseSet<Entity, Component>;

public:
    using iterator_type = typename pool_type::iterator_type;
    using entity_type = typename pool_type::entity_type;
    using size_type = typename pool_type::size_type;
    using raw_type = typename pool_type::type;

    explicit DynamicView(pool_type &pool) noexcept
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
        return const_cast<Component &>(const_cast<const DynamicView *>(this)->get(entity));
    }

private:
    pool_type &pool;
};


}


#endif // ENTT_VIEW_HPP

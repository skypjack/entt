#ifndef ENTT_ENTITY_VIEW_HPP
#define ENTT_ENTITY_VIEW_HPP


#include <tuple>
#include "sparse_set.hpp"


namespace entt {


template<typename Entity, typename... Component>
class PersistentView final {
    static_assert(sizeof...(Component) > 1, "!");

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

    template<typename Comp>
    void sort() {
        const SparseSet<Entity> &pool = std::get<pool_type<Comp> &>(pools);
        view.respect(pool);
    }

private:
    view_type &view;
    std::tuple<pool_type<Component> &...> pools;
};


template<typename Entity, typename First, typename... Other>
class View final {
    template<typename Component>
    using pool_type = SparseSet<Entity, Component> &;

    using base_pool_type = SparseSet<Entity>;
    using underlying_iterator_type = typename base_pool_type::iterator_type;
    using repo_type = std::tuple<pool_type<First>, pool_type<Other>...>;

    class Iterator {
        inline bool valid() const noexcept {
            using accumulator_type = bool[];
            auto entity = *begin;
            bool all = std::get<pool_type<First>>(pools).has(entity);
            accumulator_type accumulator =  { (all = all && std::get<pool_type<Other>>(pools).has(entity))... };
            (void)accumulator;
            return all;
        }

    public:
        using value_type = typename base_pool_type::entity_type;

        Iterator(const repo_type &pools, underlying_iterator_type begin, underlying_iterator_type end) noexcept
            : pools{pools}, begin{begin}, end{end}
        {
            if(begin != end && !valid()) {
                ++(*this);
            }
        }

        Iterator & operator++() noexcept {
            ++begin;
            while(begin != end && !valid()) { ++begin; }
            return *this;
        }

        Iterator operator++(int) noexcept {
            Iterator orig = *this;
            return ++(*this), orig;
        }

        bool operator==(const Iterator &other) const noexcept {
            return other.begin == begin;
        }

        bool operator!=(const Iterator &other) const noexcept {
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
    using iterator_type = Iterator;
    using entity_type = typename base_pool_type::entity_type;
    using size_type = typename base_pool_type::size_type;

    explicit View(pool_type<First> pool, pool_type<Other>... other) noexcept
        : pools{pool, other...}, view{nullptr}
    {
        reset();
    }

    iterator_type begin() const noexcept {
        return Iterator{pools, view->begin(), view->end()};
    }

    iterator_type end() const noexcept {
        return Iterator{pools, view->end(), view->end()};
    }

    template<typename Component>
    const Component & get(entity_type entity) const noexcept {
        return std::get<pool_type<Component>>(pools).get(entity);
    }

    template<typename Component>
    Component & get(entity_type entity) noexcept {
        return const_cast<Component &>(const_cast<const View *>(this)->get<Component>(entity));
    }

    void reset() {
        using accumulator_type = void *[];
        view = &std::get<pool_type<First>>(pools);
        accumulator_type accumulator = { (std::get<pool_type<Other>>(pools).size() < view->size() ? (view = &std::get<pool_type<Other>>(pools)) : nullptr)... };
        (void)accumulator;
    }

private:
    repo_type pools;
    base_pool_type *view;
};


/**
 * @brief Single component view specialization.
 *
 * Single component views are specialized in order to get a boost in terms of
 * performance. This kind of views can access the underlying data structure
 * directly and avoid superflous checks.<br/>
 * Order of elements during iterations are highly dependent on the order of the
 * underlying data strcture. See SparseSet and its specializations for more
 * details.
 *
 * @note Views share a reference to the underlying data structure with the
 * Registry that generated them. Therefore any change to the entities and to the
 * components made by means of the registry are immediately reflected by views.
 *
 * @note Lifetime of a view must overcome the one of the registry that generated
 * it. In any other case attempting to use a view results in undefined behavior.
 *
 * @sa View
 * @sa SparseSet
 * @sa SparseSet<Entity>
 * @sa SparseSet<Entity, Type>
 *
 * @tparam Entity A valid entity type (see entt_traits for more details).
 * @tparam Component The type of the component iterated by the view.
 */
template<typename Entity, typename Component>
class View<Entity, Component> final {
    using pool_type = SparseSet<Entity, Component>;

public:
    /*! Input iterator type. */
    using iterator_type = typename pool_type::iterator_type;
    /*! @brief Underlying entity identifier. */
    using entity_type = typename pool_type::entity_type;
    /*! @brief Unsigned integer type. */
    using size_type = typename pool_type::size_type;
    /*! The type of the component iterated by the view. */
    using raw_type = typename pool_type::type;

    /**
     * @brief Constructs a view out of a pool of components.
     * @param pool A reference to a pool of components.
     */
    explicit View(pool_type &pool) noexcept
        : pool{pool}
    {}

    /**
     * @brief Returns the number of entities that have the given component.
     * @return The number of entities that have the given component.
     */
    size_type size() const noexcept {
        return pool.size();
    }

    /**
     * @brief Direct access to the list of components.
     *
     * The returned pointer is such that range `[raw(), raw() + size()]` is
     * always a valid range, even if the container is empty.
     *
     * @note There are no guarantees on the order of the components. Use `begin`
     * and `end` if you want to iterate the view in the expected order.
     *
     * @return A pointer to the array of components.
     */
    raw_type * raw() noexcept {
        return pool.raw();
    }

    /**
     * @brief Direct access to the list of components.
     *
     * The returned pointer is such that range `[raw(), raw() + size()]` is
     * always a valid range, even if the container is empty.
     *
     * @note There are no guarantees on the order of the components. Use `begin`
     * and `end` if you want to iterate the view in the expected order.
     *
     * @return A pointer to the array of components.
     */
    const raw_type * raw() const noexcept {
        return pool.raw();
    }

    /**
     * @brief Direct access to the list of entities.
     *
     * The returned pointer is such that range `[data(), data() + size()]` is
     * always a valid range, even if the container is empty.
     *
     * @note There are no guarantees on the order of the entities. Use `begin`
     * and `end` if you want to iterate the view in the expected order.
     *
     * @return A pointer to the array of entities.
     */
    const entity_type * data() const noexcept {
        return pool.data();
    }

    /**
     * @brief Returns an iterator to the first entity that has the given
     * component.
     *
     * The returned iterator points to the first entity that has the given
     * component. If the view is empty, the returned iterator will be equal to
     * `end()`.
     *
     * @note Input iterators stay true to the order imposed to the underlying
     * data structures.
     *
     * @return An iterator to the first entity that has the given component.
     */
    iterator_type begin() const noexcept {
        return pool.begin();
    }

    /**
     * @brief Returns an iterator that is past the last entity that has the
     * given component.
     *
     * The returned iterator points to the entity following the last entity that
     * has the given component. Attempting to dereference the returned iterator
     * results in undefined behavior.
     *
     * @note Input iterators stay true to the order imposed to the underlying
     * data structures.
     *
     * @return An iterator to the entity following the last entity that has the
     * given component.
     */
    iterator_type end() const noexcept {
        return pool.end();
    }

    /**
     * @brief Returns the component assigned to the given entity.
     *
     * Attempting to use an entity that doesn't belong to the view results in
     * undefined behavior.
     *
     * @note An assertion will abort the execution at runtime in debug mode if
     * the view doesn't contain the given entity.
     *
     * @note Prefer this function instead of `Registry::get` during iterations.
     * It has far better performance than its companion function.
     *
     * @param entity A valid entity identifier.
     * @return The component assigned to the entity.
     */
    const Component & get(entity_type entity) const noexcept {
        return pool.get(entity);
    }

    /**
     * @brief Returns the component assigned to the given entity.
     *
     * Attempting to use an entity that doesn't belong to the view results in
     * undefined behavior.
     *
     * @note An assertion will abort the execution at runtime in debug mode if
     * the view doesn't contain the given entity.
     *
     * @note Prefer this function instead of `Registry::get` during iterations.
     * It has far better performance than its companion function.
     *
     * @param entity A valid entity identifier.
     * @return The component assigned to the entity.
     */
    Component & get(entity_type entity) noexcept {
        return const_cast<Component &>(const_cast<const View *>(this)->get(entity));
    }

private:
    pool_type &pool;
};


}


#endif // ENTT_ENTITY_VIEW_HPP

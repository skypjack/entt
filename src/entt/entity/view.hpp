#ifndef ENTT_ENTITY_VIEW_HPP
#define ENTT_ENTITY_VIEW_HPP


#include <array>
#include <tuple>
#include <utility>
#include <algorithm>
#include <type_traits>
#include "sparse_set.hpp"


namespace entt {


/**
 * @brief Forward declaration of the registry class.
 */
template<typename>
class Registry;


/**
 * @brief Persistent view.
 *
 * A persistent view returns all the entities and only the entities that have
 * at least the given components. Moreover, it's guaranteed that the entity list
 * is thightly packed in memory for fast iterations.<br/>
 * In general, persistent views don't stay true to the order of any set of
 * components unless users explicitly sort them.
 *
 * @b Important
 *
 * Iterators aren't invalidated if:
 *
 * * New instances of the given components are created and assigned to entities.
 * * The entity currently pointed is modified (as an example, if one of the
 * given components is removed from the entity to which the iterator points).
 *
 * In all the other cases, modify the pools of the given components somehow
 * invalidates all the iterators and using them results in undefined behavior.
 *
 * @note
 * Views share references to the underlying data structures with the Registry
 * that generated them. Therefore any change to the entities and to the
 * components made by means of the registry are immediately reflected by
 * views.<br/>
 * Moreover, sorting a persistent view affects all the other views of the same
 * type (it means that users don't have to call `sort` on each view to sort all
 * of them because they share the set of entities).
 *
 * @warning
 * Lifetime of a view must overcome the one of the registry that generated it.
 * In any other case, attempting to use a view results in undefined behavior.
 *
 * @sa View
 * @sa View<Entity, Component>
 *
 * @tparam Entity A valid entity type (see entt_traits for more details).
 * @tparam Component Types of components iterated by the view.
 */
template<typename Entity, typename... Component>
class PersistentView final {
    static_assert(sizeof...(Component) > 1, "!");

    /*! @brief A registry is allowed to create views. */
    friend class Registry<Entity>;

    template<typename Comp>
    using pool_type = SparseSet<Entity, Comp>;

    using view_type = SparseSet<Entity>;
    using pattern_type = std::tuple<pool_type<Component> &...>;

    PersistentView(view_type &view, pool_type<Component> &... pools) noexcept
        : view{view}, pools{pools...}
    {}

public:
    /*! Input iterator type. */
    using iterator_type = typename view_type::iterator_type;
    /*! @brief Underlying entity identifier. */
    using entity_type = typename view_type::entity_type;
    /*! @brief Unsigned integer type. */
    using size_type = typename view_type::size_type;

    /**
     * @brief Returns the number of entities that have the given components.
     * @return Number of entities that have the given components.
     */
    size_type size() const noexcept {
        return view.size();
    }

    /**
     * @brief Direct access to the list of entities.
     *
     * The returned pointer is such that range `[data(), data() + size()]` is
     * always a valid range, even if the container is empty.
     *
     * @note
     * There are no guarantees on the order of the entities. Use `begin` and
     * `end` if you want to iterate the view in the expected order.
     *
     * @return A pointer to the array of entities.
     */
    const entity_type * data() const noexcept {
        return view.data();
    }

    /**
     * @brief Returns an iterator to the first entity that has the given
     * components.
     *
     * The returned iterator points to the first entity that has the given
     * components. If the view is empty, the returned iterator will be equal to
     * `end()`.
     *
     * @note
     * Input iterators stay true to the order imposed to the underlying data
     * structures.
     *
     * @return An iterator to the first entity that has the given components.
     */
    iterator_type begin() const noexcept {
        return view.begin();
    }

    /**
     * @brief Returns an iterator that is past the last entity that has the
     * given components.
     *
     * The returned iterator points to the entity following the last entity that
     * has the given components. Attempting to dereference the returned iterator
     * results in undefined behavior.
     *
     * @note
     * Input iterators stay true to the order imposed to the underlying data
     * structures.
     *
     * @return An iterator to the entity following the last entity that has the
     * given components.
     */
    iterator_type end() const noexcept {
        return view.end();
    }

    /**
     * @brief Returns the component assigned to the given entity.
     *
     * Prefer this function instead of `Registry::get` during iterations. It has
     * far better performance than its companion function.
     *
     * @warning
     * Attempting to use an invalid component type results in a compilation
     * error. Attempting to use an entity that doesn't belong to the view
     * results in undefined behavior.<br/>
     * An assertion will abort the execution at runtime in debug mode if
     * the view doesn't contain the given entity.
     *
     * @tparam Comp Type of the component to get.
     * @param entity A valid entity identifier.
     * @return The component assigned to the entity.
     */
    template<typename Comp>
    const Comp & get(entity_type entity) const noexcept {
        return std::get<pool_type<Comp> &>(pools).get(entity);
    }

    /**
     * @brief Returns the component assigned to the given entity.
     *
     * Prefer this function instead of `Registry::get` during iterations.
     * It has far better performance than its companion function.
     *
     * @warning
     * Attempting to use an invalid component type results in a compilation
     * error. Attempting to use an entity that doesn't belong to the view
     * results in undefined behavior.<br/>
     * An assertion will abort the execution at runtime in debug mode if
     * the view doesn't contain the given entity.
     *
     * @tparam Comp Type of the component to get.
     * @param entity A valid entity identifier.
     * @return The component assigned to the entity.
     */
    template<typename Comp>
    Comp & get(entity_type entity) noexcept {
        return const_cast<Comp &>(const_cast<const PersistentView *>(this)->get<Comp>(entity));
    }

    /**
     * @brief Returns the components assigned to the given entity.
     *
     * Prefer this function instead of `Registry::get` during iterations. It has
     * far better performance than its companion function.
     *
     * @warning
     * Attempting to use invalid component types results in a compilation error.
     * Attempting to use an entity that doesn't belong to the view results in
     * undefined behavior.<br/>
     * An assertion will abort the execution at runtime in debug mode if
     * the view doesn't contain the given entity.
     *
     * @tparam Comp Types of the components to get.
     * @param entity A valid entity identifier.
     * @return The components assigned to the entity.
     */
    template<typename... Comp>
    std::enable_if_t<(sizeof...(Comp) > 1), std::tuple<const Comp &...>>
    get(entity_type entity) const noexcept {
        return std::tuple<const Comp &...>{ get<Comp>(entity)... };
    }

    /**
     * @brief Returns the components assigned to the given entity.
     *
     * Prefer this function instead of `Registry::get` during iterations. It has
     * far better performance than its companion function.
     *
     * @warning
     * Attempting to use invalid component types results in a compilation error.
     * Attempting to use an entity that doesn't belong to the view results in
     * undefined behavior.<br/>
     * An assertion will abort the execution at runtime in debug mode if
     * the view doesn't contain the given entity.
     *
     * @tparam Comp Types of the components to get.
     * @param entity A valid entity identifier.
     * @return The components assigned to the entity.
     */
    template<typename... Comp>
    std::enable_if_t<(sizeof...(Comp) > 1), std::tuple<Comp &...>>
    get(entity_type entity) noexcept {
        return std::tuple<Comp &...>{ get<Comp>(entity)... };
    }

    /**
     * @brief Iterate the entities and applies them the given function object.
     *
     * The function object is invoked for each entity. It is provided with the
     * entity itself and a set of const references to all the components of the
     * view.<br/>
     * The signature of the function should be equivalent to the following:
     *
     * @code{.cpp}
     * void(entity_type, const Component &...);
     * @endcode
     *
     * @tparam Func Type of the function object to invoke.
     * @param func A valid function object.
     */
    template<typename Func>
    void each(Func func) const {
        for(auto entity: *this) {
            func(entity, get<Component>(entity)...);
        }
    }

    /**
     * @brief Iterate the entities and applies them the given function object.
     *
     * The function object is invoked for each entity. It is provided with the
     * entity itself and a set of references to all the components of the
     * view.<br/>
     * The signature of the function should be equivalent to the following:
     *
     * @code{.cpp}
     * void(entity_type, Component &...);
     * @endcode
     *
     * @tparam Func Type of the function object to invoke.
     * @param func A valid function object.
     */
    template<typename Func>
    void each(Func func) {
        const_cast<const PersistentView *>(this)->each([&func](entity_type entity, const Component &... component) {
            func(entity, const_cast<Component &>(component)...);
        });
    }

    /**
     * @brief Sort the shared pool of entities according to the given component.
     *
     * Persistent views of the same type share with the Registry a pool of
     * entities with its own order that doesn't depend on the order of any pool
     * of components. Users can order the underlying data structure so that it
     * respects the order of the pool of the given component.
     *
     * @note
     * The shared pool of entities and thus its order is affected by the changes
     * to each and every pool that it tracks. Therefore changes to those pools
     * can quickly ruin the order imposed to the pool of entities shared between
     * the persistent views.
     *
     * @tparam Comp Type of the component to use to impose the order.
     */
    template<typename Comp>
    void sort() {
        view.respect(std::get<pool_type<Comp> &>(pools));
    }

private:
    view_type &view;
    pattern_type pools;
};


/**
 * @brief Multi component view.
 *
 * Multi component views iterate over those entities that have at least all the
 * given components in their bags. During initialization, a multi component view
 * looks at the number of entities available for each component and picks up a
 * reference to the smallest set of candidate entities in order to get a
 * performance boost when iterate.<br/>
 * Order of elements during iterations are highly dependent on the order of the
 * underlying data strctures. See SparseSet and its specializations for more
 * details.
 *
 * @b Important
 *
 * Iterators aren't invalidated if:
 *
 * * New instances of the given components are created and assigned to entities.
 * * The entity currently pointed is modified (as an example, if one of the
 * given components is removed from the entity to which the iterator points).
 *
 * In all the other cases, modify the pools of the given components somehow
 * invalidates all the iterators and using them results in undefined behavior.
 *
 * @note
 * Views share references to the underlying data structures with the Registry
 * that generated them. Therefore any change to the entities and to the
 * components made by means of the registry are immediately reflected by views.
 *
 * @warning
 * Lifetime of a view must overcome the one of the registry that generated it.
 * In any other case, attempting to use a view results in undefined behavior.
 *
 * @sa View<Entity, Component>
 * @sa PersistentView
 *
 * @tparam Entity A valid entity type (see entt_traits for more details).
 * @tparam Component Types of components iterated by the view.
 */
template<typename Entity, typename... Component>
class View final {
    static_assert(sizeof...(Component) > 1, "!");

    /*! @brief A registry is allowed to create views. */
    friend class Registry<Entity>;

    template<typename Comp>
    using pool_type = SparseSet<Entity, Comp>;

    using view_type = SparseSet<Entity>;
    using underlying_iterator_type = typename view_type::iterator_type;
    using unchecked_type = std::array<const view_type *, (sizeof...(Component) - 1)>;
    using pattern_type = std::tuple<pool_type<Component> &...>;

    class Iterator {
        inline bool valid() const noexcept {
            auto i = unchecked.size();
            for(const auto entity = *begin; i && unchecked[i-1]->has(entity); --i);
            return !i;
        }

    public:
        using difference_type = typename underlying_iterator_type::difference_type;
        using value_type = typename view_type::entity_type;

        Iterator(unchecked_type unchecked, underlying_iterator_type begin, underlying_iterator_type end) noexcept
            : unchecked{unchecked}, begin{begin}, end{end}
        {
            if(begin != end && !valid()) {
                ++(*this);
            }
        }

        Iterator & operator++() noexcept {
            return (++begin != end && !valid()) ? ++(*this) : *this;
        }

        Iterator operator++(int) noexcept {
            Iterator orig = *this;
            return ++(*this), orig;
        }

        Iterator & operator+=(difference_type value) noexcept {
            begin += value;
            return *this;
        }

        Iterator operator+(difference_type value) noexcept {
            return Iterator{unchecked, begin+value, end};
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
        unchecked_type unchecked;
        underlying_iterator_type begin;
        underlying_iterator_type end;
    };

    View(pool_type<Component> &... pools) noexcept
        : pools{pools...}, view{nullptr}, unchecked{}
    {
        reset();
    }

public:
    /*! Input iterator type. */
    using iterator_type = Iterator;
    /*! @brief Underlying entity identifier. */
    using entity_type = typename view_type::entity_type;
    /*! @brief Unsigned integer type. */
    using size_type = typename view_type::size_type;

    /**
     * @brief Returns an iterator to the first entity that has the given
     * components.
     *
     * The returned iterator points to the first entity that has the given
     * components. If the view is empty, the returned iterator will be equal to
     * `end()`.
     *
     * @note
     * Input iterators stay true to the order imposed to the underlying data
     * structures.
     *
     * @return An iterator to the first entity that has the given components.
     */
    iterator_type begin() const noexcept {
        return Iterator{unchecked, view->begin(), view->end()};
    }

    /**
     * @brief Returns an iterator that is past the last entity that has the
     * given components.
     *
     * The returned iterator points to the entity following the last entity that
     * has the given components. Attempting to dereference the returned iterator
     * results in undefined behavior.
     *
     * @note
     * Input iterators stay true to the order imposed to the underlying data
     * structures.
     *
     * @return An iterator to the entity following the last entity that has the
     * given components.
     */
    iterator_type end() const noexcept {
        return Iterator{unchecked, view->end(), view->end()};
    }

    /**
     * @brief Returns the component assigned to the given entity.
     *
     * Prefer this function instead of `Registry::get` during iterations.
     * It has far better performance than its companion function.
     *
     * @warning
     * Attempting to use an invalid component type results in a compilation
     * error. Attempting to use an entity that doesn't belong to the view
     * results in undefined behavior.<br/>
     * An assertion will abort the execution at runtime in debug mode if
     * the view doesn't contain the given entity.
     *
     * @tparam Comp Type of the component to get.
     * @param entity A valid entity identifier.
     * @return The component assigned to the entity.
     */
    template<typename Comp>
    const Comp & get(entity_type entity) const noexcept {
        return std::get<pool_type<Comp> &>(pools).get(entity);
    }

    /**
     * @brief Returns the component assigned to the given entity.
     *
     * Prefer this function instead of `Registry::get` during iterations. It has
     * far better performance than its companion function.
     *
     * @warning
     * Attempting to use an invalid component type results in a compilation
     * error. Attempting to use an entity that doesn't belong to the view
     * results in undefined behavior.<br/>
     * An assertion will abort the execution at runtime in debug mode if
     * the view doesn't contain the given entity.
     *
     * @tparam Comp Type of the component to get.
     * @param entity A valid entity identifier.
     * @return The component assigned to the entity.
     */
    template<typename Comp>
    Comp & get(entity_type entity) noexcept {
        return const_cast<Comp &>(const_cast<const View *>(this)->get<Comp>(entity));
    }

    /**
     * @brief Returns the components assigned to the given entity.
     *
     * Prefer this function instead of `Registry::get` during iterations. It has
     * far better performance than its companion function.
     *
     * @warning
     * Attempting to use invalid component types results in a compilation error.
     * Attempting to use an entity that doesn't belong to the view results in
     * undefined behavior.<br/>
     * An assertion will abort the execution at runtime in debug mode if
     * the view doesn't contain the given entity.
     *
     * @tparam Comp Types of the components to get.
     * @param entity A valid entity identifier.
     * @return The components assigned to the entity.
     */
    template<typename... Comp>
    std::enable_if_t<(sizeof...(Comp) > 1), std::tuple<const Comp &...>>
    get(entity_type entity) const noexcept {
        return std::tuple<const Comp &...>{ get<Comp>(entity)... };
    }

    /**
     * @brief Returns the components assigned to the given entity.
     *
     * Prefer this function instead of `Registry::get` during iterations. It has
     * far better performance than its companion function.
     *
     * @warning
     * Attempting to use invalid component types results in a compilation error.
     * Attempting to use an entity that doesn't belong to the view results in
     * undefined behavior.<br/>
     * An assertion will abort the execution at runtime in debug mode if
     * the view doesn't contain the given entity.
     *
     * @tparam Comp Types of the components to get.
     * @param entity A valid entity identifier.
     * @return The components assigned to the entity.
     */
    template<typename... Comp>
    std::enable_if_t<(sizeof...(Comp) > 1), std::tuple<Comp &...>>
    get(entity_type entity) noexcept {
        return std::tuple<Comp &...>{ get<Comp>(entity)... };
    }

    /**
     * @brief Iterate the entities and applies them the given function object.
     *
     * The function object is invoked for each entity. It is provided with the
     * entity itself and a set of const references to all the components of the
     * view.<br/>
     * The signature of the function should be equivalent to the following:
     *
     * @code{.cpp}
     * void(entity_type, const Component &...);
     * @endcode
     *
     * @tparam Func Type of the function object to invoke.
     * @param func A valid function object.
     */
    template<typename Func>
    void each(Func func) const {
        for(auto entity: *this) {
            func(entity, get<Component>(entity)...);
        }
    }

    /**
     * @brief Iterate the entities and applies them the given function object.
     *
     * The function object is invoked for each entity. It is provided with the
     * entity itself and a set of references to all the components of the
     * view.<br/>
     * The signature of the function should be equivalent to the following:
     *
     * @code{.cpp}
     * void(entity_type, Component &...);
     * @endcode
     *
     * @tparam Func Type of the function object to invoke.
     * @param func A valid function object.
     */
    template<typename Func>
    void each(Func func) {
        const_cast<const View *>(this)->each([&func](entity_type entity, const Component &... component) {
            func(entity, const_cast<Component &>(component)...);
        });
    }

    /**
     * @brief Resets the view and reinitializes it.
     *
     * A multi component view keeps a reference to the smallest set of candidate
     * entities to iterate. Resetting a view means querying the underlying data
     * structures and reinitializing the view.<br/>
     * Use it only if copies of views are stored around and there is a
     * possibility that a component has become the best candidate in the
     * meantime.
     */
    void reset() {
        using accumulator_type = size_type[];
        size_type sz = std::max({ std::get<pool_type<Component> &>(pools).size()... }) + std::size_t{1};
        size_type pos{};

        auto probe = [this](auto sz, const auto &pool) {
            return pool.size() < sz ? (view = &pool, pool.size()) : sz;
        };

        auto filter = [this](auto pos, const auto &pool) {
            return (view != &pool) ? (unchecked[pos++] = &pool, pos) : pos;
        };

        accumulator_type probing = { (sz = probe(sz, std::get<pool_type<Component> &>(pools)))... };
        accumulator_type filtering = { (pos = filter(pos, std::get<pool_type<Component> &>(pools)))... };

        (void)filtering;
        (void)probing;
    }

private:
    pattern_type pools;
    const view_type *view;
    unchecked_type unchecked;
};


/**
 * @brief Single component view specialization.
 *
 * Single component views are specialized in order to get a boost in terms of
 * performance. This kind of views can access the underlying data structure
 * directly and avoid superflous checks.<br/>
 * Order of elements during iterations are highly dependent on the order of the
 * underlying data structure. See SparseSet and its specializations for more
 * details.
 *
 * @b Important
 *
 * Iterators aren't invalidated if:
 *
 * * New instances of the given components are created and assigned to entities.
 * * The entity currently pointed is modified (as an example, if one of the
 * given components is removed from the entity to which the iterator points).
 *
 * In all the other cases, modify the pools of the given components somehow
 * invalidates all the iterators and using them results in undefined behavior.
 *
 * @note
 * Views share a reference to the underlying data structure with the Registry
 * that generated them. Therefore any change to the entities and to the
 * components made by means of the registry are immediately reflected by views.
 *
 * @warning
 * Lifetime of a view must overcome the one of the registry that generated it.
 * In any other case, attempting to use a view results in undefined behavior.
 *
 * @sa View
 * @sa PersistentView
 *
 * @tparam Entity A valid entity type (see entt_traits for more details).
 * @tparam Component Type of the component iterated by the view.
 */
template<typename Entity, typename Component>
class View<Entity, Component> final {
    /*! @brief A registry is allowed to create views. */
    friend class Registry<Entity>;

    using pool_type = SparseSet<Entity, Component>;

    View(pool_type &pool) noexcept
        : pool{pool}
    {}

public:
    /*! Input iterator type. */
    using iterator_type = typename pool_type::iterator_type;
    /*! @brief Underlying entity identifier. */
    using entity_type = typename pool_type::entity_type;
    /*! @brief Unsigned integer type. */
    using size_type = typename pool_type::size_type;
    /*! Type of the component iterated by the view. */
    using raw_type = typename pool_type::object_type;

    /**
     * @brief Returns the number of entities that have the given component.
     * @return Number of entities that have the given component.
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
     * @note
     * There are no guarantees on the order of the components. Use `begin` and
     * `end` if you want to iterate the view in the expected order.
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
     * @note
     * There are no guarantees on the order of the components. Use `begin` and
     * `end` if you want to iterate the view in the expected order.
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
     * @note
     * There are no guarantees on the order of the entities. Use `begin` and
     * `end` if you want to iterate the view in the expected order.
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
     * @note
     * Input iterators stay true to the order imposed to the underlying data
     * structures.
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
     * @note
     * Input iterators stay true to the order imposed to the underlying data
     * structures.
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
     * Prefer this function instead of `Registry::get` during iterations. It has
     * far better performance than its companion function.
     *
     * @warning
     * Attempting to use an entity that doesn't belong to the view results in
     * undefined behavior.<br/>
     * An assertion will abort the execution at runtime in debug mode if the
     * view doesn't contain the given entity.
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
     * Prefer this function instead of `Registry::get` during iterations. It has
     * far better performance than its companion function.
     *
     * @warning
     * Attempting to use an entity that doesn't belong to the view results in
     * undefined behavior.<br/>
     * An assertion will abort the execution at runtime in debug mode if the
     * view doesn't contain the given entity.
     *
     * @param entity A valid entity identifier.
     * @return The component assigned to the entity.
     */
    Component & get(entity_type entity) noexcept {
        return const_cast<Component &>(const_cast<const View *>(this)->get(entity));
    }

    /**
     * @brief Iterate the entities and applies them the given function object.
     *
     * The function object is invoked for each entity. It is provided with the
     * entity itself and a const reference to the component of the view.<br/>
     * The signature of the function should be equivalent to the following:
     *
     * @code{.cpp}
     * void(entity_type, const Component &);
     * @endcode
     *
     * @tparam Func Type of the function object to invoke.
     * @param func A valid function object.
     */
    template<typename Func>
    void each(Func func) const {
        for(auto entity: *this) {
            func(entity, get(entity));
        }
    }

    /**
     * @brief Iterate the entities and applies them the given function object.
     *
     * The function object is invoked for each entity. It is provided with the
     * entity itself and a reference to the component of the view.<br/>
     * The signature of the function should be equivalent to the following:
     *
     * @code{.cpp}
     * void(entity_type, Component &);
     * @endcode
     *
     * @tparam Func Type of the function object to invoke.
     * @param func A valid function object.
     */
    template<typename Func>
    void each(Func func) {
        const_cast<const View *>(this)->each([&func](entity_type entity, const Component &component) {
            func(entity, const_cast<Component &>(component));
        });
    }

private:
    pool_type &pool;
};


}


#endif // ENTT_ENTITY_VIEW_HPP

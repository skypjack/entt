#ifndef ENTT_ENTITY_VIEW_HPP
#define ENTT_ENTITY_VIEW_HPP


#include <cassert>
#include <array>
#include <tuple>
#include <utility>
#include <algorithm>
#include <type_traits>
#include "../config/config.h"
#include "../core/ident.hpp"
#include "entt_traits.hpp"
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
 * is tightly packed in memory for fast iterations.<br/>
 * In general, persistent views don't stay true to the order of any set of
 * components unless users explicitly sort them.
 *
 * @b Important
 *
 * Iterators aren't invalidated if:
 *
 * * New instances of the given components are created and assigned to entities.
 * * The entity currently pointed is modified (as an example, if one of the
 *   given components is removed from the entity to which the iterator points).
 *
 * In all the other cases, modifying the pools of the given components in any
 * way invalidates all the iterators and using them results in undefined
 * behavior.
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
 * @sa RawView
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

    PersistentView(view_type &view, pool_type<Component> &... pools) ENTT_NOEXCEPT
        : view{view}, pools{pools...}
    {}

public:
    /*! @brief Input iterator type. */
    using iterator_type = typename view_type::iterator_type;
    /*! @brief Constant input iterator type. */
    using const_iterator_type = typename view_type::const_iterator_type;
    /*! @brief Underlying entity identifier. */
    using entity_type = typename view_type::entity_type;
    /*! @brief Unsigned integer type. */
    using size_type = typename view_type::size_type;

    /**
     * @brief Returns the number of entities that have the given components.
     * @return Number of entities that have the given components.
     */
    size_type size() const ENTT_NOEXCEPT {
        return view.size();
    }

    /**
     * @brief Checks whether the view is empty.
     * @return True if the view is empty, false otherwise.
     */
    bool empty() const ENTT_NOEXCEPT {
        return view.empty();
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
    const entity_type * data() const ENTT_NOEXCEPT {
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
    const_iterator_type cbegin() const ENTT_NOEXCEPT {
        return view.cbegin();
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
    iterator_type begin() ENTT_NOEXCEPT {
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
    const_iterator_type cend() const ENTT_NOEXCEPT {
        return view.cend();
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
    iterator_type end() ENTT_NOEXCEPT {
        return view.end();
    }

    /**
     * @brief Checks if a view contains an entity.
     * @param entity A valid entity identifier.
     * @return True if the view contains the given entity, false otherwise.
     */
    bool contains(entity_type entity) const ENTT_NOEXCEPT {
        return view.has(entity) && (view.data()[view.get(entity)] == entity);
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
     * An assertion will abort the execution at runtime in debug mode if the
     * view doesn't contain the given entity.
     *
     * @tparam Comp Type of component to get.
     * @param entity A valid entity identifier.
     * @return The component assigned to the entity.
     */
    template<typename Comp>
    const Comp & get(entity_type entity) const ENTT_NOEXCEPT {
        assert(contains(entity));
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
     * An assertion will abort the execution at runtime in debug mode if the
     * view doesn't contain the given entity.
     *
     * @tparam Comp Type of component to get.
     * @param entity A valid entity identifier.
     * @return The component assigned to the entity.
     */
    template<typename Comp>
    inline Comp & get(entity_type entity) ENTT_NOEXCEPT {
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
     * An assertion will abort the execution at runtime in debug mode if the
     * view doesn't contain the given entity.
     *
     * @tparam Comp Types of the components to get.
     * @param entity A valid entity identifier.
     * @return The components assigned to the entity.
     */
    template<typename... Comp>
    std::enable_if_t<(sizeof...(Comp) > 1), std::tuple<const Comp &...>>
    get(entity_type entity) const ENTT_NOEXCEPT {
        assert(contains(entity));
        return std::tuple<const Comp &...>{get<Comp>(entity)...};
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
     * An assertion will abort the execution at runtime in debug mode if the
     * view doesn't contain the given entity.
     *
     * @tparam Comp Types of the components to get.
     * @param entity A valid entity identifier.
     * @return The components assigned to the entity.
     */
    template<typename... Comp>
    std::enable_if_t<(sizeof...(Comp) > 1), std::tuple<Comp &...>>
    get(entity_type entity) ENTT_NOEXCEPT {
        assert(contains(entity));
        return std::tuple<Comp &...>{get<Comp>(entity)...};
    }

    /**
     * @brief Iterates entities and components and applies the given function
     * object to them.
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
        std::for_each(view.cbegin(), view.cend(), [&func, this](const auto entity) {
            func(entity, std::get<pool_type<Component> &>(pools).get(entity)...);
        });
    }

    /**
     * @brief Iterates entities and components and applies the given function
     * object to them.
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
    inline void each(Func func) {
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
     * @tparam Comp Type of component to use to impose the order.
     */
    template<typename Comp>
    void sort() {
        view.respect(std::get<pool_type<Comp> &>(pools));
    }

private:
    view_type &view;
    const pattern_type pools;
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
 *   given components is removed from the entity to which the iterator points).
 *
 * In all the other cases, modifying the pools of the given components in any
 * way invalidates all the iterators and using them results in undefined
 * behavior.
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
 * @sa RawView
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
    using underlying_iterator_type = typename view_type::const_iterator_type;
    using unchecked_type = std::array<const view_type *, (sizeof...(Component) - 1)>;
    using pattern_type = std::tuple<pool_type<Component> &...>;
    using traits_type = entt_traits<Entity>;

    class Iterator {
        using size_type = typename view_type::size_type;

        inline bool valid() const ENTT_NOEXCEPT {
            const auto entity = *begin;
            const auto sz = size_type(entity & traits_type::entity_mask);
            auto pos = unchecked.size();

            if(sz < extent) {
                for(; pos && unchecked[pos-1]->fast(entity); --pos);
            }

            return !pos;
        }

    public:
        using difference_type = typename underlying_iterator_type::difference_type;
        using value_type = typename underlying_iterator_type::value_type;
        using pointer = typename underlying_iterator_type::pointer;
        using reference = typename underlying_iterator_type::reference;
        using iterator_category = typename underlying_iterator_type::iterator_category;

        Iterator(unchecked_type unchecked, size_type extent, underlying_iterator_type begin, underlying_iterator_type end) ENTT_NOEXCEPT
            : unchecked{unchecked},
              extent{extent},
              begin{begin},
              end{end}
        {
            if(begin != end && !valid()) {
                ++(*this);
            }
        }

        Iterator & operator++() ENTT_NOEXCEPT {
            return (++begin != end && !valid()) ? ++(*this) : *this;
        }

        Iterator operator++(int) ENTT_NOEXCEPT {
            Iterator orig = *this;
            return ++(*this), orig;
        }

        Iterator & operator+=(difference_type value) ENTT_NOEXCEPT {
            return ((begin += value) != end && !valid()) ? ++(*this) : *this;
        }

        Iterator operator+(difference_type value) ENTT_NOEXCEPT {
            return Iterator{unchecked, extent, begin+value, end};
        }

        bool operator==(const Iterator &other) const ENTT_NOEXCEPT {
            return other.begin == begin;
        }

        inline bool operator!=(const Iterator &other) const ENTT_NOEXCEPT {
            return !(*this == other);
        }

        value_type operator*() const ENTT_NOEXCEPT {
            return *begin;
        }

    private:
        const unchecked_type unchecked;
        const size_type extent;
        underlying_iterator_type begin;
        underlying_iterator_type end;
    };

    View(pool_type<Component> &... pools) ENTT_NOEXCEPT
        : pools{pools...}, view{nullptr}, unchecked{}, idx{}
    {
        reset();
    }

    template<typename Comp, typename Other, typename It>
    inline std::enable_if_t<std::is_same<Comp, Other>::value, const Other &>
    get(It &it, Entity) const { return *(it++); }

    template<typename Comp, typename Other, typename It>
    inline std::enable_if_t<!std::is_same<Comp, Other>::value, const Other &>
    get(const It &, Entity entity) const { return std::get<pool_type<Other> &>(pools).get(entity); }

    template<typename Comp, typename Func>
    void each(Func func) const {
        const auto extent = std::min({ std::get<pool_type<Component> &>(pools).extent()... });
        auto &pool = std::get<pool_type<Comp> &>(pools);

        std::for_each(pool.view_type::cbegin(), pool.view_type::cend(), [func = std::move(func), raw = pool.cbegin(), extent, this](const auto entity) mutable {
            const auto sz = size_type(entity & traits_type::entity_mask);

            if(sz < extent) {
                auto pos = unchecked.size();

                for(; pos && unchecked[pos-1]->fast(entity); --pos);

                if(!pos) {
                    // avoided indirections due to the sparse set for the pivot (this-> required because of GCC 6)
                    func(entity, this->get<Comp, Component>(raw, entity)...);
                }
            }
        });
    }

public:
    /*! @brief Input iterator type. */
    using iterator_type = Iterator;
    /*! @brief Constant input iterator type. */
    using const_iterator_type = Iterator;
    /*! @brief Underlying entity identifier. */
    using entity_type = typename view_type::entity_type;
    /*! @brief Unsigned integer type. */
    using size_type = typename view_type::size_type;

    /**
     * @brief Estimates the number of entities that have the given components.
     * @return Estimated number of entities that have the given components.
     */
    size_type size() const ENTT_NOEXCEPT {
        return view->size();
    }

    /**
     * @brief Checks if the view is definitely empty.
     * @return True if the view is definitely empty, false otherwise.
     */
    bool empty() const ENTT_NOEXCEPT {
        return view->empty();
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
    const_iterator_type cbegin() const ENTT_NOEXCEPT {
        const auto extent = std::min({ std::get<pool_type<Component> &>(pools).extent()... });
        return iterator_type{ unchecked, extent, view->cbegin(), view->cend() };
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
    inline iterator_type begin() ENTT_NOEXCEPT {
        return cbegin();
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
    const_iterator_type cend() const ENTT_NOEXCEPT {
        const auto extent = std::min({ std::get<pool_type<Component> &>(pools).extent()... });
        return iterator_type{ unchecked, extent, view->cend(), view->cend() };
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
    inline iterator_type end() ENTT_NOEXCEPT {
        return cend();
    }

    /**
     * @brief Checks if a view contains an entity.
     * @param entity A valid entity identifier.
     * @return True if the view contains the given entity, false otherwise.
     */
    bool contains(entity_type entity) const ENTT_NOEXCEPT {
        const auto extent = std::min({ std::get<pool_type<Component> &>(pools).extent()... });
        const auto sz = size_type(entity & traits_type::entity_mask);
        auto pos = unchecked.size();

        if(sz < extent && view->has(entity) && (view->data()[view->get(entity)] == entity)) {
            for(; pos && unchecked[pos-1]->fast(entity); --pos);
        }

        return !pos;
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
     * An assertion will abort the execution at runtime in debug mode if the
     * view doesn't contain the given entity.
     *
     * @tparam Comp Type of component to get.
     * @param entity A valid entity identifier.
     * @return The component assigned to the entity.
     */
    template<typename Comp>
    const Comp & get(entity_type entity) const ENTT_NOEXCEPT {
        assert(contains(entity));
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
     * An assertion will abort the execution at runtime in debug mode if the
     * view doesn't contain the given entity.
     *
     * @tparam Comp Type of component to get.
     * @param entity A valid entity identifier.
     * @return The component assigned to the entity.
     */
    template<typename Comp>
    inline Comp & get(entity_type entity) ENTT_NOEXCEPT {
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
     * An assertion will abort the execution at runtime in debug mode if the
     * view doesn't contain the given entity.
     *
     * @tparam Comp Types of the components to get.
     * @param entity A valid entity identifier.
     * @return The components assigned to the entity.
     */
    template<typename... Comp>
    std::enable_if_t<(sizeof...(Comp) > 1), std::tuple<const Comp &...>>
    get(entity_type entity) const ENTT_NOEXCEPT {
        assert(contains(entity));
        return std::tuple<const Comp &...>{get<Comp>(entity)...};
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
     * An assertion will abort the execution at runtime in debug mode if the
     * view doesn't contain the given entity.
     *
     * @tparam Comp Types of the components to get.
     * @param entity A valid entity identifier.
     * @return The components assigned to the entity.
     */
    template<typename... Comp>
    std::enable_if_t<(sizeof...(Comp) > 1), std::tuple<Comp &...>>
    get(entity_type entity) ENTT_NOEXCEPT {
        assert(contains(entity));
        return std::tuple<Comp &...>{get<Comp>(entity)...};
    }

    /**
     * @brief Iterates entities and components and applies the given function
     * object to them.
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
    inline void each(Func func) const {
        constexpr auto indexes = ident<Component...>;
        using accumulator_type = int[];
        accumulator_type accumulator = { (indexes.template get<Component>() == idx ? (each<Component>(std::move(func)), 0) : 0)... };
        (void)accumulator;
    }

    /**
     * @brief Iterates entities and components and applies the given function
     * object to them.
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
    inline void each(Func func) {
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
        size_type next{};

        auto probe = [this](auto sz, const auto &pool) {
            return pool.size() < sz ? (view = &pool, pool.size()) : sz;
        };

        auto filter = [this](auto next, const auto &pool) {
            return (view == &pool) ? (idx = next) : (unchecked[next++] = &pool, next);
        };

        accumulator_type probing = { (sz = probe(sz, std::get<pool_type<Component> &>(pools)))... };
        accumulator_type filtering = { (next = filter(next, std::get<pool_type<Component> &>(pools)))... };

        (void)filtering;
        (void)probing;
    }

private:
    const pattern_type pools;
    const view_type *view;
    unchecked_type unchecked;
    size_type idx;
};


/**
 * @brief Single component view specialization.
 *
 * Single component views are specialized in order to get a boost in terms of
 * performance. This kind of views can access the underlying data structure
 * directly and avoid superfluous checks.<br/>
 * Order of elements during iterations are highly dependent on the order of the
 * underlying data structure. See SparseSet and its specializations for more
 * details.
 *
 * @b Important
 *
 * Iterators aren't invalidated if:
 *
 * * New instances of the given component are created and assigned to entities.
 * * The entity currently pointed is modified (as an example, the given
 *   component is removed from the entity to which the iterator points).
 *
 * In all the other cases, modifying the pool of the given component in any way
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
 * @sa RawView
 *
 * @tparam Entity A valid entity type (see entt_traits for more details).
 * @tparam Component Type of component iterated by the view.
 */
template<typename Entity, typename Component>
class View<Entity, Component> final {
    /*! @brief A registry is allowed to create views. */
    friend class Registry<Entity>;

    using view_type = SparseSet<Entity>;
    using pool_type = SparseSet<Entity, Component>;

    View(pool_type &pool) ENTT_NOEXCEPT
        : pool{pool}
    {}

public:
    /*! @brief Input iterator type. */
    using iterator_type = typename view_type::iterator_type;
    /*! @brief Constant input iterator type. */
    using const_iterator_type = typename view_type::const_iterator_type;
    /*! @brief Underlying entity identifier. */
    using entity_type = typename pool_type::entity_type;
    /*! @brief Unsigned integer type. */
    using size_type = typename pool_type::size_type;
    /*! @brief Type of component iterated by the view. */
    using raw_type = typename pool_type::object_type;

    /**
     * @brief Returns the number of entities that have the given component.
     * @return Number of entities that have the given component.
     */
    size_type size() const ENTT_NOEXCEPT {
        return pool.size();
    }

    /**
     * @brief Checks whether the view is empty.
     * @return True if the view is empty, false otherwise.
     */
    bool empty() const ENTT_NOEXCEPT {
        return pool.empty();
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
    const raw_type * raw() const ENTT_NOEXCEPT {
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
    inline raw_type * raw() ENTT_NOEXCEPT {
        return const_cast<raw_type *>(const_cast<const View *>(this)->raw());
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
    const entity_type * data() const ENTT_NOEXCEPT {
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
    const_iterator_type cbegin() const ENTT_NOEXCEPT {
        return pool.view_type::cbegin();
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
    iterator_type begin() ENTT_NOEXCEPT {
        return pool.view_type::begin();
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
    const_iterator_type cend() const ENTT_NOEXCEPT {
        return pool.view_type::cend();
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
    iterator_type end() ENTT_NOEXCEPT {
        return pool.view_type::end();
    }

    /**
     * @brief Checks if a view contains an entity.
     * @param entity A valid entity identifier.
     * @return True if the view contains the given entity, false otherwise.
     */
    bool contains(entity_type entity) const ENTT_NOEXCEPT {
        return pool.has(entity) && (pool.data()[pool.view_type::get(entity)] == entity);
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
    const Component & get(entity_type entity) const ENTT_NOEXCEPT {
        assert(contains(entity));
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
    inline Component & get(entity_type entity) ENTT_NOEXCEPT {
        return const_cast<Component &>(const_cast<const View *>(this)->get(entity));
    }

    /**
     * @brief Iterates entities and components and applies the given function
     * object to them.
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
        std::for_each(pool.view_type::cbegin(), pool.view_type::cend(), [&func, raw = pool.cbegin()](const auto entity) mutable {
            func(entity, *(raw++));
        });
    }

    /**
     * @brief Iterates entities and components and applies the given function
     * object to them.
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
    inline void each(Func func) {
        const_cast<const View *>(this)->each([&func](entity_type entity, const Component &component) {
            func(entity, const_cast<Component &>(component));
        });
    }

private:
    pool_type &pool;
};


/**
 * @brief Raw view.
 *
 * Raw views are meant to easily iterate components without having to resort to
 * using any other member function, so as to further increase the performance.
 * Whenever knowing the entity to which a component belongs isn't required, this
 * should be the preferred tool.<br/>
 * Order of elements during iterations are highly dependent on the order of the
 * underlying data structure. See SparseSet and its specializations for more
 * details.
 *
 * @b Important
 *
 * Iterators aren't invalidated if:
 *
 * * New instances of the given component are created and assigned to entities.
 * * The entity to which the component belongs is modified (as an example, the
 *   given component is destroyed).
 *
 * In all the other cases, modifying the pool of the given component in any way
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
 * @sa View<Entity, Component>
 * @sa PersistentView
 *
 * @tparam Entity A valid entity type (see entt_traits for more details).
 * @tparam Component Type of component iterated by the view.
 */
template<typename Entity, typename Component>
class RawView final {
    /*! @brief A registry is allowed to create views. */
    friend class Registry<Entity>;

    using pool_type = SparseSet<Entity, Component>;

    RawView(pool_type &pool) ENTT_NOEXCEPT
        : pool{pool}
    {}

public:
    /*! @brief Input iterator type. */
    using iterator_type = typename pool_type::iterator_type;
    /*! @brief Constant input iterator type. */
    using const_iterator_type = typename pool_type::const_iterator_type;
    /*! @brief Underlying entity identifier. */
    using entity_type = typename pool_type::entity_type;
    /*! @brief Unsigned integer type. */
    using size_type = typename pool_type::size_type;
    /*! @brief Type of component iterated by the view. */
    using raw_type = typename pool_type::object_type;

    /**
     * @brief Returns the number of instances of the given type.
     * @return Number of instances of the given component.
     */
    size_type size() const ENTT_NOEXCEPT {
        return pool.size();
    }

    /**
     * @brief Checks whether the view is empty.
     * @return True if the view is empty, false otherwise.
     */
    bool empty() const ENTT_NOEXCEPT {
        return pool.empty();
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
    const raw_type * raw() const ENTT_NOEXCEPT {
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
    inline raw_type * raw() ENTT_NOEXCEPT {
        return const_cast<raw_type *>(const_cast<const RawView *>(this)->raw());
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
    const entity_type * data() const ENTT_NOEXCEPT {
        return pool.data();
    }

    /**
     * @brief Returns an iterator to the first instance of the given type.
     *
     * The returned iterator points to the first instance of the given type. If
     * the view is empty, the returned iterator will be equal to `end()`.
     *
     * @note
     * Input iterators stay true to the order imposed to the underlying data
     * structures.
     *
     * @return An iterator to the first instance of the given type.
     */
    const_iterator_type cbegin() const ENTT_NOEXCEPT {
        return pool.cbegin();
    }

    /**
     * @brief Returns an iterator to the first instance of the given type.
     *
     * The returned iterator points to the first instance of the given type. If
     * the view is empty, the returned iterator will be equal to `end()`.
     *
     * @note
     * Input iterators stay true to the order imposed to the underlying data
     * structures.
     *
     * @return An iterator to the first instance of the given type.
     */
    iterator_type begin() ENTT_NOEXCEPT {
        return pool.begin();
    }

    /**
     * @brief Returns an iterator that is past the last instance of the given
     * type.
     *
     * The returned iterator points to the element following the last instance
     * of the given type. Attempting to dereference the returned iterator
     * results in undefined behavior.
     *
     * @note
     * Input iterators stay true to the order imposed to the underlying data
     * structures.
     *
     * @return An iterator to the element following the last instance of the
     * given type.
     */
    const_iterator_type cend() const ENTT_NOEXCEPT {
        return pool.cend();
    }

    /**
     * @brief Returns an iterator that is past the last instance of the given
     * type.
     *
     * The returned iterator points to the element following the last instance
     * of the given type. Attempting to dereference the returned iterator
     * results in undefined behavior.
     *
     * @note
     * Input iterators stay true to the order imposed to the underlying data
     * structures.
     *
     * @return An iterator to the element following the last instance of the
     * given type.
     */
    iterator_type end() ENTT_NOEXCEPT {
        return pool.end();
    }

    /**
     * @brief Iterates components and applies the given function object to them.
     *
     * The function object is provided with a const reference to each component
     * of the view.<br/>
     * The signature of the function should be equivalent to the following:
     *
     * @code{.cpp}
     * void(const Component &);
     * @endcode
     *
     * @tparam Func Type of the function object to invoke.
     * @param func A valid function object.
     */
    template<typename Func>
    void each(Func func) const {
        std::for_each(pool.cbegin(), pool.cend(), func);
    }

    /**
     * @brief Iterates components and applies the given function object to them.
     *
     * The function object is provided with a const reference to each component
     * of the view.<br/>
     * The signature of the function should be equivalent to the following:
     *
     * @code{.cpp}
     * void(const Component &);
     * @endcode
     *
     * @tparam Func Type of the function object to invoke.
     * @param func A valid function object.
     */
    template<typename Func>
    void each(Func func) {
        std::for_each(pool.begin(), pool.end(), func);
    }

private:
    pool_type &pool;
};


}


#endif // ENTT_ENTITY_VIEW_HPP

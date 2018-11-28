#ifndef ENTT_ENTITY_VIEW_HPP
#define ENTT_ENTITY_VIEW_HPP


#include <iterator>
#include <cassert>
#include <cstddef>
#include <array>
#include <tuple>
#include <vector>
#include <utility>
#include <algorithm>
#include <type_traits>
#include "../config/config.h"
#include "entt_traits.hpp"
#include "sparse_set.hpp"


namespace entt {


/**
 * @brief Forward declaration of the registry class.
 */
template<typename>
class registry;


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
 * Views share references to the underlying data structures with the registry
 * that generated them. Therefore any change to the entities and to the
 * components made by means of the registry are immediately reflected by all the
 * views.<br/>
 * Moreover, sorting a persistent view affects all the other views of the same
 * type (it means that users don't have to call `sort` on each view to sort all
 * of them because they share the set of entities).
 *
 * @warning
 * Lifetime of a view must overcome the one of the registry that generated it.
 * In any other case, attempting to use a view results in undefined behavior.
 *
 * @sa view
 * @sa view<Entity, Component>
 * @sa raw_view
 * @sa runtime_view
 *
 * @tparam Entity A valid entity type (see entt_traits for more details).
 * @tparam Component Types of components iterated by the view.
 */
template<typename Entity, typename... Component>
class persistent_view final {
    static_assert(sizeof...(Component) > 1);

    /*! @brief A registry is allowed to create views. */
    friend class registry<Entity>;

    template<typename Comp>
    using pool_type = std::conditional_t<std::is_const_v<Comp>, const sparse_set<Entity, std::remove_const_t<Comp>>, sparse_set<Entity, Comp>>;

    using view_type = sparse_set<Entity>;
    using persistent_type = sparse_set<Entity, std::array<typename view_type::size_type, sizeof...(Component)>>;
    using pattern_type = std::tuple<pool_type<Component> *...>;

    // we could use pool_type<Component> *..., but vs complains about it and refuses to compile for unknown reasons (likely a bug)
    persistent_view(persistent_type *handler, sparse_set<Entity, std::remove_const_t<Component>> *... pools) ENTT_NOEXCEPT
        : handler{handler},
          pools{pools...}
    {}

    template<typename Comp>
    auto * pool() const ENTT_NOEXCEPT {
        using comp_type = std::conditional_t<std::disjunction_v<std::is_same<Comp, Component>...>, Comp, std::remove_const_t<Comp>>;
        return std::get<pool_type<comp_type> *>(pools);
    }

    const view_type * candidate() const ENTT_NOEXCEPT {
        return std::min({ static_cast<const view_type *>(pool<Component>())... }, [](const auto *lhs, const auto *rhs) {
            return lhs->size() < rhs->size();
        });
    }

    template<typename Func, std::size_t... Indexes>
    void each(Func func, std::index_sequence<Indexes...>) const {
        std::for_each(handler->view_type::begin(), handler->view_type::end(), [func = std::move(func), raw = handler->cbegin(), this](const auto entity) mutable {
            func(entity, pool<Component>()->raw()[(*raw)[Indexes]]...);
            ++raw;
        });
    }

public:
    /*! @brief Underlying entity identifier. */
    using entity_type = typename view_type::entity_type;
    /*! @brief Unsigned integer type. */
    using size_type = typename view_type::size_type;
    /*! @brief Input iterator type. */
    using iterator_type = typename view_type::iterator_type;

    /*! @brief Default copy constructor. */
    persistent_view(const persistent_view &) = default;
    /*! @brief Default move constructor. */
    persistent_view(persistent_view &&) = default;

    /*! @brief Default copy assignment operator. @return This view. */
    persistent_view & operator=(const persistent_view &) = default;
    /*! @brief Default move assignment operator. @return This view. */
    persistent_view & operator=(persistent_view &&) = default;

    /**
     * @brief Returns the number of entities that have the given components.
     * @return Number of entities that have the given components.
     */
    size_type size() const ENTT_NOEXCEPT {
        return handler->size();
    }

    /**
     * @brief Checks whether the view is empty.
     * @return True if the view is empty, false otherwise.
     */
    bool empty() const ENTT_NOEXCEPT {
        return handler->empty();
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
        return handler->data();
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
    iterator_type begin() const ENTT_NOEXCEPT {
        return handler->view_type::begin();
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
    iterator_type end() const ENTT_NOEXCEPT {
        return handler->view_type::end();
    }

    /**
     * @brief Finds an entity.
     * @param entity A valid entity identifier.
     * @return An iterator to the given entity if it's found, past the end
     * iterator otherwise.
     */
    iterator_type find(const entity_type entity) const ENTT_NOEXCEPT {
        return handler->view_type::find(entity);
    }

    /**
     * @brief Returns the identifier that occupies the given position.
     * @param pos Position of the element to return.
     * @return The identifier that occupies the given position.
     */
    entity_type operator[](const size_type pos) const ENTT_NOEXCEPT {
        return handler->view_type::begin()[pos];
    }

    /**
     * @brief Checks if a view contains an entity.
     * @param entity A valid entity identifier.
     * @return True if the view contains the given entity, false otherwise.
     */
    bool contains(const entity_type entity) const ENTT_NOEXCEPT {
        return handler->has(entity) && (handler->view_type::data()[handler->view_type::get(entity)] == entity);
    }

    /**
     * @brief Returns the components assigned to the given entity.
     *
     * Prefer this function instead of `registry::get` during iterations. It has
     * far better performance than its companion function.
     *
     * @warning
     * Attempting to use an invalid component type results in a compilation
     * error. Attempting to use an entity that doesn't belong to the view
     * results in undefined behavior.<br/>
     * An assertion will abort the execution at runtime in debug mode if the
     * view doesn't contain the given entity.
     *
     * @tparam Comp Types of components to get.
     * @param entity A valid entity identifier.
     * @return The components assigned to the entity.
     */
    template<typename... Comp>
    std::conditional_t<sizeof...(Comp) == 1, std::tuple_element_t<0, std::tuple<Comp &...>>, std::tuple<Comp &...>>
    get([[maybe_unused]] const entity_type entity) const ENTT_NOEXCEPT {
        assert(contains(entity));

        if constexpr(sizeof...(Comp) == 1) {
            static_assert(std::disjunction_v<std::is_same<Comp..., Component>..., std::is_same<std::remove_const_t<Comp>..., Component>...>);
            return (pool<Comp>()->get(entity), ...);
        } else {
            return std::tuple<Comp &...>{get<Comp>(entity)...};
        }
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
     * void(const entity_type, Component &...);
     * @endcode
     *
     * @tparam Func Type of the function object to invoke.
     * @param func A valid function object.
     */
    template<typename Func>
    inline void each(Func func) const {
        each(std::move(func), std::make_index_sequence<sizeof...(Component)>{});
    }

    /**
     * @brief Sort the shared pool of entities according to the given component.
     *
     * Persistent views of the same type share with the registry a pool of
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
    void sort() const {
        handler->respect(*pool<Comp>());
    }

private:
    persistent_type *handler;
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
 * underlying data structures. See sparse_set and its specializations for more
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
 * Views share references to the underlying data structures with the registry
 * that generated them. Therefore any change to the entities and to the
 * components made by means of the registry are immediately reflected by views.
 *
 * @warning
 * Lifetime of a view must overcome the one of the registry that generated it.
 * In any other case, attempting to use a view results in undefined behavior.
 *
 * @sa view<Entity, Component>
 * @sa persistent_view
 * @sa raw_view
 * @sa runtime_view
 *
 * @tparam Entity A valid entity type (see entt_traits for more details).
 * @tparam Component Types of components iterated by the view.
 */
template<typename Entity, typename... Component>
class view final {
    static_assert(sizeof...(Component) > 1);

    /*! @brief A registry is allowed to create views. */
    friend class registry<Entity>;

    template<typename Comp>
    using pool_type = std::conditional_t<std::is_const_v<Comp>, const sparse_set<Entity, std::remove_const_t<Comp>>, sparse_set<Entity, Comp>>;

    template<typename Comp>
    using component_iterator_type = decltype(std::declval<pool_type<Comp>>().begin());

    using view_type = sparse_set<Entity>;
    using underlying_iterator_type = typename view_type::iterator_type;
    using unchecked_type = std::array<const view_type *, (sizeof...(Component) - 1)>;
    using pattern_type = std::tuple<pool_type<Component> *...>;
    using traits_type = entt_traits<Entity>;

    class iterator {
        friend class view<Entity, Component...>;

        using extent_type = typename view_type::size_type;

        iterator(unchecked_type unchecked, underlying_iterator_type begin, underlying_iterator_type end) ENTT_NOEXCEPT
            : unchecked{unchecked},
              begin{begin},
              end{end},
              extent{min(std::make_index_sequence<unchecked.size()>{})}
        {
            if(begin != end && !valid()) {
                ++(*this);
            }
        }

        template<std::size_t... Indexes>
        extent_type min(std::index_sequence<Indexes...>) const ENTT_NOEXCEPT {
            return std::min({ std::get<Indexes>(unchecked)->extent()... });
        }

        bool valid() const ENTT_NOEXCEPT {
            const auto entity = *begin;
            const auto sz = size_type(entity & traits_type::entity_mask);

            return sz < extent && std::all_of(unchecked.cbegin(), unchecked.cend(), [entity](const view_type *view) {
                return view->fast(entity);
            });
        }

    public:
        using difference_type = typename underlying_iterator_type::difference_type;
        using value_type = typename underlying_iterator_type::value_type;
        using pointer = typename underlying_iterator_type::pointer;
        using reference = typename underlying_iterator_type::reference;
        using iterator_category = std::forward_iterator_tag;

        iterator() ENTT_NOEXCEPT = default;

        iterator(const iterator &) ENTT_NOEXCEPT = default;
        iterator & operator=(const iterator &) ENTT_NOEXCEPT = default;

        iterator & operator++() ENTT_NOEXCEPT {
            return (++begin != end && !valid()) ? ++(*this) : *this;
        }

        iterator operator++(int) ENTT_NOEXCEPT {
            iterator orig = *this;
            return ++(*this), orig;
        }

        bool operator==(const iterator &other) const ENTT_NOEXCEPT {
            return other.begin == begin;
        }

        inline bool operator!=(const iterator &other) const ENTT_NOEXCEPT {
            return !(*this == other);
        }

        pointer operator->() const ENTT_NOEXCEPT {
            return begin.operator->();
        }

        inline reference operator*() const ENTT_NOEXCEPT {
            return *operator->();
        }

    private:
        unchecked_type unchecked;
        underlying_iterator_type begin;
        underlying_iterator_type end;
        extent_type extent;
    };

    // we could use pool_type<Component> *..., but vs complains about it and refuses to compile for unknown reasons (likely a bug)
    view(sparse_set<Entity, std::remove_const_t<Component>> *... pools) ENTT_NOEXCEPT
        : pools{pools...}
    {}

    template<typename Comp>
    auto * pool() const ENTT_NOEXCEPT {
        using comp_type = std::conditional_t<std::disjunction_v<std::is_same<Comp, Component>...>, Comp, std::remove_const_t<Comp>>;
        return std::get<pool_type<comp_type> *>(pools);
    }

    const view_type * candidate() const ENTT_NOEXCEPT {
        return std::min({ static_cast<const view_type *>(pool<Component>())... }, [](const auto *lhs, const auto *rhs) {
            return lhs->size() < rhs->size();
        });
    }

    unchecked_type unchecked(const view_type *view) const ENTT_NOEXCEPT {
        unchecked_type other{};
        std::size_t pos{};
        ((pool<Component>() == view ? nullptr : (other[pos++] = pool<Component>())), ...);
        return other;
    }

    template<typename Comp, typename Other>
    inline Other & get([[maybe_unused]] component_iterator_type<Comp> it, [[maybe_unused]] const Entity entity) const ENTT_NOEXCEPT {
        if constexpr(std::is_same_v<Comp, Other>) {
            return *it;
        } else {
            return pool<Other>()->get(entity);
        }
    }

    template<typename Comp, typename Func, std::size_t... Indexes>
    void each(pool_type<Comp> *cpool, Func func, std::index_sequence<Indexes...>) const {
        const auto other = unchecked(cpool);
        std::array<underlying_iterator_type, sizeof...(Indexes)> data{{std::get<Indexes>(other)->begin()...}};
        const auto extent = std::min({ pool<Component>()->extent()... });
        auto raw = std::make_tuple(pool<Component>()->begin()...);
        const auto end = cpool->view_type::end();
        auto begin = cpool->view_type::begin();

        // we can directly use the raw iterators if pools are ordered
        while(((begin != end) && ... && (*begin == *(std::get<Indexes>(data)++)))) {
            func(*(begin++), *(std::get<component_iterator_type<Component>>(raw)++)...);
        }

        // fallback to visit what remains using indirections
        while(begin != end) {
            const auto entity = *(begin++);
            const auto it = std::get<component_iterator_type<Comp>>(raw)++;
            const auto sz = size_type(entity & traits_type::entity_mask);

            if(((sz < extent) && ... && std::get<Indexes>(other)->fast(entity))) {
                // avoided at least the indirection due to the sparse set for the pivot type (see get for more details)
                func(entity, get<Comp, Component>(it, entity)...);
            }
        }
    }

public:
    /*! @brief Underlying entity identifier. */
    using entity_type = typename view_type::entity_type;
    /*! @brief Unsigned integer type. */
    using size_type = typename view_type::size_type;
    /*! @brief Input iterator type. */
    using iterator_type = iterator;

    /*! @brief Default copy constructor. */
    view(const view &) = default;
    /*! @brief Default move constructor. */
    view(view &&) = default;

    /*! @brief Default copy assignment operator. @return This view. */
    view & operator=(const view &) = default;
    /*! @brief Default move assignment operator. @return This view. */
    view & operator=(view &&) = default;

    /**
     * @brief Estimates the number of entities that have the given components.
     * @return Estimated number of entities that have the given components.
     */
    size_type size() const ENTT_NOEXCEPT {
        return std::min({ pool<Component>()->size()... });
    }

    /**
     * @brief Checks if the view is definitely empty.
     * @return True if the view is definitely empty, false otherwise.
     */
    bool empty() const ENTT_NOEXCEPT {
        return (pool<Component>()->empty() || ...);
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
    iterator_type begin() const ENTT_NOEXCEPT {
        const auto *view = candidate();
        return iterator_type{unchecked(view), view->begin(), view->end()};
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
    iterator_type end() const ENTT_NOEXCEPT {
        const auto *view = candidate();
        return iterator_type{unchecked(view), view->end(), view->end()};
    }

    /**
     * @brief Finds an entity.
     * @param entity A valid entity identifier.
     * @return An iterator to the given entity if it's found, past the end
     * iterator otherwise.
     */
    iterator_type find(const entity_type entity) const ENTT_NOEXCEPT {
        const auto *view = candidate();
        iterator_type it{unchecked(view), view->find(entity), view->end()};
        return (it != end() && *it == entity) ? it : end();
    }

    /**
     * @brief Checks if a view contains an entity.
     * @param entity A valid entity identifier.
     * @return True if the view contains the given entity, false otherwise.
     */
    bool contains(const entity_type entity) const ENTT_NOEXCEPT {
        const auto sz = size_type(entity & traits_type::entity_mask);
        const auto extent = std::min({ pool<Component>()->extent()... });
        return ((sz < extent) && ... && (pool<Component>()->has(entity) && (pool<Component>()->data()[pool<Component>()->view_type::get(entity)] == entity)));
    }

    /**
     * @brief Returns the components assigned to the given entity.
     *
     * Prefer this function instead of `registry::get` during iterations. It has
     * far better performance than its companion function.
     *
     * @warning
     * Attempting to use an invalid component type results in a compilation
     * error. Attempting to use an entity that doesn't belong to the view
     * results in undefined behavior.<br/>
     * An assertion will abort the execution at runtime in debug mode if the
     * view doesn't contain the given entity.
     *
     * @tparam Comp Types of components to get.
     * @param entity A valid entity identifier.
     * @return The components assigned to the entity.
     */
    template<typename... Comp>
    std::conditional_t<sizeof...(Comp) == 1, std::tuple_element_t<0, std::tuple<Comp &...>>, std::tuple<Comp &...>>
    get([[maybe_unused]] const entity_type entity) const ENTT_NOEXCEPT {
        assert(contains(entity));

        if constexpr(sizeof...(Comp) == 1) {
            static_assert(std::disjunction_v<std::is_same<Comp..., Component>..., std::is_same<std::remove_const_t<Comp>..., Component>...>);
            return (pool<Comp>()->get(entity), ...);
        } else {
            return std::tuple<Comp &...>{get<Comp>(entity)...};
        }
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
     * void(const entity_type, Component &...);
     * @endcode
     *
     * @tparam Func Type of the function object to invoke.
     * @param func A valid function object.
     */
    template<typename Func>
    void each(Func func) const {
        const auto *view = candidate();
        ((pool<Component>() == view ? each<Component>(pool<Component>(), std::move(func), std::make_index_sequence<sizeof...(Component)-1>{}) : void()), ...);
    }

private:
    const pattern_type pools;
};


/**
 * @brief Single component view specialization.
 *
 * Single component views are specialized in order to get a boost in terms of
 * performance. This kind of views can access the underlying data structure
 * directly and avoid superfluous checks.<br/>
 * Order of elements during iterations are highly dependent on the order of the
 * underlying data structure. See sparse_set and its specializations for more
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
 * Views share a reference to the underlying data structure with the registry
 * that generated them. Therefore any change to the entities and to the
 * components made by means of the registry are immediately reflected by views.
 *
 * @warning
 * Lifetime of a view must overcome the one of the registry that generated it.
 * In any other case, attempting to use a view results in undefined behavior.
 *
 * @sa view
 * @sa persistent_view
 * @sa raw_view
 * @sa runtime_view
 *
 * @tparam Entity A valid entity type (see entt_traits for more details).
 * @tparam Component Type of component iterated by the view.
 */
template<typename Entity, typename Component>
class view<Entity, Component> final {
    /*! @brief A registry is allowed to create views. */
    friend class registry<Entity>;

    using view_type = sparse_set<Entity>;
    using pool_type = std::conditional_t<std::is_const_v<Component>, const sparse_set<Entity, std::remove_const_t<Component>>, sparse_set<Entity, Component>>;

    view(pool_type *pool) ENTT_NOEXCEPT
        : pool{pool}
    {}

public:
    /*! @brief Type of component iterated by the view. */
    using raw_type = std::remove_reference_t<decltype(std::declval<pool_type>().get(0))>;
    /*! @brief Underlying entity identifier. */
    using entity_type = typename pool_type::entity_type;
    /*! @brief Unsigned integer type. */
    using size_type = typename pool_type::size_type;
    /*! @brief Input iterator type. */
    using iterator_type = typename view_type::iterator_type;

    /*! @brief Default copy constructor. */
    view(const view &) = default;
    /*! @brief Default move constructor. */
    view(view &&) = default;

    /*! @brief Default copy assignment operator. @return This view. */
    view & operator=(const view &) = default;
    /*! @brief Default move assignment operator. @return This view. */
    view & operator=(view &&) = default;

    /**
     * @brief Returns the number of entities that have the given component.
     * @return Number of entities that have the given component.
     */
    size_type size() const ENTT_NOEXCEPT {
        return pool->size();
    }

    /**
     * @brief Checks whether the view is empty.
     * @return True if the view is empty, false otherwise.
     */
    bool empty() const ENTT_NOEXCEPT {
        return pool->empty();
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
    raw_type * raw() const ENTT_NOEXCEPT {
        return pool->raw();
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
        return pool->data();
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
    iterator_type begin() const ENTT_NOEXCEPT {
        return pool->view_type::begin();
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
    iterator_type end() const ENTT_NOEXCEPT {
        return pool->view_type::end();
    }

    /**
     * @brief Finds an entity.
     * @param entity A valid entity identifier.
     * @return An iterator to the given entity if it's found, past the end
     * iterator otherwise.
     */
    iterator_type find(const entity_type entity) const ENTT_NOEXCEPT {
        return pool->view_type::find(entity);
    }

    /**
     * @brief Returns the identifier that occupies the given position.
     * @param pos Position of the element to return.
     * @return The identifier that occupies the given position.
     */
    entity_type operator[](const size_type pos) const ENTT_NOEXCEPT {
        return pool->view_type::begin()[pos];
    }

    /**
     * @brief Checks if a view contains an entity.
     * @param entity A valid entity identifier.
     * @return True if the view contains the given entity, false otherwise.
     */
    bool contains(const entity_type entity) const ENTT_NOEXCEPT {
        return pool->has(entity) && (pool->data()[pool->view_type::get(entity)] == entity);
    }

    /**
     * @brief Returns the component assigned to the given entity.
     *
     * Prefer this function instead of `registry::get` during iterations. It has
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
    raw_type & get(const entity_type entity) const ENTT_NOEXCEPT {
        assert(contains(entity));
        return pool->get(entity);
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
     * void(const entity_type, const Component &);
     * @endcode
     *
     * @tparam Func Type of the function object to invoke.
     * @param func A valid function object.
     */
    template<typename Func>
    void each(Func func) const {
        std::for_each(pool->view_type::begin(), pool->view_type::end(), [func = std::move(func), raw = pool->begin()](const auto entity) mutable {
            func(entity, *(raw++));
        });
    }

private:
    pool_type *pool;
};


/**
 * @brief Raw view.
 *
 * Raw views are meant to easily iterate components without having to resort to
 * using any other member function, so as to further increase the performance.
 * Whenever knowing the entity to which a component belongs isn't required, this
 * should be the preferred tool.<br/>
 * Order of elements during iterations are highly dependent on the order of the
 * underlying data structure. See sparse_set and its specializations for more
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
 * Views share a reference to the underlying data structure with the registry
 * that generated them. Therefore any change to the entities and to the
 * components made by means of the registry are immediately reflected by views.
 *
 * @warning
 * Lifetime of a view must overcome the one of the registry that generated it.
 * In any other case, attempting to use a view results in undefined behavior.
 *
 * @sa view
 * @sa view<Entity, Component>
 * @sa persistent_view
 * @sa runtime_view
 *
 * @tparam Entity A valid entity type (see entt_traits for more details).
 * @tparam Component Type of component iterated by the view.
 */
template<typename Entity, typename Component>
class raw_view final {
    /*! @brief A registry is allowed to create views. */
    friend class registry<Entity>;

    using pool_type = std::conditional_t<std::is_const_v<Component>, const sparse_set<Entity, std::remove_const_t<Component>>, sparse_set<Entity, Component>>;

    raw_view(pool_type *pool) ENTT_NOEXCEPT
        : pool{pool}
    {}

public:
    /*! @brief Type of component iterated by the view. */
    using raw_type = std::remove_reference_t<decltype(std::declval<pool_type>().get(0))>;
    /*! @brief Underlying entity identifier. */
    using entity_type = typename pool_type::entity_type;
    /*! @brief Unsigned integer type. */
    using size_type = typename pool_type::size_type;
    /*! @brief Input iterator type. */
    using iterator_type = decltype(std::declval<pool_type>().begin());

    /*! @brief Default copy constructor. */
    raw_view(const raw_view &) = default;
    /*! @brief Default move constructor. */
    raw_view(raw_view &&) = default;

    /*! @brief Default copy assignment operator. @return This view. */
    raw_view & operator=(const raw_view &) = default;
    /*! @brief Default move assignment operator. @return This view. */
    raw_view & operator=(raw_view &&) = default;

    /**
     * @brief Returns the number of instances of the given type.
     * @return Number of instances of the given component.
     */
    size_type size() const ENTT_NOEXCEPT {
        return pool->size();
    }

    /**
     * @brief Checks whether the view is empty.
     * @return True if the view is empty, false otherwise.
     */
    bool empty() const ENTT_NOEXCEPT {
        return pool->empty();
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
    raw_type * raw() const ENTT_NOEXCEPT {
        return pool->raw();
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
        return pool->data();
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
    iterator_type begin() const ENTT_NOEXCEPT {
        return pool->begin();
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
    iterator_type end() const ENTT_NOEXCEPT {
        return pool->end();
    }

    /**
     * @brief Returns a reference to the element at the given position.
     * @param pos Position of the element to return.
     * @return A reference to the requested element.
     */
    raw_type & operator[](const size_type pos) const ENTT_NOEXCEPT {
        return pool->begin()[pos];
    }

    /**
     * @brief Iterates components and applies the given function object to them.
     *
     * The function object is provided with a reference to each component of the
     * view.<br/>
     * The signature of the function should be equivalent to the following:
     *
     * @code{.cpp}
     * void(Component &);
     * @endcode
     *
     * @tparam Func Type of the function object to invoke.
     * @param func A valid function object.
     */
    template<typename Func>
    void each(Func func) const {
        std::for_each(pool->begin(), pool->end(), std::move(func));
    }

private:
    pool_type *pool;
};


/**
 * @brief Runtime view.
 *
 * Runtime views iterate over those entities that have at least all the given
 * components in their bags. During initialization, a runtime view looks at the
 * number of entities available for each component and picks up a reference to
 * the smallest set of candidate entities in order to get a performance boost
 * when iterate.<br/>
 * Order of elements during iterations are highly dependent on the order of the
 * underlying data structures. See sparse_set and its specializations for more
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
 * Views share references to the underlying data structures with the registry
 * that generated them. Therefore any change to the entities and to the
 * components made by means of the registry are immediately reflected by views,
 * unless a pool wasn't missing when the view was built (in this case, the view
 * won't have a valid reference and won't be updated accordingly).
 *
 * @warning
 * Lifetime of a view must overcome the one of the registry that generated it.
 * In any other case, attempting to use a view results in undefined behavior.
 *
 * @sa view
 * @sa view<Entity, Component>
 * @sa persistent_view
 * @sa raw_view
 *
 * @tparam Entity A valid entity type (see entt_traits for more details).
 */
template<typename Entity>
class runtime_view {
    /*! @brief A registry is allowed to create views. */
    friend class registry<Entity>;

    using view_type = sparse_set<Entity>;
    using underlying_iterator_type = typename view_type::iterator_type;
    using pattern_type = std::vector<const view_type *>;
    using extent_type = typename view_type::size_type;
    using traits_type = entt_traits<Entity>;

    class iterator {
        friend class runtime_view<Entity>;

        iterator(underlying_iterator_type begin, underlying_iterator_type end, const view_type * const *first, const view_type * const *last, extent_type extent) ENTT_NOEXCEPT
            : begin{begin},
              end{end},
              first{first},
              last{last},
              extent{extent}
        {
            if(begin != end && !valid()) {
                ++(*this);
            }
        }

        bool valid() const ENTT_NOEXCEPT {
            const auto entity = *begin;
            const auto sz = size_type(entity & traits_type::entity_mask);

            return sz < extent && std::all_of(first, last, [entity](const auto *view) {
                return view->fast(entity);
            });
        }

    public:
        using difference_type = typename underlying_iterator_type::difference_type;
        using value_type = typename underlying_iterator_type::value_type;
        using pointer = typename underlying_iterator_type::pointer;
        using reference = typename underlying_iterator_type::reference;
        using iterator_category = std::forward_iterator_tag;

        iterator() ENTT_NOEXCEPT = default;

        iterator(const iterator &) ENTT_NOEXCEPT = default;
        iterator & operator=(const iterator &) ENTT_NOEXCEPT = default;

        iterator & operator++() ENTT_NOEXCEPT {
            return (++begin != end && !valid()) ? ++(*this) : *this;
        }

        iterator operator++(int) ENTT_NOEXCEPT {
            iterator orig = *this;
            return ++(*this), orig;
        }

        bool operator==(const iterator &other) const ENTT_NOEXCEPT {
            return other.begin == begin;
        }

        inline bool operator!=(const iterator &other) const ENTT_NOEXCEPT {
            return !(*this == other);
        }

        pointer operator->() const ENTT_NOEXCEPT {
            return begin.operator->();
        }

        inline reference operator*() const ENTT_NOEXCEPT {
            return *operator->();
        }

    private:
        underlying_iterator_type begin;
        underlying_iterator_type end;
        const view_type * const *first;
        const view_type * const *last;
        extent_type extent;
    };

    runtime_view(pattern_type others) ENTT_NOEXCEPT
        : pools{std::move(others)}
    {
        const auto it = std::min_element(pools.begin(), pools.end(), [](const auto *lhs, const auto *rhs) {
            return (!lhs && rhs) || (lhs && rhs && lhs->size() < rhs->size());
        });

        // brings the best candidate (if any) on front of the vector
        std::rotate(pools.begin(), it, pools.end());
    }

    extent_type min() const ENTT_NOEXCEPT {
        extent_type extent{};

        if(valid()) {
            const auto it = std::min_element(pools.cbegin(), pools.cend(), [](const auto *lhs, const auto *rhs) {
                return lhs->extent() < rhs->extent();
            });

            extent = (*it)->extent();
        }

        return extent;
    }

    inline bool valid() const ENTT_NOEXCEPT {
        return !pools.empty() && pools.front();
    }

public:
    /*! @brief Underlying entity identifier. */
    using entity_type = typename view_type::entity_type;
    /*! @brief Unsigned integer type. */
    using size_type = typename view_type::size_type;
    /*! @brief Input iterator type. */
    using iterator_type = iterator;

    /*! @brief Default copy constructor. */
    runtime_view(const runtime_view &) = default;
    /*! @brief Default move constructor. */
    runtime_view(runtime_view &&) = default;

    /*! @brief Default copy assignment operator. @return This view. */
    runtime_view & operator=(const runtime_view &) = default;
    /*! @brief Default move assignment operator. @return This view. */
    runtime_view & operator=(runtime_view &&) = default;

    /**
     * @brief Estimates the number of entities that have the given components.
     * @return Estimated number of entities that have the given components.
     */
    size_type size() const ENTT_NOEXCEPT {
        return valid() ? pools.front()->size() : size_type{};
    }

    /**
     * @brief Checks if the view is definitely empty.
     * @return True if the view is definitely empty, false otherwise.
     */
    bool empty() const ENTT_NOEXCEPT {
        return !valid() || pools.front()->empty();
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
    iterator_type begin() const ENTT_NOEXCEPT {
        iterator_type it{};

        if(valid()) {
            const auto &pool = *pools.front();
            const auto * const *data = pools.data();
            it = { pool.begin(), pool.end(), data + 1, data + pools.size(), min() };
        }

        return it;
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
    iterator_type end() const ENTT_NOEXCEPT {
        iterator_type it{};

        if(valid()) {
            const auto &pool = *pools.front();
            it = { pool.end(), pool.end(), nullptr, nullptr, min() };
        }

        return it;
    }

    /**
     * @brief Checks if a view contains an entity.
     * @param entity A valid entity identifier.
     * @return True if the view contains the given entity, false otherwise.
     */
    bool contains(const entity_type entity) const ENTT_NOEXCEPT {
        return valid() && std::all_of(pools.cbegin(), pools.cend(), [entity](const auto *view) {
            return view->has(entity) && view->data()[view->get(entity)] == entity;
        });
    }

    /**
     * @brief Iterates entities and applies the given function object to them.
     *
     * The function object is invoked for each entity. It is provided only with
     * the entity itself. To get the components, users can use the registry with
     * which the view was built.<br/>
     * The signature of the function should be equivalent to the following:
     *
     * @code{.cpp}
     * void(const entity_type);
     * @endcode
     *
     * @tparam Func Type of the function object to invoke.
     * @param func A valid function object.
     */
    template<typename Func>
    void each(Func func) const {
        std::for_each(begin(), end(), func);
    }

private:
    pattern_type pools;
};


}


#endif // ENTT_ENTITY_VIEW_HPP

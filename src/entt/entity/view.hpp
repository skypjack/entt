#ifndef ENTT_ENTITY_VIEW_HPP
#define ENTT_ENTITY_VIEW_HPP


#include <iterator>
#include <array>
#include <tuple>
#include <utility>
#include <algorithm>
#include <type_traits>
#include "../config/config.h"
#include "../core/type_traits.hpp"
#include "entity.hpp"
#include "fwd.hpp"
#include "pool.hpp"
#include "sparse_set.hpp"
#include "utility.hpp"


namespace entt {


/**
 * @brief View.
 *
 * Primary template isn't defined on purpose. All the specializations give a
 * compile-time error, but for a few reasonable cases.
 */
template<typename...>
class basic_view;


/**
 * @brief Multi component view.
 *
 * Multi component views iterate over those entities that have at least all the
 * given components in their bags. During initialization, a multi component view
 * looks at the number of entities available for each component and uses the
 * smallest set in order to get a performance boost when iterate.
 *
 * @b Important
 *
 * Iterators aren't invalidated if:
 *
 * * New instances of the given components are created and assigned to entities.
 * * The entity currently pointed is modified (as an example, if one of the
 *   given components is removed from the entity to which the iterator points).
 * * The entity currently pointed is destroyed.
 *
 * In all other cases, modifying the pools iterated by the view in any way
 * invalidates all the iterators and using them results in undefined behavior.
 *
 * @note
 * Views share references to the underlying data structures of the registry that
 * generated them. Therefore any change to the entities and to the components
 * made by means of the registry are immediately reflected by views.
 *
 * @warning
 * Lifetime of a view must not overcome that of the registry that generated it.
 * In any other case, attempting to use a view results in undefined behavior.
 *
 * @tparam Entity A valid entity type (see entt_traits for more details).
 * @tparam Exclude Types of components used to filter the view.
 * @tparam Component Types of components iterated by the view.
 */
template<typename Entity, typename... Exclude, typename... Component>
class basic_view<Entity, exclude_t<Exclude...>, Component...> {
    /*! @brief A registry is allowed to create views. */
    friend class basic_registry<Entity>;

    template<typename Comp>
    using pool_type = pool_t<Entity, Comp>;

    template<typename Comp>
    using component_iterator = decltype(std::declval<pool_type<Comp>>().begin());

    using underlying_iterator = typename sparse_set<Entity>::iterator;
    using unchecked_type = std::array<const sparse_set<Entity> *, (sizeof...(Component) - 1)>;
    using filter_type = std::array<const sparse_set<Entity> *, sizeof...(Exclude)>;

    class view_iterator final {
        friend class basic_view<Entity, exclude_t<Exclude...>, Component...>;

        view_iterator(const sparse_set<Entity> &candidate, unchecked_type other, filter_type ignore, underlying_iterator curr) ENTT_NOEXCEPT
            : view{&candidate},
              unchecked{other},
              filter{ignore},
              it{curr}
        {
            if(it != view->end() && !valid()) {
                ++(*this);
            }
        }

        [[nodiscard]] bool valid() const {
            return std::all_of(unchecked.cbegin(), unchecked.cend(), [entt = *it](const sparse_set<Entity> *curr) { return curr->contains(entt); })
                    && std::none_of(filter.cbegin(), filter.cend(), [entt = *it](const sparse_set<Entity> *curr) { return curr->contains(entt); });
        }

    public:
        using difference_type = typename underlying_iterator::difference_type;
        using value_type = typename underlying_iterator::value_type;
        using pointer = typename underlying_iterator::pointer;
        using reference = typename underlying_iterator::reference;
        using iterator_category = std::bidirectional_iterator_tag;

        view_iterator() ENTT_NOEXCEPT = default;

        view_iterator & operator++() {
            while(++it != view->end() && !valid());
            return *this;
        }

        view_iterator operator++(int) {
            view_iterator orig = *this;
            return ++(*this), orig;
        }

        view_iterator & operator--() ENTT_NOEXCEPT {
            while(--it != view->begin() && !valid());
            return *this;
        }

        view_iterator operator--(int) ENTT_NOEXCEPT {
            view_iterator orig = *this;
            return operator--(), orig;
        }

        [[nodiscard]] bool operator==(const view_iterator &other) const ENTT_NOEXCEPT {
            return other.it == it;
        }

        [[nodiscard]] bool operator!=(const view_iterator &other) const ENTT_NOEXCEPT {
            return !(*this == other);
        }

        [[nodiscard]] pointer operator->() const {
            return it.operator->();
        }

        [[nodiscard]] reference operator*() const {
            return *operator->();
        }

    private:
        const sparse_set<Entity> *view;
        unchecked_type unchecked;
        filter_type filter;
        underlying_iterator it;
    };

    class view_range {
        friend class basic_view<Entity, exclude_t<Exclude...>, Component...>;

        class range_iterator {
            friend class view_range;

            using ref_type = decltype(std::tuple_cat(std::declval<std::conditional_t<ENTT_IS_EMPTY(Component), std::tuple<>, std::tuple<pool_type<Component> *>>>()...));

            range_iterator(view_iterator from, ref_type ref) ENTT_NOEXCEPT
                : it{from},
                  pools{ref}
            {}

        public:
            using difference_type = std::ptrdiff_t;
            using value_type = decltype(std::tuple_cat(
                std::declval<std::tuple<Entity>>(),
                std::declval<std::conditional_t<ENTT_IS_EMPTY(Component), std::tuple<>, std::tuple<Component>>>()...
            ));
            using pointer = void;
            using reference = decltype(std::tuple_cat(
                std::declval<std::tuple<Entity>>(),
                std::declval<std::conditional_t<ENTT_IS_EMPTY(Component), std::tuple<>, std::tuple<Component &>>>()...
            ));
            using iterator_category = std::input_iterator_tag;

            range_iterator & operator++() ENTT_NOEXCEPT {
                return ++it, *this;
            }

            range_iterator operator++(int) ENTT_NOEXCEPT {
                range_iterator orig = *this;
                return ++(*this), orig;
            }

            [[nodiscard]] reference operator*() const ENTT_NOEXCEPT {
                return std::apply([entt = *it](auto *... cpool) { return reference{entt, cpool->get(entt)...}; }, pools);
            }

            [[nodiscard]] bool operator==(const range_iterator &other) const ENTT_NOEXCEPT {
                return other.it == it;
            }

            [[nodiscard]] bool operator!=(const range_iterator &other) const ENTT_NOEXCEPT {
                return !(*this == other);
            }

        private:
            view_iterator it{};
            const ref_type pools{};
        };

        view_range(view_iterator from, view_iterator to, std::tuple<pool_type<Component> *...> ref)
            : first{from},
              last{to},
              pools{ref}
        {}

    public:
        [[nodiscard]] auto begin() const ENTT_NOEXCEPT {
            return range_iterator{first, std::tuple_cat([](auto *cpool) {
                if constexpr(ENTT_IS_EMPTY(typename std::decay_t<decltype(*cpool)>::object_type)) {
                    return std::make_tuple();
                } else {
                    return std::make_tuple(cpool);
                }
            }(std::get<pool_type<Component> *>(pools))...)};
        }

        [[nodiscard]] auto end() const ENTT_NOEXCEPT {
            return range_iterator{last, std::tuple_cat([](auto *cpool) {
                if constexpr(ENTT_IS_EMPTY(typename std::decay_t<decltype(*cpool)>::object_type)) {
                    return std::make_tuple();
                } else {
                    return std::make_tuple(cpool);
                }
            }(std::get<pool_type<Component> *>(pools))...)};
        }

    private:
        view_iterator first;
        view_iterator last;
        const std::tuple<pool_type<Component> *...> pools;
    };

    basic_view(pool_type<Component> &... component, unpack_as_t<const sparse_set<Entity>, Exclude> &... epool) ENTT_NOEXCEPT
        : pools{&component...},
          filter{&epool...}
    {}

    [[nodiscard]] const sparse_set<Entity> & candidate() const ENTT_NOEXCEPT {
        return *std::min({ static_cast<const sparse_set<Entity> *>(std::get<pool_type<Component> *>(pools))... }, [](const auto *lhs, const auto *rhs) {
            return lhs->size() < rhs->size();
        });
    }

    [[nodiscard]] unchecked_type unchecked(const sparse_set<Entity> &view) const {
        std::size_t pos{};
        unchecked_type other{};
        ((std::get<pool_type<Component> *>(pools) == &view ? nullptr : (other[pos++] = std::get<pool_type<Component> *>(pools))), ...);
        return other;
    }

    template<typename Comp, typename Other>
    [[nodiscard]] decltype(auto) get([[maybe_unused]] component_iterator<Comp> it, [[maybe_unused]] pool_type<Other> *cpool, [[maybe_unused]] const Entity entt) const {
        if constexpr(std::is_same_v<Comp, Other>) {
            return *it;
        } else {
            return cpool->get(entt);
        }
    }

    template<typename Comp, typename Func, typename... Type>
    void traverse(Func func, type_list<Type...>) const {
        if constexpr(std::disjunction_v<std::is_same<Comp, Type>...>) {
            auto it = std::get<pool_type<Comp> *>(pools)->begin();

            for(const auto entt: static_cast<const sparse_set<entity_type> &>(*std::get<pool_type<Comp> *>(pools))) {
                auto curr = it++;

                if(((std::is_same_v<Comp, Component> || std::get<pool_type<Component> *>(pools)->contains(entt)) && ...)
                         && std::none_of(filter.cbegin(), filter.cend(), [entt](const sparse_set<Entity> *curr) { return curr->contains(entt); }))
                {
                    if constexpr(std::is_invocable_v<Func, decltype(get<Type>({}))...>) {
                        func(get<Comp, Type>(curr, std::get<pool_type<Type> *>(pools), entt)...);
                    } else {
                        func(entt, get<Comp, Type>(curr, std::get<pool_type<Type> *>(pools), entt)...);
                    }
                }
            }
        } else {
            for(const auto entt: static_cast<const sparse_set<entity_type> &>(*std::get<pool_type<Comp> *>(pools))) {
                if(((std::is_same_v<Comp, Component> || std::get<pool_type<Component> *>(pools)->contains(entt)) && ...)
                        && std::none_of(filter.cbegin(), filter.cend(), [entt](const sparse_set<Entity> *curr) { return curr->contains(entt); }))
                {
                    if constexpr(std::is_invocable_v<Func, decltype(get<Type>({}))...>) {
                        func(std::get<pool_type<Type> *>(pools)->get(entt)...);
                    } else {
                        func(entt, std::get<pool_type<Type> *>(pools)->get(entt)...);
                    }
                }
            }
        }
    }

public:
    /*! @brief Underlying entity identifier. */
    using entity_type = Entity;
    /*! @brief Unsigned integer type. */
    using size_type = std::size_t;
    /*! @brief Bidirectional iterator type. */
    using iterator = view_iterator;

    /**
     * @brief Returns the number of existing components of the given type.
     *
     * This isn't the number of entities iterated by the view.
     *
     * @tparam Comp Type of component of which to return the size.
     * @return Number of existing components of the given type.
     */
    template<typename Comp>
    [[nodiscard]] size_type size() const ENTT_NOEXCEPT {
        return std::get<pool_type<Comp> *>(pools)->size();
    }

    /**
     * @brief Estimates the number of entities iterated by the view.
     * @return Estimated number of entities iterated by the view.
     */
    [[nodiscard]] size_type size() const ENTT_NOEXCEPT {
        return std::min({ std::get<pool_type<Component> *>(pools)->size()... });
    }

    /**
     * @brief Checks whether a view or some pools are empty.
     *
     * The view is definitely empty if one of the pools it uses is empty. In all
     * other cases, the view may be empty and not return entities even if this
     * function returns false.
     *
     * @tparam Comp Types of components in which one is interested.
     * @return True if the view or the pools are empty, false otherwise.
     */
    template<typename... Comp>
    [[nodiscard]] bool empty() const ENTT_NOEXCEPT {
        if constexpr(sizeof...(Comp) == 0) {
            return (std::get<pool_type<Component> *>(pools)->empty() || ...);
        } else {
            return (std::get<pool_type<Comp> *>(pools)->empty() && ...);
        }
    }

    /**
     * @brief Direct access to the list of components of a given pool.
     *
     * The returned pointer is such that range
     * `[raw<Comp>(), raw<Comp>() + size<Comp>()]` is always a valid range, even
     * if the container is empty.
     *
     * @note
     * Components are in the reverse order as returned by the `begin`/`end`
     * iterators.
     *
     * @tparam Comp Type of component in which one is interested.
     * @return A pointer to the array of components.
     */
    template<typename Comp>
    [[nodiscard]] Comp * raw() const ENTT_NOEXCEPT {
        return std::get<pool_type<Comp> *>(pools)->raw();
    }

    /**
     * @brief Direct access to the list of entities of a given pool.
     *
     * The returned pointer is such that range
     * `[data<Comp>(), data<Comp>() + size<Comp>()]` is always a valid range,
     * even if the container is empty.
     *
     * @note
     * Entities are in the reverse order as returned by the `begin`/`end`
     * iterators.
     *
     * @tparam Comp Type of component in which one is interested.
     * @return A pointer to the array of entities.
     */
    template<typename Comp>
    [[nodiscard]] const entity_type * data() const ENTT_NOEXCEPT {
        return std::get<pool_type<Comp> *>(pools)->data();
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
     * Iterators stay true to the order imposed to the underlying data
     * structures.
     *
     * @return An iterator to the first entity that has the given components.
     */
    [[nodiscard]] iterator begin() const {
        const auto &view = candidate();
        return iterator{view, unchecked(view), filter, view.begin()};
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
     * Iterators stay true to the order imposed to the underlying data
     * structures.
     *
     * @return An iterator to the entity following the last entity that has the
     * given components.
     */
    [[nodiscard]] iterator end() const {
        const auto &view = candidate();
        return iterator{view, unchecked(view), filter, view.end()};
    }

    /**
     * @brief Returns the first entity that has the given components, if any.
     * @return The first entity that has the given components if one exists, the
     * null entity otherwise.
     */
    [[nodiscard]] entity_type front() const {
        const auto it = begin();
        return it != end() ? *it : null;
    }

    /**
     * @brief Returns the last entity that has the given components, if any.
     * @return The last entity that has the given components if one exists, the
     * null entity otherwise.
     */
    [[nodiscard]] entity_type back() const {
        const auto it = std::make_reverse_iterator(end());
        return it != std::make_reverse_iterator(begin()) ? *it : null;
    }

    /**
     * @brief Finds an entity.
     * @param entt A valid entity identifier.
     * @return An iterator to the given entity if it's found, past the end
     * iterator otherwise.
     */
    [[nodiscard]] iterator find(const entity_type entt) const {
        const auto &view = candidate();
        iterator it{view, unchecked(view), filter, view.find(entt)};
        return (it != end() && *it == entt) ? it : end();
    }

    /**
     * @brief Checks if a view contains an entity.
     * @param entt A valid entity identifier.
     * @return True if the view contains the given entity, false otherwise.
     */
    [[nodiscard]] bool contains(const entity_type entt) const {
        return (std::get<pool_type<Component> *>(pools)->contains(entt) && ...)
                && std::none_of(filter.begin(), filter.end(), [entt](const auto *cpool) { return cpool->contains(entt); });
    }

    /**
     * @brief Returns the components assigned to the given entity.
     *
     * Prefer this function instead of `registry::get` during iterations. It has
     * far better performance than its counterpart.
     *
     * @warning
     * Attempting to use an invalid component type results in a compilation
     * error. Attempting to use an entity that doesn't belong to the view
     * results in undefined behavior.<br/>
     * An assertion will abort the execution at runtime in debug mode if the
     * view doesn't contain the given entity.
     *
     * @tparam Comp Types of components to get.
     * @param entt A valid entity identifier.
     * @return The components assigned to the entity.
     */
    template<typename... Comp>
    [[nodiscard]] decltype(auto) get([[maybe_unused]] const entity_type entt) const {
        ENTT_ASSERT(contains(entt));

        if constexpr(sizeof...(Comp) == 0) {
            static_assert(sizeof...(Component) == 1, "Invalid component type");
            return (std::get<pool_type<Component> *>(pools)->get(entt), ...);
        } else if constexpr(sizeof...(Comp) == 1) {
            return (std::get<pool_type<Comp> *>(pools)->get(entt), ...);
        } else {
            return std::tuple<decltype(get<Comp>({}))...>{get<Comp>(entt)...};
        }
    }

    /**
     * @brief Iterates entities and components and applies the given function
     * object to them.
     *
     * The function object is invoked for each entity. It is provided with the
     * entity itself and a set of references to non-empty components. The
     * _constness_ of the components is as requested.<br/>
     * The signature of the function must be equivalent to one of the following
     * forms:
     *
     * @code{.cpp}
     * void(const entity_type, Type &...);
     * void(Type &...);
     * @endcode
     *
     * @note
     * Empty types aren't explicitly instantiated and therefore they are never
     * returned during iterations.
     *
     * @tparam Func Type of the function object to invoke.
     * @param func A valid function object.
     */
    template<typename Func>
    void each(Func func) const {
        const auto &view = candidate();
        ((std::get<pool_type<Component> *>(pools) == &view ? each<Component>(std::move(func)) : void()), ...);
    }

    /**
     * @brief Iterates entities and components and applies the given function
     * object to them.
     *
     * The pool of the suggested component is used to lead the iterations. The
     * returned entities will therefore respect the order of the pool associated
     * with that type.<br/>
     * It is no longer guaranteed that the performance is the best possible, but
     * there will be greater control over the order of iteration.
     *
     * @sa each
     *
     * @tparam Comp Type of component to use to enforce the iteration order.
     * @tparam Func Type of the function object to invoke.
     * @param func A valid function object.
     */
    template<typename Comp, typename Func>
    void each(Func func) const {
        using non_empty_type = type_list_cat_t<std::conditional_t<ENTT_IS_EMPTY(Component), type_list<>, type_list<Component>>...>;
        traverse<Comp>(std::move(func), non_empty_type{});
    }

    /**
     * @brief Returns an iterable object to use to _visit_ the view.
     *
     * The iterable object returns tuples that contain the current entity and a
     * set of references to its non-empty components. The _constness_ of the
     * components is as requested.
     *
     * @note
     * Empty types aren't explicitly instantiated and therefore they are never
     * returned during iterations.
     *
     * @return An iterable object to use to _visit_ the view.
     */
    [[nodiscard]] auto each() const ENTT_NOEXCEPT {
        return view_range{begin(), end(), pools};
    }

    /**
     * @brief Returns an iterable object to use to _visit_ the view.
     *
     * The pool of the suggested component is used to lead the iterations. The
     * returned elements will therefore respect the order of the pool associated
     * with that type.<br/>
     * It is no longer guaranteed that the performance is the best possible, but
     * there will be greater control over the order of iteration.
     *
     * @sa each
     *
     * @tparam Comp Type of component to use to enforce the iteration order.
     * @return An iterable object to use to _visit_ the view.
     */
    template<typename Comp>
    [[nodiscard]] auto each() const ENTT_NOEXCEPT {
        const sparse_set<entity_type> &view = *std::get<pool_type<Comp> *>(pools);
        return view_range{iterator{view, unchecked(view), filter, view.begin()}, iterator{view, unchecked(view), filter, view.end()}, pools};
    }

private:
    const std::tuple<pool_type<Component> *...> pools;
    filter_type filter;
};


/**
 * @brief Single component view specialization.
 *
 * Single component views are specialized in order to get a boost in terms of
 * performance. This kind of views can access the underlying data structure
 * directly and avoid superfluous checks.
 *
 * @b Important
 *
 * Iterators aren't invalidated if:
 *
 * * New instances of the given component are created and assigned to entities.
 * * The entity currently pointed is modified (as an example, the given
 *   component is removed from the entity to which the iterator points).
 * * The entity currently pointed is destroyed.
 *
 * In all other cases, modifying the pool iterated by the view in any way
 * invalidates all the iterators and using them results in undefined behavior.
 *
 * @note
 * Views share a reference to the underlying data structure of the registry that
 * generated them. Therefore any change to the entities and to the components
 * made by means of the registry are immediately reflected by views.
 *
 * @warning
 * Lifetime of a view must not overcome that of the registry that generated it.
 * In any other case, attempting to use a view results in undefined behavior.
 *
 * @tparam Entity A valid entity type (see entt_traits for more details).
 * @tparam Component Type of component iterated by the view.
 */
template<typename Entity, typename Component>
class basic_view<Entity, exclude_t<>, Component> {
    /*! @brief A registry is allowed to create views. */
    friend class basic_registry<Entity>;

    using pool_type = pool_t<Entity, Component>;

    class view_range {
        friend class basic_view<Entity, exclude_t<>, Component>;

        class range_iterator {
            friend class view_range;

            using it_type = std::conditional_t<
                ENTT_IS_EMPTY(Component),
                std::tuple<typename sparse_set<Entity>::iterator>,
                std::tuple<typename sparse_set<Entity>::iterator, decltype(std::declval<pool_type>().begin())>
            >;

            range_iterator(it_type from) ENTT_NOEXCEPT
                : it{from}
            {}

        public:
            using difference_type = std::ptrdiff_t;
            using value_type = std::conditional_t<ENTT_IS_EMPTY(Component), std::tuple<Entity>, std::tuple<Entity, Component>>;
            using pointer = void;
            using reference = std::conditional_t<ENTT_IS_EMPTY(Component), std::tuple<Entity>, std::tuple<Entity, Component &>>;
            using iterator_category = std::input_iterator_tag;

            range_iterator & operator++() ENTT_NOEXCEPT {
                return std::apply([](auto &&... curr) { (++curr, ...); }, it), *this;
            }

            range_iterator operator++(int) ENTT_NOEXCEPT {
                range_iterator orig = *this;
                return ++(*this), orig;
            }

            [[nodiscard]] reference operator*() const ENTT_NOEXCEPT {
                return std::apply([](auto &&... curr) { return reference{*curr...}; }, it);
            }

            [[nodiscard]] bool operator==(const range_iterator &other) const ENTT_NOEXCEPT {
                return std::get<0>(other.it) == std::get<0>(it);
            }

            [[nodiscard]] bool operator!=(const range_iterator &other) const ENTT_NOEXCEPT {
                return !(*this == other);
            }

        private:
            it_type it{};
        };

        view_range(pool_type &ref)
            : pool{&ref}
        {}

    public:
        [[nodiscard]] auto begin() const ENTT_NOEXCEPT {
            if constexpr(ENTT_IS_EMPTY(Component)) {
                return range_iterator{std::make_tuple(pool->sparse_set<entity_type>::begin())};
            } else {
                return range_iterator{std::make_tuple(pool->sparse_set<entity_type>::begin(), pool->begin())};
            }
        }

        [[nodiscard]] auto end() const ENTT_NOEXCEPT {
            if constexpr(ENTT_IS_EMPTY(Component)) {
                return range_iterator{std::make_tuple(pool->sparse_set<entity_type>::end())};
            } else {
                return range_iterator{std::make_tuple(pool->sparse_set<entity_type>::end(), pool->end())};
            }
        }

    private:
        pool_type *pool;
    };

    basic_view(pool_type &ref) ENTT_NOEXCEPT
        : pool{&ref}
    {}

public:
    /*! @brief Type of component iterated by the view. */
    using raw_type = Component;
    /*! @brief Underlying entity identifier. */
    using entity_type = Entity;
    /*! @brief Unsigned integer type. */
    using size_type = std::size_t;
    /*! @brief Random access iterator type. */
    using iterator = typename sparse_set<Entity>::iterator;

    /**
     * @brief Returns the number of entities that have the given component.
     * @return Number of entities that have the given component.
     */
    [[nodiscard]] size_type size() const ENTT_NOEXCEPT {
        return pool->size();
    }

    /**
     * @brief Checks whether a view is empty.
     * @return True if the view is empty, false otherwise.
     */
    [[nodiscard]] bool empty() const ENTT_NOEXCEPT {
        return pool->empty();
    }

    /**
     * @brief Direct access to the list of components.
     *
     * The returned pointer is such that range `[raw(), raw() + size()]` is
     * always a valid range, even if the container is empty.
     *
     * @note
     * Components are in the reverse order as returned by the `begin`/`end`
     * iterators.
     *
     * @return A pointer to the array of components.
     */
    [[nodiscard]] raw_type * raw() const ENTT_NOEXCEPT {
        return pool->raw();
    }

    /**
     * @brief Direct access to the list of entities.
     *
     * The returned pointer is such that range `[data(), data() + size()]` is
     * always a valid range, even if the container is empty.
     *
     * @note
     * Entities are in the reverse order as returned by the `begin`/`end`
     * iterators.
     *
     * @return A pointer to the array of entities.
     */
    [[nodiscard]] const entity_type * data() const ENTT_NOEXCEPT {
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
     * Iterators stay true to the order imposed to the underlying data
     * structures.
     *
     * @return An iterator to the first entity that has the given component.
     */
    [[nodiscard]] iterator begin() const ENTT_NOEXCEPT {
        return pool->sparse_set<Entity>::begin();
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
     * Iterators stay true to the order imposed to the underlying data
     * structures.
     *
     * @return An iterator to the entity following the last entity that has the
     * given component.
     */
    [[nodiscard]] iterator end() const ENTT_NOEXCEPT {
        return pool->sparse_set<Entity>::end();
    }

    /**
     * @brief Returns the first entity that has the given component, if any.
     * @return The first entity that has the given component if one exists, the
     * null entity otherwise.
     */
    [[nodiscard]] entity_type front() const {
        const auto it = begin();
        return it != end() ? *it : null;
    }

    /**
     * @brief Returns the last entity that has the given component, if any.
     * @return The last entity that has the given component if one exists, the
     * null entity otherwise.
     */
    [[nodiscard]] entity_type back() const {
        const auto it = std::make_reverse_iterator(end());
        return it != std::make_reverse_iterator(begin()) ? *it : null;
    }

    /**
     * @brief Finds an entity.
     * @param entt A valid entity identifier.
     * @return An iterator to the given entity if it's found, past the end
     * iterator otherwise.
     */
    [[nodiscard]] iterator find(const entity_type entt) const {
        const auto it = pool->find(entt);
        return it != end() && *it == entt ? it : end();
    }

    /**
     * @brief Returns the identifier that occupies the given position.
     * @param pos Position of the element to return.
     * @return The identifier that occupies the given position.
     */
    [[nodiscard]] entity_type operator[](const size_type pos) const {
        return begin()[pos];
    }

    /**
     * @brief Checks if a view contains an entity.
     * @param entt A valid entity identifier.
     * @return True if the view contains the given entity, false otherwise.
     */
    [[nodiscard]] bool contains(const entity_type entt) const {
        return pool->contains(entt);
    }

    /**
     * @brief Returns the component assigned to the given entity.
     *
     * Prefer this function instead of `registry::get` during iterations. It has
     * far better performance than its counterpart.
     *
     * @warning
     * Attempting to use an entity that doesn't belong to the view results in
     * undefined behavior.<br/>
     * An assertion will abort the execution at runtime in debug mode if the
     * view doesn't contain the given entity.
     *
     * @param entt A valid entity identifier.
     * @return The component assigned to the entity.
     */
    template<typename Comp = Component>
    [[nodiscard]] decltype(auto) get(const entity_type entt) const {
        static_assert(std::is_same_v<Comp, Component>, "Invalid component type");
        ENTT_ASSERT(contains(entt));
        return pool->get(entt);
    }

    /**
     * @brief Iterates entities and components and applies the given function
     * object to them.
     *
     * The function object is invoked for each entity. It is provided with the
     * entity itself and a reference to the component if it's a non-empty one.
     * The _constness_ of the component is as requested.<br/>
     * The signature of the function must be equivalent to one of the following
     * forms:
     *
     * @code{.cpp}
     * void(const entity_type, Component &);
     * void(Component &);
     * @endcode
     *
     * @note
     * Empty types aren't explicitly instantiated and therefore they are never
     * returned during iterations.
     *
     * @tparam Func Type of the function object to invoke.
     * @param func A valid function object.
     */
    template<typename Func>
    void each(Func func) const {
        if constexpr(ENTT_IS_EMPTY(Component)) {
            if constexpr(std::is_invocable_v<Func>) {
                for(auto pos = pool->size(); pos; --pos) {
                    func();
                }
            } else {
                for(const auto entt: *this) {
                    func(entt);
                }
            }
        } else {
            if constexpr(std::is_invocable_v<Func, decltype(get({}))>) {
                for(auto &&component: *pool) {
                    func(component);
                }
            } else {
                auto raw = pool->begin();

                for(const auto entt: *this) {
                    func(entt, *(raw++));
                }
            }
        }
    }

    /**
     * @brief Returns an iterable object to use to _visit_ the view.
     *
     * The iterable object returns tuples that contain the current entity and a
     * reference to its component if it's a non-empty one. The _constness_ of
     * the component is as requested.
     *
     * @note
     * Empty types aren't explicitly instantiated and therefore they are never
     * returned during iterations.
     *
     * @return An iterable object to use to _visit_ the view.
     */
    [[nodiscard]] auto each() const ENTT_NOEXCEPT {
        return view_range{*pool};
    }

private:
    pool_type *pool;
};


}


#endif

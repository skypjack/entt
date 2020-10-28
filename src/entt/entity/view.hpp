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
class basic_view<Entity, exclude_t<Exclude...>, Component...> final {
    /*! @brief A registry is allowed to create views. */
    friend class basic_registry<Entity>;

    template<typename Comp>
    using pool_type = pool_t<Entity, Comp>;

    using return_type = type_list_cat_t<std::conditional_t<is_empty_v<Component>, type_list<>, type_list<Component>>...>;
    using unchecked_type = std::array<const basic_sparse_set<Entity> *, (sizeof...(Component) - 1)>;

    template<typename It>
    class view_iterator final {
        friend class basic_view<Entity, exclude_t<Exclude...>, Component...>;

        view_iterator(It from, It to, It curr, unchecked_type other, const std::tuple<const pool_type<Exclude> *...> &ignore) ENTT_NOEXCEPT
            : first{from},
              last{to},
              it{curr},
              unchecked{other},
              filter{ignore}
        {
            if(it != last && !valid()) {
                ++(*this);
            }
        }

        [[nodiscard]] bool valid() const {
            const auto entt = *it;

            return std::all_of(unchecked.cbegin(), unchecked.cend(), [entt](const basic_sparse_set<Entity> *curr) { return curr->contains(entt); })
                && !(std::get<const pool_type<Exclude> *>(filter)->contains(entt) || ...);
        }

    public:
        using difference_type = typename std::iterator_traits<It>::difference_type;
        using value_type = typename std::iterator_traits<It>::value_type;
        using pointer = typename std::iterator_traits<It>::pointer;
        using reference = typename std::iterator_traits<It>::reference;
        using iterator_category = std::bidirectional_iterator_tag;

        view_iterator() ENTT_NOEXCEPT = default;

        view_iterator & operator++() ENTT_NOEXCEPT {
            while(++it != last && !valid());
            return *this;
        }

        view_iterator operator++(int) ENTT_NOEXCEPT {
            view_iterator orig = *this;
            return ++(*this), orig;
        }

        view_iterator & operator--() ENTT_NOEXCEPT {
            while(--it != first && !valid());
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
            return &*it;
        }

        [[nodiscard]] reference operator*() const {
            return *operator->();
        }

    private:
        It first;
        It last;
        It it;
        unchecked_type unchecked;
        std::tuple<const pool_type<Exclude> *...> filter;
    };

    class iterable_view {
        friend class basic_view<Entity, exclude_t<Exclude...>, Component...>;

        template<typename It>
        class iterable_view_iterator {
            friend class iterable_view;

            iterable_view_iterator(It from, const basic_view &parent) ENTT_NOEXCEPT
                : it{from},
                  view{parent}
            {}

        public:
            using difference_type = std::ptrdiff_t;
            using value_type = decltype(std::tuple_cat(std::tuple<Entity>{}, std::declval<basic_view>().get({})));
            using pointer = void;
            using reference = value_type;
            using iterator_category = std::input_iterator_tag;

            iterable_view_iterator & operator++() ENTT_NOEXCEPT {
                return ++it, *this;
            }

            iterable_view_iterator operator++(int) ENTT_NOEXCEPT {
                iterable_view_iterator orig = *this;
                return ++(*this), orig;
            }

            [[nodiscard]] reference operator*() const ENTT_NOEXCEPT {
                return std::tuple_cat(std::make_tuple(*it), view.get(*it));
            }

            [[nodiscard]] bool operator==(const iterable_view_iterator &other) const ENTT_NOEXCEPT {
                return other.it == it;
            }

            [[nodiscard]] bool operator!=(const iterable_view_iterator &other) const ENTT_NOEXCEPT {
                return !(*this == other);
            }

        private:
            It it;
            const basic_view view;
        };

        iterable_view(const basic_view &parent)
            : view{parent}
        {}

    public:
        using iterator = iterable_view_iterator<view_iterator<typename basic_sparse_set<Entity>::iterator>>;
        using reverse_iterator = iterable_view_iterator<view_iterator<typename basic_sparse_set<Entity>::reverse_iterator>>;

        [[nodiscard]] iterator begin() const ENTT_NOEXCEPT {
            return { view.begin(), view };
        }

        [[nodiscard]] iterator end() const ENTT_NOEXCEPT {
            return { view.end(), view };
        }

        [[nodiscard]] reverse_iterator rbegin() const ENTT_NOEXCEPT {
            return { view.rbegin(), view };
        }

        [[nodiscard]] reverse_iterator rend() const ENTT_NOEXCEPT {
            return { view.rend(), view };
        }

    private:
        const basic_view view;
    };

    basic_view(pool_type<Component> &... component, const pool_type<Exclude> &... epool) ENTT_NOEXCEPT
        : pools{&component...},
          filter{&epool...},
          view{candidate()}
    {}

    [[nodiscard]] const basic_sparse_set<Entity> * candidate() const ENTT_NOEXCEPT {
        return (std::min)({ static_cast<const basic_sparse_set<Entity> *>(std::get<pool_type<Component> *>(pools))... }, [](const auto *lhs, const auto *rhs) {
            return lhs->size() < rhs->size();
        });
    }

    [[nodiscard]] unchecked_type unchecked(const basic_sparse_set<Entity> *cpool) const {
        std::size_t pos{};
        unchecked_type other{};
        ((std::get<pool_type<Component> *>(pools) == cpool ? nullptr : (other[pos] = std::get<pool_type<Component> *>(pools), other[pos++])), ...);
        return other;
    }

    template<typename It, typename Pool>
    [[nodiscard]] decltype(auto) get([[maybe_unused]] It &it, [[maybe_unused]] Pool *cpool, [[maybe_unused]] const Entity entt) const {
        if constexpr(std::is_same_v<typename std::iterator_traits<It>::value_type, typename Pool::value_type>) {
            return *it;
        } else {
            return cpool->get(entt);
        }
    }

    template<typename Comp, typename Func, typename... Type>
    void traverse(Func func, type_list<Type...>) const {
        if constexpr(std::disjunction_v<std::is_same<Comp, Type>...>) {
            auto it = std::get<pool_type<Comp> *>(pools)->begin();

            for(const auto entt: static_cast<const basic_sparse_set<entity_type> &>(*std::get<pool_type<Comp> *>(pools))) {
                if(((std::is_same_v<Comp, Component> || std::get<pool_type<Component> *>(pools)->contains(entt)) && ...)
                    && !(std::get<const pool_type<Exclude> *>(filter)->contains(entt) || ...))
                {
                    if constexpr(std::is_invocable_v<Func, entity_type, decltype(get<Type>({}))...>) {
                        func(entt, get(it, std::get<pool_type<Type> *>(pools), entt)...);
                    } else {
                        func(get(it, std::get<pool_type<Type> *>(pools), entt)...);
                    }
                }

                ++it;
            }
        } else {
            for(const auto entt: static_cast<const basic_sparse_set<entity_type> &>(*std::get<pool_type<Comp> *>(pools))) {
                if(((std::is_same_v<Comp, Component> || std::get<pool_type<Component> *>(pools)->contains(entt)) && ...)
                    && !(std::get<const pool_type<Exclude> *>(filter)->contains(entt) || ...))
                {
                    if constexpr(std::is_invocable_v<Func, entity_type, decltype(get<Type>({}))...>) {
                        func(entt, std::get<pool_type<Type> *>(pools)->get(entt)...);
                    } else {
                        func(std::get<pool_type<Type> *>(pools)->get(entt)...);
                    }
                }
            }
        }
    }

    template<typename Func, typename... Type>
    void iterate(Func func, type_list<Type...>) const {
        const auto last = view->data() + view->size();
        auto first = view->data();

        while(first != last) {
            if((std::get<pool_type<Component> *>(pools)->contains(*first) && ...) && !(std::get<const pool_type<Exclude> *>(filter)->contains(*first) || ...)) {
                const auto base = *(first++);
                const auto chunk = (std::min)({ (std::get<pool_type<Component> *>(pools)->size() - std::get<pool_type<Component> *>(pools)->index(base))... });
                size_type length{};

                for(++length;
                    length < chunk
                        && ((*(std::get<pool_type<Component> *>(pools)->data() + std::get<pool_type<Component> *>(pools)->index(base) + length) == *first) && ...)
                        && !(std::get<const pool_type<Exclude> *>(filter)->contains(*first) || ...);
                    ++length, ++first);

                func(view->data() + view->index(base), (std::get<pool_type<Type> *>(pools)->raw() + std::get<pool_type<Type> *>(pools)->index(base))..., length);
            } else {
                ++first;
            }
        }
    }

public:
    /*! @brief Underlying entity identifier. */
    using entity_type = Entity;
    /*! @brief Unsigned integer type. */
    using size_type = std::size_t;
    /*! @brief Bidirectional iterator type. */
    using iterator = view_iterator<typename basic_sparse_set<entity_type>::iterator>;
    /*! @brief Reverse iterator type. */
    using reverse_iterator = view_iterator<typename basic_sparse_set<entity_type>::reverse_iterator>;

    /**
     * @brief Forces the type to use to drive iterations.
     * @tparam Comp Type of component to use to drive the iteration.
     */
    template<typename Comp>
    void use() const ENTT_NOEXCEPT {
        view = std::get<pool_type<Comp> *>(pools);
    }

    /**
     * @brief Estimates the number of entities iterated by the view.
     * @return Estimated number of entities iterated by the view.
     */
    [[nodiscard]] size_type size_hint() const ENTT_NOEXCEPT {
        return (std::min)({ std::get<pool_type<Component> *>(pools)->size()... });
    }

    /**
     * @brief Returns an iterator to the first entity of the view.
     *
     * The returned iterator points to the first entity of the view. If the view
     * is empty, the returned iterator will be equal to `end()`.
     *
     * @return An iterator to the first entity of the view.
     */
    [[nodiscard]] iterator begin() const {
        return { view->begin(), view->end(), view->begin(), unchecked(view), filter };
    }

    /**
     * @brief Returns an iterator that is past the last entity of the view.
     *
     * The returned iterator points to the entity following the last entity of
     * the view. Attempting to dereference the returned iterator results in
     * undefined behavior.
     *
     * @return An iterator to the entity following the last entity of the view.
     */
    [[nodiscard]] iterator end() const {
        return { view->begin(), view->end(), view->end(), unchecked(view), filter };
    }

    /**
     * @brief Returns an iterator to the first entity of the reversed view.
     *
     * The returned iterator points to the first entity of the reversed view. If
     * the view is empty, the returned iterator will be equal to `rend()`.
     *
     * @return An iterator to the first entity of the reversed view.
     */
    [[nodiscard]] reverse_iterator rbegin() const {
        return { view->rbegin(), view->rend(), view->rbegin(), unchecked(view), filter };
    }

    /**
     * @brief Returns an iterator that is past the last entity of the reversed
     * view.
     *
     * The returned iterator points to the entity following the last entity of
     * the reversed view. Attempting to dereference the returned iterator
     * results in undefined behavior.
     *
     * @return An iterator to the entity following the last entity of the
     * reversed view.
     */
    [[nodiscard]] reverse_iterator rend() const {
        return { view->rbegin(), view->rend(), view->rend(), unchecked(view), filter };
    }

    /**
     * @brief Returns the first entity of the view, if any.
     * @return The first entity of the view if one exists, the null entity
     * otherwise.
     */
    [[nodiscard]] entity_type front() const {
        const auto it = begin();
        return it != end() ? *it : null;
    }

    /**
     * @brief Returns the last entity of the view, if any.
     * @return The last entity of the view if one exists, the null entity
     * otherwise.
     */
    [[nodiscard]] entity_type back() const {
        const auto it = rbegin();
        return it != rend() ? *it : null;
    }

    /**
     * @brief Finds an entity.
     * @param entt A valid entity identifier.
     * @return An iterator to the given entity if it's found, past the end
     * iterator otherwise.
     */
    [[nodiscard]] iterator find(const entity_type entt) const {
        iterator it{view->begin(), view->end(), view->find(entt), unchecked(view), filter};
        return (it != end() && *it == entt) ? it : end();
    }

    /**
     * @brief Checks if a view contains an entity.
     * @param entt A valid entity identifier.
     * @return True if the view contains the given entity, false otherwise.
     */
    [[nodiscard]] bool contains(const entity_type entt) const {
        return (std::get<pool_type<Component> *>(pools)->contains(entt) && ...) && !(std::get<const pool_type<Exclude> *>(filter)->contains(entt) || ...);
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
            return std::tuple_cat([entt](auto *cpool) {
                if constexpr(is_empty_v<typename std::remove_reference_t<decltype(*cpool)>::value_type>) {
                    return std::tuple{};
                } else {
                    return std::forward_as_tuple(cpool->get(entt));
                }
            }(std::get<pool_type<Component> *>(pools))...);
        } else if constexpr(sizeof...(Comp) == 1) {
            return (std::get<pool_type<Comp> *>(pools)->get(entt), ...);
        } else {
            return std::forward_as_tuple(get<Comp>(entt)...);
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
        ((std::get<pool_type<Component> *>(pools) == view ? traverse<Component>(std::move(func), return_type{}) : void()), ...);
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
     * @tparam Comp Type of component to use to drive the iteration.
     * @tparam Func Type of the function object to invoke.
     * @param func A valid function object.
     */
    template<typename Comp, typename Func>
    void each(Func func) const {
        use<Comp>();
        traverse<Comp>(std::move(func), return_type{});
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
    [[nodiscard]] iterable_view each() const ENTT_NOEXCEPT {
        return iterable_view{*this};
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
     * @tparam Comp Type of component to use to drive the iteration.
     * @return An iterable object to use to _visit_ the view.
     */
    template<typename Comp>
    [[nodiscard]] iterable_view each() const ENTT_NOEXCEPT {
        use<Comp>();
        return iterable_view{*this};
    }

    /**
     * @brief Chunked iteration for entities and components
     *
     * Chunked iteration tries to spot chunks in the sets of entities and
     * components and return them one at a time along with their sizes.<br/>
     * This type of iteration is intended where it's known a priori that the
     * creation of entities and components takes place in chunk, which is
     * actually quite common. In this case, various optimizations can be applied
     * downstream to obtain even better performances from the views.
     *
     * The signature of the function must be equivalent to the following:
     *
     * @code{.cpp}
     * void(const entity_type *, Type *..., size_type);
     * @endcode
     *
     * The arguments are as follows:
     *
     * * A pointer to the entities belonging to the chunk.
     * * Pointers to the components associated with the returned entities.
     * * The length of the chunk.
     *
     * Note that the callback can be invoked 0 or more times and no guarantee is
     * given on the order of the elements.
     *
     * @note
     * Empty types aren't explicitly instantiated and therefore they are never
     * returned during iterations.
     *
     * @tparam Func Type of the function object to invoke.
     * @param func A valid function object.
     */
    template<typename Func>
    void chunked(Func func) const {
        iterate(std::move(func), return_type{});
    }

private:
    const std::tuple<pool_type<Component> *...> pools;
    const std::tuple<const pool_type<Exclude> *...> filter;
    mutable const basic_sparse_set<entity_type>* view;
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
class basic_view<Entity, exclude_t<>, Component> final {
    /*! @brief A registry is allowed to create views. */
    friend class basic_registry<Entity>;

    using pool_type = pool_t<Entity, Component>;

    class iterable_view {
        friend class basic_view<Entity, exclude_t<>, Component>;

        template<typename... It>
        class iterable_view_iterator {
            friend class iterable_view;

            template<typename... Discard>
            iterable_view_iterator(It... from, Discard...) ENTT_NOEXCEPT
                : it{from...}
            {}

        public:
            using difference_type = std::ptrdiff_t;
            using value_type = decltype(std::tuple_cat(std::tuple<Entity>{}, std::declval<basic_view>().get({})));
            using pointer = void;
            using reference = value_type;
            using iterator_category = std::input_iterator_tag;

            iterable_view_iterator & operator++() ENTT_NOEXCEPT {
                return (++std::get<It>(it), ...), *this;
            }

            iterable_view_iterator operator++(int) ENTT_NOEXCEPT {
                iterable_view_iterator orig = *this;
                return ++(*this), orig;
            }

            [[nodiscard]] reference operator*() const ENTT_NOEXCEPT {
                return { *std::get<It>(it)... };
            }

            [[nodiscard]] bool operator==(const iterable_view_iterator &other) const ENTT_NOEXCEPT {
                return std::get<0>(other.it) == std::get<0>(it);
            }

            [[nodiscard]] bool operator!=(const iterable_view_iterator &other) const ENTT_NOEXCEPT {
                return !(*this == other);
            }

        private:
            std::tuple<It...> it;
        };

        iterable_view(pool_type &ref)
            : pool{&ref}
        {}

    public:
        using iterator = std::conditional_t<
            is_empty_v<Component>,
            iterable_view_iterator<typename basic_sparse_set<Entity>::iterator>,
            iterable_view_iterator<typename basic_sparse_set<Entity>::iterator, decltype(std::declval<pool_type>().begin())>
        >;
        using reverse_iterator = std::conditional_t<
            is_empty_v<Component>,
            iterable_view_iterator<typename basic_sparse_set<Entity>::reverse_iterator>,
            iterable_view_iterator<typename basic_sparse_set<Entity>::reverse_iterator, decltype(std::declval<pool_type>().rbegin())>
        >;

        [[nodiscard]] iterator begin() const ENTT_NOEXCEPT {
            return { pool->basic_sparse_set<entity_type>::begin(), pool->begin() };
        }

        [[nodiscard]] iterator end() const ENTT_NOEXCEPT {
            return { pool->basic_sparse_set<entity_type>::end(), pool->end() };
        }

        [[nodiscard]] reverse_iterator rbegin() const ENTT_NOEXCEPT {
            return { pool->basic_sparse_set<entity_type>::rbegin(), pool->rbegin() };
        }

        [[nodiscard]] reverse_iterator rend() const ENTT_NOEXCEPT {
            return { pool->basic_sparse_set<entity_type>::rend(), pool->rend() };
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
    using iterator = typename basic_sparse_set<Entity>::iterator;
    /*! @brief Reversed iterator type. */
    using reverse_iterator = typename basic_sparse_set<Entity>::reverse_iterator;

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
     * The returned pointer is such that range `[raw(), raw() + size())` is
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
     * The returned pointer is such that range `[data(), data() + size())` is
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
     * @brief Returns an iterator to the first entity of the view.
     *
     * The returned iterator points to the first entity of the view. If the view
     * is empty, the returned iterator will be equal to `end()`.
     *
     * @return An iterator to the first entity of the view.
     */
    [[nodiscard]] iterator begin() const ENTT_NOEXCEPT {
        return pool->basic_sparse_set<Entity>::begin();
    }

    /**
     * @brief Returns an iterator that is past the last entity of the view.
     *
     * The returned iterator points to the entity following the last entity of
     * the view. Attempting to dereference the returned iterator results in
     * undefined behavior.
     *
     * @return An iterator to the entity following the last entity of the view.
     */
    [[nodiscard]] iterator end() const ENTT_NOEXCEPT {
        return pool->basic_sparse_set<Entity>::end();
    }

    /**
     * @brief Returns an iterator to the first entity of the reversed view.
     *
     * The returned iterator points to the first entity of the reversed view. If
     * the view is empty, the returned iterator will be equal to `rend()`.
     *
     * @return An iterator to the first entity of the reversed view.
     */
    [[nodiscard]] reverse_iterator rbegin() const ENTT_NOEXCEPT {
        return pool->basic_sparse_set<Entity>::rbegin();
    }

    /**
     * @brief Returns an iterator that is past the last entity of the reversed
     * view.
     *
     * The returned iterator points to the entity following the last entity of
     * the reversed view. Attempting to dereference the returned iterator
     * results in undefined behavior.
     *
     * @return An iterator to the entity following the last entity of the
     * reversed view.
     */
    [[nodiscard]] reverse_iterator rend() const ENTT_NOEXCEPT {
        return pool->basic_sparse_set<Entity>::rend();
    }

    /**
     * @brief Returns the first entity of the view, if any.
     * @return The first entity of the view if one exists, the null entity
     * otherwise.
     */
    [[nodiscard]] entity_type front() const {
        const auto it = begin();
        return it != end() ? *it : null;
    }

    /**
     * @brief Returns the last entity of the view, if any.
     * @return The last entity of the view if one exists, the null entity
     * otherwise.
     */
    [[nodiscard]] entity_type back() const {
        const auto it = rbegin();
        return it != rend() ? *it : null;
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
     * Attempting to use an invalid component type results in a compilation
     * error. Attempting to use an entity that doesn't belong to the view
     * results in undefined behavior.<br/>
     * An assertion will abort the execution at runtime in debug mode if the
     * view doesn't contain the given entity.
     *
     * @tparam Comp Types of components to get.
     * @param entt A valid entity identifier.
     * @return The component assigned to the entity.
     */
    template<typename... Comp>
    [[nodiscard]] decltype(auto) get(const entity_type entt) const {
        if constexpr(sizeof...(Comp) == 0) {
            if constexpr(is_empty_v<Component>) {
                return std::tuple{};
            } else {
                return std::forward_as_tuple(pool->get(entt));
            }
        } else {
            static_assert(std::is_same_v<Comp..., Component>, "Invalid component type");
            return pool->get(entt);
        }
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
        if constexpr(is_empty_v<Component>) {
            if constexpr(std::is_invocable_v<Func, entity_type>) {
                for(const auto entt: *this) {
                    func(entt);
                }
            } else {
                for(auto pos = pool->size(); pos; --pos) {
                    func();
                }
            }
        } else {
            if constexpr(std::is_invocable_v<Func, entity_type, std::add_lvalue_reference_t<Component>>) {
                auto raw = pool->begin();

                for(const auto entt: *this) {
                    func(entt, *(raw++));
                }
            } else {
                for(auto &&component: *pool) {
                    func(component);
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
    [[nodiscard]] iterable_view each() const ENTT_NOEXCEPT {
        return iterable_view{*pool};
    }

private:
    pool_type *pool;
};


}


#endif

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
#include "sparse_set.hpp"
#include "storage.hpp"
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
    template<typename Comp>
    using storage_type = constness_as_t<typename storage_traits<Entity, std::remove_const_t<Comp>>::storage_type, Comp>;

    using unchecked_type = std::array<const basic_sparse_set<Entity> *, (sizeof...(Component) - 1)>;

    template<typename It>
    class view_iterator final {
        friend class basic_view<Entity, exclude_t<Exclude...>, Component...>;

        view_iterator(It from, It to, It curr, unchecked_type other, const std::tuple<const storage_type<Exclude> *...> &ignore) ENTT_NOEXCEPT
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
                && !(std::get<const storage_type<Exclude> *>(filter)->contains(entt) || ...);
        }

    public:
        using difference_type = typename std::iterator_traits<It>::difference_type;
        using value_type = typename std::iterator_traits<It>::value_type;
        using pointer = typename std::iterator_traits<It>::pointer;
        using reference = typename std::iterator_traits<It>::reference;
        using iterator_category = std::bidirectional_iterator_tag;

        view_iterator() ENTT_NOEXCEPT
            : view_iterator{{}, {}, {}, {}, {}}
        {}

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
        std::tuple<const storage_type<Exclude> *...> filter;
    };

    class iterable_view final {
        friend class basic_view<Entity, exclude_t<Exclude...>, Component...>;

        template<typename It>
        class iterable_view_iterator final {
            friend class iterable_view;

            iterable_view_iterator(It from, const basic_view *parent) ENTT_NOEXCEPT
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
                return std::tuple_cat(std::make_tuple(*it), view->get(*it));
            }

            [[nodiscard]] bool operator==(const iterable_view_iterator &other) const ENTT_NOEXCEPT {
                return other.it == it;
            }

            [[nodiscard]] bool operator!=(const iterable_view_iterator &other) const ENTT_NOEXCEPT {
                return !(*this == other);
            }

        private:
            It it;
            const basic_view *view;
        };

        iterable_view(const basic_view &parent)
            : view{parent}
        {}

    public:
        using iterator = iterable_view_iterator<view_iterator<typename basic_sparse_set<Entity>::iterator>>;
        using reverse_iterator = iterable_view_iterator<view_iterator<typename basic_sparse_set<Entity>::reverse_iterator>>;

        [[nodiscard]] iterator begin() const ENTT_NOEXCEPT {
            return { view.begin(), &view };
        }

        [[nodiscard]] iterator end() const ENTT_NOEXCEPT {
            return { view.end(), &view };
        }

        [[nodiscard]] reverse_iterator rbegin() const ENTT_NOEXCEPT {
            return { view.rbegin(), &view };
        }

        [[nodiscard]] reverse_iterator rend() const ENTT_NOEXCEPT {
            return { view.rend(), &view };
        }

    private:
        const basic_view view;
    };

    [[nodiscard]] const basic_sparse_set<Entity> * candidate() const ENTT_NOEXCEPT {
        return (std::min)({ static_cast<const basic_sparse_set<entity_type> *>(std::get<storage_type<Component> *>(pools))... }, [](const auto *lhs, const auto *rhs) {
            return lhs->size() < rhs->size();
        });
    }

    [[nodiscard]] unchecked_type unchecked(const basic_sparse_set<Entity> *cpool) const {
        std::size_t pos{};
        unchecked_type other{};
        (static_cast<void>(std::get<storage_type<Component> *>(pools) == cpool ? void() : void(other[pos++] = std::get<storage_type<Component> *>(pools))), ...);
        return other;
    }

    template<typename Comp, typename It>
    [[nodiscard]] auto dispatch_get([[maybe_unused]] It &it, [[maybe_unused]] const Entity entt) const {
        if constexpr(std::is_same_v<typename std::iterator_traits<It>::value_type, typename storage_type<Comp>::value_type>) {
            return std::forward_as_tuple(*it);
        } else {
            return get_as_tuple(*std::get<storage_type<Comp> *>(pools), entt);
        }
    }

    template<typename Comp, typename Func>
    void traverse(Func func) const {
        if constexpr(std::is_void_v<decltype(std::get<storage_type<Comp> *>(pools)->get({}))>) {
            for(const auto entt: static_cast<const basic_sparse_set<entity_type> &>(*std::get<storage_type<Comp> *>(pools))) {
                if(((std::is_same_v<Comp, Component> || std::get<storage_type<Component> *>(pools)->contains(entt)) && ...)
                    && !(std::get<const storage_type<Exclude> *>(filter)->contains(entt) || ...))
                {
                    if constexpr(is_applicable_v<Func, decltype(std::tuple_cat(std::tuple<entity_type>{}, std::declval<basic_view>().get({})))>) {
                        std::apply(func, std::tuple_cat(std::make_tuple(entt), get(entt)));
                    } else {
                        std::apply(func, get(entt));
                    }
                }
            }
        } else {
            auto it = std::get<storage_type<Comp> *>(pools)->begin();

            for(const auto entt: static_cast<const basic_sparse_set<entity_type> &>(*std::get<storage_type<Comp> *>(pools))) {
                if(((std::is_same_v<Comp, Component> || std::get<storage_type<Component> *>(pools)->contains(entt)) && ...)
                    && !(std::get<const storage_type<Exclude> *>(filter)->contains(entt) || ...))
                {
                    if constexpr(is_applicable_v<Func, decltype(std::tuple_cat(std::tuple<entity_type>{}, std::declval<basic_view>().get({})))>) {
                        std::apply(func, std::tuple_cat(std::make_tuple(entt), dispatch_get<Component>(it, entt)...));
                    } else {
                        std::apply(func, std::tuple_cat(dispatch_get<Component>(it, entt)...));
                    }
                }

                ++it;
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

    /*! @brief Default constructor to use to create empty, invalid views. */
    basic_view() ENTT_NOEXCEPT
        : view{}
    {}

    /**
     * @brief Constructs a multi-type view from a set of storage classes.
     * @param component The storage for the types to iterate.
     * @param epool The storage for the types used to filter the view.
     */
    basic_view(storage_type<Component> &... component, const storage_type<Exclude> &... epool) ENTT_NOEXCEPT
        : pools{&component...},
          filter{&epool...},
          view{candidate()}
    {}

    /**
     * @brief Forces the type to use to drive iterations.
     * @tparam Comp Type of component to use to drive the iteration.
     */
    template<typename Comp>
    void use() const ENTT_NOEXCEPT {
        view = std::get<storage_type<Comp> *>(pools);
    }

    /**
     * @brief Estimates the number of entities iterated by the view.
     * @return Estimated number of entities iterated by the view.
     */
    [[nodiscard]] size_type size_hint() const ENTT_NOEXCEPT {
        return view->size();
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
        return iterator{view->begin(), view->end(), view->begin(), unchecked(view), filter};
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
        return iterator{view->begin(), view->end(), view->end(), unchecked(view), filter};
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
        return reverse_iterator{view->rbegin(), view->rend(), view->rbegin(), unchecked(view), filter};
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
        return reverse_iterator{view->rbegin(), view->rend(), view->rend(), unchecked(view), filter};
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
        const auto it = iterator{view->begin(), view->end(), view->find(entt), unchecked(view), filter};
        return (it != end() && *it == entt) ? it : end();
    }

    /**
     * @brief Checks if a view is properly initialized.
     * @return True if the view is properly initialized, false otherwise.
     */
    [[nodiscard]] explicit operator bool() const ENTT_NOEXCEPT {
        return view != nullptr;
    }

    /**
     * @brief Checks if a view contains an entity.
     * @param entt A valid entity identifier.
     * @return True if the view contains the given entity, false otherwise.
     */
    [[nodiscard]] bool contains(const entity_type entt) const {
        return (std::get<storage_type<Component> *>(pools)->contains(entt) && ...) && !(std::get<const storage_type<Exclude> *>(filter)->contains(entt) || ...);
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
     * results in undefined behavior.
     *
     * @tparam Comp Types of components to get.
     * @param entt A valid entity identifier.
     * @return The components assigned to the entity.
     */
    template<typename... Comp>
    [[nodiscard]] decltype(auto) get([[maybe_unused]] const entity_type entt) const {
        ENTT_ASSERT(contains(entt));

        if constexpr(sizeof...(Comp) == 0) {
            return std::tuple_cat(get_as_tuple(*std::get<storage_type<Component> *>(pools), entt)...);
        } else if constexpr(sizeof...(Comp) == 1) {
            return (std::get<storage_type<Comp> *>(pools)->get(entt), ...);
        } else {
            return std::tuple_cat(get_as_tuple(*std::get<storage_type<Comp> *>(pools), entt)...);
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
        ((std::get<storage_type<Component> *>(pools) == view ? traverse<Component>(std::move(func)) : void()), ...);
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
        traverse<Comp>(std::move(func));
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
     * @brief Combines two views in a _more specific_ one (friend function).
     * @tparam Id A valid entity type (see entt_traits for more details).
     * @tparam ELhs Filter list of the first view.
     * @tparam CLhs Component list of the first view.
     * @tparam ERhs Filter list of the second view.
     * @tparam CRhs Component list of the second view.
     * @return A more specific view.
     */
    template<typename Id, typename... ELhs, typename... CLhs, typename... ERhs, typename... CRhs>
    friend auto operator|(const basic_view<Id, exclude_t<ELhs...>, CLhs...> &, const basic_view<Id, exclude_t<ERhs...>, CRhs...> &);

private:
    const std::tuple<storage_type<Component> *...> pools;
    const std::tuple<const storage_type<Exclude> *...> filter;
    mutable const basic_sparse_set<entity_type> *view;
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
    using storage_type = constness_as_t<typename storage_traits<Entity, std::remove_const_t<Component>>::storage_type, Component>;

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

        iterable_view(storage_type &ref)
            : pool{&ref}
        {}

    public:
        using iterator = std::conditional_t<
            std::is_void_v<decltype(std::declval<storage_type>().get({}))>,
            iterable_view_iterator<typename basic_sparse_set<Entity>::iterator>,
            iterable_view_iterator<typename basic_sparse_set<Entity>::iterator, decltype(std::declval<storage_type>().begin())>
        >;
        using reverse_iterator = std::conditional_t<
            std::is_void_v<decltype(std::declval<storage_type>().get({}))>,
            iterable_view_iterator<typename basic_sparse_set<Entity>::reverse_iterator>,
            iterable_view_iterator<typename basic_sparse_set<Entity>::reverse_iterator, decltype(std::declval<storage_type>().rbegin())>
        >;

        [[nodiscard]] iterator begin() const ENTT_NOEXCEPT {
            return iterator{pool->basic_sparse_set<entity_type>::begin(), pool->begin()};
        }

        [[nodiscard]] iterator end() const ENTT_NOEXCEPT {
            return iterator{pool->basic_sparse_set<entity_type>::end(), pool->end()};
        }

        [[nodiscard]] reverse_iterator rbegin() const ENTT_NOEXCEPT {
            return reverse_iterator{pool->basic_sparse_set<entity_type>::rbegin(), pool->rbegin()};
        }

        [[nodiscard]] reverse_iterator rend() const ENTT_NOEXCEPT {
            return reverse_iterator{pool->basic_sparse_set<entity_type>::rend(), pool->rend()};
        }

    private:
        storage_type * const pool;
    };

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

    /*! @brief Default constructor to use to create empty, invalid views. */
    basic_view() ENTT_NOEXCEPT
        : pools{},
          filter{}
    {}

    /**
     * @brief Constructs a single-type view from a storage class.
     * @param ref The storage for the type to iterate.
     */
    basic_view(storage_type &ref) ENTT_NOEXCEPT
        : pools{&ref},
          filter{}
    {}

    /**
     * @brief Returns the number of entities that have the given component.
     * @return Number of entities that have the given component.
     */
    [[nodiscard]] size_type size() const ENTT_NOEXCEPT {
        return std::get<0>(pools)->size();
    }

    /**
     * @brief Checks whether a view is empty.
     * @return True if the view is empty, false otherwise.
     */
    [[nodiscard]] bool empty() const ENTT_NOEXCEPT {
        return std::get<0>(pools)->empty();
    }

    /**
     * @brief Direct access to the list of components.
     *
     * The returned pointer is such that range `[raw(), raw() + size())` is
     * always a valid range, even if the container is empty.
     *
     * @return A pointer to the array of components.
     */
    [[nodiscard]] raw_type * raw() const ENTT_NOEXCEPT {
        return std::get<0>(pools)->raw();
    }

    /**
     * @brief Direct access to the list of entities.
     *
     * The returned pointer is such that range `[data(), data() + size())` is
     * always a valid range, even if the container is empty.
     *
     * @return A pointer to the array of entities.
     */
    [[nodiscard]] const entity_type * data() const ENTT_NOEXCEPT {
        return std::get<0>(pools)->data();
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
        return std::get<0>(pools)->basic_sparse_set<entity_type>::begin();
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
        return std::get<0>(pools)->basic_sparse_set<entity_type>::end();
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
        return std::get<0>(pools)->basic_sparse_set<entity_type>::rbegin();
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
        return std::get<0>(pools)->basic_sparse_set<entity_type>::rend();
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
        const auto it = std::get<0>(pools)->find(entt);
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
     * @brief Checks if a view is properly initialized.
     * @return True if the view is properly initialized, false otherwise.
     */
    [[nodiscard]] explicit operator bool() const ENTT_NOEXCEPT {
        return std::get<0>(pools) != nullptr;
    }

    /**
     * @brief Checks if a view contains an entity.
     * @param entt A valid entity identifier.
     * @return True if the view contains the given entity, false otherwise.
     */
    [[nodiscard]] bool contains(const entity_type entt) const {
        return std::get<0>(pools)->contains(entt);
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
     * results in undefined behavior.
     *
     * @tparam Comp Types of components to get.
     * @param entt A valid entity identifier.
     * @return The component assigned to the entity.
     */
    template<typename... Comp>
    [[nodiscard]] decltype(auto) get(const entity_type entt) const {
        ENTT_ASSERT(contains(entt));

        if constexpr(sizeof...(Comp) == 0) {
            return get_as_tuple(*std::get<0>(pools), entt);
        } else {
            static_assert(std::is_same_v<Comp..., Component>, "Invalid component type");
            return std::get<0>(pools)->get(entt);
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
        if constexpr(std::is_void_v<decltype(std::get<0>(pools)->get({}))>) {
            if constexpr(std::is_invocable_v<Func>) {
                for(auto pos = size(); pos; --pos) {
                    func();
                }
            } else {
                for(auto entity: *this) {
                    func(entity);
                }
            }
        } else {
            if constexpr(is_applicable_v<Func, decltype(*each().begin())>) {
                for(const auto pack: each()) {
                    std::apply(func, pack);
                }
            } else {
                for(auto &&component: *std::get<0>(pools)) {
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
        return iterable_view{*std::get<0>(pools)};
    }

    /**
     * @brief Combines two views in a _more specific_ one (friend function).
     * @tparam Id A valid entity type (see entt_traits for more details).
     * @tparam ELhs Filter list of the first view.
     * @tparam CLhs Component list of the first view.
     * @tparam ERhs Filter list of the second view.
     * @tparam CRhs Component list of the second view.
     * @return A more specific view.
     */
    template<typename Id, typename... ELhs, typename... CLhs, typename... ERhs, typename... CRhs>
    friend auto operator|(const basic_view<Id, exclude_t<ELhs...>, CLhs...> &, const basic_view<Id, exclude_t<ERhs...>, CRhs...> &);

private:
    const std::tuple<storage_type *> pools;
    const std::tuple<> filter;
};


/**
 * @brief Deduction guide.
 * @tparam Storage Type of storage classes used to create the view.
 * @param storage The storage for the types to iterate.
 */
template<typename... Storage>
basic_view(Storage &... storage)
-> basic_view<std::common_type_t<typename Storage::entity_type...>, entt::exclude_t<>, constness_as_t<typename Storage::value_type, Storage>...>;


/**
 * @brief Combines two views in a _more specific_ one.
 * @tparam Entity A valid entity type (see entt_traits for more details).
 * @tparam ELhs Filter list of the first view.
 * @tparam CLhs Component list of the first view.
 * @tparam ERhs Filter list of the second view.
 * @tparam CRhs Component list of the second view.
 * @param lhs A valid reference to the first view.
 * @param rhs A valid reference to the second view.
 * @return A more specific view.
 */
template<typename Entity, typename... ELhs, typename... CLhs, typename... ERhs, typename... CRhs>
[[nodiscard]] auto operator|(const basic_view<Entity, exclude_t<ELhs...>, CLhs...> &lhs, const basic_view<Entity, exclude_t<ERhs...>, CRhs...> &rhs) {
    using view_type = basic_view<Entity, exclude_t<ELhs..., ERhs...>, CLhs..., CRhs...>;
    return std::apply([](auto *... storage) { return view_type{*storage...}; }, std::tuple_cat(lhs.pools, rhs.pools, lhs.filter, rhs.filter));
}


}


#endif

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
#include "component.hpp"
#include "entity.hpp"
#include "fwd.hpp"
#include "sparse_set.hpp"
#include "storage.hpp"
#include "utility.hpp"


namespace entt {


/**
 * @cond TURN_OFF_DOXYGEN
 * Internal details not to be documented.
 */


namespace internal {


template<typename Entity, typename Component>
class iterable_storage final {
    using storage_type = constness_as_t<typename storage_traits<Entity, std::remove_const_t<Component>>::storage_type, Component>;
    using basic_common_type = typename storage_type::base_type;

    template<typename... It>
    struct iterable_storage_iterator final {
        using difference_type = std::ptrdiff_t;
        using value_type = decltype(std::tuple_cat(std::tuple<Entity>{}, std::declval<decltype(std::declval<storage_type &>().get_as_tuple({}))>()));
        using pointer = void;
        using reference = value_type;
        using iterator_category = std::input_iterator_tag;

        template<typename... Discard>
        iterable_storage_iterator(It... from, Discard...) ENTT_NOEXCEPT
            : it{from...}
        {}

        iterable_storage_iterator & operator++() ENTT_NOEXCEPT {
            return (++std::get<It>(it), ...), *this;
        }

        iterable_storage_iterator operator++(int) ENTT_NOEXCEPT {
            iterable_storage_iterator orig = *this;
            return ++(*this), orig;
        }

        [[nodiscard]] reference operator*() const ENTT_NOEXCEPT {
            return { *std::get<It>(it)... };
        }

        [[nodiscard]] bool operator==(const iterable_storage_iterator &other) const ENTT_NOEXCEPT {
            return std::get<0>(other.it) == std::get<0>(it);
        }

        [[nodiscard]] bool operator!=(const iterable_storage_iterator &other) const ENTT_NOEXCEPT {
            return !(*this == other);
        }

    private:
        std::tuple<It...> it;
    };

public:
    using iterator = std::conditional_t<
        ignore_as_empty_v<std::remove_const_t<Component>>,
        iterable_storage_iterator<typename basic_common_type::iterator>,
        iterable_storage_iterator<typename basic_common_type::iterator, decltype(std::declval<storage_type>().begin())>
    >;
    using reverse_iterator = std::conditional_t<
        ignore_as_empty_v<std::remove_const_t<Component>>,
        iterable_storage_iterator<typename basic_common_type::reverse_iterator>,
        iterable_storage_iterator<typename basic_common_type::reverse_iterator, decltype(std::declval<storage_type>().rbegin())>
    >;

    iterable_storage(storage_type &ref)
        : pool{&ref}
    {}

    [[nodiscard]] iterator begin() const ENTT_NOEXCEPT {
        return iterator{pool->basic_common_type::begin(), pool->begin()};
    }

    [[nodiscard]] iterator end() const ENTT_NOEXCEPT {
        return iterator{pool->basic_common_type::end(), pool->end()};
    }

    [[nodiscard]] reverse_iterator rbegin() const ENTT_NOEXCEPT {
        return reverse_iterator{pool->basic_common_type::rbegin(), pool->rbegin()};
    }

    [[nodiscard]] reverse_iterator rend() const ENTT_NOEXCEPT {
        return reverse_iterator{pool->basic_common_type::rend(), pool->rend()};
    }

private:
    storage_type * const pool;
};


template<typename Type, typename It, std::size_t Component, std::size_t Exclude>
class view_iterator final {
    static constexpr auto is_multi_type_v = ((Component + Exclude) != 0u);

    [[nodiscard]] bool valid() const {
        return (is_multi_type_v || (*it != tombstone))
            && std::apply([entt = *it](const auto *... curr) { return (curr->contains(entt) && ...); }, pools)
            && std::apply([entt = *it](const auto *... curr) { return (!curr->contains(entt) && ...); }, filter);
    }

public:
    using iterator_type = It;
    using difference_type = typename std::iterator_traits<It>::difference_type;
    using value_type = typename std::iterator_traits<It>::value_type;
    using pointer = typename std::iterator_traits<It>::pointer;
    using reference = typename std::iterator_traits<It>::reference;
    using iterator_category = std::bidirectional_iterator_tag;

    view_iterator() ENTT_NOEXCEPT
        : first{},
          last{},
          it{},
          pools{},
          filter{}
    {}

    view_iterator(It from, It to, It curr, std::array<const Type *, Component> all_of, std::array<const Type *, Exclude> none_of) ENTT_NOEXCEPT
        : first{from},
          last{to},
          it{curr},
          pools{all_of},
          filter{none_of}
    {
        if(it != last && !valid()) {
            ++(*this);
        }
    }

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
    std::array<const Type *, Component> pools;
    std::array<const Type *, Exclude> filter;
};


}


/**
 * Internal details not to be documented.
 * @endcond
 */


/**
 * @brief View implementation.
 *
 * Primary template isn't defined on purpose. All the specializations give a
 * compile-time error, but for a few reasonable cases.
 */
template<typename, typename, typename, typename>
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
 * @tparam Component Types of components iterated by the view.
 * @tparam Exclude Types of components used to filter the view.
 */
template<typename Entity, typename... Component, typename... Exclude>
class basic_view<Entity, get_t<Component...>, exclude_t<Exclude...>> {
    template<typename, typename, typename, typename>
    friend class basic_view;

    static constexpr auto is_multi_type_v = ((sizeof...(Component) + sizeof...(Exclude)) != 1u);
    using basic_common_type = std::common_type_t<typename storage_traits<Entity, std::remove_const_t<Component>>::storage_type::base_type...>;

    class iterable final {
        template<typename It>
        struct iterable_iterator final {
            using difference_type = std::ptrdiff_t;
            using value_type = decltype(std::tuple_cat(std::tuple<Entity>{}, std::declval<basic_view>().get({})));
            using pointer = void;
            using reference = value_type;
            using iterator_category = std::input_iterator_tag;

            iterable_iterator(It from, const basic_view *parent) ENTT_NOEXCEPT
                : it{from},
                  view{parent}
            {}

            iterable_iterator & operator++() ENTT_NOEXCEPT {
                return ++it, *this;
            }

            iterable_iterator operator++(int) ENTT_NOEXCEPT {
                iterable_iterator orig = *this;
                return ++(*this), orig;
            }

            [[nodiscard]] reference operator*() const ENTT_NOEXCEPT {
                return std::tuple_cat(std::make_tuple(*it), view->get(*it));
            }

            [[nodiscard]] bool operator==(const iterable_iterator &other) const ENTT_NOEXCEPT {
                return other.it == it;
            }

            [[nodiscard]] bool operator!=(const iterable_iterator &other) const ENTT_NOEXCEPT {
                return !(*this == other);
            }

        private:
            It it;
            const basic_view *view;
        };

    public:
        using iterator = iterable_iterator<internal::view_iterator<basic_common_type, typename basic_common_type::iterator, sizeof...(Component) - 1u, sizeof...(Exclude)>>;
        using reverse_iterator = iterable_iterator<internal::view_iterator<basic_common_type, typename basic_common_type::reverse_iterator, sizeof...(Component) - 1u, sizeof...(Exclude)>>;

        iterable(const basic_view &parent)
            : view{parent}
        {}

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

    [[nodiscard]] const auto * candidate() const ENTT_NOEXCEPT {
        return (std::min)({ static_cast<const basic_common_type *>(std::get<storage_type<Component> *>(pools))... }, [](const auto *lhs, const auto *rhs) {
            return lhs->size() < rhs->size();
        });
    }

    [[nodiscard]] auto test_set() const ENTT_NOEXCEPT {
        std::size_t pos{};
        std::array<const basic_common_type *, sizeof...(Component) - 1u> other{};
        (static_cast<void>(std::get<storage_type<Component> *>(pools) == view ? void() : void(other[pos++] = std::get<storage_type<Component> *>(pools))), ...);
        return other;
    }

    template<typename Comp, typename Other, typename... Args>
    [[nodiscard]] auto dispatch_get(const std::tuple<Entity, Args...> &curr) const {
        if constexpr(std::is_same_v<Comp, Other>) {
            return std::forward_as_tuple(std::get<Args>(curr)...);
        } else {
            return std::get<storage_type<Other> *>(pools)->get_as_tuple(std::get<0>(curr));
        }
    }

public:
    /*! @brief Underlying entity identifier. */
    using entity_type = Entity;
    /*! @brief Unsigned integer type. */
    using size_type = std::size_t;
    /*! @brief Bidirectional iterator type. */
    using iterator = internal::view_iterator<basic_common_type, typename basic_common_type::iterator, sizeof...(Component) - 1u, sizeof...(Exclude)>;
    /*! @brief Reverse iterator type. */
    using reverse_iterator = internal::view_iterator<basic_common_type, typename basic_common_type::reverse_iterator, sizeof...(Component) - 1u, sizeof...(Exclude)>;
    /*! @brief Iterable view type. */
    using iterable_view = iterable;

    /**
     * @brief Storage type associated with a given component.
     * @tparam Type Type of component.
     */
    template<typename Comp>
    using storage_type = constness_as_t<typename storage_traits<Entity, std::remove_const_t<Comp>>::storage_type, Comp>;

    /*! @brief Default constructor to use to create empty, invalid views. */
    basic_view() ENTT_NOEXCEPT
        : pools{},
          filter{},
          view{}
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
     * @brief Creates a new view driven by a given type in its iterations.
     * @tparam Comp Type of component to use to drive the iteration.
     * @return A new view driven by the given type in its iterations.
     */
    template<typename Comp>
    [[nodiscard]] basic_view use() const ENTT_NOEXCEPT {
        basic_view other{*this};
        other.view = std::get<storage_type<Comp> *>(pools);
        return other;
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
        return iterator{view->begin(), view->end(), view->begin(), test_set(), filter};
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
        return iterator{view->begin(), view->end(), view->end(), test_set(), filter};
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
        return reverse_iterator{view->rbegin(), view->rend(), view->rbegin(), test_set(), filter};
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
        return reverse_iterator{view->rbegin(), view->rend(), view->rend(), test_set(), filter};
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
     * @param entt A valid identifier.
     * @return An iterator to the given entity if it's found, past the end
     * iterator otherwise.
     */
    [[nodiscard]] iterator find(const entity_type entt) const {
        const auto it = iterator{view->begin(), view->end(), view->find(entt), test_set(), filter};
        return (it != end() && *it == entt) ? it : end();
    }

    /**
     * @brief Returns the components assigned to the given entity.
     * @param entt A valid identifier.
     * @return The components assigned to the given entity.
     */
    [[nodiscard]] decltype(auto) operator[](const entity_type entt) const {
        return get<Component...>(entt);
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
     * @param entt A valid identifier.
     * @return True if the view contains the given entity, false otherwise.
     */
    [[nodiscard]] bool contains(const entity_type entt) const {
        return (std::get<storage_type<Component> *>(pools)->contains(entt) && ...)
            && std::apply([entt](const auto *... curr) { return (!curr->contains(entt) && ...); }, filter);
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
     * @param entt A valid identifier.
     * @return The components assigned to the entity.
     */
    template<typename... Comp>
    [[nodiscard]] decltype(auto) get([[maybe_unused]] const entity_type entt) const {
        ENTT_ASSERT(contains(entt), "View does not contain entity");

        if constexpr(sizeof...(Comp) == 0) {
            return std::tuple_cat(std::get<storage_type<Component> *>(pools)->get_as_tuple(entt)...);
        } else if constexpr(sizeof...(Comp) == 1) {
            return (std::get<storage_type<Comp> *>(pools)->get(entt), ...);
        } else {
            return std::tuple_cat(std::get<storage_type<Comp> *>(pools)->get_as_tuple(entt)...);
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
        ((std::get<storage_type<Component> *>(pools) == view ? each<Component>(std::move(func)) : void()), ...);
    }

    /**
     * @brief Iterates entities and components and applies the given function
     * object to them.
     *
     * The pool of the suggested component is used to lead the iterations. The
     * returned entities will therefore respect the order of the pool associated
     * with that type.
     *
     * @sa each
     *
     * @tparam Comp Type of component to use to drive the iteration.
     * @tparam Func Type of the function object to invoke.
     * @param func A valid function object.
     */
    template<typename Comp, typename Func>
    void each(Func func) const {
        for(const auto curr: internal::iterable_storage<Entity, Comp>{*std::get<storage_type<Comp> *>(pools)}) {
            if((is_multi_type_v || (std::get<0>(curr) != tombstone))
                && ((std::is_same_v<Comp, Component> || std::get<storage_type<Component> *>(pools)->contains(std::get<0>(curr))) && ...)
                && std::apply([entt = std::get<0>(curr)](const auto *... cpool) { return (!cpool->contains(entt) && ...); }, filter))
            {
                if constexpr(is_applicable_v<Func, decltype(std::tuple_cat(std::tuple<entity_type>{}, std::declval<basic_view>().get({})))>) {
                    std::apply(func, std::tuple_cat(std::make_tuple(std::get<0>(curr)), dispatch_get<Comp, Component>(curr)...));
                } else {
                    std::apply(func, std::tuple_cat(dispatch_get<Comp, Component>(curr)...));
                }
            }
        }
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
     * with that type.
     *
     * @sa each
     *
     * @tparam Comp Type of component to use to drive the iteration.
     * @return An iterable object to use to _visit_ the view.
     */
    template<typename Comp>
    [[nodiscard]] iterable_view each() const ENTT_NOEXCEPT {
        basic_view other{*this};
        other.view = std::get<storage_type<Comp> *>(pools);
        return iterable_view{std::move(other)};
    }

    /**
     * @brief Combines two views in a _more specific_ one (friend function).
     * @tparam Get Component list of the view to combine with.
     * @tparam Excl Filter list of the view to combine with.
     * @param other The view to combine with.
     * @return A more specific view.
     */
    template<typename... Get, typename... Excl>
    [[nodiscard]] auto operator|(const basic_view<Entity, get_t<Get...>, exclude_t<Excl...>> &other) const ENTT_NOEXCEPT {
        using view_type = basic_view<Entity, get_t<Component..., Get...>, exclude_t<Exclude..., Excl...>>;
        return std::make_from_tuple<view_type>(std::tuple_cat(
            std::apply([](auto *... curr) { return std::forward_as_tuple(*curr...); }, pools),
            std::apply([](auto *... curr) { return std::forward_as_tuple(*curr...); }, other.pools),
            std::apply([](const auto *... curr) { return std::forward_as_tuple(static_cast<const storage_type<Exclude> &>(*curr)...); }, filter),
            std::apply([](const auto *... curr) { return std::forward_as_tuple(static_cast<const storage_type<Excl> &>(*curr)...); }, other.filter)
        ));
    }


private:
    std::tuple<storage_type<Component> *...> pools;
    std::array<const basic_common_type *, sizeof...(Exclude)> filter;
    const basic_common_type *view;
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
class basic_view<Entity, get_t<Component>, exclude_t<>, 
    // Yeah, there is a reason why void_t and enable_if_t were combined here. Try removing the first one and let me know. :)
    std::void_t<std::enable_if_t<!in_place_delete_v<std::remove_const_t<Component>>>>
> {
    template<typename, typename, typename, typename>
    friend class basic_view;

    using basic_common_type = typename storage_traits<Entity, std::remove_const_t<Component>>::storage_type::base_type;

public:
    /*! @brief Underlying entity identifier. */
    using entity_type = Entity;
    /*! @brief Unsigned integer type. */
    using size_type = std::size_t;
    /*! @brief Random access iterator type. */
    using iterator = typename basic_common_type::iterator;
    /*! @brief Reversed iterator type. */
    using reverse_iterator = typename basic_common_type::reverse_iterator;
    /*! @brief Iterable view type. */
    using iterable_view = internal::iterable_storage<Entity, Component>;
    /*! @brief Storage type associated with the view component. */
    using storage_type = constness_as_t<typename storage_traits<Entity, std::remove_const_t<Component>>::storage_type, Component>;

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
     * @brief Direct access to the raw representation offered by the storage.
     * @return A pointer to the array of components.
     */
    [[nodiscard]] auto raw() const ENTT_NOEXCEPT {
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
    [[nodiscard]] auto data() const ENTT_NOEXCEPT {
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
        return std::get<0>(pools)->basic_common_type::begin();
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
        return std::get<0>(pools)->basic_common_type::end();
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
        return std::get<0>(pools)->basic_common_type::rbegin();
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
        return std::get<0>(pools)->basic_common_type::rend();
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
     * @param entt A valid identifier.
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
     * @brief Returns the component assigned to the given entity.
     * @param entt A valid identifier.
     * @return The component assigned to the given entity.
     */
    [[nodiscard]] decltype(auto) operator[](const entity_type entt) const {
        return get<Component>(entt);
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
     * @param entt A valid identifier.
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
     * @param entt A valid identifier.
     * @return The component assigned to the entity.
     */
    template<typename... Comp>
    [[nodiscard]] decltype(auto) get(const entity_type entt) const {
        ENTT_ASSERT(contains(entt), "View does not contain entity");

        if constexpr(sizeof...(Comp) == 0) {
            return std::get<0>(pools)->get_as_tuple(entt);
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
        if constexpr(ignore_as_empty_v<std::remove_const_t<Component>>) {
            if constexpr(std::is_invocable_v<Func>) {
                for(size_type pos{}, last = size(); pos < last; ++pos) {
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
     * @tparam Get Component list of the view to combine with.
     * @tparam Excl Filter list of the view to combine with.
     * @param other The view to combine with.
     * @return A more specific view.
     */
    template<typename... Get, typename... Excl>
    [[nodiscard]] auto operator|(const basic_view<Entity, get_t<Get...>, exclude_t<Excl...>> &other) const ENTT_NOEXCEPT {
        using view_type = basic_view<Entity, get_t<Component, Get...>, exclude_t<Excl...>>;
        return std::make_from_tuple<view_type>(std::tuple_cat(
            std::apply([](auto *... curr) { return std::forward_as_tuple(*curr...); }, pools),
            std::apply([](auto *... curr) { return std::forward_as_tuple(*curr...); }, other.pools),
            std::apply([](const auto *... curr) { return std::forward_as_tuple(static_cast<const typename view_type::template storage_type<Excl> &>(*curr)...); }, other.filter)
        ));
    }

private:
    std::tuple<storage_type *> pools;
    std::tuple<> filter;
};


/**
 * @brief Deduction guide.
 * @tparam Storage Type of storage classes used to create the view.
 * @param storage The storage for the types to iterate.
 */
template<typename... Storage>
basic_view(Storage &... storage)
-> basic_view<std::common_type_t<typename Storage::entity_type...>, get_t<constness_as_t<typename Storage::value_type, Storage>...>, exclude_t<>>;


}


#endif

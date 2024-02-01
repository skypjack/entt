#ifndef ENTT_ENTITY_VIEW_HPP
#define ENTT_ENTITY_VIEW_HPP

#include <array>
#include <iterator>
#include <tuple>
#include <type_traits>
#include <utility>
#include "../config/config.h"
#include "../core/iterator.hpp"
#include "../core/type_traits.hpp"
#include "entity.hpp"
#include "fwd.hpp"

namespace entt {

/*! @cond TURN_OFF_DOXYGEN */
namespace internal {

template<typename Type, typename Entity>
[[nodiscard]] bool all_of_but(const std::size_t index, const Type *const *it, const std::size_t len, const Entity entt) noexcept {
    std::size_t pos{};
    for(; (pos != index) && it[pos]->contains(entt); ++pos) {}

    if(pos == index) {
        for(++pos; (pos != len) && it[pos]->contains(entt); ++pos) {}
    }

    return pos == len;
}

template<typename Type, typename Entity>
[[nodiscard]] bool none_of(const Type *const *it, const std::size_t len, const Entity entt) noexcept {
    std::size_t pos{};
    for(; (pos != len) && !(it[pos] && it[pos]->contains(entt)); ++pos) {}
    return pos == len;
}

template<typename Type>
[[nodiscard]] bool fully_initialized(const Type *const *it, const std::size_t len) noexcept {
    std::size_t pos{};
    for(; (pos != len) && it[pos]; ++pos) {}
    return pos == len;
}

template<typename Result, typename View, typename Other, std::size_t... VGet, std::size_t... VExclude, std::size_t... OGet, std::size_t... OExclude>
[[nodiscard]] Result view_pack(const View &view, const Other &other, std::index_sequence<VGet...>, std::index_sequence<VExclude...>, std::index_sequence<OGet...>, std::index_sequence<OExclude...>) {
    Result elem{};
    // friend-initialization, avoid multiple calls to refresh
    elem.pools = {view.template storage<VGet>()..., other.template storage<OGet>()...};
    elem.filter = {view.template storage<sizeof...(VGet) + VExclude>()..., other.template storage<sizeof...(OGet) + OExclude>()...};
    elem.refresh();
    return elem;
}

template<typename Type, std::size_t Get, std::size_t Exclude>
class view_iterator final {
    using iterator_type = typename Type::const_iterator;

    [[nodiscard]] bool valid(const typename iterator_type::value_type entt) const noexcept {
        return ((Get != 1u) || (entt != tombstone)) && all_of_but(index, pools.data(), Get, entt) && none_of(filter.data(), Exclude, entt);
    }

public:
    using value_type = typename iterator_type::value_type;
    using pointer = typename iterator_type::pointer;
    using reference = typename iterator_type::reference;
    using difference_type = typename iterator_type::difference_type;
    using iterator_category = std::forward_iterator_tag;

    constexpr view_iterator() noexcept
        : it{},
          last{},
          pools{},
          filter{},
          index{} {}

    view_iterator(iterator_type curr, iterator_type to, std::array<const Type *, Get> value, std::array<const Type *, Exclude> excl, const std::size_t idx) noexcept
        : it{curr},
          last{to},
          pools{value},
          filter{excl},
          index{idx} {
        while(it != last && !valid(*it)) {
            ++it;
        }
    }

    view_iterator &operator++() noexcept {
        while(++it != last && !valid(*it)) {}
        return *this;
    }

    view_iterator operator++(int) noexcept {
        view_iterator orig = *this;
        return ++(*this), orig;
    }

    [[nodiscard]] pointer operator->() const noexcept {
        return &*it;
    }

    [[nodiscard]] reference operator*() const noexcept {
        return *operator->();
    }

    template<typename LhsType, auto... LhsArgs, typename RhsType, auto... RhsArgs>
    friend constexpr bool operator==(const view_iterator<LhsType, LhsArgs...> &, const view_iterator<RhsType, RhsArgs...> &) noexcept;

private:
    iterator_type it;
    iterator_type last;
    std::array<const Type *, Get> pools;
    std::array<const Type *, Exclude> filter;
    std::size_t index;
};

template<typename LhsType, auto... LhsArgs, typename RhsType, auto... RhsArgs>
[[nodiscard]] constexpr bool operator==(const view_iterator<LhsType, LhsArgs...> &lhs, const view_iterator<RhsType, RhsArgs...> &rhs) noexcept {
    return lhs.it == rhs.it;
}

template<typename LhsType, auto... LhsArgs, typename RhsType, auto... RhsArgs>
[[nodiscard]] constexpr bool operator!=(const view_iterator<LhsType, LhsArgs...> &lhs, const view_iterator<RhsType, RhsArgs...> &rhs) noexcept {
    return !(lhs == rhs);
}

template<typename It, typename... Type>
struct extended_view_iterator final {
    using iterator_type = It;
    using difference_type = std::ptrdiff_t;
    using value_type = decltype(std::tuple_cat(std::make_tuple(*std::declval<It>()), std::declval<Type>().get_as_tuple({})...));
    using pointer = input_iterator_pointer<value_type>;
    using reference = value_type;
    using iterator_category = std::input_iterator_tag;
    using iterator_concept = std::forward_iterator_tag;

    constexpr extended_view_iterator()
        : it{},
          pools{} {}

    extended_view_iterator(iterator_type from, std::tuple<Type *...> value)
        : it{from},
          pools{value} {}

    extended_view_iterator &operator++() noexcept {
        return ++it, *this;
    }

    extended_view_iterator operator++(int) noexcept {
        extended_view_iterator orig = *this;
        return ++(*this), orig;
    }

    [[nodiscard]] reference operator*() const noexcept {
        return std::apply([entt = *it](auto *...curr) { return std::tuple_cat(std::make_tuple(entt), curr->get_as_tuple(entt)...); }, pools);
    }

    [[nodiscard]] pointer operator->() const noexcept {
        return operator*();
    }

    [[nodiscard]] constexpr iterator_type base() const noexcept {
        return it;
    }

    template<typename... Lhs, typename... Rhs>
    friend bool constexpr operator==(const extended_view_iterator<Lhs...> &, const extended_view_iterator<Rhs...> &) noexcept;

private:
    It it;
    std::tuple<Type *...> pools;
};

template<typename... Lhs, typename... Rhs>
[[nodiscard]] constexpr bool operator==(const extended_view_iterator<Lhs...> &lhs, const extended_view_iterator<Rhs...> &rhs) noexcept {
    return lhs.it == rhs.it;
}

template<typename... Lhs, typename... Rhs>
[[nodiscard]] constexpr bool operator!=(const extended_view_iterator<Lhs...> &lhs, const extended_view_iterator<Rhs...> &rhs) noexcept {
    return !(lhs == rhs);
}

} // namespace internal
/*! @endcond */

/**
 * @brief View implementation.
 *
 * Primary template isn't defined on purpose. All the specializations give a
 * compile-time error, but for a few reasonable cases.
 *
 * @b Important
 *
 * View iterators aren't invalidated if:
 *
 * * New elements are added to the storage iterated by the view.
 * * The entity currently returned is modified (for example, components are
 *   added or removed from it).
 * * The entity currently returned is destroyed.
 *
 * In all other cases, modifying the storage iterated by a view in any way can
 * invalidate all iterators.
 */
template<typename, typename, typename>
class basic_view;

/**
 * @brief Basic storage view implementation.
 * @warning For internal use only, backward compatibility not guaranteed.
 * @tparam Type Common type among all storage types.
 * @tparam Get Number of storage iterated by the view.
 * @tparam Exclude Number of storage used to filter the view.
 */
template<typename Type, std::size_t Get, std::size_t Exclude>
class basic_common_view {
    template<typename Return, typename View, typename Other, std::size_t... VGet, std::size_t... VExclude, std::size_t... OGet, std::size_t... OExclude>
    friend Return internal::view_pack(const View &, const Other &, std::index_sequence<VGet...>, std::index_sequence<VExclude...>, std::index_sequence<OGet...>, std::index_sequence<OExclude...>);

protected:
    /*! @cond TURN_OFF_DOXYGEN */
    basic_common_view() noexcept = default;

    basic_common_view(std::array<const Type *, Get> value, std::array<const Type *, Exclude> excl) noexcept
        : pools{value},
          filter{excl},
          leading{},
          index{Get} {
        unchecked_refresh();
    }

    void use(const std::size_t pos) noexcept {
        if(leading) {
            index = pos;
            leading = pools[index];
        }
    }

    void unchecked_refresh() noexcept {
        index = 0u;

        if constexpr(Get > 1u) {
            for(size_type pos{1u}; pos < Get; ++pos) {
                if(pools[pos]->size() < pools[index]->size()) {
                    index = pos;
                }
            }
        }

        leading = pools[index];
    }
    /*! @endcond */

public:
    /*! @brief Common type among all storage types. */
    using common_type = Type;
    /*! @brief Underlying entity identifier. */
    using entity_type = typename Type::entity_type;
    /*! @brief Unsigned integer type. */
    using size_type = std::size_t;
    /*! @brief Bidirectional iterator type. */
    using iterator = internal::view_iterator<common_type, Get, Exclude>;

    /*! @brief Updates the internal leading view if required. */
    void refresh() noexcept {
        size_type pos = (leading != nullptr) * Get;
        for(; pos < Get && pools[pos] != nullptr; ++pos) {}

        if(pos == Get) {
            unchecked_refresh();
        }
    }

    /**
     * @brief Returns the leading storage of a view, if any.
     * @return The leading storage of the view.
     */
    [[nodiscard]] const common_type *handle() const noexcept {
        return leading;
    }

    /**
     * @brief Estimates the number of entities iterated by the view.
     * @return Estimated number of entities iterated by the view.
     */
    [[nodiscard]] size_type size_hint() const noexcept {
        return leading ? leading->size() : size_type{};
    }

    /**
     * @brief Returns an iterator to the first entity of the view.
     *
     * If the view is empty, the returned iterator will be equal to `end()`.
     *
     * @return An iterator to the first entity of the view.
     */
    [[nodiscard]] iterator begin() const noexcept {
        return leading ? iterator{leading->begin(0), leading->end(0), pools, filter, index} : iterator{};
    }

    /**
     * @brief Returns an iterator that is past the last entity of the view.
     * @return An iterator to the entity following the last entity of the view.
     */
    [[nodiscard]] iterator end() const noexcept {
        return leading ? iterator{leading->end(0), leading->end(0), pools, filter, index} : iterator{};
    }

    /**
     * @brief Returns the first entity of the view, if any.
     * @return The first entity of the view if one exists, the null entity
     * otherwise.
     */
    [[nodiscard]] entity_type front() const noexcept {
        const auto it = begin();
        return it != end() ? *it : null;
    }

    /**
     * @brief Returns the last entity of the view, if any.
     * @return The last entity of the view if one exists, the null entity
     * otherwise.
     */
    [[nodiscard]] entity_type back() const noexcept {
        if(leading) {
            auto it = leading->rbegin(0);
            const auto last = leading->rend(0);
            for(; it != last && !contains(*it); ++it) {}
            return it == last ? null : *it;
        }

        return null;
    }

    /**
     * @brief Finds an entity.
     * @param entt A valid identifier.
     * @return An iterator to the given entity if it's found, past the end
     * iterator otherwise.
     */
    [[nodiscard]] iterator find(const entity_type entt) const noexcept {
        return contains(entt) ? iterator{leading->find(entt), leading->end(), pools, filter, index} : end();
    }

    /**
     * @brief Checks if a view is fully initialized.
     * @return True if the view is fully initialized, false otherwise.
     */
    [[nodiscard]] explicit operator bool() const noexcept {
        return leading && internal::fully_initialized(filter.data(), Exclude);
    }

    /**
     * @brief Checks if a view contains an entity.
     * @param entt A valid identifier.
     * @return True if the view contains the given entity, false otherwise.
     */
    [[nodiscard]] bool contains(const entity_type entt) const noexcept {
        if(leading) {
            const auto idx = leading->find(entt).index();
            return (!(idx < 0 || idx > leading->begin(0).index())) && internal::all_of_but(index, pools.data(), Get, entt) && internal::none_of(filter.data(), Exclude, entt);
        }

        return false;
    }

protected:
    /*! @cond TURN_OFF_DOXYGEN */
    std::array<const common_type *, Get> pools{};
    std::array<const common_type *, Exclude> filter{};
    const common_type *leading{};
    size_type index{Get};
    /*! @endcond */
};

/**
 * @brief General purpose view.
 *
 * This view visits all entities that are at least in the given storage. During
 * initialization, it also looks at the number of elements available for each
 * storage and uses the smallest set in order to get a performance boost.
 *
 * @sa basic_view
 *
 * @tparam Get Types of storage iterated by the view.
 * @tparam Exclude Types of storage used to filter the view.
 */
template<typename... Get, typename... Exclude>
class basic_view<get_t<Get...>, exclude_t<Exclude...>>: public basic_common_view<std::common_type_t<typename Get::base_type..., typename Exclude::base_type...>, sizeof...(Get), sizeof...(Exclude)> {
    using base_type = basic_common_view<std::common_type_t<typename Get::base_type..., typename Exclude::base_type...>, sizeof...(Get), sizeof...(Exclude)>;

    template<typename Type>
    static constexpr std::size_t index_of = type_list_index_v<std::remove_const_t<Type>, type_list<typename Get::value_type..., typename Exclude::value_type...>>;

    template<std::size_t... Index>
    auto storage(std::index_sequence<Index...>) const noexcept {
        return std::make_tuple(storage<Index>()...);
    }

    template<std::size_t Curr, std::size_t Other, typename... Args>
    [[nodiscard]] auto dispatch_get(const std::tuple<typename base_type::entity_type, Args...> &curr) const {
        if constexpr(Curr == Other) {
            return std::forward_as_tuple(std::get<Args>(curr)...);
        } else {
            return storage<Other>()->get_as_tuple(std::get<0>(curr));
        }
    }

    template<std::size_t Curr, typename Func, std::size_t... Index>
    void each(Func &func, std::index_sequence<Index...>) const {
        for(const auto curr: storage<Curr>()->each()) {
            if(const auto entt = std::get<0>(curr); ((sizeof...(Get) != 1u) || (entt != tombstone)) && internal::all_of_but(this->index, this->pools.data(), sizeof...(Get), entt) && internal::none_of(this->filter.data(), sizeof...(Exclude), entt)) {
                if constexpr(is_applicable_v<Func, decltype(std::tuple_cat(std::tuple<entity_type>{}, std::declval<basic_view>().get({})))>) {
                    std::apply(func, std::tuple_cat(std::make_tuple(entt), dispatch_get<Curr, Index>(curr)...));
                } else {
                    std::apply(func, std::tuple_cat(dispatch_get<Curr, Index>(curr)...));
                }
            }
        }
    }

    template<typename Func, std::size_t... Index>
    void pick_and_each(Func &func, std::index_sequence<Index...> seq) const {
        ((storage<Index>() == base_type::handle() ? each<Index>(func, seq) : void()), ...);
    }

public:
    /*! @brief Common type among all storage types. */
    using common_type = typename base_type::common_type;
    /*! @brief Underlying entity identifier. */
    using entity_type = typename base_type::entity_type;
    /*! @brief Unsigned integer type. */
    using size_type = typename base_type::size_type;
    /*! @brief Bidirectional iterator type. */
    using iterator = typename base_type::iterator;
    /*! @brief Iterable view type. */
    using iterable = iterable_adaptor<internal::extended_view_iterator<iterator, Get...>>;

    /*! @brief Default constructor to use to create empty, invalid views. */
    basic_view() noexcept
        : base_type{} {}

    /**
     * @brief Constructs a view from a set of storage classes.
     * @param value The storage for the types to iterate.
     * @param excl The storage for the types used to filter the view.
     */
    basic_view(Get &...value, Exclude &...excl) noexcept
        : base_type{{&value...}, {&excl...}} {
    }

    /**
     * @brief Constructs a view from a set of storage classes.
     * @param value The storage for the types to iterate.
     * @param excl The storage for the types used to filter the view.
     */
    basic_view(std::tuple<Get &...> value, std::tuple<Exclude &...> excl = {}) noexcept
        : basic_view{std::make_from_tuple<basic_view>(std::tuple_cat(value, excl))} {}

    /**
     * @brief Forces a view to use a given component to drive iterations
     * @tparam Type Type of component to use to drive iterations.
     */
    template<typename Type>
    void use() noexcept {
        use<index_of<Type>>();
    }

    /**
     * @brief Forces a view to use a given component to drive iterations
     * @tparam Index Index of the component to use to drive iterations.
     */
    template<std::size_t Index>
    void use() noexcept {
        base_type::use(Index);
    }

    /**
     * @brief Returns the storage for a given component type, if any.
     * @tparam Type Type of component of which to return the storage.
     * @return The storage for the given component type.
     */
    template<typename Type>
    [[nodiscard]] auto *storage() const noexcept {
        return storage<index_of<Type>>();
    }

    /**
     * @brief Returns the storage for a given index, if any.
     * @tparam Index Index of the storage to return.
     * @return The storage for the given index.
     */
    template<std::size_t Index>
    [[nodiscard]] auto *storage() const noexcept {
        using type = type_list_element_t<Index, type_list<Get..., Exclude...>>;

        if constexpr(Index < sizeof...(Get)) {
            return static_cast<type *>(const_cast<constness_as_t<common_type, type> *>(this->pools[Index]));
        } else {
            return static_cast<type *>(const_cast<constness_as_t<common_type, type> *>(this->filter[Index - sizeof...(Get)]));
        }
    }

    /**
     * @brief Assigns a storage to a view.
     * @tparam Type Type of storage to assign to the view.
     * @param elem A storage to assign to the view.
     */
    template<typename Type>
    void storage(Type &elem) noexcept {
        storage<index_of<typename Type::value_type>>(elem);
    }

    /**
     * @brief Assigns a storage to a view.
     * @tparam Index Index of the storage to assign to the view.
     * @tparam Type Type of storage to assign to the view.
     * @param elem A storage to assign to the view.
     */
    template<std::size_t Index, typename Type>
    void storage(Type &elem) noexcept {
        static_assert(std::is_convertible_v<Type &, type_list_element_t<Index, type_list<Get..., Exclude...>> &>, "Unexpected type");

        if constexpr(Index < sizeof...(Get)) {
            this->pools[Index] = &elem;
            base_type::refresh();
        } else {
            this->filter[Index - sizeof...(Get)] = &elem;
        }
    }

    /**
     * @brief Returns the components assigned to the given entity.
     * @param entt A valid identifier.
     * @return The components assigned to the given entity.
     */
    [[nodiscard]] decltype(auto) operator[](const entity_type entt) const {
        return get(entt);
    }

    /**
     * @brief Returns the components assigned to the given entity.
     * @tparam Type Type of the component to get.
     * @tparam Other Other types of components to get.
     * @param entt A valid identifier.
     * @return The components assigned to the entity.
     */
    template<typename Type, typename... Other>
    [[nodiscard]] decltype(auto) get(const entity_type entt) const {
        return get<index_of<Type>, index_of<Other>...>(entt);
    }

    /**
     * @brief Returns the components assigned to the given entity.
     * @tparam Index Indexes of the components to get.
     * @param entt A valid identifier.
     * @return The components assigned to the entity.
     */
    template<std::size_t... Index>
    [[nodiscard]] decltype(auto) get(const entity_type entt) const {
        if constexpr(sizeof...(Index) == 0) {
            return std::apply([entt](auto *...curr) { return std::tuple_cat(curr->get_as_tuple(entt)...); }, storage(std::index_sequence_for<Get...>{}));
        } else if constexpr(sizeof...(Index) == 1) {
            return (storage<Index>()->get(entt), ...);
        } else {
            return std::tuple_cat(storage<Index>()->get_as_tuple(entt)...);
        }
    }

    /**
     * @brief Iterates entities and components and applies the given function
     * object to them.
     *
     * The signature of the function must be equivalent to one of the following
     * (non-empty types only, constness as requested):
     *
     * @code{.cpp}
     * void(const entity_type, Type &...);
     * void(Type &...);
     * @endcode
     *
     * @tparam Func Type of the function object to invoke.
     * @param func A valid function object.
     */
    template<typename Func>
    void each(Func func) const {
        if(base_type::handle() != nullptr) {
            pick_and_each(func, std::index_sequence_for<Get...>{});
        }
    }

    /**
     * @brief Returns an iterable object to use to _visit_ a view.
     *
     * The iterable object returns a tuple that contains the current entity and
     * a set of references to its non-empty components. The _constness_ of the
     * components is as requested.
     *
     * @return An iterable object to use to _visit_ the view.
     */
    [[nodiscard]] iterable each() const noexcept {
        const auto as_pools = storage(std::index_sequence_for<Get...>{});
        return {internal::extended_view_iterator{base_type::begin(), as_pools}, internal::extended_view_iterator{base_type::end(), as_pools}};
    }

    /**
     * @brief Combines two views in a _more specific_ one.
     * @tparam OGet Component list of the view to combine with.
     * @tparam OExclude Filter list of the view to combine with.
     * @param other The view to combine with.
     * @return A more specific view.
     */
    template<typename... OGet, typename... OExclude>
    [[nodiscard]] auto operator|(const basic_view<get_t<OGet...>, exclude_t<OExclude...>> &other) const noexcept {
        return internal::view_pack<basic_view<get_t<Get..., OGet...>, exclude_t<Exclude..., OExclude...>>>(
            *this, other, std::index_sequence_for<Get...>{}, std::index_sequence_for<Exclude...>{}, std::index_sequence_for<OGet...>{}, std::index_sequence_for<OExclude...>{});
    }
};

/**
 * @brief Basic storage view implementation.
 * @warning For internal use only, backward compatibility not guaranteed.
 * @tparam Type Common type among all storage types.
 */
template<typename Type>
class basic_storage_view {
protected:
    /*! @cond TURN_OFF_DOXYGEN */
    basic_storage_view() noexcept = default;

    basic_storage_view(const Type *value) noexcept
        : leading{value} {}
    /*! @endcond */

public:
    /*! @brief Common type among all storage types. */
    using common_type = Type;
    /*! @brief Underlying entity identifier. */
    using entity_type = typename common_type::entity_type;
    /*! @brief Unsigned integer type. */
    using size_type = std::size_t;
    /*! @brief Random access iterator type. */
    using iterator = typename common_type::iterator;
    /*! @brief Reversed iterator type. */
    using reverse_iterator = typename common_type::reverse_iterator;

    /**
     * @brief Returns the leading storage of a view, if any.
     * @return The leading storage of the view.
     */
    [[nodiscard]] const common_type *handle() const noexcept {
        return leading;
    }

    /**
     * @brief Returns the number of entities that have the given component.
     * @return Number of entities that have the given component.
     */
    [[nodiscard]] size_type size() const noexcept {
        return leading ? leading->size() : size_type{};
    }

    /**
     * @brief Checks whether a view is empty.
     * @return True if the view is empty, false otherwise.
     */
    [[nodiscard]] bool empty() const noexcept {
        return !leading || leading->empty();
    }

    /**
     * @brief Returns an iterator to the first entity of the view.
     *
     * If the view is empty, the returned iterator will be equal to `end()`.
     *
     * @return An iterator to the first entity of the view.
     */
    [[nodiscard]] iterator begin() const noexcept {
        return leading ? leading->begin() : iterator{};
    }

    /**
     * @brief Returns an iterator that is past the last entity of the view.
     * @return An iterator to the entity following the last entity of the view.
     */
    [[nodiscard]] iterator end() const noexcept {
        return leading ? leading->end() : iterator{};
    }

    /**
     * @brief Returns an iterator to the first entity of the reversed view.
     *
     * If the view is empty, the returned iterator will be equal to `rend()`.
     *
     * @return An iterator to the first entity of the reversed view.
     */
    [[nodiscard]] reverse_iterator rbegin() const noexcept {
        return leading ? leading->rbegin() : reverse_iterator{};
    }

    /**
     * @brief Returns an iterator that is past the last entity of the reversed
     * view.
     * @return An iterator to the entity following the last entity of the
     * reversed view.
     */
    [[nodiscard]] reverse_iterator rend() const noexcept {
        return leading ? leading->rend() : reverse_iterator{};
    }

    /**
     * @brief Returns the first entity of the view, if any.
     * @return The first entity of the view if one exists, the null entity
     * otherwise.
     */
    [[nodiscard]] entity_type front() const noexcept {
        return empty() ? null : *leading->begin();
    }

    /**
     * @brief Returns the last entity of the view, if any.
     * @return The last entity of the view if one exists, the null entity
     * otherwise.
     */
    [[nodiscard]] entity_type back() const noexcept {
        return empty() ? null : *leading->rbegin();
    }

    /**
     * @brief Finds an entity.
     * @param entt A valid identifier.
     * @return An iterator to the given entity if it's found, past the end
     * iterator otherwise.
     */
    [[nodiscard]] iterator find(const entity_type entt) const noexcept {
        return leading ? leading->find(entt) : iterator{};
    }

    /**
     * @brief Checks if a view is fully initialized.
     * @return True if the view is fully initialized, false otherwise.
     */
    [[nodiscard]] explicit operator bool() const noexcept {
        return (leading != nullptr);
    }

    /**
     * @brief Checks if a view contains an entity.
     * @param entt A valid identifier.
     * @return True if the view contains the given entity, false otherwise.
     */
    [[nodiscard]] bool contains(const entity_type entt) const noexcept {
        return leading && leading->contains(entt);
    }

protected:
    /*! @cond TURN_OFF_DOXYGEN */
    const common_type *leading{};
    /*! @endcond */
};

/**
 * @brief Storage view specialization.
 *
 * This specialization offers a boost in terms of performance. It can access the
 * underlying data structure directly and avoid superfluous checks.
 *
 * @sa basic_view
 *
 * @tparam Get Type of storage iterated by the view.
 */
template<typename Get>
class basic_view<get_t<Get>, exclude_t<>, std::void_t<std::enable_if_t<!Get::traits_type::in_place_delete>>>: public basic_storage_view<typename Get::base_type> {
    using base_type = basic_storage_view<typename Get::base_type>;

public:
    /*! @brief Common type among all storage types. */
    using common_type = typename base_type::common_type;
    /*! @brief Underlying entity identifier. */
    using entity_type = typename base_type::entity_type;
    /*! @brief Unsigned integer type. */
    using size_type = typename base_type::size_type;
    /*! @brief Random access iterator type. */
    using iterator = typename base_type::iterator;
    /*! @brief Reversed iterator type. */
    using reverse_iterator = typename base_type::reverse_iterator;
    /*! @brief Iterable view type. */
    using iterable = decltype(std::declval<Get>().each());

    /*! @brief Default constructor to use to create empty, invalid views. */
    basic_view() noexcept
        : base_type{} {}

    /**
     * @brief Constructs a view from a storage class.
     * @param value The storage for the type to iterate.
     */
    basic_view(Get &value) noexcept
        : base_type{&value} {
    }

    /**
     * @brief Constructs a view from a storage class.
     * @param value The storage for the type to iterate.
     */
    basic_view(std::tuple<Get &> value, std::tuple<> = {}) noexcept
        : basic_view{std::get<0>(value)} {}

    /**
     * @brief Returns the storage for a given component type, if any.
     * @tparam Type Type of component of which to return the storage.
     * @return The storage for the given component type.
     */
    template<typename Type = typename Get::value_type>
    [[nodiscard]] auto *storage() const noexcept {
        static_assert(std::is_same_v<std::remove_const_t<Type>, typename Get::value_type>, "Invalid component type");
        return storage<0>();
    }

    /**
     * @brief Returns the storage for a given index, if any.
     * @tparam Index Index of the storage to return.
     * @return The storage for the given index.
     */
    template<std::size_t Index>
    [[nodiscard]] auto *storage() const noexcept {
        static_assert(Index == 0u, "Index out of bounds");
        return static_cast<Get *>(const_cast<constness_as_t<common_type, Get> *>(this->leading));
    }

    /**
     * @brief Assigns a storage to a view.
     * @param elem A storage to assign to the view.
     */
    void storage(Get &elem) noexcept {
        storage<0>(elem);
    }

    /**
     * @brief Assigns a storage to a view.
     * @tparam Index Index of the storage to assign to the view.
     * @param elem A storage to assign to the view.
     */
    template<std::size_t Index>
    void storage(Get &elem) noexcept {
        static_assert(Index == 0u, "Index out of bounds");
        this->leading = &elem;
    }

    /**
     * @brief Returns the component assigned to the given entity.
     * @param entt A valid identifier.
     * @return The component assigned to the given entity.
     */
    [[nodiscard]] decltype(auto) operator[](const entity_type entt) const {
        return storage()->get(entt);
    }

    /**
     * @brief Returns the component assigned to the given entity.
     * @tparam Elem Type of the component to get.
     * @param entt A valid identifier.
     * @return The component assigned to the entity.
     */
    template<typename Elem>
    [[nodiscard]] decltype(auto) get(const entity_type entt) const {
        static_assert(std::is_same_v<std::remove_const_t<Elem>, typename Get::value_type>, "Invalid component type");
        return get<0>(entt);
    }

    /**
     * @brief Returns the component assigned to the given entity.
     * @tparam Index Index of the component to get.
     * @param entt A valid identifier.
     * @return The component assigned to the entity.
     */
    template<std::size_t... Index>
    [[nodiscard]] decltype(auto) get(const entity_type entt) const {
        if constexpr(sizeof...(Index) == 0) {
            return storage()->get_as_tuple(entt);
        } else {
            return storage<Index...>()->get(entt);
        }
    }

    /**
     * @brief Iterates entities and components and applies the given function
     * object to them.
     *
     * The signature of the function must be equivalent to one of the following
     * (non-empty types only, constness as requested):
     *
     * @code{.cpp}
     * void(const entity_type, Type &);
     * void(typename Type &);
     * @endcode
     *
     * @tparam Func Type of the function object to invoke.
     * @param func A valid function object.
     */
    template<typename Func>
    void each(Func func) const {
        if(auto *elem = storage(); elem) {
            if constexpr(is_applicable_v<Func, decltype(*elem->each().begin())>) {
                for(const auto pack: elem->each()) {
                    std::apply(func, pack);
                }
            } else if constexpr(std::is_invocable_v<Func, decltype(*elem->begin())>) {
                for(auto &&component: *elem) {
                    func(component);
                }
            } else {
                for(size_type pos = elem->size(); pos; --pos) {
                    func();
                }
            }
        }
    }

    /**
     * @brief Returns an iterable object to use to _visit_ a view.
     *
     * The iterable object returns a tuple that contains the current entity and
     * a reference to its component if it's a non-empty one. The _constness_ of
     * the component is as requested.
     *
     * @return An iterable object to use to _visit_ the view.
     */
    [[nodiscard]] iterable each() const noexcept {
        auto *elem = storage();
        return elem ? elem->each() : iterable{};
    }

    /**
     * @brief Combines two views in a _more specific_ one.
     * @tparam OGet Component list of the view to combine with.
     * @tparam OExclude Filter list of the view to combine with.
     * @param other The view to combine with.
     * @return A more specific view.
     */
    template<typename... OGet, typename... OExclude>
    [[nodiscard]] auto operator|(const basic_view<get_t<OGet...>, exclude_t<OExclude...>> &other) const noexcept {
        return internal::view_pack<basic_view<get_t<Get, OGet...>, exclude_t<OExclude...>>>(
            *this, other, std::index_sequence_for<Get>{}, std::index_sequence_for<>{}, std::index_sequence_for<OGet...>{}, std::index_sequence_for<OExclude...>{});
    }
};

/**
 * @brief Deduction guide.
 * @tparam Type Type of storage classes used to create the view.
 * @param storage The storage for the types to iterate.
 */
template<typename... Type>
basic_view(Type &...storage) -> basic_view<get_t<Type...>, exclude_t<>>;

/**
 * @brief Deduction guide.
 * @tparam Get Types of components iterated by the view.
 * @tparam Exclude Types of components used to filter the view.
 */
template<typename... Get, typename... Exclude>
basic_view(std::tuple<Get &...>, std::tuple<Exclude &...> = {}) -> basic_view<get_t<Get...>, exclude_t<Exclude...>>;

} // namespace entt

#endif

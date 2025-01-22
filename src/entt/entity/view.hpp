#ifndef ENTT_ENTITY_VIEW_HPP
#define ENTT_ENTITY_VIEW_HPP

#include <array>
#include <cstddef>
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

template<typename... Type>
static constexpr bool tombstone_check_v = ((sizeof...(Type) == 1u) && ... && (Type::storage_policy == deletion_policy::in_place));

template<typename Type>
const Type *view_placeholder() {
    static_assert(std::is_same_v<std::remove_const_t<std::remove_reference_t<Type>>, Type>, "Unexpected type");
    static const Type placeholder{};
    return &placeholder;
}

template<typename It, typename Entity>
[[nodiscard]] bool all_of(It first, const It last, const Entity entt) noexcept {
    for(; (first != last) && (*first)->contains(entt); ++first) {}
    return first == last;
}

template<typename It, typename Entity>
[[nodiscard]] bool none_of(It first, const It last, const Entity entt) noexcept {
    for(; (first != last) && !(*first)->contains(entt); ++first) {}
    return first == last;
}

template<typename It>
[[nodiscard]] bool fully_initialized(It first, const It last, const std::remove_pointer_t<typename std::iterator_traits<It>::value_type> *placeholder) noexcept {
    for(; (first != last) && *first != placeholder; ++first) {}
    return first == last;
}

template<typename Result, typename View, typename Other, std::size_t... GLhs, std::size_t... ELhs, std::size_t... GRhs, std::size_t... ERhs>
[[nodiscard]] Result view_pack(const View &view, const Other &other, std::index_sequence<GLhs...>, std::index_sequence<ELhs...>, std::index_sequence<GRhs...>, std::index_sequence<ERhs...>) {
    Result elem{};
    // friend-initialization, avoid multiple calls to refresh
    elem.pools = {view.template storage<GLhs>()..., other.template storage<GRhs>()...};
    elem.filter = {view.template storage<sizeof...(GLhs) + ELhs>()..., other.template storage<sizeof...(GRhs) + ERhs>()...};
    elem.refresh();
    return elem;
}

template<typename Type, bool Checked, std::size_t Get, std::size_t Exclude>
class view_iterator final {
    template<typename, typename...>
    friend class extended_view_iterator;

    using iterator_type = typename Type::const_iterator;
    using iterator_traits = std::iterator_traits<iterator_type>;

    [[nodiscard]] bool valid(const typename iterator_traits::value_type entt) const noexcept {
        return (!Checked || (entt != tombstone))
               && ((Get == 1u) || (internal::all_of(pools.begin(), pools.begin() + index, entt) && internal::all_of(pools.begin() + index + 1, pools.end(), entt)))
               && ((Exclude == 0u) || internal::none_of(filter.begin(), filter.end(), entt));
    }

    void seek_next() {
        for(constexpr iterator_type sentinel{}; it != sentinel && !valid(*it); ++it) {}
    }

public:
    using value_type = typename iterator_traits::value_type;
    using pointer = typename iterator_traits::pointer;
    using reference = typename iterator_traits::reference;
    using difference_type = typename iterator_traits::difference_type;
    using iterator_category = std::forward_iterator_tag;

    constexpr view_iterator() noexcept
        : it{},
          pools{},
          filter{},
          index{} {}

    view_iterator(iterator_type first, std::array<const Type *, Get> value, std::array<const Type *, Exclude> excl, const std::size_t idx) noexcept
        : it{first},
          pools{value},
          filter{excl},
          index{idx} {
        ENTT_ASSERT((Get != 1u) || (Exclude != 0u) || pools[0u]->policy() == deletion_policy::in_place, "Non in-place storage view iterator");
        seek_next();
    }

    view_iterator &operator++() noexcept {
        ++it;
        seek_next();
        return *this;
    }

    view_iterator operator++(int) noexcept {
        const view_iterator orig = *this;
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

template<typename It, typename... Get>
class extended_view_iterator final {
    template<std::size_t... Index>
    [[nodiscard]] auto dereference(std::index_sequence<Index...>) const noexcept {
        return std::tuple_cat(std::make_tuple(*it), static_cast<Get *>(const_cast<constness_as_t<typename Get::base_type, Get> *>(std::get<Index>(it.pools)))->get_as_tuple(*it)...);
    }

public:
    using iterator_type = It;
    using value_type = decltype(std::tuple_cat(std::make_tuple(*std::declval<It>()), std::declval<Get>().get_as_tuple({})...));
    using pointer = input_iterator_pointer<value_type>;
    using reference = value_type;
    using difference_type = std::ptrdiff_t;
    using iterator_category = std::input_iterator_tag;
    using iterator_concept = std::forward_iterator_tag;

    constexpr extended_view_iterator()
        : it{} {}

    extended_view_iterator(iterator_type from)
        : it{from} {}

    extended_view_iterator &operator++() noexcept {
        return ++it, *this;
    }

    extended_view_iterator operator++(int) noexcept {
        const extended_view_iterator orig = *this;
        return ++(*this), orig;
    }

    [[nodiscard]] reference operator*() const noexcept {
        return dereference(std::index_sequence_for<Get...>{});
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
 * * The entity currently returned is modified (for example, elements are added
 *   or removed from it).
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
 * @tparam Checked True to enable the tombstone check, false otherwise.
 * @tparam Get Number of storage iterated by the view.
 * @tparam Exclude Number of storage used to filter the view.
 */
template<typename Type, bool Checked, std::size_t Get, std::size_t Exclude>
class basic_common_view {
    static_assert(std::is_same_v<std::remove_const_t<std::remove_reference_t<Type>>, Type>, "Unexpected type");

    template<typename Return, typename View, typename Other, std::size_t... GLhs, std::size_t... ELhs, std::size_t... GRhs, std::size_t... ERhs>
    friend Return internal::view_pack(const View &, const Other &, std::index_sequence<GLhs...>, std::index_sequence<ELhs...>, std::index_sequence<GRhs...>, std::index_sequence<ERhs...>);

    [[nodiscard]] auto offset() const noexcept {
        ENTT_ASSERT(index != Get, "Invalid view");
        return (pools[index]->policy() == deletion_policy::swap_only) ? pools[index]->free_list() : pools[index]->size();
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
    }

protected:
    /*! @cond TURN_OFF_DOXYGEN */
    basic_common_view() noexcept {
        for(size_type pos{}; pos < Exclude; ++pos) {
            filter[pos] = placeholder;
        }
    }

    basic_common_view(std::array<const Type *, Get> value, std::array<const Type *, Exclude> excl) noexcept
        : pools{value},
          filter{excl},
          index{Get} {
        unchecked_refresh();
    }

    [[nodiscard]] const Type *pool_at(const std::size_t pos) const noexcept {
        return pools[pos];
    }

    void pool_at(const std::size_t pos, const Type *elem) noexcept {
        ENTT_ASSERT(elem != nullptr, "Unexpected element");
        pools[pos] = elem;
        refresh();
    }

    [[nodiscard]] const Type *filter_at(const std::size_t pos) const noexcept {
        return (filter[pos] == placeholder) ? nullptr : filter[pos];
    }

    void filter_at(const std::size_t pos, const Type *elem) noexcept {
        ENTT_ASSERT(elem != nullptr, "Unexpected element");
        filter[pos] = elem;
    }

    [[nodiscard]] bool none_of(const typename Type::entity_type entt) const noexcept {
        return internal::none_of(filter.begin(), filter.end(), entt);
    }

    void use(const std::size_t pos) noexcept {
        index = (index != Get) ? pos : Get;
    }
    /*! @endcond */

public:
    /*! @brief Common type among all storage types. */
    using common_type = Type;
    /*! @brief Underlying entity identifier. */
    using entity_type = typename Type::entity_type;
    /*! @brief Unsigned integer type. */
    using size_type = std::size_t;
    /*! @brief Forward iterator type. */
    using iterator = internal::view_iterator<common_type, Checked, Get, Exclude>;

    /*! @brief Updates the internal leading view if required. */
    void refresh() noexcept {
        size_type pos = static_cast<size_type>(index != Get) * Get;
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
        return (index != Get) ? pools[index] : nullptr;
    }

    /**
     * @brief Estimates the number of entities iterated by the view.
     * @return Estimated number of entities iterated by the view.
     */
    [[nodiscard]] size_type size_hint() const noexcept {
        return (index != Get) ? offset() : size_type{};
    }

    /**
     * @brief Returns an iterator to the first entity of the view.
     *
     * If the view is empty, the returned iterator will be equal to `end()`.
     *
     * @return An iterator to the first entity of the view.
     */
    [[nodiscard]] iterator begin() const noexcept {
        return (index != Get) ? iterator{pools[index]->end() - static_cast<typename iterator::difference_type>(offset()), pools, filter, index} : iterator{};
    }

    /**
     * @brief Returns an iterator that is past the last entity of the view.
     * @return An iterator to the entity following the last entity of the view.
     */
    [[nodiscard]] iterator end() const noexcept {
        return (index != Get) ? iterator{pools[index]->end(), pools, filter, index} : iterator{};
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
        if(index != Get) {
            auto it = pools[index]->rbegin();
            const auto last = it + static_cast<typename iterator::difference_type>(offset());
            for(; it != last && !(internal::all_of(pools.begin(), pools.begin() + index, *it) && internal::all_of(pools.begin() + index + 1, pools.end(), *it) && internal::none_of(filter.begin(), filter.end(), *it)); ++it) {}
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
        return contains(entt) ? iterator{pools[index]->find(entt), pools, filter, index} : end();
    }

    /**
     * @brief Checks if a view is fully initialized.
     * @return True if the view is fully initialized, false otherwise.
     */
    [[nodiscard]] explicit operator bool() const noexcept {
        return (index != Get) && internal::fully_initialized(filter.begin(), filter.end(), placeholder);
    }

    /**
     * @brief Checks if a view contains an entity.
     * @param entt A valid identifier.
     * @return True if the view contains the given entity, false otherwise.
     */
    [[nodiscard]] bool contains(const entity_type entt) const noexcept {
        return (index != Get)
               && internal::all_of(pools.begin(), pools.end(), entt)
               && internal::none_of(filter.begin(), filter.end(), entt)
               && pools[index]->index(entt) < offset();
    }

private:
    std::array<const common_type *, Get> pools{};
    std::array<const common_type *, Exclude> filter{};
    const common_type *placeholder{internal::view_placeholder<common_type>()};
    size_type index{Get};
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
class basic_view<get_t<Get...>, exclude_t<Exclude...>, std::enable_if_t<(sizeof...(Get) != 0u)>>
    : public basic_common_view<std::common_type_t<typename Get::base_type...>, internal::tombstone_check_v<Get...>, sizeof...(Get), sizeof...(Exclude)> {
    using base_type = basic_common_view<std::common_type_t<typename Get::base_type...>, internal::tombstone_check_v<Get...>, sizeof...(Get), sizeof...(Exclude)>;

    template<std::size_t Index>
    using element_at = type_list_element_t<Index, type_list<Get..., Exclude...>>;

    template<typename Type>
    static constexpr std::size_t index_of = type_list_index_v<std::remove_const_t<Type>, type_list<typename Get::element_type..., typename Exclude::element_type...>>;

    template<std::size_t... Index>
    [[nodiscard]] auto get(const typename base_type::entity_type entt, std::index_sequence<Index...>) const noexcept {
        return std::tuple_cat(storage<Index>()->get_as_tuple(entt)...);
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
            if(const auto entt = std::get<0>(curr); (!internal::tombstone_check_v<Get...> || (entt != tombstone)) && ((Curr == Index || base_type::pool_at(Index)->contains(entt)) && ...) && base_type::none_of(entt)) {
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
        if(const auto *view = base_type::handle(); view != nullptr) {
            ((view == base_type::pool_at(Index) ? each<Index>(func, seq) : void()), ...);
        }
    }

public:
    /*! @brief Common type among all storage types. */
    using common_type = typename base_type::common_type;
    /*! @brief Underlying entity identifier. */
    using entity_type = typename base_type::entity_type;
    /*! @brief Unsigned integer type. */
    using size_type = typename base_type::size_type;
    /*! @brief Forward iterator type. */
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
     * @brief Forces a view to use a given element to drive iterations
     * @tparam Type Type of element to use to drive iterations.
     */
    template<typename Type>
    void use() noexcept {
        use<index_of<Type>>();
    }

    /**
     * @brief Forces a view to use a given element to drive iterations
     * @tparam Index Index of the element to use to drive iterations.
     */
    template<std::size_t Index>
    void use() noexcept {
        base_type::use(Index);
    }

    /**
     * @brief Returns the storage for a given element type, if any.
     * @tparam Type Type of element of which to return the storage.
     * @return The storage for the given element type.
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
        if constexpr(Index < sizeof...(Get)) {
            return static_cast<element_at<Index> *>(const_cast<constness_as_t<common_type, element_at<Index>> *>(base_type::pool_at(Index)));
        } else {
            return static_cast<element_at<Index> *>(const_cast<constness_as_t<common_type, element_at<Index>> *>(base_type::filter_at(Index - sizeof...(Get))));
        }
    }

    /**
     * @brief Assigns a storage to a view.
     * @tparam Type Type of storage to assign to the view.
     * @param elem A storage to assign to the view.
     */
    template<typename Type>
    void storage(Type &elem) noexcept {
        storage<index_of<typename Type::element_type>>(elem);
    }

    /**
     * @brief Assigns a storage to a view.
     * @tparam Index Index of the storage to assign to the view.
     * @tparam Type Type of storage to assign to the view.
     * @param elem A storage to assign to the view.
     */
    template<std::size_t Index, typename Type>
    void storage(Type &elem) noexcept {
        static_assert(std::is_convertible_v<Type &, element_at<Index> &>, "Unexpected type");

        if constexpr(Index < sizeof...(Get)) {
            base_type::pool_at(Index, &elem);
        } else {
            base_type::filter_at(Index - sizeof...(Get), &elem);
        }
    }

    /**
     * @brief Returns the elements assigned to the given entity.
     * @param entt A valid identifier.
     * @return The elements assigned to the given entity.
     */
    [[nodiscard]] decltype(auto) operator[](const entity_type entt) const {
        return get(entt);
    }

    /**
     * @brief Returns the elements assigned to the given entity.
     * @tparam Type Type of the element to get.
     * @tparam Other Other types of elements to get.
     * @param entt A valid identifier.
     * @return The elements assigned to the entity.
     */
    template<typename Type, typename... Other>
    [[nodiscard]] decltype(auto) get(const entity_type entt) const {
        return get<index_of<Type>, index_of<Other>...>(entt);
    }

    /**
     * @brief Returns the elements assigned to the given entity.
     * @tparam Index Indexes of the elements to get.
     * @param entt A valid identifier.
     * @return The elements assigned to the entity.
     */
    template<std::size_t... Index>
    [[nodiscard]] decltype(auto) get(const entity_type entt) const {
        if constexpr(sizeof...(Index) == 0) {
            return get(entt, std::index_sequence_for<Get...>{});
        } else if constexpr(sizeof...(Index) == 1) {
            return (storage<Index>()->get(entt), ...);
        } else {
            return std::tuple_cat(storage<Index>()->get_as_tuple(entt)...);
        }
    }

    /**
     * @brief Iterates entities and elements and applies the given function
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
        pick_and_each(func, std::index_sequence_for<Get...>{});
    }

    /**
     * @brief Returns an iterable object to use to _visit_ a view.
     *
     * The iterable object returns a tuple that contains the current entity and
     * a set of references to its non-empty elements. The _constness_ of the
     * elements is as requested.
     *
     * @return An iterable object to use to _visit_ the view.
     */
    [[nodiscard]] iterable each() const noexcept {
        return iterable{base_type::begin(), base_type::end()};
    }

    /**
     * @brief Combines a view and a storage in _more specific_ view.
     * @tparam OGet Type of storage to combine the view with.
     * @param other The storage for the type to combine the view with.
     * @return A more specific view.
     */
    template<typename OGet>
    [[nodiscard]] std::enable_if_t<std::is_base_of_v<common_type, OGet>, basic_view<get_t<Get..., OGet>, exclude_t<Exclude...>>> operator|(OGet &other) const noexcept {
        return *this | basic_view<get_t<OGet>, exclude_t<>>{other};
    }

    /**
     * @brief Combines two views in a _more specific_ one.
     * @tparam OGet Element list of the view to combine with.
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
 * @tparam Policy Storage policy.
 */
template<typename Type, deletion_policy Policy>
class basic_storage_view {
    static_assert(std::is_same_v<std::remove_const_t<std::remove_reference_t<Type>>, Type>, "Unexpected type");

protected:
    /*! @cond TURN_OFF_DOXYGEN */
    basic_storage_view() noexcept = default;

    basic_storage_view(const Type *value) noexcept
        : leading{value} {
        ENTT_ASSERT(leading->policy() == Policy, "Unexpected storage policy");
    }
    /*! @endcond */

public:
    /*! @brief Common type among all storage types. */
    using common_type = Type;
    /*! @brief Underlying entity identifier. */
    using entity_type = typename common_type::entity_type;
    /*! @brief Unsigned integer type. */
    using size_type = std::size_t;
    /*! @brief Random access iterator type. */
    using iterator = std::conditional_t<Policy == deletion_policy::in_place, internal::view_iterator<common_type, true, 1u, 0u>, typename common_type::iterator>;
    /*! @brief Reverse iterator type. */
    using reverse_iterator = std::conditional_t<Policy == deletion_policy::in_place, void, typename common_type::reverse_iterator>;

    /**
     * @brief Returns the leading storage of a view, if any.
     * @return The leading storage of the view.
     */
    [[nodiscard]] const common_type *handle() const noexcept {
        return leading;
    }

    /**
     * @brief Returns the number of entities that have the given element.
     * @tparam Pol Dummy template parameter used for sfinae purposes only.
     * @return Number of entities that have the given element.
     */
    template<typename..., deletion_policy Pol = Policy>
    [[nodiscard]] std::enable_if_t<Pol != deletion_policy::in_place, size_type> size() const noexcept {
        if constexpr(Policy == deletion_policy::swap_and_pop) {
            return leading ? leading->size() : size_type{};
        } else {
            static_assert(Policy == deletion_policy::swap_only, "Unexpected storage policy");
            return leading ? leading->free_list() : size_type{};
        }
    }

    /**
     * @brief Estimates the number of entities iterated by the view.
     * @tparam Pol Dummy template parameter used for sfinae purposes only.
     * @return Estimated number of entities iterated by the view.
     */
    template<typename..., deletion_policy Pol = Policy>
    [[nodiscard]] std::enable_if_t<Pol == deletion_policy::in_place, size_type> size_hint() const noexcept {
        return leading ? leading->size() : size_type{};
    }

    /**
     * @brief Checks whether a view is empty.
     * @tparam Pol Dummy template parameter used for sfinae purposes only.
     * @return True if the view is empty, false otherwise.
     */
    template<typename..., deletion_policy Pol = Policy>
    [[nodiscard]] std::enable_if_t<Pol != deletion_policy::in_place, bool> empty() const noexcept {
        if constexpr(Policy == deletion_policy::swap_and_pop) {
            return !leading || leading->empty();
        } else {
            static_assert(Policy == deletion_policy::swap_only, "Unexpected storage policy");
            return !leading || (leading->free_list() == 0u);
        }
    }

    /**
     * @brief Returns an iterator to the first entity of the view.
     *
     * If the view is empty, the returned iterator will be equal to `end()`.
     *
     * @return An iterator to the first entity of the view.
     */
    [[nodiscard]] iterator begin() const noexcept {
        if constexpr(Policy == deletion_policy::swap_and_pop) {
            return leading ? leading->begin() : iterator{};
        } else if constexpr(Policy == deletion_policy::swap_only) {
            return leading ? (leading->end() - leading->free_list()) : iterator{};
        } else {
            static_assert(Policy == deletion_policy::in_place, "Unexpected storage policy");
            return leading ? iterator{leading->begin(), {leading}, {}, 0u} : iterator{};
        }
    }

    /**
     * @brief Returns an iterator that is past the last entity of the view.
     * @return An iterator to the entity following the last entity of the view.
     */
    [[nodiscard]] iterator end() const noexcept {
        if constexpr(Policy == deletion_policy::swap_and_pop || Policy == deletion_policy::swap_only) {
            return leading ? leading->end() : iterator{};
        } else {
            static_assert(Policy == deletion_policy::in_place, "Unexpected storage policy");
            return leading ? iterator{leading->end(), {leading}, {}, 0u} : iterator{};
        }
    }

    /**
     * @brief Returns an iterator to the first entity of the reversed view.
     *
     * If the view is empty, the returned iterator will be equal to `rend()`.
     *
     * @tparam Pol Dummy template parameter used for sfinae purposes only.
     * @return An iterator to the first entity of the reversed view.
     */
    template<typename..., deletion_policy Pol = Policy>
    [[nodiscard]] std::enable_if_t<Pol != deletion_policy::in_place, reverse_iterator> rbegin() const noexcept {
        return leading ? leading->rbegin() : reverse_iterator{};
    }

    /**
     * @brief Returns an iterator that is past the last entity of the reversed
     * view.
     * @tparam Pol Dummy template parameter used for sfinae purposes only.
     * @return An iterator to the entity following the last entity of the
     * reversed view.
     */
    template<typename..., deletion_policy Pol = Policy>
    [[nodiscard]] std::enable_if_t<Pol != deletion_policy::in_place, reverse_iterator> rend() const noexcept {
        if constexpr(Policy == deletion_policy::swap_and_pop) {
            return leading ? leading->rend() : reverse_iterator{};
        } else {
            static_assert(Policy == deletion_policy::swap_only, "Unexpected storage policy");
            return leading ? (leading->rbegin() + leading->free_list()) : reverse_iterator{};
        }
    }

    /**
     * @brief Returns the first entity of the view, if any.
     * @return The first entity of the view if one exists, the null entity
     * otherwise.
     */
    [[nodiscard]] entity_type front() const noexcept {
        if constexpr(Policy == deletion_policy::swap_and_pop) {
            return empty() ? null : *leading->begin();
        } else if constexpr(Policy == deletion_policy::swap_only) {
            return empty() ? null : *(leading->end() - leading->free_list());
        } else {
            static_assert(Policy == deletion_policy::in_place, "Unexpected storage policy");
            const auto it = begin();
            return (it == end()) ? null : *it;
        }
    }

    /**
     * @brief Returns the last entity of the view, if any.
     * @return The last entity of the view if one exists, the null entity
     * otherwise.
     */
    [[nodiscard]] entity_type back() const noexcept {
        if constexpr(Policy == deletion_policy::swap_and_pop || Policy == deletion_policy::swap_only) {
            return empty() ? null : *leading->rbegin();
        } else {
            static_assert(Policy == deletion_policy::in_place, "Unexpected storage policy");

            if(leading) {
                auto it = leading->rbegin();
                const auto last = leading->rend();
                for(; (it != last) && (*it == tombstone); ++it) {}
                return it == last ? null : *it;
            }

            return null;
        }
    }

    /**
     * @brief Finds an entity.
     * @param entt A valid identifier.
     * @return An iterator to the given entity if it's found, past the end
     * iterator otherwise.
     */
    [[nodiscard]] iterator find(const entity_type entt) const noexcept {
        if constexpr(Policy == deletion_policy::swap_and_pop) {
            return leading ? leading->find(entt) : iterator{};
        } else if constexpr(Policy == deletion_policy::swap_only) {
            const auto it = leading ? leading->find(entt) : iterator{};
            return leading && (static_cast<size_type>(it.index()) < leading->free_list()) ? it : iterator{};
        } else {
            return leading ? iterator{leading->find(entt), {leading}, {}, 0u} : iterator{};
        }
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
        if constexpr(Policy == deletion_policy::swap_and_pop || Policy == deletion_policy::in_place) {
            return leading && leading->contains(entt);
        } else {
            static_assert(Policy == deletion_policy::swap_only, "Unexpected storage policy");
            return leading && leading->contains(entt) && (leading->index(entt) < leading->free_list());
        }
    }

private:
    const common_type *leading{};
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
class basic_view<get_t<Get>, exclude_t<>>
    : public basic_storage_view<typename Get::base_type, Get::storage_policy> {
    using base_type = basic_storage_view<typename Get::base_type, Get::storage_policy>;

public:
    /*! @brief Common type among all storage types. */
    using common_type = typename base_type::common_type;
    /*! @brief Underlying entity identifier. */
    using entity_type = typename base_type::entity_type;
    /*! @brief Unsigned integer type. */
    using size_type = typename base_type::size_type;
    /*! @brief Random access iterator type. */
    using iterator = typename base_type::iterator;
    /*! @brief Reverse iterator type. */
    using reverse_iterator = typename base_type::reverse_iterator;
    /*! @brief Iterable view type. */
    using iterable = std::conditional_t<Get::storage_policy == deletion_policy::in_place, iterable_adaptor<internal::extended_view_iterator<iterator, Get>>, decltype(std::declval<Get>().each())>;

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
     * @brief Returns the storage for a given element type, if any.
     * @tparam Type Type of element of which to return the storage.
     * @return The storage for the given element type.
     */
    template<typename Type = typename Get::element_type>
    [[nodiscard]] auto *storage() const noexcept {
        static_assert(std::is_same_v<std::remove_const_t<Type>, typename Get::element_type>, "Invalid element type");
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
        return static_cast<Get *>(const_cast<constness_as_t<common_type, Get> *>(base_type::handle()));
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
        *this = basic_view{elem};
    }

    /**
     * @brief Returns a pointer to the underlying storage.
     * @return A pointer to the underlying storage.
     */
    [[nodiscard]] Get *operator->() const noexcept {
        return storage();
    }

    /**
     * @brief Returns the element assigned to the given entity.
     * @param entt A valid identifier.
     * @return The element assigned to the given entity.
     */
    [[nodiscard]] decltype(auto) operator[](const entity_type entt) const {
        return storage()->get(entt);
    }

    /**
     * @brief Returns the element assigned to the given entity.
     * @tparam Elem Type of the element to get.
     * @param entt A valid identifier.
     * @return The element assigned to the entity.
     */
    template<typename Elem>
    [[nodiscard]] decltype(auto) get(const entity_type entt) const {
        static_assert(std::is_same_v<std::remove_const_t<Elem>, typename Get::element_type>, "Invalid element type");
        return get<0>(entt);
    }

    /**
     * @brief Returns the element assigned to the given entity.
     * @tparam Index Index of the element to get.
     * @param entt A valid identifier.
     * @return The element assigned to the entity.
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
     * @brief Iterates entities and elements and applies the given function
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
        if constexpr(is_applicable_v<Func, decltype(std::tuple_cat(std::tuple<entity_type>{}, std::declval<basic_view>().get({})))>) {
            for(const auto pack: each()) {
                std::apply(func, pack);
            }
        } else if constexpr(Get::storage_policy == deletion_policy::swap_and_pop || Get::storage_policy == deletion_policy::swap_only) {
            if constexpr(std::is_void_v<typename Get::value_type>) {
                for(size_type pos = base_type::size(); pos; --pos) {
                    func();
                }
            } else {
                if(const auto len = base_type::size(); len != 0u) {
                    for(auto last = storage()->end(), first = last - len; first != last; ++first) {
                        func(*first);
                    }
                }
            }
        } else {
            static_assert(Get::storage_policy == deletion_policy::in_place, "Unexpected storage policy");

            for(const auto pack: each()) {
                std::apply([&func](const auto, auto &&...elem) { func(std::forward<decltype(elem)>(elem)...); }, pack);
            }
        }
    }

    /**
     * @brief Returns an iterable object to use to _visit_ a view.
     *
     * The iterable object returns a tuple that contains the current entity and
     * a reference to its element if it's a non-empty one. The _constness_ of
     * the element is as requested.
     *
     * @return An iterable object to use to _visit_ the view.
     */
    [[nodiscard]] iterable each() const noexcept {
        if constexpr(Get::storage_policy == deletion_policy::swap_and_pop || Get::storage_policy == deletion_policy::swap_only) {
            return base_type::handle() ? storage()->each() : iterable{};
        } else {
            static_assert(Get::storage_policy == deletion_policy::in_place, "Unexpected storage policy");
            return iterable{base_type::begin(), base_type::end()};
        }
    }

    /**
     * @brief Combines a view and a storage in _more specific_ view.
     * @tparam OGet Type of storage to combine the view with.
     * @param other The storage for the type to combine the view with.
     * @return A more specific view.
     */
    template<typename OGet>
    [[nodiscard]] std::enable_if_t<std::is_base_of_v<common_type, OGet>, basic_view<get_t<Get, OGet>, exclude_t<>>> operator|(OGet &other) const noexcept {
        return *this | basic_view<get_t<OGet>, exclude_t<>>{other};
    }

    /**
     * @brief Combines two views in a _more specific_ one.
     * @tparam OGet Element list of the view to combine with.
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
 * @tparam Get Types of elements iterated by the view.
 * @tparam Exclude Types of elements used to filter the view.
 */
template<typename... Get, typename... Exclude>
basic_view(std::tuple<Get &...>, std::tuple<Exclude &...> = {}) -> basic_view<get_t<Get...>, exclude_t<Exclude...>>;

} // namespace entt

#endif

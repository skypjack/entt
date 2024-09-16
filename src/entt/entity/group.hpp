#ifndef ENTT_ENTITY_GROUP_HPP
#define ENTT_ENTITY_GROUP_HPP

#include <array>
#include <cstddef>
#include <iterator>
#include <tuple>
#include <type_traits>
#include <utility>
#include "../config/config.h"
#include "../core/algorithm.hpp"
#include "../core/fwd.hpp"
#include "../core/iterator.hpp"
#include "../core/type_info.hpp"
#include "../core/type_traits.hpp"
#include "entity.hpp"
#include "fwd.hpp"

namespace entt {

/*! @cond TURN_OFF_DOXYGEN */
namespace internal {

template<typename, typename, typename>
class extended_group_iterator;

template<typename It, typename... Owned, typename... Get>
class extended_group_iterator<It, owned_t<Owned...>, get_t<Get...>> {
    template<typename Type>
    [[nodiscard]] auto index_to_element([[maybe_unused]] Type &cpool) const {
        if constexpr(std::is_void_v<typename Type::value_type>) {
            return std::make_tuple();
        } else {
            return std::forward_as_tuple(cpool.rbegin()[it.index()]);
        }
    }

public:
    using iterator_type = It;
    using value_type = decltype(std::tuple_cat(std::make_tuple(*std::declval<It>()), std::declval<Owned>().get_as_tuple({})..., std::declval<Get>().get_as_tuple({})...));
    using pointer = input_iterator_pointer<value_type>;
    using reference = value_type;
    using difference_type = std::ptrdiff_t;
    using iterator_category = std::input_iterator_tag;
    using iterator_concept = std::forward_iterator_tag;

    constexpr extended_group_iterator()
        : it{},
          pools{} {}

    extended_group_iterator(iterator_type from, std::tuple<Owned *..., Get *...> cpools)
        : it{from},
          pools{std::move(cpools)} {}

    extended_group_iterator &operator++() noexcept {
        return ++it, *this;
    }

    extended_group_iterator operator++(int) noexcept {
        extended_group_iterator orig = *this;
        return ++(*this), orig;
    }

    [[nodiscard]] reference operator*() const noexcept {
        return std::tuple_cat(std::make_tuple(*it), index_to_element(*std::get<Owned *>(pools))..., std::get<Get *>(pools)->get_as_tuple(*it)...);
    }

    [[nodiscard]] pointer operator->() const noexcept {
        return operator*();
    }

    [[nodiscard]] constexpr iterator_type base() const noexcept {
        return it;
    }

    template<typename... Lhs, typename... Rhs>
    friend constexpr bool operator==(const extended_group_iterator<Lhs...> &, const extended_group_iterator<Rhs...> &) noexcept;

private:
    It it;
    std::tuple<Owned *..., Get *...> pools;
};

template<typename... Lhs, typename... Rhs>
[[nodiscard]] constexpr bool operator==(const extended_group_iterator<Lhs...> &lhs, const extended_group_iterator<Rhs...> &rhs) noexcept {
    return lhs.it == rhs.it;
}

template<typename... Lhs, typename... Rhs>
[[nodiscard]] constexpr bool operator!=(const extended_group_iterator<Lhs...> &lhs, const extended_group_iterator<Rhs...> &rhs) noexcept {
    return !(lhs == rhs);
}

struct group_descriptor {
    using size_type = std::size_t;
    virtual ~group_descriptor() = default;
    [[nodiscard]] virtual bool owned(const id_type) const noexcept {
        return false;
    }
};

template<typename Type, std::size_t Owned, std::size_t Get, std::size_t Exclude>
class group_handler final: public group_descriptor {
    using entity_type = typename Type::entity_type;

    void swap_elements(const std::size_t pos, const entity_type entt) {
        for(size_type next{}; next < Owned; ++next) {
            pools[next]->swap_elements((*pools[next])[pos], entt);
        }
    }

    void push_on_construct(const entity_type entt) {
        if(std::apply([entt, pos = len](auto *cpool, auto *...other) { return cpool->contains(entt) && !(cpool->index(entt) < pos) && (other->contains(entt) && ...); }, pools)
           && std::apply([entt](auto *...cpool) { return (!cpool->contains(entt) && ...); }, filter)) {
            swap_elements(len++, entt);
        }
    }

    void push_on_destroy(const entity_type entt) {
        if(std::apply([entt, pos = len](auto *cpool, auto *...other) { return cpool->contains(entt) && !(cpool->index(entt) < pos) && (other->contains(entt) && ...); }, pools)
           && std::apply([entt](auto *...cpool) { return (0u + ... + cpool->contains(entt)) == 1u; }, filter)) {
            swap_elements(len++, entt);
        }
    }

    void remove_if(const entity_type entt) {
        if(pools[0u]->contains(entt) && (pools[0u]->index(entt) < len)) {
            swap_elements(--len, entt);
        }
    }

    void common_setup() {
        // we cannot iterate backwards because we want to leave behind valid entities in case of owned types
        for(auto first = pools[0u]->rbegin(), last = first + pools[0u]->size(); first != last; ++first) {
            push_on_construct(*first);
        }
    }

public:
    using common_type = Type;
    using size_type = typename Type::size_type;

    template<typename... OGType, typename... EType>
    group_handler(std::tuple<OGType &...> ogpool, std::tuple<EType &...> epool)
        : pools{std::apply([](auto &&...cpool) { return std::array<common_type *, (Owned + Get)>{&cpool...}; }, ogpool)},
          filter{std::apply([](auto &&...cpool) { return std::array<common_type *, Exclude>{&cpool...}; }, epool)} {
        std::apply([this](auto &...cpool) { ((cpool.on_construct().template connect<&group_handler::push_on_construct>(*this), cpool.on_destroy().template connect<&group_handler::remove_if>(*this)), ...); }, ogpool);
        std::apply([this](auto &...cpool) { ((cpool.on_construct().template connect<&group_handler::remove_if>(*this), cpool.on_destroy().template connect<&group_handler::push_on_destroy>(*this)), ...); }, epool);
        common_setup();
    }

    [[nodiscard]] bool owned(const id_type hash) const noexcept override {
        for(size_type pos{}; pos < Owned; ++pos) {
            if(pools[pos]->type().hash() == hash) {
                return true;
            }
        }

        return false;
    }

    [[nodiscard]] size_type length() const noexcept {
        return len;
    }

    template<std::size_t Index>
    [[nodiscard]] common_type *storage() const noexcept {
        if constexpr(Index < (Owned + Get)) {
            return pools[Index];
        } else {
            return filter[Index - (Owned + Get)];
        }
    }

private:
    std::array<common_type *, (Owned + Get)> pools;
    std::array<common_type *, Exclude> filter;
    std::size_t len{};
};

template<typename Type, std::size_t Get, std::size_t Exclude>
class group_handler<Type, 0u, Get, Exclude> final: public group_descriptor {
    using entity_type = typename Type::entity_type;

    void push_on_construct(const entity_type entt) {
        if(!elem.contains(entt)
           && std::apply([entt](auto *...cpool) { return (cpool->contains(entt) && ...); }, pools)
           && std::apply([entt](auto *...cpool) { return (!cpool->contains(entt) && ...); }, filter)) {
            elem.push(entt);
        }
    }

    void push_on_destroy(const entity_type entt) {
        if(!elem.contains(entt)
           && std::apply([entt](auto *...cpool) { return (cpool->contains(entt) && ...); }, pools)
           && std::apply([entt](auto *...cpool) { return (0u + ... + cpool->contains(entt)) == 1u; }, filter)) {
            elem.push(entt);
        }
    }

    void remove_if(const entity_type entt) {
        elem.remove(entt);
    }

    void common_setup() {
        for(const auto entity: *pools[0u]) {
            push_on_construct(entity);
        }
    }

public:
    using common_type = Type;

    template<typename Allocator, typename... GType, typename... EType>
    group_handler(const Allocator &allocator, std::tuple<GType &...> gpool, std::tuple<EType &...> epool)
        : pools{std::apply([](auto &&...cpool) { return std::array<common_type *, Get>{&cpool...}; }, gpool)},
          filter{std::apply([](auto &&...cpool) { return std::array<common_type *, Exclude>{&cpool...}; }, epool)},
          elem{allocator} {
        std::apply([this](auto &...cpool) { ((cpool.on_construct().template connect<&group_handler::push_on_construct>(*this), cpool.on_destroy().template connect<&group_handler::remove_if>(*this)), ...); }, gpool);
        std::apply([this](auto &...cpool) { ((cpool.on_construct().template connect<&group_handler::remove_if>(*this), cpool.on_destroy().template connect<&group_handler::push_on_destroy>(*this)), ...); }, epool);
        common_setup();
    }

    [[nodiscard]] common_type &handle() noexcept {
        return elem;
    }

    [[nodiscard]] const common_type &handle() const noexcept {
        return elem;
    }

    template<std::size_t Index>
    [[nodiscard]] common_type *storage() const noexcept {
        if constexpr(Index < Get) {
            return pools[Index];
        } else {
            return filter[Index - Get];
        }
    }

private:
    std::array<common_type *, Get> pools;
    std::array<common_type *, Exclude> filter;
    common_type elem;
};

} // namespace internal
/*! @endcond */

/**
 * @brief Group.
 *
 * Primary template isn't defined on purpose. All the specializations give a
 * compile-time error, but for a few reasonable cases.
 */
template<typename, typename, typename>
class basic_group;

/**
 * @brief Non-owning group.
 *
 * A non-owning group returns all entities and only the entities that are at
 * least in the given storage. Moreover, it's guaranteed that the entity list is
 * tightly packed in memory for fast iterations.
 *
 * @b Important
 *
 * Iterators aren't invalidated if:
 *
 * * New elements are added to the storage.
 * * The entity currently pointed is modified (for example, elements are added
 *   or removed from it).
 * * The entity currently pointed is destroyed.
 *
 * In all other cases, modifying the pools iterated by the group in any way
 * invalidates all the iterators.
 *
 * @tparam Get Types of storage _observed_ by the group.
 * @tparam Exclude Types of storage used to filter the group.
 */
template<typename... Get, typename... Exclude>
class basic_group<owned_t<>, get_t<Get...>, exclude_t<Exclude...>> {
    using base_type = std::common_type_t<typename Get::base_type..., typename Exclude::base_type...>;
    using underlying_type = typename base_type::entity_type;

    template<typename Type>
    static constexpr std::size_t index_of = type_list_index_v<std::remove_const_t<Type>, type_list<typename Get::element_type..., typename Exclude::element_type...>>;

    template<std::size_t... Index>
    [[nodiscard]] auto pools_for(std::index_sequence<Index...>) const noexcept {
        using return_type = std::tuple<Get *...>;
        return descriptor ? return_type{static_cast<Get *>(descriptor->template storage<Index>())...} : return_type{};
    }

public:
    /*! @brief Underlying entity identifier. */
    using entity_type = underlying_type;
    /*! @brief Unsigned integer type. */
    using size_type = std::size_t;
    /*! @brief Common type among all storage types. */
    using common_type = base_type;
    /*! @brief Random access iterator type. */
    using iterator = typename common_type::iterator;
    /*! @brief Reverse iterator type. */
    using reverse_iterator = typename common_type::reverse_iterator;
    /*! @brief Iterable group type. */
    using iterable = iterable_adaptor<internal::extended_group_iterator<iterator, owned_t<>, get_t<Get...>>>;
    /*! @brief Group handler type. */
    using handler = internal::group_handler<common_type, 0u, sizeof...(Get), sizeof...(Exclude)>;

    /**
     * @brief Group opaque identifier.
     * @return Group opaque identifier.
     */
    static id_type group_id() noexcept {
        return type_hash<basic_group<owned_t<>, get_t<std::remove_const_t<Get>...>, exclude_t<std::remove_const_t<Exclude>...>>>::value();
    }

    /*! @brief Default constructor to use to create empty, invalid groups. */
    basic_group() noexcept
        : descriptor{} {}

    /**
     * @brief Constructs a group from a set of storage classes.
     * @param ref A reference to a group handler.
     */
    basic_group(handler &ref) noexcept
        : descriptor{&ref} {}

    /**
     * @brief Returns the leading storage of a group.
     * @return The leading storage of the group.
     */
    [[nodiscard]] const common_type &handle() const noexcept {
        return descriptor->handle();
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
        using type = type_list_element_t<Index, type_list<Get..., Exclude...>>;
        return *this ? static_cast<type *>(descriptor->template storage<Index>()) : nullptr;
    }

    /**
     * @brief Returns the number of entities that are part of the group.
     * @return Number of entities that are part of the group.
     */
    [[nodiscard]] size_type size() const noexcept {
        return *this ? handle().size() : size_type{};
    }

    /**
     * @brief Returns the number of elements that a group has currently
     * allocated space for.
     * @return Capacity of the group.
     */
    [[nodiscard]] size_type capacity() const noexcept {
        return *this ? handle().capacity() : size_type{};
    }

    /*! @brief Requests the removal of unused capacity. */
    void shrink_to_fit() {
        if(*this) {
            descriptor->handle().shrink_to_fit();
        }
    }

    /**
     * @brief Checks whether a group is empty.
     * @return True if the group is empty, false otherwise.
     */
    [[nodiscard]] bool empty() const noexcept {
        return !*this || handle().empty();
    }

    /**
     * @brief Returns an iterator to the first entity of the group.
     *
     * If the group is empty, the returned iterator will be equal to `end()`.
     *
     * @return An iterator to the first entity of the group.
     */
    [[nodiscard]] iterator begin() const noexcept {
        return *this ? handle().begin() : iterator{};
    }

    /**
     * @brief Returns an iterator that is past the last entity of the group.
     * @return An iterator to the entity following the last entity of the
     * group.
     */
    [[nodiscard]] iterator end() const noexcept {
        return *this ? handle().end() : iterator{};
    }

    /**
     * @brief Returns an iterator to the first entity of the reversed group.
     *
     * If the group is empty, the returned iterator will be equal to `rend()`.
     *
     * @return An iterator to the first entity of the reversed group.
     */
    [[nodiscard]] reverse_iterator rbegin() const noexcept {
        return *this ? handle().rbegin() : reverse_iterator{};
    }

    /**
     * @brief Returns an iterator that is past the last entity of the reversed
     * group.
     * @return An iterator to the entity following the last entity of the
     * reversed group.
     */
    [[nodiscard]] reverse_iterator rend() const noexcept {
        return *this ? handle().rend() : reverse_iterator{};
    }

    /**
     * @brief Returns the first entity of the group, if any.
     * @return The first entity of the group if one exists, the null entity
     * otherwise.
     */
    [[nodiscard]] entity_type front() const noexcept {
        const auto it = begin();
        return it != end() ? *it : null;
    }

    /**
     * @brief Returns the last entity of the group, if any.
     * @return The last entity of the group if one exists, the null entity
     * otherwise.
     */
    [[nodiscard]] entity_type back() const noexcept {
        const auto it = rbegin();
        return it != rend() ? *it : null;
    }

    /**
     * @brief Finds an entity.
     * @param entt A valid identifier.
     * @return An iterator to the given entity if it's found, past the end
     * iterator otherwise.
     */
    [[nodiscard]] iterator find(const entity_type entt) const noexcept {
        return *this ? handle().find(entt) : iterator{};
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
     * @brief Checks if a group is properly initialized.
     * @return True if the group is properly initialized, false otherwise.
     */
    [[nodiscard]] explicit operator bool() const noexcept {
        return descriptor != nullptr;
    }

    /**
     * @brief Checks if a group contains an entity.
     * @param entt A valid identifier.
     * @return True if the group contains the given entity, false otherwise.
     */
    [[nodiscard]] bool contains(const entity_type entt) const noexcept {
        return *this && handle().contains(entt);
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
        const auto cpools = pools_for(std::index_sequence_for<Get...>{});

        if constexpr(sizeof...(Index) == 0) {
            return std::apply([entt](auto *...curr) { return std::tuple_cat(curr->get_as_tuple(entt)...); }, cpools);
        } else if constexpr(sizeof...(Index) == 1) {
            return (std::get<Index>(cpools)->get(entt), ...);
        } else {
            return std::tuple_cat(std::get<Index>(cpools)->get_as_tuple(entt)...);
        }
    }

    /**
     * @brief Iterates entities and elements and applies the given function
     * object to them.
     *
     * The function object is invoked for each entity. It is provided with the
     * entity itself and a set of references to non-empty elements. The
     * _constness_ of the elements is as requested.<br/>
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
        for(const auto entt: *this) {
            if constexpr(is_applicable_v<Func, decltype(std::tuple_cat(std::tuple<entity_type>{}, std::declval<basic_group>().get({})))>) {
                std::apply(func, std::tuple_cat(std::make_tuple(entt), get(entt)));
            } else {
                std::apply(func, get(entt));
            }
        }
    }

    /**
     * @brief Returns an iterable object to use to _visit_ a group.
     *
     * The iterable object returns tuples that contain the current entity and a
     * set of references to its non-empty elements. The _constness_ of the
     * elements is as requested.
     *
     * @note
     * Empty types aren't explicitly instantiated and therefore they are never
     * returned during iterations.
     *
     * @return An iterable object to use to _visit_ the group.
     */
    [[nodiscard]] iterable each() const noexcept {
        const auto cpools = pools_for(std::index_sequence_for<Get...>{});
        return iterable{{begin(), cpools}, {end(), cpools}};
    }

    /**
     * @brief Sort a group according to the given comparison function.
     *
     * The comparison function object must return `true` if the first element
     * is _less_ than the second one, `false` otherwise. The signature of the
     * comparison function should be equivalent to one of the following:
     *
     * @code{.cpp}
     * bool(std::tuple<Type &...>, std::tuple<Type &...>);
     * bool(const Type &..., const Type &...);
     * bool(const Entity, const Entity);
     * @endcode
     *
     * Where `Type` are such that they are iterated by the group.<br/>
     * Moreover, the comparison function object shall induce a
     * _strict weak ordering_ on the values.
     *
     * The sort function object must offer a member function template
     * `operator()` that accepts three arguments:
     *
     * * An iterator to the first element of the range to sort.
     * * An iterator past the last element of the range to sort.
     * * A comparison function to use to compare the elements.
     *
     * @tparam Type Optional type of element to compare.
     * @tparam Other Other optional types of elements to compare.
     * @tparam Compare Type of comparison function object.
     * @tparam Sort Type of sort function object.
     * @tparam Args Types of arguments to forward to the sort function object.
     * @param compare A valid comparison function object.
     * @param algo A valid sort function object.
     * @param args Arguments to forward to the sort function object, if any.
     */
    template<typename Type, typename... Other, typename Compare, typename Sort = std_sort, typename... Args>
    void sort(Compare compare, Sort algo = Sort{}, Args &&...args) {
        sort<index_of<Type>, index_of<Other>...>(std::move(compare), std::move(algo), std::forward<Args>(args)...);
    }

    /**
     * @brief Sort a group according to the given comparison function.
     *
     * @sa sort
     *
     * @tparam Index Optional indexes of elements to compare.
     * @tparam Compare Type of comparison function object.
     * @tparam Sort Type of sort function object.
     * @tparam Args Types of arguments to forward to the sort function object.
     * @param compare A valid comparison function object.
     * @param algo A valid sort function object.
     * @param args Arguments to forward to the sort function object, if any.
     */
    template<std::size_t... Index, typename Compare, typename Sort = std_sort, typename... Args>
    void sort(Compare compare, Sort algo = Sort{}, Args &&...args) {
        if(*this) {
            if constexpr(sizeof...(Index) == 0) {
                static_assert(std::is_invocable_v<Compare, const entity_type, const entity_type>, "Invalid comparison function");
                descriptor->handle().sort(std::move(compare), std::move(algo), std::forward<Args>(args)...);
            } else {
                auto comp = [&compare, cpools = pools_for(std::index_sequence_for<Get...>{})](const entity_type lhs, const entity_type rhs) {
                    if constexpr(sizeof...(Index) == 1) {
                        return compare((std::get<Index>(cpools)->get(lhs), ...), (std::get<Index>(cpools)->get(rhs), ...));
                    } else {
                        return compare(std::forward_as_tuple(std::get<Index>(cpools)->get(lhs)...), std::forward_as_tuple(std::get<Index>(cpools)->get(rhs)...));
                    }
                };

                descriptor->handle().sort(std::move(comp), std::move(algo), std::forward<Args>(args)...);
            }
        }
    }

    /**
     * @brief Sort entities according to their order in a range.
     *
     * The shared pool of entities and thus its order is affected by the changes
     * to each and every pool that it tracks.
     *
     * @tparam It Type of input iterator.
     * @param first An iterator to the first element of the range of entities.
     * @param last An iterator past the last element of the range of entities.
     */
    template<typename It>
    void sort_as(It first, It last) const {
        if(*this) {
            descriptor->handle().sort_as(first, last);
        }
    }

private:
    handler *descriptor;
};

/**
 * @brief Owning group.
 *
 * Owning groups returns all entities and only the entities that are at
 * least in the given storage. Moreover:
 *
 * * It's guaranteed that the entity list is tightly packed in memory for fast
 *   iterations.
 * * It's guaranteed that all elements in the owned storage are tightly packed
 *   in memory for even faster iterations and to allow direct access.
 * * They stay true to the order of the owned storage and all instances have the
 *   same order in memory.
 *
 * The more types of storage are owned, the faster it is to iterate a group.
 *
 * @b Important
 *
 * Iterators aren't invalidated if:
 *
 * * New elements are added to the storage.
 * * The entity currently pointed is modified (for example, elements are added
 *   or removed from it).
 * * The entity currently pointed is destroyed.
 *
 * In all other cases, modifying the pools iterated by the group in any way
 * invalidates all the iterators.
 *
 * @tparam Owned Types of storage _owned_ by the group.
 * @tparam Get Types of storage _observed_ by the group.
 * @tparam Exclude Types of storage used to filter the group.
 */
template<typename... Owned, typename... Get, typename... Exclude>
class basic_group<owned_t<Owned...>, get_t<Get...>, exclude_t<Exclude...>> {
    static_assert(((Owned::storage_policy != deletion_policy::in_place) && ...), "Groups do not support in-place delete");

    using base_type = std::common_type_t<typename Owned::base_type..., typename Get::base_type..., typename Exclude::base_type...>;
    using underlying_type = typename base_type::entity_type;

    template<typename Type>
    static constexpr std::size_t index_of = type_list_index_v<std::remove_const_t<Type>, type_list<typename Owned::element_type..., typename Get::element_type..., typename Exclude::element_type...>>;

    template<std::size_t... Index, std::size_t... Other>
    [[nodiscard]] auto pools_for(std::index_sequence<Index...>, std::index_sequence<Other...>) const noexcept {
        using return_type = std::tuple<Owned *..., Get *...>;
        return descriptor ? return_type{static_cast<Owned *>(descriptor->template storage<Index>())..., static_cast<Get *>(descriptor->template storage<sizeof...(Owned) + Other>())...} : return_type{};
    }

public:
    /*! @brief Underlying entity identifier. */
    using entity_type = underlying_type;
    /*! @brief Unsigned integer type. */
    using size_type = std::size_t;
    /*! @brief Common type among all storage types. */
    using common_type = base_type;
    /*! @brief Random access iterator type. */
    using iterator = typename common_type::iterator;
    /*! @brief Reverse iterator type. */
    using reverse_iterator = typename common_type::reverse_iterator;
    /*! @brief Iterable group type. */
    using iterable = iterable_adaptor<internal::extended_group_iterator<iterator, owned_t<Owned...>, get_t<Get...>>>;
    /*! @brief Group handler type. */
    using handler = internal::group_handler<common_type, sizeof...(Owned), sizeof...(Get), sizeof...(Exclude)>;

    /**
     * @brief Group opaque identifier.
     * @return Group opaque identifier.
     */
    static id_type group_id() noexcept {
        return type_hash<basic_group<owned_t<std::remove_const_t<Owned>...>, get_t<std::remove_const_t<Get>...>, exclude_t<std::remove_const_t<Exclude>...>>>::value();
    }

    /*! @brief Default constructor to use to create empty, invalid groups. */
    basic_group() noexcept
        : descriptor{} {}

    /**
     * @brief Constructs a group from a set of storage classes.
     * @param ref A reference to a group handler.
     */
    basic_group(handler &ref) noexcept
        : descriptor{&ref} {}

    /**
     * @brief Returns the leading storage of a group.
     * @return The leading storage of the group.
     */
    [[nodiscard]] const common_type &handle() const noexcept {
        return *storage<0>();
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
        using type = type_list_element_t<Index, type_list<Owned..., Get..., Exclude...>>;
        return *this ? static_cast<type *>(descriptor->template storage<Index>()) : nullptr;
    }

    /**
     * @brief Returns the number of entities that that are part of the group.
     * @return Number of entities that that are part of the group.
     */
    [[nodiscard]] size_type size() const noexcept {
        return *this ? descriptor->length() : size_type{};
    }

    /**
     * @brief Checks whether a group is empty.
     * @return True if the group is empty, false otherwise.
     */
    [[nodiscard]] bool empty() const noexcept {
        return !*this || !descriptor->length();
    }

    /**
     * @brief Returns an iterator to the first entity of the group.
     *
     * If the group is empty, the returned iterator will be equal to `end()`.
     *
     * @return An iterator to the first entity of the group.
     */
    [[nodiscard]] iterator begin() const noexcept {
        return *this ? (handle().end() - descriptor->length()) : iterator{};
    }

    /**
     * @brief Returns an iterator that is past the last entity of the group.
     * @return An iterator to the entity following the last entity of the
     * group.
     */
    [[nodiscard]] iterator end() const noexcept {
        return *this ? handle().end() : iterator{};
    }

    /**
     * @brief Returns an iterator to the first entity of the reversed group.
     *
     * If the group is empty, the returned iterator will be equal to `rend()`.
     *
     * @return An iterator to the first entity of the reversed group.
     */
    [[nodiscard]] reverse_iterator rbegin() const noexcept {
        return *this ? handle().rbegin() : reverse_iterator{};
    }

    /**
     * @brief Returns an iterator that is past the last entity of the reversed
     * group.
     * @return An iterator to the entity following the last entity of the
     * reversed group.
     */
    [[nodiscard]] reverse_iterator rend() const noexcept {
        return *this ? (handle().rbegin() + descriptor->length()) : reverse_iterator{};
    }

    /**
     * @brief Returns the first entity of the group, if any.
     * @return The first entity of the group if one exists, the null entity
     * otherwise.
     */
    [[nodiscard]] entity_type front() const noexcept {
        const auto it = begin();
        return it != end() ? *it : null;
    }

    /**
     * @brief Returns the last entity of the group, if any.
     * @return The last entity of the group if one exists, the null entity
     * otherwise.
     */
    [[nodiscard]] entity_type back() const noexcept {
        const auto it = rbegin();
        return it != rend() ? *it : null;
    }

    /**
     * @brief Finds an entity.
     * @param entt A valid identifier.
     * @return An iterator to the given entity if it's found, past the end
     * iterator otherwise.
     */
    [[nodiscard]] iterator find(const entity_type entt) const noexcept {
        const auto it = *this ? handle().find(entt) : iterator{};
        return it >= begin() ? it : iterator{};
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
     * @brief Checks if a group is properly initialized.
     * @return True if the group is properly initialized, false otherwise.
     */
    [[nodiscard]] explicit operator bool() const noexcept {
        return descriptor != nullptr;
    }

    /**
     * @brief Checks if a group contains an entity.
     * @param entt A valid identifier.
     * @return True if the group contains the given entity, false otherwise.
     */
    [[nodiscard]] bool contains(const entity_type entt) const noexcept {
        return *this && handle().contains(entt) && (handle().index(entt) < (descriptor->length()));
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
        const auto cpools = pools_for(std::index_sequence_for<Owned...>{}, std::index_sequence_for<Get...>{});

        if constexpr(sizeof...(Index) == 0) {
            return std::apply([entt](auto *...curr) { return std::tuple_cat(curr->get_as_tuple(entt)...); }, cpools);
        } else if constexpr(sizeof...(Index) == 1) {
            return (std::get<Index>(cpools)->get(entt), ...);
        } else {
            return std::tuple_cat(std::get<Index>(cpools)->get_as_tuple(entt)...);
        }
    }

    /**
     * @brief Iterates entities and elements and applies the given function
     * object to them.
     *
     * The function object is invoked for each entity. It is provided with the
     * entity itself and a set of references to non-empty elements. The
     * _constness_ of the elements is as requested.<br/>
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
        for(auto args: each()) {
            if constexpr(is_applicable_v<Func, decltype(std::tuple_cat(std::tuple<entity_type>{}, std::declval<basic_group>().get({})))>) {
                std::apply(func, args);
            } else {
                std::apply([&func](auto, auto &&...less) { func(std::forward<decltype(less)>(less)...); }, args);
            }
        }
    }

    /**
     * @brief Returns an iterable object to use to _visit_ a group.
     *
     * The iterable object returns tuples that contain the current entity and a
     * set of references to its non-empty elements. The _constness_ of the
     * elements is as requested.
     *
     * @note
     * Empty types aren't explicitly instantiated and therefore they are never
     * returned during iterations.
     *
     * @return An iterable object to use to _visit_ the group.
     */
    [[nodiscard]] iterable each() const noexcept {
        const auto cpools = pools_for(std::index_sequence_for<Owned...>{}, std::index_sequence_for<Get...>{});
        return iterable{{begin(), cpools}, {end(), cpools}};
    }

    /**
     * @brief Sort a group according to the given comparison function.
     *
     * The comparison function object must return `true` if the first element
     * is _less_ than the second one, `false` otherwise. The signature of the
     * comparison function should be equivalent to one of the following:
     *
     * @code{.cpp}
     * bool(std::tuple<Type &...>, std::tuple<Type &...>);
     * bool(const Type &, const Type &);
     * bool(const Entity, const Entity);
     * @endcode
     *
     * Where `Type` are either owned types or not but still such that they are
     * iterated by the group.<br/>
     * Moreover, the comparison function object shall induce a
     * _strict weak ordering_ on the values.
     *
     * The sort function object must offer a member function template
     * `operator()` that accepts three arguments:
     *
     * * An iterator to the first element of the range to sort.
     * * An iterator past the last element of the range to sort.
     * * A comparison function to use to compare the elements.
     *
     * @tparam Type Optional type of element to compare.
     * @tparam Other Other optional types of elements to compare.
     * @tparam Compare Type of comparison function object.
     * @tparam Sort Type of sort function object.
     * @tparam Args Types of arguments to forward to the sort function object.
     * @param compare A valid comparison function object.
     * @param algo A valid sort function object.
     * @param args Arguments to forward to the sort function object, if any.
     */
    template<typename Type, typename... Other, typename Compare, typename Sort = std_sort, typename... Args>
    void sort(Compare compare, Sort algo = Sort{}, Args &&...args) const {
        sort<index_of<Type>, index_of<Other>...>(std::move(compare), std::move(algo), std::forward<Args>(args)...);
    }

    /**
     * @brief Sort a group according to the given comparison function.
     *
     * @sa sort
     *
     * @tparam Index Optional indexes of elements to compare.
     * @tparam Compare Type of comparison function object.
     * @tparam Sort Type of sort function object.
     * @tparam Args Types of arguments to forward to the sort function object.
     * @param compare A valid comparison function object.
     * @param algo A valid sort function object.
     * @param args Arguments to forward to the sort function object, if any.
     */
    template<std::size_t... Index, typename Compare, typename Sort = std_sort, typename... Args>
    void sort(Compare compare, Sort algo = Sort{}, Args &&...args) const {
        const auto cpools = pools_for(std::index_sequence_for<Owned...>{}, std::index_sequence_for<Get...>{});

        if constexpr(sizeof...(Index) == 0) {
            static_assert(std::is_invocable_v<Compare, const entity_type, const entity_type>, "Invalid comparison function");
            storage<0>()->sort_n(descriptor->length(), std::move(compare), std::move(algo), std::forward<Args>(args)...);
        } else {
            auto comp = [&compare, &cpools](const entity_type lhs, const entity_type rhs) {
                if constexpr(sizeof...(Index) == 1) {
                    return compare((std::get<Index>(cpools)->get(lhs), ...), (std::get<Index>(cpools)->get(rhs), ...));
                } else {
                    return compare(std::forward_as_tuple(std::get<Index>(cpools)->get(lhs)...), std::forward_as_tuple(std::get<Index>(cpools)->get(rhs)...));
                }
            };

            storage<0>()->sort_n(descriptor->length(), std::move(comp), std::move(algo), std::forward<Args>(args)...);
        }

        auto cb = [this](auto *head, auto *...other) {
            for(auto next = descriptor->length(); next; --next) {
                const auto pos = next - 1;
                [[maybe_unused]] const auto entt = head->data()[pos];
                (other->swap_elements(other->data()[pos], entt), ...);
            }
        };

        std::apply(cb, cpools);
    }

private:
    handler *descriptor;
};

} // namespace entt

#endif

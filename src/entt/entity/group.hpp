#ifndef ENTT_ENTITY_GROUP_HPP
#define ENTT_ENTITY_GROUP_HPP

#include <tuple>
#include <type_traits>
#include <utility>
#include "../config/config.h"
#include "../core/iterator.hpp"
#include "../core/type_traits.hpp"
#include "component.hpp"
#include "entity.hpp"
#include "fwd.hpp"
#include "sparse_set.hpp"
#include "storage.hpp"

namespace entt {

/**
 * @cond TURN_OFF_DOXYGEN
 * Internal details not to be documented.
 */

namespace internal {

template<typename, typename, typename>
class extended_group_iterator;

template<typename It, typename... Owned, typename... Get>
class extended_group_iterator<It, owned_t<Owned...>, get_t<Get...>> {
    template<typename Type>
    auto index_to_element([[maybe_unused]] Type &cpool) const {
        if constexpr(ignore_as_empty_v<typename Type::value_type>) {
            return std::make_tuple();
        } else {
            return std::forward_as_tuple(cpool.rbegin()[it.index()]);
        }
    }

public:
    using difference_type = std::ptrdiff_t;
    using value_type = decltype(std::tuple_cat(std::make_tuple(*std::declval<It>()), std::declval<Owned>().get_as_tuple({})..., std::declval<Get>().get_as_tuple({})...));
    using pointer = input_iterator_pointer<value_type>;
    using reference = value_type;
    using iterator_category = std::input_iterator_tag;

    constexpr extended_group_iterator()
        : it{},
          pools{} {}

    extended_group_iterator(It from, const std::tuple<Owned *..., Get *...> &cpools)
        : it{from},
          pools{cpools} {}

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

} // namespace internal

/**
 * Internal details not to be documented.
 * @endcond
 */

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
 * * The entity currently pointed is modified (for example, components are added
 *   or removed from it).
 * * The entity currently pointed is destroyed.
 *
 * In all other cases, modifying the pools iterated by the group in any way
 * invalidates all the iterators and using them results in undefined behavior.
 *
 * @tparam Get Types of storage _observed_ by the group.
 * @tparam Exclude Types of storage used to filter the group.
 */
template<typename... Get, typename... Exclude>
class basic_group<owned_t<>, get_t<Get...>, exclude_t<Exclude...>> {
    using underlying_type = std::common_type_t<typename Get::entity_type..., typename Exclude::entity_type...>;
    using basic_common_type = std::common_type_t<typename Get::base_type..., typename Exclude::base_type...>;

    template<typename Type>
    static constexpr std::size_t index_of = type_list_index_v<std::remove_const_t<Type>, type_list<typename Get::value_type...>>;

public:
    /*! @brief Underlying entity identifier. */
    using entity_type = underlying_type;
    /*! @brief Unsigned integer type. */
    using size_type = std::size_t;
    /*! @brief Common type among all storage types. */
    using base_type = basic_common_type;
    /*! @brief Random access iterator type. */
    using iterator = typename base_type::iterator;
    /*! @brief Reversed iterator type. */
    using reverse_iterator = typename base_type::reverse_iterator;
    /*! @brief Iterable group type. */
    using iterable = iterable_adaptor<internal::extended_group_iterator<iterator, owned_t<>, get_t<Get...>>>;

    /*! @brief Default constructor to use to create empty, invalid groups. */
    basic_group() noexcept
        : handler{} {}

    /**
     * @brief Constructs a group from a set of storage classes.
     * @param ref The actual entities to iterate.
     * @param gpool Storage types to iterate _observed_ by the group.
     */
    basic_group(basic_common_type &ref, Get &...gpool) noexcept
        : handler{&ref},
          pools{&gpool...} {}

    /**
     * @brief Returns a const reference to the underlying handler.
     * @return A const reference to the underlying handler.
     */
    [[nodiscard]] const base_type &handle() const noexcept {
        return *handler;
    }

    /**
     * @brief Returns the storage for a given component type.
     * @tparam Type Type of component of which to return the storage.
     * @return The storage for the given component type.
     */
    template<typename Type>
    [[nodiscard]] decltype(auto) storage() const noexcept {
        return storage<index_of<Type>>();
    }

    /**
     * @brief Returns the storage for a given index.
     * @tparam Index Index of the storage to return.
     * @return The storage for the given index.
     */
    template<std::size_t Index>
    [[nodiscard]] decltype(auto) storage() const noexcept {
        return *std::get<Index>(pools);
    }

    /**
     * @brief Returns the number of entities that are part of the group.
     * @return Number of entities that are part of the group.
     */
    [[nodiscard]] size_type size() const noexcept {
        return *this ? handler->size() : size_type{};
    }

    /**
     * @brief Returns the number of elements that a group has currently
     * allocated space for.
     * @return Capacity of the group.
     */
    [[nodiscard]] size_type capacity() const noexcept {
        return *this ? handler->capacity() : size_type{};
    }

    /*! @brief Requests the removal of unused capacity. */
    void shrink_to_fit() {
        if(*this) {
            handler->shrink_to_fit();
        }
    }

    /**
     * @brief Checks whether a group is empty.
     * @return True if the group is empty, false otherwise.
     */
    [[nodiscard]] bool empty() const noexcept {
        return !*this || handler->empty();
    }

    /**
     * @brief Returns an iterator to the first entity of the group.
     *
     * The returned iterator points to the first entity of the group. If the
     * group is empty, the returned iterator will be equal to `end()`.
     *
     * @return An iterator to the first entity of the group.
     */
    [[nodiscard]] iterator begin() const noexcept {
        return *this ? handler->begin() : iterator{};
    }

    /**
     * @brief Returns an iterator that is past the last entity of the group.
     *
     * The returned iterator points to the entity following the last entity of
     * the group. Attempting to dereference the returned iterator results in
     * undefined behavior.
     *
     * @return An iterator to the entity following the last entity of the
     * group.
     */
    [[nodiscard]] iterator end() const noexcept {
        return *this ? handler->end() : iterator{};
    }

    /**
     * @brief Returns an iterator to the first entity of the reversed group.
     *
     * The returned iterator points to the first entity of the reversed group.
     * If the group is empty, the returned iterator will be equal to `rend()`.
     *
     * @return An iterator to the first entity of the reversed group.
     */
    [[nodiscard]] reverse_iterator rbegin() const noexcept {
        return *this ? handler->rbegin() : reverse_iterator{};
    }

    /**
     * @brief Returns an iterator that is past the last entity of the reversed
     * group.
     *
     * The returned iterator points to the entity following the last entity of
     * the reversed group. Attempting to dereference the returned iterator
     * results in undefined behavior.
     *
     * @return An iterator to the entity following the last entity of the
     * reversed group.
     */
    [[nodiscard]] reverse_iterator rend() const noexcept {
        return *this ? handler->rend() : reverse_iterator{};
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
        const auto it = *this ? handler->find(entt) : iterator{};
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
     * @brief Checks if a group is properly initialized.
     * @return True if the group is properly initialized, false otherwise.
     */
    [[nodiscard]] explicit operator bool() const noexcept {
        return handler != nullptr;
    }

    /**
     * @brief Checks if a group contains an entity.
     * @param entt A valid identifier.
     * @return True if the group contains the given entity, false otherwise.
     */
    [[nodiscard]] bool contains(const entity_type entt) const noexcept {
        return *this && handler->contains(entt);
    }

    /**
     * @brief Returns the components assigned to the given entity.
     *
     * Prefer this function instead of `registry::get` during iterations. It has
     * far better performance than its counterpart.
     *
     * @warning
     * Attempting to use an invalid component type results in a compilation
     * error. Attempting to use an entity that doesn't belong to the group
     * results in undefined behavior.
     *
     * @tparam Type Types of components to get.
     * @param entt A valid identifier.
     * @return The components assigned to the entity.
     */
    template<typename... Type>
    [[nodiscard]] decltype(auto) get(const entity_type entt) const {
        if constexpr(sizeof...(Type) == 0) {
            return std::apply([entt](auto *...curr) { return std::tuple_cat(curr->get_as_tuple(entt)...); }, pools);
        } else if constexpr(sizeof...(Type) == 1) {
            return (std::get<index_of<Type>>(pools)->get(entt), ...);
        } else {
            return std::tuple_cat(std::get<index_of<Type>>(pools)->get_as_tuple(entt)...);
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
     * set of references to its non-empty components. The _constness_ of the
     * components is as requested.
     *
     * @note
     * Empty types aren't explicitly instantiated and therefore they are never
     * returned during iterations.
     *
     * @return An iterable object to use to _visit_ the group.
     */
    [[nodiscard]] iterable each() const noexcept {
        return iterable{{begin(), pools}, {end(), pools}};
    }

    /**
     * @brief Sort a group according to the given comparison function.
     *
     * Sort the group so that iterating it with a couple of iterators returns
     * entities and components in the expected order. See `begin` and `end` for
     * more details.
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
     * @tparam Type Optional types of components to compare.
     * @tparam Compare Type of comparison function object.
     * @tparam Sort Type of sort function object.
     * @tparam Args Types of arguments to forward to the sort function object.
     * @param compare A valid comparison function object.
     * @param algo A valid sort function object.
     * @param args Arguments to forward to the sort function object, if any.
     */
    template<typename... Type, typename Compare, typename Sort = std_sort, typename... Args>
    void sort(Compare compare, Sort algo = Sort{}, Args &&...args) {
        if(*this) {
            if constexpr(sizeof...(Type) == 0) {
                static_assert(std::is_invocable_v<Compare, const entity_type, const entity_type>, "Invalid comparison function");
                handler->sort(std::move(compare), std::move(algo), std::forward<Args>(args)...);
            } else {
                auto comp = [this, &compare](const entity_type lhs, const entity_type rhs) {
                    if constexpr(sizeof...(Type) == 1) {
                        return compare((std::get<index_of<Type>>(pools)->get(lhs), ...), (std::get<index_of<Type>>(pools)->get(rhs), ...));
                    } else {
                        return compare(std::forward_as_tuple(std::get<index_of<Type>>(pools)->get(lhs)...), std::forward_as_tuple(std::get<index_of<Type>>(pools)->get(rhs)...));
                    }
                };

                handler->sort(std::move(comp), std::move(algo), std::forward<Args>(args)...);
            }
        }
    }

    /**
     * @brief Sort the shared pool of entities according to the given component.
     *
     * Non-owning groups of the same type share with the registry a pool of
     * entities with its own order that doesn't depend on the order of any pool
     * of components. Users can order the underlying data structure so that it
     * respects the order of the pool of the given component.
     *
     * @note
     * The shared pool of entities and thus its order is affected by the changes
     * to each and every pool that it tracks. Therefore changes to those pools
     * can quickly ruin the order imposed to the pool of entities shared between
     * the non-owning groups.
     *
     * @tparam Type Type of component to use to impose the order.
     */
    template<typename Type>
    void sort() const {
        if(*this) {
            handler->respect(*std::get<index_of<Type>>(pools));
        }
    }

private:
    base_type *const handler;
    const std::tuple<Get *...> pools;
};

/**
 * @brief Owning group.
 *
 * Owning groups returns all entities and only the entities that are at
 * least in the given storage. Moreover:
 *
 * * It's guaranteed that the entity list is tightly packed in memory for fast
 *   iterations.
 * * It's guaranteed that all components in the owned storage are tightly packed
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
 * * The entity currently pointed is modified (for example, components are added
 *   or removed from it).
 * * The entity currently pointed is destroyed.
 *
 * In all other cases, modifying the pools iterated by the group in any way
 * invalidates all the iterators and using them results in undefined behavior.
 *
 * @tparam Owned Types of storage _owned_ by the group.
 * @tparam Get Types of storage _observed_ by the group.
 * @tparam Exclude Types of storage used to filter the group.
 */
template<typename... Owned, typename... Get, typename... Exclude>
class basic_group<owned_t<Owned...>, get_t<Get...>, exclude_t<Exclude...>> {
    using underlying_type = std::common_type_t<typename Owned::entity_type..., typename Get::entity_type..., typename Exclude::entity_type...>;
    using basic_common_type = std::common_type_t<typename Owned::base_type..., typename Get::base_type..., typename Exclude::base_type...>;

    template<typename Type>
    static constexpr std::size_t index_of = type_list_index_v<std::remove_const_t<Type>, type_list<typename Owned::value_type..., typename Get::value_type...>>;

public:
    /*! @brief Underlying entity identifier. */
    using entity_type = underlying_type;
    /*! @brief Unsigned integer type. */
    using size_type = std::size_t;
    /*! @brief Common type among all storage types. */
    using base_type = basic_common_type;
    /*! @brief Random access iterator type. */
    using iterator = typename base_type::iterator;
    /*! @brief Reversed iterator type. */
    using reverse_iterator = typename base_type::reverse_iterator;
    /*! @brief Iterable group type. */
    using iterable = iterable_adaptor<internal::extended_group_iterator<iterator, owned_t<Owned...>, get_t<Get...>>>;

    /*! @brief Default constructor to use to create empty, invalid groups. */
    basic_group() noexcept
        : length{} {}

    /**
     * @brief Constructs a group from a set of storage classes.
     * @param extent The actual number of entities to iterate.
     * @param opool Storage types to iterate _owned_ by the group.
     * @param gpool Storage types to iterate _observed_ by the group.
     */
    basic_group(const std::size_t &extent, Owned &...opool, Get &...gpool) noexcept
        : pools{&opool..., &gpool...},
          length{&extent} {}

    /**
     * @brief Returns the storage for a given component type.
     * @tparam Type Type of component of which to return the storage.
     * @return The storage for the given component type.
     */
    template<typename Type>
    [[nodiscard]] decltype(auto) storage() const noexcept {
        return storage<index_of<Type>>();
    }

    /**
     * @brief Returns the storage for a given index.
     * @tparam Index Index of the storage to return.
     * @return The storage for the given index.
     */
    template<std::size_t Index>
    [[nodiscard]] decltype(auto) storage() const noexcept {
        return *std::get<Index>(pools);
    }

    /**
     * @brief Returns the number of entities that that are part of the group.
     * @return Number of entities that that are part of the group.
     */
    [[nodiscard]] size_type size() const noexcept {
        return *this ? *length : size_type{};
    }

    /**
     * @brief Checks whether a group is empty.
     * @return True if the group is empty, false otherwise.
     */
    [[nodiscard]] bool empty() const noexcept {
        return !*this || !*length;
    }

    /**
     * @brief Returns an iterator to the first entity of the group.
     *
     * The returned iterator points to the first entity of the group. If the
     * group is empty, the returned iterator will be equal to `end()`.
     *
     * @return An iterator to the first entity of the group.
     */
    [[nodiscard]] iterator begin() const noexcept {
        return *this ? (std::get<0>(pools)->base_type::end() - *length) : iterator{};
    }

    /**
     * @brief Returns an iterator that is past the last entity of the group.
     *
     * The returned iterator points to the entity following the last entity of
     * the group. Attempting to dereference the returned iterator results in
     * undefined behavior.
     *
     * @return An iterator to the entity following the last entity of the
     * group.
     */
    [[nodiscard]] iterator end() const noexcept {
        return *this ? std::get<0>(pools)->base_type::end() : iterator{};
    }

    /**
     * @brief Returns an iterator to the first entity of the reversed group.
     *
     * The returned iterator points to the first entity of the reversed group.
     * If the group is empty, the returned iterator will be equal to `rend()`.
     *
     * @return An iterator to the first entity of the reversed group.
     */
    [[nodiscard]] reverse_iterator rbegin() const noexcept {
        return *this ? std::get<0>(pools)->base_type::rbegin() : reverse_iterator{};
    }

    /**
     * @brief Returns an iterator that is past the last entity of the reversed
     * group.
     *
     * The returned iterator points to the entity following the last entity of
     * the reversed group. Attempting to dereference the returned iterator
     * results in undefined behavior.
     *
     * @return An iterator to the entity following the last entity of the
     * reversed group.
     */
    [[nodiscard]] reverse_iterator rend() const noexcept {
        return *this ? (std::get<0>(pools)->base_type::rbegin() + *length) : reverse_iterator{};
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
        const auto it = *this ? std::get<0>(pools)->find(entt) : iterator{};
        return it != end() && it >= begin() && *it == entt ? it : end();
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
        return length != nullptr;
    }

    /**
     * @brief Checks if a group contains an entity.
     * @param entt A valid identifier.
     * @return True if the group contains the given entity, false otherwise.
     */
    [[nodiscard]] bool contains(const entity_type entt) const noexcept {
        return *this && std::get<0>(pools)->contains(entt) && (std::get<0>(pools)->index(entt) < (*length));
    }

    /**
     * @brief Returns the components assigned to the given entity.
     *
     * Prefer this function instead of `registry::get` during iterations. It has
     * far better performance than its counterpart.
     *
     * @warning
     * Attempting to use an invalid component type results in a compilation
     * error. Attempting to use an entity that doesn't belong to the group
     * results in undefined behavior.
     *
     * @tparam Type Types of components to get.
     * @param entt A valid identifier.
     * @return The components assigned to the entity.
     */
    template<typename... Type>
    [[nodiscard]] decltype(auto) get(const entity_type entt) const {
        if constexpr(sizeof...(Type) == 0) {
            return std::apply([entt](auto *...curr) { return std::tuple_cat(curr->get_as_tuple(entt)...); }, pools);
        } else if constexpr(sizeof...(Type) == 1) {
            return (std::get<index_of<Type>>(pools)->get(entt), ...);
        } else {
            return std::tuple_cat(std::get<index_of<Type>>(pools)->get_as_tuple(entt)...);
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
     * set of references to its non-empty components. The _constness_ of the
     * components is as requested.
     *
     * @note
     * Empty types aren't explicitly instantiated and therefore they are never
     * returned during iterations.
     *
     * @return An iterable object to use to _visit_ the group.
     */
    [[nodiscard]] iterable each() const noexcept {
        return {{begin(), pools}, {end(), pools}};
    }

    /**
     * @brief Sort a group according to the given comparison function.
     *
     * Sort the group so that iterating it with a couple of iterators returns
     * entities and components in the expected order. See `begin` and `end` for
     * more details.
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
     * @tparam Type Optional types of components to compare.
     * @tparam Compare Type of comparison function object.
     * @tparam Sort Type of sort function object.
     * @tparam Args Types of arguments to forward to the sort function object.
     * @param compare A valid comparison function object.
     * @param algo A valid sort function object.
     * @param args Arguments to forward to the sort function object, if any.
     */
    template<typename... Type, typename Compare, typename Sort = std_sort, typename... Args>
    void sort(Compare compare, Sort algo = Sort{}, Args &&...args) const {
        if constexpr(sizeof...(Type) == 0) {
            static_assert(std::is_invocable_v<Compare, const entity_type, const entity_type>, "Invalid comparison function");
            std::get<0>(pools)->sort_n(*length, std::move(compare), std::move(algo), std::forward<Args>(args)...);
        } else {
            auto comp = [this, &compare](const entity_type lhs, const entity_type rhs) {
                if constexpr(sizeof...(Type) == 1) {
                    return compare((std::get<index_of<Type>>(pools)->get(lhs), ...), (std::get<index_of<Type>>(pools)->get(rhs), ...));
                } else {
                    return compare(std::forward_as_tuple(std::get<index_of<Type>>(pools)->get(lhs)...), std::forward_as_tuple(std::get<index_of<Type>>(pools)->get(rhs)...));
                }
            };

            std::get<0>(pools)->sort_n(*length, std::move(comp), std::move(algo), std::forward<Args>(args)...);
        }

        std::apply([this](auto *head, auto *...other) {
            for(auto next = *length; next; --next) {
                const auto pos = next - 1;
                [[maybe_unused]] const auto entt = head->data()[pos];
                (other->swap_elements(other->data()[pos], entt), ...);
            }
        },
                   pools);
    }

private:
    const std::tuple<Owned *..., Get *...> pools;
    const size_type *const length;
};

} // namespace entt

#endif

#ifndef ENTT_ENTITY_STORAGE_HPP
#define ENTT_ENTITY_STORAGE_HPP

#include <cstddef>
#include <iterator>
#include <memory>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>
#include "../config/config.h"
#include "../core/bit.hpp"
#include "../core/iterator.hpp"
#include "../core/memory.hpp"
#include "../core/type_info.hpp"
#include "component.hpp"
#include "entity.hpp"
#include "fwd.hpp"
#include "sparse_set.hpp"

namespace entt {

/*! @cond TURN_OFF_DOXYGEN */
namespace internal {

template<typename Container>
class storage_iterator final {
    friend storage_iterator<const Container>;

    using container_type = std::remove_const_t<Container>;
    using alloc_traits = std::allocator_traits<typename container_type::allocator_type>;

    using iterator_traits = std::iterator_traits<std::conditional_t<
        std::is_const_v<Container>,
        typename alloc_traits::template rebind_traits<typename std::pointer_traits<typename container_type::value_type>::element_type>::const_pointer,
        typename alloc_traits::template rebind_traits<typename std::pointer_traits<typename container_type::value_type>::element_type>::pointer>>;

public:
    using value_type = typename iterator_traits::value_type;
    using pointer = typename iterator_traits::pointer;
    using reference = typename iterator_traits::reference;
    using difference_type = typename iterator_traits::difference_type;
    using iterator_category = std::random_access_iterator_tag;

    constexpr storage_iterator() noexcept = default;

    constexpr storage_iterator(Container *ref, const difference_type idx) noexcept
        : payload{ref},
          offset{idx} {}

    template<bool Const = std::is_const_v<Container>, typename = std::enable_if_t<Const>>
    constexpr storage_iterator(const storage_iterator<std::remove_const_t<Container>> &other) noexcept
        : storage_iterator{other.payload, other.offset} {}

    constexpr storage_iterator &operator++() noexcept {
        return --offset, *this;
    }

    constexpr storage_iterator operator++(int) noexcept {
        const storage_iterator orig = *this;
        return ++(*this), orig;
    }

    constexpr storage_iterator &operator--() noexcept {
        return ++offset, *this;
    }

    constexpr storage_iterator operator--(int) noexcept {
        const storage_iterator orig = *this;
        return operator--(), orig;
    }

    constexpr storage_iterator &operator+=(const difference_type value) noexcept {
        offset -= value;
        return *this;
    }

    constexpr storage_iterator operator+(const difference_type value) const noexcept {
        storage_iterator copy = *this;
        return (copy += value);
    }

    constexpr storage_iterator &operator-=(const difference_type value) noexcept {
        return (*this += -value);
    }

    constexpr storage_iterator operator-(const difference_type value) const noexcept {
        return (*this + -value);
    }

    [[nodiscard]] constexpr reference operator[](const difference_type value) const noexcept {
        const auto pos = index() - value;
        constexpr auto page_size = component_traits<value_type>::page_size;
        return (*payload)[pos / page_size][fast_mod(static_cast<std::size_t>(pos), page_size)];
    }

    [[nodiscard]] constexpr pointer operator->() const noexcept {
        return std::addressof(operator[](0));
    }

    [[nodiscard]] constexpr reference operator*() const noexcept {
        return operator[](0);
    }

    [[nodiscard]] constexpr difference_type index() const noexcept {
        return offset - 1;
    }

private:
    Container *payload;
    difference_type offset;
};

template<typename Lhs, typename Rhs>
[[nodiscard]] constexpr std::ptrdiff_t operator-(const storage_iterator<Lhs> &lhs, const storage_iterator<Rhs> &rhs) noexcept {
    return rhs.index() - lhs.index();
}

template<typename Lhs, typename Rhs>
[[nodiscard]] constexpr bool operator==(const storage_iterator<Lhs> &lhs, const storage_iterator<Rhs> &rhs) noexcept {
    return lhs.index() == rhs.index();
}

template<typename Lhs, typename Rhs>
[[nodiscard]] constexpr bool operator!=(const storage_iterator<Lhs> &lhs, const storage_iterator<Rhs> &rhs) noexcept {
    return !(lhs == rhs);
}

template<typename Lhs, typename Rhs>
[[nodiscard]] constexpr bool operator<(const storage_iterator<Lhs> &lhs, const storage_iterator<Rhs> &rhs) noexcept {
    return lhs.index() > rhs.index();
}

template<typename Lhs, typename Rhs>
[[nodiscard]] constexpr bool operator>(const storage_iterator<Lhs> &lhs, const storage_iterator<Rhs> &rhs) noexcept {
    return rhs < lhs;
}

template<typename Lhs, typename Rhs>
[[nodiscard]] constexpr bool operator<=(const storage_iterator<Lhs> &lhs, const storage_iterator<Rhs> &rhs) noexcept {
    return !(lhs > rhs);
}

template<typename Lhs, typename Rhs>
[[nodiscard]] constexpr bool operator>=(const storage_iterator<Lhs> &lhs, const storage_iterator<Rhs> &rhs) noexcept {
    return !(lhs < rhs);
}

template<typename It, typename... Other>
class extended_storage_iterator final {
    template<typename Iter, typename... Args>
    friend class extended_storage_iterator;

public:
    using iterator_type = It;
    using value_type = decltype(std::tuple_cat(std::make_tuple(*std::declval<It>()), std::forward_as_tuple(*std::declval<Other>()...)));
    using pointer = input_iterator_pointer<value_type>;
    using reference = value_type;
    using difference_type = std::ptrdiff_t;
    using iterator_category = std::input_iterator_tag;
    using iterator_concept = std::forward_iterator_tag;

    constexpr extended_storage_iterator()
        : it{} {}

    constexpr extended_storage_iterator(iterator_type base, Other... other)
        : it{base, other...} {}

    template<typename... Args, typename = std::enable_if_t<(!std::is_same_v<Other, Args> && ...) && (std::is_constructible_v<Other, Args> && ...)>>
    constexpr extended_storage_iterator(const extended_storage_iterator<It, Args...> &other)
        : it{other.it} {}

    constexpr extended_storage_iterator &operator++() noexcept {
        return ++std::get<It>(it), (++std::get<Other>(it), ...), *this;
    }

    constexpr extended_storage_iterator operator++(int) noexcept {
        const extended_storage_iterator orig = *this;
        return ++(*this), orig;
    }

    [[nodiscard]] constexpr pointer operator->() const noexcept {
        return operator*();
    }

    [[nodiscard]] constexpr reference operator*() const noexcept {
        return {*std::get<It>(it), *std::get<Other>(it)...};
    }

    [[nodiscard]] constexpr iterator_type base() const noexcept {
        return std::get<It>(it);
    }

    template<typename... Lhs, typename... Rhs>
    friend constexpr bool operator==(const extended_storage_iterator<Lhs...> &, const extended_storage_iterator<Rhs...> &) noexcept;

private:
    std::tuple<It, Other...> it;
};

template<typename... Lhs, typename... Rhs>
[[nodiscard]] constexpr bool operator==(const extended_storage_iterator<Lhs...> &lhs, const extended_storage_iterator<Rhs...> &rhs) noexcept {
    return std::get<0>(lhs.it) == std::get<0>(rhs.it);
}

template<typename... Lhs, typename... Rhs>
[[nodiscard]] constexpr bool operator!=(const extended_storage_iterator<Lhs...> &lhs, const extended_storage_iterator<Rhs...> &rhs) noexcept {
    return !(lhs == rhs);
}

} // namespace internal
/*! @endcond */

/**
 * @brief Storage implementation.
 *
 * Internal data structures arrange elements to maximize performance. There are
 * no guarantees that objects are returned in the insertion order when iterate
 * a storage. Do not make assumption on the order in any case.
 *
 * @warning
 * Empty types aren't explicitly instantiated. Therefore, many of the functions
 * normally available for non-empty types will not be available for empty ones.
 *
 * @tparam Type Element type.
 * @tparam Entity A valid entity type.
 * @tparam Allocator Type of allocator used to manage memory and elements.
 */
template<typename Type, typename Entity, typename Allocator, typename>
class basic_storage: public basic_sparse_set<Entity, typename std::allocator_traits<Allocator>::template rebind_alloc<Entity>> {
    using alloc_traits = std::allocator_traits<Allocator>;
    static_assert(std::is_same_v<typename alloc_traits::value_type, Type>, "Invalid value type");
    using container_type = std::vector<typename alloc_traits::pointer, typename alloc_traits::template rebind_alloc<typename alloc_traits::pointer>>;
    using underlying_type = basic_sparse_set<Entity, typename alloc_traits::template rebind_alloc<Entity>>;
    using underlying_iterator = typename underlying_type::basic_iterator;
    using traits_type = component_traits<Type>;

    [[nodiscard]] auto &element_at(const std::size_t pos) const {
        return payload[pos / traits_type::page_size][fast_mod(pos, traits_type::page_size)];
    }

    auto assure_at_least(const std::size_t pos) {
        const auto idx = pos / traits_type::page_size;

        if(!(idx < payload.size())) {
            auto curr = payload.size();
            allocator_type allocator{get_allocator()};
            payload.resize(idx + 1u, nullptr);

            ENTT_TRY {
                for(const auto last = payload.size(); curr < last; ++curr) {
                    payload[curr] = alloc_traits::allocate(allocator, traits_type::page_size);
                }
            }
            ENTT_CATCH {
                payload.resize(curr);
                ENTT_THROW;
            }
        }

        return payload[idx] + fast_mod(pos, traits_type::page_size);
    }

    template<typename... Args>
    auto emplace_element(const Entity entt, const bool force_back, Args &&...args) {
        const auto it = base_type::try_emplace(entt, force_back);

        ENTT_TRY {
            auto *elem = to_address(assure_at_least(static_cast<size_type>(it.index())));
            entt::uninitialized_construct_using_allocator(elem, get_allocator(), std::forward<Args>(args)...);
        }
        ENTT_CATCH {
            base_type::pop(it, it + 1u);
            ENTT_THROW;
        }

        return it;
    }

    void shrink_to_size(const std::size_t sz) {
        const auto from = (sz + traits_type::page_size - 1u) / traits_type::page_size;
        allocator_type allocator{get_allocator()};

        for(auto pos = sz, length = base_type::size(); pos < length; ++pos) {
            if constexpr(traits_type::in_place_delete) {
                if(base_type::data()[pos] != tombstone) {
                    alloc_traits::destroy(allocator, std::addressof(element_at(pos)));
                }
            } else {
                alloc_traits::destroy(allocator, std::addressof(element_at(pos)));
            }
        }

        for(auto pos = from, last = payload.size(); pos < last; ++pos) {
            alloc_traits::deallocate(allocator, payload[pos], traits_type::page_size);
        }

        payload.resize(from);
        payload.shrink_to_fit();
    }

    void swap_at(const std::size_t lhs, const std::size_t rhs) {
        using std::swap;
        swap(element_at(lhs), element_at(rhs));
    }

    void move_to(const std::size_t lhs, const std::size_t rhs) {
        auto &elem = element_at(lhs);
        allocator_type allocator{get_allocator()};
        entt::uninitialized_construct_using_allocator(to_address(assure_at_least(rhs)), allocator, std::move(elem));
        alloc_traits::destroy(allocator, std::addressof(elem));
    }

private:
    [[nodiscard]] const void *get_at(const std::size_t pos) const final {
        return std::addressof(element_at(pos));
    }

    void swap_or_move([[maybe_unused]] const std::size_t from, [[maybe_unused]] const std::size_t to) override {
        static constexpr bool is_pinned_type = !(std::is_move_constructible_v<Type> && std::is_move_assignable_v<Type>);
        // use a runtime value to avoid compile-time suppression that drives the code coverage tool crazy
        ENTT_ASSERT((from + 1u) && !is_pinned_type, "Pinned type");

        if constexpr(!is_pinned_type) {
            if constexpr(traits_type::in_place_delete) {
                (base_type::operator[](to) == tombstone) ? move_to(from, to) : swap_at(from, to);
            } else {
                swap_at(from, to);
            }
        }
    }

protected:
    /**
     * @brief Erases entities from a storage.
     * @param first An iterator to the first element of the range of entities.
     * @param last An iterator past the last element of the range of entities.
     */
    void pop(underlying_iterator first, underlying_iterator last) override {
        for(allocator_type allocator{get_allocator()}; first != last; ++first) {
            // cannot use first.index() because it would break with cross iterators
            auto &elem = element_at(base_type::index(*first));

            if constexpr(traits_type::in_place_delete) {
                base_type::in_place_pop(first);
                alloc_traits::destroy(allocator, std::addressof(elem));
            } else {
                auto &other = element_at(base_type::size() - 1u);
                // destroying on exit allows reentrant destructors
                [[maybe_unused]] auto unused = std::exchange(elem, std::move(other));
                alloc_traits::destroy(allocator, std::addressof(other));
                base_type::swap_and_pop(first);
            }
        }
    }

    /*! @brief Erases all entities of a storage. */
    void pop_all() override {
        allocator_type allocator{get_allocator()};

        for(auto first = base_type::begin(); !(first.index() < 0); ++first) {
            if constexpr(traits_type::in_place_delete) {
                if(*first != tombstone) {
                    base_type::in_place_pop(first);
                    alloc_traits::destroy(allocator, std::addressof(element_at(static_cast<size_type>(first.index()))));
                }
            } else {
                base_type::swap_and_pop(first);
                alloc_traits::destroy(allocator, std::addressof(element_at(static_cast<size_type>(first.index()))));
            }
        }
    }

    /**
     * @brief Assigns an entity to a storage.
     * @param entt A valid identifier.
     * @param value Optional opaque value.
     * @param force_back Force back insertion.
     * @return Iterator pointing to the emplaced element.
     */
    underlying_iterator try_emplace([[maybe_unused]] const Entity entt, [[maybe_unused]] const bool force_back, const void *value) override {
        if(value != nullptr) {
            if constexpr(std::is_copy_constructible_v<element_type>) {
                return emplace_element(entt, force_back, *static_cast<const element_type *>(value));
            } else {
                return base_type::end();
            }
        } else {
            if constexpr(std::is_default_constructible_v<element_type>) {
                return emplace_element(entt, force_back);
            } else {
                return base_type::end();
            }
        }
    }

public:
    /*! @brief Allocator type. */
    using allocator_type = Allocator;
    /*! @brief Base type. */
    using base_type = underlying_type;
    /*! @brief Element type. */
    using element_type = Type;
    /*! @brief Type of the objects assigned to entities. */
    using value_type = element_type;
    /*! @brief Underlying entity identifier. */
    using entity_type = Entity;
    /*! @brief Unsigned integer type. */
    using size_type = std::size_t;
    /*! @brief Pointer type to contained elements. */
    using pointer = typename container_type::pointer;
    /*! @brief Constant pointer type to contained elements. */
    using const_pointer = typename alloc_traits::template rebind_traits<typename alloc_traits::const_pointer>::const_pointer;
    /*! @brief Random access iterator type. */
    using iterator = internal::storage_iterator<container_type>;
    /*! @brief Constant random access iterator type. */
    using const_iterator = internal::storage_iterator<const container_type>;
    /*! @brief Reverse iterator type. */
    using reverse_iterator = std::reverse_iterator<iterator>;
    /*! @brief Constant reverse iterator type. */
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;
    /*! @brief Extended iterable storage proxy. */
    using iterable = iterable_adaptor<internal::extended_storage_iterator<typename base_type::iterator, iterator>>;
    /*! @brief Constant extended iterable storage proxy. */
    using const_iterable = iterable_adaptor<internal::extended_storage_iterator<typename base_type::const_iterator, const_iterator>>;
    /*! @brief Extended reverse iterable storage proxy. */
    using reverse_iterable = iterable_adaptor<internal::extended_storage_iterator<typename base_type::reverse_iterator, reverse_iterator>>;
    /*! @brief Constant extended reverse iterable storage proxy. */
    using const_reverse_iterable = iterable_adaptor<internal::extended_storage_iterator<typename base_type::const_reverse_iterator, const_reverse_iterator>>;
    /*! @brief Storage deletion policy. */
    static constexpr deletion_policy storage_policy{traits_type::in_place_delete};

    /*! @brief Default constructor. */
    basic_storage()
        : basic_storage{allocator_type{}} {}

    /**
     * @brief Constructs an empty storage with a given allocator.
     * @param allocator The allocator to use.
     */
    explicit basic_storage(const allocator_type &allocator)
        : base_type{type_id<element_type>(), storage_policy, allocator},
          payload{allocator} {}

    /*! @brief Default copy constructor, deleted on purpose. */
    basic_storage(const basic_storage &) = delete;

    /**
     * @brief Move constructor.
     * @param other The instance to move from.
     */
    // NOLINTBEGIN(bugprone-use-after-move)
    basic_storage(basic_storage &&other) noexcept
        : base_type{std::move(other)},
          payload{std::move(other.payload)} {}
    // NOLINTEND(bugprone-use-after-move)

    /**
     * @brief Allocator-extended move constructor.
     * @param other The instance to move from.
     * @param allocator The allocator to use.
     */
    // NOLINTBEGIN(bugprone-use-after-move)
    basic_storage(basic_storage &&other, const allocator_type &allocator)
        : base_type{std::move(other), allocator},
          payload{std::move(other.payload), allocator} {
        ENTT_ASSERT(alloc_traits::is_always_equal::value || get_allocator() == other.get_allocator(), "Copying a storage is not allowed");
    }
    // NOLINTEND(bugprone-use-after-move)

    /*! @brief Default destructor. */
    // NOLINTNEXTLINE(bugprone-exception-escape)
    ~basic_storage() override {
        shrink_to_size(0u);
    }

    /**
     * @brief Default copy assignment operator, deleted on purpose.
     * @return This storage.
     */
    basic_storage &operator=(const basic_storage &) = delete;

    /**
     * @brief Move assignment operator.
     * @param other The instance to move from.
     * @return This storage.
     */
    basic_storage &operator=(basic_storage &&other) noexcept {
        ENTT_ASSERT(alloc_traits::is_always_equal::value || get_allocator() == other.get_allocator(), "Copying a storage is not allowed");
        swap(other);
        return *this;
    }

    /**
     * @brief Exchanges the contents with those of a given storage.
     * @param other Storage to exchange the content with.
     */
    void swap(basic_storage &other) noexcept {
        using std::swap;
        swap(payload, other.payload);
        base_type::swap(other);
    }

    /**
     * @brief Returns the associated allocator.
     * @return The associated allocator.
     */
    [[nodiscard]] constexpr allocator_type get_allocator() const noexcept {
        return payload.get_allocator();
    }

    /**
     * @brief Increases the capacity of a storage.
     *
     * If the new capacity is greater than the current capacity, new storage is
     * allocated, otherwise the method does nothing.
     *
     * @param cap Desired capacity.
     */
    void reserve(const size_type cap) override {
        if(cap != 0u) {
            base_type::reserve(cap);
            assure_at_least(cap - 1u);
        }
    }

    /**
     * @brief Returns the number of elements that a storage has currently
     * allocated space for.
     * @return Capacity of the storage.
     */
    [[nodiscard]] size_type capacity() const noexcept override {
        return payload.size() * traits_type::page_size;
    }

    /*! @brief Requests the removal of unused capacity. */
    void shrink_to_fit() override {
        base_type::shrink_to_fit();
        shrink_to_size(base_type::size());
    }

    /**
     * @brief Direct access to the array of objects.
     * @return A pointer to the array of objects.
     */
    [[nodiscard]] const_pointer raw() const noexcept {
        return payload.data();
    }

    /*! @copydoc raw */
    [[nodiscard]] pointer raw() noexcept {
        return payload.data();
    }

    /**
     * @brief Returns an iterator to the beginning.
     *
     * If the storage is empty, the returned iterator will be equal to `end()`.
     *
     * @return An iterator to the first instance of the internal array.
     */
    [[nodiscard]] const_iterator cbegin() const noexcept {
        const auto pos = static_cast<typename iterator::difference_type>(base_type::size());
        return const_iterator{&payload, pos};
    }

    /*! @copydoc cbegin */
    [[nodiscard]] const_iterator begin() const noexcept {
        return cbegin();
    }

    /*! @copydoc begin */
    [[nodiscard]] iterator begin() noexcept {
        const auto pos = static_cast<typename iterator::difference_type>(base_type::size());
        return iterator{&payload, pos};
    }

    /**
     * @brief Returns an iterator to the end.
     * @return An iterator to the element following the last instance of the
     * internal array.
     */
    [[nodiscard]] const_iterator cend() const noexcept {
        return const_iterator{&payload, {}};
    }

    /*! @copydoc cend */
    [[nodiscard]] const_iterator end() const noexcept {
        return cend();
    }

    /*! @copydoc end */
    [[nodiscard]] iterator end() noexcept {
        return iterator{&payload, {}};
    }

    /**
     * @brief Returns a reverse iterator to the beginning.
     *
     * If the storage is empty, the returned iterator will be equal to `rend()`.
     *
     * @return An iterator to the first instance of the reversed internal array.
     */
    [[nodiscard]] const_reverse_iterator crbegin() const noexcept {
        return std::make_reverse_iterator(cend());
    }

    /*! @copydoc crbegin */
    [[nodiscard]] const_reverse_iterator rbegin() const noexcept {
        return crbegin();
    }

    /*! @copydoc rbegin */
    [[nodiscard]] reverse_iterator rbegin() noexcept {
        return std::make_reverse_iterator(end());
    }

    /**
     * @brief Returns a reverse iterator to the end.
     * @return An iterator to the element following the last instance of the
     * reversed internal array.
     */
    [[nodiscard]] const_reverse_iterator crend() const noexcept {
        return std::make_reverse_iterator(cbegin());
    }

    /*! @copydoc crend */
    [[nodiscard]] const_reverse_iterator rend() const noexcept {
        return crend();
    }

    /*! @copydoc rend */
    [[nodiscard]] reverse_iterator rend() noexcept {
        return std::make_reverse_iterator(begin());
    }

    /**
     * @brief Returns the object assigned to an entity.
     *
     * @warning
     * Attempting to use an entity that doesn't belong to the storage results in
     * undefined behavior.
     *
     * @param entt A valid identifier.
     * @return The object assigned to the entity.
     */
    [[nodiscard]] const value_type &get(const entity_type entt) const noexcept {
        return element_at(base_type::index(entt));
    }

    /*! @copydoc get */
    [[nodiscard]] value_type &get(const entity_type entt) noexcept {
        return const_cast<value_type &>(std::as_const(*this).get(entt));
    }

    /**
     * @brief Returns the object assigned to an entity as a tuple.
     * @param entt A valid identifier.
     * @return The object assigned to the entity as a tuple.
     */
    [[nodiscard]] std::tuple<const value_type &> get_as_tuple(const entity_type entt) const noexcept {
        return std::forward_as_tuple(get(entt));
    }

    /*! @copydoc get_as_tuple */
    [[nodiscard]] std::tuple<value_type &> get_as_tuple(const entity_type entt) noexcept {
        return std::forward_as_tuple(get(entt));
    }

    /**
     * @brief Assigns an entity to a storage and constructs its object.
     *
     * @warning
     * Attempting to use an entity that already belongs to the storage results
     * in undefined behavior.
     *
     * @tparam Args Types of arguments to use to construct the object.
     * @param entt A valid identifier.
     * @param args Parameters to use to construct an object for the entity.
     * @return A reference to the newly created object.
     */
    template<typename... Args>
    value_type &emplace(const entity_type entt, Args &&...args) {
        if constexpr(std::is_aggregate_v<value_type> && (sizeof...(Args) != 0u || !std::is_default_constructible_v<value_type>)) {
            const auto it = emplace_element(entt, false, Type{std::forward<Args>(args)...});
            return element_at(static_cast<size_type>(it.index()));
        } else {
            const auto it = emplace_element(entt, false, std::forward<Args>(args)...);
            return element_at(static_cast<size_type>(it.index()));
        }
    }

    /**
     * @brief Updates the instance assigned to a given entity in-place.
     * @tparam Func Types of the function objects to invoke.
     * @param entt A valid identifier.
     * @param func Valid function objects.
     * @return A reference to the updated instance.
     */
    template<typename... Func>
    value_type &patch(const entity_type entt, Func &&...func) {
        const auto idx = base_type::index(entt);
        auto &elem = element_at(idx);
        (std::forward<Func>(func)(elem), ...);
        return elem;
    }

    /**
     * @brief Assigns one or more entities to a storage and constructs their
     * objects from a given instance.
     *
     * @warning
     * Attempting to assign an entity that already belongs to the storage
     * results in undefined behavior.
     *
     * @tparam It Type of input iterator.
     * @param first An iterator to the first element of the range of entities.
     * @param last An iterator past the last element of the range of entities.
     * @param value An instance of the object to construct.
     * @return Iterator pointing to the first element inserted, if any.
     */
    template<typename It>
    iterator insert(It first, It last, const value_type &value = {}) {
        for(; first != last; ++first) {
            emplace_element(*first, true, value);
        }

        return begin();
    }

    /**
     * @brief Assigns one or more entities to a storage and constructs their
     * objects from a given range.
     *
     * @sa construct
     *
     * @tparam EIt Type of input iterator.
     * @tparam CIt Type of input iterator.
     * @param first An iterator to the first element of the range of entities.
     * @param last An iterator past the last element of the range of entities.
     * @param from An iterator to the first element of the range of objects.
     * @return Iterator pointing to the first element inserted, if any.
     */
    template<typename EIt, typename CIt, typename = std::enable_if_t<std::is_same_v<typename std::iterator_traits<CIt>::value_type, value_type>>>
    iterator insert(EIt first, EIt last, CIt from) {
        for(; first != last; ++first, ++from) {
            emplace_element(*first, true, *from);
        }

        return begin();
    }

    /**
     * @brief Returns an iterable object to use to _visit_ a storage.
     *
     * The iterable object returns a tuple that contains the current entity and
     * a reference to its element.
     *
     * @return An iterable object to use to _visit_ the storage.
     */
    [[nodiscard]] iterable each() noexcept {
        return iterable{{base_type::begin(), begin()}, {base_type::end(), end()}};
    }

    /*! @copydoc each */
    [[nodiscard]] const_iterable each() const noexcept {
        return const_iterable{{base_type::cbegin(), cbegin()}, {base_type::cend(), cend()}};
    }

    /**
     * @brief Returns a reverse iterable object to use to _visit_ a storage.
     *
     * @sa each
     *
     * @return A reverse iterable object to use to _visit_ the storage.
     */
    [[nodiscard]] reverse_iterable reach() noexcept {
        return reverse_iterable{{base_type::rbegin(), rbegin()}, {base_type::rend(), rend()}};
    }

    /*! @copydoc reach */
    [[nodiscard]] const_reverse_iterable reach() const noexcept {
        return const_reverse_iterable{{base_type::crbegin(), crbegin()}, {base_type::crend(), crend()}};
    }

private:
    container_type payload;
};

/*! @copydoc basic_storage */
template<typename Type, typename Entity, typename Allocator>
class basic_storage<Type, Entity, Allocator, std::enable_if_t<component_traits<Type>::page_size == 0u>>
    : public basic_sparse_set<Entity, typename std::allocator_traits<Allocator>::template rebind_alloc<Entity>> {
    using alloc_traits = std::allocator_traits<Allocator>;
    static_assert(std::is_same_v<typename alloc_traits::value_type, Type>, "Invalid value type");
    using traits_type = component_traits<Type>;

public:
    /*! @brief Allocator type. */
    using allocator_type = Allocator;
    /*! @brief Base type. */
    using base_type = basic_sparse_set<Entity, typename alloc_traits::template rebind_alloc<Entity>>;
    /*! @brief Element type. */
    using element_type = Type;
    /*! @brief Type of the objects assigned to entities. */
    using value_type = void;
    /*! @brief Underlying entity identifier. */
    using entity_type = Entity;
    /*! @brief Unsigned integer type. */
    using size_type = std::size_t;
    /*! @brief Extended iterable storage proxy. */
    using iterable = iterable_adaptor<internal::extended_storage_iterator<typename base_type::iterator>>;
    /*! @brief Constant extended iterable storage proxy. */
    using const_iterable = iterable_adaptor<internal::extended_storage_iterator<typename base_type::const_iterator>>;
    /*! @brief Extended reverse iterable storage proxy. */
    using reverse_iterable = iterable_adaptor<internal::extended_storage_iterator<typename base_type::reverse_iterator>>;
    /*! @brief Constant extended reverse iterable storage proxy. */
    using const_reverse_iterable = iterable_adaptor<internal::extended_storage_iterator<typename base_type::const_reverse_iterator>>;
    /*! @brief Storage deletion policy. */
    static constexpr deletion_policy storage_policy{traits_type::in_place_delete};

    /*! @brief Default constructor. */
    basic_storage()
        : basic_storage{allocator_type{}} {}

    /**
     * @brief Constructs an empty container with a given allocator.
     * @param allocator The allocator to use.
     */
    explicit basic_storage(const allocator_type &allocator)
        : base_type{type_id<element_type>(), storage_policy, allocator} {}

    /*! @brief Default copy constructor, deleted on purpose. */
    basic_storage(const basic_storage &) = delete;

    /**
     * @brief Move constructor.
     * @param other The instance to move from.
     */
    basic_storage(basic_storage &&other) noexcept = default;

    /**
     * @brief Allocator-extended move constructor.
     * @param other The instance to move from.
     * @param allocator The allocator to use.
     */
    basic_storage(basic_storage &&other, const allocator_type &allocator)
        : base_type{std::move(other), allocator} {}

    /*! @brief Default destructor. */
    ~basic_storage() override = default;

    /**
     * @brief Default copy assignment operator, deleted on purpose.
     * @return This storage.
     */
    basic_storage &operator=(const basic_storage &) = delete;

    /**
     * @brief Move assignment operator.
     * @param other The instance to move from.
     * @return This storage.
     */
    basic_storage &operator=(basic_storage &&other) noexcept = default;

    /**
     * @brief Returns the associated allocator.
     * @return The associated allocator.
     */
    [[nodiscard]] constexpr allocator_type get_allocator() const noexcept {
        // std::allocator<void> has no cross constructors (waiting for C++20)
        if constexpr(std::is_void_v<element_type> && !std::is_constructible_v<allocator_type, typename base_type::allocator_type>) {
            return allocator_type{};
        } else {
            return allocator_type{base_type::get_allocator()};
        }
    }

    /**
     * @brief Returns the object assigned to an entity, that is `void`.
     *
     * @warning
     * Attempting to use an entity that doesn't belong to the storage results in
     * undefined behavior.
     *
     * @param entt A valid identifier.
     */
    void get([[maybe_unused]] const entity_type entt) const noexcept {
        ENTT_ASSERT(base_type::contains(entt), "Invalid entity");
    }

    /**
     * @brief Returns an empty tuple.
     * @param entt A valid identifier.
     * @return Returns an empty tuple.
     */
    [[nodiscard]] std::tuple<> get_as_tuple([[maybe_unused]] const entity_type entt) const noexcept {
        ENTT_ASSERT(base_type::contains(entt), "Invalid entity");
        return std::tuple{};
    }

    /**
     * @brief Assigns an entity to a storage and constructs its object.
     *
     * @warning
     * Attempting to use an entity that already belongs to the storage results
     * in undefined behavior.
     *
     * @tparam Args Types of arguments to use to construct the object.
     * @param entt A valid identifier.
     */
    template<typename... Args>
    // NOLINTNEXTLINE(cppcoreguidelines-missing-std-forward)
    void emplace(const entity_type entt, Args &&...) {
        base_type::try_emplace(entt, false);
    }

    /**
     * @brief Updates the instance assigned to a given entity in-place.
     * @tparam Func Types of the function objects to invoke.
     * @param entt A valid identifier.
     * @param func Valid function objects.
     */
    template<typename... Func>
    void patch([[maybe_unused]] const entity_type entt, Func &&...func) {
        ENTT_ASSERT(base_type::contains(entt), "Invalid entity");
        (std::forward<Func>(func)(), ...);
    }

    /**
     * @brief Assigns entities to a storage.
     * @tparam It Type of input iterator.
     * @tparam Args Types of optional arguments.
     * @param first An iterator to the first element of the range of entities.
     * @param last An iterator past the last element of the range of entities.
     */
    template<typename It, typename... Args>
    // NOLINTNEXTLINE(cppcoreguidelines-missing-std-forward)
    void insert(It first, It last, Args &&...) {
        for(; first != last; ++first) {
            base_type::try_emplace(*first, true);
        }
    }

    /**
     * @brief Returns an iterable object to use to _visit_ a storage.
     *
     * The iterable object returns a tuple that contains the current entity.
     *
     * @return An iterable object to use to _visit_ the storage.
     */
    [[nodiscard]] iterable each() noexcept {
        return iterable{base_type::begin(), base_type::end()};
    }

    /*! @copydoc each */
    [[nodiscard]] const_iterable each() const noexcept {
        return const_iterable{base_type::cbegin(), base_type::cend()};
    }

    /**
     * @brief Returns a reverse iterable object to use to _visit_ a storage.
     *
     * @sa each
     *
     * @return A reverse iterable object to use to _visit_ the storage.
     */
    [[nodiscard]] reverse_iterable reach() noexcept {
        return reverse_iterable{{base_type::rbegin()}, {base_type::rend()}};
    }

    /*! @copydoc reach */
    [[nodiscard]] const_reverse_iterable reach() const noexcept {
        return const_reverse_iterable{{base_type::crbegin()}, {base_type::crend()}};
    }
};

/**
 * @brief Swap-only entity storage specialization.
 * @tparam Entity A valid entity type.
 * @tparam Allocator Type of allocator used to manage memory and elements.
 */
template<typename Entity, typename Allocator>
class basic_storage<Entity, Entity, Allocator>
    : public basic_sparse_set<Entity, Allocator> {
    using alloc_traits = std::allocator_traits<Allocator>;
    static_assert(std::is_same_v<typename alloc_traits::value_type, Entity>, "Invalid value type");
    using underlying_iterator = typename basic_sparse_set<Entity, Allocator>::basic_iterator;
    using traits_type = entt_traits<Entity>;

    auto next() noexcept {
        entity_type entt = null;

        do {
            ENTT_ASSERT(placeholder < traits_type::to_entity(null), "No more entities available");
            entt = traits_type::combine(static_cast<typename traits_type::entity_type>(placeholder++), {});
        } while(base_type::current(entt) != traits_type::to_version(tombstone) && entt != null);

        return entt;
    }

protected:
    /*! @brief Erases all entities of a storage. */
    void pop_all() override {
        base_type::pop_all();
        placeholder = {};
    }

    /**
     * @brief Assigns an entity to a storage.
     * @param hint A valid identifier.
     * @return Iterator pointing to the emplaced element.
     */
    underlying_iterator try_emplace(const Entity hint, const bool, const void *) override {
        return base_type::find(generate(hint));
    }

public:
    /*! @brief Allocator type. */
    using allocator_type = Allocator;
    /*! @brief Base type. */
    using base_type = basic_sparse_set<Entity, Allocator>;
    /*! @brief Element type. */
    using element_type = Entity;
    /*! @brief Type of the objects assigned to entities. */
    using value_type = void;
    /*! @brief Underlying entity identifier. */
    using entity_type = Entity;
    /*! @brief Unsigned integer type. */
    using size_type = std::size_t;
    /*! @brief Extended iterable storage proxy. */
    using iterable = iterable_adaptor<internal::extended_storage_iterator<typename base_type::iterator>>;
    /*! @brief Constant extended iterable storage proxy. */
    using const_iterable = iterable_adaptor<internal::extended_storage_iterator<typename base_type::const_iterator>>;
    /*! @brief Extended reverse iterable storage proxy. */
    using reverse_iterable = iterable_adaptor<internal::extended_storage_iterator<typename base_type::reverse_iterator>>;
    /*! @brief Constant extended reverse iterable storage proxy. */
    using const_reverse_iterable = iterable_adaptor<internal::extended_storage_iterator<typename base_type::const_reverse_iterator>>;
    /*! @brief Storage deletion policy. */
    static constexpr deletion_policy storage_policy = deletion_policy::swap_only;

    /*! @brief Default constructor. */
    basic_storage()
        : basic_storage{allocator_type{}} {
    }

    /**
     * @brief Constructs an empty container with a given allocator.
     * @param allocator The allocator to use.
     */
    explicit basic_storage(const allocator_type &allocator)
        : base_type{type_id<void>(), storage_policy, allocator} {}

    /*! @brief Default copy constructor, deleted on purpose. */
    basic_storage(const basic_storage &) = delete;

    /**
     * @brief Move constructor.
     * @param other The instance to move from.
     */
    // NOLINTBEGIN(bugprone-use-after-move)
    basic_storage(basic_storage &&other) noexcept
        : base_type{std::move(other)},
          placeholder{other.placeholder} {}
    // NOLINTEND(bugprone-use-after-move)

    /**
     * @brief Allocator-extended move constructor.
     * @param other The instance to move from.
     * @param allocator The allocator to use.
     */
    // NOLINTBEGIN(bugprone-use-after-move)
    basic_storage(basic_storage &&other, const allocator_type &allocator)
        : base_type{std::move(other), allocator},
          placeholder{other.placeholder} {}
    // NOLINTEND(bugprone-use-after-move)

    /*! @brief Default destructor. */
    ~basic_storage() override = default;

    /**
     * @brief Default copy assignment operator, deleted on purpose.
     * @return This storage.
     */
    basic_storage &operator=(const basic_storage &) = delete;

    /**
     * @brief Move assignment operator.
     * @param other The instance to move from.
     * @return This storage.
     */
    basic_storage &operator=(basic_storage &&other) noexcept {
        placeholder = other.placeholder;
        base_type::operator=(std::move(other));
        return *this;
    }

    /**
     * @brief Returns the object assigned to an entity, that is `void`.
     *
     * @warning
     * Attempting to use an entity that doesn't belong to the storage results in
     * undefined behavior.
     *
     * @param entt A valid identifier.
     */
    void get([[maybe_unused]] const entity_type entt) const noexcept {
        ENTT_ASSERT(base_type::index(entt) < base_type::free_list(), "The requested entity is not a live one");
    }

    /**
     * @brief Returns an empty tuple.
     * @param entt A valid identifier.
     * @return Returns an empty tuple.
     */
    [[nodiscard]] std::tuple<> get_as_tuple([[maybe_unused]] const entity_type entt) const noexcept {
        ENTT_ASSERT(base_type::index(entt) < base_type::free_list(), "The requested entity is not a live one");
        return std::tuple{};
    }

    /**
     * @brief Creates a new identifier or recycles a destroyed one.
     * @return A valid identifier.
     */
    entity_type generate() {
        const auto len = base_type::free_list();
        const auto entt = (len == base_type::size()) ? next() : base_type::data()[len];
        return *base_type::try_emplace(entt, true);
    }

    /**
     * @brief Creates a new identifier or recycles a destroyed one.
     *
     * If the requested identifier isn't in use, the suggested one is used.
     * Otherwise, a new identifier is returned.
     *
     * @param hint Required identifier.
     * @return A valid identifier.
     */
    entity_type generate(const entity_type hint) {
        if(hint != null && hint != tombstone) {
            if(const auto curr = traits_type::construct(traits_type::to_entity(hint), base_type::current(hint)); curr == tombstone || !(base_type::index(curr) < base_type::free_list())) {
                return *base_type::try_emplace(hint, true);
            }
        }

        return generate();
    }

    /**
     * @brief Assigns each element in a range an identifier.
     * @tparam It Type of mutable forward iterator.
     * @param first An iterator to the first element of the range to generate.
     * @param last An iterator past the last element of the range to generate.
     */
    template<typename It>
    void generate(It first, It last) {
        for(const auto sz = base_type::size(); first != last && base_type::free_list() != sz; ++first) {
            *first = *base_type::try_emplace(base_type::data()[base_type::free_list()], true);
        }

        for(; first != last; ++first) {
            *first = *base_type::try_emplace(next(), true);
        }
    }

    /**
     * @brief Creates a new identifier or recycles a destroyed one.
     * @return A valid identifier.
     */
    [[deprecated("use ::generate() instead")]] entity_type emplace() {
        return generate();
    }

    /**
     * @brief Creates a new identifier or recycles a destroyed one.
     *
     * If the requested identifier isn't in use, the suggested one is used.
     * Otherwise, a new identifier is returned.
     *
     * @param hint Required identifier.
     * @return A valid identifier.
     */
    [[deprecated("use ::generate(hint) instead")]] entity_type emplace(const entity_type hint) {
        return generate(hint);
    }

    /**
     * @brief Updates a given identifier.
     * @tparam Func Types of the function objects to invoke.
     * @param entt A valid identifier.
     * @param func Valid function objects.
     */
    template<typename... Func>
    void patch([[maybe_unused]] const entity_type entt, Func &&...func) {
        ENTT_ASSERT(base_type::index(entt) < base_type::free_list(), "The requested entity is not a live one");
        (std::forward<Func>(func)(), ...);
    }

    /**
     * @brief Assigns each element in a range an identifier.
     * @tparam It Type of mutable forward iterator.
     * @param first An iterator to the first element of the range to generate.
     * @param last An iterator past the last element of the range to generate.
     */
    template<typename It>
    [[deprecated("use ::generate(first, last) instead")]] void insert(It first, It last) {
        generate(std::move(first), std::move(last));
    }

    /**
     * @brief Returns an iterable object to use to _visit_ a storage.
     *
     * The iterable object returns a tuple that contains the current entity.
     *
     * @return An iterable object to use to _visit_ the storage.
     */
    [[nodiscard]] iterable each() noexcept {
        return std::as_const(*this).each();
    }

    /*! @copydoc each */
    [[nodiscard]] const_iterable each() const noexcept {
        const auto it = base_type::cend();
        return const_iterable{it - base_type::free_list(), it};
    }

    /**
     * @brief Returns a reverse iterable object to use to _visit_ a storage.
     *
     * @sa each
     *
     * @return A reverse iterable object to use to _visit_ the storage.
     */
    [[nodiscard]] reverse_iterable reach() noexcept {
        return std::as_const(*this).reach();
    }

    /*! @copydoc reach */
    [[nodiscard]] const_reverse_iterable reach() const noexcept {
        const auto it = base_type::crbegin();
        return const_reverse_iterable{it, it + base_type::free_list()};
    }

private:
    size_type placeholder{};
};

} // namespace entt

#endif

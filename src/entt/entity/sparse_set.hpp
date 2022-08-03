#ifndef ENTT_ENTITY_SPARSE_SET_HPP
#define ENTT_ENTITY_SPARSE_SET_HPP

#include <cstddef>
#include <iterator>
#include <memory>
#include <type_traits>
#include <utility>
#include <vector>
#include "../config/config.h"
#include "../core/algorithm.hpp"
#include "../core/any.hpp"
#include "../core/memory.hpp"
#include "../core/type_info.hpp"
#include "entity.hpp"
#include "fwd.hpp"

namespace entt {

/**
 * @cond TURN_OFF_DOXYGEN
 * Internal details not to be documented.
 */

namespace internal {

template<typename Container>
struct sparse_set_iterator final {
    using value_type = typename Container::value_type;
    using pointer = typename Container::const_pointer;
    using reference = typename Container::const_reference;
    using difference_type = typename Container::difference_type;
    using iterator_category = std::random_access_iterator_tag;

    constexpr sparse_set_iterator() noexcept
        : packed{},
          offset{} {}

    constexpr sparse_set_iterator(const Container &ref, const difference_type idx) noexcept
        : packed{std::addressof(ref)},
          offset{idx} {}

    constexpr sparse_set_iterator &operator++() noexcept {
        return --offset, *this;
    }

    constexpr sparse_set_iterator operator++(int) noexcept {
        sparse_set_iterator orig = *this;
        return ++(*this), orig;
    }

    constexpr sparse_set_iterator &operator--() noexcept {
        return ++offset, *this;
    }

    constexpr sparse_set_iterator operator--(int) noexcept {
        sparse_set_iterator orig = *this;
        return operator--(), orig;
    }

    constexpr sparse_set_iterator &operator+=(const difference_type value) noexcept {
        offset -= value;
        return *this;
    }

    constexpr sparse_set_iterator operator+(const difference_type value) const noexcept {
        sparse_set_iterator copy = *this;
        return (copy += value);
    }

    constexpr sparse_set_iterator &operator-=(const difference_type value) noexcept {
        return (*this += -value);
    }

    constexpr sparse_set_iterator operator-(const difference_type value) const noexcept {
        return (*this + -value);
    }

    [[nodiscard]] constexpr reference operator[](const difference_type value) const noexcept {
        return packed->data()[index() - value];
    }

    [[nodiscard]] constexpr pointer operator->() const noexcept {
        return packed->data() + index();
    }

    [[nodiscard]] constexpr reference operator*() const noexcept {
        return *operator->();
    }

    [[nodiscard]] constexpr difference_type index() const noexcept {
        return offset - 1;
    }

private:
    const Container *packed;
    difference_type offset;
};

template<typename Type, typename Other>
[[nodiscard]] constexpr std::ptrdiff_t operator-(const sparse_set_iterator<Type> &lhs, const sparse_set_iterator<Other> &rhs) noexcept {
    return rhs.index() - lhs.index();
}

template<typename Type, typename Other>
[[nodiscard]] constexpr bool operator==(const sparse_set_iterator<Type> &lhs, const sparse_set_iterator<Other> &rhs) noexcept {
    return lhs.index() == rhs.index();
}

template<typename Type, typename Other>
[[nodiscard]] constexpr bool operator!=(const sparse_set_iterator<Type> &lhs, const sparse_set_iterator<Other> &rhs) noexcept {
    return !(lhs == rhs);
}

template<typename Type, typename Other>
[[nodiscard]] constexpr bool operator<(const sparse_set_iterator<Type> &lhs, const sparse_set_iterator<Other> &rhs) noexcept {
    return lhs.index() > rhs.index();
}

template<typename Type, typename Other>
[[nodiscard]] constexpr bool operator>(const sparse_set_iterator<Type> &lhs, const sparse_set_iterator<Other> &rhs) noexcept {
    return lhs.index() < rhs.index();
}

template<typename Type, typename Other>
[[nodiscard]] constexpr bool operator<=(const sparse_set_iterator<Type> &lhs, const sparse_set_iterator<Other> &rhs) noexcept {
    return !(lhs > rhs);
}

template<typename Type, typename Other>
[[nodiscard]] constexpr bool operator>=(const sparse_set_iterator<Type> &lhs, const sparse_set_iterator<Other> &rhs) noexcept {
    return !(lhs < rhs);
}

} // namespace internal

/**
 * Internal details not to be documented.
 * @endcond
 */

/*! @brief Sparse set deletion policy. */
enum class deletion_policy : std::uint8_t {
    /*! @brief Swap-and-pop deletion policy. */
    swap_and_pop = 0u,
    /*! @brief In-place deletion policy. */
    in_place = 1u
};

/**
 * @brief Basic sparse set implementation.
 *
 * Sparse set or packed array or whatever is the name users give it.<br/>
 * Two arrays: an _external_ one and an _internal_ one; a _sparse_ one and a
 * _packed_ one; one used for direct access through contiguous memory, the other
 * one used to get the data through an extra level of indirection.<br/>
 * This is largely used by the registry to offer users the fastest access ever
 * to the components. Views and groups in general are almost entirely designed
 * around sparse sets.
 *
 * This type of data structure is widely documented in the literature and on the
 * web. This is nothing more than a customized implementation suitable for the
 * purpose of the framework.
 *
 * @note
 * Internal data structures arrange elements to maximize performance. There are
 * no guarantees that entities are returned in the insertion order when iterate
 * a sparse set. Do not make assumption on the order in any case.
 *
 * @tparam Entity A valid entity type (see entt_traits for more details).
 * @tparam Allocator Type of allocator used to manage memory and elements.
 */
template<typename Entity, typename Allocator>
class basic_sparse_set {
    using alloc_traits = std::allocator_traits<Allocator>;
    static_assert(std::is_same_v<typename alloc_traits::value_type, Entity>, "Invalid value type");
    using sparse_container_type = std::vector<typename alloc_traits::pointer, typename alloc_traits::template rebind_alloc<typename alloc_traits::pointer>>;
    using packed_container_type = std::vector<Entity, Allocator>;
    using entity_traits = entt_traits<Entity>;

    [[nodiscard]] auto sparse_ptr(const Entity entt) const {
        const auto pos = static_cast<size_type>(entity_traits::to_entity(entt));
        const auto page = pos / entity_traits::page_size;
        return (page < sparse.size() && sparse[page]) ? (sparse[page] + fast_mod(pos, entity_traits::page_size)) : nullptr;
    }

    [[nodiscard]] auto &sparse_ref(const Entity entt) const {
        ENTT_ASSERT(sparse_ptr(entt), "Invalid element");
        const auto pos = static_cast<size_type>(entity_traits::to_entity(entt));
        return sparse[pos / entity_traits::page_size][fast_mod(pos, entity_traits::page_size)];
    }

    [[nodiscard]] auto &assure_at_least(const Entity entt) {
        const auto pos = static_cast<size_type>(entity_traits::to_entity(entt));
        const auto page = pos / entity_traits::page_size;

        if(!(page < sparse.size())) {
            sparse.resize(page + 1u, nullptr);
        }

        if(!sparse[page]) {
            auto page_allocator{packed.get_allocator()};
            sparse[page] = alloc_traits::allocate(page_allocator, entity_traits::page_size);
            std::uninitialized_fill(sparse[page], sparse[page] + entity_traits::page_size, null);
        }

        auto &elem = sparse[page][fast_mod(pos, entity_traits::page_size)];
        ENTT_ASSERT(elem == null, "Slot not available");
        return elem;
    }

    void release_sparse_pages() {
        auto page_allocator{packed.get_allocator()};

        for(auto &&page: sparse) {
            if(page != nullptr) {
                std::destroy(page, page + entity_traits::page_size);
                alloc_traits::deallocate(page_allocator, page, entity_traits::page_size);
                page = nullptr;
            }
        }
    }

private:
    virtual const void *get_at(const std::size_t) const {
        return nullptr;
    }

    virtual void swap_at(const std::size_t, const std::size_t) {}
    virtual void move_element(const std::size_t, const std::size_t) {}

protected:
    /*! @brief Random access iterator type. */
    using basic_iterator = internal::sparse_set_iterator<packed_container_type>;

    /**
     * @brief Erases an entity from a sparse set.
     * @param it An iterator to the element to pop.
     */
    void swap_and_pop(const basic_iterator it) {
        ENTT_ASSERT(mode == deletion_policy::swap_and_pop, "Deletion policy mismatched");
        auto &self = sparse_ref(*it);
        const auto entt = entity_traits::to_entity(self);
        sparse_ref(packed.back()) = entity_traits::combine(entt, entity_traits::to_integral(packed.back()));
        packed[static_cast<size_type>(entt)] = packed.back();
        // unnecessary but it helps to detect nasty bugs
        ENTT_ASSERT((packed.back() = null, true), "");
        // lazy self-assignment guard
        self = null;
        packed.pop_back();
    }

    /**
     * @brief Erases an entity from a sparse set.
     * @param it An iterator to the element to pop.
     */
    void in_place_pop(const basic_iterator it) {
        ENTT_ASSERT(mode == deletion_policy::in_place, "Deletion policy mismatched");
        const auto entt = entity_traits::to_entity(std::exchange(sparse_ref(*it), null));
        packed[static_cast<size_type>(entt)] = std::exchange(free_list, entity_traits::combine(entt, entity_traits::reserved));
    }

protected:
    /**
     * @brief Erases entities from a sparse set.
     * @param first An iterator to the first element of the range of entities.
     * @param last An iterator past the last element of the range of entities.
     */
    virtual void pop(basic_iterator first, basic_iterator last) {
        if(mode == deletion_policy::swap_and_pop) {
            for(; first != last; ++first) {
                swap_and_pop(first);
            }
        } else {
            for(; first != last; ++first) {
                in_place_pop(first);
            }
        }
    }

    /**
     * @brief Assigns an entity to a sparse set.
     * @param entt A valid identifier.
     * @param force_back Force back insertion.
     * @return Iterator pointing to the emplaced element.
     */
    virtual basic_iterator try_emplace(const Entity entt, const bool force_back, const void * = nullptr) {
        ENTT_ASSERT(!contains(entt), "Set already contains entity");

        if(auto &elem = assure_at_least(entt); free_list == null || force_back) {
            packed.push_back(entt);
            elem = entity_traits::combine(static_cast<typename entity_traits::entity_type>(packed.size() - 1u), entity_traits::to_integral(entt));
            return begin();
        } else {
            const auto pos = static_cast<size_type>(entity_traits::to_entity(free_list));
            elem = entity_traits::combine(entity_traits::to_integral(free_list), entity_traits::to_integral(entt));
            free_list = std::exchange(packed[pos], entt);
            return --(end() - pos);
        }
    }

public:
    /*! @brief Allocator type. */
    using allocator_type = Allocator;
    /*! @brief Underlying entity identifier. */
    using entity_type = typename entity_traits::value_type;
    /*! @brief Underlying version type. */
    using version_type = typename entity_traits::version_type;
    /*! @brief Unsigned integer type. */
    using size_type = std::size_t;
    /*! @brief Pointer type to contained entities. */
    using pointer = typename packed_container_type::const_pointer;
    /*! @brief Random access iterator type. */
    using iterator = basic_iterator;
    /*! @brief Constant random access iterator type. */
    using const_iterator = iterator;
    /*! @brief Reverse iterator type. */
    using reverse_iterator = std::reverse_iterator<iterator>;
    /*! @brief Constant reverse iterator type. */
    using const_reverse_iterator = reverse_iterator;

    /*! @brief Default constructor. */
    basic_sparse_set()
        : basic_sparse_set{type_id<void>()} {}

    /**
     * @brief Constructs an empty container with a given allocator.
     * @param allocator The allocator to use.
     */
    explicit basic_sparse_set(const allocator_type &allocator)
        : basic_sparse_set{type_id<void>(), deletion_policy::swap_and_pop, allocator} {}

    /**
     * @brief Constructs an empty container with the given policy and allocator.
     * @param pol Type of deletion policy.
     * @param allocator The allocator to use (possibly default-constructed).
     */
    explicit basic_sparse_set(deletion_policy pol, const allocator_type &allocator = {})
        : basic_sparse_set{type_id<void>(), pol, allocator} {}

    /**
     * @brief Constructs an empty container with the given value type, policy
     * and allocator.
     * @param value Returned value type, if any.
     * @param pol Type of deletion policy.
     * @param allocator The allocator to use (possibly default-constructed).
     */
    explicit basic_sparse_set(const type_info &value, deletion_policy pol = deletion_policy::swap_and_pop, const allocator_type &allocator = {})
        : sparse{allocator},
          packed{allocator},
          info{&value},
          free_list{tombstone},
          mode{pol} {}

    /**
     * @brief Move constructor.
     * @param other The instance to move from.
     */
    basic_sparse_set(basic_sparse_set &&other) noexcept
        : sparse{std::move(other.sparse)},
          packed{std::move(other.packed)},
          info{other.info},
          free_list{std::exchange(other.free_list, tombstone)},
          mode{other.mode} {}

    /**
     * @brief Allocator-extended move constructor.
     * @param other The instance to move from.
     * @param allocator The allocator to use.
     */
    basic_sparse_set(basic_sparse_set &&other, const allocator_type &allocator) noexcept
        : sparse{std::move(other.sparse), allocator},
          packed{std::move(other.packed), allocator},
          info{other.info},
          free_list{std::exchange(other.free_list, tombstone)},
          mode{other.mode} {
        ENTT_ASSERT(alloc_traits::is_always_equal::value || packed.get_allocator() == other.packed.get_allocator(), "Copying a sparse set is not allowed");
    }

    /*! @brief Default destructor. */
    virtual ~basic_sparse_set() {
        release_sparse_pages();
    }

    /**
     * @brief Move assignment operator.
     * @param other The instance to move from.
     * @return This sparse set.
     */
    basic_sparse_set &operator=(basic_sparse_set &&other) noexcept {
        ENTT_ASSERT(alloc_traits::is_always_equal::value || packed.get_allocator() == other.packed.get_allocator(), "Copying a sparse set is not allowed");

        release_sparse_pages();
        sparse = std::move(other.sparse);
        packed = std::move(other.packed);
        info = other.info;
        free_list = std::exchange(other.free_list, tombstone);
        mode = other.mode;
        return *this;
    }

    /**
     * @brief Exchanges the contents with those of a given sparse set.
     * @param other Sparse set to exchange the content with.
     */
    void swap(basic_sparse_set &other) {
        using std::swap;
        swap(sparse, other.sparse);
        swap(packed, other.packed);
        swap(info, other.info);
        swap(free_list, other.free_list);
        swap(mode, other.mode);
    }

    /**
     * @brief Returns the associated allocator.
     * @return The associated allocator.
     */
    [[nodiscard]] constexpr allocator_type get_allocator() const noexcept {
        return packed.get_allocator();
    }

    /**
     * @brief Returns the deletion policy of a sparse set.
     * @return The deletion policy of the sparse set.
     */
    [[nodiscard]] deletion_policy policy() const noexcept {
        return mode;
    }

    /**
     * @brief Increases the capacity of a sparse set.
     *
     * If the new capacity is greater than the current capacity, new storage is
     * allocated, otherwise the method does nothing.
     *
     * @param cap Desired capacity.
     */
    virtual void reserve(const size_type cap) {
        packed.reserve(cap);
    }

    /**
     * @brief Returns the number of elements that a sparse set has currently
     * allocated space for.
     * @return Capacity of the sparse set.
     */
    [[nodiscard]] virtual size_type capacity() const noexcept {
        return packed.capacity();
    }

    /*! @brief Requests the removal of unused capacity. */
    virtual void shrink_to_fit() {
        packed.shrink_to_fit();
    }

    /**
     * @brief Returns the extent of a sparse set.
     *
     * The extent of a sparse set is also the size of the internal sparse array.
     * There is no guarantee that the internal packed array has the same size.
     * Usually the size of the internal sparse array is equal or greater than
     * the one of the internal packed array.
     *
     * @return Extent of the sparse set.
     */
    [[nodiscard]] size_type extent() const noexcept {
        return sparse.size() * entity_traits::page_size;
    }

    /**
     * @brief Returns the number of elements in a sparse set.
     *
     * The number of elements is also the size of the internal packed array.
     * There is no guarantee that the internal sparse array has the same size.
     * Usually the size of the internal sparse array is equal or greater than
     * the one of the internal packed array.
     *
     * @return Number of elements.
     */
    [[nodiscard]] size_type size() const noexcept {
        return packed.size();
    }

    /**
     * @brief Checks whether a sparse set is empty.
     * @return True if the sparse set is empty, false otherwise.
     */
    [[nodiscard]] bool empty() const noexcept {
        return packed.empty();
    }

    /**
     * @brief Direct access to the internal packed array.
     * @return A pointer to the internal packed array.
     */
    [[nodiscard]] pointer data() const noexcept {
        return packed.data();
    }

    /**
     * @brief Returns an iterator to the beginning.
     *
     * The returned iterator points to the first entity of the internal packed
     * array. If the sparse set is empty, the returned iterator will be equal to
     * `end()`.
     *
     * @return An iterator to the first entity of the sparse set.
     */
    [[nodiscard]] const_iterator begin() const noexcept {
        const auto pos = static_cast<typename iterator::difference_type>(packed.size());
        return iterator{packed, pos};
    }

    /*! @copydoc begin */
    [[nodiscard]] const_iterator cbegin() const noexcept {
        return begin();
    }

    /**
     * @brief Returns an iterator to the end.
     *
     * The returned iterator points to the element following the last entity in
     * a sparse set. Attempting to dereference the returned iterator results in
     * undefined behavior.
     *
     * @return An iterator to the element following the last entity of a sparse
     * set.
     */
    [[nodiscard]] iterator end() const noexcept {
        return iterator{packed, {}};
    }

    /*! @copydoc end */
    [[nodiscard]] const_iterator cend() const noexcept {
        return end();
    }

    /**
     * @brief Returns a reverse iterator to the beginning.
     *
     * The returned iterator points to the first entity of the reversed internal
     * packed array. If the sparse set is empty, the returned iterator will be
     * equal to `rend()`.
     *
     * @return An iterator to the first entity of the reversed internal packed
     * array.
     */
    [[nodiscard]] const_reverse_iterator rbegin() const noexcept {
        return std::make_reverse_iterator(end());
    }

    /*! @copydoc rbegin */
    [[nodiscard]] const_reverse_iterator crbegin() const noexcept {
        return rbegin();
    }

    /**
     * @brief Returns a reverse iterator to the end.
     *
     * The returned iterator points to the element following the last entity in
     * the reversed sparse set. Attempting to dereference the returned iterator
     * results in undefined behavior.
     *
     * @return An iterator to the element following the last entity of the
     * reversed sparse set.
     */
    [[nodiscard]] reverse_iterator rend() const noexcept {
        return std::make_reverse_iterator(begin());
    }

    /*! @copydoc rend */
    [[nodiscard]] const_reverse_iterator crend() const noexcept {
        return rend();
    }

    /**
     * @brief Finds an entity.
     * @param entt A valid identifier.
     * @return An iterator to the given entity if it's found, past the end
     * iterator otherwise.
     */
    [[nodiscard]] iterator find(const entity_type entt) const noexcept {
        return contains(entt) ? --(end() - index(entt)) : end();
    }

    /**
     * @brief Checks if a sparse set contains an entity.
     * @param entt A valid identifier.
     * @return True if the sparse set contains the entity, false otherwise.
     */
    [[nodiscard]] bool contains(const entity_type entt) const noexcept {
        const auto elem = sparse_ptr(entt);
        constexpr auto cap = entity_traits::to_entity(null);
        // testing versions permits to avoid accessing the packed array
        return elem && (((~cap & entity_traits::to_integral(entt)) ^ entity_traits::to_integral(*elem)) < cap);
    }

    /**
     * @brief Returns the contained version for an identifier.
     * @param entt A valid identifier.
     * @return The version for the given identifier if present, the tombstone
     * version otherwise.
     */
    [[nodiscard]] version_type current(const entity_type entt) const noexcept {
        const auto elem = sparse_ptr(entt);
        constexpr auto fallback = entity_traits::to_version(tombstone);
        return elem ? entity_traits::to_version(*elem) : fallback;
    }

    /**
     * @brief Returns the position of an entity in a sparse set.
     *
     * @warning
     * Attempting to get the position of an entity that doesn't belong to the
     * sparse set results in undefined behavior.
     *
     * @param entt A valid identifier.
     * @return The position of the entity in the sparse set.
     */
    [[nodiscard]] size_type index(const entity_type entt) const noexcept {
        ENTT_ASSERT(contains(entt), "Set does not contain entity");
        return static_cast<size_type>(entity_traits::to_entity(sparse_ref(entt)));
    }

    /**
     * @brief Returns the entity at specified location, with bounds checking.
     * @param pos The position for which to return the entity.
     * @return The entity at specified location if any, a null entity otherwise.
     */
    [[nodiscard]] entity_type at(const size_type pos) const noexcept {
        return pos < packed.size() ? packed[pos] : null;
    }

    /**
     * @brief Returns the entity at specified location, without bounds checking.
     * @param pos The position for which to return the entity.
     * @return The entity at specified location.
     */
    [[nodiscard]] entity_type operator[](const size_type pos) const noexcept {
        ENTT_ASSERT(pos < packed.size(), "Position is out of bounds");
        return packed[pos];
    }

    /**
     * @brief Returns the element assigned to an entity, if any.
     *
     * @warning
     * Attempting to use an entity that doesn't belong to the sparse set results
     * in undefined behavior.
     *
     * @param entt A valid identifier.
     * @return An opaque pointer to the element assigned to the entity, if any.
     */
    [[nodiscard]] const void *get(const entity_type entt) const noexcept {
        return get_at(index(entt));
    }

    /*! @copydoc get */
    [[nodiscard]] void *get(const entity_type entt) noexcept {
        return const_cast<void *>(std::as_const(*this).get(entt));
    }

    /**
     * @brief Assigns an entity to a sparse set.
     *
     * @warning
     * Attempting to assign an entity that already belongs to the sparse set
     * results in undefined behavior.
     *
     * @param entt A valid identifier.
     * @param value Optional opaque value to forward to mixins, if any.
     * @return Iterator pointing to the emplaced element in case of success, the
     * `end()` iterator otherwise.
     */
    iterator emplace(const entity_type entt, const void *value = nullptr) {
        return try_emplace(entt, false, value);
    }

    /**
     * @brief Bump the version number of an entity.
     *
     * @warning
     * Attempting to bump the version of an entity that doesn't belong to the
     * sparse set results in undefined behavior.
     *
     * @param entt A valid identifier.
     */
    void bump(const entity_type entt) {
        auto &entity = sparse_ref(entt);
        ENTT_ASSERT(entt != tombstone && entity != null, "Cannot set the required version");
        entity = entity_traits::combine(entity_traits::to_integral(entity), entity_traits::to_integral(entt));
        packed[static_cast<size_type>(entity_traits::to_entity(entity))] = entt;
    }

    /**
     * @brief Assigns one or more entities to a sparse set.
     *
     * @warning
     * Attempting to assign an entity that already belongs to the sparse set
     * results in undefined behavior.
     *
     * @tparam It Type of input iterator.
     * @param first An iterator to the first element of the range of entities.
     * @param last An iterator past the last element of the range of entities.
     * @return Iterator pointing to the first element inserted in case of
     * success, the `end()` iterator otherwise.
     */
    template<typename It>
    iterator insert(It first, It last) {
        for(auto it = first; it != last; ++it) {
            try_emplace(*it, true);
        }

        return first == last ? end() : find(*first);
    }

    /**
     * @brief Erases an entity from a sparse set.
     *
     * @warning
     * Attempting to erase an entity that doesn't belong to the sparse set
     * results in undefined behavior.
     *
     * @param entt A valid identifier.
     */
    void erase(const entity_type entt) {
        const auto it = --(end() - index(entt));
        pop(it, it + 1u);
    }

    /**
     * @brief Erases entities from a set.
     *
     * @sa erase
     *
     * @tparam It Type of input iterator.
     * @param first An iterator to the first element of the range of entities.
     * @param last An iterator past the last element of the range of entities.
     */
    template<typename It>
    void erase(It first, It last) {
        if constexpr(std::is_same_v<It, basic_iterator>) {
            pop(first, last);
        } else {
            for(; first != last; ++first) {
                erase(*first);
            }
        }
    }

    /**
     * @brief Removes an entity from a sparse set if it exists.
     * @param entt A valid identifier.
     * @return True if the entity is actually removed, false otherwise.
     */
    bool remove(const entity_type entt) {
        return contains(entt) && (erase(entt), true);
    }

    /**
     * @brief Removes entities from a sparse set if they exist.
     * @tparam It Type of input iterator.
     * @param first An iterator to the first element of the range of entities.
     * @param last An iterator past the last element of the range of entities.
     * @return The number of entities actually removed.
     */
    template<typename It>
    size_type remove(It first, It last) {
        size_type count{};

        for(; first != last; ++first) {
            count += remove(*first);
        }

        return count;
    }

    /*! @brief Removes all tombstones from the packed array of a sparse set. */
    void compact() {
        size_type from = packed.size();
        for(; from && packed[from - 1u] == tombstone; --from) {}

        for(auto *it = &free_list; *it != null && from; it = std::addressof(packed[entity_traits::to_entity(*it)])) {
            if(const size_type to = entity_traits::to_entity(*it); to < from) {
                --from;
                move_element(from, to);

                using std::swap;
                swap(packed[from], packed[to]);

                const auto entity = static_cast<typename entity_traits::entity_type>(to);
                sparse_ref(packed[to]) = entity_traits::combine(entity, entity_traits::to_integral(packed[to]));
                *it = entity_traits::combine(static_cast<typename entity_traits::entity_type>(from), entity_traits::reserved);
                for(; from && packed[from - 1u] == tombstone; --from) {}
            }
        }

        free_list = tombstone;
        packed.resize(from);
    }

    /**
     * @brief Swaps two entities in a sparse set.
     *
     * For what it's worth, this function affects both the internal sparse array
     * and the internal packed array. Users should not care of that anyway.
     *
     * @warning
     * Attempting to swap entities that don't belong to the sparse set results
     * in undefined behavior.
     *
     * @param lhs A valid identifier.
     * @param rhs A valid identifier.
     */
    void swap_elements(const entity_type lhs, const entity_type rhs) {
        ENTT_ASSERT(contains(lhs) && contains(rhs), "Set does not contain entities");

        auto &entt = sparse_ref(lhs);
        auto &other = sparse_ref(rhs);

        const auto from = entity_traits::to_entity(entt);
        const auto to = entity_traits::to_entity(other);

        // basic no-leak guarantee (with invalid state) if swapping throws
        swap_at(static_cast<size_type>(from), static_cast<size_type>(to));
        entt = entity_traits::combine(to, entity_traits::to_integral(packed[from]));
        other = entity_traits::combine(from, entity_traits::to_integral(packed[to]));

        using std::swap;
        swap(packed[from], packed[to]);
    }

    /**
     * @brief Sort the first count elements according to the given comparison
     * function.
     *
     * The comparison function object must return `true` if the first element
     * is _less_ than the second one, `false` otherwise. The signature of the
     * comparison function should be equivalent to the following:
     *
     * @code{.cpp}
     * bool(const Entity, const Entity);
     * @endcode
     *
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
     * @tparam Compare Type of comparison function object.
     * @tparam Sort Type of sort function object.
     * @tparam Args Types of arguments to forward to the sort function object.
     * @param length Number of elements to sort.
     * @param compare A valid comparison function object.
     * @param algo A valid sort function object.
     * @param args Arguments to forward to the sort function object, if any.
     */
    template<typename Compare, typename Sort = std_sort, typename... Args>
    void sort_n(const size_type length, Compare compare, Sort algo = Sort{}, Args &&...args) {
        ENTT_ASSERT(!(length > packed.size()), "Length exceeds the number of elements");
        ENTT_ASSERT(free_list == null, "Partial sorting with tombstones is not supported");

        algo(packed.rend() - length, packed.rend(), std::move(compare), std::forward<Args>(args)...);

        for(size_type pos{}; pos < length; ++pos) {
            auto curr = pos;
            auto next = index(packed[curr]);

            while(curr != next) {
                const auto idx = index(packed[next]);
                const auto entt = packed[curr];

                swap_at(next, idx);
                const auto entity = static_cast<typename entity_traits::entity_type>(curr);
                sparse_ref(entt) = entity_traits::combine(entity, entity_traits::to_integral(packed[curr]));
                curr = std::exchange(next, idx);
            }
        }
    }

    /**
     * @brief Sort all elements according to the given comparison function.
     *
     * @sa sort_n
     *
     * @tparam Compare Type of comparison function object.
     * @tparam Sort Type of sort function object.
     * @tparam Args Types of arguments to forward to the sort function object.
     * @param compare A valid comparison function object.
     * @param algo A valid sort function object.
     * @param args Arguments to forward to the sort function object, if any.
     */
    template<typename Compare, typename Sort = std_sort, typename... Args>
    void sort(Compare compare, Sort algo = Sort{}, Args &&...args) {
        compact();
        sort_n(packed.size(), std::move(compare), std::move(algo), std::forward<Args>(args)...);
    }

    /**
     * @brief Sort entities according to their order in another sparse set.
     *
     * Entities that are part of both the sparse sets are ordered internally
     * according to the order they have in `other`. All the other entities goes
     * to the end of the list and there are no guarantees on their order.<br/>
     * In other terms, this function can be used to impose the same order on two
     * sets by using one of them as a master and the other one as a slave.
     *
     * Iterating the sparse set with a couple of iterators returns elements in
     * the expected order after a call to `respect`. See `begin` and `end` for
     * more details.
     *
     * @param other The sparse sets that imposes the order of the entities.
     */
    void respect(const basic_sparse_set &other) {
        compact();

        const auto to = other.end();
        auto from = other.begin();

        for(size_type pos = packed.size() - 1; pos && from != to; ++from) {
            if(contains(*from)) {
                if(*from != packed[pos]) {
                    // basic no-leak guarantee (with invalid state) if swapping throws
                    swap_elements(packed[pos], *from);
                }

                --pos;
            }
        }
    }

    /*! @brief Clears a sparse set. */
    void clear() {
        if(const auto last = end(); free_list == null) {
            pop(begin(), last);
        } else {
            for(auto &&entity: *this) {
                // tombstone filter on itself
                if(const auto it = find(entity); it != last) {
                    pop(it, it + 1u);
                }
            }
        }

        free_list = tombstone;
        packed.clear();
    }

    /**
     * @brief Returned value type, if any.
     * @return Returned value type, if any.
     */
    const type_info &type() const noexcept {
        return *info;
    }

    /*! @brief Forwards variables to derived classes, if any. */
    virtual void bind(any) noexcept {}

private:
    sparse_container_type sparse;
    packed_container_type packed;
    const type_info *info;
    entity_type free_list;
    deletion_policy mode;
};

} // namespace entt

#endif

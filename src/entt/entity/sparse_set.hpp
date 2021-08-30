#ifndef ENTT_ENTITY_SPARSE_SET_HPP
#define ENTT_ENTITY_SPARSE_SET_HPP


#include <cstddef>
#include <iterator>
#include <memory>
#include <type_traits>
#include <utility>
#include "../config/config.h"
#include "../core/algorithm.hpp"
#include "../core/compressed_pair.hpp"
#include "../core/memory.hpp"
#include "entity.hpp"
#include "fwd.hpp"


namespace entt {


/**
 * @cond TURN_OFF_DOXYGEN
 * Internal details not to be documented.
 */


namespace internal {


template<typename Traits>
struct sparse_set_iterator final {
    using value_type = typename Traits::value_type;
    using pointer = typename Traits::pointer;
    using reference = typename Traits::reference;
    using difference_type = typename Traits::difference_type;
    using iterator_category = std::random_access_iterator_tag;

    sparse_set_iterator() ENTT_NOEXCEPT = default;

    sparse_set_iterator(const pointer *ref, const difference_type idx) ENTT_NOEXCEPT
        : packed{ref},
          index{idx}
    {}

    sparse_set_iterator & operator++() ENTT_NOEXCEPT {
        return --index, *this;
    }

    sparse_set_iterator operator++(int) ENTT_NOEXCEPT {
        sparse_set_iterator orig = *this;
        return ++(*this), orig;
    }

    sparse_set_iterator & operator--() ENTT_NOEXCEPT {
        return ++index, *this;
    }

    sparse_set_iterator operator--(int) ENTT_NOEXCEPT {
        sparse_set_iterator orig = *this;
        return operator--(), orig;
    }

    sparse_set_iterator & operator+=(const difference_type value) ENTT_NOEXCEPT {
        index -= value;
        return *this;
    }

    sparse_set_iterator operator+(const difference_type value) const ENTT_NOEXCEPT {
        sparse_set_iterator copy = *this;
        return (copy += value);
    }

    sparse_set_iterator & operator-=(const difference_type value) ENTT_NOEXCEPT {
        return (*this += -value);
    }

    sparse_set_iterator operator-(const difference_type value) const ENTT_NOEXCEPT {
        return (*this + -value);
    }

    difference_type operator-(const sparse_set_iterator &other) const ENTT_NOEXCEPT {
        return other.index - index;
    }

    [[nodiscard]] reference operator[](const difference_type value) const {
        return *operator+(value);
    }

    [[nodiscard]] bool operator==(const sparse_set_iterator &other) const ENTT_NOEXCEPT {
        return other.index == index;
    }

    [[nodiscard]] bool operator!=(const sparse_set_iterator &other) const ENTT_NOEXCEPT {
        return !(*this == other);
    }

    [[nodiscard]] bool operator<(const sparse_set_iterator &other) const ENTT_NOEXCEPT {
        return index > other.index;
    }

    [[nodiscard]] bool operator>(const sparse_set_iterator &other) const ENTT_NOEXCEPT {
        return index < other.index;
    }

    [[nodiscard]] bool operator<=(const sparse_set_iterator &other) const ENTT_NOEXCEPT {
        return !(*this > other);
    }

    [[nodiscard]] bool operator>=(const sparse_set_iterator &other) const ENTT_NOEXCEPT {
        return !(*this < other);
    }

    [[nodiscard]] pointer operator->() const {
        const auto pos = index - 1;
        return (*packed) + pos;
    }

    [[nodiscard]] reference operator*() const {
        return *operator->();
    }

private:
    const pointer *packed;
    difference_type index;
};


}


/**
 * Internal details not to be documented.
 * @endcond
 */


/*! @brief Sparse set deletion policy. */
enum class deletion_policy: std::uint8_t {
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
    static constexpr auto growth_factor_v = 1.5;
    static constexpr auto sparse_page_v = ENTT_SPARSE_PAGE;

    using allocator_traits = std::allocator_traits<Allocator>;

    using alloc = typename allocator_traits::template rebind_alloc<Entity>;
    using alloc_traits = typename std::allocator_traits<alloc>;
    using alloc_const_pointer = typename alloc_traits::const_pointer;
    using alloc_pointer = typename alloc_traits::pointer;

    using alloc_ptr = typename allocator_traits::template rebind_alloc<alloc_pointer>;
    using alloc_ptr_traits = typename std::allocator_traits<alloc_ptr>;
    using alloc_ptr_pointer = typename alloc_ptr_traits::pointer;

    using entity_traits = entt_traits<Entity>;

    [[nodiscard]] auto sparse_ptr(const Entity entt) const {
        const auto pos = static_cast<size_type>(entity_traits::to_entity(entt));
        const auto page = pos / sparse_page_v;
        return (page < bucket && sparse[page]) ? (sparse[page] + fast_mod<sparse_page_v>(pos)) : alloc_pointer{};
    }

    [[nodiscard]] auto & sparse_ref(const Entity entt) const {
        ENTT_ASSERT(sparse_ptr(entt), "Invalid element");
        const auto pos = static_cast<size_type>(entity_traits::to_entity(entt));
        return sparse[pos / sparse_page_v][fast_mod<sparse_page_v>(pos)];
    }

    void resize_packed(const std::size_t req) {
        ENTT_ASSERT((req != reserved.second()) && !(req < count), "Invalid request");
        const auto mem = alloc_traits::allocate(reserved.first(), req);

        std::uninitialized_fill(mem + count, mem + req, tombstone);

        if(packed) {
            std::uninitialized_copy(packed, packed + count, mem);
            std::destroy(packed, packed + reserved.second());
            alloc_traits::deallocate(reserved.first(), packed, reserved.second());
        }

        packed = mem;
        reserved.second() = req;
    }

    void release_memory() {
        if(packed) {
            std::destroy(packed, packed + reserved.second());
            alloc_traits::deallocate(reserved.first(), packed, reserved.second());
        }

        if(sparse) {
            for(size_type pos{}; pos < bucket; ++pos) {
                if(sparse[pos]) {
                    std::destroy(sparse[pos], sparse[pos] + sparse_page_v);
                    alloc_traits::deallocate(reserved.first(), sparse[pos], sparse_page_v);
                }

                std::destroy_at(std::addressof(sparse[pos]));
            }

            alloc_ptr allocator_ptr{reserved.first()};
            alloc_ptr_traits::deallocate(allocator_ptr, sparse, bucket);
        }
    }

protected:
    /*! @brief Exchanges the contents with those of a given sparse set. */
    virtual void swap_contents(basic_sparse_set &) {}

    /*! @brief Swaps two entities in a sparse set. */
    virtual void swap_at(const std::size_t, const std::size_t) {}

    /*! @brief Moves an entity in a sparse set. */
    virtual void move_and_pop(const std::size_t, const std::size_t) {}

    /**
     * @brief Erase an entity from a sparse set.
     * @param entt A valid identifier.
     */
    virtual void swap_and_pop(const Entity entt, void *) {
        auto &ref = sparse_ref(entt);
        const auto pos = static_cast<size_type>(entity_traits::to_entity(ref));
        ENTT_ASSERT(packed[pos] == entt, "Invalid identifier");
        auto &last = packed[--count];

        packed[pos] = last;
        auto &elem = sparse_ref(last);
        elem = entity_traits::combine(entity_traits::to_integral(ref), entity_traits::to_integral(elem));
        // lazy self-assignment guard
        ref = null;
        // unnecessary but it helps to detect nasty bugs
        ENTT_ASSERT((last = tombstone, true), "");
    }

    /**
     * @brief Erase an entity from a sparse set.
     * @param entt A valid identifier.
     */
    virtual void in_place_pop(const Entity entt, void *) {
        auto &ref = sparse_ref(entt);
        const auto pos = static_cast<size_type>(entity_traits::to_entity(ref));
        ENTT_ASSERT(packed[pos] == entt, "Invalid identifier");

        packed[pos] = std::exchange(free_list, entity_traits::combine(static_cast<typename entity_traits::entity_type>(pos), entity_traits::reserved));
        // lazy self-assignment guard
        ref = null;
    }

    /**
     * @brief Assigns an entity to a sparse set.
     * @param entt A valid identifier.
     */
    virtual void try_emplace(const Entity entt, void *) {
        const auto pos = static_cast<size_type>(entity_traits::to_entity(entt));
        const auto page = pos / sparse_page_v;

        if(!(page < bucket)) {
            const size_type sz = page + 1u;
            alloc_ptr allocator_ptr{reserved.first()};
            const auto mem = alloc_ptr_traits::allocate(allocator_ptr, sz);

            std::uninitialized_value_construct(mem + bucket, mem + sz);

            if(sparse) {
                std::uninitialized_copy(sparse, sparse + bucket, mem);
                std::destroy(sparse, sparse + bucket);
                alloc_ptr_traits::deallocate(allocator_ptr, sparse, bucket);
            }

            sparse = mem;
            bucket = sz;
        }

        if(!sparse[page]) {
            sparse[page] = alloc_traits::allocate(reserved.first(), sparse_page_v);
            std::uninitialized_fill(sparse[page], sparse[page] + sparse_page_v, null);
        }

        auto &elem = sparse[page][fast_mod<sparse_page_v>(pos)];
        ENTT_ASSERT(entity_traits::to_version(elem) == entity_traits::to_version(tombstone), "Slot not available");

        if(free_list == null) {
            if(count == reserved.second()) {
                const size_type sz = static_cast<size_type>(reserved.second() * growth_factor_v);
                resize_packed(sz + !(sz > reserved.second()));
            }

            elem = entity_traits::combine(static_cast<typename entity_traits::entity_type>(count), entity_traits::to_integral(entt));
            packed[count++] = entt;
        } else {
            elem = entity_traits::combine(entity_traits::to_integral(free_list), entity_traits::to_integral(entt));
            free_list = std::exchange(packed[static_cast<size_type>(entity_traits::to_entity(free_list))], entt);
        }
    }

public:
    /*! @brief Allocator type. */
    using allocator_type = Allocator;
    /*! @brief Underlying entity identifier. */
    using entity_type = Entity;
    /*! @brief Underlying version type. */
    using version_type = typename entity_traits::version_type;
    /*! @brief Unsigned integer type. */
    using size_type = std::size_t;
    /*! @brief Pointer type to contained entities. */
    using pointer = alloc_const_pointer;
    /*! @brief Random access iterator type. */
    using iterator = internal::sparse_set_iterator<std::iterator_traits<pointer>>;
    /*! @brief Reverse iterator type. */
    using reverse_iterator = std::reverse_iterator<iterator>;

    /*! @brief Default constructor. */
    basic_sparse_set()
        : basic_sparse_set{allocator_type{}}
    {}

    /**
     * @brief Constructs an empty container with a given allocator.
     * @param allocator The allocator to use.
     */
    explicit basic_sparse_set(const allocator_type &allocator)
        : basic_sparse_set{deletion_policy::swap_and_pop, allocator}
    {}

    /**
     * @brief Constructs an empty container with the given policy and allocator.
     * @param pol Type of deletion policy.
     * @param allocator The allocator to use (possibly default-constructed).
     */
    explicit basic_sparse_set(deletion_policy pol, const allocator_type &allocator = {})
        : reserved{allocator, size_type{}},
          sparse{},
          packed{},
          bucket{},
          count{},
          free_list{tombstone},
          mode{pol}
    {}

    /**
     * @brief Move constructor.
     * @param other The instance to move from.
     */
    basic_sparse_set(basic_sparse_set &&other) ENTT_NOEXCEPT
        : reserved{std::move(other.reserved.first()), std::exchange(other.reserved.second(), size_type{})},
          sparse{std::exchange(other.sparse, alloc_ptr_pointer{})},
          packed{std::exchange(other.packed, alloc_pointer{})},
          bucket{std::exchange(other.bucket, size_type{})},
          count{std::exchange(other.count, size_type{})},
          free_list{std::exchange(other.free_list, tombstone)},
          mode{other.mode}
    {}

    /**
     * @brief Allocator-extended move constructor.
     * @param other The instance to move from.
     * @param allocator The allocator to use.
     */
    basic_sparse_set(basic_sparse_set &&other, const allocator_type &allocator) ENTT_NOEXCEPT
        : reserved{allocator, std::exchange(other.reserved.second(), size_type{})},
          sparse{std::exchange(other.sparse, alloc_ptr_pointer{})},
          packed{std::exchange(other.packed, alloc_pointer{})},
          bucket{std::exchange(other.bucket, size_type{})},
          count{std::exchange(other.count, size_type{})},
          free_list{std::exchange(other.free_list, tombstone)},
          mode{other.mode}
    {
        ENTT_ASSERT(alloc_traits::is_always_equal::value || reserved.first() == other.reserved.first(), "Copying a sparse set is not allowed");
    }

    /*! @brief Default destructor. */
    virtual ~basic_sparse_set() {
        release_memory();
    }

    /**
     * @brief Move assignment operator.
     * @param other The instance to move from.
     * @return This sparse set.
     */
    basic_sparse_set & operator=(basic_sparse_set &&other) ENTT_NOEXCEPT {
        release_memory();
        propagate_on_container_move_assignment(reserved.first(), other.reserved.first());
        ENTT_ASSERT(alloc_traits::is_always_equal::value || reserved.first() == other.reserved.first(), "Copying a sparse set is not allowed");
        reserved.second() = std::exchange(other.reserved.second(), size_type{});
        sparse = std::exchange(other.sparse, alloc_ptr_pointer{});
        packed = std::exchange(other.packed, alloc_pointer{});
        bucket = std::exchange(other.bucket, size_type{});
        count = std::exchange(other.count, size_type{});
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
        swap_contents(other);
        propagate_on_container_swap(reserved.first(), other.reserved.first());
        swap(reserved.second(), other.reserved.second());
        swap(sparse, other.sparse);
        swap(packed, other.packed);
        swap(bucket, other.bucket);
        swap(count, other.count);
        swap(free_list, other.free_list);
        swap(mode, other.mode);
    }

    /**
     * @brief Returns the associated allocator.
     * @return The associated allocator.
     */
    [[nodiscard]] constexpr allocator_type get_allocator() const ENTT_NOEXCEPT {
        return allocator_type{reserved.first()};
    }

    /**
     * @brief Returns the deletion policy of a sparse set.
     * @return The deletion policy of the sparse set.
     */
    [[nodiscard]] deletion_policy policy() const ENTT_NOEXCEPT {
        return mode;
    }

    /**
     * @brief Returns the next slot available for insertion.
     * @return The next slot available for insertion.
     */
    [[nodiscard]] size_type slot() const ENTT_NOEXCEPT {
        return free_list == null ? count : static_cast<size_type>(entity_traits::to_entity(free_list));
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
        if(cap > reserved.second()) {
            resize_packed(cap);
        }
    }

    /**
     * @brief Returns the number of elements that a sparse set has currently
     * allocated space for.
     * @return Capacity of the sparse set.
     */
    [[nodiscard]] virtual size_type capacity() const ENTT_NOEXCEPT {
        return reserved.second();
    }

    /*! @brief Requests the removal of unused capacity. */
    virtual void shrink_to_fit() {
        if(count < reserved.second()) {
            resize_packed(count);
        }
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
    [[nodiscard]] size_type extent() const ENTT_NOEXCEPT {
        return bucket * sparse_page_v;
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
    [[nodiscard]] size_type size() const ENTT_NOEXCEPT {
        return count;
    }

    /**
     * @brief Checks whether a sparse set is empty.
     * @return True if the sparse set is empty, false otherwise.
     */
    [[nodiscard]] bool empty() const ENTT_NOEXCEPT {
        return (count == size_type{});
    }

    /**
     * @brief Direct access to the internal packed array.
     * @return A pointer to the internal packed array.
     */
    [[nodiscard]] pointer data() const ENTT_NOEXCEPT {
        return packed;
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
    [[nodiscard]] iterator begin() const ENTT_NOEXCEPT {
        const auto pos = static_cast<typename iterator::difference_type>(count);
        return iterator{std::addressof(packed), pos};
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
    [[nodiscard]] iterator end() const ENTT_NOEXCEPT {
        return iterator{std::addressof(packed), {}};
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
    [[nodiscard]] reverse_iterator rbegin() const ENTT_NOEXCEPT {
        return std::make_reverse_iterator(end());
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
    [[nodiscard]] reverse_iterator rend() const ENTT_NOEXCEPT {
        return std::make_reverse_iterator(begin());
    }

    /**
     * @brief Finds an entity.
     * @param entt A valid identifier.
     * @return An iterator to the given entity if it's found, past the end
     * iterator otherwise.
     */
    [[nodiscard]] iterator find(const entity_type entt) const ENTT_NOEXCEPT {
        return contains(entt) ? --(end() - index(entt)) : end();
    }

    /**
     * @brief Checks if a sparse set contains an entity.
     * @param entt A valid identifier.
     * @return True if the sparse set contains the entity, false otherwise.
     */
    [[nodiscard]] bool contains(const entity_type entt) const ENTT_NOEXCEPT {
        if(auto elem = sparse_ptr(entt); elem) {
            constexpr auto cap = entity_traits::to_entity(entt::null);
            // testing versions permits to avoid accessing the packed array
            return (((~cap & entity_traits::to_integral(entt)) ^ entity_traits::to_integral(*elem)) < cap);
        }

        return false;
    }

    /**
     * @brief Returns the contained version for an identifier.
     * @param entt A valid identifier.
     * @return The version for the given identifier if present, the tombstone
     * version otherwise.
     */
    [[nodiscard]] version_type current(const entity_type entt) const {
        if(auto elem = sparse_ptr(entt); elem) {
            return entity_traits::to_version(*elem);
        }

        return entity_traits::to_version(tombstone);
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
    [[nodiscard]] size_type index(const entity_type entt) const ENTT_NOEXCEPT {
        ENTT_ASSERT(contains(entt), "Set does not contain entity");
        return static_cast<size_type>(entity_traits::to_entity(sparse_ref(entt)));
    }

    /**
     * @brief Returns the entity at specified location, with bounds checking.
     * @param pos The position for which to return the entity.
     * @return The entity at specified location if any, a null entity otherwise.
     */
    [[nodiscard]] entity_type at(const size_type pos) const ENTT_NOEXCEPT {
        return pos < count ? packed[pos] : null;
    }

    /**
     * @brief Returns the entity at specified location, without bounds checking.
     * @param pos The position for which to return the entity.
     * @return The entity at specified location.
     */
    [[nodiscard]] entity_type operator[](const size_type pos) const ENTT_NOEXCEPT {
        ENTT_ASSERT(pos < count, "Position is out of bounds");
        return packed[pos];
    }

    /**
     * @brief Assigns an entity to a sparse set.
     *
     * @warning
     * Attempting to assign an entity that already belongs to the sparse set
     * results in undefined behavior.
     *
     * @param entt A valid identifier.
     * @param ud Optional user data that are forwarded as-is to derived classes.
     */
    void emplace(const entity_type entt, void *ud = nullptr) {
        try_emplace(entt, ud);
        ENTT_ASSERT(contains(entt), "Emplace did not take place");
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
     * @param ud Optional user data that are forwarded as-is to derived classes.
     */
    template<typename It>
    void insert(It first, It last, void *ud = nullptr) {
        for(; first != last && free_list != null; ++first) {
            emplace(*first, ud);
        }

        reserve(count + std::distance(first, last));

        for(; first != last; ++first) {
            emplace(*first, ud);
        }
    }

    /**
     * @brief Erases an entity from a sparse set.
     *
     * @warning
     * Attempting to erase an entity that doesn't belong to the sparse set
     * results in undefined behavior.
     *
     * @param entt A valid identifier.
     * @param ud Optional user data that are forwarded as-is to derived classes.
     */
    void erase(const entity_type entt, void *ud = nullptr) {
        ENTT_ASSERT(contains(entt), "Set does not contain entity");
        (mode == deletion_policy::in_place) ? in_place_pop(entt, ud) : swap_and_pop(entt, ud);
        ENTT_ASSERT(!contains(entt), "Destruction did not take place");
    }

    /**
     * @brief Erases entities from a set.
     *
     * @sa erase
     *
     * @tparam It Type of input iterator.
     * @param first An iterator to the first element of the range of entities.
     * @param last An iterator past the last element of the range of entities.
     * @param ud Optional user data that are forwarded as-is to derived classes.
     */
    template<typename It>
    void erase(It first, It last, void *ud = nullptr) {
        for(; first != last; ++first) {
            erase(*first, ud);
        }
    }

    /**
     * @brief Removes an entity from a sparse set if it exists.
     * @param entt A valid identifier.
     * @param ud Optional user data that are forwarded as-is to derived classes.
     * @return True if the entity is actually removed, false otherwise.
     */
    bool remove(const entity_type entt, void *ud = nullptr) {
        return contains(entt) && (erase(entt, ud), true);
    }

    /**
     * @brief Removes entities from a sparse set if they exist.
     * @tparam It Type of input iterator.
     * @param first An iterator to the first element of the range of entities.
     * @param last An iterator past the last element of the range of entities.
     * @param ud Optional user data that are forwarded as-is to derived classes.
     * @return The number of entities actually removed.
     */
    template<typename It>
    size_type remove(It first, It last, void *ud = nullptr) {
        size_type found{};

        for(; first != last; ++first) {
            found += remove(*first, ud);
        }

        return found;
    }

    /*! @brief Removes all tombstones from the packed array of a sparse set. */
    void compact() {
        size_type next = count;
        for(; next && packed[next - 1u] == tombstone; --next);

        for(auto *it = &free_list; *it != null && next; it = std::addressof(packed[entity_traits::to_entity(*it)])) {
            if(const size_type pos = entity_traits::to_entity(*it); pos < next) {
                --next;
                move_and_pop(next, pos);
                std::swap(packed[next], packed[pos]);
                const auto entity = static_cast<typename entity_traits::entity_type>(pos);
                sparse_ref(packed[pos]) = entity_traits::combine(entity, entity_traits::to_integral(packed[pos]));
                *it = entity_traits::combine(static_cast<typename entity_traits::entity_type>(next), entity_traits::reserved);
                for(; next && packed[next - 1u] == tombstone; --next);
            }
        }

        free_list = tombstone;
        count = next;
    }

    /**
     * @copybrief swap_at
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
        std::swap(packed[from], packed[to]);
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
    void sort_n(const size_type length, Compare compare, Sort algo = Sort{}, Args &&... args) {
        // basic no-leak guarantee (with invalid state) if sorting throws
        ENTT_ASSERT(!(length > count), "Length exceeds the number of elements");
        compact();

        algo(std::make_reverse_iterator(packed + length), std::make_reverse_iterator(packed), std::move(compare), std::forward<Args>(args)...);

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
    void sort(Compare compare, Sort algo = Sort{}, Args &&... args) {
        sort_n(count, std::move(compare), std::move(algo), std::forward<Args>(args)...);
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

        for(size_type pos = count - 1; pos && from != to; ++from) {
            if(contains(*from)) {
                if(*from != packed[pos]) {
                    // basic no-leak guarantee (with invalid state) if swapping throws
                    swap_elements(packed[pos], *from);
                }

                --pos;
            }
        }
    }

    /**
     * @brief Clears a sparse set.
     * @param ud Optional user data that are forwarded as-is to derived classes.
     */
    void clear(void *ud = nullptr) {
        for(auto &&entity: *this) {
            // honor the modality and filter all tombstones
            remove(entity, ud);
        }
    }

private:
    compressed_pair<alloc, size_type> reserved;
    alloc_ptr_pointer sparse;
    alloc_pointer packed;
    size_type bucket;
    size_type count;
    entity_type free_list;
    deletion_policy mode;
};


}


#endif

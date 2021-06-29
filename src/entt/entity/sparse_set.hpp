#ifndef ENTT_ENTITY_SPARSE_SET_HPP
#define ENTT_ENTITY_SPARSE_SET_HPP


#include <cstddef>
#include <iterator>
#include <memory>
#include <type_traits>
#include <utility>
#include "../config/config.h"
#include "../core/algorithm.hpp"
#include "../core/fwd.hpp"
#include "entity.hpp"
#include "fwd.hpp"


namespace entt {


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
    static constexpr auto growth_factor = 1.5;
    static constexpr auto sparse_page = ENTT_SPARSE_PAGE;

    using traits_type = entt_traits<Entity>;

    using alloc_traits = typename std::allocator_traits<Allocator>::template rebind_traits<Entity>;
    using alloc_pointer = typename alloc_traits::pointer;
    using alloc_const_pointer = typename alloc_traits::const_pointer;

    using bucket_alloc_traits = typename std::allocator_traits<Allocator>::template rebind_traits<alloc_pointer>;
    using bucket_alloc_pointer = typename bucket_alloc_traits::pointer;

    static_assert(alloc_traits::propagate_on_container_move_assignment::value);
    static_assert(bucket_alloc_traits::propagate_on_container_move_assignment::value);

    struct sparse_set_iterator final {
        using difference_type = typename traits_type::difference_type;
        using value_type = Entity;
        using pointer = const value_type *;
        using reference = const value_type &;
        using iterator_category = std::random_access_iterator_tag;

        sparse_set_iterator() ENTT_NOEXCEPT = default;

        sparse_set_iterator(const alloc_const_pointer *ref, const difference_type idx) ENTT_NOEXCEPT
            : packed{ref},
              index{idx}
        {}

        sparse_set_iterator & operator++() ENTT_NOEXCEPT {
            return --index, *this;
        }

        sparse_set_iterator operator++(int) ENTT_NOEXCEPT {
            iterator orig = *this;
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
            const auto pos = size_type(index-value-1u);
            return (*packed)[pos];
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
            const auto pos = size_type(index-1u);
            return std::addressof((*packed)[pos]);
        }

        [[nodiscard]] reference operator*() const {
            return *operator->();
        }

    private:
        const alloc_const_pointer *packed;
        difference_type index;
    };

    [[nodiscard]] static auto page(const Entity entt) ENTT_NOEXCEPT {
        return size_type{traits_type::to_entity(entt) / sparse_page};
    }

    [[nodiscard]] static auto offset(const Entity entt) ENTT_NOEXCEPT {
        return size_type{traits_type::to_entity(entt) & (sparse_page - 1)};
    }

    [[nodiscard]] auto assure_page(const std::size_t idx) {
        if(!(idx < bucket)) {
            const size_type sz = idx + 1u;
            const auto mem = bucket_alloc_traits::allocate(bucket_allocator, sz);

            std::uninitialized_value_construct(mem + bucket, mem + sz);
            std::uninitialized_copy(sparse, sparse + bucket, mem);

            std::destroy(sparse, sparse + bucket);
            bucket_alloc_traits::deallocate(bucket_allocator, sparse, bucket);

            sparse = mem;
            bucket = sz;
        }

        if(!sparse[idx]) {
            sparse[idx] = alloc_traits::allocate(allocator, sparse_page);
            std::uninitialized_fill(sparse[idx], sparse[idx] + sparse_page, null);
        }

        return sparse[idx];
    }

    void resize_packed(const std::size_t req) {
        ENTT_ASSERT((req != reserved) && !(req < count), "Invalid request");
        const auto mem = alloc_traits::allocate(allocator, req);

        std::uninitialized_copy(packed, packed + count, mem);
        std::uninitialized_fill(mem + count, mem + req, tombstone);

        std::destroy(packed, packed + reserved);
        alloc_traits::deallocate(allocator, packed, reserved);

        packed = mem;
        reserved = req;
    }

    void release_memory() {
        if(packed) {
            for(size_type pos{}; pos < bucket; ++pos) {
                if(sparse[pos]) {
                    std::destroy(sparse[pos], sparse[pos] + sparse_page);
                    alloc_traits::deallocate(allocator, sparse[pos], sparse_page);
                }
            }

            std::destroy(packed, packed + reserved);
            std::destroy(sparse, sparse + bucket);
            alloc_traits::deallocate(allocator, packed, reserved);
            bucket_alloc_traits::deallocate(bucket_allocator, sparse, bucket);
        }
    }

protected:
    /**
     * @brief Swaps two entities in the internal packed array.
     * @param lhs A valid position of an entity within storage.
     * @param rhs A valid position of an entity within storage.
     */
    virtual void swap_at([[maybe_unused]] const std::size_t lhs, [[maybe_unused]] const std::size_t rhs) {}

    /**
     * @brief Moves an entity in the internal packed array.
     * @param from A valid position of an entity within storage.
     * @param to A valid position of an entity within storage.
     */
    virtual void move_and_pop([[maybe_unused]] const std::size_t from, [[maybe_unused]] const std::size_t to) {}

    /**
     * @brief Attempts to erase an entity from the internal packed array.
     * @param entt A valid entity identifier.
     * @param ud Optional user data that are forwarded as-is to derived classes.
     */
    virtual void swap_and_pop(const Entity entt, [[maybe_unused]] void *ud) {
        auto &ref = sparse[page(entt)][offset(entt)];
        const auto pos = size_type{traits_type::to_entity(ref)};
        ENTT_ASSERT(packed[pos] == entt, "Invalid entity identifier");
        auto &last = packed[--count];

        packed[pos] = last;
        sparse[page(last)][offset(last)] = ref;
        // lazy self-assignment guard
        ref = null;
        // unnecessary but it helps to detect nasty bugs
        ENTT_ASSERT((last = tombstone, true), "");
    }

    /**
     * @brief Attempts to erase an entity from the internal packed array.
     * @param entt A valid entity identifier.
     * @param ud Optional user data that are forwarded as-is to derived classes.
     */
    virtual void in_place_pop(const Entity entt, [[maybe_unused]] void *ud) {
        auto &ref = sparse[page(entt)][offset(entt)];
        const auto pos = size_type{traits_type::to_entity(ref)};
        ENTT_ASSERT(packed[pos] == entt, "Invalid entity identifier");

        packed[pos] = std::exchange(free_list, traits_type::construct(static_cast<typename traits_type::entity_type>(pos)));
        // lazy self-assignment guard
        ref = null;
    }

public:
    /*! @brief Allocator type. */
    using allocator_type = typename alloc_traits::allocator_type;
    /*! @brief Underlying entity identifier. */
    using entity_type = Entity;
    /*! @brief Unsigned integer type. */
    using size_type = std::size_t;
    /*! @brief Pointer type to contained entities. */
    using pointer = alloc_const_pointer;
    /*! @brief Random access iterator type. */
    using iterator = sparse_set_iterator;
    /*! @brief Reverse iterator type. */
    using reverse_iterator = std::reverse_iterator<iterator>;

    /**
     * @brief Constructs an empty container with the given policy and allocator.
     * @param pol Type of deletion policy.
     * @param alloc Allocator to use (possibly default-constructed).
     */
    explicit basic_sparse_set(deletion_policy pol, const allocator_type &alloc = {})
        : allocator{alloc},
          bucket_allocator{alloc},
          sparse{bucket_alloc_traits::allocate(bucket_allocator, 0u)},
          packed{alloc_traits::allocate(allocator, 0u)},
          bucket{0u},
          count{0u},
          reserved{0u},
          free_list{tombstone},
          mode{pol}
    {}

    /**
     * @brief Constructs an empty container with the given allocator.
     * @param alloc Allocator to use (possibly default-constructed).
     */
    explicit basic_sparse_set(const allocator_type &alloc = {})
        : basic_sparse_set{deletion_policy::swap_and_pop, alloc}
    {}

    /**
     * @brief Move constructor.
     * @param other The instance to move from.
     */
    basic_sparse_set(basic_sparse_set &&other) ENTT_NOEXCEPT
        : allocator{std::move(other.allocator)},
          bucket_allocator{std::move(other.bucket_allocator)},
          sparse{std::exchange(other.sparse, bucket_alloc_pointer{})},
          packed{std::exchange(other.packed, alloc_pointer{})},
          bucket{std::exchange(other.bucket, 0u)},
          count{std::exchange(other.count, 0u)},
          reserved{std::exchange(other.reserved, 0u)},
          free_list{std::exchange(other.free_list, tombstone)},
          mode{other.mode}
    {}

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

        allocator = std::move(other.allocator);
        bucket_allocator = std::move(other.bucket_allocator);
        sparse = std::exchange(other.sparse, bucket_alloc_pointer{});
        packed = std::exchange(other.packed, alloc_pointer{});
        bucket = std::exchange(other.bucket, 0u);
        count = std::exchange(other.count, 0u);
        reserved = std::exchange(other.reserved, 0u);
        free_list = std::exchange(other.free_list, tombstone);
        mode = other.mode;

        return *this;
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
        return free_list == null ? count : size_type{traits_type::to_entity(free_list)};
    }

    /**
     * @brief Increases the capacity of a sparse set.
     *
     * If the new capacity is greater than the current capacity, new storage is
     * allocated, otherwise the method does nothing.
     *
     * @param cap Desired capacity.
     */
    void reserve(const size_type cap) {
        if(cap > reserved) {
            resize_packed(cap);
        }
    }

    /**
     * @brief Returns the number of elements that a sparse set has currently
     * allocated space for.
     * @return Capacity of the sparse set.
     */
    [[nodiscard]] size_type capacity() const ENTT_NOEXCEPT {
        return reserved;
    }

    /*! @brief Requests the removal of unused capacity. */
    void shrink_to_fit() {
        if(count < reserved) {
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
        return bucket * sparse_page;
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
     * @return An iterator to the first entity of the internal packed array.
     */
    [[nodiscard]] iterator begin() const ENTT_NOEXCEPT {
        return iterator{std::addressof(packed), static_cast<typename traits_type::difference_type>(count)};
    }

    /**
     * @brief Returns an iterator to the end.
     *
     * The returned iterator points to the element following the last entity in
     * the internal packed array. Attempting to dereference the returned
     * iterator results in undefined behavior.
     *
     * @return An iterator to the element following the last entity of the
     * internal packed array.
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
     * the reversed internal packed array. Attempting to dereference the
     * returned iterator results in undefined behavior.
     *
     * @return An iterator to the element following the last entity of the
     * reversed internal packed array.
     */
    [[nodiscard]] reverse_iterator rend() const ENTT_NOEXCEPT {
        return std::make_reverse_iterator(begin());
    }

    /**
     * @brief Finds an entity.
     * @param entt A valid entity identifier.
     * @return An iterator to the given entity if it's found, past the end
     * iterator otherwise.
     */
    [[nodiscard]] iterator find(const entity_type entt) const ENTT_NOEXCEPT {
        return contains(entt) ? --(end() - index(entt)) : end();
    }

    /**
     * @brief Checks if a sparse set contains an entity.
     * @param entt A valid entity identifier.
     * @return True if the sparse set contains the entity, false otherwise.
     */
    [[nodiscard]] bool contains(const entity_type entt) const ENTT_NOEXCEPT {
        ENTT_ASSERT(entt != tombstone && entt != null, "Invalid entity");
        const auto curr = page(entt);
        // testing versions permits to avoid accessing the packed array
        return (curr < bucket && sparse[curr] && sparse[curr][offset(entt)] != null);
    }

    /**
     * @brief Returns the position of an entity in a sparse set.
     *
     * @warning
     * Attempting to get the position of an entity that doesn't belong to the
     * sparse set results in undefined behavior.
     *
     * @param entt A valid entity identifier.
     * @return The position of the entity in the sparse set.
     */
    [[nodiscard]] size_type index(const entity_type entt) const ENTT_NOEXCEPT {
        ENTT_ASSERT(contains(entt), "Set does not contain entity");
        return size_type{traits_type::to_entity(sparse[page(entt)][offset(entt)])};
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
     * @brief Appends an entity to a sparse set.
     *
     * @warning
     * Attempting to assign an entity that already belongs to the sparse set
     * results in undefined behavior.
     *
     * @param entt A valid entity identifier.
     * @return The slot used for insertion.
     */
    size_type emplace_back(const entity_type entt) {
        ENTT_ASSERT(!contains(entt), "Set already contains entity");

        if(count == reserved) {
            const size_type sz = static_cast<size_type>(reserved * growth_factor);
            resize_packed(sz + !(sz > reserved));
        }

        assure_page(page(entt))[offset(entt)] = traits_type::construct(static_cast<typename traits_type::entity_type>(count));
        packed[count] = entt;
        return count++;
    }

    /**
     * @brief Assigns an entity to a sparse set.
     *
     * @warning
     * Attempting to assign an entity that already belongs to the sparse set
     * results in undefined behavior.
     *
     * @param entt A valid entity identifier.
     * @return The slot used for insertion.
     */
    size_type emplace(const entity_type entt) {
        if(free_list == null) {
            return emplace_back(entt);
        } else {
            ENTT_ASSERT(!contains(entt), "Set already contains entity");
            const auto pos = size_type{traits_type::to_entity(free_list)};
            sparse[page(entt)][offset(entt)] = traits_type::construct(static_cast<typename traits_type::entity_type>(pos));
            free_list = std::exchange(packed[pos], entt);
            return pos;
        }
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
     */
    template<typename It>
    void insert(It first, It last) {
        reserve(count + std::distance(first, last));

        for(; first != last; ++first) {
            const auto entt = *first;
            ENTT_ASSERT(!contains(entt), "Set already contains entity");
            assure_page(page(entt))[offset(entt)] = traits_type::construct(static_cast<typename traits_type::entity_type>(count));
            packed[count++] = entt;
        }
    }

    /**
     * @brief Erases an entity from a sparse set.
     *
     * @warning
     * Attempting to erase an entity that doesn't belong to the sparse set
     * results in undefined behavior.
     *
     * @param entt A valid entity identifier.
     * @param ud Optional user data that are forwarded as-is to derived classes.
     */
    void erase(const entity_type entt, void *ud = nullptr) {
        ENTT_ASSERT(contains(entt), "Set does not contain entity");
        (mode == deletion_policy::in_place) ? in_place_pop(entt, ud) : swap_and_pop(entt, ud);
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
     * @param entt A valid entity identifier.
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

        for(auto *it = &free_list; *it != null && next; it = std::addressof(packed[traits_type::to_entity(*it)])) {
            if(const size_type pos = traits_type::to_entity(*it); pos < next) {
                --next;
                move_and_pop(next, pos);
                std::swap(packed[next], packed[pos]);
                sparse[page(packed[pos])][offset(packed[pos])] = traits_type::construct(static_cast<const typename traits_type::entity_type>(pos));
                *it = traits_type::construct(static_cast<typename traits_type::entity_type>(next));
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
     * @param lhs A valid entity identifier.
     * @param rhs A valid entity identifier.
     */
    void swap(const entity_type lhs, const entity_type rhs) {
        ENTT_ASSERT(contains(lhs), "Set does not contain entity");
        ENTT_ASSERT(contains(rhs), "Set does not contain entity");

        auto &entt = sparse[page(lhs)][offset(lhs)];
        auto &other = sparse[page(rhs)][offset(rhs)];

        const auto from = size_type{traits_type::to_entity(entt)};
        const auto to = size_type{traits_type::to_entity(other)};

        // basic no-leak guarantee (with invalid state) if swapping throws
        swap_at(from, to);
        std::swap(entt, other);
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
                sparse[page(entt)][offset(entt)] = traits_type::construct(static_cast<typename traits_type::entity_type>(curr));
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
                    swap(packed[pos], *from);
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
            if(entity != tombstone) {
                in_place_pop(entity, ud);
            }
        }

        free_list = tombstone;
        count = 0u;
    }

private:
    typename alloc_traits::allocator_type allocator;
    typename bucket_alloc_traits::allocator_type bucket_allocator;
    bucket_alloc_pointer sparse;
    alloc_pointer packed;
    std::size_t bucket;
    std::size_t count;
    std::size_t reserved;
    entity_type free_list;
    deletion_policy mode;
};


}


#endif

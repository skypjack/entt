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

    using alloc_type = typename std::allocator_traits<Allocator>::template rebind_alloc<Entity>;
    using alloc_traits = std::allocator_traits<alloc_type>;
    using alloc_pointer = typename alloc_traits::pointer;
    using alloc_const_pointer = typename alloc_traits::const_pointer;

    using bucket_alloc_type = typename std::allocator_traits<Allocator>::template rebind_alloc<alloc_pointer>;
    using bucket_alloc_traits = std::allocator_traits<bucket_alloc_type>;
    using bucket_alloc_pointer = typename bucket_alloc_traits::pointer;

    static_assert(alloc_traits::propagate_on_container_move_assignment::value);
    static_assert(bucket_alloc_traits::propagate_on_container_move_assignment::value);

    class sparse_set_iterator final {
        friend class basic_sparse_set<Entity>;

        using index_type = typename traits_type::difference_type;

        sparse_set_iterator(const alloc_const_pointer *ref, const index_type idx) ENTT_NOEXCEPT
            : packed{ref}, index{idx}
        {}

    public:
        using difference_type = index_type;
        using value_type = Entity;
        using pointer = const value_type *;
        using reference = const value_type &;
        using iterator_category = std::random_access_iterator_tag;

        sparse_set_iterator() ENTT_NOEXCEPT = default;

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
        index_type index;
    };

    [[nodiscard]] static auto page(const Entity entt) ENTT_NOEXCEPT {
        return size_type{(to_integral(entt) & traits_type::entity_mask) / sparse_page};
    }

    [[nodiscard]] static auto offset(const Entity entt) ENTT_NOEXCEPT {
        return size_type{to_integral(entt) & (sparse_page - 1)};
    }

    [[nodiscard]] auto assure_page(const std::size_t idx) {
        if(!(idx < bucket)) {
            const size_type sz = idx + 1u;
            const auto old = std::exchange(sparse, bucket_alloc_traits::allocate(bucket_allocator, sz));
            std::uninitialized_fill(sparse + bucket, sparse + sz, alloc_pointer{});

            for(size_type pos{}; pos < bucket; ++pos) {
                bucket_alloc_traits::construct(bucket_allocator, std::addressof(sparse[pos]), std::move(old[pos]));
                bucket_alloc_traits::destroy(bucket_allocator, std::addressof(old[pos]));
            }

            bucket_alloc_traits::deallocate(bucket_allocator, old, std::exchange(bucket, sz));
        }

        if(!sparse[idx]) {
            sparse[idx] = alloc_traits::allocate(allocator, sparse_page);
            std::uninitialized_fill(sparse[idx], sparse[idx] + sparse_page, null);
        }

        return sparse[idx];
    }

    void resize_packed(const std::size_t req) {
        ENTT_ASSERT((req != reserved) && !(req < count), "Invalid request");
        auto old = std::exchange(packed, alloc_traits::allocate(allocator, req));

        for(size_type pos{}; pos < count; ++pos) {
            alloc_traits::construct(allocator, std::addressof(packed[pos]), std::move(old[pos]));
            alloc_traits::destroy(allocator, std::addressof(old[pos]));
        }

        alloc_traits::deallocate(allocator, old, std::exchange(reserved, req));
    }

    void release_memory() {
        if(packed) {
            ENTT_ASSERT(sparse, "Something very wrong happened");

            std::destroy(packed, packed + std::exchange(count, 0u));
            alloc_traits::deallocate(allocator, std::exchange(packed, alloc_pointer{}), std::exchange(reserved, 0u));

            for(size_type pos{}; pos < bucket; ++pos) {
                if(sparse[pos]) {
                    std::destroy(sparse[pos], sparse[pos] + sparse_page);
                    alloc_traits::deallocate(allocator, sparse[pos], sparse_page);
                }

                bucket_alloc_traits::destroy(bucket_allocator, std::addressof(sparse[pos]));
            }

            bucket_alloc_traits::deallocate(bucket_allocator, sparse, std::exchange(bucket, 0u));
        }
    }

    void push_back(const Entity entt) {
        ENTT_ASSERT(count != reserved, "No more space left");
        assure_page(page(entt))[offset(entt)] = entity_type{static_cast<typename traits_type::entity_type>(count)};
        alloc_traits::construct(allocator, std::addressof(packed[count]), entt);
        // exception safety guarantee requires to update this after construction
        ++count;
    }

    void pop(const Entity entt, void *ud) {
        ENTT_ASSERT(contains(entt), "Set does not contain entity");
        // last chance to use the entity for derived classes and mixins, if any
        about_to_erase(entt, ud);

        auto &ref = sparse[page(entt)][offset(entt)];
        const auto pos = size_type{to_integral(ref)};

        const auto last = --count;
        packed[pos] = std::exchange(packed[last], entt);
        sparse[page(packed[pos])][offset(packed[pos])] = ref;
        // no risks when pos == count, accessing packed is no longer required
        alloc_traits::destroy(allocator, std::addressof(packed[last]));
        ref = null;

        // don't expect exceptions here, instead allow for nosy destructors
        swap_and_pop(pos);
    }

protected:
    /**
     * @brief Swaps two entities in the internal packed array.
     * @param lhs A valid position of an entity within storage.
     * @param rhs A valid position of an entity within storage.
     */
    virtual void swap_at([[maybe_unused]] const std::size_t lhs, [[maybe_unused]] const std::size_t rhs) {}

    /**
     * @brief Attempts to erase an entity from the internal packed array.
     * @param pos A valid position of an entity within storage.
     */
    virtual void swap_and_pop([[maybe_unused]] const std::size_t pos) {}

    /**
     * @brief Last chance to use an entity that is about to be erased.
     * @param entity A valid entity identifier.
     * @param ud Optional user data that are forwarded as-is to derived classes.
     */
    virtual void about_to_erase([[maybe_unused]] const Entity entity, [[maybe_unused]] void *ud) {}

public:
    /*! @brief Allocator type. */
    using allocator_type = alloc_type;
    /*! @brief Underlying entity identifier. */
    using entity_type = Entity;
    /*! @brief Unsigned integer type. */
    using size_type = std::size_t;
    /*! @brief Pointer type to contained entities. */
    using pointer = alloc_const_pointer;
    /*! @brief Random access iterator type. */
    using iterator = sparse_set_iterator;
    /*! @brief Reverse iterator type. */
    using reverse_iterator = pointer;

    /**
     * @brief Default constructor.
     * @param alloc Allocator to use (possibly default-constructed).
     */
    explicit basic_sparse_set(const allocator_type &alloc = {})
        : allocator{alloc},
          bucket_allocator{alloc},
          sparse{bucket_alloc_traits::allocate(bucket_allocator, 0u)},
          packed{alloc_traits::allocate(allocator, 0u)},
          bucket{0u},
          count{0u},
          reserved{0u}
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
          reserved{std::exchange(other.reserved, 0u)}
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

        return *this;
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
        return iterator{&packed, static_cast<typename traits_type::difference_type>(count)};
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
        return iterator{&packed, {}};
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
        return data();
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
        return rbegin() + count;
    }

    /**
     * @brief Finds an entity.
     * @param entt A valid entity identifier.
     * @return An iterator to the given entity if it's found, past the end
     * iterator otherwise.
     */
    [[nodiscard]] iterator find(const entity_type entt) const {
        return contains(entt) ? --(end() - index(entt)) : end();
    }

    /**
     * @brief Checks if a sparse set contains an entity.
     * @param entt A valid entity identifier.
     * @return True if the sparse set contains the entity, false otherwise.
     */
    [[nodiscard]] bool contains(const entity_type entt) const {
        const auto curr = page(entt);
        // testing against null permits to avoid accessing the packed array
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
    [[nodiscard]] size_type index(const entity_type entt) const {
        ENTT_ASSERT(contains(entt), "Set does not contain entity");
        return size_type{to_integral(sparse[page(entt)][offset(entt)])};
    }

    /**
     * @brief Returns the entity at specified location, with bounds checking.
     * @param pos The position for which to return the entity.
     * @return The entity at specified location if any, a null entity otherwise.
     */
    [[nodiscard]] entity_type at(const size_type pos) const {
        return pos < count ? packed[pos] : null;
    }

    /**
     * @brief Returns the entity at specified location, without bounds checking.
     * @param pos The position for which to return the entity.
     * @return The entity at specified location.
     */
    [[nodiscard]] entity_type operator[](const size_type pos) const {
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
     * @param entt A valid entity identifier.
     */
    void emplace(const entity_type entt) {
        ENTT_ASSERT(!contains(entt), "Set already contains entity");

        if(count == reserved) {
            const size_type sz = static_cast<size_type>(reserved * growth_factor);
            resize_packed(sz + !(sz > reserved));
        }

        push_back(entt);
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
            ENTT_ASSERT(!contains(*first), "Set already contains entity");
            push_back(*first);
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
        pop(entt, ud);
    }

    /**
     * @brief Erases multiple entities from a set.
     * @tparam It Type of input iterator.
     * @param first An iterator to the first element of the range of entities.
     * @param last An iterator past the last element of the range of entities.
     * @param ud Optional user data that are forwarded as-is to derived classes.
     */
    template<typename It>
    void erase(It first, It last, void *ud = nullptr) {
        for(; first != last; ++first) {
            pop(*first, ud);
        }
    }

    /**
     * @brief Removes an entity from a sparse set if it exists.
     * @param entt A valid entity identifier.
     * @param ud Optional user data that are forwarded as-is to derived classes.
     * @return True if the entity is actually removed, false otherwise.
     */
    bool remove(const entity_type entt, void *ud = nullptr) {
        return contains(entt) ? (pop(entt, ud), true) : false;
    }

    /**
     * @brief Removes multiple entities from a sparse set if they exist.
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
        const auto from = index(lhs);
        const auto to = index(rhs);

        // derived classes first for a bare-minimum exception safety guarantee
        swap_at(from, to);

        std::swap(sparse[page(lhs)][offset(lhs)], sparse[page(rhs)][offset(rhs)]);
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
        ENTT_ASSERT(!(length > count), "Length exceeds the number of elements");

        algo(std::make_reverse_iterator(packed + length), std::make_reverse_iterator(packed), std::move(compare), std::forward<Args>(args)...);

        for(size_type pos{}; pos < length; ++pos) {
            auto curr = pos;
            auto next = index(packed[curr]);

            while(curr != next) {
                const auto idx = index(packed[next]);
                const auto entt = packed[curr];

                swap_at(next, idx);
                sparse[page(entt)][offset(entt)] = entity_type{static_cast<typename traits_type::entity_type>(curr)};

                curr = next;
                next = idx;
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
        const auto to = other.end();
        auto from = other.begin();

        size_type pos = count - 1;

        while(pos && from != to) {
            if(contains(*from)) {
                if(*from != packed[pos]) {
                    swap(packed[pos], *from);
                }

                --pos;
            }

            ++from;
        }
    }

    /**
     * @brief Clears a sparse set.
     * @param ud Optional user data that are forwarded as-is to derived classes.
     */
    void clear(void *ud = nullptr) ENTT_NOEXCEPT {
        erase(begin(), end(), ud);
    }

private:
    alloc_type allocator;
    bucket_alloc_type bucket_allocator;
    bucket_alloc_pointer sparse;
    alloc_pointer packed;
    std::size_t bucket;
    std::size_t count;
    std::size_t reserved;
};


}


#endif

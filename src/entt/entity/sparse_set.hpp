#ifndef ENTT_ENTITY_SPARSE_SET_HPP
#define ENTT_ENTITY_SPARSE_SET_HPP


#include <iterator>
#include <utility>
#include <vector>
#include <memory>
#include <cstddef>
#include <type_traits>
#include "../config/config.h"
#include "../core/algorithm.hpp"
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
 */
template<typename Entity>
class sparse_set {
    static_assert(ENTT_PAGE_SIZE && ((ENTT_PAGE_SIZE & (ENTT_PAGE_SIZE - 1)) == 0), "ENTT_PAGE_SIZE must be a power of two");
    static constexpr auto entt_per_page = ENTT_PAGE_SIZE / sizeof(Entity);

    using traits_type = entt_traits<Entity>;
    using page_type = std::unique_ptr<Entity[]>;

    class sparse_set_iterator final {
        friend class sparse_set<Entity>;

        using packed_type = std::vector<Entity>;
        using index_type = typename traits_type::difference_type;

        sparse_set_iterator(const packed_type &ref, const index_type idx) ENTT_NOEXCEPT
            : packed{&ref}, index{idx}
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
            return &(*packed)[pos];
        }

        [[nodiscard]] reference operator*() const {
            return *operator->();
        }

    private:
        const packed_type *packed;
        index_type index;
    };

    [[nodiscard]] auto page(const Entity entt) const ENTT_NOEXCEPT {
        return size_type{(to_integral(entt) & traits_type::entity_mask) / entt_per_page};
    }

    [[nodiscard]] auto offset(const Entity entt) const ENTT_NOEXCEPT {
        return size_type{to_integral(entt) & (entt_per_page - 1)};
    }

    [[nodiscard]] page_type & assure(const std::size_t pos) {
        if(!(pos < sparse.size())) {
            sparse.resize(pos+1);
        }

        if(!sparse[pos]) {
            sparse[pos].reset(new entity_type[entt_per_page]);
            // null is safe in all cases for our purposes
            for(auto *first = sparse[pos].get(), *last = first + entt_per_page; first != last; ++first) {
                *first = null;
            }
        }

        return sparse[pos];
    }

public:
    /*! @brief Underlying entity identifier. */
    using entity_type = Entity;
    /*! @brief Unsigned integer type. */
    using size_type = std::size_t;
    /*! @brief Random access iterator type. */
    using iterator = sparse_set_iterator;
    /*! @brief Reverse iterator type. */
    using reverse_iterator = const entity_type *;

    /*! @brief Default constructor. */
    sparse_set() = default;

    /*! @brief Default move constructor. */
    sparse_set(sparse_set &&) = default;

    /*! @brief Default destructor. */
    virtual ~sparse_set() = default;

    /*! @brief Default move assignment operator. @return This sparse set. */
    sparse_set & operator=(sparse_set &&) = default;

    /**
     * @brief Increases the capacity of a sparse set.
     *
     * If the new capacity is greater than the current capacity, new storage is
     * allocated, otherwise the method does nothing.
     *
     * @param cap Desired capacity.
     */
    void reserve(const size_type cap) {
        packed.reserve(cap);
    }

    /**
     * @brief Returns the number of elements that a sparse set has currently
     * allocated space for.
     * @return Capacity of the sparse set.
     */
    [[nodiscard]] size_type capacity() const ENTT_NOEXCEPT {
        return packed.capacity();
    }

    /*! @brief Requests the removal of unused capacity. */
    void shrink_to_fit() {
        // conservative approach
        if(packed.empty()) {
            sparse.clear();
        }

        sparse.shrink_to_fit();
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
    [[nodiscard]] size_type extent() const ENTT_NOEXCEPT {
        return sparse.size() * entt_per_page;
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
        return packed.size();
    }

    /**
     * @brief Checks whether a sparse set is empty.
     * @return True if the sparse set is empty, false otherwise.
     */
    [[nodiscard]] bool empty() const ENTT_NOEXCEPT {
        return packed.empty();
    }

    /**
     * @brief Direct access to the internal packed array.
     *
     * The returned pointer is such that range `[data(), data() + size()]` is
     * always a valid range, even if the container is empty.
     *
     * @note
     * Entities are in the reverse order as returned by the `begin`/`end`
     * iterators.
     *
     * @return A pointer to the internal packed array.
     */
    [[nodiscard]] const entity_type * data() const ENTT_NOEXCEPT {
        return packed.data();
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
        const typename traits_type::difference_type pos = packed.size();
        return iterator{packed, pos};
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
        return iterator{packed, {}};
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
        return packed.data();
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
        return rbegin() + packed.size();
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
        return (curr < sparse.size() && sparse[curr] && sparse[curr][offset(entt)] != null);
    }

    /**
     * @brief Returns the position of an entity in a sparse set.
     *
     * @warning
     * Attempting to get the position of an entity that doesn't belong to the
     * sparse set results in undefined behavior.<br/>
     * An assertion will abort the execution at runtime in debug mode if the
     * sparse set doesn't contain the given entity.
     *
     * @param entt A valid entity identifier.
     * @return The position of the entity in the sparse set.
     */
    [[nodiscard]] size_type index(const entity_type entt) const {
        ENTT_ASSERT(contains(entt));
        return size_type{to_integral(sparse[page(entt)][offset(entt)])};
    }

    /**
     * @brief Assigns an entity to a sparse set.
     *
     * @warning
     * Attempting to assign an entity that already belongs to the sparse set
     * results in undefined behavior.<br/>
     * An assertion will abort the execution at runtime in debug mode if the
     * sparse set already contains the given entity.
     *
     * @param entt A valid entity identifier.
     */
    void emplace(const entity_type entt) {
        ENTT_ASSERT(!contains(entt));
        assure(page(entt))[offset(entt)] = entity_type{static_cast<typename traits_type::entity_type>(packed.size())};
        packed.push_back(entt);
    }

    /**
     * @brief Assigns one or more entities to a sparse set.
     *
     * @warning
     * Attempting to assign an entity that already belongs to the sparse set
     * results in undefined behavior.<br/>
     * An assertion will abort the execution at runtime in debug mode if the
     * sparse set already contains the given entity.
     *
     * @tparam It Type of input iterator.
     * @param first An iterator to the first element of the range of entities.
     * @param last An iterator past the last element of the range of entities.
     */
    template<typename It>
    void insert(It first, It last) {
        auto next = static_cast<typename traits_type::entity_type>(packed.size());
        packed.insert(packed.end(), first, last);

        while(first != last) {
            const auto entt = *(first++);
            ENTT_ASSERT(!contains(entt));
            assure(page(entt))[offset(entt)] = entity_type{next++};
        }
    }

    /**
     * @brief Removes an entity from a sparse set.
     *
     * @warning
     * Attempting to remove an entity that doesn't belong to the sparse set
     * results in undefined behavior.<br/>
     * An assertion will abort the execution at runtime in debug mode if the
     * sparse set doesn't contain the given entity.
     *
     * @param entt A valid entity identifier.
     */
    void erase(const entity_type entt) {
        ENTT_ASSERT(contains(entt));
        const auto curr = page(entt);
        const auto pos = offset(entt);
        packed[size_type{to_integral(sparse[curr][pos])}] = packed.back();
        sparse[page(packed.back())][offset(packed.back())] = sparse[curr][pos];
        sparse[curr][pos] = null;
        packed.pop_back();
    }

    /**
     * @brief Swaps two entities in the internal packed array.
     *
     * For what it's worth, this function affects both the internal sparse array
     * and the internal packed array. Users should not care of that anyway.
     *
     * @warning
     * Attempting to swap entities that don't belong to the sparse set results
     * in undefined behavior.<br/>
     * An assertion will abort the execution at runtime in debug mode if the
     * sparse set doesn't contain the given entities.
     *
     * @param lhs A valid entity identifier.
     * @param rhs A valid entity identifier.
     */
    virtual void swap(const entity_type lhs, const entity_type rhs) {
        auto &from = sparse[page(lhs)][offset(lhs)];
        auto &to = sparse[page(rhs)][offset(rhs)];
        std::swap(packed[size_type{to_integral(from)}], packed[size_type{to_integral(to)}]);
        std::swap(from, to);
    }

    /**
     * @brief Sort elements according to the given comparison function.
     *
     * Sort the elements so that iterating the range with a couple of iterators
     * returns them in the expected order. See `begin` and `end` for more
     * details.
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
     * The sort function oject must offer a member function template
     * `operator()` that accepts three arguments:
     *
     * * An iterator to the first element of the range to sort.
     * * An iterator past the last element of the range to sort.
     * * A comparison function to use to compare the elements.
     *
     * @tparam Compare Type of comparison function object.
     * @tparam Sort Type of sort function object.
     * @tparam Args Types of arguments to forward to the sort function object.
     * @param first An iterator to the first element of the range to sort.
     * @param last An iterator past the last element of the range to sort.
     * @param compare A valid comparison function object.
     * @param algo A valid sort function object.
     * @param args Arguments to forward to the sort function object, if any.
     */
    template<typename Compare, typename Sort = std_sort, typename... Args>
    void sort(iterator first, iterator last, Compare compare, Sort algo = Sort{}, Args &&... args) {
        ENTT_ASSERT(!(last < first));
        ENTT_ASSERT(!(last > end()));

        const auto length = std::distance(first, last);
        const auto skip = std::distance(last, end());
        const auto to = packed.rend() - skip;
        const auto from = to - length;

        algo(from, to, std::move(compare), std::forward<Args>(args)...);

        for(size_type pos = skip, end = skip+length; pos < end; ++pos) {
            sparse[page(packed[pos])][offset(packed[pos])] = entity_type{static_cast<typename traits_type::entity_type>(pos)};
        }
    }

    /**
     * @brief Sort elements according to the given comparison function.
     *
     * @sa sort
     *
     * This function is a slightly slower version of `sort` that invokes the
     * caller to indicate which entities are swapped.<br/>
     * It's recommended when the caller wants to sort its own data structures to
     * align them with the order induced in the sparse set.
     *
     * The signature of the callback should be equivalent to the following:
     *
     * @code{.cpp}
     * bool(const Entity, const Entity);
     * @endcode
     *
     * @tparam Apply Type of function object to invoke to notify the caller.
     * @tparam Compare Type of comparison function object.
     * @tparam Sort Type of sort function object.
     * @tparam Args Types of arguments to forward to the sort function object.
     * @param first An iterator to the first element of the range to sort.
     * @param last An iterator past the last element of the range to sort.
     * @param apply A valid function object to use as a callback.
     * @param compare A valid comparison function object.
     * @param algo A valid sort function object.
     * @param args Arguments to forward to the sort function object, if any.
     */
    template<typename Apply, typename Compare, typename Sort = std_sort, typename... Args>
    void arrange(iterator first, iterator last, Apply apply, Compare compare, Sort algo = Sort{}, Args &&... args) {
        ENTT_ASSERT(!(last < first));
        ENTT_ASSERT(!(last > end()));

        const auto length = std::distance(first, last);
        const auto skip = std::distance(last, end());
        const auto to = packed.rend() - skip;
        const auto from = to - length;

        algo(from, to, std::move(compare), std::forward<Args>(args)...);

        for(size_type pos = skip, end = skip+length; pos < end; ++pos) {
            auto curr = pos;
            auto next = index(packed[curr]);

            while(curr != next) {
                apply(packed[curr], packed[next]);
                sparse[page(packed[curr])][offset(packed[curr])] = entity_type{static_cast<typename traits_type::entity_type>(curr)};

                curr = next;
                next = index(packed[curr]);
            }
        }
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
    void respect(const sparse_set &other) {
        const auto to = other.end();
        auto from = other.begin();

        size_type pos = packed.size() - 1;

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
     */
    void clear() ENTT_NOEXCEPT {
        sparse.clear();
        packed.clear();
    }

private:
    std::vector<page_type> sparse;
    std::vector<entity_type> packed;
};


}


#endif

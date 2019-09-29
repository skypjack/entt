#ifndef ENTT_ENTITY_SPARSE_SET_HPP
#define ENTT_ENTITY_SPARSE_SET_HPP


#include <algorithm>
#include <iterator>
#include <utility>
#include <vector>
#include <memory>
#include <cstddef>
#include <numeric>
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
 * There are no guarantees that entities are returned in the insertion order
 * when iterate a sparse set. Do not make assumption on the order in any case.
 *
 * @note
 * Internal data structures arrange elements to maximize performance. Because of
 * that, there are no guarantees that elements have the expected order when
 * iterate directly the internal packed array (see `data` and `size` member
 * functions for that). Use `begin` and `end` instead.
 *
 * @tparam Entity A valid entity type (see entt_traits for more details).
 */
template<typename Entity>
class sparse_set {
    using traits_type = entt_traits<std::underlying_type_t<Entity>>;

    static_assert(ENTT_PAGE_SIZE && ((ENTT_PAGE_SIZE & (ENTT_PAGE_SIZE - 1)) == 0));
    static constexpr auto entt_per_page = ENTT_PAGE_SIZE / sizeof(typename traits_type::entity_type);

    class iterator {
        friend class sparse_set<Entity>;

        using direct_type = const std::vector<Entity>;
        using index_type = typename traits_type::difference_type;

        iterator(direct_type *ref, const index_type idx) ENTT_NOEXCEPT
            : direct{ref}, index{idx}
        {}

    public:
        using difference_type = index_type;
        using value_type = Entity;
        using pointer = const value_type *;
        using reference = const value_type &;
        using iterator_category = std::random_access_iterator_tag;

        iterator() ENTT_NOEXCEPT = default;

        iterator & operator++() ENTT_NOEXCEPT {
            return --index, *this;
        }

        iterator operator++(int) ENTT_NOEXCEPT {
            iterator orig = *this;
            return ++(*this), orig;
        }

        iterator & operator--() ENTT_NOEXCEPT {
            return ++index, *this;
        }

        iterator operator--(int) ENTT_NOEXCEPT {
            iterator orig = *this;
            return --(*this), orig;
        }

        iterator & operator+=(const difference_type value) ENTT_NOEXCEPT {
            index -= value;
            return *this;
        }

        iterator operator+(const difference_type value) const ENTT_NOEXCEPT {
            return iterator{direct, index-value};
        }

        iterator & operator-=(const difference_type value) ENTT_NOEXCEPT {
            return (*this += -value);
        }

        iterator operator-(const difference_type value) const ENTT_NOEXCEPT {
            return (*this + -value);
        }

        difference_type operator-(const iterator &other) const ENTT_NOEXCEPT {
            return other.index - index;
        }

        reference operator[](const difference_type value) const ENTT_NOEXCEPT {
            const auto pos = size_type(index-value-1);
            return (*direct)[pos];
        }

        bool operator==(const iterator &other) const ENTT_NOEXCEPT {
            return other.index == index;
        }

        bool operator!=(const iterator &other) const ENTT_NOEXCEPT {
            return !(*this == other);
        }

        bool operator<(const iterator &other) const ENTT_NOEXCEPT {
            return index > other.index;
        }

        bool operator>(const iterator &other) const ENTT_NOEXCEPT {
            return index < other.index;
        }

        bool operator<=(const iterator &other) const ENTT_NOEXCEPT {
            return !(*this > other);
        }

        bool operator>=(const iterator &other) const ENTT_NOEXCEPT {
            return !(*this < other);
        }

        pointer operator->() const ENTT_NOEXCEPT {
            const auto pos = size_type(index-1);
            return &(*direct)[pos];
        }

        reference operator*() const ENTT_NOEXCEPT {
            return *operator->();
        }

    private:
        direct_type *direct;
        index_type index;
    };

    void assure(const std::size_t page) {
        if(!(page < reverse.size())) {
            reverse.resize(page+1);
        }

        if(!reverse[page]) {
            reverse[page] = std::make_unique<entity_type[]>(entt_per_page);
            // null is safe in all cases for our purposes
            std::fill_n(reverse[page].get(), entt_per_page, null);
        }
    }

    auto map(const Entity entt) const ENTT_NOEXCEPT {
        const auto identifier = to_integer(entt) & traits_type::entity_mask;
        const auto page = size_type(identifier / entt_per_page);
        const auto offset = size_type(identifier & (entt_per_page - 1));
        return std::make_pair(page, offset);
    }

public:
    /*! @brief Underlying entity identifier. */
    using entity_type = Entity;
    /*! @brief Unsigned integer type. */
    using size_type = std::size_t;
    /*! @brief Random access iterator type. */
    using iterator_type = iterator;

    /*! @brief Default constructor. */
    sparse_set() = default;

    /**
     * @brief Copy constructor.
     * @param other The instance to copy from.
     */
    sparse_set(const sparse_set &other)
        : reverse{},
          direct{other.direct}
    {
        for(size_type pos{}, last = other.reverse.size(); pos < last; ++pos) {
            if(other.reverse[pos]) {
                assure(pos);
                std::copy_n(other.reverse[pos].get(), entt_per_page, reverse[pos].get());
            }
        }
    }

    /*! @brief Default move constructor. */
    sparse_set(sparse_set &&) = default;

    /*! @brief Default destructor. */
    virtual ~sparse_set() ENTT_NOEXCEPT = default;

    /**
     * @brief Copy assignment operator.
     * @param other The instance to copy from.
     * @return This sparse set.
     */
    sparse_set & operator=(const sparse_set &other) {
        if(&other != this) {
            auto tmp{other};
            *this = std::move(tmp);
        }

        return *this;
    }

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
        direct.reserve(cap);
    }

    /**
     * @brief Returns the number of elements that a sparse set has currently
     * allocated space for.
     * @return Capacity of the sparse set.
     */
    size_type capacity() const ENTT_NOEXCEPT {
        return direct.capacity();
    }

    /*! @brief Requests the removal of unused capacity. */
    void shrink_to_fit() {
        // conservative approach
        if(direct.empty()) {
            reverse.clear();
        }

        reverse.shrink_to_fit();
        direct.shrink_to_fit();
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
    size_type extent() const ENTT_NOEXCEPT {
        return reverse.size() * entt_per_page;
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
    size_type size() const ENTT_NOEXCEPT {
        return direct.size();
    }

    /**
     * @brief Checks whether a sparse set is empty.
     * @return True if the sparse set is empty, false otherwise.
     */
    bool empty() const ENTT_NOEXCEPT {
        return direct.empty();
    }

    /**
     * @brief Direct access to the internal packed array.
     *
     * The returned pointer is such that range `[data(), data() + size()]` is
     * always a valid range, even if the container is empty.
     *
     * @note
     * There are no guarantees on the order, even though `respect` has been
     * previously invoked. Internal data structures arrange elements to maximize
     * performance. Accessing them directly gives a performance boost but less
     * guarantees. Use `begin` and `end` if you want to iterate the sparse set
     * in the expected order.
     *
     * @return A pointer to the internal packed array.
     */
    const entity_type * data() const ENTT_NOEXCEPT {
        return direct.data();
    }

    /**
     * @brief Returns an iterator to the beginning.
     *
     * The returned iterator points to the first entity of the internal packed
     * array. If the sparse set is empty, the returned iterator will be equal to
     * `end()`.
     *
     * @note
     * Random access iterators stay true to the order imposed by a call to
     * `respect`.
     *
     * @return An iterator to the first entity of the internal packed array.
     */
    iterator_type begin() const ENTT_NOEXCEPT {
        const typename traits_type::difference_type pos = direct.size();
        return iterator_type{&direct, pos};
    }

    /**
     * @brief Returns an iterator to the end.
     *
     * The returned iterator points to the element following the last entity in
     * the internal packed array. Attempting to dereference the returned
     * iterator results in undefined behavior.
     *
     * @note
     * Random access iterators stay true to the order imposed by a call to
     * `respect`.
     *
     * @return An iterator to the element following the last entity of the
     * internal packed array.
     */
    iterator_type end() const ENTT_NOEXCEPT {
        return iterator_type{&direct, {}};
    }

    /**
     * @brief Finds an entity.
     * @param entt A valid entity identifier.
     * @return An iterator to the given entity if it's found, past the end
     * iterator otherwise.
     */
    iterator_type find(const entity_type entt) const ENTT_NOEXCEPT {
        return has(entt) ? --(end() - index(entt)) : end();
    }

    /**
     * @brief Checks if a sparse set contains an entity.
     * @param entt A valid entity identifier.
     * @return True if the sparse set contains the entity, false otherwise.
     */
    bool has(const entity_type entt) const ENTT_NOEXCEPT {
        auto [page, offset] = map(entt);
        // testing against null permits to avoid accessing the direct vector
        return (page < reverse.size() && reverse[page] && reverse[page][offset] != null);
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
    size_type index(const entity_type entt) const ENTT_NOEXCEPT {
        ENTT_ASSERT(has(entt));
        auto [page, offset] = map(entt);
        return size_type(reverse[page][offset]);
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
    void construct(const entity_type entt) {
        ENTT_ASSERT(!has(entt));
        auto [page, offset] = map(entt);
        assure(page);
        reverse[page][offset] = entity_type(direct.size());
        direct.push_back(entt);
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
     * @tparam It Type of forward iterator.
     * @param first An iterator to the first element of the range of entities.
     * @param last An iterator past the last element of the range of entities.
     */
    template<typename It>
    void batch(It first, It last) {
        std::for_each(first, last, [this, next = direct.size()](const auto entt) mutable {
            ENTT_ASSERT(!has(entt));
            auto [page, offset] = map(entt);
            assure(page);
            reverse[page][offset] = entity_type(next++);
        });

        direct.insert(direct.end(), first, last);
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
    void destroy(const entity_type entt) {
        ENTT_ASSERT(has(entt));
        auto [from_page, from_offset] = map(entt);
        auto [to_page, to_offset] = map(direct.back());
        direct[size_type(reverse[from_page][from_offset])] = entity_type(direct.back());
        reverse[to_page][to_offset] = reverse[from_page][from_offset];
        reverse[from_page][from_offset] = null;
        direct.pop_back();
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
    virtual void swap(const entity_type lhs, const entity_type rhs) ENTT_NOEXCEPT {
        auto [src_page, src_offset] = map(lhs);
        auto [dst_page, dst_offset] = map(rhs);
        auto &from = reverse[src_page][src_offset];
        auto &to = reverse[dst_page][dst_offset];
        std::swap(direct[size_type(from)], direct[size_type(to)]);
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
     * @note
     * Attempting to iterate elements using a raw pointer returned by a call to
     * `data` gives no guarantees on the order, even though `sort` has been
     * invoked.
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
    void sort(iterator_type first, iterator_type last, Compare compare, Sort algo = Sort{}, Args &&... args) {
        ENTT_ASSERT(!(last < first));
        ENTT_ASSERT(!(last > end()));

        const auto length = std::distance(first, last);
        const auto skip = std::distance(last, end());
        const auto to = direct.rend() - skip;
        const auto from = to - length;

        algo(from, to, std::move(compare), std::forward<Args>(args)...);

        for(size_type pos = skip, end = skip+length; pos < end; ++pos) {
            auto [page, offset] = map(direct[pos]);
            reverse[page][offset] = entity_type(pos);
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
    void arrange(iterator_type first, iterator_type last, Apply apply, Compare compare, Sort algo = Sort{}, Args &&... args) {
        ENTT_ASSERT(!(last < first));
        ENTT_ASSERT(!(last > end()));

        const auto length = std::distance(first, last);
        const auto skip = std::distance(last, end());
        const auto to = direct.rend() - skip;
        const auto from = to - length;

        algo(from, to, std::move(compare), std::forward<Args>(args)...);

        for(size_type pos = skip, end = skip+length; pos < end; ++pos) {
            auto curr = pos;
            auto next = index(direct[curr]);

            while(curr != next) {
                apply(direct[curr], direct[next]);
                auto [page, offset] = map(direct[curr]);
                reverse[page][offset] = entity_type(curr);

                curr = next;
                next = index(direct[curr]);
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
     * @note
     * Attempting to iterate elements using a raw pointer returned by a call to
     * `data` gives no guarantees on the order, even though `respect` has been
     * invoked.
     *
     * @param other The sparse sets that imposes the order of the entities.
     */
    void respect(const sparse_set &other) ENTT_NOEXCEPT {
        const auto to = other.end();
        auto from = other.begin();

        size_type pos = direct.size() - 1;

        while(pos && from != to) {
            if(has(*from)) {
                if(*from != direct[pos]) {
                    swap(direct[pos], *from);
                }

                --pos;
            }

            ++from;
        }
    }

    /**
     * @brief Resets a sparse set.
     */
    void reset() {
        reverse.clear();
        direct.clear();
    }

private:
    std::vector<std::unique_ptr<entity_type[]>> reverse;
    std::vector<entity_type> direct;
};


}


#endif // ENTT_ENTITY_SPARSE_SET_HPP

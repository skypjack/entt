#ifndef ENTT_ENTITY_SPARSE_SET_HPP
#define ENTT_ENTITY_SPARSE_SET_HPP


#include <algorithm>
#include <iterator>
#include <numeric>
#include <utility>
#include <vector>
#include <memory>
#include <cstddef>
#include <type_traits>
#include "../config/config.h"
#include "../core/algorithm.hpp"
#include "entt_traits.hpp"
#include "entity.hpp"


namespace entt {


/**
 * @brief Sparse set.
 *
 * Primary template isn't defined on purpose. All the specializations give a
 * compile-time error, but for a few reasonable cases.
 */
template<typename...>
class sparse_set;


/**
 * @brief Basic sparse set implementation.
 *
 * Sparse set or packed array or whatever is the name users give it.<br/>
 * Two arrays: an _external_ one and an _internal_ one; a _sparse_ one and a
 * _packed_ one; one used for direct access through contiguous memory, the other
 * one used to get the data through an extra level of indirection.<br/>
 * This is largely used by the registry to offer users the fastest access ever
 * to the components. Views in general are almost entirely designed around
 * sparse sets.
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
class sparse_set<Entity> {
    using traits_type = entt_traits<Entity>;

    static_assert(ENTT_PAGE_SIZE && ((ENTT_PAGE_SIZE & (ENTT_PAGE_SIZE - 1)) == 0));
    static constexpr auto entt_per_page = ENTT_PAGE_SIZE / sizeof(typename entt_traits<Entity>::entity_type);

    class iterator {
        friend class sparse_set<Entity>;

        using direct_type = const std::vector<Entity>;
        using index_type = typename traits_type::difference_type;

        iterator(direct_type *ref, index_type idx) ENTT_NOEXCEPT
            : direct{ref}, index{idx}
        {}

    public:
        using difference_type = index_type;
        using value_type = const Entity;
        using pointer = value_type *;
        using reference = value_type &;
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

        inline iterator & operator-=(const difference_type value) ENTT_NOEXCEPT {
            return (*this += -value);
        }

        inline iterator operator-(const difference_type value) const ENTT_NOEXCEPT {
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

        inline bool operator!=(const iterator &other) const ENTT_NOEXCEPT {
            return !(*this == other);
        }

        bool operator<(const iterator &other) const ENTT_NOEXCEPT {
            return index > other.index;
        }

        bool operator>(const iterator &other) const ENTT_NOEXCEPT {
            return index < other.index;
        }

        inline bool operator<=(const iterator &other) const ENTT_NOEXCEPT {
            return !(*this > other);
        }

        inline bool operator>=(const iterator &other) const ENTT_NOEXCEPT {
            return !(*this < other);
        }

        pointer operator->() const ENTT_NOEXCEPT {
            const auto pos = size_type(index-1);
            return &(*direct)[pos];
        }

        inline reference operator*() const ENTT_NOEXCEPT {
            return *operator->();
        }

    private:
        direct_type *direct;
        index_type index;
    };

    void assure(std::size_t page) {
        if(!(page < reverse.size())) {
            reverse.resize(page+1);
        }

        if(!reverse[page].first) {
            reverse[page].first = std::make_unique<entity_type[]>(entt_per_page);
            // null is safe in all cases for our purposes
            std::fill_n(reverse[page].first.get(), entt_per_page, null);
        }
    }

    auto index(Entity entt) const ENTT_NOEXCEPT {
        const auto identifier = entt & traits_type::entity_mask;
        const auto page = size_type(identifier / entt_per_page);
        const auto offset = size_type(identifier & (entt_per_page - 1));
        return std::make_pair(page, offset);
    }

public:
    /*! @brief Underlying entity identifier. */
    using entity_type = Entity;
    /*! @brief Unsigned integer type. */
    using size_type = std::size_t;
    /*! @brief Input iterator type. */
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
        for(size_type i = {}, last = other.reverse.size(); i < last; ++i) {
            if(other.reverse[i].first) {
                assure(i);
                std::copy_n(other.reverse[i].first.get(), entt_per_page, reverse[i].first.get());
                reverse[i].second = other.reverse[i].second;
            }
        }
    }

    /*! @brief Default move constructor. */
    sparse_set(sparse_set &&) = default;

    /*! @brief Default destructor. */
    virtual ~sparse_set() ENTT_NOEXCEPT = default;

    /*! @brief Default move assignment operator. @return This sparse set. */
    sparse_set & operator=(sparse_set &&) = default;

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

    /**
     * @brief Increases the capacity of a sparse set.
     *
     * If the new capacity is greater than the current capacity, new storage is
     * allocated, otherwise the method does nothing.
     *
     * @param cap Desired capacity.
     */
    virtual void reserve(const size_type cap) {
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
    virtual void shrink_to_fit() {
        while(!reverse.empty() && !reverse.back().second) {
            reverse.pop_back();
        }

        for(auto &&data: reverse) {
            if(!data.second) {
                data.first.reset();
            }
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
     * Input iterators stay true to the order imposed by a call to `respect`.
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
     * Input iterators stay true to the order imposed by a call to `respect`.
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
        return has(entt) ? --(end() - get(entt)) : end();
    }

    /**
     * @brief Checks if a sparse set contains an entity.
     * @param entt A valid entity identifier.
     * @return True if the sparse set contains the entity, false otherwise.
     */
    bool has(const entity_type entt) const ENTT_NOEXCEPT {
        auto [page, offset] = index(entt);
        // testing against null permits to avoid accessing the direct vector
        return (page < reverse.size() && reverse[page].second && reverse[page].first[offset] != null);
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
    size_type get(const entity_type entt) const ENTT_NOEXCEPT {
        ENTT_ASSERT(has(entt));
        auto [page, offset] = index(entt);
        return size_type(reverse[page].first[offset]);
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
        auto [page, offset] = index(entt);
        assure(page);
        reverse[page].first[offset] = entity_type(direct.size());
        reverse[page].second++;
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
        std::for_each(first, last, [next = entity_type(direct.size()), this](const auto entt) mutable {
            ENTT_ASSERT(!has(entt));
            auto [page, offset] = index(entt);
            assure(page);
            reverse[page].first[offset] = next++;
            reverse[page].second++;
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
    virtual void destroy(const entity_type entt) {
        ENTT_ASSERT(has(entt));
        auto [from_page, from_offset] = index(entt);
        auto [to_page, to_offset] = index(direct.back());
        std::swap(direct[size_type(reverse[from_page].first[from_offset])], direct.back());
        std::swap(reverse[from_page].first[from_offset], reverse[to_page].first[to_offset]);
        reverse[from_page].first[from_offset] = null;
        reverse[from_page].second--;
        direct.pop_back();
    }

    /**
     * @brief Swaps the position of two entities in the internal packed array.
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
     * @param lhs A valid position within the sparse set.
     * @param rhs A valid position within the sparse set.
     */
    void swap(const size_type lhs, const size_type rhs) ENTT_NOEXCEPT {
        ENTT_ASSERT(lhs < direct.size());
        ENTT_ASSERT(rhs < direct.size());
        auto [src_page, src_offset] = index(direct[lhs]);
        auto [dst_page, dst_offset] = index(direct[rhs]);
        std::swap(reverse[src_page].first[src_offset], reverse[dst_page].first[dst_offset]);
        std::swap(direct[lhs], direct[rhs]);
    }

    /**
     * @brief Sort entities according to their order in another sparse set.
     *
     * Entities that are part of both the sparse sets are ordered internally
     * according to the order they have in `other`. All the other entities goes
     * to the end of the list and there are no guarantess on their order.<br/>
     * In other terms, this function can be used to impose the same order on two
     * sets by using one of them as a master and the other one as a slave.
     *
     * Iterating the sparse set with a couple of iterators returns elements in
     * the expected order after a call to `respect`. See `begin` and `end` for
     * more details.
     *
     * @note
     * Attempting to iterate elements using the raw pointer returned by `data`
     * gives no guarantees on the order, even though `respect` has been invoked.
     *
     * @param other The sparse sets that imposes the order of the entities.
     */
    virtual void respect(const sparse_set &other) ENTT_NOEXCEPT {
        const auto to = other.end();
        auto from = other.begin();

        size_type pos = direct.size() - 1;

        while(pos && from != to) {
            if(has(*from)) {
                if(*from != direct[pos]) {
                    swap(pos, get(*from));
                }

                --pos;
            }

            ++from;
        }
    }

    /**
     * @brief Resets a sparse set.
     */
    virtual void reset() {
        reverse.clear();
        direct.clear();
    }

    /**
     * @brief Clones and returns a sparse set.
     *
     * The basic implementation of a sparse set is always copyable. Therefore,
     * the returned instance is always valid.
     *
     * @return A fresh copy of the given sparse set.
     */
    virtual std::unique_ptr<sparse_set> clone() const {
        return std::make_unique<sparse_set>(*this);
    }

private:
    std::vector<std::pair<std::unique_ptr<entity_type[]>, size_type>> reverse;
    std::vector<entity_type> direct;
};


/**
 * @brief Extended sparse set implementation.
 *
 * This specialization of a sparse set associates an object to an entity. The
 * main purpose of this class is to use sparse sets to store components in a
 * registry. It guarantees fast access both to the elements and to the entities.
 *
 * @note
 * Entities and objects have the same order. It's guaranteed both in case of raw
 * access (either to entities or objects) and when using input iterators.
 *
 * @note
 * Internal data structures arrange elements to maximize performance. Because of
 * that, there are no guarantees that elements have the expected order when
 * iterate directly the internal packed array (see `raw` and `size` member
 * functions for that). Use `begin` and `end` instead.
 *
 * @sa sparse_set<Entity>
 *
 * @tparam Entity A valid entity type (see entt_traits for more details).
 * @tparam Type Type of objects assigned to the entities.
 */
template<typename Entity, typename Type>
class sparse_set<Entity, Type>: public sparse_set<Entity> {
    using underlying_type = sparse_set<Entity>;
    using traits_type = entt_traits<Entity>;

    template<bool Const, bool = std::is_empty_v<Type>>
    class iterator {
        friend class sparse_set<Entity, Type>;

        using instance_type = std::conditional_t<Const, const std::vector<Type>, std::vector<Type>>;
        using index_type = typename traits_type::difference_type;

        iterator(instance_type *ref, index_type idx) ENTT_NOEXCEPT
            : instances{ref}, index{idx}
        {}

    public:
        using difference_type = index_type;
        using value_type = std::conditional_t<Const, const Type, Type>;
        using pointer = value_type *;
        using reference = value_type &;
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
            return iterator{instances, index-value};
        }

        inline iterator & operator-=(const difference_type value) ENTT_NOEXCEPT {
            return (*this += -value);
        }

        inline iterator operator-(const difference_type value) const ENTT_NOEXCEPT {
            return (*this + -value);
        }

        difference_type operator-(const iterator &other) const ENTT_NOEXCEPT {
            return other.index - index;
        }

        reference operator[](const difference_type value) const ENTT_NOEXCEPT {
            const auto pos = size_type(index-value-1);
            return (*instances)[pos];
        }

        bool operator==(const iterator &other) const ENTT_NOEXCEPT {
            return other.index == index;
        }

        inline bool operator!=(const iterator &other) const ENTT_NOEXCEPT {
            return !(*this == other);
        }

        bool operator<(const iterator &other) const ENTT_NOEXCEPT {
            return index > other.index;
        }

        bool operator>(const iterator &other) const ENTT_NOEXCEPT {
            return index < other.index;
        }

        inline bool operator<=(const iterator &other) const ENTT_NOEXCEPT {
            return !(*this > other);
        }

        inline bool operator>=(const iterator &other) const ENTT_NOEXCEPT {
            return !(*this < other);
        }

        pointer operator->() const ENTT_NOEXCEPT {
            const auto pos = size_type(index-1);
            return &(*instances)[pos];
        }

        inline reference operator*() const ENTT_NOEXCEPT {
            return *operator->();
        }

    private:
        instance_type *instances;
        index_type index;
    };

    template<bool Const>
    class iterator<Const, true> {
        friend class sparse_set<Entity, Type>;

        using instance_type = std::conditional_t<Const, const Type, Type>;
        using index_type = typename traits_type::difference_type;

        iterator(instance_type *ref, index_type idx) ENTT_NOEXCEPT
            : instance{ref}, index{idx}
        {}

    public:
        using difference_type = index_type;
        using value_type = std::conditional_t<Const, const Type, Type>;
        using pointer = value_type *;
        using reference = value_type &;
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
            return iterator{instance, index-value};
        }

        inline iterator & operator-=(const difference_type value) ENTT_NOEXCEPT {
            return (*this += -value);
        }

        inline iterator operator-(const difference_type value) const ENTT_NOEXCEPT {
            return (*this + -value);
        }

        difference_type operator-(const iterator &other) const ENTT_NOEXCEPT {
            return other.index - index;
        }

        reference operator[](const difference_type) const ENTT_NOEXCEPT {
            return *instance;
        }

        bool operator==(const iterator &other) const ENTT_NOEXCEPT {
            return other.index == index;
        }

        inline bool operator!=(const iterator &other) const ENTT_NOEXCEPT {
            return !(*this == other);
        }

        bool operator<(const iterator &other) const ENTT_NOEXCEPT {
            return index > other.index;
        }

        bool operator>(const iterator &other) const ENTT_NOEXCEPT {
            return index < other.index;
        }

        inline bool operator<=(const iterator &other) const ENTT_NOEXCEPT {
            return !(*this > other);
        }

        inline bool operator>=(const iterator &other) const ENTT_NOEXCEPT {
            return !(*this < other);
        }

        pointer operator->() const ENTT_NOEXCEPT {
            return instance;
        }

        inline reference operator*() const ENTT_NOEXCEPT {
            return *operator->();
        }

    private:
        instance_type *instance;
        index_type index;
    };

public:
    /*! @brief Type of the objects associated with the entities. */
    using object_type = Type;
    /*! @brief Underlying entity identifier. */
    using entity_type = typename underlying_type::entity_type;
    /*! @brief Unsigned integer type. */
    using size_type = typename underlying_type::size_type;
    /*! @brief Input iterator type. */
    using iterator_type = iterator<false>;
    /*! @brief Constant input iterator type. */
    using const_iterator_type = iterator<true>;

    /**
     * @brief Increases the capacity of a sparse set.
     *
     * If the new capacity is greater than the current capacity, new storage is
     * allocated, otherwise the method does nothing.
     *
     * @param cap Desired capacity.
     */
    void reserve(const size_type cap) override {
        underlying_type::reserve(cap);

        if constexpr(!std::is_empty_v<object_type>) {
            instances.reserve(cap);
        }
    }

    /**
     * @brief Requests the removal of unused capacity.
     *
     * @note
     * Empty components aren't explicitly instantiated. Only one instance of the
     * given type is created. Therefore, this function does nothing.
     */
    void shrink_to_fit() override {
        underlying_type::shrink_to_fit();

        if constexpr(!std::is_empty_v<object_type>) {
            instances.shrink_to_fit();
        }
    }

    /**
     * @brief Direct access to the array of objects.
     *
     * The returned pointer is such that range `[raw(), raw() + size()]` is
     * always a valid range, even if the container is empty.
     *
     * @note
     * There are no guarantees on the order, even though either `sort` or
     * `respect` has been previously invoked. Internal data structures arrange
     * elements to maximize performance. Accessing them directly gives a
     * performance boost but less guarantees. Use `begin` and `end` if you want
     * to iterate the sparse set in the expected order.
     *
     * @note
     * Empty components aren't explicitly instantiated. Only one instance of the
     * given type is created. Therefore, this function always returns a pointer
     * to that instance.
     *
     * @return A pointer to the array of objects.
     */
    const object_type * raw() const ENTT_NOEXCEPT {
        if constexpr(std::is_empty_v<object_type>) {
            return &instances;
        } else {
            return instances.data();
        }
    }

    /*! @copydoc raw */
    object_type * raw() ENTT_NOEXCEPT {
        return const_cast<object_type *>(std::as_const(*this).raw());
    }

    /**
     * @brief Returns the entity to which a given component is assigned.
     * @param instance A valid reference to an object.
     * @return A valid entity identifier if the instance belongs to the sparse
     * set, the null entity otherwise.
     */
    inline entity_type entity(const object_type &instance) {
        const auto address = std::addressof(instance);
        const bool valid = !(instances.data() > address) && (address < (instances.data() + instances.size()));
        return valid ? sparse_set<entity_type>::data()[address - instances.data()] : null;
    }

    /**
     * @brief Returns an iterator to the beginning.
     *
     * The returned iterator points to the first instance of the given type. If
     * the sparse set is empty, the returned iterator will be equal to `end()`.
     *
     * @note
     * Input iterators stay true to the order imposed by a call to either `sort`
     * or `respect`.
     *
     * @return An iterator to the first instance of the given type.
     */
    const_iterator_type cbegin() const ENTT_NOEXCEPT {
        const typename traits_type::difference_type pos = underlying_type::size();
        return const_iterator_type{&instances, pos};
    }

    /*! @copydoc cbegin */
    inline const_iterator_type begin() const ENTT_NOEXCEPT {
        return cbegin();
    }

    /*! @copydoc begin */
    iterator_type begin() ENTT_NOEXCEPT {
        const typename traits_type::difference_type pos = underlying_type::size();
        return iterator_type{&instances, pos};
    }

    /**
     * @brief Returns an iterator to the end.
     *
     * The returned iterator points to the element following the last instance
     * of the given type. Attempting to dereference the returned iterator
     * results in undefined behavior.
     *
     * @note
     * Input iterators stay true to the order imposed by a call to either `sort`
     * or `respect`.
     *
     * @return An iterator to the element following the last instance of the
     * given type.
     */
    const_iterator_type cend() const ENTT_NOEXCEPT {
        return const_iterator_type{&instances, {}};
    }

    /*! @copydoc cend */
    inline const_iterator_type end() const ENTT_NOEXCEPT {
        return cend();
    }

    /*! @copydoc end */
    iterator_type end() ENTT_NOEXCEPT {
        return iterator_type{&instances, {}};
    }

    /**
     * @brief Returns the object associated with an entity.
     *
     * @warning
     * Attempting to use an entity that doesn't belong to the sparse set results
     * in undefined behavior.<br/>
     * An assertion will abort the execution at runtime in debug mode if the
     * sparse set doesn't contain the given entity.
     *
     * @param entt A valid entity identifier.
     * @return The object associated with the entity.
     */
    const object_type & get([[maybe_unused]] const entity_type entt) const ENTT_NOEXCEPT {
        if constexpr(std::is_empty_v<object_type>) {
            ENTT_ASSERT(underlying_type::has(entt));
            return instances;
        } else {
            return instances[underlying_type::get(entt)];
        }
    }

    /*! @copydoc get */
    inline object_type & get(const entity_type entt) ENTT_NOEXCEPT {
        return const_cast<object_type &>(std::as_const(*this).get(entt));
    }

    /**
     * @brief Returns a pointer to the object associated with an entity, if any.
     * @param entt A valid entity identifier.
     * @return The object associated with the entity, if any.
     */
    const object_type * try_get(const entity_type entt) const ENTT_NOEXCEPT {
        if constexpr(std::is_empty_v<object_type>) {
            return underlying_type::has(entt) ? &instances : nullptr;
        } else {
            return underlying_type::has(entt) ? (instances.data() + underlying_type::get(entt)) : nullptr;
        }
    }

    /*! @copydoc try_get */
    inline object_type * try_get(const entity_type entt) ENTT_NOEXCEPT {
        return const_cast<object_type *>(std::as_const(*this).try_get(entt));
    }

    /**
     * @brief Assigns an entity to a sparse set and constructs its object.
     *
     * This version accept both types that can be constructed in place directly
     * and types like aggregates that do not work well with a placement new as
     * performed usually under the hood during an _emplace back_.
     *
     * @warning
     * Attempting to use an entity that already belongs to the sparse set
     * results in undefined behavior.<br/>
     * An assertion will abort the execution at runtime in debug mode if the
     * sparse set already contains the given entity.
     *
     * @tparam Args Types of arguments to use to construct the object.
     * @param entt A valid entity identifier.
     * @param args Parameters to use to construct an object for the entity.
     * @return The object associated with the entity.
     */
    template<typename... Args>
    object_type & construct(const entity_type entt, [[maybe_unused]] Args &&... args) {
        if constexpr(std::is_empty_v<object_type>) {
            underlying_type::construct(entt);
            return instances;
        } else {
            if constexpr(std::is_aggregate_v<object_type>) {
                instances.emplace_back(Type{std::forward<Args>(args)...});
            } else {
                instances.emplace_back(std::forward<Args>(args)...);
            }

            // entity goes after component in case constructor throws
            underlying_type::construct(entt);
            return instances.back();
        }
    }

    /**
     * @brief Assigns one or more entities to a sparse set and constructs their
     * objects.
     *
     * The object type must be at least default constructible.
     *
     * @note
     * Empty components aren't explicitly instantiated. Only one instance of the
     * given type is created. Therefore, this function always returns a pointer
     * to that instance.
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
     * @return A pointer to the array of instances just created and sorted the
     * same of the entities.
     */
    template<typename It>
    object_type * batch(It first, It last) {
        if constexpr(std::is_empty_v<object_type>) {
            underlying_type::batch(first, last);
            return &instances;
        } else {
            static_assert(std::is_default_constructible_v<object_type>);
            const auto skip = instances.size();
            instances.insert(instances.end(), last-first, {});
            // entity goes after component in case constructor throws
            underlying_type::batch(first, last);
            return instances.data() + skip;
        }
    }

    /**
     * @brief Removes an entity from a sparse set and destroies its object.
     *
     * @warning
     * Attempting to use an entity that doesn't belong to the sparse set results
     * in undefined behavior.<br/>
     * An assertion will abort the execution at runtime in debug mode if the
     * sparse set doesn't contain the given entity.
     *
     * @param entt A valid entity identifier.
     */
    void destroy(const entity_type entt) override {
        if constexpr(!std::is_empty_v<object_type>) {
            std::swap(instances[underlying_type::get(entt)], instances.back());
            instances.pop_back();
        }

        underlying_type::destroy(entt);
    }

    /**
     * @brief Sort components according to the given comparison function.
     *
     * Sort the elements so that iterating the sparse set with a couple of
     * iterators returns them in the expected order. See `begin` and `end` for
     * more details.
     *
     * The comparison function object must return `true` if the first element
     * is _less_ than the second one, `false` otherwise. The signature of the
     * comparison function should be equivalent to the following:
     *
     * @code{.cpp}
     * bool(const Type &, const Type &);
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
     * The comparison funtion object received by the sort function object hasn't
     * necessarily the type of the one passed along with the other parameters to
     * this member function.
     *
     * @note
     * Empty components aren't explicitly instantiated. Therefore, this function
     * isn't available for them.
     *
     * @note
     * Attempting to iterate elements using a raw pointer returned by a call to
     * either `data` or `raw` gives no guarantees on the order, even though
     * `sort` has been invoked.
     *
     * @tparam Compare Type of comparison function object.
     * @tparam Sort Type of sort function object.
     * @tparam Args Types of arguments to forward to the sort function object.
     * @param compare A valid comparison function object.
     * @param sort A valid sort function object.
     * @param args Arguments to forward to the sort function object, if any.
     */
    template<typename Compare, typename Sort = std_sort, typename... Args>
    void sort(Compare compare, Sort sort = Sort{}, Args &&... args) {
        static_assert(!std::is_empty_v<object_type>);

        std::vector<size_type> copy(instances.size());
        std::iota(copy.begin(), copy.end(), 0);

        sort(copy.begin(), copy.end(), [this, compare = std::move(compare)](const auto lhs, const auto rhs) {
            return compare(std::as_const(instances[rhs]), std::as_const(instances[lhs]));
        }, std::forward<Args>(args)...);

        for(size_type pos = 0, last = copy.size(); pos < last; ++pos) {
            auto curr = pos;
            auto next = copy[curr];

            while(curr != next) {
                const auto lhs = copy[curr];
                const auto rhs = copy[next];
                std::swap(instances[lhs], instances[rhs]);
                underlying_type::swap(lhs, rhs);
                copy[curr] = curr;
                curr = next;
                next = copy[curr];
            }
        }
    }

    /**
     * @brief Sort components according to the order of the entities in another
     * sparse set.
     *
     * Entities that are part of both the sparse sets are ordered internally
     * according to the order they have in `other`. All the other entities goes
     * to the end of the list and there are no guarantess on their order.
     * Components are sorted according to the entities to which they
     * belong.<br/>
     * In other terms, this function can be used to impose the same order on two
     * sets by using one of them as a master and the other one as a slave.
     *
     * Iterating the sparse set with a couple of iterators returns elements in
     * the expected order after a call to `respect`. See `begin` and `end` for
     * more details.
     *
     * @note
     * Attempting to iterate elements using a raw pointer returned by a call to
     * either `data` or `raw` gives no guarantees on the order, even though
     * `respect` has been invoked.
     *
     * @param other The sparse sets that imposes the order of the entities.
     */
    void respect(const sparse_set<Entity> &other) ENTT_NOEXCEPT override {
        if constexpr(std::is_empty_v<object_type>) {
            underlying_type::respect(other);
        } else {
            const auto to = other.end();
            auto from = other.begin();

            size_type pos = underlying_type::size() - 1;
            const auto *local = underlying_type::data();

            while(pos && from != to) {
                const auto curr = *from;

                if(underlying_type::has(curr)) {
                    if(curr != *(local + pos)) {
                        auto candidate = underlying_type::get(curr);
                        std::swap(instances[pos], instances[candidate]);
                        underlying_type::swap(pos, candidate);
                    }

                    --pos;
                }

                ++from;
            }
        }
    }

    /*! @brief Resets a sparse set. */
    void reset() override {
        underlying_type::reset();

        if constexpr(!std::is_empty_v<object_type>) {
            instances.clear();
        }
    }

    /**
     * @brief Clones and returns a sparse set if possible.
     *
     * The extended implementation of a sparse set is copyable only if its
     * object type is copyable. Because of that, this member functions isn't
     * guaranteed to return always a valid pointer.
     *
     * @return A fresh copy of the given sparse set if its object type is
     * copyable, an empty unique pointer otherwise.
     */
    std::unique_ptr<sparse_set<Entity>> clone() const override {
        if constexpr(std::is_copy_constructible_v<object_type>) {
            return std::make_unique<sparse_set>(*this);
        } else {
            return nullptr;
        }
    }

private:
    std::conditional_t<std::is_empty_v<object_type>, object_type, std::vector<object_type>> instances;
};


}


#endif // ENTT_ENTITY_SPARSE_SET_HPP

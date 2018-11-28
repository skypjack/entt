#ifndef ENTT_ENTITY_SPARSE_SET_HPP
#define ENTT_ENTITY_SPARSE_SET_HPP


#include <algorithm>
#include <iterator>
#include <numeric>
#include <utility>
#include <vector>
#include <cstddef>
#include <cassert>
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

    class iterator final {
        friend class sparse_set<Entity>;

        using direct_type = const std::vector<Entity>;
        using index_type = typename traits_type::difference_type;

        iterator(direct_type *direct, index_type index) ENTT_NOEXCEPT
            : direct{direct}, index{index}
        {}

    public:
        using difference_type = index_type;
        using value_type = const Entity;
        using pointer = value_type *;
        using reference = value_type &;
        using iterator_category = std::random_access_iterator_tag;

        iterator() ENTT_NOEXCEPT = default;

        iterator(const iterator &) ENTT_NOEXCEPT = default;
        iterator & operator=(const iterator &) ENTT_NOEXCEPT = default;

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

public:
    /*! @brief Underlying entity identifier. */
    using entity_type = Entity;
    /*! @brief Unsigned integer type. */
    using size_type = std::size_t;
    /*! @brief Input iterator type. */
    using iterator_type = iterator;

    /*! @brief Default constructor. */
    sparse_set() ENTT_NOEXCEPT = default;

    /*! @brief Default destructor. */
    virtual ~sparse_set() ENTT_NOEXCEPT = default;

    /*! @brief Copying a sparse set isn't allowed. */
    sparse_set(const sparse_set &) = delete;
    /*! @brief Default move constructor. */
    sparse_set(sparse_set &&) = default;

    /*! @brief Copying a sparse set isn't allowed. @return This sparse set. */
    sparse_set & operator=(const sparse_set &) = delete;
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
        return reverse.size();
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
     * @param entity A valid entity identifier.
     * @return An iterator to the given entity if it's found, past the end
     * iterator otherwise.
     */
    iterator_type find(const entity_type entity) const ENTT_NOEXCEPT {
        return has(entity) ? --(end() - get(entity)) : end();
    }

    /**
     * @brief Checks if a sparse set contains an entity.
     * @param entity A valid entity identifier.
     * @return True if the sparse set contains the entity, false otherwise.
     */
    bool has(const entity_type entity) const ENTT_NOEXCEPT {
        const auto pos = size_type(entity & traits_type::entity_mask);
        // testing against null permits to avoid accessing the direct vector
        return (pos < reverse.size()) && (reverse[pos] != null);
    }

    /**
     * @brief Checks if a sparse set contains an entity (unsafe).
     *
     * Alternative version of `has`. It accesses the underlying data structures
     * without bounds checking and thus it's both unsafe and risky to use.<br/>
     * You should not invoke directly this function unless you know exactly what
     * you are doing. Prefer the `has` member function instead.
     *
     * @warning
     * Attempting to use an entity that doesn't belong to the sparse set can
     * result in undefined behavior.<br/>
     * An assertion will abort the execution at runtime in debug mode in case of
     * bounds violation.
     *
     * @param entity A valid entity identifier.
     * @return True if the sparse set contains the entity, false otherwise.
     */
    bool fast(const entity_type entity) const ENTT_NOEXCEPT {
        const auto pos = size_type(entity & traits_type::entity_mask);
        assert(pos < reverse.size());
        // testing against null permits to avoid accessing the direct vector
        return (reverse[pos] != null);
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
     * @param entity A valid entity identifier.
     * @return The position of the entity in the sparse set.
     */
    size_type get(const entity_type entity) const ENTT_NOEXCEPT {
        assert(has(entity));
        const auto pos = size_type(entity & traits_type::entity_mask);
        return size_type(reverse[pos]);
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
     * @param entity A valid entity identifier.
     */
    void construct(const entity_type entity) {
        assert(!has(entity));
        const auto pos = size_type(entity & traits_type::entity_mask);

        if(!(pos < reverse.size())) {
            // null is safe in all cases for our purposes
            reverse.resize(pos+1, null);
        }

        reverse[pos] = entity_type(direct.size());
        direct.push_back(entity);
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
     * @param entity A valid entity identifier.
     */
    virtual void destroy(const entity_type entity) {
        assert(has(entity));
        const auto back = direct.back();
        auto &candidate = reverse[size_type(entity & traits_type::entity_mask)];
        // swapping isn't required here, we are getting rid of the last element
        reverse[back & traits_type::entity_mask] = candidate;
        direct[size_type(candidate)] = back;
        candidate = null;
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
        assert(lhs < direct.size());
        assert(rhs < direct.size());
        auto &src = direct[lhs];
        auto &dst = direct[rhs];
        std::swap(reverse[src & traits_type::entity_mask], reverse[dst & traits_type::entity_mask]);
        std::swap(src, dst);
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
    void respect(const sparse_set &other) ENTT_NOEXCEPT {
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

private:
    std::vector<entity_type> reverse;
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

    template<bool Const>
    class iterator final {
        friend class sparse_set<Entity, Type>;

        using instance_type = std::conditional_t<Const, const std::vector<Type>, std::vector<Type>>;
        using index_type = typename traits_type::difference_type;

        iterator(instance_type *instances, index_type index) ENTT_NOEXCEPT
            : instances{instances}, index{index}
        {}

    public:
        using difference_type = index_type;
        using value_type = std::conditional_t<Const, const Type, Type>;
        using pointer = value_type *;
        using reference = value_type &;
        using iterator_category = std::random_access_iterator_tag;

        iterator() ENTT_NOEXCEPT = default;

        iterator(const iterator &) ENTT_NOEXCEPT = default;
        iterator & operator=(const iterator &) ENTT_NOEXCEPT = default;

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

    /*! @brief Default constructor. */
    sparse_set() ENTT_NOEXCEPT = default;

    /*! @brief Copying a sparse set isn't allowed. */
    sparse_set(const sparse_set &) = delete;
    /*! @brief Default move constructor. */
    sparse_set(sparse_set &&) = default;

    /*! @brief Copying a sparse set isn't allowed. @return This sparse set. */
    sparse_set & operator=(const sparse_set &) = delete;
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
        underlying_type::reserve(cap);
        instances.reserve(cap);
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
     * @return A pointer to the array of objects.
     */
    const object_type * raw() const ENTT_NOEXCEPT {
        return instances.data();
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
     * @return A pointer to the array of objects.
     */
    object_type * raw() ENTT_NOEXCEPT {
        return instances.data();
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
        const typename traits_type::difference_type pos = instances.size();
        return const_iterator_type{&instances, pos};
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
    inline const_iterator_type begin() const ENTT_NOEXCEPT {
        return cbegin();
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
    iterator_type begin() ENTT_NOEXCEPT {
        const typename traits_type::difference_type pos = instances.size();
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
    inline const_iterator_type end() const ENTT_NOEXCEPT {
        return cend();
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
     * @param entity A valid entity identifier.
     * @return The object associated with the entity.
     */
    const object_type & get(const entity_type entity) const ENTT_NOEXCEPT {
        return instances[underlying_type::get(entity)];
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
     * @param entity A valid entity identifier.
     * @return The object associated with the entity.
     */
    inline object_type & get(const entity_type entity) ENTT_NOEXCEPT {
        return const_cast<object_type &>(std::as_const(*this).get(entity));
    }

    /**
     * @brief Returns a pointer to the object associated with an entity, if any.
     * @param entity A valid entity identifier.
     * @return The object associated with the entity, if any.
     */
    const object_type * try_get(const entity_type entity) const ENTT_NOEXCEPT {
        return underlying_type::has(entity) ? (instances.data() + underlying_type::get(entity)) : nullptr;
    }

    /**
     * @brief Returns a pointer to the object associated with an entity, if any.
     * @param entity A valid entity identifier.
     * @return The object associated with the entity, if any.
     */
    inline object_type * try_get(const entity_type entity) ENTT_NOEXCEPT {
        return const_cast<object_type *>(std::as_const(*this).try_get(entity));
    }

    /**
     * @brief Assigns an entity to a sparse set and constructs its object.
     *
     * @note
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
     * @param entity A valid entity identifier.
     * @param args Parameters to use to construct an object for the entity.
     * @return The object associated with the entity.
     */
    template<typename... Args>
    object_type & construct(const entity_type entity, Args &&... args) {
        underlying_type::construct(entity);

        if constexpr(std::is_aggregate_v<Type>) {
            instances.emplace_back(Type{std::forward<Args>(args)...});
        } else {
            instances.emplace_back(std::forward<Args>(args)...);
        }

        return instances.back();
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
     * @param entity A valid entity identifier.
     */
    void destroy(const entity_type entity) override {
        // swapping isn't required here, we are getting rid of the last element
        // however, we must protect ourselves from self assignments (see #37)
        auto tmp = std::move(instances.back());
        instances[underlying_type::get(entity)] = std::move(tmp);
        instances.pop_back();
        underlying_type::destroy(entity);
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
    void respect(const sparse_set<Entity> &other) ENTT_NOEXCEPT {
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

    /**
     * @brief Resets a sparse set.
     */
    void reset() override {
        underlying_type::reset();
        instances.clear();
    }

private:
    std::vector<object_type> instances;
};


}


#endif // ENTT_ENTITY_SPARSE_SET_HPP

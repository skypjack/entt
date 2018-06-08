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


namespace entt {


/**
 * @brief Sparse set.
 *
 * Primary template isn't defined on purpose. All the specializations give a
 * compile-time error, but for a few reasonable cases.
 */
template<typename...>
class SparseSet;


/**
 * @brief Basic sparse set implementation.
 *
 * Sparse set or packed array or whatever is the name users give it.<br/>
 * Two arrays: an _external_ one and an _internal_ one; a _sparse_ one and a
 * _packed_ one; one used for direct access through contiguous memory, the other
 * one used to get the data through an extra level of indirection.<br/>
 * This is largely used by the Registry to offer users the fastest access ever
 * to the components. View and PersistentView are entirely designed around
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
class SparseSet<Entity> {
    using traits_type = entt_traits<Entity>;

    struct Iterator final {
        using difference_type = std::size_t;
        using value_type = Entity;
        using pointer = const value_type *;
        using reference = value_type;
        using iterator_category = std::forward_iterator_tag;

        Iterator(pointer direct, std::size_t pos)
            : direct{direct}, pos{pos}
        {}
		Iterator()
			: direct{ nullptr }, pos{ 0 }
		{}
        Iterator & operator++() ENTT_NOEXCEPT {
            return --pos, *this;
        }

        Iterator operator++(int) ENTT_NOEXCEPT {
            Iterator orig = *this;
            return ++(*this), orig;
        }

        Iterator & operator+=(const difference_type value) ENTT_NOEXCEPT {
            pos -= value;
            return *this;
        }

        Iterator operator+(const difference_type value) const ENTT_NOEXCEPT {
            return Iterator{direct, pos-value};
        }

        bool operator==(const Iterator &other) const ENTT_NOEXCEPT {
            return other.pos == pos;
        }

        inline bool operator!=(const Iterator &other) const ENTT_NOEXCEPT {
            return !(*this == other);
        }

        reference operator*() const ENTT_NOEXCEPT {
            return direct[pos-1];
        }

    private:
        pointer direct;
        std::size_t pos;
    };

    static constexpr auto pending = ~typename traits_type::entity_type{};

public:
    /*! @brief Underlying entity identifier. */
    using entity_type = Entity;
    /*! @brief Entity dependent position type. */
    using pos_type = entity_type;
    /*! @brief Unsigned integer type. */
    using size_type = std::size_t;
    /*! @brief Input iterator type. */
    using iterator_type = Iterator;
    /*! @brief Constant input iterator type. */
    using const_iterator_type = Iterator;

    /*! @brief Default constructor. */
    SparseSet() ENTT_NOEXCEPT = default;

    /*! @brief Default destructor. */
    virtual ~SparseSet() ENTT_NOEXCEPT = default;

    /*! @brief Copying a sparse set isn't allowed. */
    SparseSet(const SparseSet &) = delete;
    /*! @brief Default move constructor. */
    SparseSet(SparseSet &&) = default;

    /*! @brief Copying a sparse set isn't allowed. @return This sparse set. */
    SparseSet & operator=(const SparseSet &) = delete;
    /*! @brief Default move assignment operator. @return This sparse set. */
    SparseSet & operator=(SparseSet &&) = default;

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
    const_iterator_type cbegin() const ENTT_NOEXCEPT {
        return const_iterator_type{direct.data(), direct.size()};
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
    inline const_iterator_type begin() const ENTT_NOEXCEPT {
        return cbegin();
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
    inline iterator_type begin() ENTT_NOEXCEPT {
        return cbegin();
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
    const_iterator_type cend() const ENTT_NOEXCEPT {
        return const_iterator_type{direct.data(), {}};
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
    inline const_iterator_type end() const ENTT_NOEXCEPT {
        return cend();
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
    inline iterator_type end() ENTT_NOEXCEPT {
        return cend();
    }

    /**
     * @brief Checks if a sparse set contains an entity.
     * @param entity A valid entity identifier.
     * @return True if the sparse set contains the entity, false otherwise.
     */
    bool has(const entity_type entity) const ENTT_NOEXCEPT {
        const auto pos = size_type(entity & traits_type::entity_mask);
        // testing against pending permits to avoid accessing the direct vector
        return (pos < reverse.size()) && (reverse[pos] != pending);
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
        // testing against pending permits to avoid accessing the direct vector
        return (reverse[pos] != pending);
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
    pos_type get(const entity_type entity) const ENTT_NOEXCEPT {
        assert(has(entity));
        return reverse[entity & traits_type::entity_mask];
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
            const auto value = pending;
            reverse.resize(pos+1, value);
        }

        reverse[pos] = pos_type(direct.size());
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
        auto &candidate = reverse[entity & traits_type::entity_mask];
        // swapping isn't required here, we are getting rid of the last element
        reverse[back & traits_type::entity_mask] = candidate;
        direct[candidate] = back;
        candidate = pending;
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
    void swap(const pos_type lhs, const pos_type rhs) ENTT_NOEXCEPT {
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
    void respect(const SparseSet<Entity> &other) ENTT_NOEXCEPT {
        auto from = other.cbegin();
        auto to = other.cend();

        pos_type pos = direct.size() - 1;

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
    std::vector<pos_type> reverse;
    std::vector<entity_type> direct;
};


/**
 * @brief Extended sparse set implementation.
 *
 * This specialization of a sparse set associates an object to an entity. The
 * main purpose of this class is to use sparse sets to store components in a
 * Registry. It guarantees fast access both to the elements and to the entities.
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
 * @sa SparseSet<Entity>
 *
 * @tparam Entity A valid entity type (see entt_traits for more details).
 * @tparam Type Type of objects assigned to the entities.
 */
template<typename Entity, typename Type>
class SparseSet<Entity, Type>: public SparseSet<Entity> {
    using underlying_type = SparseSet<Entity>;

    template<bool Const>
    struct Iterator final {
        using difference_type = std::size_t;
        using value_type = std::conditional_t<Const, const Type, Type>;
        using pointer = value_type *;
        using reference = value_type &;
        using iterator_category = std::forward_iterator_tag;

        Iterator(pointer instances, std::size_t pos)
            : instances{instances}, pos{pos}
        {}
		Iterator()
			: instances{ nullptr }, pos{ 0 }
		{}

        Iterator & operator++() ENTT_NOEXCEPT {
            return --pos, *this;
        }

        Iterator operator++(int) ENTT_NOEXCEPT {
            Iterator orig = *this;
            return ++(*this), orig;
        }

        Iterator & operator+=(const difference_type value) ENTT_NOEXCEPT {
            pos -= value;
            return *this;
        }

        Iterator operator+(const difference_type value) const ENTT_NOEXCEPT {
            return Iterator{instances, pos-value};
        }

        bool operator==(const Iterator &other) const ENTT_NOEXCEPT {
            return other.pos == pos;
        }

        inline bool operator!=(const Iterator &other) const ENTT_NOEXCEPT {
            return !(*this == other);
        }

        reference operator*() const ENTT_NOEXCEPT {
            return instances[pos-1];
        }

        pointer operator->() const ENTT_NOEXCEPT {
            return (instances+pos-1);
        }

    private:
        pointer instances;
        std::size_t pos;
    };

public:
    /*! @brief Type of the objects associated to the entities. */
    using object_type = Type;
    /*! @brief Underlying entity identifier. */
    using entity_type = typename underlying_type::entity_type;
    /*! @brief Entity dependent position type. */
    using pos_type = typename underlying_type::pos_type;
    /*! @brief Unsigned integer type. */
    using size_type = typename underlying_type::size_type;
    /*! @brief Input iterator type. */
    using iterator_type = Iterator<false>;
    /*! @brief Constant input iterator type. */
    using const_iterator_type = Iterator<true>;

    /*! @brief Default constructor. */
    SparseSet() ENTT_NOEXCEPT = default;

    /*! @brief Copying a sparse set isn't allowed. */
    SparseSet(const SparseSet &) = delete;
    /*! @brief Default move constructor. */
    SparseSet(SparseSet &&) = default;

    /*! @brief Copying a sparse set isn't allowed. @return This sparse set. */
    SparseSet & operator=(const SparseSet &) = delete;
    /*! @brief Default move assignment operator. @return This sparse set. */
    SparseSet & operator=(SparseSet &&) = default;

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
        return const_iterator_type{instances.data(), instances.size()};
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
        return iterator_type{instances.data(), instances.size()};
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
        return const_iterator_type{instances.data(), {}};
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
        return iterator_type{instances.data(), {}};
    }

    /**
     * @brief Returns the object associated to an entity.
     *
     * @warning
     * Attempting to use an entity that doesn't belong to the sparse set results
     * in undefined behavior.<br/>
     * An assertion will abort the execution at runtime in debug mode if the
     * sparse set doesn't contain the given entity.
     *
     * @param entity A valid entity identifier.
     * @return The object associated to the entity.
     */
    const object_type & get(const entity_type entity) const ENTT_NOEXCEPT {
        return instances[underlying_type::get(entity)];
    }

    /**
     * @brief Returns the object associated to an entity.
     *
     * @warning
     * Attempting to use an entity that doesn't belong to the sparse set results
     * in undefined behavior.<br/>
     * An assertion will abort the execution at runtime in debug mode if the
     * sparse set doesn't contain the given entity.
     *
     * @param entity A valid entity identifier.
     * @return The object associated to the entity.
     */
    inline object_type & get(const entity_type entity) ENTT_NOEXCEPT {
        return const_cast<object_type &>(const_cast<const SparseSet *>(this)->get(entity));
    }

    /**
     * @brief Assigns an entity to a sparse set and constructs its object.
     *
     * @note
     * _Sfinae'd_ function.<br/>
     * This version is used for types that can be constructed in place directly.
     * It doesn't work well with aggregates because of the placement new usually
     * performed under the hood during an _emplace back_.
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
     * @return The object associated to the entity.
     */
    template<typename... Args>
    std::enable_if_t<std::is_constructible<Type, Args...>::value, object_type &>
    construct(const entity_type entity, Args &&... args) {
        underlying_type::construct(entity);
        instances.emplace_back(std::forward<Args>(args)...);
        return instances.back();
    }

    /**
     * @brief Assigns an entity to a sparse set and constructs its object.
     *
     * @note
     * _Sfinae'd_ function.<br/>
     * Fallback for aggregates and types in general that do not work well with a
     * placement new as performed usually under the hood during an
     * _emplace back_.
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
     * @return The object associated to the entity.
     */
    template<typename... Args>
    std::enable_if_t<!std::is_constructible<Type, Args...>::value, object_type &>
    construct(const entity_type entity, Args &&... args) {
        underlying_type::construct(entity);
        instances.emplace_back(Type{std::forward<Args>(args)...});
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
     * bool(const Type &, const Type &)
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
     * @param compare A valid comparison function object.
     * @param sort A valid sort function object.
     */
    template<typename Compare, typename Sort = StdSort>
    void sort(Compare compare, Sort sort = Sort{}) {
        std::vector<pos_type> copy(instances.size());
        std::iota(copy.begin(), copy.end(), 0);

        sort(copy.begin(), copy.end(), [this, compare = std::move(compare)](const auto lhs, const auto rhs) {
            return compare(const_cast<const object_type &>(instances[rhs]), const_cast<const object_type &>(instances[lhs]));
        });

        for(pos_type pos = 0, last = copy.size(); pos < last; ++pos) {
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
    void respect(const SparseSet<Entity> &other) ENTT_NOEXCEPT {
        auto from = other.cbegin();
        auto to = other.cend();

        pos_type pos = underlying_type::size() - 1;
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

#ifndef ENTT_ENTITY_STORAGE_HPP
#define ENTT_ENTITY_STORAGE_HPP


#include <algorithm>
#include <iterator>
#include <utility>
#include <vector>
#include <cstddef>
#include <type_traits>
#include "../config/config.h"
#include "../core/algorithm.hpp"
#include "sparse_set.hpp"
#include "entity.hpp"


namespace entt {


/**
 * @brief Basic storage implementation.
 *
 * This class is a refinement of a sparse set that associates an object to an
 * entity. The main purpose of this class is to extend sparse sets to store
 * components in a registry. It guarantees fast access both to the elements and
 * to the entities.
 *
 * @note
 * Entities and objects have the same order. It's guaranteed both in case of raw
 * access (either to entities or objects) and when using random or input access
 * iterators.
 *
 * @note
 * Internal data structures arrange elements to maximize performance. Because of
 * that, there are no guarantees that elements have the expected order when
 * iterate directly the internal packed array (see `raw` and `size` member
 * functions for that). Use `begin` and `end` instead.
 *
 * @warning
 * Empty types aren't explicitly instantiated. Temporary objects are returned in
 * place of the instances of the components and raw access isn't available for
 * them.
 *
 * @sa sparse_set<Entity>
 *
 * @tparam Entity A valid entity type (see entt_traits for more details).
 * @tparam Type Type of objects assigned to the entities.
 */
template<typename Entity, typename Type, typename = std::void_t<>>
class storage: public sparse_set<Entity> {
    using underlying_type = sparse_set<Entity>;
    using traits_type = entt_traits<std::underlying_type_t<Entity>>;

    template<bool Const>
    class iterator {
        friend class storage<Entity, Type>;

        using instance_type = std::conditional_t<Const, const std::vector<Type>, std::vector<Type>>;
        using index_type = typename traits_type::difference_type;

        iterator(instance_type *ref, const index_type idx) ENTT_NOEXCEPT
            : instances{ref}, index{idx}
        {}

    public:
        using difference_type = index_type;
        using value_type = Type;
        using pointer = std::conditional_t<Const, const value_type *, value_type *>;
        using reference = std::conditional_t<Const, const value_type &, value_type &>;
        using iterator_category = std::random_access_iterator_tag;

        iterator() ENTT_NOEXCEPT = default;

        iterator & operator++() ENTT_NOEXCEPT {
            return --index, *this;
        }

        iterator operator++(int) ENTT_NOEXCEPT {
            iterator orig = *this;
            return operator++(), orig;
        }

        iterator & operator--() ENTT_NOEXCEPT {
            return ++index, *this;
        }

        iterator operator--(int) ENTT_NOEXCEPT {
            iterator orig = *this;
            return operator--(), orig;
        }

        iterator & operator+=(const difference_type value) ENTT_NOEXCEPT {
            index -= value;
            return *this;
        }

        iterator operator+(const difference_type value) const ENTT_NOEXCEPT {
            return iterator{instances, index-value};
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
            return (*instances)[pos];
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
            return &(*instances)[pos];
        }

        reference operator*() const ENTT_NOEXCEPT {
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
    using entity_type = Entity;
    /*! @brief Unsigned integer type. */
    using size_type = std::size_t;
    /*! @brief Random access iterator type. */
    using iterator_type = iterator<false>;
    /*! @brief Constant random access iterator type. */
    using const_iterator_type = iterator<true>;

    /**
     * @brief Increases the capacity of a storage.
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

    /*! @brief Requests the removal of unused capacity. */
    void shrink_to_fit() {
        underlying_type::shrink_to_fit();
        instances.shrink_to_fit();
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
     * to iterate the storage in the expected order.
     *
     * @return A pointer to the array of objects.
     */
    const object_type * raw() const ENTT_NOEXCEPT {
        return instances.data();
    }

    /*! @copydoc raw */
    object_type * raw() ENTT_NOEXCEPT {
        return const_cast<object_type *>(std::as_const(*this).raw());
    }

    /**
     * @brief Returns an iterator to the beginning.
     *
     * The returned iterator points to the first instance of the given type. If
     * the storage is empty, the returned iterator will be equal to `end()`.
     *
     * @note
     * Random access iterators stay true to the order imposed by a call to
     * either `sort` or `respect`.
     *
     * @return An iterator to the first instance of the given type.
     */
    const_iterator_type cbegin() const ENTT_NOEXCEPT {
        const typename traits_type::difference_type pos = underlying_type::size();
        return const_iterator_type{&instances, pos};
    }

    /*! @copydoc cbegin */
    const_iterator_type begin() const ENTT_NOEXCEPT {
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
     * Random access iterators stay true to the order imposed by a call to
     * either `sort` or `respect`.
     *
     * @return An iterator to the element following the last instance of the
     * given type.
     */
    const_iterator_type cend() const ENTT_NOEXCEPT {
        return const_iterator_type{&instances, {}};
    }

    /*! @copydoc cend */
    const_iterator_type end() const ENTT_NOEXCEPT {
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
     * Attempting to use an entity that doesn't belong to the storage results in
     * undefined behavior.<br/>
     * An assertion will abort the execution at runtime in debug mode if the
     * storage doesn't contain the given entity.
     *
     * @param entt A valid entity identifier.
     * @return The object associated with the entity.
     */
    const object_type & get(const entity_type entt) const {
        return instances[underlying_type::index(entt)];
    }

    /*! @copydoc get */
    object_type & get(const entity_type entt) {
        return const_cast<object_type &>(std::as_const(*this).get(entt));
    }

    /**
     * @brief Returns a pointer to the object associated with an entity, if any.
     * @param entt A valid entity identifier.
     * @return The object associated with the entity, if any.
     */
    const object_type * try_get(const entity_type entt) const {
        return underlying_type::has(entt) ? (instances.data() + underlying_type::index(entt)) : nullptr;
    }

    /*! @copydoc try_get */
    object_type * try_get(const entity_type entt) {
        return const_cast<object_type *>(std::as_const(*this).try_get(entt));
    }

    /**
     * @brief Assigns an entity to a storage and constructs its object.
     *
     * This version accept both types that can be constructed in place directly
     * and types like aggregates that do not work well with a placement new as
     * performed usually under the hood during an _emplace back_.
     *
     * @warning
     * Attempting to use an entity that already belongs to the storage results
     * in undefined behavior.<br/>
     * An assertion will abort the execution at runtime in debug mode if the
     * storage already contains the given entity.
     *
     * @tparam Args Types of arguments to use to construct the object.
     * @param entt A valid entity identifier.
     * @param args Parameters to use to construct an object for the entity.
     */
    template<typename... Args>
    void construct(const entity_type entt, Args &&... args) {
        if constexpr(std::is_aggregate_v<object_type>) {
            instances.push_back(Type{std::forward<Args>(args)...});
        } else {
            instances.emplace_back(std::forward<Args>(args)...);
        }

        // entity goes after component in case constructor throws
        underlying_type::construct(entt);
    }

    /**
     * @brief Assigns one or more entities to a storage and constructs their
     * objects from a given instance.
     *
     * @warning
     * Attempting to assign an entity that already belongs to the storage
     * results in undefined behavior.<br/>
     * An assertion will abort the execution at runtime in debug mode if the
     * storage already contains the given entity.
     *
     * @tparam It Type of input iterator.
     * @param first An iterator to the first element of the range of entities.
     * @param last An iterator past the last element of the range of entities.
     * @param value An instance of the object to construct.
     */
    template<typename It>
    std::enable_if_t<std::is_same_v<typename std::iterator_traits<It>::value_type, entity_type>, void>
    construct(It first, It last, const object_type &value = {}) {
        instances.insert(instances.end(), std::distance(first, last), value);
        // entities go after components in case constructors throw
        underlying_type::construct(first, last);
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
     * @param value An iterator to the first element of the range of objects.
     */
    template<typename EIt, typename CIt>
    std::enable_if_t<std::is_same_v<typename std::iterator_traits<EIt>::value_type, entity_type>, void>
    construct(EIt first, EIt last, CIt value) {
        instances.insert(instances.end(), value, value + std::distance(first, last));
        // entities go after components in case constructors throw
        underlying_type::construct(first, last);
    }

    /**
     * @brief Removes an entity from a storage and destroys its object.
     *
     * @warning
     * Attempting to use an entity that doesn't belong to the storage results in
     * undefined behavior.<br/>
     * An assertion will abort the execution at runtime in debug mode if the
     * storage doesn't contain the given entity.
     *
     * @param entt A valid entity identifier.
     */
    void destroy(const entity_type entt) {
        auto other = std::move(instances.back());
        instances[underlying_type::index(entt)] = std::move(other);
        instances.pop_back();
        underlying_type::destroy(entt);
    }

    /**
     * @brief Swaps entities and objects in the internal packed arrays.
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
    void swap(const entity_type lhs, const entity_type rhs) override {
        std::swap(instances[underlying_type::index(lhs)], instances[underlying_type::index(rhs)]);
        underlying_type::swap(lhs, rhs);
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
     * comparison function should be equivalent to one of the following:
     *
     * @code{.cpp}
     * bool(const Entity, const Entity);
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
     * @note
     * Attempting to iterate elements using a raw pointer returned by a call to
     * either `data` or `raw` gives no guarantees on the order, even though
     * `sort` has been invoked.
     *
     * @warning
     * Empty types are never instantiated. Therefore, only comparison function
     * objects that require to return entities rather than components are
     * accepted.
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

        const auto from = underlying_type::begin() + std::distance(begin(), first);
        const auto to = from + std::distance(first, last);

        const auto apply = [this](const auto lhs, const auto rhs) {
            std::swap(instances[underlying_type::index(lhs)], instances[underlying_type::index(rhs)]);
        };

        if constexpr(std::is_invocable_v<Compare, const object_type &, const object_type &>) {
            underlying_type::arrange(from, to, std::move(apply), [this, compare = std::move(compare)](const auto lhs, const auto rhs) {
                return compare(std::as_const(instances[underlying_type::index(lhs)]), std::as_const(instances[underlying_type::index(rhs)]));
            }, std::move(algo), std::forward<Args>(args)...);
        } else {
            underlying_type::arrange(from, to, std::move(apply), std::move(compare), std::move(algo), std::forward<Args>(args)...);
        }
    }

    /*! @brief Clears a storage. */
    void clear() {
        underlying_type::clear();
        instances.clear();
    }

private:
    std::vector<object_type> instances;
};


/*! @copydoc storage */
template<typename Entity, typename Type>
class storage<Entity, Type, std::enable_if_t<ENTT_ENABLE_ETO(Type)>>: public sparse_set<Entity> {
    using traits_type = entt_traits<std::underlying_type_t<Entity>>;
    using underlying_type = sparse_set<Entity>;

    class iterator {
        friend class storage<Entity, Type>;

        using index_type = typename traits_type::difference_type;

        iterator(const index_type idx) ENTT_NOEXCEPT
            : index{idx}
        {}

    public:
        using difference_type = index_type;
        using value_type = Type;
        using pointer = const value_type *;
        using reference = value_type;
        using iterator_category = std::input_iterator_tag;

        iterator() ENTT_NOEXCEPT = default;

        iterator & operator++() ENTT_NOEXCEPT {
            return --index, *this;
        }

        iterator operator++(int) ENTT_NOEXCEPT {
            iterator orig = *this;
            return operator++(), orig;
        }

        iterator & operator--() ENTT_NOEXCEPT {
            return ++index, *this;
        }

        iterator operator--(int) ENTT_NOEXCEPT {
            iterator orig = *this;
            return operator--(), orig;
        }

        iterator & operator+=(const difference_type value) ENTT_NOEXCEPT {
            index -= value;
            return *this;
        }

        iterator operator+(const difference_type value) const ENTT_NOEXCEPT {
            return iterator{index-value};
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

        reference operator[](const difference_type) const ENTT_NOEXCEPT {
            return {};
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
            return nullptr;
        }

        reference operator*() const ENTT_NOEXCEPT {
            return {};
        }

    private:
        index_type index;
    };

public:
    /*! @brief Type of the objects associated with the entities. */
    using object_type = Type;
    /*! @brief Underlying entity identifier. */
    using entity_type = Entity;
    /*! @brief Unsigned integer type. */
    using size_type = std::size_t;
    /*! @brief Random access iterator type. */
    using iterator_type = iterator;

    /**
     * @brief Returns an iterator to the beginning.
     *
     * The returned iterator points to the first instance of the given type. If
     * the storage is empty, the returned iterator will be equal to `end()`.
     *
     * @note
     * Input iterators stay true to the order imposed by a call to either `sort`
     * or `respect`.
     *
     * @return An iterator to the first instance of the given type.
     */
    iterator_type cbegin() const ENTT_NOEXCEPT {
        const typename traits_type::difference_type pos = underlying_type::size();
        return iterator_type{pos};
    }

    /*! @copydoc cbegin */
    iterator_type begin() const ENTT_NOEXCEPT {
        return cbegin();
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
    iterator_type cend() const ENTT_NOEXCEPT {
        return iterator_type{};
    }

    /*! @copydoc cend */
    iterator_type end() const ENTT_NOEXCEPT {
        return cend();
    }

    /**
     * @brief Returns the object associated with an entity.
     *
     * @note
     * Empty types aren't explicitly instantiated. Therefore, this function
     * always returns a temporary object.
     *
     * @warning
     * Attempting to use an entity that doesn't belong to the storage results in
     * undefined behavior.<br/>
     * An assertion will abort the execution at runtime in debug mode if the
     * storage doesn't contain the given entity.
     *
     * @param entt A valid entity identifier.
     * @return The object associated with the entity.
     */
    object_type get([[maybe_unused]] const entity_type entt) const {
        ENTT_ASSERT(underlying_type::has(entt));
        return {};
    }

    /**
     * @brief Assigns an entity to a storage and constructs its object.
     *
     * @warning
     * Attempting to use an entity that already belongs to the storage results
     * in undefined behavior.<br/>
     * An assertion will abort the execution at runtime in debug mode if the
     * storage already contains the given entity.
     *
     * @tparam Args Types of arguments to use to construct the object.
     * @param entt A valid entity identifier.
     */
    template<typename... Args>
    void construct(const entity_type entt, Args &&...) {
        underlying_type::construct(entt);
    }

    /**
     * @brief Assigns one or more entities to a storage.
     *
     * @warning
     * Attempting to assign an entity that already belongs to the storage
     * results in undefined behavior.<br/>
     * An assertion will abort the execution at runtime in debug mode if the
     * storage already contains the given entity.
     *
     * @tparam It Type of input iterator.
     * @param first An iterator to the first element of the range of entities.
     * @param last An iterator past the last element of the range of entities.
     */
    template<typename It>
    std::enable_if_t<std::is_same_v<typename std::iterator_traits<It>::value_type, entity_type>, void>
    construct(It first, It last, const object_type & = {}) {
        underlying_type::construct(first, last);
    }

    /*! @copydoc storage::sort */
    template<typename Compare, typename Sort = std_sort, typename... Args>
    void sort(iterator_type first, iterator_type last, Compare compare, Sort algo = Sort{}, Args &&... args) {
        ENTT_ASSERT(!(last < first));
        ENTT_ASSERT(!(last > end()));

        const auto from = underlying_type::begin() + std::distance(begin(), first);
        const auto to = from + std::distance(first, last);

        underlying_type::sort(from, to, std::move(compare), std::move(algo), std::forward<Args>(args)...);
    }
};


}


#endif

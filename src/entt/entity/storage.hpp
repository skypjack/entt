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
#include "../core/type_traits.hpp"
#include "entity.hpp"
#include "fwd.hpp"
#include "sparse_set.hpp"


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
 * Internal data structures arrange elements to maximize performance. There are
 * no guarantees that objects are returned in the insertion order when iterate
 * a storage. Do not make assumption on the order in any case.
 *
 * @warning
 * Empty types aren't explicitly instantiated. Therefore, many of the functions
 * normally available for non-empty types will not be available for empty ones.
 *
 * @sa sparse_set<Entity>
 *
 * @tparam Entity A valid entity type (see entt_traits for more details).
 * @tparam Type Type of objects assigned to the entities.
 */
template<typename Entity, typename Type, typename = void>
class basic_storage: public basic_sparse_set<Entity> {
    static_assert(std::is_move_constructible_v<Type> && std::is_move_assignable_v<Type>, "The managed type must be at least move constructible and assignable");

    using underlying_type = basic_sparse_set<Entity>;
    using traits_type = entt_traits<Entity>;

    template<bool Const>
    class storage_iterator final {
        friend class basic_storage<Entity, Type>;

        using instance_type = std::conditional_t<Const, const std::vector<Type>, std::vector<Type>>;
        using index_type = typename traits_type::difference_type;

        storage_iterator(instance_type &ref, const index_type idx) ENTT_NOEXCEPT
            : instances{&ref}, index{idx}
        {}

    public:
        using difference_type = index_type;
        using value_type = Type;
        using pointer = std::conditional_t<Const, const value_type *, value_type *>;
        using reference = std::conditional_t<Const, const value_type &, value_type &>;
        using iterator_category = std::random_access_iterator_tag;

        storage_iterator() ENTT_NOEXCEPT = default;

        storage_iterator & operator++() ENTT_NOEXCEPT {
            return --index, *this;
        }

        storage_iterator operator++(int) ENTT_NOEXCEPT {
            storage_iterator orig = *this;
            return ++(*this), orig;
        }

        storage_iterator & operator--() ENTT_NOEXCEPT {
            return ++index, *this;
        }

        storage_iterator operator--(int) ENTT_NOEXCEPT {
            storage_iterator orig = *this;
            return operator--(), orig;
        }

        storage_iterator & operator+=(const difference_type value) ENTT_NOEXCEPT {
            index -= value;
            return *this;
        }

        storage_iterator operator+(const difference_type value) const ENTT_NOEXCEPT {
            storage_iterator copy = *this;
            return (copy += value);
        }

        storage_iterator & operator-=(const difference_type value) ENTT_NOEXCEPT {
            return (*this += -value);
        }

        storage_iterator operator-(const difference_type value) const ENTT_NOEXCEPT {
            return (*this + -value);
        }

        difference_type operator-(const storage_iterator &other) const ENTT_NOEXCEPT {
            return other.index - index;
        }

        [[nodiscard]] reference operator[](const difference_type value) const ENTT_NOEXCEPT {
            const auto pos = size_type(index-value-1);
            return (*instances)[pos];
        }

        [[nodiscard]] bool operator==(const storage_iterator &other) const ENTT_NOEXCEPT {
            return other.index == index;
        }

        [[nodiscard]] bool operator!=(const storage_iterator &other) const ENTT_NOEXCEPT {
            return !(*this == other);
        }

        [[nodiscard]] bool operator<(const storage_iterator &other) const ENTT_NOEXCEPT {
            return index > other.index;
        }

        [[nodiscard]] bool operator>(const storage_iterator &other) const ENTT_NOEXCEPT {
            return index < other.index;
        }

        [[nodiscard]] bool operator<=(const storage_iterator &other) const ENTT_NOEXCEPT {
            return !(*this > other);
        }

        [[nodiscard]] bool operator>=(const storage_iterator &other) const ENTT_NOEXCEPT {
            return !(*this < other);
        }

        [[nodiscard]] pointer operator->() const ENTT_NOEXCEPT {
            const auto pos = size_type(index-1u);
            return &(*instances)[pos];
        }

        [[nodiscard]] reference operator*() const ENTT_NOEXCEPT {
            return *operator->();
        }

    private:
        instance_type *instances;
        index_type index;
    };

    void swap_at(const std::size_t lhs, const std::size_t rhs) final {
        std::swap(instances[lhs], instances[rhs]);
    }

    void swap_and_pop(const std::size_t pos) final {
        instances[pos] = std::move(instances.back());
        instances.pop_back();
    }

    void clear_all() ENTT_NOEXCEPT final {
        instances.clear();
    }

public:
    /*! @brief Type of the objects associated with the entities. */
    using value_type = Type;
    /*! @brief Underlying entity identifier. */
    using entity_type = Entity;
    /*! @brief Unsigned integer type. */
    using size_type = std::size_t;
    /*! @brief Random access iterator type. */
    using iterator = storage_iterator<false>;
    /*! @brief Constant random access iterator type. */
    using const_iterator = storage_iterator<true>;
    /*! @brief Reverse iterator type. */
    using reverse_iterator = Type *;
    /*! @brief Constant reverse iterator type. */
    using const_reverse_iterator = const Type *;

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
     * The returned pointer is such that range `[raw(), raw() + size())` is
     * always a valid range, even if the container is empty.
     *
     * @note
     * Objects are in the reverse order as returned by the `begin`/`end`
     * iterators.
     *
     * @return A pointer to the array of objects.
     */
    [[nodiscard]] const value_type * raw() const ENTT_NOEXCEPT {
        return instances.data();
    }

    /*! @copydoc raw */
    [[nodiscard]] value_type * raw() ENTT_NOEXCEPT {
        return const_cast<value_type *>(std::as_const(*this).raw());
    }

    /**
     * @brief Returns an iterator to the beginning.
     *
     * The returned iterator points to the first instance of the internal array.
     * If the storage is empty, the returned iterator will be equal to `end()`.
     *
     * @return An iterator to the first instance of the internal array.
     */
    [[nodiscard]] const_iterator cbegin() const ENTT_NOEXCEPT {
        const typename traits_type::difference_type pos = underlying_type::size();
        return const_iterator{instances, pos};
    }

    /*! @copydoc cbegin */
    [[nodiscard]] const_iterator begin() const ENTT_NOEXCEPT {
        return cbegin();
    }

    /*! @copydoc begin */
    [[nodiscard]] iterator begin() ENTT_NOEXCEPT {
        const typename traits_type::difference_type pos = underlying_type::size();
        return iterator{instances, pos};
    }

    /**
     * @brief Returns an iterator to the end.
     *
     * The returned iterator points to the element following the last instance
     * of the internal array. Attempting to dereference the returned iterator
     * results in undefined behavior.
     *
     * @return An iterator to the element following the last instance of the
     * internal array.
     */
    [[nodiscard]] const_iterator cend() const ENTT_NOEXCEPT {
        return const_iterator{instances, {}};
    }

    /*! @copydoc cend */
    [[nodiscard]] const_iterator end() const ENTT_NOEXCEPT {
        return cend();
    }

    /*! @copydoc end */
    [[nodiscard]] iterator end() ENTT_NOEXCEPT {
        return iterator{instances, {}};
    }

    /**
     * @brief Returns a reverse iterator to the beginning.
     *
     * The returned iterator points to the first instance of the reversed
     * internal array. If the storage is empty, the returned iterator will be
     * equal to `rend()`.
     *
     * @return An iterator to the first instance of the reversed internal array.
     */
    [[nodiscard]] const_reverse_iterator crbegin() const ENTT_NOEXCEPT {
        return instances.data();
    }

    /*! @copydoc crbegin */
    [[nodiscard]] const_reverse_iterator rbegin() const ENTT_NOEXCEPT {
        return crbegin();
    }

    /*! @copydoc rbegin */
    [[nodiscard]] reverse_iterator rbegin() ENTT_NOEXCEPT {
        return instances.data();
    }

    /**
     * @brief Returns a reverse iterator to the end.
     *
     * The returned iterator points to the element following the last instance
     * of the reversed internal array. Attempting to dereference the returned
     * iterator results in undefined behavior.
     *
     * @return An iterator to the element following the last instance of the
     * reversed internal array.
     */
    [[nodiscard]] const_reverse_iterator crend() const ENTT_NOEXCEPT {
        return crbegin() + instances.size();
    }

    /*! @copydoc crend */
    [[nodiscard]] const_reverse_iterator rend() const ENTT_NOEXCEPT {
        return crend();
    }

    /*! @copydoc rend */
    [[nodiscard]] reverse_iterator rend() ENTT_NOEXCEPT {
        return rbegin() + instances.size();
    }

    /**
     * @brief Returns the object associated with an entity.
     *
     * @warning
     * Attempting to use an entity that doesn't belong to the storage results in
     * undefined behavior.
     *
     * @param entt A valid entity identifier.
     * @return The object associated with the entity.
     */
    [[nodiscard]] const value_type & get(const entity_type entt) const {
        return instances[underlying_type::index(entt)];
    }

    /*! @copydoc get */
    [[nodiscard]] value_type & get(const entity_type entt) {
        return const_cast<value_type &>(std::as_const(*this).get(entt));
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
     * in undefined behavior.
     *
     * @tparam Args Types of arguments to use to construct the object.
     * @param entt A valid entity identifier.
     * @param args Parameters to use to construct an object for the entity.
     * @return A reference to the newly created object.
     */
    template<typename... Args>
    value_type & emplace(const entity_type entt, Args &&... args) {
        if constexpr(std::is_aggregate_v<value_type>) {
            instances.push_back(Type{std::forward<Args>(args)...});
        } else {
            instances.emplace_back(std::forward<Args>(args)...);
        }

        // entity goes after component in case constructor throws
        underlying_type::emplace(entt);
        return instances.back();
    }

    /**
     * @brief Assigns one or more entities to a storage and constructs their
     * objects from a given instance.
     *
     * @warning
     * Attempting to assign an entity that already belongs to the storage
     * results in undefined behavior.
     *
     * @tparam It Type of input iterator.
     * @param first An iterator to the first element of the range of entities.
     * @param last An iterator past the last element of the range of entities.
     * @param value An instance of the object to construct.
     */
    template<typename It>
    void insert(It first, It last, const value_type &value = {}) {
        instances.insert(instances.end(), std::distance(first, last), value);
        // entities go after components in case constructors throw
        underlying_type::insert(first, last);
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
     * @param from An iterator to the first element of the range of objects.
     * @param to An iterator past the last element of the range of objects.
     */
    template<typename EIt, typename CIt>
    void insert(EIt first, EIt last, CIt from, CIt to) {
        instances.insert(instances.end(), from, to);
        // entities go after components in case constructors throw
        underlying_type::insert(first, last);
    }

    /**
     * @brief Sort elements according to the given comparison function.
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
     * @warning
     * Empty types are never instantiated. Therefore, only comparison function
     * objects that require to return entities rather than components are
     * accepted.
     *
     * @tparam Compare Type of comparison function object.
     * @tparam Sort Type of sort function object.
     * @tparam Args Types of arguments to forward to the sort function object.
     * @param count Number of elements to sort.
     * @param compare A valid comparison function object.
     * @param algo A valid sort function object.
     * @param args Arguments to forward to the sort function object, if any.
     */
    template<typename Compare, typename Sort = std_sort, typename... Args>
    void sort_n(const size_type count, Compare compare, Sort algo = Sort{}, Args &&... args) {
        if constexpr(std::is_invocable_v<Compare, const value_type &, const value_type &>) {
            underlying_type::sort_n(count, [this, compare = std::move(compare)](const auto lhs, const auto rhs) {
                return compare(std::as_const(instances[underlying_type::index(lhs)]), std::as_const(instances[underlying_type::index(rhs)]));
            }, std::move(algo), std::forward<Args>(args)...);
        } else {
            underlying_type::sort_n(count, std::move(compare), std::move(algo), std::forward<Args>(args)...);
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
        sort_n(size(), std::move(compare), std::move(algo), std::forward<Args>(args)...);
    }

private:
    std::vector<value_type> instances;
};


/*! @copydoc basic_storage */
template<typename Entity, typename Type>
class basic_storage<Entity, Type, std::enable_if_t<is_empty_v<Type>>>: public basic_sparse_set<Entity> {
    using underlying_type = basic_sparse_set<Entity>;

public:
    /*! @brief Type of the objects associated with the entities. */
    using value_type = Type;
    /*! @brief Underlying entity identifier. */
    using entity_type = Entity;
    /*! @brief Unsigned integer type. */
    using size_type = std::size_t;

    /**
     * @brief Assigns an entity to a storage and constructs its object.
     *
     * @warning
     * Attempting to use an entity that already belongs to the storage results
     * in undefined behavior.
     *
     * @tparam Args Types of arguments to use to construct the object.
     * @param entt A valid entity identifier.
     * @param args Parameters to use to construct an object for the entity.
     */
    template<typename... Args>
    void emplace(const entity_type entt, Args &&... args) {
        [[maybe_unused]] value_type instance{std::forward<Args>(args)...};
        underlying_type::emplace(entt);
    }

    /**
     * @brief Assigns one or more entities to a storage.
     *
     * @warning
     * Attempting to assign an entity that already belongs to the storage
     * results in undefined behavior.
     *
     * @tparam It Type of input iterator.
     * @param first An iterator to the first element of the range of entities.
     * @param last An iterator past the last element of the range of entities.
     */
    template<typename It>
    void insert(It first, It last, const value_type & = {}) {
        underlying_type::insert(first, last);
    }
};


}


#endif

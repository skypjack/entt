#ifndef ENTT_ENTITY_STORAGE_HPP
#define ENTT_ENTITY_STORAGE_HPP


#include <algorithm>
#include <iterator>
#include <numeric>
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
 * access (either to entities or objects) and when using input iterators.
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
class basic_storage: public sparse_set<Entity> {
    using underlying_type = sparse_set<Entity>;
    using traits_type = entt_traits<Entity>;

    template<bool Const>
    class iterator {
        friend class basic_storage<Entity, Type>;

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
    void reserve(const size_type cap) override {
        underlying_type::reserve(cap);
        instances.reserve(cap);
    }

    /*! @brief Requests the removal of unused capacity. */
    void shrink_to_fit() override {
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
     * Attempting to use an entity that doesn't belong to the storage results in
     * undefined behavior.<br/>
     * An assertion will abort the execution at runtime in debug mode if the
     * storage doesn't contain the given entity.
     *
     * @param entt A valid entity identifier.
     * @return The object associated with the entity.
     */
    const object_type & get(const entity_type entt) const ENTT_NOEXCEPT {
        return instances[underlying_type::get(entt)];
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
        return underlying_type::has(entt) ? (instances.data() + underlying_type::get(entt)) : nullptr;
    }

    /*! @copydoc try_get */
    inline object_type * try_get(const entity_type entt) ENTT_NOEXCEPT {
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
     * @return The object associated with the entity.
     */
    template<typename... Args>
    object_type & construct(const entity_type entt, Args &&... args) {
        if constexpr(std::is_aggregate_v<object_type>) {
            instances.emplace_back(Type{std::forward<Args>(args)...});
        } else {
            instances.emplace_back(std::forward<Args>(args)...);
        }

        // entity goes after component in case constructor throws
        underlying_type::construct(entt);
        return instances.back();
    }

    /**
     * @brief Assigns one or more entities to a storage and constructs their
     * objects.
     *
     * The object type must be at least default constructible.
     *
     * @warning
     * Attempting to assign an entity that already belongs to the storage
     * results in undefined behavior.<br/>
     * An assertion will abort the execution at runtime in debug mode if the
     * storage already contains the given entity.
     *
     * @tparam It Type of forward iterator.
     * @param first An iterator to the first element of the range of entities.
     * @param last An iterator past the last element of the range of entities.
     * @return A pointer to the array of instances just created and sorted the
     * same of the entities.
     */
    template<typename It>
    object_type * batch(It first, It last) {
        static_assert(std::is_default_constructible_v<object_type>);
        const auto skip = instances.size();
        instances.insert(instances.end(), last-first, {});
        // entity goes after component in case constructor throws
        underlying_type::batch(first, last);
        return instances.data() + skip;
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
    void destroy(const entity_type entt) override {
        std::swap(instances[underlying_type::get(entt)], instances.back());
        instances.pop_back();
        underlying_type::destroy(entt);
    }

    /**
     * @brief Sort instances according to the given comparison function.
     *
     * Sort the elements so that iterating the storage with a couple of
     * iterators returns them in the expected order. See `begin` and `end` for
     * more details.
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
     * The comparison function object received by the sort function object
     * hasn't necessarily the type of the one passed along with the other
     * parameters to this member function.
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
     * @param algo A valid sort function object.
     * @param args Arguments to forward to the sort function object, if any.
     */
    template<typename Compare, typename Sort = std_sort, typename... Args>
    void sort(Compare compare, Sort algo = Sort{}, Args &&... args) {
        std::vector<size_type> copy(instances.size());
        std::iota(copy.begin(), copy.end(), 0);

        if constexpr(std::is_invocable_v<Compare, const object_type &, const object_type &>) {
            static_assert(!std::is_empty_v<object_type>);

            algo(copy.rbegin(), copy.rend(), [this, compare = std::move(compare)](const auto lhs, const auto rhs) {
                return compare(std::as_const(instances[lhs]), std::as_const(instances[rhs]));
            }, std::forward<Args>(args)...);
        } else {
            algo(copy.rbegin(), copy.rend(), [compare = std::move(compare), data = underlying_type::data()](const auto lhs, const auto rhs) {
                return compare(data[lhs], data[rhs]);
            }, std::forward<Args>(args)...);
        }

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
     * @brief Sort instances according to the order of the entities in another
     * sparse set.
     *
     * Entities that are part of both the storage are ordered internally
     * according to the order they have in `other`. All the other entities goes
     * to the end of the list and there are no guarantess on their order.
     * Instances are sorted according to the entities to which they belong.<br/>
     * In other terms, this function can be used to impose the same order on two
     * sets by using one of them as a master and the other one as a slave.
     *
     * Iterating the storage with a couple of iterators returns elements in the
     * expected order after a call to `respect`. See `begin` and `end` for more
     * details.
     *
     * @note
     * Attempting to iterate elements using a raw pointer returned by a call to
     * either `data` or `raw` gives no guarantees on the order, even though
     * `respect` has been invoked.
     *
     * @param other The sparse sets that imposes the order of the entities.
     */
    void respect(const sparse_set<Entity> &other) ENTT_NOEXCEPT override {
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

    /*! @brief Resets a storage. */
    void reset() override {
        underlying_type::reset();
        instances.clear();
    }

private:
    std::vector<object_type> instances;
};


/*! @copydoc basic_storage */
template<typename Entity, typename Type>
class basic_storage<Entity, Type, std::enable_if_t<std::is_empty_v<Type>>>: public sparse_set<Entity> {
    using underlying_type = sparse_set<Entity>;
    using traits_type = entt_traits<Entity>;

    class iterator {
        friend class basic_storage<Entity, Type>;

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
            return iterator{index-value};
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
            return {};
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
            return nullptr;
        }

        inline reference operator*() const ENTT_NOEXCEPT {
            return {};
        }

    private:
        index_type index;
    };

public:
    /*! @brief Type of the objects associated with the entities. */
    using object_type = Type;
    /*! @brief Underlying entity identifier. */
    using entity_type = typename underlying_type::entity_type;
    /*! @brief Unsigned integer type. */
    using size_type = typename underlying_type::size_type;
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
    inline iterator_type begin() const ENTT_NOEXCEPT {
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
    inline iterator_type end() const ENTT_NOEXCEPT {
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
    object_type get([[maybe_unused]] const entity_type entt) const ENTT_NOEXCEPT {
        ENTT_ASSERT(underlying_type::has(entt));
        return {};
    }
};

/*! @copydoc basic_storage */
template<typename Entity, typename Type>
struct storage: basic_storage<Entity, Type> {};


}


#endif // ENTT_ENTITY_STORAGE_HPP

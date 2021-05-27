#ifndef ENTT_ENTITY_STORAGE_HPP
#define ENTT_ENTITY_STORAGE_HPP


#include <cstddef>
#include <iterator>
#include <memory>
#include <tuple>
#include <type_traits>
#include <utility>
#include "../config/config.h"
#include "../core/algorithm.hpp"
#include "../core/fwd.hpp"
#include "../core/type_traits.hpp"
#include "../signal/sigh.hpp"
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
 * Entities and objects have the same order.
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
 * @tparam Allocator Type of allocator used to manage memory and elements.
 */
template<typename Entity, typename Type, typename Allocator, typename>
class basic_storage: public basic_sparse_set<Entity, typename std::allocator_traits<Allocator>::template rebind_alloc<Entity>> {
    static_assert(std::is_move_constructible_v<Type> && std::is_move_assignable_v<Type>, "The managed type must be at least move constructible and assignable");

    static constexpr auto packed_page = ENTT_PACKED_PAGE;

    using underlying_type = basic_sparse_set<Entity>;
    using traits_type = entt_traits<Entity>;

    using alloc_type = typename std::allocator_traits<Allocator>::template rebind_alloc<Type>;
    using alloc_traits = std::allocator_traits<alloc_type>;
    using alloc_pointer = typename alloc_traits::pointer;
    using alloc_const_pointer = typename alloc_traits::const_pointer;

    using bucket_alloc_type = typename std::allocator_traits<Allocator>::template rebind_alloc<alloc_pointer>;
    using bucket_alloc_traits = std::allocator_traits<bucket_alloc_type>;
    using bucket_alloc_pointer = typename bucket_alloc_traits::pointer;

    using bucket_alloc_const_type = typename std::allocator_traits<Allocator>::template rebind_alloc<alloc_const_pointer>;
    using bucket_alloc_const_pointer = typename std::allocator_traits<bucket_alloc_const_type>::const_pointer;

    using entity_alloc_type = typename std::allocator_traits<Allocator>::template rebind_alloc<Entity>;

    static_assert(alloc_traits::propagate_on_container_move_assignment::value);
    static_assert(bucket_alloc_traits::propagate_on_container_move_assignment::value);

    template<typename Value>
    class storage_iterator final {
        friend class basic_storage<Entity, Type>;

        storage_iterator(bucket_alloc_pointer const *ref, const typename traits_type::difference_type idx) ENTT_NOEXCEPT
            : packed{ref}, index{idx}
        {}

    public:
        using difference_type = typename traits_type::difference_type;
        using value_type = Value;
        using pointer = value_type *;
        using reference = value_type &;
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
            return (*packed)[page(pos)][offset(pos)];
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
            return std::addressof((*packed)[page(pos)][offset(pos)]);
        }

        [[nodiscard]] reference operator*() const ENTT_NOEXCEPT {
            return *operator->();
        }

    private:
        bucket_alloc_pointer const *packed;
        difference_type index;
    };

    [[nodiscard]] static auto page(const std::size_t pos) ENTT_NOEXCEPT {
        return pos / packed_page;
    }

    [[nodiscard]] static auto offset(const std::size_t pos) ENTT_NOEXCEPT {
        return pos & (packed_page - 1);
    }

    void release_memory() {
        if(packed) {
            for(size_type pos{}; pos < bucket; ++pos) {
                if(count) {
                    const auto sz = count > packed_page ? packed_page : count;
                    std::destroy(packed[pos], packed[pos] + sz);
                    count -= sz;
                }

                alloc_traits::deallocate(allocator, packed[pos], packed_page);
                bucket_alloc_traits::destroy(bucket_allocator, std::addressof(packed[pos]));
            }

            bucket_alloc_traits::deallocate(bucket_allocator, std::exchange(packed, bucket_alloc_pointer{}), std::exchange(bucket, 0u));
        }
    }

    void maybe_resize_packed(const std::size_t req) {
        ENTT_ASSERT(!(req < count), "Invalid request");

        if(const auto length = std::exchange(bucket, (req + packed_page - 1u) / packed_page); bucket != length) {
            const auto old = std::exchange(packed, bucket_alloc_traits::allocate(bucket_allocator, bucket));

            if(bucket > length) {
                for(size_type pos{}; pos < length; ++pos) {
                    bucket_alloc_traits::construct(bucket_allocator, std::addressof(packed[pos]), old[pos]);
                    bucket_alloc_traits::destroy(bucket_allocator, std::addressof(old[pos]));
                }

                for(auto pos = length; pos < bucket; ++pos) {
                    bucket_alloc_traits::construct(bucket_allocator, std::addressof(packed[pos]), alloc_traits::allocate(allocator, packed_page));
                }
            } else if(bucket < length) {
                for(size_type pos{}; pos < bucket; ++pos) {
                    bucket_alloc_traits::construct(bucket_allocator, std::addressof(packed[pos]), old[pos]);
                    bucket_alloc_traits::destroy(bucket_allocator, std::addressof(old[pos]));
                }

                for(auto pos = bucket; pos < length; ++pos) {
                    alloc_traits::deallocate(allocator, old[pos], packed_page);
                    bucket_alloc_traits::destroy(bucket_allocator, std::addressof(old[pos]));
                }
            }

            bucket_alloc_traits::deallocate(bucket_allocator, old, length);
        }
    }

    template<typename... Args>
    Type & push_back(Args &&... args) {
        ENTT_ASSERT(count < (bucket * packed_page), "No more space left");
        auto &ref = packed[page(count)][offset(count)];

        if constexpr(std::is_aggregate_v<value_type>) {
            alloc_traits::construct(allocator, std::addressof(ref), Type{std::forward<Args>(args)...});
        } else {
            alloc_traits::construct(allocator, std::addressof(ref), std::forward<Args>(args)...);
        }

        // exception safety guarantee requires to update this after construction
        ++count;

        return ref;
    }

protected:
    /*! @copydoc basic_sparse_set::swap_at */
    void swap_at(const std::size_t lhs, const std::size_t rhs) final {
        std::swap(packed[page(lhs)][offset(lhs)], packed[page(rhs)][offset(rhs)]);
    }

    /*! @copydoc basic_sparse_set::swap_and_pop */
    void swap_and_pop(const std::size_t pos) final {
        auto &&elem = packed[page(pos)][offset(pos)];
        [[maybe_unused]] auto other = std::move(elem);
        auto &&last = (--count, packed[page(count)][offset(count)]);

        elem = std::move(last);
        alloc_traits::destroy(allocator, std::addressof(last));
    }

public:
    /*! @brief Allocator type. */
    using allocator_type = alloc_type;
    /*! @brief Type of the objects assigned to entities. */
    using value_type = Type;
    /*! @brief Underlying entity identifier. */
    using entity_type = Entity;
    /*! @brief Unsigned integer type. */
    using size_type = std::size_t;
    /*! @brief Pointer type to contained elements. */
    using pointer = bucket_alloc_pointer;
    /*! @brief Constant pointer type to contained elements. */
    using const_pointer = bucket_alloc_const_pointer;
    /*! @brief Random access iterator type. */
    using iterator = storage_iterator<Type>;
    /*! @brief Constant random access iterator type. */
    using const_iterator = storage_iterator<const Type>;
    /*! @brief Reverse iterator type. */
    using reverse_iterator = std::reverse_iterator<iterator>;
    /*! @brief Constant reverse iterator type. */
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;

    /**
     * @brief Default constructor.
     * @param alloc Allocator to use (possibly default-constructed).
     */
    explicit basic_storage(const allocator_type &alloc = {})
        : basic_sparse_set<entity_type, entity_alloc_type>{alloc},
          allocator{alloc},
          bucket_allocator{alloc},
          packed{},
          bucket{},
          count{}
    {}

    /**
     * @brief Move constructor.
     * @param other The instance to move from.
     */
    basic_storage(basic_storage &&other) ENTT_NOEXCEPT
        : basic_sparse_set<entity_type, entity_alloc_type>{std::move(other)},
          allocator{std::move(other.allocator)},
          bucket_allocator{std::move(other.bucket_allocator)},
          packed{std::exchange(other.packed, bucket_alloc_pointer{})},
          bucket{std::exchange(other.bucket, 0u)},
          count{std::exchange(other.count, 0u)}
    {}

    /*! @brief Default destructor. */
    ~basic_storage() override {
        release_memory();
    }

    /**
     * @brief Move assignment operator.
     * @param other The instance to move from.
     * @return This sparse set.
     */
    basic_storage & operator=(basic_storage &&other) ENTT_NOEXCEPT {
        basic_sparse_set<entity_type, entity_alloc_type>::operator=(std::move(other));

        release_memory();

        allocator = std::move(other.allocator);
        bucket_allocator = std::move(other.bucket_allocator);
        packed = std::exchange(other.packed, bucket_alloc_pointer{});
        bucket = std::exchange(other.bucket, 0u);
        count = std::exchange(other.count, 0u);

        return *this;
    }

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

        if(cap > (bucket * packed_page)) {
            maybe_resize_packed(cap);
        }
    }

    /**
     * @brief Returns the number of elements that a storage has currently
     * allocated space for.
     * @return Capacity of the storage.
     */
    [[nodiscard]] size_type capacity() const ENTT_NOEXCEPT {
        return bucket * packed_page;
    }

    /*! @brief Requests the removal of unused capacity. */
    void shrink_to_fit() {
        underlying_type::shrink_to_fit();
        maybe_resize_packed(count);
    }

    /**
     * @brief Direct access to the array of objects.
     * @return A pointer to the array of objects.
     */
    [[nodiscard]] const_pointer raw() const ENTT_NOEXCEPT {
        return packed;
    }

    /*! @copydoc raw */
    [[nodiscard]] pointer raw() ENTT_NOEXCEPT {
        return packed;
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
        return const_iterator{&packed, pos};
    }

    /*! @copydoc cbegin */
    [[nodiscard]] const_iterator begin() const ENTT_NOEXCEPT {
        return cbegin();
    }

    /*! @copydoc begin */
    [[nodiscard]] iterator begin() ENTT_NOEXCEPT {
        const typename traits_type::difference_type pos = underlying_type::size();
        return iterator{&packed, pos};
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
        return const_iterator{&packed, {}};
    }

    /*! @copydoc cend */
    [[nodiscard]] const_iterator end() const ENTT_NOEXCEPT {
        return cend();
    }

    /*! @copydoc end */
    [[nodiscard]] iterator end() ENTT_NOEXCEPT {
        return iterator{&packed, {}};
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
        return std::make_reverse_iterator(cend());
    }

    /*! @copydoc crbegin */
    [[nodiscard]] const_reverse_iterator rbegin() const ENTT_NOEXCEPT {
        return crbegin();
    }

    /*! @copydoc rbegin */
    [[nodiscard]] reverse_iterator rbegin() ENTT_NOEXCEPT {
        return std::make_reverse_iterator(end());
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
        return std::make_reverse_iterator(cbegin());
    }

    /*! @copydoc crend */
    [[nodiscard]] const_reverse_iterator rend() const ENTT_NOEXCEPT {
        return crend();
    }

    /*! @copydoc rend */
    [[nodiscard]] reverse_iterator rend() ENTT_NOEXCEPT {
        return std::make_reverse_iterator(begin());
    }

    /**
     * @brief Returns the object assigned to an entity.
     *
     * @warning
     * Attempting to use an entity that doesn't belong to the storage results in
     * undefined behavior.
     *
     * @param entt A valid entity identifier.
     * @return The object assigned to the entity.
     */
    [[nodiscard]] const value_type & get(const entity_type entt) const {
        const auto idx = underlying_type::index(entt);
        return packed[page(idx)][offset(idx)];
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
        maybe_resize_packed(count + 1u);
        auto &value = push_back(std::forward<Args>(args)...);
        // entity goes after component in case constructor throws
        underlying_type::emplace(entt);
        return value;
    }

    /**
     * @brief Updates the instance assigned to a given entity in-place.
     * @tparam Func Types of the function objects to invoke.
     * @param entity A valid entity identifier.
     * @param func Valid function objects.
     * @return A reference to the updated instance.
     */
    template<typename... Func>
    decltype(auto) patch(const entity_type entity, Func &&... func) {
        const auto idx = underlying_type::index(entity);
        auto &&elem = packed[page(idx)][offset(idx)];
        (std::forward<Func>(func)(elem), ...);
        return elem;
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
        const auto sz = count + std::distance(first, last);
        underlying_type::reserve(sz);
        maybe_resize_packed(sz);

        for(; first != last; ++first) {
            push_back(value);
            underlying_type::emplace(*first);
        }
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
     */
    template<typename EIt, typename CIt, typename = std::enable_if_t<std::is_same_v<std::decay_t<typename std::iterator_traits<CIt>::value_type>, value_type>>>
    void insert(EIt first, EIt last, CIt from) {
        const auto sz = count + std::distance(first, last);
        underlying_type::reserve(sz);
        maybe_resize_packed(sz);

        for(; first != last; ++first, ++from) {
            push_back(*from);
            underlying_type::emplace(*first);
        }
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
     * @param length Number of elements to sort.
     * @param compare A valid comparison function object.
     * @param algo A valid sort function object.
     * @param args Arguments to forward to the sort function object, if any.
     */
    template<typename Compare, typename Sort = std_sort, typename... Args>
    void sort_n(const size_type length, Compare compare, Sort algo = Sort{}, Args &&... args) {
        if constexpr(std::is_invocable_v<Compare, const value_type &, const value_type &>) {
            underlying_type::sort_n(length, [this, compare = std::move(compare)](const auto lhs, const auto rhs) {
                const auto ilhs = underlying_type::index(lhs), irhs = underlying_type::index(rhs);
                return compare(std::as_const(packed[page(ilhs)][offset(ilhs)]), std::as_const(packed[page(irhs)][offset(irhs)]));
            }, std::move(algo), std::forward<Args>(args)...);
        } else {
            underlying_type::sort_n(length, std::move(compare), std::move(algo), std::forward<Args>(args)...);
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
        sort_n(underlying_type::size(), std::move(compare), std::move(algo), std::forward<Args>(args)...);
    }

private:
    alloc_type allocator;
    bucket_alloc_type bucket_allocator;
    bucket_alloc_pointer packed;
    std::size_t bucket;
    std::size_t count;
};


/*! @copydoc basic_storage */
template<typename Entity, typename Type, typename Allocator>
class basic_storage<Entity, Type, Allocator, std::enable_if_t<is_empty_v<Type>>>: public basic_sparse_set<Entity, typename std::allocator_traits<Allocator>::template rebind_alloc<Entity>> {
    using underlying_type = basic_sparse_set<Entity>;

public:
    /*! @brief Type of the objects assigned to entities. */
    using value_type = Type;
    /*! @brief Underlying entity identifier. */
    using entity_type = Entity;
    /*! @brief Unsigned integer type. */
    using size_type = std::size_t;

    /**
     * @brief Fake get function.
     *
     * @warning
     * Attempting to use an entity that doesn't belong to the storage results in
     * undefined behavior.
     *
     * @param entt A valid entity identifier.
     */
    void get([[maybe_unused]] const entity_type entt) const {
        ENTT_ASSERT(underlying_type::contains(entt), "Storage does not contain entity");
    }

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
    * @brief Updates the instance assigned to a given entity in-place.
    * @tparam Func Types of the function objects to invoke.
    * @param entity A valid entity identifier.
    * @param func Valid function objects.
    */
    template<typename... Func>
    void patch([[maybe_unused]] const entity_type entity, Func &&... func) {
        ENTT_ASSERT(underlying_type::contains(entity), "Storage does not contain entity");
        (std::forward<Func>(func)(), ...);
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


/**
 * @brief Mixin type to use to wrap basic storage classes.
 * @tparam Type The type of the underlying storage.
 */
template<typename Type>
struct storage_adapter_mixin: Type {
    static_assert(std::is_same_v<typename Type::value_type, std::decay_t<typename Type::value_type>>, "Invalid object type");

    /*! @brief Type of the objects assigned to entities. */
    using value_type = typename Type::value_type;
    /*! @brief Underlying entity identifier. */
    using entity_type = typename Type::entity_type;

    /*! @brief Inherited constructors. */
    using Type::Type;

    /**
     * @brief Assigns entities to a storage.
     * @tparam Args Types of arguments to use to construct the object.
     * @param entity A valid entity identifier.
     * @param args Parameters to use to initialize the object.
     * @return A reference to the newly created object.
     */
    template<typename... Args>
    decltype(auto) emplace(basic_registry<entity_type> &, const entity_type entity, Args &&... args) {
        return Type::emplace(entity, std::forward<Args>(args)...);
    }

    /**
     * @brief Assigns entities to a storage.
     * @tparam It Type of input iterator.
     * @tparam Args Types of arguments to use to construct the objects assigned
     * to the entities.
     * @param first An iterator to the first element of the range of entities.
     * @param last An iterator past the last element of the range of entities.
     * @param args Parameters to use to initialize the objects assigned to the
     * entities.
     */
    template<typename It, typename... Args>
    void insert(basic_registry<entity_type> &, It first, It last, Args &&... args) {
        Type::insert(first, last, std::forward<Args>(args)...);
    }

    /**
     * @brief Patches the given instance for an entity.
     * @tparam Func Types of the function objects to invoke.
     * @param entity A valid entity identifier.
     * @param func Valid function objects.
     * @return A reference to the patched instance.
     */
    template<typename... Func>
    decltype(auto) patch(basic_registry<entity_type> &, const entity_type entity, Func &&... func) {
        return Type::patch(entity, std::forward<Func>(func)...);
    }
};


/**
 * @brief Mixin type to use to add signal support to storage types.
 * @tparam Type The type of the underlying storage.
 */
template<typename Type>
class sigh_storage_mixin final: public Type {
    /*! @copydoc basic_sparse_set::about_to_erase */
    void about_to_erase(const typename Type::entity_type entity, void *ud) final {
        ENTT_ASSERT(ud != nullptr, "Invalid pointer to registry");
        destruction.publish(*static_cast<basic_registry<typename Type::entity_type> *>(ud), entity);
        Type::about_to_erase(entity, ud);
    }

public:
    /*! @brief Underlying value type. */
    using value_type = typename Type::value_type;
    /*! @brief Underlying entity identifier. */
    using entity_type = typename Type::entity_type;

    /*! @brief Inherited constructors. */
    using Type::Type;

    /**
     * @brief Returns a sink object.
     *
     * The sink returned by this function can be used to receive notifications
     * whenever a new instance is created and assigned to an entity.<br/>
     * The function type for a listener is equivalent to:
     *
     * @code{.cpp}
     * void(basic_registry<entity_type> &, entity_type);
     * @endcode
     *
     * Listeners are invoked **after** the object has been assigned to the
     * entity.
     *
     * @sa sink
     *
     * @return A temporary sink object.
     */
    [[nodiscard]] auto on_construct() ENTT_NOEXCEPT {
        return sink{construction};
    }

    /**
     * @brief Returns a sink object.
     *
     * The sink returned by this function can be used to receive notifications
     * whenever an instance is explicitly updated.<br/>
     * The function type for a listener is equivalent to:
     *
     * @code{.cpp}
     * void(basic_registry<entity_type> &, entity_type);
     * @endcode
     *
     * Listeners are invoked **after** the object has been updated.
     *
     * @sa sink
     *
     * @return A temporary sink object.
     */
    [[nodiscard]] auto on_update() ENTT_NOEXCEPT {
        return sink{update};
    }

    /**
     * @brief Returns a sink object.
     *
     * The sink returned by this function can be used to receive notifications
     * whenever an instance is removed from an entity and thus destroyed.<br/>
     * The function type for a listener is equivalent to:
     *
     * @code{.cpp}
     * void(basic_registry<entity_type> &, entity_type);
     * @endcode
     *
     * Listeners are invoked **before** the object has been removed from the
     * entity.
     *
     * @sa sink
     *
     * @return A temporary sink object.
     */
    [[nodiscard]] auto on_destroy() ENTT_NOEXCEPT {
        return sink{destruction};
    }

    /**
     * @brief Assigns entities to a storage.
     * @tparam Args Types of arguments to use to construct the object.
     * @param owner The registry that issued the request.
     * @param entity A valid entity identifier.
     * @param args Parameters to use to initialize the object.
     * @return A reference to the newly created object.
     */
    template<typename... Args>
    decltype(auto) emplace(basic_registry<entity_type> &owner, const entity_type entity, Args &&... args) {
        Type::emplace(entity, std::forward<Args>(args)...);
        construction.publish(owner, entity);
        return this->get(entity);
    }

    /**
     * @brief Assigns entities to a storage.
     * @tparam It Type of input iterator.
     * @tparam Args Types of arguments to use to construct the objects assigned
     * to the entities.
     * @param owner The registry that issued the request.
     * @param first An iterator to the first element of the range of entities.
     * @param last An iterator past the last element of the range of entities.
     * @param args Parameters to use to initialize the objects assigned to the
     * entities.
     */
    template<typename It, typename... Args>
    void insert(basic_registry<entity_type> &owner, It first, It last, Args &&... args) {
        Type::insert(first, last, std::forward<Args>(args)...);

        if(!construction.empty()) {
            for(; first != last; ++first) {
                construction.publish(owner, *first);
            }
        }
    }

    /**
     * @brief Patches the given instance for an entity.
     * @tparam Func Types of the function objects to invoke.
     * @param owner The registry that issued the request.
     * @param entity A valid entity identifier.
     * @param func Valid function objects.
     * @return A reference to the patched instance.
     */
    template<typename... Func>
    decltype(auto) patch(basic_registry<entity_type> &owner, const entity_type entity, Func &&... func) {
        Type::patch(entity, std::forward<Func>(func)...);
        update.publish(owner, entity);
        return this->get(entity);
    }

private:
    sigh<void(basic_registry<entity_type> &, const entity_type)> construction{};
    sigh<void(basic_registry<entity_type> &, const entity_type)> destruction{};
    sigh<void(basic_registry<entity_type> &, const entity_type)> update{};
};


/**
 * @brief Defines the component-to-storage conversion.
 * @tparam Entity A valid entity type (see entt_traits for more details).
 * @tparam Type Type of objects assigned to the entities.
 */
template<typename Entity, typename Type, typename = void>
struct storage_traits {
    /*! @brief Resulting type after component-to-storage conversion. */
    using storage_type = sigh_storage_mixin<basic_storage<Entity, Type>>;
};


/**
 * @brief Gets the element assigned to an entity from a storage, if any.
 * @tparam Type Storage type.
 * @param container A valid instance of a storage class.
 * @param entity A valid entity identifier.
 * @return A possibly empty tuple containing the requested element.
 */
template<typename Type>
[[nodiscard]] auto get_as_tuple([[maybe_unused]] Type &container, [[maybe_unused]] const typename Type::entity_type entity) {
    static_assert(std::is_same_v<std::remove_const_t<Type>, typename storage_traits<typename Type::entity_type, typename Type::value_type>::storage_type>, "Invalid storage");

    if constexpr(std::is_void_v<decltype(container.get({}))>) {
        return std::make_tuple();
    } else {
        return std::forward_as_tuple(container.get(entity));
    }
}


}


#endif

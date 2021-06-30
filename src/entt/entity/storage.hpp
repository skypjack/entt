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
#include "component.hpp"
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
template<typename Entity, typename Type, typename Allocator, typename = void>
class basic_storage_impl: public basic_sparse_set<Entity, typename std::allocator_traits<Allocator>::template rebind_alloc<Entity>> {
    static constexpr auto packed_page = ENTT_PACKED_PAGE;

    using comp_traits = component_traits<Type>;

    using underlying_type = basic_sparse_set<Entity, typename std::allocator_traits<Allocator>::template rebind_alloc<Entity>>;
    using difference_type = typename entt_traits<Entity>::difference_type;

    using alloc_traits = typename std::allocator_traits<Allocator>::template rebind_traits<Type>;
    using alloc_pointer = typename alloc_traits::pointer;
    using alloc_const_pointer = typename alloc_traits::const_pointer;

    using bucket_alloc_traits = typename std::allocator_traits<Allocator>::template rebind_traits<alloc_pointer>;
    using bucket_alloc_pointer = typename bucket_alloc_traits::pointer;

    using bucket_alloc_const_type = typename std::allocator_traits<Allocator>::template rebind_alloc<alloc_const_pointer>;
    using bucket_alloc_const_pointer = typename std::allocator_traits<bucket_alloc_const_type>::const_pointer;

    static_assert(alloc_traits::propagate_on_container_move_assignment::value);
    static_assert(bucket_alloc_traits::propagate_on_container_move_assignment::value);

    template<typename Value>
    struct storage_iterator final {
        using difference_type = typename basic_storage_impl::difference_type;
        using value_type = Value;
        using pointer = value_type *;
        using reference = value_type &;
        using iterator_category = std::random_access_iterator_tag;

        storage_iterator() ENTT_NOEXCEPT = default;

        storage_iterator(bucket_alloc_pointer const *ref, const typename basic_storage_impl::difference_type idx) ENTT_NOEXCEPT
            : packed{ref},
              index{idx}
        {}

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
            // no-throw stable erase iteration
            underlying_type::clear();

            for(size_type pos{}; pos < bucket; ++pos) {
                alloc_traits::deallocate(allocator, packed[pos], packed_page);
                bucket_alloc_traits::destroy(bucket_allocator, std::addressof(packed[pos]));
            }

            bucket_alloc_traits::deallocate(bucket_allocator, packed, bucket);
        }
    }

    void assure_at_least(const std::size_t last) {
        if(const auto idx = page(last - 1u); !(idx < bucket)) {
            const size_type sz = idx + 1u;
            const auto mem = bucket_alloc_traits::allocate(bucket_allocator, sz);
            std::uninitialized_copy(packed, packed + bucket, mem);
            size_type pos{};

            ENTT_TRY {
                for(pos = bucket; pos < sz; ++pos) {
                    auto pg = alloc_traits::allocate(allocator, packed_page);
                    bucket_alloc_traits::construct(bucket_allocator, std::addressof(mem[pos]), pg);
                }
            } ENTT_CATCH {
                for(auto next = bucket; next < pos; ++next) {
                    alloc_traits::deallocate(allocator, mem[next], packed_page);
                }

                std::destroy(mem, mem + pos);
                bucket_alloc_traits::deallocate(bucket_allocator, mem, sz);
                ENTT_THROW;
            }

            std::destroy(packed, packed + bucket);
            bucket_alloc_traits::deallocate(bucket_allocator, packed, bucket);

            packed = mem;
            bucket = sz;
        }
    }

    void release_unused_pages() {
        if(const auto length = underlying_type::size() / packed_page; length < bucket) {
            const auto mem = bucket_alloc_traits::allocate(bucket_allocator, length);
            std::uninitialized_copy(packed, packed + length, mem);

            for(auto pos = length; pos < bucket; ++pos) {
                alloc_traits::deallocate(allocator, packed[pos], packed_page);
                bucket_alloc_traits::destroy(bucket_allocator, std::addressof(packed[pos]));
            }

            bucket_alloc_traits::deallocate(bucket_allocator, packed, bucket);

            packed = mem;
            bucket = length;
        }
    }

    template<typename... Args>
    auto & push_at(const std::size_t pos, Args &&... args) {
        ENTT_ASSERT(pos < (bucket * packed_page), "Out of bounds index");
        auto *instance = std::addressof(packed[page(pos)][offset(pos)]);

        if constexpr(std::is_aggregate_v<value_type>) {
            alloc_traits::construct(allocator, instance, Type{std::forward<Args>(args)...});
        } else {
            alloc_traits::construct(allocator, instance, std::forward<Args>(args)...);
        }

        return *instance;
    }

    void pop_at(const std::size_t pos) {
        alloc_traits::destroy(allocator, std::addressof(packed[page(pos)][offset(pos)]));
    }

protected:
    /*! @copydoc basic_sparse_set::swap_at */
    void swap_at(const std::size_t lhs, const std::size_t rhs) final {
        std::swap(packed[page(lhs)][offset(lhs)], packed[page(rhs)][offset(rhs)]);
    }

    /*! @copydoc basic_sparse_set::move_and_pop */
    void move_and_pop(const std::size_t from, const std::size_t to) final {
        push_at(to, std::move(packed[page(from)][offset(from)]));
        pop_at(from);
    }

    /*! @copydoc basic_sparse_set::swap_and_pop */
    void swap_and_pop(const Entity entt, void *ud) override {
        const auto pos = underlying_type::index(entt);
        const auto last = underlying_type::size() - 1u;
        auto &&elem = packed[page(pos)][offset(pos)];

        // support for nosy destructors
        [[maybe_unused]] auto unused = std::move(elem);
        elem = std::move(packed[page(last)][offset(last)]);
        pop_at(last);

        underlying_type::swap_and_pop(entt, ud);
    }

    /*! @copydoc basic_sparse_set::in_place_pop */
    void in_place_pop(const Entity entt, void *ud) override {
        const auto pos = underlying_type::index(entt);
        underlying_type::in_place_pop(entt, ud);
        // support for nosy destructors
        pop_at(pos);
    }

public:
    /*! @brief Allocator type. */
    using allocator_type = typename alloc_traits::allocator_type;
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
    using iterator = storage_iterator<value_type>;
    /*! @brief Constant random access iterator type. */
    using const_iterator = storage_iterator<const value_type>;
    /*! @brief Reverse iterator type. */
    using reverse_iterator = std::reverse_iterator<iterator>;
    /*! @brief Constant reverse iterator type. */
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;

    /**
     * @brief Default constructor.
     * @param alloc Allocator to use (possibly default-constructed).
     */
    explicit basic_storage_impl(const allocator_type &alloc = {})
        : underlying_type{deletion_policy{comp_traits::in_place_delete::value}, alloc},
          allocator{alloc},
          bucket_allocator{alloc},
          packed{bucket_alloc_traits::allocate(bucket_allocator, 0u)},
          bucket{}
    {}

    /**
     * @brief Move constructor.
     * @param other The instance to move from.
     */
    basic_storage_impl(basic_storage_impl &&other) ENTT_NOEXCEPT
        : underlying_type{std::move(other)},
          allocator{std::move(other.allocator)},
          bucket_allocator{std::move(other.bucket_allocator)},
          packed{std::exchange(other.packed, bucket_alloc_pointer{})},
          bucket{std::exchange(other.bucket, 0u)}
    {}

    /*! @brief Default destructor. */
    ~basic_storage_impl() override {
        release_memory();
    }

    /**
     * @brief Move assignment operator.
     * @param other The instance to move from.
     * @return This sparse set.
     */
    basic_storage_impl & operator=(basic_storage_impl &&other) ENTT_NOEXCEPT {
        release_memory();

        underlying_type::operator=(std::move(other));

        allocator = std::move(other.allocator);
        bucket_allocator = std::move(other.bucket_allocator);
        packed = std::exchange(other.packed, bucket_alloc_pointer{});
        bucket = std::exchange(other.bucket, 0u);

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

        if(cap > underlying_type::size()) {
            assure_at_least(cap);
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
        release_unused_pages();
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
        const difference_type pos = underlying_type::size();
        return const_iterator{std::addressof(packed), pos};
    }

    /*! @copydoc cbegin */
    [[nodiscard]] const_iterator begin() const ENTT_NOEXCEPT {
        return cbegin();
    }

    /*! @copydoc begin */
    [[nodiscard]] iterator begin() ENTT_NOEXCEPT {
        const difference_type pos = underlying_type::size();
        return iterator{std::addressof(packed), pos};
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
        return const_iterator{std::addressof(packed), {}};
    }

    /*! @copydoc cend */
    [[nodiscard]] const_iterator end() const ENTT_NOEXCEPT {
        return cend();
    }

    /*! @copydoc end */
    [[nodiscard]] iterator end() ENTT_NOEXCEPT {
        return iterator{std::addressof(packed), {}};
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
    [[nodiscard]] const value_type & get(const entity_type entt) const ENTT_NOEXCEPT {
        const auto idx = underlying_type::index(entt);
        return packed[page(idx)][offset(idx)];
    }

    /*! @copydoc get */
    [[nodiscard]] value_type & get(const entity_type entt) ENTT_NOEXCEPT {
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
        const auto pos = underlying_type::slot();
        assure_at_least(pos + 1u);

        auto &value = push_at(pos, std::forward<Args>(args)...);

        ENTT_TRY {
            [[maybe_unused]] const auto curr = underlying_type::emplace(entt);
            ENTT_ASSERT(pos == curr, "Misplaced component");
        } ENTT_CATCH {
            pop_at(pos);
            ENTT_THROW;
        }

        return value;
    }

    /**
     * @brief Updates the instance assigned to a given entity in-place.
     * @tparam Func Types of the function objects to invoke.
     * @param entt A valid entity identifier.
     * @param func Valid function objects.
     * @return A reference to the updated instance.
     */
    template<typename... Func>
    decltype(auto) patch(const entity_type entt, Func &&... func) {
        const auto idx = underlying_type::index(entt);
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
        const auto cap = underlying_type::size() + std::distance(first, last);
        underlying_type::reserve(cap);
        assure_at_least(cap);

        for(; first != last; ++first) {
            push_at(underlying_type::size(), value);

            ENTT_TRY {
                underlying_type::emplace_back(*first);
            } ENTT_CATCH {
                pop_at(underlying_type::size());
                ENTT_THROW;
            }
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
        const auto cap = underlying_type::size() + std::distance(first, last);
        underlying_type::reserve(cap);
        assure_at_least(cap);

        for(; first != last; ++first, ++from) {
            push_at(underlying_type::size(), *from);

            ENTT_TRY {
                underlying_type::emplace_back(*first);
            } ENTT_CATCH {
                pop_at(underlying_type::size());
                ENTT_THROW;
            }
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
    typename alloc_traits::allocator_type allocator;
    typename bucket_alloc_traits::allocator_type bucket_allocator;
    bucket_alloc_pointer packed;
    size_type bucket;
};


/*! @copydoc basic_storage_impl */
template<typename Entity, typename Type, typename Allocator>
class basic_storage_impl<Entity, Type, Allocator, std::enable_if_t<component_traits<Type>::ignore_if_empty::value && std::is_empty_v<Type>>>
    : public basic_sparse_set<Entity, typename std::allocator_traits<Allocator>::template rebind_alloc<Entity>>
{
    using comp_traits = component_traits<Type>;
    using underlying_type = basic_sparse_set<Entity, typename std::allocator_traits<Allocator>::template rebind_alloc<Entity>>;
    using alloc_traits = typename std::allocator_traits<Allocator>::template rebind_traits<Type>;

public:
    /*! @brief Allocator type. */
    using allocator_type = typename alloc_traits::allocator_type;
    /*! @brief Type of the objects assigned to entities. */
    using value_type = Type;
    /*! @brief Underlying entity identifier. */
    using entity_type = Entity;
    /*! @brief Unsigned integer type. */
    using size_type = std::size_t;

    /**
     * @brief Default constructor.
     * @param alloc Allocator to use (possibly default-constructed).
     */
    explicit basic_storage_impl(const allocator_type &alloc = {})
        : underlying_type{deletion_policy{comp_traits::in_place_delete::value}, alloc}
    {}

    /**
     * @brief Fake get function.
     *
     * @warning
     * Attempting to use an entity that doesn't belong to the storage results in
     * undefined behavior.
     *
     * @param entt A valid entity identifier.
     */
    void get([[maybe_unused]] const entity_type entt) const ENTT_NOEXCEPT {
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
    * @param entt A valid entity identifier.
    * @param func Valid function objects.
    */
    template<typename... Func>
    void patch([[maybe_unused]] const entity_type entt, Func &&... func) {
        ENTT_ASSERT(underlying_type::contains(entt), "Storage does not contain entity");
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
     * @param entt A valid entity identifier.
     * @param args Parameters to use to initialize the object.
     * @return A reference to the newly created object.
     */
    template<typename... Args>
    decltype(auto) emplace(basic_registry<entity_type> &, const entity_type entt, Args &&... args) {
        return Type::emplace(entt, std::forward<Args>(args)...);
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
     * @param entt A valid entity identifier.
     * @param func Valid function objects.
     * @return A reference to the patched instance.
     */
    template<typename... Func>
    decltype(auto) patch(basic_registry<entity_type> &, const entity_type entt, Func &&... func) {
        return Type::patch(entt, std::forward<Func>(func)...);
    }
};


/**
 * @brief Mixin type to use to add signal support to storage types.
 * @tparam Type The type of the underlying storage.
 */
template<typename Type>
class sigh_storage_mixin final: public Type {
    /*! @copydoc basic_sparse_set::swap_and_pop */
    void swap_and_pop(const typename Type::entity_type entt, void *ud) final {
        ENTT_ASSERT(ud != nullptr, "Invalid pointer to registry");
        destruction.publish(*static_cast<basic_registry<typename Type::entity_type> *>(ud), entt);
        Type::swap_and_pop(entt, ud);
    }

    /*! @copydoc basic_sparse_set::in_place_pop */
    void in_place_pop(const typename Type::entity_type entt, void *ud) final {
        ENTT_ASSERT(ud != nullptr, "Invalid pointer to registry");
        destruction.publish(*static_cast<basic_registry<typename Type::entity_type> *>(ud), entt);
        Type::in_place_pop(entt, ud);
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
     * @param entt A valid entity identifier.
     * @param args Parameters to use to initialize the object.
     * @return A reference to the newly created object.
     */
    template<typename... Args>
    decltype(auto) emplace(basic_registry<entity_type> &owner, const entity_type entt, Args &&... args) {
        Type::emplace(entt, std::forward<Args>(args)...);
        construction.publish(owner, entt);
        return this->get(entt);
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
     * @param entt A valid entity identifier.
     * @param func Valid function objects.
     * @return A reference to the patched instance.
     */
    template<typename... Func>
    decltype(auto) patch(basic_registry<entity_type> &owner, const entity_type entt, Func &&... func) {
        Type::patch(entt, std::forward<Func>(func)...);
        update.publish(owner, entt);
        return this->get(entt);
    }

private:
    sigh<void(basic_registry<entity_type> &, const entity_type)> construction{};
    sigh<void(basic_registry<entity_type> &, const entity_type)> destruction{};
    sigh<void(basic_registry<entity_type> &, const entity_type)> update{};
};


/**
 * @brief Storage implementation dispatcher.
 * @tparam Entity A valid entity type (see entt_traits for more details).
 * @tparam Type Type of objects assigned to the entities.
 * @tparam Allocator Type of allocator used to manage memory and elements.
 */
template<typename Entity, typename Type, typename Allocator>
struct basic_storage: basic_storage_impl<Entity, Type, Allocator> {
    using basic_storage_impl<Entity, Type, Allocator>::basic_storage_impl;
};


/**
 * @brief Provides a common way to access certain properties of storage types.
 * @tparam Entity A valid entity type (see entt_traits for more details).
 * @tparam Type Type of objects managed by the storage class.
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
 * @param entt A valid entity identifier.
 * @return A possibly empty tuple containing the requested element.
 */
template<typename Type>
[[nodiscard]] auto get_as_tuple([[maybe_unused]] Type &container, [[maybe_unused]] const typename Type::entity_type entt) {
    static_assert(std::is_same_v<std::remove_const_t<Type>, typename storage_traits<typename Type::entity_type, typename Type::value_type>::storage_type>, "Invalid storage");

    if constexpr(std::is_void_v<decltype(container.get({}))>) {
        return std::make_tuple();
    } else {
        return std::forward_as_tuple(container.get(entt));
    }
}


}


#endif

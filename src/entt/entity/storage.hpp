#ifndef ENTT_ENTITY_STORAGE_HPP
#define ENTT_ENTITY_STORAGE_HPP

#include <cstddef>
#include <iterator>
#include <memory>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>
#include "../config/config.h"
#include "../core/algorithm.hpp"
#include "../core/any.hpp"
#include "../core/compressed_pair.hpp"
#include "../core/iterator.hpp"
#include "../core/memory.hpp"
#include "../core/type_info.hpp"
#include "../core/type_traits.hpp"
#include "../signal/sigh.hpp"
#include "component.hpp"
#include "entity.hpp"
#include "fwd.hpp"
#include "sparse_set.hpp"

namespace entt {

/**
 * @cond TURN_OFF_DOXYGEN
 * Internal details not to be documented.
 */

namespace internal {

template<typename Container>
class storage_iterator final {
    friend storage_iterator<const Container>;

    using container_type = std::remove_const_t<Container>;
    using allocator_traits = std::allocator_traits<typename container_type::allocator_type>;
    using comp_traits = component_traits<typename container_type::value_type>;

    using iterator_traits = std::iterator_traits<std::conditional_t<
        std::is_const_v<Container>,
        typename allocator_traits::template rebind_traits<typename std::pointer_traits<typename container_type::value_type>::element_type>::const_pointer,
        typename allocator_traits::template rebind_traits<typename std::pointer_traits<typename container_type::value_type>::element_type>::pointer>>;

public:
    using difference_type = typename iterator_traits::difference_type;
    using value_type = typename iterator_traits::value_type;
    using pointer = typename iterator_traits::pointer;
    using reference = typename iterator_traits::reference;
    using iterator_category = std::random_access_iterator_tag;

    storage_iterator() ENTT_NOEXCEPT = default;

    storage_iterator(Container *ref, difference_type idx) ENTT_NOEXCEPT
        : packed{ref},
          offset{idx} {}

    template<bool Const = std::is_const_v<Container>, typename = std::enable_if_t<Const>>
    storage_iterator(const storage_iterator<std::remove_const_t<Container>> &other) ENTT_NOEXCEPT
        : packed{other.packed},
          offset{other.offset} {}

    storage_iterator &operator++() ENTT_NOEXCEPT {
        return --offset, *this;
    }

    storage_iterator operator++(int) ENTT_NOEXCEPT {
        storage_iterator orig = *this;
        return ++(*this), orig;
    }

    storage_iterator &operator--() ENTT_NOEXCEPT {
        return ++offset, *this;
    }

    storage_iterator operator--(int) ENTT_NOEXCEPT {
        storage_iterator orig = *this;
        return operator--(), orig;
    }

    storage_iterator &operator+=(const difference_type value) ENTT_NOEXCEPT {
        offset -= value;
        return *this;
    }

    storage_iterator operator+(const difference_type value) const ENTT_NOEXCEPT {
        storage_iterator copy = *this;
        return (copy += value);
    }

    storage_iterator &operator-=(const difference_type value) ENTT_NOEXCEPT {
        return (*this += -value);
    }

    storage_iterator operator-(const difference_type value) const ENTT_NOEXCEPT {
        return (*this + -value);
    }

    [[nodiscard]] reference operator[](const difference_type value) const ENTT_NOEXCEPT {
        const auto pos = offset - value - 1;
        return (*packed)[pos / comp_traits::page_size][fast_mod(pos, comp_traits::page_size)];
    }

    [[nodiscard]] pointer operator->() const ENTT_NOEXCEPT {
        const auto pos = offset - 1;
        return (*packed)[pos / comp_traits::page_size] + fast_mod(pos, comp_traits::page_size);
    }

    [[nodiscard]] reference operator*() const ENTT_NOEXCEPT {
        return *operator->();
    }

    [[nodiscard]] difference_type index() const ENTT_NOEXCEPT {
        return offset;
    }

private:
    Container *packed;
    difference_type offset;
};

template<typename CLhs, typename CRhs>
[[nodiscard]] auto operator-(const storage_iterator<CLhs> &lhs, const storage_iterator<CRhs> &rhs) ENTT_NOEXCEPT {
    return rhs.index() - lhs.index();
}

template<typename CLhs, typename CRhs>
[[nodiscard]] bool operator==(const storage_iterator<CLhs> &lhs, const storage_iterator<CRhs> &rhs) ENTT_NOEXCEPT {
    return lhs.index() == rhs.index();
}

template<typename CLhs, typename CRhs>
[[nodiscard]] bool operator!=(const storage_iterator<CLhs> &lhs, const storage_iterator<CRhs> &rhs) ENTT_NOEXCEPT {
    return !(lhs == rhs);
}

template<typename CLhs, typename CRhs>
[[nodiscard]] bool operator<(const storage_iterator<CLhs> &lhs, const storage_iterator<CRhs> &rhs) ENTT_NOEXCEPT {
    return lhs.index() > rhs.index();
}

template<typename CLhs, typename CRhs>
[[nodiscard]] bool operator>(const storage_iterator<CLhs> &lhs, const storage_iterator<CRhs> &rhs) ENTT_NOEXCEPT {
    return lhs.index() < rhs.index();
}

template<typename CLhs, typename CRhs>
[[nodiscard]] bool operator<=(const storage_iterator<CLhs> &lhs, const storage_iterator<CRhs> &rhs) ENTT_NOEXCEPT {
    return !(lhs > rhs);
}

template<typename CLhs, typename CRhs>
[[nodiscard]] bool operator>=(const storage_iterator<CLhs> &lhs, const storage_iterator<CRhs> &rhs) ENTT_NOEXCEPT {
    return !(lhs < rhs);
}

template<typename It, typename... Other>
class extended_storage_iterator final {
    template<typename Iter, typename... Args>
    friend class extended_storage_iterator;

public:
    using difference_type = typename std::iterator_traits<It>::difference_type;
    using value_type = decltype(std::tuple_cat(std::make_tuple(*std::declval<It>()), std::forward_as_tuple(*std::declval<Other>()...)));
    using pointer = input_iterator_pointer<value_type>;
    using reference = value_type;
    using iterator_category = std::input_iterator_tag;

    extended_storage_iterator() = default;

    extended_storage_iterator(It base, Other... other) ENTT_NOEXCEPT
        : it{base, other...} {}

    template<typename... Args, typename = std::enable_if_t<(!std::is_same_v<Other, Args> && ...) && (std::is_constructible_v<Other, Args> && ...)>>
    extended_storage_iterator(const extended_storage_iterator<It, Args...> &other) ENTT_NOEXCEPT
        : it{other.it} {}

    extended_storage_iterator &operator++() ENTT_NOEXCEPT {
        return ++std::get<It>(it), (++std::get<Other>(it), ...), *this;
    }

    extended_storage_iterator operator++(int) ENTT_NOEXCEPT {
        extended_storage_iterator orig = *this;
        return ++(*this), orig;
    }

    [[nodiscard]] pointer operator->() const ENTT_NOEXCEPT {
        return operator*();
    }

    [[nodiscard]] reference operator*() const ENTT_NOEXCEPT {
        return {*std::get<It>(it), *std::get<Other>(it)...};
    }

    template<typename... CLhs, typename... CRhs>
    friend bool operator==(const extended_storage_iterator<CLhs...> &, const extended_storage_iterator<CRhs...> &) ENTT_NOEXCEPT;

private:
    std::tuple<It, Other...> it;
};

template<typename... CLhs, typename... CRhs>
[[nodiscard]] bool operator==(const extended_storage_iterator<CLhs...> &lhs, const extended_storage_iterator<CRhs...> &rhs) ENTT_NOEXCEPT {
    return std::get<0>(lhs.it) == std::get<0>(rhs.it);
}

template<typename... CLhs, typename... CRhs>
[[nodiscard]] bool operator!=(const extended_storage_iterator<CLhs...> &lhs, const extended_storage_iterator<CRhs...> &rhs) ENTT_NOEXCEPT {
    return !(lhs == rhs);
}

} // namespace internal

/**
 * Internal details not to be documented.
 * @endcond
 */

/**
 * @brief Basic storage implementation.
 *
 * Internal data structures arrange elements to maximize performance. There are
 * no guarantees that objects are returned in the insertion order when iterate
 * a storage. Do not make assumption on the order in any case.
 *
 * @warning
 * Empty types aren't explicitly instantiated. Therefore, many of the functions
 * normally available for non-empty types will not be available for empty ones.
 *
 * @tparam Entity A valid entity type (see entt_traits for more details).
 * @tparam Type Type of objects assigned to the entities.
 * @tparam Allocator Type of allocator used to manage memory and elements.
 */
template<typename Entity, typename Type, typename Allocator, typename>
class basic_storage: public basic_sparse_set<Entity, typename std::allocator_traits<Allocator>::template rebind_alloc<Entity>> {
    using allocator_traits = std::allocator_traits<Allocator>;
    using alloc = typename allocator_traits::template rebind_alloc<Type>;
    using alloc_traits = typename std::allocator_traits<alloc>;

    using comp_traits = component_traits<Type>;
    using underlying_type = basic_sparse_set<Entity, typename allocator_traits::template rebind_alloc<Entity>>;
    using container_type = std::vector<typename alloc_traits::pointer, typename alloc_traits::template rebind_alloc<typename alloc_traits::pointer>>;

    [[nodiscard]] auto &element_at(const std::size_t pos) const {
        return packed.first()[pos / comp_traits::page_size][fast_mod(pos, comp_traits::page_size)];
    }

    auto assure_at_least(const std::size_t pos) {
        auto &&container = packed.first();
        const auto idx = pos / comp_traits::page_size;

        if(!(idx < container.size())) {
            auto curr = container.size();
            container.resize(idx + 1u, nullptr);

            ENTT_TRY {
                for(const auto last = container.size(); curr < last; ++curr) {
                    container[curr] = alloc_traits::allocate(packed.second(), comp_traits::page_size);
                }
            }
            ENTT_CATCH {
                container.resize(curr);
                ENTT_THROW;
            }
        }

        return container[idx] + fast_mod(pos, comp_traits::page_size);
    }

    void release_unused_pages() {
        auto &&container = packed.first();
        auto page_allocator{packed.second()};
        const auto in_use = (base_type::size() + comp_traits::page_size - 1u) / comp_traits::page_size;

        for(auto pos = in_use, last = container.size(); pos < last; ++pos) {
            alloc_traits::deallocate(page_allocator, container[pos], comp_traits::page_size);
        }

        container.resize(in_use);
    }

    void release_all_pages() {
        for(size_type pos{}, last = base_type::size(); pos < last; ++pos) {
            if constexpr(comp_traits::in_place_delete) {
                if(base_type::at(pos) != tombstone) {
                    std::destroy_at(std::addressof(element_at(pos)));
                }
            } else {
                std::destroy_at(std::addressof(element_at(pos)));
            }
        }

        auto &&container = packed.first();
        auto page_allocator{packed.second()};

        for(size_type pos{}, last = container.size(); pos < last; ++pos) {
            alloc_traits::deallocate(page_allocator, container[pos], comp_traits::page_size);
        }
    }

    template<typename... Args>
    void construct(typename alloc_traits::pointer ptr, Args &&...args) {
        if constexpr(std::is_aggregate_v<value_type>) {
            alloc_traits::construct(packed.second(), to_address(ptr), Type{std::forward<Args>(args)...});
        } else {
            alloc_traits::construct(packed.second(), to_address(ptr), std::forward<Args>(args)...);
        }
    }

    template<typename It, typename Generator>
    void consume_range(It first, It last, Generator generator) {
        for(const auto sz = base_type::size(); first != last && base_type::slot() != sz; ++first) {
            emplace(*first, generator());
        }

        const auto req = base_type::size() + std::distance(first, last);
        base_type::reserve(req);
        reserve(req);

        for(; first != last; ++first) {
            emplace(*first, generator());
        }
    }

protected:
    /**
     * @brief Returns the element assigned to an entity.
     * @param pos A valid position of an element within a storage.
     * @return An opaque pointer to the element assigned to the entity.
     */
    const void *get_at(const std::size_t pos) const ENTT_NOEXCEPT override {
        return std::addressof(element_at(pos));
    }

    /**
     * @brief Swaps two elements in a storage.
     * @param lhs A valid position of an element within a storage.
     * @param rhs A valid position of an element within a storage.
     */
    void swap_at(const std::size_t lhs, const std::size_t rhs) final {
        std::swap(element_at(lhs), element_at(rhs));
    }

    /**
     * @brief Moves an element within a storage.
     * @param from A valid position of an element within a storage.
     * @param to A valid position of an element within a storage.
     */
    void move_and_pop(const std::size_t from, const std::size_t to) final {
        auto &elem = element_at(from);
        construct(assure_at_least(to), std::move(elem));
        std::destroy_at(std::addressof(elem));
    }

    /**
     * @brief Erase an element from a storage.
     * @param entt A valid identifier.
     */
    void swap_and_pop(const Entity entt) override {
        const auto pos = base_type::index(entt);
        const auto last = base_type::size() - 1u;

        auto &target = element_at(pos);
        auto &elem = element_at(last);

        // support for nosy destructors
        [[maybe_unused]] auto unused = std::move(target);
        target = std::move(elem);
        std::destroy_at(std::addressof(elem));

        base_type::swap_and_pop(entt);
    }

    /**
     * @brief Erases an element from a storage.
     * @param entt A valid identifier.
     */
    void in_place_pop(const Entity entt) override {
        const auto pos = base_type::index(entt);
        base_type::in_place_pop(entt);
        // support for nosy destructors
        std::destroy_at(std::addressof(element_at(pos)));
    }

    /**
     * @brief Assigns an entity to a storage.
     * @param entt A valid identifier.
     * @param value Optional opaque value.
     */
    void try_emplace([[maybe_unused]] const Entity entt, const void *value) override {
        if(value) {
            if constexpr(std::is_copy_constructible_v<value_type>) {
                emplace(entt, *static_cast<const value_type *>(value));
            }
        } else {
            if constexpr(std::is_default_constructible_v<value_type>) {
                emplace(entt);
            }
        }
    }

public:
    /*! @brief Base type. */
    using base_type = underlying_type;
    /*! @brief Allocator type. */
    using allocator_type = Allocator;
    /*! @brief Type of the objects assigned to entities. */
    using value_type = Type;
    /*! @brief Underlying entity identifier. */
    using entity_type = Entity;
    /*! @brief Unsigned integer type. */
    using size_type = std::size_t;
    /*! @brief Pointer type to contained elements. */
    using pointer = typename container_type::pointer;
    /*! @brief Constant pointer type to contained elements. */
    using const_pointer = typename alloc_traits::template rebind_traits<typename alloc_traits::const_pointer>::const_pointer;
    /*! @brief Random access iterator type. */
    using iterator = internal::storage_iterator<container_type>;
    /*! @brief Constant random access iterator type. */
    using const_iterator = internal::storage_iterator<const container_type>;
    /*! @brief Reverse iterator type. */
    using reverse_iterator = std::reverse_iterator<iterator>;
    /*! @brief Constant reverse iterator type. */
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;
    /*! @brief Extended iterable storage proxy. */
    using iterable = iterable_adaptor<internal::extended_storage_iterator<typename base_type::iterator, iterator>>;
    /*! @brief Constant extended iterable storage proxy. */
    using const_iterable = iterable_adaptor<internal::extended_storage_iterator<typename base_type::const_iterator, const_iterator>>;

    /*! @brief Default constructor. */
    basic_storage()
        : basic_storage{allocator_type{}} {}

    /**
     * @brief Constructs an empty storage with a given allocator.
     * @param allocator The allocator to use.
     */
    explicit basic_storage(const allocator_type &allocator)
        : base_type{type_id<value_type>(), deletion_policy{comp_traits::in_place_delete}, allocator},
          packed{container_type{allocator}, allocator} {}

    /**
     * @brief Move constructor.
     * @param other The instance to move from.
     */
    basic_storage(basic_storage &&other) ENTT_NOEXCEPT
        : base_type{std::move(other)},
          packed{std::move(other.packed)} {}

    /**
     * @brief Allocator-extended move constructor.
     * @param other The instance to move from.
     * @param allocator The allocator to use.
     */
    basic_storage(basic_storage &&other, const allocator_type &allocator) ENTT_NOEXCEPT
        : base_type{std::move(other), allocator},
          packed{container_type{std::move(other.packed.first()), allocator}, allocator} {
        ENTT_ASSERT(alloc_traits::is_always_equal::value || packed.second() == other.packed.second(), "Copying a storage is not allowed");
    }

    /*! @brief Default destructor. */
    ~basic_storage() override {
        release_all_pages();
    }

    /**
     * @brief Move assignment operator.
     * @param other The instance to move from.
     * @return This storage.
     */
    basic_storage &operator=(basic_storage &&other) ENTT_NOEXCEPT {
        ENTT_ASSERT(alloc_traits::is_always_equal::value || packed.second() == other.packed.second(), "Copying a sparse set is not allowed");

        release_all_pages();
        base_type::operator=(std::move(other));
        packed.first() = std::move(other.packed.first());
        propagate_on_container_move_assignment(packed.second(), other.packed.second());
        return *this;
    }

    /**
     * @brief Exchanges the contents with those of a given storage.
     * @param other Storage to exchange the content with.
     */
    void swap(basic_storage &other) {
        using std::swap;
        underlying_type::swap(other);
        propagate_on_container_swap(packed.second(), other.packed.second());
        swap(packed.first(), other.packed.first());
    }

    /**
     * @brief Returns the associated allocator.
     * @return The associated allocator.
     */
    [[nodiscard]] constexpr allocator_type get_allocator() const ENTT_NOEXCEPT {
        return allocator_type{packed.second()};
    }

    /**
     * @brief Increases the capacity of a storage.
     *
     * If the new capacity is greater than the current capacity, new storage is
     * allocated, otherwise the method does nothing.
     *
     * @param cap Desired capacity.
     */
    void reserve(const size_type cap) override {
        base_type::reserve(cap);

        if(cap > base_type::size()) {
            assure_at_least(cap - 1u);
        }
    }

    /**
     * @brief Returns the number of elements that a storage has currently
     * allocated space for.
     * @return Capacity of the storage.
     */
    [[nodiscard]] size_type capacity() const ENTT_NOEXCEPT override {
        return packed.first().size() * comp_traits::page_size;
    }

    /*! @brief Requests the removal of unused capacity. */
    void shrink_to_fit() override {
        base_type::shrink_to_fit();
        release_unused_pages();
    }

    /**
     * @brief Direct access to the array of objects.
     * @return A pointer to the array of objects.
     */
    [[nodiscard]] const_pointer raw() const ENTT_NOEXCEPT {
        return packed.first().data();
    }

    /*! @copydoc raw */
    [[nodiscard]] pointer raw() ENTT_NOEXCEPT {
        return packed.first().data();
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
        const auto pos = static_cast<typename iterator::difference_type>(base_type::size());
        return const_iterator{&packed.first(), pos};
    }

    /*! @copydoc cbegin */
    [[nodiscard]] const_iterator begin() const ENTT_NOEXCEPT {
        return cbegin();
    }

    /*! @copydoc begin */
    [[nodiscard]] iterator begin() ENTT_NOEXCEPT {
        const auto pos = static_cast<typename iterator::difference_type>(base_type::size());
        return iterator{&packed.first(), pos};
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
        return const_iterator{&packed.first(), {}};
    }

    /*! @copydoc cend */
    [[nodiscard]] const_iterator end() const ENTT_NOEXCEPT {
        return cend();
    }

    /*! @copydoc end */
    [[nodiscard]] iterator end() ENTT_NOEXCEPT {
        return iterator{&packed.first(), {}};
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
     * @param entt A valid identifier.
     * @return The object assigned to the entity.
     */
    [[nodiscard]] const value_type &get(const entity_type entt) const ENTT_NOEXCEPT {
        return element_at(base_type::index(entt));
    }

    /*! @copydoc get */
    [[nodiscard]] value_type &get(const entity_type entt) ENTT_NOEXCEPT {
        return const_cast<value_type &>(std::as_const(*this).get(entt));
    }

    /**
     * @brief Returns the object assigned to an entity as a tuple.
     *
     * @sa get
     *
     * @param entt A valid identifier.
     * @return The object assigned to the entity as a tuple.
     */
    [[nodiscard]] std::tuple<const value_type &> get_as_tuple(const entity_type entt) const ENTT_NOEXCEPT {
        return std::forward_as_tuple(get(entt));
    }

    /*! @copydoc get_as_tuple */
    [[nodiscard]] std::tuple<value_type &> get_as_tuple(const entity_type entt) ENTT_NOEXCEPT {
        return std::forward_as_tuple(get(entt));
    }

    /**
     * @brief Assigns an entity to a storage and constructs its object.
     *
     * @warning
     * Attempting to use an entity that already belongs to the storage results
     * in undefined behavior.
     *
     * @tparam Args Types of arguments to use to construct the object.
     * @param entt A valid identifier.
     * @param args Parameters to use to construct an object for the entity.
     * @return A reference to the newly created object.
     */
    template<typename... Args>
    value_type &emplace(const entity_type entt, Args &&...args) {
        const auto pos = base_type::slot();
        auto elem = assure_at_least(pos);
        construct(elem, std::forward<Args>(args)...);

        ENTT_TRY {
            base_type::try_emplace(entt);
            ENTT_ASSERT(pos == base_type::index(entt), "Misplaced component");
        }
        ENTT_CATCH {
            std::destroy_at(std::addressof(*elem));
            ENTT_THROW;
        }

        return *elem;
    }

    /**
     * @brief Updates the instance assigned to a given entity in-place.
     * @tparam Func Types of the function objects to invoke.
     * @param entt A valid identifier.
     * @param func Valid function objects.
     * @return A reference to the updated instance.
     */
    template<typename... Func>
    value_type &patch(const entity_type entt, Func &&...func) {
        const auto idx = base_type::index(entt);
        auto &elem = element_at(idx);
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
        consume_range(std::move(first), std::move(last), [&value]() -> decltype(auto) { return value; });
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
        consume_range(std::move(first), std::move(last), [&from]() -> decltype(auto) { return *(from++); });
    }

    /**
     * @brief Returns an iterable object to use to _visit_ a storage.
     *
     * The iterable object returns a tuple that contains the current entity and
     * a reference to its component.
     *
     * @return An iterable object to use to _visit_ the storage.
     */
    [[nodiscard]] iterable each() ENTT_NOEXCEPT {
        return {internal::extended_storage_iterator{base_type::begin(), begin()}, internal::extended_storage_iterator{base_type::end(), end()}};
    }

    /*! @copydoc each */
    [[nodiscard]] const_iterable each() const ENTT_NOEXCEPT {
        return {internal::extended_storage_iterator{base_type::cbegin(), cbegin()}, internal::extended_storage_iterator{base_type::cend(), cend()}};
    }

private:
    compressed_pair<container_type, alloc> packed;
};

/*! @copydoc basic_storage */
template<typename Entity, typename Type, typename Allocator>
class basic_storage<Entity, Type, Allocator, std::enable_if_t<ignore_as_empty_v<Type>>>
    : public basic_sparse_set<Entity, typename std::allocator_traits<Allocator>::template rebind_alloc<Entity>> {
    using allocator_traits = std::allocator_traits<Allocator>;
    using comp_traits = component_traits<Type>;

public:
    /*! @brief Base type. */
    using base_type = basic_sparse_set<Entity, typename allocator_traits::template rebind_alloc<Entity>>;
    /*! @brief Allocator type. */
    using allocator_type = Allocator;
    /*! @brief Type of the objects assigned to entities. */
    using value_type = Type;
    /*! @brief Underlying entity identifier. */
    using entity_type = Entity;
    /*! @brief Unsigned integer type. */
    using size_type = std::size_t;
    /*! @brief Extended iterable storage proxy. */
    using iterable = iterable_adaptor<internal::extended_storage_iterator<typename base_type::iterator>>;
    /*! @brief Constant extended iterable storage proxy. */
    using const_iterable = iterable_adaptor<internal::extended_storage_iterator<typename base_type::const_iterator>>;

    /*! @brief Default constructor. */
    basic_storage()
        : basic_storage{allocator_type{}} {}

    /**
     * @brief Constructs an empty container with a given allocator.
     * @param allocator The allocator to use.
     */
    explicit basic_storage(const allocator_type &allocator)
        : base_type{type_id<value_type>(), deletion_policy{comp_traits::in_place_delete}, allocator} {}

    /**
     * @brief Move constructor.
     * @param other The instance to move from.
     */
    basic_storage(basic_storage &&other) ENTT_NOEXCEPT = default;

    /**
     * @brief Allocator-extended move constructor.
     * @param other The instance to move from.
     * @param allocator The allocator to use.
     */
    basic_storage(basic_storage &&other, const allocator_type &allocator) ENTT_NOEXCEPT
        : base_type{std::move(other), allocator} {}

    /**
     * @brief Move assignment operator.
     * @param other The instance to move from.
     * @return This storage.
     */
    basic_storage &operator=(basic_storage &&other) ENTT_NOEXCEPT = default;

    /**
     * @brief Returns the associated allocator.
     * @return The associated allocator.
     */
    [[nodiscard]] constexpr allocator_type get_allocator() const ENTT_NOEXCEPT {
        return allocator_type{base_type::get_allocator()};
    }

    /**
     * @brief Returns the object assigned to an entity, that is `void`.
     *
     * @warning
     * Attempting to use an entity that doesn't belong to the storage results in
     * undefined behavior.
     *
     * @param entt A valid identifier.
     */
    void get([[maybe_unused]] const entity_type entt) const ENTT_NOEXCEPT {
        ENTT_ASSERT(base_type::contains(entt), "Storage does not contain entity");
    }

    /**
     * @brief Returns an empty tuple.
     *
     * @warning
     * Attempting to use an entity that doesn't belong to the storage results in
     * undefined behavior.
     *
     * @param entt A valid identifier.
     * @return Returns an empty tuple.
     */
    [[nodiscard]] std::tuple<> get_as_tuple([[maybe_unused]] const entity_type entt) const ENTT_NOEXCEPT {
        ENTT_ASSERT(base_type::contains(entt), "Storage does not contain entity");
        return std::tuple{};
    }

    /**
     * @brief Assigns an entity to a storage and constructs its object.
     *
     * @warning
     * Attempting to use an entity that already belongs to the storage results
     * in undefined behavior.
     *
     * @tparam Args Types of arguments to use to construct the object.
     * @param entt A valid identifier.
     * @param args Parameters to use to construct an object for the entity.
     */
    template<typename... Args>
    void emplace(const entity_type entt, Args &&...args) {
        [[maybe_unused]] const value_type elem{std::forward<Args>(args)...};
        base_type::try_emplace(entt);
    }

    /**
    * @brief Updates the instance assigned to a given entity in-place.
    * @tparam Func Types of the function objects to invoke.
    * @param entt A valid identifier.
    * @param func Valid function objects.
    */
    template<typename... Func>
    void patch([[maybe_unused]] const entity_type entt, Func &&...func) {
        ENTT_ASSERT(base_type::contains(entt), "Storage does not contain entity");
        (std::forward<Func>(func)(), ...);
    }

    /**
     * @brief Assigns entities to a storage.
     * @tparam It Type of input iterator.
     * @tparam Args Types of optional arguments.
     * @param first An iterator to the first element of the range of entities.
     * @param last An iterator past the last element of the range of entities.
     */
    template<typename It, typename... Args>
    void insert(It first, It last, Args &&...) {
        for(const auto sz = base_type::size(); first != last && base_type::slot() != sz; ++first) {
            emplace(*first);
        }

        base_type::reserve(base_type::size() + std::distance(first, last));

        for(; first != last; ++first) {
            emplace(*first);
        }
    }

    /**
     * @brief Returns an iterable object to use to _visit_ a storage.
     *
     * The iterable object returns a tuple that contains the current entity.
     *
     * @return An iterable object to use to _visit_ the storage.
     */
    [[nodiscard]] iterable each() ENTT_NOEXCEPT {
        return {internal::extended_storage_iterator{base_type::begin()}, internal::extended_storage_iterator{base_type::end()}};
    }

    /*! @copydoc each */
    [[nodiscard]] const_iterable each() const ENTT_NOEXCEPT {
        return {internal::extended_storage_iterator{base_type::cbegin()}, internal::extended_storage_iterator{base_type::cend()}};
    }
};

/**
 * @brief Mixin type used to add signal support to storage types.
 *
 * The function type of a listener is equivalent to:
 *
 * @code{.cpp}
 * void(basic_registry<entity_type> &, entity_type);
 * @endcode
 *
 * This applies to all signals made available.
 *
 * @tparam Type The type of the underlying storage.
 */
template<typename Type>
class sigh_storage_mixin final: public Type {
    /*! @copydoc basic_sparse_set::swap_and_pop */
    void swap_and_pop(const typename Type::entity_type entt) final {
        ENTT_ASSERT(owner != nullptr, "Invalid pointer to registry");
        destruction.publish(*owner, entt);
        Type::swap_and_pop(entt);
    }

    /*! @copydoc basic_sparse_set::in_place_pop */
    void in_place_pop(const typename Type::entity_type entt) final {
        ENTT_ASSERT(owner != nullptr, "Invalid pointer to registry");
        destruction.publish(*owner, entt);
        Type::in_place_pop(entt);
    }

    /*! @copydoc basic_sparse_set::try_emplace */
    void try_emplace(const typename Type::entity_type entt, const void *value) final {
        ENTT_ASSERT(owner != nullptr, "Invalid pointer to registry");
        Type::try_emplace(entt, value);
        construction.publish(*owner, entt);
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
     * Listeners are invoked after the object has been assigned to the entity.
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
     * Listeners are invoked after the object has been updated.
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
     * Listeners are invoked before the object has been removed from the entity.
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
     * @param entt A valid identifier.
     * @param args Parameters to use to initialize the object.
     * @return A reference to the newly created object.
     */
    template<typename... Args>
    decltype(auto) emplace(const entity_type entt, Args &&...args) {
        Type::emplace(entt, std::forward<Args>(args)...);
        construction.publish(*owner, entt);
        return this->get(entt);
    }

    /**
     * @brief Patches the given instance for an entity.
     * @tparam Func Types of the function objects to invoke.
     * @param entt A valid identifier.
     * @param func Valid function objects.
     * @return A reference to the patched instance.
     */
    template<typename... Func>
    decltype(auto) patch(const entity_type entt, Func &&...func) {
        Type::patch(entt, std::forward<Func>(func)...);
        update.publish(*owner, entt);
        return this->get(entt);
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
    void insert(It first, It last, Args &&...args) {
        Type::insert(first, last, std::forward<Args>(args)...);

        if(!construction.empty()) {
            for(; first != last; ++first) {
                construction.publish(*owner, *first);
            }
        }
    }

    /**
     * @brief Forwards variables to mixins, if any.
     * @param value A variable wrapped in an opaque container.
     */
    void bind(any value) ENTT_NOEXCEPT final {
        auto *reg = any_cast<basic_registry<entity_type>>(&value);
        owner = reg ? reg : owner;
        Type::bind(std::move(value));
    }

private:
    sigh<void(basic_registry<entity_type> &, const entity_type)> construction{};
    sigh<void(basic_registry<entity_type> &, const entity_type)> destruction{};
    sigh<void(basic_registry<entity_type> &, const entity_type)> update{};
    basic_registry<entity_type> *owner{};
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

} // namespace entt

#endif

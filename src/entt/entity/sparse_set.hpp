#ifndef ENTT_ENTITY_SPARSE_SET_HPP
#define ENTT_ENTITY_SPARSE_SET_HPP

#include <cstddef>
#include <iterator>
#include <memory>
#include <type_traits>
#include <utility>
#include <vector>
#include "../config/config.h"
#include "../core/algorithm.hpp"
#include "../core/any.hpp"
#include "../core/bit.hpp"
#include "../core/type_info.hpp"
#include "entity.hpp"
#include "fwd.hpp"

namespace entt {

/*! @cond TURN_OFF_DOXYGEN */
namespace internal {

template<typename Container>
struct sparse_set_iterator final {
    using value_type = typename Container::value_type;
    using pointer = typename Container::const_pointer;
    using reference = typename Container::const_reference;
    using difference_type = typename Container::difference_type;
    using iterator_category = std::random_access_iterator_tag;

    constexpr sparse_set_iterator() noexcept
        : packed{},
          offset{} {}

    constexpr sparse_set_iterator(const Container &ref, const difference_type idx) noexcept
        : packed{&ref},
          offset{idx} {}

    constexpr sparse_set_iterator &operator++() noexcept {
        return --offset, *this;
    }

    constexpr sparse_set_iterator operator++(int) noexcept {
        const sparse_set_iterator orig = *this;
        return ++(*this), orig;
    }

    constexpr sparse_set_iterator &operator--() noexcept {
        return ++offset, *this;
    }

    constexpr sparse_set_iterator operator--(int) noexcept {
        const sparse_set_iterator orig = *this;
        return operator--(), orig;
    }

    constexpr sparse_set_iterator &operator+=(const difference_type value) noexcept {
        offset -= value;
        return *this;
    }

    constexpr sparse_set_iterator operator+(const difference_type value) const noexcept {
        sparse_set_iterator copy = *this;
        return (copy += value);
    }

    constexpr sparse_set_iterator &operator-=(const difference_type value) noexcept {
        return (*this += -value);
    }

    constexpr sparse_set_iterator operator-(const difference_type value) const noexcept {
        return (*this + -value);
    }

    [[nodiscard]] constexpr reference operator[](const difference_type value) const noexcept {
        return (*packed)[static_cast<typename Container::size_type>(index() - value)];
    }

    [[nodiscard]] constexpr pointer operator->() const noexcept {
        return std::addressof(operator[](0));
    }

    [[nodiscard]] constexpr reference operator*() const noexcept {
        return operator[](0);
    }

    [[nodiscard]] constexpr pointer data() const noexcept {
        return packed ? packed->data() : nullptr;
    }

    [[nodiscard]] constexpr difference_type index() const noexcept {
        return offset - 1;
    }

private:
    const Container *packed;
    difference_type offset;
};

template<typename Container>
[[nodiscard]] constexpr std::ptrdiff_t operator-(const sparse_set_iterator<Container> &lhs, const sparse_set_iterator<Container> &rhs) noexcept {
    return rhs.index() - lhs.index();
}

template<typename Container>
[[nodiscard]] constexpr bool operator==(const sparse_set_iterator<Container> &lhs, const sparse_set_iterator<Container> &rhs) noexcept {
    return lhs.index() == rhs.index();
}

template<typename Container>
[[nodiscard]] constexpr bool operator!=(const sparse_set_iterator<Container> &lhs, const sparse_set_iterator<Container> &rhs) noexcept {
    return !(lhs == rhs);
}

template<typename Container>
[[nodiscard]] constexpr bool operator<(const sparse_set_iterator<Container> &lhs, const sparse_set_iterator<Container> &rhs) noexcept {
    return lhs.index() > rhs.index();
}

template<typename Container>
[[nodiscard]] constexpr bool operator>(const sparse_set_iterator<Container> &lhs, const sparse_set_iterator<Container> &rhs) noexcept {
    return rhs < lhs;
}

template<typename Container>
[[nodiscard]] constexpr bool operator<=(const sparse_set_iterator<Container> &lhs, const sparse_set_iterator<Container> &rhs) noexcept {
    return !(lhs > rhs);
}

template<typename Container>
[[nodiscard]] constexpr bool operator>=(const sparse_set_iterator<Container> &lhs, const sparse_set_iterator<Container> &rhs) noexcept {
    return !(lhs < rhs);
}

} // namespace internal
/*! @endcond */

/**
 * @brief Sparse set implementation.
 *
 * Sparse set or packed array or whatever is the name users give it.<br/>
 * Two arrays: an _external_ one and an _internal_ one; a _sparse_ one and a
 * _packed_ one; one used for direct access through contiguous memory, the other
 * one used to get the data through an extra level of indirection.<br/>
 * This type of data structure is widely documented in the literature and on the
 * web. This is nothing more than a customized implementation suitable for the
 * purpose of the framework.
 *
 * @note
 * Internal data structures arrange elements to maximize performance. There are
 * no guarantees that entities are returned in the insertion order when iterate
 * a sparse set. Do not make assumption on the order in any case.
 *
 * @tparam Entity A valid entity type.
 * @tparam Allocator Type of allocator used to manage memory and elements.
 */
template<typename Entity, typename Allocator>
class basic_sparse_set {
    using alloc_traits = std::allocator_traits<Allocator>;
    static_assert(std::is_same_v<typename alloc_traits::value_type, Entity>, "Invalid value type");
    using sparse_container_type = std::vector<typename alloc_traits::pointer, typename alloc_traits::template rebind_alloc<typename alloc_traits::pointer>>;
    using packed_container_type = std::vector<Entity, Allocator>;
    using traits_type = entt_traits<Entity>;

    static constexpr auto max_size = static_cast<std::size_t>(traits_type::to_entity(null));

    // it could be auto but gcc complains and emits a warning due to a false positive
    [[nodiscard]] std::size_t policy_to_head() const noexcept {
        return static_cast<size_type>(max_size * static_cast<std::remove_const_t<decltype(max_size)>>(mode != deletion_policy::swap_only));
    }

    [[nodiscard]] auto entity_to_pos(const Entity entt) const noexcept {
        return static_cast<size_type>(traits_type::to_entity(entt));
    }

    [[nodiscard]] auto pos_to_page(const std::size_t pos) const noexcept {
        return static_cast<size_type>(pos / traits_type::page_size);
    }

    [[nodiscard]] auto sparse_ptr(const Entity entt) const {
        const auto pos = entity_to_pos(entt);
        const auto page = pos_to_page(pos);
        return (page < sparse.size() && sparse[page]) ? (sparse[page] + fast_mod(pos, traits_type::page_size)) : nullptr;
    }

    [[nodiscard]] auto &sparse_ref(const Entity entt) const {
        ENTT_ASSERT(sparse_ptr(entt), "Invalid element");
        const auto pos = entity_to_pos(entt);
        return sparse[pos_to_page(pos)][fast_mod(pos, traits_type::page_size)];
    }

    [[nodiscard]] auto to_iterator(const Entity entt) const {
        return --(end() - static_cast<difference_type>(index(entt)));
    }

    [[nodiscard]] auto &assure_at_least(const Entity entt) {
        const auto pos = entity_to_pos(entt);
        const auto page = pos_to_page(pos);

        if(!(page < sparse.size())) {
            sparse.resize(page + 1u, nullptr);
        }

        if(!sparse[page]) {
            constexpr entity_type init = null;
            auto page_allocator{packed.get_allocator()};
            sparse[page] = alloc_traits::allocate(page_allocator, traits_type::page_size);
            std::uninitialized_fill(sparse[page], sparse[page] + traits_type::page_size, init);
        }

        return sparse[page][fast_mod(pos, traits_type::page_size)];
    }

    void release_sparse_pages() {
        auto page_allocator{packed.get_allocator()};

        for(auto &&page: sparse) {
            if(page != nullptr) {
                std::destroy(page, page + traits_type::page_size);
                alloc_traits::deallocate(page_allocator, page, traits_type::page_size);
                page = nullptr;
            }
        }
    }

    void swap_at(const std::size_t lhs, const std::size_t rhs) {
        auto &from = packed[lhs];
        auto &to = packed[rhs];

        sparse_ref(from) = traits_type::combine(static_cast<typename traits_type::entity_type>(rhs), traits_type::to_integral(from));
        sparse_ref(to) = traits_type::combine(static_cast<typename traits_type::entity_type>(lhs), traits_type::to_integral(to));

        std::swap(from, to);
    }

private:
    [[nodiscard]] virtual const void *get_at(const std::size_t) const {
        return nullptr;
    }

    virtual void swap_or_move([[maybe_unused]] const std::size_t lhs, [[maybe_unused]] const std::size_t rhs) {
        ENTT_ASSERT((mode != deletion_policy::swap_only) || ((lhs < head) == (rhs < head)), "Cross swapping is not supported");
    }

protected:
    /*! @brief Random access iterator type. */
    using basic_iterator = internal::sparse_set_iterator<packed_container_type>;

    /**
     * @brief Erases an entity from a sparse set.
     * @param it An iterator to the element to pop.
     */
    void swap_only(const basic_iterator it) {
        ENTT_ASSERT(mode == deletion_policy::swap_only, "Deletion policy mismatch");
        const auto pos = index(*it);
        bump(traits_type::next(*it));
        swap_at(pos, head -= (pos < head));
    }

    /**
     * @brief Erases an entity from a sparse set.
     * @param it An iterator to the element to pop.
     */
    void swap_and_pop(const basic_iterator it) {
        ENTT_ASSERT(mode == deletion_policy::swap_and_pop, "Deletion policy mismatch");
        auto &self = sparse_ref(*it);
        const auto entt = traits_type::to_entity(self);
        sparse_ref(packed.back()) = traits_type::combine(entt, traits_type::to_integral(packed.back()));
        packed[static_cast<size_type>(entt)] = packed.back();
        // unnecessary but it helps to detect nasty bugs
        // NOLINTNEXTLINE(bugprone-assert-side-effect)
        ENTT_ASSERT((packed.back() = null, true), "");
        // lazy self-assignment guard
        self = null;
        packed.pop_back();
    }

    /**
     * @brief Erases an entity from a sparse set.
     * @param it An iterator to the element to pop.
     */
    void in_place_pop(const basic_iterator it) {
        ENTT_ASSERT(mode == deletion_policy::in_place, "Deletion policy mismatch");
        const auto pos = entity_to_pos(std::exchange(sparse_ref(*it), null));
        packed[pos] = traits_type::combine(static_cast<typename traits_type::entity_type>(std::exchange(head, pos)), tombstone);
    }

    /**
     * @brief Erases entities from a sparse set.
     * @param first An iterator to the first element of the range of entities.
     * @param last An iterator past the last element of the range of entities.
     */
    virtual void pop(basic_iterator first, basic_iterator last) {
        switch(mode) {
        case deletion_policy::swap_and_pop:
            for(; first != last; ++first) {
                swap_and_pop(first);
            }
            break;
        case deletion_policy::in_place:
            for(; first != last; ++first) {
                in_place_pop(first);
            }
            break;
        case deletion_policy::swap_only:
            for(; first != last; ++first) {
                swap_only(first);
            }
            break;
        }
    }

    /*! @brief Erases all entities of a sparse set. */
    virtual void pop_all() {
        switch(mode) {
        case deletion_policy::in_place:
            if(head != max_size) {
                for(auto &&elem: packed) {
                    if(elem != tombstone) {
                        sparse_ref(elem) = null;
                    }
                }
                break;
            }
            [[fallthrough]];
        case deletion_policy::swap_only:
        case deletion_policy::swap_and_pop:
            for(auto &&elem: packed) {
                sparse_ref(elem) = null;
            }
            break;
        }

        head = policy_to_head();
        packed.clear();
    }

    /**
     * @brief Assigns an entity to a sparse set.
     * @param entt A valid identifier.
     * @param force_back Force back insertion.
     * @return Iterator pointing to the emplaced element.
     */
    virtual basic_iterator try_emplace(const Entity entt, const bool force_back, const void * = nullptr) {
        ENTT_ASSERT(entt != null && entt != tombstone, "Invalid element");
        auto &elem = assure_at_least(entt);
        auto pos = size();

        switch(mode) {
        case deletion_policy::in_place:
            if(head != max_size && !force_back) {
                pos = head;
                ENTT_ASSERT(elem == null, "Slot not available");
                elem = traits_type::combine(static_cast<typename traits_type::entity_type>(head), traits_type::to_integral(entt));
                head = entity_to_pos(std::exchange(packed[pos], entt));
                break;
            }
            [[fallthrough]];
        case deletion_policy::swap_and_pop:
            packed.push_back(entt);
            ENTT_ASSERT(elem == null, "Slot not available");
            elem = traits_type::combine(static_cast<typename traits_type::entity_type>(packed.size() - 1u), traits_type::to_integral(entt));
            break;
        case deletion_policy::swap_only:
            if(elem == null) {
                packed.push_back(entt);
                elem = traits_type::combine(static_cast<typename traits_type::entity_type>(packed.size() - 1u), traits_type::to_integral(entt));
            } else {
                ENTT_ASSERT(!(entity_to_pos(elem) < head), "Slot not available");
                bump(entt);
            }

            pos = head++;
            swap_at(entity_to_pos(elem), pos);
            break;
        }

        return --(end() - static_cast<difference_type>(pos));
    }

    /*! @brief Forwards variables to derived classes, if any. */
    // NOLINTNEXTLINE(performance-unnecessary-value-param)
    virtual void bind_any(any) noexcept {}

public:
    /*! @brief Allocator type. */
    using allocator_type = Allocator;
    /*! @brief Underlying entity identifier. */
    using entity_type = typename traits_type::value_type;
    /*! @brief Underlying version type. */
    using version_type = typename traits_type::version_type;
    /*! @brief Unsigned integer type. */
    using size_type = std::size_t;
    /*! @brief Signed integer type. */
    using difference_type = std::ptrdiff_t;
    /*! @brief Pointer type to contained entities. */
    using pointer = typename packed_container_type::const_pointer;
    /*! @brief Random access iterator type. */
    using iterator = basic_iterator;
    /*! @brief Constant random access iterator type. */
    using const_iterator = iterator;
    /*! @brief Reverse iterator type. */
    using reverse_iterator = std::reverse_iterator<iterator>;
    /*! @brief Constant reverse iterator type. */
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;

    /*! @brief Default constructor. */
    basic_sparse_set()
        : basic_sparse_set{type_id<void>()} {}

    /**
     * @brief Constructs an empty container with a given allocator.
     * @param allocator The allocator to use.
     */
    explicit basic_sparse_set(const allocator_type &allocator)
        : basic_sparse_set{deletion_policy::swap_and_pop, allocator} {}

    /**
     * @brief Constructs an empty container with the given policy and allocator.
     * @param pol Type of deletion policy.
     * @param allocator The allocator to use (possibly default-constructed).
     */
    explicit basic_sparse_set(deletion_policy pol, const allocator_type &allocator = {})
        : basic_sparse_set{type_id<void>(), pol, allocator} {}

    /**
     * @brief Constructs an empty container with the given value type, policy
     * and allocator.
     * @param elem Returned value type, if any.
     * @param pol Type of deletion policy.
     * @param allocator The allocator to use (possibly default-constructed).
     */
    explicit basic_sparse_set(const type_info &elem, deletion_policy pol = deletion_policy::swap_and_pop, const allocator_type &allocator = {})
        : sparse{allocator},
          packed{allocator},
          info{&elem},
          mode{pol},
          head{policy_to_head()} {
        ENTT_ASSERT(traits_type::version_mask || mode != deletion_policy::in_place, "Policy does not support zero-sized versions");
    }

    /*! @brief Default copy constructor, deleted on purpose. */
    basic_sparse_set(const basic_sparse_set &) = delete;

    /**
     * @brief Move constructor.
     * @param other The instance to move from.
     */
    basic_sparse_set(basic_sparse_set &&other) noexcept
        : sparse{std::move(other.sparse)},
          packed{std::move(other.packed)},
          info{other.info},
          mode{other.mode},
          head{std::exchange(other.head, policy_to_head())} {}

    /**
     * @brief Allocator-extended move constructor.
     * @param other The instance to move from.
     * @param allocator The allocator to use.
     */
    basic_sparse_set(basic_sparse_set &&other, const allocator_type &allocator)
        : sparse{std::move(other.sparse), allocator},
          packed{std::move(other.packed), allocator},
          info{other.info},
          mode{other.mode},
          head{std::exchange(other.head, policy_to_head())} {
        ENTT_ASSERT(alloc_traits::is_always_equal::value || get_allocator() == other.get_allocator(), "Copying a sparse set is not allowed");
    }

    /*! @brief Default destructor. */
    virtual ~basic_sparse_set() {
        release_sparse_pages();
    }

    /**
     * @brief Default copy assignment operator, deleted on purpose.
     * @return This sparse set.
     */
    basic_sparse_set &operator=(const basic_sparse_set &) = delete;

    /**
     * @brief Move assignment operator.
     * @param other The instance to move from.
     * @return This sparse set.
     */
    basic_sparse_set &operator=(basic_sparse_set &&other) noexcept {
        ENTT_ASSERT(alloc_traits::is_always_equal::value || get_allocator() == other.get_allocator(), "Copying a sparse set is not allowed");
        swap(other);
        return *this;
    }

    /**
     * @brief Exchanges the contents with those of a given sparse set.
     * @param other Sparse set to exchange the content with.
     */
    void swap(basic_sparse_set &other) noexcept {
        using std::swap;
        swap(sparse, other.sparse);
        swap(packed, other.packed);
        swap(info, other.info);
        swap(mode, other.mode);
        swap(head, other.head);
    }

    /**
     * @brief Returns the associated allocator.
     * @return The associated allocator.
     */
    [[nodiscard]] constexpr allocator_type get_allocator() const noexcept {
        return packed.get_allocator();
    }

    /**
     * @brief Returns the deletion policy of a sparse set.
     * @return The deletion policy of the sparse set.
     */
    [[nodiscard]] deletion_policy policy() const noexcept {
        return mode;
    }

    /**
     * @brief Returns data on the free list whose meaning depends on the mode.
     * @return Free list information that is mode dependent.
     */
    [[nodiscard]] size_type free_list() const noexcept {
        return head;
    }

    /**
     * @brief Sets data on the free list whose meaning depends on the mode.
     * @param value Free list information that is mode dependent.
     */
    void free_list(const size_type value) noexcept {
        ENTT_ASSERT((mode == deletion_policy::swap_only) && !(value > packed.size()), "Invalid value");
        head = value;
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
        packed.reserve(cap);
    }

    /**
     * @brief Returns the number of elements that a sparse set has currently
     * allocated space for.
     * @return Capacity of the sparse set.
     */
    [[nodiscard]] virtual size_type capacity() const noexcept {
        return packed.capacity();
    }

    /*! @brief Requests the removal of unused capacity. */
    virtual void shrink_to_fit() {
        sparse_container_type other{sparse.get_allocator()};
        const auto len = sparse.size();
        size_type cnt{};

        other.reserve(len);

        for(auto &&elem: std::as_const(packed)) {
            if(elem != tombstone) {
                if(const auto page = pos_to_page(entity_to_pos(elem)); sparse[page] != nullptr) {
                    if(const auto sz = page + 1u; sz > other.size()) {
                        other.resize(sz, nullptr);
                    }

                    other[page] = std::exchange(sparse[page], nullptr);

                    if(++cnt == len) {
                        // early exit due to lack of pages
                        break;
                    }
                }
            }
        }

        release_sparse_pages();
        sparse.swap(other);

        sparse.shrink_to_fit();
        packed.shrink_to_fit();
    }

    /**
     * @brief Returns the extent of a sparse set.
     *
     * The extent of a sparse set is also the size of the internal sparse array.
     * There is no guarantee that all pages have been allocated, nor that the
     * internal packed array is be the same size.
     *
     * @return Extent of the sparse set.
     */
    [[nodiscard]] size_type extent() const noexcept {
        return sparse.size() * traits_type::page_size;
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
    [[nodiscard]] size_type size() const noexcept {
        return packed.size();
    }

    /**
     * @brief Checks whether a sparse set is empty.
     * @return True if the sparse set is empty, false otherwise.
     */
    [[nodiscard]] bool empty() const noexcept {
        return packed.empty();
    }

    /**
     * @brief Checks whether a sparse set is fully packed.
     * @return True if the sparse set is fully packed, false otherwise.
     */
    [[nodiscard]] bool contiguous() const noexcept {
        return (mode != deletion_policy::in_place) || (head == max_size);
    }

    /**
     * @brief Direct access to the internal packed array.
     * @return A pointer to the internal packed array.
     */
    [[nodiscard]] pointer data() const noexcept {
        return packed.data();
    }

    /**
     * @brief Returns an iterator to the beginning.
     *
     * If the sparse set is empty, the returned iterator will be equal to
     * `end()`.
     *
     * @return An iterator to the first entity of the sparse set.
     */
    [[nodiscard]] iterator begin() const noexcept {
        const auto pos = static_cast<difference_type>(packed.size());
        return iterator{packed, pos};
    }

    /*! @copydoc begin */
    [[nodiscard]] const_iterator cbegin() const noexcept {
        return begin();
    }

    /**
     * @brief Returns an iterator to the end.
     * @return An iterator to the element following the last entity of a sparse
     * set.
     */
    [[nodiscard]] iterator end() const noexcept {
        return iterator{packed, {}};
    }

    /*! @copydoc end */
    [[nodiscard]] const_iterator cend() const noexcept {
        return end();
    }

    /**
     * @brief Returns a reverse iterator to the beginning.
     *
     * If the sparse set is empty, the returned iterator will be equal to
     * `rend()`.
     *
     * @return An iterator to the first entity of the reversed internal packed
     * array.
     */
    [[nodiscard]] reverse_iterator rbegin() const noexcept {
        return std::make_reverse_iterator(end());
    }

    /*! @copydoc rbegin */
    [[nodiscard]] const_reverse_iterator crbegin() const noexcept {
        return rbegin();
    }

    /**
     * @brief Returns a reverse iterator to the end.
     * @return An iterator to the element following the last entity of the
     * reversed sparse set.
     */
    [[nodiscard]] reverse_iterator rend() const noexcept {
        return std::make_reverse_iterator(begin());
    }

    /*! @copydoc rend */
    [[nodiscard]] const_reverse_iterator crend() const noexcept {
        return rend();
    }

    /**
     * @brief Finds an entity.
     * @param entt A valid identifier.
     * @return An iterator to the given entity if it's found, past the end
     * iterator otherwise.
     */
    [[nodiscard]] const_iterator find(const entity_type entt) const noexcept {
        return contains(entt) ? to_iterator(entt) : end();
    }

    /**
     * @brief Checks if a sparse set contains an entity.
     * @param entt A valid identifier.
     * @return True if the sparse set contains the entity, false otherwise.
     */
    [[nodiscard]] bool contains(const entity_type entt) const noexcept {
        const auto *elem = sparse_ptr(entt);
        constexpr auto cap = traits_type::entity_mask;
        constexpr auto mask = traits_type::to_integral(null) & ~cap;
        // testing versions permits to avoid accessing the packed array
        return elem && (((mask & traits_type::to_integral(entt)) ^ traits_type::to_integral(*elem)) < cap);
    }

    /**
     * @brief Returns the contained version for an identifier.
     * @param entt A valid identifier.
     * @return The version for the given identifier if present, the tombstone
     * version otherwise.
     */
    [[nodiscard]] version_type current(const entity_type entt) const noexcept {
        const auto *elem = sparse_ptr(entt);
        constexpr auto fallback = traits_type::to_version(tombstone);
        return elem ? traits_type::to_version(*elem) : fallback;
    }

    /**
     * @brief Returns the position of an entity in a sparse set.
     *
     * @warning
     * Attempting to get the position of an entity that doesn't belong to the
     * sparse set results in undefined behavior.
     *
     * @param entt A valid identifier.
     * @return The position of the entity in the sparse set.
     */
    [[nodiscard]] size_type index(const entity_type entt) const noexcept {
        ENTT_ASSERT(contains(entt), "Set does not contain entity");
        return entity_to_pos(sparse_ref(entt));
    }

    /**
     * @brief Returns the entity at specified location.
     * @param pos The position for which to return the entity.
     * @return The entity at specified location.
     */
    [[nodiscard]] entity_type operator[](const size_type pos) const noexcept {
        ENTT_ASSERT(pos < packed.size(), "Index out of bounds");
        return packed[pos];
    }

    /**
     * @brief Returns the element assigned to an entity, if any.
     *
     * @warning
     * Attempting to use an entity that doesn't belong to the sparse set results
     * in undefined behavior.
     *
     * @param entt A valid identifier.
     * @return An opaque pointer to the element assigned to the entity, if any.
     */
    [[nodiscard]] const void *value(const entity_type entt) const noexcept {
        return get_at(index(entt));
    }

    /*! @copydoc value */
    [[nodiscard]] void *value(const entity_type entt) noexcept {
        return const_cast<void *>(std::as_const(*this).value(entt));
    }

    /**
     * @brief Assigns an entity to a sparse set.
     *
     * @warning
     * Attempting to assign an entity that already belongs to the sparse set
     * results in undefined behavior.
     *
     * @param entt A valid identifier.
     * @param elem Optional opaque element to forward to mixins, if any.
     * @return Iterator pointing to the emplaced element in case of success, the
     * `end()` iterator otherwise.
     */
    iterator push(const entity_type entt, const void *elem = nullptr) {
        return try_emplace(entt, false, elem);
    }

    /**
     * @brief Assigns one or more entities to a sparse set.
     *
     * @warning
     * Attempting to assign an entity that already belongs to the sparse set
     * results in undefined behavior.
     *
     * @tparam It Type of input iterator.
     * @param first An iterator to the first element of the range of entities.
     * @param last An iterator past the last element of the range of entities.
     * @return Iterator pointing to the first element inserted in case of
     * success, the `end()` iterator otherwise.
     */
    template<typename It>
    iterator push(It first, It last) {
        auto curr = end();

        for(; first != last; ++first) {
            curr = try_emplace(*first, true);
        }

        return curr;
    }

    /**
     * @brief Bump the version number of an entity.
     *
     * @warning
     * Attempting to bump the version of an entity that doesn't belong to the
     * sparse set results in undefined behavior.
     *
     * @param entt A valid identifier.
     * @return The version of the given identifier.
     */
    version_type bump(const entity_type entt) {
        auto &elem = sparse_ref(entt);
        ENTT_ASSERT(entt != null && elem != tombstone, "Cannot set the required version");
        elem = traits_type::combine(traits_type::to_integral(elem), traits_type::to_integral(entt));
        packed[entity_to_pos(elem)] = entt;
        return traits_type::to_version(entt);
    }

    /**
     * @brief Erases an entity from a sparse set.
     *
     * @warning
     * Attempting to erase an entity that doesn't belong to the sparse set
     * results in undefined behavior.
     *
     * @param entt A valid identifier.
     */
    void erase(const entity_type entt) {
        const auto it = to_iterator(entt);
        pop(it, it + 1u);
    }

    /**
     * @brief Erases entities from a set.
     *
     * @sa erase
     *
     * @tparam It Type of input iterator.
     * @param first An iterator to the first element of the range of entities.
     * @param last An iterator past the last element of the range of entities.
     */
    template<typename It>
    void erase(It first, It last) {
        if constexpr(std::is_same_v<It, basic_iterator>) {
            pop(first, last);
        } else {
            for(; first != last; ++first) {
                erase(*first);
            }
        }
    }

    /**
     * @brief Removes an entity from a sparse set if it exists.
     * @param entt A valid identifier.
     * @return True if the entity is actually removed, false otherwise.
     */
    bool remove(const entity_type entt) {
        return contains(entt) && (erase(entt), true);
    }

    /**
     * @brief Removes entities from a sparse set if they exist.
     * @tparam It Type of input iterator.
     * @param first An iterator to the first element of the range of entities.
     * @param last An iterator past the last element of the range of entities.
     * @return The number of entities actually removed.
     */
    template<typename It>
    size_type remove(It first, It last) {
        size_type count{};

        if constexpr(std::is_same_v<It, basic_iterator>) {
            while(first != last) {
                while(first != last && !contains(*first)) {
                    ++first;
                }

                const auto it = first;

                while(first != last && contains(*first)) {
                    ++first;
                }

                count += static_cast<size_type>(std::distance(it, first));
                erase(it, first);
            }
        } else {
            for(; first != last; ++first) {
                count += remove(*first);
            }
        }

        return count;
    }

    /*! @brief Removes all tombstones from a sparse set. */
    void compact() {
        if(mode == deletion_policy::in_place) {
            size_type from = packed.size();
            size_type pos = std::exchange(head, max_size);

            for(; from && packed[from - 1u] == tombstone; --from) {}

            while(pos != max_size) {
                if(const auto to = std::exchange(pos, entity_to_pos(packed[pos])); to < from) {
                    --from;
                    swap_or_move(from, to);

                    packed[to] = packed[from];
                    const auto elem = static_cast<typename traits_type::entity_type>(to);
                    sparse_ref(packed[to]) = traits_type::combine(elem, traits_type::to_integral(packed[to]));

                    for(; from && packed[from - 1u] == tombstone; --from) {}
                }
            }

            packed.erase(packed.begin() + static_cast<difference_type>(from), packed.end());
        }
    }

    /**
     * @brief Swaps two entities in a sparse set.
     *
     * For what it's worth, this function affects both the internal sparse array
     * and the internal packed array. Users should not care of that anyway.
     *
     * @warning
     * Attempting to swap entities that don't belong to the sparse set results
     * in undefined behavior.
     *
     * @param lhs A valid identifier.
     * @param rhs A valid identifier.
     */
    void swap_elements(const entity_type lhs, const entity_type rhs) {
        const auto from = index(lhs);
        const auto to = index(rhs);

        // basic no-leak guarantee if swapping throws
        swap_or_move(from, to);
        swap_at(from, to);
    }

    /**
     * @brief Sort the first count elements according to the given comparison
     * function.
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
     * The sort function object must offer a member function template
     * `operator()` that accepts three arguments:
     *
     * * An iterator to the first element of the range to sort.
     * * An iterator past the last element of the range to sort.
     * * A comparison function to use to compare the elements.
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
    void sort_n(const size_type length, Compare compare, Sort algo = Sort{}, Args &&...args) {
        ENTT_ASSERT((mode != deletion_policy::in_place) || (head == max_size), "Sorting with tombstones not allowed");
        ENTT_ASSERT(!(length > packed.size()), "Length exceeds the number of elements");

        algo(packed.rend() - static_cast<difference_type>(length), packed.rend(), std::move(compare), std::forward<Args>(args)...);

        for(size_type pos{}; pos < length; ++pos) {
            auto curr = pos;
            auto next = index(packed[curr]);

            while(curr != next) {
                const auto idx = index(packed[next]);
                const auto entt = packed[curr];

                swap_or_move(next, idx);
                const auto elem = static_cast<typename traits_type::entity_type>(curr);
                sparse_ref(entt) = traits_type::combine(elem, traits_type::to_integral(packed[curr]));
                curr = std::exchange(next, idx);
            }
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
    void sort(Compare compare, Sort algo = Sort{}, Args &&...args) {
        const size_type len = (mode == deletion_policy::swap_only) ? head : packed.size();
        sort_n(len, std::move(compare), std::move(algo), std::forward<Args>(args)...);
    }

    /**
     * @brief Sort entities according to their order in a range.
     *
     * Entities that are part of both the sparse set and the range are ordered
     * internally according to the order they have in the range.<br/>
     * All other entities goes to the end of the sparse set and there are no
     * guarantees on their order.
     *
     * @tparam It Type of input iterator.
     * @param first An iterator to the first element of the range of entities.
     * @param last An iterator past the last element of the range of entities.
     * @return An iterator past the last of the elements actually shared.
     */
    template<typename It>
    iterator sort_as(It first, It last) {
        ENTT_ASSERT((mode != deletion_policy::in_place) || (head == max_size), "Sorting with tombstones not allowed");
        const size_type len = (mode == deletion_policy::swap_only) ? head : packed.size();
        auto it = end() - static_cast<difference_type>(len);

        for(const auto other = end(); (it != other) && (first != last); ++first) {
            if(const auto curr = *first; contains(curr)) {
                if(const auto entt = *it; entt != curr) {
                    // basic no-leak guarantee (with invalid state) if swapping throws
                    swap_elements(entt, curr);
                }

                ++it;
            }
        }

        return it;
    }

    /*! @brief Clears a sparse set. */
    void clear() {
        pop_all();
        // sanity check to avoid subtle issues due to storage classes
        ENTT_ASSERT((compact(), size()) == 0u, "Non-empty set");
        head = policy_to_head();
        packed.clear();
    }

    /**
     * @brief Returned value type, if any.
     * @return Returned value type, if any.
     */
    [[nodiscard]] const type_info &type() const noexcept {
        return *info;
    }

    /**
     * @brief Forwards variables to derived classes, if any.
     * @tparam Type Type of the element to forward.
     * @param value The element to forward.
     */
    template<typename Type>
    void bind(Type &&value) noexcept {
        bind_any(forward_as_any(std::forward<Type>(value)));
    }

private:
    sparse_container_type sparse;
    packed_container_type packed;
    const type_info *info;
    deletion_policy mode;
    size_type head;
};

} // namespace entt

#endif

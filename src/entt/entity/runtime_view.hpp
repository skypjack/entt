#ifndef ENTT_ENTITY_RUNTIME_VIEW_HPP
#define ENTT_ENTITY_RUNTIME_VIEW_HPP

#include <algorithm>
#include <iterator>
#include <type_traits>
#include <utility>
#include <vector>
#include "../config/config.h"
#include "entity.hpp"
#include "fwd.hpp"
#include "sparse_set.hpp"

namespace entt {

/**
 * @cond TURN_OFF_DOXYGEN
 * Internal details not to be documented.
 */

namespace internal {

template<typename Set>
class runtime_view_iterator final {
    using iterator_type = typename Set::iterator;

    [[nodiscard]] bool valid() const {
        return (!tombstone_check || *it != tombstone)
               && std::all_of(++pools->begin(), pools->end(), [entt = *it](const auto *curr) { return curr->contains(entt); })
               && std::none_of(filter->cbegin(), filter->cend(), [entt = *it](const auto *curr) { return curr && curr->contains(entt); });
    }

public:
    using difference_type = typename iterator_type::difference_type;
    using value_type = typename iterator_type::value_type;
    using pointer = typename iterator_type::pointer;
    using reference = typename iterator_type::reference;
    using iterator_category = std::bidirectional_iterator_tag;

    runtime_view_iterator() ENTT_NOEXCEPT
        : pools{},
          filter{},
          it{},
          tombstone_check{} {}

    runtime_view_iterator(const std::vector<const Set *> &cpools, const std::vector<const Set *> &ignore, iterator_type curr) ENTT_NOEXCEPT
        : pools{&cpools},
          filter{&ignore},
          it{curr},
          tombstone_check{pools->size() == 1u && (*pools)[0u]->policy() == deletion_policy::in_place} {
        if(it != (*pools)[0]->end() && !valid()) {
            ++(*this);
        }
    }

    runtime_view_iterator &operator++() {
        while(++it != (*pools)[0]->end() && !valid()) {}
        return *this;
    }

    runtime_view_iterator operator++(int) {
        runtime_view_iterator orig = *this;
        return ++(*this), orig;
    }

    runtime_view_iterator &operator--() {
        while(--it != (*pools)[0]->begin() && !valid()) {}
        return *this;
    }

    runtime_view_iterator operator--(int) {
        runtime_view_iterator orig = *this;
        return operator--(), orig;
    }

    [[nodiscard]] pointer operator->() const ENTT_NOEXCEPT {
        return it.operator->();
    }

    [[nodiscard]] reference operator*() const ENTT_NOEXCEPT {
        return *operator->();
    }

    [[nodiscard]] bool operator==(const runtime_view_iterator &other) const ENTT_NOEXCEPT {
        return it == other.it;
    }

    [[nodiscard]] bool operator!=(const runtime_view_iterator &other) const ENTT_NOEXCEPT {
        return !(*this == other);
    }

private:
    const std::vector<const Set *> *pools;
    const std::vector<const Set *> *filter;
    iterator_type it;
    bool tombstone_check;
};

} // namespace internal

/**
 * Internal details not to be documented.
 * @endcond
 */

/**
 * @brief Runtime view implementation.
 *
 * Primary template isn't defined on purpose. All the specializations give a
 * compile-time error, but for a few reasonable cases.
 */
template<typename>
struct basic_runtime_view;

/**
 * @brief Generic runtime view.
 *
 * Runtime views iterate over those entities that have at least all the given
 * components in their bags. During initialization, a runtime view looks at the
 * number of entities available for each component and picks up a reference to
 * the smallest set of candidate entities in order to get a performance boost
 * when iterate.<br/>
 * Order of elements during iterations are highly dependent on the order of the
 * underlying data structures. See sparse_set and its specializations for more
 * details.
 *
 * @b Important
 *
 * Iterators aren't invalidated if:
 *
 * * New instances of the given components are created and assigned to entities.
 * * The entity currently pointed is modified (as an example, if one of the
 *   given components is removed from the entity to which the iterator points).
 * * The entity currently pointed is destroyed.
 *
 * In all the other cases, modifying the pools of the given components in any
 * way invalidates all the iterators and using them results in undefined
 * behavior.
 *
 * @note
 * Views share references to the underlying data structures of the registry that
 * generated them. Therefore any change to the entities and to the components
 * made by means of the registry are immediately reflected by the views, unless
 * a pool was missing when the view was built (in this case, the view won't
 * have a valid reference and won't be updated accordingly).
 *
 * @warning
 * Lifetime of a view must not overcome that of the registry that generated it.
 * In any other case, attempting to use a view results in undefined behavior.
 *
 * @tparam Entity A valid entity type (see entt_traits for more details).
 * @tparam Allocator Type of allocator used to manage memory and elements.
 */
template<typename Entity, typename Allocator>
struct basic_runtime_view<basic_sparse_set<Entity, Allocator>> {
    /*! @brief Underlying entity identifier. */
    using entity_type = Entity;
    /*! @brief Unsigned integer type. */
    using size_type = std::size_t;
    /*! @brief Common type among all storage types. */
    using base_type = basic_sparse_set<Entity, Allocator>;
    /*! @brief Bidirectional iterator type. */
    using iterator = internal::runtime_view_iterator<base_type>;

    /*! @brief Default constructor to use to create empty, invalid views. */
    basic_runtime_view() ENTT_NOEXCEPT
        : pools{},
          filter{} {}

    /**
     * @brief Appends an opaque storage object to a runtime view.
     * @param base An opaque reference to a storage object.
     * @return This runtime view.
     */
    basic_runtime_view &iterate(const base_type &base) {
        if(pools.empty() || !(base.size() < pools[0u]->size())) {
            pools.push_back(&base);
        } else {
            pools.push_back(std::exchange(pools[0u], &base));
        }

        return *this;
    }

    /**
     * @brief Adds an opaque storage object as a filter of a runtime view.
     * @param base An opaque reference to a storage object.
     * @return This runtime view.
     */
    basic_runtime_view &exclude(const base_type &base) {
        filter.push_back(&base);
        return *this;
    }

    /**
     * @brief Estimates the number of entities iterated by the view.
     * @return Estimated number of entities iterated by the view.
     */
    [[nodiscard]] size_type size_hint() const {
        return pools.empty() ? size_type{} : pools.front()->size();
    }

    /**
     * @brief Returns an iterator to the first entity that has the given
     * components.
     *
     * The returned iterator points to the first entity that has the given
     * components. If the view is empty, the returned iterator will be equal to
     * `end()`.
     *
     * @return An iterator to the first entity that has the given components.
     */
    [[nodiscard]] iterator begin() const {
        return pools.empty() ? iterator{} : iterator{pools, filter, pools[0]->begin()};
    }

    /**
     * @brief Returns an iterator that is past the last entity that has the
     * given components.
     *
     * The returned iterator points to the entity following the last entity that
     * has the given components. Attempting to dereference the returned iterator
     * results in undefined behavior.
     *
     * @return An iterator to the entity following the last entity that has the
     * given components.
     */
    [[nodiscard]] iterator end() const {
        return pools.empty() ? iterator{} : iterator{pools, filter, pools[0]->end()};
    }

    /**
     * @brief Checks if a view contains an entity.
     * @param entt A valid identifier.
     * @return True if the view contains the given entity, false otherwise.
     */
    [[nodiscard]] bool contains(const entity_type entt) const {
        return !pools.empty()
               && std::all_of(pools.cbegin(), pools.cend(), [entt](const auto *curr) { return curr->contains(entt); })
               && std::none_of(filter.cbegin(), filter.cend(), [entt](const auto *curr) { return curr && curr->contains(entt); });
    }

    /**
     * @brief Iterates entities and applies the given function object to them.
     *
     * The function object is invoked for each entity. It is provided only with
     * the entity itself. To get the components, users can use the registry with
     * which the view was built.<br/>
     * The signature of the function should be equivalent to the following:
     *
     * @code{.cpp}
     * void(const entity_type);
     * @endcode
     *
     * @tparam Func Type of the function object to invoke.
     * @param func A valid function object.
     */
    template<typename Func>
    void each(Func func) const {
        for(const auto entity: *this) {
            func(entity);
        }
    }

private:
    std::vector<const base_type *> pools;
    std::vector<const base_type *> filter;
};

} // namespace entt

#endif

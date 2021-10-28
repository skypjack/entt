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

template<typename Type>
class runtime_view_iterator final {
    [[nodiscard]] bool valid() const {
        return (no_tombstone_check || (*it != tombstone))
               && std::all_of(pools->begin()++, pools->end(), [entt = *it](const auto *curr) { return curr->contains(entt); })
               && std::none_of(filter->cbegin(), filter->cend(), [entt = *it](const auto *curr) { return curr && curr->contains(entt); });
    }

public:
    using iterator_type = typename Type::iterator;
    using difference_type = typename iterator_type::difference_type;
    using value_type = typename iterator_type::value_type;
    using pointer = typename iterator_type::pointer;
    using reference = typename iterator_type::reference;
    using iterator_category = std::bidirectional_iterator_tag;

    runtime_view_iterator() ENTT_NOEXCEPT = default;

    runtime_view_iterator(const std::vector<const Type *> &cpools, const std::vector<const Type *> &ignore, iterator_type curr) ENTT_NOEXCEPT
        : pools{&cpools},
          filter{&ignore},
          it{curr},
          no_tombstone_check{std::all_of(pools->cbegin(), pools->cend(), [](const Type *cpool) { return (cpool->policy() == deletion_policy::swap_and_pop); })} {
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

    runtime_view_iterator &operator--() ENTT_NOEXCEPT {
        while(--it != (*pools)[0]->begin() && !valid()) {}
        return *this;
    }

    runtime_view_iterator operator--(int) ENTT_NOEXCEPT {
        runtime_view_iterator orig = *this;
        return operator--(), orig;
    }

    [[nodiscard]] pointer operator->() const {
        return it.operator->();
    }

    [[nodiscard]] reference operator*() const {
        return *operator->();
    }

    [[nodiscard]] bool operator==(const runtime_view_iterator &other) const ENTT_NOEXCEPT {
        return it == other.it;
    }

    [[nodiscard]] bool operator!=(const runtime_view_iterator &other) const ENTT_NOEXCEPT {
        return !(*this == other);
    }

private:
    const std::vector<const Type *> *pools;
    const std::vector<const Type *> *filter;
    iterator_type it;
    bool no_tombstone_check;
};

} // namespace internal

/**
 * Internal details not to be documented.
 * @endcond
 */

/**
 * @brief Runtime view.
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
 */
template<typename Entity>
class basic_runtime_view final {
    using basic_common_type = basic_sparse_set<Entity>;

    [[nodiscard]] bool valid() const {
        return !pools.empty() && pools.front();
    }

public:
    /*! @brief Underlying entity identifier. */
    using entity_type = Entity;
    /*! @brief Unsigned integer type. */
    using size_type = std::size_t;
    /*! @brief Bidirectional iterator type. */
    using iterator = internal::runtime_view_iterator<basic_common_type>;

    /*! @brief Default constructor to use to create empty, invalid views. */
    basic_runtime_view() ENTT_NOEXCEPT
        : pools{},
          filter{} {}

    /**
     * @brief Constructs a runtime view from a set of storage classes.
     * @param cpools The storage for the types to iterate.
     * @param epools The storage for the types used to filter the view.
     */
    basic_runtime_view(std::vector<const basic_common_type *> cpools, std::vector<const basic_common_type *> epools) ENTT_NOEXCEPT
        : pools{std::move(cpools)},
          filter{std::move(epools)} {
        auto candidate = std::min_element(pools.begin(), pools.end(), [](const auto *lhs, const auto *rhs) {
            return (!lhs && rhs) || (lhs && rhs && lhs->size() < rhs->size());
        });

        // brings the best candidate (if any) on front of the vector
        std::rotate(pools.begin(), candidate, pools.end());
    }

    /**
     * @brief Estimates the number of entities iterated by the view.
     * @return Estimated number of entities iterated by the view.
     */
    [[nodiscard]] size_type size_hint() const {
        return valid() ? pools.front()->size() : size_type{};
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
        return valid() ? iterator{pools, filter, pools[0]->begin()} : iterator{};
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
        return valid() ? iterator{pools, filter, pools[0]->end()} : iterator{};
    }

    /**
     * @brief Checks if a view contains an entity.
     * @param entt A valid identifier.
     * @return True if the view contains the given entity, false otherwise.
     */
    [[nodiscard]] bool contains(const entity_type entt) const {
        return valid() && std::all_of(pools.cbegin(), pools.cend(), [entt](const auto *curr) { return curr->contains(entt); })
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
    std::vector<const basic_common_type *> pools;
    std::vector<const basic_common_type *> filter;
};

} // namespace entt

#endif

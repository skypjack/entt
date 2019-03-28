#ifndef ENTT_ENTITY_RUNTIME_VIEW_HPP
#define ENTT_ENTITY_RUNTIME_VIEW_HPP


#include <iterator>
#include <cassert>
#include <vector>
#include <utility>
#include <algorithm>
#include "../config/config.h"
#include "entt_traits.hpp"
#include "sparse_set.hpp"
#include "fwd.hpp"


namespace entt {


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
 * Lifetime of a view must overcome the one of the registry that generated it.
 * In any other case, attempting to use a view results in undefined behavior.
 *
 * @tparam Entity A valid entity type (see entt_traits for more details).
 */
template<typename Entity>
class basic_runtime_view {
    /*! @brief A registry is allowed to create views. */
    friend class basic_registry<Entity>;

    using underlying_iterator_type = typename sparse_set<Entity>::iterator_type;
    using extent_type = typename sparse_set<Entity>::size_type;
    using traits_type = entt_traits<Entity>;

    class iterator {
        friend class basic_runtime_view<Entity>;

        iterator(underlying_iterator_type first, underlying_iterator_type last, const sparse_set<Entity> * const *others, const sparse_set<Entity> * const *length, extent_type ext) ENTT_NOEXCEPT
            : begin{first},
              end{last},
              from{others},
              to{length},
              extent{ext}
        {
            if(begin != end && !valid()) {
                ++(*this);
            }
        }

        bool valid() const ENTT_NOEXCEPT {
            const auto entt = *begin;
            const auto sz = size_type(entt & traits_type::entity_mask);

            return sz < extent && std::all_of(from, to, [entt](const auto *view) {
                return view->has(entt);
            });
        }

    public:
        using difference_type = typename underlying_iterator_type::difference_type;
        using value_type = typename underlying_iterator_type::value_type;
        using pointer = typename underlying_iterator_type::pointer;
        using reference = typename underlying_iterator_type::reference;
        using iterator_category = std::forward_iterator_tag;

        iterator() ENTT_NOEXCEPT = default;

        iterator & operator++() ENTT_NOEXCEPT {
            return (++begin != end && !valid()) ? ++(*this) : *this;
        }

        iterator operator++(int) ENTT_NOEXCEPT {
            iterator orig = *this;
            return ++(*this), orig;
        }

        bool operator==(const iterator &other) const ENTT_NOEXCEPT {
            return other.begin == begin;
        }

        inline bool operator!=(const iterator &other) const ENTT_NOEXCEPT {
            return !(*this == other);
        }

        pointer operator->() const ENTT_NOEXCEPT {
            return begin.operator->();
        }

        inline reference operator*() const ENTT_NOEXCEPT {
            return *operator->();
        }

    private:
        underlying_iterator_type begin;
        underlying_iterator_type end;
        const sparse_set<Entity> * const *from;
        const sparse_set<Entity> * const *to;
        extent_type extent;
    };

    basic_runtime_view(std::vector<const sparse_set<Entity> *> others) ENTT_NOEXCEPT
        : pools{std::move(others)}
    {
        const auto it = std::min_element(pools.begin(), pools.end(), [](const auto *lhs, const auto *rhs) {
            return (!lhs && rhs) || (lhs && rhs && lhs->size() < rhs->size());
        });

        // brings the best candidate (if any) on front of the vector
        std::rotate(pools.begin(), it, pools.end());
    }

    extent_type min() const ENTT_NOEXCEPT {
        extent_type extent{};

        if(valid()) {
            const auto it = std::min_element(pools.cbegin(), pools.cend(), [](const auto *lhs, const auto *rhs) {
                return lhs->extent() < rhs->extent();
            });

            extent = (*it)->extent();
        }

        return extent;
    }

    inline bool valid() const ENTT_NOEXCEPT {
        return !pools.empty() && pools.front();
    }

public:
    /*! @brief Underlying entity identifier. */
    using entity_type = typename sparse_set<Entity>::entity_type;
    /*! @brief Unsigned integer type. */
    using size_type = typename sparse_set<Entity>::size_type;
    /*! @brief Input iterator type. */
    using iterator_type = iterator;

    /**
     * @brief Estimates the number of entities that have the given components.
     * @return Estimated number of entities that have the given components.
     */
    size_type size() const ENTT_NOEXCEPT {
        return valid() ? pools.front()->size() : size_type{};
    }

    /**
     * @brief Checks if the view is definitely empty.
     * @return True if the view is definitely empty, false otherwise.
     */
    bool empty() const ENTT_NOEXCEPT {
        return !valid() || pools.front()->empty();
    }

    /**
     * @brief Returns an iterator to the first entity that has the given
     * components.
     *
     * The returned iterator points to the first entity that has the given
     * components. If the view is empty, the returned iterator will be equal to
     * `end()`.
     *
     * @note
     * Input iterators stay true to the order imposed to the underlying data
     * structures.
     *
     * @return An iterator to the first entity that has the given components.
     */
    iterator_type begin() const ENTT_NOEXCEPT {
        iterator_type it{};

        if(valid()) {
            const auto &pool = *pools.front();
            const auto * const *data = pools.data();
            it = { pool.begin(), pool.end(), data + 1, data + pools.size(), min() };
        }

        return it;
    }

    /**
     * @brief Returns an iterator that is past the last entity that has the
     * given components.
     *
     * The returned iterator points to the entity following the last entity that
     * has the given components. Attempting to dereference the returned iterator
     * results in undefined behavior.
     *
     * @note
     * Input iterators stay true to the order imposed to the underlying data
     * structures.
     *
     * @return An iterator to the entity following the last entity that has the
     * given components.
     */
    iterator_type end() const ENTT_NOEXCEPT {
        iterator_type it{};

        if(valid()) {
            const auto &pool = *pools.front();
            it = { pool.end(), pool.end(), nullptr, nullptr, min() };
        }

        return it;
    }

    /**
     * @brief Checks if a view contains an entity.
     * @param entt A valid entity identifier.
     * @return True if the view contains the given entity, false otherwise.
     */
    bool contains(const entity_type entt) const ENTT_NOEXCEPT {
        return valid() && std::all_of(pools.cbegin(), pools.cend(), [entt](const auto *view) {
            return view->has(entt) && view->data()[view->get(entt)] == entt;
        });
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
        std::for_each(begin(), end(), func);
    }

private:
    std::vector<const sparse_set<Entity> *> pools;
};


}


#endif // ENTT_ENTITY_RUNTIME_VIEW_HPP

#ifndef ENTT_META_RANGE_HPP
#define ENTT_META_RANGE_HPP

#include <cstddef>
#include <iterator>
#include "../core/iterator.hpp"

namespace entt {

/**
 * @cond TURN_OFF_DOXYGEN
 * Internal details not to be documented.
 */

namespace internal {

template<typename Type, typename Node>
struct meta_range_iterator final {
    using difference_type = std::ptrdiff_t;
    using value_type = Type;
    using pointer = input_iterator_pointer<value_type>;
    using reference = value_type;
    using iterator_category = std::input_iterator_tag;
    using node_type = Node;

    meta_range_iterator() ENTT_NOEXCEPT
        : it{} {}

    meta_range_iterator(node_type *head) ENTT_NOEXCEPT
        : it{head} {}

    meta_range_iterator &operator++() ENTT_NOEXCEPT {
        return (it = it->next), *this;
    }

    meta_range_iterator operator++(int) ENTT_NOEXCEPT {
        meta_range_iterator orig = *this;
        return ++(*this), orig;
    }

    [[nodiscard]] reference operator*() const ENTT_NOEXCEPT {
        return it;
    }

    [[nodiscard]] pointer operator->() const ENTT_NOEXCEPT {
        return operator*();
    }

    [[nodiscard]] bool operator==(const meta_range_iterator &other) const ENTT_NOEXCEPT {
        return it == other.it;
    }

    [[nodiscard]] bool operator!=(const meta_range_iterator &other) const ENTT_NOEXCEPT {
        return !(*this == other);
    }

private:
    node_type *it;
};

} // namespace internal

/**
 * Internal details not to be documented.
 * @endcond
 */

/**
 * @brief Iterable range to use to iterate all types of meta objects.
 * @tparam Type Type of meta objects returned.
 * @tparam Node Type of meta nodes iterated.
 */
template<typename Type, typename Node = typename Type::node_type>
struct meta_range final {
    /*! @brief Node type. */
    using node_type = Node;
    /*! @brief Input iterator type. */
    using iterator = internal::meta_range_iterator<Type, Node>;
    /*! @brief Constant input iterator type. */
    using const_iterator = iterator;

    /*! @brief Default constructor. */
    meta_range() ENTT_NOEXCEPT = default;

    /**
     * @brief Constructs a meta range from a given node.
     * @param head The underlying node with which to construct the range.
     */
    meta_range(node_type *head) ENTT_NOEXCEPT
        : node{head} {}

    /**
     * @brief Returns an iterator to the beginning.
     * @return An iterator to the first meta object of the range.
     */
    [[nodiscard]] const_iterator cbegin() const ENTT_NOEXCEPT {
        return iterator{node};
    }

    /*! @copydoc cbegin */
    [[nodiscard]] iterator begin() const ENTT_NOEXCEPT {
        return cbegin();
    }

    /**
     * @brief Returns an iterator to the end.
     * @return An iterator to the element following the last meta object of the
     * range.
     */
    [[nodiscard]] const_iterator cend() const ENTT_NOEXCEPT {
        return iterator{};
    }

    /*! @copydoc cend */
    [[nodiscard]] iterator end() const ENTT_NOEXCEPT {
        return cend();
    }

private:
    node_type *node{nullptr};
};

} // namespace entt

#endif

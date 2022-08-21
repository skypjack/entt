#ifndef ENTT_META_RANGE_HPP
#define ENTT_META_RANGE_HPP

#include <cstddef>
#include <iterator>
#include <utility>
#include "../core/fwd.hpp"
#include "../core/iterator.hpp"

namespace entt {

/**
 * @cond TURN_OFF_DOXYGEN
 * Internal details not to be documented.
 */

namespace internal {

template<typename Type, typename Node>
struct old_meta_range_iterator final {
    using difference_type = std::ptrdiff_t;
    using value_type = Type;
    using pointer = input_iterator_pointer<value_type>;
    using reference = value_type;
    using iterator_category = std::input_iterator_tag;
    using node_type = Node;

    old_meta_range_iterator() noexcept
        : it{} {}

    old_meta_range_iterator(node_type *head) noexcept
        : it{head} {}

    old_meta_range_iterator &operator++() noexcept {
        return (it = it->next), *this;
    }

    old_meta_range_iterator operator++(int) noexcept {
        old_meta_range_iterator orig = *this;
        return ++(*this), orig;
    }

    [[nodiscard]] reference operator*() const noexcept {
        return it;
    }

    [[nodiscard]] pointer operator->() const noexcept {
        return operator*();
    }

    [[nodiscard]] bool operator==(const old_meta_range_iterator &other) const noexcept {
        return it == other.it;
    }

    [[nodiscard]] bool operator!=(const old_meta_range_iterator &other) const noexcept {
        return !(*this == other);
    }

private:
    node_type *it;
};

template<typename Type, typename It>
struct meta_range_iterator final {
    using difference_type = std::ptrdiff_t;
    using value_type = std::pair<id_type, Type>;
    using pointer = input_iterator_pointer<value_type>;
    using reference = value_type;
    using iterator_category = std::input_iterator_tag;

    meta_range_iterator() noexcept
        : it{} {}

    meta_range_iterator(It iter) noexcept
        : it{iter} {}

    meta_range_iterator &operator++() noexcept {
        return ++it, *this;
    }

    meta_range_iterator operator++(int) noexcept {
        meta_range_iterator orig = *this;
        return ++(*this), orig;
    }

    [[nodiscard]] reference operator*() const noexcept {
        return std::make_pair(it->first, &it->second);
    }

    [[nodiscard]] pointer operator->() const noexcept {
        return operator*();
    }

    [[nodiscard]] bool operator==(const meta_range_iterator &other) const noexcept {
        return it == other.it;
    }

    [[nodiscard]] bool operator!=(const meta_range_iterator &other) const noexcept {
        return !(*this == other);
    }

private:
    It it;
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
using old_meta_range = iterable_adaptor<internal::old_meta_range_iterator<Type, Node>>;

/**
 * @brief Iterable range to use to iterate all types of meta objects.
 * @tparam Type Type of meta objects returned.
 * @tparam It Type of forward iterator.
 */
template<typename Type, typename It>
using meta_range = iterable_adaptor<internal::meta_range_iterator<Type, It>>;

} // namespace entt

#endif

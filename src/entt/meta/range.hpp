#ifndef ENTT_META_RANGE_HPP
#define ENTT_META_RANGE_HPP


#include "internal.hpp"


namespace entt {


/**
 * @brief Meta iterator to use to iterate all types of meta objects.
 * @tparam Type Type of meta objects returned by the iterator.
 */
template<typename Type>
struct meta_iterator {
    /*! @brief Signed integer type. */
    using difference_type = std::ptrdiff_t;
    /*! @brief Type of meta objects returned by the iterator. */
    using value_type = Type;
    /*! @brief Pointer type, `void` on purpose. */
    using pointer = void;
    /*! @brief Type of proxy object. */
    using reference = value_type;
    /*! @brief Iterator category. */
    using iterator_category = std::input_iterator_tag;
    /*! @brief Node type. */
    using node_type = typename Type::node_type;

    /*! @brief Default constructor. */
    meta_iterator() ENTT_NOEXCEPT = default;

    /**
     * @brief Constructs a meta iterator from a given node.
     * @param head The underlying node with which to construct the iterator.
     */
    meta_iterator(node_type *head) ENTT_NOEXCEPT
        : it{head}
    {}

    /*! @brief Pre-increment operator. @return This iterator. */
    meta_iterator & operator++() ENTT_NOEXCEPT {
        return ++it, *this;
    }

    /*! @brief Post-increment operator. @return A copy of this iterator. */
    meta_iterator operator++(int) ENTT_NOEXCEPT {
        meta_iterator orig = *this;
        return ++it, orig;
    }

    /**
     * @brief Indirection operator.
     * @return A proxy object to the meta item pointed-to by the iterator.
     */
    [[nodiscard]] reference operator*() const ENTT_NOEXCEPT {
        return it.operator->();
    }

    /**
     * @brief Checks if two meta iterators refer to the same meta object.
     * @param other The meta iterator with which to compare.
     * @return True if the two meta iterators refer to the same meta object,
     * false otherwise.
     */
    [[nodiscard]] bool operator==(const meta_iterator &other) const ENTT_NOEXCEPT {
        return other.it == it;
    }

    /**
     * @brief Checks if two meta iterators refer to the same meta object.
     * @param other The meta iterator with which to compare.
     * @return False if the two meta iterators refer to the same meta object,
     * true otherwise.
     */
    [[nodiscard]] bool operator!=(const meta_iterator &other) const ENTT_NOEXCEPT {
        return !(*this == other);
    }

private:
    internal::meta_iterator<node_type> it{};
};


/**
 * @brief Iterable range to use to iterate all types of meta objects.
 * @tparam Type Type of meta objects iterated.
 */
template<typename Type>
struct meta_range {
    /*! @brief Node type. */
    using node_type = typename Type::node_type;
    /*! @brief Input iterator type. */
    using iterator = meta_iterator<Type>;

    /*! @brief Default constructor. */
    meta_range() ENTT_NOEXCEPT = default;

    /**
     * @brief Constructs a meta range from a given node.
     * @param head The underlying node with which to construct the range.
     */
    meta_range(node_type *head)
        : node{head}
    {}

    /**
     * @brief Returns an iterator to the beginning.
     * @return An iterator to the first meta object of the range.
     */
    iterator begin() const ENTT_NOEXCEPT {
        return iterator{node};
    }

    /**
     * @brief Returns an iterator to the end.
     * @return An iterator to the element following the last meta object of the
     * range.
     */
    iterator end() const ENTT_NOEXCEPT {
        return iterator{};
    }

private:
    node_type *node{nullptr};
};


}


#endif

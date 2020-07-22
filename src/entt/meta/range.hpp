#ifndef ENTT_META_RANGE_HPP
#define ENTT_META_RANGE_HPP


#include "internal.hpp"


namespace entt {


/**
 * @brief Iterable range to use to iterate all types of meta objects.
 * @tparam Type Type of meta objects iterated.
 */
template<typename Type>
class meta_range {
    struct range_iterator {
        using difference_type = std::ptrdiff_t;
        using value_type = Type;
        using pointer = void;
        using reference = value_type;
        using iterator_category = std::input_iterator_tag;
        using node_type = typename Type::node_type;

        range_iterator() ENTT_NOEXCEPT = default;

        range_iterator(node_type *head) ENTT_NOEXCEPT
            : it{head}
        {}

        range_iterator & operator++() ENTT_NOEXCEPT {
            return ++it, *this;
        }

        range_iterator operator++(int) ENTT_NOEXCEPT {
            range_iterator orig = *this;
            return ++(*this), orig;
        }

        [[nodiscard]] reference operator*() const ENTT_NOEXCEPT {
            return it.operator->();
        }

        [[nodiscard]] bool operator==(const range_iterator &other) const ENTT_NOEXCEPT {
            return other.it == it;
        }

        [[nodiscard]] bool operator!=(const range_iterator &other) const ENTT_NOEXCEPT {
            return !(*this == other);
        }

    private:
        typename internal::meta_range<node_type>::iterator it{};
    };

public:
    /*! @brief Node type. */
    using node_type = typename Type::node_type;
    /*! @brief Input iterator type. */
    using iterator = range_iterator;

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
    [[nodiscard]] iterator begin() const ENTT_NOEXCEPT {
        return iterator{node};
    }

    /**
     * @brief Returns an iterator to the end.
     * @return An iterator to the element following the last meta object of the
     * range.
     */
    [[nodiscard]] iterator end() const ENTT_NOEXCEPT {
        return iterator{};
    }

private:
    node_type *node{nullptr};
};


}


#endif

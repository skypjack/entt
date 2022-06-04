#ifndef ENTT_GRAPH_ADJACENCY_MATRIX_HPP
#define ENTT_GRAPH_ADJACENCY_MATRIX_HPP

#include <cstddef>
#include <iterator>
#include <memory>
#include <type_traits>
#include <utility>
#include <vector>
#include "../config/config.h"
#include "../core/iterator.hpp"
#include "fwd.hpp"

namespace entt {

/**
 * @cond TURN_OFF_DOXYGEN
 * Internal details not to be documented.
 */

namespace internal {

struct edge_iterator {
    using value_type = std::pair<std::size_t, std::size_t>;
    using pointer = input_iterator_pointer<value_type>;
    using reference = value_type;
    using difference_type = std::ptrdiff_t;
    using iterator_category = std::input_iterator_tag;

    constexpr edge_iterator() noexcept
        : matrix{},
          vert{},
          it{},
          last{} {}

    constexpr edge_iterator(const std::size_t *ref, const std::size_t vertices, const std::size_t from) noexcept
        : edge_iterator{ref, vertices, from, vertices * vertices} {}

    constexpr edge_iterator(const std::size_t *ref, const std::size_t vertices, const std::size_t from, const std::size_t to) noexcept
        : matrix{ref},
          vert{vertices},
          it{from},
          last{to} {
        for(; it != last && !matrix[it]; ++it) {}
    }

    constexpr edge_iterator &operator++() noexcept {
        for(++it; it != last && !matrix[it]; ++it) {}
        return *this;
    }

    constexpr edge_iterator operator++(int) noexcept {
        edge_iterator orig = *this;
        return ++(*this), orig;
    }

    [[nodiscard]] constexpr reference operator*() const noexcept {
        return *operator->();
    }

    [[nodiscard]] constexpr pointer operator->() const noexcept {
        return std::make_pair<std::size_t>(it / vert, it % vert);
    }

    friend constexpr bool operator==(const edge_iterator &, const edge_iterator &) noexcept;

private:
    const std::size_t *matrix;
    std::size_t vert;
    std::size_t it;
    std::size_t last;
};

[[nodiscard]] inline constexpr bool operator==(const edge_iterator &lhs, const edge_iterator &rhs) noexcept {
    return lhs.it == rhs.it;
}

[[nodiscard]] inline constexpr bool operator!=(const edge_iterator &lhs, const edge_iterator &rhs) noexcept {
    return !(lhs == rhs);
}

} // namespace internal

/**
 * Internal details not to be documented.
 * @endcond
 */

/**
 * @brief Basic implementation of a directed adjacency matrix.
 * @tparam Allocator Type of allocator used to manage memory and elements.
 */
template<typename Allocator>
class basic_adjacency_matrix {
    using alloc_traits = std::allocator_traits<Allocator>;
    static_assert(std::is_same_v<typename alloc_traits::value_type, std::size_t>, "Invalid value type");
    using container_type = std::vector<std::size_t, typename alloc_traits::template rebind_alloc<std::size_t>>;

public:
    /*! @brief Allocator type. */
    using allocator_type = Allocator;
    /*! @brief Unsigned integer type. */
    using size_type = std::size_t;
    /*! @brief Vertex type. */
    using vertex_type = size_type;
    /*! @brief Edge type. */
    using edge_type = std::pair<vertex_type, vertex_type>;
    /*! @brief Vertex iterator type. */
    using vertex_iterator = iota_iterator<vertex_type>;
    /*! @brief Edge iterator type. */
    using edge_iterator = internal::edge_iterator;

    /*! @brief Default constructor. */
    basic_adjacency_matrix() noexcept(noexcept(allocator_type{}))
        : basic_adjacency_matrix{0u} {}

    /**
     * @brief Constructs an empty container with a given allocator.
     * @param allocator The allocator to use.
     */
    explicit basic_adjacency_matrix(const allocator_type &allocator) noexcept
        : basic_adjacency_matrix{0u, allocator} {}

    /**
     * @brief Constructs an empty container with a given allocator and user
     * supplied number of vertices.
     * @param vertices Number of vertices.
     * @param allocator The allocator to use.
     */
    basic_adjacency_matrix(const size_type vertices, const allocator_type &allocator = allocator_type{})
        : matrix(vertices * vertices),
          vert{vertices} {}

    /**
     * @brief Copy constructor.
     * @param other The instance to copy from.
     */
    basic_adjacency_matrix(const basic_adjacency_matrix &other)
        : basic_adjacency_matrix{other, other.get_allocator()} {}

    /**
     * @brief Allocator-extended copy constructor.
     * @param other The instance to copy from.
     * @param allocator The allocator to use.
     */
    basic_adjacency_matrix(const basic_adjacency_matrix &other, const allocator_type &allocator)
        : matrix{other.matrix, allocator},
          vert{other.vert} {}

    /**
     * @brief Move constructor.
     * @param other The instance to move from.
     */
    basic_adjacency_matrix(basic_adjacency_matrix &&other) noexcept
        : basic_adjacency_matrix{std::move(other), other.get_allocator()} {}

    /**
     * @brief Allocator-extended move constructor.
     * @param other The instance to move from.
     * @param allocator The allocator to use.
     */
    basic_adjacency_matrix(basic_adjacency_matrix &&other, const allocator_type &allocator)
        : matrix{std::move(other.matrix), allocator},
          vert{std::exchange(other.vert, 0u)} {}

    /**
     * @brief Default copy assignment operator.
     * @param other The instance to copy from.
     * @return This container.
     */
    basic_adjacency_matrix &operator=(const basic_adjacency_matrix &other) {
        matrix = other.matrix;
        vert = other.vert;
        return *this;
    }

    /**
     * @brief Default move assignment operator.
     * @param other The instance to move from.
     * @return This container.
     */
    basic_adjacency_matrix &operator=(basic_adjacency_matrix &&other) noexcept {
        matrix = std::move(other.matrix);
        vert = std::exchange(other.vert, 0u);
        return *this;
    }

    /**
     * @brief Returns the associated allocator.
     * @return The associated allocator.
     */
    [[nodiscard]] constexpr allocator_type get_allocator() const noexcept {
        return matrix.get_allocator();
    }

    /*! @brief Clears the adjacency matrix. */
    void clear() noexcept {
        matrix.clear();
        vert = {};
    }

    /**
     * @brief Exchanges the contents with those of a given adjacency matrix.
     * @param other Adjacency matrix to exchange the content with.
     */
    void swap(adjacency_matrix &other) {
        using std::swap;
        swap(matrix, other.matrix);
        swap(vert, other.vert);
    }

    /**
     * @brief Returns the number of vertices.
     * @return The number of vertices.
     */
    [[nodiscard]] size_type size() const noexcept {
        return vert;
    }

    /**
     * @brief Returns an iterable object to visit all vertices of a matrix.
     * @return An iterable object to visit all vertices of a matrix.
     */
    [[nodiscard]] iterable_adaptor<vertex_iterator> vertices() const noexcept {
        return {0u, vert};
    }

    /**
     * @brief Returns an iterable object to visit all edges of a matrix.
     * @return An iterable object to visit all edges of a matrix.
     */
    [[nodiscard]] iterable_adaptor<edge_iterator> edges() const noexcept {
        return {{matrix.data(), vert, 0u}, {matrix.data(), vert, matrix.size()}};
    }

    /**
     * @brief Returns an iterable object to visit all edges of a vertex.
     * @param vertex The vertex of which to return all edges.
     * @return An iterable object to visit all edges of a vertex.
     */
    [[nodiscard]] iterable_adaptor<edge_iterator> edges(const vertex_type vertex) const noexcept {
        const auto first = vertex * vert;
        const auto last = vertex * vert + vert;
        return {{matrix.data(), vert, first, last}, {matrix.data(), vert, last, last}};
    }

    /**
     * @brief Resizes an adjacency matrix.
     * @param vertices The new number of vertices.
     */
    void resize(const size_type vertices) {
        matrix.resize(vertices * vertices);
        vert = vertices;
    }

    /**
     * @brief Inserts an edge into the adjacency matrix, if it does not exist.
     * @param lhs The left hand vertex of the edge.
     * @param rhs The right hand vertex of the edge.
     * @return A pair consisting of an iterator to the inserted element (or to
     * the element that prevented the insertion) and a bool denoting whether the
     * insertion took place.
     */
    std::pair<edge_iterator, bool> insert(const vertex_type lhs, const vertex_type rhs) {
        const auto pos = lhs * vert + rhs;
        const auto inserted = !std::exchange(matrix[pos], 1u);
        return {edge_iterator{matrix.data(), vert, pos}, inserted};
    }

    /**
     * @brief Removes the edge associated with a pair of given vertices.
     * @param lhs The left hand vertex of the edge.
     * @param rhs The right hand vertex of the edge.
     * @return Number of elements removed (either 0 or 1).
     */
    size_type erase(const vertex_type lhs, const vertex_type rhs) {
        return std::exchange(matrix[lhs * vert + rhs], 0u);
    }

    /**
     * @brief Checks if an adjacency matrix contains a given edge.
     * @param lhs The left hand vertex of the edge.
     * @param rhs The right hand vertex of the edge.
     * @return True if there is such an edge, false otherwise.
     */
    [[nodiscard]] bool contains(const vertex_type lhs, const vertex_type rhs) const {
        const auto pos = lhs * vert + rhs;
        return pos < matrix.size() && matrix[pos];
    }

private:
    container_type matrix;
    size_type vert;
};

} // namespace entt

#endif

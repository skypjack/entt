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

/*! @cond TURN_OFF_DOXYGEN */
namespace internal {

template<typename It>
class edge_iterator {
    using size_type = std::size_t;

public:
    using value_type = std::pair<size_type, size_type>;
    using pointer = input_iterator_pointer<value_type>;
    using reference = value_type;
    using difference_type = std::ptrdiff_t;
    using iterator_category = std::input_iterator_tag;
    using iterator_concept = std::forward_iterator_tag;

    constexpr edge_iterator() noexcept = default;

    constexpr edge_iterator(It base, const size_type vertices, const size_type from, const size_type to, const size_type step) noexcept
        : it{std::move(base)},
          vert{vertices},
          pos{from},
          last{to},
          offset{step} {
        for(; pos != last && !it[pos]; pos += offset) {}
    }

    constexpr edge_iterator &operator++() noexcept {
        for(pos += offset; pos != last && !it[pos]; pos += offset) {}
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
        return std::make_pair<size_type>(pos / vert, pos % vert);
    }

    template<typename Type>
    friend constexpr bool operator==(const edge_iterator<Type> &, const edge_iterator<Type> &) noexcept;

private:
    It it{};
    size_type vert{};
    size_type pos{};
    size_type last{};
    size_type offset{};
};

template<typename Container>
[[nodiscard]] inline constexpr bool operator==(const edge_iterator<Container> &lhs, const edge_iterator<Container> &rhs) noexcept {
    return lhs.pos == rhs.pos;
}

template<typename Container>
[[nodiscard]] inline constexpr bool operator!=(const edge_iterator<Container> &lhs, const edge_iterator<Container> &rhs) noexcept {
    return !(lhs == rhs);
}

} // namespace internal
/*! @endcond */

/**
 * @brief Basic implementation of a directed adjacency matrix.
 * @tparam Category Either a directed or undirected category tag.
 * @tparam Allocator Type of allocator used to manage memory and elements.
 */
template<typename Category, typename Allocator>
class adjacency_matrix {
    using alloc_traits = std::allocator_traits<Allocator>;
    static_assert(std::is_base_of_v<directed_tag, Category>, "Invalid graph category");
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
    using edge_iterator = internal::edge_iterator<typename container_type::const_iterator>;
    /*! @brief Out-edge iterator type. */
    using out_edge_iterator = edge_iterator;
    /*! @brief In-edge iterator type. */
    using in_edge_iterator = edge_iterator;
    /*! @brief Graph category tag. */
    using graph_category = Category;

    /*! @brief Default constructor. */
    adjacency_matrix() noexcept(noexcept(allocator_type{}))
        : adjacency_matrix{0u} {
    }

    /**
     * @brief Constructs an empty container with a given allocator.
     * @param allocator The allocator to use.
     */
    explicit adjacency_matrix(const allocator_type &allocator) noexcept
        : adjacency_matrix{0u, allocator} {}

    /**
     * @brief Constructs an empty container with a given allocator and user
     * supplied number of vertices.
     * @param vertices Number of vertices.
     * @param allocator The allocator to use.
     */
    adjacency_matrix(const size_type vertices, const allocator_type &allocator = allocator_type{})
        : matrix{vertices * vertices, allocator},
          vert{vertices} {}

    /*! @brief Default copy constructor. */
    adjacency_matrix(const adjacency_matrix &) = default;

    /**
     * @brief Allocator-extended copy constructor.
     * @param other The instance to copy from.
     * @param allocator The allocator to use.
     */
    adjacency_matrix(const adjacency_matrix &other, const allocator_type &allocator)
        : matrix{other.matrix, allocator},
          vert{other.vert} {}

    /*! @brief Default move constructor. */
    adjacency_matrix(adjacency_matrix &&) noexcept = default;

    /**
     * @brief Allocator-extended move constructor.
     * @param other The instance to move from.
     * @param allocator The allocator to use.
     */
    adjacency_matrix(adjacency_matrix &&other, const allocator_type &allocator)
        : matrix{std::move(other.matrix), allocator},
          vert{other.vert} {}

    /*! @brief Default destructor. */
    ~adjacency_matrix() noexcept = default;

    /**
     * @brief Default copy assignment operator.
     * @return This container.
     */
    adjacency_matrix &operator=(const adjacency_matrix &) = default;

    /**
     * @brief Default move assignment operator.
     * @return This container.
     */
    adjacency_matrix &operator=(adjacency_matrix &&) noexcept = default;

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
     * @brief Returns true if an adjacency matrix is empty, false otherwise.
     *
     * @warning
     * Potentially expensive, try to avoid it on hot paths.
     *
     * @return True if the adjacency matrix is empty, false otherwise.
     */
    [[nodiscard]] bool empty() const noexcept {
        const auto iterable = edges();
        return (iterable.begin() == iterable.end());
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
        const auto it = matrix.cbegin();
        const auto sz = matrix.size();
        return {{it, vert, 0u, sz, 1u}, {it, vert, sz, sz, 1u}};
    }

    /**
     * @brief Returns an iterable object to visit all out-edges of a vertex.
     * @param vertex The vertex of which to return all out-edges.
     * @return An iterable object to visit all out-edges of a vertex.
     */
    [[nodiscard]] iterable_adaptor<out_edge_iterator> out_edges(const vertex_type vertex) const noexcept {
        const auto it = matrix.cbegin();
        const auto from = vertex * vert;
        const auto to = from + vert;
        return {{it, vert, from, to, 1u}, {it, vert, to, to, 1u}};
    }

    /**
     * @brief Returns an iterable object to visit all in-edges of a vertex.
     * @param vertex The vertex of which to return all in-edges.
     * @return An iterable object to visit all in-edges of a vertex.
     */
    [[nodiscard]] iterable_adaptor<in_edge_iterator> in_edges(const vertex_type vertex) const noexcept {
        const auto it = matrix.cbegin();
        const auto from = vertex;
        const auto to = vert * vert + from;
        return {{it, vert, from, to, vert}, {it, vert, to, to, vert}};
    }

    /**
     * @brief Resizes an adjacency matrix.
     * @param vertices The new number of vertices.
     */
    void resize(const size_type vertices) {
        adjacency_matrix other{vertices, get_allocator()};

        for(auto [lhs, rhs]: edges()) {
            other.insert(lhs, rhs);
        }

        other.swap(*this);
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

        if constexpr(std::is_same_v<graph_category, undirected_tag>) {
            const auto rev = rhs * vert + lhs;
            ENTT_ASSERT(matrix[pos] == matrix[rev], "Something went really wrong");
            matrix[rev] = 1u;
        }

        const auto inserted = !std::exchange(matrix[pos], 1u);
        return {edge_iterator{matrix.cbegin(), vert, pos, matrix.size(), 1u}, inserted};
    }

    /**
     * @brief Removes the edge associated with a pair of given vertices.
     * @param lhs The left hand vertex of the edge.
     * @param rhs The right hand vertex of the edge.
     * @return Number of elements removed (either 0 or 1).
     */
    size_type erase(const vertex_type lhs, const vertex_type rhs) {
        const auto pos = lhs * vert + rhs;

        if constexpr(std::is_same_v<graph_category, undirected_tag>) {
            const auto rev = rhs * vert + lhs;
            ENTT_ASSERT(matrix[pos] == matrix[rev], "Something went really wrong");
            matrix[rev] = 0u;
        }

        return std::exchange(matrix[pos], 0u);
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

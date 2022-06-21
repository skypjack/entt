#include <memory>
#include <utility>
#include <gtest/gtest.h>
#include <entt/graph/adjacency_matrix.hpp>
#include "../common/throwing_allocator.hpp"

TEST(AdjacencyMatrix, Resize) {
    entt::adjacency_matrix<entt::directed_tag> adjacency_matrix{2};
    adjacency_matrix.insert(1u, 0u);

    ASSERT_EQ(adjacency_matrix.size(), 2u);
    ASSERT_TRUE(adjacency_matrix.contains(1u, 0u));

    adjacency_matrix.resize(3u);

    ASSERT_EQ(adjacency_matrix.size(), 3u);
    ASSERT_TRUE(adjacency_matrix.contains(1u, 0u));
}

TEST(AdjacencyMatrix, Constructors) {
    entt::adjacency_matrix<entt::directed_tag> adjacency_matrix{};

    ASSERT_EQ(adjacency_matrix.size(), 0u);

    adjacency_matrix = entt::adjacency_matrix<entt::directed_tag>{std::allocator<bool>{}};
    adjacency_matrix = entt::adjacency_matrix<entt::directed_tag>{3u, std::allocator<bool>{}};

    ASSERT_EQ(adjacency_matrix.size(), 3u);

    adjacency_matrix.insert(0u, 1u);

    entt::adjacency_matrix<entt::directed_tag> temp{adjacency_matrix, adjacency_matrix.get_allocator()};
    entt::adjacency_matrix<entt::directed_tag> other{std::move(adjacency_matrix), adjacency_matrix.get_allocator()};

    ASSERT_EQ(adjacency_matrix.size(), 0u);
    ASSERT_EQ(other.size(), 3u);

    ASSERT_FALSE(adjacency_matrix.contains(0u, 1u));
    ASSERT_TRUE(other.contains(0u, 1u));
}

TEST(AdjacencyMatrix, Copy) {
    entt::adjacency_matrix<entt::directed_tag> adjacency_matrix{3u};
    adjacency_matrix.insert(0u, 1u);

    entt::adjacency_matrix<entt::directed_tag> other{adjacency_matrix};

    ASSERT_EQ(adjacency_matrix.size(), 3u);
    ASSERT_EQ(other.size(), 3u);

    ASSERT_TRUE(adjacency_matrix.contains(0u, 1u));
    ASSERT_TRUE(other.contains(0u, 1u));

    adjacency_matrix.resize(4u);
    adjacency_matrix.insert(0u, 2u);
    other.insert(1u, 2u);

    other = adjacency_matrix;

    ASSERT_EQ(other.size(), 4u);
    ASSERT_EQ(adjacency_matrix.size(), 4u);

    ASSERT_TRUE(other.contains(0u, 1u));
    ASSERT_FALSE(other.contains(1u, 2u));
    ASSERT_TRUE(other.contains(0u, 2u));
}

TEST(AdjacencyMatrix, Move) {
    entt::adjacency_matrix<entt::directed_tag> adjacency_matrix{3u};
    adjacency_matrix.insert(0u, 1u);

    entt::adjacency_matrix<entt::directed_tag> other{std::move(adjacency_matrix)};

    ASSERT_EQ(adjacency_matrix.size(), 0u);
    ASSERT_EQ(other.size(), 3u);

    ASSERT_FALSE(adjacency_matrix.contains(0u, 1u));
    ASSERT_TRUE(other.contains(0u, 1u));

    adjacency_matrix = {};
    adjacency_matrix.resize(4u);
    adjacency_matrix.insert(0u, 2u);
    other.insert(1u, 2u);

    other = std::move(adjacency_matrix);

    ASSERT_EQ(other.size(), 4u);
    ASSERT_EQ(adjacency_matrix.size(), 0u);

    ASSERT_FALSE(other.contains(0u, 1u));
    ASSERT_FALSE(other.contains(1u, 2u));
    ASSERT_TRUE(other.contains(0u, 2u));
}

TEST(AdjacencyMatrix, Swap) {
    entt::adjacency_matrix<entt::directed_tag> adjacency_matrix{3u};
    entt::adjacency_matrix<entt::directed_tag> other{};

    adjacency_matrix.insert(0u, 1u);

    ASSERT_EQ(other.size(), 0u);
    ASSERT_EQ(adjacency_matrix.size(), 3u);
    ASSERT_TRUE(adjacency_matrix.contains(0u, 1u));
    ASSERT_FALSE(other.contains(0u, 1u));

    adjacency_matrix.swap(other);

    ASSERT_EQ(other.size(), 3u);
    ASSERT_EQ(adjacency_matrix.size(), 0u);
    ASSERT_FALSE(adjacency_matrix.contains(0u, 1u));
    ASSERT_TRUE(other.contains(0u, 1u));
}

TEST(AdjacencyMatrix, InsertDirected) {
    entt::adjacency_matrix<entt::directed_tag> adjacency_matrix{3u};

    auto first = adjacency_matrix.insert(0u, 1u);
    auto second = adjacency_matrix.insert(0u, 2u);
    auto other = adjacency_matrix.insert(0u, 1u);

    ASSERT_TRUE(first.second);
    ASSERT_TRUE(second.second);
    ASSERT_FALSE(other.second);

    ASSERT_NE(first.first, second.first);
    ASSERT_EQ(first.first, other.first);

    ASSERT_EQ(*first.first, std::make_pair(std::size_t{0u}, std::size_t{1u}));
    ASSERT_EQ(*second.first, std::make_pair(std::size_t{0u}, std::size_t{2u}));

    ASSERT_TRUE(adjacency_matrix.contains(0u, 1u));
    ASSERT_FALSE(adjacency_matrix.contains(2u, 0u));
}

TEST(AdjacencyMatrix, InsertUndirected) {
    entt::adjacency_matrix<entt::undirected_tag> adjacency_matrix{3u};

    auto first = adjacency_matrix.insert(0u, 1u);
    auto second = adjacency_matrix.insert(0u, 2u);
    auto other = adjacency_matrix.insert(0u, 1u);

    ASSERT_TRUE(first.second);
    ASSERT_TRUE(second.second);
    ASSERT_FALSE(other.second);

    ASSERT_NE(first.first, second.first);
    ASSERT_EQ(first.first, other.first);

    ASSERT_EQ(*first.first, std::make_pair(std::size_t{0u}, std::size_t{1u}));
    ASSERT_EQ(*second.first, std::make_pair(std::size_t{0u}, std::size_t{2u}));

    ASSERT_TRUE(adjacency_matrix.contains(0u, 1u));
    ASSERT_TRUE(adjacency_matrix.contains(2u, 0u));
}

TEST(AdjacencyMatrix, EraseDirected) {
    entt::adjacency_matrix<entt::directed_tag> adjacency_matrix{3u};

    adjacency_matrix.insert(0u, 1u);

    ASSERT_TRUE(adjacency_matrix.contains(0u, 1u));
    ASSERT_FALSE(adjacency_matrix.contains(1u, 0u));

    ASSERT_EQ(adjacency_matrix.erase(0u, 1u), 1u);
    ASSERT_EQ(adjacency_matrix.erase(0u, 1u), 0u);

    ASSERT_FALSE(adjacency_matrix.contains(0u, 1u));
    ASSERT_FALSE(adjacency_matrix.contains(1u, 0u));
}

TEST(AdjacencyMatrix, EraseUndirected) {
    entt::adjacency_matrix<entt::undirected_tag> adjacency_matrix{3u};

    adjacency_matrix.insert(0u, 1u);

    ASSERT_TRUE(adjacency_matrix.contains(0u, 1u));
    ASSERT_TRUE(adjacency_matrix.contains(1u, 0u));

    ASSERT_EQ(adjacency_matrix.erase(0u, 1u), 1u);
    ASSERT_EQ(adjacency_matrix.erase(0u, 1u), 0u);

    ASSERT_FALSE(adjacency_matrix.contains(0u, 1u));
    ASSERT_FALSE(adjacency_matrix.contains(1u, 0u));
}

TEST(AdjacencyMatrix, Clear) {
    entt::adjacency_matrix<entt::directed_tag> adjacency_matrix{3u};

    adjacency_matrix.insert(0u, 1u);
    adjacency_matrix.insert(0u, 2u);

    ASSERT_TRUE(adjacency_matrix.contains(0u, 1u));
    ASSERT_TRUE(adjacency_matrix.contains(0u, 2u));
    ASSERT_EQ(adjacency_matrix.size(), 3u);

    adjacency_matrix.clear();

    ASSERT_FALSE(adjacency_matrix.contains(0u, 1u));
    ASSERT_FALSE(adjacency_matrix.contains(0u, 2u));
    ASSERT_EQ(adjacency_matrix.size(), 0u);
}

TEST(AdjacencyMatrix, VertexIterator) {
    using iterator = typename entt::adjacency_matrix<entt::directed_tag>::vertex_iterator;

    static_assert(std::is_same_v<iterator::value_type, std::size_t>);
    static_assert(std::is_same_v<iterator::pointer, void>);
    static_assert(std::is_same_v<iterator::reference, std::size_t>);

    entt::adjacency_matrix<entt::directed_tag> adjacency_matrix{2u};
    const auto iterable = adjacency_matrix.vertices();

    iterator end{iterable.begin()};
    iterator begin{};

    begin = iterable.end();
    std::swap(begin, end);

    ASSERT_EQ(begin, iterable.begin());
    ASSERT_EQ(end, iterable.end());
    ASSERT_NE(begin, end);

    ASSERT_EQ(*begin, 0u);
    ASSERT_EQ(begin++, iterable.begin());
    ASSERT_EQ(*begin, 1u);
    ASSERT_EQ(++begin, iterable.end());
}

TEST(AdjacencyMatrix, EdgeIterator) {
    using iterator = typename entt::adjacency_matrix<entt::directed_tag>::edge_iterator;

    static_assert(std::is_same_v<iterator::value_type, std::pair<std::size_t, std::size_t>>);
    static_assert(std::is_same_v<iterator::pointer, entt::input_iterator_pointer<std::pair<std::size_t, std::size_t>>>);
    static_assert(std::is_same_v<iterator::reference, std::pair<std::size_t, std::size_t>>);

    entt::adjacency_matrix<entt::directed_tag> adjacency_matrix{3u};

    adjacency_matrix.insert(0u, 1u);
    adjacency_matrix.insert(0u, 2u);

    const auto iterable = adjacency_matrix.edges();

    iterator end{iterable.begin()};
    iterator begin{};

    begin = iterable.end();
    std::swap(begin, end);

    ASSERT_EQ(begin, iterable.begin());
    ASSERT_EQ(end, iterable.end());
    ASSERT_NE(begin, end);

    ASSERT_EQ(*begin, std::make_pair(std::size_t{0u}, std::size_t{1u}));
    ASSERT_EQ(begin++, iterable.begin());
    ASSERT_EQ(*begin.operator->(), std::make_pair(std::size_t{0u}, std::size_t{2u}));
    ASSERT_EQ(++begin, iterable.end());
}

TEST(AdjacencyMatrix, Vertices) {
    entt::adjacency_matrix<entt::directed_tag> adjacency_matrix{};
    auto iterable = adjacency_matrix.vertices();

    ASSERT_EQ(adjacency_matrix.size(), 0u);
    ASSERT_EQ(iterable.begin(), iterable.end());

    adjacency_matrix.resize(2u);
    iterable = adjacency_matrix.vertices();

    ASSERT_EQ(adjacency_matrix.size(), 2u);
    ASSERT_NE(iterable.begin(), iterable.end());

    auto it = iterable.begin();

    ASSERT_EQ(*it++, 0u);
    ASSERT_EQ(*it, 1u);
    ASSERT_EQ(++it, iterable.end());
}

TEST(AdjacencyMatrix, EdgesDirected) {
    entt::adjacency_matrix<entt::directed_tag> adjacency_matrix{3u};
    auto iterable = adjacency_matrix.edges();

    ASSERT_EQ(iterable.begin(), iterable.end());

    adjacency_matrix.insert(0u, 1u);
    adjacency_matrix.insert(1u, 2u);
    iterable = adjacency_matrix.edges();

    ASSERT_NE(iterable.begin(), iterable.end());

    auto it = iterable.begin();

    ASSERT_EQ(*it++, std::make_pair(std::size_t{0u}, std::size_t{1u}));
    ASSERT_EQ(*it.operator->(), std::make_pair(std::size_t{1u}, std::size_t{2u}));
    ASSERT_EQ(++it, iterable.end());
}

TEST(AdjacencyMatrix, EdgesUndirected) {
    entt::adjacency_matrix<entt::undirected_tag> adjacency_matrix{3u};
    auto iterable = adjacency_matrix.edges();

    ASSERT_EQ(iterable.begin(), iterable.end());

    adjacency_matrix.insert(0u, 1u);
    adjacency_matrix.insert(1u, 2u);
    iterable = adjacency_matrix.edges();

    ASSERT_NE(iterable.begin(), iterable.end());

    auto it = iterable.begin();

    ASSERT_EQ(*it++, std::make_pair(std::size_t{0u}, std::size_t{1u}));
    ASSERT_EQ(*it.operator->(), std::make_pair(std::size_t{1u}, std::size_t{0u}));
    ASSERT_EQ(*(++it).operator->(), std::make_pair(std::size_t{1u}, std::size_t{2u}));
    ASSERT_EQ(*++it, std::make_pair(std::size_t{2u}, std::size_t{1u}));

    ASSERT_EQ(++it, iterable.end());
}

TEST(AdjacencyMatrix, OutEdgesDirected) {
    entt::adjacency_matrix<entt::directed_tag> adjacency_matrix{3u};
    auto iterable = adjacency_matrix.out_edges(0u);

    ASSERT_EQ(iterable.begin(), iterable.end());

    adjacency_matrix.insert(0u, 1u);
    adjacency_matrix.insert(1u, 2u);
    iterable = adjacency_matrix.out_edges(0u);

    ASSERT_NE(iterable.begin(), iterable.end());

    auto it = iterable.begin();

    ASSERT_EQ(*it++, std::make_pair(std::size_t{0u}, std::size_t{1u}));
    ASSERT_EQ(it, iterable.end());

    iterable = adjacency_matrix.out_edges(2u);
    it = iterable.cbegin();

    ASSERT_EQ(it, iterable.cend());
}

TEST(AdjacencyMatrix, OutEdgesUndirected) {
    entt::adjacency_matrix<entt::undirected_tag> adjacency_matrix{3u};
    auto iterable = adjacency_matrix.out_edges(0u);

    ASSERT_EQ(iterable.begin(), iterable.end());

    adjacency_matrix.insert(0u, 1u);
    adjacency_matrix.insert(1u, 2u);
    iterable = adjacency_matrix.out_edges(0u);

    ASSERT_NE(iterable.begin(), iterable.end());

    auto it = iterable.begin();

    ASSERT_EQ(*it++, std::make_pair(std::size_t{0u}, std::size_t{1u}));
    ASSERT_EQ(it, iterable.end());

    iterable = adjacency_matrix.out_edges(2u);
    it = iterable.cbegin();

    ASSERT_NE(it, iterable.cend());
    ASSERT_EQ(*it++, std::make_pair(std::size_t{2u}, std::size_t{1u}));
    ASSERT_EQ(it, iterable.cend());
}

TEST(AdjacencyMatrix, InEdgesDirected) {
    entt::adjacency_matrix<entt::directed_tag> adjacency_matrix{3u};
    auto iterable = adjacency_matrix.in_edges(1u);

    ASSERT_EQ(iterable.begin(), iterable.end());

    adjacency_matrix.insert(0u, 1u);
    adjacency_matrix.insert(1u, 2u);
    iterable = adjacency_matrix.in_edges(1u);

    ASSERT_NE(iterable.begin(), iterable.end());

    auto it = iterable.begin();

    ASSERT_EQ(*it++, std::make_pair(std::size_t{0u}, std::size_t{1u}));
    ASSERT_EQ(it, iterable.end());

    iterable = adjacency_matrix.in_edges(0u);
    it = iterable.cbegin();

    ASSERT_EQ(it, iterable.cend());
}

TEST(AdjacencyMatrix, InEdgesUndirected) {
    entt::adjacency_matrix<entt::undirected_tag> adjacency_matrix{3u};
    auto iterable = adjacency_matrix.in_edges(1u);

    ASSERT_EQ(iterable.begin(), iterable.end());

    adjacency_matrix.insert(0u, 1u);
    adjacency_matrix.insert(1u, 2u);
    iterable = adjacency_matrix.in_edges(1u);

    ASSERT_NE(iterable.begin(), iterable.end());

    auto it = iterable.begin();

    ASSERT_EQ(*it++, std::make_pair(std::size_t{0u}, std::size_t{1u}));
    ASSERT_EQ(it, iterable.end());

    iterable = adjacency_matrix.in_edges(0u);
    it = iterable.cbegin();

    ASSERT_NE(it, iterable.cend());
    ASSERT_EQ(*it++, std::make_pair(std::size_t{1u}, std::size_t{0u}));
    ASSERT_EQ(it, iterable.cend());
}

TEST(AdjacencyMatrix, ThrowingAllocator) {
    using allocator = test::throwing_allocator<std::size_t>;
    using exception = typename allocator::exception_type;

    entt::adjacency_matrix<entt::directed_tag, allocator> adjacency_matrix{2u};
    adjacency_matrix.insert(0u, 1u);

    allocator::trigger_on_allocate = true;

    ASSERT_EQ(adjacency_matrix.size(), 2u);
    ASSERT_TRUE(adjacency_matrix.contains(0u, 1u));

    ASSERT_THROW(adjacency_matrix.resize(4u), exception);

    ASSERT_EQ(adjacency_matrix.size(), 2u);
    ASSERT_TRUE(adjacency_matrix.contains(0u, 1u));
}

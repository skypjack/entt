#include <cstddef>
#include <sstream>
#include <string>
#include <gtest/gtest.h>
#include <entt/graph/adjacency_matrix.hpp>
#include <entt/graph/dot.hpp>

TEST(Dot, DirectedGraph) {
    std::ostringstream output{};
    entt::adjacency_matrix<entt::directed_tag> adjacency_matrix{3u};

    adjacency_matrix.insert(0u, 1u);
    adjacency_matrix.insert(1u, 2u);
    adjacency_matrix.insert(0u, 2u);

    entt::dot(output, adjacency_matrix);

    const std::string expected = "digraph{0[];1[];2[];0->1;0->2;1->2;}";
    const auto str = output.str();

    ASSERT_FALSE(str.empty());
    ASSERT_EQ(str, expected);
}

TEST(Dot, UndirectedGraph) {
    std::ostringstream output{};
    entt::adjacency_matrix<entt::undirected_tag> adjacency_matrix{3u};

    adjacency_matrix.insert(0u, 1u);
    adjacency_matrix.insert(1u, 2u);
    adjacency_matrix.insert(0u, 2u);

    entt::dot(output, adjacency_matrix);

    const std::string expected = "graph{0[];1[];2[];0--1;0--2;1--0;1--2;2--0;2--1;}";
    const auto str = output.str();

    ASSERT_FALSE(str.empty());
    ASSERT_EQ(str, expected);
}

TEST(Dot, CustomWriter) {
    std::ostringstream output{};
    entt::adjacency_matrix<entt::directed_tag> adjacency_matrix{3u};

    adjacency_matrix.insert(0u, 1u);
    adjacency_matrix.insert(1u, 2u);
    adjacency_matrix.insert(0u, 2u);

    entt::dot(output, adjacency_matrix, [&adjacency_matrix](std::ostream &out, std::size_t vertex) {
        out << "label=\"v" << vertex << "\"";

        if(auto in_edges = adjacency_matrix.in_edges(vertex); in_edges.cbegin() == in_edges.cend()) {
            out << ",shape=\"box\"";
        }
    });

    const std::string expected = R"(digraph{0[label="v0",shape="box"];1[label="v1"];2[label="v2"];0->1;0->2;1->2;})";
    const auto str = output.str();

    ASSERT_FALSE(str.empty());
    ASSERT_EQ(str, expected);
}

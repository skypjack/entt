#ifndef ENTT_GRAPH_DOT_HPP
#define ENTT_GRAPH_DOT_HPP

#include <ostream>
#include "adjacency_matrix.hpp"

namespace entt {

/**
 * @brief Outputs a graph in dot format.
 * @tparam Graph Graph type, valid as long as it exposes edges and vertices.
 * @tparam Writer Vertex decorator type.
 * @param out A standard output stream.
 * @param graph The graph to output.
 * @param writer Vertex decorator object.
 */
template<typename Graph, typename Writer>
void dot(std::ostream &out, const Graph &graph, Writer writer) {
    out << "digraph{";

    for(auto &&vertex: graph.vertices()) {
        out << vertex << "[";
        writer(out, vertex);
        out << "];";
    }

    for(auto [lhs, rhs]: graph.edges()) {
        out << lhs << "->" << rhs << ";";
    }

    out << "}";
}

/**
 * @brief Outputs a graph in dot format.
 * @tparam Graph Graph type, valid as long as it exposes edges and vertices.
 * @param out A standard output stream.
 * @param graph The graph to output.
 */
template<typename Graph>
void dot(std::ostream &out, const Graph &graph) {
    return dot(out, graph, [](auto &&...) {});
}

} // namespace entt

#endif

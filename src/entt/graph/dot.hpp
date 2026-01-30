#ifndef ENTT_GRAPH_DOT_HPP
#define ENTT_GRAPH_DOT_HPP

#include <concepts>
#include <ostream>
#include "fwd.hpp"

namespace entt {

/**
 * @brief Outputs a graph in dot format.
 * @tparam Graph Graph type, valid as long as it exposes edges and vertices.
 * @param out A standard output stream.
 * @param graph The graph to output.
 * @param writer Vertex decorator object.
 */
template<typename Graph>
requires std::derived_from<typename Graph::graph_category, directed_tag>
void dot(std::ostream &out, const Graph &graph, std::invocable<std::ostream &, typename Graph::vertex_type> auto writer) {
    if constexpr(std::same_as<typename Graph::graph_category, undirected_tag>) {
        out << "graph{";
    } else {
        out << "digraph{";
    }

    for(auto &&vertex: graph.vertices()) {
        out << vertex << "[";
        writer(out, vertex);
        out << "];";
    }

    for(auto [lhs, rhs]: graph.edges()) {
        if constexpr(std::same_as<typename Graph::graph_category, undirected_tag>) {
            out << lhs << "--" << rhs << ";";
        } else {
            out << lhs << "->" << rhs << ";";
        }
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

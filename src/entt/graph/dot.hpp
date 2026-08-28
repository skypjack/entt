#ifndef ENTT_GRAPH_DOT_HPP
#define ENTT_GRAPH_DOT_HPP

#include "../stl/concepts.hpp"
#include "fwd.hpp"

namespace entt {

/**
 * @brief Outputs a graph in dot format.
 * @tparam Out Type of the generic data stream where the data are written.
 * @tparam Graph Graph type, valid as long as it exposes edges and vertices.
 * @param out A generic data sink that satisfies the insertion expression.
 * @param graph The graph to output.
 * @param writer Vertex decorator object.
 */
template<typename Out, typename Graph>
requires stl::derived_from<typename Graph::graph_category, directed_tag>
void dot(Out &out, const Graph &graph, stl::invocable<Out &, typename Graph::vertex_type> auto writer) {
    if constexpr(stl::same_as<typename Graph::graph_category, undirected_tag>) {
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
        if constexpr(stl::same_as<typename Graph::graph_category, undirected_tag>) {
            out << lhs << "--" << rhs << ";";
        } else {
            out << lhs << "->" << rhs << ";";
        }
    }

    out << "}";
}

/**
 * @brief Outputs a graph in dot format.
 * @param out A generic data sink that satisfies the insertion expression.
 * @param graph The graph to output.
 */
void dot(auto &out, const auto &graph) {
    return dot(out, graph, [](auto &&...) {});
}

} // namespace entt

#endif

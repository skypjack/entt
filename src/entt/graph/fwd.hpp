#ifndef ENTT_GRAPH_FWD_HPP
#define ENTT_GRAPH_FWD_HPP

#include <cstddef>
#include <memory>
#include "../core/fwd.hpp"

namespace entt {

/*! @brief Undirected graph category tag. */
struct directed_tag {};

/*! @brief Directed graph category tag. */
struct undirected_tag: directed_tag {};

template<typename, typename = std::allocator<std::size_t>>
class adjacency_matrix;

template<typename = std::allocator<id_type>>
class basic_flow;

/*! @brief Alias declaration for the most common use case. */
using flow = basic_flow<>;

} // namespace entt

#endif

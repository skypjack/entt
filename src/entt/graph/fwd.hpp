#ifndef ENTT_GRAPH_FWD_HPP
#define ENTT_GRAPH_FWD_HPP

#include <cstddef>
#include <memory>
#include "../core/fwd.hpp"

namespace entt {

template<typename = std::allocator<std::size_t>>
class basic_adjacency_matrix;

template<typename = std::allocator<id_type>>
class basic_flow;

/*! @brief Alias declaration for the most common use case. */
using adjacency_matrix = basic_adjacency_matrix<>;

/*! @brief Alias declaration for the most common use case. */
using flow = basic_flow<>;

} // namespace entt

#endif

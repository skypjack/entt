#ifndef ENTT_GRAPH_FWD_HPP
#define ENTT_GRAPH_FWD_HPP

#include <cstddef>
#include <memory>

namespace entt {

template<typename = std::allocator<std::size_t>>
struct basic_adjacency_matrix;

/*! @brief Alias declaration for the most common use case. */
using adjacency_matrix = basic_adjacency_matrix<>;

} // namespace entt

#endif

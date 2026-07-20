#ifndef ENTT_GRAPH_FWD_HPP
#define ENTT_GRAPH_FWD_HPP

#include "../core/fwd.hpp"
#include "../stl/concepts.hpp"
#include "../stl/cstddef.hpp"
#include "../stl/memory.hpp"

namespace entt {

/*! @brief Undirected graph category tag. */
struct directed_tag {};

/*! @brief Directed graph category tag. */
struct undirected_tag: directed_tag {};

template<stl::derived_from<directed_tag>, typename = stl::allocator<stl::size_t>>
class adjacency_matrix;

template<typename = stl::allocator<id_type>>
class basic_flow;

/*! @brief Alias declaration for the most common use case. */
using flow = basic_flow<>;

} // namespace entt

#endif

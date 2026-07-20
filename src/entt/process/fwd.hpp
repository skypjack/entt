#ifndef ENTT_PROCESS_FWD_HPP
#define ENTT_PROCESS_FWD_HPP

#include "../stl/cstdint.hpp"
#include "../stl/memory.hpp"

namespace entt {

template<typename, typename = stl::allocator<void>>
class basic_process;

/*! @brief Alias declaration for the most common use case. */
using process = basic_process<stl::uint32_t>;

template<typename, typename = stl::allocator<void>>
class basic_scheduler;

/*! @brief Alias declaration for the most common use case. */
using scheduler = basic_scheduler<stl::uint32_t>;

} // namespace entt

#endif

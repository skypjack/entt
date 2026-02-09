#ifndef ENTT_PROCESS_FWD_HPP
#define ENTT_PROCESS_FWD_HPP

#include <cstdint>
#include <memory>
#include "../core/concepts.hpp"

namespace entt {

template<typename, allocator_like = std::allocator<void>>
class basic_process;

/*! @brief Alias declaration for the most common use case. */
using process = basic_process<std::uint32_t>;

template<typename, allocator_like = std::allocator<void>>
class basic_scheduler;

/*! @brief Alias declaration for the most common use case. */
using scheduler = basic_scheduler<std::uint32_t>;

} // namespace entt

#endif

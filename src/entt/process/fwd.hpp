#ifndef ENTT_PROCESS_FWD_HPP
#define ENTT_PROCESS_FWD_HPP

#include <cstdint>
#include <memory>

namespace entt {

template<typename, typename>
class process;

template<typename = std::uint32_t, typename = std::allocator<void>>
class basic_scheduler;

/*! @brief Alias declaration for the most common use case. */
using scheduler = basic_scheduler<>;

} // namespace entt

#endif

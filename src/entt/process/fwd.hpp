#ifndef ENTT_PROCESS_FWD_HPP
#define ENTT_PROCESS_FWD_HPP

#include "../config/module.h"

#ifndef ENTT_MODULE
#   include <cstdint>
#   include <memory>
#endif // ENTT_MODULE

ENTT_MODULE_EXPORT namespace entt {

template<typename, typename = std::allocator<void>>
class basic_process;

/*! @brief Alias declaration for the most common use case. */
using process = basic_process<std::uint32_t>;

template<typename, typename = std::allocator<void>>
class basic_scheduler;

/*! @brief Alias declaration for the most common use case. */
using scheduler = basic_scheduler<std::uint32_t>;

} // namespace entt

#endif

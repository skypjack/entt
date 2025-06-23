#ifndef ENTT_PROCESS_FWD_HPP
#define ENTT_PROCESS_FWD_HPP

#include <cstdint>
#include <memory>

namespace entt {

template<typename>
class basic_process;

template<typename, typename>
struct basic_process_adaptor;

/*! @brief Alias declaration for the most common use case. */
using process = basic_process<std::uint32_t>;

/**
 * @brief Alias declaration for the most common use case.
 * @tparam Func Actual type of process.
 */
template<typename Func>
using process_adaptor = basic_process_adaptor<std::uint32_t, Func>;

template<typename, typename = std::allocator<void>>
class basic_scheduler;

/*! @brief Alias declaration for the most common use case. */
using scheduler = basic_scheduler<std::uint32_t>;

} // namespace entt

#endif

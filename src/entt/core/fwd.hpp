#ifndef ENTT_CORE_FWD_HPP
#define ENTT_CORE_FWD_HPP

#include <cstdint>
#include <type_traits>
#include "../config/config.h"

namespace entt {

template<std::size_t Len = sizeof(double[2]), std::size_t = alignof(typename std::aligned_storage_t<Len + !Len>)>
class basic_any;

/*! @brief Alias declaration for type identifiers. */
using id_type = ENTT_ID_TYPE;

/*! @brief Alias declaration for the most common use case. */
using any = basic_any<>;

} // namespace entt

#endif

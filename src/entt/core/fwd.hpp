#ifndef ENTT_CORE_FWD_HPP
#define ENTT_CORE_FWD_HPP

#include <cstddef>
#include "../config/config.h"

namespace entt {

// NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays, modernize-avoid-c-arrays)
template<std::size_t Len = sizeof(double[2]), std::size_t = alignof(double[2])>
class basic_any;

/*! @brief Alias declaration for type identifiers. */
using id_type = ENTT_ID_TYPE;

/*! @brief Alias declaration for the most common use case. */
using any = basic_any<>;

template<typename, typename>
class compressed_pair;

template<typename>
class basic_hashed_string;

/*! @brief Aliases for common character types. */
using hashed_string = basic_hashed_string<char>;

/*! @brief Aliases for common character types. */
using hashed_wstring = basic_hashed_string<wchar_t>;

// NOLINTNEXTLINE(bugprone-forward-declaration-namespace)
struct type_info;

} // namespace entt

#endif

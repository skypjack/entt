#ifndef ENTT_CORE_FWD_HPP
#define ENTT_CORE_FWD_HPP

#include <cstddef>
#include <cstdint>
#include "../config/config.h"

namespace entt {

/*! @brief Possible modes of an any object. */
enum class any_policy : std::uint8_t {
    /*! @brief Default mode, no element available. */
    empty,
    /*! @brief Owning mode, dynamically allocated element. */
    dynamic,
    /*! @brief Owning mode, embedded element. */
    embedded,
    /*! @brief Aliasing mode, non-const reference. */
    ref,
    /*! @brief Const aliasing mode, const reference. */
    cref
};

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

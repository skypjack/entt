#ifndef ENTT_CORE_FWD_HPP
#define ENTT_CORE_FWD_HPP

#include <cstddef>
#include <cstdint>
#include "../config/config.h"

namespace entt {

/*! @brief Possible modes of an any object. */
enum class any_policy : std::uint8_t {
    /*! @brief Default mode, the object does not own any elements. */
    empty,
    /*! @brief Owning mode, the object owns a dynamically allocated element. */
    dynamic,
    /*! @brief Owning mode, the object owns an embedded element. */
    embedded,
    /*! @brief Aliasing mode, the object _points_ to a non-const element. */
    ref,
    /*! @brief Const aliasing mode, the object _points_ to a const element. */
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

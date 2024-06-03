#ifndef ENTT_CORE_FAMILY_HPP
#define ENTT_CORE_FAMILY_HPP

#include "../config/config.h"
#include "fwd.hpp"

namespace entt {

/**
 * @brief Dynamic identifier generator.
 *
 * Utility class template that can be used to assign unique identifiers to types
 * at runtime. Use different specializations to create separate sets of
 * identifiers.
 */
template<typename...>
class family {
    // NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
    inline static ENTT_MAYBE_ATOMIC(id_type) identifier{};

public:
    /*! @brief Unsigned integer type. */
    using value_type = id_type;

    /*! @brief Statically generated unique identifier for the given type. */
    template<typename... Type>
    // at the time I'm writing, clang crashes during compilation if auto is used instead of family_type
    inline static const value_type value = identifier++;
};

} // namespace entt

#endif

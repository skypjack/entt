#ifndef ENTT_CORE_FAMILY_HPP
#define ENTT_CORE_FAMILY_HPP


#include <type_traits>
#include <cstddef>
#include <atomic>
#include "../config/config.h"


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
    inline static std::atomic<std::size_t> identifier;

    template<typename...>
    inline static const auto inner = identifier.fetch_add(1);

public:
    /*! @brief Unsigned integer type. */
    using family_type = std::size_t;

    /*! @brief Statically generated unique identifier for the given type. */
    template<typename... Type>
    // at the time I'm writing, clang crashes during compilation if auto is used in place of family_type here
    inline static const family_type type = inner<std::decay_t<Type>...>;
};


}


#endif // ENTT_CORE_FAMILY_HPP

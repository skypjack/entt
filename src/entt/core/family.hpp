#ifndef ENTT_CORE_FAMILY_HPP
#define ENTT_CORE_FAMILY_HPP


#include <type_traits>
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
    inline static ENTT_MAYBE_ATOMIC(ENTT_ID_TYPE) identifier;

    template<typename...>
    // clang (since version 9) started to complain if auto is used instead of ENTT_ID_TYPE
    inline static const ENTT_ID_TYPE inner = identifier++;

public:
    /*! @brief Unsigned integer type. */
    using family_type = ENTT_ID_TYPE;

    /*! @brief Statically generated unique identifier for the given type. */
    template<typename... Type>
    // at the time I'm writing, clang crashes during compilation if auto is used instead of family_type
    inline static const family_type type = inner<std::decay_t<Type>...>;
};


}


#endif // ENTT_CORE_FAMILY_HPP

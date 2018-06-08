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
class Family {
    static std::atomic<std::size_t> identifier;

    template<typename...>
    static std::size_t family() ENTT_NOEXCEPT {
        static const std::size_t value = identifier.fetch_add(1);
        return value;
    }

public:
    /*! @brief Unsigned integer type. */
    using family_type = std::size_t;

    /**
     * @brief Returns an unique identifier for the given type.
     * @return Statically generated unique identifier for the given type.
     */
    template<typename... Type>
    inline static family_type type() ENTT_NOEXCEPT {
        return family<std::decay_t<Type>...>();
    }
};


template<typename... Types>
std::atomic<std::size_t> Family<Types...>::identifier{};


}


#endif // ENTT_CORE_FAMILY_HPP

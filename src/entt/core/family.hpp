#ifndef ENTT_CORE_FAMILY_HPP
#define ENTT_CORE_FAMILY_HPP


#include<type_traits>
#include<cstddef>
#include<utility>


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
    static std::size_t identifier() noexcept {
        static std::size_t value = 0;
        return value++;
    }

public:
    /**
     * @brief Returns an unique identifier for the given type.
     * @return Statically generated unique identifier for the given type.
     */
    template<typename...>
    static std::size_t type() noexcept {
        static const std::size_t value = identifier();
        return value;
    }
};


}


#endif // ENTT_CORE_FAMILY_HPP

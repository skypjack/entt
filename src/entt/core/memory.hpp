#ifndef ENTT_CORE_MEMORY_HPP
#define ENTT_CORE_MEMORY_HPP


#include <memory>
#include <type_traits>
#include "../config/config.h"


namespace entt {



/**
 * @brief Unwraps fancy pointers, does nothing otherwise.
 * @tparam Type Pointer type.
 * @param ptr A pointer to evaluate.
 * @return A plain pointer.
 */
template<typename Type>
[[nodiscard]] constexpr auto * unfancy(Type ptr) ENTT_NOEXCEPT {
    if constexpr(std::is_pointer_v<Type>) {
        return ptr;
    } else {
        return std::addressof(*ptr);
    }
}


}


#endif

#ifndef ENTT_CORE_MEMORY_HPP
#define ENTT_CORE_MEMORY_HPP


#include <memory>
#include <type_traits>
#include <utility>
#include "../config/config.h"


namespace entt {


/**
 * @brief Unwraps fancy pointers, does nothing otherwise (waiting for C++20).
 * @tparam Type Pointer type.
 * @param ptr Fancy or raw pointer.
 * @return A raw pointer that represents the address of the original pointer.
 */
template<typename Type>
constexpr auto to_address(Type &&ptr) ENTT_NOEXCEPT {
    if constexpr(std::is_pointer_v<std::remove_const_t<std::remove_reference_t<Type>>>) {
        return ptr;
    } else {
        return to_address(std::forward<Type>(ptr).operator->());
    }
}


}


#endif

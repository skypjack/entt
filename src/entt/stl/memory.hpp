#ifndef ENTT_STL_MEMORY_HPP
#define ENTT_STL_MEMORY_HPP

#include <memory>
#include <type_traits>
#include <utility>

namespace entt::stl {

/*! @cond TURN_OFF_DOXYGEN */
namespace internal {

template<typename Type>
[[nodiscard]] constexpr auto to_address(Type &&ptr) noexcept {
    if constexpr(std::is_pointer_v<std::decay_t<Type>>) {
        return ptr;
    } else {
        return to_address(std::forward<Type>(ptr).operator->());
    }
}

} // namespace internal
/*! @endcond */

#if __has_include(<version>)
#    include <version>
#
#    if defined(__cpp_lib_to_address)
#        define ENTT_STL_TO_ADDRESS std::to_address
#    endif
#endif

#ifndef ENTT_STL_TO_ADDRESS
#    define ENTT_STL_TO_ADDRESS internal::to_address
#endif

using ENTT_STL_TO_ADDRESS;

} // namespace entt::stl

#endif

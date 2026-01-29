#ifndef ENTT_STL_MEMORY_HPP
#define ENTT_STL_MEMORY_HPP

#include "../config/config.h"

/*! @cond ENTT_INTERNAL */
namespace entt::stl {

#ifndef ENTT_FORCE_STL
#    if __has_include(<version>)
#        include <version>
#
#        if defined(__cpp_lib_to_address)
#            define ENTT_HAS_TO_ADDRESS
#            include <memory>
using std::to_address;
#        endif
#    endif
#endif

#ifndef ENTT_HAS_TO_ADDRESS
#    include <memory>
#    include <type_traits>

template<typename Type>
constexpr Type *to_address(Type *ptr) noexcept {
    static_assert(!std::is_function_v<Type>, "Invalid type");
    return ptr;
}

template<typename Type>
constexpr auto to_address(const Type &ptr) noexcept {
    if constexpr(requires { std::pointer_traits<Type>::to_address(ptr); }) {
        return std::pointer_traits<Type>::to_address(ptr);
    } else {
        return to_address(ptr.operator->());
    }
}
#endif
/*! @endcond */

} // namespace entt::stl

#endif

#ifndef ENTT_STL_MEMORY_HPP
#define ENTT_STL_MEMORY_HPP

/*! @cond ENTT_INTERNAL */
#include "../config/config.h"

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
#    include <type_traits>
#    include <utility>

template<typename Type>
[[nodiscard]] constexpr auto to_address(Type &&ptr) noexcept {
    if constexpr(std::is_pointer_v<std::decay_t<Type>>) {
        return ptr;
    } else {
        return to_address(std::forward<Type>(ptr).operator->());
    }
}
#endif
/*! @endcond */

} // namespace entt::stl

#endif

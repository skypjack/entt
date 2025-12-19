#ifndef ENTT_STL_MEMORY_HPP
#define ENTT_STL_MEMORY_HPP

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
#        include <memory>
using std::to_address;
#    else
using stl::internal::to_address;
#    endif
#
#else
using stl::internal;
#endif

} // namespace entt::stl

#endif

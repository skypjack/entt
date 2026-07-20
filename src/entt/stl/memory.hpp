#ifndef ENTT_STL_MEMORY_HPP
#define ENTT_STL_MEMORY_HPP

/*! @cond ENTT_INTERNAL */
#if __has_include(<entt/ext/stl/memory.hpp>)
#    include <entt/ext/stl/memory.hpp>
#else
#    include <memory>
#    include <version>
#    include "../config/config.h"

namespace entt::stl {

using std::addressof;
using std::allocate_shared;
using std::allocator;
using std::allocator_arg;
using std::allocator_arg_t;
using std::allocator_traits;
using std::default_delete;
using std::destroy;
using std::enable_shared_from_this;
using std::make_shared;
using std::make_unique;
using std::pointer_traits;
using std::shared_ptr;
using std::static_pointer_cast;
using std::uninitialized_fill;
using std::unique_ptr;
using std::uses_allocator_v;

} // namespace entt::stl

#    ifndef ENTT_FORCE_STL
#        if defined(__cpp_lib_to_address)
#            define ENTT_HAS_TO_ADDRESS

namespace entt::stl {

using std::to_address;

} // namespace entt::stl

#        endif
#    endif

#    ifndef ENTT_HAS_TO_ADDRESS
#        include <type_traits>

namespace entt::stl {

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

} // namespace entt::stl
#    endif
#endif
/*! @endcond */

#undef ENTT_HAS_TO_ADDRESS

#endif

#ifndef ENTT_STL_FUNCTIONAL_HPP
#define ENTT_STL_FUNCTIONAL_HPP

/*! @cond ENTT_INTERNAL */
#if __has_include(<entt/ext/stl/functional.hpp>)
#    include <entt/ext/stl/functional.hpp>
#else
#    include <functional>
#    include <version>
#    include "../config/config.h"

namespace entt::stl {

using std::equal_to;
using std::function;
using std::hash;
using std::invoke;
using std::less;

} // namespace entt::stl

#    ifndef ENTT_FORCE_STL
#        if defined(__cpp_lib_ranges)
#            define ENTT_HAS_IDENTITY
namespace entt::stl {

using std::identity;

} // namespace entt::stl
#        endif
#    endif

#    ifndef ENTT_HAS_IDENTITY
#        include <utility>

namespace entt::stl {

struct identity {
    using is_transparent = void;

    template<typename Type>
    [[nodiscard]] constexpr Type &&operator()(Type &&value) const noexcept {
        return std::forward<Type>(value);
    }
};

} // namespace entt::stl
#    endif

#endif
/*! @endcond */

#undef ENTT_HAS_IDENTITY

#endif

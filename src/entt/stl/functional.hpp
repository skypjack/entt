#ifndef ENTT_STL_FUNCTIONAL_HPP
#define ENTT_STL_FUNCTIONAL_HPP

#include "../config/config.h"

/*! @cond ENTT_INTERNAL */
namespace entt::stl {

#ifndef ENTT_FORCE_STL
#    if __has_include(<version>)
#        include <version>
#
#        if defined(__cpp_lib_ranges)
#            define ENTT_HAS_IDENTITY
#            include <functional>
using std::identity;
#        endif
#    endif
#endif

#ifndef ENTT_HAS_IDENTITY
#    include <utility>

struct identity {
    using is_transparent = void;

    template<typename Type>
    [[nodiscard]] constexpr Type &&operator()(Type &&value) const noexcept {
        return std::forward<Type>(value);
    }
};
#endif

} // namespace entt::stl
/*! @endcond */

#endif

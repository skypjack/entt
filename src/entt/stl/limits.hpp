#ifndef ENTT_STL_LIMITS_HPP
#define ENTT_STL_LIMITS_HPP

/*! @cond ENTT_INTERNAL */
#if __has_include(<entt/ext/stl/limits.hpp>)
#    include <entt/ext/stl/limits.hpp>
#else
#    include <limits>

namespace entt::stl {

using std::numeric_limits;

} // namespace entt::stl
#endif
/*! @endcond */

#endif

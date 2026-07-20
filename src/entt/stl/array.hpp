#ifndef ENTT_STL_ARRAY_HPP
#define ENTT_STL_ARRAY_HPP

/*! @cond ENTT_INTERNAL */
#if __has_include(<entt/ext/stl/array.hpp>)
#    include <entt/ext/stl/array.hpp>
#else
#    include <array>

namespace entt::stl {

using std::array;
using std::get;

} // namespace entt::stl
#endif
/*! @endcond */

#endif

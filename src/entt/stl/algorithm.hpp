#ifndef ENTT_STL_ALGORITHM_HPP
#define ENTT_STL_ALGORITHM_HPP

/*! @cond ENTT_INTERNAL */
#if __has_include(<entt/ext/stl/algorithm.hpp>)
#    include <entt/ext/stl/algorithm.hpp>
#else
#    include <algorithm>

namespace entt::stl {

using std::all_of;
using std::any_of;
using std::find_if;
using std::none_of;
using std::sort;

} // namespace entt::stl
#endif
/*! @endcond */

#endif

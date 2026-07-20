#ifndef ENTT_STL_STRING_VIEW_HPP
#define ENTT_STL_STRING_VIEW_HPP

/*! @cond ENTT_INTERNAL */
#if __has_include(<entt/ext/stl/string_view.hpp>)
#    include <entt/ext/stl/string_view.hpp>
#else
#    include <string_view>

namespace entt::stl {

using std::string_view;
using std::swap;

} // namespace entt::stl
#endif
/*! @endcond */

#endif

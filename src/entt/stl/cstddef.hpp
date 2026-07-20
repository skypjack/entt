#ifndef ENTT_STL_CSTDDEF_HPP
#define ENTT_STL_CSTDDEF_HPP

/*! @cond ENTT_INTERNAL */
#if __has_include(<entt/ext/stl/cstddef.hpp>)
#    include <entt/ext/stl/cstddef.hpp>
#else
#    include <cstddef>

namespace entt::stl {

using std::byte;
using std::nullptr_t;
using std::ptrdiff_t;
using std::size_t;

} // namespace entt::stl
#endif
/*! @endcond */

#endif

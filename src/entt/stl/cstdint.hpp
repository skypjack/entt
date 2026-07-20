#ifndef ENTT_STL_CSTDINT_HPP
#define ENTT_STL_CSTDINT_HPP

/*! @cond ENTT_INTERNAL */
#if __has_include(<entt/ext/stl/cstdint.hpp>)
#    include <entt/ext/stl/cstdint.hpp>
#else
#    include <cstdint>

namespace entt::stl {

using std::uint16_t;
using std::uint32_t;
using std::uint64_t;
using std::uint8_t;

} // namespace entt::stl
#endif
/*! @endcond */

#endif

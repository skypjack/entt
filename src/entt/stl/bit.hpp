#ifndef ENTT_STL_BIT_HPP
#define ENTT_STL_BIT_HPP

/*! @cond ENTT_INTERNAL */
#if __has_include(<entt/ext/stl/bit.hpp>)
#    include <entt/ext/stl/bit.hpp>
#else
#    include <bit>

namespace entt::stl {

using std::bit_ceil;
using std::has_single_bit;
using std::popcount;

} // namespace entt::stl
#endif
/*! @endcond */

#endif

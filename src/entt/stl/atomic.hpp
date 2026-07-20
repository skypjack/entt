#ifndef ENTT_STL_ATOMIC_HPP
#define ENTT_STL_ATOMIC_HPP

/*! @cond ENTT_INTERNAL */
#if __has_include(<entt/ext/stl/atomic.hpp>)
#    include <entt/ext/stl/atomic.hpp>
#else
#    include <atomic>

namespace entt::stl {

using std::atomic;

} // namespace entt::stl
#endif
/*! @endcond */

#endif

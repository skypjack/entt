#ifndef ENTT_STL_CONCEPTS_HPP
#define ENTT_STL_CONCEPTS_HPP

/*! @cond ENTT_INTERNAL */
#if __has_include(<entt/ext/stl/concepts.hpp>)
#    include <entt/ext/stl/concepts.hpp>
#else
#    include <concepts>

namespace entt::stl {

using std::constructible_from;
using std::default_initializable;
using std::derived_from;
using std::integral;
using std::invocable;
using std::same_as;
using std::unsigned_integral;

} // namespace entt::stl
#endif
/*! @endcond */

#endif

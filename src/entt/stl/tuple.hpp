#ifndef ENTT_STL_TUPLE_HPP
#define ENTT_STL_TUPLE_HPP

/*! @cond ENTT_INTERNAL */
#if __has_include(<entt/ext/stl/tuple.hpp>)
#    include <entt/ext/stl/tuple.hpp>
#else
#    include <tuple>

namespace entt::stl {

using std::apply;
using std::forward_as_tuple;
using std::get;
using std::make_from_tuple;
using std::make_tuple;
using std::tuple;
using std::tuple_cat;
using std::tuple_element;
using std::tuple_element_t;
using std::tuple_size;
using std::tuple_size_v;

} // namespace entt::stl
#endif
/*! @endcond */

#endif

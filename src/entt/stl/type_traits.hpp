#ifndef ENTT_STL_TYPE_TRAITS_HPP
#define ENTT_STL_TYPE_TRAITS_HPP

#include <type_traits>

/*! @cond ENTT_INTERNAL */
namespace entt::stl {

using std::false_type;
using std::invoke_result_t;
using std::is_invocable_r_v;
using std::is_lvalue_reference_v;
using std::is_member_object_pointer_v;
using std::is_pointer_v;
using std::is_same_v;
using std::is_trivially_destructible_v;
using std::is_void_v;
using std::remove_const_t;
using std::remove_cvref_t;
using std::remove_pointer_t;
using std::remove_reference_t;
using std::true_type;

} // namespace entt::stl
/*! @endcond */

#endif

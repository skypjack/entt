#ifndef ENTT_STL_TYPE_TRAITS_HPP
#define ENTT_STL_TYPE_TRAITS_HPP

#include <type_traits>

/*! @cond ENTT_INTERNAL */
namespace entt::stl {

using std::bool_constant;
using std::decay_t;
using std::extent_v;
using std::false_type;
using std::invoke_result_t;
using std::is_aggregate_v;
using std::is_array_v;
using std::is_const_v;
using std::is_copy_assignable_v;
using std::is_copy_constructible_v;
using std::is_default_constructible_v;
using std::is_function_v;
using std::is_invocable_r_v;
using std::is_lvalue_reference_v;
using std::is_member_object_pointer_v;
using std::is_move_assignable_v;
using std::is_nothrow_move_constructible_v;
using std::is_pointer_v;
using std::is_same_v;
using std::is_trivially_destructible_v;
using std::is_void_v;
using std::remove_const_t;
using std::remove_cvref_t;
using std::remove_pointer_t;
using std::remove_reference_t;
using std::true_type;
using std::underlying_type_t;

} // namespace entt::stl
/*! @endcond */

#endif

#ifndef ENTT_META_RESOLVE_HPP
#define ENTT_META_RESOLVE_HPP


#include <algorithm>
#include "ctx.hpp"
#include "meta.hpp"
#include "range.hpp"


namespace entt {


/**
 * @brief Returns the meta type associated with a given type.
 * @tparam Type Type to use to search for a meta type.
 * @return The meta type associated with the given type, if any.
 */
template<typename Type>
[[nodiscard]] meta_type resolve() ENTT_NOEXCEPT {
    return internal::meta_info<Type>::resolve();
}


/**
 * @brief Returns a range to use to visit all meta types.
 * @return An iterable range to use to visit all meta types.
 */
[[nodiscard]] inline meta_range<meta_type> resolve() {
    return *internal::meta_context::global();
}


/**
 * @brief Iterates all the reflected types.
 * @tparam Op Type of the function object to invoke.
 * @param op A valid function object.
 */
template<typename Op>
[[deprecated("use resolve() and entt::meta_range<meta_type> instead")]]
void resolve(Op op) {
    for(auto curr: resolve()) {
        op(curr);
    }
}


/**
 * @brief Returns the first meta type that satisfies specific criteria, if any.
 * @tparam Func Type of the unary predicate to use to test the meta types.
 * @param func Unary predicate which returns ​true for the required element.
 * @return The first meta type satisfying the condition, if any.
 */
template<typename Func>
[[deprecated("use std::find_if and entt::meta_range<meta_type> instead")]]
[[nodiscard]] meta_type resolve_if(Func func) ENTT_NOEXCEPT {
    internal::meta_range range{*internal::meta_context::global()};
    return std::find_if(range.begin(), range.end(), [&func](const auto &curr) { return func(meta_type{&curr}); }).operator->();
}


/**
 * @brief Returns the meta type associated with a given identifier, if any.
 * @param id Unique identifier.
 * @return The meta type associated with the given identifier, if any.
 */
[[nodiscard]] inline meta_type resolve_id(const id_type id) ENTT_NOEXCEPT {
    internal::meta_range range{*internal::meta_context::global()};
    return std::find_if(range.begin(), range.end(), [id](const auto &curr) { return curr.id == id; }).operator->();
}


/**
 * @brief Returns the meta type associated with a given type id, if any.
 * @param id Unique identifier.
 * @return The meta type associated with the given type id, if any.
 */
[[nodiscard]] inline meta_type resolve_type(const id_type id) ENTT_NOEXCEPT {
    internal::meta_range range{*internal::meta_context::global()};
    return std::find_if(range.begin(), range.end(), [id](const auto &curr) { return curr.type_id == id; }).operator->();
}


}


#endif

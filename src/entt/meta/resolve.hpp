#ifndef ENTT_META_RESOLVE_HPP
#define ENTT_META_RESOLVE_HPP


#include <algorithm>
#include "../core/type_info.hpp"
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
 * @brief Returns the meta type associated with a given identifier, if any.
 * @param id Unique identifier.
 * @return The meta type associated with the given identifier, if any.
 */
[[nodiscard]] inline meta_type resolve(const id_type id) ENTT_NOEXCEPT {
    internal::meta_range range{*internal::meta_context::global()};
    return std::find_if(range.begin(), range.end(), [id](const auto &curr) { return curr.id == id; }).operator->();
}


/**
 * @brief Returns the meta type associated with a given type info object, if
 * any.
 * @param info The type info object of the requested type.
 * @return The meta type associated with the given type info object, if any.
 */
[[nodiscard]] inline meta_type resolve(const type_info info) ENTT_NOEXCEPT {
    internal::meta_range range{*internal::meta_context::global()};
    return std::find_if(range.begin(), range.end(), [info](const auto &curr) { return curr.info == info; }).operator->();
}


}


#endif

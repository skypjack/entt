#ifndef ENTT_META_RESOLVE_HPP
#define ENTT_META_RESOLVE_HPP


#include <type_traits>
#include "meta.hpp"


namespace entt {


/**
 * @brief Returns the meta type associated with a given type.
 * @tparam Type Type to use to search for a meta type.
 * @return The meta type associated with the given type, if any.
 */
template<typename Type>
inline meta_type resolve() ENTT_NOEXCEPT {
    return internal::meta_info<Type>::resolve();
}


/**
 * @brief Returns the first meta type that satisfies specific criteria, if any.
 * @tparam Func Type of the unary predicate to use to test the meta types.
 * @param func Unary predicate which returns ​true for the required element.
 * @return The first meta type satisfying the condition, if any.
 */
template<typename Func>
inline meta_type resolve_if(Func func) ENTT_NOEXCEPT {
    return internal::find_if([&func](const auto *curr) {
        return func(meta_type{curr});
    }, *internal::meta_context::global);
}


/**
 * @brief Returns the meta type associated with a given identifier, if any.
 * @param id Unique identifier.
 * @return The meta type associated with the given identifier, if any.
 */
inline meta_type resolve_id(const id_type id) ENTT_NOEXCEPT {
    return resolve_if([id](const auto type) { return type.id() == id; });
}


/**
 * @brief Returns the meta type associated with a given type id, if any.
 * @param id Unique identifier.
 * @return The meta type associated with the given type id, if any.
 */
inline meta_type resolve_type(const id_type id) ENTT_NOEXCEPT {
    return resolve_if([id](const auto type) { return type.type_id() == id; });
}


/*! @copydoc resolve_id */
[[deprecated("use entt::resolve_id instead")]]
inline meta_type resolve(const id_type id) ENTT_NOEXCEPT {
    return resolve_id(id);
}


/**
 * @brief Iterates all the reflected types.
 * @tparam Op Type of the function object to invoke.
 * @param op A valid function object.
 */
template<typename Op>
inline std::enable_if_t<std::is_invocable_v<Op, meta_type>, void>
resolve(Op op) {
    internal::visit<meta_type>(op, *internal::meta_context::global);
}


}


#endif

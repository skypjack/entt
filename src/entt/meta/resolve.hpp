#ifndef ENTT_META_RESOLVE_HPP
#define ENTT_META_RESOLVE_HPP

#include <type_traits>
#include "../core/type_info.hpp"
#include "../locator/locator.hpp"
#include "context.hpp"
#include "meta.hpp"
#include "node.hpp"
#include "range.hpp"

namespace entt {

/**
 * @brief Returns the meta type associated with a given type.
 * @tparam Type Type to use to search for a meta type.
 * @param ctx The context from which to search for meta types.
 * @return The meta type associated with the given type, if any.
 */
template<typename Type>
[[nodiscard]] meta_type resolve(const meta_ctx &ctx) noexcept {
    auto &&context = internal::meta_context::from(ctx);
    return {ctx, internal::resolve<std::remove_cv_t<std::remove_reference_t<Type>>>(context)};
}

/**
 * @brief Returns the meta type associated with a given type.
 * @tparam Type Type to use to search for a meta type.
 * @return The meta type associated with the given type, if any.
 */
template<typename Type>
[[nodiscard]] meta_type resolve() noexcept {
    return resolve<Type>(locator<meta_ctx>::value_or());
}

/**
 * @brief Returns a range to use to visit all meta types.
 * @param ctx The context from which to search for meta types.
 * @return An iterable range to use to visit all meta types.
 */
[[nodiscard]] inline meta_range<meta_type, typename decltype(internal::meta_context::value)::const_iterator> resolve(const meta_ctx &ctx) noexcept {
    auto &&context = internal::meta_context::from(ctx);
    return {{ctx, context.value.cbegin()}, {ctx, context.value.cend()}};
}

/**
 * @brief Returns a range to use to visit all meta types.
 * @return An iterable range to use to visit all meta types.
 */
[[nodiscard]] inline meta_range<meta_type, typename decltype(internal::meta_context::value)::const_iterator> resolve() noexcept {
    return resolve(locator<meta_ctx>::value_or());
}

/**
 * @brief Returns the meta type associated with a given identifier, if any.
 * @param ctx The context from which to search for meta types.
 * @param id Unique identifier.
 * @return The meta type associated with the given identifier, if any.
 */
[[nodiscard]] inline meta_type resolve(const meta_ctx &ctx, const id_type id) noexcept {
    for(auto &&curr: resolve(ctx)) {
        if(curr.second.id() == id) {
            return curr.second;
        }
    }

    return meta_type{};
}

/**
 * @brief Returns the meta type associated with a given identifier, if any.
 * @param id Unique identifier.
 * @return The meta type associated with the given identifier, if any.
 */
[[nodiscard]] inline meta_type resolve(const id_type id) noexcept {
    return resolve(locator<meta_ctx>::value_or(), id);
}

/**
 * @brief Returns the meta type associated with a given type info object.
 * @param ctx The context from which to search for meta types.
 * @param info The type info object of the requested type.
 * @return The meta type associated with the given type info object, if any.
 */
[[nodiscard]] inline meta_type resolve(const meta_ctx &ctx, const type_info &info) noexcept {
    auto &&context = internal::meta_context::from(ctx);
    const auto *elem = internal::try_resolve(context, info);
    return elem ? meta_type{ctx, *elem} : meta_type{};
}

/**
 * @brief Returns the meta type associated with a given type info object.
 * @param info The type info object of the requested type.
 * @return The meta type associated with the given type info object, if any.
 */
[[nodiscard]] inline meta_type resolve(const type_info &info) noexcept {
    return resolve(locator<meta_ctx>::value_or(), info);
}

} // namespace entt

#endif

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
 * @param context The context from which to search for meta types.
 * @return The meta type associated with the given type, if any.
 */
template<typename Type>
[[nodiscard]] meta_type resolve(const meta_ctx &context = locator<meta_ctx>::value_or()) noexcept {
    auto &&ctx = internal::meta_context::from(context);
    return internal::resolve<std::remove_cv_t<std::remove_reference_t<Type>>>(ctx);
}

/**
 * @brief Returns a range to use to visit all meta types.
 * @param context The context from which to search for meta types.
 * @return An iterable range to use to visit all meta types.
 */
[[nodiscard]] inline meta_range<meta_type, typename decltype(internal::meta_context::value)::const_iterator> resolve(const meta_ctx &context = locator<meta_ctx>::value_or()) noexcept {
    auto &&ctx = internal::meta_context::from(context);
    return {ctx.value.cbegin(), ctx.value.cend()};
}

/**
 * @brief Returns the meta type associated with a given identifier, if any.
 * @param id Unique identifier.
 * @param context The context from which to search for meta types.
 * @return The meta type associated with the given identifier, if any.
 */
[[nodiscard]] inline meta_type resolve(const id_type id, const meta_ctx &context = locator<meta_ctx>::value_or()) noexcept {
    for(auto &&curr: resolve(context)) {
        if(curr.second.id() == id) {
            return curr.second;
        }
    }

    return {};
}

/**
 * @brief Returns the meta type associated with a given type info object.
 * @param info The type info object of the requested type.
 * @param context The context from which to search for meta types.
 * @return The meta type associated with the given type info object, if any.
 */
[[nodiscard]] inline meta_type resolve(const type_info &info, const meta_ctx &context = locator<meta_ctx>::value_or()) noexcept {
    auto &&ctx = internal::meta_context::from(context);
    const auto *elem = internal::try_resolve(info, ctx);
    return elem ? *elem : meta_type{};
}

} // namespace entt

#endif

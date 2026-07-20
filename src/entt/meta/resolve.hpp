#ifndef ENTT_META_RESOLVE_HPP
#define ENTT_META_RESOLVE_HPP

#include "../core/type_info.hpp"
#include "../locator/locator.hpp"
#include "../stl/type_traits.hpp"
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
    const auto &context = internal::meta_context::from(ctx);
    return {ctx, internal::resolve<stl::remove_cvref_t<Type>>(context)};
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
[[nodiscard]] inline meta_range<meta_type, typename internal::meta_context::bucket_type::const_iterator> resolve(const meta_ctx &ctx) noexcept {
    const auto &context = internal::meta_context::from(ctx);
    return {{ctx, context.bucket.cbegin()}, {ctx, context.bucket.cend()}};
}

/**
 * @brief Returns a range to use to visit all meta types.
 * @return An iterable range to use to visit all meta types.
 */
[[nodiscard]] inline meta_range<meta_type, typename internal::meta_context::bucket_type::const_iterator> resolve() noexcept {
    return resolve(locator<meta_ctx>::value_or());
}

/**
 * @brief Returns the meta type associated with a given identifier, if any.
 * @param ctx The context from which to search for meta types.
 * @param alias Unique identifier.
 * @return The meta type associated with the given identifier, if any.
 */
[[nodiscard]] inline meta_type resolve(const meta_ctx &ctx, const id_type alias) noexcept {
    const auto &context = internal::meta_context::from(ctx);

    // fast lookup for unsearchable and overloaded types
    if(const auto it = context.bucket.find(alias); it != context.bucket.end()) {
        return meta_type{ctx, *it->second};
    }

    for(auto &&curr: context.bucket) {
        if(curr.second->alias == alias) {
            return meta_type{ctx, *curr.second};
        }
    }

    return meta_type{};
}

/**
 * @brief Returns the meta type associated with a given identifier, if any.
 * @param alias Unique identifier.
 * @return The meta type associated with the given identifier, if any.
 */
[[nodiscard]] inline meta_type resolve(const id_type alias) noexcept {
    return resolve(locator<meta_ctx>::value_or(), alias);
}

/**
 * @brief Returns the meta type associated with a given type info object.
 * @param ctx The context from which to search for meta types.
 * @param info The type info object of the requested type.
 * @return The meta type associated with the given type info object, if any.
 */
[[nodiscard]] inline meta_type resolve(const meta_ctx &ctx, const type_info &info) noexcept {
    const auto &context = internal::meta_context::from(ctx);
    const auto it = context.bucket.find(info.hash());
    return (it == context.bucket.cend()) ? meta_type{} : meta_type{ctx, *it->second};
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

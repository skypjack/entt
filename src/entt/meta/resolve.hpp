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
 * @return The meta type associated with the given type, if any.
 */
template<typename Type>
[[nodiscard]] meta_type resolve() noexcept {
    return internal::resolve<std::remove_cv_t<std::remove_reference_t<Type>>>();
}

/**
 * @brief Returns a range to use to visit all meta types.
 * @return An iterable range to use to visit all meta types.
 */
[[nodiscard]] inline meta_range<meta_type, typename decltype(internal::meta_context::value)::const_iterator> resolve() noexcept {
    auto &&context = internal::meta_context::from(locator<meta_ctx>::value_or());
    return {context.value.cbegin(), context.value.cend()};
}

/**
 * @brief Returns the meta type associated with a given identifier, if any.
 * @param id Unique identifier.
 * @return The meta type associated with the given identifier, if any.
 */
[[nodiscard]] inline meta_type resolve(const id_type id) noexcept {
    for(auto &&curr: resolve()) {
        if(curr.second.id() == id) {
            return curr.second;
        }
    }

    return {};
}

/**
 * @brief Returns the meta type associated with a given type info object.
 * @param info The type info object of the requested type.
 * @return The meta type associated with the given type info object, if any.
 */
[[nodiscard]] inline meta_type resolve(const type_info &info) noexcept {
    const auto *elem = internal::try_resolve(info);
    return elem ? *elem : meta_type{};
}

} // namespace entt

#endif

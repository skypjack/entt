#ifndef ENTT_CORE_TUPLE_HPP
#define ENTT_CORE_TUPLE_HPP

#include <tuple>
#include <type_traits>
#include <utility>
#include "../config/config.h"
#include "type_traits.hpp"

namespace entt {

/**
 * @brief Utility function to unwrap tuples of a single element.
 * @tparam Type Tuple type of any sizes.
 * @param value A tuple object of the given type.
 * @return The tuple itself if it contains more than one element, the first
 * element otherwise.
 */
template<typename Type>
constexpr decltype(auto) unwrap_tuple(Type &&value) ENTT_NOEXCEPT {
    if constexpr(std::tuple_size_v<std::remove_reference_t<Type>> == 1u) {
        return std::get<0>(std::forward<Type>(value));
    } else {
        return std::forward<Type>(value);
    }
}

/**
 * \brief Functional type, which forwards the given tuple and the internal functional object to ``std::apply`` on invocation.
 * \tparam Func Type of the stored functional object.
 */
template <typename Func>
class apply_fn : Func
{
public:
	template <class... Args>
    constexpr apply_fn(Args&&... args) ENTT_NOEXCEPT_IF((std::is_nothrow_constructible_v<Func, Args...>))
        : Func{ std::forward<Args>(args)... }
	{}

	template <class Tuple>
	constexpr decltype(auto) operator ()(Tuple&& tuple) ENTT_NOEXCEPT_IF(noexcept(std::apply(std::declval<Func&>(), std::declval<Tuple>())))
	{
		return std::apply(static_cast<Func&>(*this), std::forward<Tuple>(tuple));
	}

    template <class Tuple>
	constexpr decltype(auto) operator ()(Tuple&& tuple) const ENTT_NOEXCEPT_IF(noexcept(std::apply(std::declval<const Func&>(), std::declval<Tuple>())))
	{
		return std::apply(static_cast<const Func&>(*this), std::forward<Tuple>(tuple));
	}
};

template <typename Func>
apply_fn(Func) -> apply_fn<std::remove_reference_t<std::remove_cv_t<Func>>>;
} // namespace entt

#endif

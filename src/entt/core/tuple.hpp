#ifndef ENTT_CORE_TUPLE_HPP
#define ENTT_CORE_TUPLE_HPP

#include <tuple>
#include <type_traits>
#include <utility>
#include "../config/config.h"

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
 * \brief Functional type, which forwards the given tuple and the stored function to ``std::apply`` on invocation.
 * \tparam Func Type of the stored functional object.
 */
template <typename Func>
struct apply_fn
{
	Func func{};

	template <class Tuple>
	constexpr decltype(auto) operator ()(Tuple&& tuple)
	{
		return std::apply(std::ref(func), std::forward<Tuple>(tuple));
	}
};

template <typename Func>
	apply_fn(Func) -> apply_fn<Func>;
} // namespace entt

#endif

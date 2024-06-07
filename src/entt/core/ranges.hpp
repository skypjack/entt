#ifndef ENTT_ENTITY_RANGES_HPP
#define ENTT_ENTITY_RANGES_HPP

#if __cplusplus >= 202002L

#    include <ranges>
#    include "iterator.hpp"

template<class... Args>
inline constexpr bool std::ranges::enable_borrowed_range<entt::iterable_adaptor<Args...>>{true};

template<class... Args>
inline constexpr bool std::ranges::enable_view<entt::iterable_adaptor<Args...>>{true};

#endif

#endif
#ifndef ENTT_ENTITY_RANGES_HPP
#define ENTT_ENTITY_RANGES_HPP

#if __cplusplus >= 202002L

#    include <ranges>
#    include "fwd.hpp"

template<class... Args>
inline constexpr bool std::ranges::enable_borrowed_range<entt::basic_view<Args...>>{true};

template<class... Args>
inline constexpr bool std::ranges::enable_borrowed_range<entt::basic_group<Args...>>{true};

template<class... Args>
inline constexpr bool std::ranges::enable_view<entt::basic_view<Args...>>{true};

template<class... Args>
inline constexpr bool std::ranges::enable_view<entt::basic_group<Args...>>{true};

#endif

#endif
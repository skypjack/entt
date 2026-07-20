#ifndef ENTT_ENTITY_RANGES_HPP
#define ENTT_ENTITY_RANGES_HPP

#include <version>

#if defined(__cpp_lib_ranges)
#    include <ranges>
#    include "fwd.hpp"

namespace std::ranges {

template<class... Args>
inline constexpr bool enable_borrowed_range<entt::basic_view<Args...>>{true};

template<class... Args>
inline constexpr bool enable_borrowed_range<entt::basic_group<Args...>>{true};

template<class... Args>
inline constexpr bool enable_view<entt::basic_view<Args...>>{true};

template<class... Args>
inline constexpr bool enable_view<entt::basic_group<Args...>>{true};

} // namespace std::ranges

#endif

#endif

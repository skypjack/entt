#ifndef ENTT_CORE_RANGES_HPP
#define ENTT_CORE_RANGES_HPP

#if __has_include(<version>)
#    include <version>
#
#    if defined(__cpp_lib_ranges)
#        include <ranges>
#        include "iterator.hpp"

template<class... Args>
inline constexpr bool std::ranges::enable_borrowed_range<entt::iterable_adaptor<Args...>>{true};

template<class... Args>
inline constexpr bool std::ranges::enable_view<entt::iterable_adaptor<Args...>>{true};

#    endif
#endif

#endif
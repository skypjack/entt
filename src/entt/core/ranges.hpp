#ifndef ENTT_CORE_RANGES_HPP
#define ENTT_CORE_RANGES_HPP

#include <version>

#if defined(__cpp_lib_ranges)
#    include <ranges>
#    include "iterator.hpp"

namespace std::ranges {

template<class... Args>
inline constexpr bool enable_borrowed_range<entt::iterable_adaptor<Args...>>{true};

template<class... Args>
inline constexpr bool enable_view<entt::iterable_adaptor<Args...>>{true};

} // namespace std::ranges

#endif

#endif

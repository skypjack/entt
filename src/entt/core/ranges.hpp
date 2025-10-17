#ifndef ENTT_CORE_RANGES_HPP
#define ENTT_CORE_RANGES_HPP

#include "../config/module.h"

#if __has_include(<version>)
#    ifndef ENTT_MODULE
#        include <version>
#    endif // ENTT_MODULE
#
#    if defined(__cpp_lib_ranges)
#        ifndef ENTT_MODULE
#           include <ranges>
#           include "iterator.hpp"
#        endif // ENTT_MODULE

ENTT_MODULE_EXPORT_BEGIN

template<class... Args>
inline constexpr bool std::ranges::enable_borrowed_range<entt::iterable_adaptor<Args...>>{true};

template<class... Args>
inline constexpr bool std::ranges::enable_view<entt::iterable_adaptor<Args...>>{true};

ENTT_MODULE_EXPORT_END

#    endif
#endif

#endif
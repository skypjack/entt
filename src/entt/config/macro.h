#ifndef ENTT_CONFIG_MACRO_H
#define ENTT_CONFIG_MACRO_H

// NOLINTBEGIN(cppcoreguidelines-macro-usage)

#define ENTT_STR(arg) #arg
#define ENTT_XSTR(arg) ENTT_STR(arg)

#define ENTT_CONSTEVAL constexpr

#if __has_include(<version>)
#    include <version>
#
#    if defined(__cpp_consteval)
#        undef ENTT_CONSTEVAL
#        define ENTT_CONSTEVAL consteval
#    endif
#endif

// NOLINTEND(cppcoreguidelines-macro-usage)

#endif

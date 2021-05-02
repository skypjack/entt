#ifndef ENTT_CONFIG_CONFIG_H
#define ENTT_CONFIG_CONFIG_H


#ifndef ENTT_NOEXCEPT
#   define ENTT_NOEXCEPT noexcept
#endif


#if defined(__cpp_lib_launder) && __cpp_lib_launder >= 201606L
#   include <new>
#   define ENTT_LAUNDER(expr) std::launder(expr)
#else
#   define ENTT_LAUNDER(expr) expr
#endif


#ifndef ENTT_USE_ATOMIC
#   define ENTT_MAYBE_ATOMIC(Type) Type
#else
#   include <atomic>
#   define ENTT_MAYBE_ATOMIC(Type) std::atomic<Type>
#endif


#ifndef ENTT_ID_TYPE
#   include <cstdint>
#   define ENTT_ID_TYPE std::uint32_t
#endif


#ifdef ENTT_PAGE_SIZE
static_assert(ENTT_PAGE_SIZE && ((ENTT_PAGE_SIZE & (ENTT_PAGE_SIZE - 1)) == 0), "ENTT_PAGE_SIZE must be a power of two");
#else
#   define ENTT_PAGE_SIZE 4096
#endif


#ifdef ENTT_DISABLE_ASSERT
#   undef ENTT_ASSERT
#   define ENTT_ASSERT(...) (void(0))
#elif !defined ENTT_ASSERT
#   include <cassert>
#   define ENTT_ASSERT(condition, ...) assert(condition)
#endif


#ifndef ENTT_NO_ETO
#   include <type_traits>
#   define ENTT_IS_EMPTY(Type) std::is_empty<Type>
#else
#   include <type_traits>
#   define ENTT_IS_EMPTY(Type) std::false_type
#endif


#ifndef ENTT_STANDARD_CPP
#    if defined __clang__ || defined __GNUC__
#       define ENTT_PRETTY_FUNCTION __PRETTY_FUNCTION__
#       define ENTT_PRETTY_FUNCTION_PREFIX '='
#       define ENTT_PRETTY_FUNCTION_SUFFIX ']'
#    elif defined _MSC_VER
#       define ENTT_PRETTY_FUNCTION __FUNCSIG__
#       define ENTT_PRETTY_FUNCTION_PREFIX '<'
#       define ENTT_PRETTY_FUNCTION_SUFFIX '>'
#   endif
#endif


#endif

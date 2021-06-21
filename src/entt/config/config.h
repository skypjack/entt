#ifndef ENTT_CONFIG_CONFIG_H
#define ENTT_CONFIG_CONFIG_H


#if defined(__cpp_exceptions) && !defined(ENTT_NOEXCEPTION)
#   define ENTT_NOEXCEPT noexcept
#   define ENTT_THROW throw
#   define ENTT_TRY try
#   define ENTT_CATCH catch(...)
#else
#   define ENTT_NOEXCEPT
#   define ENTT_THROW
#   define ENTT_TRY if(true)
#   define ENTT_CATCH if(false)
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


#ifdef ENTT_SPARSE_PAGE
	static_assert(ENTT_SPARSE_PAGE && ((ENTT_SPARSE_PAGE & (ENTT_SPARSE_PAGE - 1)) == 0), "ENTT_SPARSE_PAGE must be a power of two");
#else
#   define ENTT_SPARSE_PAGE 4096
#endif


#ifdef ENTT_PACKED_PAGE
static_assert(ENTT_PACKED_PAGE && ((ENTT_PACKED_PAGE & (ENTT_PACKED_PAGE - 1)) == 0), "ENTT_PACKED_PAGE must be a power of two");
#else
#   define ENTT_PACKED_PAGE 1024
#endif


#ifdef ENTT_DISABLE_ASSERT
#   undef ENTT_ASSERT
#   define ENTT_ASSERT(...) (void(0))
#elif !defined ENTT_ASSERT
#   include <cassert>
#   define ENTT_ASSERT(condition, ...) assert(condition)
#endif


#ifdef ENTT_NO_ETO
#   include <type_traits>
#   define ENTT_IGNORE_IF_EMPTY std::false_type
#else
#   include <type_traits>
#   define ENTT_IGNORE_IF_EMPTY std::true_type
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

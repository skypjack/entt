#ifndef ENTT_CONFIG_CONFIG_H
#define ENTT_CONFIG_CONFIG_H

#include "version.h"

#if defined(__cpp_exceptions) && !defined(ENTT_NOEXCEPTION)
#    define ENTT_THROW throw
#    define ENTT_TRY try
#    define ENTT_CATCH catch(...)
#else
#    define ENTT_THROW
#    define ENTT_TRY if(true)
#    define ENTT_CATCH if(false)
#endif

#ifndef ENTT_NOEXCEPT
#    define ENTT_NOEXCEPT noexcept
#    define ENTT_NOEXCEPT_IF(expr) noexcept(expr)
# else
#    define ENTT_NOEXCEPT_IF(...)
#endif

#ifdef ENTT_USE_ATOMIC
#    include <atomic>
#    define ENTT_MAYBE_ATOMIC(Type) std::atomic<Type>
#else
#    define ENTT_MAYBE_ATOMIC(Type) Type
#endif

#ifndef ENTT_ID_TYPE
#    include <cstdint>
#    define ENTT_ID_TYPE std::uint32_t
#endif

#ifndef ENTT_SPARSE_PAGE
#    define ENTT_SPARSE_PAGE 4096
#endif

#ifndef ENTT_PACKED_PAGE
#    define ENTT_PACKED_PAGE 1024
#endif

#ifdef ENTT_DISABLE_ASSERT
#    undef ENTT_ASSERT
#    define ENTT_ASSERT(...) (void(0))
#elif !defined ENTT_ASSERT
#    include <cassert>
#    define ENTT_ASSERT(condition, ...) assert(condition)
#endif

#ifdef ENTT_NO_ETO
#    define ENTT_IGNORE_IF_EMPTY false
#else
#    define ENTT_IGNORE_IF_EMPTY true
#endif

#ifdef ENTT_STANDARD_CPP
#    define ENTT_NONSTD false
#else
#    define ENTT_NONSTD true
#    if defined __clang__ || defined __GNUC__
#        define ENTT_PRETTY_FUNCTION __PRETTY_FUNCTION__
#        define ENTT_PRETTY_FUNCTION_PREFIX '='
#        define ENTT_PRETTY_FUNCTION_SUFFIX ']'
#    elif defined _MSC_VER
#        define ENTT_PRETTY_FUNCTION __FUNCSIG__
#        define ENTT_PRETTY_FUNCTION_PREFIX '<'
#        define ENTT_PRETTY_FUNCTION_SUFFIX '>'
#    endif
#endif

#if defined _MSC_VER
#    pragma detect_mismatch("entt.version", ENTT_VERSION)
#    pragma detect_mismatch("entt.noexcept", ENTT_XSTR(ENTT_TRY))
#    pragma detect_mismatch("entt.id", ENTT_XSTR(ENTT_ID_TYPE))
#    pragma detect_mismatch("entt.nonstd", ENTT_XSTR(ENTT_NONSTD))
#endif

#endif

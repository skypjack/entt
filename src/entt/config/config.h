#ifndef ENTT_CONFIG_CONFIG_H
#define ENTT_CONFIG_CONFIG_H


#ifndef ENTT_NOEXCEPT
#   define ENTT_NOEXCEPT noexcept
#endif


#ifndef ENTT_HS_SUFFIX
#   define ENTT_HS_SUFFIX _hs
#endif


#ifndef ENTT_HWS_SUFFIX
#   define ENTT_HWS_SUFFIX _hws
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


#ifndef ENTT_PAGE_SIZE
#   define ENTT_PAGE_SIZE 32768
#endif


#ifndef ENTT_ASSERT
#   include <cassert>
#   define ENTT_ASSERT(condition) assert(condition)
#endif


#ifndef ENTT_NO_ETO
#   include <type_traits>
#   define ENTT_IS_EMPTY(Type) std::is_empty_v<Type>
#else
#   include <type_traits>
#   // sfinae-friendly definition
#   define ENTT_IS_EMPTY(Type) (void(std::is_empty_v<Type>), false)
#endif


#ifndef ENTT_STANDARD_CPP
#   if defined __clang__ || (defined __GNUC__ && __GNUC__ > 8)
#       define ENTT_PRETTY_FUNCTION_CONSTEXPR
#       define ENTT_PRETTY_FUNCTION __PRETTY_FUNCTION__
#       define ENTT_PRETTY_FUNCTION_PREFIX '='
#       define ENTT_PRETTY_FUNCTION_SUFFIX ']'
#   elif defined __GNUC__
#       define ENTT_PRETTY_FUNCTION __PRETTY_FUNCTION__
#       define ENTT_PRETTY_FUNCTION_PREFIX '='
#       define ENTT_PRETTY_FUNCTION_SUFFIX ']'
#   elif defined _MSC_VER
#       define ENTT_PRETTY_FUNCTION_CONSTEXPR
#       define ENTT_PRETTY_FUNCTION __FUNCSIG__
#       define ENTT_PRETTY_FUNCTION_PREFIX '<'
#       define ENTT_PRETTY_FUNCTION_SUFFIX '>'
#   endif
#endif


#ifndef ENTT_STANDALONE
#   define ENTT_FAST_PATH(...) false
#else
#   define ENTT_FAST_PATH(Cond) Cond
#endif


#endif

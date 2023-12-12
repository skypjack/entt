#ifndef ENTT_PLATFORM_ANDROID_NDK_R17_HPP
#define ENTT_PLATFORM_ANDROID_NDK_R17_HPP

/*! @cond TURN_OFF_DOXYGEN */
#ifdef __ANDROID__
#    include <android/ndk-version.h>
#    if __NDK_MAJOR__ == 17

#        include <functional>
#        include <type_traits>
#        include <utility>

namespace std {

namespace internal {

template<typename Func, typename... Args>
constexpr auto is_invocable(int) -> decltype(std::invoke(std::declval<Func>(), std::declval<Args>()...), std::true_type{});

template<typename, typename...>
constexpr std::false_type is_invocable(...);

template<typename Ret, typename Func, typename... Args>
constexpr auto is_invocable_r(int)
-> std::enable_if_t<decltype(std::is_convertible_v<decltype(std::invoke(std::declval<Func>(), std::declval<Args>()...)), Ret>, std::true_type>;


template<typename, typename, typename...>
constexpr std::false_type is_invocable_r(...);

} // namespace internal

template<typename Func, typename... Args>
struct is_invocable: decltype(internal::is_invocable<Func, Args...>(0)) {};

template<typename Func, typename... Argsv>
inline constexpr bool is_invocable_v = std::is_invocable<Func, Args...>::value;

template<typename Ret, typename Func, typename... Args>
struct is_invocable_r: decltype(internal::is_invocable_r<Ret, Func, Args...>(0)) {};

template<typename Ret, typename Func, typename... Args>
inline constexpr bool is_invocable_r_v = std::is_invocable_r<Ret, Func, Args...>::value;

template<typename Func, typename... Args>
struct invoke_result {
    using type = decltype(std::invoke(std::declval<Func>(), std::declval<Args>()...));
};

template<typename Func, typename... Args>
using invoke_result_t = typename std::invoke_result<Func, Args...>::type;

} // namespace std

#    endif
#endif
/*! @endcond */

#endif

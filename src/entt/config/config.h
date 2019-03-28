#ifndef ENTT_CONFIG_CONFIG_H
#define ENTT_CONFIG_CONFIG_H


#ifndef ENTT_NOEXCEPT
#define ENTT_NOEXCEPT noexcept
#endif // ENTT_NOEXCEPT


#ifndef ENTT_HS_SUFFIX
#define ENTT_HS_SUFFIX _hs
#endif // ENTT_HS_SUFFIX


#ifndef ENTT_NO_ATOMIC
#include <atomic>
template<typename Type>
using maybe_atomic_t = std::atomic<Type>;
#else // ENTT_NO_ATOMIC
template<typename Type>
using maybe_atomic_t = Type;
#endif // ENTT_NO_ATOMIC


#ifndef ENTT_ID_TYPE
#include <cstdint>
#define ENTT_ID_TYPE std::uint32_t
#endif // ENTT_ID_TYPE


#ifndef ENTT_PAGE_SIZE
#define ENTT_PAGE_SIZE 32768
#endif


#ifndef ENTT_DISABLE_ASSERT
#include <cassert>
#define ENTT_ASSERT(condition) assert(condition)
#else // ENTT_DISABLE_ASSERT
#define ENTT_ASSERT(...) ((void)0)
#endif // ENTT_DISABLE_ASSERT


#endif // ENTT_CONFIG_CONFIG_H

#ifndef ENTT_CONFIG_CONFIG_H
#define ENTT_CONFIG_CONFIG_H


#ifndef ENTT_NOEXCEPT
#define ENTT_NOEXCEPT noexcept
#endif // ENTT_NOEXCEPT


#ifndef ENTT_HS_SUFFIX
#define ENTT_HS_SUFFIX _hs
#endif // ENTT_HS_SUFFIX


#ifndef ENTT_TAG_SUFFIX
#define ENTT_TAG_SUFFIX _tag
#endif // ENTT_TAG_SUFFIX


#ifndef ENTT_NO_ATOMIC
#include <atomic>
template<typename Type>
using maybe_atomic_t = std::atomic<Type>;
#else // ENTT_USE_ATOMIC
template<typename Type>
using maybe_atomic_t = Type;
#endif // ENTT_USE_ATOMIC


#ifndef ENTT_ID_TYPE
#include <cstdint>
#define ENTT_ID_TYPE std::uint32_t
#endif // ENTT_ID_TYPE


#ifndef ENTT_ENTITY_TYPE
#include <cstdint>
#define ENTT_ENTITY_TYPE std::uint32_t
#endif // ENTT_ENTITY_TYPE


#endif // ENTT_CONFIG_CONFIG_H

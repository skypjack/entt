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
using maybe_atomic_type = std::atomic<Type>;
#else
template<typename Type>
using maybe_atomic_type = Type;
#endif // ENTT_USE_ATOMIC


#endif // ENTT_CONFIG_CONFIG_H

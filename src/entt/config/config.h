#ifndef ENTT_CONFIG_CONFIG_H
#define ENTT_CONFIG_CONFIG_H


#ifndef ENTT_NOEXCEPT
#define ENTT_NOEXCEPT noexcept
#endif


//parallel algorithms only on C++ 17
#if _HAS_CXX17

#ifndef ENTT_HAS_PARALLEL_VIEW
#define ENTT_HAS_PARALLEL_VIEW
#endif

#endif /* _HAS_CXX17 */

#endif // ENTT_CONFIG_CONFIG_H

#ifndef ENTT_CONFIG_CONFIG_H
#define ENTT_CONFIG_CONFIG_H


#ifndef ENTT_NOEXCEPT
#define ENTT_NOEXCEPT noexcept
#endif // ENTT_NOEXCEPT


#ifndef ENTT_HS_SUFFIX
#define ENTT_HS_SUFFIX _hs
#endif // ENTT_HS_SUFFIX


#ifndef ENTT_EXPORT
#if defined _WIN32 || defined __CYGWIN__
#define ENTT_EXPORT __declspec(dllexport)
#elif defined __GNUC__
#define ENTT_EXPORT __attribute__((visibility("default")))
#else
#define ENTT_EXPORT
#endif
#endif // ENTT_EXPORT


#ifndef ENTT_FORCE_EXPORT_FAMILY
#define ENTT_FORCE_EXPORT_FAMILY 0
#endif // ENTT_FORCE_EXPORT_FAMILY

#endif // ENTT_CONFIG_CONFIG_H

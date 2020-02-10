#ifndef ENTT_CORE_ATTRIBUTE_H
#define ENTT_CORE_ATTRIBUTE_H


#ifndef ENTT_EXPORT
#   if defined _WIN32 || defined __CYGWIN__ || defined _MSC_VER
#       define ENTT_EXPORT __declspec(dllexport)
#       define ENTT_IMPORT __declspec(dllimport)
#       define ENTT_HIDDEN
#   elif defined __GNUC__ && __GNUC__ >= 4
#       define ENTT_EXPORT __attribute__((visibility("default")))
#       define ENTT_IMPORT __attribute__((visibility("default")))
#       define ENTT_HIDDEN __attribute__((visibility("hidden")))
#   else /* Unsupported compiler */
#       define ENTT_EXPORT
#       define ENTT_IMPORT
#       define ENTT_HIDDEN
#   endif
#endif


#ifndef ENTT_API
#   if defined ENTT_API_EXPORT
#       define ENTT_API ENTT_EXPORT
#   elif defined ENTT_API_IMPORT
#       define ENTT_API ENTT_IMPORT
#   else /* No API */
#       define ENTT_API
#   endif
#endif


#endif

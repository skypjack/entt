#include <entt/signal/dispatcher.hpp>
#include "types.h"

#ifndef LIB_EXPORT
#if defined _WIN32 || defined __CYGWIN__
#define LIB_EXPORT __declspec(dllexport)
#elif defined __GNUC__
#define LIB_EXPORT __attribute__((visibility("default")))
#else
#define LIB_EXPORT
#endif
#endif

LIB_EXPORT void trigger_event(int value, entt::dispatcher &dispatcher) {
    dispatcher.trigger<event>(value);
}

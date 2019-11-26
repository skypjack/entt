#include <entt/signal/emitter.hpp>
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

LIB_EXPORT void emit_event(int value, test_emitter &emitter) {
    emitter.publish<event>(value);
}

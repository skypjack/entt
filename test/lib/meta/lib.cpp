#include <entt/meta/factory.hpp>
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

LIB_EXPORT void bind_ctx(entt::meta_ctx context) {
    entt::meta_ctx::bind(context);
}

LIB_EXPORT void meta_init() {
    entt::meta<char>().type().data<'c'>("c"_hs);
    entt::meta<int>().type().data<0>("0"_hs);
}

LIB_EXPORT void meta_deinit() {
    entt::meta<char>().reset();
    entt::meta<int>().reset();
}

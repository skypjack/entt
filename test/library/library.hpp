#ifndef ENTT_TEST_ENTTLIBRARY
#define ENTT_TEST_ENTTLIBRARY

#include <entt/entity/registry.hpp>

namespace enttlibrary {
struct ENTT_EXPORT SomeLibraryTagA {};
struct ENTT_EXPORT SomeLibraryTagB {};
struct ENTT_EXPORT SomeLibraryTagC {};
struct ENTT_EXPORT SomeLibraryComponentA {};
struct ENTT_EXPORT SomeLibraryComponentB {};
struct ENTT_EXPORT SomeLibraryComponentC {};
void ENTT_EXPORT init_library_for_user(entt::Registry<uint32_t> &r);
}

#endif

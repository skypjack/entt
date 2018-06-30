#define ENTT_FORCE_EXPORT_FAMILY 1
#include <entt/entt.hpp>
#include <cassert>
#include <cstdint>
#include "library.hpp"

struct PrivateComponent {};
struct PrivateTag {};

void enttlibrary::init_library_for_user(entt::Registry<std::uint32_t> &r) {
    auto foo = r.create();
    r.assign<PrivateTag>(entt::tag_t{}, foo);
    r.assign<SomeLibraryTagA>(entt::tag_t{}, foo);
    r.assign<SomeLibraryTagB>(entt::tag_t{}, foo);
    r.assign<SomeLibraryTagC>(entt::tag_t{}, foo);
    r.assign<PrivateComponent>(foo);
    r.assign<SomeLibraryComponentA>(foo);
    r.assign<SomeLibraryComponentB>(foo);
    r.assign<SomeLibraryComponentC>(foo);
}

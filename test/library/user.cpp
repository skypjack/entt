#include <cstdint>
#include <entt/entity/registry.hpp>
#include <gtest/gtest.h>

#include "library.hpp"

struct ALocalTag {};
struct ALocalComponent {};

TEST(Library, User) {
    entt::Registry<uint32_t> r;
    EXPECT_EQ(0, r.type<ALocalTag>(entt::tag_t{}));
    EXPECT_EQ(0, r.type<ALocalComponent>());

    enttlibrary::init_library_for_user(r);

    EXPECT_EQ(0, r.type<ALocalTag>(entt::tag_t{}));
    EXPECT_EQ(0, r.type<ALocalComponent>());

    EXPECT_EQ(4, r.type<enttlibrary::SomeLibraryTagC>(entt::tag_t{}));
    EXPECT_EQ(3, r.type<enttlibrary::SomeLibraryTagB>(entt::tag_t{}));
    EXPECT_EQ(2, r.type<enttlibrary::SomeLibraryTagA>(entt::tag_t{}));
    EXPECT_EQ(4, r.type<enttlibrary::SomeLibraryComponentC>());
    EXPECT_EQ(3, r.type<enttlibrary::SomeLibraryComponentB>());
    EXPECT_EQ(2, r.type<enttlibrary::SomeLibraryComponentA>());
}

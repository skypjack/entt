#include <gtest/gtest.h>
#include <entt/core/hashed_string.hpp>
#include <entt/meta/meta.hpp>
#include <entt/meta/pointer.hpp>
#include <entt/meta/resolve.hpp>

TEST(MetaPointerLike, DereferenceOperatorInvalidType) {
    int value = 0;
    entt::meta_any any{value};

    ASSERT_FALSE(any.type().is_pointer());
    ASSERT_FALSE(any.type().is_pointer_like());
    ASSERT_EQ(any.type(), entt::resolve<int>());

    auto deref = *any;

    ASSERT_FALSE(deref);
}

TEST(MetaPointerLike, DereferenceOperatorConstType) {
    const int value = 0;
    entt::meta_any any{&value};

    ASSERT_TRUE(any.type().is_pointer());
    ASSERT_TRUE(any.type().is_pointer_like());
    ASSERT_EQ(any.type(), entt::resolve<const int *>());

    auto deref = *any;

    ASSERT_TRUE(deref);
    ASSERT_FALSE(deref.type().is_pointer());
    ASSERT_FALSE(deref.type().is_pointer_like());
    ASSERT_EQ(deref.type(), entt::resolve<int>());

    deref.cast<int>() = 42;

    ASSERT_EQ(*any.cast<const int *>(), 0);
    ASSERT_EQ(value, 0);
}

TEST(MetaPointerLike, DereferenceOperatorRawPointer) {
    int value = 0;
    entt::meta_any any{&value};

    ASSERT_TRUE(any.type().is_pointer());
    ASSERT_TRUE(any.type().is_pointer_like());
    ASSERT_EQ(any.type(), entt::resolve<int *>());

    auto deref = *any;

    ASSERT_TRUE(deref);
    ASSERT_FALSE(deref.type().is_pointer());
    ASSERT_FALSE(deref.type().is_pointer_like());
    ASSERT_EQ(deref.type(), entt::resolve<int>());

    deref.cast<int>() = 42;

    ASSERT_EQ(*any.cast<int *>(), 42);
    ASSERT_EQ(value, 42);
}

TEST(MetaPointerLike, DereferenceOperatorSmartPointer) {
    auto value = std::make_shared<int>(0);
    entt::meta_any any{value};

    ASSERT_FALSE(any.type().is_pointer());
    ASSERT_TRUE(any.type().is_pointer_like());
    ASSERT_EQ(any.type(), entt::resolve<std::shared_ptr<int>>());

    auto deref = *any;

    ASSERT_TRUE(deref);
    ASSERT_FALSE(deref.type().is_pointer());
    ASSERT_FALSE(deref.type().is_pointer_like());
    ASSERT_EQ(deref.type(), entt::resolve<int>());

    deref.cast<int>() = 42;

    ASSERT_EQ(*any.cast<std::shared_ptr<int>>(), 42);
    ASSERT_EQ(*value, 42);
}

#include <utility>
#include <gtest/gtest.h>
#include <entt/core/hashed_string.hpp>
#include <entt/locator/locator.hpp>
#include <entt/meta/context.hpp>
#include <entt/meta/factory.hpp>
#include <entt/meta/meta.hpp>
#include <entt/meta/node.hpp>
#include <entt/meta/resolve.hpp>

struct base_1 {
    base_1() = default;
    int value_1{};
};

struct base_2 {
    base_2() = default;

    operator int() const {
        return value_2;
    }

    int value_2{};
};

struct base_3: base_2 {
    base_3() = default;
    int value_3{};
};

struct derived: base_1, base_3 {
    derived() = default;
    int value{};
};

struct MetaBase: ::testing::Test {
    void SetUp() override {
        using namespace entt::literals;

        entt::meta_factory<base_1>{}
            .data<&base_1::value_1>("value_1"_hs);

        entt::meta_factory<base_2>{}
            .conv<int>()
            .data<&base_2::value_2>("value_2"_hs);

        entt::meta_factory<base_3>{}
            .base<base_2>()
            .data<&base_3::value_3>("value_3"_hs);

        entt::meta_factory<derived>{}
            .type("derived"_hs)
            .base<base_1>()
            .base<base_3>()
            .data<&derived::value>("value"_hs);
    }

    void TearDown() override {
        entt::meta_reset();
    }
};

TEST_F(MetaBase, Base) {
    auto any = entt::resolve<derived>().construct();
    any.cast<derived &>().value_1 = 2;
    auto as_derived = any.as_ref();

    ASSERT_TRUE(any.allow_cast<base_1 &>());

    ASSERT_FALSE(any.allow_cast<char>());
    ASSERT_FALSE(as_derived.allow_cast<char>());

    ASSERT_TRUE(any);
    ASSERT_EQ(any.cast<base_1 &>().value_1, as_derived.cast<derived &>().value_1);

    any.cast<base_1 &>().value_1 = 3;

    ASSERT_EQ(any.cast<const base_1 &>().value_1, as_derived.cast<const derived &>().value_1);
}

TEST_F(MetaBase, SetGetWithMutatingThis) {
    using namespace entt::literals;

    derived instance;
    auto any = entt::forward_as_meta(instance);
    auto as_cref = std::as_const(any).as_ref();

    ASSERT_NE(static_cast<const void *>(static_cast<const base_1 *>(&instance)), static_cast<const void *>(static_cast<const base_2 *>(&instance)));
    ASSERT_NE(static_cast<const void *>(static_cast<const base_1 *>(&instance)), static_cast<const void *>(static_cast<const base_3 *>(&instance)));
    ASSERT_EQ(static_cast<const void *>(static_cast<const base_2 *>(&instance)), static_cast<const void *>(static_cast<const base_3 *>(&instance)));
    ASSERT_EQ(static_cast<const void *>(&instance), static_cast<const void *>(static_cast<const base_1 *>(&instance)));

    ASSERT_TRUE(any.set("value"_hs, 0));
    ASSERT_TRUE(any.set("value_1"_hs, 1));
    ASSERT_TRUE(any.set("value_2"_hs, 2));
    ASSERT_TRUE(any.set("value_3"_hs, 3));

    ASSERT_FALSE(as_cref.set("value"_hs, 4));
    ASSERT_FALSE(as_cref.set("value_1"_hs, 4));
    ASSERT_FALSE(as_cref.set("value_2"_hs, 4));
    ASSERT_FALSE(as_cref.set("value_3"_hs, 4));

    ASSERT_EQ(any.get("value"_hs).cast<int>(), 0);
    ASSERT_EQ(any.get("value_1"_hs).cast<const int>(), 1);
    ASSERT_EQ(any.get("value_2"_hs).cast<int>(), 2);
    ASSERT_EQ(any.get("value_3"_hs).cast<const int>(), 3);

    ASSERT_EQ(as_cref.get("value"_hs).cast<const int>(), 0);
    ASSERT_EQ(as_cref.get("value_1"_hs).cast<int>(), 1);
    ASSERT_EQ(as_cref.get("value_2"_hs).cast<const int>(), 2);
    ASSERT_EQ(as_cref.get("value_3"_hs).cast<int>(), 3);

    ASSERT_EQ(instance.value, 0);
    ASSERT_EQ(instance.value_1, 1);
    ASSERT_EQ(instance.value_2, 2);
    ASSERT_EQ(instance.value_3, 3);
}

TEST_F(MetaBase, ConvWithMutatingThis) {
    entt::meta_any any{derived{}};
    auto &&ref = any.cast<derived &>();
    auto as_cref = std::as_const(any).as_ref();
    ref.value_2 = 2;

    auto conv = std::as_const(any).allow_cast<int>();
    auto from_cref = std::as_const(as_cref).allow_cast<int>();

    ASSERT_TRUE(conv);
    ASSERT_TRUE(from_cref);
    ASSERT_EQ(conv.cast<int>(), 2);
    ASSERT_EQ(from_cref.cast<int>(), 2);

    ASSERT_TRUE(as_cref.allow_cast<int>());
    ASSERT_TRUE(any.allow_cast<int>());

    ASSERT_EQ(as_cref.cast<int>(), 2);
    ASSERT_EQ(any.cast<int>(), 2);
}

TEST_F(MetaBase, OpaqueConvWithMutatingThis) {
    entt::meta_any any{derived{}};
    auto as_cref = std::as_const(any).as_ref();
    any.cast<derived &>().value_2 = 2;

    auto conv = std::as_const(any).allow_cast(entt::resolve<int>());
    auto from_cref = std::as_const(as_cref).allow_cast(entt::resolve<int>());

    ASSERT_TRUE(conv);
    ASSERT_TRUE(from_cref);
    ASSERT_EQ(conv.cast<int>(), 2);
    ASSERT_EQ(from_cref.cast<int>(), 2);

    ASSERT_TRUE(as_cref.allow_cast(entt::resolve<int>()));
    ASSERT_TRUE(any.allow_cast(entt::resolve<int>()));

    ASSERT_EQ(as_cref.cast<int>(), 2);
    ASSERT_EQ(any.cast<int>(), 2);
}

TEST_F(MetaBase, AssignWithMutatingThis) {
    using namespace entt::literals;

    entt::meta_any dst{base_2{}};
    entt::meta_any src{derived{}};

    dst.cast<base_2 &>().value_2 = 0;
    src.cast<derived &>().value_2 = 1;

    ASSERT_TRUE(dst.assign(src));
    ASSERT_EQ(dst.get("value_2"_hs).cast<int>(), 1);
}

TEST_F(MetaBase, TransferWithMutatingThis) {
    using namespace entt::literals;

    entt::meta_any dst{base_2{}};
    entt::meta_any src{derived{}};

    dst.cast<base_2 &>().value_2 = 0;
    src.cast<derived &>().value_2 = 1;

    ASSERT_TRUE(dst.assign(std::move(src)));
    ASSERT_EQ(dst.get("value_2"_hs).cast<int>(), 1);
}

TEST_F(MetaBase, ReRegistration) {
    SetUp();

    auto &&node = entt::internal::resolve<derived>(entt::internal::meta_context::from(entt::locator<entt::meta_ctx>::value_or()));

    ASSERT_TRUE(node.details);
    ASSERT_FALSE(node.details->base.empty());
    ASSERT_EQ(node.details->base.size(), 2u);
}

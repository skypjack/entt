#include <utility>
#include <gtest/gtest.h>
#include <entt/core/hashed_string.hpp>
#include <entt/meta/factory.hpp>
#include <entt/meta/meta.hpp>
#include <entt/meta/node.hpp>
#include <entt/meta/resolve.hpp>

struct base_1_t {
    base_1_t() = default;
    int value_1{};
};

struct base_2_t {
    base_2_t() = default;

    operator int() const {
        return value_2;
    }

    int value_2{};
};

struct base_3_t: base_2_t {
    base_3_t() = default;
    int value_3{};
};

struct derived_t: base_1_t, base_3_t {
    derived_t() = default;
    int value{};
};

struct MetaBase: ::testing::Test {
    void SetUp() override {
        using namespace entt::literals;

        entt::meta<base_1_t>()
            .data<&base_1_t::value_1>("value_1"_hs);

        entt::meta<base_2_t>()
            .conv<int>()
            .data<&base_2_t::value_2>("value_2"_hs);

        entt::meta<base_3_t>()
            .base<base_2_t>()
            .data<&base_3_t::value_3>("value_3"_hs);

        entt::meta<derived_t>()
            .type("derived"_hs)
            .base<base_1_t>()
            .base<base_3_t>()
            .data<&derived_t::value>("value"_hs);
    }

    void TearDown() override {
        entt::meta_reset();
    }
};

TEST_F(MetaBase, Functionalities) {
    auto any = entt::resolve<derived_t>().construct();
    any.cast<derived_t &>().value_1 = 42;
    auto as_derived = any.as_ref();

    ASSERT_TRUE(any.allow_cast<base_1_t &>());

    ASSERT_FALSE(any.allow_cast<char>());
    ASSERT_FALSE(as_derived.allow_cast<char>());

    ASSERT_TRUE(any);
    ASSERT_EQ(any.cast<base_1_t &>().value_1, as_derived.cast<derived_t &>().value_1);

    any.cast<base_1_t &>().value_1 = 3;

    ASSERT_EQ(any.cast<const base_1_t &>().value_1, as_derived.cast<const derived_t &>().value_1);
}

TEST_F(MetaBase, SetGetWithMutatingThis) {
    using namespace entt::literals;

    derived_t instance;
    auto any = entt::forward_as_meta(instance);
    auto as_cref = std::as_const(any).as_ref();

    ASSERT_NE(static_cast<const void *>(static_cast<const base_1_t *>(&instance)), static_cast<const void *>(static_cast<const base_2_t *>(&instance)));
    ASSERT_NE(static_cast<const void *>(static_cast<const base_1_t *>(&instance)), static_cast<const void *>(static_cast<const base_3_t *>(&instance)));
    ASSERT_EQ(static_cast<const void *>(static_cast<const base_2_t *>(&instance)), static_cast<const void *>(static_cast<const base_3_t *>(&instance)));
    ASSERT_EQ(static_cast<const void *>(&instance), static_cast<const void *>(static_cast<const base_1_t *>(&instance)));

    ASSERT_TRUE(any.set("value"_hs, 42));
    ASSERT_TRUE(any.set("value_1"_hs, 1));
    ASSERT_TRUE(any.set("value_2"_hs, 2));
    ASSERT_TRUE(any.set("value_3"_hs, 3));

    ASSERT_FALSE(as_cref.set("value"_hs, 0));
    ASSERT_FALSE(as_cref.set("value_1"_hs, 0));
    ASSERT_FALSE(as_cref.set("value_2"_hs, 0));
    ASSERT_FALSE(as_cref.set("value_3"_hs, 0));

    ASSERT_EQ(any.get("value"_hs).cast<int>(), 42);
    ASSERT_EQ(any.get("value_1"_hs).cast<const int>(), 1);
    ASSERT_EQ(any.get("value_2"_hs).cast<int>(), 2);
    ASSERT_EQ(any.get("value_3"_hs).cast<const int>(), 3);

    ASSERT_EQ(as_cref.get("value"_hs).cast<const int>(), 42);
    ASSERT_EQ(as_cref.get("value_1"_hs).cast<int>(), 1);
    ASSERT_EQ(as_cref.get("value_2"_hs).cast<const int>(), 2);
    ASSERT_EQ(as_cref.get("value_3"_hs).cast<int>(), 3);

    ASSERT_EQ(instance.value, 42);
    ASSERT_EQ(instance.value_1, 1);
    ASSERT_EQ(instance.value_2, 2);
    ASSERT_EQ(instance.value_3, 3);
}

TEST_F(MetaBase, ConvWithMutatingThis) {
    entt::meta_any any{derived_t{}};
    auto as_cref = std::as_const(any).as_ref();
    any.cast<derived_t &>().value_2 = 42;

    auto conv = std::as_const(any).allow_cast<int>();
    auto from_cref = std::as_const(as_cref).allow_cast<int>();

    ASSERT_TRUE(conv);
    ASSERT_TRUE(from_cref);
    ASSERT_EQ(conv.cast<int>(), 42);
    ASSERT_EQ(from_cref.cast<int>(), 42);

    ASSERT_TRUE(any.allow_cast<int>());
    ASSERT_TRUE(as_cref.allow_cast<int>());
    ASSERT_EQ(any.cast<int>(), 42);
    ASSERT_EQ(as_cref.cast<int>(), 42);
}

TEST_F(MetaBase, OpaqueConvWithMutatingThis) {
    entt::meta_any any{derived_t{}};
    auto as_cref = std::as_const(any).as_ref();
    any.cast<derived_t &>().value_2 = 42;

    auto conv = std::as_const(any).allow_cast(entt::resolve<int>());
    auto from_cref = std::as_const(as_cref).allow_cast(entt::resolve<int>());

    ASSERT_TRUE(conv);
    ASSERT_TRUE(from_cref);
    ASSERT_EQ(conv.cast<int>(), 42);
    ASSERT_EQ(from_cref.cast<int>(), 42);

    ASSERT_TRUE(any.allow_cast(entt::resolve<int>()));
    ASSERT_TRUE(as_cref.allow_cast(entt::resolve<int>()));
    ASSERT_EQ(any.cast<int>(), 42);
    ASSERT_EQ(as_cref.cast<int>(), 42);
}

TEST_F(MetaBase, AssignWithMutatingThis) {
    using namespace entt::literals;

    entt::meta_any dst{base_2_t{}};
    entt::meta_any src{derived_t{}};

    dst.cast<base_2_t &>().value_2 = 0;
    src.cast<derived_t &>().value_2 = 42;

    ASSERT_TRUE(dst.assign(src));
    ASSERT_EQ(dst.get("value_2"_hs).cast<int>(), 42);
}

TEST_F(MetaBase, TransferWithMutatingThis) {
    using namespace entt::literals;

    entt::meta_any dst{base_2_t{}};
    entt::meta_any src{derived_t{}};

    dst.cast<base_2_t &>().value_2 = 0;
    src.cast<derived_t &>().value_2 = 42;

    ASSERT_TRUE(dst.assign(std::move(src)));
    ASSERT_EQ(dst.get("value_2"_hs).cast<int>(), 42);
}

TEST_F(MetaBase, ReRegistration) {
    SetUp();

    auto *node = entt::internal::meta_node<derived_t>::resolve();

    ASSERT_NE(node->base, nullptr);
    ASSERT_NE(node->base->type->base, nullptr);
    ASSERT_EQ(node->base->type->base->next, nullptr);
    ASSERT_EQ(node->base->type->base->type->base, nullptr);

    ASSERT_NE(node->base->next, nullptr);
    ASSERT_EQ(node->base->next->type->base, nullptr);

    ASSERT_EQ(node->base->next->next, nullptr);
}

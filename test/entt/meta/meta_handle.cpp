#include <utility>
#include <gtest/gtest.h>
#include <entt/core/hashed_string.hpp>
#include <entt/locator/locator.hpp>
#include <entt/meta/context.hpp>
#include <entt/meta/factory.hpp>
#include <entt/meta/meta.hpp>

struct clazz {
    void incr() {
        ++value;
    }

    void decr() {
        --value;
    }

    int value{};
};

struct MetaHandle: ::testing::Test {
    void SetUp() override {
        using namespace entt::literals;

        entt::meta_factory<clazz>{}
            .type("clazz"_hs)
            .func<&clazz::incr>("incr"_hs)
            .func<&clazz::decr>("decr"_hs);
    }

    void TearDown() override {
        entt::meta_reset();
    }
};

TEST_F(MetaHandle, Handle) {
    using namespace entt::literals;

    clazz instance{};
    entt::meta_handle handle{};
    entt::meta_handle chandle{};

    ASSERT_FALSE(handle);
    ASSERT_FALSE(chandle);

    handle = entt::meta_handle{instance};
    chandle = entt::meta_handle{std::as_const(instance)};

    ASSERT_TRUE(handle);
    ASSERT_TRUE(chandle);

    ASSERT_TRUE(handle->invoke("incr"_hs));
    ASSERT_FALSE(chandle->invoke("incr"_hs));
    ASSERT_FALSE(std::as_const(handle)->invoke("incr"_hs));
    ASSERT_EQ(instance.value, 1);

    auto any = entt::forward_as_meta(instance);
    handle = entt::meta_handle{any};
    chandle = entt::meta_handle{std::as_const(any)};

    ASSERT_TRUE(handle->invoke("decr"_hs));
    ASSERT_FALSE(chandle->invoke("decr"_hs));
    ASSERT_FALSE(std::as_const(handle)->invoke("decr"_hs));
    ASSERT_EQ(instance.value, 0);
}

TEST_F(MetaHandle, Value) {
    int value{2};
    entt::meta_handle handle{value};
    entt::meta_handle chandle{std::as_const(value)};

    ASSERT_NE(handle->try_cast<int>(), nullptr);
    ASSERT_NE(handle->try_cast<const int>(), nullptr);
    ASSERT_EQ(chandle->try_cast<int>(), nullptr);
    ASSERT_NE(chandle->try_cast<const int>(), nullptr);

    ASSERT_EQ(&handle->context(), &entt::locator<entt::meta_ctx>::value_or());
    ASSERT_EQ(&chandle->context(), &entt::locator<entt::meta_ctx>::value_or());
}

TEST_F(MetaHandle, MetaAny) {
    entt::meta_any value{2};
    entt::meta_handle handle{value};
    entt::meta_handle chandle{std::as_const(value)};

    ASSERT_NE(handle->try_cast<int>(), nullptr);
    ASSERT_NE(handle->try_cast<const int>(), nullptr);
    ASSERT_EQ(chandle->try_cast<int>(), nullptr);
    ASSERT_NE(chandle->try_cast<const int>(), nullptr);

    ASSERT_EQ(&handle->context(), &entt::locator<entt::meta_ctx>::value_or());
    ASSERT_EQ(&chandle->context(), &entt::locator<entt::meta_ctx>::value_or());
}

TEST_F(MetaHandle, ScopedMetaAny) {
    const entt::meta_ctx ctx{};
    entt::meta_any value{ctx, 2};
    entt::meta_handle handle{value};
    entt::meta_handle chandle{std::as_const(value)};

    ASSERT_NE(handle->try_cast<int>(), nullptr);
    ASSERT_NE(handle->try_cast<const int>(), nullptr);
    ASSERT_EQ(chandle->try_cast<int>(), nullptr);
    ASSERT_NE(chandle->try_cast<const int>(), nullptr);

    ASSERT_EQ(&handle->context(), &ctx);
    ASSERT_EQ(&chandle->context(), &ctx);
}

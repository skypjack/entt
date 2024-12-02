#include <gtest/gtest.h>
#include <entt/core/hashed_string.hpp>
#include <entt/meta/factory.hpp>
#include <entt/meta/meta.hpp>
#include <entt/meta/resolve.hpp>
#include "../../common/config.h"

struct clazz {
    int i{2};
    char j{'c'};

    [[nodiscard]] int f(int) const {
        return i;
    }

    [[nodiscard]] char g(char) const {
        return j;
    }
};

struct MetaCustom: ::testing::Test {
    void SetUp() override {
        using namespace entt::literals;

        entt::meta_factory<clazz>{}
            .type("clazz"_hs)
            .custom<char>('c')
            .data<&clazz::i>("i"_hs)
            .custom<int>(0)
            .data<&clazz::j>("j"_hs)
            .func<&clazz::f>("f"_hs)
            .custom<int>(1)
            .func<&clazz::g>("g"_hs);
    }

    void TearDown() override {
        entt::meta_reset();
    }
};

using MetaCustomDeathTest = MetaCustom;

TEST_F(MetaCustom, Custom) {
    entt::meta_custom custom{};

    ASSERT_EQ(static_cast<const char *>(custom), nullptr);

    custom = entt::resolve<clazz>().custom();

    ASSERT_NE(static_cast<const char *>(custom), nullptr);

    ASSERT_EQ(*static_cast<const char *>(custom), 'c');
    ASSERT_EQ(static_cast<const char &>(custom), 'c');
}

ENTT_DEBUG_TEST_F(MetaCustomDeathTest, Custom) {
    entt::meta_custom custom{};

    ASSERT_DEATH([[maybe_unused]] const char &value = custom, "");

    custom = entt::resolve<clazz>().custom();

    ASSERT_DEATH([[maybe_unused]] const int &value = custom, "");
}

TEST_F(MetaCustom, Type) {
    ASSERT_NE(static_cast<const char *>(entt::resolve<clazz>().custom()), nullptr);
    ASSERT_EQ(*static_cast<const char *>(entt::resolve<clazz>().custom()), 'c');
    ASSERT_EQ(static_cast<const char &>(entt::resolve<clazz>().custom()), 'c');

    ASSERT_EQ(static_cast<const int *>(entt::resolve<clazz>().custom()), nullptr);
    ASSERT_EQ(static_cast<const char *>(entt::resolve<int>().custom()), nullptr);
}

TEST_F(MetaCustom, Data) {
    using namespace entt::literals;

    const clazz instance{};

    ASSERT_TRUE(entt::resolve<clazz>().data("i"_hs));
    ASSERT_EQ(entt::resolve<clazz>().get("i"_hs, instance).cast<int>(), 2);

    ASSERT_TRUE(entt::resolve<clazz>().data("j"_hs));
    ASSERT_EQ(entt::resolve<clazz>().get("j"_hs, instance).cast<char>(), 'c');

    ASSERT_NE(static_cast<const int *>(entt::resolve<clazz>().data("i"_hs).custom()), nullptr);
    ASSERT_EQ(*static_cast<const int *>(entt::resolve<clazz>().data("i"_hs).custom()), 0);
    ASSERT_EQ(static_cast<const int &>(entt::resolve<clazz>().data("i"_hs).custom()), 0);

    ASSERT_EQ(static_cast<const char *>(entt::resolve<clazz>().data("i"_hs).custom()), nullptr);
    ASSERT_EQ(static_cast<const int *>(entt::resolve<clazz>().data("j"_hs).custom()), nullptr);
}

TEST_F(MetaCustom, Func) {
    using namespace entt::literals;

    const clazz instance{};

    ASSERT_TRUE(entt::resolve<clazz>().func("f"_hs));
    ASSERT_EQ(entt::resolve<clazz>().invoke("f"_hs, instance, 0).cast<int>(), 2);

    ASSERT_TRUE(entt::resolve<clazz>().func("g"_hs));
    ASSERT_EQ(entt::resolve<clazz>().invoke("g"_hs, instance, 'c').cast<char>(), 'c');

    ASSERT_NE(static_cast<const int *>(entt::resolve<clazz>().func("f"_hs).custom()), nullptr);
    ASSERT_EQ(*static_cast<const int *>(entt::resolve<clazz>().func("f"_hs).custom()), 1);
    ASSERT_EQ(static_cast<const int &>(entt::resolve<clazz>().func("f"_hs).custom()), 1);

    ASSERT_EQ(static_cast<const char *>(entt::resolve<clazz>().func("f"_hs).custom()), nullptr);
    ASSERT_EQ(static_cast<const int *>(entt::resolve<clazz>().func("g"_hs).custom()), nullptr);
}

TEST_F(MetaCustom, ConstNonConstAndAllInBetween) {
    testing::StaticAssertTypeEq<decltype(static_cast<int *>(entt::meta_custom{})), int *>();
    testing::StaticAssertTypeEq<decltype(static_cast<int &>(entt::meta_custom{})), int &>();
    testing::StaticAssertTypeEq<decltype(static_cast<const int *>(entt::meta_custom{})), const int *>();
    testing::StaticAssertTypeEq<decltype(static_cast<const int &>(entt::meta_custom{})), const int &>();

    static_cast<char &>(entt::resolve<clazz>().custom()) = '\n';

    ASSERT_EQ(*static_cast<const char *>(entt::resolve<clazz>().custom()), '\n');
}

TEST_F(MetaCustom, ReRegistration) {
    using namespace entt::literals;

    SetUp();

    auto type = entt::resolve<clazz>();

    ASSERT_EQ(static_cast<const int *>(type.custom()), nullptr);
    ASSERT_NE(static_cast<const char *>(type.custom()), nullptr);
    ASSERT_EQ(*static_cast<const char *>(type.custom()), 'c');

    entt::meta_factory<clazz>{}.custom<int>(1);
    type = entt::resolve<clazz>();

    ASSERT_NE(static_cast<const int *>(type.custom()), nullptr);
    ASSERT_EQ(static_cast<const char *>(type.custom()), nullptr);
    ASSERT_EQ(*static_cast<const int *>(type.custom()), 1);
}

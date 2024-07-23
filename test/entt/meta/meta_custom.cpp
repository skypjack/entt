#include <gtest/gtest.h>
#include <entt/core/hashed_string.hpp>
#include <entt/core/type_info.hpp>
#include <entt/locator/locator.hpp>
#include <entt/meta/context.hpp>
#include <entt/meta/factory.hpp>
#include <entt/meta/meta.hpp>
#include <entt/meta/resolve.hpp>
#include "../../common/config.h"

struct clazz {
    int i{0};
    char j{1};

    void f(int) {}
    void g(char) {}
};

struct MetaCustom: ::testing::Test {
    void SetUp() override {
        using namespace entt::literals;

        entt::meta<clazz>()
            .type("clazz"_hs)
            .custom<char>('c')
            .data<&clazz::i>("i"_hs)
            .custom<int>(2)
            .data<&clazz::j>("j"_hs)
            .func<&clazz::f>("f"_hs)
            .custom<int>(3)
            .func<&clazz::g>("g"_hs);
    }

    void TearDown() override {
        entt::meta_reset();
    }
};

using MetaCustomDeathTest = MetaCustom;

TEST_F(MetaCustom, Functionalities) {
    entt::meta_custom custom{};

    ASSERT_EQ(static_cast<const char *>(custom), nullptr);

    custom = entt::resolve<clazz>().custom();

    ASSERT_NE(static_cast<const char *>(custom), nullptr);

    ASSERT_EQ(*static_cast<const char *>(custom), 'c');
    ASSERT_EQ(static_cast<const char &>(custom), 'c');
}

ENTT_DEBUG_TEST_F(MetaCustomDeathTest, Functionalities) {
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

    ASSERT_NE(static_cast<const int *>(entt::resolve<clazz>().data("i"_hs).custom()), nullptr);
    ASSERT_EQ(*static_cast<const int *>(entt::resolve<clazz>().data("i"_hs).custom()), 2);
    ASSERT_EQ(static_cast<const int &>(entt::resolve<clazz>().data("i"_hs).custom()), 2);

    ASSERT_EQ(static_cast<const char *>(entt::resolve<clazz>().data("i"_hs).custom()), nullptr);
    ASSERT_EQ(static_cast<const int *>(entt::resolve<clazz>().data("j"_hs).custom()), nullptr);
}

TEST_F(MetaCustom, Func) {
    using namespace entt::literals;

    ASSERT_NE(static_cast<const int *>(entt::resolve<clazz>().func("f"_hs).custom()), nullptr);
    ASSERT_EQ(*static_cast<const int *>(entt::resolve<clazz>().func("f"_hs).custom()), 3);
    ASSERT_EQ(static_cast<const int &>(entt::resolve<clazz>().func("f"_hs).custom()), 3);

    ASSERT_EQ(static_cast<const char *>(entt::resolve<clazz>().func("f"_hs).custom()), nullptr);
    ASSERT_EQ(static_cast<const int *>(entt::resolve<clazz>().func("g"_hs).custom()), nullptr);
}

TEST_F(MetaCustom, ReRegistration) {
    using namespace entt::literals;

    SetUp();

    auto type = entt::resolve<clazz>();

    ASSERT_EQ(static_cast<const int *>(type.custom()), nullptr);
    ASSERT_NE(static_cast<const char *>(type.custom()), nullptr);
    ASSERT_EQ(*static_cast<const char *>(type.custom()), 'c');

    entt::meta<clazz>().custom<int>(1);
    type = entt::resolve<clazz>();

    ASSERT_NE(static_cast<const int *>(type.custom()), nullptr);
    ASSERT_EQ(static_cast<const char *>(type.custom()), nullptr);
    ASSERT_EQ(*static_cast<const int *>(type.custom()), 1);
}

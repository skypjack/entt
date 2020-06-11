#include <gtest/gtest.h>
#include <entt/core/hashed_string.hpp>
#include <entt/meta/factory.hpp>
#include <entt/meta/meta.hpp>
#include <entt/meta/resolve.hpp>

struct clazz_t {
    int f() const {
        return i;
    }

    static char g(const clazz_t &type) {
        return type.c;
    }

    int i{};
    char c{};
};

struct MetaConv: ::testing::Test {
    static void SetUpTestCase() {
        entt::meta<double>().conv<int>();
        entt::meta<clazz_t>().conv<&clazz_t::f>().conv<&clazz_t::g>();
    }
};

TEST_F(MetaConv, Functionalities) {
    auto conv = entt::resolve<double>().conv<int>();
    double value = 3.;

    ASSERT_TRUE(conv);
    ASSERT_EQ(conv.parent(), entt::resolve<double>());
    ASSERT_EQ(conv.type(), entt::resolve<int>());

    auto any = conv.convert(&value);

    ASSERT_TRUE(any);
    ASSERT_EQ(any.type(), entt::resolve<int>());
    ASSERT_EQ(any.cast<int>(), 3);
}

TEST_F(MetaConv, AsFreeFunctions) {
    auto conv = entt::resolve<clazz_t>().conv<int>();
    clazz_t clazz{42, 'c'};

    ASSERT_TRUE(conv);
    ASSERT_EQ(conv.parent(), entt::resolve<clazz_t>());
    ASSERT_EQ(conv.type(), entt::resolve<int>());

    auto any = conv.convert(&clazz);

    ASSERT_TRUE(any);
    ASSERT_EQ(any.type(), entt::resolve<int>());
    ASSERT_EQ(any.cast<int>(), 42);
}

TEST_F(MetaConv, AsMemberFunctions) {
    auto conv = entt::resolve<clazz_t>().conv<char>();
    clazz_t clazz{42, 'c'};

    ASSERT_TRUE(conv);
    ASSERT_EQ(conv.parent(), entt::resolve<clazz_t>());
    ASSERT_EQ(conv.type(), entt::resolve<char>());

    auto any = conv.convert(&clazz);

    ASSERT_TRUE(any);
    ASSERT_EQ(any.type(), entt::resolve<char>());
    ASSERT_EQ(any.cast<char>(), 'c');
}

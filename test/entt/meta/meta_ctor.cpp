#include <gtest/gtest.h>
#include <entt/core/hashed_string.hpp>
#include <entt/core/utility.hpp>
#include <entt/meta/factory.hpp>
#include <entt/meta/meta.hpp>
#include <entt/meta/resolve.hpp>

struct base_t { char value{}; };
struct derived_t: base_t {};

struct clazz_t {
    clazz_t(const base_t &other, int iv)
        : clazz_t{iv, other.value}
    {}

    clazz_t(const int &iv, char cv)
        : i{iv}, c{cv}
    {}

    static clazz_t factory(int value) {
        return {value, 'c'};
    }

    static clazz_t factory(base_t other, int value, int mul) {
        return {value * mul, other.value};
    }

    int i{};
    char c{};
};

struct MetaCtor: ::testing::Test {
    static void SetUpTestCase() {
        entt::meta<double>().conv<int>();
        entt::meta<derived_t>().base<base_t>();

        entt::meta<clazz_t>().type("clazz"_hs)
                .ctor<const base_t &, int>()
                .ctor<const int &, char>().prop(3, false)
                .ctor<entt::overload<clazz_t(int)>(&clazz_t::factory)>().prop('c', 42)
                .ctor<entt::overload<clazz_t(base_t, int, int)>(&clazz_t::factory)>();
    }
};

TEST_F(MetaCtor, Functionalities) {
    auto ctor = entt::resolve<clazz_t>().ctor<const int &, char>();

    ASSERT_TRUE(ctor);
    ASSERT_EQ(ctor.parent(), entt::resolve_id("clazz"_hs));
    ASSERT_EQ(ctor.size(), 2u);
    ASSERT_EQ(ctor.arg(0u), entt::resolve<int>());
    ASSERT_EQ(ctor.arg(1u), entt::resolve<char>());
    ASSERT_FALSE(ctor.arg(2u));

    auto any = ctor.invoke(42, 'c');
    auto empty = ctor.invoke();

    ASSERT_FALSE(empty);
    ASSERT_TRUE(any);
    ASSERT_EQ(any.cast<clazz_t>().i, 42);
    ASSERT_EQ(any.cast<clazz_t>().c, 'c');

    for(auto curr: ctor.prop()) {
        ASSERT_EQ(curr.key(), 3);
        ASSERT_FALSE(curr.value().template cast<bool>());
    }

    ASSERT_FALSE(ctor.prop(2));
    ASSERT_FALSE(ctor.prop('c'));

    auto prop = ctor.prop(3);

    ASSERT_TRUE(prop);
    ASSERT_EQ(prop.key(), 3);
    ASSERT_FALSE(prop.value().template cast<bool>());
}

TEST_F(MetaCtor, Func) {
    auto ctor = entt::resolve<clazz_t>().ctor<int>();

    ASSERT_TRUE(ctor);
    ASSERT_EQ(ctor.parent(), entt::resolve_id("clazz"_hs));
    ASSERT_EQ(ctor.size(), 1u);
    ASSERT_EQ(ctor.arg(0u), entt::resolve<int>());
    ASSERT_FALSE(ctor.arg(1u));

    auto any = ctor.invoke(42);
    auto empty = ctor.invoke(3, 'c');

    ASSERT_FALSE(empty);
    ASSERT_TRUE(any);
    ASSERT_EQ(any.cast<clazz_t>().i, 42);
    ASSERT_EQ(any.cast<clazz_t>().c, 'c');

    for(auto curr: ctor.prop()) {
        ASSERT_EQ(curr.key(), 'c');
        ASSERT_EQ(curr.value(), 42);
    }

    ASSERT_FALSE(ctor.prop('d'));
    ASSERT_FALSE(ctor.prop(3));

    auto prop = ctor.prop('c');

    ASSERT_TRUE(prop);
    ASSERT_EQ(prop.key(), 'c');
    ASSERT_EQ(prop.value(), 42);
}

TEST_F(MetaCtor, MetaAnyArgs) {
    auto any = entt::resolve<clazz_t>().ctor<int, char>().invoke(entt::meta_any{42}, entt::meta_any{'c'});

    ASSERT_TRUE(any);
    ASSERT_EQ(any.cast<clazz_t>().i, 42);
    ASSERT_EQ(any.cast<clazz_t>().c, 'c');
}

TEST_F(MetaCtor, InvalidArgs) {
    auto ctor = entt::resolve<clazz_t>().ctor<int, char>();
    ASSERT_FALSE(ctor.invoke('c', 42));
}

TEST_F(MetaCtor, CastAndConvert) {
    auto any = entt::resolve<clazz_t>().ctor<const base_t &, int>().invoke(derived_t{{'c'}}, 42.);

    ASSERT_TRUE(any);
    ASSERT_EQ(any.cast<clazz_t>().i, 42);
    ASSERT_EQ(any.cast<clazz_t>().c, 'c');
}

TEST_F(MetaCtor, FuncMetaAnyArgs) {
    auto any = entt::resolve<clazz_t>().ctor<base_t, int>().invoke(entt::meta_any{base_t{'c'}}, entt::meta_any{42});

    ASSERT_TRUE(any);
    ASSERT_EQ(any.cast<clazz_t>().i, 42);
    ASSERT_EQ(any.cast<clazz_t>().c, 'c');
}

TEST_F(MetaCtor, FuncInvalidArgs) {
    auto ctor = entt::resolve<clazz_t>().ctor<const base_t &, int>();
    ASSERT_FALSE(ctor.invoke(base_t{}, 'c'));
}

TEST_F(MetaCtor, FuncCastAndConvert) {
    auto any = entt::resolve<clazz_t>().ctor<base_t, int, int>().invoke(derived_t{{'c'}}, 3., 3);

    ASSERT_TRUE(any);
    ASSERT_EQ(any.cast<clazz_t>().i, 9);
    ASSERT_EQ(any.cast<clazz_t>().c, 'c');
}

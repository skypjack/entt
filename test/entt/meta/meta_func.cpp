#include <gtest/gtest.h>
#include <entt/core/hashed_string.hpp>
#include <entt/core/utility.hpp>
#include <entt/entity/registry.hpp>
#include <entt/meta/factory.hpp>
#include <entt/meta/meta.hpp>
#include <entt/meta/resolve.hpp>

struct base_t {
    virtual ~base_t() = default;
    static void destroy(base_t &) {
        ++counter;
    }

    void func(int v) {
        value = v;
    }

    inline static int counter = 0;
    int value{3};
};

struct derived_t: base_t {
    derived_t() {}
};

struct func_t {
    int f(const base_t &, int a, int b) {
        return f(a, b);
    }

    int f(int a, int b) {
        value = a;
        return b*b;
    }

    int f(int v) const {
        return v*v;
    }

    void g(int v) {
        value = v*v;
    }

    static int h(int &v) {
        return (v *= value);
    }

    static void k(int v) {
        value = v;
    }

    int v(int v) const {
        return (value = v);
    }

    int & a() const {
        return value;
    }

    inline static int value = 0;
};

struct MetaFunc: ::testing::Test {
    void SetUp() override {
        using namespace entt::literals;

        entt::meta<double>()
            .type("double"_hs)
            .conv<int>();

        entt::meta<base_t>()
            .type("base"_hs)
            .dtor<&base_t::destroy>()
            .func<&base_t::func>("func"_hs);

        entt::meta<derived_t>()
            .type("derived"_hs)
            .base<base_t>()
            .dtor<&derived_t::destroy>();

        entt::meta<func_t>()
            .type("func"_hs)
            .func<&entt::registry::emplace_or_replace<func_t>, entt::as_ref_t>("emplace"_hs)
            .func<entt::overload<int(const base_t &, int, int)>(&func_t::f)>("f3"_hs)
            .func<entt::overload<int(int, int)>(&func_t::f)>("f2"_hs).prop(true, false)
            .func<entt::overload<int(int) const>(&func_t::f)>("f1"_hs).prop(true, false)
            .func<&func_t::g>("g"_hs).prop(true, false)
            .func<&func_t::h>("h"_hs).prop(true, false)
            .func<&func_t::k>("k"_hs).prop(true, false)
            .func<&func_t::v, entt::as_void_t>("v"_hs)
            .func<&func_t::a, entt::as_ref_t>("a"_hs)
            .func<&func_t::a, entt::as_cref_t>("ca"_hs);

        base_t::counter = 0;
    }

    void TearDown() override {
        for(auto type: entt::resolve()) {
            type.reset();
        }
    }
};

TEST_F(MetaFunc, Functionalities) {
    using namespace entt::literals;

    auto func = entt::resolve<func_t>().func("f2"_hs);
    func_t instance{};

    ASSERT_TRUE(func);
    ASSERT_EQ(func.parent(), entt::resolve("func"_hs));
    ASSERT_EQ(func.id(), "f2"_hs);
    ASSERT_EQ(func.arity(), 2u);
    ASSERT_FALSE(func.is_const());
    ASSERT_FALSE(func.is_static());
    ASSERT_EQ(func.ret(), entt::resolve<int>());
    ASSERT_EQ(func.arg(0u), entt::resolve<int>());
    ASSERT_EQ(func.arg(1u), entt::resolve<int>());
    ASSERT_FALSE(func.arg(2u));

    auto any = func.invoke(instance, 3, 2);
    auto empty = func.invoke(instance);

    ASSERT_FALSE(empty);
    ASSERT_TRUE(any);
    ASSERT_EQ(any.type(), entt::resolve<int>());
    ASSERT_EQ(any.cast<int>(), 4);
    ASSERT_EQ(func_t::value, 3);

    for(auto curr: func.prop()) {
        ASSERT_EQ(curr.key(), true);
        ASSERT_FALSE(curr.value().template cast<bool>());
    }

    ASSERT_FALSE(func.prop(false));
    ASSERT_FALSE(func.prop('c'));

    auto prop = func.prop(true);

    ASSERT_TRUE(prop);
    ASSERT_EQ(prop.key(), true);
    ASSERT_FALSE(prop.value().cast<bool>());
}

TEST_F(MetaFunc, Const) {
    using namespace entt::literals;

    auto func = entt::resolve<func_t>().func("f1"_hs);
    func_t instance{};

    ASSERT_TRUE(func);
    ASSERT_EQ(func.parent(), entt::resolve("func"_hs));
    ASSERT_EQ(func.id(), "f1"_hs);
    ASSERT_EQ(func.arity(), 1u);
    ASSERT_TRUE(func.is_const());
    ASSERT_FALSE(func.is_static());
    ASSERT_EQ(func.ret(), entt::resolve<int>());
    ASSERT_EQ(func.arg(0u), entt::resolve<int>());
    ASSERT_FALSE(func.arg(1u));

    auto any = func.invoke(instance, 4);
    auto empty = func.invoke(instance, 'c');

    ASSERT_FALSE(empty);
    ASSERT_TRUE(any);
    ASSERT_EQ(any.type(), entt::resolve<int>());
    ASSERT_EQ(any.cast<int>(), 16);

    for(auto curr: func.prop()) {
        ASSERT_EQ(curr.key(), true);
        ASSERT_FALSE(curr.value().template cast<bool>());
    }

    ASSERT_FALSE(func.prop(false));
    ASSERT_FALSE(func.prop('c'));

    auto prop = func.prop(true);

    ASSERT_TRUE(prop);
    ASSERT_EQ(prop.key(), true);
    ASSERT_FALSE(prop.value().cast<bool>());
}

TEST_F(MetaFunc, RetVoid) {
    using namespace entt::literals;

    auto func = entt::resolve<func_t>().func("g"_hs);
    func_t instance{};

    ASSERT_TRUE(func);
    ASSERT_EQ(func.parent(), entt::resolve("func"_hs));
    ASSERT_EQ(func.id(), "g"_hs);
    ASSERT_EQ(func.arity(), 1u);
    ASSERT_FALSE(func.is_const());
    ASSERT_FALSE(func.is_static());
    ASSERT_EQ(func.ret(), entt::resolve<void>());
    ASSERT_EQ(func.arg(0u), entt::resolve<int>());
    ASSERT_FALSE(func.arg(1u));

    auto any = func.invoke(instance, 5);

    ASSERT_TRUE(any);
    ASSERT_EQ(any.type(), entt::resolve<void>());
    ASSERT_EQ(func_t::value, 25);

    for(auto curr: func.prop()) {
        ASSERT_EQ(curr.key(), true);
        ASSERT_FALSE(curr.value().template cast<bool>());
    }

    ASSERT_FALSE(func.prop(false));
    ASSERT_FALSE(func.prop('c'));

    auto prop = func.prop(true);

    ASSERT_TRUE(prop);
    ASSERT_EQ(prop.key(), true);
    ASSERT_FALSE(prop.value().cast<bool>());
}

TEST_F(MetaFunc, Static) {
    using namespace entt::literals;

    auto func = entt::resolve<func_t>().func("h"_hs);
    func_t::value = 2;

    ASSERT_TRUE(func);
    ASSERT_EQ(func.parent(), entt::resolve("func"_hs));
    ASSERT_EQ(func.id(), "h"_hs);
    ASSERT_EQ(func.arity(), 1u);
    ASSERT_FALSE(func.is_const());
    ASSERT_TRUE(func.is_static());
    ASSERT_EQ(func.ret(), entt::resolve<int>());
    ASSERT_EQ(func.arg(0u), entt::resolve<int>());
    ASSERT_FALSE(func.arg(1u));

    auto any = func.invoke({}, 3);
    auto empty = func.invoke({}, 'c');

    ASSERT_FALSE(empty);
    ASSERT_TRUE(any);
    ASSERT_EQ(any.type(), entt::resolve<int>());
    ASSERT_EQ(any.cast<int>(), 6);

    for(auto curr: func.prop()) {
        ASSERT_EQ(curr.key(), true);
        ASSERT_FALSE(curr.value().template cast<bool>());
    }

    ASSERT_FALSE(func.prop(false));
    ASSERT_FALSE(func.prop('c'));

    auto prop = func.prop(true);

    ASSERT_TRUE(prop);
    ASSERT_EQ(prop.key(), true);
    ASSERT_FALSE(prop.value().cast<bool>());
}

TEST_F(MetaFunc, StaticRetVoid) {
    using namespace entt::literals;

    auto func = entt::resolve<func_t>().func("k"_hs);

    ASSERT_TRUE(func);
    ASSERT_EQ(func.parent(), entt::resolve("func"_hs));
    ASSERT_EQ(func.id(), "k"_hs);
    ASSERT_EQ(func.arity(), 1u);
    ASSERT_FALSE(func.is_const());
    ASSERT_TRUE(func.is_static());
    ASSERT_EQ(func.ret(), entt::resolve<void>());
    ASSERT_EQ(func.arg(0u), entt::resolve<int>());
    ASSERT_FALSE(func.arg(1u));

    auto any = func.invoke({}, 42);

    ASSERT_TRUE(any);
    ASSERT_EQ(any.type(), entt::resolve<void>());
    ASSERT_EQ(func_t::value, 42);

    for(auto curr: func.prop()) {
        ASSERT_TRUE(curr);
        ASSERT_EQ(curr.key(), true);
        ASSERT_FALSE(curr.value().template cast<bool>());
    }

    ASSERT_FALSE(func.prop(false));
    ASSERT_FALSE(func.prop('c'));

    auto prop = func.prop(true);

    ASSERT_TRUE(prop);
    ASSERT_EQ(prop.key(), true);
    ASSERT_FALSE(prop.value().cast<bool>());
}

TEST_F(MetaFunc, MetaAnyArgs) {
    using namespace entt::literals;

    func_t instance;
    auto any = entt::resolve<func_t>().func("f1"_hs).invoke(instance, 3);

    ASSERT_TRUE(any);
    ASSERT_EQ(any.type(), entt::resolve<int>());
    ASSERT_EQ(any.cast<int>(), 9);
}

TEST_F(MetaFunc, InvalidArgs) {
    using namespace entt::literals;

    int value = 3;
    ASSERT_FALSE(entt::resolve<func_t>().func("f1"_hs).invoke(value, 'c'));
}

TEST_F(MetaFunc, CastAndConvert) {
    using namespace entt::literals;

    func_t instance;
    auto any = entt::resolve<func_t>().func("f3"_hs).invoke(instance, derived_t{}, 0, 3.);

    ASSERT_TRUE(any);
    ASSERT_EQ(any.type(), entt::resolve<int>());
    ASSERT_EQ(any.cast<int>(), 9);
}

TEST_F(MetaFunc, ArgsByRef) {
    using namespace entt::literals;

    auto func = entt::resolve<func_t>().func("h"_hs);
    func_t::value = 2;
    entt::meta_any any{3};
    int value = 4;

    ASSERT_EQ(func.invoke({}, entt::make_meta_any<int &>(value)).cast<int>(), 8);
    ASSERT_EQ(func.invoke({}, any.as_ref()).cast<int>(), 6);
    ASSERT_EQ(any.cast<int>(), 6);
    ASSERT_EQ(value, 8);
}

TEST_F(MetaFunc, ArgsByConstRef) {
    using namespace entt::literals;

    func_t instance{};
    auto func = entt::resolve<func_t>().func("g"_hs);
    entt::meta_any any{2};
    int value = 3;

    ASSERT_TRUE(func.invoke(instance, entt::make_meta_any<const int &>(value)));
    ASSERT_EQ(func_t::value, 9);

    ASSERT_TRUE(func.invoke(instance, std::as_const(any).as_ref()));
    ASSERT_EQ(func_t::value, 4);
}

TEST_F(MetaFunc, ConstInstance) {
    using namespace entt::literals;

    func_t instance{};
    auto any = entt::resolve<func_t>().func("f1"_hs).invoke(std::as_const(instance), 2);

    ASSERT_FALSE(entt::resolve<func_t>().func("g"_hs).invoke(std::as_const(instance), 42));
    ASSERT_TRUE(any);
    ASSERT_EQ(any.cast<int>(), 4);
}

TEST_F(MetaFunc, AsVoid) {
    using namespace entt::literals;

    auto func = entt::resolve<func_t>().func("v"_hs);
    func_t instance{};

    ASSERT_EQ(func.invoke(instance, 42), entt::meta_any{std::in_place_type<void>});
    ASSERT_EQ(func.ret(), entt::resolve<void>());
    ASSERT_EQ(instance.value, 42);
}

TEST_F(MetaFunc, AsRef) {
    using namespace entt::literals;

    func_t instance{};
    auto func = entt::resolve<func_t>().func("a"_hs);
    func.invoke(instance).cast<int &>() = 3;

    ASSERT_EQ(func.ret(), entt::resolve<int>());
    ASSERT_EQ(instance.value, 3);
}

TEST_F(MetaFunc, AsConstRef) {
    using namespace entt::literals;

    func_t instance{};
    auto func = entt::resolve<func_t>().func("ca"_hs);

    ASSERT_DEATH((func.invoke(instance).cast<int &>() = 3), "");
    ASSERT_EQ(func.ret(), entt::resolve<int>());
    ASSERT_EQ(func.invoke(instance).cast<const int &>(), 3);
    ASSERT_EQ(func.invoke(instance).cast<int>(), 3);
}

TEST_F(MetaFunc, FromBase) {
    using namespace entt::literals;

    auto type = entt::resolve<derived_t>();
    derived_t instance{};

    ASSERT_TRUE(type.func("func"_hs));
    ASSERT_EQ(type.func("func"_hs).parent(), entt::resolve<base_t>());
    ASSERT_EQ(instance.value, 3);

    type.func("func"_hs).invoke(instance, 42);

    ASSERT_EQ(instance.value, 42);
}

TEST_F(MetaFunc, ExternalMemberFunction) {
    using namespace entt::literals;

    auto func = entt::resolve<func_t>().func("emplace"_hs);

    ASSERT_TRUE(func);
    ASSERT_EQ(func.parent(), entt::resolve("func"_hs));
    ASSERT_EQ(func.id(), "emplace"_hs);
    ASSERT_EQ(func.arity(), 2u);
    ASSERT_FALSE(func.is_const());
    ASSERT_TRUE(func.is_static());
    ASSERT_EQ(func.ret(), entt::resolve<void>());
    ASSERT_EQ(func.arg(0u), entt::resolve<entt::registry>());
    ASSERT_EQ(func.arg(1u), entt::resolve<entt::entity>());
    ASSERT_FALSE(func.arg(2u));

    entt::registry registry;
    const auto entity = registry.create();

    ASSERT_FALSE(registry.all_of<func_t>(entity));

    func.invoke({}, entt::make_meta_any<entt::registry &>(registry), entity);

    ASSERT_TRUE(registry.all_of<func_t>(entity));
}

TEST_F(MetaFunc, ReRegistration) {
    using namespace entt::literals;

    auto reset_and_check = [this]() {
        int count = 0;

        for(auto func: entt::resolve<func_t>().func()) {
            count += static_cast<bool>(func);
        }

        SetUp();

        for(auto func: entt::resolve<func_t>().func()) {
            count -= static_cast<bool>(func);
        }

        ASSERT_EQ(count, 0);
    };

    reset_and_check();

    func_t instance{};
    auto type = entt::resolve<func_t>();

    ASSERT_TRUE(type.func("f2"_hs));
    ASSERT_FALSE(type.invoke("f2"_hs, instance, 0));
    ASSERT_TRUE(type.invoke("f2"_hs, instance, 0, 0));

    ASSERT_TRUE(type.func("f1"_hs));
    ASSERT_TRUE(type.invoke("f1"_hs, instance, 0));
    ASSERT_FALSE(type.invoke("f1"_hs, instance, 0, 0));

    entt::meta<func_t>()
        .func<entt::overload<int(int, int)>(&func_t::f)>("f"_hs)
        .func<entt::overload<int(int) const>(&func_t::f)>("f"_hs);

    ASSERT_FALSE(type.func("f1"_hs));
    ASSERT_FALSE(type.func("f2"_hs));
    ASSERT_TRUE(type.func("f"_hs));

    ASSERT_TRUE(type.invoke("f"_hs, instance, 0));
    ASSERT_TRUE(type.invoke("f"_hs, instance, 0, 0));

    reset_and_check();
}

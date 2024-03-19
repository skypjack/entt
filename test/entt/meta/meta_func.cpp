#include <cstddef>
#include <utility>
#include <gtest/gtest.h>
#include <entt/core/hashed_string.hpp>
#include <entt/core/utility.hpp>
#include <entt/entity/registry.hpp>
#include <entt/meta/factory.hpp>
#include <entt/meta/meta.hpp>
#include <entt/meta/policy.hpp>
#include <entt/meta/range.hpp>
#include <entt/meta/resolve.hpp>
#include "../../common/config.h"

struct base {
    base() = default;
    virtual ~base() = default;

    static void destroy(base &) {
        ++counter;
    }

    void setter(int v) {
        value = v;
    }

    [[nodiscard]] int getter() const {
        return value;
    }

    static void static_setter(base &ref, int v) {
        ref.value = v;
    }

    inline static int counter = 0; // NOLINT
    int value{3};
};

void fake_member(base &instance, int value) {
    instance.value = value;
}

[[nodiscard]] int fake_const_member(const base &instance) {
    return instance.value;
}

struct derived: base {
    derived()
        : base{} {}
};

struct function {
    [[nodiscard]] int f(const base &, int a, int b) {
        return f(a, b);
    }

    [[nodiscard]] int f(int a, const int b) {
        value = a;
        return b * b;
    }

    [[nodiscard]] int f(int v) const {
        return v * v;
    }

    void g(int v) {
        value = v * v;
    }

    [[nodiscard]] static int h(int &v) {
        return (v *= value);
    }

    static void k(int v) {
        value = v;
    }

    [[nodiscard]] int v(int v) const {
        return (value = v);
    }

    [[nodiscard]] int &a() const {
        return value;
    }

    [[nodiscard]] operator int() const {
        return value;
    }

    inline static int value = 0; // NOLINT
};

double double_member(const double &value) {
    return value * value;
}

struct MetaFunc: ::testing::Test {
    void SetUp() override {
        using namespace entt::literals;

        entt::meta<double>()
            .type("double"_hs)
            .func<&double_member>("member"_hs);

        entt::meta<base>()
            .type("base"_hs)
            .dtor<base::destroy>()
            .func<&base::setter>("setter"_hs)
            .func<fake_member>("fake_member"_hs)
            .func<fake_const_member>("fake_const_member"_hs);

        entt::meta<derived>()
            .type("derived"_hs)
            .base<base>()
            .func<&base::setter>("setter_from_base"_hs)
            .func<&base::getter>("getter_from_base"_hs)
            .func<&base::static_setter>("static_setter_from_base"_hs)
            .dtor<derived::destroy>();

        entt::meta<function>()
            .type("func"_hs)
            .func<&entt::registry::emplace_or_replace<function>>("emplace"_hs)
            .func<entt::overload<int(const base &, int, int)>(&function::f)>("f3"_hs)
            .func<entt::overload<int(int, int)>(&function::f)>("f2"_hs)
            .prop("true"_hs, false)
            .func<entt::overload<int(int) const>(&function::f)>("f1"_hs)
            .prop("true"_hs, false)
            .func<&function::g>("g"_hs)
            .prop("true"_hs, false)
            .func<function::h>("h"_hs)
            .prop("true"_hs, false)
            .func<function::k>("k"_hs)
            .prop("true"_hs, false)
            .func<&function::v, entt::as_void_t>("v"_hs)
            .func<&function::a, entt::as_ref_t>("a"_hs)
            .func<&function::a, entt::as_cref_t>("ca"_hs)
            .conv<int>();

        base::counter = 0;
    }

    void TearDown() override {
        entt::meta_reset();
    }

    std::size_t reset_and_check() {
        std::size_t count = 0;

        for(auto func: entt::resolve<function>().func()) {
            for(auto curr = func.second; curr; curr = curr.next()) {
                ++count;
            }
        }

        SetUp();

        for(auto func: entt::resolve<function>().func()) {
            for(auto curr = func.second; curr; curr = curr.next()) {
                --count;
            }
        }

        return count;
    };
};

using MetaFuncDeathTest = MetaFunc;

TEST_F(MetaFunc, Functionalities) {
    using namespace entt::literals;

    auto func = entt::resolve<function>().func("f2"_hs);
    function instance{};

    ASSERT_TRUE(func);

    ASSERT_EQ(func, func);
    ASSERT_NE(func, entt::meta_func{});
    ASSERT_FALSE(func != func);
    ASSERT_TRUE(func == func);

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
    ASSERT_EQ(function::value, 3);

    for(auto curr: func.prop()) {
        ASSERT_EQ(curr.first, "true"_hs);
        ASSERT_FALSE(curr.second.value().template cast<bool>());
    }

    ASSERT_FALSE(func.prop(false));
    ASSERT_FALSE(func.prop('c'));

    auto prop = func.prop("true"_hs);

    ASSERT_TRUE(prop);
    ASSERT_FALSE(prop.value().cast<bool>());
}

TEST_F(MetaFunc, Const) {
    using namespace entt::literals;

    auto func = entt::resolve<function>().func("f1"_hs);
    function instance{};

    ASSERT_TRUE(func);
    ASSERT_EQ(func.arity(), 1u);
    ASSERT_TRUE(func.is_const());
    ASSERT_FALSE(func.is_static());
    ASSERT_EQ(func.ret(), entt::resolve<int>());
    ASSERT_EQ(func.arg(0u), entt::resolve<int>());
    ASSERT_FALSE(func.arg(1u));

    auto any = func.invoke(instance, 4);
    auto empty = func.invoke(instance, derived{});

    ASSERT_FALSE(empty);
    ASSERT_TRUE(any);
    ASSERT_EQ(any.type(), entt::resolve<int>());
    ASSERT_EQ(any.cast<int>(), 16);

    for(auto curr: func.prop()) {
        ASSERT_EQ(curr.first, "true"_hs);
        ASSERT_FALSE(curr.second.value().template cast<bool>());
    }

    ASSERT_FALSE(func.prop(false));
    ASSERT_FALSE(func.prop('c'));

    auto prop = func.prop("true"_hs);

    ASSERT_TRUE(prop);
    ASSERT_FALSE(prop.value().cast<bool>());
}

TEST_F(MetaFunc, RetVoid) {
    using namespace entt::literals;

    auto func = entt::resolve<function>().func("g"_hs);
    function instance{};

    ASSERT_TRUE(func);
    ASSERT_EQ(func.arity(), 1u);
    ASSERT_FALSE(func.is_const());
    ASSERT_FALSE(func.is_static());
    ASSERT_EQ(func.ret(), entt::resolve<void>());
    ASSERT_EQ(func.arg(0u), entt::resolve<int>());
    ASSERT_FALSE(func.arg(1u));

    auto any = func.invoke(instance, 4);

    ASSERT_TRUE(any);
    ASSERT_EQ(any.type(), entt::resolve<void>());
    ASSERT_EQ(function::value, 16);

    for(auto curr: func.prop()) {
        ASSERT_EQ(curr.first, "true"_hs);
        ASSERT_FALSE(curr.second.value().template cast<bool>());
    }

    ASSERT_FALSE(func.prop(false));
    ASSERT_FALSE(func.prop('c'));

    auto prop = func.prop("true"_hs);

    ASSERT_TRUE(prop);
    ASSERT_FALSE(prop.value().cast<bool>());
}

TEST_F(MetaFunc, Static) {
    using namespace entt::literals;

    auto func = entt::resolve<function>().func("h"_hs);
    function::value = 2;

    ASSERT_TRUE(func);
    ASSERT_EQ(func.arity(), 1u);
    ASSERT_FALSE(func.is_const());
    ASSERT_TRUE(func.is_static());
    ASSERT_EQ(func.ret(), entt::resolve<int>());
    ASSERT_EQ(func.arg(0u), entt::resolve<int>());
    ASSERT_FALSE(func.arg(1u));

    auto any = func.invoke({}, 3);
    auto empty = func.invoke({}, derived{});

    ASSERT_FALSE(empty);
    ASSERT_TRUE(any);
    ASSERT_EQ(any.type(), entt::resolve<int>());
    ASSERT_EQ(any.cast<int>(), 6);

    for(auto curr: func.prop()) {
        ASSERT_EQ(curr.first, "true"_hs);
        ASSERT_FALSE(curr.second.value().template cast<bool>());
    }

    ASSERT_FALSE(func.prop(false));
    ASSERT_FALSE(func.prop('c'));

    auto prop = func.prop("true"_hs);

    ASSERT_TRUE(prop);
    ASSERT_FALSE(prop.value().cast<bool>());
}

TEST_F(MetaFunc, StaticRetVoid) {
    using namespace entt::literals;

    auto func = entt::resolve<function>().func("k"_hs);

    ASSERT_TRUE(func);
    ASSERT_EQ(func.arity(), 1u);
    ASSERT_FALSE(func.is_const());
    ASSERT_TRUE(func.is_static());
    ASSERT_EQ(func.ret(), entt::resolve<void>());
    ASSERT_EQ(func.arg(0u), entt::resolve<int>());
    ASSERT_FALSE(func.arg(1u));

    auto any = func.invoke({}, 3);

    ASSERT_TRUE(any);
    ASSERT_EQ(any.type(), entt::resolve<void>());
    ASSERT_EQ(function::value, 3);

    for(auto curr: func.prop()) {
        ASSERT_EQ(curr.first, "true"_hs);
        ASSERT_FALSE(curr.second.value().template cast<bool>());
    }

    ASSERT_FALSE(func.prop(false));
    ASSERT_FALSE(func.prop('c'));

    auto prop = func.prop("true"_hs);

    ASSERT_TRUE(prop);
    ASSERT_FALSE(prop.value().cast<bool>());
}

TEST_F(MetaFunc, StaticAsMember) {
    using namespace entt::literals;

    base instance{};
    auto func = entt::resolve<base>().func("fake_member"_hs);
    auto any = func.invoke(instance, 3);

    ASSERT_TRUE(func);
    ASSERT_EQ(func.arity(), 1u);
    ASSERT_FALSE(func.is_const());
    ASSERT_FALSE(func.is_static());
    ASSERT_EQ(func.ret(), entt::resolve<void>());
    ASSERT_EQ(func.arg(0u), entt::resolve<int>());
    ASSERT_FALSE(func.arg(1u));

    ASSERT_EQ(func.prop().cbegin(), func.prop().cend());

    ASSERT_FALSE(func.invoke({}, 3));
    ASSERT_FALSE(func.invoke(std::as_const(instance), 3));

    ASSERT_TRUE(any);
    ASSERT_EQ(any.type(), entt::resolve<void>());
    ASSERT_EQ(instance.value, 3);
}

TEST_F(MetaFunc, StaticAsConstMember) {
    using namespace entt::literals;

    base instance{};
    auto func = entt::resolve<base>().func("fake_const_member"_hs);
    auto any = func.invoke(std::as_const(instance));

    ASSERT_TRUE(func);
    ASSERT_EQ(func.arity(), 0u);
    ASSERT_TRUE(func.is_const());
    ASSERT_FALSE(func.is_static());
    ASSERT_EQ(func.ret(), entt::resolve<int>());
    ASSERT_FALSE(func.arg(0u));

    ASSERT_EQ(func.prop().cbegin(), func.prop().cend());

    ASSERT_FALSE(func.invoke({}));
    ASSERT_TRUE(func.invoke(instance));

    ASSERT_TRUE(any);
    ASSERT_EQ(any.type(), entt::resolve<int>());
    ASSERT_EQ(any.cast<int>(), 3);
}

TEST_F(MetaFunc, NonClassTypeMember) {
    using namespace entt::literals;

    double instance = 3.;
    auto func = entt::resolve<double>().func("member"_hs);
    auto any = func.invoke(instance);

    ASSERT_TRUE(func);
    ASSERT_EQ(func.arity(), 0u);
    ASSERT_TRUE(func.is_const());
    ASSERT_FALSE(func.is_static());
    ASSERT_EQ(func.ret(), entt::resolve<double>());
    ASSERT_FALSE(func.arg(0u));

    ASSERT_EQ(func.prop().cbegin(), func.prop().cend());

    ASSERT_FALSE(func.invoke({}));
    ASSERT_TRUE(func.invoke(instance));

    ASSERT_TRUE(any);
    ASSERT_EQ(any.type(), entt::resolve<double>());
    ASSERT_EQ(any.cast<double>(), instance * instance);
}

TEST_F(MetaFunc, MetaAnyArgs) {
    using namespace entt::literals;

    function instance;
    auto any = entt::resolve<function>().func("f1"_hs).invoke(instance, 3);

    ASSERT_TRUE(any);
    ASSERT_EQ(any.type(), entt::resolve<int>());
    ASSERT_EQ(any.cast<int>(), 9);
}

TEST_F(MetaFunc, InvalidArgs) {
    using namespace entt::literals;

    int value = 3;
    ASSERT_FALSE(entt::resolve<function>().func("f1"_hs).invoke(value, 'c'));
}

TEST_F(MetaFunc, CastAndConvert) {
    using namespace entt::literals;

    function instance;
    instance.value = 3;
    auto any = entt::resolve<function>().func("f3"_hs).invoke(instance, derived{}, 0, instance);

    ASSERT_TRUE(any);
    ASSERT_EQ(any.type(), entt::resolve<int>());
    ASSERT_EQ(any.cast<int>(), 9);
    ASSERT_EQ(instance.value, 0);
}

TEST_F(MetaFunc, ArithmeticConversion) {
    using namespace entt::literals;

    function instance;
    auto any = entt::resolve<function>().func("f2"_hs).invoke(instance, true, 4.2);

    ASSERT_TRUE(any);
    ASSERT_EQ(any.type(), entt::resolve<int>());
    ASSERT_EQ(any.cast<int>(), 16);
    ASSERT_EQ(instance.value, 1);
}

TEST_F(MetaFunc, ArgsByRef) {
    using namespace entt::literals;

    auto func = entt::resolve<function>().func("h"_hs);
    function::value = 2;
    entt::meta_any any{3};
    int value = 4;

    ASSERT_EQ(func.invoke({}, entt::forward_as_meta(value)).cast<int>(), 8);
    ASSERT_EQ(func.invoke({}, any.as_ref()).cast<int>(), 6);
    ASSERT_EQ(any.cast<int>(), 6);
    ASSERT_EQ(value, 8);
}

TEST_F(MetaFunc, ArgsByConstRef) {
    using namespace entt::literals;

    function instance{};
    auto func = entt::resolve<function>().func("g"_hs);
    entt::meta_any any{2};
    int value = 3;

    ASSERT_TRUE(func.invoke(instance, entt::forward_as_meta(std::as_const(value))));
    ASSERT_EQ(function::value, 9);

    ASSERT_TRUE(func.invoke(instance, std::as_const(any).as_ref()));
    ASSERT_EQ(function::value, 4);
}

TEST_F(MetaFunc, ConstInstance) {
    using namespace entt::literals;

    function instance{};
    auto any = entt::resolve<function>().func("f1"_hs).invoke(std::as_const(instance), 2);

    ASSERT_FALSE(entt::resolve<function>().func("g"_hs).invoke(std::as_const(instance), 1));
    ASSERT_TRUE(any);
    ASSERT_EQ(any.cast<int>(), 4);
}

TEST_F(MetaFunc, AsVoid) {
    using namespace entt::literals;

    auto func = entt::resolve<function>().func("v"_hs);
    function instance{};

    ASSERT_EQ(func.invoke(instance, 1), entt::meta_any{std::in_place_type<void>});
    ASSERT_EQ(func.ret(), entt::resolve<void>());
    ASSERT_EQ(instance.value, 1);
}

TEST_F(MetaFunc, AsRef) {
    using namespace entt::literals;

    function instance{};
    auto func = entt::resolve<function>().func("a"_hs);
    func.invoke(instance).cast<int &>() = 3;

    ASSERT_EQ(func.ret(), entt::resolve<int>());
    ASSERT_EQ(instance.value, 3);
}

TEST_F(MetaFunc, AsConstRef) {
    using namespace entt::literals;

    function instance{};
    auto func = entt::resolve<function>().func("ca"_hs);

    ASSERT_EQ(func.ret(), entt::resolve<int>());
    ASSERT_EQ(func.invoke(instance).cast<const int &>(), 3);
    ASSERT_EQ(func.invoke(instance).cast<int>(), 3);
}

ENTT_DEBUG_TEST_F(MetaFuncDeathTest, AsConstRef) {
    using namespace entt::literals;

    function instance{};
    auto func = entt::resolve<function>().func("ca"_hs);

    ASSERT_DEATH((func.invoke(instance).cast<int &>() = 3), "");
}

TEST_F(MetaFunc, InvokeBaseFunction) {
    using namespace entt::literals;

    auto type = entt::resolve<derived>();
    derived instance{};

    ASSERT_TRUE(type.func("setter"_hs));
    ASSERT_EQ(instance.value, 3);

    type.func("setter"_hs).invoke(instance, 1);

    ASSERT_EQ(instance.value, 1);
}

TEST_F(MetaFunc, InvokeFromBase) {
    using namespace entt::literals;

    auto type = entt::resolve<derived>();
    derived instance{};

    auto setter_from_base = type.func("setter_from_base"_hs);

    ASSERT_TRUE(setter_from_base);
    ASSERT_EQ(instance.value, 3);

    setter_from_base.invoke(instance, 1);

    ASSERT_EQ(instance.value, 1);

    auto getter_from_base = type.func("getter_from_base"_hs);

    ASSERT_TRUE(getter_from_base);
    ASSERT_EQ(getter_from_base.invoke(instance).cast<int>(), 1);

    auto static_setter_from_base = type.func("static_setter_from_base"_hs);

    ASSERT_TRUE(static_setter_from_base);
    ASSERT_EQ(instance.value, 1);

    static_setter_from_base.invoke(instance, 3);

    ASSERT_EQ(instance.value, 3);
}

TEST_F(MetaFunc, ExternalMemberFunction) {
    using namespace entt::literals;

    auto func = entt::resolve<function>().func("emplace"_hs);

    ASSERT_TRUE(func);
    ASSERT_EQ(func.arity(), 2u);
    ASSERT_FALSE(func.is_const());
    ASSERT_TRUE(func.is_static());
    ASSERT_EQ(func.ret(), entt::resolve<void>());
    ASSERT_EQ(func.arg(0u), entt::resolve<entt::registry>());
    ASSERT_EQ(func.arg(1u), entt::resolve<entt::entity>());
    ASSERT_FALSE(func.arg(2u));

    entt::registry registry;
    const auto entity = registry.create();

    ASSERT_FALSE(registry.all_of<function>(entity));

    func.invoke({}, entt::forward_as_meta(registry), entity);

    ASSERT_TRUE(registry.all_of<function>(entity));
}

TEST_F(MetaFunc, ReRegistration) {
    using namespace entt::literals;

    ASSERT_EQ(reset_and_check(), 0u);

    function instance{};
    auto type = entt::resolve<function>();

    ASSERT_TRUE(type.func("f2"_hs));
    ASSERT_FALSE(type.invoke("f2"_hs, instance, 0));
    ASSERT_TRUE(type.invoke("f2"_hs, instance, 0, 0));

    ASSERT_TRUE(type.func("f1"_hs));
    ASSERT_TRUE(type.invoke("f1"_hs, instance, 0));
    ASSERT_FALSE(type.invoke("f1"_hs, instance, 0, 0));

    entt::meta<function>()
        .func<entt::overload<int(int, int)>(&function::f)>("f"_hs)
        .func<entt::overload<int(int) const>(&function::f)>("f"_hs);

    ASSERT_TRUE(type.func("f1"_hs));
    ASSERT_TRUE(type.func("f2"_hs));
    ASSERT_TRUE(type.func("f"_hs));

    ASSERT_TRUE(type.invoke("f"_hs, instance, 0));
    ASSERT_TRUE(type.invoke("f"_hs, instance, 0, 0));

    ASSERT_EQ(reset_and_check(), 0u);
}

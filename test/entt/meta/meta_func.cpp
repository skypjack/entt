#include <cstddef>
#include <type_traits>
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
#include "../../common/meta_traits.h"

struct base {
    base() = default;
    virtual ~base() = default;

    void setter(int iv) {
        value = iv;
    }

    [[nodiscard]] int getter() const {
        return value;
    }

    static void static_setter(base &ref, int iv) {
        ref.value = iv;
    }

    int value{3};
};

void fake_member(base &instance, int value) {
    instance.value = value;
}

[[nodiscard]] int fake_const_member(const base &instance) {
    return instance.value;
}

struct derived: base {
    derived() = default;
};

struct function {
    [[nodiscard]] int f(const base &, int val, int other) {
        return f(val, other);
    }

    [[nodiscard]] int f(int val, const int other) {
        value = val;
        return other * other;
    }

    [[nodiscard]] int f(int iv) const {
        return value * iv;
    }

    void g(int iv) {
        value = iv * iv;
    }

    [[nodiscard]] static int h(int &iv, const function &instance) {
        return (iv *= instance.value);
    }

    static void k(int iv, function &instance) {
        instance.value = iv;
    }

    [[nodiscard]] int v(int &iv) const {
        return (iv = value);
    }

    [[nodiscard]] int &a() {
        return value;
    }

    [[nodiscard]] operator int() const {
        return value;
    }

    int value{};
};

double double_member(const double &value) {
    return value * value;
}

struct MetaFunc: ::testing::Test {
    void SetUp() override {
        using namespace entt::literals;

        entt::meta_factory<double>{}
            .type("double"_hs)
            .func<&double_member>("member"_hs);

        entt::meta_factory<base>{}
            .type("base"_hs)
            .func<&base::setter>("setter"_hs)
            .func<fake_member>("fake_member"_hs)
            .func<fake_const_member>("fake_const_member"_hs);

        entt::meta_factory<derived>{}
            .type("derived"_hs)
            .base<base>()
            .func<&base::setter>("setter_from_base"_hs)
            .func<&base::getter>("getter_from_base")
            .func<&base::static_setter>("static_setter_from_base"_hs, "static setter");

        entt::meta_factory<function>{}
            .type("func"_hs)
            .func<&entt::registry::emplace_or_replace<function>, entt::as_ref_t>("emplace"_hs)
            .traits(test::meta_traits::one | test::meta_traits::two | test::meta_traits::three)
            .func<entt::overload<int(const base &, int, int)>(&function::f)>("f3"_hs)
            .traits(test::meta_traits::three)
            .func<entt::overload<int(int, int)>(&function::f)>("f2"_hs)
            .traits(test::meta_traits::two)
            .custom<int>(2)
            .func<entt::overload<int(int) const>(&function::f)>("f1"_hs)
            .traits(test::meta_traits::one)
            .func<&function::g>("g"_hs)
            .custom<char>('c')
            .func<function::h>("h"_hs)
            .func<function::k>("k"_hs)
            .func<&function::v, entt::as_void_t>("v"_hs)
            .func<&function::a, entt::as_ref_t>("a"_hs)
            .func<&function::a, entt::as_cref_t>("ca"_hs)
            .conv<int>();
    }

    void TearDown() override {
        entt::meta_reset();
    }

    std::size_t reset_and_check() {
        std::size_t count = 0;

        for(const auto &func: entt::resolve<function>().func()) {
            for(auto curr = func.second; curr; curr = curr.next()) {
                ++count;
            }
        }

        SetUp();

        for(const auto &func: entt::resolve<function>().func()) {
            for(auto curr = func.second; curr; curr = curr.next()) {
                --count;
            }
        }

        return count;
    };
};

using MetaFuncDeathTest = MetaFunc;

TEST_F(MetaFunc, SafeWhenEmpty) {
    const entt::meta_func func{};
    entt::meta_any *args = nullptr;

    ASSERT_FALSE(func);
    ASSERT_EQ(func, entt::meta_func{});
    ASSERT_EQ(func.arity(), 0u);
    ASSERT_FALSE(func.is_const());
    ASSERT_FALSE(func.is_static());
    ASSERT_EQ(func.ret(), entt::meta_type{});
    ASSERT_EQ(func.arg(0u), entt::meta_type{});
    ASSERT_EQ(func.arg(1u), entt::meta_type{});
    ASSERT_FALSE(func.invoke({}, args, 0u));
    ASSERT_FALSE(func.invoke({}, args, 1u));
    ASSERT_FALSE(func.invoke({}));
    ASSERT_FALSE(func.invoke({}, 'c'));
    ASSERT_EQ(func.traits<test::meta_traits>(), test::meta_traits::none);
    ASSERT_EQ(static_cast<const char *>(func.custom()), nullptr);
    ASSERT_EQ(func.next(), func);
}

TEST_F(MetaFunc, UserTraits) {
    using namespace entt::literals;

    ASSERT_EQ(entt::resolve<function>().func("h"_hs).traits<test::meta_traits>(), test::meta_traits::none);
    ASSERT_EQ(entt::resolve<function>().func("k"_hs).traits<test::meta_traits>(), test::meta_traits::none);

    ASSERT_EQ(entt::resolve<function>().func("emplace"_hs).traits<test::meta_traits>(), test::meta_traits::one | test::meta_traits::two | test::meta_traits::three);
    ASSERT_EQ(entt::resolve<function>().func("f1"_hs).traits<test::meta_traits>(), test::meta_traits::one);
    ASSERT_EQ(entt::resolve<function>().func("f2"_hs).traits<test::meta_traits>(), test::meta_traits::two);
    ASSERT_EQ(entt::resolve<function>().func("f3"_hs).traits<test::meta_traits>(), test::meta_traits::three);
}

ENTT_DEBUG_TEST_F(MetaFuncDeathTest, UserTraits) {
    using namespace entt::literals;

    using traits_type = entt::internal::meta_traits;
    constexpr auto value = traits_type{static_cast<std::underlying_type_t<traits_type>>(traits_type::_user_defined_traits) + 1u};
    ASSERT_DEATH(entt::meta_factory<function>{}.func<&function::g>("g"_hs).traits(value), "");
}

TEST_F(MetaFunc, Custom) {
    using namespace entt::literals;

    ASSERT_EQ(*static_cast<const char *>(entt::resolve<function>().func("g"_hs).custom()), 'c');
    ASSERT_EQ(static_cast<const char &>(entt::resolve<function>().func("g"_hs).custom()), 'c');

    ASSERT_EQ(static_cast<const int *>(entt::resolve<function>().func("g"_hs).custom()), nullptr);
    ASSERT_EQ(static_cast<const int *>(entt::resolve<function>().func("h"_hs).custom()), nullptr);
}

ENTT_DEBUG_TEST_F(MetaFuncDeathTest, Custom) {
    using namespace entt::literals;

    ASSERT_DEATH([[maybe_unused]] const int value = entt::resolve<function>().func("g"_hs).custom(), "");
    ASSERT_DEATH([[maybe_unused]] const char value = entt::resolve<function>().func("h"_hs).custom(), "");
}

TEST_F(MetaFunc, Name) {
    using namespace entt::literals;

    const entt::meta_type type = entt::resolve<derived>();

    ASSERT_EQ(type.func("setter_from_base"_hs).name(), nullptr);
    ASSERT_STREQ(type.func("getter_from_base"_hs).name(), "getter_from_base");
    ASSERT_STREQ(type.func("static_setter_from_base"_hs).name(), "static setter");
    ASSERT_EQ(type.func("none"_hs).name(), nullptr);
}

TEST_F(MetaFunc, Comparison) {
    using namespace entt::literals;

    auto func = entt::resolve<function>().func("f2"_hs);

    ASSERT_TRUE(func);
    ASSERT_EQ(func, func);
    ASSERT_NE(func, entt::meta_func{});
    ASSERT_FALSE(func != func);
    ASSERT_TRUE(func == func);
}

TEST_F(MetaFunc, NonConst) {
    using namespace entt::literals;

    auto func = entt::resolve<function>().func("f2"_hs);
    function instance{};

    ASSERT_TRUE(func);
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
    ASSERT_EQ(instance.value, 3);
}

TEST_F(MetaFunc, Const) {
    using namespace entt::literals;

    auto func = entt::resolve<function>().func("f1"_hs);
    function instance{2};

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
    ASSERT_EQ(any.cast<int>(), 8);
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
    ASSERT_EQ(instance.value, 16);
}

TEST_F(MetaFunc, Static) {
    using namespace entt::literals;

    auto func = entt::resolve<function>().func("h"_hs);
    function instance{2};

    ASSERT_TRUE(func);
    ASSERT_EQ(func.arity(), 2u);
    ASSERT_FALSE(func.is_const());
    ASSERT_TRUE(func.is_static());
    ASSERT_EQ(func.ret(), entt::resolve<int>());
    ASSERT_EQ(func.arg(0u), entt::resolve<int>());
    ASSERT_EQ(func.arg(1u), entt::resolve<function>());
    ASSERT_FALSE(func.arg(2u));

    auto any = func.invoke({}, 3, entt::forward_as_meta(instance));
    auto empty = func.invoke({}, derived{}, entt::forward_as_meta(instance));

    ASSERT_FALSE(empty);
    ASSERT_TRUE(any);
    ASSERT_EQ(any.type(), entt::resolve<int>());
    ASSERT_EQ(any.cast<int>(), 6);
}

TEST_F(MetaFunc, StaticRetVoid) {
    using namespace entt::literals;

    auto func = entt::resolve<function>().func("k"_hs);
    function instance{};

    ASSERT_TRUE(func);
    ASSERT_EQ(func.arity(), 2u);
    ASSERT_FALSE(func.is_const());
    ASSERT_TRUE(func.is_static());
    ASSERT_EQ(func.ret(), entt::resolve<void>());
    ASSERT_EQ(func.arg(0u), entt::resolve<int>());
    ASSERT_EQ(func.arg(1u), entt::resolve<function>());
    ASSERT_FALSE(func.arg(2u));

    auto any = func.invoke({}, 3, entt::forward_as_meta(instance));

    ASSERT_TRUE(any);
    ASSERT_EQ(any.type(), entt::resolve<void>());
    ASSERT_EQ(instance.value, 3);
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

    ASSERT_FALSE(func.invoke({}));
    ASSERT_TRUE(func.invoke(instance));

    ASSERT_TRUE(any);
    ASSERT_EQ(any.type(), entt::resolve<double>());
    ASSERT_EQ(any.cast<double>(), instance * instance);
}

TEST_F(MetaFunc, MetaAnyArgs) {
    using namespace entt::literals;

    function instance{3};
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

    function instance{2};
    entt::meta_any any{3};
    int value = 4;

    ASSERT_EQ(func.invoke({}, entt::forward_as_meta(value), entt::forward_as_meta(instance)).cast<int>(), 8);
    ASSERT_EQ(func.invoke({}, any.as_ref(), entt::forward_as_meta(instance)).cast<int>(), 6);
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
    ASSERT_EQ(instance.value, 9);

    ASSERT_TRUE(func.invoke(instance, std::as_const(any).as_ref()));
    ASSERT_EQ(instance.value, 4);
}

TEST_F(MetaFunc, ConstInstance) {
    using namespace entt::literals;

    function instance{2};
    auto any = entt::resolve<function>().func("f1"_hs).invoke(std::as_const(instance), 2);

    ASSERT_FALSE(entt::resolve<function>().func("g"_hs).invoke(std::as_const(instance), 1));
    ASSERT_TRUE(any);
    ASSERT_EQ(any.cast<int>(), 4);
}

TEST_F(MetaFunc, AsVoid) {
    using namespace entt::literals;

    auto func = entt::resolve<function>().func("v"_hs);
    function instance{3};
    int value{2};

    ASSERT_EQ(func.invoke(instance, entt::forward_as_meta(value)), entt::meta_any{std::in_place_type<void>});
    ASSERT_EQ(func.ret(), entt::resolve<void>());
    ASSERT_EQ(value, instance.value);
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

    function instance{3};
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
    ASSERT_EQ(func.ret(), entt::resolve<function>());
    ASSERT_EQ(func.arg(0u), entt::resolve<entt::registry>());
    ASSERT_EQ(func.arg(1u), entt::resolve<entt::entity>());
    ASSERT_FALSE(func.arg(2u));

    entt::registry registry;
    const auto entity = registry.create();

    ASSERT_FALSE(registry.all_of<function>(entity));

    func.invoke({}, entt::forward_as_meta(registry), entity);

    ASSERT_TRUE(registry.all_of<function>(entity));
}

TEST_F(MetaFunc, Overloaded) {
    using namespace entt::literals;

    auto type = entt::resolve<function>();

    ASSERT_FALSE(type.func("f2"_hs).next());

    entt::meta_factory<function>{}
        // this should not overwrite traits and custom data
        .func<entt::overload<int(int, int)>(&function::f)>("f2"_hs)
        // this should put traits and custom data on the new overload instead
        .func<entt::overload<int(int) const>(&function::f)>("f2"_hs)
        .traits(test::meta_traits::three)
        .custom<int>(3);

    ASSERT_TRUE(type.func("f2"_hs).next());
    ASSERT_FALSE(type.func("f2"_hs).next().next());

    ASSERT_EQ(type.func("f2"_hs).traits<test::meta_traits>(), test::meta_traits::two);
    ASSERT_EQ(type.func("f2"_hs).next().traits<test::meta_traits>(), test::meta_traits::three);

    ASSERT_NE(static_cast<const int *>(type.func("f2"_hs).custom()), nullptr);
    ASSERT_NE(static_cast<const int *>(type.func("f2"_hs).next().custom()), nullptr);

    ASSERT_EQ(static_cast<const int &>(type.func("f2"_hs).custom()), 2);
    ASSERT_EQ(static_cast<const int &>(type.func("f2"_hs).next().custom()), 3);
}

TEST_F(MetaFunc, OverloadedOrder) {
    using namespace entt::literals;

    entt::meta_factory<function>{}
        .func<entt::overload<int(int, int)>(&function::f)>("f2"_hs)
        .func<entt::overload<int(int) const>(&function::f)>("f2"_hs);

    auto type = entt::resolve<function>();
    auto func = type.func("f2"_hs);

    ASSERT_TRUE(func);
    ASSERT_EQ(func.arity(), 2u);
    ASSERT_FALSE(func.is_const());
    ASSERT_EQ(func.ret(), entt::resolve<int>());

    func = func.next();

    ASSERT_TRUE(func);
    ASSERT_EQ(func.arity(), 1u);
    ASSERT_TRUE(func.is_const());
    ASSERT_EQ(func.ret(), entt::resolve<int>());

    func = func.next();

    ASSERT_FALSE(func);
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

    entt::meta_factory<function>{}
        .func<entt::overload<int(int, int)>(&function::f)>("f"_hs)
        .func<entt::overload<int(int) const>(&function::f)>("f"_hs);

    ASSERT_TRUE(type.func("f1"_hs));
    ASSERT_TRUE(type.func("f2"_hs));
    ASSERT_TRUE(type.func("f"_hs));

    ASSERT_TRUE(type.invoke("f"_hs, instance, 0));
    ASSERT_TRUE(type.invoke("f"_hs, instance, 0, 0));

    entt::meta_factory<function>{}
        .func<entt::overload<int(int, int)>(&function::f)>("f"_hs)
        .traits(test::meta_traits::one)
        .custom<int>(3)
        // this should not overwrite traits and custom data
        .func<entt::overload<int(int, int)>(&function::f)>("f"_hs);

    ASSERT_EQ(type.func("f"_hs).traits<test::meta_traits>(), test::meta_traits::one);
    ASSERT_NE(static_cast<const int *>(type.func("f"_hs).custom()), nullptr);

    ASSERT_EQ(reset_and_check(), 0u);
}

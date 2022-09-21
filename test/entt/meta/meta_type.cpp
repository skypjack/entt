#include <algorithm>
#include <map>
#include <memory>
#include <type_traits>
#include <utility>
#include <vector>
#include <gtest/gtest.h>
#include <entt/core/hashed_string.hpp>
#include <entt/core/type_info.hpp>
#include <entt/core/utility.hpp>
#include <entt/locator/locator.hpp>
#include <entt/meta/container.hpp>
#include <entt/meta/context.hpp>
#include <entt/meta/factory.hpp>
#include <entt/meta/meta.hpp>
#include <entt/meta/pointer.hpp>
#include <entt/meta/resolve.hpp>
#include <entt/meta/template.hpp>
#include "../common/config.h"

template<typename Type>
void set(Type &prop, Type value) {
    prop = value;
}

template<typename Type>
Type get(Type &prop) {
    return prop;
}

struct base_t {
    base_t()
        : value{'c'} {};

    char value;
};

struct derived_t: base_t {
    derived_t()
        : base_t{} {}
};

struct abstract_t {
    virtual ~abstract_t() = default;

    virtual void func(int) {}
    void base_only(int) {}
};

struct concrete_t: base_t, abstract_t {
    void func(int v) override {
        abstract_t::func(v);
        value = v;
    }

    int value{3};
};

struct clazz_t {
    clazz_t() = default;

    clazz_t(const base_t &, int v)
        : value{v} {}

    void member() {}
    static void func() {}

    operator int() const {
        return value;
    }

    int value{};
};

struct overloaded_func_t {
    int e(int v) const {
        return v + v;
    }

    int f(const base_t &, int a, int b) {
        return f(a, b);
    }

    int f(int a, int b) {
        value = a;
        return g(b);
    }

    int f(int v) {
        return 2 * std::as_const(*this).f(v);
    }

    int f(int v) const {
        return g(v);
    }

    float f(int a, float b) {
        value = a;
        return static_cast<float>(e(static_cast<int>(b)));
    }

    int g(int v) const {
        return v * v;
    }

    inline static int value = 0;
};

enum class property_t : entt::id_type {
    random,
    value,
    key_only,
    list
};

struct MetaType: ::testing::Test {
    void SetUp() override {
        using namespace entt::literals;

        entt::meta<double>()
            .type("double"_hs)
            .data<set<double>, get<double>>("var"_hs);

        entt::meta<unsigned int>()
            .type("unsigned int"_hs)
            .data<0u>("min"_hs)
            .data<100u>("max"_hs);

        entt::meta<base_t>()
            .type("base"_hs)
            .data<&base_t::value>("value"_hs);

        entt::meta<derived_t>()
            .type("derived"_hs)
            .base<base_t>();

        entt::meta<abstract_t>()
            .type("abstract"_hs)
            .func<&abstract_t::func>("func"_hs)
            .func<&abstract_t::base_only>("base_only"_hs);

        entt::meta<concrete_t>()
            .type("concrete"_hs)
            .base<base_t>()
            .base<abstract_t>();

        entt::meta<overloaded_func_t>()
            .type("overloaded_func"_hs)
            .func<&overloaded_func_t::e>("e"_hs)
            .func<entt::overload<int(const base_t &, int, int)>(&overloaded_func_t::f)>("f"_hs)
            .func<entt::overload<int(int, int)>(&overloaded_func_t::f)>("f"_hs)
            .func<entt::overload<int(int)>(&overloaded_func_t::f)>("f"_hs)
            .func<entt::overload<int(int) const>(&overloaded_func_t::f)>("f"_hs)
            .func<entt::overload<float(int, float)>(&overloaded_func_t::f)>("f"_hs)
            .func<&overloaded_func_t::g>("g"_hs);

        entt::meta<property_t>()
            .type("property"_hs)
            .data<property_t::random>("random"_hs)
            .prop(static_cast<entt::id_type>(property_t::random), 0)
            .prop(static_cast<entt::id_type>(property_t::value), 3)
            .data<property_t::value>("value"_hs)
            .prop(static_cast<entt::id_type>(property_t::random), true)
            .prop(static_cast<entt::id_type>(property_t::value), 0)
            .prop(static_cast<entt::id_type>(property_t::key_only))
            .prop(static_cast<entt::id_type>(property_t::list))
            .data<property_t::key_only>("key_only"_hs)
            .prop(static_cast<entt::id_type>(property_t::key_only))
            .data<property_t::list>("list"_hs)
            .prop(static_cast<entt::id_type>(property_t::random), false)
            .prop(static_cast<entt::id_type>(property_t::value), 0)
            .prop(static_cast<entt::id_type>(property_t::key_only))
            .data<set<property_t>, get<property_t>>("var"_hs);

        entt::meta<clazz_t>()
            .type("clazz"_hs)
            .prop(static_cast<entt::id_type>(property_t::value), 42)
            .ctor<const base_t &, int>()
            .data<&clazz_t::value>("value"_hs)
            .func<&clazz_t::member>("member"_hs)
            .func<clazz_t::func>("func"_hs)
            .conv<int>();
    }

    void TearDown() override {
        entt::meta_reset();
    }
};

using MetaTypeDeathTest = MetaType;

TEST_F(MetaType, Resolve) {
    using namespace entt::literals;

    ASSERT_EQ(entt::resolve<double>(), entt::resolve("double"_hs));
    ASSERT_EQ(entt::resolve<double>(), entt::resolve(entt::type_id<double>()));
    ASSERT_FALSE(entt::resolve(entt::type_id<void>()));

    auto range = entt::resolve();
    // it could be "char"_hs rather than entt::hashed_string::value("char") if it weren't for a bug in VS2017
    const auto it = std::find_if(range.begin(), range.end(), [](auto curr) { return curr.second.id() == entt::hashed_string::value("clazz"); });

    ASSERT_NE(it, range.end());
    ASSERT_EQ(it->second, entt::resolve<clazz_t>());

    bool found = false;

    for(auto curr: entt::resolve()) {
        found = found || curr.second == entt::resolve<double>();
    }

    ASSERT_TRUE(found);
}

TEST_F(MetaType, Functionalities) {
    using namespace entt::literals;

    auto type = entt::resolve<clazz_t>();

    ASSERT_TRUE(type);
    ASSERT_NE(type, entt::meta_type{});
    ASSERT_EQ(type.id(), "clazz"_hs);
    ASSERT_EQ(type.info(), entt::type_id<clazz_t>());

    for(auto curr: type.prop()) {
        ASSERT_EQ(curr.first, static_cast<entt::id_type>(property_t::value));
        ASSERT_EQ(curr.second.value(), 42);
    }

    ASSERT_FALSE(type.prop(static_cast<entt::id_type>(property_t::key_only)));
    ASSERT_FALSE(type.prop("property"_hs));

    auto prop = type.prop(static_cast<entt::id_type>(property_t::value));

    ASSERT_TRUE(prop);
    ASSERT_EQ(prop.value(), 42);
}

TEST_F(MetaType, SizeOf) {
    ASSERT_EQ(entt::resolve<void>().size_of(), 0u);
    ASSERT_EQ(entt::resolve<int>().size_of(), sizeof(int));
    ASSERT_EQ(entt::resolve<int[]>().size_of(), 0u);
    ASSERT_EQ(entt::resolve<int[3]>().size_of(), sizeof(int[3]));
}

TEST_F(MetaType, Traits) {
    ASSERT_TRUE(entt::resolve<bool>().is_arithmetic());
    ASSERT_TRUE(entt::resolve<double>().is_arithmetic());
    ASSERT_FALSE(entt::resolve<clazz_t>().is_arithmetic());

    ASSERT_TRUE(entt::resolve<int>().is_integral());
    ASSERT_FALSE(entt::resolve<double>().is_integral());
    ASSERT_FALSE(entt::resolve<clazz_t>().is_integral());

    ASSERT_TRUE(entt::resolve<long>().is_signed());
    ASSERT_FALSE(entt::resolve<unsigned int>().is_signed());
    ASSERT_FALSE(entt::resolve<clazz_t>().is_signed());

    ASSERT_TRUE(entt::resolve<int[5]>().is_array());
    ASSERT_TRUE(entt::resolve<int[5][3]>().is_array());
    ASSERT_FALSE(entt::resolve<int>().is_array());

    ASSERT_TRUE(entt::resolve<property_t>().is_enum());
    ASSERT_FALSE(entt::resolve<char>().is_enum());

    ASSERT_TRUE(entt::resolve<derived_t>().is_class());
    ASSERT_FALSE(entt::resolve<double>().is_class());

    ASSERT_TRUE(entt::resolve<int *>().is_pointer());
    ASSERT_FALSE(entt::resolve<int>().is_pointer());

    ASSERT_TRUE(entt::resolve<int *>().is_pointer_like());
    ASSERT_TRUE(entt::resolve<std::shared_ptr<int>>().is_pointer_like());
    ASSERT_FALSE(entt::resolve<int>().is_pointer_like());

    ASSERT_FALSE((entt::resolve<int>().is_sequence_container()));
    ASSERT_TRUE(entt::resolve<std::vector<int>>().is_sequence_container());
    ASSERT_FALSE((entt::resolve<std::map<int, char>>().is_sequence_container()));

    ASSERT_FALSE((entt::resolve<int>().is_associative_container()));
    ASSERT_TRUE((entt::resolve<std::map<int, char>>().is_associative_container()));
    ASSERT_FALSE(entt::resolve<std::vector<int>>().is_associative_container());
}

TEST_F(MetaType, RemovePointer) {
    ASSERT_EQ(entt::resolve<void *>().remove_pointer(), entt::resolve<void>());
    ASSERT_EQ(entt::resolve<char **>().remove_pointer(), entt::resolve<char *>());
    ASSERT_EQ(entt::resolve<int (*)(char, double)>().remove_pointer(), entt::resolve<int(char, double)>());
    ASSERT_EQ(entt::resolve<derived_t>().remove_pointer(), entt::resolve<derived_t>());
}

TEST_F(MetaType, TemplateInfo) {
    ASSERT_FALSE(entt::resolve<int>().is_template_specialization());
    ASSERT_EQ(entt::resolve<int>().template_arity(), 0u);
    ASSERT_EQ(entt::resolve<int>().template_type(), entt::meta_type{});
    ASSERT_EQ(entt::resolve<int>().template_arg(0u), entt::meta_type{});

    ASSERT_TRUE(entt::resolve<std::shared_ptr<int>>().is_template_specialization());
    ASSERT_EQ(entt::resolve<std::shared_ptr<int>>().template_arity(), 1u);
    ASSERT_EQ(entt::resolve<std::shared_ptr<int>>().template_type(), entt::resolve<entt::meta_class_template_tag<std::shared_ptr>>());
    ASSERT_EQ(entt::resolve<std::shared_ptr<int>>().template_arg(0u), entt::resolve<int>());
    ASSERT_EQ(entt::resolve<std::shared_ptr<int>>().template_arg(1u), entt::meta_type{});
}

TEST_F(MetaType, Base) {
    using namespace entt::literals;
    auto type = entt::resolve<derived_t>();

    ASSERT_NE(type.base().cbegin(), type.base().cend());

    for(auto curr: type.base()) {
        ASSERT_EQ(curr.first, entt::type_id<base_t>().hash());
        ASSERT_EQ(curr.second, entt::resolve<base_t>());
    }
}

TEST_F(MetaType, Ctor) {
    derived_t derived;
    base_t &base = derived;
    auto type = entt::resolve<clazz_t>();

    ASSERT_TRUE((type.construct(entt::forward_as_meta(derived), 42)));
    ASSERT_TRUE((type.construct(entt::forward_as_meta(base), 42)));

    // use the implicitly generated default constructor
    auto any = type.construct();

    ASSERT_TRUE(any);
    ASSERT_EQ(any.type(), entt::resolve<clazz_t>());
}

TEST_F(MetaType, Data) {
    using namespace entt::literals;

    auto type = entt::resolve<clazz_t>();
    int counter{};

    for([[maybe_unused]] auto curr: type.data()) {
        ++counter;
    }

    ASSERT_EQ(counter, 1);
    ASSERT_TRUE(type.data("value"_hs));

    type = entt::resolve<void>();

    ASSERT_TRUE(type);
    ASSERT_EQ(type.data().cbegin(), type.data().cend());
}

TEST_F(MetaType, Func) {
    using namespace entt::literals;

    auto type = entt::resolve<clazz_t>();
    clazz_t instance{};
    int counter{};

    for([[maybe_unused]] auto curr: type.func()) {
        ++counter;
    }

    ASSERT_EQ(counter, 2);
    ASSERT_TRUE(type.func("member"_hs));
    ASSERT_TRUE(type.func("func"_hs));
    ASSERT_TRUE(type.func("member"_hs).invoke(instance));
    ASSERT_TRUE(type.func("func"_hs).invoke({}));

    type = entt::resolve<void>();

    ASSERT_TRUE(type);
    ASSERT_EQ(type.func().cbegin(), type.func().cend());
}

TEST_F(MetaType, Invoke) {
    using namespace entt::literals;

    auto type = entt::resolve<clazz_t>();
    clazz_t instance{};

    ASSERT_TRUE(type.invoke("member"_hs, instance));
    ASSERT_FALSE(type.invoke("rebmem"_hs, {}));
}

TEST_F(MetaType, InvokeFromBase) {
    using namespace entt::literals;

    auto type = entt::resolve<concrete_t>();
    concrete_t instance{};

    ASSERT_TRUE(type.invoke("base_only"_hs, instance, 42));
    ASSERT_FALSE(type.invoke("ylno_esab"_hs, {}, 'c'));
}

TEST_F(MetaType, OverloadedFunc) {
    using namespace entt::literals;

    const auto type = entt::resolve<overloaded_func_t>();
    overloaded_func_t instance{};
    entt::meta_any res{};

    ASSERT_TRUE(type.func("f"_hs));
    ASSERT_TRUE(type.func("e"_hs));
    ASSERT_TRUE(type.func("g"_hs));

    res = type.invoke("f"_hs, instance, base_t{}, 1, 2);

    ASSERT_TRUE(res);
    ASSERT_EQ(overloaded_func_t::value, 1);
    ASSERT_NE(res.try_cast<int>(), nullptr);
    ASSERT_EQ(res.cast<int>(), 4);

    res = type.invoke("f"_hs, instance, 3, 4);

    ASSERT_TRUE(res);
    ASSERT_EQ(overloaded_func_t::value, 3);
    ASSERT_NE(res.try_cast<int>(), nullptr);
    ASSERT_EQ(res.cast<int>(), 16);

    res = type.invoke("f"_hs, instance, 5);

    ASSERT_TRUE(res);
    ASSERT_EQ(overloaded_func_t::value, 3);
    ASSERT_NE(res.try_cast<int>(), nullptr);
    ASSERT_EQ(res.cast<int>(), 50);

    res = type.invoke("f"_hs, std::as_const(instance), 5);

    ASSERT_TRUE(res);
    ASSERT_EQ(overloaded_func_t::value, 3);
    ASSERT_NE(res.try_cast<int>(), nullptr);
    ASSERT_EQ(res.cast<int>(), 25);

    res = type.invoke("f"_hs, instance, 6, 7.f);

    ASSERT_TRUE(res);
    ASSERT_EQ(overloaded_func_t::value, 6);
    ASSERT_NE(res.try_cast<float>(), nullptr);
    ASSERT_EQ(res.cast<float>(), 14.f);

    res = type.invoke("f"_hs, instance, 8, 9.f);

    ASSERT_TRUE(res);
    ASSERT_EQ(overloaded_func_t::value, 8);
    ASSERT_NE(res.try_cast<float>(), nullptr);
    ASSERT_EQ(res.cast<float>(), 18.f);

    // it fails as an ambiguous call
    ASSERT_FALSE(type.invoke("f"_hs, instance, 8, 9.));
}

TEST_F(MetaType, Construct) {
    auto any = entt::resolve<clazz_t>().construct(base_t{}, 42);

    ASSERT_TRUE(any);
    ASSERT_EQ(any.cast<clazz_t>().value, 42);
}

TEST_F(MetaType, ConstructNoArgs) {
    // this should work, no other tests required
    auto any = entt::resolve<clazz_t>().construct();

    ASSERT_TRUE(any);
}

TEST_F(MetaType, ConstructMetaAnyArgs) {
    auto any = entt::resolve<clazz_t>().construct(entt::meta_any{base_t{}}, entt::meta_any{42});

    ASSERT_TRUE(any);
    ASSERT_EQ(any.cast<clazz_t>().value, 42);
}

TEST_F(MetaType, ConstructInvalidArgs) {
    ASSERT_FALSE(entt::resolve<clazz_t>().construct('c', base_t{}));
}

TEST_F(MetaType, LessArgs) {
    ASSERT_FALSE(entt::resolve<clazz_t>().construct(base_t{}));
}

TEST_F(MetaType, ConstructCastAndConvert) {
    auto any = entt::resolve<clazz_t>().construct(derived_t{}, clazz_t{derived_t{}, 42});

    ASSERT_TRUE(any);
    ASSERT_EQ(any.cast<clazz_t>().value, 42);
}

TEST_F(MetaType, ConstructArithmeticConversion) {
    auto any = entt::resolve<clazz_t>().construct(derived_t{}, clazz_t{derived_t{}, true});

    ASSERT_TRUE(any);
    ASSERT_EQ(any.cast<clazz_t>().value, 1);
}

TEST_F(MetaType, FromVoid) {
    using namespace entt::literals;

    ASSERT_FALSE(entt::resolve<double>().from_void(static_cast<double *>(nullptr)));
    ASSERT_FALSE(entt::resolve<double>().from_void(static_cast<const double *>(nullptr)));

    auto type = entt::resolve<double>();
    double value = 4.2;

    ASSERT_FALSE(entt::resolve<void>().from_void(static_cast<void *>(&value)));
    ASSERT_FALSE(entt::resolve<void>().from_void(static_cast<const void *>(&value)));

    auto as_void = type.from_void(static_cast<void *>(&value));
    auto as_const_void = type.from_void(static_cast<const void *>(&value));

    ASSERT_TRUE(as_void);
    ASSERT_TRUE(as_const_void);

    ASSERT_EQ(as_void.type(), entt::resolve<double>());
    ASSERT_NE(as_void.try_cast<double>(), nullptr);

    ASSERT_EQ(as_const_void.type(), entt::resolve<double>());
    ASSERT_EQ(as_const_void.try_cast<double>(), nullptr);
    ASSERT_NE(as_const_void.try_cast<const double>(), nullptr);

    value = 1.2;

    ASSERT_EQ(as_void.cast<double>(), as_const_void.cast<double>());
    ASSERT_EQ(as_void.cast<double>(), 1.2);
}

TEST_F(MetaType, Reset) {
    using namespace entt::literals;

    ASSERT_TRUE(entt::resolve("clazz"_hs));
    ASSERT_EQ(entt::resolve<clazz_t>().id(), "clazz"_hs);
    ASSERT_TRUE(entt::resolve<clazz_t>().prop(static_cast<entt::id_type>(property_t::value)));
    ASSERT_TRUE(entt::resolve<clazz_t>().data("value"_hs));
    ASSERT_TRUE((entt::resolve<clazz_t>().construct(derived_t{}, clazz_t{})));
    // implicitly generated default constructor
    ASSERT_TRUE(entt::resolve<clazz_t>().construct());

    entt::meta_reset("clazz"_hs);

    ASSERT_FALSE(entt::resolve("clazz"_hs));
    ASSERT_NE(entt::resolve<clazz_t>().id(), "clazz"_hs);
    ASSERT_FALSE(entt::resolve<clazz_t>().prop(static_cast<entt::id_type>(property_t::value)));
    ASSERT_FALSE(entt::resolve<clazz_t>().data("value"_hs));
    ASSERT_FALSE((entt::resolve<clazz_t>().construct(derived_t{}, clazz_t{})));
    // implicitly generated default constructor is not cleared
    ASSERT_TRUE(entt::resolve<clazz_t>().construct());

    entt::meta<clazz_t>().type("clazz"_hs);

    ASSERT_TRUE(entt::resolve("clazz"_hs));
}

TEST_F(MetaType, ResetLast) {
    auto id = (entt::resolve().cend() - 1u)->second.id();

    ASSERT_TRUE(entt::resolve(id));

    entt::meta_reset(id);

    ASSERT_FALSE(entt::resolve(id));
}

TEST_F(MetaType, ResetAll) {
    using namespace entt::literals;

    ASSERT_NE(entt::resolve().begin(), entt::resolve().end());

    ASSERT_TRUE(entt::resolve("clazz"_hs));
    ASSERT_TRUE(entt::resolve("overloaded_func"_hs));
    ASSERT_TRUE(entt::resolve("double"_hs));

    entt::meta_reset();

    ASSERT_FALSE(entt::resolve("clazz"_hs));
    ASSERT_FALSE(entt::resolve("overloaded_func"_hs));
    ASSERT_FALSE(entt::resolve("double"_hs));

    ASSERT_EQ(entt::resolve().begin(), entt::resolve().end());
}

TEST_F(MetaType, AbstractClass) {
    using namespace entt::literals;

    auto type = entt::resolve<abstract_t>();
    concrete_t instance;

    ASSERT_EQ(type.info(), entt::type_id<abstract_t>());
    ASSERT_EQ(instance.base_t::value, 'c');
    ASSERT_EQ(instance.value, 3);

    type.func("func"_hs).invoke(instance, 42);

    ASSERT_EQ(instance.base_t::value, 'c');
    ASSERT_EQ(instance.value, 42);
}

TEST_F(MetaType, EnumAndNamedConstants) {
    using namespace entt::literals;

    auto type = entt::resolve<property_t>();

    ASSERT_TRUE(type.data("random"_hs));
    ASSERT_TRUE(type.data("value"_hs));

    ASSERT_EQ(type.data("random"_hs).type(), type);
    ASSERT_EQ(type.data("value"_hs).type(), type);

    ASSERT_FALSE(type.data("random"_hs).set({}, property_t::value));
    ASSERT_FALSE(type.data("value"_hs).set({}, property_t::random));

    ASSERT_EQ(type.data("random"_hs).get({}).cast<property_t>(), property_t::random);
    ASSERT_EQ(type.data("value"_hs).get({}).cast<property_t>(), property_t::value);
}

TEST_F(MetaType, ArithmeticTypeAndNamedConstants) {
    using namespace entt::literals;

    auto type = entt::resolve<unsigned int>();

    ASSERT_TRUE(type.data("min"_hs));
    ASSERT_TRUE(type.data("max"_hs));

    ASSERT_EQ(type.data("min"_hs).type(), type);
    ASSERT_EQ(type.data("max"_hs).type(), type);

    ASSERT_FALSE(type.data("min"_hs).set({}, 100u));
    ASSERT_FALSE(type.data("max"_hs).set({}, 0u));

    ASSERT_EQ(type.data("min"_hs).get({}).cast<unsigned int>(), 0u);
    ASSERT_EQ(type.data("max"_hs).get({}).cast<unsigned int>(), 100u);
}

TEST_F(MetaType, Variables) {
    using namespace entt::literals;

    auto p_data = entt::resolve<property_t>().data("var"_hs);
    auto d_data = entt::resolve("double"_hs).data("var"_hs);

    property_t prop{property_t::key_only};
    double d = 3.;

    p_data.set(prop, property_t::random);
    d_data.set(d, 42.);

    ASSERT_EQ(p_data.get(prop).cast<property_t>(), property_t::random);
    ASSERT_EQ(d_data.get(d).cast<double>(), 42.);
    ASSERT_EQ(prop, property_t::random);
    ASSERT_EQ(d, 42.);
}

TEST_F(MetaType, PropertiesAndCornerCases) {
    using namespace entt::literals;

    auto type = entt::resolve<property_t>();

    ASSERT_EQ(type.prop().cbegin(), type.prop().cend());

    ASSERT_EQ(type.data("random"_hs).prop(static_cast<entt::id_type>(property_t::random)).value().cast<int>(), 0);
    ASSERT_EQ(type.data("random"_hs).prop(static_cast<entt::id_type>(property_t::value)).value().cast<int>(), 3);

    ASSERT_EQ(type.data("value"_hs).prop(static_cast<entt::id_type>(property_t::random)).value().cast<bool>(), true);
    ASSERT_EQ(type.data("value"_hs).prop(static_cast<entt::id_type>(property_t::value)).value().cast<int>(), 0);
    ASSERT_TRUE(type.data("value"_hs).prop(static_cast<entt::id_type>(property_t::key_only)));
    ASSERT_FALSE(type.data("value"_hs).prop(static_cast<entt::id_type>(property_t::key_only)).value());

    ASSERT_TRUE(type.data("key_only"_hs).prop(static_cast<entt::id_type>(property_t::key_only)));
    ASSERT_FALSE(type.data("key_only"_hs).prop(static_cast<entt::id_type>(property_t::key_only)).value());

    ASSERT_EQ(type.data("list"_hs).prop(static_cast<entt::id_type>(property_t::random)).value().cast<bool>(), false);
    ASSERT_EQ(type.data("list"_hs).prop(static_cast<entt::id_type>(property_t::value)).value().cast<int>(), 0);
    ASSERT_TRUE(type.data("list"_hs).prop(static_cast<entt::id_type>(property_t::key_only)));
    ASSERT_FALSE(type.data("list"_hs).prop(static_cast<entt::id_type>(property_t::key_only)).value());

    type = entt::resolve<void>();

    ASSERT_EQ(type.prop().cbegin(), type.prop().cend());
}

TEST_F(MetaType, ResetAndReRegistrationAfterReset) {
    using namespace entt::literals;

    ASSERT_FALSE(entt::internal::meta_context::from(entt::locator<entt::meta_ctx>::value_or()).value.empty());

    entt::meta_reset<double>();
    entt::meta_reset<unsigned int>();
    entt::meta_reset<base_t>();
    entt::meta_reset<derived_t>();
    entt::meta_reset<abstract_t>();
    entt::meta_reset<concrete_t>();
    entt::meta_reset<overloaded_func_t>();
    entt::meta_reset<property_t>();
    entt::meta_reset<clazz_t>();

    ASSERT_FALSE(entt::resolve("double"_hs));
    ASSERT_FALSE(entt::resolve("base"_hs));
    ASSERT_FALSE(entt::resolve("derived"_hs));
    ASSERT_FALSE(entt::resolve("clazz"_hs));

    ASSERT_TRUE(entt::internal::meta_context::from(entt::locator<entt::meta_ctx>::value_or()).value.empty());

    ASSERT_FALSE(entt::resolve<clazz_t>().prop(static_cast<entt::id_type>(property_t::value)));
    // implicitly generated default constructor is not cleared
    ASSERT_TRUE(entt::resolve<clazz_t>().construct());
    ASSERT_FALSE(entt::resolve<clazz_t>().data("value"_hs));
    ASSERT_FALSE(entt::resolve<clazz_t>().func("member"_hs));

    entt::meta<double>().type("double"_hs);
    entt::meta_any any{42.};

    ASSERT_TRUE(any);
    ASSERT_TRUE(any.allow_cast<int>());
    ASSERT_TRUE(any.allow_cast<float>());

    ASSERT_FALSE(entt::resolve("derived"_hs));
    ASSERT_TRUE(entt::resolve("double"_hs));

    entt::meta<property_t>()
        .type("property"_hs)
        .data<property_t::random>("rand"_hs)
        .prop(static_cast<entt::id_type>(property_t::value), 42)
        .prop(static_cast<entt::id_type>(property_t::random), 3);

    ASSERT_TRUE(entt::resolve<property_t>().data("rand"_hs).prop(static_cast<entt::id_type>(property_t::value)));
    ASSERT_TRUE(entt::resolve<property_t>().data("rand"_hs).prop(static_cast<entt::id_type>(property_t::random)));
}

TEST_F(MetaType, ReRegistration) {
    using namespace entt::literals;

    int count = 0;

    for([[maybe_unused]] auto type: entt::resolve()) {
        ++count;
    }

    SetUp();

    for([[maybe_unused]] auto type: entt::resolve()) {
        --count;
    }

    ASSERT_EQ(count, 0);
    ASSERT_TRUE(entt::resolve("double"_hs));

    entt::meta<double>().type("real"_hs);

    ASSERT_FALSE(entt::resolve("double"_hs));
    ASSERT_TRUE(entt::resolve("real"_hs));
    ASSERT_TRUE(entt::resolve("real"_hs).data("var"_hs));
}

TEST_F(MetaType, NameCollision) {
    using namespace entt::literals;

    ASSERT_NO_FATAL_FAILURE(entt::meta<clazz_t>().type("clazz"_hs));
    ASSERT_TRUE(entt::resolve("clazz"_hs));

    ASSERT_NO_FATAL_FAILURE(entt::meta<clazz_t>().type("quux"_hs));
    ASSERT_FALSE(entt::resolve("clazz"_hs));
    ASSERT_TRUE(entt::resolve("quux"_hs));
}

ENTT_DEBUG_TEST_F(MetaTypeDeathTest, NameCollision) {
    using namespace entt::literals;

    ASSERT_DEATH(entt::meta<clazz_t>().type("abstract"_hs), "");
}

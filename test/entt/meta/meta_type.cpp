#include <map>
#include <memory>
#include <vector>
#include <gtest/gtest.h>
#include <entt/core/hashed_string.hpp>
#include <entt/meta/container.hpp>
#include <entt/meta/factory.hpp>
#include <entt/meta/meta.hpp>
#include <entt/meta/pointer.hpp>
#include <entt/meta/resolve.hpp>
#include <entt/meta/template.hpp>

template<typename Type>
void set(Type &prop, Type value) {
    prop = value;
}

template<typename Type>
Type get(Type &prop) {
    return prop;
}

struct base_t { base_t(): value{'c'} {}; char value; };
struct derived_t: base_t { derived_t(): base_t{} {} };

struct abstract_t {
    virtual ~abstract_t() = default;
    virtual void func(int) {}
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
        : value{v}
    {}

    void member() {}
    static void func() {}

    int value;
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

enum class property_t {
    random,
    value,
    key_only,
    list
};

union union_t {
    int i;
    double d;
};

struct MetaType: ::testing::Test {
    void SetUp() override {
        using namespace entt::literals;

        entt::meta<double>()
            .type("double"_hs)
            .conv<int>()
            .data<&set<double>, &get<double>>("var"_hs);

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
            .func<&abstract_t::func>("func"_hs);

        entt::meta<concrete_t>()
            .type("concrete"_hs)
            .base<base_t>()
            .base<abstract_t>();

        entt::meta<overloaded_func_t>()
            .type("overloaded_func"_hs)
            .func<&overloaded_func_t::e> ("e"_hs)
            .func<entt::overload<int(const base_t &, int, int)>(&overloaded_func_t::f)>("f"_hs)
            .func<entt::overload<int(int, int)>(&overloaded_func_t::f)>("f"_hs)
            .func<entt::overload<int(int) const>(&overloaded_func_t::f)>("f"_hs)
            .func<entt::overload<float(int, float)> (&overloaded_func_t::f)> ("f"_hs)
            .func<&overloaded_func_t::g> ("g"_hs);

        entt::meta<property_t>()
            .type("property"_hs)
            .data<property_t::random>("random"_hs)
                .prop(property_t::random, 0)
                .prop(property_t::value, 3)
            .data<property_t::value>("value"_hs)
                .prop(std::make_tuple(std::make_pair(property_t::random, true), std::make_pair(property_t::value, 0), property_t::key_only))
                .prop(property_t::list)
            .data<property_t::key_only>("key_only"_hs)
                .prop([]() { return property_t::key_only; })
            .data<property_t::list>("list"_hs)
               .props(std::make_pair(property_t::random, false), std::make_pair(property_t::value, 0), property_t::key_only)
            .data<&set<property_t>, &get<property_t>>("var"_hs);

        entt::meta<clazz_t>()
            .type("clazz"_hs)
                .prop(property_t::value, 42)
            .ctor<const base_t &, int>()
            .data<&clazz_t::value>("value"_hs)
            .func<&clazz_t::member>("member"_hs)
            .func<&clazz_t::func>("func"_hs);
    }

    void TearDown() override {
        for(auto type: entt::resolve()) {
            type.reset();
        }
    }
};

TEST_F(MetaType, Resolve) {
    using namespace entt::literals;

    ASSERT_EQ(entt::resolve(entt::type_info{}), entt::meta_type{});
    ASSERT_EQ(entt::resolve<double>(), entt::resolve("double"_hs));
    ASSERT_EQ(entt::resolve<double>(), entt::resolve(entt::type_id<double>()));

    auto range = entt::resolve();
    // it could be "char"_hs rather than entt::hashed_string::value("char") if it weren't for a bug in VS2017
    const auto it = std::find_if(range.begin(), range.end(), [](auto type) { return type.id() == entt::hashed_string::value("clazz"); });

    ASSERT_NE(it, range.end());
    ASSERT_EQ(*it, entt::resolve<clazz_t>());

    bool found = false;

    for(auto curr: entt::resolve()) {
        found = found || curr == entt::resolve<double>();
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
        ASSERT_EQ(curr.key(), property_t::value);
        ASSERT_EQ(curr.value(), 42);
    }

    ASSERT_FALSE(type.prop(property_t::key_only));
    ASSERT_FALSE(type.prop("property"_hs));

    auto prop = type.prop(property_t::value);

    ASSERT_TRUE(prop);
    ASSERT_EQ(prop.key(), property_t::value);
    ASSERT_EQ(prop.value(), 42);
}

TEST_F(MetaType, SizeOf) {
    ASSERT_EQ(entt::resolve<void>().size_of(), 0u);
    ASSERT_EQ(entt::resolve<int>().size_of(), sizeof(int));
    ASSERT_EQ(entt::resolve<int[]>().size_of(), 0u);
    ASSERT_EQ(entt::resolve<int[3]>().size_of(), sizeof(int[3]));
}

TEST_F(MetaType, Traits) {
    ASSERT_TRUE(entt::resolve<void>().is_void());
    ASSERT_FALSE(entt::resolve<int>().is_void());

    ASSERT_TRUE(entt::resolve<bool>().is_integral());
    ASSERT_FALSE(entt::resolve<double>().is_integral());

    ASSERT_TRUE(entt::resolve<double>().is_floating_point());
    ASSERT_FALSE(entt::resolve<int>().is_floating_point());

    ASSERT_TRUE(entt::resolve<int[5]>().is_array());
    ASSERT_TRUE(entt::resolve<int[5][3]>().is_array());
    ASSERT_FALSE(entt::resolve<int>().is_array());

    ASSERT_TRUE(entt::resolve<property_t>().is_enum());
    ASSERT_FALSE(entt::resolve<char>().is_enum());

    ASSERT_TRUE(entt::resolve<union_t>().is_union());
    ASSERT_FALSE(entt::resolve<derived_t>().is_union());

    ASSERT_TRUE(entt::resolve<derived_t>().is_class());
    ASSERT_FALSE(entt::resolve<union_t>().is_class());

    ASSERT_TRUE(entt::resolve<int *>().is_pointer());
    ASSERT_FALSE(entt::resolve<int>().is_pointer());

    ASSERT_TRUE(entt::resolve<decltype(&clazz_t::func)>().is_function_pointer());
    ASSERT_FALSE(entt::resolve<decltype(&clazz_t::member)>().is_function_pointer());

    ASSERT_TRUE(entt::resolve<decltype(&clazz_t::value)>().is_member_object_pointer());
    ASSERT_FALSE(entt::resolve<decltype(&clazz_t::member)>().is_member_object_pointer());

    ASSERT_TRUE(entt::resolve<decltype(&clazz_t::member)>().is_member_function_pointer());
    ASSERT_FALSE(entt::resolve<decltype(&clazz_t::value)>().is_member_function_pointer());

    ASSERT_TRUE(entt::resolve<int *>().is_pointer_like());
    ASSERT_TRUE(entt::resolve<std::shared_ptr<int>>().is_pointer_like());
    ASSERT_FALSE(entt::resolve<int>().is_pointer_like());

    ASSERT_TRUE(entt::resolve<std::vector<int>>().is_sequence_container());
    ASSERT_FALSE((entt::resolve<std::map<int, char>>().is_sequence_container()));

    ASSERT_TRUE((entt::resolve<std::map<int, char>>().is_associative_container()));
    ASSERT_FALSE(entt::resolve<std::vector<int>>().is_associative_container());

    ASSERT_EQ(entt::resolve<int>().rank(), 0u);
    ASSERT_EQ(entt::resolve<int[5][3]>().rank(), 2u);
    ASSERT_EQ(entt::resolve<int>().extent(), 0u);
    ASSERT_EQ(entt::resolve<int[5][3]>().extent(), 5u);
    ASSERT_EQ(entt::resolve<int[5][3]>().extent(1u), 3u);
    ASSERT_EQ(entt::resolve<int[5][3]>().extent(2u), 0u);
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

TEST_F(MetaType, RemovePointer) {
    ASSERT_EQ(entt::resolve<void *>().remove_pointer(), entt::resolve<void>());
    ASSERT_EQ(entt::resolve<int(*)(char, double)>().remove_pointer(), entt::resolve<int(char, double)>());
    ASSERT_EQ(entt::resolve<derived_t>().remove_pointer(), entt::resolve<derived_t>());
}

TEST_F(MetaType, RemoveExtent) {
    ASSERT_EQ(entt::resolve<int[3]>().remove_extent(), entt::resolve<int>());
    ASSERT_EQ(entt::resolve<int[3][3]>().remove_extent(), entt::resolve<int[3]>());
    ASSERT_EQ(entt::resolve<derived_t>().remove_extent(), entt::resolve<derived_t>());
}

TEST_F(MetaType, Base) {
    using namespace entt::literals;

    auto type = entt::resolve<derived_t>();
    bool iterate = false;

    for(auto curr: type.base()) {
        ASSERT_EQ(curr, entt::resolve<base_t>());
        iterate = true;
    }

    ASSERT_TRUE(iterate);
    ASSERT_EQ(type.base("base"_hs), entt::resolve<base_t>());
    ASSERT_FALSE(type.base("esabe"_hs));
}

TEST_F(MetaType, Ctor) {
    auto type = entt::resolve<clazz_t>();
    int counter{};

    for([[maybe_unused]] auto curr: type.ctor()) {
        ++counter;
    }

    // we only register a constructor, the default one is implicitly generated for us
    ASSERT_EQ(counter, 2);
    ASSERT_TRUE((type.ctor<>()));
    ASSERT_TRUE((type.ctor<const base_t &, int>()));
    ASSERT_TRUE((type.ctor<const derived_t &, double>()));

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
}

TEST_F(MetaType, Invoke) {
    using namespace entt::literals;

    auto type = entt::resolve<clazz_t>();
    clazz_t instance{};

    ASSERT_TRUE(type.invoke("member"_hs, instance));
    ASSERT_FALSE(type.invoke("rebmem"_hs, {}));
}

TEST_F(MetaType, OverloadedFunc) {
    using namespace entt::literals;

    entt::meta<float>().conv<int>();
    entt::meta<double>().conv<float>();

    const auto type = entt::resolve<overloaded_func_t>();
    overloaded_func_t instance{};

    ASSERT_TRUE(type.func("f"_hs));
    ASSERT_TRUE(type.func("e"_hs));
    ASSERT_TRUE(type.func("g"_hs));

    const auto first = type.invoke("f"_hs, instance, base_t{}, 1, 2);

    ASSERT_TRUE(first);
    ASSERT_EQ(overloaded_func_t::value, 1);
    ASSERT_NE(first.try_cast<int>(), nullptr);
    ASSERT_EQ(first.cast<int>(), 4);

    const auto second = type.invoke("f"_hs, instance, 3, 4);

    ASSERT_TRUE(second);
    ASSERT_EQ(overloaded_func_t::value, 3);
    ASSERT_NE(second.try_cast<int>(), nullptr);
    ASSERT_EQ(second.cast<int>(), 16);

    const auto third = type.invoke("f"_hs, instance, 5);

    ASSERT_TRUE(third);
    ASSERT_EQ(overloaded_func_t::value, 3);
    ASSERT_NE(third.try_cast<int>(), nullptr);
    ASSERT_EQ(third.cast<int>(), 25);

    const auto fourth = type.invoke("f"_hs, instance, 6, 7.f);

    ASSERT_TRUE(fourth);
    ASSERT_EQ(overloaded_func_t::value, 6);
    ASSERT_NE(fourth.try_cast<float>(), nullptr);
    ASSERT_EQ(fourth.cast<float>(), 14.f);

    const auto cast = type.invoke("f"_hs, instance, 8, 9.f);

    ASSERT_TRUE(cast);
    ASSERT_EQ(overloaded_func_t::value, 8);
    ASSERT_NE(cast.try_cast<float>(), nullptr);
    ASSERT_EQ(cast.cast<float>(), 18.f);

    const auto ambiguous = type.invoke("f"_hs, instance, 8, 9.);

    ASSERT_FALSE(ambiguous);
}

TEST_F(MetaType, SetGet) {
    using namespace entt::literals;

    auto type = entt::resolve<clazz_t>();
    clazz_t instance{};

    ASSERT_TRUE(type.set("value"_hs, instance, 42));
    ASSERT_FALSE(type.set("eulav"_hs, instance, 3));
    ASSERT_EQ(instance.value, 42);

    ASSERT_FALSE(type.get("eulav"_hs, instance));
    ASSERT_TRUE(type.get("value"_hs, instance));
    ASSERT_EQ(type.get("value"_hs, instance).cast<int>(), 42);
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
    ASSERT_FALSE(entt::resolve<clazz_t>().construct(base_t{}, 'c'));
}

TEST_F(MetaType, LessArgs) {
    ASSERT_FALSE(entt::resolve<clazz_t>().construct(base_t{}));
}

TEST_F(MetaType, ConstructCastAndConvert) {
    auto any = entt::resolve<clazz_t>().construct(derived_t{}, 42.);

    ASSERT_TRUE(any);
    ASSERT_EQ(any.cast<clazz_t>().value, 42);
}

TEST_F(MetaType, Reset) {
    using namespace entt::literals;

    ASSERT_TRUE(entt::resolve("clazz"_hs));
    ASSERT_EQ(entt::resolve<clazz_t>().id(), "clazz"_hs);
    ASSERT_TRUE(entt::resolve<clazz_t>().prop(property_t::value));
    ASSERT_TRUE(entt::resolve<clazz_t>().data("value"_hs));
    ASSERT_TRUE((entt::resolve<clazz_t>().ctor<const base_t &, int>()));
    // implicitly generated default constructor
    ASSERT_TRUE(entt::resolve<clazz_t>().ctor<>());

    entt::resolve("clazz"_hs).reset();

    ASSERT_FALSE(entt::resolve("clazz"_hs));
    ASSERT_NE(entt::resolve<clazz_t>().id(), "clazz"_hs);
    ASSERT_FALSE(entt::resolve<clazz_t>().prop(property_t::value));
    ASSERT_FALSE(entt::resolve<clazz_t>().data("value"_hs));
    ASSERT_FALSE((entt::resolve<clazz_t>().ctor<const base_t &, int>()));
    // the implicitly generated default constructor is there after a reset
    ASSERT_TRUE(entt::resolve<clazz_t>().ctor<>());

    entt::meta<clazz_t>().type("clazz"_hs);

    ASSERT_TRUE(entt::resolve("clazz"_hs));
    // the implicitly generated default constructor must be there in any case
    ASSERT_TRUE(entt::resolve<clazz_t>().ctor<>());
}

TEST_F(MetaType, ResetAll) {
    using namespace entt::literals;

    ASSERT_NE(entt::resolve().begin(), entt::resolve().end());

    ASSERT_TRUE(entt::resolve("clazz"_hs));
    ASSERT_TRUE(entt::resolve("overloaded_func"_hs));
    ASSERT_TRUE(entt::resolve("double"_hs));

    for(auto type: entt::resolve()) {
        // we exploit the fact that the iterators aren't invalidated
        // because EnTT leaves a dangling ::next in the underlying node
        type.reset();
    }

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

    ASSERT_EQ(type.data("random"_hs).prop(property_t::random).value().cast<int>(), 0);
    ASSERT_EQ(type.data("random"_hs).prop(property_t::value).value().cast<int>(), 3);

    ASSERT_EQ(type.data("value"_hs).prop(property_t::random).value().cast<bool>(), true);
    ASSERT_EQ(type.data("value"_hs).prop(property_t::value).value().cast<int>(), 0);
    ASSERT_TRUE(type.data("value"_hs).prop(property_t::key_only));
    ASSERT_FALSE(type.data("value"_hs).prop(property_t::key_only).value());

    ASSERT_TRUE(type.data("key_only"_hs).prop(property_t::key_only));
    ASSERT_FALSE(type.data("key_only"_hs).prop(property_t::key_only).value());

    ASSERT_EQ(type.data("list"_hs).prop(property_t::random).value().cast<bool>(), false);
    ASSERT_EQ(type.data("list"_hs).prop(property_t::value).value().cast<int>(), 0);
    ASSERT_TRUE(type.data("list"_hs).prop(property_t::key_only));
    ASSERT_FALSE(type.data("list"_hs).prop(property_t::key_only).value());
}

TEST_F(MetaType, ResetAndReRegistrationAfterReset) {
    using namespace entt::literals;

    ASSERT_NE(*entt::internal::meta_context::global(), nullptr);

    entt::resolve<double>().reset();
    entt::resolve<unsigned int>().reset();
    entt::resolve<base_t>().reset();
    entt::resolve<derived_t>().reset();
    entt::resolve<abstract_t>().reset();
    entt::resolve<concrete_t>().reset();
    entt::resolve<overloaded_func_t> ().reset ();
    entt::resolve<property_t>().reset();
    entt::resolve<clazz_t>().reset();

    ASSERT_FALSE(entt::resolve("double"_hs));
    ASSERT_FALSE(entt::resolve("base"_hs));
    ASSERT_FALSE(entt::resolve("derived"_hs));
    ASSERT_FALSE(entt::resolve("clazz"_hs));

    ASSERT_EQ(*entt::internal::meta_context::global(), nullptr);

    ASSERT_FALSE(entt::resolve<clazz_t>().prop(property_t::value));
    // the implicitly generated default constructor is there after a reset
    ASSERT_TRUE(entt::resolve<clazz_t>().ctor<>());
    ASSERT_FALSE(entt::resolve<clazz_t>().data("value"_hs));
    ASSERT_FALSE(entt::resolve<clazz_t>().func("member"_hs));

    entt::meta<double>().type("double"_hs).conv<float>();
    entt::meta_any any{42.};

    ASSERT_TRUE(any);
    ASSERT_FALSE(any.allow_cast<int>());
    ASSERT_TRUE(any.allow_cast<float>());

    ASSERT_FALSE(entt::resolve("derived"_hs));
    ASSERT_TRUE(entt::resolve("double"_hs));

    entt::meta<property_t>()
        .type("property"_hs)
        .data<property_t::random>("rand"_hs)
            .prop(property_t::value, 42)
            .prop(property_t::random, 3);

    ASSERT_TRUE(entt::resolve<property_t>().data("rand"_hs).prop(property_t::value));
    ASSERT_TRUE(entt::resolve<property_t>().data("rand"_hs).prop(property_t::random));
}

TEST_F(MetaType, ReRegistration) {
    using namespace entt::literals;

    int count = 0;

    for(auto type: entt::resolve()) {
        count += static_cast<bool>(type);
    }

    SetUp();

    for(auto type: entt::resolve()) {
        count -= static_cast<bool>(type);
    }

    ASSERT_EQ(count, 0);
    ASSERT_TRUE(entt::resolve("double"_hs));

    entt::meta<double>().type("real"_hs);

    ASSERT_FALSE(entt::resolve("double"_hs));
    ASSERT_TRUE(entt::resolve("real"_hs));
    ASSERT_TRUE(entt::resolve("real"_hs).data("var"_hs));
}

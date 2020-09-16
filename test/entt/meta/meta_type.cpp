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

template<typename Type>
void set(Type &prop, Type value) {
    prop = value;
}

template<typename Type>
Type get(Type &prop) {
    return prop;
}

struct base_t { char value{'c'}; };
struct derived_t: base_t {};

struct abstract_t {
    virtual ~abstract_t() = default;
    virtual void func(int) = 0;
};

struct concrete_t: base_t, abstract_t {
    void func(int v) override {
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
    static void SetUpTestCase() {
        entt::meta<double>().type("double"_hs).conv<int>().data<&set<double>, &get<double>>("var"_hs);
        entt::meta<unsigned int>().data<0u>("min"_hs).data<100u>("max"_hs);
        entt::meta<base_t>().type("base"_hs).data<&base_t::value>("value"_hs);
        entt::meta<derived_t>().type("derived"_hs).base<base_t>();
        entt::meta<abstract_t>().func<&abstract_t::func>("func"_hs);
        entt::meta<concrete_t>().base<base_t>().base<abstract_t>();

        entt::meta<property_t>()
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
                .ctor().ctor<const base_t &, int>()
                .data<&clazz_t::value>("value"_hs)
                .func<&clazz_t::member>("member"_hs)
                .func<&clazz_t::func>("func"_hs);
    }
};

TEST_F(MetaType, Resolve) {
    ASSERT_EQ(entt::resolve<double>(), entt::resolve_id("double"_hs));
    ASSERT_EQ(entt::resolve<double>(), entt::resolve_type(entt::type_info<double>::id()));

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
    auto type = entt::resolve<clazz_t>();

    ASSERT_TRUE(type);
    ASSERT_NE(type, entt::meta_type{});
    ASSERT_EQ(type.id(), "clazz"_hs);
    ASSERT_EQ(type.type_id(), entt::type_info<clazz_t>::id());

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
    auto type = entt::resolve<derived_t>();
    bool iterate = false;

    for(auto curr: type.base()) {
        ASSERT_EQ(curr.type(), entt::resolve<base_t>());
        iterate = true;
    }

    ASSERT_TRUE(iterate);
    ASSERT_EQ(type.base("base"_hs).type(), entt::resolve<base_t>());
}

TEST_F(MetaType, Conv) {
    auto type = entt::resolve<double>();
    bool iterate = false;

    for(auto curr: type.conv()) {
        ASSERT_EQ(curr.type(), entt::resolve<int>());
        iterate = true;
    }

    ASSERT_TRUE(iterate);

    auto conv = type.conv<int>();

    ASSERT_EQ(conv.type(), entt::resolve<int>());
    ASSERT_FALSE(type.conv<char>());
}

TEST_F(MetaType, Ctor) {
    auto type = entt::resolve<clazz_t>();
    int counter{};

    for([[maybe_unused]] auto curr: type.ctor()) {
        ++counter;
    }

    ASSERT_EQ(counter, 2);
    ASSERT_TRUE((type.ctor<>()));
    ASSERT_TRUE((type.ctor<const base_t &, int>()));
    ASSERT_TRUE((type.ctor<const derived_t &, double>()));
}

TEST_F(MetaType, Data) {
    auto type = entt::resolve<clazz_t>();
    int counter{};

    for([[maybe_unused]] auto curr: type.data()) {
        ++counter;
    }

    ASSERT_EQ(counter, 1);
    ASSERT_TRUE(type.data("value"_hs));
}

TEST_F(MetaType, Func) {
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
    auto type = entt::resolve<clazz_t>();
    clazz_t instance{};

    ASSERT_TRUE(type.invoke("member"_hs, instance));
    ASSERT_FALSE(type.invoke("rebmem"_hs, {}));
}

TEST_F(MetaType, SetGet) {
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
    ASSERT_TRUE(entt::resolve_id("clazz"_hs));

    entt::resolve_id("clazz"_hs).reset();

    ASSERT_FALSE(entt::resolve_id("clazz"_hs));
    ASSERT_NE(entt::resolve<clazz_t>().id(), "clazz"_hs);
    ASSERT_FALSE(entt::resolve<clazz_t>().prop(property_t::value));
    ASSERT_FALSE(entt::resolve<clazz_t>().data("value"_hs));

    entt::meta<clazz_t>().type("clazz"_hs);

    ASSERT_TRUE(entt::resolve_id("clazz"_hs));
}

TEST_F(MetaType, AbstractClass) {
    auto type = entt::resolve<abstract_t>();
    concrete_t instance;

    ASSERT_EQ(type.type_id(), entt::type_info<abstract_t>::id());
    ASSERT_EQ(instance.base_t::value, 'c');
    ASSERT_EQ(instance.value, 3);

    type.func("func"_hs).invoke(instance, 42);

    ASSERT_EQ(instance.base_t::value, 'c');
    ASSERT_EQ(instance.value, 42);
}

TEST_F(MetaType, EnumAndNamedConstants) {
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
    auto p_data = entt::resolve<property_t>().data("var"_hs);
    auto d_data = entt::resolve_id("double"_hs).data("var"_hs);

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
    ASSERT_NE(*entt::internal::meta_context::global(), nullptr);

    entt::resolve<double>().reset();
    entt::resolve<unsigned int>().reset();
    entt::resolve<base_t>().reset();
    entt::resolve<derived_t>().reset();
    entt::resolve<abstract_t>().reset();
    entt::resolve<concrete_t>().reset();
    entt::resolve<property_t>().reset();
    entt::resolve<clazz_t>().reset();

    ASSERT_FALSE(entt::resolve_id("double"_hs));
    ASSERT_FALSE(entt::resolve_id("base"_hs));
    ASSERT_FALSE(entt::resolve_id("derived"_hs));
    ASSERT_FALSE(entt::resolve_id("clazz"_hs));

    ASSERT_EQ(*entt::internal::meta_context::global(), nullptr);

    ASSERT_FALSE(entt::resolve<clazz_t>().prop(property_t::value));
    ASSERT_FALSE(entt::resolve<clazz_t>().ctor<>());
    ASSERT_FALSE(entt::resolve<clazz_t>().data("value"_hs));
    ASSERT_FALSE(entt::resolve<clazz_t>().func("member"_hs));

    entt::meta<double>().type("double"_hs).conv<float>();
    entt::meta_any any{42.};

    ASSERT_TRUE(any);
    ASSERT_FALSE(any.convert<int>());
    ASSERT_TRUE(any.convert<float>());

    ASSERT_FALSE(entt::resolve_id("derived"_hs));
    ASSERT_TRUE(entt::resolve_id("double"_hs));

    entt::meta<property_t>().data<property_t::random>("rand"_hs).prop(property_t::value, 42).prop(property_t::random, 3);

    ASSERT_TRUE(entt::resolve<property_t>().data("rand"_hs).prop(property_t::value));
    ASSERT_TRUE(entt::resolve<property_t>().data("rand"_hs).prop(property_t::random));
}

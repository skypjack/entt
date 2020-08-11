#include <gtest/gtest.h>
#include <entt/core/hashed_string.hpp>
#include <entt/meta/factory.hpp>
#include <entt/meta/meta.hpp>
#include <entt/meta/resolve.hpp>

struct base_t {
    virtual ~base_t() = default;
    static void destroy(base_t &) {
        ++counter;
    }

    inline static int counter = 0;
    int value{3};
};

struct derived_t: base_t {};

struct clazz_t {
    int i{0};
    const int j{1};
    base_t base{};
    inline static int h{2};
    inline static const int k{3};
};

struct setter_getter_t {
    int setter(int val) {
        return value = val;
    }

    int getter() {
        return value;
    }

    int setter_with_ref(const int &val) {
        return value = val;
    }

    const int & getter_with_ref() {
        return value;
    }

    static int static_setter(setter_getter_t &type, int value) {
        return type.value = value;
    }

    static int static_getter(const setter_getter_t &type) {
        return type.value;
    }

    int value{};
};

struct array_t {
    static inline int global[3];
    int local[5];
};

enum class property_t {
    random,
    value
};

struct MetaData: ::testing::Test {
    static void SetUpTestCase() {
        entt::meta<double>().conv<int>();
        entt::meta<base_t>().dtor<&base_t::destroy>().data<&base_t::value>("value"_hs);
        entt::meta<derived_t>().base<base_t>().dtor<&derived_t::destroy>();

        entt::meta<clazz_t>().type("clazz"_hs)
                .data<&clazz_t::i, entt::as_ref_t>("i"_hs).prop(3, 0)
                .data<&clazz_t::j>("j"_hs).prop(true, 1)
                .data<&clazz_t::h>("h"_hs).prop(property_t::random, 2)
                .data<&clazz_t::k>("k"_hs).prop(property_t::value, 3)
                .data<&clazz_t::base>("base"_hs)
                .data<&clazz_t::i, entt::as_void_t>("void"_hs);

        entt::meta<setter_getter_t>()
                .type("setter_getter"_hs)
                .data<&setter_getter_t::static_setter, &setter_getter_t::static_getter>("x"_hs)
                .data<&setter_getter_t::setter, &setter_getter_t::getter>("y"_hs)
                .data<&setter_getter_t::static_setter, &setter_getter_t::getter>("z"_hs)
                .data<&setter_getter_t::setter_with_ref, &setter_getter_t::getter_with_ref>("w"_hs)
                .data<nullptr, &setter_getter_t::getter>("z_ro"_hs)
                .data<nullptr, &setter_getter_t::value>("value"_hs);

        entt::meta<array_t>()
                .type("array"_hs)
                .data<&array_t::global>("global"_hs)
                .data<&array_t::local>("local"_hs);
    }

    void SetUp() override {
        base_t::counter = 0;
    }
};

TEST_F(MetaData, Functionalities) {
    auto data = entt::resolve<clazz_t>().data("i"_hs);
    clazz_t instance{};

    ASSERT_TRUE(data);
    ASSERT_EQ(data.parent(), entt::resolve_id("clazz"_hs));
    ASSERT_EQ(data.type(), entt::resolve<int>());
    ASSERT_EQ(data.id(), "i"_hs);
    ASSERT_FALSE(data.is_const());
    ASSERT_FALSE(data.is_static());
    ASSERT_EQ(data.get(instance).cast<int>(), 0);
    ASSERT_TRUE(data.set(instance, 42));
    ASSERT_EQ(data.get(instance).cast<int>(), 42);

    for(auto curr: data.prop()) {
        ASSERT_EQ(curr.key(), 3);
        ASSERT_EQ(curr.value(), 0);
    }

    ASSERT_FALSE(data.prop(2));
    ASSERT_FALSE(data.prop('c'));

    auto prop = data.prop(3);

    ASSERT_TRUE(prop);
    ASSERT_EQ(prop.key(), 3);
    ASSERT_EQ(prop.value(), 0);
}

TEST_F(MetaData, Const) {
    auto data = entt::resolve<clazz_t>().data("j"_hs);
    clazz_t instance{};

    ASSERT_TRUE(data);
    ASSERT_EQ(data.parent(), entt::resolve_id("clazz"_hs));
    ASSERT_EQ(data.type(), entt::resolve<int>());
    ASSERT_EQ(data.id(), "j"_hs);
    ASSERT_TRUE(data.is_const());
    ASSERT_FALSE(data.is_static());
    ASSERT_EQ(data.get(instance).cast<int>(), 1);
    ASSERT_FALSE(data.set(instance, 42));
    ASSERT_EQ(data.get(instance).cast<int>(), 1);

    for(auto curr: data.prop()) {
        ASSERT_EQ(curr.key(), true);
        ASSERT_EQ(curr.value(), 1);
    }

    ASSERT_FALSE(data.prop(false));
    ASSERT_FALSE(data.prop('c'));

    auto prop = data.prop(true);

    ASSERT_TRUE(prop);
    ASSERT_EQ(prop.key(), true);
    ASSERT_EQ(prop.value(), 1);
}

TEST_F(MetaData, Static) {
    auto data = entt::resolve<clazz_t>().data("h"_hs);

    ASSERT_TRUE(data);
    ASSERT_EQ(data.parent(), entt::resolve_id("clazz"_hs));
    ASSERT_EQ(data.type(), entt::resolve<int>());
    ASSERT_EQ(data.id(), "h"_hs);
    ASSERT_FALSE(data.is_const());
    ASSERT_TRUE(data.is_static());
    ASSERT_EQ(data.get({}).cast<int>(), 2);
    ASSERT_TRUE(data.set({}, 42));
    ASSERT_EQ(data.get({}).cast<int>(), 42);

    for(auto curr: data.prop()) {
        ASSERT_EQ(curr.key(), property_t::random);
        ASSERT_EQ(curr.value(), 2);
    }

    ASSERT_FALSE(data.prop(property_t::value));
    ASSERT_FALSE(data.prop('c'));

    auto prop = data.prop(property_t::random);

    ASSERT_TRUE(prop);
    ASSERT_EQ(prop.key(), property_t::random);
    ASSERT_EQ(prop.value(), 2);
}

TEST_F(MetaData, ConstStatic) {
    auto data = entt::resolve<clazz_t>().data("k"_hs);

    ASSERT_TRUE(data);
    ASSERT_EQ(data.parent(), entt::resolve_id("clazz"_hs));
    ASSERT_EQ(data.type(), entt::resolve<int>());
    ASSERT_EQ(data.id(), "k"_hs);
    ASSERT_TRUE(data.is_const());
    ASSERT_TRUE(data.is_static());
    ASSERT_EQ(data.get({}).cast<int>(), 3);
    ASSERT_FALSE(data.set({}, 42));
    ASSERT_EQ(data.get({}).cast<int>(), 3);

    for(auto curr: data.prop()) {
        ASSERT_EQ(curr.key(), property_t::value);
        ASSERT_EQ(curr.value(), 3);
    }

    ASSERT_FALSE(data.prop(property_t::random));
    ASSERT_FALSE(data.prop('c'));

    auto prop = data.prop(property_t::value);

    ASSERT_TRUE(prop);
    ASSERT_EQ(prop.key(), property_t::value);
    ASSERT_EQ(prop.value(), 3);
}

TEST_F(MetaData, GetMetaAnyArg) {
    entt::meta_any any{clazz_t{}};
    any.cast<clazz_t>().i = 99;
    const auto value = entt::resolve<clazz_t>().data("i"_hs).get(any);

    ASSERT_TRUE(value);
    ASSERT_TRUE(value.cast<int>());
    ASSERT_EQ(value.cast<int>(), 99);
}

TEST_F(MetaData, GetInvalidArg) {
    auto instance = 0;
    ASSERT_FALSE(entt::resolve<clazz_t>().data("i"_hs).get(instance));
}

TEST_F(MetaData, SetMetaAnyArg) {
    entt::meta_any any{clazz_t{}};
    entt::meta_any value{42};

    ASSERT_EQ(any.cast<clazz_t>().i, 0);
    ASSERT_TRUE(entt::resolve<clazz_t>().data("i"_hs).set(any, value));
    ASSERT_EQ(any.cast<clazz_t>().i, 42);
}

TEST_F(MetaData, SetInvalidArg) {
    ASSERT_FALSE(entt::resolve<clazz_t>().data("i"_hs).set({}, 'c'));
}

TEST_F(MetaData, SetCast) {
    clazz_t instance{};

    ASSERT_EQ(base_t::counter, 0);
    ASSERT_TRUE(entt::resolve<clazz_t>().data("base"_hs).set(instance, derived_t{}));
    ASSERT_EQ(base_t::counter, 1);
}

TEST_F(MetaData, SetConvert) {
    clazz_t instance{};

    ASSERT_EQ(instance.i, 0);
    ASSERT_TRUE(entt::resolve<clazz_t>().data("i"_hs).set(instance, 3.));
    ASSERT_EQ(instance.i, 3);
}

TEST_F(MetaData, SetterGetterAsFreeFunctions) {
    auto data = entt::resolve<setter_getter_t>().data("x"_hs);
    setter_getter_t instance{};

    ASSERT_TRUE(data);
    ASSERT_EQ(data.parent(), entt::resolve_id("setter_getter"_hs));
    ASSERT_EQ(data.type(), entt::resolve<int>());
    ASSERT_EQ(data.id(), "x"_hs);
    ASSERT_FALSE(data.is_const());
    ASSERT_FALSE(data.is_static());
    ASSERT_EQ(data.get(instance).cast<int>(), 0);
    ASSERT_TRUE(data.set(instance, 42));
    ASSERT_EQ(data.get(instance).cast<int>(), 42);
}

TEST_F(MetaData, SetterGetterAsMemberFunctions) {
    auto data = entt::resolve<setter_getter_t>().data("y"_hs);
    setter_getter_t instance{};

    ASSERT_TRUE(data);
    ASSERT_EQ(data.parent(), entt::resolve_id("setter_getter"_hs));
    ASSERT_EQ(data.type(), entt::resolve<int>());
    ASSERT_EQ(data.id(), "y"_hs);
    ASSERT_FALSE(data.is_const());
    ASSERT_FALSE(data.is_static());
    ASSERT_EQ(data.get(instance).cast<int>(), 0);
    ASSERT_TRUE(data.set(instance, 42));
    ASSERT_EQ(data.get(instance).cast<int>(), 42);
}

TEST_F(MetaData, SetterGetterWithRefAsMemberFunctions) {
    auto data = entt::resolve<setter_getter_t>().data("w"_hs);
    setter_getter_t instance{};

    ASSERT_TRUE(data);
    ASSERT_EQ(data.parent(), entt::resolve_id("setter_getter"_hs));
    ASSERT_EQ(data.type(), entt::resolve<int>());
    ASSERT_EQ(data.id(), "w"_hs);
    ASSERT_FALSE(data.is_const());
    ASSERT_FALSE(data.is_static());
    ASSERT_EQ(data.get(instance).cast<int>(), 0);
    ASSERT_TRUE(data.set(instance, 42));
    ASSERT_EQ(data.get(instance).cast<int>(), 42);
}

TEST_F(MetaData, SetterGetterMixed) {
    auto data = entt::resolve<setter_getter_t>().data("z"_hs);
    setter_getter_t instance{};

    ASSERT_TRUE(data);
    ASSERT_EQ(data.parent(), entt::resolve_id("setter_getter"_hs));
    ASSERT_EQ(data.type(), entt::resolve<int>());
    ASSERT_EQ(data.id(), "z"_hs);
    ASSERT_FALSE(data.is_const());
    ASSERT_FALSE(data.is_static());
    ASSERT_EQ(data.get(instance).cast<int>(), 0);
    ASSERT_TRUE(data.set(instance, 42));
    ASSERT_EQ(data.get(instance).cast<int>(), 42);
}

TEST_F(MetaData, SetterGetterReadOnly) {
    auto data = entt::resolve<setter_getter_t>().data("z_ro"_hs);
    setter_getter_t instance{};

    ASSERT_TRUE(data);
    ASSERT_EQ(data.parent(), entt::resolve_id("setter_getter"_hs));
    ASSERT_EQ(data.type(), entt::resolve<int>());
    ASSERT_EQ(data.id(), "z_ro"_hs);
    ASSERT_TRUE(data.is_const());
    ASSERT_FALSE(data.is_static());
    ASSERT_EQ(data.get(instance).cast<int>(), 0);
    ASSERT_FALSE(data.set(instance, 42));
    ASSERT_EQ(data.get(instance).cast<int>(), 0);
}

TEST_F(MetaData, SetterGetterReadOnlyDataMember) {
    auto data = entt::resolve<setter_getter_t>().data("value"_hs);
    setter_getter_t instance{};

    ASSERT_TRUE(data);
    ASSERT_EQ(data.parent(), entt::resolve_id("setter_getter"_hs));
    ASSERT_EQ(data.type(), entt::resolve<int>());
    ASSERT_EQ(data.id(), "value"_hs);
    ASSERT_TRUE(data.is_const());
    ASSERT_FALSE(data.is_static());
    ASSERT_EQ(data.get(instance).cast<int>(), 0);
    ASSERT_FALSE(data.set(instance, 42));
    ASSERT_EQ(data.get(instance).cast<int>(), 0);
}

TEST_F(MetaData, ArrayStatic) {
    auto data = entt::resolve<array_t>().data("global"_hs);

    ASSERT_TRUE(data);
    ASSERT_EQ(data.parent(), entt::resolve_id("array"_hs));
    ASSERT_EQ(data.type(), entt::resolve<int[3]>());
    ASSERT_EQ(data.id(), "global"_hs);
    ASSERT_FALSE(data.is_const());
    ASSERT_TRUE(data.is_static());
    ASSERT_TRUE(data.type().is_array());
    ASSERT_EQ(data.type().extent(), 3u);
    ASSERT_FALSE(data.get({}));
}

TEST_F(MetaData, Array) {
    auto data = entt::resolve<array_t>().data("local"_hs);
    array_t instance;

    ASSERT_TRUE(data);
    ASSERT_EQ(data.parent(), entt::resolve_id("array"_hs));
    ASSERT_EQ(data.type(), entt::resolve<int[5]>());
    ASSERT_EQ(data.id(), "local"_hs);
    ASSERT_FALSE(data.is_const());
    ASSERT_FALSE(data.is_static());
    ASSERT_TRUE(data.type().is_array());
    ASSERT_EQ(data.type().extent(), 5u);
    ASSERT_FALSE(data.get(instance));
}

TEST_F(MetaData, AsVoid) {
    auto data = entt::resolve<clazz_t>().data("void"_hs);
    clazz_t instance{};

    ASSERT_TRUE(data.set(instance, 42));
    ASSERT_EQ(instance.i, 42);
    ASSERT_EQ(data.get(instance), entt::meta_any{std::in_place_type<void>});
}

TEST_F(MetaData, AsRef) {
    clazz_t instance{};

    auto h_data = entt::resolve<clazz_t>().data("h"_hs);
    auto i_data = entt::resolve<clazz_t>().data("i"_hs);

    ASSERT_EQ(h_data.type(), entt::resolve<int>());
    ASSERT_EQ(i_data.type(), entt::resolve<int>());

    h_data.get(instance).cast<int>() = 3;
    i_data.get(instance).cast<int>() = 3;

    ASSERT_NE(instance.h, 3);
    ASSERT_EQ(instance.i, 3);
}

TEST_F(MetaData, FromBase) {
    auto type = entt::resolve<derived_t>();
    derived_t instance;

    ASSERT_TRUE(type.data("value"_hs));

    ASSERT_EQ(instance.value, 3);
    ASSERT_TRUE(type.data("value"_hs).set(instance, 42));
    ASSERT_EQ(instance.value, 42);
}

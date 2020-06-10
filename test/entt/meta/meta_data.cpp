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
    int local[3];
};

enum class properties {
    random,
    value
};

struct Meta: ::testing::Test {
    static void SetUpTestCase() {
        entt::meta<double>().conv<int>();
        entt::meta<base_t>().dtor<&base_t::destroy>();
        entt::meta<derived_t>().base<base_t>().dtor<&derived_t::destroy>();

        entt::meta<clazz_t>().type("clazz"_hs)
                .data<&clazz_t::i, entt::as_ref_t>("i"_hs).prop(3, 0)
                .data<&clazz_t::j>("j"_hs).prop(true, 1)
                .data<&clazz_t::h>("h"_hs).prop(properties::random, 2)
                .data<&clazz_t::k>("k"_hs).prop(properties::value, 3)
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

TEST_F(Meta, MetaData) {
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

    data.prop([](auto prop) {
        ASSERT_EQ(prop.key(), 3);
        ASSERT_EQ(prop.value(), 0);
    });

    ASSERT_FALSE(data.prop(2));
    ASSERT_FALSE(data.prop('c'));

    auto prop = data.prop(3);

    ASSERT_TRUE(prop);
    ASSERT_EQ(prop.key(), 3);
    ASSERT_EQ(prop.value(), 0);
}

TEST_F(Meta, MetaDataConst) {
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

    data.prop([](auto prop) {
        ASSERT_EQ(prop.key(), true);
        ASSERT_EQ(prop.value(), 1);
    });

    ASSERT_FALSE(data.prop(false));
    ASSERT_FALSE(data.prop('c'));

    auto prop = data.prop(true);

    ASSERT_TRUE(prop);
    ASSERT_EQ(prop.key(), true);
    ASSERT_EQ(prop.value(), 1);
}

TEST_F(Meta, MetaDataStatic) {
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

    data.prop([](auto prop) {
        ASSERT_EQ(prop.key(), properties::random);
        ASSERT_EQ(prop.value(), 2);
    });

    ASSERT_FALSE(data.prop(properties::value));
    ASSERT_FALSE(data.prop('c'));

    auto prop = data.prop(properties::random);

    ASSERT_TRUE(prop);
    ASSERT_EQ(prop.key(), properties::random);
    ASSERT_EQ(prop.value(), 2);
}

TEST_F(Meta, MetaDataConstStatic) {
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

    data.prop([](auto prop) {
        ASSERT_EQ(prop.key(), properties::value);
        ASSERT_EQ(prop.value(), 3);
    });

    ASSERT_FALSE(data.prop(properties::random));
    ASSERT_FALSE(data.prop('c'));

    auto prop = data.prop(properties::value);

    ASSERT_TRUE(prop);
    ASSERT_EQ(prop.key(), properties::value);
    ASSERT_EQ(prop.value(), 3);
}

TEST_F(Meta, MetaDataGetMetaAnyArg) {
    entt::meta_any any{clazz_t{}};
    any.cast<clazz_t>().i = 99;
    const auto value = entt::resolve<clazz_t>().data("i"_hs).get(any);

    ASSERT_TRUE(value);
    ASSERT_TRUE(value.cast<int>());
    ASSERT_EQ(value.cast<int>(), 99);
}

TEST_F(Meta, MetaDataGetInvalidArg) {
    auto instance = 0;
    ASSERT_FALSE(entt::resolve<clazz_t>().data("i"_hs).get(instance));
}

TEST_F(Meta, MetaDataSetMetaAnyArg) {
    entt::meta_any any{clazz_t{}};
    entt::meta_any value{42};

    ASSERT_EQ(any.cast<clazz_t>().i, 0);
    ASSERT_TRUE(entt::resolve<clazz_t>().data("i"_hs).set(any, value));
    ASSERT_EQ(any.cast<clazz_t>().i, 42);
}

TEST_F(Meta, MetaDataSetInvalidArg) {
    ASSERT_FALSE(entt::resolve<clazz_t>().data("i"_hs).set({}, 'c'));
}

TEST_F(Meta, MetaDataSetCast) {
    clazz_t instance{};

    ASSERT_EQ(base_t::counter, 0);
    ASSERT_TRUE(entt::resolve<clazz_t>().data("base"_hs).set(instance, derived_t{}));
    ASSERT_EQ(base_t::counter, 1);
}

TEST_F(Meta, MetaDataSetConvert) {
    clazz_t instance{};

    ASSERT_EQ(instance.i, 0);
    ASSERT_TRUE(entt::resolve<clazz_t>().data("i"_hs).set(instance, 3.));
    ASSERT_EQ(instance.i, 3);
}

TEST_F(Meta, MetaDataSetterGetterAsFreeFunctions) {
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

TEST_F(Meta, MetaDataSetterGetterAsMemberFunctions) {
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

TEST_F(Meta, MetaDataSetterGetterWithRefAsMemberFunctions) {
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

TEST_F(Meta, MetaDataSetterGetterMixed) {
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

TEST_F(Meta, MetaDataSetterGetterReadOnly) {
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

TEST_F(Meta, MetaDataSetterGetterReadOnlyDataMember) {
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

TEST_F(Meta, MetaDataArrayStatic) {
    auto data = entt::resolve<array_t>().data("global"_hs);

    array_t::global[0] = 3;
    array_t::global[1] = 5;
    array_t::global[2] = 7;

    ASSERT_TRUE(data);
    ASSERT_EQ(data.parent(), entt::resolve_id("array"_hs));
    ASSERT_EQ(data.type(), entt::resolve<int[3]>());
    ASSERT_EQ(data.id(), "global"_hs);
    ASSERT_FALSE(data.is_const());
    ASSERT_TRUE(data.is_static());
    ASSERT_TRUE(data.type().is_array());
    ASSERT_EQ(data.type().extent(), 3);
    ASSERT_EQ(data.get({}, 0).cast<int>(), 3);
    ASSERT_EQ(data.get({}, 1).cast<int>(), 5);
    ASSERT_EQ(data.get({}, 2).cast<int>(), 7);
    ASSERT_FALSE(data.set({}, 0, 'c'));
    ASSERT_EQ(data.get({}, 0).cast<int>(), 3);
    ASSERT_TRUE(data.set({}, 0, data.get({}, 0).cast<int>()+2));
    ASSERT_TRUE(data.set({}, 1, data.get({}, 1).cast<int>()+2));
    ASSERT_TRUE(data.set({}, 2, data.get({}, 2).cast<int>()+2));
    ASSERT_EQ(data.get({}, 0).cast<int>(), 5);
    ASSERT_EQ(data.get({}, 1).cast<int>(), 7);
    ASSERT_EQ(data.get({}, 2).cast<int>(), 9);
}

TEST_F(Meta, MetaDataArray) {
    auto data = entt::resolve<array_t>().data("local"_hs);
    array_t instance;

    instance.local[0] = 3;
    instance.local[1] = 5;
    instance.local[2] = 7;

    ASSERT_TRUE(data);
    ASSERT_EQ(data.parent(), entt::resolve_id("array"_hs));
    ASSERT_EQ(data.type(), entt::resolve<int[3]>());
    ASSERT_EQ(data.id(), "local"_hs);
    ASSERT_FALSE(data.is_const());
    ASSERT_FALSE(data.is_static());
    ASSERT_TRUE(data.type().is_array());
    ASSERT_EQ(data.type().extent(), 3);
    ASSERT_EQ(data.get(instance, 0).cast<int>(), 3);
    ASSERT_EQ(data.get(instance, 1).cast<int>(), 5);
    ASSERT_EQ(data.get(instance, 2).cast<int>(), 7);
    ASSERT_FALSE(data.set(instance, 0, 'c'));
    ASSERT_EQ(data.get(instance, 0).cast<int>(), 3);
    ASSERT_TRUE(data.set(instance, 0, data.get(instance, 0).cast<int>()+2));
    ASSERT_TRUE(data.set(instance, 1, data.get(instance, 1).cast<int>()+2));
    ASSERT_TRUE(data.set(instance, 2, data.get(instance, 2).cast<int>()+2));
    ASSERT_EQ(data.get(instance, 0).cast<int>(), 5);
    ASSERT_EQ(data.get(instance, 1).cast<int>(), 7);
    ASSERT_EQ(data.get(instance, 2).cast<int>(), 9);
}

TEST_F(Meta, MetaDataAsVoid) {
    auto data = entt::resolve<clazz_t>().data("void"_hs);
    clazz_t instance{};

    ASSERT_TRUE(data.set(instance, 42));
    ASSERT_EQ(instance.i, 42);
    ASSERT_EQ(data.get(instance), entt::meta_any{std::in_place_type<void>});
}

TEST_F(Meta, MetaDataAsRef) {
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

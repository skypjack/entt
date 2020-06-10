#include <utility>
#include <functional>
#include <type_traits>
#include <gtest/gtest.h>
#include <entt/core/hashed_string.hpp>
#include <entt/core/type_info.hpp>
#include <entt/meta/factory.hpp>
#include <entt/meta/meta.hpp>
#include <entt/meta/resolve.hpp>
#include "fixture.h"

TEST_F(Meta, MetaData) {
    auto data = entt::resolve<data_type>().data("i"_hs);
    data_type instance{};

    ASSERT_TRUE(data);
    ASSERT_EQ(data.parent(), entt::resolve_id("data"_hs));
    ASSERT_EQ(data.type(), entt::resolve<int>());
    ASSERT_EQ(data.id(), "i"_hs);
    ASSERT_FALSE(data.is_const());
    ASSERT_FALSE(data.is_static());
    ASSERT_EQ(data.get(instance).cast<int>(), 0);
    ASSERT_TRUE(data.set(instance, 42));
    ASSERT_EQ(data.get(instance).cast<int>(), 42);

    data.prop([](auto prop) {
        ASSERT_EQ(prop.key(), props::prop_int);
        ASSERT_EQ(prop.value(), 0);
    });

    ASSERT_FALSE(data.prop(props::prop_bool));

    auto prop = data.prop(props::prop_int);

    ASSERT_TRUE(prop);
    ASSERT_EQ(prop.key(), props::prop_int);
    ASSERT_EQ(prop.value(), 0);
}

TEST_F(Meta, MetaDataConst) {
    auto data = entt::resolve<data_type>().data("j"_hs);
    data_type instance{};

    ASSERT_TRUE(data);
    ASSERT_EQ(data.parent(), entt::resolve_id("data"_hs));
    ASSERT_EQ(data.type(), entt::resolve<int>());
    ASSERT_EQ(data.id(), "j"_hs);
    ASSERT_TRUE(data.is_const());
    ASSERT_FALSE(data.is_static());
    ASSERT_EQ(data.get(instance).cast<int>(), 1);
    ASSERT_FALSE(data.set(instance, 42));
    ASSERT_EQ(data.get(instance).cast<int>(), 1);

    data.prop([](auto prop) {
        ASSERT_EQ(prop.key(), props::prop_int);
        ASSERT_EQ(prop.value(), 1);
    });

    ASSERT_FALSE(data.prop(props::prop_bool));

    auto prop = data.prop(props::prop_int);

    ASSERT_TRUE(prop);
    ASSERT_EQ(prop.key(), props::prop_int);
    ASSERT_EQ(prop.value(), 1);
}

TEST_F(Meta, MetaDataStatic) {
    auto data = entt::resolve<data_type>().data("h"_hs);

    ASSERT_TRUE(data);
    ASSERT_EQ(data.parent(), entt::resolve_id("data"_hs));
    ASSERT_EQ(data.type(), entt::resolve<int>());
    ASSERT_EQ(data.id(), "h"_hs);
    ASSERT_FALSE(data.is_const());
    ASSERT_TRUE(data.is_static());
    ASSERT_EQ(data.get({}).cast<int>(), 2);
    ASSERT_TRUE(data.set({}, 42));
    ASSERT_EQ(data.get({}).cast<int>(), 42);

    data.prop([](auto prop) {
        ASSERT_EQ(prop.key(), props::prop_int);
        ASSERT_EQ(prop.value(), 2);
    });

    ASSERT_FALSE(data.prop(props::prop_bool));

    auto prop = data.prop(props::prop_int);

    ASSERT_TRUE(prop);
    ASSERT_EQ(prop.key(), props::prop_int);
    ASSERT_EQ(prop.value(), 2);
}

TEST_F(Meta, MetaDataConstStatic) {
    auto data = entt::resolve<data_type>().data("k"_hs);

    ASSERT_TRUE(data);
    ASSERT_EQ(data.parent(), entt::resolve_id("data"_hs));
    ASSERT_EQ(data.type(), entt::resolve<int>());
    ASSERT_EQ(data.id(), "k"_hs);
    ASSERT_TRUE(data.is_const());
    ASSERT_TRUE(data.is_static());
    ASSERT_EQ(data.get({}).cast<int>(), 3);
    ASSERT_FALSE(data.set({}, 42));
    ASSERT_EQ(data.get({}).cast<int>(), 3);

    data.prop([](auto prop) {
        ASSERT_EQ(prop.key(), props::prop_int);
        ASSERT_EQ(prop.value(), 3);
    });

    ASSERT_FALSE(data.prop(props::prop_bool));

    auto prop = data.prop(props::prop_int);

    ASSERT_TRUE(prop);
    ASSERT_EQ(prop.key(), props::prop_int);
    ASSERT_EQ(prop.value(), 3);
}

TEST_F(Meta, MetaDataGetMetaAnyArg) {
    entt::meta_any any{data_type{}};
    any.cast<data_type>().i = 99;
    const auto value = entt::resolve<data_type>().data("i"_hs).get(any);

    ASSERT_TRUE(value);
    ASSERT_TRUE(value.cast<int>());
    ASSERT_EQ(value.cast<int>(), 99);
}

TEST_F(Meta, MetaDataGetInvalidArg) {
    auto instance = 0;
    ASSERT_FALSE(entt::resolve<data_type>().data("i"_hs).get(instance));
}

TEST_F(Meta, MetaDataSetMetaAnyArg) {
    entt::meta_any any{data_type{}};
    entt::meta_any value{42};

    ASSERT_EQ(any.cast<data_type>().i, 0);
    ASSERT_TRUE(entt::resolve<data_type>().data("i"_hs).set(any, value));
    ASSERT_EQ(any.cast<data_type>().i, 42);
}

TEST_F(Meta, MetaDataSetInvalidArg) {
    ASSERT_FALSE(entt::resolve<data_type>().data("i"_hs).set({}, 'c'));
}

TEST_F(Meta, MetaDataSetCast) {
    data_type instance{};

    ASSERT_EQ(empty_type::counter, 0);
    ASSERT_TRUE(entt::resolve<data_type>().data("empty"_hs).set(instance, fat_type{}));
    ASSERT_EQ(empty_type::counter, 1);
}

TEST_F(Meta, MetaDataSetConvert) {
    data_type instance{};

    ASSERT_EQ(instance.i, 0);
    ASSERT_TRUE(entt::resolve<data_type>().data("i"_hs).set(instance, 3.));
    ASSERT_EQ(instance.i, 3);
}

TEST_F(Meta, MetaDataSetterGetterAsFreeFunctions) {
    auto data = entt::resolve<setter_getter_type>().data("x"_hs);
    setter_getter_type instance{};

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
    auto data = entt::resolve<setter_getter_type>().data("y"_hs);
    setter_getter_type instance{};

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
    auto data = entt::resolve<setter_getter_type>().data("w"_hs);
    setter_getter_type instance{};

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
    auto data = entt::resolve<setter_getter_type>().data("z"_hs);
    setter_getter_type instance{};

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
    auto data = entt::resolve<setter_getter_type>().data("z_ro"_hs);
    setter_getter_type instance{};

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
    auto data = entt::resolve<setter_getter_type>().data("value"_hs);
    setter_getter_type instance{};

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
    auto data = entt::resolve<array_type>().data("global"_hs);

    array_type::global[0] = 3;
    array_type::global[1] = 5;
    array_type::global[2] = 7;

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
    auto data = entt::resolve<array_type>().data("local"_hs);
    array_type instance;

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
    auto data = entt::resolve<data_type>().data("v"_hs);
    data_type instance{};

    ASSERT_TRUE(data.set(instance, 42));
    ASSERT_EQ(instance.v, 42);
    ASSERT_EQ(data.get(instance), entt::meta_any{std::in_place_type<void>});
}

TEST_F(Meta, MetaDataAsAlias) {
    data_type instance{};
    auto h_data = entt::resolve<data_type>().data("h"_hs);
    auto i_data = entt::resolve<data_type>().data("i"_hs);

    h_data.get(instance).cast<int>() = 3;
    i_data.get(instance).cast<int>() = 3;

    ASSERT_EQ(h_data.type(), entt::resolve<int>());
    ASSERT_EQ(i_data.type(), entt::resolve<int>());
    ASSERT_NE(instance.h, 3);
    ASSERT_EQ(instance.i, 3);
}

TEST_F(Meta, MetaFunc) {
    auto func = entt::resolve<func_type>().func("f2"_hs);
    func_type instance{};

    ASSERT_TRUE(func);
    ASSERT_EQ(func.parent(), entt::resolve_id("func"_hs));
    ASSERT_EQ(func.id(), "f2"_hs);
    ASSERT_EQ(func.size(), 2u);
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
    ASSERT_EQ(func_type::value, 3);

    func.prop([](auto prop) {
        ASSERT_EQ(prop.key(), props::prop_bool);
        ASSERT_FALSE(prop.value().template cast<bool>());
    });

    ASSERT_FALSE(func.prop(props::prop_int));

    auto prop = func.prop(props::prop_bool);

    ASSERT_TRUE(prop);
    ASSERT_EQ(prop.key(), props::prop_bool);
    ASSERT_FALSE(prop.value().cast<bool>());
}

TEST_F(Meta, MetaFuncConst) {
    auto func = entt::resolve<func_type>().func("f1"_hs);
    func_type instance{};

    ASSERT_TRUE(func);
    ASSERT_EQ(func.parent(), entt::resolve_id("func"_hs));
    ASSERT_EQ(func.id(), "f1"_hs);
    ASSERT_EQ(func.size(), 1u);
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

    func.prop([](auto prop) {
        ASSERT_EQ(prop.key(), props::prop_bool);
        ASSERT_FALSE(prop.value().template cast<bool>());
    });

    ASSERT_FALSE(func.prop(props::prop_int));

    auto prop = func.prop(props::prop_bool);

    ASSERT_TRUE(prop);
    ASSERT_EQ(prop.key(), props::prop_bool);
    ASSERT_FALSE(prop.value().cast<bool>());
}

TEST_F(Meta, MetaFuncRetVoid) {
    auto func = entt::resolve<func_type>().func("g"_hs);
    func_type instance{};

    ASSERT_TRUE(func);
    ASSERT_EQ(func.parent(), entt::resolve_id("func"_hs));
    ASSERT_EQ(func.id(), "g"_hs);
    ASSERT_EQ(func.size(), 1u);
    ASSERT_FALSE(func.is_const());
    ASSERT_FALSE(func.is_static());
    ASSERT_EQ(func.ret(), entt::resolve<void>());
    ASSERT_EQ(func.arg(0u), entt::resolve<int>());
    ASSERT_FALSE(func.arg(1u));

    auto any = func.invoke(instance, 5);

    ASSERT_TRUE(any);
    ASSERT_EQ(any.type(), entt::resolve<void>());
    ASSERT_EQ(func_type::value, 25);

    func.prop([](auto prop) {
        ASSERT_EQ(prop.key(), props::prop_bool);
        ASSERT_FALSE(prop.value().template cast<bool>());
    });

    ASSERT_FALSE(func.prop(props::prop_int));

    auto prop = func.prop(props::prop_bool);

    ASSERT_TRUE(prop);
    ASSERT_EQ(prop.key(), props::prop_bool);
    ASSERT_FALSE(prop.value().cast<bool>());
}

TEST_F(Meta, MetaFuncStatic) {
    auto func = entt::resolve<func_type>().func("h"_hs);
    func_type::value = 2;

    ASSERT_TRUE(func);
    ASSERT_EQ(func.parent(), entt::resolve_id("func"_hs));
    ASSERT_EQ(func.id(), "h"_hs);
    ASSERT_EQ(func.size(), 1u);
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

    func.prop([](auto prop) {
        ASSERT_EQ(prop.key(), props::prop_bool);
        ASSERT_FALSE(prop.value().template cast<bool>());
    });

    ASSERT_FALSE(func.prop(props::prop_int));

    auto prop = func.prop(props::prop_bool);

    ASSERT_TRUE(prop);
    ASSERT_EQ(prop.key(), props::prop_bool);
    ASSERT_FALSE(prop.value().cast<bool>());
}

TEST_F(Meta, MetaFuncStaticRetVoid) {
    auto func = entt::resolve<func_type>().func("k"_hs);

    ASSERT_TRUE(func);
    ASSERT_EQ(func.parent(), entt::resolve_id("func"_hs));
    ASSERT_EQ(func.id(), "k"_hs);
    ASSERT_EQ(func.size(), 1u);
    ASSERT_FALSE(func.is_const());
    ASSERT_TRUE(func.is_static());
    ASSERT_EQ(func.ret(), entt::resolve<void>());
    ASSERT_EQ(func.arg(0u), entt::resolve<int>());
    ASSERT_FALSE(func.arg(1u));

    auto any = func.invoke({}, 42);

    ASSERT_TRUE(any);
    ASSERT_EQ(any.type(), entt::resolve<void>());
    ASSERT_EQ(func_type::value, 42);

    func.prop([](auto prop) {
        ASSERT_TRUE(prop);
        ASSERT_EQ(prop.key(), props::prop_bool);
        ASSERT_FALSE(prop.value().template cast<bool>());
    });

    ASSERT_FALSE(func.prop(props::prop_int));

    auto prop = func.prop(props::prop_bool);

    ASSERT_TRUE(prop);
    ASSERT_EQ(prop.key(), props::prop_bool);
    ASSERT_FALSE(prop.value().cast<bool>());
}

TEST_F(Meta, MetaFuncMetaAnyArgs) {
    func_type instance;
    auto any = entt::resolve<func_type>().func("f1"_hs).invoke(instance, 3);

    ASSERT_TRUE(any);
    ASSERT_EQ(any.type(), entt::resolve<int>());
    ASSERT_EQ(any.cast<int>(), 9);
}

TEST_F(Meta, MetaFuncInvalidArgs) {
    empty_type instance;

    ASSERT_FALSE(entt::resolve<func_type>().func("f1"_hs).invoke(instance, 'c'));
}

TEST_F(Meta, MetaFuncCastAndConvert) {
    func_type instance;
    auto any = entt::resolve<func_type>().func("f3"_hs).invoke(instance, derived_type{}, 0, 3.);

    ASSERT_TRUE(any);
    ASSERT_EQ(any.type(), entt::resolve<int>());
    ASSERT_EQ(any.cast<int>(), 9);
}

TEST_F(Meta, MetaFuncAsVoid) {
    auto func = entt::resolve<func_type>().func("v"_hs);
    func_type instance{};

    ASSERT_EQ(func.invoke(instance, 42), entt::meta_any{std::in_place_type<void>});
    ASSERT_EQ(func.ret(), entt::resolve<void>());
    ASSERT_EQ(instance.value, 42);
}

TEST_F(Meta, MetaFuncAsAlias) {
    func_type instance{};
    auto func = entt::resolve<func_type>().func("a"_hs);
    func.invoke(instance).cast<int>() = 3;

    ASSERT_EQ(func.ret(), entt::resolve<int>());
    ASSERT_EQ(instance.value, 3);
}

TEST_F(Meta, MetaFuncByReference) {
    auto func = entt::resolve<func_type>().func("h"_hs);
    func_type::value = 2;
    entt::meta_any any{3};
    int value = 4;

    ASSERT_EQ(func.invoke({}, std::ref(value)).cast<int>(), 8);
    ASSERT_EQ(func.invoke({}, *any).cast<int>(), 6);
    ASSERT_EQ(any.cast<int>(), 6);
    ASSERT_EQ(value, 8);
}

TEST_F(Meta, MetaType) {
    auto type = entt::resolve<derived_type>();

    ASSERT_TRUE(type);
    ASSERT_NE(type, entt::meta_type{});
    ASSERT_EQ(type.id(), "derived"_hs);
    ASSERT_EQ(type.type_id(), entt::type_info<derived_type>::id());

    type.prop([](auto prop) {
        ASSERT_EQ(prop.key(), props::prop_int);
        ASSERT_EQ(prop.value(), 99);
    });

    ASSERT_FALSE(type.prop(props::prop_bool));

    auto prop = type.prop(props::prop_int);

    ASSERT_TRUE(prop);
    ASSERT_EQ(prop.key(), props::prop_int);
    ASSERT_EQ(prop.value(), 99);
}

TEST_F(Meta, MetaTypeTraits) {
    ASSERT_TRUE(entt::resolve<void>().is_void());
    ASSERT_TRUE(entt::resolve<bool>().is_integral());
    ASSERT_TRUE(entt::resolve<double>().is_floating_point());
    ASSERT_TRUE(entt::resolve<props>().is_enum());
    ASSERT_TRUE(entt::resolve<union_type>().is_union());
    ASSERT_TRUE(entt::resolve<derived_type>().is_class());
    ASSERT_TRUE(entt::resolve<int *>().is_pointer());
    ASSERT_TRUE(entt::resolve<decltype(&empty_type::destroy)>().is_function_pointer());
    ASSERT_TRUE(entt::resolve<decltype(&data_type::i)>().is_member_object_pointer());
    ASSERT_TRUE(entt::resolve<decltype(&func_type::g)>().is_member_function_pointer());
}

TEST_F(Meta, MetaTypeRemovePointer) {
    ASSERT_EQ(entt::resolve<void *>().remove_pointer(), entt::resolve<void>());
    ASSERT_EQ(entt::resolve<int(*)(char, double)>().remove_pointer(), entt::resolve<int(char, double)>());
    ASSERT_EQ(entt::resolve<derived_type>().remove_pointer(), entt::resolve<derived_type>());
}

TEST_F(Meta, MetaTypeRemoveExtent) {
    ASSERT_EQ(entt::resolve<int[3]>().remove_extent(), entt::resolve<int>());
    ASSERT_EQ(entt::resolve<int[3][3]>().remove_extent(), entt::resolve<int[3]>());
    ASSERT_EQ(entt::resolve<derived_type>().remove_extent(), entt::resolve<derived_type>());
}

TEST_F(Meta, MetaTypeBase) {
    auto type = entt::resolve<derived_type>();
    bool iterate = false;

    type.base([&iterate](auto base) {
        ASSERT_EQ(base.type(), entt::resolve<base_type>());
        iterate = true;
    });

    ASSERT_TRUE(iterate);
    ASSERT_EQ(type.base("base"_hs).type(), entt::resolve<base_type>());
}

TEST_F(Meta, MetaTypeConv) {
    auto type = entt::resolve<double>();
    bool iterate = false;

    type.conv([&iterate](auto conv) {
        ASSERT_EQ(conv.type(), entt::resolve<int>());
        iterate = true;
    });

    ASSERT_TRUE(iterate);

    auto conv = type.conv<int>();

    ASSERT_EQ(conv.type(), entt::resolve<int>());
    ASSERT_FALSE(type.conv<char>());
}

TEST_F(Meta, MetaTypeCtor) {
    auto type = entt::resolve<derived_type>();
    int counter{};

    type.ctor([&counter](auto) {
        ++counter;
    });

    ASSERT_EQ(counter, 2);
    ASSERT_TRUE((type.ctor<const base_type &, int>()));
    ASSERT_TRUE((type.ctor<const derived_type &, double>()));
}

TEST_F(Meta, MetaTypeData) {
    auto type = entt::resolve<data_type>();
    int counter{};

    type.data([&counter](auto) {
        ++counter;
    });

    ASSERT_EQ(counter, 6);
    ASSERT_TRUE(type.data("i"_hs));
}

TEST_F(Meta, MetaTypeFunc) {
    auto type = entt::resolve<func_type>();
    int counter{};

    type.func([&counter](auto) {
        ++counter;
    });

    ASSERT_EQ(counter, 8);
    ASSERT_TRUE(type.func("f1"_hs));
}

TEST_F(Meta, MetaTypeConstruct) {
    auto any = entt::resolve<derived_type>().construct(base_type{}, 42, 'c');

    ASSERT_TRUE(any);
    ASSERT_EQ(any.cast<derived_type>().i, 42);
    ASSERT_EQ(any.cast<derived_type>().c, 'c');
}

TEST_F(Meta, MetaTypeConstructNoArgs) {
    // this should work, no other tests required
    auto any = entt::resolve<empty_type>().construct();

    ASSERT_TRUE(any);
}

TEST_F(Meta, MetaTypeConstructMetaAnyArgs) {
    auto any = entt::resolve<derived_type>().construct(base_type{}, 42, 'c');

    ASSERT_TRUE(any);
    ASSERT_EQ(any.cast<derived_type>().i, 42);
    ASSERT_EQ(any.cast<derived_type>().c, 'c');
}

TEST_F(Meta, MetaTypeConstructInvalidArgs) {
    ASSERT_FALSE(entt::resolve<derived_type>().construct(base_type{}, 'c', 42));
}

TEST_F(Meta, MetaTypeLessArgs) {
    ASSERT_FALSE(entt::resolve<derived_type>().construct(base_type{}));
}

TEST_F(Meta, MetaTypeConstructCastAndConvert) {
    auto any = entt::resolve<derived_type>().construct(derived_type{}, 42., 'c');

    ASSERT_TRUE(any);
    ASSERT_EQ(any.cast<derived_type>().i, 42);
    ASSERT_EQ(any.cast<derived_type>().c, 'c');
}

TEST_F(Meta, MetaTypeDetach) {
    ASSERT_TRUE(entt::resolve_id("char"_hs));

    entt::resolve([](auto type) {
        if(type.id() == "char"_hs) {
            type.detach();
        }
    });

    ASSERT_FALSE(entt::resolve_id("char"_hs));
    ASSERT_EQ(entt::resolve<char>().id(), "char"_hs);
    ASSERT_EQ(entt::resolve<char>().prop(props::prop_int).value().cast<int>(), 42);
    ASSERT_TRUE(entt::resolve<char>().data("value"_hs));

    entt::meta<char>().type("char"_hs);

    ASSERT_TRUE(entt::resolve_id("char"_hs));
}

TEST_F(Meta, MetaDataFromBase) {
    auto type = entt::resolve<concrete_type>();
    concrete_type instance;

    ASSERT_TRUE(type.data("i"_hs));
    ASSERT_TRUE(type.data("j"_hs));

    ASSERT_EQ(instance.i, 0);
    ASSERT_EQ(instance.j, char{});
    ASSERT_TRUE(type.data("i"_hs).set(instance, 3));
    ASSERT_TRUE(type.data("j"_hs).set(instance, 'c'));
    ASSERT_EQ(instance.i, 3);
    ASSERT_EQ(instance.j, 'c');
}

TEST_F(Meta, MetaFuncFromBase) {
    auto type = entt::resolve<concrete_type>();
    auto base = entt::resolve<an_abstract_type>();
    concrete_type instance;

    ASSERT_TRUE(type.func("f"_hs));
    ASSERT_TRUE(type.func("g"_hs));
    ASSERT_TRUE(type.func("h"_hs));

    ASSERT_EQ(type.func("f"_hs).parent(), entt::resolve<concrete_type>());
    ASSERT_EQ(type.func("g"_hs).parent(), entt::resolve<an_abstract_type>());
    ASSERT_EQ(type.func("h"_hs).parent(), entt::resolve<another_abstract_type>());

    ASSERT_EQ(instance.i, 0);
    ASSERT_EQ(instance.j, char{});

    type.func("f"_hs).invoke(instance, 3);
    type.func("h"_hs).invoke(instance, 'c');

    ASSERT_EQ(instance.i, 9);
    ASSERT_EQ(instance.j, 'c');

    base.func("g"_hs).invoke(instance, 3);

    ASSERT_EQ(instance.i, -3);
}

TEST_F(Meta, AbstractClass) {
    auto type = entt::resolve<an_abstract_type>();
    concrete_type instance;

    ASSERT_EQ(type.type_id(), entt::type_info<an_abstract_type>::id());
    ASSERT_EQ(instance.i, 0);

    type.func("f"_hs).invoke(instance, 3);

    ASSERT_EQ(instance.i, 3);

    type.func("g"_hs).invoke(instance, 3);

    ASSERT_EQ(instance.i, -3);
}

TEST_F(Meta, EnumAndNamedConstants) {
    auto type = entt::resolve<props>();

    ASSERT_TRUE(type.data("prop_bool"_hs));
    ASSERT_TRUE(type.data("prop_int"_hs));

    ASSERT_EQ(type.data("prop_bool"_hs).type(), type);
    ASSERT_EQ(type.data("prop_int"_hs).type(), type);

    ASSERT_FALSE(type.data("prop_bool"_hs).set({}, props::prop_int));
    ASSERT_FALSE(type.data("prop_int"_hs).set({}, props::prop_bool));

    ASSERT_EQ(type.data("prop_bool"_hs).get({}).cast<props>(), props::prop_bool);
    ASSERT_EQ(type.data("prop_int"_hs).get({}).cast<props>(), props::prop_int);
}

TEST_F(Meta, ArithmeticTypeAndNamedConstants) {
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

TEST_F(Meta, Variables) {
    auto p_data = entt::resolve<props>().data("value"_hs);
    auto c_data = entt::resolve_id("char"_hs).data("value"_hs);

    props prop{props::prop_int};
    char c = 'c';

    p_data.set(prop, props::prop_bool);
    c_data.set(c, 'x');

    ASSERT_EQ(p_data.get(prop).cast<props>(), props::prop_bool);
    ASSERT_EQ(c_data.get(c).cast<char>(), 'x');
    ASSERT_EQ(prop, props::prop_bool);
    ASSERT_EQ(c, 'x');
}

TEST_F(Meta, PropertiesAndCornerCases) {
    auto type = entt::resolve<props>();

    ASSERT_EQ(type.data("prop_bool"_hs).prop(props::prop_int).value().cast<int>(), 0);
    ASSERT_EQ(type.data("prop_bool"_hs).prop(props::prop_value).value().cast<int>(), 3);

    ASSERT_EQ(type.data("prop_int"_hs).prop(props::prop_bool).value().cast<bool>(), true);
    ASSERT_EQ(type.data("prop_int"_hs).prop(props::prop_int).value().cast<int>(), 0);
    ASSERT_EQ(type.data("prop_int"_hs).prop(props::prop_value).value().cast<int>(), 3);
    ASSERT_TRUE(type.data("prop_int"_hs).prop(props::key_only));
    ASSERT_FALSE(type.data("prop_int"_hs).prop(props::key_only).value());

    ASSERT_EQ(type.data("prop_list"_hs).prop(props::prop_bool).value().cast<bool>(), false);
    ASSERT_EQ(type.data("prop_list"_hs).prop(props::prop_int).value().cast<int>(), 0);
    ASSERT_EQ(type.data("prop_list"_hs).prop(props::prop_value).value().cast<int>(), 3);
    ASSERT_TRUE(type.data("prop_list"_hs).prop(props::key_only));
    ASSERT_FALSE(type.data("prop_list"_hs).prop(props::key_only).value());
}

TEST_F(Meta, Reset) {
    ASSERT_NE(*entt::internal::meta_context::global(), nullptr);

    entt::meta<char>().reset();
    entt::meta<concrete_type>().reset();
    entt::meta<setter_getter_type>().reset();
    entt::meta<fat_type>().reset();
    entt::meta<data_type>().reset();
    entt::meta<func_type>().reset();
    entt::meta<array_type>().reset();
    entt::meta<double>().reset();
    entt::meta<props>().reset();
    entt::meta<base_type>().reset();
    entt::meta<derived_type>().reset();
    entt::meta<empty_type>().reset();
    entt::meta<an_abstract_type>().reset();
    entt::meta<another_abstract_type>().reset();
    entt::meta<unsigned int>().reset();

    ASSERT_FALSE(entt::resolve_id("char"_hs));
    ASSERT_FALSE(entt::resolve_id("base"_hs));
    ASSERT_FALSE(entt::resolve_id("derived"_hs));
    ASSERT_FALSE(entt::resolve_id("empty"_hs));
    ASSERT_FALSE(entt::resolve_id("fat"_hs));
    ASSERT_FALSE(entt::resolve_id("data"_hs));
    ASSERT_FALSE(entt::resolve_id("func"_hs));
    ASSERT_FALSE(entt::resolve_id("setter_getter"_hs));
    ASSERT_FALSE(entt::resolve_id("an_abstract_type"_hs));
    ASSERT_FALSE(entt::resolve_id("another_abstract_type"_hs));
    ASSERT_FALSE(entt::resolve_id("concrete"_hs));

    ASSERT_EQ(*entt::internal::meta_context::global(), nullptr);

    Meta::SetUpAfterUnregistration();
    entt::meta_any any{42.};

    ASSERT_TRUE(any);
    ASSERT_FALSE(any.convert<int>());
    ASSERT_TRUE(any.convert<float>());

    ASSERT_FALSE(entt::resolve_id("derived"_hs));
    ASSERT_TRUE(entt::resolve_id("my_type"_hs));

    entt::resolve<derived_type>().prop([](auto prop) {
        ASSERT_EQ(prop.key(), props::prop_bool);
        ASSERT_FALSE(prop.value().template cast<bool>());
    });

    ASSERT_FALSE((entt::resolve<derived_type>().ctor<const base_type &, int, char>()));
    ASSERT_TRUE((entt::resolve<derived_type>().ctor<>()));

    ASSERT_TRUE(entt::resolve_id("your_type"_hs).data("a_data_member"_hs));
    ASSERT_FALSE(entt::resolve_id("your_type"_hs).data("another_data_member"_hs));

    ASSERT_TRUE(entt::resolve_id("your_type"_hs).func("a_member_function"_hs));
    ASSERT_FALSE(entt::resolve_id("your_type"_hs).func("another_member_function"_hs));
}

TEST_F(Meta, ReRegistrationAfterReset) {
    ASSERT_TRUE(entt::resolve<props>().data("prop_bool"_hs).prop(props::prop_int));
    ASSERT_TRUE(entt::resolve<props>().data("prop_bool"_hs).prop(props::prop_value));

    entt::meta<double>().reset();
    entt::meta<props>().reset();
    entt::meta<derived_type>().reset();
    entt::meta<another_abstract_type>().reset();

    Meta::SetUpAfterUnregistration();

    ASSERT_TRUE(entt::resolve<props>().data("prop_bool"_hs).prop(props::prop_int));
    ASSERT_TRUE(entt::resolve<props>().data("prop_bool"_hs).prop(props::prop_value));
}

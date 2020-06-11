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

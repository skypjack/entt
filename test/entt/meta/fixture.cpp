#include <entt/core/hashed_string.hpp>
#include <entt/meta/factory.hpp>
#include <entt/meta/meta.hpp>
#include <entt/meta/resolve.hpp>
#include "fixture.h"

void empty_type::destroy(empty_type &) {
    ++counter;
}

fat_type::fat_type(int *value)
    : foo{value}, bar{value}
{}

bool fat_type::operator==(const fat_type &other) const {
    return foo == other.foo && bar == other.bar;
}

derived_type::derived_type(const base_type &, int value, char character)
    : i{value}, c{character}
{}

int derived_type::f() const {
    return i;
}

char derived_type::g(const derived_type &type) {
    return type.c;
}

derived_type derived_factory(const base_type &, int value) {
    return {derived_type{}, value, 'c'};
}

int func_type::f(const base_type &, int a, int b) {
    return f(a, b);
}

int func_type::f(int a, int b) {
    value = a;
    return b*b;
}

int func_type::f(int v) const {
    return v*v;
}

void func_type::g(int v) {
    value = v*v;
}

int func_type::h(int &v) {
    return (v *= value);
}

void func_type::k(int v) {
    value = v;
}

int func_type::v(int v) const {
    return (value = v);
}

int & func_type::a() const {
    return value;
}

int setter_getter_type::setter(int val) {
    return value = val;
}

int setter_getter_type::getter() {
    return value;
}

int setter_getter_type::setter_with_ref(const int &val) {
    return value = val;
}

const int & setter_getter_type::getter_with_ref() {
    return value;
}

int setter_getter_type::static_setter(setter_getter_type &type, int value) {
    return type.value = value;
}

int setter_getter_type::static_getter(const setter_getter_type &type) {
    return type.value;
}

void an_abstract_type::f(int v) {
    i = v;
}

void concrete_type::f(int v) {
    i = v*v;
}

void concrete_type::g(int v) {
    i = -v;
}

void concrete_type::h(char c) {
    j = c;
}

void Meta::SetUpTestCase() {
    entt::meta<double>().conv<int>();

    entt::meta<char>()
            .type("char"_hs)
                .prop(props::prop_int, 42)
            .data<&set<char>, &get<char>>("value"_hs);

    entt::meta<props>()
            .data<props::prop_bool>("prop_bool"_hs)
                .prop(props::prop_int, 0)
                .prop(props::prop_value, 3)
            .data<props::prop_int>("prop_int"_hs)
                .prop(std::make_tuple(std::make_pair(props::prop_bool, true), std::make_pair(props::prop_int, 0), std::make_pair(props::prop_value, 3)))
                .prop(props::key_only)
            .data<props::key_only>("key_only"_hs)
                .prop([]() { return props::key_only; })
            .data<&set<props>, &get<props>>("value"_hs)
            .data<props::prop_list>("prop_list"_hs)
                .props(std::make_pair(props::prop_bool, false), std::make_pair(props::prop_int, 0), std::make_pair(props::prop_value, 3), props::key_only);

    entt::meta<unsigned int>().data<0u>("min"_hs).data<100u>("max"_hs);

    entt::meta<base_type>()
            .type("base"_hs);

    entt::meta<derived_type>()
            .type("derived"_hs)
                .prop(props::prop_int, 99)
            .base<base_type>()
            .ctor<const base_type &, int, char>()
                .prop(props::prop_bool, false)
            .ctor<&derived_factory>()
                .prop(props::prop_int, 42)
            .conv<&derived_type::f>()
            .conv<&derived_type::g>();

    entt::meta<empty_type>()
            .ctor<>()
            .type("empty"_hs)
            .dtor<&empty_type::destroy>();

    entt::meta<fat_type>()
            .type("fat"_hs)
            .base<empty_type>()
            .dtor<&fat_type::destroy>();

    entt::meta<data_type>()
            .type("data"_hs)
            .data<&data_type::i, entt::as_ref_t>("i"_hs)
                .prop(props::prop_int, 0)
            .data<&data_type::j>("j"_hs)
                .prop(props::prop_int, 1)
            .data<&data_type::h>("h"_hs)
                .prop(props::prop_int, 2)
            .data<&data_type::k>("k"_hs)
                .prop(props::prop_int, 3)
            .data<&data_type::empty>("empty"_hs)
            .data<&data_type::v, entt::as_void_t>("v"_hs);

    entt::meta<array_type>()
            .type("array"_hs)
            .data<&array_type::global>("global"_hs)
            .data<&array_type::local>("local"_hs);

    entt::meta<func_type>()
            .type("func"_hs)
            .func<entt::overload<int(const base_type &, int, int)>(&func_type::f)>("f3"_hs)
            .func<entt::overload<int(int, int)>(&func_type::f)>("f2"_hs)
                .prop(props::prop_bool, false)
            .func<entt::overload<int(int) const>(&func_type::f)>("f1"_hs)
                .prop(props::prop_bool, false)
            .func<&func_type::g>("g"_hs)
                .prop(props::prop_bool, false)
            .func<&func_type::h>("h"_hs)
                .prop(props::prop_bool, false)
            .func<&func_type::k>("k"_hs)
                .prop(props::prop_bool, false)
            .func<&func_type::v, entt::as_void_t>("v"_hs)
            .func<&func_type::a, entt::as_ref_t>("a"_hs);

    entt::meta<setter_getter_type>()
            .type("setter_getter"_hs)
            .data<&setter_getter_type::static_setter, &setter_getter_type::static_getter>("x"_hs)
            .data<&setter_getter_type::setter, &setter_getter_type::getter>("y"_hs)
            .data<&setter_getter_type::static_setter, &setter_getter_type::getter>("z"_hs)
            .data<&setter_getter_type::setter_with_ref, &setter_getter_type::getter_with_ref>("w"_hs)
            .data<nullptr, &setter_getter_type::getter>("z_ro"_hs)
            .data<nullptr, &setter_getter_type::value>("value"_hs);

    entt::meta<an_abstract_type>()
            .type("an_abstract_type"_hs)
                .prop(props::prop_bool, false)
            .data<&an_abstract_type::i>("i"_hs)
            .func<&an_abstract_type::f>("f"_hs)
            .func<&an_abstract_type::g>("g"_hs);

    entt::meta<another_abstract_type>()
            .type("another_abstract_type"_hs)
                .prop(props::prop_int, 42)
            .data<&another_abstract_type::j>("j"_hs)
            .func<&another_abstract_type::h>("h"_hs);

    entt::meta<concrete_type>()
            .type("concrete"_hs)
            .base<an_abstract_type>()
            .base<another_abstract_type>()
            .func<&concrete_type::f>("f"_hs);
}


void Meta::SetUpAfterUnregistration() {
    entt::meta<double>().conv<float>();

    entt::meta<props>()
            .data<props::prop_bool>("prop_bool"_hs)
                .prop(props::prop_int, 0)
                .prop(props::prop_value, 3);

    entt::meta<derived_type>()
            .type("my_type"_hs)
                .prop(props::prop_bool, false)
            .ctor<>();

    entt::meta<another_abstract_type>()
            .type("your_type"_hs)
            .data<&another_abstract_type::j>("a_data_member"_hs)
            .func<&another_abstract_type::h>("a_member_function"_hs);
}


void Meta::SetUp() {
    empty_type::counter = 0;
    func_type::value = 0;
}

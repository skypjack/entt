#include <gtest/gtest.h>
#include <entt/core/hashed_string.hpp>
#include <entt/meta/factory.hpp>
#include <entt/meta/meta.hpp>
#include <entt/meta/resolve.hpp>

template<typename Type>
void set(Type &prop, Type value) {
    prop = value;
}

template<typename Type>
Type get(Type &prop) {
    return prop;
}

struct base_t {};
struct derived_t: base_t {};

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

struct Meta: ::testing::Test {
    static void SetUpTestCase() {
        entt::meta<double>().conv<int>();
        entt::meta<base_t>().type("base"_hs);
        entt::meta<derived_t>().type("derived"_hs).base<base_t>();

        entt::meta<property_t>()
                .data<property_t::random>("random"_hs)
                    .prop(property_t::random, 0)
                    .prop(property_t::value, 3)
                .data<property_t::value>("value"_hs)
                    .prop(std::make_tuple(std::make_pair(property_t::random, true), std::make_pair(property_t::value, 0), std::make_pair(property_t::key_only, 3)))
                    .prop(property_t::list)
                .data<property_t::key_only>("key_only"_hs)
                    .prop([]() { return property_t::key_only; })
                .data<&set<property_t>, &get<property_t>>("wrap"_hs)
                .data<property_t::list>("list"_hs)
                   .props(std::make_pair(property_t::random, false), std::make_pair(property_t::value, 0), property_t::key_only);

        entt::meta<clazz_t>()
                .type("clazz"_hs)
                    .prop(property_t::value, 42)
                .ctor().ctor<const base_t &, int>()
                .data<&clazz_t::value>("value"_hs)
                .func<&clazz_t::member>("member"_hs)
                .func<&clazz_t::func>("func"_hs);
    }
};

TEST_F(Meta, MetaType) {
    auto type = entt::resolve<clazz_t>();

    ASSERT_TRUE(type);
    ASSERT_NE(type, entt::meta_type{});
    ASSERT_EQ(type.id(), "clazz"_hs);
    ASSERT_EQ(type.type_id(), entt::type_info<clazz_t>::id());

    type.prop([](auto prop) {
        ASSERT_EQ(prop.key(), property_t::value);
        ASSERT_EQ(prop.value(), 42);
    });

    ASSERT_FALSE(type.prop(property_t::key_only));
    ASSERT_FALSE(type.prop("property"_hs));

    auto prop = type.prop(property_t::value);

    ASSERT_TRUE(prop);
    ASSERT_EQ(prop.key(), property_t::value);
    ASSERT_EQ(prop.value(), 42);
}

TEST_F(Meta, MetaTypeTraits) {
    ASSERT_TRUE(entt::resolve<void>().is_void());
    ASSERT_TRUE(entt::resolve<bool>().is_integral());
    ASSERT_TRUE(entt::resolve<double>().is_floating_point());
    ASSERT_TRUE(entt::resolve<property_t>().is_enum());
    ASSERT_TRUE(entt::resolve<union_t>().is_union());
    ASSERT_TRUE(entt::resolve<derived_t>().is_class());
    ASSERT_TRUE(entt::resolve<int *>().is_pointer());
    ASSERT_TRUE(entt::resolve<decltype(&clazz_t::func)>().is_function_pointer());
    ASSERT_TRUE(entt::resolve<decltype(&clazz_t::value)>().is_member_object_pointer());
    ASSERT_TRUE(entt::resolve<decltype(&clazz_t::member)>().is_member_function_pointer());
}

TEST_F(Meta, MetaTypeRemovePointer) {
    ASSERT_EQ(entt::resolve<void *>().remove_pointer(), entt::resolve<void>());
    ASSERT_EQ(entt::resolve<int(*)(char, double)>().remove_pointer(), entt::resolve<int(char, double)>());
    ASSERT_EQ(entt::resolve<derived_t>().remove_pointer(), entt::resolve<derived_t>());
}

TEST_F(Meta, MetaTypeRemoveExtent) {
    ASSERT_EQ(entt::resolve<int[3]>().remove_extent(), entt::resolve<int>());
    ASSERT_EQ(entt::resolve<int[3][3]>().remove_extent(), entt::resolve<int[3]>());
    ASSERT_EQ(entt::resolve<derived_t>().remove_extent(), entt::resolve<derived_t>());
}

TEST_F(Meta, MetaTypeBase) {
    auto type = entt::resolve<derived_t>();
    bool iterate = false;

    type.base([&iterate](auto base) {
        ASSERT_EQ(base.type(), entt::resolve<base_t>());
        iterate = true;
    });

    ASSERT_TRUE(iterate);
    ASSERT_EQ(type.base("base"_hs).type(), entt::resolve<base_t>());
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
    auto type = entt::resolve<clazz_t>();
    int counter{};

    type.ctor([&counter](auto) {
        ++counter;
    });

    ASSERT_EQ(counter, 2);
    ASSERT_TRUE((type.ctor()));
    ASSERT_TRUE((type.ctor<const base_t &, int>()));
    ASSERT_TRUE((type.ctor<const derived_t &, double>()));
}

TEST_F(Meta, MetaTypeData) {
    auto type = entt::resolve<clazz_t>();
    int counter{};

    type.data([&counter](auto) {
        ++counter;
    });

    ASSERT_EQ(counter, 1);
    ASSERT_TRUE(type.data("value"_hs));
}

TEST_F(Meta, MetaTypeFunc) {
    auto type = entt::resolve<clazz_t>();
    int counter{};

    type.func([&counter](auto) {
        ++counter;
    });

    ASSERT_EQ(counter, 2);
    ASSERT_TRUE(type.func("member"_hs));
    ASSERT_TRUE(type.func("func"_hs));
}

TEST_F(Meta, MetaTypeConstruct) {
    auto any = entt::resolve<clazz_t>().construct(base_t{}, 42);

    ASSERT_TRUE(any);
    ASSERT_EQ(any.cast<clazz_t>().value, 42);
}

TEST_F(Meta, MetaTypeConstructNoArgs) {
    // this should work, no other tests required
    auto any = entt::resolve<clazz_t>().construct();

    ASSERT_TRUE(any);
}

TEST_F(Meta, MetaTypeConstructMetaAnyArgs) {
    auto any = entt::resolve<clazz_t>().construct(entt::meta_any{base_t{}}, entt::meta_any{42});

    ASSERT_TRUE(any);
    ASSERT_EQ(any.cast<clazz_t>().value, 42);
}

TEST_F(Meta, MetaTypeConstructInvalidArgs) {
    ASSERT_FALSE(entt::resolve<clazz_t>().construct(base_t{}, 'c'));
}

TEST_F(Meta, MetaTypeLessArgs) {
    ASSERT_FALSE(entt::resolve<clazz_t>().construct(base_t{}));
}

TEST_F(Meta, MetaTypeConstructCastAndConvert) {
    auto any = entt::resolve<clazz_t>().construct(derived_t{}, 42.);

    ASSERT_TRUE(any);
    ASSERT_EQ(any.cast<clazz_t>().value, 42);
}

TEST_F(Meta, MetaTypeDetach) {
    ASSERT_TRUE(entt::resolve_id("clazz"_hs));

    entt::resolve([](auto type) {
        if(type.id() == "clazz"_hs) {
            type.detach();
        }
    });

    ASSERT_FALSE(entt::resolve_id("clazz"_hs));
    ASSERT_EQ(entt::resolve<clazz_t>().id(), "clazz"_hs);
    ASSERT_EQ(entt::resolve<clazz_t>().prop(property_t::value).value().cast<int>(), 42);
    ASSERT_TRUE(entt::resolve<clazz_t>().data("value"_hs));

    entt::meta<clazz_t>().type("clazz"_hs);

    ASSERT_TRUE(entt::resolve_id("clazz"_hs));
}

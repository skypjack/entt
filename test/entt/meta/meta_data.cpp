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

struct derived_t: base_t {
    derived_t() {}
};

struct clazz_t {
    clazz_t()
        : i{0},
          j{1},
          base{}
    {}

    int i{0};
    const int j{1};
    base_t base{};
    inline static int h{2};
    inline static const int k{3};
};

struct setter_getter_t {
    setter_getter_t(): value{0} {}

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

    int value;
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
    void SetUp() override {
        using namespace entt::literals;

        entt::meta<double>()
            .type("double"_hs)
            .conv<int>();

        entt::meta<base_t>()
            .type("base"_hs)
            .dtor<&base_t::destroy>()
            .data<&base_t::value>("value"_hs);

        entt::meta<derived_t>()
            .type("derived"_hs)
            .base<base_t>()
            .dtor<&derived_t::destroy>();

        entt::meta<clazz_t>()
            .type("clazz"_hs)
            .data<&clazz_t::i, entt::as_ref_t>("i"_hs).prop(3, 0)
            .data<&clazz_t::i, entt::as_cref_t>("ci"_hs)
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

        base_t::counter = 0;
    }

    void TearDown() override {
        for(auto type: entt::resolve()) {
            type.reset();
        }
    }
};

TEST_F(MetaData, Functionalities) {
    using namespace entt::literals;

    auto data = entt::resolve<clazz_t>().data("i"_hs);
    clazz_t instance{};

    ASSERT_TRUE(data);
    ASSERT_EQ(data.parent(), entt::resolve("clazz"_hs));
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
    using namespace entt::literals;

    auto data = entt::resolve<clazz_t>().data("j"_hs);
    clazz_t instance{};

    ASSERT_TRUE(data);
    ASSERT_EQ(data.parent(), entt::resolve("clazz"_hs));
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
    using namespace entt::literals;

    auto data = entt::resolve<clazz_t>().data("h"_hs);

    ASSERT_TRUE(data);
    ASSERT_EQ(data.parent(), entt::resolve("clazz"_hs));
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
    using namespace entt::literals;

    auto data = entt::resolve<clazz_t>().data("k"_hs);

    ASSERT_TRUE(data);
    ASSERT_EQ(data.parent(), entt::resolve("clazz"_hs));
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
    using namespace entt::literals;

    entt::meta_any any{clazz_t{}};
    any.cast<clazz_t &>().i = 99;
    const auto value = entt::resolve<clazz_t>().data("i"_hs).get(any);

    ASSERT_TRUE(value);
    ASSERT_TRUE(static_cast<bool>(value.cast<int>()));
    ASSERT_EQ(value.cast<int>(), 99);
}

TEST_F(MetaData, GetInvalidArg) {
    using namespace entt::literals;

    auto instance = 0;
    ASSERT_FALSE(entt::resolve<clazz_t>().data("i"_hs).get(instance));
}

TEST_F(MetaData, SetMetaAnyArg) {
    using namespace entt::literals;

    entt::meta_any any{clazz_t{}};
    entt::meta_any value{42};

    ASSERT_EQ(any.cast<clazz_t>().i, 0);
    ASSERT_TRUE(entt::resolve<clazz_t>().data("i"_hs).set(any, value));
    ASSERT_EQ(any.cast<clazz_t>().i, 42);
}

TEST_F(MetaData, SetInvalidArg) {
    using namespace entt::literals;

    ASSERT_FALSE(entt::resolve<clazz_t>().data("i"_hs).set({}, 'c'));
}

TEST_F(MetaData, SetCast) {
    using namespace entt::literals;

    clazz_t instance{};

    ASSERT_EQ(base_t::counter, 0);
    ASSERT_TRUE(entt::resolve<clazz_t>().data("base"_hs).set(instance, derived_t{}));
    ASSERT_EQ(base_t::counter, 1);
}

TEST_F(MetaData, SetConvert) {
    using namespace entt::literals;

    clazz_t instance{};

    ASSERT_EQ(instance.i, 0);
    ASSERT_TRUE(entt::resolve<clazz_t>().data("i"_hs).set(instance, 3.));
    ASSERT_EQ(instance.i, 3);
}

TEST_F(MetaData, SetByRef) {
    using namespace entt::literals;

    entt::meta_any any{clazz_t{}};
    int value{42};

    ASSERT_EQ(any.cast<clazz_t>().i, 0);
    ASSERT_TRUE(entt::resolve<clazz_t>().data("i"_hs).set(any, entt::make_meta_any<int &>(value)));
    ASSERT_EQ(any.cast<clazz_t>().i, 42);

    value = 3;
    auto wrapper = entt::make_meta_any<int &>(value);

    ASSERT_TRUE(entt::resolve<clazz_t>().data("i"_hs).set(any, wrapper.as_ref()));
    ASSERT_EQ(any.cast<clazz_t>().i, 3);
}

TEST_F(MetaData, SetByConstRef) {
    using namespace entt::literals;

    entt::meta_any any{clazz_t{}};
    int value{42};

    ASSERT_EQ(any.cast<clazz_t>().i, 0);
    ASSERT_TRUE(entt::resolve<clazz_t>().data("i"_hs).set(any, entt::make_meta_any<const int &>(value)));
    ASSERT_EQ(any.cast<clazz_t>().i, 42);

    value = 3;
    auto wrapper = entt::make_meta_any<const int &>(value);

    ASSERT_TRUE(entt::resolve<clazz_t>().data("i"_hs).set(any, wrapper.as_ref()));
    ASSERT_EQ(any.cast<clazz_t>().i, 3);
}

TEST_F(MetaData, SetterGetterAsFreeFunctions) {
    using namespace entt::literals;

    auto data = entt::resolve<setter_getter_t>().data("x"_hs);
    setter_getter_t instance{};

    ASSERT_TRUE(data);
    ASSERT_EQ(data.parent(), entt::resolve("setter_getter"_hs));
    ASSERT_EQ(data.type(), entt::resolve<int>());
    ASSERT_EQ(data.id(), "x"_hs);
    ASSERT_FALSE(data.is_const());
    ASSERT_FALSE(data.is_static());
    ASSERT_EQ(data.get(instance).cast<int>(), 0);
    ASSERT_TRUE(data.set(instance, 42));
    ASSERT_EQ(data.get(instance).cast<int>(), 42);
}

TEST_F(MetaData, SetterGetterAsMemberFunctions) {
    using namespace entt::literals;

    auto data = entt::resolve<setter_getter_t>().data("y"_hs);
    setter_getter_t instance{};

    ASSERT_TRUE(data);
    ASSERT_EQ(data.parent(), entt::resolve("setter_getter"_hs));
    ASSERT_EQ(data.type(), entt::resolve<int>());
    ASSERT_EQ(data.id(), "y"_hs);
    ASSERT_FALSE(data.is_const());
    ASSERT_FALSE(data.is_static());
    ASSERT_EQ(data.get(instance).cast<int>(), 0);
    ASSERT_TRUE(data.set(instance, 42));
    ASSERT_EQ(data.get(instance).cast<int>(), 42);
}

TEST_F(MetaData, SetterGetterWithRefAsMemberFunctions) {
    using namespace entt::literals;

    auto data = entt::resolve<setter_getter_t>().data("w"_hs);
    setter_getter_t instance{};

    ASSERT_TRUE(data);
    ASSERT_EQ(data.parent(), entt::resolve("setter_getter"_hs));
    ASSERT_EQ(data.type(), entt::resolve<int>());
    ASSERT_EQ(data.id(), "w"_hs);
    ASSERT_FALSE(data.is_const());
    ASSERT_FALSE(data.is_static());
    ASSERT_EQ(data.get(instance).cast<int>(), 0);
    ASSERT_TRUE(data.set(instance, 42));
    ASSERT_EQ(data.get(instance).cast<int>(), 42);
}

TEST_F(MetaData, SetterGetterMixed) {
    using namespace entt::literals;

    auto data = entt::resolve<setter_getter_t>().data("z"_hs);
    setter_getter_t instance{};

    ASSERT_TRUE(data);
    ASSERT_EQ(data.parent(), entt::resolve("setter_getter"_hs));
    ASSERT_EQ(data.type(), entt::resolve<int>());
    ASSERT_EQ(data.id(), "z"_hs);
    ASSERT_FALSE(data.is_const());
    ASSERT_FALSE(data.is_static());
    ASSERT_EQ(data.get(instance).cast<int>(), 0);
    ASSERT_TRUE(data.set(instance, 42));
    ASSERT_EQ(data.get(instance).cast<int>(), 42);
}

TEST_F(MetaData, SetterGetterReadOnly) {
    using namespace entt::literals;

    auto data = entt::resolve<setter_getter_t>().data("z_ro"_hs);
    setter_getter_t instance{};

    ASSERT_TRUE(data);
    ASSERT_EQ(data.parent(), entt::resolve("setter_getter"_hs));
    ASSERT_EQ(data.type(), entt::resolve<int>());
    ASSERT_EQ(data.id(), "z_ro"_hs);
    ASSERT_TRUE(data.is_const());
    ASSERT_FALSE(data.is_static());
    ASSERT_EQ(data.get(instance).cast<int>(), 0);
    ASSERT_FALSE(data.set(instance, 42));
    ASSERT_EQ(data.get(instance).cast<int>(), 0);
}

TEST_F(MetaData, SetterGetterReadOnlyDataMember) {
    using namespace entt::literals;

    auto data = entt::resolve<setter_getter_t>().data("value"_hs);
    setter_getter_t instance{};

    ASSERT_TRUE(data);
    ASSERT_EQ(data.parent(), entt::resolve("setter_getter"_hs));
    ASSERT_EQ(data.type(), entt::resolve<int>());
    ASSERT_EQ(data.id(), "value"_hs);
    ASSERT_TRUE(data.is_const());
    ASSERT_FALSE(data.is_static());
    ASSERT_EQ(data.get(instance).cast<int>(), 0);
    ASSERT_FALSE(data.set(instance, 42));
    ASSERT_EQ(data.get(instance).cast<int>(), 0);
}

TEST_F(MetaData, ConstInstance) {
    using namespace entt::literals;

    clazz_t instance{};

    ASSERT_NE(entt::resolve<clazz_t>().data("i"_hs).get(instance).try_cast<int>(), nullptr);
    ASSERT_NE(entt::resolve<clazz_t>().data("i"_hs).get(instance).try_cast<const int>(), nullptr);
    ASSERT_EQ(entt::resolve<clazz_t>().data("i"_hs).get(std::as_const(instance)).try_cast<int>(), nullptr);
    // as_ref_t adapts to the constness of the passed object and returns const references in case
    ASSERT_NE(entt::resolve<clazz_t>().data("i"_hs).get(std::as_const(instance)).try_cast<const int>(), nullptr);

    ASSERT_TRUE(entt::resolve<clazz_t>().data("i"_hs).get(instance));
    ASSERT_TRUE(entt::resolve<clazz_t>().data("i"_hs).set(instance, 3));
    ASSERT_TRUE(entt::resolve<clazz_t>().data("i"_hs).get(std::as_const(instance)));
    ASSERT_FALSE(entt::resolve<clazz_t>().data("i"_hs).set(std::as_const(instance), 3));

    ASSERT_TRUE(entt::resolve<clazz_t>().data("ci"_hs).get(instance));
    ASSERT_TRUE(entt::resolve<clazz_t>().data("ci"_hs).set(instance, 3));
    ASSERT_TRUE(entt::resolve<clazz_t>().data("ci"_hs).get(std::as_const(instance)));
    ASSERT_FALSE(entt::resolve<clazz_t>().data("ci"_hs).set(std::as_const(instance), 3));

    ASSERT_TRUE(entt::resolve<clazz_t>().data("j"_hs).get(instance));
    ASSERT_FALSE(entt::resolve<clazz_t>().data("j"_hs).set(instance, 3));
    ASSERT_TRUE(entt::resolve<clazz_t>().data("j"_hs).get(std::as_const(instance)));
    ASSERT_FALSE(entt::resolve<clazz_t>().data("j"_hs).set(std::as_const(instance), 3));
}

TEST_F(MetaData, ArrayStatic) {
    using namespace entt::literals;

    auto data = entt::resolve<array_t>().data("global"_hs);

    ASSERT_TRUE(data);
    ASSERT_EQ(data.parent(), entt::resolve("array"_hs));
    ASSERT_EQ(data.type(), entt::resolve<int[3]>());
    ASSERT_EQ(data.id(), "global"_hs);
    ASSERT_FALSE(data.is_const());
    ASSERT_TRUE(data.is_static());
    ASSERT_TRUE(data.type().is_array());
    ASSERT_EQ(data.type().extent(), 3u);
    ASSERT_FALSE(data.get({}));
}

TEST_F(MetaData, Array) {
    using namespace entt::literals;

    auto data = entt::resolve<array_t>().data("local"_hs);
    array_t instance{};

    ASSERT_TRUE(data);
    ASSERT_EQ(data.parent(), entt::resolve("array"_hs));
    ASSERT_EQ(data.type(), entt::resolve<int[5]>());
    ASSERT_EQ(data.id(), "local"_hs);
    ASSERT_FALSE(data.is_const());
    ASSERT_FALSE(data.is_static());
    ASSERT_TRUE(data.type().is_array());
    ASSERT_EQ(data.type().extent(), 5u);
    ASSERT_FALSE(data.get(instance));
}

TEST_F(MetaData, AsVoid) {
    using namespace entt::literals;

    auto data = entt::resolve<clazz_t>().data("void"_hs);
    clazz_t instance{};

    ASSERT_TRUE(data.set(instance, 42));
    ASSERT_EQ(instance.i, 42);
    ASSERT_EQ(data.get(instance), entt::meta_any{std::in_place_type<void>});
}

TEST_F(MetaData, AsRef) {
    using namespace entt::literals;

    clazz_t instance{};
    auto data = entt::resolve<clazz_t>().data("i"_hs);

    ASSERT_EQ(instance.i, 0);
    ASSERT_EQ(data.type(), entt::resolve<int>());

    data.get(instance).cast<int &>() = 3;

    ASSERT_EQ(instance.i, 3);
}

TEST_F(MetaData, AsConstRef) {
    using namespace entt::literals;

    clazz_t instance{};
    auto data = entt::resolve<clazz_t>().data("ci"_hs);

    ASSERT_EQ(instance.i, 0);
    ASSERT_EQ(data.type(), entt::resolve<int>());
    ASSERT_DEATH(data.get(instance).cast<int &>() = 3, "");
    ASSERT_EQ(data.get(instance).cast<const int &>(), 0);
    ASSERT_EQ(data.get(instance).cast<int>(), 0);
    ASSERT_EQ(instance.i, 0);
}

TEST_F(MetaData, FromBase) {
    using namespace entt::literals;

    auto type = entt::resolve<derived_t>();
    derived_t instance{};

    ASSERT_TRUE(type.data("value"_hs));

    ASSERT_EQ(instance.value, 3);
    ASSERT_TRUE(type.data("value"_hs).set(instance, 42));
    ASSERT_EQ(instance.value, 42);
}

TEST_F(MetaData, ReRegistration) {
    using namespace entt::literals;

    SetUp();

    auto *node = entt::internal::meta_info<base_t>::resolve();
    auto type = entt::resolve<base_t>();

    ASSERT_NE(node->data, nullptr);
    ASSERT_EQ(node->data->next, nullptr);
    ASSERT_TRUE(type.data("value"_hs));

    entt::meta<base_t>().data<&base_t::value>("field"_hs);

    ASSERT_NE(node->data, nullptr);
    ASSERT_EQ(node->data->next, nullptr);
    ASSERT_FALSE(type.data("value"_hs));
    ASSERT_TRUE(type.data("field"_hs));
}

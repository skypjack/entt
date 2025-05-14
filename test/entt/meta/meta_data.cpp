#include <cstdlib>
#include <string>
#include <utility>
#include <gtest/gtest.h>
#include <entt/core/hashed_string.hpp>
#include <entt/core/type_traits.hpp>
#include <entt/locator/locator.hpp>
#include <entt/meta/context.hpp>
#include <entt/meta/factory.hpp>
#include <entt/meta/meta.hpp>
#include <entt/meta/node.hpp>
#include <entt/meta/policy.hpp>
#include <entt/meta/range.hpp>
#include <entt/meta/resolve.hpp>
#include "../../common/config.h"
#include "../../common/meta_traits.h"

struct base {
    virtual ~base() = default;

    static void destroy(base &) {
        ++counter;
    }

    inline static int counter = 0; // NOLINT
    int value{3};
};

struct derived: base {
    derived() = default;
};

struct clazz {
    operator int() const {
        return h;
    }

    int i{0};
    const int j{1}; // NOLINT
    base instance{};
    inline static int h{2};       // NOLINT
    inline static const int k{3}; // NOLINT
};

struct setter_getter {
    int setter(double val) {
        return value = static_cast<int>(val);
    }

    [[nodiscard]] int getter() const {
        return value;
    }

    int setter_with_ref(const int &val) {
        return value = val;
    }

    [[nodiscard]] const int &getter_with_ref() const {
        return value;
    }

    static int static_setter(setter_getter &type, int value) {
        return type.value = value;
    }

    static int static_getter(const setter_getter &type) {
        return type.value;
    }

    int value{0};
};

struct array {
    inline static int global[2]; // NOLINT
    int local[4];                // NOLINT
};

struct MetaData: ::testing::Test {
    void SetUp() override {
        using namespace entt::literals;

        entt::meta_factory<base>{}
            .type("base"_hs)
            .dtor<base::destroy>()
            .data<&base::value>("value"_hs);

        entt::meta_factory<derived>{}
            .type("derived"_hs)
            .base<base>()
            .dtor<derived::destroy>()
            .data<&base::value>("value_from_base"_hs);

        entt::meta_factory<clazz>{}
            .type("clazz"_hs)
            .data<&clazz::i, entt::as_ref_t>("i"_hs)
            .custom<char>('c')
            .traits(test::meta_traits::one | test::meta_traits::two | test::meta_traits::three)
            .data<&clazz::i, entt::as_cref_t>("ci"_hs)
            .data<&clazz::j>("j")
            .traits(test::meta_traits::one)
            .data<&clazz::h>("h"_hs, "hhh")
            .traits(test::meta_traits::two)
            .data<&clazz::k>("k"_hs)
            .traits(test::meta_traits::three)
            .data<'c'>("l"_hs)
            .data<&clazz::instance>("base"_hs)
            .data<&clazz::i, entt::as_void_t>("void"_hs)
            .conv<int>();

        entt::meta_factory<setter_getter>{}
            .type("setter_getter"_hs)
            .data<&setter_getter::static_setter, &setter_getter::static_getter>("x"_hs)
            .data<&setter_getter::setter, &setter_getter::getter>("y"_hs)
            .data<&setter_getter::static_setter, &setter_getter::getter>("z"_hs)
            .data<&setter_getter::setter_with_ref, &setter_getter::getter_with_ref>("w")
            .data<nullptr, &setter_getter::getter>("z_ro"_hs, "readonly")
            .data<nullptr, &setter_getter::value>("value"_hs);

        entt::meta_factory<array>{}
            .type("array"_hs)
            .data<&array::global>("global"_hs)
            .data<&array::local>("local"_hs);

        base::counter = 0;
    }

    void TearDown() override {
        entt::meta_reset();
    }
};

using MetaDataDeathTest = MetaData;

TEST_F(MetaData, SafeWhenEmpty) {
    const entt::meta_data data{};

    ASSERT_FALSE(data);
    ASSERT_EQ(data, entt::meta_data{});
    ASSERT_EQ(data.arity(), 0u);
    ASSERT_FALSE(data.is_const());
    ASSERT_FALSE(data.is_static());
    ASSERT_EQ(data.type(), entt::meta_type{});
    ASSERT_FALSE(data.set({}, 0));
    ASSERT_FALSE(data.get({}));
    ASSERT_EQ(data.arg(0u), entt::meta_type{});
    ASSERT_EQ(data.traits<test::meta_traits>(), test::meta_traits::none);
    ASSERT_EQ(static_cast<const char *>(data.custom()), nullptr);
}

TEST_F(MetaData, UserTraits) {
    using namespace entt::literals;

    ASSERT_EQ(entt::resolve<clazz>().data("ci"_hs).traits<test::meta_traits>(), test::meta_traits::none);
    ASSERT_EQ(entt::resolve<clazz>().data("base"_hs).traits<test::meta_traits>(), test::meta_traits::none);

    ASSERT_EQ(entt::resolve<clazz>().data("i"_hs).traits<test::meta_traits>(), test::meta_traits::one | test::meta_traits::two | test::meta_traits::three);
    ASSERT_EQ(entt::resolve<clazz>().data("j"_hs).traits<test::meta_traits>(), test::meta_traits::one);
    ASSERT_EQ(entt::resolve<clazz>().data("h"_hs).traits<test::meta_traits>(), test::meta_traits::two);
    ASSERT_EQ(entt::resolve<clazz>().data("k"_hs).traits<test::meta_traits>(), test::meta_traits::three);
}

ENTT_DEBUG_TEST_F(MetaDataDeathTest, UserTraits) {
    using namespace entt::literals;

    using traits_type = entt::internal::meta_traits;
    constexpr auto value = traits_type{static_cast<std::underlying_type_t<traits_type>>(traits_type::_user_defined_traits) + 1u};
    ASSERT_DEATH(entt::meta_factory<clazz>{}.data<&clazz::i>("j"_hs).traits(value), "");
}

TEST_F(MetaData, Custom) {
    using namespace entt::literals;

    ASSERT_EQ(*static_cast<const char *>(entt::resolve<clazz>().data("i"_hs).custom()), 'c');
    ASSERT_EQ(static_cast<const char &>(entt::resolve<clazz>().data("i"_hs).custom()), 'c');

    ASSERT_EQ(static_cast<const int *>(entt::resolve<clazz>().data("i"_hs).custom()), nullptr);
    ASSERT_EQ(static_cast<const int *>(entt::resolve<clazz>().data("j"_hs).custom()), nullptr);
}

ENTT_DEBUG_TEST_F(MetaDataDeathTest, Custom) {
    using namespace entt::literals;

    ASSERT_DEATH([[maybe_unused]] const int value = entt::resolve<clazz>().data("i"_hs).custom(), "");
    ASSERT_DEATH([[maybe_unused]] const char value = entt::resolve<clazz>().data("j"_hs).custom(), "");
}

TEST_F(MetaData, Name) {
    using namespace entt::literals;

    const entt::meta_type type = entt::resolve<clazz>();
    const entt::meta_type other = entt::resolve<setter_getter>();

    ASSERT_EQ(type.data("i"_hs).name(), nullptr);
    ASSERT_STREQ(type.data("j"_hs).name(), "j");
    ASSERT_STREQ(type.data("h"_hs).name(), "hhh");
    ASSERT_EQ(type.data("none"_hs).name(), nullptr);

    ASSERT_EQ(other.data("z"_hs).name(), nullptr);
    ASSERT_STREQ(other.data("w"_hs).name(), "w");
    ASSERT_STREQ(other.data("z_ro"_hs).name(), "readonly");
    ASSERT_EQ(other.data("none"_hs).name(), nullptr);
}

TEST_F(MetaData, Comparison) {
    using namespace entt::literals;

    auto data = entt::resolve<clazz>().data("i"_hs);

    ASSERT_TRUE(data);

    ASSERT_EQ(data, data);
    ASSERT_NE(data, entt::meta_data{});
    ASSERT_FALSE(data != data);
    ASSERT_TRUE(data == data);
}

TEST_F(MetaData, NonConst) {
    using namespace entt::literals;

    auto data = entt::resolve<clazz>().data("i"_hs);
    clazz instance{};

    ASSERT_TRUE(data);
    ASSERT_EQ(data.arity(), 1u);
    ASSERT_EQ(data.type(), entt::resolve<int>());
    ASSERT_EQ(data.arg(0u), entt::resolve<int>());
    ASSERT_FALSE(data.is_const());
    ASSERT_FALSE(data.is_static());
    ASSERT_EQ(data.get(instance).cast<int>(), 0);
    ASSERT_TRUE(data.set(instance, 1));
    ASSERT_EQ(data.get(instance).cast<int>(), 1);
}

TEST_F(MetaData, Const) {
    using namespace entt::literals;

    auto data = entt::resolve<clazz>().data("j"_hs);
    clazz instance{};

    ASSERT_TRUE(data);
    ASSERT_EQ(data.arity(), 1u);
    ASSERT_EQ(data.type(), entt::resolve<int>());
    ASSERT_EQ(data.arg(0u), entt::resolve<int>());
    ASSERT_TRUE(data.is_const());
    ASSERT_FALSE(data.is_static());
    ASSERT_EQ(data.get(instance).cast<int>(), 1);
    ASSERT_FALSE(data.set(instance, 1));
    ASSERT_EQ(data.get(instance).cast<int>(), 1);
}

TEST_F(MetaData, Static) {
    using namespace entt::literals;

    auto data = entt::resolve<clazz>().data("h"_hs);

    ASSERT_TRUE(data);
    ASSERT_EQ(data.arity(), 1u);
    ASSERT_EQ(data.type(), entt::resolve<int>());
    ASSERT_EQ(data.arg(0u), entt::resolve<int>());
    ASSERT_FALSE(data.is_const());
    ASSERT_TRUE(data.is_static());
    ASSERT_EQ(data.get({}).cast<int>(), 2);
    ASSERT_TRUE(data.set({}, 1));
    ASSERT_EQ(data.get({}).cast<int>(), 1);
}

TEST_F(MetaData, ConstStatic) {
    using namespace entt::literals;

    auto data = entt::resolve<clazz>().data("k"_hs);

    ASSERT_TRUE(data);
    ASSERT_EQ(data.arity(), 1u);
    ASSERT_EQ(data.type(), entt::resolve<int>());
    ASSERT_EQ(data.arg(0u), entt::resolve<int>());
    ASSERT_TRUE(data.is_const());
    ASSERT_TRUE(data.is_static());
    ASSERT_EQ(data.get({}).cast<int>(), 3);
    ASSERT_FALSE(data.set({}, 1));
    ASSERT_EQ(data.get({}).cast<int>(), 3);
}

TEST_F(MetaData, Literal) {
    using namespace entt::literals;

    auto data = entt::resolve<clazz>().data("l"_hs);

    ASSERT_TRUE(data);
    ASSERT_EQ(data.arity(), 1u);
    ASSERT_EQ(data.type(), entt::resolve<char>());
    ASSERT_EQ(data.arg(0u), entt::resolve<char>());
    ASSERT_TRUE(data.is_const());
    ASSERT_TRUE(data.is_static());
    ASSERT_EQ(data.get({}).cast<char>(), 'c');
    ASSERT_FALSE(data.set({}, 'a'));
    ASSERT_EQ(data.get({}).cast<char>(), 'c');
}

TEST_F(MetaData, GetMetaAnyArg) {
    using namespace entt::literals;

    entt::meta_any any{clazz{}};
    any.cast<clazz &>().i = 3;
    const auto value = entt::resolve<clazz>().data("i"_hs).get(any);

    ASSERT_TRUE(value);
    ASSERT_TRUE(static_cast<bool>(value.cast<int>()));
    ASSERT_EQ(value.cast<int>(), 3);
}

TEST_F(MetaData, GetInvalidArg) {
    using namespace entt::literals;

    auto instance = 0;
    ASSERT_FALSE(entt::resolve<clazz>().data("i"_hs).get(instance));
}

TEST_F(MetaData, SetMetaAnyArg) {
    using namespace entt::literals;

    entt::meta_any any{clazz{}};
    const entt::meta_any value{1};

    ASSERT_EQ(any.cast<clazz>().i, 0);
    ASSERT_TRUE(entt::resolve<clazz>().data("i"_hs).set(any, value));
    ASSERT_EQ(any.cast<clazz>().i, 1);
}

TEST_F(MetaData, SetInvalidArg) {
    using namespace entt::literals;

    ASSERT_FALSE(entt::resolve<clazz>().data("i"_hs).set({}, 'c'));
}

TEST_F(MetaData, SetCast) {
    using namespace entt::literals;

    clazz instance{};

    ASSERT_EQ(base::counter, 0);
    ASSERT_TRUE(entt::resolve<clazz>().data("base"_hs).set(instance, derived{}));
    ASSERT_EQ(base::counter, 1);
}

TEST_F(MetaData, SetConvert) {
    using namespace entt::literals;

    clazz instance{};
    clazz::h = 1;

    ASSERT_EQ(instance.i, 0);
    ASSERT_TRUE(entt::resolve<clazz>().data("i"_hs).set(instance, instance));
    ASSERT_EQ(instance.i, 1);
}

TEST_F(MetaData, SetByRef) {
    using namespace entt::literals;

    entt::meta_any any{clazz{}};
    int value{1};

    ASSERT_EQ(any.cast<clazz>().i, 0);
    ASSERT_TRUE(entt::resolve<clazz>().data("i"_hs).set(any, entt::forward_as_meta(value)));
    ASSERT_EQ(any.cast<clazz>().i, 1);

    value = 3;
    auto wrapper = entt::forward_as_meta(value);

    ASSERT_TRUE(entt::resolve<clazz>().data("i"_hs).set(any, wrapper.as_ref()));
    ASSERT_EQ(any.cast<clazz>().i, 3);
}

TEST_F(MetaData, SetByConstRef) {
    using namespace entt::literals;

    entt::meta_any any{clazz{}};
    int value{1};

    ASSERT_EQ(any.cast<clazz>().i, 0);
    ASSERT_TRUE(entt::resolve<clazz>().data("i"_hs).set(any, entt::forward_as_meta(std::as_const(value))));
    ASSERT_EQ(any.cast<clazz>().i, 1);

    value = 3;
    auto wrapper = entt::forward_as_meta(std::as_const(value));

    ASSERT_TRUE(entt::resolve<clazz>().data("i"_hs).set(any, wrapper.as_ref()));
    ASSERT_EQ(any.cast<clazz>().i, 3);
}

TEST_F(MetaData, SetterGetterAsFreeFunctions) {
    using namespace entt::literals;

    auto data = entt::resolve<setter_getter>().data("x"_hs);
    setter_getter instance{};

    ASSERT_TRUE(data);
    ASSERT_EQ(data.arity(), 1u);
    ASSERT_EQ(data.type(), entt::resolve<int>());
    ASSERT_EQ(data.arg(0u), entt::resolve<int>());
    ASSERT_FALSE(data.is_const());
    ASSERT_FALSE(data.is_static());
    ASSERT_EQ(data.get(instance).cast<int>(), 0);
    ASSERT_TRUE(data.set(instance, 1));
    ASSERT_EQ(data.get(instance).cast<int>(), 1);
}

TEST_F(MetaData, SetterGetterAsMemberFunctions) {
    using namespace entt::literals;

    auto data = entt::resolve<setter_getter>().data("y"_hs);
    setter_getter instance{};

    ASSERT_TRUE(data);
    ASSERT_EQ(data.arity(), 1u);
    ASSERT_EQ(data.type(), entt::resolve<int>());
    ASSERT_EQ(data.arg(0u), entt::resolve<double>());
    ASSERT_FALSE(data.is_const());
    ASSERT_FALSE(data.is_static());
    ASSERT_EQ(data.get(instance).cast<int>(), 0);
    ASSERT_TRUE(data.set(instance, 1.));
    ASSERT_EQ(data.get(instance).cast<int>(), 1);
    ASSERT_TRUE(data.set(instance, 3));
    ASSERT_EQ(data.get(instance).cast<int>(), 3);
}

TEST_F(MetaData, SetterGetterWithRefAsMemberFunctions) {
    using namespace entt::literals;

    auto data = entt::resolve<setter_getter>().data("w"_hs);
    setter_getter instance{};

    ASSERT_TRUE(data);
    ASSERT_EQ(data.arity(), 1u);
    ASSERT_EQ(data.type(), entt::resolve<int>());
    ASSERT_EQ(data.arg(0u), entt::resolve<int>());
    ASSERT_FALSE(data.is_const());
    ASSERT_FALSE(data.is_static());
    ASSERT_EQ(data.get(instance).cast<int>(), 0);
    ASSERT_TRUE(data.set(instance, 1));
    ASSERT_EQ(data.get(instance).cast<int>(), 1);
}

TEST_F(MetaData, SetterGetterMixed) {
    using namespace entt::literals;

    auto data = entt::resolve<setter_getter>().data("z"_hs);
    setter_getter instance{};

    ASSERT_TRUE(data);
    ASSERT_EQ(data.arity(), 1u);
    ASSERT_EQ(data.type(), entt::resolve<int>());
    ASSERT_EQ(data.arg(0u), entt::resolve<int>());
    ASSERT_FALSE(data.is_const());
    ASSERT_FALSE(data.is_static());
    ASSERT_EQ(data.get(instance).cast<int>(), 0);
    ASSERT_TRUE(data.set(instance, 1));
    ASSERT_EQ(data.get(instance).cast<int>(), 1);
}

TEST_F(MetaData, SetterGetterReadOnly) {
    using namespace entt::literals;

    auto data = entt::resolve<setter_getter>().data("z_ro"_hs);
    setter_getter instance{};

    ASSERT_TRUE(data);
    ASSERT_EQ(data.arity(), 0u);
    ASSERT_EQ(data.type(), entt::resolve<int>());
    ASSERT_EQ(data.arg(0u), entt::meta_type{});
    ASSERT_TRUE(data.is_const());
    ASSERT_FALSE(data.is_static());
    ASSERT_EQ(data.get(instance).cast<int>(), 0);
    ASSERT_FALSE(data.set(instance, 1));
    ASSERT_EQ(data.get(instance).cast<int>(), 0);
}

TEST_F(MetaData, SetterGetterReadOnlyDataMember) {
    using namespace entt::literals;

    auto data = entt::resolve<setter_getter>().data("value"_hs);
    setter_getter instance{};

    ASSERT_TRUE(data);
    ASSERT_EQ(data.arity(), 0u);
    ASSERT_EQ(data.type(), entt::resolve<int>());
    ASSERT_EQ(data.arg(0u), entt::meta_type{});
    ASSERT_TRUE(data.is_const());
    ASSERT_FALSE(data.is_static());
    ASSERT_EQ(data.get(instance).cast<int>(), 0);
    ASSERT_FALSE(data.set(instance, 1));
    ASSERT_EQ(data.get(instance).cast<int>(), 0);
}

TEST_F(MetaData, ConstInstance) {
    using namespace entt::literals;

    clazz instance{};

    ASSERT_NE(entt::resolve<clazz>().data("i"_hs).get(instance).try_cast<int>(), nullptr);
    ASSERT_NE(entt::resolve<clazz>().data("i"_hs).get(instance).try_cast<const int>(), nullptr);
    ASSERT_EQ(entt::resolve<clazz>().data("i"_hs).get(std::as_const(instance)).try_cast<int>(), nullptr);
    // as_ref_t adapts to the constness of the passed object and returns const references in case
    ASSERT_NE(entt::resolve<clazz>().data("i"_hs).get(std::as_const(instance)).try_cast<const int>(), nullptr);

    ASSERT_TRUE(entt::resolve<clazz>().data("i"_hs).get(instance));
    ASSERT_TRUE(entt::resolve<clazz>().data("i"_hs).set(instance, 3));
    ASSERT_TRUE(entt::resolve<clazz>().data("i"_hs).get(std::as_const(instance)));
    ASSERT_FALSE(entt::resolve<clazz>().data("i"_hs).set(std::as_const(instance), 3));

    ASSERT_TRUE(entt::resolve<clazz>().data("ci"_hs).get(instance));
    ASSERT_TRUE(entt::resolve<clazz>().data("ci"_hs).set(instance, 3));
    ASSERT_TRUE(entt::resolve<clazz>().data("ci"_hs).get(std::as_const(instance)));
    ASSERT_FALSE(entt::resolve<clazz>().data("ci"_hs).set(std::as_const(instance), 3));

    ASSERT_TRUE(entt::resolve<clazz>().data("j"_hs).get(instance));
    ASSERT_FALSE(entt::resolve<clazz>().data("j"_hs).set(instance, 3));
    ASSERT_TRUE(entt::resolve<clazz>().data("j"_hs).get(std::as_const(instance)));
    ASSERT_FALSE(entt::resolve<clazz>().data("j"_hs).set(std::as_const(instance), 3));
}

TEST_F(MetaData, ArrayStatic) {
    using namespace entt::literals;

    auto data = entt::resolve<array>().data("global"_hs);

    ASSERT_TRUE(data);
    ASSERT_EQ(data.arity(), 1u);
    // NOLINTBEGIN(*-avoid-c-arrays)
    ASSERT_EQ(data.type(), entt::resolve<int[2]>());
    ASSERT_EQ(data.arg(0u), entt::resolve<int[2]>());
    // NOLINTEND(*-avoid-c-arrays)
    ASSERT_FALSE(data.is_const());
    ASSERT_TRUE(data.is_static());
    ASSERT_TRUE(data.type().is_array());
    ASSERT_FALSE(data.get({}));
}

TEST_F(MetaData, Array) {
    using namespace entt::literals;

    auto data = entt::resolve<array>().data("local"_hs);
    array instance{};

    ASSERT_TRUE(data);
    ASSERT_EQ(data.arity(), 1u);
    // NOLINTBEGIN(*-avoid-c-arrays)
    ASSERT_EQ(data.type(), entt::resolve<int[4]>());
    ASSERT_EQ(data.arg(0u), entt::resolve<int[4]>());
    // NOLINTEND(*-avoid-c-arrays)
    ASSERT_FALSE(data.is_const());
    ASSERT_FALSE(data.is_static());
    ASSERT_TRUE(data.type().is_array());
    ASSERT_FALSE(data.get(instance));
}

TEST_F(MetaData, AsVoid) {
    using namespace entt::literals;

    auto data = entt::resolve<clazz>().data("void"_hs);
    clazz instance{};

    ASSERT_TRUE(data);
    ASSERT_EQ(data.arity(), 1u);
    ASSERT_EQ(data.type(), entt::resolve<int>());
    ASSERT_EQ(data.arg(0u), entt::resolve<int>());
    ASSERT_TRUE(data.set(instance, 1));
    ASSERT_EQ(instance.i, 1);
    ASSERT_EQ(data.get(instance), entt::meta_any{std::in_place_type<void>});
}

TEST_F(MetaData, AsRef) {
    using namespace entt::literals;

    clazz instance{};
    auto data = entt::resolve<clazz>().data("i"_hs);

    ASSERT_TRUE(data);
    ASSERT_EQ(data.arity(), 1u);
    ASSERT_EQ(data.type(), entt::resolve<int>());
    ASSERT_EQ(data.arg(0u), entt::resolve<int>());
    ASSERT_EQ(instance.i, 0);

    data.get(instance).cast<int &>() = 3;

    ASSERT_EQ(instance.i, 3);
}

TEST_F(MetaData, AsConstRef) {
    using namespace entt::literals;

    clazz instance{};
    auto data = entt::resolve<clazz>().data("ci"_hs);

    ASSERT_EQ(instance.i, 0);
    ASSERT_EQ(data.arity(), 1u);
    ASSERT_EQ(data.type(), entt::resolve<int>());
    ASSERT_EQ(data.arg(0u), entt::resolve<int>());
    ASSERT_EQ(data.get(instance).cast<const int &>(), 0);
    ASSERT_EQ(data.get(instance).cast<int>(), 0);
    ASSERT_EQ(instance.i, 0);
}

ENTT_DEBUG_TEST_F(MetaDataDeathTest, AsConstRef) {
    using namespace entt::literals;

    clazz instance{};
    auto data = entt::resolve<clazz>().data("ci"_hs);

    ASSERT_DEATH(data.get(instance).cast<int &>() = 3, "");
}

TEST_F(MetaData, SetGetBaseData) {
    using namespace entt::literals;

    auto type = entt::resolve<derived>();
    derived instance{};

    ASSERT_TRUE(type.data("value"_hs));

    ASSERT_EQ(instance.value, 3);
    ASSERT_TRUE(type.data("value"_hs).set(instance, 1));
    ASSERT_EQ(type.data("value"_hs).get(instance).cast<int>(), 1);
    ASSERT_EQ(instance.value, 1);
}

TEST_F(MetaData, SetGetFromBase) {
    using namespace entt::literals;

    auto type = entt::resolve<derived>();
    derived instance{};

    ASSERT_TRUE(type.data("value_from_base"_hs));

    ASSERT_EQ(instance.value, 3);
    ASSERT_TRUE(type.data("value_from_base"_hs).set(instance, 1));
    ASSERT_EQ(type.data("value_from_base"_hs).get(instance).cast<int>(), 1);
    ASSERT_EQ(instance.value, 1);
}

TEST_F(MetaData, ReRegistration) {
    using namespace entt::literals;

    SetUp();

    auto &&node = entt::internal::resolve<base>(entt::internal::meta_context::from(entt::locator<entt::meta_ctx>::value_or()));
    auto type = entt::resolve<base>();

    ASSERT_TRUE(node.details);
    ASSERT_FALSE(node.details->data.empty());
    ASSERT_EQ(node.details->data.size(), 1u);
    ASSERT_TRUE(type.data("value"_hs));

    entt::meta_factory<base>{}.data<&base::value>("field"_hs);

    ASSERT_TRUE(node.details);
    ASSERT_EQ(node.details->data.size(), 2u);
    ASSERT_TRUE(type.data("value"_hs));
    ASSERT_TRUE(type.data("field"_hs));

    entt::meta_factory<base>{}
        .data<&base::value>("field"_hs)
        .traits(test::meta_traits::one)
        .custom<int>(3)
        // this should not overwrite traits and custom data
        .data<&base::value>("field"_hs);

    ASSERT_EQ(type.data("field"_hs).traits<test::meta_traits>(), test::meta_traits::one);
    ASSERT_NE(static_cast<const int *>(type.data("field"_hs).custom()), nullptr);
}

TEST_F(MetaData, CollisionAndReuse) {
    using namespace entt::literals;

    ASSERT_TRUE(entt::resolve<clazz>().data("j"_hs));
    ASSERT_FALSE(entt::resolve<clazz>().data("cj"_hs));
    ASSERT_TRUE(entt::resolve<clazz>().data("j"_hs).is_const());

    ASSERT_NO_THROW(entt::meta_factory<clazz>{}.data<&clazz::i>("j"_hs));
    ASSERT_NO_THROW(entt::meta_factory<clazz>{}.data<&clazz::j>("cj"_hs));

    ASSERT_TRUE(entt::resolve<clazz>().data("j"_hs));
    ASSERT_TRUE(entt::resolve<clazz>().data("cj"_hs));
    ASSERT_FALSE(entt::resolve<clazz>().data("j"_hs).is_const());
}

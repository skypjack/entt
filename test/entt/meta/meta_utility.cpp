#include <utility>
#include <gtest/gtest.h>
#include <entt/core/type_traits.hpp>
#include <entt/meta/meta.hpp>
#include <entt/meta/resolve.hpp>
#include <entt/meta/utility.hpp>
#include "../common/config.h"

struct clazz {
    void setter(int v) {
        member = v;
    }

    int getter() const {
        return member;
    }

    static void static_setter(clazz &instance, int v) {
        instance.member = v;
    }

    static int static_getter(const clazz &instance) {
        return instance.member;
    }

    static void reset_value() {
        value = 0;
    }

    static int get_value() {
        return value;
    }

    static clazz factory(int v) {
        clazz instance{};
        instance.member = v;
        return instance;
    }

    int member{};
    const int cmember{};
    inline static int value{};
    inline static const int cvalue{};
    inline static int arr[3u]{};
};

struct dummy {};

struct MetaUtility: ::testing::Test {
    void SetUp() override {
        clazz::value = 0;
    }
};

using MetaUtilityDeathTest = MetaUtility;

TEST_F(MetaUtility, MetaDispatch) {
    int value = 42;

    auto as_void = entt::meta_dispatch<entt::as_void_t>(value);
    auto as_ref = entt::meta_dispatch<entt::as_ref_t>(value);
    auto as_cref = entt::meta_dispatch<entt::as_cref_t>(value);
    auto as_is = entt::meta_dispatch(value);

    ASSERT_EQ(as_void.type(), entt::resolve<void>());
    ASSERT_EQ(as_ref.type(), entt::resolve<int>());
    ASSERT_EQ(as_cref.type(), entt::resolve<int>());
    ASSERT_EQ(as_is.type(), entt::resolve<int>());

    ASSERT_NE(as_is.try_cast<int>(), nullptr);
    ASSERT_NE(as_ref.try_cast<int>(), nullptr);
    ASSERT_EQ(as_cref.try_cast<int>(), nullptr);
    ASSERT_NE(as_cref.try_cast<const int>(), nullptr);

    ASSERT_EQ(as_is.cast<int>(), 42);
    ASSERT_EQ(as_ref.cast<int>(), 42);
    ASSERT_EQ(as_cref.cast<int>(), 42);
}

TEST_F(MetaUtility, MetaDispatchMetaAny) {
    entt::meta_any any{42};

    auto from_any = entt::meta_dispatch(any);
    auto from_const_any = entt::meta_dispatch(std::as_const(any));

    ASSERT_EQ(from_any.type(), entt::resolve<int>());
    ASSERT_EQ(from_const_any.type(), entt::resolve<int>());

    ASSERT_NE(from_any.try_cast<int>(), nullptr);
    ASSERT_NE(from_const_any.try_cast<int>(), nullptr);

    ASSERT_EQ(from_any.cast<int>(), 42);
    ASSERT_EQ(from_const_any.cast<int>(), 42);
}

TEST_F(MetaUtility, MetaDispatchMetaAnyAsRef) {
    entt::meta_any any{42};

    auto from_any = entt::meta_dispatch(any.as_ref());
    auto from_const_any = entt::meta_dispatch(std::as_const(any).as_ref());

    ASSERT_EQ(from_any.type(), entt::resolve<int>());
    ASSERT_EQ(from_const_any.type(), entt::resolve<int>());

    ASSERT_NE(from_any.try_cast<int>(), nullptr);
    ASSERT_EQ(from_const_any.try_cast<int>(), nullptr);
    ASSERT_NE(from_const_any.try_cast<const int>(), nullptr);

    ASSERT_EQ(from_any.cast<int>(), 42);
    ASSERT_EQ(from_const_any.cast<int>(), 42);
}

TEST_F(MetaUtility, MetaArg) {
    ASSERT_EQ((entt::meta_arg<entt::type_list<int, char>>(0u)), entt::resolve<int>());
    ASSERT_EQ((entt::meta_arg<entt::type_list<int, char>>(1u)), entt::resolve<char>());
}

ENTT_DEBUG_TEST_F(MetaUtilityDeathTest, MetaArg) {
    ASSERT_DEATH([[maybe_unused]] auto type = entt::meta_arg<entt::type_list<>>(0u), "");
    ASSERT_DEATH([[maybe_unused]] auto type = entt::meta_arg<entt::type_list<int>>(3u), "");
}

TEST_F(MetaUtility, MetaSetter) {
    const int invalid{};
    clazz instance{};

    ASSERT_FALSE((entt::meta_setter<clazz, &clazz::static_setter>(instance, instance)));
    ASSERT_FALSE((entt::meta_setter<clazz, &clazz::static_setter>(std::as_const(instance), 42)));
    ASSERT_FALSE((entt::meta_setter<clazz, &clazz::static_setter>(invalid, 42)));
    ASSERT_TRUE((entt::meta_setter<clazz, &clazz::static_setter>(instance, 42)));
    ASSERT_EQ(instance.member, 42);

    ASSERT_FALSE((entt::meta_setter<clazz, &clazz::setter>(instance, instance)));
    ASSERT_FALSE((entt::meta_setter<clazz, &clazz::setter>(std::as_const(instance), 3)));
    ASSERT_FALSE((entt::meta_setter<clazz, &clazz::setter>(invalid, 3)));
    ASSERT_TRUE((entt::meta_setter<clazz, &clazz::setter>(instance, 3)));
    ASSERT_EQ(instance.member, 3);

    ASSERT_FALSE((entt::meta_setter<clazz, &clazz::member>(instance, instance)));
    ASSERT_FALSE((entt::meta_setter<clazz, &clazz::member>(invalid, 99)));
    ASSERT_TRUE((entt::meta_setter<clazz, &clazz::member>(instance, 99)));
    ASSERT_EQ(instance.member, 99);

    ASSERT_FALSE((entt::meta_setter<clazz, &clazz::cmember>(instance, 99)));
    ASSERT_FALSE((entt::meta_setter<clazz, &clazz::cmember>(invalid, 99)));
    ASSERT_EQ(instance.cmember, 0);

    ASSERT_FALSE((entt::meta_setter<clazz, &clazz::value>(instance, instance)));
    ASSERT_TRUE((entt::meta_setter<clazz, &clazz::value>(invalid, 1)));
    ASSERT_TRUE((entt::meta_setter<clazz, &clazz::value>(instance, 2)));
    ASSERT_EQ(clazz::value, 2);

    ASSERT_FALSE((entt::meta_setter<clazz, &clazz::cvalue>(instance, 1)));
    ASSERT_FALSE((entt::meta_setter<clazz, &clazz::cvalue>(invalid, 1)));
    ASSERT_EQ(clazz::cvalue, 0);
}

TEST_F(MetaUtility, MetaGetter) {
    const int invalid{};
    clazz instance{};

    ASSERT_FALSE((entt::meta_getter<clazz, &clazz::static_getter>(invalid)));
    ASSERT_EQ((entt::meta_getter<clazz, &clazz::static_getter>(instance)).cast<int>(), 0);

    ASSERT_FALSE((entt::meta_getter<clazz, &clazz::getter>(invalid)));
    ASSERT_EQ((entt::meta_getter<clazz, &clazz::getter>(instance)).cast<int>(), 0);

    ASSERT_FALSE((entt::meta_getter<clazz, &clazz::member>(invalid)));
    ASSERT_EQ((entt::meta_getter<clazz, &clazz::member>(instance)).cast<int>(), 0);
    ASSERT_EQ((entt::meta_getter<clazz, &clazz::member>(std::as_const(instance))).cast<int>(), 0);

    ASSERT_FALSE((entt::meta_getter<clazz, &clazz::cmember>(invalid)));
    ASSERT_EQ((entt::meta_getter<clazz, &clazz::cmember>(instance)).cast<int>(), 0);
    ASSERT_EQ((entt::meta_getter<clazz, &clazz::cmember>(std::as_const(instance))).cast<int>(), 0);

    ASSERT_FALSE((entt::meta_getter<clazz, &clazz::arr>(invalid)));
    ASSERT_FALSE((entt::meta_getter<clazz, &clazz::arr>(instance)));

    ASSERT_EQ((entt::meta_getter<clazz, &clazz::value>(invalid)).cast<int>(), 0);
    ASSERT_EQ((entt::meta_getter<clazz, &clazz::value>(instance)).cast<int>(), 0);

    ASSERT_EQ((entt::meta_getter<clazz, &clazz::cvalue>(invalid)).cast<int>(), 0);
    ASSERT_EQ((entt::meta_getter<clazz, &clazz::cvalue>(instance)).cast<int>(), 0);

    ASSERT_EQ((entt::meta_getter<clazz, 42>(invalid)).cast<int>(), 42);
    ASSERT_EQ((entt::meta_getter<clazz, 42>(instance)).cast<int>(), 42);
}

TEST_F(MetaUtility, MetaInvokeWithCandidate) {
    entt::meta_any args[2u]{clazz{}, 42};
    args[0u].cast<clazz &>().value = 99;

    ASSERT_FALSE((entt::meta_invoke<clazz>({}, &clazz::setter, nullptr)));
    ASSERT_FALSE((entt::meta_invoke<clazz>({}, &clazz::getter, nullptr)));

    ASSERT_TRUE((entt::meta_invoke<clazz>(args[0u], &clazz::setter, args + 1u)));
    ASSERT_FALSE((entt::meta_invoke<clazz>(args[0u], &clazz::setter, args)));
    ASSERT_EQ((entt::meta_invoke<clazz>(args[0u], &clazz::getter, nullptr)).cast<int>(), 42);
    ASSERT_FALSE((entt::meta_invoke<clazz>(args[1u], &clazz::getter, nullptr)));

    ASSERT_EQ((entt::meta_invoke<clazz>({}, &clazz::get_value, nullptr)).cast<int>(), 99);
    ASSERT_TRUE((entt::meta_invoke<clazz>({}, &clazz::reset_value, nullptr)));
    ASSERT_EQ(args[0u].cast<clazz &>().value, 0);

    const auto setter = [](int &value) { value = 3; };
    const auto getter = [](int value) { return value * 2; };

    ASSERT_TRUE(entt::meta_invoke<dummy>({}, setter, args + 1u));
    ASSERT_EQ(entt::meta_invoke<dummy>({}, getter, args + 1u).cast<int>(), 6);
}

TEST_F(MetaUtility, MetaInvoke) {
    entt::meta_any args[2u]{clazz{}, 42};
    args[0u].cast<clazz &>().value = 99;

    ASSERT_FALSE((entt::meta_invoke<clazz, &clazz::setter>({}, nullptr)));
    ASSERT_FALSE((entt::meta_invoke<clazz, &clazz::getter>({}, nullptr)));

    ASSERT_TRUE((entt::meta_invoke<clazz, &clazz::setter>(args[0u], args + 1u)));
    ASSERT_FALSE((entt::meta_invoke<clazz, &clazz::setter>(args[0u], args)));
    ASSERT_EQ((entt::meta_invoke<clazz, &clazz::getter>(args[0u], nullptr)).cast<int>(), 42);
    ASSERT_FALSE((entt::meta_invoke<clazz, &clazz::getter>(args[1u], nullptr)));

    ASSERT_EQ((entt::meta_invoke<clazz, &clazz::get_value>({}, nullptr)).cast<int>(), 99);
    ASSERT_TRUE((entt::meta_invoke<clazz, &clazz::reset_value>({}, nullptr)));
    ASSERT_EQ(args[0u].cast<clazz &>().value, 0);
}

TEST_F(MetaUtility, MetaConstructArgsOnly) {
    entt::meta_any args[2u]{clazz{}, 42};
    const auto any = entt::meta_construct<clazz, int>(args + 1u);

    ASSERT_TRUE(any);
    ASSERT_FALSE((entt::meta_construct<clazz, int>(args)));
    ASSERT_EQ(any.cast<const clazz &>().member, 42);
}

TEST_F(MetaUtility, MetaConstructWithCandidate) {
    entt::meta_any args[2u]{clazz{}, 42};
    const auto any = entt::meta_construct<clazz>(&clazz::factory, args + 1u);

    ASSERT_TRUE(any);
    ASSERT_FALSE((entt::meta_construct<clazz>(&clazz::factory, args)));
    ASSERT_EQ(any.cast<const clazz &>().member, 42);

    ASSERT_EQ(args[0u].cast<const clazz &>().member, 0);
    ASSERT_TRUE((entt::meta_construct<clazz>(&clazz::static_setter, args)));
    ASSERT_EQ(args[0u].cast<const clazz &>().member, 42);

    const auto setter = [](int &value) { value = 3; };
    const auto builder = [](int value) { return value * 2; };

    ASSERT_TRUE(entt::meta_construct<dummy>(setter, args + 1u));
    ASSERT_EQ(entt::meta_construct<dummy>(builder, args + 1u).cast<int>(), 6);
}

TEST_F(MetaUtility, MetaConstruct) {
    entt::meta_any args[2u]{clazz{}, 42};
    const auto any = entt::meta_construct<clazz, &clazz::factory>(args + 1u);

    ASSERT_TRUE(any);
    ASSERT_FALSE((entt::meta_construct<clazz, &clazz::factory>(args)));
    ASSERT_EQ(any.cast<const clazz &>().member, 42);

    ASSERT_EQ(args[0u].cast<const clazz &>().member, 0);
    ASSERT_TRUE((entt::meta_construct<clazz, &clazz::static_setter>(args)));
    ASSERT_EQ(args[0u].cast<const clazz &>().member, 42);
}

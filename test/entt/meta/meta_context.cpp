#include <iterator>
#include <unordered_map>
#include <utility>
#include <vector>
#include <gtest/gtest.h>
#include <entt/core/hashed_string.hpp>
#include <entt/core/type_info.hpp>
#include <entt/meta/container.hpp>
#include <entt/meta/context.hpp>
#include <entt/meta/factory.hpp>
#include <entt/meta/meta.hpp>
#include <entt/meta/pointer.hpp>
#include <entt/meta/resolve.hpp>
#include <entt/meta/template.hpp>
#include "../../common/empty.h"

struct base {
    base() = default;

    base(char cv)
        : value{cv} {}

    [[nodiscard]] char get() const {
        return value;
    }

    char value;
};

struct clazz: base {
    clazz()
        : base{} {}

    clazz(int iv)
        : base{},
          value{iv} {}

    clazz(char cv, int iv) // NOLINT
        : base{cv},
          value{iv} {}

    [[nodiscard]] int func(int iv) {
        return (value = iv);
    }

    [[nodiscard]] int cfunc(int) const {
        return value;
    }

    static void move_to_bucket(const clazz &instance) {
        bucket = instance.value;
    }

    int value{};
    inline static int bucket{}; // NOLINT
};

struct argument {
    argument(int val)
        : value{val} {}

    [[nodiscard]] int get() const {
        return value;
    }

    [[nodiscard]] int get_mul() const {
        return value * 2;
    }

private:
    int value{};
};

template<typename...>
struct template_clazz {};

class MetaContext: public ::testing::Test {
    static void init_global_context() {
        using namespace entt::literals;

        entt::meta_factory<int>{}
            .data<global_marker>("marker"_hs);

        entt::meta_factory<argument>{}
            .conv<&argument::get>();

        entt::meta_factory<clazz>{}
            .type("foo"_hs)
            .custom<int>(3)
            .ctor<int>()
            .data<&clazz::value>("value"_hs)
            .data<&clazz::value>("rw"_hs)
            .func<&clazz::func>("func"_hs);

        entt::meta_factory<template_clazz<int>>{}
            .type("template"_hs);
    }

    void init_local_context() {
        using namespace entt::literals;

        entt::meta_factory<int>{context}
            .data<local_marker>("marker"_hs);

        entt::meta_factory<test::empty>{context}
            .type("quux"_hs);

        entt::meta_factory<argument>{context}
            .conv<&argument::get_mul>();

        entt::meta_factory<base>{context}
            .data<&base::value>("char"_hs)
            .func<&base::get>("get"_hs);

        entt::meta_factory<clazz>{context}
            .type("bar"_hs)
            .custom<char>('c')
            .base<base>()
            .ctor<char, int>()
            .dtor<&clazz::move_to_bucket>()
            .data<nullptr, &clazz::value>("value"_hs)
            .data<&clazz::value>("rw"_hs)
            .func<&clazz::cfunc>("func"_hs);

        entt::meta_factory<template_clazz<int, char>>{context}
            .type("template"_hs);
    }

public:
    void SetUp() override {
        init_global_context();
        init_local_context();

        clazz::bucket = bucket_value;
    }

    void TearDown() override {
        entt::meta_reset(context);
        entt::meta_reset();
    }

    [[nodiscard]] const entt::meta_ctx &ctx() const noexcept {
        return context;
    }

protected:
    static constexpr int global_marker = 1;
    static constexpr int local_marker = 4;
    static constexpr int bucket_value = 2;

private:
    entt::meta_ctx context{};
};

TEST_F(MetaContext, Resolve) {
    using namespace entt::literals;

    ASSERT_TRUE(entt::resolve<clazz>());
    ASSERT_TRUE(entt::resolve<clazz>(ctx()));

    ASSERT_TRUE(entt::resolve<test::empty>());
    ASSERT_TRUE(entt::resolve<test::empty>(ctx()));

    ASSERT_TRUE(entt::resolve(entt::type_id<clazz>()));
    ASSERT_TRUE(entt::resolve(ctx(), entt::type_id<clazz>()));

    ASSERT_FALSE(entt::resolve(entt::type_id<test::empty>()));
    ASSERT_TRUE(entt::resolve(ctx(), entt::type_id<test::empty>()));

    ASSERT_TRUE(entt::resolve("foo"_hs));
    ASSERT_FALSE(entt::resolve(ctx(), "foo"_hs));

    ASSERT_FALSE(entt::resolve("bar"_hs));
    ASSERT_TRUE(entt::resolve(ctx(), "bar"_hs));

    ASSERT_FALSE(entt::resolve("quux"_hs));
    ASSERT_TRUE(entt::resolve(ctx(), "quux"_hs));

    ASSERT_EQ((std::distance(entt::resolve().cbegin(), entt::resolve().cend())), 4);
    ASSERT_EQ((std::distance(entt::resolve(ctx()).cbegin(), entt::resolve(ctx()).cend())), 6);
}

TEST_F(MetaContext, MetaType) {
    using namespace entt::literals;

    const auto global = entt::resolve<clazz>();
    const auto local = entt::resolve<clazz>(ctx());

    ASSERT_TRUE(global);
    ASSERT_TRUE(local);

    ASSERT_NE(global, local);

    ASSERT_EQ(global, entt::resolve("foo"_hs));
    ASSERT_EQ(local, entt::resolve(ctx(), "bar"_hs));

    ASSERT_EQ(global.id(), "foo"_hs);
    ASSERT_EQ(local.id(), "bar"_hs);

    clazz instance{'c', 8};
    const argument value{2};

    ASSERT_NE(instance.value, value.get());
    ASSERT_EQ(global.invoke("func"_hs, instance, value).cast<int>(), value.get());
    ASSERT_EQ(instance.value, value.get());

    ASSERT_NE(instance.value, value.get_mul());
    ASSERT_EQ(local.invoke("func"_hs, instance, value).cast<int>(), instance.value);
    ASSERT_NE(instance.value, value.get_mul());

    ASSERT_FALSE(global.invoke("get"_hs, instance));
    ASSERT_EQ(local.invoke("get"_hs, instance).cast<char>(), 'c');
}

TEST_F(MetaContext, MetaBase) {
    const auto global = entt::resolve<clazz>();
    const auto local = entt::resolve<clazz>(ctx());

    ASSERT_EQ((std::distance(global.base().cbegin(), global.base().cend())), 0);
    ASSERT_EQ((std::distance(local.base().cbegin(), local.base().cend())), 1);

    ASSERT_EQ(local.base().cbegin()->second.info(), entt::type_id<base>());

    ASSERT_FALSE(entt::resolve(entt::type_id<base>()));
    ASSERT_TRUE(entt::resolve(ctx(), entt::type_id<base>()));
}

TEST_F(MetaContext, MetaData) {
    using namespace entt::literals;

    const auto global = entt::resolve<clazz>();
    const auto local = entt::resolve<clazz>(ctx());

    ASSERT_TRUE(global.data("value"_hs));
    ASSERT_TRUE(local.data("value"_hs));

    ASSERT_FALSE(global.data("value"_hs).is_const());
    ASSERT_TRUE(local.data("value"_hs).is_const());

    ASSERT_EQ(global.data("value"_hs).type().data("marker"_hs).get({}).cast<int>(), global_marker);
    ASSERT_EQ(local.data("value"_hs).type().data("marker"_hs).get({}).cast<int>(), local_marker);

    ASSERT_EQ(global.data("rw"_hs).arg(0u).data("marker"_hs).get({}).cast<int>(), global_marker);
    ASSERT_EQ(local.data("rw"_hs).arg(0u).data("marker"_hs).get({}).cast<int>(), local_marker);

    clazz instance{'c', 8};
    const argument value{2};

    ASSERT_NE(instance.value, value.get());
    ASSERT_TRUE(global.data("rw"_hs).set(instance, value));
    ASSERT_EQ(instance.value, value.get());

    ASSERT_NE(instance.value, value.get_mul());
    ASSERT_TRUE(local.data("rw"_hs).set(instance, value));
    ASSERT_EQ(instance.value, value.get_mul());

    ASSERT_FALSE(global.data("char"_hs));
    ASSERT_EQ(local.data("char"_hs).get(instance).cast<char>(), 'c');
    ASSERT_TRUE(local.data("char"_hs).set(instance, 'x'));
    ASSERT_EQ(instance.base::value, 'x');
}

TEST_F(MetaContext, MetaFunc) {
    using namespace entt::literals;

    const auto global = entt::resolve<clazz>();
    const auto local = entt::resolve<clazz>(ctx());

    ASSERT_TRUE(global.func("func"_hs));
    ASSERT_TRUE(local.func("func"_hs));

    ASSERT_FALSE(global.func("func"_hs).is_const());
    ASSERT_TRUE(local.func("func"_hs).is_const());

    ASSERT_EQ(global.func("func"_hs).arg(0u).data("marker"_hs).get({}).cast<int>(), global_marker);
    ASSERT_EQ(local.func("func"_hs).arg(0u).data("marker"_hs).get({}).cast<int>(), local_marker);

    ASSERT_EQ(global.func("func"_hs).ret().data("marker"_hs).get({}).cast<int>(), global_marker);
    ASSERT_EQ(local.func("func"_hs).ret().data("marker"_hs).get({}).cast<int>(), local_marker);

    clazz instance{'c', 8};
    const argument value{2};

    ASSERT_NE(instance.value, value.get());
    ASSERT_EQ(global.func("func"_hs).invoke(instance, value).cast<int>(), value.get());
    ASSERT_EQ(instance.value, value.get());

    ASSERT_NE(instance.value, value.get_mul());
    ASSERT_EQ(local.func("func"_hs).invoke(instance, value).cast<int>(), instance.value);
    ASSERT_NE(instance.value, value.get_mul());

    ASSERT_FALSE(global.func("get"_hs));
    ASSERT_EQ(local.func("get"_hs).invoke(instance).cast<char>(), 'c');
}

TEST_F(MetaContext, MetaCtor) {
    const auto global = entt::resolve<clazz>();
    const auto local = entt::resolve<clazz>(ctx());

    auto any = global.construct();
    auto other = local.construct();

    ASSERT_TRUE(any);
    ASSERT_TRUE(other);

    ASSERT_EQ(any.cast<const clazz &>().value, 0);
    ASSERT_EQ(other.cast<const clazz &>().value, 0);

    const argument argument{2};

    any = global.construct(argument);
    other = local.construct(argument);

    ASSERT_TRUE(any);
    ASSERT_FALSE(other);
    ASSERT_EQ(any.cast<const clazz &>().value, 2);

    any = global.construct('c', argument);
    other = local.construct('c', argument);

    ASSERT_FALSE(any);
    ASSERT_TRUE(other);
    ASSERT_EQ(other.cast<const clazz &>().value, 4);
}

TEST_F(MetaContext, MetaConv) {
    argument value{2};

    auto global = entt::forward_as_meta(value);
    auto local = entt::forward_as_meta(ctx(), value);

    ASSERT_TRUE(global.allow_cast<int>());
    ASSERT_TRUE(local.allow_cast<int>());

    ASSERT_EQ(global.cast<int>(), value.get());
    ASSERT_EQ(local.cast<int>(), value.get_mul());
}

TEST_F(MetaContext, MetaDtor) {
    auto global = entt::resolve<clazz>().construct();
    auto local = entt::resolve<clazz>(ctx()).construct();

    ASSERT_EQ(clazz::bucket, bucket_value);

    global.reset();

    ASSERT_EQ(clazz::bucket, bucket_value);

    local.reset();

    ASSERT_NE(clazz::bucket, bucket_value);
}

TEST_F(MetaContext, MetaCustom) {
    using namespace entt::literals;

    const auto global = entt::resolve<clazz>();
    const auto local = entt::resolve<clazz>(ctx());

    ASSERT_NE(static_cast<const int *>(global.custom()), nullptr);
    ASSERT_NE(static_cast<const char *>(local.custom()), nullptr);

    ASSERT_EQ(static_cast<int>(global.custom()), 3);
    ASSERT_EQ(static_cast<char>(local.custom()), 'c');
}

TEST_F(MetaContext, MetaTemplate) {
    using namespace entt::literals;

    const auto global = entt::resolve("template"_hs);
    const auto local = entt::resolve(ctx(), "template"_hs);

    ASSERT_TRUE(global.is_template_specialization());
    ASSERT_TRUE(local.is_template_specialization());

    ASSERT_EQ(global.template_arity(), 1u);
    ASSERT_EQ(local.template_arity(), 2u);

    ASSERT_EQ(global.template_arg(0u), entt::resolve<int>());
    ASSERT_EQ(local.template_arg(0u), entt::resolve<int>(ctx()));
    ASSERT_EQ(local.template_arg(1u), entt::resolve<char>(ctx()));

    ASSERT_EQ(global.template_arg(0u).data("marker"_hs).get({}).cast<int>(), global_marker);
    ASSERT_EQ(local.template_arg(0u).data("marker"_hs).get({}).cast<int>(), local_marker);
}

TEST_F(MetaContext, MetaPointer) {
    using namespace entt::literals;

    int value = 2;

    const entt::meta_any global{&value};
    const entt::meta_any local{ctx(), &value};

    ASSERT_TRUE(global.type().is_pointer());
    ASSERT_TRUE(local.type().is_pointer());

    ASSERT_TRUE(global.type().is_pointer_like());
    ASSERT_TRUE(local.type().is_pointer_like());

    ASSERT_EQ((*global).type().data("marker"_hs).get({}).cast<int>(), global_marker);
    ASSERT_EQ((*local).type().data("marker"_hs).get({}).cast<int>(), local_marker);
}

TEST_F(MetaContext, MetaAssociativeContainer) {
    using namespace entt::literals;

    std::unordered_map<int, int> map{{{0, 0}}};

    auto global = entt::forward_as_meta(map).as_associative_container();
    auto local = entt::forward_as_meta(ctx(), map).as_associative_container();

    ASSERT_TRUE(global);
    ASSERT_TRUE(local);

    ASSERT_EQ(global.size(), 1u);
    ASSERT_EQ(local.size(), 1u);

    ASSERT_EQ(global.key_type().data("marker"_hs).get({}).cast<int>(), global_marker);
    ASSERT_EQ(local.key_type().data("marker"_hs).get({}).cast<int>(), local_marker);

    ASSERT_EQ(global.mapped_type().data("marker"_hs).get({}).cast<int>(), global_marker);
    ASSERT_EQ(local.mapped_type().data("marker"_hs).get({}).cast<int>(), local_marker);

    ASSERT_EQ((*global.begin()).first.type().data("marker"_hs).get({}).cast<int>(), global_marker);
    ASSERT_EQ((*local.begin()).first.type().data("marker"_hs).get({}).cast<int>(), local_marker);

    ASSERT_EQ((*global.begin()).second.type().data("marker"_hs).get({}).cast<int>(), global_marker);
    ASSERT_EQ((*local.begin()).second.type().data("marker"_hs).get({}).cast<int>(), local_marker);
}

TEST_F(MetaContext, MetaSequenceContainer) {
    using namespace entt::literals;

    std::vector<int> vec{0};

    auto global = entt::forward_as_meta(vec).as_sequence_container();
    auto local = entt::forward_as_meta(ctx(), vec).as_sequence_container();

    ASSERT_TRUE(global);
    ASSERT_TRUE(local);

    ASSERT_EQ(global.size(), 1u);
    ASSERT_EQ(local.size(), 1u);

    ASSERT_EQ(global.value_type().data("marker"_hs).get({}).cast<int>(), global_marker);
    ASSERT_EQ(local.value_type().data("marker"_hs).get({}).cast<int>(), local_marker);

    ASSERT_EQ((*global.begin()).type().data("marker"_hs).get({}).cast<int>(), global_marker);
    ASSERT_EQ((*local.begin()).type().data("marker"_hs).get({}).cast<int>(), local_marker);
}

TEST_F(MetaContext, MetaAny) {
    using namespace entt::literals;

    const entt::meta_any global{2};
    const entt::meta_any ctx_value{ctx(), 2};
    const entt::meta_any in_place{ctx(), std::in_place_type<int>, 2};
    entt::meta_any two_step_local{entt::meta_ctx_arg, ctx()};

    ASSERT_TRUE(global);
    ASSERT_TRUE(ctx_value);
    ASSERT_TRUE(in_place);
    ASSERT_FALSE(two_step_local);

    two_step_local = 2;

    ASSERT_TRUE(two_step_local);

    ASSERT_EQ(global.type().data("marker"_hs).get({}).cast<int>(), global_marker);
    ASSERT_EQ(ctx_value.type().data("marker"_hs).get({}).cast<int>(), local_marker);
    ASSERT_EQ(in_place.type().data("marker"_hs).get({}).cast<int>(), local_marker);
    ASSERT_EQ(two_step_local.type().data("marker"_hs).get({}).cast<int>(), local_marker);
}

TEST_F(MetaContext, MetaHandle) {
    using namespace entt::literals;

    int value = 2;

    entt::meta_handle global{value};
    entt::meta_handle ctx_value{ctx(), value};
    entt::meta_handle two_step_local{entt::meta_ctx_arg, ctx()};

    ASSERT_TRUE(global);
    ASSERT_TRUE(ctx_value);
    ASSERT_FALSE(two_step_local);

    two_step_local->emplace<int &>(value);

    ASSERT_TRUE(two_step_local);

    ASSERT_EQ(global->type().data("marker"_hs).get({}).cast<int>(), global_marker);
    ASSERT_EQ(ctx_value->type().data("marker"_hs).get({}).cast<int>(), local_marker);
    ASSERT_EQ(two_step_local->type().data("marker"_hs).get({}).cast<int>(), local_marker);
}

TEST_F(MetaContext, ForwardAsMeta) {
    using namespace entt::literals;

    const auto global = entt::forward_as_meta(2);
    const auto local = entt::forward_as_meta(ctx(), 2);

    ASSERT_TRUE(global);
    ASSERT_TRUE(local);

    ASSERT_EQ(global.type().data("marker"_hs).get({}).cast<int>(), global_marker);
    ASSERT_EQ(local.type().data("marker"_hs).get({}).cast<int>(), local_marker);
}

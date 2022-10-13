#include <unordered_map>
#include <vector>
#include <gtest/gtest.h>
#include <entt/core/hashed_string.hpp>
#include <entt/meta/container.hpp>
#include <entt/meta/context.hpp>
#include <entt/meta/factory.hpp>
#include <entt/meta/pointer.hpp>
#include <entt/meta/template.hpp>

struct base {};

struct clazz: base {
    clazz() = default;

    clazz(int)
        : clazz{} {}

    clazz(char, int)
        : clazz{} {}

    int func(int v) {
        return (value = v);
    }

    int cfunc(int v) const {
        return v;
    }

    static void move_to_bucket(const clazz &instance) {
        bucket = instance.value;
    }

    int value{};
    static inline int bucket{};
};

struct local_only {};

struct argument {
    argument(int val)
        : value{val} {}

    int get() const {
        return value;
    }

    int get_mul() const {
        return value * 2;
    }

private:
    int value{};
};

template<typename...>
struct template_clazz {};

class MetaContext: public ::testing::Test {
    void init_global_context() {
        using namespace entt::literals;

        entt::meta<int>()
            .data<global_marker>("marker"_hs);

        entt::meta<argument>()
            .conv<&argument::get>();

        entt::meta<clazz>()
            .type("foo"_hs)
            .prop("prop"_hs, prop_value)
            .ctor<int>()
            .data<&clazz::value>("value"_hs)
            .data<&clazz::value>("rw"_hs)
            .func<&clazz::func>("func"_hs);

        entt::meta<template_clazz<int>>()
            .type("template"_hs);
    }

    void init_local_context() {
        using namespace entt::literals;

        entt::meta<int>(context)
            .data<local_marker>("marker"_hs);

        entt::meta<local_only>(context)
            .type("quux"_hs);

        entt::meta<argument>(context)
            .conv<&argument::get_mul>();

        entt::meta<clazz>(context)
            .type("bar"_hs)
            .prop("prop"_hs, prop_value)
            .base<base>()
            .ctor<char, int>()
            .dtor<&clazz::move_to_bucket>()
            .data<nullptr, &clazz::value>("value"_hs)
            .data<&clazz::value>("rw"_hs)
            .func<&clazz::cfunc>("func"_hs);

        entt::meta<template_clazz<int, char>>(context)
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

protected:
    static constexpr int global_marker = 1;
    static constexpr int local_marker = 42;
    static constexpr int bucket_value = 99;
    static constexpr int prop_value = 3;
    entt::meta_ctx context{};
};

TEST_F(MetaContext, Resolve) {
    using namespace entt::literals;

    ASSERT_TRUE(entt::resolve<clazz>());
    ASSERT_TRUE(entt::resolve<clazz>(context));

    ASSERT_TRUE(entt::resolve<local_only>());
    ASSERT_TRUE(entt::resolve<local_only>(context));

    ASSERT_TRUE(entt::resolve(entt::type_id<clazz>()));
    ASSERT_TRUE(entt::resolve(context, entt::type_id<clazz>()));

    ASSERT_FALSE(entt::resolve(entt::type_id<local_only>()));
    ASSERT_TRUE(entt::resolve(context, entt::type_id<local_only>()));

    ASSERT_TRUE(entt::resolve("foo"_hs));
    ASSERT_FALSE(entt::resolve(context, "foo"_hs));

    ASSERT_FALSE(entt::resolve("bar"_hs));
    ASSERT_TRUE(entt::resolve(context, "bar"_hs));

    ASSERT_FALSE(entt::resolve("quux"_hs));
    ASSERT_TRUE(entt::resolve(context, "quux"_hs));

    ASSERT_EQ((std::distance(entt::resolve().cbegin(), entt::resolve().cend())), 4);
    ASSERT_EQ((std::distance(entt::resolve(context).cbegin(), entt::resolve(context).cend())), 5);
}

TEST_F(MetaContext, MetaType) {
    using namespace entt::literals;

    const auto global = entt::resolve<clazz>();
    const auto local = entt::resolve<clazz>(context);

    ASSERT_TRUE(global);
    ASSERT_TRUE(local);

    ASSERT_NE(global, local);

    ASSERT_EQ(global, entt::resolve("foo"_hs));
    ASSERT_EQ(local, entt::resolve(context, "bar"_hs));

    ASSERT_EQ(global.id(), "foo"_hs);
    ASSERT_EQ(local.id(), "bar"_hs);
}

TEST_F(MetaContext, MetaBase) {
    const auto global = entt::resolve<clazz>();
    const auto local = entt::resolve<clazz>(context);

    ASSERT_EQ((std::distance(global.base().cbegin(), global.base().cend())), 0);
    ASSERT_EQ((std::distance(local.base().cbegin(), local.base().cend())), 1);

    ASSERT_EQ(local.base().cbegin()->second.info(), entt::type_id<base>());

    ASSERT_FALSE(entt::resolve(entt::type_id<base>()));
    ASSERT_FALSE(entt::resolve(context, entt::type_id<base>()));
}

TEST_F(MetaContext, MetaData) {
    using namespace entt::literals;

    const auto global = entt::resolve<clazz>().data("value"_hs);
    const auto local = entt::resolve<clazz>(context).data("value"_hs);

    ASSERT_TRUE(global);
    ASSERT_TRUE(local);

    ASSERT_FALSE(global.is_const());
    ASSERT_TRUE(local.is_const());

    ASSERT_EQ(global.type().data("marker"_hs).get({}).cast<int>(), global_marker);
    ASSERT_EQ(local.type().data("marker"_hs).get({}).cast<int>(), local_marker);

    const auto grw = entt::resolve<clazz>().data("rw"_hs);
    const auto lrw = entt::resolve<clazz>(context).data("rw"_hs);

    ASSERT_EQ(grw.arg(0u).data("marker"_hs).get({}).cast<int>(), global_marker);
    ASSERT_EQ(lrw.arg(0u).data("marker"_hs).get({}).cast<int>(), local_marker);

    clazz instance{};
    const argument value{2};

    grw.set(instance, value);

    ASSERT_EQ(instance.value, value.get());

    lrw.set(entt::meta_handle{context, instance}, entt::meta_any{context, value});

    ASSERT_EQ(instance.value, value.get_mul());
}

TEST_F(MetaContext, MetaFunc) {
    using namespace entt::literals;

    const auto global = entt::resolve<clazz>().func("func"_hs);
    const auto local = entt::resolve<clazz>(context).func("func"_hs);

    ASSERT_TRUE(global);
    ASSERT_TRUE(local);

    ASSERT_FALSE(global.is_const());
    ASSERT_TRUE(local.is_const());

    ASSERT_EQ(global.arg(0u).data("marker"_hs).get({}).cast<int>(), global_marker);
    ASSERT_EQ(local.arg(0u).data("marker"_hs).get({}).cast<int>(), local_marker);

    ASSERT_EQ(global.ret().data("marker"_hs).get({}).cast<int>(), global_marker);
    ASSERT_EQ(local.ret().data("marker"_hs).get({}).cast<int>(), local_marker);

    clazz instance{};
    const argument value{2};

    ASSERT_NE(instance.value, value.get());
    ASSERT_EQ(global.invoke(instance, value).cast<int>(), value.get());
    ASSERT_EQ(instance.value, value.get());

    ASSERT_NE(instance.value, value.get_mul());
    ASSERT_EQ(local.invoke(entt::meta_handle{context, instance}, entt::meta_any{context, value}).cast<int>(), value.get_mul());
    ASSERT_NE(instance.value, value.get_mul());
}

TEST_F(MetaContext, MetaCtor) {
    const auto global = entt::resolve<clazz>();
    const auto local = entt::resolve<clazz>(context);

    ASSERT_TRUE(global.construct());
    ASSERT_TRUE(local.construct());

    ASSERT_TRUE(global.construct(0));
    ASSERT_FALSE(local.construct(0));

    ASSERT_FALSE(global.construct('c', 0));
    ASSERT_TRUE(local.construct('c', 0));
}

TEST_F(MetaContext, MetaConv) {
    argument value{2};

    auto global = entt::forward_as_meta(value);
    auto local = entt::forward_as_meta(context, value);

    ASSERT_TRUE(global.allow_cast<int>());
    ASSERT_TRUE(local.allow_cast<int>());

    ASSERT_EQ(global.cast<int>(), value.get());
    ASSERT_EQ(local.cast<int>(), value.get_mul());
}

TEST_F(MetaContext, MetaDtor) {
    auto global = entt::resolve<clazz>().construct();
    auto local = entt::resolve<clazz>(context).construct();

    ASSERT_EQ(clazz::bucket, bucket_value);

    global.reset();

    ASSERT_EQ(clazz::bucket, bucket_value);

    local.reset();

    ASSERT_NE(clazz::bucket, bucket_value);
}

TEST_F(MetaContext, MetaProp) {
    using namespace entt::literals;

    const auto global = entt::resolve<clazz>().prop("prop"_hs);
    const auto local = entt::resolve<clazz>(context).prop("prop"_hs);

    ASSERT_TRUE(global);
    ASSERT_TRUE(local);

    ASSERT_EQ(global.value().type(), entt::resolve<int>());
    ASSERT_EQ(local.value().type(), entt::resolve<int>(context));

    ASSERT_EQ(global.value().cast<int>(), prop_value);
    ASSERT_EQ(local.value().cast<int>(), prop_value);

    ASSERT_EQ(global.value().type().data("marker"_hs).get({}).cast<int>(), global_marker);
    ASSERT_EQ(local.value().type().data("marker"_hs).get({}).cast<int>(), local_marker);
}

TEST_F(MetaContext, MetaTemplate) {
    using namespace entt::literals;

    const auto global = entt::resolve("template"_hs);
    const auto local = entt::resolve(context, "template"_hs);

    ASSERT_TRUE(global.is_template_specialization());
    ASSERT_TRUE(local.is_template_specialization());

    ASSERT_EQ(global.template_arity(), 1u);
    ASSERT_EQ(local.template_arity(), 2u);

    ASSERT_EQ(global.template_arg(0u), entt::resolve<int>());
    ASSERT_EQ(local.template_arg(0u), entt::resolve<int>(context));
    ASSERT_EQ(local.template_arg(1u), entt::resolve<char>(context));

    ASSERT_EQ(global.template_arg(0u).data("marker"_hs).get({}).cast<int>(), global_marker);
    ASSERT_EQ(local.template_arg(0u).data("marker"_hs).get({}).cast<int>(), local_marker);
}

TEST_F(MetaContext, MetaPointer) {
    using namespace entt::literals;

    int value = 42;

    const entt::meta_any global{&value};
    const entt::meta_any local{context, &value};

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
    auto local = entt::forward_as_meta(context, map).as_associative_container();

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
    auto local = entt::forward_as_meta(context, vec).as_sequence_container();

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

    entt::meta_any global{42};
    entt::meta_any ctx_value{context, 42};
    entt::meta_any in_place{context, std::in_place_type<int>, 42};
    entt::meta_any two_step_local{entt::meta_ctx_arg, context};

    ASSERT_TRUE(global);
    ASSERT_TRUE(ctx_value);
    ASSERT_TRUE(in_place);
    ASSERT_FALSE(two_step_local);

    two_step_local = 42;

    ASSERT_TRUE(two_step_local);

    ASSERT_EQ(global.type().data("marker"_hs).get({}).cast<int>(), global_marker);
    ASSERT_EQ(ctx_value.type().data("marker"_hs).get({}).cast<int>(), local_marker);
    ASSERT_EQ(in_place.type().data("marker"_hs).get({}).cast<int>(), local_marker);
    ASSERT_EQ(two_step_local.type().data("marker"_hs).get({}).cast<int>(), local_marker);
}

TEST_F(MetaContext, MetaHandle) {
    // TODO
}

TEST_F(MetaContext, ForwardAsMeta) {
    using namespace entt::literals;

    const auto global = entt::forward_as_meta(42);
    const auto local = entt::forward_as_meta(context, 42);

    ASSERT_TRUE(global);
    ASSERT_TRUE(local);

    ASSERT_EQ(global.type().data("marker"_hs).get({}).cast<int>(), global_marker);
    ASSERT_EQ(local.type().data("marker"_hs).get({}).cast<int>(), local_marker);
}

TEST_F(MetaContext, ContextMix) {
    // TODO
}

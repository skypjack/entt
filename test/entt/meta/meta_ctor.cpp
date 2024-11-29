#include <utility>
#include <gtest/gtest.h>
#include <entt/core/hashed_string.hpp>
#include <entt/core/utility.hpp>
#include <entt/entity/registry.hpp>
#include <entt/locator/locator.hpp>
#include <entt/meta/context.hpp>
#include <entt/meta/factory.hpp>
#include <entt/meta/meta.hpp>
#include <entt/meta/policy.hpp>
#include <entt/meta/resolve.hpp>

struct base {
    char value{'c'};
};

struct derived: base {
    derived()
        : base{} {}
};

struct clazz {
    clazz(const base &other, int &iv)
        : clazz{iv, other.value} {}

    clazz(const int &iv, char cv)
        : i{iv}, c{cv} {}

    operator int() const {
        return i;
    }

    static clazz factory(int value) {
        return {value, 'c'};
    }

    static clazz factory(base other, int value, int mul) {
        return {value * mul, other.value};
    }

    int i{};
    char c{};
};

double double_factory() {
    return 1.;
}

struct MetaCtor: ::testing::Test {
    void SetUp() override {
        using namespace entt::literals;

        entt::meta_factory<double>{}
            .type("double"_hs)
            .ctor<double_factory>();

        entt::meta_factory<derived>{}
            .type("derived"_hs)
            .base<base>();

        entt::meta_factory<clazz>{}
            .type("clazz"_hs)
            .ctor<&entt::registry::emplace_or_replace<clazz, const int &, const char &>, entt::as_ref_t>()
            .ctor<const base &, int &>()
            .ctor<const int &, char>()
            .ctor<entt::overload<clazz(int)>(clazz::factory)>()
            .ctor<entt::overload<clazz(base, int, int)>(clazz::factory)>()
            .conv<int>();
    }

    void TearDown() override {
        entt::meta_reset();
    }
};

TEST_F(MetaCtor, Ctor) {
    auto any = entt::resolve<clazz>().construct(1, 'c');

    ASSERT_TRUE(any);
    ASSERT_EQ(any.cast<clazz>().i, 1);
    ASSERT_EQ(any.cast<clazz>().c, 'c');
}

TEST_F(MetaCtor, Func) {
    auto any = entt::resolve<clazz>().construct(1);

    ASSERT_TRUE(any);
    ASSERT_EQ(any.cast<clazz>().i, 1);
    ASSERT_EQ(any.cast<clazz>().c, 'c');
}

TEST_F(MetaCtor, MetaAnyArgs) {
    auto any = entt::resolve<clazz>().construct(entt::meta_any{1}, entt::meta_any{'c'});

    ASSERT_TRUE(any);
    ASSERT_EQ(any.cast<clazz>().i, 1);
    ASSERT_EQ(any.cast<clazz>().c, 'c');
}

TEST_F(MetaCtor, InvalidArgs) {
    ASSERT_FALSE(entt::resolve<clazz>().construct(entt::meta_any{}, derived{}));
}

TEST_F(MetaCtor, CastAndConvert) {
    auto any = entt::resolve<clazz>().construct(derived{}, clazz{1, 'd'});

    ASSERT_TRUE(any);
    ASSERT_EQ(any.cast<clazz>().i, 1);
    ASSERT_EQ(any.cast<clazz>().c, 'c');
}

TEST_F(MetaCtor, ArithmeticConversion) {
    auto any = entt::resolve<clazz>().construct(true, 4.2);

    ASSERT_TRUE(any);
    ASSERT_EQ(any.cast<clazz>().i, 1);
    ASSERT_EQ(any.cast<clazz>().c, char{4});
}

TEST_F(MetaCtor, ConstNonConstRefArgs) {
    int ivalue = 1;
    const char cvalue = 'c';
    auto any = entt::resolve<clazz>().construct(entt::forward_as_meta(ivalue), entt::forward_as_meta(cvalue));

    ASSERT_TRUE(any);
    ASSERT_EQ(any.cast<clazz>().i, 1);
    ASSERT_EQ(any.cast<clazz>().c, 'c');
}

TEST_F(MetaCtor, WrongConstness) {
    int value = 1;
    auto any = entt::resolve<clazz>().construct(derived{}, entt::forward_as_meta(value));
    auto other = entt::resolve<clazz>().construct(derived{}, entt::forward_as_meta(std::as_const(value)));

    ASSERT_TRUE(any);
    ASSERT_FALSE(other);
    ASSERT_EQ(any.cast<clazz>().i, 1);
    ASSERT_EQ(any.cast<clazz>().c, 'c');
}

TEST_F(MetaCtor, FuncMetaAnyArgs) {
    auto any = entt::resolve<clazz>().construct(entt::meta_any{1});

    ASSERT_TRUE(any);
    ASSERT_EQ(any.cast<clazz>().i, 1);
    ASSERT_EQ(any.cast<clazz>().c, 'c');
}

TEST_F(MetaCtor, FuncCastAndConvert) {
    auto any = entt::resolve<clazz>().construct(derived{}, 3., clazz{3, 'd'});

    ASSERT_TRUE(any);
    ASSERT_EQ(any.cast<clazz>().i, 9);
    ASSERT_EQ(any.cast<clazz>().c, 'c');
}

TEST_F(MetaCtor, FuncArithmeticConversion) {
    auto any = entt::resolve<clazz>().construct(4.2);

    ASSERT_TRUE(any);
    ASSERT_EQ(any.cast<clazz>().i, 4);
    ASSERT_EQ(any.cast<clazz>().c, 'c');
}

TEST_F(MetaCtor, FuncConstNonConstRefArgs) {
    int ivalue = 1;
    auto any = entt::resolve<clazz>().construct(entt::forward_as_meta(ivalue));
    auto other = entt::resolve<clazz>().construct(entt::forward_as_meta(std::as_const(ivalue)));

    ASSERT_TRUE(any);
    ASSERT_TRUE(other);
    ASSERT_EQ(any.cast<clazz>().i, 1);
    ASSERT_EQ(other.cast<clazz>().i, 1);
}

TEST_F(MetaCtor, ExternalMemberFunction) {
    entt::registry registry;
    const auto entity = registry.create();

    ASSERT_FALSE(registry.all_of<clazz>(entity));

    const auto any = entt::resolve<clazz>().construct(entt::forward_as_meta(registry), entity, 3, 'c');

    ASSERT_TRUE(any);
    ASSERT_TRUE(registry.all_of<clazz>(entity));
    ASSERT_EQ(registry.get<clazz>(entity).i, 3);
    ASSERT_EQ(registry.get<clazz>(entity).c, 'c');
}

TEST_F(MetaCtor, OverrideImplicitlyGeneratedDefaultConstructor) {
    auto type = entt::resolve<double>();
    auto any = type.construct();

    ASSERT_TRUE(any);
    ASSERT_EQ(any.type(), entt::resolve<double>());
    ASSERT_EQ(any.cast<double>(), 1.);
}

TEST_F(MetaCtor, NonDefaultConstructibleType) {
    auto type = entt::resolve<clazz>();
    // no implicitly generated default constructor
    ASSERT_FALSE(type.construct());
}

TEST_F(MetaCtor, ReRegistration) {
    SetUp();

    auto &&node = entt::internal::resolve<double>(entt::internal::meta_context::from(entt::locator<entt::meta_ctx>::value_or()));

    ASSERT_TRUE(node.details);
    ASSERT_FALSE(node.details->ctor.empty());
    // implicitly generated default constructor is not cleared
    ASSERT_NE(node.default_constructor, nullptr);
}

#include <algorithm>
#include <cstdint>
#include <map>
#include <memory>
#include <utility>
#include <vector>
#include <gtest/gtest.h>
#include <entt/core/hashed_string.hpp>
#include <entt/core/type_info.hpp>
#include <entt/core/utility.hpp>
#include <entt/locator/locator.hpp>
#include <entt/meta/container.hpp>
#include <entt/meta/context.hpp>
#include <entt/meta/factory.hpp>
#include <entt/meta/meta.hpp>
#include <entt/meta/pointer.hpp>
#include <entt/meta/range.hpp>
#include <entt/meta/resolve.hpp>
#include <entt/meta/template.hpp>
#include "../../common/config.h"
#include "../../common/meta_traits.h"

template<typename Type>
void set(Type &elem, Type value) {
    elem = value;
}

template<typename Type>
Type get(Type &elem) {
    return elem;
}

struct base {
    char value{'c'};
};

struct derived: base {
    derived()
        : base{} {}
};

struct abstract {
    virtual ~abstract() = default;

    virtual void func(int) {}
    void base_only(int) {}
};

struct concrete: base, abstract {
    void func(int val) override {
        abstract::func(val);
        value = val;
    }

    int value{3};
};

struct clazz {
    clazz() = default;

    clazz(const base &, int val)
        : value{val} {}

    void member() {}
    static void func() {}

    [[nodiscard]] operator int() const {
        return value;
    }

    int value{};
};

struct overloaded_func {
    [[nodiscard]] int f(const base &, int first, int second) {
        return f(first, second);
    }

    [[nodiscard]] int f(int first, const int second) {
        value = first;
        return second * second;
    }

    [[nodiscard]] int f(int val) {
        return 2 * std::as_const(*this).f(val);
    }

    [[nodiscard]] int f(int val) const {
        return val * value;
    }

    [[nodiscard]] float f(int first, const float second) {
        value = first;
        return second + second;
    }

    int value{};
};

struct from_void_callback {
    from_void_callback(bool &ref)
        : cb{&ref} {}

    from_void_callback(const from_void_callback &) = delete;
    from_void_callback &operator=(const from_void_callback &) = delete;

    ~from_void_callback() {
        *cb = !*cb;
    }

private:
    bool *cb;
};

enum class property_type : std::uint8_t {
    value,
    other
};

struct MetaType: ::testing::Test {
    void SetUp() override {
        using namespace entt::literals;

        entt::meta_factory<double>{}
            .type("double"_hs)
            .traits(test::meta_traits::one)
            .data<set<double>, get<double>>("var"_hs);

        entt::meta_factory<unsigned int>{}
            .type("unsigned int"_hs)
            .traits(test::meta_traits::two)
            .data<0u>("min"_hs)
            .data<128u>("max"_hs);

        entt::meta_factory<base>{}
            .type("base"_hs)
            .data<&base::value>("value"_hs);

        entt::meta_factory<derived>{}
            .type("derived"_hs)
            .traits(test::meta_traits::one | test::meta_traits::three)
            .base<base>();

        entt::meta_factory<abstract>{}
            .type("abstract"_hs)
            .func<&abstract::func>("func"_hs)
            .func<&abstract::base_only>("base_only"_hs);

        entt::meta_factory<concrete>{}
            .type("concrete"_hs)
            .base<base>()
            .base<abstract>();

        entt::meta_factory<overloaded_func>{}
            .type("overloaded_func"_hs)
            .func<entt::overload<int(const base &, int, int)>(&overloaded_func::f)>("f"_hs)
            .func<entt::overload<int(int, int)>(&overloaded_func::f)>("f"_hs)
            .func<entt::overload<int(int)>(&overloaded_func::f)>("f"_hs)
            .func<entt::overload<int(int) const>(&overloaded_func::f)>("f"_hs)
            .func<entt::overload<float(int, float)>(&overloaded_func::f)>("f"_hs);

        entt::meta_factory<property_type>{}
            .type("property"_hs)
            .traits(test::meta_traits::two | test::meta_traits::three)
            .data<property_type::value>("value"_hs)
            .data<property_type::other>("other"_hs)
            .data<set<property_type>, get<property_type>>("var"_hs);

        entt::meta_factory<clazz>{}
            .type("class"_hs)
            .custom<char>('c')
            .ctor<const base &, int>()
            .data<&clazz::value>("value"_hs)
            .func<&clazz::member>("member"_hs)
            .func<clazz::func>("func"_hs)
            .conv<int>();
    }

    void TearDown() override {
        entt::meta_reset();
    }
};

using MetaTypeDeathTest = MetaType;

TEST_F(MetaType, Resolve) {
    using namespace entt::literals;

    ASSERT_EQ(entt::resolve<double>(), entt::resolve("double"_hs));
    ASSERT_EQ(entt::resolve<double>(), entt::resolve(entt::type_id<double>()));
    ASSERT_FALSE(entt::resolve(entt::type_id<void>()));

    auto range = entt::resolve();
    const auto it = std::find_if(range.begin(), range.end(), [](auto curr) { return curr.second.id() == "class"_hs; });

    ASSERT_NE(it, range.end());
    ASSERT_EQ(it->second, entt::resolve<clazz>());

    bool found = false;

    for(auto curr: entt::resolve()) {
        found = found || curr.second == entt::resolve<double>();
    }

    ASSERT_TRUE(found);
}

TEST_F(MetaType, SafeWhenEmpty) {
    using namespace entt::literals;

    entt::meta_type type{};
    entt::meta_any *args = nullptr;

    ASSERT_FALSE(type);
    ASSERT_EQ(type, entt::meta_type{});
    ASSERT_EQ(type.info(), entt::type_id<void>());
    ASSERT_EQ(type.id(), entt::id_type{});
    ASSERT_EQ(type.size_of(), 0u);
    ASSERT_FALSE(type.is_arithmetic());
    ASSERT_FALSE(type.is_integral());
    ASSERT_FALSE(type.is_signed());
    ASSERT_FALSE(type.is_array());
    ASSERT_FALSE(type.is_enum());
    ASSERT_FALSE(type.is_class());
    ASSERT_FALSE(type.is_pointer());
    ASSERT_EQ(type.remove_pointer(), type);
    ASSERT_FALSE(type.is_pointer_like());
    ASSERT_FALSE(type.is_sequence_container());
    ASSERT_FALSE(type.is_associative_container());
    ASSERT_FALSE(type.is_template_specialization());
    ASSERT_EQ(type.template_arity(), 0u);
    ASSERT_EQ(type.template_type(), type);
    ASSERT_EQ(type.template_arg(0u), type);
    ASSERT_EQ(type.template_arg(1u), type);
    ASSERT_FALSE(type.can_cast(type));
    ASSERT_FALSE(type.can_cast(entt::resolve<void>()));
    ASSERT_FALSE(type.can_convert(type));
    ASSERT_FALSE(type.can_convert(entt::resolve<void>()));
    ASSERT_EQ(type.base().begin(), type.base().end());
    ASSERT_EQ(type.data().begin(), type.data().end());
    ASSERT_EQ(type.data("data"_hs), entt::meta_data{});
    ASSERT_EQ(type.func().begin(), type.func().end());
    ASSERT_EQ(type.func("func"_hs), entt::meta_func{});
    ASSERT_FALSE(type.construct(args, 0u));
    ASSERT_FALSE(type.construct(args, 1u));
    ASSERT_FALSE(type.construct());
    ASSERT_FALSE(type.construct(0.0));
    ASSERT_FALSE(type.from_void(static_cast<void *>(nullptr)));
    ASSERT_FALSE(type.from_void(static_cast<void *>(nullptr), true));
    ASSERT_FALSE(type.from_void(static_cast<void *>(&type)));
    ASSERT_FALSE(type.from_void(static_cast<void *>(&type), true));
    ASSERT_FALSE(type.from_void(static_cast<const void *>(nullptr)));
    ASSERT_FALSE(type.from_void(static_cast<const void *>(&type)));
    ASSERT_FALSE(type.invoke("func"_hs, {}, args, 0u));
    ASSERT_FALSE(type.invoke("func"_hs, {}, args, 1u));
    ASSERT_FALSE(type.invoke("func"_hs, {}));
    ASSERT_FALSE(type.invoke("func"_hs, {}, 'c'));
    ASSERT_FALSE(type.set("data"_hs, {}, 0));
    ASSERT_FALSE(type.get("data"_hs, {}));
    ASSERT_EQ(type.traits<test::meta_traits>(), test::meta_traits::none);
    ASSERT_EQ(static_cast<const char *>(type.custom()), nullptr);
}

TEST_F(MetaType, UserTraits) {
    ASSERT_EQ(entt::resolve<bool>().traits<test::meta_traits>(), test::meta_traits::none);
    ASSERT_EQ(entt::resolve<clazz>().traits<test::meta_traits>(), test::meta_traits::none);

    ASSERT_EQ(entt::resolve<double>().traits<test::meta_traits>(), test::meta_traits::one);
    ASSERT_EQ(entt::resolve<unsigned int>().traits<test::meta_traits>(), test::meta_traits::two);
    ASSERT_EQ(entt::resolve<derived>().traits<test::meta_traits>(), test::meta_traits::one | test::meta_traits::three);
    ASSERT_EQ(entt::resolve<property_type>().traits<test::meta_traits>(), test::meta_traits::two | test::meta_traits::three);
}

ENTT_DEBUG_TEST_F(MetaTypeDeathTest, UserTraits) {
    using traits_type = entt::internal::meta_traits;
    constexpr auto value = traits_type{static_cast<std::underlying_type_t<traits_type>>(traits_type::_user_defined_traits) + 1u};
    ASSERT_DEATH(entt::meta_factory<clazz>{}.traits(value), "");
}

TEST_F(MetaType, Custom) {
    ASSERT_EQ(*static_cast<const char *>(entt::resolve<clazz>().custom()), 'c');
    ASSERT_EQ(static_cast<const char &>(entt::resolve<clazz>().custom()), 'c');

    ASSERT_EQ(static_cast<const int *>(entt::resolve<clazz>().custom()), nullptr);
    ASSERT_EQ(static_cast<const int *>(entt::resolve<base>().custom()), nullptr);
}

ENTT_DEBUG_TEST_F(MetaTypeDeathTest, Custom) {
    ASSERT_DEATH([[maybe_unused]] const int value = entt::resolve<clazz>().custom(), "");
    ASSERT_DEATH([[maybe_unused]] const char value = entt::resolve<base>().custom(), "");
}

TEST_F(MetaType, IdAndInfo) {
    using namespace entt::literals;

    auto type = entt::resolve<clazz>();

    ASSERT_TRUE(type);
    ASSERT_NE(type, entt::meta_type{});
    ASSERT_EQ(type.id(), "class"_hs);
    ASSERT_EQ(type.info(), entt::type_id<clazz>());
}

TEST_F(MetaType, SizeOf) {
    ASSERT_EQ(entt::resolve<void>().size_of(), 0u);
    ASSERT_EQ(entt::resolve<int>().size_of(), sizeof(int));
    // NOLINTBEGIN(*-avoid-c-arrays)
    ASSERT_EQ(entt::resolve<int[]>().size_of(), 0u);
    ASSERT_EQ(entt::resolve<int[3]>().size_of(), sizeof(int[3]));
    // NOLINTEND(*-avoid-c-arrays)
}

TEST_F(MetaType, Traits) {
    ASSERT_TRUE(entt::resolve<bool>().is_arithmetic());
    ASSERT_TRUE(entt::resolve<double>().is_arithmetic());
    ASSERT_FALSE(entt::resolve<clazz>().is_arithmetic());

    ASSERT_TRUE(entt::resolve<int>().is_integral());
    ASSERT_FALSE(entt::resolve<double>().is_integral());
    ASSERT_FALSE(entt::resolve<clazz>().is_integral());

    ASSERT_TRUE(entt::resolve<long>().is_signed());
    ASSERT_FALSE(entt::resolve<unsigned int>().is_signed());
    ASSERT_FALSE(entt::resolve<clazz>().is_signed());

    // NOLINTBEGIN(*-avoid-c-arrays)
    ASSERT_TRUE(entt::resolve<int[5]>().is_array());
    ASSERT_TRUE(entt::resolve<int[5][3]>().is_array());
    // NOLINTEND(*-avoid-c-arrays)
    ASSERT_FALSE(entt::resolve<int>().is_array());

    ASSERT_TRUE(entt::resolve<property_type>().is_enum());
    ASSERT_FALSE(entt::resolve<char>().is_enum());

    ASSERT_TRUE(entt::resolve<derived>().is_class());
    ASSERT_FALSE(entt::resolve<double>().is_class());

    ASSERT_TRUE(entt::resolve<int *>().is_pointer());
    ASSERT_FALSE(entt::resolve<int>().is_pointer());

    ASSERT_TRUE(entt::resolve<int *>().is_pointer_like());
    ASSERT_TRUE(entt::resolve<std::shared_ptr<int>>().is_pointer_like());
    ASSERT_FALSE(entt::resolve<int>().is_pointer_like());

    ASSERT_FALSE((entt::resolve<int>().is_sequence_container()));
    ASSERT_TRUE(entt::resolve<std::vector<int>>().is_sequence_container());
    ASSERT_FALSE((entt::resolve<std::map<int, char>>().is_sequence_container()));

    ASSERT_FALSE((entt::resolve<int>().is_associative_container()));
    ASSERT_TRUE((entt::resolve<std::map<int, char>>().is_associative_container()));
    ASSERT_FALSE(entt::resolve<std::vector<int>>().is_associative_container());
}

TEST_F(MetaType, RemovePointer) {
    ASSERT_EQ(entt::resolve<void *>().remove_pointer(), entt::resolve<void>());
    ASSERT_EQ(entt::resolve<char **>().remove_pointer(), entt::resolve<char *>());
    ASSERT_EQ(entt::resolve<int (*)(char, double)>().remove_pointer(), entt::resolve<int(char, double)>());
    ASSERT_EQ(entt::resolve<derived>().remove_pointer(), entt::resolve<derived>());
}

TEST_F(MetaType, TemplateInfo) {
    ASSERT_FALSE(entt::resolve<int>().is_template_specialization());
    ASSERT_EQ(entt::resolve<int>().template_arity(), 0u);
    ASSERT_EQ(entt::resolve<int>().template_type(), entt::meta_type{});
    ASSERT_EQ(entt::resolve<int>().template_arg(0u), entt::meta_type{});

    ASSERT_TRUE(entt::resolve<std::shared_ptr<int>>().is_template_specialization());
    ASSERT_EQ(entt::resolve<std::shared_ptr<int>>().template_arity(), 1u);
    ASSERT_EQ(entt::resolve<std::shared_ptr<int>>().template_type(), entt::resolve<entt::meta_class_template_tag<std::shared_ptr>>());
    ASSERT_EQ(entt::resolve<std::shared_ptr<int>>().template_arg(0u), entt::resolve<int>());
    ASSERT_EQ(entt::resolve<std::shared_ptr<int>>().template_arg(1u), entt::meta_type{});
}

TEST_F(MetaType, CanCast) {
    auto type = entt::resolve<derived>();

    ASSERT_FALSE(type.can_cast(entt::resolve<void>()));
    ASSERT_TRUE(type.can_cast(entt::resolve<base>()));
    ASSERT_TRUE(type.can_cast(entt::resolve<derived>()));
}

TEST_F(MetaType, CanConvert) {
    auto instance = entt::resolve<clazz>();
    auto other = entt::resolve<derived>();
    auto arithmetic = entt::resolve<int>();

    ASSERT_TRUE(instance.can_convert(entt::resolve<clazz>()));
    ASSERT_TRUE(instance.can_convert(entt::resolve<int>()));

    ASSERT_TRUE(other.can_convert(entt::resolve<derived>()));
    ASSERT_TRUE(other.can_convert(entt::resolve<base>()));
    ASSERT_FALSE(other.can_convert(entt::resolve<int>()));

    ASSERT_TRUE(arithmetic.can_convert(entt::resolve<int>()));
    ASSERT_FALSE(arithmetic.can_convert(entt::resolve<clazz>()));
    ASSERT_TRUE(arithmetic.can_convert(entt::resolve<double>()));
    ASSERT_TRUE(arithmetic.can_convert(entt::resolve<float>()));
}

TEST_F(MetaType, Base) {
    auto type = entt::resolve<derived>();

    ASSERT_NE(type.base().cbegin(), type.base().cend());

    for(auto curr: type.base()) {
        ASSERT_EQ(curr.first, entt::type_id<base>().hash());
        ASSERT_EQ(curr.second, entt::resolve<base>());
    }
}

TEST_F(MetaType, Ctor) {
    derived instance;
    base &as_base = instance;
    auto type = entt::resolve<clazz>();

    ASSERT_TRUE((type.construct(entt::forward_as_meta(instance), 3)));
    ASSERT_TRUE((type.construct(entt::forward_as_meta(as_base), 3)));

    // use the implicitly generated default constructor
    auto any = type.construct();

    ASSERT_TRUE(any);
    ASSERT_EQ(any.type(), entt::resolve<clazz>());
}

TEST_F(MetaType, Data) {
    using namespace entt::literals;

    auto type = entt::resolve<clazz>();
    int counter{};

    for([[maybe_unused]] auto &&curr: type.data()) {
        ++counter;
    }

    ASSERT_EQ(counter, 1);
    ASSERT_TRUE(type.data("value"_hs));

    type = entt::resolve<void>();

    ASSERT_TRUE(type);
    ASSERT_EQ(type.data().cbegin(), type.data().cend());
}

TEST_F(MetaType, Func) {
    using namespace entt::literals;

    auto type = entt::resolve<clazz>();
    clazz instance{};
    int counter{};

    for([[maybe_unused]] auto &&curr: type.func()) {
        ++counter;
    }

    ASSERT_EQ(counter, 2);
    ASSERT_TRUE(type.func("member"_hs));
    ASSERT_TRUE(type.func("func"_hs));
    ASSERT_TRUE(type.func("member"_hs).invoke(instance));
    ASSERT_TRUE(type.func("func"_hs).invoke({}));

    type = entt::resolve<void>();

    ASSERT_TRUE(type);
    ASSERT_EQ(type.func().cbegin(), type.func().cend());
}

TEST_F(MetaType, Invoke) {
    using namespace entt::literals;

    auto type = entt::resolve<clazz>();
    clazz instance{};

    ASSERT_TRUE(type.invoke("member"_hs, instance));
    ASSERT_FALSE(type.invoke("rebmem"_hs, instance));

    ASSERT_TRUE(type.invoke("func"_hs, {}));
    ASSERT_FALSE(type.invoke("cnuf"_hs, {}));
}

TEST_F(MetaType, InvokeFromBase) {
    using namespace entt::literals;

    auto type = entt::resolve<concrete>();
    concrete instance{};

    ASSERT_TRUE(type.invoke("base_only"_hs, instance, 3));
    ASSERT_FALSE(type.invoke("ylno_esab"_hs, {}, 'c'));
}

TEST_F(MetaType, OverloadedFunc) {
    using namespace entt::literals;

    const auto type = entt::resolve<overloaded_func>();
    overloaded_func instance{};
    entt::meta_any res{};

    ASSERT_TRUE(type.func("f"_hs));

    res = type.invoke("f"_hs, instance, base{}, 1, 2);

    ASSERT_TRUE(res);
    ASSERT_EQ(instance.value, 1);
    ASSERT_NE(res.try_cast<int>(), nullptr);
    ASSERT_EQ(res.cast<int>(), 4);

    res = type.invoke("f"_hs, instance, 3, 4);

    ASSERT_TRUE(res);
    ASSERT_EQ(instance.value, 3);
    ASSERT_NE(res.try_cast<int>(), nullptr);
    ASSERT_EQ(res.cast<int>(), 16);

    res = type.invoke("f"_hs, instance, 2);

    ASSERT_TRUE(res);
    ASSERT_EQ(instance.value, 3);
    ASSERT_NE(res.try_cast<int>(), nullptr);
    ASSERT_EQ(res.cast<int>(), 12);

    res = type.invoke("f"_hs, std::as_const(instance), 2);

    ASSERT_TRUE(res);
    ASSERT_EQ(instance.value, 3);
    ASSERT_NE(res.try_cast<int>(), nullptr);
    ASSERT_EQ(res.cast<int>(), 6);

    res = type.invoke("f"_hs, instance, 0, 1.f);

    ASSERT_TRUE(res);
    ASSERT_EQ(instance.value, 0);
    ASSERT_NE(res.try_cast<float>(), nullptr);
    ASSERT_EQ(res.cast<float>(), 2.f);

    res = type.invoke("f"_hs, instance, 4, 8.f);

    ASSERT_TRUE(res);
    ASSERT_EQ(instance.value, 4);
    ASSERT_NE(res.try_cast<float>(), nullptr);
    ASSERT_EQ(res.cast<float>(), 16.f);

    // it fails as an ambiguous call
    ASSERT_FALSE(type.invoke("f"_hs, instance, 4, 8.));
}

TEST_F(MetaType, OverloadedFuncOrder) {
    using namespace entt::literals;

    const auto type = entt::resolve<overloaded_func>();
    auto func = type.func("f"_hs);

    ASSERT_TRUE(func);
    ASSERT_EQ(func.arity(), 3u);
    ASSERT_FALSE(func.is_const());
    ASSERT_EQ(func.ret(), entt::resolve<int>());

    func = func.next();

    ASSERT_TRUE(func);
    ASSERT_EQ(func.arity(), 2u);
    ASSERT_FALSE(func.is_const());
    ASSERT_EQ(func.ret(), entt::resolve<int>());

    func = func.next();

    ASSERT_TRUE(func);
    ASSERT_EQ(func.arity(), 1u);
    ASSERT_FALSE(func.is_const());
    ASSERT_EQ(func.ret(), entt::resolve<int>());

    func = func.next();

    ASSERT_TRUE(func);
    ASSERT_EQ(func.arity(), 1u);
    ASSERT_TRUE(func.is_const());
    ASSERT_EQ(func.ret(), entt::resolve<int>());

    func = func.next();

    ASSERT_TRUE(func);
    ASSERT_EQ(func.arity(), 2u);
    ASSERT_FALSE(func.is_const());
    ASSERT_EQ(func.ret(), entt::resolve<float>());
}

TEST_F(MetaType, Construct) {
    auto any = entt::resolve<clazz>().construct(base{}, 2);

    ASSERT_TRUE(any);
    ASSERT_EQ(any.cast<clazz>().value, 2);
}

TEST_F(MetaType, ConstructNoArgs) {
    // this should work, no other tests required
    auto any = entt::resolve<clazz>().construct();

    ASSERT_TRUE(any);
}

TEST_F(MetaType, ConstructMetaAnyArgs) {
    auto any = entt::resolve<clazz>().construct(entt::meta_any{base{}}, entt::meta_any{3});

    ASSERT_TRUE(any);
    ASSERT_EQ(any.cast<clazz>().value, 3);
}

TEST_F(MetaType, ConstructInvalidArgs) {
    ASSERT_FALSE(entt::resolve<clazz>().construct('c', base{}));
}

TEST_F(MetaType, LessArgs) {
    ASSERT_FALSE(entt::resolve<clazz>().construct(base{}));
}

TEST_F(MetaType, ConstructCastAndConvert) {
    auto any = entt::resolve<clazz>().construct(derived{}, clazz{derived{}, 3});

    ASSERT_TRUE(any);
    ASSERT_EQ(any.cast<clazz>().value, 3);
}

TEST_F(MetaType, ConstructArithmeticConversion) {
    auto any = entt::resolve<clazz>().construct(derived{}, true);

    ASSERT_TRUE(any);
    ASSERT_EQ(any.cast<clazz>().value, 1);
}

TEST_F(MetaType, FromVoid) {
    ASSERT_FALSE(entt::resolve<double>().from_void(static_cast<double *>(nullptr)));
    ASSERT_FALSE(entt::resolve<double>().from_void(static_cast<const double *>(nullptr)));

    double value = 4.2;

    ASSERT_FALSE(entt::resolve<void>().from_void(static_cast<void *>(&value)));
    ASSERT_FALSE(entt::resolve<void>().from_void(static_cast<const void *>(&value)));

    auto type = entt::resolve<double>();
    auto as_void = type.from_void(static_cast<void *>(&value));
    auto as_const_void = type.from_void(static_cast<const void *>(&value));

    ASSERT_TRUE(as_void);
    ASSERT_TRUE(as_const_void);

    ASSERT_EQ(as_void.type(), entt::resolve<double>());
    ASSERT_NE(as_void.try_cast<double>(), nullptr);

    ASSERT_EQ(as_const_void.type(), entt::resolve<double>());
    ASSERT_EQ(as_const_void.try_cast<double>(), nullptr);
    ASSERT_NE(as_const_void.try_cast<const double>(), nullptr);

    value = 1.2;

    ASSERT_EQ(as_void.cast<double>(), as_const_void.cast<double>());
    ASSERT_EQ(as_void.cast<double>(), 1.2);
}

TEST_F(MetaType, FromVoidOwnership) {
    bool check = false;
    auto type = entt::resolve<from_void_callback>();
    void *instance = std::make_unique<from_void_callback>(check).release();

    auto any = type.from_void(instance);
    // NOLINTNEXTLINE(clang-analyzer-cplusplus.NewDeleteLeaks)
    auto other = type.from_void(instance, true);

    ASSERT_TRUE(any);
    ASSERT_TRUE(other);

    ASSERT_FALSE(check);

    any.reset();

    ASSERT_FALSE(check);

    other.reset();

    ASSERT_TRUE(check);
}

TEST_F(MetaType, Reset) {
    using namespace entt::literals;

    ASSERT_TRUE(entt::resolve("class"_hs));
    ASSERT_EQ(entt::resolve<clazz>().id(), "class"_hs);
    ASSERT_TRUE(entt::resolve<clazz>().data("value"_hs));
    ASSERT_TRUE((entt::resolve<clazz>().construct(derived{}, clazz{})));
    // implicitly generated default constructor
    ASSERT_TRUE(entt::resolve<clazz>().construct());

    entt::meta_reset("class"_hs);

    ASSERT_FALSE(entt::resolve("class"_hs));
    ASSERT_NE(entt::resolve<clazz>().id(), "class"_hs);
    ASSERT_FALSE(entt::resolve<clazz>().data("value"_hs));
    ASSERT_FALSE((entt::resolve<clazz>().construct(derived{}, clazz{})));
    // implicitly generated default constructor is not cleared
    ASSERT_TRUE(entt::resolve<clazz>().construct());

    entt::meta_factory<clazz>{}.type("class"_hs);

    ASSERT_TRUE(entt::resolve("class"_hs));
}

TEST_F(MetaType, ResetLast) {
    auto id = (entt::resolve().cend() - 1u)->second.id();

    ASSERT_TRUE(entt::resolve(id));

    entt::meta_reset(id);

    ASSERT_FALSE(entt::resolve(id));
}

TEST_F(MetaType, ResetAll) {
    using namespace entt::literals;

    ASSERT_NE(entt::resolve().begin(), entt::resolve().end());

    ASSERT_TRUE(entt::resolve("class"_hs));
    ASSERT_TRUE(entt::resolve("overloaded_func"_hs));
    ASSERT_TRUE(entt::resolve("double"_hs));

    entt::meta_reset();

    ASSERT_FALSE(entt::resolve("class"_hs));
    ASSERT_FALSE(entt::resolve("overloaded_func"_hs));
    ASSERT_FALSE(entt::resolve("double"_hs));

    ASSERT_EQ(entt::resolve().begin(), entt::resolve().end());
}

TEST_F(MetaType, AbstractClass) {
    using namespace entt::literals;

    auto type = entt::resolve<abstract>();
    concrete instance;

    ASSERT_EQ(type.info(), entt::type_id<abstract>());
    ASSERT_EQ(instance.base::value, 'c');
    ASSERT_EQ(instance.value, 3);

    type.func("func"_hs).invoke(instance, 2);

    ASSERT_EQ(instance.base::value, 'c');
    ASSERT_EQ(instance.value, 2);
}

TEST_F(MetaType, EnumAndNamedConstants) {
    using namespace entt::literals;

    auto type = entt::resolve<property_type>();

    ASSERT_TRUE(type.data("value"_hs));
    ASSERT_TRUE(type.data("other"_hs));

    ASSERT_EQ(type.data("value"_hs).type(), type);
    ASSERT_EQ(type.data("other"_hs).type(), type);

    ASSERT_EQ(type.data("value"_hs).get({}).cast<property_type>(), property_type::value);
    ASSERT_EQ(type.data("other"_hs).get({}).cast<property_type>(), property_type::other);

    ASSERT_FALSE(type.data("value"_hs).set({}, property_type::other));
    ASSERT_FALSE(type.data("other"_hs).set({}, property_type::value));

    ASSERT_EQ(type.data("value"_hs).get({}).cast<property_type>(), property_type::value);
    ASSERT_EQ(type.data("other"_hs).get({}).cast<property_type>(), property_type::other);
}

TEST_F(MetaType, ArithmeticTypeAndNamedConstants) {
    using namespace entt::literals;

    auto type = entt::resolve<unsigned int>();

    ASSERT_TRUE(type.data("min"_hs));
    ASSERT_TRUE(type.data("max"_hs));

    ASSERT_EQ(type.data("min"_hs).type(), type);
    ASSERT_EQ(type.data("max"_hs).type(), type);

    ASSERT_FALSE(type.data("min"_hs).set({}, 128u));
    ASSERT_FALSE(type.data("max"_hs).set({}, 0u));

    ASSERT_EQ(type.data("min"_hs).get({}).cast<unsigned int>(), 0u);
    ASSERT_EQ(type.data("max"_hs).get({}).cast<unsigned int>(), 128u);
}

TEST_F(MetaType, Variables) {
    using namespace entt::literals;

    auto p_data = entt::resolve<property_type>().data("var"_hs);
    auto d_data = entt::resolve("double"_hs).data("var"_hs);

    property_type prop{property_type::value};
    double value = 3.;

    p_data.set(prop, property_type::other);
    d_data.set(value, 3.);

    ASSERT_EQ(p_data.get(prop).cast<property_type>(), property_type::other);
    ASSERT_EQ(d_data.get(value).cast<double>(), 3.);
    ASSERT_EQ(prop, property_type::other);
    ASSERT_EQ(value, 3.);
}

TEST_F(MetaType, ResetAndReRegistrationAfterReset) {
    using namespace entt::literals;

    ASSERT_FALSE(entt::internal::meta_context::from(entt::locator<entt::meta_ctx>::value_or()).value.empty());

    entt::meta_reset<double>();
    entt::meta_reset<unsigned int>();
    entt::meta_reset<base>();
    entt::meta_reset<derived>();
    entt::meta_reset<abstract>();
    entt::meta_reset<concrete>();
    entt::meta_reset<overloaded_func>();
    entt::meta_reset<property_type>();
    entt::meta_reset<clazz>();

    ASSERT_FALSE(entt::resolve("double"_hs));
    ASSERT_FALSE(entt::resolve("base"_hs));
    ASSERT_FALSE(entt::resolve("derived"_hs));
    ASSERT_FALSE(entt::resolve("class"_hs));

    ASSERT_TRUE(entt::internal::meta_context::from(entt::locator<entt::meta_ctx>::value_or()).value.empty());

    // implicitly generated default constructor is not cleared
    ASSERT_TRUE(entt::resolve<clazz>().construct());
    ASSERT_FALSE(entt::resolve<clazz>().data("value"_hs));
    ASSERT_FALSE(entt::resolve<clazz>().func("member"_hs));

    entt::meta_factory<double>{}.type("double"_hs);
    entt::meta_any any{3.};

    ASSERT_TRUE(any);
    ASSERT_TRUE(any.allow_cast<int>());
    ASSERT_TRUE(any.allow_cast<float>());

    ASSERT_FALSE(entt::resolve("derived"_hs));
    ASSERT_TRUE(entt::resolve("double"_hs));

    entt::meta_factory<base>{}
        .traits(test::meta_traits::one)
        .custom<int>(3)
        // this should not overwrite traits and custom data
        .type("base"_hs);

    // this should not overwrite traits and custom data
    [[maybe_unused]] const entt::meta_factory<base> factory{};

    ASSERT_EQ(entt::resolve<base>().traits<test::meta_traits>(), test::meta_traits::one);
    ASSERT_NE(static_cast<const int *>(entt::resolve("base"_hs).custom()), nullptr);
}

TEST_F(MetaType, ReRegistration) {
    using namespace entt::literals;

    int count = 0;

    for([[maybe_unused]] auto type: entt::resolve()) {
        ++count;
    }

    SetUp();

    for([[maybe_unused]] auto type: entt::resolve()) {
        --count;
    }

    ASSERT_EQ(count, 0);
    ASSERT_TRUE(entt::resolve("double"_hs));

    entt::meta_factory<double>{}
        .type("real"_hs)
        .traits(test::meta_traits::one)
        .custom<int>(3);

    // this should not overwrite traits and custom data
    entt::meta_factory<double>{}.type("real"_hs);

    ASSERT_FALSE(entt::resolve("double"_hs));
    ASSERT_TRUE(entt::resolve("real"_hs));
    ASSERT_TRUE(entt::resolve("real"_hs).data("var"_hs));

    ASSERT_EQ(entt::resolve<double>().traits<test::meta_traits>(), test::meta_traits::one);
    ASSERT_NE(static_cast<const int *>(entt::resolve<double>().custom()), nullptr);
}

TEST_F(MetaType, NameCollision) {
    using namespace entt::literals;

    ASSERT_NO_THROW(entt::meta_factory<clazz>{}.type("class"_hs));
    ASSERT_TRUE(entt::resolve("class"_hs));

    ASSERT_NO_THROW(entt::meta_factory<clazz>{}.type("quux"_hs));
    ASSERT_FALSE(entt::resolve("class"_hs));
    ASSERT_TRUE(entt::resolve("quux"_hs));
}

ENTT_DEBUG_TEST_F(MetaTypeDeathTest, NameCollision) {
    using namespace entt::literals;

    ASSERT_DEATH(entt::meta_factory<clazz>{}.type("abstract"_hs), "");
}

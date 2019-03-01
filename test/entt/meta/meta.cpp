#include <type_traits>
#include <gtest/gtest.h>
#include <entt/core/hashed_string.hpp>
#include <entt/core/utility.hpp>
#include <entt/meta/factory.hpp>
#include <entt/meta/meta.hpp>

enum class properties {
    prop_int,
    prop_bool
};

struct empty_type {
    virtual ~empty_type() = default;

    static void destroy(empty_type *) {
        ++counter;
    }

    inline static int counter = 0;
};

struct fat_type: empty_type {
    fat_type() = default;

    fat_type(int *value)
        : foo{value}, bar{value}
    {}

    int *foo{nullptr};
    int *bar{nullptr};

    bool operator==(const fat_type &other) const {
        return foo == other.foo && bar == other.bar;
    }
};

union union_type {
    int i;
    double d;
};

bool operator!=(const fat_type &lhs, const fat_type &rhs) {
    return !(lhs == rhs);
}

struct base_type {
    virtual ~base_type() = default;
};

struct derived_type: base_type {
    derived_type() = default;
    derived_type(const base_type &, int i, char c): i{i}, c{c} {}

    const int i{};
    const char c{};
};

derived_type derived_factory(const base_type &, int value) {
    return {derived_type{}, value, 'c'};
}

struct data_type {
    int i{0};
    const int j{1};
    inline static int h{2};
    inline static const int k{3};
    empty_type empty{};
};

struct func_type {
    int f(const base_type &, int a, int b) { return f(a, b); }
    int f(int a, int b) { value = a; return b*b; }
    int f(int v) const { return v*v; }
    void g(int v) { value = v*v; }

    static int h(int v) { return v; }
    static void k(int v) { value = v; }

    inline static int value = 0;
};

struct setter_getter_type {
    int value{};

    int setter(int value) { return this->value = value; }
    int getter() { return value; }

    int setter_with_ref(const int &value) { return this->value = value; }
    const int & getter_with_ref() { return value; }

    static int static_setter(setter_getter_type *type, int value) { return type->value = value; }
    static int static_getter(const setter_getter_type *type) { return type->value; }
};

struct not_comparable_type {
    bool operator==(const not_comparable_type &) const = delete;
};

bool operator!=(const not_comparable_type &, const not_comparable_type &) = delete;

struct an_abstract_type {
    virtual ~an_abstract_type() = default;
    void f(int v) { i = v; }
    virtual void g(int) = 0;
    int i{};
};

struct another_abstract_type {
    virtual ~another_abstract_type() = default;
    virtual void h(char) = 0;
    char j{};
};

struct concrete_type: an_abstract_type, another_abstract_type {
    void f(int v) { i = v*v; } // hide, it's ok :-)
    void g(int v) override { i = -v; }
    void h(char c) override { j = c; }
};

struct Meta: public ::testing::Test {
    static void SetUpTestCase() {
        entt::reflect<double>().conv<int>();

        entt::reflect<char>("char", std::make_pair(properties::prop_int, 42));

        entt::reflect<properties>()
                .data<properties::prop_bool>("prop_bool")
                .data<properties::prop_int>("prop_int");

        entt::reflect<unsigned int>().data<0u>("min").data<100u>("max");

        entt::reflect<base_type>("base");

        entt::reflect<derived_type>("derived", std::make_pair(properties::prop_int, 99))
                .base<base_type>()
                .ctor<const base_type &, int, char>(std::make_pair(properties::prop_bool, false))
                .ctor<&derived_factory>(std::make_pair(properties::prop_int, 42));

        entt::reflect<empty_type>("empty")
                .dtor<&empty_type::destroy>();

        entt::reflect<fat_type>("fat")
                .base<empty_type>()
                .dtor<&fat_type::destroy>();

        entt::reflect<data_type>("data")
                .data<&data_type::i>("i", std::make_pair(properties::prop_int, 0))
                .data<&data_type::j>("j", std::make_pair(properties::prop_int, 1))
                .data<&data_type::h>("h", std::make_pair(properties::prop_int, 2))
                .data<&data_type::k>("k", std::make_pair(properties::prop_int, 3))
                .data<&data_type::empty>("empty");

        entt::reflect<func_type>("func")
                .func<entt::overload<int(const base_type &, int, int)>(&func_type::f)>("f3")
                .func<entt::overload<int(int, int)>(&func_type::f)>("f2", std::make_pair(properties::prop_bool, false))
                .func<entt::overload<int(int) const>(&func_type::f)>("f1", std::make_pair(properties::prop_bool, false))
                .func<&func_type::g>("g", std::make_pair(properties::prop_bool, false))
                .func<&func_type::h>("h", std::make_pair(properties::prop_bool, false))
                .func<&func_type::k>("k", std::make_pair(properties::prop_bool, false));

        entt::reflect<setter_getter_type>("setter_getter")
                .data<&setter_getter_type::static_setter, &setter_getter_type::static_getter>("x")
                .data<&setter_getter_type::setter, &setter_getter_type::getter>("y")
                .data<&setter_getter_type::static_setter, &setter_getter_type::getter>("z")
                .data<&setter_getter_type::setter_with_ref, &setter_getter_type::getter_with_ref>("w");

        entt::reflect<an_abstract_type>("an_abstract_type", std::make_pair(properties::prop_bool, false))
                .data<&an_abstract_type::i>("i")
                .func<&an_abstract_type::f>("f")
                .func<&an_abstract_type::g>("g");

        entt::reflect<another_abstract_type>("another_abstract_type", std::make_pair(properties::prop_int, 42))
                .data<&another_abstract_type::j>("j")
                .func<&another_abstract_type::h>("h");

        entt::reflect<concrete_type>("concrete")
                .base<an_abstract_type>()
                .base<another_abstract_type>()
                .func<&concrete_type::f>("f");
    }

    void SetUp() override {
        empty_type::counter = 0;
        func_type::value = 0;
    }
};

TEST_F(Meta, Resolve) {
    ASSERT_EQ(entt::resolve<derived_type>(), entt::resolve("derived"));

    bool found = false;

    entt::resolve([&found](auto type) {
        found = found || type == entt::resolve<derived_type>();
    });

    ASSERT_TRUE(found);
}

TEST_F(Meta, MetaAnySBO) {
    entt::meta_any any{'c'};

    ASSERT_TRUE(any);
    ASSERT_FALSE(any.can_cast<void>());
    ASSERT_TRUE(any.can_cast<char>());
    ASSERT_EQ(any.cast<char>(), 'c');
    ASSERT_EQ(std::as_const(any).cast<char>(), 'c');
    ASSERT_NE(any.data(), nullptr);
    ASSERT_NE(std::as_const(any).data(), nullptr);
    ASSERT_EQ(any, entt::meta_any{'c'});
    ASSERT_NE(any, entt::meta_any{'h'});
}

TEST_F(Meta, MetaAnyNoSBO) {
    int value = 42;
    fat_type instance{&value};
    entt::meta_any any{instance};

    ASSERT_TRUE(any);
    ASSERT_FALSE(any.can_cast<void>());
    ASSERT_TRUE(any.can_cast<fat_type>());
    ASSERT_EQ(any.cast<fat_type>(), instance);
    ASSERT_EQ(std::as_const(any).cast<fat_type>(), instance);
    ASSERT_NE(any.data(), nullptr);
    ASSERT_NE(std::as_const(any).data(), nullptr);
    ASSERT_EQ(any, entt::meta_any{instance});
    ASSERT_NE(any, fat_type{});
}

TEST_F(Meta, MetaAnyEmpty) {
    entt::meta_any any{};

    ASSERT_FALSE(any);
    ASSERT_FALSE(any.type());
    ASSERT_FALSE(any.can_cast<void>());
    ASSERT_FALSE(any.can_cast<empty_type>());
    ASSERT_EQ(any.data(), nullptr);
    ASSERT_EQ(std::as_const(any).data(), nullptr);
    ASSERT_EQ(any, entt::meta_any{});
    ASSERT_NE(any, entt::meta_any{'c'});
}

TEST_F(Meta, MetaAnySBOCopyConstruction) {
    entt::meta_any any{42};
    entt::meta_any other{any};

    ASSERT_TRUE(any);
    ASSERT_TRUE(other);
    ASSERT_FALSE(other.can_cast<void>());
    ASSERT_TRUE(other.can_cast<int>());
    ASSERT_EQ(other.cast<int>(), 42);
    ASSERT_EQ(std::as_const(other).cast<int>(), 42);
    ASSERT_EQ(other, entt::meta_any{42});
    ASSERT_NE(other, entt::meta_any{0});
}

TEST_F(Meta, MetaAnySBOCopyAssignment) {
    entt::meta_any any{42};
    entt::meta_any other{};

    other = any;

    ASSERT_TRUE(any);
    ASSERT_TRUE(other);
    ASSERT_FALSE(other.can_cast<void>());
    ASSERT_TRUE(other.can_cast<int>());
    ASSERT_EQ(other.cast<int>(), 42);
    ASSERT_EQ(std::as_const(other).cast<int>(), 42);
    ASSERT_EQ(other, entt::meta_any{42});
    ASSERT_NE(other, entt::meta_any{0});
}

TEST_F(Meta, MetaAnySBOMoveConstruction) {
    entt::meta_any any{42};
    entt::meta_any other{std::move(any)};

    ASSERT_FALSE(any);
    ASSERT_TRUE(other);
    ASSERT_FALSE(other.can_cast<void>());
    ASSERT_TRUE(other.can_cast<int>());
    ASSERT_EQ(other.cast<int>(), 42);
    ASSERT_EQ(std::as_const(other).cast<int>(), 42);
    ASSERT_EQ(other, entt::meta_any{42});
    ASSERT_NE(other, entt::meta_any{0});
}

TEST_F(Meta, MetaAnySBOMoveAssignment) {
    entt::meta_any any{42};
    entt::meta_any other{};

    other = std::move(any);

    ASSERT_FALSE(any);
    ASSERT_TRUE(other);
    ASSERT_FALSE(other.can_cast<void>());
    ASSERT_TRUE(other.can_cast<int>());
    ASSERT_EQ(other.cast<int>(), 42);
    ASSERT_EQ(std::as_const(other).cast<int>(), 42);
    ASSERT_EQ(other, entt::meta_any{42});
    ASSERT_NE(other, entt::meta_any{0});
}

TEST_F(Meta, MetaAnyNoSBOCopyConstruction) {
    int value = 42;
    fat_type instance{&value};
    entt::meta_any any{instance};
    entt::meta_any other{any};

    ASSERT_TRUE(any);
    ASSERT_TRUE(other);
    ASSERT_FALSE(other.can_cast<void>());
    ASSERT_TRUE(other.can_cast<fat_type>());
    ASSERT_EQ(other.cast<fat_type>(), instance);
    ASSERT_EQ(std::as_const(other).cast<fat_type>(), instance);
    ASSERT_EQ(other, entt::meta_any{instance});
    ASSERT_NE(other, fat_type{});
}

TEST_F(Meta, MetaAnyNoSBOCopyAssignment) {
    int value = 42;
    fat_type instance{&value};
    entt::meta_any any{instance};
    entt::meta_any other{};

    other = any;

    ASSERT_TRUE(any);
    ASSERT_TRUE(other);
    ASSERT_FALSE(other.can_cast<void>());
    ASSERT_TRUE(other.can_cast<fat_type>());
    ASSERT_EQ(other.cast<fat_type>(), instance);
    ASSERT_EQ(std::as_const(other).cast<fat_type>(), instance);
    ASSERT_EQ(other, entt::meta_any{instance});
    ASSERT_NE(other, fat_type{});
}

TEST_F(Meta, MetaAnyNoSBOMoveConstruction) {
    int value = 42;
    fat_type instance{&value};
    entt::meta_any any{instance};
    entt::meta_any other{std::move(any)};

    ASSERT_FALSE(any);
    ASSERT_TRUE(other);
    ASSERT_FALSE(other.can_cast<void>());
    ASSERT_TRUE(other.can_cast<fat_type>());
    ASSERT_EQ(other.cast<fat_type>(), instance);
    ASSERT_EQ(std::as_const(other).cast<fat_type>(), instance);
    ASSERT_EQ(other, entt::meta_any{instance});
    ASSERT_NE(other, fat_type{});
}

TEST_F(Meta, MetaAnyNoSBOMoveAssignment) {
    int value = 42;
    fat_type instance{&value};
    entt::meta_any any{instance};
    entt::meta_any other{};

    other = std::move(any);

    ASSERT_FALSE(any);
    ASSERT_TRUE(other);
    ASSERT_FALSE(other.can_cast<void>());
    ASSERT_TRUE(other.can_cast<fat_type>());
    ASSERT_EQ(other.cast<fat_type>(), instance);
    ASSERT_EQ(std::as_const(other).cast<fat_type>(), instance);
    ASSERT_EQ(other, entt::meta_any{instance});
    ASSERT_NE(other, fat_type{});
}

TEST_F(Meta, MetaAnySBODestruction) {
    ASSERT_EQ(empty_type::counter, 0);
    { entt::meta_any any{empty_type{}}; }
    ASSERT_EQ(empty_type::counter, 1);
}

TEST_F(Meta, MetaAnyNoSBODestruction) {
    ASSERT_EQ(fat_type::counter, 0);
    { entt::meta_any any{fat_type{}}; }
    ASSERT_EQ(fat_type::counter, 1);
}

TEST_F(Meta, MetaAnySBOSwap) {
    entt::meta_any lhs{'c'};
    entt::meta_any rhs{42};

    std::swap(lhs, rhs);

    ASSERT_TRUE(lhs.can_cast<int>());
    ASSERT_EQ(lhs.cast<int>(), 42);
    ASSERT_TRUE(rhs.can_cast<char>());
    ASSERT_EQ(rhs.cast<char>(), 'c');
}

TEST_F(Meta, MetaAnyNoSBOSwap) {
    int i, j;
    entt::meta_any lhs{fat_type{&i}};
    entt::meta_any rhs{fat_type{&j}};

    std::swap(lhs, rhs);

    ASSERT_EQ(lhs.cast<fat_type>().foo, &j);
    ASSERT_EQ(rhs.cast<fat_type>().bar, &i);
}

TEST_F(Meta, MetaAnySBOWithNoSBOSwap) {
    int value = 42;
    entt::meta_any lhs{fat_type{&value}};
    entt::meta_any rhs{'c'};

    std::swap(lhs, rhs);

    ASSERT_TRUE(lhs.can_cast<char>());
    ASSERT_EQ(lhs.cast<char>(), 'c');
    ASSERT_TRUE(rhs.can_cast<fat_type>());
    ASSERT_EQ(rhs.cast<fat_type>().foo, &value);
    ASSERT_EQ(rhs.cast<fat_type>().bar, &value);
}

TEST_F(Meta, MetaAnySBOWithEmptySwap) {
    entt::meta_any lhs{'c'};
    entt::meta_any rhs{};

    std::swap(lhs, rhs);

    ASSERT_FALSE(lhs);
    ASSERT_TRUE(rhs.can_cast<char>());
    ASSERT_EQ(rhs.cast<char>(), 'c');

    std::swap(lhs, rhs);

    ASSERT_FALSE(rhs);
    ASSERT_TRUE(lhs.can_cast<char>());
    ASSERT_EQ(lhs.cast<char>(), 'c');
}

TEST_F(Meta, MetaAnyNoSBOWithEmptySwap) {
    int i;
    entt::meta_any lhs{fat_type{&i}};
    entt::meta_any rhs{};

    std::swap(lhs, rhs);

    ASSERT_EQ(rhs.cast<fat_type>().bar, &i);

    std::swap(lhs, rhs);

    ASSERT_EQ(lhs.cast<fat_type>().bar, &i);
}

TEST_F(Meta, MetaAnyComparable) {
    entt::meta_any any{'c'};

    ASSERT_EQ(any, any);
    ASSERT_EQ(any, entt::meta_any{'c'});
    ASSERT_NE(any, entt::meta_any{'a'});
    ASSERT_NE(any, entt::meta_any{});

    ASSERT_TRUE(any == any);
    ASSERT_TRUE(any == entt::meta_any{'c'});
    ASSERT_FALSE(any == entt::meta_any{'a'});
    ASSERT_TRUE(any != entt::meta_any{'a'});
    ASSERT_TRUE(any != entt::meta_any{});
}

TEST_F(Meta, MetaAnyNotComparable) {
    entt::meta_any any{not_comparable_type{}};

    ASSERT_EQ(any, any);
    ASSERT_NE(any, entt::meta_any{not_comparable_type{}});
    ASSERT_NE(any, entt::meta_any{});

    ASSERT_TRUE(any == any);
    ASSERT_FALSE(any == entt::meta_any{not_comparable_type{}});
    ASSERT_TRUE(any != entt::meta_any{});
}

TEST_F(Meta, MetaAnyCast) {
    entt::meta_any any{derived_type{}};
    entt::meta_handle handle{any};

    ASSERT_TRUE(any);
    ASSERT_EQ(any.type(), entt::resolve<derived_type>());
    ASSERT_FALSE(any.can_cast<void>());
    ASSERT_TRUE(any.can_cast<base_type>());
    ASSERT_TRUE(any.can_cast<derived_type>());
    ASSERT_EQ(&any.cast<base_type>(), handle.try_cast<base_type>());
    ASSERT_EQ(&any.cast<derived_type>(), handle.try_cast<derived_type>());
    ASSERT_EQ(&std::as_const(any).cast<base_type>(), handle.try_cast<base_type>());
    ASSERT_EQ(&std::as_const(any).cast<derived_type>(), handle.try_cast<derived_type>());
}

TEST_F(Meta, MetaAnyConvert) {
    entt::meta_any any{42.};

    ASSERT_TRUE(any);
    ASSERT_EQ(any.type(), entt::resolve<double>());
    ASSERT_FALSE(any.can_convert<char>());
    ASSERT_TRUE(any.can_convert<double>());
    ASSERT_TRUE(any.can_convert<int>());

    ASSERT_TRUE(any.convert<double>());
    ASSERT_FALSE(any.convert<char>());

    ASSERT_EQ(any.type(), entt::resolve<double>());
    ASSERT_EQ(any.cast<double>(), 42.);

    ASSERT_TRUE(any.convert<int>());

    ASSERT_EQ(any.type(), entt::resolve<int>());
    ASSERT_EQ(any.cast<int>(), 42);
}

TEST_F(Meta, MetaAnyConstConvert) {
    const entt::meta_any any{42.};

    ASSERT_TRUE(any);
    ASSERT_EQ(any.type(), entt::resolve<double>());
    ASSERT_FALSE(any.can_convert<char>());
    ASSERT_TRUE(any.can_convert<double>());
    ASSERT_TRUE(any.can_convert<int>());

    ASSERT_TRUE(any.convert<double>());
    ASSERT_FALSE(any.convert<char>());

    ASSERT_EQ(any.type(), entt::resolve<double>());
    ASSERT_EQ(any.cast<double>(), 42.);

    auto other = any.convert<int>();

    ASSERT_EQ(any.type(), entt::resolve<double>());
    ASSERT_EQ(any.cast<double>(), 42.);
    ASSERT_EQ(other.type(), entt::resolve<int>());
    ASSERT_EQ(other.cast<int>(), 42);
}

TEST_F(Meta, MetaHandleFromObject) {
    empty_type empty{};
    entt::meta_handle handle{empty};

    ASSERT_TRUE(handle);
    ASSERT_EQ(handle.type(), entt::resolve<empty_type>());
    ASSERT_EQ(handle.try_cast<void>(), nullptr);
    ASSERT_EQ(handle.try_cast<empty_type>(), &empty);
    ASSERT_EQ(std::as_const(handle).try_cast<empty_type>(), &empty);
    ASSERT_EQ(handle.data(), &empty);
    ASSERT_EQ(std::as_const(handle).data(), &empty);
}

TEST_F(Meta, MetaHandleFromMetaAny) {
    entt::meta_any any{42};
    entt::meta_handle handle{any};

    ASSERT_TRUE(handle);
    ASSERT_EQ(handle.type(), entt::resolve<int>());
    ASSERT_EQ(handle.try_cast<void>(), nullptr);
    ASSERT_EQ(handle.try_cast<int>(), any.data());
    ASSERT_EQ(std::as_const(handle).try_cast<int>(), any.data());
    ASSERT_EQ(handle.data(), any.data());
    ASSERT_EQ(std::as_const(handle).data(), any.data());
}

TEST_F(Meta, MetaHandleEmpty) {
    entt::meta_handle handle{};

    ASSERT_FALSE(handle);
    ASSERT_FALSE(handle.type());
    ASSERT_EQ(handle.try_cast<void>(), nullptr);
    ASSERT_EQ(handle.try_cast<empty_type>(), nullptr);
    ASSERT_EQ(handle.data(), nullptr);
    ASSERT_EQ(std::as_const(handle).data(), nullptr);
}

TEST_F(Meta, MetaHandleTryCast) {
    derived_type derived{};
    base_type *base = &derived;
    entt::meta_handle handle{derived};

    ASSERT_TRUE(handle);
    ASSERT_EQ(handle.type(), entt::resolve<derived_type>());
    ASSERT_EQ(handle.try_cast<void>(), nullptr);
    ASSERT_EQ(handle.try_cast<base_type>(), base);
    ASSERT_EQ(handle.try_cast<derived_type>(), &derived);
    ASSERT_EQ(std::as_const(handle).try_cast<base_type>(), base);
    ASSERT_EQ(std::as_const(handle).try_cast<derived_type>(), &derived);
    ASSERT_EQ(handle.data(), &derived);
    ASSERT_EQ(std::as_const(handle).data(), &derived);
}

TEST_F(Meta, MetaProp) {
    auto prop = entt::resolve<char>().prop(properties::prop_int);

    ASSERT_TRUE(prop);
    ASSERT_NE(prop, entt::meta_prop{});
    ASSERT_EQ(prop.key(), properties::prop_int);
    ASSERT_EQ(prop.value(), 42);
}

TEST_F(Meta, MetaBase) {
    auto base = entt::resolve<derived_type>().base("base");
    derived_type derived{};

    ASSERT_TRUE(base);
    ASSERT_NE(base, entt::meta_base{});
    ASSERT_EQ(base.parent(), entt::resolve("derived"));
    ASSERT_EQ(base.type(), entt::resolve<base_type>());
    ASSERT_EQ(base.cast(&derived), static_cast<base_type *>(&derived));
}

TEST_F(Meta, MetaConv) {
    auto conv = entt::resolve<double>().conv<int>();
    double value = 3.;

    ASSERT_TRUE(conv);
    ASSERT_NE(conv, entt::meta_conv{});
    ASSERT_EQ(conv.parent(), entt::resolve<double>());
    ASSERT_EQ(conv.type(), entt::resolve<int>());

    auto any = conv.convert(&value);

    ASSERT_TRUE(any);
    ASSERT_EQ(any.type(), entt::resolve<int>());
    ASSERT_EQ(any.cast<int>(), 3);
}

TEST_F(Meta, MetaCtor) {
    auto ctor = entt::resolve<derived_type>().ctor<const base_type &, int, char>();

    ASSERT_TRUE(ctor);
    ASSERT_NE(ctor, entt::meta_ctor{});
    ASSERT_EQ(ctor.parent(), entt::resolve("derived"));
    ASSERT_EQ(ctor.size(), entt::meta_ctor::size_type{3});
    ASSERT_EQ(ctor.arg(entt::meta_ctor::size_type{0}), entt::resolve<base_type>());
    ASSERT_EQ(ctor.arg(entt::meta_ctor::size_type{1}), entt::resolve<int>());
    ASSERT_EQ(ctor.arg(entt::meta_ctor::size_type{2}), entt::resolve<char>());
    ASSERT_FALSE(ctor.arg(entt::meta_ctor::size_type{3}));

    auto any = ctor.invoke(base_type{}, 42, 'c');
    auto empty = ctor.invoke();

    ASSERT_FALSE(empty);
    ASSERT_TRUE(any);
    ASSERT_TRUE(any.can_cast<derived_type>());
    ASSERT_EQ(any.cast<derived_type>().i, 42);
    ASSERT_EQ(any.cast<derived_type>().c, 'c');

    ctor.prop([](auto prop) {
        ASSERT_TRUE(prop);
        ASSERT_EQ(prop.key(), properties::prop_bool);
        ASSERT_EQ(prop.value(), false);
    });

    ASSERT_FALSE(ctor.prop(properties::prop_int));

    auto prop = ctor.prop(properties::prop_bool);

    ASSERT_TRUE(prop);
    ASSERT_EQ(prop.key(), properties::prop_bool);
    ASSERT_EQ(prop.value(), false);
}

TEST_F(Meta, MetaCtorFunc) {
    auto ctor = entt::resolve<derived_type>().ctor<const base_type &, int>();

    ASSERT_TRUE(ctor);
    ASSERT_EQ(ctor.parent(), entt::resolve("derived"));
    ASSERT_EQ(ctor.size(), entt::meta_ctor::size_type{2});
    ASSERT_EQ(ctor.arg(entt::meta_ctor::size_type{0}), entt::resolve<base_type>());
    ASSERT_EQ(ctor.arg(entt::meta_ctor::size_type{1}), entt::resolve<int>());
    ASSERT_FALSE(ctor.arg(entt::meta_ctor::size_type{2}));

    auto any = ctor.invoke(derived_type{}, 42);
    auto empty = ctor.invoke(3, 'c');

    ASSERT_FALSE(empty);
    ASSERT_TRUE(any);
    ASSERT_TRUE(any.can_cast<derived_type>());
    ASSERT_EQ(any.cast<derived_type>().i, 42);
    ASSERT_EQ(any.cast<derived_type>().c, 'c');

    ctor.prop([](auto prop) {
        ASSERT_TRUE(prop);
        ASSERT_EQ(prop.key(), properties::prop_int);
        ASSERT_EQ(prop.value(), 42);
    });

    ASSERT_FALSE(ctor.prop(properties::prop_bool));

    auto prop = ctor.prop(properties::prop_int);

    ASSERT_TRUE(prop);
    ASSERT_EQ(prop.key(), properties::prop_int);
    ASSERT_EQ(prop.value(), 42);
}

TEST_F(Meta, MetaCtorMetaAnyArgs) {
    auto ctor = entt::resolve<derived_type>().ctor<const base_type &, int, char>();
    auto any = ctor.invoke(base_type{}, entt::meta_any{42}, entt::meta_any{'c'});

    ASSERT_TRUE(any);
    ASSERT_TRUE(any.can_cast<derived_type>());
    ASSERT_EQ(any.cast<derived_type>().i, 42);
    ASSERT_EQ(any.cast<derived_type>().c, 'c');
}

TEST_F(Meta, MetaCtorInvalidArgs) {
    auto ctor = entt::resolve<derived_type>().ctor<const base_type &, int, char>();
    ASSERT_FALSE(ctor.invoke(base_type{}, entt::meta_any{'c'}, entt::meta_any{42}));
}

TEST_F(Meta, MetaCtorCastAndConvert) {
    auto ctor = entt::resolve<derived_type>().ctor<const base_type &, int, char>();
    auto any = ctor.invoke(entt::meta_any{derived_type{}}, entt::meta_any{42.}, entt::meta_any{'c'});

    ASSERT_TRUE(any);
    ASSERT_TRUE(any.can_cast<derived_type>());
    ASSERT_EQ(any.cast<derived_type>().i, 42);
    ASSERT_EQ(any.cast<derived_type>().c, 'c');
}

TEST_F(Meta, MetaCtorFuncMetaAnyArgs) {
    auto ctor = entt::resolve<derived_type>().ctor<const base_type &, int>();
    auto any = ctor.invoke(base_type{}, entt::meta_any{42});

    ASSERT_TRUE(any);
    ASSERT_TRUE(any.can_cast<derived_type>());
    ASSERT_EQ(any.cast<derived_type>().i, 42);
    ASSERT_EQ(any.cast<derived_type>().c, 'c');
}

TEST_F(Meta, MetaCtorFuncInvalidArgs) {
    auto ctor = entt::resolve<derived_type>().ctor<const base_type &, int>();
    ASSERT_FALSE(ctor.invoke(base_type{}, entt::meta_any{'c'}));
}

TEST_F(Meta, MetaCtorFuncCastAndConvert) {
    auto ctor = entt::resolve<derived_type>().ctor<const base_type &, int>();
    auto any = ctor.invoke(entt::meta_any{derived_type{}}, entt::meta_any{42.});

    ASSERT_TRUE(any);
    ASSERT_TRUE(any.can_cast<derived_type>());
    ASSERT_EQ(any.cast<derived_type>().i, 42);
    ASSERT_EQ(any.cast<derived_type>().c, 'c');
}

TEST_F(Meta, MetaDtor) {
    auto dtor = entt::resolve<empty_type>().dtor();
    empty_type empty{};

    ASSERT_TRUE(dtor);
    ASSERT_NE(dtor, entt::meta_dtor{});
    ASSERT_EQ(dtor.parent(), entt::resolve("empty"));
    ASSERT_EQ(empty_type::counter, 0);
    ASSERT_TRUE(dtor.invoke(empty));
    ASSERT_EQ(empty_type::counter, 1);
}

TEST_F(Meta, MetaDtorMetaAnyArg) {
    auto dtor = entt::resolve<empty_type>().dtor();
    entt::meta_any any{empty_type{}};

    ASSERT_EQ(empty_type::counter, 0);
    ASSERT_TRUE(dtor.invoke(any));
    ASSERT_EQ(empty_type::counter, 1);
}

TEST_F(Meta, MetaDtorMetaAnyInvalidArg) {
    ASSERT_FALSE(entt::resolve<empty_type>().dtor().invoke(int{}));
}


TEST_F(Meta, MetaData) {
    auto data = entt::resolve<data_type>().data("i");
    data_type instance{};

    ASSERT_TRUE(data);
    ASSERT_NE(data, entt::meta_data{});
    ASSERT_EQ(data.parent(), entt::resolve("data"));
    ASSERT_EQ(data.type(), entt::resolve<int>());
    ASSERT_STREQ(data.name(), "i");
    ASSERT_FALSE(data.is_const());
    ASSERT_FALSE(data.is_static());
    ASSERT_EQ(data.get(instance).cast<int>(), 0);
    ASSERT_TRUE(data.set(instance, 42));
    ASSERT_EQ(data.get(instance).cast<int>(), 42);

    data.prop([](auto prop) {
        ASSERT_TRUE(prop);
        ASSERT_EQ(prop.key(), properties::prop_int);
        ASSERT_EQ(prop.value(), 0);
    });

    ASSERT_FALSE(data.prop(properties::prop_bool));

    auto prop = data.prop(properties::prop_int);

    ASSERT_TRUE(prop);
    ASSERT_EQ(prop.key(), properties::prop_int);
    ASSERT_EQ(prop.value(), 0);
}

TEST_F(Meta, MetaDataConst) {
    auto data = entt::resolve<data_type>().data("j");
    data_type instance{};

    ASSERT_TRUE(data);
    ASSERT_EQ(data.parent(), entt::resolve("data"));
    ASSERT_EQ(data.type(), entt::resolve<int>());
    ASSERT_STREQ(data.name(), "j");
    ASSERT_TRUE(data.is_const());
    ASSERT_FALSE(data.is_static());
    ASSERT_EQ(data.get(instance).cast<int>(), 1);
    ASSERT_FALSE(data.set(instance, 42));
    ASSERT_EQ(data.get(instance).cast<int>(), 1);

    data.prop([](auto prop) {
        ASSERT_TRUE(prop);
        ASSERT_EQ(prop.key(), properties::prop_int);
        ASSERT_EQ(prop.value(), 1);
    });

    ASSERT_FALSE(data.prop(properties::prop_bool));

    auto prop = data.prop(properties::prop_int);

    ASSERT_TRUE(prop);
    ASSERT_EQ(prop.key(), properties::prop_int);
    ASSERT_EQ(prop.value(), 1);
}

TEST_F(Meta, MetaDataStatic) {
    auto data = entt::resolve<data_type>().data("h");

    ASSERT_TRUE(data);
    ASSERT_EQ(data.parent(), entt::resolve("data"));
    ASSERT_EQ(data.type(), entt::resolve<int>());
    ASSERT_STREQ(data.name(), "h");
    ASSERT_FALSE(data.is_const());
    ASSERT_TRUE(data.is_static());
    ASSERT_EQ(data.get({}).cast<int>(), 2);
    ASSERT_TRUE(data.set({}, 42));
    ASSERT_EQ(data.get({}).cast<int>(), 42);

    data.prop([](auto prop) {
        ASSERT_TRUE(prop);
        ASSERT_EQ(prop.key(), properties::prop_int);
        ASSERT_EQ(prop.value(), 2);
    });

    ASSERT_FALSE(data.prop(properties::prop_bool));

    auto prop = data.prop(properties::prop_int);

    ASSERT_TRUE(prop);
    ASSERT_EQ(prop.key(), properties::prop_int);
    ASSERT_EQ(prop.value(), 2);
}

TEST_F(Meta, MetaDataConstStatic) {
    auto data = entt::resolve<data_type>().data("k");

    ASSERT_TRUE(data);
    ASSERT_EQ(data.parent(), entt::resolve("data"));
    ASSERT_EQ(data.type(), entt::resolve<int>());
    ASSERT_STREQ(data.name(), "k");
    ASSERT_TRUE(data.is_const());
    ASSERT_TRUE(data.is_static());
    ASSERT_EQ(data.get({}).cast<int>(), 3);
    ASSERT_FALSE(data.set({}, 42));
    ASSERT_EQ(data.get({}).cast<int>(), 3);

    data.prop([](auto prop) {
        ASSERT_TRUE(prop);
        ASSERT_EQ(prop.key(), properties::prop_int);
        ASSERT_EQ(prop.value(), 3);
    });

    ASSERT_FALSE(data.prop(properties::prop_bool));

    auto prop = data.prop(properties::prop_int);

    ASSERT_TRUE(prop);
    ASSERT_EQ(prop.key(), properties::prop_int);
    ASSERT_EQ(prop.value(), 3);
}

TEST_F(Meta, MetaDataGetMetaAnyArg) {
    auto data = entt::resolve<data_type>().data("i");
    entt::meta_any any{data_type{}};
    any.cast<data_type>().i = 99;
    const auto value = data.get(any);

    ASSERT_TRUE(value);
    ASSERT_TRUE(value.can_cast<int>());
    ASSERT_EQ(value.cast<int>(), 99);
}

TEST_F(Meta, MetaDataGetInvalidArg) {
    ASSERT_FALSE(entt::resolve<data_type>().data("i").get(0));
}

TEST_F(Meta, MetaDataSetMetaAnyArg) {
    auto data = entt::resolve<data_type>().data("i");
    entt::meta_any any{data_type{}};
    entt::meta_any value{42};

    ASSERT_EQ(any.cast<data_type>().i, 0);
    ASSERT_TRUE(data.set(any, value));
    ASSERT_EQ(any.cast<data_type>().i, 42);
}

TEST_F(Meta, MetaDataSetInvalidArg) {
    ASSERT_FALSE(entt::resolve<data_type>().data("i").set({}, 'c'));
}

TEST_F(Meta, MetaDataSetCast) {
    auto data = entt::resolve<data_type>().data("empty");
    data_type instance{};

    ASSERT_EQ(empty_type::counter, 0);
    ASSERT_TRUE(data.set(instance, fat_type{}));
    ASSERT_EQ(empty_type::counter, 1);
}

TEST_F(Meta, MetaDataSetConvert) {
    auto data = entt::resolve<data_type>().data("i");
    data_type instance{};

    ASSERT_EQ(instance.i, 0);
    ASSERT_TRUE(data.set(instance, 3.));
    ASSERT_EQ(instance.i, 3);
}

TEST_F(Meta, MetaDataSetterGetterAsFreeFunctions) {
    auto data = entt::resolve<setter_getter_type>().data("x");
    setter_getter_type instance{};

    ASSERT_TRUE(data);
    ASSERT_NE(data, entt::meta_data{});
    ASSERT_EQ(data.parent(), entt::resolve("setter_getter"));
    ASSERT_EQ(data.type(), entt::resolve<int>());
    ASSERT_STREQ(data.name(), "x");
    ASSERT_FALSE(data.is_const());
    ASSERT_FALSE(data.is_static());
    ASSERT_EQ(data.get(instance).cast<int>(), 0);
    ASSERT_TRUE(data.set(instance, 42));
    ASSERT_EQ(data.get(instance).cast<int>(), 42);
}

TEST_F(Meta, MetaDataSetterGetterAsMemberFunctions) {
    auto data = entt::resolve<setter_getter_type>().data("y");
    setter_getter_type instance{};

    ASSERT_TRUE(data);
    ASSERT_NE(data, entt::meta_data{});
    ASSERT_EQ(data.parent(), entt::resolve("setter_getter"));
    ASSERT_EQ(data.type(), entt::resolve<int>());
    ASSERT_STREQ(data.name(), "y");
    ASSERT_FALSE(data.is_const());
    ASSERT_FALSE(data.is_static());
    ASSERT_EQ(data.get(instance).cast<int>(), 0);
    ASSERT_TRUE(data.set(instance, 42));
    ASSERT_EQ(data.get(instance).cast<int>(), 42);
}

TEST_F(Meta, MetaDataSetterGetterWithRefAsMemberFunctions) {
    auto data = entt::resolve<setter_getter_type>().data("w");
    setter_getter_type instance{};

    ASSERT_TRUE(data);
    ASSERT_NE(data, entt::meta_data{});
    ASSERT_EQ(data.parent(), entt::resolve("setter_getter"));
    ASSERT_EQ(data.type(), entt::resolve<int>());
    ASSERT_STREQ(data.name(), "w");
    ASSERT_FALSE(data.is_const());
    ASSERT_FALSE(data.is_static());
    ASSERT_EQ(data.get(instance).cast<int>(), 0);
    ASSERT_TRUE(data.set(instance, 42));
    ASSERT_EQ(data.get(instance).cast<int>(), 42);
}

TEST_F(Meta, MetaDataSetterGetterMixed) {
    auto data = entt::resolve<setter_getter_type>().data("z");
    setter_getter_type instance{};

    ASSERT_TRUE(data);
    ASSERT_NE(data, entt::meta_data{});
    ASSERT_EQ(data.parent(), entt::resolve("setter_getter"));
    ASSERT_EQ(data.type(), entt::resolve<int>());
    ASSERT_STREQ(data.name(), "z");
    ASSERT_FALSE(data.is_const());
    ASSERT_FALSE(data.is_static());
    ASSERT_EQ(data.get(instance).cast<int>(), 0);
    ASSERT_TRUE(data.set(instance, 42));
    ASSERT_EQ(data.get(instance).cast<int>(), 42);
}

TEST_F(Meta, MetaFunc) {
    auto func = entt::resolve<func_type>().func("f2");
    func_type instance{};

    ASSERT_TRUE(func);
    ASSERT_NE(func, entt::meta_func{});
    ASSERT_EQ(func.parent(), entt::resolve("func"));
    ASSERT_STREQ(func.name(), "f2");
    ASSERT_EQ(func.size(), entt::meta_func::size_type{2});
    ASSERT_FALSE(func.is_const());
    ASSERT_FALSE(func.is_static());
    ASSERT_EQ(func.ret(), entt::resolve<int>());
    ASSERT_EQ(func.arg(entt::meta_func::size_type{0}), entt::resolve<int>());
    ASSERT_EQ(func.arg(entt::meta_func::size_type{1}), entt::resolve<int>());
    ASSERT_FALSE(func.arg(entt::meta_func::size_type{2}));

    auto any = func.invoke(instance, 3, 2);
    auto empty = func.invoke(instance);

    ASSERT_FALSE(empty);
    ASSERT_TRUE(any);
    ASSERT_EQ(any.type(), entt::resolve<int>());
    ASSERT_EQ(any.cast<int>(), 4);
    ASSERT_EQ(func_type::value, 3);

    func.prop([](auto prop) {
        ASSERT_TRUE(prop);
        ASSERT_EQ(prop.key(), properties::prop_bool);
        ASSERT_FALSE(prop.value().template cast<bool>());
    });

    ASSERT_FALSE(func.prop(properties::prop_int));

    auto prop = func.prop(properties::prop_bool);

    ASSERT_TRUE(prop);
    ASSERT_EQ(prop.key(), properties::prop_bool);
    ASSERT_FALSE(prop.value().cast<bool>());
}

TEST_F(Meta, MetaFuncConst) {
    auto func = entt::resolve<func_type>().func("f1");
    func_type instance{};

    ASSERT_TRUE(func);
    ASSERT_EQ(func.parent(), entt::resolve("func"));
    ASSERT_STREQ(func.name(), "f1");
    ASSERT_EQ(func.size(), entt::meta_func::size_type{1});
    ASSERT_TRUE(func.is_const());
    ASSERT_FALSE(func.is_static());
    ASSERT_EQ(func.ret(), entt::resolve<int>());
    ASSERT_EQ(func.arg(entt::meta_func::size_type{0}), entt::resolve<int>());
    ASSERT_FALSE(func.arg(entt::meta_func::size_type{1}));

    auto any = func.invoke(instance, 4);
    auto empty = func.invoke(instance, 'c');

    ASSERT_FALSE(empty);
    ASSERT_TRUE(any);
    ASSERT_EQ(any.type(), entt::resolve<int>());
    ASSERT_EQ(any.cast<int>(), 16);

    func.prop([](auto prop) {
        ASSERT_TRUE(prop);
        ASSERT_EQ(prop.key(), properties::prop_bool);
        ASSERT_FALSE(prop.value().template cast<bool>());
    });

    ASSERT_FALSE(func.prop(properties::prop_int));

    auto prop = func.prop(properties::prop_bool);

    ASSERT_TRUE(prop);
    ASSERT_EQ(prop.key(), properties::prop_bool);
    ASSERT_FALSE(prop.value().cast<bool>());
}

TEST_F(Meta, MetaFuncRetVoid) {
    auto func = entt::resolve<func_type>().func("g");
    func_type instance{};

    ASSERT_TRUE(func);
    ASSERT_EQ(func.parent(), entt::resolve("func"));
    ASSERT_STREQ(func.name(), "g");
    ASSERT_EQ(func.size(), entt::meta_func::size_type{1});
    ASSERT_FALSE(func.is_const());
    ASSERT_FALSE(func.is_static());
    ASSERT_EQ(func.ret(), entt::resolve<void>());
    ASSERT_EQ(func.arg(entt::meta_func::size_type{0}), entt::resolve<int>());
    ASSERT_FALSE(func.arg(entt::meta_func::size_type{1}));

    auto any = func.invoke(instance, 5);

    ASSERT_FALSE(any);
    ASSERT_EQ(func_type::value, 25);

    func.prop([](auto prop) {
        ASSERT_TRUE(prop);
        ASSERT_EQ(prop.key(), properties::prop_bool);
        ASSERT_FALSE(prop.value().template cast<bool>());
    });

    ASSERT_FALSE(func.prop(properties::prop_int));

    auto prop = func.prop(properties::prop_bool);

    ASSERT_TRUE(prop);
    ASSERT_EQ(prop.key(), properties::prop_bool);
    ASSERT_FALSE(prop.value().cast<bool>());
}

TEST_F(Meta, MetaFuncStatic) {
    auto func = entt::resolve<func_type>().func("h");

    ASSERT_TRUE(func);
    ASSERT_EQ(func.parent(), entt::resolve("func"));
    ASSERT_STREQ(func.name(), "h");
    ASSERT_EQ(func.size(), entt::meta_func::size_type{1});
    ASSERT_FALSE(func.is_const());
    ASSERT_TRUE(func.is_static());
    ASSERT_EQ(func.ret(), entt::resolve<int>());
    ASSERT_EQ(func.arg(entt::meta_func::size_type{0}), entt::resolve<int>());
    ASSERT_FALSE(func.arg(entt::meta_func::size_type{1}));

    auto any = func.invoke({}, 42);
    auto empty = func.invoke({}, 'c');

    ASSERT_FALSE(empty);
    ASSERT_TRUE(any);
    ASSERT_EQ(any.type(), entt::resolve<int>());
    ASSERT_EQ(any.cast<int>(), 42);

    func.prop([](auto prop) {
        ASSERT_TRUE(prop);
        ASSERT_EQ(prop.key(), properties::prop_bool);
        ASSERT_FALSE(prop.value().template cast<bool>());
    });

    ASSERT_FALSE(func.prop(properties::prop_int));

    auto prop = func.prop(properties::prop_bool);

    ASSERT_TRUE(prop);
    ASSERT_EQ(prop.key(), properties::prop_bool);
    ASSERT_FALSE(prop.value().cast<bool>());
}

TEST_F(Meta, MetaFuncStaticRetVoid) {
    auto func = entt::resolve<func_type>().func("k");

    ASSERT_TRUE(func);
    ASSERT_EQ(func.parent(), entt::resolve("func"));
    ASSERT_STREQ(func.name(), "k");
    ASSERT_EQ(func.size(), entt::meta_func::size_type{1});
    ASSERT_FALSE(func.is_const());
    ASSERT_TRUE(func.is_static());
    ASSERT_EQ(func.ret(), entt::resolve<void>());
    ASSERT_EQ(func.arg(entt::meta_func::size_type{0}), entt::resolve<int>());
    ASSERT_FALSE(func.arg(entt::meta_func::size_type{1}));

    auto any = func.invoke({}, 42);

    ASSERT_FALSE(any);
    ASSERT_EQ(func_type::value, 42);

    func.prop([](auto *prop) {
        ASSERT_TRUE(prop);
        ASSERT_EQ(prop->key(), properties::prop_bool);
        ASSERT_FALSE(prop->value().template cast<bool>());
    });

    ASSERT_FALSE(func.prop(properties::prop_int));

    auto prop = func.prop(properties::prop_bool);

    ASSERT_TRUE(prop);
    ASSERT_EQ(prop.key(), properties::prop_bool);
    ASSERT_FALSE(prop.value().cast<bool>());
}

TEST_F(Meta, MetaFuncMetaAnyArgs) {
    auto func = entt::resolve<func_type>().func("f1");
    auto any = func.invoke(func_type{}, entt::meta_any{3});

    ASSERT_TRUE(any);
    ASSERT_EQ(any.type(), entt::resolve<int>());
    ASSERT_EQ(any.cast<int>(), 9);
}

TEST_F(Meta, MetaFuncInvalidArgs) {
    auto func = entt::resolve<func_type>().func("f1");
    ASSERT_FALSE(func.invoke(empty_type{}, entt::meta_any{'c'}));
}

TEST_F(Meta, MetaFuncCastAndConvert) {
    auto func = entt::resolve<func_type>().func("f3");
    auto any = func.invoke(func_type{}, derived_type{}, 0, 3.);

    ASSERT_TRUE(any);
    ASSERT_EQ(any.type(), entt::resolve<int>());
    ASSERT_EQ(any.cast<int>(), 9);
}

TEST_F(Meta, MetaType) {
    auto type = entt::resolve<derived_type>();

    ASSERT_TRUE(type);
    ASSERT_NE(type, entt::meta_type{});
    ASSERT_STREQ(type.name(), "derived");

    type.prop([](auto prop) {
        ASSERT_TRUE(prop);
        ASSERT_EQ(prop.key(), properties::prop_int);
        ASSERT_EQ(prop.value(), 99);
    });

    ASSERT_FALSE(type.prop(properties::prop_bool));

    auto prop = type.prop(properties::prop_int);

    ASSERT_TRUE(prop);
    ASSERT_EQ(prop.key(), properties::prop_int);
    ASSERT_EQ(prop.value(), 99);
}

TEST_F(Meta, MetaTypeTraits) {
    ASSERT_TRUE(entt::resolve<void>().is_void());
    ASSERT_TRUE(entt::resolve<bool>().is_integral());
    ASSERT_TRUE(entt::resolve<double>().is_floating_point());
    ASSERT_TRUE(entt::resolve<properties>().is_enum());
    ASSERT_TRUE(entt::resolve<union_type>().is_union());
    ASSERT_TRUE(entt::resolve<derived_type>().is_class());
    ASSERT_TRUE(entt::resolve<int *>().is_pointer());
    ASSERT_TRUE(entt::resolve<decltype(empty_type::destroy)>().is_function());
    ASSERT_TRUE(entt::resolve<decltype(&data_type::i)>().is_member_object_pointer());
    ASSERT_TRUE(entt::resolve<decltype(&func_type::g)>().is_member_function_pointer());
}

TEST_F(Meta, MetaTypeRemovePointer) {
    ASSERT_EQ(entt::resolve<void *>().remove_pointer(), entt::resolve<void>());
    ASSERT_EQ(entt::resolve<int(*)(char, double)>().remove_pointer(), entt::resolve<int(char, double)>());
    ASSERT_EQ(entt::resolve<derived_type>().remove_pointer(), entt::resolve<derived_type>());
}

TEST_F(Meta, MetaTypeBase) {
    auto type = entt::resolve<derived_type>();
    bool iterate = false;

    type.base([&iterate](auto base) {
        ASSERT_EQ(base.type(), entt::resolve<base_type>());
        iterate = true;
    });

    ASSERT_TRUE(iterate);
    ASSERT_EQ(type.base("base").type(), entt::resolve<base_type>());
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
    auto type = entt::resolve<derived_type>();
    int counter{};

    type.ctor([&counter](auto) {
        ++counter;
    });

    ASSERT_EQ(counter, 2);
    ASSERT_TRUE((type.ctor<const base_type &, int>()));
    ASSERT_TRUE((type.ctor<const derived_type &, double>()));
}

TEST_F(Meta, MetaTypeDtor) {
    ASSERT_TRUE(entt::resolve<fat_type>().dtor());
    ASSERT_FALSE(entt::resolve<int>().dtor());
}

TEST_F(Meta, MetaTypeData) {
    auto type = entt::resolve<data_type>();
    int counter{};

    type.data([&counter](auto) {
        ++counter;
    });

    ASSERT_EQ(counter, 5);
    ASSERT_TRUE(type.data("i"));
}

TEST_F(Meta, MetaTypeFunc) {
    auto type = entt::resolve<func_type>();
    int counter{};

    type.func([&counter](auto) {
        ++counter;
    });

    ASSERT_EQ(counter, 6);
    ASSERT_TRUE(type.func("f1"));
}

TEST_F(Meta, MetaTypeConstruct) {
    auto type = entt::resolve<derived_type>();
    auto any = type.construct(base_type{}, 42, 'c');

    ASSERT_TRUE(any);
    ASSERT_TRUE(any.can_cast<derived_type>());
    ASSERT_EQ(any.cast<derived_type>().i, 42);
    ASSERT_EQ(any.cast<derived_type>().c, 'c');
}

TEST_F(Meta, MetaTypeConstructMetaAnyArgs) {
    auto type = entt::resolve<derived_type>();
    auto any = type.construct(entt::meta_any{base_type{}}, entt::meta_any{42}, entt::meta_any{'c'});

    ASSERT_TRUE(any);
    ASSERT_TRUE(any.can_cast<derived_type>());
    ASSERT_EQ(any.cast<derived_type>().i, 42);
    ASSERT_EQ(any.cast<derived_type>().c, 'c');
}

TEST_F(Meta, MetaTypeConstructInvalidArgs) {
    auto type = entt::resolve<derived_type>();
    auto any = type.construct(entt::meta_any{base_type{}}, entt::meta_any{'c'}, entt::meta_any{42});
    ASSERT_FALSE(any);
}

TEST_F(Meta, MetaTypeConstructCastAndConvert) {
    auto type = entt::resolve<derived_type>();
    auto any = type.construct(entt::meta_any{derived_type{}}, entt::meta_any{42.}, entt::meta_any{'c'});

    ASSERT_TRUE(any);
    ASSERT_TRUE(any.can_cast<derived_type>());
    ASSERT_EQ(any.cast<derived_type>().i, 42);
    ASSERT_EQ(any.cast<derived_type>().c, 'c');
}

TEST_F(Meta, MetaTypeDestroyDtor) {
    auto type = entt::resolve<empty_type>();

    ASSERT_EQ(empty_type::counter, 0);
    ASSERT_TRUE(type.destroy(empty_type{}));
    ASSERT_EQ(empty_type::counter, 1);
}

TEST_F(Meta, MetaTypeDestroyDtorInvalidArg) {
    auto type = entt::resolve<empty_type>();

    ASSERT_EQ(empty_type::counter, 0);
    ASSERT_FALSE(type.destroy('c'));
    ASSERT_EQ(empty_type::counter, 0);
}

TEST_F(Meta, MetaTypeDestroyDtorCastAndConvert) {
    auto type = entt::resolve<empty_type>();

    ASSERT_EQ(empty_type::counter, 0);
    ASSERT_FALSE(type.destroy(fat_type{}));
    ASSERT_EQ(empty_type::counter, 0);
    ASSERT_FALSE(entt::resolve<int>().destroy(42.));
}

TEST_F(Meta, MetaTypeDestroyNoDtor) {
    ASSERT_TRUE(entt::resolve<char>().destroy('c'));
}

TEST_F(Meta, MetaTypeDestroyNoDtorInvalidArg) {
    ASSERT_FALSE(entt::resolve<char>().destroy(42));
}

TEST_F(Meta, MetaTypeDestroyNoDtorVoid) {
    ASSERT_FALSE(entt::resolve<void>().destroy({}));
}

TEST_F(Meta, MetaTypeDestroyNoDtorCastAndConvert) {
    ASSERT_FALSE(entt::resolve<int>().destroy(42.));
}

TEST_F(Meta, MetaDataFromBase) {
    auto type = entt::resolve<concrete_type>();
    concrete_type instance;

    ASSERT_TRUE(type.data("i"));
    ASSERT_TRUE(type.data("j"));

    ASSERT_EQ(instance.i, 0);
    ASSERT_EQ(instance.j, char{});
    ASSERT_TRUE(type.data("i").set(instance, 3));
    ASSERT_TRUE(type.data("j").set(instance, 'c'));
    ASSERT_EQ(instance.i, 3);
    ASSERT_EQ(instance.j, 'c');
}

TEST_F(Meta, MetaFuncFromBase) {
    auto type = entt::resolve<concrete_type>();
    auto base = entt::resolve<an_abstract_type>();
    concrete_type instance;

    ASSERT_TRUE(type.func("f"));
    ASSERT_TRUE(type.func("g"));
    ASSERT_TRUE(type.func("h"));

    ASSERT_EQ(type.func("f").parent(), entt::resolve<concrete_type>());
    ASSERT_EQ(type.func("g").parent(), entt::resolve<an_abstract_type>());
    ASSERT_EQ(type.func("h").parent(), entt::resolve<another_abstract_type>());

    ASSERT_EQ(instance.i, 0);
    ASSERT_EQ(instance.j, char{});

    type.func("f").invoke(instance, 3);
    type.func("h").invoke(instance, 'c');

    ASSERT_EQ(instance.i, 9);
    ASSERT_EQ(instance.j, 'c');

    base.func("g").invoke(instance, 3);

    ASSERT_EQ(instance.i, -3);
}

TEST_F(Meta, MetaPropFromBase) {
    auto type = entt::resolve<concrete_type>();
    auto prop_bool = type.prop(properties::prop_bool);
    auto prop_int = type.prop(properties::prop_int);

    ASSERT_TRUE(prop_bool);
    ASSERT_TRUE(prop_int);

    ASSERT_FALSE(prop_bool.value().cast<bool>());
    ASSERT_EQ(prop_int.value().cast<int>(), 42);
}

TEST_F(Meta, AbstractClass) {
    auto type = entt::resolve<an_abstract_type>();
    concrete_type instance;

    ASSERT_EQ(instance.i, 0);

    type.func("f").invoke(instance, 3);

    ASSERT_EQ(instance.i, 3);

    type.func("g").invoke(instance, 3);

    ASSERT_EQ(instance.i, -3);
}

TEST_F(Meta, EnumAndNamedConstants) {
    auto type = entt::resolve<properties>();

    ASSERT_TRUE(type.data("prop_bool"));
    ASSERT_TRUE(type.data("prop_int"));

    ASSERT_EQ(type.data("prop_bool").type(), type);
    ASSERT_EQ(type.data("prop_int").type(), type);

    ASSERT_FALSE(type.data("prop_bool").set({}, properties::prop_int));
    ASSERT_FALSE(type.data("prop_int").set({}, properties::prop_bool));

    ASSERT_EQ(type.data("prop_bool").get({}).cast<properties>(), properties::prop_bool);
    ASSERT_EQ(type.data("prop_int").get({}).cast<properties>(), properties::prop_int);
}

TEST_F(Meta, ArithmeticTypeAndNamedConstants) {
    auto type = entt::resolve<unsigned int>();

    ASSERT_TRUE(type.data("min"));
    ASSERT_TRUE(type.data("max"));

    ASSERT_EQ(type.data("min").type(), type);
    ASSERT_EQ(type.data("max").type(), type);

    ASSERT_FALSE(type.data("min").set({}, 100u));
    ASSERT_FALSE(type.data("max").set({}, 0u));

    ASSERT_EQ(type.data("min").get({}).cast<unsigned int>(), 0u);
    ASSERT_EQ(type.data("max").get({}).cast<unsigned int>(), 100u);
}

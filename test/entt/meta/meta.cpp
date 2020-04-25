#include <utility>
#include <functional>
#include <type_traits>
#include <gtest/gtest.h>
#include <entt/core/hashed_string.hpp>
#include <entt/core/type_info.hpp>
#include <entt/core/utility.hpp>
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

enum class props {
    prop_int,
    prop_value,
    prop_bool,
    key_only,
    prop_list
};

struct empty_type {
    virtual ~empty_type() = default;

    static void destroy(empty_type &) {
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

struct base_type {
    virtual ~base_type() = default;
};

struct derived_type: base_type {
    derived_type() = default;

    derived_type(const base_type &, int value, char character)
        : i{value}, c{character}
    {}

    int f() const { return i; }
    static char g(const derived_type &type) { return type.c; }

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
    int v{0};
};

struct array_type {
    static inline int global[3];
    int local[3];
};

struct func_type {
    int f(const base_type &, int a, int b) { return f(a, b); }
    int f(int a, int b) { value = a; return b*b; }
    int f(int v) const { return v*v; }
    void g(int v) { value = v*v; }

    static int h(int &v) { return (v *= value); }
    static void k(int v) { value = v; }

    int v(int v) const { return (value = v); }
    int & a() const { return value; }

    inline static int value = 0;
};

struct setter_getter_type {
    int value{};

    int setter(int val) { return value = val; }
    int getter() { return value; }

    int setter_with_ref(const int &val) { return value = val; }
    const int & getter_with_ref() { return value; }

    static int static_setter(setter_getter_type &type, int value) { return type.value = value; }
    static int static_getter(const setter_getter_type &type) { return type.value; }
};

struct not_comparable_type {
    bool operator==(const not_comparable_type &) const = delete;
};

struct unmanageable_type {
    unmanageable_type() = default;
    unmanageable_type(const unmanageable_type &) = delete;
    unmanageable_type(unmanageable_type &&) = delete;
    unmanageable_type & operator=(const unmanageable_type &) = delete;
    unmanageable_type & operator=(unmanageable_type &&) = delete;
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

struct Meta: ::testing::Test {
    static void SetUpTestCase() {
        entt::meta<double>().conv<int>();

        entt::meta<char>()
                .alias("char"_hs)
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
                .alias("base"_hs);

        entt::meta<derived_type>()
                .alias("derived"_hs)
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
                .alias("empty"_hs)
                .dtor<&empty_type::destroy>();

        entt::meta<fat_type>()
                .alias("fat"_hs)
                .base<empty_type>()
                .dtor<&fat_type::destroy>();

        entt::meta<data_type>()
                .alias("data"_hs)
                .data<&data_type::i, entt::as_alias_t>("i"_hs)
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
                .alias("array"_hs)
                .data<&array_type::global>("global"_hs)
                .data<&array_type::local>("local"_hs);

        entt::meta<func_type>()
                .alias("func"_hs)
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
                .func<&func_type::a, entt::as_alias_t>("a"_hs);

        entt::meta<setter_getter_type>()
                .alias("setter_getter"_hs)
                .data<&setter_getter_type::static_setter, &setter_getter_type::static_getter>("x"_hs)
                .data<&setter_getter_type::setter, &setter_getter_type::getter>("y"_hs)
                .data<&setter_getter_type::static_setter, &setter_getter_type::getter>("z"_hs)
                .data<&setter_getter_type::setter_with_ref, &setter_getter_type::getter_with_ref>("w"_hs);

        entt::meta<an_abstract_type>()
                .alias("an_abstract_type"_hs)
                    .prop(props::prop_bool, false)
                .data<&an_abstract_type::i>("i"_hs)
                .func<&an_abstract_type::f>("f"_hs)
                .func<&an_abstract_type::g>("g"_hs);

        entt::meta<another_abstract_type>()
                .alias("another_abstract_type"_hs)
                    .prop(props::prop_int, 42)
                .data<&another_abstract_type::j>("j"_hs)
                .func<&another_abstract_type::h>("h"_hs);

        entt::meta<concrete_type>()
                .alias("concrete"_hs)
                .base<an_abstract_type>()
                .base<another_abstract_type>()
                .func<&concrete_type::f>("f"_hs);
    }

    static void SetUpAfterUnregistration() {
        entt::meta<double>().conv<float>();

        entt::meta<props>()
                .data<props::prop_bool>("prop_bool"_hs)
                    .prop(props::prop_int, 0)
                    .prop(props::prop_value, 3);

        entt::meta<derived_type>()
                .alias("my_type"_hs)
                    .prop(props::prop_bool, false)
                .ctor<>();

        entt::meta<another_abstract_type>()
                .alias("your_type"_hs)
                .data<&another_abstract_type::j>("a_data_member"_hs)
                .func<&another_abstract_type::h>("a_member_function"_hs);
    }

    void SetUp() override {
        empty_type::counter = 0;
        func_type::value = 0;
    }
};

TEST_F(Meta, Resolve) {
    ASSERT_EQ(entt::resolve<derived_type>(), entt::resolve_id("derived"_hs));
    ASSERT_EQ(entt::resolve<derived_type>(), entt::resolve_type(entt::type_info<derived_type>::id()));
    // it could be "char"_hs rather than entt::hashed_string::value("char") if it weren't for a bug in VS2017
    ASSERT_EQ(entt::resolve_if([](auto type) { return type.id() == entt::hashed_string::value("char"); }), entt::resolve<char>());

    bool found = false;

    entt::resolve([&found](auto type) {
        found = found || type == entt::resolve<derived_type>();
    });

    ASSERT_TRUE(found);
}

TEST_F(Meta, MetaAnySBO) {
    entt::meta_any any{'c'};

    ASSERT_TRUE(any);
    ASSERT_FALSE(any.try_cast<std::size_t>());
    ASSERT_EQ(any.cast<char>(), 'c');
    ASSERT_NE(any.data(), nullptr);
    ASSERT_EQ(any, entt::meta_any{'c'});
    ASSERT_NE(entt::meta_any{'h'}, any);
}

TEST_F(Meta, MetaAnyNoSBO) {
    int value = 42;
    fat_type instance{&value};
    entt::meta_any any{instance};

    ASSERT_TRUE(any);
    ASSERT_FALSE(any.try_cast<std::size_t>());
    ASSERT_EQ(any.cast<fat_type>(), instance);
    ASSERT_NE(any.data(), nullptr);
    ASSERT_EQ(any, entt::meta_any{instance});
    ASSERT_NE(fat_type{}, any);
}

TEST_F(Meta, MetaAnyEmpty) {
    entt::meta_any any{};

    ASSERT_FALSE(any);
    ASSERT_FALSE(any.type());
    ASSERT_FALSE(any.try_cast<std::size_t>());
    ASSERT_EQ(any.data(), nullptr);
    ASSERT_EQ(any, entt::meta_any{});
    ASSERT_NE(entt::meta_any{'c'}, any);
}

TEST_F(Meta, MetaAnySBOInPlaceTypeConstruction) {
    entt::meta_any any{std::in_place_type<int>, 42};

    ASSERT_TRUE(any);
    ASSERT_FALSE(any.try_cast<std::size_t>());
    ASSERT_EQ(any.cast<int>(), 42);
    ASSERT_NE(any.data(), nullptr);
    ASSERT_EQ(any, (entt::meta_any{std::in_place_type<int>, 42}));
    ASSERT_EQ(any, entt::meta_any{42});
    ASSERT_NE(entt::meta_any{3}, any);
}

TEST_F(Meta, MetaAnySBOAsAliasConstruction) {
    int value = 3;
    int other = 42;
    entt::meta_any any{std::ref(value)};

    ASSERT_TRUE(any);
    ASSERT_FALSE(any.try_cast<std::size_t>());
    ASSERT_EQ(any.cast<int>(), 3);
    ASSERT_NE(any.data(), nullptr);
    ASSERT_EQ(any, (entt::meta_any{std::ref(value)}));
    ASSERT_NE(any, (entt::meta_any{std::ref(other)}));
    ASSERT_NE(any, entt::meta_any{42});
    ASSERT_EQ(entt::meta_any{3}, any);
}

TEST_F(Meta, MetaAnySBOCopyConstruction) {
    entt::meta_any any{42};
    entt::meta_any other{any};

    ASSERT_TRUE(any);
    ASSERT_TRUE(other);
    ASSERT_FALSE(other.try_cast<std::size_t>());
    ASSERT_EQ(other.cast<int>(), 42);
    ASSERT_EQ(other, entt::meta_any{42});
    ASSERT_NE(other, entt::meta_any{0});
}

TEST_F(Meta, MetaAnySBOCopyAssignment) {
    entt::meta_any any{42};
    entt::meta_any other{3};

    other = any;

    ASSERT_TRUE(any);
    ASSERT_TRUE(other);
    ASSERT_FALSE(other.try_cast<std::size_t>());
    ASSERT_EQ(other.cast<int>(), 42);
    ASSERT_EQ(other, entt::meta_any{42});
    ASSERT_NE(other, entt::meta_any{0});
}

TEST_F(Meta, MetaAnySBOMoveConstruction) {
    entt::meta_any any{42};
    entt::meta_any other{std::move(any)};

    ASSERT_FALSE(any);
    ASSERT_TRUE(other);
    ASSERT_FALSE(other.try_cast<std::size_t>());
    ASSERT_EQ(other.cast<int>(), 42);
    ASSERT_EQ(other, entt::meta_any{42});
    ASSERT_NE(other, entt::meta_any{0});
}

TEST_F(Meta, MetaAnySBOMoveAssignment) {
    entt::meta_any any{42};
    entt::meta_any other{3};

    other = std::move(any);

    ASSERT_FALSE(any);
    ASSERT_TRUE(other);
    ASSERT_FALSE(other.try_cast<std::size_t>());
    ASSERT_EQ(other.cast<int>(), 42);
    ASSERT_EQ(other, entt::meta_any{42});
    ASSERT_NE(other, entt::meta_any{0});
}

TEST_F(Meta, MetaAnySBODirectAssignment) {
    entt::meta_any any{};
    any = 42;

    ASSERT_FALSE(any.try_cast<std::size_t>());
    ASSERT_EQ(any.cast<int>(), 42);
    ASSERT_EQ(any, entt::meta_any{42});
    ASSERT_NE(entt::meta_any{0}, any);
}

TEST_F(Meta, MetaAnyNoSBOInPlaceTypeConstruction) {
    int value = 42;
    fat_type instance{&value};
    entt::meta_any any{std::in_place_type<fat_type>, instance};

    ASSERT_TRUE(any);
    ASSERT_FALSE(any.try_cast<std::size_t>());
    ASSERT_EQ(any.cast<fat_type>(), instance);
    ASSERT_NE(any.data(), nullptr);
    ASSERT_EQ(any, (entt::meta_any{std::in_place_type<fat_type>, instance}));
    ASSERT_EQ(any, entt::meta_any{instance});
    ASSERT_NE(entt::meta_any{fat_type{}}, any);
}

TEST_F(Meta, MetaAnyNoSBOAsAliasConstruction) {
    int value = 3;
    fat_type instance{&value};
    entt::meta_any any{std::ref(instance)};

    ASSERT_TRUE(any);
    ASSERT_FALSE(any.try_cast<std::size_t>());
    ASSERT_EQ(any.cast<fat_type>(), instance);
    ASSERT_NE(any.data(), nullptr);
    ASSERT_EQ(any, (entt::meta_any{std::ref(instance)}));
    ASSERT_EQ(any, entt::meta_any{instance});
    ASSERT_NE(entt::meta_any{fat_type{}}, any);
}

TEST_F(Meta, MetaAnyNoSBOCopyConstruction) {
    int value = 42;
    fat_type instance{&value};
    entt::meta_any any{instance};
    entt::meta_any other{any};

    ASSERT_TRUE(any);
    ASSERT_TRUE(other);
    ASSERT_FALSE(other.try_cast<std::size_t>());
    ASSERT_EQ(other.cast<fat_type>(), instance);
    ASSERT_EQ(other, entt::meta_any{instance});
    ASSERT_NE(other, fat_type{});
}

TEST_F(Meta, MetaAnyNoSBOCopyAssignment) {
    int value = 42;
    fat_type instance{&value};
    entt::meta_any any{instance};
    entt::meta_any other{3};

    other = any;

    ASSERT_TRUE(any);
    ASSERT_TRUE(other);
    ASSERT_FALSE(other.try_cast<std::size_t>());
    ASSERT_EQ(other.cast<fat_type>(), instance);
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
    ASSERT_FALSE(other.try_cast<std::size_t>());
    ASSERT_EQ(other.cast<fat_type>(), instance);
    ASSERT_EQ(other, entt::meta_any{instance});
    ASSERT_NE(other, fat_type{});
}

TEST_F(Meta, MetaAnyNoSBOMoveAssignment) {
    int value = 42;
    fat_type instance{&value};
    entt::meta_any any{instance};
    entt::meta_any other{3};

    other = std::move(any);

    ASSERT_FALSE(any);
    ASSERT_TRUE(other);
    ASSERT_FALSE(other.try_cast<std::size_t>());
    ASSERT_EQ(other.cast<fat_type>(), instance);
    ASSERT_EQ(other, entt::meta_any{instance});
    ASSERT_NE(other, fat_type{});
}

TEST_F(Meta, MetaAnyNoSBODirectAssignment) {
    int value = 42;
    entt::meta_any any{};
    any = fat_type{&value};

    ASSERT_FALSE(any.try_cast<std::size_t>());
    ASSERT_EQ(any.cast<fat_type>(), fat_type{&value});
    ASSERT_EQ(any, entt::meta_any{fat_type{&value}});
    ASSERT_NE(fat_type{}, any);
}

TEST_F(Meta, MetaAnyVoidInPlaceTypeConstruction) {
    entt::meta_any any{std::in_place_type<void>};

    ASSERT_TRUE(any);
    ASSERT_FALSE(any.try_cast<char>());
    ASSERT_EQ(any.data(), nullptr);
    ASSERT_EQ(any.type(), entt::resolve<void>());
    ASSERT_EQ(any, entt::meta_any{std::in_place_type<void>});
    ASSERT_NE(entt::meta_any{3}, any);
}

TEST_F(Meta, MetaAnyVoidCopyConstruction) {
    entt::meta_any any{std::in_place_type<void>};
    entt::meta_any other{any};

    ASSERT_TRUE(any);
    ASSERT_TRUE(other);
    ASSERT_EQ(any.type(), entt::resolve<void>());
    ASSERT_EQ(other, entt::meta_any{std::in_place_type<void>});
}

TEST_F(Meta, MetaAnyVoidCopyAssignment) {
    entt::meta_any any{std::in_place_type<void>};
    entt::meta_any other{std::in_place_type<void>};

    other = any;

    ASSERT_TRUE(any);
    ASSERT_TRUE(other);
    ASSERT_EQ(any.type(), entt::resolve<void>());
    ASSERT_EQ(other, entt::meta_any{std::in_place_type<void>});
}

TEST_F(Meta, MetaAnyVoidMoveConstruction) {
    entt::meta_any any{std::in_place_type<void>};
    entt::meta_any other{std::move(any)};

    ASSERT_FALSE(any);
    ASSERT_TRUE(other);
    ASSERT_EQ(other.type(), entt::resolve<void>());
    ASSERT_EQ(other, entt::meta_any{std::in_place_type<void>});
}

TEST_F(Meta, MetaAnyVoidMoveAssignment) {
    entt::meta_any any{std::in_place_type<void>};
    entt::meta_any other{std::in_place_type<void>};

    other = std::move(any);

    ASSERT_FALSE(any);
    ASSERT_TRUE(other);
    ASSERT_EQ(other.type(), entt::resolve<void>());
    ASSERT_EQ(other, entt::meta_any{std::in_place_type<void>});
}

TEST_F(Meta, MetaAnySBOMoveInvalidate) {
    entt::meta_any any{42};
    entt::meta_any other{std::move(any)};
    entt::meta_any valid = std::move(other);

    ASSERT_FALSE(any);
    ASSERT_FALSE(other);
    ASSERT_TRUE(valid);
}

TEST_F(Meta, MetaAnyNoSBOMoveInvalidate) {
    int value = 42;
    fat_type instance{&value};
    entt::meta_any any{instance};
    entt::meta_any other{std::move(any)};
    entt::meta_any valid = std::move(other);

    ASSERT_FALSE(any);
    ASSERT_FALSE(other);
    ASSERT_TRUE(valid);
}

TEST_F(Meta, MetaAnyVoidMoveInvalidate) {
    entt::meta_any any{std::in_place_type<void>};
    entt::meta_any other{std::move(any)};
    entt::meta_any valid = std::move(other);

    ASSERT_FALSE(any);
    ASSERT_FALSE(other);
    ASSERT_TRUE(valid);
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

TEST_F(Meta, MetaAnyVoidDestruction) {
    // just let asan tell us if everything is ok here
    [[maybe_unused]] entt::meta_any any{std::in_place_type<void>};
}

TEST_F(Meta, MetaAnyEmplace) {
    entt::meta_any any{};
    any.emplace<int>(42);

    ASSERT_TRUE(any);
    ASSERT_FALSE(any.try_cast<std::size_t>());
    ASSERT_EQ(any.cast<int>(), 42);
    ASSERT_NE(any.data(), nullptr);
    ASSERT_EQ(any, (entt::meta_any{std::in_place_type<int>, 42}));
    ASSERT_EQ(any, entt::meta_any{42});
    ASSERT_NE(entt::meta_any{3}, any);
}

TEST_F(Meta, MetaAnyEmplaceVoid) {
    entt::meta_any any{};
    any.emplace<void>();

    ASSERT_TRUE(any);
    ASSERT_EQ(any.data(), nullptr);
    ASSERT_EQ(any.type(), entt::resolve<void>());
    ASSERT_EQ(any, (entt::meta_any{std::in_place_type<void>}));
}

TEST_F(Meta, MetaAnySBOSwap) {
    entt::meta_any lhs{'c'};
    entt::meta_any rhs{42};

    std::swap(lhs, rhs);

    ASSERT_FALSE(lhs.try_cast<char>());
    ASSERT_EQ(lhs.cast<int>(), 42);
    ASSERT_FALSE(rhs.try_cast<int>());
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

TEST_F(Meta, MetaAnyVoidSwap) {
    entt::meta_any lhs{std::in_place_type<void>};
    entt::meta_any rhs{std::in_place_type<void>};
    const auto *pre = lhs.data();

    std::swap(lhs, rhs);

    ASSERT_EQ(pre, lhs.data());
}

TEST_F(Meta, MetaAnySBOWithNoSBOSwap) {
    int value = 42;
    entt::meta_any lhs{fat_type{&value}};
    entt::meta_any rhs{'c'};

    std::swap(lhs, rhs);

    ASSERT_FALSE(lhs.try_cast<fat_type>());
    ASSERT_EQ(lhs.cast<char>(), 'c');
    ASSERT_FALSE(rhs.try_cast<char>());
    ASSERT_EQ(rhs.cast<fat_type>().foo, &value);
    ASSERT_EQ(rhs.cast<fat_type>().bar, &value);
}

TEST_F(Meta, MetaAnySBOWithEmptySwap) {
    entt::meta_any lhs{'c'};
    entt::meta_any rhs{};

    std::swap(lhs, rhs);

    ASSERT_FALSE(lhs);
    ASSERT_EQ(rhs.cast<char>(), 'c');

    std::swap(lhs, rhs);

    ASSERT_FALSE(rhs);
    ASSERT_EQ(lhs.cast<char>(), 'c');
}

TEST_F(Meta, MetaAnySBOWithVoidSwap) {
    entt::meta_any lhs{'c'};
    entt::meta_any rhs{std::in_place_type<void>};

    std::swap(lhs, rhs);

    ASSERT_EQ(lhs.type(), entt::resolve<void>());
    ASSERT_EQ(rhs.cast<char>(), 'c');
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

TEST_F(Meta, MetaAnyNoSBOWithVoidSwap) {
    int i;
    entt::meta_any lhs{fat_type{&i}};
    entt::meta_any rhs{std::in_place_type<void>};

    std::swap(lhs, rhs);

    ASSERT_EQ(rhs.cast<fat_type>().bar, &i);

    std::swap(lhs, rhs);

    ASSERT_EQ(lhs.cast<fat_type>().bar, &i);
}

TEST_F(Meta, MetaAnyComparable) {
    entt::meta_any any{'c'};

    ASSERT_EQ(any, any);
    ASSERT_EQ(any, entt::meta_any{'c'});
    ASSERT_NE(entt::meta_any{'a'}, any);
    ASSERT_NE(any, entt::meta_any{});

    ASSERT_TRUE(any == any);
    ASSERT_TRUE(any == entt::meta_any{'c'});
    ASSERT_FALSE(entt::meta_any{'a'} == any);
    ASSERT_TRUE(any != entt::meta_any{'a'});
    ASSERT_TRUE(entt::meta_any{} != any);
}

TEST_F(Meta, MetaAnyNotComparable) {
    entt::meta_any any{not_comparable_type{}};

    ASSERT_EQ(any, any);
    ASSERT_NE(any, entt::meta_any{not_comparable_type{}});
    ASSERT_NE(entt::meta_any{}, any);

    ASSERT_TRUE(any == any);
    ASSERT_FALSE(any == entt::meta_any{not_comparable_type{}});
    ASSERT_TRUE(entt::meta_any{} != any);
}

TEST_F(Meta, MetaAnyCompareVoid) {
    entt::meta_any any{std::in_place_type<void>};

    ASSERT_EQ(any, any);
    ASSERT_EQ(any, entt::meta_any{std::in_place_type<void>});
    ASSERT_NE(entt::meta_any{'a'}, any);
    ASSERT_NE(any, entt::meta_any{});

    ASSERT_TRUE(any == any);
    ASSERT_TRUE(any == entt::meta_any{std::in_place_type<void>});
    ASSERT_FALSE(entt::meta_any{'a'} == any);
    ASSERT_TRUE(any != entt::meta_any{'a'});
    ASSERT_TRUE(entt::meta_any{} != any);
}

TEST_F(Meta, MetaAnyTryCast) {
    entt::meta_any any{derived_type{}};

    ASSERT_TRUE(any);
    ASSERT_EQ(any.type(), entt::resolve<derived_type>());
    ASSERT_EQ(any.try_cast<void>(), nullptr);
    ASSERT_NE(any.try_cast<base_type>(), nullptr);
    ASSERT_EQ(any.try_cast<derived_type>(), any.data());
    ASSERT_EQ(std::as_const(any).try_cast<base_type>(), any.try_cast<base_type>());
    ASSERT_EQ(std::as_const(any).try_cast<derived_type>(), any.data());
}

TEST_F(Meta, MetaAnyCast) {
    entt::meta_any any{derived_type{}};

    ASSERT_TRUE(any);
    ASSERT_EQ(any.type(), entt::resolve<derived_type>());
    ASSERT_EQ(any.try_cast<std::size_t>(), nullptr);
    ASSERT_NE(any.try_cast<base_type>(), nullptr);
    ASSERT_EQ(any.try_cast<derived_type>(), any.data());
    ASSERT_EQ(std::as_const(any).try_cast<base_type>(), any.try_cast<base_type>());
    ASSERT_EQ(std::as_const(any).try_cast<derived_type>(), any.data());
}

TEST_F(Meta, MetaAnyConvert) {
    entt::meta_any any{42.};

    ASSERT_TRUE(any);
    ASSERT_EQ(any.type(), entt::resolve<double>());
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

TEST_F(Meta, MetaAnyUnmanageableType) {
    unmanageable_type instance;
    entt::meta_any any{std::ref(instance)};
    entt::meta_any other = any;

    std::swap(any, other);

    ASSERT_TRUE(any);
    ASSERT_TRUE(other);

    ASSERT_EQ(any.type(), entt::resolve<unmanageable_type>());
    ASSERT_NE(any.data(), nullptr);
    ASSERT_EQ(any.try_cast<int>(), nullptr);
    ASSERT_NE(any.try_cast<unmanageable_type>(), nullptr);

    ASSERT_TRUE(any.convert<unmanageable_type>());
    ASSERT_FALSE(any.convert<int>());

    ASSERT_TRUE(std::as_const(any).convert<unmanageable_type>());
    ASSERT_FALSE(std::as_const(any).convert<int>());
}

TEST_F(Meta, MetaProp) {
    auto prop = entt::resolve<char>().prop(props::prop_int);

    ASSERT_TRUE(prop);
    ASSERT_EQ(prop.key(), props::prop_int);
    ASSERT_EQ(prop.value(), 42);
}

TEST_F(Meta, MetaBase) {
    auto base = entt::resolve<derived_type>().base("base"_hs);
    derived_type derived{};

    ASSERT_TRUE(base);
    ASSERT_EQ(base.parent(), entt::resolve("derived"_hs));
    ASSERT_EQ(base.type(), entt::resolve<base_type>());
    ASSERT_EQ(base.cast(&derived), static_cast<base_type *>(&derived));
}

TEST_F(Meta, MetaConv) {
    auto conv = entt::resolve<double>().conv<int>();
    double value = 3.;

    ASSERT_TRUE(conv);
    ASSERT_EQ(conv.parent(), entt::resolve<double>());
    ASSERT_EQ(conv.type(), entt::resolve<int>());

    auto any = conv.convert(&value);

    ASSERT_TRUE(any);
    ASSERT_EQ(any.type(), entt::resolve<int>());
    ASSERT_EQ(any.cast<int>(), 3);
}

TEST_F(Meta, MetaConvAsFreeFunctions) {
    auto conv = entt::resolve<derived_type>().conv<int>();
    derived_type derived{derived_type{}, 42, 'c'};

    ASSERT_TRUE(conv);
    ASSERT_EQ(conv.parent(), entt::resolve<derived_type>());
    ASSERT_EQ(conv.type(), entt::resolve<int>());

    auto any = conv.convert(&derived);

    ASSERT_TRUE(any);
    ASSERT_EQ(any.type(), entt::resolve<int>());
    ASSERT_EQ(any.cast<int>(), 42);
}

TEST_F(Meta, MetaConvAsMemberFunctions) {
    auto conv = entt::resolve<derived_type>().conv<char>();
    derived_type derived{derived_type{}, 42, 'c'};

    ASSERT_TRUE(conv);
    ASSERT_EQ(conv.parent(), entt::resolve<derived_type>());
    ASSERT_EQ(conv.type(), entt::resolve<char>());

    auto any = conv.convert(&derived);

    ASSERT_TRUE(any);
    ASSERT_EQ(any.type(), entt::resolve<char>());
    ASSERT_EQ(any.cast<char>(), 'c');
}

TEST_F(Meta, MetaCtor) {
    auto ctor = entt::resolve<derived_type>().ctor<const base_type &, int, char>();

    ASSERT_TRUE(ctor);
    ASSERT_EQ(ctor.parent(), entt::resolve("derived"_hs));
    ASSERT_EQ(ctor.size(), 3u);
    ASSERT_EQ(ctor.arg(0u), entt::resolve<base_type>());
    ASSERT_EQ(ctor.arg(1u), entt::resolve<int>());
    ASSERT_EQ(ctor.arg(2u), entt::resolve<char>());
    ASSERT_FALSE(ctor.arg(3u));

    auto any = ctor.invoke(base_type{}, 42, 'c');
    auto empty = ctor.invoke();

    ASSERT_FALSE(empty);
    ASSERT_TRUE(any);
    ASSERT_EQ(any.cast<derived_type>().i, 42);
    ASSERT_EQ(any.cast<derived_type>().c, 'c');

    ctor.prop([](auto prop) {
        ASSERT_EQ(prop.key(), props::prop_bool);
        ASSERT_FALSE(prop.value().template cast<bool>());
    });

    ASSERT_FALSE(ctor.prop(props::prop_int));

    auto prop = ctor.prop(props::prop_bool);

    ASSERT_TRUE(prop);
    ASSERT_EQ(prop.key(), props::prop_bool);
    ASSERT_FALSE(prop.value().template cast<bool>());
}

TEST_F(Meta, MetaCtorFunc) {
    auto ctor = entt::resolve<derived_type>().ctor<const base_type &, int>();

    ASSERT_TRUE(ctor);
    ASSERT_EQ(ctor.parent(), entt::resolve("derived"_hs));
    ASSERT_EQ(ctor.size(), 2u);
    ASSERT_EQ(ctor.arg(0u), entt::resolve<base_type>());
    ASSERT_EQ(ctor.arg(1u), entt::resolve<int>());
    ASSERT_FALSE(ctor.arg(2u));

    auto any = ctor.invoke(derived_type{}, 42);
    auto empty = ctor.invoke(3, 'c');

    ASSERT_FALSE(empty);
    ASSERT_TRUE(any);
    ASSERT_EQ(any.cast<derived_type>().i, 42);
    ASSERT_EQ(any.cast<derived_type>().c, 'c');

    ctor.prop([](auto prop) {
        ASSERT_EQ(prop.key(), props::prop_int);
        ASSERT_EQ(prop.value(), 42);
    });

    ASSERT_FALSE(ctor.prop(props::prop_bool));

    auto prop = ctor.prop(props::prop_int);

    ASSERT_TRUE(prop);
    ASSERT_EQ(prop.key(), props::prop_int);
    ASSERT_EQ(prop.value(), 42);
}

TEST_F(Meta, MetaCtorMetaAnyArgs) {
    auto any = entt::resolve<derived_type>().ctor<const base_type &, int, char>().invoke(base_type{}, 42, 'c');

    ASSERT_TRUE(any);
    ASSERT_EQ(any.cast<derived_type>().i, 42);
    ASSERT_EQ(any.cast<derived_type>().c, 'c');
}

TEST_F(Meta, MetaCtorInvalidArgs) {
    auto ctor = entt::resolve<derived_type>().ctor<const base_type &, int, char>();
    ASSERT_FALSE(ctor.invoke(base_type{}, 'c', 42));
}

TEST_F(Meta, MetaCtorCastAndConvert) {
    auto any = entt::resolve<derived_type>().ctor<const base_type &, int, char>().invoke(derived_type{}, 42., 'c');

    ASSERT_TRUE(any);
    ASSERT_EQ(any.cast<derived_type>().i, 42);
    ASSERT_EQ(any.cast<derived_type>().c, 'c');
}

TEST_F(Meta, MetaCtorFuncMetaAnyArgs) {
    auto any = entt::resolve<derived_type>().ctor<const base_type &, int>().invoke(base_type{}, 42);

    ASSERT_TRUE(any);
    ASSERT_EQ(any.cast<derived_type>().i, 42);
    ASSERT_EQ(any.cast<derived_type>().c, 'c');
}

TEST_F(Meta, MetaCtorFuncInvalidArgs) {
    auto ctor = entt::resolve<derived_type>().ctor<const base_type &, int>();
    ASSERT_FALSE(ctor.invoke(base_type{}, 'c'));
}

TEST_F(Meta, MetaCtorFuncCastAndConvert) {
    auto any = entt::resolve<derived_type>().ctor<const base_type &, int>().invoke(derived_type{}, 42.);

    ASSERT_TRUE(any);
    ASSERT_EQ(any.cast<derived_type>().i, 42);
    ASSERT_EQ(any.cast<derived_type>().c, 'c');
}

TEST_F(Meta, MetaData) {
    auto data = entt::resolve<data_type>().data("i"_hs);
    data_type instance{};

    ASSERT_TRUE(data);
    ASSERT_EQ(data.parent(), entt::resolve("data"_hs));
    ASSERT_EQ(data.type(), entt::resolve<int>());
    ASSERT_EQ(data.alias(), "i"_hs);
    ASSERT_FALSE(data.is_const());
    ASSERT_FALSE(data.is_static());
    ASSERT_EQ(data.get(instance).cast<int>(), 0);
    ASSERT_TRUE(data.set(instance, 42));
    ASSERT_EQ(data.get(instance).cast<int>(), 42);

    data.prop([](auto prop) {
        ASSERT_EQ(prop.key(), props::prop_int);
        ASSERT_EQ(prop.value(), 0);
    });

    ASSERT_FALSE(data.prop(props::prop_bool));

    auto prop = data.prop(props::prop_int);

    ASSERT_TRUE(prop);
    ASSERT_EQ(prop.key(), props::prop_int);
    ASSERT_EQ(prop.value(), 0);
}

TEST_F(Meta, MetaDataConst) {
    auto data = entt::resolve<data_type>().data("j"_hs);
    data_type instance{};

    ASSERT_TRUE(data);
    ASSERT_EQ(data.parent(), entt::resolve("data"_hs));
    ASSERT_EQ(data.type(), entt::resolve<int>());
    ASSERT_EQ(data.alias(), "j"_hs);
    ASSERT_TRUE(data.is_const());
    ASSERT_FALSE(data.is_static());
    ASSERT_EQ(data.get(instance).cast<int>(), 1);
    ASSERT_FALSE(data.set(instance, 42));
    ASSERT_EQ(data.get(instance).cast<int>(), 1);

    data.prop([](auto prop) {
        ASSERT_EQ(prop.key(), props::prop_int);
        ASSERT_EQ(prop.value(), 1);
    });

    ASSERT_FALSE(data.prop(props::prop_bool));

    auto prop = data.prop(props::prop_int);

    ASSERT_TRUE(prop);
    ASSERT_EQ(prop.key(), props::prop_int);
    ASSERT_EQ(prop.value(), 1);
}

TEST_F(Meta, MetaDataStatic) {
    auto data = entt::resolve<data_type>().data("h"_hs);

    ASSERT_TRUE(data);
    ASSERT_EQ(data.parent(), entt::resolve("data"_hs));
    ASSERT_EQ(data.type(), entt::resolve<int>());
    ASSERT_EQ(data.alias(), "h"_hs);
    ASSERT_FALSE(data.is_const());
    ASSERT_TRUE(data.is_static());
    ASSERT_EQ(data.get({}).cast<int>(), 2);
    ASSERT_TRUE(data.set({}, 42));
    ASSERT_EQ(data.get({}).cast<int>(), 42);

    data.prop([](auto prop) {
        ASSERT_EQ(prop.key(), props::prop_int);
        ASSERT_EQ(prop.value(), 2);
    });

    ASSERT_FALSE(data.prop(props::prop_bool));

    auto prop = data.prop(props::prop_int);

    ASSERT_TRUE(prop);
    ASSERT_EQ(prop.key(), props::prop_int);
    ASSERT_EQ(prop.value(), 2);
}

TEST_F(Meta, MetaDataConstStatic) {
    auto data = entt::resolve<data_type>().data("k"_hs);

    ASSERT_TRUE(data);
    ASSERT_EQ(data.parent(), entt::resolve("data"_hs));
    ASSERT_EQ(data.type(), entt::resolve<int>());
    ASSERT_EQ(data.alias(), "k"_hs);
    ASSERT_TRUE(data.is_const());
    ASSERT_TRUE(data.is_static());
    ASSERT_EQ(data.get({}).cast<int>(), 3);
    ASSERT_FALSE(data.set({}, 42));
    ASSERT_EQ(data.get({}).cast<int>(), 3);

    data.prop([](auto prop) {
        ASSERT_EQ(prop.key(), props::prop_int);
        ASSERT_EQ(prop.value(), 3);
    });

    ASSERT_FALSE(data.prop(props::prop_bool));

    auto prop = data.prop(props::prop_int);

    ASSERT_TRUE(prop);
    ASSERT_EQ(prop.key(), props::prop_int);
    ASSERT_EQ(prop.value(), 3);
}

TEST_F(Meta, MetaDataGetMetaAnyArg) {
    entt::meta_any any{data_type{}};
    any.cast<data_type>().i = 99;
    const auto value = entt::resolve<data_type>().data("i"_hs).get(any);

    ASSERT_TRUE(value);
    ASSERT_TRUE(value.cast<int>());
    ASSERT_EQ(value.cast<int>(), 99);
}

TEST_F(Meta, MetaDataGetInvalidArg) {
    auto instance = 0;
    ASSERT_FALSE(entt::resolve<data_type>().data("i"_hs).get(instance));
}

TEST_F(Meta, MetaDataSetMetaAnyArg) {
    entt::meta_any any{data_type{}};
    entt::meta_any value{42};

    ASSERT_EQ(any.cast<data_type>().i, 0);
    ASSERT_TRUE(entt::resolve<data_type>().data("i"_hs).set(any, value));
    ASSERT_EQ(any.cast<data_type>().i, 42);
}

TEST_F(Meta, MetaDataSetInvalidArg) {
    ASSERT_FALSE(entt::resolve<data_type>().data("i"_hs).set({}, 'c'));
}

TEST_F(Meta, MetaDataSetCast) {
    data_type instance{};

    ASSERT_EQ(empty_type::counter, 0);
    ASSERT_TRUE(entt::resolve<data_type>().data("empty"_hs).set(instance, fat_type{}));
    ASSERT_EQ(empty_type::counter, 1);
}

TEST_F(Meta, MetaDataSetConvert) {
    data_type instance{};

    ASSERT_EQ(instance.i, 0);
    ASSERT_TRUE(entt::resolve<data_type>().data("i"_hs).set(instance, 3.));
    ASSERT_EQ(instance.i, 3);
}

TEST_F(Meta, MetaDataSetterGetterAsFreeFunctions) {
    auto data = entt::resolve<setter_getter_type>().data("x"_hs);
    setter_getter_type instance{};

    ASSERT_TRUE(data);
    ASSERT_EQ(data.parent(), entt::resolve("setter_getter"_hs));
    ASSERT_EQ(data.type(), entt::resolve<int>());
    ASSERT_EQ(data.alias(), "x"_hs);
    ASSERT_FALSE(data.is_const());
    ASSERT_FALSE(data.is_static());
    ASSERT_EQ(data.get(instance).cast<int>(), 0);
    ASSERT_TRUE(data.set(instance, 42));
    ASSERT_EQ(data.get(instance).cast<int>(), 42);
}

TEST_F(Meta, MetaDataSetterGetterAsMemberFunctions) {
    auto data = entt::resolve<setter_getter_type>().data("y"_hs);
    setter_getter_type instance{};

    ASSERT_TRUE(data);
    ASSERT_EQ(data.parent(), entt::resolve("setter_getter"_hs));
    ASSERT_EQ(data.type(), entt::resolve<int>());
    ASSERT_EQ(data.alias(), "y"_hs);
    ASSERT_FALSE(data.is_const());
    ASSERT_FALSE(data.is_static());
    ASSERT_EQ(data.get(instance).cast<int>(), 0);
    ASSERT_TRUE(data.set(instance, 42));
    ASSERT_EQ(data.get(instance).cast<int>(), 42);
}

TEST_F(Meta, MetaDataSetterGetterWithRefAsMemberFunctions) {
    auto data = entt::resolve<setter_getter_type>().data("w"_hs);
    setter_getter_type instance{};

    ASSERT_TRUE(data);
    ASSERT_EQ(data.parent(), entt::resolve("setter_getter"_hs));
    ASSERT_EQ(data.type(), entt::resolve<int>());
    ASSERT_EQ(data.alias(), "w"_hs);
    ASSERT_FALSE(data.is_const());
    ASSERT_FALSE(data.is_static());
    ASSERT_EQ(data.get(instance).cast<int>(), 0);
    ASSERT_TRUE(data.set(instance, 42));
    ASSERT_EQ(data.get(instance).cast<int>(), 42);
}

TEST_F(Meta, MetaDataSetterGetterMixed) {
    auto data = entt::resolve<setter_getter_type>().data("z"_hs);
    setter_getter_type instance{};

    ASSERT_TRUE(data);
    ASSERT_EQ(data.parent(), entt::resolve("setter_getter"_hs));
    ASSERT_EQ(data.type(), entt::resolve<int>());
    ASSERT_EQ(data.alias(), "z"_hs);
    ASSERT_FALSE(data.is_const());
    ASSERT_FALSE(data.is_static());
    ASSERT_EQ(data.get(instance).cast<int>(), 0);
    ASSERT_TRUE(data.set(instance, 42));
    ASSERT_EQ(data.get(instance).cast<int>(), 42);
}

TEST_F(Meta, MetaDataArrayStatic) {
    auto data = entt::resolve<array_type>().data("global"_hs);

    array_type::global[0] = 3;
    array_type::global[1] = 5;
    array_type::global[2] = 7;

    ASSERT_TRUE(data);
    ASSERT_EQ(data.parent(), entt::resolve("array"_hs));
    ASSERT_EQ(data.type(), entt::resolve<int[3]>());
    ASSERT_EQ(data.alias(), "global"_hs);
    ASSERT_FALSE(data.is_const());
    ASSERT_TRUE(data.is_static());
    ASSERT_TRUE(data.type().is_array());
    ASSERT_EQ(data.type().extent(), 3);
    ASSERT_EQ(data.get({}, 0).cast<int>(), 3);
    ASSERT_EQ(data.get({}, 1).cast<int>(), 5);
    ASSERT_EQ(data.get({}, 2).cast<int>(), 7);
    ASSERT_FALSE(data.set({}, 0, 'c'));
    ASSERT_EQ(data.get({}, 0).cast<int>(), 3);
    ASSERT_TRUE(data.set({}, 0, data.get({}, 0).cast<int>()+2));
    ASSERT_TRUE(data.set({}, 1, data.get({}, 1).cast<int>()+2));
    ASSERT_TRUE(data.set({}, 2, data.get({}, 2).cast<int>()+2));
    ASSERT_EQ(data.get({}, 0).cast<int>(), 5);
    ASSERT_EQ(data.get({}, 1).cast<int>(), 7);
    ASSERT_EQ(data.get({}, 2).cast<int>(), 9);
}

TEST_F(Meta, MetaDataArray) {
    auto data = entt::resolve<array_type>().data("local"_hs);
    array_type instance;

    instance.local[0] = 3;
    instance.local[1] = 5;
    instance.local[2] = 7;

    ASSERT_TRUE(data);
    ASSERT_EQ(data.parent(), entt::resolve("array"_hs));
    ASSERT_EQ(data.type(), entt::resolve<int[3]>());
    ASSERT_EQ(data.alias(), "local"_hs);
    ASSERT_FALSE(data.is_const());
    ASSERT_FALSE(data.is_static());
    ASSERT_TRUE(data.type().is_array());
    ASSERT_EQ(data.type().extent(), 3);
    ASSERT_EQ(data.get(instance, 0).cast<int>(), 3);
    ASSERT_EQ(data.get(instance, 1).cast<int>(), 5);
    ASSERT_EQ(data.get(instance, 2).cast<int>(), 7);
    ASSERT_FALSE(data.set(instance, 0, 'c'));
    ASSERT_EQ(data.get(instance, 0).cast<int>(), 3);
    ASSERT_TRUE(data.set(instance, 0, data.get(instance, 0).cast<int>()+2));
    ASSERT_TRUE(data.set(instance, 1, data.get(instance, 1).cast<int>()+2));
    ASSERT_TRUE(data.set(instance, 2, data.get(instance, 2).cast<int>()+2));
    ASSERT_EQ(data.get(instance, 0).cast<int>(), 5);
    ASSERT_EQ(data.get(instance, 1).cast<int>(), 7);
    ASSERT_EQ(data.get(instance, 2).cast<int>(), 9);
}

TEST_F(Meta, MetaDataAsVoid) {
    auto data = entt::resolve<data_type>().data("v"_hs);
    data_type instance{};

    ASSERT_TRUE(data.set(instance, 42));
    ASSERT_EQ(instance.v, 42);
    ASSERT_EQ(data.get(instance), entt::meta_any{std::in_place_type<void>});
}

TEST_F(Meta, MetaDataAsAlias) {
    data_type instance{};
    auto h_data = entt::resolve<data_type>().data("h"_hs);
    auto i_data = entt::resolve<data_type>().data("i"_hs);

    h_data.get(instance).cast<int>() = 3;
    i_data.get(instance).cast<int>() = 3;

    ASSERT_EQ(h_data.type(), entt::resolve<int>());
    ASSERT_EQ(i_data.type(), entt::resolve<int>());
    ASSERT_NE(instance.h, 3);
    ASSERT_EQ(instance.i, 3);
}

TEST_F(Meta, MetaFunc) {
    auto func = entt::resolve<func_type>().func("f2"_hs);
    func_type instance{};

    ASSERT_TRUE(func);
    ASSERT_EQ(func.parent(), entt::resolve("func"_hs));
    ASSERT_EQ(func.alias(), "f2"_hs);
    ASSERT_EQ(func.size(), 2u);
    ASSERT_FALSE(func.is_const());
    ASSERT_FALSE(func.is_static());
    ASSERT_EQ(func.ret(), entt::resolve<int>());
    ASSERT_EQ(func.arg(0u), entt::resolve<int>());
    ASSERT_EQ(func.arg(1u), entt::resolve<int>());
    ASSERT_FALSE(func.arg(2u));

    auto any = func.invoke(instance, 3, 2);
    auto empty = func.invoke(instance);

    ASSERT_FALSE(empty);
    ASSERT_TRUE(any);
    ASSERT_EQ(any.type(), entt::resolve<int>());
    ASSERT_EQ(any.cast<int>(), 4);
    ASSERT_EQ(func_type::value, 3);

    func.prop([](auto prop) {
        ASSERT_EQ(prop.key(), props::prop_bool);
        ASSERT_FALSE(prop.value().template cast<bool>());
    });

    ASSERT_FALSE(func.prop(props::prop_int));

    auto prop = func.prop(props::prop_bool);

    ASSERT_TRUE(prop);
    ASSERT_EQ(prop.key(), props::prop_bool);
    ASSERT_FALSE(prop.value().cast<bool>());
}

TEST_F(Meta, MetaFuncConst) {
    auto func = entt::resolve<func_type>().func("f1"_hs);
    func_type instance{};

    ASSERT_TRUE(func);
    ASSERT_EQ(func.parent(), entt::resolve("func"_hs));
    ASSERT_EQ(func.alias(), "f1"_hs);
    ASSERT_EQ(func.size(), 1u);
    ASSERT_TRUE(func.is_const());
    ASSERT_FALSE(func.is_static());
    ASSERT_EQ(func.ret(), entt::resolve<int>());
    ASSERT_EQ(func.arg(0u), entt::resolve<int>());
    ASSERT_FALSE(func.arg(1u));

    auto any = func.invoke(instance, 4);
    auto empty = func.invoke(instance, 'c');

    ASSERT_FALSE(empty);
    ASSERT_TRUE(any);
    ASSERT_EQ(any.type(), entt::resolve<int>());
    ASSERT_EQ(any.cast<int>(), 16);

    func.prop([](auto prop) {
        ASSERT_EQ(prop.key(), props::prop_bool);
        ASSERT_FALSE(prop.value().template cast<bool>());
    });

    ASSERT_FALSE(func.prop(props::prop_int));

    auto prop = func.prop(props::prop_bool);

    ASSERT_TRUE(prop);
    ASSERT_EQ(prop.key(), props::prop_bool);
    ASSERT_FALSE(prop.value().cast<bool>());
}

TEST_F(Meta, MetaFuncRetVoid) {
    auto func = entt::resolve<func_type>().func("g"_hs);
    func_type instance{};

    ASSERT_TRUE(func);
    ASSERT_EQ(func.parent(), entt::resolve("func"_hs));
    ASSERT_EQ(func.alias(), "g"_hs);
    ASSERT_EQ(func.size(), 1u);
    ASSERT_FALSE(func.is_const());
    ASSERT_FALSE(func.is_static());
    ASSERT_EQ(func.ret(), entt::resolve<void>());
    ASSERT_EQ(func.arg(0u), entt::resolve<int>());
    ASSERT_FALSE(func.arg(1u));

    auto any = func.invoke(instance, 5);

    ASSERT_TRUE(any);
    ASSERT_EQ(any.type(), entt::resolve<void>());
    ASSERT_EQ(func_type::value, 25);

    func.prop([](auto prop) {
        ASSERT_EQ(prop.key(), props::prop_bool);
        ASSERT_FALSE(prop.value().template cast<bool>());
    });

    ASSERT_FALSE(func.prop(props::prop_int));

    auto prop = func.prop(props::prop_bool);

    ASSERT_TRUE(prop);
    ASSERT_EQ(prop.key(), props::prop_bool);
    ASSERT_FALSE(prop.value().cast<bool>());
}

TEST_F(Meta, MetaFuncStatic) {
    auto func = entt::resolve<func_type>().func("h"_hs);
    func_type::value = 2;

    ASSERT_TRUE(func);
    ASSERT_EQ(func.parent(), entt::resolve("func"_hs));
    ASSERT_EQ(func.alias(), "h"_hs);
    ASSERT_EQ(func.size(), 1u);
    ASSERT_FALSE(func.is_const());
    ASSERT_TRUE(func.is_static());
    ASSERT_EQ(func.ret(), entt::resolve<int>());
    ASSERT_EQ(func.arg(0u), entt::resolve<int>());
    ASSERT_FALSE(func.arg(1u));

    auto any = func.invoke({}, 3);
    auto empty = func.invoke({}, 'c');

    ASSERT_FALSE(empty);
    ASSERT_TRUE(any);
    ASSERT_EQ(any.type(), entt::resolve<int>());
    ASSERT_EQ(any.cast<int>(), 6);

    func.prop([](auto prop) {
        ASSERT_EQ(prop.key(), props::prop_bool);
        ASSERT_FALSE(prop.value().template cast<bool>());
    });

    ASSERT_FALSE(func.prop(props::prop_int));

    auto prop = func.prop(props::prop_bool);

    ASSERT_TRUE(prop);
    ASSERT_EQ(prop.key(), props::prop_bool);
    ASSERT_FALSE(prop.value().cast<bool>());
}

TEST_F(Meta, MetaFuncStaticRetVoid) {
    auto func = entt::resolve<func_type>().func("k"_hs);

    ASSERT_TRUE(func);
    ASSERT_EQ(func.parent(), entt::resolve("func"_hs));
    ASSERT_EQ(func.alias(), "k"_hs);
    ASSERT_EQ(func.size(), 1u);
    ASSERT_FALSE(func.is_const());
    ASSERT_TRUE(func.is_static());
    ASSERT_EQ(func.ret(), entt::resolve<void>());
    ASSERT_EQ(func.arg(0u), entt::resolve<int>());
    ASSERT_FALSE(func.arg(1u));

    auto any = func.invoke({}, 42);

    ASSERT_TRUE(any);
    ASSERT_EQ(any.type(), entt::resolve<void>());
    ASSERT_EQ(func_type::value, 42);

    func.prop([](auto *prop) {
        ASSERT_TRUE(prop);
        ASSERT_EQ(prop->key(), props::prop_bool);
        ASSERT_FALSE(prop->value().template cast<bool>());
    });

    ASSERT_FALSE(func.prop(props::prop_int));

    auto prop = func.prop(props::prop_bool);

    ASSERT_TRUE(prop);
    ASSERT_EQ(prop.key(), props::prop_bool);
    ASSERT_FALSE(prop.value().cast<bool>());
}

TEST_F(Meta, MetaFuncMetaAnyArgs) {
    func_type instance;
    auto any = entt::resolve<func_type>().func("f1"_hs).invoke(instance, 3);

    ASSERT_TRUE(any);
    ASSERT_EQ(any.type(), entt::resolve<int>());
    ASSERT_EQ(any.cast<int>(), 9);
}

TEST_F(Meta, MetaFuncInvalidArgs) {
    empty_type instance;

    ASSERT_FALSE(entt::resolve<func_type>().func("f1"_hs).invoke(instance, 'c'));
}

TEST_F(Meta, MetaFuncCastAndConvert) {
    func_type instance;
    auto any = entt::resolve<func_type>().func("f3"_hs).invoke(instance, derived_type{}, 0, 3.);

    ASSERT_TRUE(any);
    ASSERT_EQ(any.type(), entt::resolve<int>());
    ASSERT_EQ(any.cast<int>(), 9);
}

TEST_F(Meta, MetaFuncAsVoid) {
    auto func = entt::resolve<func_type>().func("v"_hs);
    func_type instance{};

    ASSERT_EQ(func.invoke(instance, 42), entt::meta_any{std::in_place_type<void>});
    ASSERT_EQ(func.ret(), entt::resolve<void>());
    ASSERT_EQ(instance.value, 42);
}

TEST_F(Meta, MetaFuncAsAlias) {
    func_type instance{};
    auto func = entt::resolve<func_type>().func("a"_hs);
    func.invoke(instance).cast<int>() = 3;

    ASSERT_EQ(func.ret(), entt::resolve<int>());
    ASSERT_EQ(instance.value, 3);
}

TEST_F(Meta, MetaFuncByReference) {
    auto func = entt::resolve<func_type>().func("h"_hs);
    func_type::value = 2;
    entt::meta_any any{3};
    int value = 4;

    ASSERT_EQ(func.invoke({}, std::ref(value)).cast<int>(), 8);
    ASSERT_EQ(func.invoke({}, *any).cast<int>(), 6);
    ASSERT_EQ(any.cast<int>(), 6);
    ASSERT_EQ(value, 8);
}

TEST_F(Meta, MetaType) {
    auto type = entt::resolve<derived_type>();

    ASSERT_TRUE(type);
    ASSERT_NE(type, entt::meta_type{});
    ASSERT_EQ(type.alias(), "derived"_hs);
    ASSERT_EQ(type.type_id(), entt::type_info<derived_type>::id());

    type.prop([](auto prop) {
        ASSERT_EQ(prop.key(), props::prop_int);
        ASSERT_EQ(prop.value(), 99);
    });

    ASSERT_FALSE(type.prop(props::prop_bool));

    auto prop = type.prop(props::prop_int);

    ASSERT_TRUE(prop);
    ASSERT_EQ(prop.key(), props::prop_int);
    ASSERT_EQ(prop.value(), 99);
}

TEST_F(Meta, MetaTypeTraits) {
    ASSERT_TRUE(entt::resolve<void>().is_void());
    ASSERT_TRUE(entt::resolve<bool>().is_integral());
    ASSERT_TRUE(entt::resolve<double>().is_floating_point());
    ASSERT_TRUE(entt::resolve<props>().is_enum());
    ASSERT_TRUE(entt::resolve<union_type>().is_union());
    ASSERT_TRUE(entt::resolve<derived_type>().is_class());
    ASSERT_TRUE(entt::resolve<int *>().is_pointer());
    ASSERT_TRUE(entt::resolve<decltype(&empty_type::destroy)>().is_function_pointer());
    ASSERT_TRUE(entt::resolve<decltype(&data_type::i)>().is_member_object_pointer());
    ASSERT_TRUE(entt::resolve<decltype(&func_type::g)>().is_member_function_pointer());
}

TEST_F(Meta, MetaTypeRemovePointer) {
    ASSERT_EQ(entt::resolve<void *>().remove_pointer(), entt::resolve<void>());
    ASSERT_EQ(entt::resolve<int(*)(char, double)>().remove_pointer(), entt::resolve<int(char, double)>());
    ASSERT_EQ(entt::resolve<derived_type>().remove_pointer(), entt::resolve<derived_type>());
}

TEST_F(Meta, MetaTypeRemoveExtent) {
    ASSERT_EQ(entt::resolve<int[3]>().remove_extent(), entt::resolve<int>());
    ASSERT_EQ(entt::resolve<int[3][3]>().remove_extent(), entt::resolve<int[3]>());
    ASSERT_EQ(entt::resolve<derived_type>().remove_extent(), entt::resolve<derived_type>());
}

TEST_F(Meta, MetaTypeBase) {
    auto type = entt::resolve<derived_type>();
    bool iterate = false;

    type.base([&iterate](auto base) {
        ASSERT_EQ(base.type(), entt::resolve<base_type>());
        iterate = true;
    });

    ASSERT_TRUE(iterate);
    ASSERT_EQ(type.base("base"_hs).type(), entt::resolve<base_type>());
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

TEST_F(Meta, MetaTypeData) {
    auto type = entt::resolve<data_type>();
    int counter{};

    type.data([&counter](auto) {
        ++counter;
    });

    ASSERT_EQ(counter, 6);
    ASSERT_TRUE(type.data("i"_hs));
}

TEST_F(Meta, MetaTypeFunc) {
    auto type = entt::resolve<func_type>();
    int counter{};

    type.func([&counter](auto) {
        ++counter;
    });

    ASSERT_EQ(counter, 8);
    ASSERT_TRUE(type.func("f1"_hs));
}

TEST_F(Meta, MetaTypeConstruct) {
    auto any = entt::resolve<derived_type>().construct(base_type{}, 42, 'c');

    ASSERT_TRUE(any);
    ASSERT_EQ(any.cast<derived_type>().i, 42);
    ASSERT_EQ(any.cast<derived_type>().c, 'c');
}

TEST_F(Meta, MetaTypeConstructNoArgs) {
    // this should work, no other tests required
    auto any = entt::resolve<empty_type>().construct();

    ASSERT_TRUE(any);
}

TEST_F(Meta, MetaTypeConstructMetaAnyArgs) {
    auto any = entt::resolve<derived_type>().construct(base_type{}, 42, 'c');

    ASSERT_TRUE(any);
    ASSERT_EQ(any.cast<derived_type>().i, 42);
    ASSERT_EQ(any.cast<derived_type>().c, 'c');
}

TEST_F(Meta, MetaTypeConstructInvalidArgs) {
    ASSERT_FALSE(entt::resolve<derived_type>().construct(base_type{}, 'c', 42));
}

TEST_F(Meta, MetaTypeLessArgs) {
    ASSERT_FALSE(entt::resolve<derived_type>().construct(base_type{}));
}

TEST_F(Meta, MetaTypeConstructCastAndConvert) {
    auto any = entt::resolve<derived_type>().construct(derived_type{}, 42., 'c');

    ASSERT_TRUE(any);
    ASSERT_EQ(any.cast<derived_type>().i, 42);
    ASSERT_EQ(any.cast<derived_type>().c, 'c');
}

TEST_F(Meta, MetaTypeDetach) {
    ASSERT_TRUE(entt::resolve("char"_hs));

    entt::resolve([](auto type) {
        if(type.alias() == "char"_hs) {
            type.detach();
        }
    });

    ASSERT_FALSE(entt::resolve("char"_hs));
    ASSERT_EQ(entt::resolve<char>().alias(), "char"_hs);
    ASSERT_EQ(entt::resolve<char>().prop(props::prop_int).value().cast<int>(), 42);
    ASSERT_TRUE(entt::resolve<char>().data("value"_hs));

    entt::meta_factory<char>().alias("char"_hs);

    ASSERT_TRUE(entt::resolve("char"_hs));
}

TEST_F(Meta, MetaDataFromBase) {
    auto type = entt::resolve<concrete_type>();
    concrete_type instance;

    ASSERT_TRUE(type.data("i"_hs));
    ASSERT_TRUE(type.data("j"_hs));

    ASSERT_EQ(instance.i, 0);
    ASSERT_EQ(instance.j, char{});
    ASSERT_TRUE(type.data("i"_hs).set(instance, 3));
    ASSERT_TRUE(type.data("j"_hs).set(instance, 'c'));
    ASSERT_EQ(instance.i, 3);
    ASSERT_EQ(instance.j, 'c');
}

TEST_F(Meta, MetaFuncFromBase) {
    auto type = entt::resolve<concrete_type>();
    auto base = entt::resolve<an_abstract_type>();
    concrete_type instance;

    ASSERT_TRUE(type.func("f"_hs));
    ASSERT_TRUE(type.func("g"_hs));
    ASSERT_TRUE(type.func("h"_hs));

    ASSERT_EQ(type.func("f"_hs).parent(), entt::resolve<concrete_type>());
    ASSERT_EQ(type.func("g"_hs).parent(), entt::resolve<an_abstract_type>());
    ASSERT_EQ(type.func("h"_hs).parent(), entt::resolve<another_abstract_type>());

    ASSERT_EQ(instance.i, 0);
    ASSERT_EQ(instance.j, char{});

    type.func("f"_hs).invoke(instance, 3);
    type.func("h"_hs).invoke(instance, 'c');

    ASSERT_EQ(instance.i, 9);
    ASSERT_EQ(instance.j, 'c');

    base.func("g"_hs).invoke(instance, 3);

    ASSERT_EQ(instance.i, -3);
}

TEST_F(Meta, MetaPropFromBase) {
    auto type = entt::resolve<concrete_type>();
    auto prop_bool = type.prop(props::prop_bool);
    auto prop_int = type.prop(props::prop_int);

    ASSERT_TRUE(prop_bool);
    ASSERT_TRUE(prop_int);

    ASSERT_FALSE(prop_bool.value().cast<bool>());
    ASSERT_EQ(prop_int.value().cast<int>(), 42);
}

TEST_F(Meta, AbstractClass) {
    auto type = entt::resolve<an_abstract_type>();
    concrete_type instance;

    ASSERT_EQ(type.type_id(), entt::type_info<an_abstract_type>::id());
    ASSERT_EQ(instance.i, 0);

    type.func("f"_hs).invoke(instance, 3);

    ASSERT_EQ(instance.i, 3);

    type.func("g"_hs).invoke(instance, 3);

    ASSERT_EQ(instance.i, -3);
}

TEST_F(Meta, EnumAndNamedConstants) {
    auto type = entt::resolve<props>();

    ASSERT_TRUE(type.data("prop_bool"_hs));
    ASSERT_TRUE(type.data("prop_int"_hs));

    ASSERT_EQ(type.data("prop_bool"_hs).type(), type);
    ASSERT_EQ(type.data("prop_int"_hs).type(), type);

    ASSERT_FALSE(type.data("prop_bool"_hs).set({}, props::prop_int));
    ASSERT_FALSE(type.data("prop_int"_hs).set({}, props::prop_bool));

    ASSERT_EQ(type.data("prop_bool"_hs).get({}).cast<props>(), props::prop_bool);
    ASSERT_EQ(type.data("prop_int"_hs).get({}).cast<props>(), props::prop_int);
}

TEST_F(Meta, ArithmeticTypeAndNamedConstants) {
    auto type = entt::resolve<unsigned int>();

    ASSERT_TRUE(type.data("min"_hs));
    ASSERT_TRUE(type.data("max"_hs));

    ASSERT_EQ(type.data("min"_hs).type(), type);
    ASSERT_EQ(type.data("max"_hs).type(), type);

    ASSERT_FALSE(type.data("min"_hs).set({}, 100u));
    ASSERT_FALSE(type.data("max"_hs).set({}, 0u));

    ASSERT_EQ(type.data("min"_hs).get({}).cast<unsigned int>(), 0u);
    ASSERT_EQ(type.data("max"_hs).get({}).cast<unsigned int>(), 100u);
}

TEST_F(Meta, Variables) {
    auto p_data = entt::resolve<props>().data("value"_hs);
    auto c_data = entt::resolve("char"_hs).data("value"_hs);

    props prop{props::prop_int};
    char c = 'c';

    p_data.set(prop, props::prop_bool);
    c_data.set(c, 'x');

    ASSERT_EQ(p_data.get(prop).cast<props>(), props::prop_bool);
    ASSERT_EQ(c_data.get(c).cast<char>(), 'x');
    ASSERT_EQ(prop, props::prop_bool);
    ASSERT_EQ(c, 'x');
}

TEST_F(Meta, PropertiesAndCornerCases) {
    auto type = entt::resolve<props>();

    ASSERT_EQ(type.data("prop_bool"_hs).prop(props::prop_int).value().cast<int>(), 0);
    ASSERT_EQ(type.data("prop_bool"_hs).prop(props::prop_value).value().cast<int>(), 3);

    ASSERT_EQ(type.data("prop_int"_hs).prop(props::prop_bool).value().cast<bool>(), true);
    ASSERT_EQ(type.data("prop_int"_hs).prop(props::prop_int).value().cast<int>(), 0);
    ASSERT_EQ(type.data("prop_int"_hs).prop(props::prop_value).value().cast<int>(), 3);
    ASSERT_TRUE(type.data("prop_int"_hs).prop(props::key_only));
    ASSERT_FALSE(type.data("prop_int"_hs).prop(props::key_only).value());

    ASSERT_EQ(type.data("prop_list"_hs).prop(props::prop_bool).value().cast<bool>(), false);
    ASSERT_EQ(type.data("prop_list"_hs).prop(props::prop_int).value().cast<int>(), 0);
    ASSERT_EQ(type.data("prop_list"_hs).prop(props::prop_value).value().cast<int>(), 3);
    ASSERT_TRUE(type.data("prop_list"_hs).prop(props::key_only));
    ASSERT_FALSE(type.data("prop_list"_hs).prop(props::key_only).value());
}

TEST_F(Meta, Reset) {
    ASSERT_NE(*entt::internal::meta_context::global, nullptr);

    entt::meta<char>().reset();
    entt::meta<concrete_type>().reset();
    entt::meta<setter_getter_type>().reset();
    entt::meta<fat_type>().reset();
    entt::meta<data_type>().reset();
    entt::meta<func_type>().reset();
    entt::meta<array_type>().reset();
    entt::meta<double>().reset();
    entt::meta<props>().reset();
    entt::meta<base_type>().reset();
    entt::meta<derived_type>().reset();
    entt::meta<empty_type>().reset();
    entt::meta<an_abstract_type>().reset();
    entt::meta<another_abstract_type>().reset();
    entt::meta<unsigned int>().reset();

    ASSERT_FALSE(entt::resolve("char"_hs));
    ASSERT_FALSE(entt::resolve("base"_hs));
    ASSERT_FALSE(entt::resolve("derived"_hs));
    ASSERT_FALSE(entt::resolve("empty"_hs));
    ASSERT_FALSE(entt::resolve("fat"_hs));
    ASSERT_FALSE(entt::resolve("data"_hs));
    ASSERT_FALSE(entt::resolve("func"_hs));
    ASSERT_FALSE(entt::resolve("setter_getter"_hs));
    ASSERT_FALSE(entt::resolve("an_abstract_type"_hs));
    ASSERT_FALSE(entt::resolve("another_abstract_type"_hs));
    ASSERT_FALSE(entt::resolve("concrete"_hs));

    ASSERT_EQ(*entt::internal::meta_context::global, nullptr);

    Meta::SetUpAfterUnregistration();
    entt::meta_any any{42.};

    ASSERT_TRUE(any);
    ASSERT_FALSE(any.convert<int>());
    ASSERT_TRUE(any.convert<float>());

    ASSERT_FALSE(entt::resolve("derived"_hs));
    ASSERT_TRUE(entt::resolve("my_type"_hs));

    entt::resolve<derived_type>().prop([](auto prop) {
        ASSERT_EQ(prop.key(), props::prop_bool);
        ASSERT_FALSE(prop.value().template cast<bool>());
    });

    ASSERT_FALSE((entt::resolve<derived_type>().ctor<const base_type &, int, char>()));
    ASSERT_TRUE((entt::resolve<derived_type>().ctor<>()));

    ASSERT_TRUE(entt::resolve("your_type"_hs).data("a_data_member"_hs));
    ASSERT_FALSE(entt::resolve("your_type"_hs).data("another_data_member"_hs));

    ASSERT_TRUE(entt::resolve("your_type"_hs).func("a_member_function"_hs));
    ASSERT_FALSE(entt::resolve("your_type"_hs).func("another_member_function"_hs));
}

TEST_F(Meta, ReRegistrationAfterReset) {
    ASSERT_TRUE(entt::resolve<props>().data("prop_bool"_hs).prop(props::prop_int));
    ASSERT_TRUE(entt::resolve<props>().data("prop_bool"_hs).prop(props::prop_value));

    entt::meta<double>().reset();
    entt::meta<props>().reset();
    entt::meta<derived_type>().reset();
    entt::meta<another_abstract_type>().reset();

    Meta::SetUpAfterUnregistration();

    ASSERT_TRUE(entt::resolve<props>().data("prop_bool"_hs).prop(props::prop_int));
    ASSERT_TRUE(entt::resolve<props>().data("prop_bool"_hs).prop(props::prop_value));
}

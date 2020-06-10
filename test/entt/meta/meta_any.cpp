#include <gtest/gtest.h>
#include <entt/core/hashed_string.hpp>
#include <entt/meta/factory.hpp>
#include <entt/meta/meta.hpp>
#include <entt/meta/resolve.hpp>

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

    bool operator==(const fat_type &other) const {
        return foo == other.foo && bar == other.bar;
    }

    int *foo{nullptr};
    int *bar{nullptr};
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

struct Meta: ::testing::Test {
    static void SetUpTestCase() {
        entt::meta<double>().conv<int>();
        entt::meta<empty_type>().dtor<&empty_type::destroy>();
        entt::meta<fat_type>().base<empty_type>().dtor<&fat_type::destroy>();
    }

    void SetUp() override {
        empty_type::counter = 0;
    }
};

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
    entt::meta_any any{fat_type{}};

    ASSERT_TRUE(any);
    ASSERT_EQ(any.type(), entt::resolve<fat_type>());
    ASSERT_EQ(any.try_cast<void>(), nullptr);
    ASSERT_NE(any.try_cast<empty_type>(), nullptr);
    ASSERT_EQ(any.try_cast<fat_type>(), any.data());
    ASSERT_EQ(std::as_const(any).try_cast<empty_type>(), any.try_cast<empty_type>());
    ASSERT_EQ(std::as_const(any).try_cast<fat_type>(), any.data());
}

TEST_F(Meta, MetaAnyCast) {
    entt::meta_any any{fat_type{}};

    ASSERT_TRUE(any);
    ASSERT_EQ(any.type(), entt::resolve<fat_type>());
    ASSERT_EQ(any.try_cast<std::size_t>(), nullptr);
    ASSERT_NE(any.try_cast<empty_type>(), nullptr);
    ASSERT_EQ(any.try_cast<fat_type>(), any.data());
    ASSERT_EQ(std::as_const(any).try_cast<empty_type>(), any.try_cast<empty_type>());
    ASSERT_EQ(std::as_const(any).try_cast<fat_type>(), any.data());
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

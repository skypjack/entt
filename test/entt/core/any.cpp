#include <algorithm>
#include <gtest/gtest.h>
#include <entt/core/any.hpp>

struct fat {
    double value[4];
    inline static int counter = 0;

    ~fat() { ++counter; }

    bool operator==(const fat &other) const {
        return std::equal(std::begin(value), std::end(value), std::begin(other.value), std::end(other.value));
    }
};

struct empty {
    inline static int counter = 0;
    ~empty() { ++counter; }
};

TEST(Any, SBO) {
    entt::any any{'c'};

    ASSERT_TRUE(any);
    ASSERT_EQ(any.type(), entt::type_id<char>());
    ASSERT_EQ(entt::any_cast<double>(&any), nullptr);
    ASSERT_EQ(entt::any_cast<char>(any), 'c');
}

TEST(Any, NoSBO) {
    fat instance{{.1, .2, .3, .4}};
    entt::any any{instance};

    ASSERT_TRUE(any);
    ASSERT_EQ(any.type(), entt::type_id<fat>());
    ASSERT_EQ(entt::any_cast<double>(&any), nullptr);
    ASSERT_EQ(entt::any_cast<fat>(any), instance);
}

TEST(Any, Empty) {
    entt::any any{};

    ASSERT_FALSE(any);
    ASSERT_FALSE(any.type());
    ASSERT_EQ(entt::any_cast<double>(&any), nullptr);
    ASSERT_EQ(any.data(), nullptr);
}

TEST(Any, SBOInPlaceTypeConstruction) {
    entt::any any{std::in_place_type<int>, 42};

    ASSERT_TRUE(any);
    ASSERT_EQ(any.type(), entt::type_id<int>());
    ASSERT_EQ(entt::any_cast<double>(&any), nullptr);
    ASSERT_EQ(entt::any_cast<int>(any), 42);

    auto other = as_ref(any);

    ASSERT_TRUE(other);
    ASSERT_EQ(other.type(), entt::type_id<int>());
    ASSERT_EQ(entt::any_cast<int>(other), 42);
    ASSERT_EQ(other.data(), any.data());
}

TEST(Any, SBOAsRefConstruction) {
    int value = 42;
    entt::any any{std::ref(value)};

    ASSERT_TRUE(any);
    ASSERT_EQ(any.type(), entt::type_id<int>());
    ASSERT_EQ(entt::any_cast<double>(&any), nullptr);
    ASSERT_EQ(entt::any_cast<int>(any), 42);
    ASSERT_EQ(any.data(), &value);

    auto other = as_ref(any);

    ASSERT_TRUE(other);
    ASSERT_EQ(other.type(), entt::type_id<int>());
    ASSERT_EQ(entt::any_cast<int>(other), 42);
    ASSERT_EQ(other.data(), any.data());
}

TEST(Any, SBOCopyConstruction) {
    entt::any any{42};
    entt::any other{any};

    ASSERT_TRUE(any);
    ASSERT_TRUE(other);
    ASSERT_EQ(any.type(), entt::type_id<int>());
    ASSERT_EQ(other.type(), entt::type_id<int>());
    ASSERT_EQ(entt::any_cast<double>(&other), nullptr);
    ASSERT_EQ(entt::any_cast<int>(other), 42);
}

TEST(Any, SBOCopyAssignment) {
    entt::any any{42};
    entt::any other{3};

    other = any;

    ASSERT_TRUE(any);
    ASSERT_TRUE(other);
    ASSERT_EQ(any.type(), entt::type_id<int>());
    ASSERT_EQ(other.type(), entt::type_id<int>());
    ASSERT_EQ(entt::any_cast<double>(&other), nullptr);
    ASSERT_EQ(entt::any_cast<int>(other), 42);
}

TEST(Any, SBOMoveConstruction) {
    entt::any any{42};
    entt::any other{std::move(any)};

    ASSERT_FALSE(any);
    ASSERT_TRUE(other);
    ASSERT_FALSE(any.type());
    ASSERT_EQ(other.type(), entt::type_id<int>());
    ASSERT_EQ(entt::any_cast<double>(&other), nullptr);
    ASSERT_EQ(entt::any_cast<int>(other), 42);
}

TEST(Any, SBOMoveAssignment) {
    entt::any any{42};
    entt::any other{3};

    other = std::move(any);

    ASSERT_FALSE(any);
    ASSERT_TRUE(other);
    ASSERT_FALSE(any.type());
    ASSERT_EQ(other.type(), entt::type_id<int>());
    ASSERT_EQ(entt::any_cast<double>(&other), nullptr);
    ASSERT_EQ(entt::any_cast<int>(other), 42);
}

TEST(Any, SBODirectAssignment) {
    entt::any any{};
    any = 42;

    ASSERT_TRUE(any);
    ASSERT_EQ(any.type(), entt::type_id<int>());
    ASSERT_EQ(entt::any_cast<double>(&any), nullptr);
    ASSERT_EQ(entt::any_cast<int>(any), 42);
}

TEST(Any, NoSBOInPlaceTypeConstruction) {
    fat instance{{.1, .2, .3, .4}};
    entt::any any{std::in_place_type<fat>, instance};

    ASSERT_TRUE(any);
    ASSERT_EQ(any.type(), entt::type_id<fat>());
    ASSERT_EQ(entt::any_cast<double>(&any), nullptr);
    ASSERT_EQ(entt::any_cast<fat>(any), instance);

    auto other = as_ref(any);

    ASSERT_TRUE(other);
    ASSERT_EQ(other.type(), entt::type_id<fat>());
    ASSERT_EQ(entt::any_cast<fat>(other), (fat{{.1, .2, .3, .4}}));
    ASSERT_EQ(other.data(), any.data());
}

TEST(Any, NoSBOAsRefConstruction) {
    fat instance{{.1, .2, .3, .4}};
    entt::any any{std::ref(instance)};

    ASSERT_TRUE(any);
    ASSERT_EQ(any.type(), entt::type_id<fat>());
    ASSERT_EQ(entt::any_cast<double>(&any), nullptr);
    ASSERT_EQ(entt::any_cast<fat>(any), instance);
    ASSERT_EQ(any.data(), &instance);

    auto other = as_ref(any);

    ASSERT_TRUE(other);
    ASSERT_EQ(other.type(), entt::type_id<fat>());
    ASSERT_EQ(entt::any_cast<fat>(other), (fat{{.1, .2, .3, .4}}));
    ASSERT_EQ(other.data(), any.data());
}

TEST(Any, NoSBOCopyConstruction) {
    fat instance{{.1, .2, .3, .4}};
    entt::any any{instance};
    entt::any other{any};

    ASSERT_TRUE(any);
    ASSERT_TRUE(other);
    ASSERT_EQ(any.type(), entt::type_id<fat>());
    ASSERT_EQ(other.type(), entt::type_id<fat>());
    ASSERT_EQ(entt::any_cast<double>(&other), nullptr);
    ASSERT_EQ(entt::any_cast<fat>(other), instance);
}

TEST(Any, NoSBOCopyAssignment) {
    fat instance{{.1, .2, .3, .4}};
    entt::any any{instance};
    entt::any other{3};

    other = any;

    ASSERT_TRUE(any);
    ASSERT_TRUE(other);
    ASSERT_EQ(any.type(), entt::type_id<fat>());
    ASSERT_EQ(other.type(), entt::type_id<fat>());
    ASSERT_EQ(entt::any_cast<double>(&other), nullptr);
    ASSERT_EQ(entt::any_cast<fat>(other), instance);
}

TEST(Any, NoSBOMoveConstruction) {
    fat instance{{.1, .2, .3, .4}};
    entt::any any{instance};
    entt::any other{std::move(any)};

    ASSERT_FALSE(any);
    ASSERT_TRUE(other);
    ASSERT_FALSE(any.type());
    ASSERT_EQ(other.type(), entt::type_id<fat>());
    ASSERT_EQ(entt::any_cast<double>(&other), nullptr);
    ASSERT_EQ(entt::any_cast<fat>(other), instance);
}

TEST(Any, NoSBOMoveAssignment) {
    fat instance{{.1, .2, .3, .4}};
    entt::any any{instance};
    entt::any other{3};

    other = std::move(any);

    ASSERT_FALSE(any);
    ASSERT_TRUE(other);
    ASSERT_FALSE(any.type());
    ASSERT_EQ(other.type(), entt::type_id<fat>());
    ASSERT_EQ(entt::any_cast<double>(&other), nullptr);
    ASSERT_EQ(entt::any_cast<fat>(other), instance);
}

TEST(Any, NoSBODirectAssignment) {
    fat instance{{.1, .2, .3, .4}};
    entt::any any{};
    any = instance;

    ASSERT_TRUE(any);
    ASSERT_EQ(any.type(), entt::type_id<fat>());
    ASSERT_EQ(entt::any_cast<double>(&any), nullptr);
    ASSERT_EQ(entt::any_cast<fat>(any), instance);
}

TEST(Any, VoidInPlaceTypeConstruction) {
    entt::any any{std::in_place_type<void>};

    ASSERT_FALSE(any);
    ASSERT_FALSE(any.type());
    ASSERT_EQ(entt::any_cast<int>(&any), nullptr);
}

TEST(Any, VoidCopyConstruction) {
    entt::any any{std::in_place_type<void>};
    entt::any other{any};

    ASSERT_FALSE(any);
    ASSERT_FALSE(other);
    ASSERT_FALSE(any.type());
    ASSERT_FALSE(other.type());
    ASSERT_EQ(entt::any_cast<int>(&any), nullptr);
    ASSERT_EQ(entt::any_cast<double>(&other), nullptr);
}

TEST(Any, VoidCopyAssignment) {
    entt::any any{std::in_place_type<void>};
    entt::any other{std::in_place_type<void>};

    other = any;

    ASSERT_FALSE(any);
    ASSERT_FALSE(other);
    ASSERT_FALSE(any.type());
    ASSERT_FALSE(other.type());
    ASSERT_EQ(entt::any_cast<int>(&any), nullptr);
    ASSERT_EQ(entt::any_cast<double>(&other), nullptr);
}

TEST(Any, VoidMoveConstruction) {
    entt::any any{std::in_place_type<void>};
    entt::any other{std::move(any)};

    ASSERT_FALSE(any);
    ASSERT_FALSE(other);
    ASSERT_FALSE(any.type());
    ASSERT_FALSE(other.type());
    ASSERT_EQ(entt::any_cast<int>(&any), nullptr);
    ASSERT_EQ(entt::any_cast<double>(&other), nullptr);
}

TEST(Any, VoidMoveAssignment) {
    entt::any any{std::in_place_type<void>};
    entt::any other{std::in_place_type<void>};

    other = std::move(any);

    ASSERT_FALSE(any);
    ASSERT_FALSE(other);
    ASSERT_FALSE(any.type());
    ASSERT_FALSE(other.type());
    ASSERT_EQ(entt::any_cast<int>(&any), nullptr);
    ASSERT_EQ(entt::any_cast<double>(&other), nullptr);
}

TEST(Any, SBOMoveInvalidate) {
    entt::any any{42};
    entt::any other{std::move(any)};
    entt::any valid = std::move(other);

    ASSERT_FALSE(any);
    ASSERT_FALSE(other);
    ASSERT_TRUE(valid);
}

TEST(Any, NoSBOMoveInvalidate) {
    fat instance{{.1, .2, .3, .4}};
    entt::any any{instance};
    entt::any other{std::move(any)};
    entt::any valid = std::move(other);

    ASSERT_FALSE(any);
    ASSERT_FALSE(other);
    ASSERT_TRUE(valid);
}

TEST(Any, VoidMoveInvalidate) {
    entt::any any{std::in_place_type<void>};
    entt::any other{std::move(any)};
    entt::any valid = std::move(other);

    ASSERT_FALSE(any);
    ASSERT_FALSE(other);
    ASSERT_FALSE(valid);
}

TEST(Any, SBODestruction) {
    {
        entt::any any{empty{}};
        empty::counter = 0;
    }

    ASSERT_EQ(empty::counter, 1);
}

TEST(Any, NoSBODestruction) {
    {
        entt::any any{fat{}};
        fat::counter = 0;
    }

    ASSERT_EQ(fat::counter, 1);
}

TEST(Any, VoidDestruction) {
    // just let asan tell us if everything is ok here
    [[maybe_unused]] entt::any any{std::in_place_type<void>};
}

TEST(Any, Emplace) {
    entt::any any{};
    any.emplace<int>(42);

    ASSERT_TRUE(any);
    ASSERT_EQ(any.type(), entt::type_id<int>());
    ASSERT_EQ(entt::any_cast<double>(&any), nullptr);
    ASSERT_EQ(entt::any_cast<int>(any), 42);
}

TEST(Any, EmplaceVoid) {
    entt::any any{};
    any.emplace<void>();

    ASSERT_FALSE(any);
    ASSERT_FALSE(any.type());
 }

TEST(Any, SBOSwap) {
    entt::any lhs{'c'};
    entt::any rhs{42};

    std::swap(lhs, rhs);

    ASSERT_EQ(lhs.type(), entt::type_id<int>());
    ASSERT_EQ(rhs.type(), entt::type_id<char>());
    ASSERT_EQ(entt::any_cast<char>(&lhs), nullptr);
    ASSERT_EQ(entt::any_cast<int>(&rhs), nullptr);
    ASSERT_EQ(entt::any_cast<int>(lhs), 42);
    ASSERT_EQ(entt::any_cast<char>(rhs), 'c');
}

TEST(Any, NoSBOSwap) {
    entt::any lhs{fat{{.1, .2, .3, .4}}};
    entt::any rhs{fat{{.4, .3, .2, .1}}};

    std::swap(lhs, rhs);

    ASSERT_EQ(entt::any_cast<fat>(lhs), (fat{{.4, .3, .2, .1}}));
    ASSERT_EQ(entt::any_cast<fat>(rhs), (fat{{.1, .2, .3, .4}}));
}

TEST(Any, VoidSwap) {
    entt::any lhs{std::in_place_type<void>};
    entt::any rhs{std::in_place_type<void>};
    const auto *pre = lhs.data();

    std::swap(lhs, rhs);

    ASSERT_EQ(pre, lhs.data());
}

TEST(Any, SBOWithNoSBOSwap) {
    entt::any lhs{fat{{.1, .2, .3, .4}}};
    entt::any rhs{'c'};

    std::swap(lhs, rhs);

    ASSERT_EQ(lhs.type(), entt::type_id<char>());
    ASSERT_EQ(rhs.type(), entt::type_id<fat>());
    ASSERT_EQ(entt::any_cast<fat>(&lhs), nullptr);
    ASSERT_EQ(entt::any_cast<char>(&rhs), nullptr);
    ASSERT_EQ(entt::any_cast<char>(lhs), 'c');
    ASSERT_EQ(entt::any_cast<fat>(rhs), (fat{{.1, .2, .3, .4}}));
}

TEST(Any, SBOWithRefSwap) {
    int value = 3;
    entt::any lhs{std::ref(value)};
    entt::any rhs{'c'};

    std::swap(lhs, rhs);

    ASSERT_EQ(lhs.type(), entt::type_id<char>());
    ASSERT_EQ(rhs.type(), entt::type_id<int>());
    ASSERT_EQ(entt::any_cast<int>(&lhs), nullptr);
    ASSERT_EQ(entt::any_cast<char>(&rhs), nullptr);
    ASSERT_EQ(entt::any_cast<char>(lhs), 'c');
    ASSERT_EQ(entt::any_cast<int>(rhs), 3);
    ASSERT_EQ(rhs.data(), &value);
}

TEST(Any, SBOWithEmptySwap) {
    entt::any lhs{'c'};
    entt::any rhs{};

    std::swap(lhs, rhs);

    ASSERT_FALSE(lhs);
    ASSERT_EQ(rhs.type(), entt::type_id<char>());
    ASSERT_EQ(entt::any_cast<char>(&lhs), nullptr);
    ASSERT_EQ(entt::any_cast<double>(&rhs), nullptr);
    ASSERT_EQ(entt::any_cast<char>(rhs), 'c');

    std::swap(lhs, rhs);

    ASSERT_FALSE(rhs);
    ASSERT_EQ(lhs.type(), entt::type_id<char>());
    ASSERT_EQ(entt::any_cast<double>(&lhs), nullptr);
    ASSERT_EQ(entt::any_cast<char>(&rhs), nullptr);
    ASSERT_EQ(entt::any_cast<char>(lhs), 'c');
}

TEST(Any, SBOWithVoidSwap) {
    entt::any lhs{'c'};
    entt::any rhs{std::in_place_type<void>};

    std::swap(lhs, rhs);

    ASSERT_FALSE(lhs);
    ASSERT_EQ(rhs.type(), entt::type_id<char>());
    ASSERT_EQ(entt::any_cast<char>(&lhs), nullptr);
    ASSERT_EQ(entt::any_cast<double>(&rhs), nullptr);
    ASSERT_EQ(entt::any_cast<char>(rhs), 'c');

    std::swap(lhs, rhs);

    ASSERT_FALSE(rhs);
    ASSERT_EQ(lhs.type(), entt::type_id<char>());
    ASSERT_EQ(entt::any_cast<double>(&lhs), nullptr);
    ASSERT_EQ(entt::any_cast<char>(&rhs), nullptr);
    ASSERT_EQ(entt::any_cast<char>(lhs), 'c');
}

TEST(Any, NoSBOWithRefSwap) {
    int value = 3;
    entt::any lhs{std::ref(value)};
    entt::any rhs{fat{{.1, .2, .3, .4}}};

    std::swap(lhs, rhs);

    ASSERT_EQ(lhs.type(), entt::type_id<fat>());
    ASSERT_EQ(rhs.type(), entt::type_id<int>());
    ASSERT_EQ(entt::any_cast<int>(&lhs), nullptr);
    ASSERT_EQ(entt::any_cast<fat>(&rhs), nullptr);
    ASSERT_EQ(entt::any_cast<fat>(lhs), (fat{{.1, .2, .3, .4}}));
    ASSERT_EQ(entt::any_cast<int>(rhs), 3);
    ASSERT_EQ(rhs.data(), &value);
}

TEST(Any, NoSBOWithEmptySwap) {
    entt::any lhs{fat{{.1, .2, .3, .4}}};
    entt::any rhs{};

    std::swap(lhs, rhs);

    ASSERT_FALSE(lhs);
    ASSERT_EQ(rhs.type(), entt::type_id<fat>());
    ASSERT_EQ(entt::any_cast<fat>(&lhs), nullptr);
    ASSERT_EQ(entt::any_cast<double>(&rhs), nullptr);
    ASSERT_EQ(entt::any_cast<fat>(rhs), (fat{{.1, .2, .3, .4}}));

    std::swap(lhs, rhs);

    ASSERT_FALSE(rhs);
    ASSERT_EQ(lhs.type(), entt::type_id<fat>());
    ASSERT_EQ(entt::any_cast<double>(&lhs), nullptr);
    ASSERT_EQ(entt::any_cast<fat>(&rhs), nullptr);
    ASSERT_EQ(entt::any_cast<fat>(lhs), (fat{{.1, .2, .3, .4}}));
}

TEST(Any, NoSBOWithVoidSwap) {
    entt::any lhs{fat{{.1, .2, .3, .4}}};
    entt::any rhs{std::in_place_type<void>};

    std::swap(lhs, rhs);

    ASSERT_FALSE(lhs);
    ASSERT_EQ(rhs.type(), entt::type_id<fat>());
    ASSERT_EQ(entt::any_cast<fat>(&lhs), nullptr);
    ASSERT_EQ(entt::any_cast<double>(&rhs), nullptr);
    ASSERT_EQ(entt::any_cast<fat>(rhs), (fat{{.1, .2, .3, .4}}));

    std::swap(lhs, rhs);

    ASSERT_FALSE(rhs);
    ASSERT_EQ(lhs.type(), entt::type_id<fat>());
    ASSERT_EQ(entt::any_cast<double>(&lhs), nullptr);
    ASSERT_EQ(entt::any_cast<fat>(&rhs), nullptr);
    ASSERT_EQ(entt::any_cast<fat>(lhs), (fat{{.1, .2, .3, .4}}));
}

TEST(Any, AnyCast) {
    entt::any any{42};
    const auto &cany = any;

    ASSERT_EQ(entt::any_cast<char>(&any), nullptr);
    ASSERT_EQ(entt::any_cast<char>(&cany), nullptr);
    ASSERT_EQ(*entt::any_cast<int>(&any), 42);
    ASSERT_EQ(*entt::any_cast<int>(&cany), 42);
    ASSERT_EQ(entt::any_cast<int &>(any), 42);
    ASSERT_EQ(entt::any_cast<const int &>(cany), 42);
    ASSERT_EQ(entt::any_cast<int>(42), 42);
}
